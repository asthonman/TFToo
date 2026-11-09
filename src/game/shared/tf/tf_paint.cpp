//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Game-side replacement for Emulsion's engine paintmap and blob stream.
// Stamps are a networked log (80 events). The coat itself is a luxel atlas,
// one chart per plane, shaded with the Grimes GDC 2011 surface shader into
// vertex colors on tftoo/paint/gel. Portal 2's lightmappedpaint shader is
// not used; TF2 cannot link it. Blobs and pops still use the soft discs.
//
//=============================================================================
#include "cbase.h"
#include "tf_paint.h"
#include "tf_paint_atlas.h"
#include "engine/IEngineTrace.h"
#include "igamesystem.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "model_types.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#define CTFPaintMap C_TFPaintMap
#else
#include "tf_player.h"
#include "recipientfilter.h"
#endif

#include "tier0/memdbgon.h"

ConVar tf_paint_enabled( "tf_paint_enabled", "1", FCVAR_REPLICATED, "Mobility gels affect movement and can be fired." );
ConVar tf_paint_trace_radius( "tf_paint_trace_radius", "26", FCVAR_REPLICATED, "Extra reach when testing if a player is standing in gel." );
ConVar tf_paint_speed( "tf_paint_speed", "600", FCVAR_REPLICATED, "Max ground speed on propulsion gel." );
ConVar tf_paint_speed_accel( "tf_paint_speed_accel", "5", FCVAR_REPLICATED, "Ground acceleration on propulsion gel." );
ConVar tf_paint_speed_friction( "tf_paint_speed_friction", "0.85", FCVAR_REPLICATED, "Friction scale on propulsion gel." );
ConVar tf_paint_stick_speed( "tf_paint_stick_speed", "150", FCVAR_REPLICATED, "Move speed on adhesion gel." );
ConVar tf_paint_bounce_min( "tf_paint_bounce_min", "500", FCVAR_REPLICATED, "Minimum launch speed off repulsion gel." );
ConVar tf_paint_bounce_wall( "tf_paint_bounce_wall", "150", FCVAR_REPLICATED, "Minimum launch speed off a repulsion-gel wall." );
ConVar tf_paint_splat_radius( "tf_paint_splat_radius", "48", FCVAR_REPLICATED, "Radius of a blob splat. Emulsion uses 48." );

static const char *const s_pszPaintSounds[] =
{
	"Player.EnterBouncePaint",
	"Player.ExitBouncePaint",
	"Player.EnterSpeedPaint",
	"Player.ExitSpeedPaint",
	"Player.EnterStickPaint",
	"Player.ExitStickPaint",
	"Player.JumpPowerUse",
	"Paintgun.Fire",
	"Paint.Splat",
};

void TFPaint_GetColor( int type, int &r, int &g, int &b )
{
	switch ( type )
	{
	case BOUNCE_POWER:	r = 0;   g = 165; b = 255; break;
	case SPEED_POWER:	r = 255; g = 106; b = 0;   break;
	case PORTAL_POWER:	r = 0;   g = 200; b = 33;  break;
	case FIFTH_POWER:	r = 251; g = 255; b = 0;   break;
	case NO_POWER:		r = 16;  g = 16;  b = 16;  break;
	default:			r = 255; g = 255; b = 255; break;
	}
}

const char *TFPaint_Name( int type )
{
	switch ( type )
	{
	case BOUNCE_POWER:	return "Repulsion gel";
	case SPEED_POWER:	return "Propulsion gel";
	case PORTAL_POWER:	return "Adhesion gel";
	case FIFTH_POWER:	return "Yellow gel";
	case NO_POWER:		return "Cleansing gel";
	default:			return "Gel";
	}
}

void TFPaint_PlaySound( CBaseEntity *pEnt, const char *pszSound )
{
#ifdef GAME_DLL
	if ( pEnt && pszSound && pszSound[0] )
		pEnt->EmitSound( pszSound );
#else
	(void)pEnt;
	(void)pszSound;
#endif
}

