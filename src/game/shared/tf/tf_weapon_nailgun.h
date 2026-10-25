//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Madcap Nailgun. A Syringe Gun clone.
//
//=============================================================================
#ifndef TF_WEAPON_NAILGUN_H
#define TF_WEAPON_NAILGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_syringegun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFNailgun C_TFNailgun
#endif

//=============================================================================
//
// Madcap's Nailgun. Same projectile, damage, and rate of fire as the Syringe Gun.
//
class CTFNailgun : public CTFSyringeGun
{
public:

	DECLARE_CLASS( CTFNailgun, CTFSyringeGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFNailgun() {}
	~CTFNailgun() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_NAILGUN; }
	virtual void	Precache( void );

private:

	CTFNailgun( const CTFNailgun & ) {}
};

#endif // TF_WEAPON_NAILGUN_H
