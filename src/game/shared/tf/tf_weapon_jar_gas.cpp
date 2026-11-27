//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_jar_gas.h"
#include "tf_chemist.h"
#include "tf_paint.h"
#include "weapon_parse.h"
#include "decals.h"
#include "debugoverlay_shared.h"
#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "dt_utlvector_recv.h"
#else
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "func_break.h"
#include "func_nogrenades.h"
#include "Sprite.h"
#include "tf_fx.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "bone_setup.h"
#include "tf_flame.h"
#include "dt_utlvector_send.h"
#include "collisionutils.h"
#include "recipientfilter.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFJarGas, DT_TFWeaponJarGas )

BEGIN_NETWORK_TABLE( CTFJarGas, DT_TFWeaponJarGas )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_jar_gas, CTFJarGas );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar_gas );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_JarGas, DT_TFProjectile_JarGas )
BEGIN_NETWORK_TABLE( CTFProjectile_JarGas, DT_TFProjectile_JarGas )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_jar_gas, CTFProjectile_JarGas );
PRECACHE_WEAPON_REGISTER( tf_projectile_jar_gas );

CTFProjectile_JarGas::CTFProjectile_JarGas()
{
	m_bPoison = false;
}


IMPLEMENT_NETWORKCLASS_ALIASED( TFGasManager, DT_TFGasManager );

BEGIN_NETWORK_TABLE( CTFGasManager, DT_TFGasManager )
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFGasManager )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_gas_manager, CTFGasManager );

#ifdef CLIENT_DLL
gas_particle_t::gas_particle_t()
{
	m_nUniqueID = 0;
	m_hParticleEffect = NULL;
	m_hParticleOwner = NULL;
}

