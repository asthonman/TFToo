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
#include "movevars_shared.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "model_types.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "texture_group_names.h"
#define CTFPaintMap C_TFPaintMap
#else
#include "tf_player.h"
#include "tf_obj.h"
#include "physics_shared.h"
#include "recipientfilter.h"
#include "particle_system.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "texture_group_names.h"
#endif

#include "materialsystem/imaterialvar.h"
#include "particle_parse.h"
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
ConVar tf_paint_grease_lifetime( "tf_paint_grease_lifetime", "10", FCVAR_REPLICATED, "Seconds a grease-gun splat stays before it is removed." );
ConVar tf_paint_ice_time( "tf_paint_ice_time", "3", FCVAR_REPLICATED, "Seconds of ice physics after a grease blob hits an enemy." );
ConVar tf_paint_ice_friction( "tf_paint_ice_friction", "0.05", FCVAR_REPLICATED, "Ground friction scale while iced." );
ConVar tf_paint_ice_accel( "tf_paint_ice_accel", "0.2", FCVAR_REPLICATED, "Ground acceleration scale while iced." );

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

void TFPaint_GetColor( int type, int &r, int &g, int &b, int team )
{
	int gel = type & 0x0F;
	if ( team <= 0 )
		team = ( type >> 4 ) & 0x0F;

	switch ( gel )
	{
	case BOUNCE_POWER:	r = 0;   g = 165; b = 255; break;
	case SPEED_POWER:	r = 255; g = 106; b = 0;   break;
	case PORTAL_POWER:	r = 0;   g = 200; b = 33;  break;
	case FIFTH_POWER:	r = 251; g = 255; b = 0;   break;
	case SIXTH_POWER:
		// Two browns. One yellow-brown puddle could not be told apart by team.
		if ( team == TF_TEAM_BLUE )
		{
			r = 78; g = 118; b = 40;	// Greenish brown.
		}
		else if ( team == TF_TEAM_RED )
		{
			r = 186; g = 96; b = 28;	// Orangish brown.
		}
		else
		{
			r = 120; g = 88; b = 36;
		}
		break;
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
	case SIXTH_POWER:	return "Grease";
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

// $nodecal (MATERIAL_VAR_SUPPRESS_DECALS) and SURF_NODECALS.
// trace_t is a typedef, so this stays out of the header.
static bool TFPaint_SurfaceBlocksPaint( const trace_t &tr )
{
	if ( tr.surface.flags & SURF_NODECALS )
		return true;

	if ( !materials )
		return false;

	const char *pszName = tr.surface.name;
	if ( !pszName || !pszName[0] )
		return false;

	IMaterial *pMat = materials->FindMaterial( pszName, TEXTURE_GROUP_OTHER, false );
	if ( !pMat || pMat->IsErrorMaterial() )
		return false;

	if ( pMat->GetMaterialVarFlag( MATERIAL_VAR_SUPPRESS_DECALS ) )
		return true;

	bool bFound = false;
	IMaterialVar *pVar = pMat->FindVar( "$nodecal", &bFound, false );
	if ( bFound && pVar && pVar->GetIntValue() != 0 )
		return true;

	return false;
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
		return CTraceFilterSimple::ShouldHitEntity( pHandle, contentsMask );
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
	// This SDK has no SetRenderBounds. The coat has to draw anywhere on the map.
	virtual void GetRenderBounds( Vector &mins, Vector &maxs )
	{
		mins.Init( -MAX_COORD_FLOAT, -MAX_COORD_FLOAT, -MAX_COORD_FLOAT );
		maxs.Init( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
	}
#else
	virtual void Think( void );
	void PaintSphere( const Vector &pos, const Vector &normal, int type, float radius, float flCoatLife, int iTeam, bool bForce );
	void EraseSphere( const Vector &pos, float radius );
	void AddBlob( const Vector &pos, const Vector &vel, int type, CBaseEntity *pOwner, float flCoatLife );
	void ReplayStamps( void );
	void AddTimedCoat( const Vector &pos, const Vector &normal, int type, float radius, float lifetime, bool bNitric, CBaseEntity *pOwner );
	void EraseCoatsNear( const Vector &pos, float radius );
	void IgniteNitricSegment( const Vector &start, const Vector &end );
	void IgniteNitricAt( const Vector &pos, float extra );
#endif

	TFPaintContact_t Query( const Vector &origin, float extra );
	int StampTeamNear( const Vector &origin, int type );
	TFPaintStampInfo_t StampInfoNear( const Vector &origin, int type, float extra );

	static CTFPaintMap *Get() { return s_pMap; }

private:
	static CTFPaintMap *s_pMap;

	CNetworkArray( Vector, m_vecStampPos, TF_PAINT_STAMP_MAX );
	CNetworkArray( Vector, m_vecStampNormal, TF_PAINT_STAMP_MAX );
	CNetworkArray( int, m_nStampType, TF_PAINT_STAMP_MAX );
	CNetworkArray( int, m_nStampTeam, TF_PAINT_STAMP_MAX );
	CNetworkArray( float, m_flStampRadius, TF_PAINT_STAMP_MAX );
	// 0 means the splat lasts until erased. Grease gun stamps store an absolute time.
	CNetworkArray( float, m_flStampDie, TF_PAINT_STAMP_MAX );
	CNetworkVar( int, m_nStampCursor );

	CNetworkArray( Vector, m_vecBlobPos, TF_PAINT_BLOB_MAX );
	CNetworkArray( int, m_nBlobType, TF_PAINT_BLOB_MAX );

	CTFPaintAtlas m_Atlas;

#ifndef CLIENT_DLL
	Vector	m_vecBlobVel[TF_PAINT_BLOB_MAX];
	float	m_flBlobDie[TF_PAINT_BLOB_MAX];
	float	m_flBlobCoatLife[TF_PAINT_BLOB_MAX];
	EHANDLE	m_hBlobOwner[TF_PAINT_BLOB_MAX];
	int		m_iBlobCursor;
	float	m_flNextSplatSound;
	float	m_flNextCoatExpire;
	void	PushStamp( const Vector &pos, const Vector &normal, int type, float radius, float flCoatLife, int iTeam );
	void	PlaySplatSound( const Vector &pos );

	struct ChemCoat_t
	{
		Vector	pos;
		float	radius;
		float	die;
		int		type;
		bool	nitric;
		bool	ignited;
		EHANDLE	owner;
		EHANDLE	fx;
	};
	CUtlVector< ChemCoat_t > m_ChemCoats;
	void	ThinkCoats( void );
	void	IgniteCoat( ChemCoat_t &coat );
	void	RemoveCoat( int index );

	struct BouncedProp_t
	{
		EHANDLE prop;
		EHANDLE owner;
		float until;
	};
	CUtlVector< BouncedProp_t > m_BouncedProps;
	float	m_flNextPropScan;
	void	ThinkProps( void );
	void	NoteBouncedProp( CBaseEntity *pProp, CBaseEntity *pOwner );
	CBaseEntity *BounceCoatOwnerNear( const Vector &pos );
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
	RecvPropArray3( RECVINFO_ARRAY( m_nStampType ), RecvPropInt( RECVINFO( m_nStampType[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nStampTeam ), RecvPropInt( RECVINFO( m_nStampTeam[0] ) ) ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flStampRadius[0] ) ), m_flStampRadius ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flStampDie[0] ) ), m_flStampDie ),
	RecvPropInt( RECVINFO( m_nStampCursor ) ),
	RecvPropArray( RecvPropVector( RECVINFO( m_vecBlobPos[0] ) ), m_vecBlobPos ),
	RecvPropArray3( RECVINFO_ARRAY( m_nBlobType ), RecvPropInt( RECVINFO( m_nBlobType[0] ) ) ),
#else
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vecStampPos ), -1, SPROP_COORD ), m_vecStampPos ),
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vecStampNormal ), -1, SPROP_NOSCALE ), m_vecStampNormal ),
	SendPropArray3( SENDINFO_ARRAY3( m_nStampType ), SendPropInt( SENDINFO_ARRAY( m_nStampType ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nStampTeam ), SendPropInt( SENDINFO_ARRAY( m_nStampTeam ), 4, SPROP_UNSIGNED ) ),
	SendPropArray( SendPropFloat( SENDINFO_ARRAY( m_flStampRadius ), 10, SPROP_ROUNDDOWN, 0.0f, 256.0f ), m_flStampRadius ),
	SendPropArray( SendPropFloat( SENDINFO_ARRAY( m_flStampDie ) ), m_flStampDie ),
	SendPropInt( SENDINFO( m_nStampCursor ), 32, SPROP_UNSIGNED ),
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vecBlobPos ), -1, SPROP_COORD ), m_vecBlobPos ),
	SendPropArray3( SENDINFO_ARRAY3( m_nBlobType ), SendPropInt( SENDINFO_ARRAY( m_nBlobType ), 8, SPROP_UNSIGNED ) ),
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
	m_flNextCoatExpire = 0.f;
	m_flNextPropScan = 0.f;
	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		m_vecBlobVel[i].Init();
		m_flBlobDie[i] = 0.f;
		m_flBlobCoatLife[i] = 0.f;
		m_hBlobOwner[i] = NULL;
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
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheParticleSystem( "burningplayer_red" );
	PrecacheParticleSystem( "burningplayer_blue" );
	PrecacheParticleSystem( "burningplayer_corpse" );
	PrecacheParticleSystem( "pyro_blast" );

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
		m_nStampTeam.Set( i, 0 );
		m_flStampRadius.Set( i, 0.f );
		m_flStampDie.Set( i, 0.f );
	}
	for ( int i = 0; i < TF_PAINT_BLOB_MAX; ++i )
	{
		m_vecBlobPos.Set( i, vec3_origin );
		m_nBlobType.Set( i, 0 );
	}
	SetNextThink( gpGlobals->curtime + 0.05f );
