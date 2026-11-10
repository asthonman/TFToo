//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "tf_weapon_paintgun.h"
#include "tf_paint.h"
#include "in_buttons.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_projectile_arrow.h"
#include "tf_powerup.h"
#else
#include "c_tf_player.h"
#include "c_baseviewmodel.h"
#endif

#include "tier0/memdbgon.h"

// Emulsion defaults: 40 blobs/s, 950-1050 speed, 10 degree cone, offsets (20, 64, -20).
ConVar tf_paintgun_blobs_per_second( "tf_paintgun_blobs_per_second", "40", FCVAR_REPLICATED, "Gel blobs fired per second." );
ConVar tf_paintgun_min_speed( "tf_paintgun_min_speed", "950", FCVAR_REPLICATED );
ConVar tf_paintgun_max_speed( "tf_paintgun_max_speed", "1050", FCVAR_REPLICATED );
ConVar tf_paintgun_spread( "tf_paintgun_spread", "10", FCVAR_REPLICATED, "Cone, in degrees." );
ConVar tf_paintgun_offset_x( "tf_paintgun_offset_x", "20", FCVAR_REPLICATED );
ConVar tf_paintgun_offset_y( "tf_paintgun_offset_y", "64", FCVAR_REPLICATED );
ConVar tf_paintgun_offset_z( "tf_paintgun_offset_z", "-20", FCVAR_REPLICATED );
ConVar tf_paintgun_fire_blobs( "tf_paintgun_fire_blobs", "1", FCVAR_REPLICATED, "If 0, the gun paints a spot along the crosshair instead of firing blobs." );
ConVar tf_paintgun_rad( "tf_paintgun_rad", "64", FCVAR_REPLICATED, "Radius of a direct (non-blob) splat." );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPaintgun, DT_TFWeaponPaintgun )

BEGIN_NETWORK_TABLE( CTFWeaponPaintgun, DT_TFWeaponPaintgun )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nPaintType ) ),
#else
	SendPropInt( SENDINFO( m_nPaintType ), 4, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPaintgun )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_nPaintType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_paintgun, CTFWeaponPaintgun );
PRECACHE_WEAPON_REGISTER( tf_weapon_paintgun );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponPaintgun )
END_DATADESC()
#endif

CTFWeaponPaintgun::CTFWeaponPaintgun()
{
	m_nPaintType = BOUNCE_POWER;
	m_flNextFireSound = 0.f;
	m_flNextPaintAnim = 0.f;
}

void CTFWeaponPaintgun::ApplyColor( void )
{
	int r, g, b;
	TFPaint_GetColor( m_nPaintType, r, g, b );
	SetRenderColor( r, g, b, 255 );

#ifdef CLIENT_DLL
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		C_BaseViewModel *pVM = pOwner->GetViewModel( 0 );
		if ( pVM )
			pVM->SetRenderColor( r, g, b, 255 );
	}
#endif
}

#ifdef CLIENT_DLL
void CTFWeaponPaintgun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	ApplyColor();
}
#endif

void CTFWeaponPaintgun::CyclePaint( void )
{
	static const int s_cycle[] =
	{
		BOUNCE_POWER,
		SPEED_POWER,
		PORTAL_POWER,
		FIFTH_POWER,
	};

	int next = s_cycle[0];
	for ( int i = 0; i < ARRAYSIZE( s_cycle ); ++i )
	{
		if ( s_cycle[i] == m_nPaintType )
		{
			next = s_cycle[( i + 1 ) % ARRAYSIZE( s_cycle )];
			break;
		}
	}
	m_nPaintType = next;
	ApplyColor();

#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
		ClientPrint( pOwner, HUD_PRINTCENTER, TFPaint_Name( m_nPaintType ) );
#endif
}

#ifdef GAME_DLL
static void FirePaintBlob( CTFPlayer *pOwner, int type, float flCoatLife = 0.f )
{
	if ( !pOwner )
		return;

	Vector forward, right, up;
	AngleVectors( pOwner->EyeAngles(), &forward, &right, &up );
	Vector eye = pOwner->EyePosition();
	eye += forward * tf_paintgun_offset_y.GetFloat();
	eye += right * tf_paintgun_offset_x.GetFloat();
	eye += up * tf_paintgun_offset_z.GetFloat();

	if ( !tf_paintgun_fire_blobs.GetBool() )
	{
		trace_t tr;
		CTraceFilterSimple filter( pOwner, COLLISION_GROUP_NONE );
		UTIL_TraceLine( pOwner->EyePosition(), pOwner->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SOLID, &filter, &tr );
		if ( tr.fraction < 1.f && !( tr.surface.flags & SURF_SKY ) )
		{
			float rad = tf_paintgun_rad.GetFloat();
			if ( type == NO_POWER )
				TFPaint_EraseSphere( tr.endpos, rad );
			else
				TFPaint_PaintSphere( tr.endpos, tr.plane.normal, type, rad, flCoatLife );
		}
		return;
	}

	float spread = sinf( DEG2RAD( tf_paintgun_spread.GetFloat() ) );
	Vector dir = forward;
	dir += right * RandomFloat( -spread, spread );
	dir += up * RandomFloat( -spread, spread );
	VectorNormalize( dir );

	float speed = RandomFloat( tf_paintgun_min_speed.GetFloat(), tf_paintgun_max_speed.GetFloat() );
	TFPaint_AddBlob( eye, dir * speed, type, pOwner, flCoatLife );
}
#endif

