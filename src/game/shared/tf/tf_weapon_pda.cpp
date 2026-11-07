//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

#ifdef CLIENT_DLL
#include "iinput.h"
#endif

#include "tf_weapon_pda.h"
#include "in_buttons.h"
#include "tf_gamerules.h"
#include "tf_weaponbase_gun.h"

// Server specific.
#if !defined( CLIENT_DLL )
	#include "tf_player.h"
	#include "vguiscreen.h"
// Client specific.
#else
	#include "c_tf_player.h"
	#include <igameevents.h>
	#include "tf_hud_menu_engy_build.h"
	#include "tf_hud_menu_engy_destroy.h"
	#include "tf_hud_menu_spy_disguise.h"
	#include "prediction.h"
	#include "iclientmode.h"
	#include "ienginevgui.h"
	#include <vgui/ILocalize.h>
	#include "inputsystem/iinputsystem.h"
	#include "halloween/tf_weapon_spellbook.h"
	#include "vgui/IScheme.h"
	#ifdef SIXENSE
		#include "sixense/in_sixense.h"
	#endif
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA, DT_TFWeaponPDA )

BEGIN_NETWORK_TABLE( CTFWeaponPDA, DT_TFWeaponPDA )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA )
END_PREDICTION_DATA()

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponPDA )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_pda_expansion_dispenser, CTFWeaponPDAExpansion_Dispenser );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDAExpansion_Dispenser, DT_TFWeaponPDAExpansion_Dispenser )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWeaponPDAExpansion_Dispenser, DT_TFWeaponPDAExpansion_Dispenser )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWeaponPDAExpansion_Dispenser )
END_DATADESC()

//************************************************************************************************
LINK_ENTITY_TO_CLASS( tf_pda_expansion_teleporter, CTFWeaponPDAExpansion_Teleporter );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDAExpansion_Teleporter, DT_TFWeaponPDAExpansion_Teleporter )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWeaponPDAExpansion_Teleporter, DT_TFWeaponPDAExpansion_Teleporter )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWeaponPDAExpansion_Teleporter )
END_DATADESC()


CTFWeaponPDA::CTFWeaponPDA()
{
}


void CTFWeaponPDA::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: cancel menu
//-----------------------------------------------------------------------------
void CTFWeaponPDA::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	pOwner->SelectLastItem();
}

//-----------------------------------------------------------------------------
// Purpose: toggle invis
//-----------------------------------------------------------------------------
void CTFWeaponPDA::SecondaryAttack( void )
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