#else
	m_nAppliedCursor = 0;
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

int CTFPaintMap::StampTeamNear( const Vector &origin, int type )
{
	return StampInfoNear( origin, type, 28.f ).team;
}

TFPaintStampInfo_t CTFPaintMap::StampInfoNear( const Vector &origin, int type, float extra )
{
	TFPaintStampInfo_t info;
#ifdef CLIENT_DLL
	ApplyNetworkStamps();
#endif
	if ( extra < 0.f )
		extra = 0.f;

	int cur = m_nStampCursor;
	int start = ( cur > TF_PAINT_STAMP_MAX ) ? ( cur - TF_PAINT_STAMP_MAX ) : 0;
	const int want = type + 1;
	float best = 1.0e12f;
	for ( int s = start; s < cur; ++s )
	{
		int slot = s % TF_PAINT_STAMP_MAX;
		if ( m_nStampType[slot] != want )
			continue;
		float die = m_flStampDie[slot];
		if ( die > 0.f && gpGlobals->curtime >= die )
			continue;
		float rad = m_flStampRadius[slot] + extra;
		float d2 = ( origin - m_vecStampPos[slot] ).LengthSqr();
		if ( d2 > rad * rad || d2 >= best )
			continue;
		best = d2;
		info.found = true;
		info.team = m_nStampTeam[slot];
		info.die = die;
	}
	return info;
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
	int prevApplied = m_nAppliedCursor;
	int windowStart = ( cur > TF_PAINT_STAMP_MAX ) ? ( cur - TF_PAINT_STAMP_MAX ) : 0;

	bool bExpired = false;
	for ( int s = windowStart; s < cur; ++s )
	{
		int slot = s % TF_PAINT_STAMP_MAX;
		float die = m_flStampDie[slot];
		if ( die > 0.f && gpGlobals->curtime >= die && m_nStampType[slot] > 0 )
		{
			bExpired = true;
			break;
		}
	}

	bool bRewound = ( cur < prevApplied );
	bool bGap = ( !bRewound && ( cur - prevApplied ) > TF_PAINT_STAMP_MAX );
	int applied = prevApplied;
	if ( bExpired || bRewound || bGap )
	{
		m_Atlas.Clear();
		applied = windowStart;
		// A rewind or a gap bigger than the log is a late join. Don't pop the backlog.
		// An expiry rebuild still pops only stamps past the cursor we had already drawn.
		if ( bRewound || bGap )
			prevApplied = applied;
	}

	int behind = cur - prevApplied;
	bool bEffects = ( behind >= 1 && behind <= 3 );

	for ( int s = applied; s < cur; ++s )
	{
		int slot = s % TF_PAINT_STAMP_MAX;
		int stored = m_nStampType[slot];
		if ( stored <= 0 )
			continue;
		float die = m_flStampDie[slot];
		if ( die > 0.f && gpGlobals->curtime >= die )
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
		m_Atlas.Stamp( pos, n, type, radius, m_nStampTeam[slot] );
		if ( bEffects && s >= prevApplied )
			NotePop( pos, n, type | ( ( m_nStampTeam[slot] & 0x0F ) << 4 ) );
	}
	m_nAppliedCursor = cur;
}

