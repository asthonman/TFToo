//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BUFF_ITEM_H
#define TF_WEAPON_BUFF_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_item_wearable.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
#include "c_tf_buff_banner.h"
#define CTFBuffItem C_TFBuffItem
class C_TFBuffBanner;
#endif

//=============================================================================
//
// Buff item weapon class.
//
enum EBuffItemTypes
{
	EBuffBanner = 1,
	EBattalion,
	EConcheror,
	EParachute,

	NUM_BUFF_ITEM_TYPES = EParachute
};

enum EParachuteStates
{
	EParachuteDeployed,
	EParachuteDeployed_Idle,
	EParachuteRetracted,
	EParachuteRetracted_Idle,
};

class CTFBuffItem : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBuffItem, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBuffItem();
	~CTFBuffItem();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_BUFF_ITEM; }

	virtual void	Precache();

	virtual void	PrimaryAttack();

	virtual void	Equip( CBaseCombatCharacter *pOwner ) OVERRIDE;
	virtual void	Detach( void ) OVERRIDE;

	void			FireGameEvent( IGameEvent* event );
	virtual void	CreateBanner();

	virtual Activity TranslateViewmodelHandActivityInternal( Activity actBase ) OVERRIDE;

	void			BlowHorn( void );
	void			RaiseFlag( void );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual bool	CanReload( void );

	virtual int		GetBuffType() { int iBuffType = 0; CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type ); return iBuffType; }

#ifdef CLIENT_DLL
	void			SetBanner( C_TFBuffBanner* pNewBanner ) { m_hBannerEntity.Set( pNewBanner ); }
	virtual void	NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );
#endif

	virtual void	WeaponReset( void );

	float			GetProgress( void );
	bool			IsFull( void ); // same as GetProgress() without the division by 100.0f
	const char*		GetEffectLabelText( void ) { return "#TF_RAGE"; }
	bool			EffectMeterShouldFlash( void );

protected:
#ifdef CLIENT_DLL
	CHandle<C_TFBuffBanner>		m_hBannerEntity;
	int							m_iBuffType;
#endif // CLIENT_DLL

private:

	CTFBuffItem( const CTFBuffItem & ) {}

	bool						m_bPlayingHorn;
};


#include "tf_weapon_jar.h"

#ifdef CLIENT_DLL
#define CTFMoneyBag C_TFMoneyBag
#define CTFProjectile_MoneyBag C_TFProjectile_MoneyBag
#define CTFProtectionBag C_TFProtectionBag
#define CTFProjectile_ProtectionBag C_TFProjectile_ProtectionBag
#define CTFSpeakeasy C_TFSpeakeasy
#define CTFProjectile_Speakeasy C_TFProjectile_Speakeasy
#endif

// *************************************************************************************************************************
// Mobster Money Bag. A cleaver-style throwable. Once it lands, teammates in
// line of sight get the Buff Banner mini-crit for 5 seconds, lingering 1 second
// after they leave.
class CTFMoneyBag : public CTFCleaver
{
public:
	DECLARE_CLASS( CTFMoneyBag, CTFCleaver );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CLEAVER; }
	virtual const char*	GetEffectLabelText( void )			{ return "#TF_Weapon_MoneyBag"; }
	// The player model only has item1 poses. The viewmodel is item3.
	virtual int			GetActivityWeaponRole() const		{ return TF_WPN_TYPE_ITEM1; }
	virtual bool		WantsViewmodelActivityTranslation( void ) const { return true; }
	virtual Activity	TranslateViewmodelHandActivityInternal( Activity actBase );
	virtual void		Precache( void );

#ifdef CLIENT_DLL
	// The bag mesh is a bodygroup on the bonemerged v_moneybag. The throw
	// sequence plays on the arms, so the hide is done from code.
	virtual void		ModifyViewModelBodygroups( CBaseAnimating *pVM );
#endif

#ifdef GAME_DLL
	virtual const AngularImpulse GetAngularImpulse( void )	{ return AngularImpulse( -550, -550, 50 ); }
	virtual Vector		GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );
	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif
};

class CTFProjectile_MoneyBag : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_MoneyBag, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

	CTFProjectile_MoneyBag();

	virtual int			GetWeaponID( void ) const OVERRIDE	{ return TF_WEAPON_GRENADE_CLEAVER; }
	virtual void		Precache() OVERRIDE;

	// Buff Banner and Battalion's Backup share a 450-unit range, a 10 second
	// uptime, and 600 damage to fill. Those ratios are 1, so the bag keeps the
	// Money Bag's 375-unit line of sight, 5 second uptime, and 1 second linger.
	virtual const char	*GetClosedModel( void ) const	{ return "models/weapons/w_moneybag_closed.mdl"; }
	virtual const char	*GetOpenModel( void ) const		{ return "models/weapons/w_moneybag_open.mdl"; }
	virtual float		GetBagModelScale( void ) const	{ return 1.4f; }
	virtual ETFCond		GetAuraCondition( void ) const	{ return TF_COND_OFFENSEBUFF; }
	virtual float		GetAuraRadius( void ) const		{ return 375.f; }
	virtual float		GetAuraDuration( void ) const	{ return 5.f; }
	virtual float		GetAuraLinger( void ) const		{ return 1.f; }
	// How long the condition lasts when it is applied. flTimeRemaining is how long
	// the aura will still accept people. The Money Bag ignores that and reapplies
	// a 1 second buff. Speakeasy uses the time left plus that 1 second.
	virtual float		GetAppliedBuffDuration( float flTimeRemaining ) const { (void)flTimeRemaining; return GetAuraLinger(); }
	virtual bool		GrantsAuraOnce( void ) const	{ return false; }
	virtual const char	*GetDeploySound( void ) const	{ return "Mobster.MoneyBag.Deploy"; }
	virtual const char	*GetDeployParticle( void ) const { return "weapon_moneybag_parent"; }

