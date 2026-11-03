//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_buff_item.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_baseentity.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "particle_parse.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Weapon Buff Item tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBuffItem, DT_TFWeaponBuffItem )

BEGIN_NETWORK_TABLE( CTFBuffItem, DT_TFWeaponBuffItem )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBuffItem )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_buff_item, CTFBuffItem );
PRECACHE_WEAPON_REGISTER( tf_weapon_buff_item );

// Models and Buffs need to be aligned
const char* BannerModels[] =
{
	"models/weapons/c_models/c_buffbanner/c_buffbanner.mdl",
	"models/workshop/weapons/c_models/c_battalion_buffbanner/c_battalion_buffbanner.mdl",
	"models/workshop_partner/weapons/c_models/c_shogun_warbanner/c_shogun_warbanner.mdl",
	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_parachute.mdl"
};

#define CLOSED_PARACHUTE_MDL "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack.mdl"
#define OPEN_PARACHUTE_MDL "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack_open.mdl"

COMPILE_TIME_ASSERT( ARRAYSIZE( BannerModels ) == NUM_BUFF_ITEM_TYPES );
//=============================================================================
//
// Weapon Buff Item functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBuffItem::CTFBuffItem()
{
#ifdef CLIENT_DLL
	ListenForGameEvent( "deploy_buff_banner" );
	m_iBuffType = -1;
	m_hBannerEntity = NULL;
#endif
	m_bPlayingHorn = false;

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBuffItem::~CTFBuffItem()
{
#ifdef CLIENT_DLL
	if ( m_hBannerEntity )
	{
		m_hBannerEntity->Release();
		m_hBannerEntity = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBuffItem::Precache()
{
	// Need to make this dynamic, but the item data isn't available here?
	for ( int i=0; i<ARRAYSIZE(BannerModels); i++ )
	{
		PrecacheModel( BannerModels[i] );
	}

	PrecacheModel( "models/weapons/c_models/c_buffpack/c_buffpack.mdl" );
	PrecacheModel( "models/workshop/weapons/c_models/c_battalion_buffpack/c_battalion_buffpack.mdl" );
	PrecacheModel( "models/workshop_partner/weapons/c_models/c_shogun_warpack/c_shogun_warpack.mdl" );
	PrecacheModel( OPEN_PARACHUTE_MDL );
	PrecacheModel( CLOSED_PARACHUTE_MDL );

	PrecacheScriptSound( "Weapon_BuffBanner.HornRed" );
	PrecacheScriptSound( "Weapon_BuffBanner.HornBlue" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornRed" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornBlue" );
	PrecacheScriptSound( "Weapon_BuffBanner.Flag" );
	PrecacheScriptSound( "Samurai.Conch" );

	BaseClass::Precache();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFBuffItem::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.GetRageMeter() < 100.f )
		return;

	if ( m_bPlayingHorn )
		return;

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	SetContextThink( &CTFBuffItem::BlowHorn, gpGlobals->curtime + 0.22f, "BlowHorn" );
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the item to the player.
//-----------------------------------------------------------------------------
void CTFBuffItem::Equip( CBaseCombatCharacter* pOwner )
{
	BaseClass::Equip( pOwner );

	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetParachuteEquipped( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove item from the player.
//-----------------------------------------------------------------------------
void CTFBuffItem::Detach( void )
{
	CTFPlayer *pTFPlayer = GetTFPlayerOwner();
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetParachuteEquipped( false );
	}

	BaseClass::Detach();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::BlowHorn( void )
{
	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
		return;

#ifdef GAME_DLL
	// Use the buff type to try and figure out what sort of mesh we have and what
	// sort of sound to play. This isn't the cleanest thing in the world.
	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );

	// Samurai conch shell.
	if ( iBuffType == EConcheror )
	{
		pPlayer->EmitSound( "Samurai.Conch" );
	}
	// Bugle.
	else if ( iBuffType == EBuffBanner )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pPlayer->EmitSound( "Weapon_BuffBanner.HornBlue" );
		}
		else
		{
			pPlayer->EmitSound( "Weapon_BuffBanner.HornRed" );
		}
	}
	else if ( iBuffType == EBattalion )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pPlayer->EmitSound( "Weapon_BattalionsBackup.HornBlue" );
		}
		else
		{
			pPlayer->EmitSound( "Weapon_BattalionsBackup.HornRed" );
		}
	}

	// Strange Tracking
	if ( !pPlayer->IsBot() )
	{
		EconEntity_OnOwnerKillEaterEventNoPartner( dynamic_cast<CEconEntity *>( this ), pPlayer, kKillEaterEvent_BannersDeployed );
	}
#endif

	m_bPlayingHorn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::RaiseFlag( void )
{
	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
		return;

	m_bPlayingHorn = false;

	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );

#if GAME_DLL
	// this event needs to be sent before we call ActivateSoldierBuff() below
	IGameEvent* event = gameeventmanager->CreateEvent( "deploy_buff_banner" );
	if ( event )
	{
		event->SetInt( "buff_type", iBuffType );
		event->SetInt( "buff_owner", pPlayer->GetUserID() );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->EmitSound( "Weapon_BuffBanner.Flag" );
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
#endif

	pPlayer->m_Shared.ActivateRageBuff( this, iBuffType );
	pPlayer->SelectLastItem();

}

//-----------------------------------------------------------------------------
// Don't let them holster while they are blowing the horn.
//-----------------------------------------------------------------------------
bool CTFBuffItem::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bPlayingHorn )
	{
		return false;
	}
	else
	{
		return BaseClass::Holster( pSwitchingTo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::FireGameEvent( IGameEvent* event )
{
#ifdef CLIENT_DLL
	const char *name = event->GetName();

	if ( 0 == Q_strcmp( name, "deploy_buff_banner" ) )
	{
		CTFPlayer* pPlayer = ToTFPlayer( ClientEntityList().GetEnt( engine->GetPlayerForUserID( event->GetInt( "buff_owner" ) ) ) );
		if ( pPlayer == GetOwner() )
		{
			CreateBanner();
		}
	}

	BaseClass::FireGameEvent( event );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFBuffItem::CreateBanner()
{
#ifdef CLIENT_DLL

	if ( m_hBannerEntity == NULL )
	{
		CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer || !pPlayer->IsAlive() )
			return;

		C_TFBuffBanner* pBanner = new C_TFBuffBanner;
		if ( !pBanner )
			return;

		//Assert( iBuffType > 0 );
		//Assert( iBuffType <= ARRAYSIZE(BannerModels) );
		pBanner->m_nSkin = 0;
		pBanner->InitializeAsClientEntity( BannerModels[GetBuffType()-1], RENDER_GROUP_OPAQUE_ENTITY );
		pBanner->SetBuffItem( this );
		pBanner->SetBuffType( GetBuffType() );
		pBanner->ForceClientSideAnimationOn();
		SetBanner( pBanner );
		int iSpine = pPlayer->LookupBone( "bip_spine_3" );
		Assert( iSpine != -1 );
		if ( iSpine != -1 )
		{
			pBanner->AttachEntityToBone( pPlayer, iSpine );
		}
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
void CTFBuffItem::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	if ( state == SHOULDTRANSMIT_END )
	{
		// Kill the item - It will be recreated later
		if ( m_hBannerEntity )
		{
			m_hBannerEntity->Release();
			m_hBannerEntity = NULL;
		}
	}

	BaseClass::NotifyShouldTransmit( state );
}

void CTFBuffItem::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	BaseClass::OnDataChanged( updateType );
}

void CTFBuffItem::ClientThink( void )
{
	if ( m_iBuffType == -1 || m_iBuffType == 0 )
	{
		m_iBuffType = GetBuffType();

		if ( m_iBuffType != EParachute )
		{
			SetNextClientThink( CLIENT_THINK_NEVER );
		}
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_bPlayingHorn = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CTFBuffItem::TranslateViewmodelHandActivityInternal( Activity actBase )
{
	Activity iActivity = actBase;
	switch ( iActivity )
	{
	case ACT_VM_DRAW:
		iActivity = ACT_ITEM1_VM_DRAW;
		break;
	case ACT_VM_HOLSTER:
		iActivity = ACT_ITEM1_VM_HOLSTER;
		break;
	case ACT_VM_IDLE:
		if ( m_bPlayingHorn )
		{
			RaiseFlag();
			iActivity = ACT_RESET;
		}
		else
		{
			iActivity = ACT_ITEM1_VM_IDLE;
		}
		break;
	case ACT_VM_PULLBACK:
		iActivity = ACT_ITEM1_VM_PULLBACK;
		break;
	case ACT_VM_PRIMARYATTACK:
		m_bPlayingHorn = true;
		iActivity = ACT_ITEM1_VM_PRIMARYATTACK;
		break;
	case ACT_VM_SECONDARYATTACK:
		m_bPlayingHorn = true;
		iActivity = ACT_ITEM1_VM_SECONDARYATTACK;
		break;
	case ACT_VM_IDLE_TO_LOWERED:
		iActivity = ACT_ITEM1_VM_IDLE_TO_LOWERED;
		break;
	case ACT_VM_IDLE_LOWERED:
		iActivity = ACT_ITEM1_VM_IDLE_LOWERED;
		break;
	case ACT_VM_LOWERED_TO_IDLE:
		iActivity = ACT_ITEM1_VM_LOWERED_TO_IDLE;
		break;
	default:
		break;
	}

	return BaseClass::TranslateViewmodelHandActivityInternal( iActivity );
}


bool CTFBuffItem::CanReload( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: UI Progress
//-----------------------------------------------------------------------------
float CTFBuffItem::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetRageMeter() / 100.0f;
}


//-----------------------------------------------------------------------------
// Purpose: UI Progress (same as GetProgress() without the division by 100.0f)
//-----------------------------------------------------------------------------
bool CTFBuffItem::IsFull( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	return ( pPlayer->m_Shared.GetRageMeter() >= 100.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFBuffItem::EffectMeterShouldFlash( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer && (IsFull() || pPlayer->m_Shared.IsRageDraining()) )
		return true;
	else
		return false;
}

// Money Bag, Protection Insurance, and Speakeasy.
// The throw comes from the stock cleaver. Only these classes moved off the jar file.
IMPLEMENT_NETWORKCLASS_ALIASED( TFMoneyBag, DT_TFWeaponMoneyBag )
BEGIN_NETWORK_TABLE( CTFMoneyBag, DT_TFWeaponMoneyBag )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFMoneyBag )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_moneybag, CTFMoneyBag );
PRECACHE_WEAPON_REGISTER( tf_weapon_moneybag );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_MoneyBag, DT_TFProjectile_MoneyBag )
BEGIN_NETWORK_TABLE( CTFProjectile_MoneyBag, DT_TFProjectile_MoneyBag )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_moneybag, CTFProjectile_MoneyBag );
PRECACHE_WEAPON_REGISTER( tf_projectile_moneybag );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProtectionBag, DT_TFWeaponProtectionBag )
BEGIN_NETWORK_TABLE( CTFProtectionBag, DT_TFWeaponProtectionBag )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFProtectionBag )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_protectionbag, CTFProtectionBag );
PRECACHE_WEAPON_REGISTER( tf_weapon_protectionbag );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_ProtectionBag, DT_TFProjectile_ProtectionBag )
BEGIN_NETWORK_TABLE( CTFProjectile_ProtectionBag, DT_TFProjectile_ProtectionBag )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_protectionbag, CTFProjectile_ProtectionBag );
PRECACHE_WEAPON_REGISTER( tf_projectile_protectionbag );

