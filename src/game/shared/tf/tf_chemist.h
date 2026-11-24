//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Chemist (class B7). Concoctions and jet-boot helpers.
//
//=============================================================================
#ifndef TF_CHEMIST_H
#define TF_CHEMIST_H
#ifdef _WIN32
#pragma once
#endif

class CTFPlayer;

enum
{
	CHEM_POISON = 0,
	CHEM_REPULSION,
	CHEM_NITRIC,
	CHEM_NEUTRAL,
	CHEM_COUNT
};

// Brew slot value when nothing is brewing. Fits in 3 bits.
#define CHEM_BREW_NONE 7

// m_nChemPacked:
//  0-1  selected concoction
//  2-4  brew slot (CHEM_BREW_NONE if idle)
//  5-16 uses, 3 bits each, CHEM_POISON .. CHEM_NEUTRAL (max 7)
// 17-18 previous selection, for the last-disguise swap
inline int ChemPack_Selected( int packed ) { return packed & 3; }
inline int ChemPack_Brew( int packed ) { return ( packed >> 2 ) & 7; }
inline int ChemPack_Uses( int packed, int slot )
{
	if ( slot < 0 || slot >= CHEM_COUNT )
		return 0;
	return ( packed >> ( 5 + slot * 3 ) ) & 7;
}
inline int ChemPack_Prev( int packed ) { return ( packed >> 17 ) & 3; }

inline int ChemPack_SetSelected( int packed, int slot )
{
	return ( packed & ~3 ) | ( slot & 3 );
}
inline int ChemPack_SetBrew( int packed, int slot )
{
	return ( packed & ~( 7 << 2 ) ) | ( ( slot & 7 ) << 2 );
}
inline int ChemPack_SetUses( int packed, int slot, int uses )
{
	if ( slot < 0 || slot >= CHEM_COUNT )
		return packed;
	if ( uses < 0 )
		uses = 0;
	if ( uses > 7 )
		uses = 7;
	const int shift = 5 + slot * 3;
	return ( packed & ~( 7 << shift ) ) | ( uses << shift );
}
inline int ChemPack_SetPrev( int packed, int slot )
{
	return ( packed & ~( 3 << 17 ) ) | ( ( slot & 3 ) << 17 );
}

float	ChemBrewSeconds( int slot );
int		ChemUsesPerBrew( int slot );
const char *ChemConcoctionName( int slot );

#ifdef GAME_DLL
void	Chemist_Throw( CTFPlayer *pPlayer, int slot );
void	Chemist_AirblastDispel( CTFPlayer *pPyro );
#endif

#endif // TF_CHEMIST_H
