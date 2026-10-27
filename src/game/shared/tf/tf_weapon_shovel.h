//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOVEL_H
#define TF_WEAPON_SHOVEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFShovel C_TFShovel
#define CTFLeadPipe C_TFLeadPipe
#endif

enum shovel_weapontypes_t
{
	SHOVEL_STANDARD = 0,
	SHOVEL_DAMAGE_BOOST,
	SHOVEL_SPEED_BOOST,
};

//=============================================================================
//
// Shovel class.
//
class CTFShovel : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFShovel, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShovel();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOVEL; }
	virtual void	PrimaryAttack();

	int				GetShovelType( void ) { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; };
	virtual bool	HasDamageBoost( void ) { return (GetShovelType() == SHOVEL_DAMAGE_BOOST); }
	virtual bool	HasSpeedBoost( void ) { return (GetShovelType() == SHOVEL_SPEED_BOOST); }
	virtual void	ItemPreFrame( void ) OVERRIDE;
	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage );
	virtual float	GetSpeedMod( void );

	void			MoveSpeedThink( void );

	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

#ifndef CLIENT_DLL
	virtual float	GetForceScale( void );
	virtual int		GetDamageCustom();
#endif

private:

	CTFShovel( const CTFShovel & ) {}

	bool			m_bHolstering;
	float			m_flLastHealthRatio;
};

// Mobster Lead Pipe. A shovel with no Atomizer penalties. Hit sounds are in the weapon script.
class CTFLeadPipe : public CTFShovel
{
public:
	DECLARE_CLASS( CTFLeadPipe, CTFShovel );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFLeadPipe() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOVEL; }
	virtual void	Precache( void );

#ifdef CLIENT_DLL
	// v_leadpipe's bodygroup 0 has no mesh. The pipe is bodygroup 1.
	virtual void	ModifyViewModelBodygroups( CBaseAnimating *pVM ) OVERRIDE;
#endif

private:
	CTFLeadPipe( const CTFLeadPipe & ) {}
};

#endif // TF_WEAPON_SHOVEL_H