IMPLEMENT_NETWORKCLASS_ALIASED( TFSpeakeasy, DT_TFWeaponSpeakeasy )
BEGIN_NETWORK_TABLE( CTFSpeakeasy, DT_TFWeaponSpeakeasy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSpeakeasy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_speakeasy, CTFSpeakeasy );
PRECACHE_WEAPON_REGISTER( tf_weapon_speakeasy );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Speakeasy, DT_TFProjectile_Speakeasy )
BEGIN_NETWORK_TABLE( CTFProjectile_Speakeasy, DT_TFProjectile_Speakeasy )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_speakeasy, CTFProjectile_Speakeasy );
PRECACHE_WEAPON_REGISTER( tf_projectile_speakeasy );


static const char *kMoneyBagClosedModel = "models/weapons/w_moneybag_closed.mdl";
static const char *kMoneyBagOpenModel = "models/weapons/w_moneybag_open.mdl";
static const char *kProtectionBugleModel = "models/workshop/weapons/c_models/c_battalion_bugle/c_battalion_bugle.mdl";
static const char *kProtectionFlagModel = "models/workshop/weapons/c_models/c_battalion_buffbanner/c_battalion_buffbanner.mdl";

//-----------------------------------------------------------------------------
// v_moneybag has draw, idle, throw, and inspect. Inspect is item3 so it
// does not share the typewriter's secondary alt1 activities on c_mobster_arms.
// Throw is ACT_ITEM3_VM_THROW, which stock TF2 does not define. There is no
// item2 sequence on this model or on the player.
//-----------------------------------------------------------------------------
Activity CTFMoneyBag::TranslateViewmodelHandActivityInternal( Activity actBase )
{
	switch ( actBase )
	{
	case ACT_VM_DRAW:
		return ACT_ITEM3_VM_DRAW;
	case ACT_VM_IDLE:
		return ACT_ITEM3_VM_IDLE;
	case ACT_VM_PRIMARYATTACK:
	case ACT_VM_SECONDARYATTACK:
		return ACT_ITEM3_VM_THROW;
	case ACT_SECONDARY_VM_INSPECT_START:
		return ACT_ITEM3_VM_INSPECT_START;
	case ACT_SECONDARY_VM_INSPECT_IDLE:
		return ACT_ITEM3_VM_INSPECT_IDLE;
	case ACT_SECONDARY_VM_INSPECT_END:
		return ACT_ITEM3_VM_INSPECT_END;
	default:
		return BaseClass::TranslateViewmodelHandActivityInternal( actBase );
	}
}

