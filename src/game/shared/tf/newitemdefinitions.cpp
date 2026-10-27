//========= Copyright Valve Corporation, All rights reserved. ============//
//
// New item schema. Not vanilla TF2.
//
// There is no itemdefinitions.cpp in this repo. Vanilla definitions live in
// items_game.txt and are looked up by a pure number. Everything in this file
// is an item that file does not have. Every definition id starts with 'b'
// ("b1", "b2", ...). A lookup that sees that leading letter comes here.
// Nothing in this file checks the backpack. These are stock items, so the
// player is treated as owning all of them.
//
// Compiled by #include from tf_item_schema.cpp. Do not also add this file to
// the Visual Studio project or the symbols will be defined twice.
//
//=============================================================================//

#include "cbase.h"
#include "newitemdefinitions.h"
#include "tf_item_schema.h"
#include "econ_item_view.h"
#include "tf_shareddefs.h"

struct NewItemAttribute_t
{
	const char *pszName;
	const char *pszValue;
};

struct NewItemDefDesc_t
{
	const char *pszAltId;
	const char *pszItemClass;
	const char *pszItemName;
	const char *pszSlot;
	int			iSlot;
	int			iClass;
	const char *pszPlayerModel;
	const char *pszWorldModel;
	const char *pszLogName;
	int			iAttachToHands;
	const NewItemAttribute_t *pAttrs;
	int			nAttrs;
};

static const NewItemAttribute_t s_BrickAttrs[] =
{
	{ "projectile entity name", "tf_projectile_throwable_brick" },
	{ "is throwable chargeable", "1" },
	{ "throwable detonation time", "1.5" },
	{ "throwable recharge time", "6" },
};

static const NewItemAttribute_t s_TypewriterAttrs[] =
{
	{ "bullets per shot bonus", "0.75" },
	{ "fire rate bonus", "0.34" },
	{ "damage bonus HIDDEN", "0.5" },
	{ "Reload time increased", "1.8" },
	{ "dmg bonus vs buildings", "1.15" },
	{ "spread penalty", "1.3" },
};

static const NewItemAttribute_t s_MoneyBagAttrs[] =
{
	{ "effect bar recharge rate increased", "6" },
};

// Madcap. b1 is the Nailgun (primary). b2 is the Brick (secondary).
static const NewItemDefDesc_t s_NewItems[] =
{
	{
		"b1",
		"tf_weapon_nailgun",
		"#TF_Weapon_Nailgun",
		"primary",
		LOADOUT_POSITION_PRIMARY,
		TF_CLASS_MADCAP,
		"models/weapons/w_models/w_nailgun.mdl",
		"models/weapons/w_models/w_nailgun.mdl",
		"nailgun",
		0,
		NULL,
		0,
	},
	{
		"b2",
		"tf_weapon_throwable_brick",
		"#TF_Weapon_Brick",
		"secondary",
		LOADOUT_POSITION_SECONDARY,
		TF_CLASS_MADCAP,
		"models/weapons/w_models/w_brick_scout.mdl",
		"models/weapons/w_models/w_brick_scout.mdl",
		"brick",
		0,
		s_BrickAttrs,
		ARRAYSIZE( s_BrickAttrs ),
	},
	{
		"b3",
		"tf_weapon_typewriter",
		"#TF_Weapon_TommyGun",
		"primary",
		LOADOUT_POSITION_PRIMARY,
		TF_CLASS_MOBSTER,
		"models/weapons/w_typewriter.mdl",
		"models/weapons/w_typewriter.mdl",
		"typewriter",
		0,
		s_TypewriterAttrs,
		ARRAYSIZE( s_TypewriterAttrs ),
	},
	{
		"b4",
		"tf_weapon_moneybag",
		"#TF_Weapon_MoneyBag",
		"secondary",
		LOADOUT_POSITION_SECONDARY,
		TF_CLASS_MOBSTER,
		"models/weapons/w_moneybag_closed.mdl",
		"models/weapons/w_moneybag_closed.mdl",
		"moneybag",
		0,
		s_MoneyBagAttrs,
		ARRAYSIZE( s_MoneyBagAttrs ),
	},
	{
		"b5",
		"tf_weapon_leadpipe",
		"#TF_Weapon_LeadPipe",
		"melee",
		LOADOUT_POSITION_MELEE,
		TF_CLASS_MOBSTER,
		"models/weapons/w_leadpipe.mdl",
		"models/weapons/w_leadpipe.mdl",
		"leadpipe",
		1,
		NULL,
		0,
	},
};