gas_particle_t::~gas_particle_t()
{
	Assert( m_hParticleOwner );
	if ( m_hParticleEffect.GetObject() && m_hParticleOwner )
	{
		m_hParticleOwner->ParticleProp()->StopEmission( m_hParticleEffect.GetObject() );
	}
}
#endif // CLIENT_DLL


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar *CTFJarGas::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	RemoveJarGas( pOwner );
	return CTFProjectile_JarGas::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJarGas::GetAfterburnRateOnHit() const
{
	return TF_GAS_AFTERBURN_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJarGas::OnResourceMeterFilled()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	pOwner->GiveAmmo( 1, m_iPrimaryAmmoType, false, kAmmoSource_ResourceMeter );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJarGas::GetProjectileSpeed( void )
{ 
	return TF_GAS_PROJ_SPEED;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarGas::Precache()
{
	PrecacheModel( TF_WEAPON_JAR_GAS_JAR_MODEL );
	PrecacheModel( TF_WEAPON_CHEMIST_VIAL_MODEL );
	PrecacheParticleSystem( "gas_can_blue" );
	PrecacheParticleSystem( "gas_can_red" );
	PrecacheScriptSound( TF_WEAPON_JARGAS_EXPLODE_SOUND );

	BaseClass::Precache();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_JarGas* CTFProjectile_JarGas::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_JarGas *pGrenade = static_cast< CTFProjectile_JarGas* >( CBaseEntity::CreateNoSpawn( "tf_projectile_jar_gas", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
		pGrenade->m_flFullDamage = 0;
		pGrenade->m_bPoison = false;
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarGas::SetCustomPipebombModel()
{
	SetModel( TF_WEAPON_JAR_GAS_JAR_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarGas::Explode( trace_t *pTrace, int bitsDamageType )
{
	if ( m_takedamage == DAMAGE_NO )
		return;

	BaseClass::Explode( pTrace, bitsDamageType );

	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CTFPlayer *pThrower = ToTFPlayer( GetThrower() );
	if ( pThrower && !m_bPoison )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		Vector vecManagerPos = vecAbsOrigin + Vector( 0, 0, TF_GAS_POINT_RADIUS * 1.2 );

		trace_t	tr;
		UTIL_TraceLine( vecAbsOrigin, vecManagerPos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0f )
		{
			vecManagerPos = vecAbsOrigin;
		}
		CTFGasManager *pGasManager = CTFGasManager::Create( pThrower, vecManagerPos );
		if ( pGasManager )
			pGasManager->AddGas();
	}
	else if ( m_bPoison )
	{
		if ( pThrower )
		{
			Vector vecAbsOrigin = GetAbsOrigin();
			Vector vecManagerPos = vecAbsOrigin + Vector( 0, 0, TF_GAS_POINT_RADIUS * 1.2 );

			trace_t	tr;
			UTIL_TraceLine( vecAbsOrigin, vecManagerPos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0f )
			{
				vecManagerPos = vecAbsOrigin;
			}
			CTFPoisonGas *pPoison = CTFPoisonGas::Create( pThrower, vecManagerPos );
			if ( pPoison )
				pPoison->AddGas();
		}

		Vector normal = ( pTrace->fraction != 1.0f ) ? pTrace->plane.normal : Vector( 0, 0, 1 );
		CPVSFilter particleFilter( GetAbsOrigin() );
		TE_TFParticleEffect( particleFilter, 0.0f, GetImpactEffect(), GetAbsOrigin(), vec3_angle );
		CBroadcastRecipientFilter soundFilter;
		Vector vecOrigin = GetAbsOrigin();
		EmitSound( soundFilter, entindex(), GetExplodeSound(), &vecOrigin );
		Chemist_Impact( pThrower, GetAbsOrigin(), normal, CHEM_POISON );
	}

	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
}
#endif // GAME_DLL

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFJarGas::ModifyEventParticles( const char* token )
{
	if ( FStrEq( token, "energydrink_splash" ) )
	{
		CEconItemView *pItem = m_AttributeManager.GetItem();
		int iSystems = pItem->GetStaticData()->GetNumAttachedParticles( GetTeamNumber() );
		for ( int i = 0; i < iSystems; i++ )
		{
			attachedparticlesystem_t *pSystem = pItem->GetStaticData()->GetAttachedParticleData( GetTeamNumber(), i );
			if ( pSystem->iCustomType == 1 )
			{
				return pSystem->pszSystemName;
			}
		}
	}

	return BaseClass::ModifyEventParticles( token );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFJarGas::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->IsLocalPlayer() )
	{
		C_BaseEntity *pParticleEnt = pOwner->GetViewModel( 0 );
		if ( pParticleEnt )
		{
			pOwner->StopViewModelParticles( pParticleEnt );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJarGas::RemoveJarGas( CBaseCombatCharacter *pOwner )
{
	{
		CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
		if ( pTFOwner )
		{
			pTFOwner->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 0.f );
			pTFOwner->RemoveAmmo( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJarGas::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFJarGas::CanAttack()
{
	{
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			if ( pOwner->m_Shared.GetItemChargeMeter( LOADOUT_POSITION_SECONDARY ) < 100.f )
				return false;
		}
	}

	return BaseClass::CanAttack();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFJarGas::ShouldUpdateMeter() const
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
		return pOwner->IsAlive();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGasManager::CTFGasManager()
{
	m_flLastMoveUpdate = -1.f;
	m_bKeepMovingPoints = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	m_hGasParticleEffects.RemoveAll();
#endif // CLIENT_DLL

	BaseClass::UpdateOnRemove();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGasManager* CTFGasManager::Create( CBaseEntity *pOwner, const Vector& vPos )
{
	CTFGasManager *pGasManager = static_cast< CTFGasManager* >( CBaseEntity::Create( "tf_gas_manager", vPos, vec3_angle, pOwner ) );
	if ( pGasManager )
	{
		// Initialize the owner.
		pGasManager->SetOwnerEntity( pOwner );

		// Set team.
		pGasManager->ChangeTeam( pOwner->GetTeamNumber() );
	}

	return pGasManager;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::AddGas()
{
	int iCurrentTick = TIME_TO_TICKS( gpGlobals->curtime );
	while ( CanAddPoint() )
	{
		if ( !AddPoint( iCurrentTick ) )
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGasManager::ShouldCollide( CBaseEntity *pEnt ) const
{
	if ( !pEnt->IsPlayer() )
		return false;

	if ( pEnt->GetTeamNumber() == GetTeamNumber() )
		return false;

	if ( TFGameRules() && TFGameRules()->IsTruceActive() )
		return false;

	if ( m_Touched.Find( pEnt ) != m_Touched.InvalidIndex() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::OnCollide( CBaseEntity *pEnt, int iPointIndex )
{

	CTFPlayer *pTFPlayer = assert_cast< CTFPlayer* >( pEnt );
	// add the condition if we can
	if ( !pTFPlayer->m_Shared.IsInvulnerable() && !pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) && !pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) && pTFPlayer->CanGetWet() )
	{
		pTFPlayer->m_Shared.AddCond( TF_COND_GAS, 10.f, GetOwnerEntity() );
	}

	m_Touched.AddToTail( pTFPlayer );
}

#endif // GAME_DLL

IMPLEMENT_NETWORKCLASS_ALIASED( TFPoisonGas, DT_TFPoisonGas );

BEGIN_NETWORK_TABLE( CTFPoisonGas, DT_TFPoisonGas )
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFPoisonGas )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_poison_gas, CTFPoisonGas );

#ifdef GAME_DLL
CTFPoisonGas *CTFPoisonGas::Create( CBaseEntity *pOwner, const Vector &vPos )
{
	CTFPoisonGas *pGas = static_cast< CTFPoisonGas * >( CBaseEntity::Create( "tf_poison_gas", vPos, vec3_angle, pOwner ) );
	if ( pGas )
	{
		pGas->SetOwnerEntity( pOwner );
		pGas->ChangeTeam( pOwner->GetTeamNumber() );
	}
	return pGas;
}

bool CTFPoisonGas::ShouldCollide( CBaseEntity *pEnt ) const
{
	if ( !pEnt->IsPlayer() )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( pEnt );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;
	if ( pPlayer->IsPlayerClass( TF_CLASS_PYRO ) )
		return false;
	if ( pPlayer->GetTeamNumber() == GetTeamNumber() )
		return false;
	if ( TFGameRules() && TFGameRules()->IsTruceActive() )
		return false;
	return true;
}

void CTFPoisonGas::OnCollide( CBaseEntity *pEnt, int iPointIndex )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEnt );
	if ( !pPlayer )
		return;
	if ( pPlayer->m_Shared.IsInvulnerable() || pPlayer->m_Shared.InCond( TF_COND_PHASE ) || !pPlayer->CanGetWet() )
		return;

	// Refresh the duration. Do not reset the next damage tick, so it cannot stack.
	pPlayer->m_Shared.ApplyPoison( ToTFPlayer( GetOwnerEntity() ), 5.0f );
	(void)iPointIndex;
}

static bool Chemist_Blocked( const Vector &from, const Vector &to )
{
	// Fences and other $compilepassbullets brushes are CONTENTS_GRATE. The splash
	// coats and paints through them, the same way a bullet would.
	return TFPaint_SplashBlocked( from, to );
}

static float Chemist_DistSqr( CBaseEntity *pEnt, const Vector &pos )
{
	const float a = pEnt->GetAbsOrigin().DistToSqr( pos );
	const float b = pEnt->WorldSpaceCenter().DistToSqr( pos );
	return ( a < b ) ? a : b;
}

static void Chemist_SplashNeutral( CTFPlayer *pPlayer, const Vector &pos )
{
	TFPaint_EraseCoatsNear( pos, 160.f );
	if ( !pPlayer )
		return;

	const Vector src = pos + Vector( 0.f, 0.f, 8.f );
	const float r2 = 160.f * 160.f;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pAlly = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pAlly || !pAlly->IsAlive() )
			continue;
		if ( pAlly->GetTeamNumber() != pPlayer->GetTeamNumber() )
			continue;
		if ( Chemist_DistSqr( pAlly, pos ) > r2 )
			continue;
		if ( Chemist_Blocked( src, pAlly->WorldSpaceCenter() ) )
			continue;

		pAlly->m_Shared.RemoveCond( TF_COND_BURNING );
		pAlly->m_Shared.RemoveCond( TF_COND_URINE );
		pAlly->m_Shared.RemoveCond( TF_COND_MAD_MILK );
		pAlly->m_Shared.RemoveCond( TF_COND_GAS );
		pAlly->m_Shared.ClearPoison();
	}

	// The splash still hits enemies. They get wet. They are not cleansed.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pEnemy = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pEnemy || !pEnemy->IsAlive() )
			continue;
		if ( pPlayer && pEnemy->GetTeamNumber() == pPlayer->GetTeamNumber() )
			continue;
		if ( Chemist_DistSqr( pEnemy, pos ) > r2 )
			continue;
		if ( Chemist_Blocked( src, pEnemy->WorldSpaceCenter() ) )
			continue;
		pEnemy->m_Shared.AddWet( 10.f );
	}
}

void Chemist_Impact( CTFPlayer *pPlayer, const Vector &pos, const Vector &normal, int slot )
{
	Vector n = normal;
	if ( n.LengthSqr() < 0.01f )
		n.Init( 0.f, 0.f, 1.f );
	VectorNormalize( n );

	if ( slot == CHEM_REPULSION )
		TFPaint_AddTimedCoat( pos, n, BOUNCE_POWER, 80.f, 30.f, false, pPlayer );
	else if ( slot == CHEM_NITRIC )
		TFPaint_AddTimedCoat( pos, n, FIFTH_POWER, 80.f, TF_GAS_LIFETIME, true, pPlayer );
	else if ( slot == CHEM_NEUTRAL )
	{
		Chemist_SplashNeutral( pPlayer, pos );
		return;
	}
	else if ( slot != CHEM_POISON )
	{
		return;
	}

	if ( !pPlayer )
		return;
	if ( TFGameRules() && TFGameRules()->IsTruceActive() )
		return;

	const float radius = ( slot == CHEM_POISON ) ? 100.f : 80.f;
	const float r2 = radius * radius;
	const Vector src = pos + n * 8.f;
	const int team = pPlayer->GetTeamNumber();

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pEnemy = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pEnemy || !pEnemy->IsAlive() || pEnemy == pPlayer )
			continue;
		if ( pEnemy->GetTeamNumber() == team )
			continue;
		if ( Chemist_DistSqr( pEnemy, pos ) > r2 )
			continue;
		if ( Chemist_Blocked( src, pEnemy->WorldSpaceCenter() ) )
			continue;

		if ( slot == CHEM_POISON )
		{
			if ( pEnemy->m_Shared.IsInvulnerable() || pEnemy->m_Shared.InCond( TF_COND_PHASE ) || !pEnemy->CanGetWet() )
				continue;
			pEnemy->m_Shared.ApplyPoison( pPlayer, 5.0f );
		}
		else if ( slot == CHEM_NITRIC )
		{
			if ( pEnemy->m_Shared.IsInvulnerable() || pEnemy->m_Shared.InCond( TF_COND_PHASE ) )
				continue;
			// Coated like the gas passer (10s). A later weapon hit lights them.
			pEnemy->m_Shared.ApplyNitric( pPlayer, 10.f );
		}
		else if ( slot == CHEM_REPULSION )
		{
			TFPaint_CoatWithRepulsion( pEnemy, pPlayer );
			pEnemy->m_Shared.AddWet( 10.f );
			Vector vel = pEnemy->GetAbsVelocity();
			if ( vel.z < 300.f )
			{
				vel.z += 450.f;
				pEnemy->SetAbsVelocity( vel );
				pEnemy->SetGroundEntity( NULL );
			}
		}
	}

	if ( slot != CHEM_REPULSION )
		return;

	for ( int o = 0; o < IBaseObjectAutoList::AutoList().Count(); ++o )
	{
		CBaseObject *pObj = static_cast< CBaseObject * >( IBaseObjectAutoList::AutoList()[o] );
		if ( !pObj || !pObj->IsAlive() || pObj->IsPlacing() || pObj->IsCarried() )
			continue;
		if ( pObj->GetTeamNumber() == team )
			continue;
		if ( Chemist_DistSqr( pObj, pos ) > r2 )
			continue;
		if ( Chemist_Blocked( src, pObj->WorldSpaceCenter() ) )
			continue;
		TFPaint_CoatWithRepulsion( pObj, pPlayer );
		pObj->NoteEnemyRepulse( team, gpGlobals->curtime + tf_paint_repulse_ally_time.GetFloat() );
	}

	// Allies are coated, not launched. A friendly Brick keeps the gel for the ally timer.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pAlly = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pAlly || !pAlly->IsAlive() || pAlly == pPlayer )
			continue;
		if ( pAlly->GetTeamNumber() != team )
			continue;
		if ( Chemist_DistSqr( pAlly, pos ) > r2 )
			continue;
		if ( Chemist_Blocked( src, pAlly->WorldSpaceCenter() ) )
			continue;
		TFPaint_CoatWithRepulsion( pAlly, pPlayer );
	}

	CBaseEntity *pProp = NULL;
	while ( ( pProp = gEntList.FindEntityByClassname( pProp, "prop_physics*" ) ) != NULL )
	{
		if ( Chemist_DistSqr( pProp, pos ) > r2 )
			continue;
		if ( Chemist_Blocked( src, pProp->WorldSpaceCenter() ) )
			continue;
		TFPaint_CoatWithRepulsion( pProp, pPlayer );
	}
}

void Chemist_Throw( CTFPlayer *pPlayer, int slot )
{
	if ( !pPlayer )
		return;
	if ( slot < 0 || slot >= CHEM_COUNT )
		return;

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );
	const Vector vecEye = pPlayer->EyePosition();

	const char *pszWeapon = ( slot == CHEM_POISON ) ? "tf_weapon_jar_gas" : "tf_weapon_jar";
	WEAPON_FILE_INFO_HANDLE hWpn = LookupWeaponInfoSlot( pszWeapon );
	if ( hWpn == GetInvalidWeaponInfoHandle() )
		return;
	CTFWeaponInfo *pInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( hWpn ) );
	if ( !pInfo )
		return;

	Vector vecSrc = vecEye + vecForward * 16.f + vecRight * 8.f + vecUp * -6.f;
	trace_t trace;
	CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecEye, vecSrc, -Vector( 8, 8, 8 ), Vector( 8, 8, 8 ), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );
	if ( trace.startsolid )
		vecSrc = vecEye;
	else
		vecSrc = trace.endpos;

	AngularImpulse ang( 300, 0, 0 );
	if ( slot == CHEM_POISON )
	{
		const Vector vecVel = vecForward * TF_GAS_PROJ_SPEED;
		CTFProjectile_JarGas *pProj = CTFProjectile_JarGas::Create( vecSrc, pPlayer->EyeAngles(), vecVel, ang, pPlayer, *pInfo );
		if ( pProj )
		{
			pProj->m_bPoison = true;
			pProj->SetChemSlot( CHEM_POISON );
			pProj->SetModel( TF_WEAPON_CHEMIST_VIAL_MODEL );
			pProj->m_nSkin = 0;
		}
		return;
	}

	// Same toss as a stock jarate jar.
	Vector vecVel = ( vecForward * 1000.f ) + ( vecUp * 200.f )
		+ ( random->RandomFloat( -10.f, 10.f ) * vecRight )
		+ ( random->RandomFloat( -10.f, 10.f ) * vecUp );
	CTFProjectile_Jar *pProj = CTFProjectile_Jar::Create( vecSrc, pPlayer->EyeAngles(), vecVel, ang, pPlayer, *pInfo );
	if ( pProj )
	{
		pProj->SetChemSlot( slot );
		pProj->SetModel( TF_WEAPON_CHEMIST_VIAL_MODEL );
		pProj->m_nSkin = 0;
	}
}

void Chemist_AirblastDispel( CTFPlayer *pPyro )
{
	if ( !pPyro )
		return;

	Vector vecForward;
	AngleVectors( pPyro->EyeAngles(), &vecForward );
	const Vector vecStart = pPyro->EyePosition();
	const Vector vecEnd = vecStart + vecForward * 280.f;

	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_poison_gas" ) ) != NULL )
	{
		Vector vecDelta = pEnt->WorldSpaceCenter() - vecStart;
		if ( vecDelta.LengthSqr() > 280.f * 280.f )
			continue;
		Vector vecDir = vecDelta;
		VectorNormalize( vecDir );
		if ( DotProduct( vecDir, vecForward ) < 0.5f )
			continue;
		UTIL_Remove( pEnt );
	}

	TFPaint_IgniteNitricSegment( vecStart, vecEnd );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::Update()
{

	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 )

	bool bUpdatePoints = ( GetPointVec().Count() > 0 );

	// remove any gas points that shouldn't be included anymore
	FOR_EACH_VEC_BACK( GetPointVec(), i )
	{
		bool bShouldRemove = false;
		// expired
		if ( gpGlobals->curtime > GetPointVec()[i]->m_flSpawnTime + GetPointVec()[i]->m_flLifeTime )
		{
			bShouldRemove = true;
		}

		// in water?
		int nContents = UTIL_PointContents( GetPointVec()[i]->m_vecPosition );
		if ( ( nContents & MASK_WATER ) )
		{
			bShouldRemove = true;
		}

		if ( bShouldRemove )
		{
			RemovePoint( i );
		}
	}

	if ( GetPointVec().Count() <= 0 )
	{
		if ( bUpdatePoints )
		{
			SetContextThink( &CTFGasManager::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
		}

		return;
	}

	// store the previous position
	FOR_EACH_VEC( GetPointVec(), i )
	{
		GetPointVec()[i]->m_vecPrevPosition = GetPointVec()[i]->m_vecPosition;
	}

	float flMinDistanceApart = ( TF_GAS_POINT_RADIUS * 1.9f );
	if ( m_flLastMoveUpdate <= 1.f )
	{
		m_flLastMoveUpdate = gpGlobals->curtime;
	}
	float flDelta = gpGlobals->curtime - m_flLastMoveUpdate;
	m_flLastMoveUpdate = gpGlobals->curtime;

	bool bAnyoneMoved = false;

	if ( m_bKeepMovingPoints )
	{
		// move them down
		FOR_EACH_VEC( GetPointVec(), i )
		{
			Vector vecDownDir = flDelta * Vector( 0.f, 0.f, -TF_GAS_POINT_RADIUS );

			trace_t	tr;
			Vector vecPos = GetPointVec()[i]->m_vecPosition + Vector( 0.f, 0.f, -TF_GAS_POINT_RADIUS ); // bottom of the sphere
			UTIL_TraceLine( vecPos, vecPos + vecDownDir, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.f )
				continue;

			GetPointVec()[i]->m_vecPosition += ( vecDownDir * TF_GAS_FALLRATE );
			bAnyoneMoved = true;
		}

		// now have them separate
		FOR_EACH_VEC( GetPointVec(), point1 )
		{
			int nNeighborCount = 0;
			Vector vecResult( 0, 0, 0 );

			FOR_EACH_VEC( GetPointVec(), point2 )
			{
				if ( point1 == point2 )
					continue;

				Vector vecDist = GetPointVec()[point2]->m_vecPosition - GetPointVec()[point1]->m_vecPosition;
				if ( vecDist.Length() < flMinDistanceApart )
				{
					vecResult.x += ( GetPointVec()[point2]->m_vecPosition.x - GetPointVec()[point1]->m_vecPosition.x );
					vecResult.y += ( GetPointVec()[point2]->m_vecPosition.y - GetPointVec()[point1]->m_vecPosition.y );
					vecResult.z += ( GetPointVec()[point2]->m_vecPosition.z - GetPointVec()[point1]->m_vecPosition.z );

					nNeighborCount++;
				}
			}

			if ( nNeighborCount > 0 )
			{
				vecResult.x /= nNeighborCount;
				vecResult.y /= nNeighborCount;
				vecResult.z /= nNeighborCount;

				Vector vecNewDir = vecResult.Normalized();
				vecNewDir *= ( TF_GAS_MOVE_DISTANCE * -1.f );

				// can we move there?
				trace_t	tr;
				UTIL_TraceLine( GetPointVec()[point1]->m_vecPosition, GetPointVec()[point1]->m_vecPosition + vecNewDir, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction != 1.f )
					continue;

				GetPointVec()[point1]->m_vecPosition += vecNewDir;
				bAnyoneMoved = true;
			}
		}
	}

// 	if ( bAnyoneMoved )
// 	{
// 		FOR_EACH_VEC( GetPointVec(), i )
// 		{
// #ifdef GAME_DLL
// 			DevMsg( "SERVER MOVE: index [%d], pos[ %f %f %f ]\n", i, XYZ( GetPointVec()[i]->m_vecPosition ) );
// #else
// 			DevMsg( "CLIENT MOVE: index [%d], pos[ %f %f %f ]\n", i, XYZ( GetPointVec()[i]->m_vecPosition ) );
// #endif
// 		}
// 	}

	m_bKeepMovingPoints = bAnyoneMoved;
#ifdef GAME_DLL
	// update bounds if necessary
	if ( bAnyoneMoved )
	{
		Vector vHullMin( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
		Vector vHullMax( MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT );
		FOR_EACH_VEC( GetPointVec(), i )
		{
			tf_point_t *pPoint = GetPointVec()[i];
			float flRadius = GetRadius( pPoint );
			Vector vExtent( flRadius, flRadius, flRadius );
			VectorMin( vHullMin, pPoint->m_vecPosition - vExtent, vHullMin );
			VectorMax( vHullMax, pPoint->m_vecPosition + vExtent, vHullMax );
		}

		Vector vExtent = 0.5f * ( vHullMax - vHullMin );
		Vector vOrigin = vHullMin + vExtent;
		SetAbsOrigin( vOrigin );
		UTIL_SetSize( this, -vExtent, vExtent );
	}
#endif // GAME_DLL

#ifdef CLIENT_DLL
	// clean up gas that has gone away
	FOR_EACH_VEC_BACK( m_hGasParticleEffects, nIndex )
	{
		int nUniqueID = m_hGasParticleEffects[nIndex].m_nUniqueID;
		bool bFound = false;
		FOR_EACH_VEC( GetPointVec(), i )
		{
			if ( GetPointVec()[i]->m_nPointIndex == nUniqueID )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			m_hGasParticleEffects.Remove( nIndex );
		}
	}

	// add/update gas that's still around
	FOR_EACH_VEC( GetPointVec(), i )
	{
		int iFoundIndex = -1;
		FOR_EACH_VEC_BACK( m_hGasParticleEffects, iIndex )
		{
			if ( m_hGasParticleEffects[iIndex].m_nUniqueID == GetPointVec()[i]->m_nPointIndex )
			{
				iFoundIndex = iIndex;
				break;
			}
		}

		if ( iFoundIndex == -1 )
		{
			// create a new effect
			CSmartPtr<CNewParticleEffect> pEffect = ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_BLUE ) ? "gas_can_blue" : "gas_can_red", PATTACH_CUSTOMORIGIN, 0 );
			if ( pEffect.IsValid() && pEffect->IsValid() )
			{
				pEffect->SetControlPoint( 0, GetPointVec()[i]->m_vecPosition );

				int nNewIndex = m_hGasParticleEffects.AddToTail();
				m_hGasParticleEffects[nNewIndex].m_hParticleOwner = this;
				m_hGasParticleEffects[nNewIndex].m_hParticleEffect = pEffect;
				m_hGasParticleEffects[nNewIndex].m_nUniqueID = GetPointVec()[i]->m_nPointIndex;
			}
		}
		else
		{
			m_hGasParticleEffects[iFoundIndex].m_hParticleEffect->SetControlPoint( 0, GetPointVec()[i]->m_vecPosition );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CTFGasManager::GetInitialPosition() const
{
	float flDistance = TF_GAS_POINT_RADIUS;

#ifdef GAME_DLL
	Vector vecOrigin = GetAbsOrigin();
#else
	Vector vecOrigin = GetNetworkOrigin();
#endif

	return ( vecOrigin + Vector( m_randomStream.RandomFloat( flDistance * -1.f, flDistance ), m_randomStream.RandomFloat( flDistance * -1.f, flDistance ), 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGasManager::OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT )
{
	// set pos to some offset from the surface
	vecNewPos = tr.endpos + ( ( GetRadius( pPoint ) + 5 ) * tr.plane.normal );
	vecNewVelocity = pPoint->m_vecVelocity;

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		BaseClass::PostDataUpdate( updateType );
		return;
	}

	// intentionally skip the BaseClass version after we're created
	CBaseEntity::PostDataUpdate( updateType );
}
#endif // CLIENT_DLL