#ifdef CLIENT_DLL
void CTFMoneyBag::ModifyViewModelBodygroups( CBaseAnimating *pVM )
{
	if ( !pVM )
		return;

	// Submodel 0 is the bag. Submodel 1 is empty. Hide it once the throw
	// lets go, which is the event at cycle 0.588. The sequence plays on the
	// arms; the bodygroup lives on the bonemerged v_moneybag.
	int iValue = 0;
	const char *pszSeq = pVM->GetSequenceName( pVM->GetSequence() );
	if ( pszSeq && !Q_stricmp( pszSeq, "bag_throw" ) && pVM->GetCycle() >= 0.55f )
		iValue = 1;

	CBaseAnimating *pTarget = GetViewmodelAttachment();
	if ( !pTarget )
		pTarget = pVM;
	const int iGroup = pTarget->FindBodygroupByName( "Moneybag" );
	if ( iGroup >= 0 )
		pTarget->SetBodygroup( iGroup, iValue );
}
#endif

void CTFMoneyBag::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( kMoneyBagClosedModel );
	PrecacheModel( kMoneyBagOpenModel );
	PrecacheModel( "models/weapons/v_moneybag.mdl" );
	PrecacheModel( "models/weapons/c_mobster_arms.mdl" );
	PrecacheModel( "models/vip_mobster/weapons/c_mobster_arms.mdl" );
	PrecacheScriptSound( "Mobster.MoneyBag.Deploy" );
	PrecacheScriptSound( "Mobster.MoneyBag.Hit" );
	PrecacheScriptSound( "Mobster.MoneyBag.Resist" );
	PrecacheScriptSound( "Mobster.MoneyBag.Voice" );
	PrecacheParticleSystem( "weapon_moneybag_parent" );
}