//-----------------------------------------------------------------------------
class CTraceFilterNoPlayers : public CTraceFilterSimple
{
public:
	CTraceFilterNoPlayers( const IHandleEntity *pass )
		: CTraceFilterSimple( pass, COLLISION_GROUP_NONE ) {}

	virtual bool ShouldHitEntity( IHandleEntity *pHandle, int contentsMask )
	{
		CBaseEntity *pEnt = EntityFromEntityHandle( pHandle );
		if ( pEnt && pEnt->IsPlayer() )
			return false;
		return BaseClass::ShouldHitEntity( pHandle, contentsMask );
	}
};

//-----------------------------------------------------------------------------
class CTFPaintMap : public CBaseEntity
{
	DECLARE_CLASS( CTFPaintMap, CBaseEntity );
	DECLARE_NETWORKCLASS();

public:
	CTFPaintMap();
	~CTFPaintMap();

	virtual void Spawn( void );
#ifdef GAME_DLL
	virtual int UpdateTransmitState( void );
#endif

#ifdef CLIENT_DLL
	virtual int DrawModel( int flags );
	virtual void ClientThink( void );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual RenderGroup_t GetRenderGroup( void ) { return RENDER_GROUP_TRANSLUCENT_ENTITY; }
#else
	virtual void Think( void );
	void PaintSphere( const Vector &pos, const Vector &normal, int type, float radius );
	void EraseSphere( const Vector &pos, float radius );
	void AddBlob( const Vector &pos, const Vector &vel, int type );
#endif

	TFPaintContact_t Query( const Vector &origin, float extra );

	static CTFPaintMap *Get() { return s_pMap; }

private:
	static CTFPaintMap *s_pMap;

	CNetworkArray( Vector, m_vecStampPos, TF_PAINT_STAMP_MAX );
	CNetworkArray( Vector, m_vecStampNormal, TF_PAINT_STAMP_MAX );
	CNetworkArray( int, m_nStampType, TF_PAINT_STAMP_MAX );
	CNetworkArray( float, m_flStampRadius, TF_PAINT_STAMP_MAX );
	CNetworkVar( int, m_nStampCursor );

	CNetworkArray( Vector, m_vecBlobPos, TF_PAINT_BLOB_MAX );
	CNetworkArray( int, m_nBlobType, TF_PAINT_BLOB_MAX );

	CTFPaintAtlas m_Atlas;

#ifndef CLIENT_DLL
	Vector	m_vecBlobVel[TF_PAINT_BLOB_MAX];
	float	m_flBlobDie[TF_PAINT_BLOB_MAX];
	int		m_iBlobCursor;
	float	m_flNextSplatSound;
	void	PushStamp( const Vector &pos, const Vector &normal, int type, float radius );
#endif

#ifdef CLIENT_DLL
	int		m_nAppliedCursor;
	Vector	m_vecBlobSeen[TF_PAINT_BLOB_MAX];
	int		m_nBlobSeen[TF_PAINT_BLOB_MAX];
	struct Pop_t
	{
		Vector pos;
		Vector normal;
		int type;
		float time;
	};
	Pop_t	m_Pops[16];
	int		m_iPop;
	bool	m_bHaveSeen;
	void	ApplyNetworkStamps( void );
	void	NotePop( const Vector &pos, const Vector &normal, int type );
	void	DrawDisc( const Vector &pos, const Vector &normal, float radius, int type, int alpha, bool additive );
#endif
};

CTFPaintMap *CTFPaintMap::s_pMap = NULL;

IMPLEMENT_NETWORKCLASS_ALIASED( TFPaintMap, DT_TFPaintMap )

BEGIN_NETWORK_TABLE( CTFPaintMap, DT_TFPaintMap )
#ifdef CLIENT_DLL
	RecvPropArray( RecvPropVector( RECVINFO( m_vecStampPos[0] ) ), m_vecStampPos ),
	RecvPropArray( RecvPropVector( RECVINFO( m_vecStampNormal[0] ) ), m_vecStampNormal ),
	RecvPropArray( RecvPropInt( RECVINFO( m_nStampType[0] ) ), m_nStampType ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flStampRadius[0] ) ), m_flStampRadius ),
	RecvPropInt( RECVINFO( m_nStampCursor ) ),
	RecvPropArray( RecvPropVector( RECVINFO( m_vecBlobPos[0] ) ), m_vecBlobPos ),
	RecvPropArray( RecvPropInt( RECVINFO( m_nBlobType[0] ) ), m_nBlobType ),