void CTFPaintMap::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );
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
		{
			int packed = m_nBlobSeen[i];
			int gel = ( packed & 0x0F ) - 1;
			int team = ( packed >> 4 ) & 0x0F;
			NotePop( m_vecBlobSeen[i], Vector( 0, 0, 1 ), gel | ( team << 4 ) );
		}
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
			int packed = m_nBlobType[i];
			int gel = ( packed & 0x0F ) - 1;
			int team = ( packed >> 4 ) & 0x0F;
			int r, g, b;
			TFPaint_GetColor( gel, r, g, b, team );
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

// Grease blobs hit enemies. Teammates and the shooter are skipped so the blob
// can still paint the wall behind them.
class CTraceFilterPaintBlob : public CTraceFilterSimple
{
public:
	CTraceFilterPaintBlob( const IHandleEntity *pass, CBaseEntity *pOwner )
		: CTraceFilterSimple( pass, COLLISION_GROUP_NONE ), m_pOwner( pOwner ) {}

	virtual bool ShouldHitEntity( IHandleEntity *pHandle, int contentsMask )
	{
		CBaseEntity *pEnt = EntityFromEntityHandle( pHandle );
		if ( pEnt && pEnt->IsPlayer() )
		{
			if ( !m_pOwner || pEnt == m_pOwner )
				return false;
			if ( pEnt->GetTeamNumber() == m_pOwner->GetTeamNumber() )
				return false;
			return true;
		}
		return CTraceFilterSimple::ShouldHitEntity( pHandle, contentsMask );
	}

private:
	CBaseEntity *m_pOwner;
};

// Dynamic props and func_brush stop a bullet, but they are not gel surfaces.
static bool TFPaint_EntityRejectsPaint( CBaseEntity *pEnt )
{
	if ( !pEnt || pEnt->IsWorld() )
		return false;
	const char *cls = pEnt->GetClassname();
	if ( !cls || !cls[0] )
		return false;
	if ( !Q_strnicmp( cls, "prop_dynamic", 12 ) )
		return true;
	if ( !Q_stricmp( cls, "func_brush" ) )
		return true;
	return false;
}

// Splash damage uses TF_PAINT_VIS_MASK. A surface that mask does not stop on
// is bullet-pass, same as a grate between a blast and a player.
static bool TFPaint_SplashMisses( const Vector &start, const Vector &end, const trace_t &solidHit )
{
	if ( solidHit.contents & CONTENTS_GRATE )
		return true;

	trace_t vis;
	CTraceFilterNoPlayers filter( NULL );
	UTIL_TraceLine( start, end, TF_PAINT_VIS_MASK, &filter, &vis );
	if ( vis.startsolid && vis.m_pEnt == solidHit.m_pEnt )
		return false;
	if ( vis.m_pEnt == solidHit.m_pEnt && vis.fraction <= solidHit.fraction + 0.02f )
		return false;
	return true;
}

// bForce still paints over $nodecal (allied buildings). It does not paint a
// bullet-pass material, a dynamic prop, or a func_brush.
static bool TFPaint_SpotRejectsPaint( const Vector &pos, const Vector &normal, bool bForce )
{
	Vector n = normal;
	if ( n.LengthSqr() < 0.01f )
		n.Init( 0, 0, 1 );
	VectorNormalize( n );

	const Vector start = pos + n * 2.f;
	const Vector end = pos - n * 8.f;

	trace_t tr;
	CTraceFilterNoPlayers filter( NULL );
	UTIL_TraceLine( start, end, MASK_SOLID, &filter, &tr );
	if ( ( tr.fraction >= 1.f && !tr.startsolid ) || !tr.m_pEnt )
		return false;
	if ( TFPaint_EntityRejectsPaint( tr.m_pEnt ) )
		return true;
	if ( TFPaint_SplashMisses( start, end, tr ) )
		return true;
	if ( !bForce && TFPaint_SurfaceBlocksPaint( tr ) )
		return true;
	return false;
}

