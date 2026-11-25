//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Items that are not in vanilla TF2 live in scripts/items/items_mod.txt.
// Definition ids are "b1", "b2", ... and the schema stores them as 0xB000+N.
// This file does not describe those items. It only makes the local inventory
// treat them as owned, because there is no backpack item for them.
//
// Compiled by #include from tf_item_schema.cpp. Do not also add this file to
// the Visual Studio project or the symbols will be defined twice.
//
//=============================================================================//

#include "cbase.h"
#include "newitemdefinitions.h"
#include "tf_item_schema.h"
#include "econ_item_view.h"

static bool NewItem_SharesLoadoutSlot( CTFItemDefinition *pA, CTFItemDefinition *pB )
{
	if ( !pA || !pB )
		return false;

	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		if ( !pA->CanBeUsedByClass( iClass ) || !pB->CanBeUsedByClass( iClass ) )
			continue;

		const int iSlot = pA->GetLoadoutSlot( iClass );
		if ( iSlot >= 0 && iSlot == pB->GetLoadoutSlot( iClass ) )
			return true;
	}

	return false;
}

// The selection panel equips the first base item in a slot by clearing the
// slot. Later base items in that slot have to be real loadout entries.
static bool NewItem_IsFirstStockBaseItem( CTFItemDefinition *pDef )
{
	if ( !pDef || !pDef->IsBaseItem() || !GetItemSchema() )
		return false;

	const CEconItemSchema::BaseItemDefinitionMap_t &mapItems = GetItemSchema()->GetBaseItemDefinitionMap();
	for ( int it = 0; it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
	{
		CTFItemDefinition *pOther = static_cast<CTFItemDefinition *>( mapItems[it] );
		if ( !pOther || !pOther->IsBaseItem() )
			continue;

		if ( pOther->GetDefinitionIndex() == pDef->GetDefinitionIndex() )
			return true;

		if ( NewItem_SharesLoadoutSlot( pDef, pOther ) )
			return false;
	}

	return true;
}

static CTFItemDefinition *NewItem_Definition( item_definition_index_t iIndex )
{
	if ( !IsNewItemDefinitionIndex( iIndex ) )
		return NULL;

	CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinition( iIndex );
	if ( !pDef || pDef->GetDefinitionIndex() != iIndex )
		return NULL;

	return static_cast<CTFItemDefinition *>( pDef );
}

CEconItemView *NewItem_AlwaysOwnedView( item_definition_index_t iIndex )
{
	CTFItemDefinition *pDef = NewItem_Definition( iIndex );
	if ( !pDef )
		return NULL;

	static CUtlVector<CEconItemView *> s_Views;
	FOR_EACH_VEC( s_Views, i )
	{
		if ( s_Views[i] && s_Views[i]->GetItemDefIndex() == iIndex )
			return s_Views[i];
	}

	CEconItemView *pView = new CEconItemView();
	pView->Init( iIndex, AE_USE_SCRIPT_VALUE, AE_USE_SCRIPT_VALUE, 0 );
	if ( !pView->IsValid() )
	{
		delete pView;
		return NULL;
	}
	// Stock weapons keep INVALID_ITEM_ID so choosing them clears the slot.
	// An unlock needs an id the local loadout can store. It is not a Steam item.
	// Wearables that are not base items (the empty Resupply hat) are not unlocks.
	// A second stock blueprint in the same slot (speed pad, jump pad) also needs
	// an id, or equipping it would just put the slot back on the first base item.
	if ( ( !pDef->IsBaseItem() && pDef->IsActingAsAWeapon() ) || ( pDef->IsBaseItem() && !NewItem_IsFirstStockBaseItem( pDef ) ) )
		pView->SetItemID( NewItem_FakeItemID( iIndex ) );
	s_Views.AddToTail( pView );
	return pView;
}

CEconItemView *NewItem_ViewForFakeItemID( itemid_t id )
{
	if ( !NewItem_IsFakeItemID( id ) )
		return NULL;

	return NewItem_AlwaysOwnedView( NewItem_DefIndexFromFakeItemID( id ) );
}

CEconItemView *NewItem_AlwaysOwnedViewForLogName( const char *pszLogOrName )
{
	if ( !pszLogOrName || !pszLogOrName[0] || !GetItemSchema() )
		return NULL;

	const CEconItemSchema::ItemDefinitionMap_t &mapItems = GetItemSchema()->GetItemDefinitionMap();
	FOR_EACH_MAP_FAST( mapItems, i )
	{
		CTFItemDefinition *pDef = static_cast<CTFItemDefinition *>( mapItems[i] );
		if ( !pDef )
			continue;

		const char *pszLog = pDef->GetLogClassname();
		const bool bLog = pszLog && !Q_stricmp( pszLog, pszLogOrName );
		const bool bName = pDef->GetDefinitionName() && !Q_stricmp( pDef->GetDefinitionName(), pszLogOrName );
		if ( !bLog && !bName )
			continue;

		return NewItem_AlwaysOwnedView( pDef->GetDefinitionIndex() );
	}

	return NULL;
}

void NewItem_AddStockItemsForSlot( int iClass, int iSlot, CUtlVector<CEconItemView *> *pList )
{
	if ( !pList )
		return;

	const CEconItemSchema::ItemDefinitionMap_t &mapItems = GetItemSchema()->GetItemDefinitionMap();
	FOR_EACH_MAP_FAST( mapItems, i )
	{
		CTFItemDefinition *pDef = static_cast<CTFItemDefinition *>( mapItems[i] );
		if ( !pDef || !IsNewItemDefinitionIndex( pDef->GetDefinitionIndex() ) )
			continue;

		if ( pDef->IsBaseItem() )
		{
			// The selection panel appends the first base item itself. Extra
			// stock blueprints that share the slot (speed pad, jump pad) are
			// listed here so they can be equipped.
			if ( !pDef->CanBeUsedByClass( iClass ) || pDef->GetLoadoutSlot( iClass ) != iSlot )
				continue;
			if ( NewItem_IsFirstStockBaseItem( pDef ) )
				continue;
		}
		else if ( !pDef->IsActingAsAWeapon() )
		{
			continue;
		}
		else if ( !pDef->CanBeUsedByClass( iClass ) || pDef->GetLoadoutSlot( iClass ) != iSlot )
		{
			continue;
		}

		const item_definition_index_t iIndex = pDef->GetDefinitionIndex();
		bool bAlready = false;
		FOR_EACH_VEC( *pList, n )
		{
			if ( (*pList)[n] && (*pList)[n]->GetItemDefIndex() == iIndex )
			{
				bAlready = true;
				break;
			}
		}
		if ( bAlready )
			continue;

		CEconItemView *pView = NewItem_AlwaysOwnedView( iIndex );
		if ( pView )
			pList->AddToTail( pView );
	}
}
