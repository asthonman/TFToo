//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_JAR_H
#define TF_WEAPON_JAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grenade_pipebomb.h"

#ifdef CLIENT_DLL
#define CTFJar C_TFJar
#define CTFProjectile_Jar C_TFProjectile_Jar
#define CTFJarMilk C_TFJarMilk
#define CTFCleaver C_TFCleaver
#define CTFThrowable C_TFThrowable
#define CTFProjectile_JarMilk C_TFProjectile_JarMilk
#define CTFProjectile_Cleaver C_TFProjectile_Cleaver
#define CTFProjectile_Throwable C_TFProjectile_Throwable
#define CTFMoneyBag C_TFMoneyBag
#define CTFProjectile_MoneyBag C_TFProjectile_MoneyBag
#define CTFProtectionBag C_TFProtectionBag
#define CTFProjectile_ProtectionBag C_TFProjectile_ProtectionBag
#define CTFSpeakeasy C_TFSpeakeasy
#define CTFProjectile_Speakeasy C_TFProjectile_Speakeasy
#endif

class CTFProjectile_Jar;

enum EThrowableTypes 
{
	EThrowableBase				= 0,
};

#define JAR_EXPLODE_RADIUS 200		// TF_ROCKET_RADIUS and grenade explosions is 146
#define TF_WEAPON_PEEJAR_EXPLODE_SOUND	"Jar.Explode"

//=============================================================================
//
// Jar class.
//
class CTFJar : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFJar, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFJar();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_JAR; }
	virtual float		GetProjectileSpeed( void );
	virtual void		PrimaryAttack( void );

	float				GetProgress( void ) { return GetEffectBarProgress(); }

	virtual CBaseEntity *FireJar( CTFPlayer *pPlayer );
	virtual void		TossJarThink( void );

	virtual float		InternalGetEffectBarRechargeTime( void ) { return 20.1; }

	virtual const char*			GetEffectLabelText( void ) { return "#TF_JAR"; }

	virtual void		GetProjectileEntityName( CAttribute_String *attrProjectileEntityName );

#ifdef GAME_DLL
	virtual const AngularImpulse GetAngularImpulse( void ){ return AngularImpulse( 300, 0, 0 ); }
	virtual Vector GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

	virtual bool		ShouldSpeakWhenFiring( void ){ return true; }

//	virtual bool		SendWeaponAnim( int iActivity );

	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif

	virtual bool CanThrowUnderWater( void ){ return false; }

private:

	int m_iProjectileType;

	CTFJar( const CTFJar & ) {}
};

// *************************************************************************************************************************
class CTFJarMilk : public CTFJar
{
public:
	DECLARE_CLASS( CTFJarMilk, CTFJar );
	DECLARE_NETWORKCLASS(); 

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_JAR_MILK; }

#ifdef GAME_DLL
	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif

#ifdef CLIENT_DLL
	const char*			ModifyEventParticles( const char* token );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
#endif
};

// *************************************************************************************************************************
class CTFCleaver : public CTFJar
{
public:
	DECLARE_CLASS( CTFCleaver, CTFJar );
	DECLARE_NETWORKCLASS(); 

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CLEAVER; }
	virtual float		GetProjectileSpeed( void );

	virtual void		SecondaryAttack( void );

	virtual const char*			GetEffectLabelText( void ) { return "#TF_CLEAVER"; }

	virtual float		InternalGetEffectBarRechargeTime( void ) { return 5.1; }

#ifdef GAME_DLL
	virtual const AngularImpulse GetAngularImpulse( void ){ return AngularImpulse( 0, 500, 0 ); }
	virtual Vector GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

	virtual bool		ShouldSpeakWhenFiring( void ){ return false; }

	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif

#ifdef CLIENT_DLL
	const char*			ModifyEventParticles( const char* token );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
#endif
};

//=============================================================================
//
// JarBomb projectile class.
//
class CTFProjectile_Jar : public CTFGrenadePipebombProjectile
{
public:
	DECLARE_CLASS( CTFProjectile_Jar, CTFGrenadePipebombProjectile );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void );