static bool TFPaint_HitRejectsPaint( const trace_t &tr, bool bForce )
{
	if ( TFPaint_EntityRejectsPaint( tr.m_pEnt ) )
		return true;
	if ( tr.contents & CONTENTS_GRATE )
		return true;
	if ( !bForce && TFPaint_SurfaceBlocksPaint( tr ) )
		return true;
	return false;
}

void CTFPaintMap::PaintSphere( const Vector &pos, const Vector &normal, int type, float radius, float flCoatLife, int iTeam, bool bForce )
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

	if ( TFPaint_SpotRejectsPaint( pos, n, bForce ) )
		return;

	PushStamp( pos, n, type, radius, flCoatLife, iTeam );
	ReplayStamps();
}

void CTFPaintMap::PushStamp( const Vector &pos, const Vector &normal, int type, float radius, float flCoatLife, int iTeam )
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

	float die = 0.f;
	if ( flCoatLife > 0.f )
		die = gpGlobals->curtime + flCoatLife;

	if ( iTeam < 0 )
		iTeam = 0;
	if ( iTeam > 15 )
		iTeam = 15;

	m_vecStampPos.Set( slot, pos );
	m_vecStampNormal.Set( slot, normal );
	m_nStampType.Set( slot, stored );
	m_nStampTeam.Set( slot, iTeam );
	m_flStampRadius.Set( slot, r );
	m_flStampDie.Set( slot, die );
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
	PushStamp( pos, vec3_origin, NO_POWER, r2, 0.f, 0 );
	ReplayStamps();
}

void CTFPaintMap::ReplayStamps( void )
{
	m_Atlas.Clear();

	int cur = m_nStampCursor;
	int start = 0;
	if ( cur > TF_PAINT_STAMP_MAX )
		start = cur - TF_PAINT_STAMP_MAX;

	float flNext = 0.f;
	for ( int s = start; s < cur; ++s )
	{
		int slot = s % TF_PAINT_STAMP_MAX;
		int stored = m_nStampType[slot];
		if ( stored <= 0 )
			continue;

		float die = m_flStampDie[slot];
		if ( die > 0.f && gpGlobals->curtime >= die )
			continue;
		if ( die > 0.f && ( flNext <= 0.f || die < flNext ) )
			flNext = die;

		int type = stored - 1;
		float radius = m_flStampRadius[slot];
		Vector pos = m_vecStampPos[slot];
		Vector n = m_vecStampNormal[slot];
		if ( type == NO_POWER || n.LengthSqr() < 0.01f )
		{
			m_Atlas.Erase( pos, radius );
			continue;
		}
		m_Atlas.Stamp( pos, n, type, radius, m_nStampTeam[slot] );
	}

	m_flNextCoatExpire = flNext;
}

void CTFPaintMap::AddBlob( const Vector &pos, const Vector &vel, int type, CBaseEntity *pOwner, float flCoatLife )
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

	int team = 0;
	if ( pOwner )
		team = pOwner->GetTeamNumber();
	m_vecBlobPos.Set( slot, pos );
	m_vecBlobVel[slot] = vel;
	m_nBlobType.Set( slot, ( ( type + 1 ) & 0x0F ) | ( ( team & 0x0F ) << 4 ) );
	m_flBlobDie[slot] = gpGlobals->curtime + 8.0f;
	m_flBlobCoatLife[slot] = flCoatLife;
	m_hBlobOwner[slot] = pOwner;
}

void CTFPaintMap::PlaySplatSound( const Vector &pos )
{
	if ( gpGlobals->curtime < m_flNextSplatSound )
		return;

	CPASAttenuationFilter sndFilter( pos, SNDLVL_70dB );
	EmitSound_t ep;
	ep.m_pSoundName = "Paint.Splat";
	ep.m_flVolume = 0.55f;
	ep.m_SoundLevel = SNDLVL_70dB;
	ep.m_pOrigin = &pos;
	EmitSound( sndFilter, entindex(), ep );
	m_flNextSplatSound = gpGlobals->curtime + 0.07f;
}

