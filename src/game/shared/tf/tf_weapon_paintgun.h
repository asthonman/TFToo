//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Gel gun. Attack fires the selected gel, attack2 erases, reload cycles.
// Multiplayer: each gun keeps its own gel. Not an economy item.
//
//=============================================================================
#ifndef TF_WEAPON_PAINTGUN_H
#define TF_WEAPON_PAINTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#define CTFWeaponPaintgun C_TFWeaponPaintgun
#define CTFWeaponGreasegun C_TFWeaponGreasegun
#endif

class CTFWeaponPaintgun : public CTFWeaponBase
{
	DECLARE_CLASS( CTFWeaponPaintgun, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CTFWeaponPaintgun();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_NONE; }
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual void ItemPostFrame( void );

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif

private:
	void FireBlob( bool bErase );
	void CyclePaint( void );
	void ApplyColor( void );

	CNetworkVar( int, m_nPaintType );
	float m_flNextFireSound;
	float m_flNextPaintAnim;
};

//-----------------------------------------------------------------------------
// Madcap stock secondary. Propulsion gel only. Ammo is TF_AMMO_METAL so it
// does not share the Mason's Missile charge, which is TF_AMMO_SECONDARY.
//-----------------------------------------------------------------------------
class CTFWeaponGreasegun : public CTFWeaponBase
{
	DECLARE_CLASS( CTFWeaponGreasegun, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CTFWeaponGreasegun();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_NONE; }
	virtual void PrimaryAttack( void );

private:
	float m_flNextPaintAnim;
};

#ifdef CLIENT_DLL
#define CTFWeaponHelper C_TFWeaponHelper
#endif

//-----------------------------------------------------------------------------
// Guard Dog secondary. Mouse1 fires a short hook. If it brings a pickup back,
// mouse1 throws that pickup and weapon switches stay locked until it is gone.
//-----------------------------------------------------------------------------
class CTFWeaponHelper : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFWeaponHelper, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponHelper();

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_NONE; }
	virtual void	Precache( void );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void ) {}
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void	UpdateOnRemove( void );

	bool	IsCarryingPickup( void ) const { return m_bCarrying; }

#ifdef GAME_DLL
	void	ReceivePickup( CBaseEntity *pPickup );
	void	ClearHook( CBaseEntity *pHook );
#endif

private:
#ifdef GAME_DLL
	void	FireHook( void );
	void	ThrowCarried( void );
	void	DropCarried( const Vector &vecVelocity );

	EHANDLE	m_hHook;
	EHANDLE	m_hCarried;
#endif

	CNetworkVar( bool, m_bCarrying );

	CTFWeaponHelper( const CTFWeaponHelper & ) {}
};

#endif