#if !defined( CLIENT_DLL )

	void CTFWeaponPDA::Precache()
	{
		BaseClass::Precache();
		PrecacheVGuiScreen( GetPanelName() );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Gets info about the control panels
	//-----------------------------------------------------------------------------
	void CTFWeaponPDA::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
	{
		pPanelName = GetPanelName();
	}

#else

	float CTFWeaponPDA::CalcViewmodelBob( void )
	{
		// no bob
		return BaseClass::CalcViewmodelBob();
	}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponPDA::ShouldShowControlPanels( void )
{
	return true;
}

#ifdef CLIENT_DLL
void CTFWeaponPDA::OnDataChanged( DataUpdateType_t type )
{
	if ( m_iState != m_iOldState && GetOwner() == C_TFPlayer::GetLocalTFPlayer() )
	{
		// Was active, now not
		if ( m_iOldState == WEAPON_IS_ACTIVE && m_iState != m_iOldState )
		{
			CHudBaseBuildMenu *pBuildMenu = GetBuildMenu();
			Assert( pBuildMenu );
			if ( pBuildMenu )
			{
				pBuildMenu->SetBuilderEquipped( false );
			}
		}
		else if ( m_iState == WEAPON_IS_ACTIVE && m_iOldState == WEAPON_IS_CARRIED_BY_PLAYER ) // Was inactive, now is
		{
			CHudBaseBuildMenu *pBuildMenu = GetBuildMenu();
			Assert( pBuildMenu );
			if ( pBuildMenu )
			{
				pBuildMenu->SetBuilderEquipped( true );
			}
		}
	}

	BaseClass::OnDataChanged( type );
}


void CTFWeaponPDA::UpdateOnRemove()
{
	CHudBaseBuildMenu *pBuildMenu = GetBuildMenu();
	Assert( pBuildMenu );
	if ( pBuildMenu )
	{
		pBuildMenu->SetBuilderEquipped( false );
	}
	return BaseClass::UpdateOnRemove();
}

#endif

//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Engineer_Build, DT_TFWeaponPDA_Engineer_Build )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Engineer_Build, DT_TFWeaponPDA_Engineer_Build )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Engineer_Build )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_engineer_build, CTFWeaponPDA_Engineer_Build );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_engineer_build );

#ifdef CLIENT_DLL
CHudBaseBuildMenu *CTFWeaponPDA_Engineer_Build::GetBuildMenu() const
{ 
	return GET_HUDELEMENT( CHudMenuEngyBuild );
}
#endif // CLIENT_DLL


//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Engineer_Destroy, DT_TFWeaponPDA_Engineer_Destroy )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Engineer_Destroy, DT_TFWeaponPDA_Engineer_Destroy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Engineer_Destroy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_engineer_destroy, CTFWeaponPDA_Engineer_Destroy );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_engineer_destroy );

#ifdef CLIENT_DLL
CHudBaseBuildMenu *CTFWeaponPDA_Engineer_Destroy::GetBuildMenu() const 
{ 
	return GET_HUDELEMENT( CHudMenuEngyDestroy );
}
#endif // CLIENT_DLL

//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Spy, DT_TFWeaponPDA_Spy )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Spy, DT_TFWeaponPDA_Spy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Spy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_spy, CTFWeaponPDA_Spy );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_spy );

#ifdef CLIENT_DLL
CHudBaseBuildMenu *CTFWeaponPDA_Spy::GetBuildMenu() const
{ 
	return GET_HUDELEMENT( CHudMenuSpyDisguise );
}
#endif // CLIENT_DLL


//==============================

void CTFWeaponPDA_Spy::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	ProcessDisguiseImpulse();
	CheckDisguiseTimer();
}

void CTFWeaponPDA_Spy::ItemBusyFrame( void )
{
	BaseClass::ItemBusyFrame();

	ProcessDisguiseImpulse();
	CheckDisguiseTimer();
}

void CTFWeaponPDA_Spy::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	ProcessDisguiseImpulse();
	CheckDisguiseTimer();
}

void CTFWeaponPDA_Spy::ProcessDisguiseImpulse( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->m_Shared.ProcessDisguiseImpulse( pPlayer );
}

void CTFWeaponPDA_Spy::CheckDisguiseTimer( void )
{
	/*
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		if ( gpGlobals->curtime > pPlayer->m_Shared.GetDisguiseCompleteTime() )
		{
			pPlayer->m_Shared.CompleteDisguise();
		}
	}
	*/
}

#ifdef CLIENT_DLL

bool CTFWeaponPDA_Spy::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy && prediction->IsFirstTimePredicted() )
	{
		// let the spy pda menu know to reset
		IGameEvent *event = gameeventmanager->CreateEvent( "spy_pda_reset" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	return bDeploy;
}

#endif


bool CTFWeaponPDA_Spy::CanBeSelected( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && !pOwner->CanDisguise() )
	{
		return false;
	}

	return BaseClass::CanBeSelected();
}


bool CTFWeaponPDA_Spy::VisibleInWeaponSelection( void )
{
	if ( !CanBeSelected() )
		return false;

	return BaseClass::VisibleInWeaponSelection();
}


//-----------------------------------------------------------------------------
// PDA Expansion Slots
void CTFWeaponPDAExpansion_Dispenser::Equip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_DISPENSER );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::Equip( pOwner );
}
//-----------------------------------------------------------------------------
void CTFWeaponPDAExpansion_Dispenser::UnEquip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_DISPENSER );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::UnEquip( pOwner );
}

//-----------------------------------------------------------------------------
void CTFWeaponPDAExpansion_Teleporter::Equip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate entrance and exit
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 0 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 1 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}

		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 2 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 3 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::Equip( pOwner );
}
//-----------------------------------------------------------------------------
void CTFWeaponPDAExpansion_Teleporter::UnEquip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate entrance and exit
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 0 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 1 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}

		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 2 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 3 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::UnEquip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: Check if we should show the "destroy" panel