void CTFProtectionBag::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( kProtectionBugleModel );
	PrecacheModel( kProtectionFlagModel );
	PrecacheScriptSound( "Weapon_BuffBanner.Flag" );
}

void CTFSpeakeasy::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/workshop/weapons/c_models/c_scotland_shard/c_scotland_shard.mdl" );
	PrecacheModel( "models/player/items/taunts/beer_crate/beer_crate.mdl" );
	PrecacheScriptSound( "Samurai.Conch" );
}

#ifdef GAME_DLL
Vector CTFMoneyBag::GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
	// The bag's throw is an absolute velocity, not the cleaver's normalized 3000.
	(void)vecRight;
	(void)vecUp;
	return vecForward * 4400.f + Vector( 0, 0, 200 );
}

CTFProjectile_Jar *CTFMoneyBag::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	return CTFProjectile_MoneyBag::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}

CTFProjectile_MoneyBag::CTFProjectile_MoneyBag()
{
	m_bDeployed = false;
	m_flFlightDieTime = 0.f;
	m_flRefreshUntil = 0.f;
	m_flRemoveTime = 0.f;
}

void CTFProjectile_MoneyBag::Precache()
{
	PrecacheModel( GetClosedModel() );
	PrecacheModel( GetOpenModel() );
	if ( GetDeploySound() && GetDeploySound()[0] )
		PrecacheScriptSound( GetDeploySound() );
	if ( GetDeployParticle() && GetDeployParticle()[0] )
		PrecacheParticleSystem( GetDeployParticle() );
	PrecacheScriptSound( "Mobster.MoneyBag.Hit" );
	PrecacheScriptSound( "Mobster.MoneyBag.Resist" );
	PrecacheScriptSound( "Mobster.MoneyBag.Voice" );
	BaseClass::Precache();
}