void CTFWeaponPaintgun::FireBlob( bool bErase )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	int type = bErase ? NO_POWER : m_nPaintType.Get();
	FirePaintBlob( pOwner, type );

	if ( gpGlobals->curtime >= m_flNextFireSound )
	{
		pOwner->EmitSound( "Paintgun.Fire" );
		m_flNextFireSound = gpGlobals->curtime + 0.12f;
	}
#else
	(void)bErase;
#endif
}

void CTFWeaponPaintgun::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

	float flRate = tf_paintgun_blobs_per_second.GetFloat();
	if ( flRate < 1.f )
		flRate = 1.f;
	m_flNextPrimaryAttack = gpGlobals->curtime + ( 1.0f / flRate );
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	FireBlob( false );
	if ( gpGlobals->curtime >= m_flNextPaintAnim )
	{
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner )
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		m_flNextPaintAnim = gpGlobals->curtime + 0.15f;
	}
}

void CTFWeaponPaintgun::SecondaryAttack( void )
{
	if ( !CanAttack() )
		return;

	float flRate = tf_paintgun_blobs_per_second.GetFloat();
	if ( flRate < 1.f )
		flRate = 1.f;
	m_flNextSecondaryAttack = gpGlobals->curtime + ( 1.0f / flRate );

	FireBlob( true );
}

void CTFWeaponPaintgun::ItemPostFrame( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && ( pOwner->m_afButtonPressed & IN_RELOAD ) )
		CyclePaint();

	BaseClass::ItemPostFrame();
}

#ifdef GAME_DLL
static void CC_GivePaintgun( const CCommand &args )
{
	(void)args;
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	if ( CBaseCombatWeapon *pHave = pPlayer->Weapon_OwnsThisType( "tf_weapon_paintgun" ) )
	{
		pPlayer->Weapon_Switch( pHave );
		return;
	}

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>( CreateEntityByName( "tf_weapon_paintgun" ) );
	if ( !pWeapon )
	{
		Msg( "Couldn't create tf_weapon_paintgun\n" );
		return;
	}

	DispatchSpawn( pWeapon );
	pPlayer->Weapon_Equip( pWeapon );
	pPlayer->Weapon_Switch( pWeapon );
	ClientPrint( pPlayer, HUD_PRINTCENTER, "Repulsion gel" );
}
ConCommand tf_give_paintgun( "tf_give_paintgun", CC_GivePaintgun, "Equip the gel gun. Attack paints, attack2 erases, reload cycles gels." );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponGreasegun, DT_WeaponGreasegun )

BEGIN_NETWORK_TABLE( CTFWeaponGreasegun, DT_WeaponGreasegun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponGreasegun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_greasegun, CTFWeaponGreasegun );
PRECACHE_WEAPON_REGISTER( tf_weapon_greasegun );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponGreasegun )
END_DATADESC()
#endif

CTFWeaponGreasegun::CTFWeaponGreasegun()
{
	m_flNextPaintAnim = 0.f;
}

void CTFWeaponGreasegun::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_iPrimaryAmmoType < 0 || pOwner->GetAmmoCount( m_iPrimaryAmmoType ) < 1 )
	{
		WeaponSound( EMPTY );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
		return;
	}

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	float flDelay = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay;
	if ( flDelay < 0.05f )
		flDelay = 0.05f;
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

#ifdef GAME_DLL
	FirePaintBlob( pOwner, SPEED_POWER, tf_paint_grease_lifetime.GetFloat() );
#endif
	WeaponSound( SINGLE );

	if ( gpGlobals->curtime >= m_flNextPaintAnim )
	{
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		m_flNextPaintAnim = gpGlobals->curtime + 0.15f;
	}
}

//=============================================================================
// The Helper. One carried pickup. The hook is the grappling-hook model.
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponHelper, DT_WeaponHelper )

BEGIN_NETWORK_TABLE( CTFWeaponHelper, DT_WeaponHelper )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCarrying ) ),
#else
	SendPropBool( SENDINFO( m_bCarrying ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponHelper )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bCarrying, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_helper, CTFWeaponHelper );
PRECACHE_WEAPON_REGISTER( tf_weapon_helper );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponHelper )
END_DATADESC()
#endif

static const Vector s_vecHelperCarryOffset( 28.f, 12.f, 16.f );