#else
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vecStampPos ), -1, SPROP_COORD ), m_vecStampPos ),
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vecStampNormal ), -1, SPROP_NOSCALE ), m_vecStampNormal ),
	SendPropArray3( SENDINFO_ARRAY3( m_nStampType ), SendPropInt( SENDINFO_ARRAY( m_nStampType ), 4, SPROP_UNSIGNED ) ),
	SendPropArray( SendPropFloat( SENDINFO_ARRAY( m_flStampRadius ), 10, SPROP_ROUNDDOWN, 0.0f, 256.0f ), m_flStampRadius ),
	SendPropInt( SENDINFO( m_nStampCursor ), 32, SPROP_UNSIGNED ),
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vecBlobPos ), -1, SPROP_COORD ), m_vecBlobPos ),
	SendPropArray3( SENDINFO_ARRAY3( m_nBlobType ), SendPropInt( SENDINFO_ARRAY( m_nBlobType ), 4, SPROP_UNSIGNED ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_paint_map, CTFPaintMap );

CTFPaintMap::CTFPaintMap()
{
	s_pMap = this;
#ifdef CLIENT_DLL
	m_iPop = 0;
	m_bHaveSeen = false;
	m_nAppliedCursor = 0;
	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		m_nBlobSeen[i] = 0;
		m_vecBlobSeen[i].Init();
	}
	for ( int i = 0; i < 16; ++i )
	{
		m_Pops[i].type = 0;
		m_Pops[i].time = 0.f;
	}
#else
	m_iBlobCursor = 0;
	m_flNextSplatSound = 0.f;
	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		m_vecBlobVel[i].Init();
		m_flBlobDie[i] = 0.f;
	}
#endif
}

CTFPaintMap::~CTFPaintMap()
{
	if ( s_pMap == this )
		s_pMap = NULL;
}

void CTFPaintMap::Spawn( void )
{
	BaseClass::Spawn();

	PrecacheModel( "models/weapons/w_models/w_bat.mdl" );
	SetModel( "models/weapons/w_models/w_bat.mdl" );

	for ( int i = 0; i < ARRAYSIZE( s_pszPaintSounds ); ++i )
		PrecacheScriptSound( s_pszPaintSounds[i] );

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetSize( vec3_origin, vec3_origin );
	AddEffects( EF_NOSHADOW | EF_NORECEIVESHADOW );
	SetRenderMode( kRenderTransAlpha );
	SetRenderColor( 255, 255, 255, 255 );
	SetAbsOrigin( vec3_origin );
	m_Atlas.Clear();

#ifndef CLIENT_DLL
	m_nStampCursor = 0;
	for ( int i = 0; i < TF_PAINT_STAMP_MAX; ++i )
	{
		m_vecStampPos.Set( i, vec3_origin );
		m_vecStampNormal.Set( i, Vector( 0, 0, 1 ) );
		m_nStampType.Set( i, 0 );
		m_flStampRadius.Set( i, 0.f );
	}
	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		m_vecBlobPos.Set( i, vec3_origin );
		m_nBlobType.Set( i, 0 );
	}
	SetNextThink( gpGlobals->curtime + 0.05f );
#else
	m_nAppliedCursor = 0;
	SetRenderBounds( Vector( -MAX_COORD_FLOAT, -MAX_COORD_FLOAT, -MAX_COORD_FLOAT ),
		Vector( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT ) );
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif

	s_pMap = this;
}

#ifndef CLIENT_DLL
int CTFPaintMap::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

