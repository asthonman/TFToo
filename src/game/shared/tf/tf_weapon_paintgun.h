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

#endif