#endif

#ifdef GAME_DLL
	CTFProjectile_Jar();

	static CTFProjectile_Jar *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual int			GetProjectileType() const OVERRIDE			{ return m_iProjectileType; }

	virtual int			GetWeaponID( void ) const OVERRIDE			{ return TF_WEAPON_GRENADE_JAR; }
	virtual float		GetDamage()									{ return 0.f; }
	virtual bool		ExplodesOnHit()								{ return true; }

	virtual void		Precache() OVERRIDE;
	virtual void		SetCustomPipebombModel() OVERRIDE;

	virtual float		GetDamageRadius()							{ return JAR_EXPLODE_RADIUS ; }
	virtual void		OnHit( CBaseEntity *pOther )				{}
	virtual void		OnHitWorld( void )							{}
	virtual void		Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;
	

	virtual void		PipebombTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) OVERRIDE;
	void				VPhysicsCollisionThink( void );

	void OnBreadMonsterHit( CBaseEntity *pOther, trace_t *pTrace );

	bool PositionArrowOnBone( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim );
	void GetBoneAttachmentInfo( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim, Vector &bonePosition, QAngle &boneAngles, int &boneIndexAttached, int &physicsBoneIndex );
	void CreateStickyAttachmentToTarget( CTFPlayer *pOwner, CTFPlayer *pVictim, trace_t *trace );

	virtual const char* GetImpactEffect() { return "peejar_impact"; }
	virtual ETFCond		GetEffectCondition( void ) { return TF_COND_URINE; }

	virtual const char* GetExplodeSound() { return TF_WEAPON_PEEJAR_EXPLODE_SOUND; }

protected:
	Vector		m_vCollisionVelocity;
	int			m_iProjectileType;
#endif

};

// *************************************************************************************************************************
class CTFProjectile_JarMilk : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_JarMilk, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	static CTFProjectile_JarMilk *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual void		SetCustomPipebombModel() OVERRIDE;

	virtual const char* GetImpactEffect() OVERRIDE { return "peejar_impact_milk"; }
	virtual ETFCond		GetEffectCondition( void ) OVERRIDE { return TF_COND_MAD_MILK; }
#endif

	virtual int			GetWeaponID( void ) const OVERRIDE			{ return TF_WEAPON_GRENADE_JAR_MILK; }

	virtual void		Precache() OVERRIDE;
	
};

// *************************************************************************************************************************
class CTFProjectile_Cleaver : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_Cleaver, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void );
#endif

#ifdef GAME_DLL
	static CTFProjectile_Cleaver *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int nSkin );
#endif

	CTFProjectile_Cleaver();

	virtual int			GetWeaponID( void ) const OVERRIDE		{ return TF_WEAPON_GRENADE_CLEAVER; }

	virtual void		Precache() OVERRIDE;
	virtual void		SetCustomPipebombModel() OVERRIDE;

#ifdef GAME_DLL
	virtual float		GetDamage() OVERRIDE { return 50.f; }
	virtual void		OnHit( CBaseEntity *pOther ) OVERRIDE;
	virtual void		Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;
	virtual void		Detonate() OVERRIDE;
	virtual const char* GetImpactEffect() OVERRIDE { return ""; }
	virtual ETFCond		GetEffectCondition( void ) OVERRIDE { return TF_COND_BLEEDING; }
	
	void				RemoveCleaver( void );

private:
	bool m_bHitPlayer;
	bool m_bSoundPlayed;
#endif
};

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
	virtual void		Precache( void );

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

void JarExplode( int iEntIndex, CTFPlayer *pAttacker, CBaseEntity *pOriginalWeapon, CBaseEntity *pWeapon, const Vector& vContactPoint, int iTeam, float flRadius, ETFCond cond, float flDuration, const char *pszImpactEffect, const char *pszExplodeSound );

#endif // TF_WEAPON_JAR_H