static const NewItemDefDesc_t *NewItem_FindDesc( item_definition_index_t iIndex )
{
	for ( int i = 0; i < ARRAYSIZE( s_NewItems ); ++i )
	{
		if ( ItemDefIndexFromAltId( s_NewItems[i].pszAltId ) == iIndex )
			return &s_NewItems[i];
	}
	return NULL;
}

static bool NewItem_AddAttributeIfPresent( CTFItemSchema *pSchema, KeyValues *pAttrs, const char *pszName, const char *pszValue )
{
	if ( !pSchema->GetAttributeDefinitionByName( pszName ) )
	{
		Warning( "New item schema: attribute \"%s\" is not in the vanilla schema. Skipping it.\n", pszName );
		return false;
	}

	KeyValues *pAttr = new KeyValues( pszName );
	pAttr->SetString( "value", pszValue );
	pAttrs->AddSubKey( pAttr );
	return true;
}

static void AddTypewriterAnimReplacements( KeyValues *pKV )
{
	// Body poses that the secondary table asks for, but this model did not compile.
	// Shove is the Shortstop push, not a normal secondary attack.
	static const char *s_pszRepl[][2] =
	{
		{ "ACT_MP_WALK_SECONDARY", "ACT_MP_RUN_SECONDARY" },
		{ "ACT_MP_JUMP_SECONDARY", "ACT_MP_JUMP_FLOAT_SECONDARY" },
		{ "ACT_MP_ATTACK_AIRWALK_SECONDARY", "ACT_MP_ATTACK_STAND_SECONDARY" },
		{ "ACT_MP_ATTACK_STAND_SECONDARYFIRE", "ACT_MP_PUSH_STAND_SECONDARY" },
		{ "ACT_MP_ATTACK_CROUCH_SECONDARYFIRE", "ACT_MP_PUSH_CROUCH_SECONDARY" },
		{ "ACT_MP_ATTACK_SWIM_SECONDARYFIRE", "ACT_MP_PUSH_SWIM_SECONDARY" },
	};

	KeyValues *pVisuals = new KeyValues( "visuals" );
	KeyValues *pRepl = new KeyValues( "animation_replacement" );
	for ( int i = 0; i < ARRAYSIZE( s_pszRepl ); ++i )
	{
		pRepl->SetString( s_pszRepl[i][0], s_pszRepl[i][1] );
	}
	pVisuals->AddSubKey( pRepl );
	pKV->AddSubKey( pVisuals );
	pKV->SetString( "anim_slot", "SECONDARY" );
}

static void AddMoneyBagAnimReplacements( KeyValues *pKV )
{
	// Body poses the item1 table asks for that this model did not compile.
	// No item2 sequences exist. Walk, the bare jump, and an air throw fall back.
	static const char *s_pszRepl[][2] =
	{
		{ "ACT_MP_WALK_ITEM1", "ACT_MP_RUN_ITEM1" },
		{ "ACT_MP_JUMP_ITEM1", "ACT_MP_JUMP_FLOAT_ITEM1" },
		{ "ACT_MP_ATTACK_AIRWALK_ITEM1", "ACT_MP_ATTACK_STAND_ITEM1" },
	};

	KeyValues *pVisuals = new KeyValues( "visuals" );
	KeyValues *pRepl = new KeyValues( "animation_replacement" );
	for ( int i = 0; i < ARRAYSIZE( s_pszRepl ); ++i )
	{
		pRepl->SetString( s_pszRepl[i][0], s_pszRepl[i][1] );
	}
	pVisuals->AddSubKey( pRepl );
	pKV->AddSubKey( pVisuals );
	pKV->SetString( "anim_slot", "ITEM1" );
}