void CTFProjectile_MoneyBag::SetCustomPipebombModel()
{
	SetModel( GetClosedModel() );
	ApplyTeamSkin();
}

void CTFProjectile_MoneyBag::Spawn()
{
	BaseClass::Spawn();

	SetDetonateTimerLength( FLT_MAX );
	SetModelScale( GetBagModelScale() );
	m_bDeployed = false;
	m_flFlightDieTime = gpGlobals->curtime + 15.f;
	// deploy on the open bag is cosmetic. The client has to advance it,
	// because the placed bag is MOVETYPE_NONE.
	UseClientSideAnimation();
	SetContextThink( &CTFProjectile_MoneyBag::FlightThink, gpGlobals->curtime + 0.05f, "MoneyBagFlight" );
}

void CTFProjectile_MoneyBag::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// The jar base detonates on the first enemy and the pipebomb base bounces like a pill.
	// Neither is the bag. Drop the projectile if it goes into the sky.
	if ( m_bDeployed )
		return;

	int otherIndex = !index;
	CBaseEntity *pHit = pEvent->pEntities[otherIndex];
	if ( !pHit )
		return;

	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	if ( pprops && pprops->game.material == 'X' )
		UTIL_Remove( this );
}

void CTFProjectile_MoneyBag::FlightThink( void )
{
	if ( m_bDeployed )
	{
		AuraThink();
		return;
	}

	if ( gpGlobals->curtime >= m_flFlightDieTime )
	{
		UTIL_Remove( this );
		return;
	}

	Vector vecSrc = GetAbsOrigin();
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector( 0, 0, 25 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 0.98f )
	{
		if ( tr.surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		Deploy( tr.endpos + tr.plane.normal * 2.f );
		return;
	}

	SetContextThink( &CTFProjectile_MoneyBag::FlightThink, gpGlobals->curtime + 0.05f, "MoneyBagFlight" );
}

void CTFProjectile_MoneyBag::Deploy( const Vector &vecOrigin )
{
	if ( m_bDeployed )
		return;

	// Placed models sit on +Z: open money bag, buff banner, and beer crate.
	// Keep the throw's yaw and drop the tumble so they land right side up.
	QAngle angLand = GetAbsAngles();
	Vector vecForward = GetAbsVelocity();
	vecForward.z = 0.f;
	if ( vecForward.LengthSqr() < 1.f )
	{
		AngleVectors( GetAbsAngles(), &vecForward );
		vecForward.z = 0.f;
	}
	if ( vecForward.LengthSqr() < 0.01f )
		vecForward = Vector( 1.f, 0.f, 0.f );

	VectorNormalize( vecForward );
	VectorAngles( vecForward, Vector( 0.f, 0.f, 1.f ), angLand );

	m_bDeployed = true;
	SetAbsOrigin( vecOrigin );
	SetAbsAngles( angLand );
	SetAbsVelocity( vec3_origin );
	SetLocalAngularVelocity( vec3_angle );
	SetModel( GetOpenModel() );
	SetModelScale( GetBagModelScale() );
	ApplyTeamSkin();

	int iSeq = LookupSequence( "deploy" );
	if ( iSeq >= 0 )
		ResetSequence( iSeq );

	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->SetVelocity( &vec3_origin, &vec3_origin );
		pPhys->SetPosition( vecOrigin, angLand, true );
		pPhys->EnableMotion( false );
	}

	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetTouch( NULL );

	EmitSound( GetDeploySound() );
#ifdef GAME_DLL
	if ( CTFPlayer *pOwner = ToTFPlayer( GetThrower() ) )
		pOwner->EmitSound( "Mobster.MoneyBag.Voice" );
#endif
	if ( GetDeployParticle() && GetDeployParticle()[0] )
		DispatchParticleEffect( GetDeployParticle(), PATTACH_ABSORIGIN_FOLLOW, this );

	m_flRefreshUntil = gpGlobals->curtime + GetAuraDuration();
	m_flRemoveTime = m_flRefreshUntil + GetAuraLinger();
	SetContextThink( &CTFProjectile_MoneyBag::AuraThink, gpGlobals->curtime, "MoneyBagAura" );
}