//-----------------------------------------------------------------------------
bool	CTFWeaponPDA_Engineer_Destroy::VisibleInWeaponSelection( void )
{
	if ( IsConsole()
#ifdef CLIENT_DLL
		|| ::input->IsSteamControllerActive()
		|| tf_build_menu_controller_mode.GetBool()
#endif 
		)
	{
		return false;
	}

	return BaseClass::VisibleInWeaponSelection();
}




//==============================
// Witch spellbook PDA
//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Spellbook, DT_TFWeaponPDA_Spellbook )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Spellbook, DT_TFWeaponPDA_Spellbook )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Spellbook )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_spellbook, CTFWeaponPDA_Spellbook );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_spellbook );

#ifdef CLIENT_DLL

CHudBaseBuildMenu *CTFWeaponPDA_Spellbook::GetBuildMenu() const
{
	return GET_HUDELEMENT( CHudMenuSpellbook );
}

bool CTFWeaponPDA_Spellbook::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy && prediction->IsFirstTimePredicted() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "spellbook_pda_reset" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	return bDeploy;
}

extern ConVar tf_simple_disguise_menu;
extern ConVar tf_disguise_menu_controller_mode;

DECLARE_HUDELEMENT( CHudMenuSpellbook );

CHudMenuSpellbook::CHudMenuSpellbook( const char *pElementName )
	: CHudBaseBuildMenu( pElementName, "HudMenuSpellbook" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i = 0; i < 9; ++i )
	{
		char buf[32];
		Q_snprintf( buf, sizeof( buf ), "class_item_red_%d", i + 1 );
		m_pClassItems_Red[i] = new EditablePanel( this, buf );

		m_pKeyIcons_Red[i] = new CIconPanel( m_pClassItems_Red[i], "NumberBg" );
		m_pKeyLabels_Red[i] = new CExLabel( m_pClassItems_Red[i], "NumberLabel", "" );
		m_pKeyLabelsNew_Red[i] = new CExLabel( m_pClassItems_Red[i], "NewNumberLabel", "" );
		m_pSpellIcons_Red[i] = new ImagePanel( m_pClassItems_Red[i], "SpellIcon" );
		m_pSpellIcons_Red[i]->SetShouldScaleImage( true );
		m_pSpellIcons_Red[i]->SetVisible( false );
		m_pCostLabels_Red[i] = new CExLabel( m_pClassItems_Red[i], "CostLabel", "" );
		m_pCostLabels_Red[i]->SetContentAlignment( vgui::Label::a_center );
		m_pCostLabels_Red[i]->SetVisible( false );

		Q_snprintf( buf, sizeof( buf ), "class_item_blue_%d", i + 1 );
		m_pClassItems_Blue[i] = new EditablePanel( this, buf );

		m_pKeyIcons_Blue[i] = new CIconPanel( m_pClassItems_Blue[i], "NumberBg" );
		m_pKeyLabels_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "NumberLabel", "" );
		m_pKeyLabelsNew_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "NewNumberLabel", "" );
		m_pSpellIcons_Blue[i] = new ImagePanel( m_pClassItems_Blue[i], "SpellIcon" );
		m_pSpellIcons_Blue[i]->SetShouldScaleImage( true );
		m_pSpellIcons_Blue[i]->SetVisible( false );
		m_pCostLabels_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "CostLabel", "" );
		m_pCostLabels_Blue[i]->SetContentAlignment( vgui::Label::a_center );
		m_pCostLabels_Blue[i]->SetVisible( false );
	}

	m_pKeyIcons_Category[0] = new CIconPanel( this, "NumberBg1" );
	m_pKeyIcons_Category[1] = new CIconPanel( this, "NumberBg2" );
	m_pKeyIcons_Category[2] = new CIconPanel( this, "NumberBg3" );
	m_pKeyLabels_Category[0] = new CExLabel( this, "NumberLabel1", "" );
	m_pKeyLabels_Category[1] = new CExLabel( this, "NumberLabel2", "" );
	m_pKeyLabels_Category[2] = new CExLabel( this, "NumberLabel3", "" );

	m_iShowingTeam = TF_TEAM_RED;

	ListenForGameEvent( "spellbook_pda_reset" );
	ListenForGameEvent( "gameui_hidden" );

	m_iSelectedItem = -1;
	m_iGroupSelection = -1;
	m_pActiveSelection = NULL;

	InvalidateLayout( false, true );

	m_bInConsoleMode = false;

	RegisterForRenderGroup( "mid" );
}