TFPaintContact_t CTFPaintMap::Query( const Vector &origin, float extra )
{
	if ( !tf_paint_enabled.GetBool() )
		return TFPaintContact_t();

#ifdef CLIENT_DLL
	// Prediction asks before the render think. Catch the log up first so
	// movement and the coat read the same charts. Pops are recorded once,
	// on the apply that consumes the cursor.
	ApplyNetworkStamps();
#endif
	return m_Atlas.Query( origin, extra );
}

#ifdef CLIENT_DLL

void CTFPaintMap::NotePop( const Vector &pos, const Vector &normal, int type )
{
	Pop_t &pop = m_Pops[m_iPop % 16];
	pop.pos = pos;
	pop.normal = normal;
	pop.type = type;
	pop.time = gpGlobals->curtime;
	++m_iPop;
}

void CTFPaintMap::ApplyNetworkStamps( void )
{
	int cur = m_nStampCursor;
	int applied = m_nAppliedCursor;
	if ( cur < applied )
	{
		m_Atlas.Clear();
		applied = 0;
	}

	int behind = cur - applied;
	bool bEffects = ( behind >= 1 && behind <= 3 );
	if ( behind > TF_PAINT_STAMP_MAX )
	{
		// Late join, or a gap bigger than the log. Older gel is gone.
		m_Atlas.Clear();
		applied = cur - TF_PAINT_STAMP_MAX;
		bEffects = false;
	}

	for ( int s = applied; s < cur; ++s )
	{
		int slot = s % TF_PAINT_STAMP_MAX;
		int stored = m_nStampType[slot];
		if ( stored <= 0 )
			continue;
		int type = stored - 1;
		float radius = m_flStampRadius[slot];
		Vector pos = m_vecStampPos[slot];
		Vector n = m_vecStampNormal[slot];
		if ( type == NO_POWER || n.LengthSqr() < 0.01f )
		{
			m_Atlas.Erase( pos, radius );
			continue;
		}
		m_Atlas.Stamp( pos, n, type, radius );
		if ( bEffects )
			NotePop( pos, n, type );
	}
	m_nAppliedCursor = cur;
}

void CTFPaintMap::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );
	SetRenderBounds( Vector( -MAX_COORD_FLOAT, -MAX_COORD_FLOAT, -MAX_COORD_FLOAT ),
		Vector( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT ) );
	UpdateVisibility();
	ApplyNetworkStamps();

	if ( !m_bHaveSeen )
	{
		for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
		{
			m_nBlobSeen[i] = m_nBlobType[i];
			m_vecBlobSeen[i] = m_vecBlobPos[i];
		}
		m_bHaveSeen = true;
		return;
	}

	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		if ( m_nBlobSeen[i] > 0 && m_nBlobType[i] <= 0 )
			NotePop( m_vecBlobSeen[i], Vector( 0, 0, 1 ), m_nBlobSeen[i] - 1 );
		m_nBlobSeen[i] = m_nBlobType[i];
		m_vecBlobSeen[i] = m_vecBlobPos[i];
	}
}

void CTFPaintMap::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	s_pMap = this;
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_nAppliedCursor = 0;
		m_Atlas.Clear();
		SetRenderBounds( Vector( -MAX_COORD_FLOAT, -MAX_COORD_FLOAT, -MAX_COORD_FLOAT ),
			Vector( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT ) );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