void CTFProjectile_MoneyBag::AuraThink( void )
{
	if ( gpGlobals->curtime < m_flRefreshUntil )
	{
		CUtlVector< CTFPlayer * > players;
		CollectPlayers( &players, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );

		const Vector vecSrc = GetAbsOrigin() + Vector( 0, 0, 20 );
		CTFPlayer *pProvider = ToTFPlayer( GetThrower() );
		FOR_EACH_VEC( players, i )
		{
			CTFPlayer *pPlayer = players[i];
			if ( !pPlayer )
				continue;

			const bool bSameTeam = pPlayer->GetTeamNumber() == GetTeamNumber();
			const bool bDisguisedAsUs = pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber();
			if ( !bSameTeam && !bDisguisedAsUs )
				continue;

			if ( ( pPlayer->EyePosition() - vecSrc ).Length() >= GetAuraRadius() )
				continue;

			trace_t tr;
			UTIL_TraceLine( vecSrc, pPlayer->EyePosition(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction <= 0.95f )
				continue;

			if ( GrantsAuraOnce() )
			{
				if ( m_AuraGrantedEntIndex.Find( pPlayer->entindex() ) != m_AuraGrantedEntIndex.InvalidIndex() )
					continue;
				m_AuraGrantedEntIndex.AddToTail( pPlayer->entindex() );
			}

			// Money Bag and Protection Insurance refresh a 1 second buff.
			// Speakeasy grants its buff once. A full 5 seconds of aura is left at
			// deploy, so that grant is 6 seconds, and it falls to 1 second as the crate ends.
			float flTimeRemaining = m_flRefreshUntil - gpGlobals->curtime;
			CBaseEntity *pBuffProvider = pProvider ? static_cast< CBaseEntity * >( pProvider ) : this;
			pPlayer->m_Shared.AddCond( GetAuraCondition(), GetAppliedBuffDuration( flTimeRemaining ), pBuffProvider );
		}
	}

	if ( gpGlobals->curtime >= m_flRemoveTime )
	{
		UTIL_Remove( this );
		return;
	}

	SetContextThink( &CTFProjectile_MoneyBag::AuraThink, gpGlobals->curtime + 0.2f, "MoneyBagAura" );
}

CTFProjectile_MoneyBag *CTFProjectile_MoneyBag::Create( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_MoneyBag *pGrenade = static_cast< CTFProjectile_MoneyBag * >( CBaseEntity::CreateNoSpawn( "tf_projectile_moneybag", position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
		pGrenade->m_flFullDamage = 0;
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetModelScale( pGrenade->GetBagModelScale() );
	}
	return pGrenade;
}

CTFProjectile_Jar *CTFProtectionBag::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	return CTFProjectile_ProtectionBag::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}

CTFProjectile_ProtectionBag *CTFProjectile_ProtectionBag::Create( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_ProtectionBag *pGrenade = static_cast< CTFProjectile_ProtectionBag * >( CBaseEntity::CreateNoSpawn( "tf_projectile_protectionbag", position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
		pGrenade->m_flFullDamage = 0;
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetModelScale( pGrenade->GetBagModelScale() );
		pGrenade->ApplyTeamSkin();
	}
	return pGrenade;
}

void CTFProjectile_ProtectionBag::ApplyTeamSkin( void )
{
	SetSkin( GetTeamNumber() == TF_TEAM_BLUE ? 1 : 0 );
}

CTFProjectile_Jar *CTFSpeakeasy::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	return CTFProjectile_Speakeasy::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}

CTFProjectile_Speakeasy *CTFProjectile_Speakeasy::Create( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_Speakeasy *pGrenade = static_cast< CTFProjectile_Speakeasy * >( CBaseEntity::CreateNoSpawn( "tf_projectile_speakeasy", position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
		pGrenade->m_flFullDamage = 0;
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetModelScale( pGrenade->GetBagModelScale() );
	}
	return pGrenade;
}
#else
CTFProjectile_MoneyBag::CTFProjectile_MoneyBag()
{
}

void CTFProjectile_MoneyBag::Precache()
{
	PrecacheModel( GetClosedModel() );
	PrecacheModel( GetOpenModel() );
	if ( GetDeploySound() && GetDeploySound()[0] )
		PrecacheScriptSound( GetDeploySound() );
	if ( GetDeployParticle() && GetDeployParticle()[0] )
		PrecacheParticleSystem( GetDeployParticle() );
	BaseClass::Precache();
}

const char *CTFProjectile_MoneyBag::GetTrailParticleName( void )
{
	return "";
}

void CTFProjectile_MoneyBag::CreateTrailParticles( void )
{
}
#endif