void CTFPaintMap::Think( void )
{
	SetNextThink( gpGlobals->curtime );

	if ( m_flNextCoatExpire > 0.f && gpGlobals->curtime >= m_flNextCoatExpire )
		ReplayStamps();

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
		int packed = m_nBlobType[i];
		if ( ( packed & 0x0F ) <= 0 )
			continue;

		if ( gpGlobals->curtime > m_flBlobDie[i] || m_vecBlobPos[i].z < -2048.f )
		{
			m_nBlobType.Set( i, 0 );
			m_flBlobCoatLife[i] = 0.f;
			m_hBlobOwner[i] = NULL;
			continue;
		}

		m_vecBlobVel[i].z -= gravity * dt;
		Vector next = m_vecBlobPos[i] + m_vecBlobVel[i] * dt;

		const bool bGrease = ( m_flBlobCoatLife[i] > 0.f );
		trace_t tr;
		const int mask = bGrease ? ( TF_PAINT_VIS_MASK | CONTENTS_HITBOX ) : TF_PAINT_VIS_MASK;
		if ( bGrease )
		{
			CTraceFilterPaintBlob filter( this, m_hBlobOwner[i].Get() );
			UTIL_TraceHull( m_vecBlobPos[i], next, mins, maxs, mask, &filter, &tr );
		}
		else
		{
			CTraceFilterNoPlayers filter( this );
			UTIL_TraceHull( m_vecBlobPos[i], next, mins, maxs, mask, &filter, &tr );
		}

		if ( tr.fraction < 1.f && !( tr.surface.flags & SURF_SKY ) )
		{
			CTFPlayer *pVictim = ToTFPlayer( tr.m_pEnt );
			if ( bGrease && pVictim )
			{
				if ( pVictim->IsAlive() )
				{
					pVictim->m_Shared.AddIcePhysics( tf_paint_ice_time.GetFloat() );
					pVictim->m_Shared.AddWet( 10.f );
					PlaySplatSound( tr.endpos );
				}
				m_nBlobType.Set( i, 0 );
				m_flBlobCoatLife[i] = 0.f;
				m_hBlobOwner[i] = NULL;
				continue;
			}

			// Bullet-pass, dynamic props, and func_brush are not gel surfaces.
			// Splash damage does not stop on them either, so the blob keeps going.
			if ( ( tr.contents & CONTENTS_GRATE ) || TFPaint_EntityRejectsPaint( tr.m_pEnt ) )
			{
				Vector push = m_vecBlobVel[i];
				if ( push.NormalizeInPlace() < 0.1f )
					push = next - m_vecBlobPos[i];
				VectorNormalize( push );
				m_vecBlobPos.Set( i, tr.endpos + push * 24.f );
				continue;
			}

			int team = ( packed >> 4 ) & 0x0F;
			CBaseEntity *pOwner = m_hBlobOwner[i].Get();
			if ( team < TF_TEAM_RED && pOwner )
				team = pOwner->GetTeamNumber();
			int type = ( packed & 0x0F ) - 1;

			// Grease on an allied building has to stick even when the model
			// suppresses decals, and it has to cover the building, not just the pixel we hit.
			bool bCoatedBuilding = false;
			if ( bGrease && tr.m_pEnt && tr.m_pEnt->IsBaseObject() && type == SIXTH_POWER )
			{
				if ( team < TF_TEAM_RED || tr.m_pEnt->GetTeamNumber() == team )
				{
					const float flLife = m_flBlobCoatLife[i];
					PaintSphere( tr.m_pEnt->GetAbsOrigin(), Vector( 0.f, 0.f, 1.f ), type, 96.f, flLife, team, true );
					PaintSphere( tr.m_pEnt->WorldSpaceCenter(), Vector( 0.f, 0.f, 1.f ), type, 96.f, flLife, team, true );
					bCoatedBuilding = true;
				}
			}

			if ( TFPaint_HitRejectsPaint( tr, bCoatedBuilding ) && !bCoatedBuilding )
			{
				m_nBlobType.Set( i, 0 );
				m_flBlobCoatLife[i] = 0.f;
				m_hBlobOwner[i] = NULL;
				continue;
			}

			if ( !TFPaint_HitRejectsPaint( tr, false ) )
			{
				if ( type == NO_POWER )
					EraseSphere( tr.endpos, splatR );
				else
					PaintSphere( tr.endpos, tr.plane.normal, type, splatR, m_flBlobCoatLife[i], team, false );
			}

			PlaySplatSound( tr.endpos );
			m_nBlobType.Set( i, 0 );
			m_flBlobCoatLife[i] = 0.f;
			m_hBlobOwner[i] = NULL;
			continue;
		}

		m_vecBlobPos.Set( i, next );
	}

	ThinkCoats();
	ThinkProps();
}

void CTFPaintMap::AddTimedCoat( const Vector &pos, const Vector &normal, int type, float radius, float lifetime, bool bNitric, CBaseEntity *pOwner )
{
	if ( !tf_paint_enabled.GetBool() )
		return;
	if ( radius < 16.f )
		radius = 16.f;

	Vector n = normal;
	if ( n.LengthSqr() < 0.01f )
		n.Init( 0.f, 0.f, 1.f );
	VectorNormalize( n );
	// Same rejects as a blob. bForce only skips $nodecal.
	if ( TFPaint_SpotRejectsPaint( pos, n, true ) )
		return;

	if ( m_ChemCoats.Count() >= 24 )
	{
		RemoveCoat( 0 );
	}

	ChemCoat_t coat;
	coat.pos = pos;
	coat.radius = radius;
	coat.die = gpGlobals->curtime + lifetime;
	coat.type = type;
	coat.nitric = bNitric;
	coat.ignited = false;
	coat.owner = pOwner;
	coat.fx = NULL;
	m_ChemCoats.AddToTail( coat );
	int team = 0;
	if ( pOwner )
		team = pOwner->GetTeamNumber();
	PaintSphere( pos, normal, type, radius, lifetime, team, true );
}

void CTFPaintMap::EraseCoatsNear( const Vector &pos, float radius )
{
	const float r2 = radius * radius;
	for ( int i = m_ChemCoats.Count() - 1; i >= 0; --i )
	{
		if ( m_ChemCoats[i].pos.DistToSqr( pos ) <= r2 )
			RemoveCoat( i );
	}
	EraseSphere( pos, radius );
}

void CTFPaintMap::RemoveCoat( int index )
{
	if ( index < 0 || index >= m_ChemCoats.Count() )
		return;
	if ( m_ChemCoats[index].fx.Get() )
		UTIL_Remove( m_ChemCoats[index].fx );
	EraseSphere( m_ChemCoats[index].pos, m_ChemCoats[index].radius );
	m_ChemCoats.Remove( index );
}

