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
	if ( !pDef->IsBaseItem() && pDef->IsActingAsAWeapon() )
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

		// The selection panel already appends the base item. Listing it here
		// would show the stock weapon twice. Non-weapons (the empty hat) stay off the list.
		if ( pDef->IsBaseItem() || !pDef->IsActingAsAWeapon() )
			continue;
		if ( !pDef->CanBeUsedByClass( iClass ) || pDef->GetLoadoutSlot( iClass ) != iSlot )
			continue;

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