void CTFPaintMap::DrawDisc( const Vector &pos, const Vector &normal, float radius, int type, int alpha, bool additive )
{
	if ( radius < 1.f || alpha <= 0 )
		return;

	static IMaterial *s_pSplat = NULL;
	static IMaterial *s_pBlob = NULL;
	if ( !s_pSplat )
	{
		s_pSplat = materials->FindMaterial( "tftoo/paint/splat", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		s_pBlob = materials->FindMaterial( "tftoo/paint/blob", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	}
	IMaterial *pMat = additive ? s_pBlob : s_pSplat;
	if ( !pMat || pMat->IsErrorMaterial() )
		return;

	Vector n = normal;
	if ( n.LengthSqr() < 0.01f )
		n.Init( 0, 0, 1 );
	VectorNormalize( n );

	Vector tangent;
	if ( fabsf( n.z ) > 0.9f )
		tangent = CrossProduct( Vector( 0, 1, 0 ), n );
	else
		tangent = CrossProduct( Vector( 0, 0, 1 ), n );
	VectorNormalize( tangent );
	Vector bitangent = CrossProduct( n, tangent );

	int r, g, b;
	TFPaint_GetColor( type, r, g, b );

	const int segs = 8;
	const float kTau = 6.2831853f;
	Vector center = pos + n * 0.6f;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( pMat );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMat );
	CMeshBuilder mb;
	mb.Begin( pMesh, MATERIAL_TRIANGLES, segs );

	for ( int s = 0; s < segs; ++s )
	{
		float a0 = (float)s / (float)segs * kTau;
		float a1 = (float)( s + 1 ) / (float)segs * kTau;
		Vector p0 = center + tangent * ( cosf( a0 ) * radius ) + bitangent * ( sinf( a0 ) * radius );
		Vector p1 = center + tangent * ( cosf( a1 ) * radius ) + bitangent * ( sinf( a1 ) * radius );

		mb.Position3fv( center.Base() );
		mb.Color4ub( r, g, b, alpha );
		mb.TexCoord2f( 0, 0.5f, 0.5f );
		mb.AdvanceVertex();

		mb.Position3fv( p0.Base() );
		mb.Color4ub( r, g, b, 0 );
		mb.TexCoord2f( 0, 0.5f + cosf( a0 ) * 0.5f, 0.5f + sinf( a0 ) * 0.5f );
		mb.AdvanceVertex();

		mb.Position3fv( p1.Base() );
		mb.Color4ub( r, g, b, 0 );
		mb.TexCoord2f( 0, 0.5f + cosf( a1 ) * 0.5f, 0.5f + sinf( a1 ) * 0.5f );
		mb.AdvanceVertex();
	}

	mb.End();
	pMesh->Draw();
}

int CTFPaintMap::DrawModel( int flags )
{
	if ( flags & STUDIO_SHADOWDEPTHTEXTURE )
		return 0;
	if ( !tf_paint_enabled.GetBool() )
		return 0;

	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	Vector eye = GetAbsOrigin();
	Vector camRight( 1, 0, 0 ), camUp( 0, 0, 1 );
	if ( pLocal )
	{
		eye = pLocal->EyePosition();
		AngleVectors( pLocal->EyeAngles(), NULL, &camRight, &camUp );
	}

	ApplyNetworkStamps();
	m_Atlas.Draw( eye );

	static IMaterial *s_pBlob = NULL;
	if ( !s_pBlob )
		s_pBlob = materials->FindMaterial( "tftoo/paint/blob", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	IMaterial *pBlob = s_pBlob;
	if ( pBlob && !pBlob->IsErrorMaterial() )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( pBlob );
		for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
		{
			if ( m_nBlobType[i] <= 0 )
				continue;
			int r, g, b;
			TFPaint_GetColor( m_nBlobType[i] - 1, r, g, b );
			const float rad = 7.f;
			Vector c = m_vecBlobPos[i];
			Vector p0 = c - camRight * rad - camUp * rad;
			Vector p1 = c + camRight * rad - camUp * rad;
			Vector p2 = c + camRight * rad + camUp * rad;
			Vector p3 = c - camRight * rad + camUp * rad;

			IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pBlob );
			CMeshBuilder mb;
			mb.Begin( pMesh, MATERIAL_TRIANGLES, 2 );
			mb.Position3fv( p0.Base() ); mb.Color4ub( r, g, b, 40 ); mb.TexCoord2f( 0, 0, 1 ); mb.AdvanceVertex();
			mb.Position3fv( p1.Base() ); mb.Color4ub( r, g, b, 40 ); mb.TexCoord2f( 0, 1, 1 ); mb.AdvanceVertex();
			mb.Position3fv( p2.Base() ); mb.Color4ub( r, g, b, 220 ); mb.TexCoord2f( 0, 1, 0 ); mb.AdvanceVertex();
			mb.Position3fv( p0.Base() ); mb.Color4ub( r, g, b, 40 ); mb.TexCoord2f( 0, 0, 1 ); mb.AdvanceVertex();
			mb.Position3fv( p2.Base() ); mb.Color4ub( r, g, b, 220 ); mb.TexCoord2f( 0, 1, 0 ); mb.AdvanceVertex();
			mb.Position3fv( p3.Base() ); mb.Color4ub( r, g, b, 40 ); mb.TexCoord2f( 0, 0, 0 ); mb.AdvanceVertex();
			mb.End();
			pMesh->Draw();
		}
	}

	for ( int i = 0; i < 16; ++i )
	{
		float age = gpGlobals->curtime - m_Pops[i].time;
		if ( m_Pops[i].time <= 0.f || age < 0.f || age > 0.35f )
			continue;
		float u = age / 0.35f;
		float rad = 10.f + u * 36.f;
		int alpha = (int)( 180.f * ( 1.f - u ) );
		DrawDisc( m_Pops[i].pos, m_Pops[i].normal, rad, m_Pops[i].type, alpha, true );
	}

	return 1;
}