void CTFPaintMap::IgniteCoat( ChemCoat_t &coat )
{
	if ( !coat.nitric || coat.ignited )
		return;

	coat.ignited = true;
	// Gas passer clouds last TF_GAS_LIFETIME. Don't cut a fresh puddle short,
	// but a hit in the last moment still has to show fire.
	if ( coat.die < gpGlobals->curtime + 1.f )
		coat.die = gpGlobals->curtime + 1.f;

	const Vector vecFire = coat.pos + Vector( 0.f, 0.f, 8.f );
	DispatchParticleEffect( "pyro_blast", vecFire, vec3_angle );
	CParticleSystem *pFx = (CParticleSystem *)CreateEntityByName( "info_particle_system" );
	if ( pFx )
	{
		pFx->KeyValue( "effect_name", "burningplayer_corpse" );
		pFx->KeyValue( "start_active", "1" );
		pFx->SetAbsOrigin( vecFire );
		DispatchSpawn( pFx );
		pFx->Activate();
		pFx->StartParticleSystem();
		coat.fx = pFx;
	}

	CPASAttenuationFilter sndFilter( vecFire, SNDLVL_NORM );
	EmitSound_t ep;
	ep.m_pSoundName = "Weapon_FlameThrower.AirBurstAttack";
	ep.m_flVolume = 0.8f;
	ep.m_SoundLevel = SNDLVL_NORM;
	ep.m_pOrigin = &vecFire;
	EmitSound( sndFilter, entindex(), ep );
}

void CTFPaintMap::IgniteNitricSegment( const Vector &start, const Vector &end )
{
	const Vector seg = end - start;
	const float segLenSqr = seg.LengthSqr();
	for ( int i = 0; i < m_ChemCoats.Count(); ++i )
	{
		ChemCoat_t &coat = m_ChemCoats[i];
		if ( !coat.nitric || coat.ignited )
			continue;

		float t = 0.f;
		if ( segLenSqr > 1.f )
		{
			t = DotProduct( coat.pos - start, seg ) / segLenSqr;
			if ( t < 0.f )
				t = 0.f;
			if ( t > 1.f )
				t = 1.f;
		}
		const Vector closest = start + seg * t;
		if ( closest.DistToSqr( coat.pos ) > ( coat.radius + 32.f ) * ( coat.radius + 32.f ) )
			continue;

		IgniteCoat( coat );
	}
}

void CTFPaintMap::IgniteNitricAt( const Vector &pos, float extra )
{
	if ( extra < 0.f )
		extra = 0.f;
	for ( int i = 0; i < m_ChemCoats.Count(); ++i )
	{
		ChemCoat_t &coat = m_ChemCoats[i];
		if ( !coat.nitric || coat.ignited )
			continue;
		const float lim = coat.radius + extra;
		if ( coat.pos.DistToSqr( pos ) > lim * lim )
			continue;
		IgniteCoat( coat );
	}
}

void CTFPaintMap::ThinkCoats( void )
{
	if ( m_ChemCoats.Count() <= 0 )
		return;

	for ( int i = m_ChemCoats.Count() - 1; i >= 0; --i )
	{
		ChemCoat_t &coat = m_ChemCoats[i];
		if ( gpGlobals->curtime >= coat.die )
		{
			RemoveCoat( i );
			continue;
		}

		if ( !coat.nitric )
			continue;

		const float r2 = coat.radius * coat.radius;
		bool bIgnite = coat.ignited;
		for ( int p = 1; p <= gpGlobals->maxClients && !bIgnite; ++p )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( p ) );
			if ( !pPlayer || !pPlayer->IsAlive() )
				continue;
			if ( pPlayer->GetAbsOrigin().DistToSqr( coat.pos ) > r2 )
				continue;
			if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
				bIgnite = true;
		}

		if ( bIgnite && !coat.ignited )
			IgniteCoat( coat );

		// Unlit nitric does nothing to someone who walks onto it.
		// Lit nitric only sets them on fire. It does not apply the gas condition.
		if ( !coat.ignited )
			continue;

		CTFPlayer *pOwner = ToTFPlayer( coat.owner.Get() );
		for ( int p = 1; p <= gpGlobals->maxClients; ++p )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( p ) );
			if ( !pPlayer || !pPlayer->IsAlive() )
				continue;
			if ( pPlayer->GetAbsOrigin().DistToSqr( coat.pos ) > r2 )
				continue;
			if ( pOwner && pPlayer->GetTeamNumber() == pOwner->GetTeamNumber() )
				continue;
			if ( pPlayer->m_Shared.IsInvulnerable() )
				continue;
			if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
				continue;

			// Same afterburn the gas passer uses when its coat is lit (TF_GAS_AFTERBURN_RATE).
			pPlayer->m_Shared.Burn( pOwner ? pOwner : pPlayer, NULL, 10.f );
		}
	}
}

static float TFPaint_SpeedDamage( float flSpeed )
{
	// gDefaultPlayerImpactDamageTable linear steps. First entry is the minimum.
	static const float flMin2[6] = {
		150.f * 150.f, 250.f * 250.f, 450.f * 450.f,
		550.f * 550.f, 700.f * 700.f, 1000.f * 1000.f
	};
	static const float flDmg[6] = { 5.f, 10.f, 20.f, 50.f, 100.f, 500.f };
	const float s2 = flSpeed * flSpeed;
	if ( s2 < flMin2[0] )
		return 0.f;
	if ( s2 >= flMin2[5] )
		return flDmg[5];
	for ( int i = 0; i < 5; ++i )
	{
		if ( s2 < flMin2[i + 1] )
		{
			const float t = ( s2 - flMin2[i] ) / ( flMin2[i + 1] - flMin2[i] );
			return flDmg[i] + ( flDmg[i + 1] - flDmg[i] ) * t;
		}
	}
	return flDmg[5];
}

struct TFPaintSmashPair_t
{
	EHANDLE mover;
	EHANDLE victim;
	float next;
};
static CUtlVector< TFPaintSmashPair_t > s_SmashPairs;