void CHudMenuSpellbook::ApplySchemeSettings( IScheme *pScheme )
{
	bool bSteamController = ::input->IsSteamControllerActive();
	bool b360Style = ( bSteamController || IsConsole() || tf_disguise_menu_controller_mode.GetBool() );

	if ( b360Style )
	{
		auto res_dir = bSteamController ? "resource/UI/disguise_menu_sc" : "resource/UI/disguise_menu_360";
		LoadControlSettings( VarArgs( "%s/HudMenuSpyDisguise.res", res_dir ) );

		m_pClassItems_Red[0]->LoadControlSettings( VarArgs( "%s/scout_red.res", res_dir ) );
		m_pClassItems_Red[1]->LoadControlSettings( VarArgs( "%s/soldier_red.res", res_dir ) );
		m_pClassItems_Red[2]->LoadControlSettings( VarArgs( "%s/pyro_red.res", res_dir ) );
		m_pClassItems_Red[3]->LoadControlSettings( VarArgs( "%s/demoman_red.res", res_dir ) );
		m_pClassItems_Red[4]->LoadControlSettings( VarArgs( "%s/heavy_red.res", res_dir ) );
		m_pClassItems_Red[5]->LoadControlSettings( VarArgs( "%s/engineer_red.res", res_dir ) );
		m_pClassItems_Red[6]->LoadControlSettings( VarArgs( "%s/medic_red.res", res_dir ) );
		m_pClassItems_Red[7]->LoadControlSettings( VarArgs( "%s/sniper_red.res", res_dir ) );
		m_pClassItems_Red[8]->LoadControlSettings( VarArgs( "%s/spy_red.res", res_dir ) );

		m_pClassItems_Blue[0]->LoadControlSettings( VarArgs( "%s/scout_blue.res", res_dir ) );
		m_pClassItems_Blue[1]->LoadControlSettings( VarArgs( "%s/soldier_blue.res", res_dir ) );
		m_pClassItems_Blue[2]->LoadControlSettings( VarArgs( "%s/pyro_blue.res", res_dir ) );
		m_pClassItems_Blue[3]->LoadControlSettings( VarArgs( "%s/demoman_blue.res", res_dir ) );
		m_pClassItems_Blue[4]->LoadControlSettings( VarArgs( "%s/heavy_blue.res", res_dir ) );
		m_pClassItems_Blue[5]->LoadControlSettings( VarArgs( "%s/engineer_blue.res", res_dir ) );
		m_pClassItems_Blue[6]->LoadControlSettings( VarArgs( "%s/medic_blue.res", res_dir ) );
		m_pClassItems_Blue[7]->LoadControlSettings( VarArgs( "%s/sniper_blue.res", res_dir ) );
		m_pClassItems_Blue[8]->LoadControlSettings( VarArgs( "%s/spy_blue.res", res_dir ) );

		m_pActiveSelection = dynamic_cast< EditablePanel * >( FindChildByName( "active_selection_bg" ) );

		m_iSelectedItem = -1;
		SetSelectedItem( 1 );
	}
	else
	{
		LoadControlSettings( "resource/UI/disguise_menu/HudMenuSpyDisguise.res" );

		m_pClassItems_Red[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_red.res" );
		m_pClassItems_Red[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_red.res" );
		m_pClassItems_Red[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_red.res" );
		m_pClassItems_Red[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_red.res" );
		m_pClassItems_Red[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_red.res" );
		m_pClassItems_Red[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_red.res" );
		m_pClassItems_Red[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_red.res" );
		m_pClassItems_Red[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_red.res" );
		m_pClassItems_Red[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_red.res" );

		m_pClassItems_Blue[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_blue.res" );
		m_pClassItems_Blue[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_blue.res" );
		m_pClassItems_Blue[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_blue.res" );
		m_pClassItems_Blue[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_blue.res" );
		m_pClassItems_Blue[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_blue.res" );
		m_pClassItems_Blue[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_blue.res" );
		m_pClassItems_Blue[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_blue.res" );
		m_pClassItems_Blue[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_blue.res" );
		m_pClassItems_Blue[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_blue.res" );

		m_pActiveSelection = NULL;
	}

	m_iGroupSelection = -1;
	ToggleSelectionIcons( false );

	BaseClass::ApplySchemeSettings( pScheme );

	vgui::IScheme *pFonts = vgui::scheme()->GetIScheme( GetScheme() );
	if ( pFonts )
	{
		vgui::HFont hCostFont = pFonts->GetFont( "HudFontSmallestBold", true );
		if ( hCostFont == vgui::INVALID_FONT )
			hCostFont = pFonts->GetFont( "DefaultSmall", true );
		for ( int i = 0; i < 9; ++i )
		{
			if ( hCostFont != vgui::INVALID_FONT )
			{
				m_pCostLabels_Red[i]->SetFont( hCostFont );
				m_pCostLabels_Blue[i]->SetFont( hCostFont );
			}
			m_pCostLabels_Red[i]->SetFgColor( Color( 235, 226, 202, 255 ) );
			m_pCostLabels_Blue[i]->SetFgColor( Color( 235, 226, 202, 255 ) );
		}
	}

	PlaceOnScreen();
	UpdateSpellItems();
}

//-----------------------------------------------------------------------------
// HudLayout positions HudMenuSpyDisguise. This panel has its own name, so size it the same way.
//-----------------------------------------------------------------------------
void CHudMenuSpellbook::PlaceOnScreen( void )
{
	vgui::Panel *pParent = GetParent();
	if ( !pParent )
		return;

	const int iWide = scheme()->GetProportionalScaledValueEx( GetScheme(), 470 );
	const int iTall = scheme()->GetProportionalScaledValueEx( GetScheme(), 200 );
	const int x = pParent->GetWide() / 2 - scheme()->GetProportionalScaledValueEx( GetScheme(), 235 );
	const int y = pParent->GetTall() / 2 - scheme()->GetProportionalScaledValueEx( GetScheme(), 52 );
	SetBounds( x, y, iWide, iTall );
	SetZPos( 20 );
}

bool CHudMenuSpellbook::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn || pPlayer != pWpn->GetOwner() || pWpn->GetWeaponID() != TF_WEAPON_PDA_SPELLBOOK )
		return false;

	return CHudElement::ShouldDraw();
}

int CHudMenuSpellbook::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
		return 1;

	if ( !down )
		return 1;

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );

	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch ( keynum )
		{
		case KEY_XBUTTON_UP:
		case STEAMCONTROLLER_DPAD_UP:
			iNewSelection = 9;
			break;

		case KEY_XBUTTON_DOWN:
		case STEAMCONTROLLER_DPAD_DOWN:
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
		case STEAMCONTROLLER_DPAD_RIGHT:
			iNewSelection++;
			if ( iNewSelection > 9 )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
		case STEAMCONTROLLER_DPAD_LEFT:
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 9;
			break;

		case KEY_XBUTTON_RTRIGGER:
		case KEY_XBUTTON_A:
		case STEAMCONTROLLER_A:
			ChooseSlot( m_iSelectedItem - 1 );
			return 0;

		case KEY_XBUTTON_B:
		case STEAMCONTROLLER_B:
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;
		}

		SetSelectedItem( iNewSelection );
		return 0;
	}

	int iSlot = -1;

#ifdef SIXENSE
	if ( !tf_simple_disguise_menu.GetBool() && !g_pSixenseInput->IsEnabled() )
#else
	if ( !tf_simple_disguise_menu.GetBool() )
#endif
	{
		if ( pszCurrentBinding && !Q_strncmp( pszCurrentBinding, "slot", 4 ) && Q_strlen( pszCurrentBinding ) > 4 )
		{
			const char *pszNum = pszCurrentBinding + 4;
			iSlot = atoi( pszNum );

			if ( iSlot == 10 )
			{
				engine->ExecuteClientCmd( "lastinv" );
				return 0;
			}

			iSlot -= 1;

			if ( iSlot < 0 || iSlot > 8 )
				return 1;
		}
	}

	if ( iSlot == -1 )
	{
		if ( m_iGroupSelection > -1 )
		{
			switch ( keynum )
			{
			case KEY_1:
			case KEY_2:
			case KEY_3:
				iSlot = m_iGroupSelection * 3 + keynum - KEY_1;
				break;
			case KEY_4:
			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
			case KEY_9:
#ifdef SIXENSE
				if ( !tf_simple_disguise_menu.GetBool() && !g_pSixenseInput->IsEnabled() )
#else
				if ( !tf_simple_disguise_menu.GetBool() )
#endif
				{
					iSlot = keynum - KEY_1;
				}
				break;

			case KEY_0:
				engine->ExecuteClientCmd( "lastinv" );
				return 0;
			}
		}
		else
		{
#ifdef SIXENSE
			if ( !tf_simple_disguise_menu.GetBool() && !g_pSixenseInput->IsEnabled() )
#else
			if ( !tf_simple_disguise_menu.GetBool() )
#endif
			{
				switch ( keynum )
				{
				case KEY_1:
				case KEY_2:
				case KEY_3:
				case KEY_4:
				case KEY_5:
				case KEY_6:
				case KEY_7:
				case KEY_8:
				case KEY_9:
					iSlot = keynum - KEY_1;
					break;

				case KEY_0:
					engine->ExecuteClientCmd( "lastinv" );
					return 0;

				default:
					return 1;
				}
			}
			else
			{
				switch ( keynum )
				{
				case KEY_1:
				case KEY_2:
				case KEY_3:
					m_iGroupSelection = keynum - KEY_1;
					ToggleSelectionIcons( true );
					return 0;
				}
			}
		}
	}

	if ( iSlot >= 0 )
	{
		ChooseSlot( iSlot );

		m_iGroupSelection = -1;
		ToggleSelectionIcons( false );
		return 0;
	}

	return 1;
}

void CHudMenuSpellbook::FindToggleBinding( void )
{
	const char *key = engine->Key_LookupBinding( "lastinv" );
	if ( !key )
	{
		key = "< not bound >";
	}
	SetDialogVariable( "lastinv", key );
	SetDialogVariable( "disguiseteam", "" );
	SetDialogVariable( "reload", "" );

	vgui::Label *pToggle = dynamic_cast< vgui::Label * >( FindChildByName( "ToggleLabel" ) );
	if ( pToggle )
	{
		pToggle->SetVisible( false );
	}
}

void CHudMenuSpellbook::ToggleSelectionIcons( bool bGroup )
{
	if ( tf_disguise_menu_controller_mode.GetBool() || ::input->IsSteamControllerActive() )
	{
		for ( int i = 0; i < 9; ++i )
		{
			m_pKeyIcons_Red[i]->SetVisible( false );
			m_pKeyLabels_Red[i]->SetVisible( false );
			m_pKeyLabelsNew_Red[i]->SetVisible( false );
			m_pKeyIcons_Blue[i]->SetVisible( false );
			m_pKeyLabels_Blue[i]->SetVisible( false );
			m_pKeyLabelsNew_Blue[i]->SetVisible( false );
		}
	}
	else
#ifdef SIXENSE
	if ( tf_simple_disguise_menu.GetBool() || g_pSixenseInput->IsEnabled() )
#else
	if ( tf_simple_disguise_menu.GetBool() )
#endif
	{
		for ( int i = 0; i < 3; ++i )
		{
			m_pKeyIcons_Category[i]->SetVisible( !bGroup );
			m_pKeyLabels_Category[i]->SetVisible( !bGroup );
		}
		for ( int i = 0; i < 9; ++i )
		{
			int index = i - ( m_iGroupSelection * 3 );
			bool bVisible = bGroup && ( ( m_iGroupSelection == -1 ) || ( ( index <= 2 ) && ( index >= 0 ) ) );
			m_pKeyIcons_Red[i]->SetVisible( bVisible );
			m_pKeyLabels_Red[i]->SetVisible( false );
			m_pKeyLabelsNew_Red[i]->SetVisible( bVisible );
			m_pKeyIcons_Blue[i]->SetVisible( bVisible );
			m_pKeyLabels_Blue[i]->SetVisible( false );
			m_pKeyLabelsNew_Blue[i]->SetVisible( bVisible );
		}
	}
	else
	{
		for ( int i = 0; i < 3; ++i )
		{
			m_pKeyIcons_Category[i]->SetVisible( false );
			m_pKeyLabels_Category[i]->SetVisible( false );
		}
		for ( int i = 0; i < 9; ++i )
		{
			m_pKeyIcons_Red[i]->SetVisible( true );
			m_pKeyLabels_Red[i]->SetVisible( true );
			m_pKeyLabelsNew_Red[i]->SetVisible( false );
			m_pKeyIcons_Blue[i]->SetVisible( true );
			m_pKeyLabels_Blue[i]->SetVisible( true );
			m_pKeyLabelsNew_Blue[i]->SetVisible( false );
		}
	}
}

void CHudMenuSpellbook::ChooseSlot( int iSlot )
{
	if ( iSlot < 0 || iSlot >= GetWitchSpellCount() )
		return;

	char szCmd[32];
	Q_snprintf( szCmd, sizeof( szCmd ), "witch_spell %d", iSlot + 1 );
	engine->ClientCmd( szCmd );
	engine->ExecuteClientCmd( "lastinv" );
}

void CHudMenuSpellbook::UpdateSpellItems( void )
{
	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	m_iShowingTeam = ( pLocal && pLocal->GetTeamNumber() == TF_TEAM_BLUE ) ? TF_TEAM_BLUE : TF_TEAM_RED;
	const bool bShowBlue = ( m_iShowingTeam == TF_TEAM_BLUE );

	vgui::Label *pTitle = dynamic_cast< vgui::Label * >( FindChildByName( "TitleLabel" ) );
	if ( pTitle )
	{
		pTitle->SetText( "Spells" );
		pTitle->SetVisible( true );
	}
	vgui::Label *pTitleShadow = dynamic_cast< vgui::Label * >( FindChildByName( "TitleLabelDropshadow" ) );
	if ( pTitleShadow )
	{
		pTitleShadow->SetText( "Spells" );
		pTitleShadow->SetVisible( true );
	}
	vgui::Label *pDisguiseTitle = dynamic_cast< vgui::Label * >( FindChildByName( "DisguiseTitle" ) );
	if ( pDisguiseTitle )
	{
		pDisguiseTitle->SetText( "Spells" );
	}

	const int nSpells = GetWitchSpellCount();
	int iHudW = 26;
	int iHudH = 26;
	CHudSpellMenu *pSpellHud = GET_HUDELEMENT( CHudSpellMenu );
	if ( pSpellHud )
		pSpellHud->GetSpellIconSize( iHudW, iHudH );
	if ( iHudW <= 0 )
		iHudW = 26;
	if ( iHudH <= 0 )
		iHudH = 26;

	for ( int i = 0; i < 9; ++i )
	{
		const bool bHasSpell = ( i < nSpells );
		m_pClassItems_Red[i]->SetVisible( bHasSpell && !bShowBlue );
		m_pClassItems_Blue[i]->SetVisible( bHasSpell && bShowBlue );

		if ( !bHasSpell )
			continue;

		const char *pszIcon = GetWitchSpellIcon( i );
		const char *pszName = GetWitchSpellUiName( i );

		EditablePanel *pPanels[2] = { m_pClassItems_Red[i], m_pClassItems_Blue[i] };
		ImagePanel *pSpellIcons[2] = { m_pSpellIcons_Red[i], m_pSpellIcons_Blue[i] };
		CExLabel *pCosts[2] = { m_pCostLabels_Red[i], m_pCostLabels_Blue[i] };
		for ( int nSide = 0; nSide < 2; ++nSide )
		{
			CIconPanel *pIcon = dynamic_cast< CIconPanel * >( pPanels[nSide]->FindChildByName( "ClassIcon" ) );
			if ( pIcon )
			{
				int x, y, w, h;
				pIcon->GetBounds( x, y, w, h );
				if ( w <= 0 )
					w = 32;
				if ( h <= 0 )
					h = w;

				// Class portraits are tall. Keep the HUD icon's height/width.
				int iDrawW = w;
				int iDrawH = ( iDrawW * iHudH + iHudW / 2 ) / iHudW;
				if ( iDrawH < 1 )
					iDrawH = 1;
				if ( iDrawH > h )
				{
					iDrawH = h;
					iDrawW = ( iDrawH * iHudW + iHudH / 2 ) / iHudH;
					if ( iDrawW < 1 )
						iDrawW = 1;
					if ( iDrawW > w )
						iDrawW = w;
				}
				pSpellIcons[nSide]->SetBounds( x + ( w - iDrawW ) / 2, y + ( h - iDrawH ) / 2, iDrawW, iDrawH );
				pIcon->SetVisible( false );
			}

			pSpellIcons[nSide]->SetVisible( true );
			pSpellIcons[nSide]->SetImage( pszIcon ? pszIcon : "" );
			pSpellIcons[nSide]->MoveToFront();

			if ( nSide == 0 )
			{
				m_pKeyIcons_Red[i]->MoveToFront();
				m_pKeyLabels_Red[i]->MoveToFront();
				m_pKeyLabelsNew_Red[i]->MoveToFront();
			}
			else
			{
				m_pKeyIcons_Blue[i]->MoveToFront();
				m_pKeyLabels_Blue[i]->MoveToFront();
				m_pKeyLabelsNew_Blue[i]->MoveToFront();
			}

			vgui::Label *pName = dynamic_cast< vgui::Label * >( pPanels[nSide]->FindChildByName( "ItemNameLabel" ) );
			if ( !pName )
				pName = dynamic_cast< vgui::Label * >( pPanels[nSide]->FindChildByName( "ItemName" ) );
			if ( pName && pszName && pszName[0] )
			{
				pName->SetText( pszName );
			}

			if ( pCosts[nSide] )
			{
				int sw, sh;
				pPanels[nSide]->GetSize( sw, sh );
				const int iCostH = scheme()->GetProportionalScaledValueEx( GetScheme(), 16 );
				if ( sw <= 0 )
					sw = scheme()->GetProportionalScaledValueEx( GetScheme(), 45 );
				if ( sh <= 0 )
					sh = scheme()->GetProportionalScaledValueEx( GetScheme(), 120 );
				pCosts[nSide]->SetBounds( 0, sh - iCostH, sw, iCostH );
				pCosts[nSide]->SetContentAlignment( vgui::Label::a_center );
				// Stealth has no lump cost. The meter drains while cloaked.
				if ( IsWitchSpellStealth( i ) )
				{
					pCosts[nSide]->SetText( "" );
					pCosts[nSide]->SetVisible( false );
				}
				else
				{
					char szCost[16];
					Q_snprintf( szCost, sizeof( szCost ), "%d", GetWitchSpellManaCostForPlayer( C_TFPlayer::GetLocalTFPlayer(), i ) );
					pCosts[nSide]->SetText( szCost );
					pCosts[nSide]->SetVisible( true );
					pCosts[nSide]->SetFgColor( Color( 235, 226, 202, 255 ) );
					pCosts[nSide]->MoveToFront();
				}
			}
		}
	}
}

void CHudMenuSpellbook::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		if ( m_pActiveSelection && m_iSelectedItem >= 1 && m_iSelectedItem <= 9 )
		{
			int x, y;
			m_pClassItems_Blue[m_iSelectedItem - 1]->GetPos( x, y );
			m_pActiveSelection->SetPos( x, y );
		}
	}
}

void CHudMenuSpellbook::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( Q_strcmp( type, "spellbook_pda_reset" ) == 0 )
	{
		m_iGroupSelection = -1;
		UpdateSpellItems();
		ToggleSelectionIcons( false );
	}
	else if ( Q_strcmp( type, "gameui_hidden" ) == 0 )
	{
		FindToggleBinding();
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

void CHudMenuSpellbook::SetVisible( bool state )
{
	if ( state == true )
	{
		PlaceOnScreen();
		engine->ClientCmd( "cancelselect" );

		bool bConsoleMode = ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() || ::input->IsSteamControllerActive() );

		if ( bConsoleMode != m_bInConsoleMode )
		{
			InvalidateLayout( true, true );
			m_bInConsoleMode = bConsoleMode;
		}

		FindToggleBinding();

		HideLowerPriorityHudElementsInGroup( "mid" );

		m_iGroupSelection = -1;
		ToggleSelectionIcons( false );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

#endif // CLIENT_DLL