#else // GAME_DLL

void CTFPaintMap::PaintSphere( const Vector &pos, const Vector &normal, int type, float radius )
{
	if ( !tf_paint_enabled.GetBool() )
		return;
	if ( type == NO_POWER )
	{
		EraseSphere( pos, radius );
		return;
	}
	if ( radius < 4.f )
		radius = 4.f;
	if ( radius > 250.f )
		radius = 250.f;

	Vector n = normal;
	if ( n.LengthSqr() < 0.01f )
		n.Init( 0, 0, 1 );
	VectorNormalize( n );

	m_Atlas.Stamp( pos, n, type, radius );
	PushStamp( pos, n, type, radius );
}

void CTFPaintMap::PushStamp( const Vector &pos, const Vector &normal, int type, float radius )
{
	int cursor = m_nStampCursor;
	int slot = cursor % TF_PAINT_STAMP_MAX;
	int stored = type + 1;
	if ( stored < 1 )
		stored = 1;
	if ( stored > 15 )
		stored = 15;
	float r = radius;
	if ( r < 0.f )
		r = 0.f;
	if ( r > 250.f )
		r = 250.f;

	m_vecStampPos.Set( slot, pos );
	m_vecStampNormal.Set( slot, normal );
	m_nStampType.Set( slot, stored );
	m_flStampRadius.Set( slot, r );
	m_nStampCursor = cursor + 1;
}

void CTFPaintMap::EraseSphere( const Vector &pos, float radius )
{
	float r2 = radius;
	if ( r2 < 8.f )
		r2 = 8.f;
	if ( r2 > 250.f )
		r2 = 250.f;
	// An erase is an event. Zeroing an old slot would delete it from the log
	// and the client would replay the gel that was supposed to be gone.
	m_Atlas.Erase( pos, r2 );
	PushStamp( pos, vec3_origin, NO_POWER, r2 );
}

void CTFPaintMap::AddBlob( const Vector &pos, const Vector &vel, int type )
{
	if ( !tf_paint_enabled.GetBool() )
		return;

	int slot = -1;
	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		if ( m_nBlobType[i] <= 0 )
		{
			slot = i;
			break;
		}
	}
	if ( slot < 0 )
	{
		slot = m_iBlobCursor % TF_PAINT_BLOB_MAX;
		++m_iBlobCursor;
	}

	m_vecBlobPos.Set( slot, pos );
	m_vecBlobVel[slot] = vel;
	m_nBlobType.Set( slot, type + 1 );
	m_flBlobDie[slot] = gpGlobals->curtime + 8.0f;
}