static bool TFPaint_SmashCooling( CBaseEntity *pMover, CBaseEntity *pVictim )
{
	for ( int i = s_SmashPairs.Count() - 1; i >= 0; --i )
	{
		TFPaintSmashPair_t &rec = s_SmashPairs[i];
		if ( !rec.mover.Get() || gpGlobals->curtime >= rec.next )
		{
			s_SmashPairs.FastRemove( i );
			continue;
		}
		if ( rec.mover.Get() == pMover && rec.victim.Get() == pVictim )
			return true;
	}
	return false;
}

void TFPaint_SmashEnemies( CBaseEntity *pMover, const Vector &origin, const Vector &velocity, float radius, CBaseEntity *pAttacker )
{
	if ( !pMover || radius <= 0.f )
		return;
	if ( !pAttacker )
		pAttacker = pMover;

	const float dmg = TFPaint_SpeedDamage( velocity.Length() );
	if ( dmg < 1.f )
		return;

	int team = pAttacker->GetTeamNumber();
	if ( team < TF_TEAM_RED )
		team = pMover->GetTeamNumber();

	const float r2 = radius * radius;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() || pPlayer == pMover )
			continue;
		if ( pPlayer->m_Shared.IsInvulnerable() )
			continue;
		if ( team >= TF_TEAM_RED && pPlayer->GetTeamNumber() == team )
			continue;
		if ( pPlayer->WorldSpaceCenter().DistToSqr( origin ) > r2 && pPlayer->GetAbsOrigin().DistToSqr( origin ) > r2 )
			continue;
		if ( TFPaint_SmashCooling( pMover, pPlayer ) )
			continue;

		CTakeDamageInfo info( pMover, pAttacker, dmg, DMG_CRUSH );
		pPlayer->TakeDamage( info );

		TFPaintSmashPair_t rec;
		rec.mover = pMover;
		rec.victim = pPlayer;
		rec.next = gpGlobals->curtime + 0.35f;
		s_SmashPairs.AddToTail( rec );
	}

	for ( int o = 0; o < IBaseObjectAutoList::AutoList().Count(); ++o )
	{
		CBaseObject *pObj = static_cast< CBaseObject * >( IBaseObjectAutoList::AutoList()[o] );
		if ( !pObj || pObj == pMover || !pObj->IsAlive() || pObj->IsPlacing() || pObj->IsCarried() )
			continue;
		if ( pObj->m_takedamage == DAMAGE_NO )
			continue;
		if ( team >= TF_TEAM_RED && pObj->GetTeamNumber() == team )
			continue;
		if ( pObj->WorldSpaceCenter().DistToSqr( origin ) > r2 && pObj->GetAbsOrigin().DistToSqr( origin ) > r2 )
			continue;
		if ( TFPaint_SmashCooling( pMover, pObj ) )
			continue;

		CTakeDamageInfo info( pMover, pAttacker, dmg, DMG_CRUSH );
		pObj->TakeDamage( info );

		TFPaintSmashPair_t rec;
		rec.mover = pMover;
		rec.victim = pObj;
		rec.next = gpGlobals->curtime + 0.35f;
		s_SmashPairs.AddToTail( rec );
	}
}

void CTFPaintMap::NoteBouncedProp( CBaseEntity *pProp, CBaseEntity *pOwner )
{
	const float until = gpGlobals->curtime + 0.6f;
	for ( int i = 0; i < m_BouncedProps.Count(); ++i )
	{
		if ( m_BouncedProps[i].prop.Get() == pProp )
		{
			m_BouncedProps[i].until = until;
			if ( pOwner )
				m_BouncedProps[i].owner = pOwner;
			return;
		}
	}

	BouncedProp_t rec;
	rec.prop = pProp;
	rec.owner = pOwner;
	rec.until = until;
	m_BouncedProps.AddToTail( rec );
}

CBaseEntity *CTFPaintMap::BounceCoatOwnerNear( const Vector &pos )
{
	CBaseEntity *pBest = NULL;
	float best = 1.0e12f;
	for ( int i = 0; i < m_ChemCoats.Count(); ++i )
	{
		const ChemCoat_t &coat = m_ChemCoats[i];
		if ( coat.type != BOUNCE_POWER )
			continue;
		const float lim = coat.radius + 64.f;
		const float d2 = coat.pos.DistToSqr( pos );
		if ( d2 > lim * lim || d2 >= best )
			continue;
		best = d2;
		pBest = coat.owner.Get();
	}
	return pBest;
}

