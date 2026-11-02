//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Items that are not in vanilla TF2. Definition ids are alphanumeric and
// start with 'b' ("b1", "b2", ...) so a lookup can tell them apart from the
// numeric ids in items_game.txt. The definitions themselves are
// scripts/items/items_mod.txt.
//
//=============================================================================//

#ifndef NEWITEMDEFINITIONS_H
#define NEWITEMDEFINITIONS_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_item_constants.h"
#include "tier1/strtools.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

class CEconItemView;

// True when this is a new-schema id: a leading 'b' and then digits.
// Vanilla names also contain letters, so a letter anywhere is not the flag.
// "The Scattergun" stays in the numeric schema.
inline bool ItemDefIndexIsNewSchema( const char *pszIndex )
{
	if ( !pszIndex || ( pszIndex[0] != 'b' && pszIndex[0] != 'B' ) || !pszIndex[1] )
		return false;

	for ( const char *p = pszIndex + 1; *p; ++p )
	{
		if ( *p < '0' || *p > '9' )
			return false;
	}

	return true;
}

// "bN" is stored in the networked uint16 def index as 0xB000+N.
// 0xB000 is the letter. The id you write in this schema is still "bN".
// Attribute ids invented for the mod use the same keys. Items and
// attributes are separate maps, so "b1" can be both.
static const item_definition_index_t kNewItemDefBase = 0xB000;

inline item_definition_index_t ItemDefIndexFromAltId( const char *pszAltId )
{
	if ( !ItemDefIndexIsNewSchema( pszAltId ) )
		return INVALID_ITEM_DEF_INDEX;

	const int n = Q_atoi( pszAltId + 1 );
	if ( n <= 0 || n >= 0x1000 )
		return INVALID_ITEM_DEF_INDEX;

	return (item_definition_index_t)( kNewItemDefBase + n );
}

inline item_definition_index_t NewItem_DefIndexFromFakeItemID( itemid_t id )
{
	return (item_definition_index_t)( id & 0xFFFF );
}

// Unlocks are not Steam items. 0xB1 in the top 16 bits is how the local
// loadout remembers them. The low 16 bits are the definition index.
inline itemid_t NewItem_FakeItemID( item_definition_index_t iIndex )
{
	return 0xB100000000000000ULL | (itemid_t)iIndex;
}

inline bool NewItem_IsFakeItemID( itemid_t id )
{
	return ( id & 0xFFFF000000000000ULL ) == 0xB100000000000000ULL;
}

inline bool IsNewItemDefinitionIndex( item_definition_index_t iIndex )
{
	return iIndex > kNewItemDefBase && iIndex < (item_definition_index_t)( kNewItemDefBase + 0x1000 );
}

// New-schema items are not backpack items. The player always owns them.
CEconItemView *NewItem_AlwaysOwnedView( item_definition_index_t iIndex );
CEconItemView *NewItem_ViewForFakeItemID( itemid_t id );
void NewItem_AddStockItemsForSlot( int iClass, int iSlot, CUtlVector<CEconItemView *> *pList );

#endif // NEWITEMDEFINITIONS_H