void CTFPaintMap::Think( void )
{
	SetNextThink( gpGlobals->curtime );

	const float dt = Clamp( gpGlobals->frametime, 0.f, 0.05f );
	if ( dt <= 0.f )
		return;

	const float gravity = GetCurrentGravity();
	const float blobR = 3.0f;
	const float splatR = tf_paint_splat_radius.GetFloat();
	Vector mins( -blobR, -blobR, -blobR );
	Vector maxs( blobR, blobR, blobR );

	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		int stored = m_nBlobType[i];
		if ( stored <= 0 )
			continue;

		if ( gpGlobals->curtime > m_flBlobDie[i] || m_vecBlobPos[i].z < -2048.f )
		{
			m_nBlobType.Set( i, 0 );
			continue;
		}

		m_vecBlobVel[i].z -= gravity * dt;
		Vector next = m_vecBlobPos[i] + m_vecBlobVel[i] * dt;

		CTraceFilterNoPlayers filter( this );
		trace_t tr;
		UTIL_TraceHull( m_vecBlobPos[i], next, mins, maxs, MASK_SOLID, &filter, &tr );

		if ( tr.fraction < 1.f && !( tr.surface.flags & SURF_SKY ) )
		{
			int type = stored - 1;
			if ( type == NO_POWER )
				EraseSphere( tr.endpos, splatR );
			else
				PaintSphere( tr.endpos, tr.plane.normal, type, splatR );

			if ( gpGlobals->curtime >= m_flNextSplatSound )
			{
				CPASAttenuationFilter sndFilter( tr.endpos, SNDLVL_70dB );
				EmitSound_t ep;
				ep.m_pSoundName = "Paint.Splat";
				ep.m_flVolume = 0.55f;
				ep.m_SoundLevel = SNDLVL_70dB;
				ep.m_pOrigin = &tr.endpos;
				EmitSound( sndFilter, entindex(), ep );
				m_flNextSplatSound = gpGlobals->curtime + 0.07f;
			}

			m_nBlobType.Set( i, 0 );
			continue;
		}

		m_vecBlobPos.Set( i, next );
	}
}

static CTFPaintMap *TFPaint_Ensure( void )
{
	if ( CTFPaintMap::Get() )
		return CTFPaintMap::Get();

	CBaseEntity *pEnt = CreateEntityByName( "tf_paint_map" );
	if ( !pEnt )
		return NULL;
	DispatchSpawn( pEnt );
	return CTFPaintMap::Get();
}

class CTFPaintSystem : public CAutoGameSystem
{
public:
	CTFPaintSystem() : CAutoGameSystem( "TFPaint" ) {}
	virtual void LevelInitPostEntity( void ) { TFPaint_Ensure(); }
};
static CTFPaintSystem s_TFPaintSystem;

#endif // GAME_DLL

TFPaintContact_t TFPaint_Query( const Vector &origin, float extra )
{
	CTFPaintMap *pMap = CTFPaintMap::Get();
	if ( !pMap )
		return TFPaintContact_t();
	return pMap->Query( origin, extra );
}

void TFPaint_AddBlob( const Vector &pos, const Vector &vel, int type )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->AddBlob( pos, vel, type );
#else
	(void)pos; (void)vel; (void)type;
#endif
}

void TFPaint_PaintSphere( const Vector &pos, const Vector &normal, int type, float radius )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->PaintSphere( pos, normal, type, radius );
#else
	(void)pos; (void)normal; (void)type; (void)radius;
#endif
}

void TFPaint_EraseSphere( const Vector &pos, float radius )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->EraseSphere( pos, radius );
#else
	(void)pos; (void)radius;
#endif
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Continuous blob sprayer. Angles aim the stream.
//-----------------------------------------------------------------------------
class CEnvPaintSprayer : public CPointEntity
{
	DECLARE_CLASS( CEnvPaintSprayer, CPointEntity );
public:
	void Spawn( void );
	void Think( void );
	void InputTurnOn( inputdata_t &input );
	void InputTurnOff( inputdata_t &input );