void CTFPaintMap::ThinkProps( void )
{
	for ( int i = m_BouncedProps.Count() - 1; i >= 0; --i )
	{
		CBaseEntity *pProp = m_BouncedProps[i].prop.Get();
		if ( !pProp || gpGlobals->curtime >= m_BouncedProps[i].until )
		{
			m_BouncedProps.FastRemove( i );
			continue;
		}

		IPhysicsObject *pPhys = pProp->VPhysicsGetObject();
		if ( !pPhys )
			continue;

		Vector vel;
		AngularImpulse ang;
		pPhys->GetVelocity( &vel, &ang );
		CBaseEntity *pAttacker = m_BouncedProps[i].owner.Get();
		if ( !pAttacker )
			pAttacker = pProp;

		const Vector size = pProp->WorldAlignMaxs() - pProp->WorldAlignMins();
		float rad = size.Length() * 0.35f;
		if ( rad < 48.f )
			rad = 48.f;
		if ( rad > 120.f )
			rad = 120.f;
		TFPaint_SmashEnemies( pProp, pProp->WorldSpaceCenter(), vel, rad, pAttacker );
	}

	if ( gpGlobals->curtime < m_flNextPropScan )
		return;
	m_flNextPropScan = gpGlobals->curtime + 0.05f;
	if ( !tf_paint_enabled.GetBool() )
		return;

	const float flMin = tf_paint_bounce_min.GetFloat();
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "prop_physics*" ) ) != NULL )
	{
		if ( pEnt->GetMoveType() != MOVETYPE_VPHYSICS )
			continue;

		IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
		if ( !pPhys || !pPhys->IsMoveable() )
			continue;
		if ( pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			continue;

		TFPaintContact_t hit = TFPaint_Query( pEnt->WorldSpaceCenter(), 28.f );
		if ( !hit.painted || hit.type != BOUNCE_POWER )
			hit = TFPaint_Query( pEnt->GetAbsOrigin(), 36.f );
		if ( !hit.painted || hit.type != BOUNCE_POWER )
			continue;

		Vector n = hit.normal;
		if ( n.NormalizeInPlace() < 0.1f )
			n.Init( 0.f, 0.f, 1.f );

		Vector vel;
		AngularImpulse ang;
		pPhys->GetVelocity( &vel, &ang );
		const float into = DotProduct( vel, n );
		if ( into < 120.f )
		{
			Vector out = vel - n * into;
			float away = flMin;
			if ( into < 0.f && -into > away )
				away = -into;
			out += n * away;
			pPhys->SetVelocity( &out, &ang );
			pPhys->Wake();
			pEnt->SetGroundEntity( NULL );
		}

		NoteBouncedProp( pEnt, BounceCoatOwnerNear( pEnt->WorldSpaceCenter() ) );
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

int TFPaint_QueryStampTeam( const Vector &origin, int type )
{
	CTFPaintMap *pMap = CTFPaintMap::Get();
	if ( !pMap )
		return 0;
	return pMap->StampTeamNear( origin, type );
}

TFPaintStampInfo_t TFPaint_QueryStampInfo( const Vector &origin, int type, float extra )
{
	CTFPaintMap *pMap = CTFPaintMap::Get();
	if ( !pMap )
		return TFPaintStampInfo_t();
	return pMap->StampInfoNear( origin, type, extra );
}

void TFPaint_AddBlob( const Vector &pos, const Vector &vel, int type, CBaseEntity *pOwner, float flCoatLife )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->AddBlob( pos, vel, type, pOwner, flCoatLife );
#else
	(void)pos; (void)vel; (void)type; (void)pOwner; (void)flCoatLife;
#endif
}

void TFPaint_PaintSphere( const Vector &pos, const Vector &normal, int type, float radius, float flCoatLife, int iTeam, bool bForce )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->PaintSphere( pos, normal, type, radius, flCoatLife, iTeam, bForce );
#else
	(void)pos; (void)normal; (void)type; (void)radius; (void)flCoatLife; (void)iTeam; (void)bForce;
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

void TFPaint_AddTimedCoat( const Vector &pos, const Vector &normal, int type, float radius, float lifetime, bool bNitric, CBaseEntity *pOwner )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->AddTimedCoat( pos, normal, type, radius, lifetime, bNitric, pOwner );
#else
	(void)pos; (void)normal; (void)type; (void)radius; (void)lifetime; (void)bNitric; (void)pOwner;
#endif
}

void TFPaint_EraseCoatsNear( const Vector &pos, float radius )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = TFPaint_Ensure();
	if ( pMap )
		pMap->EraseCoatsNear( pos, radius );
#else
	(void)pos; (void)radius;
#endif
}

void TFPaint_IgniteNitricSegment( const Vector &start, const Vector &end )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = CTFPaintMap::Get();
	if ( pMap )
		pMap->IgniteNitricSegment( start, end );
#else
	(void)start; (void)end;
#endif
}

void TFPaint_IgniteNitricAt( const Vector &pos, float extra )
{
#ifdef GAME_DLL
	CTFPaintMap *pMap = CTFPaintMap::Get();
	if ( pMap )
		pMap->IgniteNitricAt( pos, extra );
#else
	(void)pos; (void)extra;
#endif
}

bool TFPaint_ReflectProjectile( CBaseEntity *pEnt, const Vector &hitPos, const Vector &nOutward )
{
	if ( !pEnt || !tf_paint_enabled.GetBool() )
		return false;

	TFPaintContact_t hit = TFPaint_Query( hitPos, 16.f );
	if ( hit.type != BOUNCE_POWER )
		return false;

	Vector n = nOutward;
	if ( n.LengthSqr() < 0.01f )
		n = hit.normal;
	if ( n.NormalizeInPlace() < 0.1f )
		return false;

	Vector vel = pEnt->GetAbsVelocity();
	IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
	AngularImpulse ang( 0, 0, 0 );
	if ( pPhys && vel.LengthSqr() < 100.f )
		pPhys->GetVelocity( &vel, &ang );

	const float into = DotProduct( vel, n );
	// Already leaving the surface. A second check after a bounce must not reverse it again.
	if ( into >= -1.f )
		return false;

	const float flSpeed = vel.Length();
	Vector out = vel - 2.f * into * n;
	const float flOut = out.Length();
	if ( flSpeed > 1.f )
	{
		if ( flOut > 1.f )
			out *= flSpeed / flOut;
		else
			out = n * flSpeed;
	}

	pEnt->SetGroundEntity( NULL );
	pEnt->SetAbsOrigin( hitPos + n * 8.f );
	pEnt->SetAbsVelocity( out );
	if ( pPhys )
	{
		AngularImpulse zero( 0, 0, 0 );
		pPhys->SetVelocity( &out, &zero );
		pPhys->Wake();
	}
	return true;
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