CTFWeaponHelper::CTFWeaponHelper()
{
	m_bCarrying = false;
}

void CTFWeaponHelper::Precache( void )
{
	BaseClass::Precache();
#ifdef GAME_DLL
	UTIL_PrecacheOther( "tf_projectile_helperhook" );
#endif
}

void CTFWeaponHelper::ItemPostFrame( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

#ifdef GAME_DLL
	if ( m_bCarrying && !m_hCarried.Get() )
		m_bCarrying = false;
#endif

	if ( ( pOwner->m_nButtons & IN_ATTACK ) && m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		PrimaryAttack();
		return;
	}

	WeaponIdle();
}

void CTFWeaponHelper::PrimaryAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner || !CanAttack() )
		return;

	float flDelay = GetTFWpnData().m_WeaponData[ TF_WEAPON_PRIMARY_MODE ].m_flTimeFireDelay;
	if ( flDelay < 0.05f )
		flDelay = 0.5f;
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;

#ifdef GAME_DLL
	if ( m_bCarrying )
		ThrowCarried();
	else if ( !m_hHook.Get() )
		FireHook();
#endif

	WeaponSound( SINGLE );
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

bool CTFWeaponHelper::CanHolster( void ) const
{
	if ( m_bCarrying )
		return false;

	return BaseClass::CanHolster();
}

bool CTFWeaponHelper::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bCarrying )
		return false;

#ifdef GAME_DLL
	if ( m_hHook.Get() )
	{
		CBaseEntity *pHook = m_hHook;
		m_hHook = NULL;
		UTIL_Remove( pHook );
	}
#endif

	return BaseClass::Holster( pSwitchingTo );
}

void CTFWeaponHelper::UpdateOnRemove( void )
{
#ifdef GAME_DLL
	DropCarried( Vector( 0.f, 0.f, 80.f ) );

	if ( m_hHook.Get() )
	{
		CBaseEntity *pHook = m_hHook;
		m_hHook = NULL;
		UTIL_Remove( pHook );
	}
#endif

	BaseClass::UpdateOnRemove();
}

#ifdef GAME_DLL
void CTFWeaponHelper::FireHook( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	Vector vecForward;
	AngleVectors( pOwner->EyeAngles(), &vecForward );
	Vector vecSrc = pOwner->Weapon_ShootPosition();

	m_hHook = CTFProjectile_HelperHook::Create( pOwner, this, vecSrc, vecForward );
}

void CTFWeaponHelper::ReceivePickup( CBaseEntity *pPickup )
{
	CTFPowerup *pPower = dynamic_cast< CTFPowerup * >( pPickup );
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pPower || !pOwner )
	{
		if ( pPower )
		{
			Vector vecDrop( 0.f, 0.f, 80.f );
			pPower->SetParent( NULL );
			pPower->DropSingleInstance( vecDrop, pOwner, 0.3f );
		}
		return;
	}

	if ( m_hCarried.Get() && m_hCarried != pPower )
	{
		Vector vecDrop( 0.f, 0.f, 80.f );
		pPower->SetParent( NULL );
		pPower->DropSingleInstance( vecDrop, pOwner, 0.3f );
		return;
	}

	pPower->SetParent( NULL );
	pPower->SetParent( pOwner );
	pPower->SetLocalOrigin( s_vecHelperCarryOffset );
	pPower->SetLocalAngles( vec3_angle );
	pPower->SetSolid( SOLID_NONE );
	pPower->SetMoveType( MOVETYPE_NONE );
	pPower->SetAbsVelocity( vec3_origin );
	m_hCarried = pPower;
	m_bCarrying = true;
}

void CTFWeaponHelper::ClearHook( CBaseEntity *pHook )
{
	if ( m_hHook == pHook )
		m_hHook = NULL;
}

void CTFWeaponHelper::ThrowCarried( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	Vector vecForward;
	QAngle angThrow = pOwner->EyeAngles() + QAngle( -10.f, 0.f, 0.f );
	AngleVectors( angThrow, &vecForward );
	DropCarried( vecForward * 500.f );
}

void CTFWeaponHelper::DropCarried( const Vector &vecVelocity )
{
	CTFPowerup *pPower = dynamic_cast< CTFPowerup * >( m_hCarried.Get() );
	m_hCarried = NULL;
	m_bCarrying = false;
	if ( !pPower )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	pPower->SetParent( NULL );
	if ( pOwner )
	{
		QAngle angThrow = pOwner->EyeAngles() + QAngle( -10.f, 0.f, 0.f );
		pPower->SetAbsOrigin( pOwner->EyePosition() + Vector( 0.f, 0.f, -8.f ) );
		pPower->SetAbsAngles( angThrow );
	}

	Vector vecVel = vecVelocity;
	pPower->DropSingleInstance( vecVel, pOwner, 0.3f );
}
#endif