	int		m_nPaintType;
	float	m_flPerSecond;
	float	m_flSpeed;
	float	m_flRadius;
	int		m_bStartOn;
	bool	m_bOn;
	float	m_flAcc;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( env_paint_sprayer, CEnvPaintSprayer );

BEGIN_DATADESC( CEnvPaintSprayer )
	DEFINE_KEYFIELD( m_nPaintType, FIELD_INTEGER, "paint_type" ),
	DEFINE_KEYFIELD( m_flPerSecond, FIELD_FLOAT, "blobs_per_second" ),
	DEFINE_KEYFIELD( m_flSpeed, FIELD_FLOAT, "speed" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_bStartOn, FIELD_INTEGER, "start_on" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
END_DATADESC()

void CEnvPaintSprayer::Spawn( void )
{
	BaseClass::Spawn();
	if ( m_flPerSecond <= 0.f )
		m_flPerSecond = 8.f;
	if ( m_flSpeed <= 0.f )
		m_flSpeed = 700.f;
	if ( m_flRadius <= 0.f )
		m_flRadius = 48.f;
	m_bOn = ( m_bStartOn != 0 );
	m_flAcc = 0.f;
	SetNextThink( gpGlobals->curtime + 0.05f );
}

void CEnvPaintSprayer::InputTurnOn( inputdata_t &input )
{
	(void)input;
	m_bOn = true;
}

void CEnvPaintSprayer::InputTurnOff( inputdata_t &input )
{
	(void)input;
	m_bOn = false;
}

void CEnvPaintSprayer::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.05f );
	if ( !m_bOn || !tf_paint_enabled.GetBool() )
		return;

	m_flAcc += 0.05f * m_flPerSecond;
	if ( m_flAcc > 6.f )
		m_flAcc = 6.f;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	while ( m_flAcc >= 1.f )
	{
		m_flAcc -= 1.f;
		Vector vel = forward * m_flSpeed;
		vel.x += RandomFloat( -30.f, 30.f );
		vel.y += RandomFloat( -30.f, 30.f );
		TFPaint_AddBlob( GetAbsOrigin(), vel, m_nPaintType );
	}
}

//-----------------------------------------------------------------------------
// Map-placed gel. Keyfields match emulsion's paint_sphere.
//-----------------------------------------------------------------------------
class CPaintSphere : public CPointEntity
{
	DECLARE_CLASS( CPaintSphere, CPointEntity );
public:
	void Spawn( void );
	void Paint( void );
	void InputPaint( inputdata_t &inputdata );

	int		m_nPaintType;
	float	m_flRad;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( paint_sphere, CPaintSphere );

BEGIN_DATADESC( CPaintSphere )
	DEFINE_KEYFIELD( m_nPaintType, FIELD_INTEGER, "paint_type" ),
	DEFINE_KEYFIELD( m_flRad, FIELD_FLOAT, "radius" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Paint", InputPaint ),
END_DATADESC()

void CPaintSphere::Spawn( void )
{
	BaseClass::Spawn();
	if ( m_flRad <= 0.f )
		m_flRad = 128.f;
	// Map entities spawn before LevelInitPostEntity. Ensure the paint map, then paint.
	Paint();
}

void CPaintSphere::Paint( void )
{
	static const Vector dirs[6] =
	{
		Vector( 1, 0, 0 ), Vector( -1, 0, 0 ),
		Vector( 0, 1, 0 ), Vector( 0, -1, 0 ),
		Vector( 0, 0, 1 ), Vector( 0, 0, -1 ),
	};

	float rad = m_flRad;
	if ( rad > 256.f )
		rad = 256.f;

	for ( int i = 0; i < 6; ++i )
	{
		trace_t tr;
		CTraceFilterNoPlayers filter( this );
		UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin() + dirs[i] * m_flRad,
			Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), MASK_SOLID, &filter, &tr );
		if ( tr.fraction < 1.f && !( tr.surface.flags & SURF_SKY ) )
		{
			if ( m_nPaintType == NO_POWER )
				TFPaint_EraseSphere( tr.endpos, rad );
			else
				TFPaint_PaintSphere( tr.endpos, tr.plane.normal, m_nPaintType, rad );
		}
	}
}

void CPaintSphere::InputPaint( inputdata_t &inputdata )
{
	(void)inputdata;
	Paint();
}

#endif // GAME_DLL