void RegisterNewItemDefinitions( CTFItemSchema *pSchema, CUtlVector<CUtlString> *pVecErrors )
{
	if ( !pSchema )
		return;

	for ( int i = 0; i < ARRAYSIZE( s_NewItems ); ++i )
	{
		const NewItemDefDesc_t &desc = s_NewItems[i];
		if ( pSchema->GetItemDefinitionByName( desc.pszAltId ) )
			continue;

		// The key is the alt id ("b1"), not a vanilla number. BInitFromKV turns
		// that into the 0xB000+N index the rest of the engine can carry.
		KeyValues *pKV = new KeyValues( desc.pszAltId );
		pKV->SetString( "name", desc.pszAltId );
		pKV->SetString( "item_class", desc.pszItemClass );
		pKV->SetString( "item_name", desc.pszItemName );
		pKV->SetString( "item_type_name", desc.pszItemName );
		pKV->SetString( "item_logname", desc.pszLogName );
		pKV->SetString( "item_iconname", desc.pszLogName );
		pKV->SetString( "item_slot", desc.pszSlot );
		pKV->SetString( "item_quality", "normal" );
		pKV->SetInt( "baseitem", 1 );
		pKV->SetInt( "enabled", 1 );
		pKV->SetInt( "hidden", 0 );
		pKV->SetInt( "propername", 0 );
		pKV->SetInt( "min_ilevel", 1 );
		pKV->SetInt( "max_ilevel", 1 );
		pKV->SetInt( "act_as_weapon", 1 );
		pKV->SetInt( "attach_to_hands", desc.iAttachToHands );
		pKV->SetString( "model_player", desc.pszPlayerModel );
		pKV->SetString( "model_world", desc.pszWorldModel );

		KeyValues *pClasses = new KeyValues( "used_by_classes" );
		const CUtlVector<const char *> &classes = pSchema->GetClassUsabilityStrings();
		const char *pszClass = ( desc.iClass >= 0 && desc.iClass < classes.Count() ) ? classes[ desc.iClass ] : "Madcap";
		pClasses->SetString( pszClass ? pszClass : "Madcap", "1" );
		pKV->AddSubKey( pClasses );

		if ( desc.nAttrs > 0 )
		{
			KeyValues *pAttrs = new KeyValues( "attributes" );
			for ( int a = 0; a < desc.nAttrs; ++a )
			{
				NewItem_AddAttributeIfPresent( pSchema, pAttrs, desc.pAttrs[a].pszName, desc.pAttrs[a].pszValue );
			}
			pKV->AddSubKey( pAttrs );
		}

		if ( !Q_stricmp( desc.pszAltId, "b3" ) )
		{
			AddTypewriterAnimReplacements( pKV );
		}
		else if ( !Q_stricmp( desc.pszAltId, "b4" ) )
		{
			AddMoneyBagAnimReplacements( pKV );
		}

		if ( !pSchema->AddRuntimeItemDefinition( pKV, pVecErrors ) )
		{
			Warning( "New item schema: \"%s\" (%s) was not added.\n", desc.pszAltId, desc.pszItemClass );
		}
		pKV->deleteThis();
	}
}

CEconItemView *NewItem_AlwaysOwnedView( item_definition_index_t iIndex )
{
	if ( !NewItem_FindDesc( iIndex ) )
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
	s_Views.AddToTail( pView );
	return pView;
}

void NewItem_AddStockItemsForSlot( int iClass, int iSlot, CUtlVector<CEconItemView *> *pList )
{
	if ( !pList )
		return;

	for ( int i = 0; i < ARRAYSIZE( s_NewItems ); ++i )
	{
		const NewItemDefDesc_t &desc = s_NewItems[i];
		if ( desc.iClass != iClass || desc.iSlot != iSlot )
			continue;

		const item_definition_index_t iIndex = ItemDefIndexFromAltId( desc.pszAltId );
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