#ifdef CLIENT_DLL
	virtual const char*	GetTrailParticleName( void );
	virtual void		CreateTrailParticles( void );
#endif

#ifdef GAME_DLL
	static CTFProjectile_MoneyBag *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual void		Spawn() OVERRIDE;
	virtual void		SetCustomPipebombModel() OVERRIDE;
	virtual float		GetDamage() OVERRIDE				{ return 0.f; }
	virtual bool		ExplodesOnHit() OVERRIDE			{ return false; }
	virtual void		Detonate() OVERRIDE					{}
	virtual void		Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE { (void)pTrace; (void)bitsDamageType; }
	virtual void		PipebombTouch( CBaseEntity *pOther ) OVERRIDE { (void)pOther; }
	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) OVERRIDE;

	void				FlightThink( void );
	void				AuraThink( void );
	void				Deploy( const Vector &vecOrigin );
	virtual void		ApplyTeamSkin( void )			{}

private:
	CUtlVector<int>		m_AuraGrantedEntIndex;
	bool				m_bDeployed;
	float				m_flFlightDieTime;
	float				m_flRefreshUntil;
	float				m_flRemoveTime;
#endif
};

// Battalion's Backup, thrown like the Money Bag. Same line of sight, duration,
// and 1 second linger. The buff is the Backup's damage resistance, and the
// wearer keeps the +20 max health. Held model is the bugle. Placed model is the flag.
class CTFProtectionBag : public CTFMoneyBag
{
public:
	DECLARE_CLASS( CTFProtectionBag, CTFMoneyBag );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual const char*	GetEffectLabelText( void )			{ return "#TF_Weapon_ProtectionInsurance"; }
	virtual void		Precache( void );

#ifdef GAME_DLL
	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif
};

class CTFProjectile_ProtectionBag : public CTFProjectile_MoneyBag
{
public:
	DECLARE_CLASS( CTFProjectile_ProtectionBag, CTFProjectile_MoneyBag );
	DECLARE_NETWORKCLASS();

	virtual const char	*GetClosedModel( void ) const	{ return "models/workshop/weapons/c_models/c_battalion_bugle/c_battalion_bugle.mdl"; }
	virtual const char	*GetOpenModel( void ) const		{ return "models/workshop/weapons/c_models/c_battalion_buffbanner/c_battalion_buffbanner.mdl"; }
	virtual float		GetBagModelScale( void ) const	{ return 1.0f; }
	virtual ETFCond		GetAuraCondition( void ) const	{ return TF_COND_DEFENSEBUFF; }
	virtual const char	*GetDeploySound( void ) const	{ return "Weapon_BuffBanner.Flag"; }
	virtual const char	*GetDeployParticle( void ) const { return NULL; }

#ifdef GAME_DLL
	static CTFProjectile_ProtectionBag *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual void		ApplyTeamSkin( void );
#endif
};

// The Concheror, thrown like the Money Bag. It fills on 480 damage instead of
// 600, so the recharge is 4/5 of the Money Bag. The crate lasts 5 seconds.
// Each teammate is buffed once: 6 seconds if they are there when it lands,
// down to 1 second if they arrive as it ends. The wearer keeps its health regen.
// Held model is the Scottish Handshake. Placed model is the beer crate.
class CTFSpeakeasy : public CTFMoneyBag
{
public:
	DECLARE_CLASS( CTFSpeakeasy, CTFMoneyBag );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual const char*	GetEffectLabelText( void )			{ return "#TF_Weapon_Speakeasy"; }
	virtual void		Precache( void );

#ifdef GAME_DLL
	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif
};

class CTFProjectile_Speakeasy : public CTFProjectile_MoneyBag
{
public:
	DECLARE_CLASS( CTFProjectile_Speakeasy, CTFProjectile_MoneyBag );
	DECLARE_NETWORKCLASS();

	virtual const char	*GetClosedModel( void ) const	{ return "models/workshop/weapons/c_models/c_scotland_shard/c_scotland_shard.mdl"; }
	virtual const char	*GetOpenModel( void ) const		{ return "models/player/items/taunts/beer_crate/beer_crate.mdl"; }
	virtual float		GetBagModelScale( void ) const	{ return 1.0f; }
	virtual ETFCond		GetAuraCondition( void ) const	{ return TF_COND_REGENONDAMAGEBUFF; }
	virtual float		GetAuraDuration( void ) const	{ return 5.f; }
	virtual float		GetAuraLinger( void ) const		{ return 0.f; }
	virtual float		GetAppliedBuffDuration( float flTimeRemaining ) const { return flTimeRemaining + 1.f; }
	virtual bool		GrantsAuraOnce( void ) const	{ return true; }
	virtual const char	*GetDeploySound( void ) const	{ return "Samurai.Conch"; }
	virtual const char	*GetDeployParticle( void ) const { return NULL; }

#ifdef GAME_DLL
	static CTFProjectile_Speakeasy *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif
};

#endif // TF_WEAPON_BUFF_ITEM_H
