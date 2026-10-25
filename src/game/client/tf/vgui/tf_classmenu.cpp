//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_classmenu.h"
#include "IGameUIFuncs.h" // for key bindings
#include "tf_hud_notification_panel.h"
#include "character_info_panel.h"
#include "playerspawncache.h"
#include "iclientmode.h"
#include "econ_gcmessages.h"
#include "gc_clientsystem.h"
#include "vgui/IInput.h"
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/ScrollBarSlider.h>
#include "tf_gamerules.h"
#include "engine/IEngineSound.h"
#include "inputsystem/iinputsystem.h"

#if defined( REPLAY_ENABLED )
#include "replay/iclientreplaycontext.h"
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#include "replay/shared_defs.h"
#endif

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

ConVar _cl_classmenuopen( "_cl_classmenuopen", "0", 0, "internal cvar used to tell server when class menu is open" );

#if defined( REPLAY_ENABLED )
ConVar replay_replaywelcomedlgcount( "replay_replaywelcomedlgcount", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE | FCVAR_HIDDEN, "The number of times the replay help dialog has displayed." );
#endif

ConVar tf_mvm_classupgradehelpcount( "tf_mvm_classupgradehelpcount", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE | FCVAR_HIDDEN, "The number of times the player upgrade help dialog has displayed." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFClassTipsItemPanel::CTFClassTipsItemPanel( Panel *parent, const char *name, int iListItemID ) : BaseClass( parent, name )
{
	m_pTipIcon = new vgui::ImagePanel( this, "TipIcon" );
	m_pTipLabel = new CExLabel( this, "TipLabel", "" );

// 	m_pTipIcon = dynamic_cast<vgui::ImagePanel*>( FindChildByName( "TipIcon" ) );
// 	m_pTipLabel = dynamic_cast<CExLabel*>( FindChildByName( "TipLabel" ) );
}

CTFClassTipsItemPanel::~CTFClassTipsItemPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassTipsItemPanel::SetClassTip( const wchar_t *pwszText, const char *pszIcon )
{
	// Set tf_english string and .res icon path
	if ( m_pTipLabel && m_pTipIcon )
	{
		if ( pszIcon )
		{
			m_pTipIcon->SetImage( pszIcon );
		}
		else
		{
			m_pTipIcon->SetVisible( false );
// 			int iX, iY = 0;
// 			m_pTipLabel->GetPos( iX, iY );
// 			int nIconWidth = m_pTipIcon->GetWide();
// 			m_pTipLabel->SetPos( ( iX - nIconWidth ), iY );
		}
		m_pTipLabel->SetText( pwszText );
	}

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassTipsItemPanel::ApplySchemeSettings( IScheme* pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ClassTipsItem.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFClassTipsListPanel : public PanelListPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassTipsListPanel, PanelListPanel );

public:
	CTFClassTipsListPanel( Panel *parent, const char *panelName )
		:	PanelListPanel( parent, panelName )
	{
		m_pScrollBar = GetScrollbar();

		m_pUpArrow = new CExImageButton( this, "UpArrow", "" );
		if ( m_pUpArrow )
		{
			m_pUpArrow->AddActionSignalTarget( m_pScrollBar );
			m_pUpArrow->SetCommand(new KeyValues("ScrollButtonPressed", "index", 0));
			m_pUpArrow->GetImage()->SetShouldScaleImage( true );
			m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
			m_pUpArrow->SetAlpha( 255 );
			m_pUpArrow->SetPaintBackgroundEnabled( false );
			m_pUpArrow->SetVisible( false );
			m_pUpArrow->PassMouseTicksTo( m_pScrollBar );
			m_pUpArrow->SetImageDefault( "chalkboard_scroll_up" );
		}

		m_pDownArrow = new CExImageButton( this, "DownArrow", "" );
		if ( m_pDownArrow )
		{
			m_pDownArrow->AddActionSignalTarget( m_pScrollBar );
			m_pDownArrow->SetCommand(new KeyValues("ScrollButtonPressed", "index", 1));
			m_pDownArrow->GetImage()->SetShouldScaleImage( true );
			m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
			m_pDownArrow->SetAlpha( 255 );
			m_pDownArrow->SetPaintBackgroundEnabled( false );
			m_pDownArrow->SetVisible( false );
			m_pDownArrow->PassMouseTicksTo( m_pScrollBar );
			m_pDownArrow->SetImageDefault( "chalkboard_scroll_down" );
		}

		if ( m_pScrollBar )
		{
			m_pScrollBar->SetOverriddenButtons( m_pUpArrow, m_pDownArrow );
			m_pScrollBar->SetVisible( false );
		}

		m_pLine = new vgui::ImagePanel( this, "Line" );
		m_pBox = new vgui::ImagePanel( this, "Box" );
		if ( m_pLine )
		{
			m_pLine->SetImage( "chalkboard_scroll_line" );
			m_pLine->SetShouldScaleImage( true );
		}

		if ( m_pBox )
		{
			m_pBox->SetImage( "chalkboard_scroll_box" );
			m_pBox->SetShouldScaleImage( true );
		}

		SetScrollBarImagesVisible( false );

		vgui::ivgui()->AddTickSignal( GetVPanel() );
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	~CTFClassTipsListPanel()
	{
		ivgui()->RemoveTickSignal( GetVPanel() );
	}

private:

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	int GetListHeight( void )
	{
		int nHeight = 0;

		for ( int i = FirstItem(); i < GetItemCount(); i++ )
		{
			vgui::Panel *pClassTipsItemPanel = GetItemPanel( i );
			if ( pClassTipsItemPanel )
			{
				nHeight += pClassTipsItemPanel->GetTall();
			}
		}

		return nHeight;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void SetScrollBarImagesVisible( bool visible )
	{
		if ( m_pDownArrow && m_pDownArrow->IsVisible() != visible )
		{
			m_pDownArrow->SetVisible( visible );
			m_pDownArrow->SetEnabled( visible );
		}

		if ( m_pUpArrow && m_pUpArrow->IsVisible() != visible )
		{
			m_pUpArrow->SetVisible( visible );
			m_pUpArrow->SetEnabled( visible );
		}

		if ( m_pLine && m_pLine->IsVisible() != visible )
		{
			m_pLine->SetVisible( visible );
		}

		if ( m_pBox && m_pBox->IsVisible() != visible )
		{
			m_pBox->SetVisible( visible );
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void OnTick()
	{
		if ( !IsVisible() )
			return;

		if ( m_pDownArrow && m_pUpArrow && m_pLine && m_pBox )
		{
			if ( m_pScrollBar && m_pScrollBar->IsVisible() && GetListHeight() > GetTall() )
			{
				SetScrollBarImagesVisible ( true );

				// set the alpha on the up arrow
				int nMin, nMax;
				m_pScrollBar->GetRange( nMin, nMax );
				int nScrollPos = m_pScrollBar->GetValue();
				int nRangeWindow = m_pScrollBar->GetRangeWindow();
				int nBottom = nMax - nRangeWindow;
				if ( nBottom < 0 )
				{
					nBottom = 0;
				}

				int nAlpha = ( nScrollPos - nMin <= 0 ) ? 90 : 255;
				m_pUpArrow->SetAlpha( nAlpha );

				// set the alpha on the down arrow
				nAlpha = ( nScrollPos >= nBottom ) ? 90 : 255;
				m_pDownArrow->SetAlpha( nAlpha );

				ScrollBarSlider *pSlider = m_pScrollBar->GetSlider();
				if ( pSlider && pSlider->GetRangeWindow() > 0 )
				{
					int x, y, w, t, min, max;
					m_pLine->GetBounds( x, y, w, t );
					pSlider->GetNobPos( min, max );
					m_pBox->SetBounds( x, y + min, w, ( max - min ) );
				}
			}
			else
			{
				// turn off our images
				SetScrollBarImagesVisible ( false );
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		if ( m_pScrollBar )
		{
			m_pScrollBar->SetZPos( 500 );
			m_pUpArrow->SetZPos( 501 );
			m_pDownArrow->SetZPos( 501 );

			// turn off painting the vertical scrollbar
			m_pScrollBar->SetPaintBackgroundEnabled( false );
			m_pScrollBar->SetPaintBorderEnabled( false );
			m_pScrollBar->SetPaintEnabled( false );
			m_pScrollBar->SetScrollbarButtonsVisible( false );
			m_pScrollBar->GetButton(0)->SetMouseInputEnabled( false );
			m_pScrollBar->GetButton(1)->SetMouseInputEnabled( false );

			if ( m_pScrollBar->IsVisible() )
			{
				int nMin, nMax;
				m_pScrollBar->GetRange( nMin, nMax );
				m_pScrollBar->SetValue( nMin );

				int nScrollbarWide = m_pScrollBar->GetWide();

				int wide, tall;
				GetSize( wide, tall );

				if ( m_pUpArrow )
				{
					m_pUpArrow->SetBounds( wide - nScrollbarWide, 0, nScrollbarWide, nScrollbarWide );
					m_pUpArrow->GetImage()->SetSize( nScrollbarWide, nScrollbarWide );
				}

				if ( m_pLine )
				{
					m_pLine->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, tall - ( 2 * nScrollbarWide ) );
				}

				if ( m_pBox )
				{
					m_pBox->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, nScrollbarWide );
				}

				if ( m_pDownArrow )
				{
					m_pDownArrow->SetBounds( wide - nScrollbarWide, tall - nScrollbarWide, nScrollbarWide, nScrollbarWide );
					m_pDownArrow->GetImage()->SetSize( nScrollbarWide, nScrollbarWide );
				}

				SetScrollBarImagesVisible( false );
			}
		}
	}

	vgui::ScrollBar			*m_pScrollBar;
	CExImageButton			*m_pUpArrow;
	CExImageButton			*m_pDownArrow;
	vgui::ImagePanel		*m_pLine;
	vgui::ImagePanel		*m_pBox;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFClassTipsPanel : public EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassTipsPanel, EditablePanel );

public:
	CTFClassTipsPanel( Panel *parent, const char *panelName )
		:	EditablePanel( parent, panelName )
	{
		m_pClassTipsListPanel = new CTFClassTipsListPanel( this, "ClassTipsListPanel" );
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	~CTFClassTipsPanel()
	{
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void SetClass( int iClass )
	{
		const char *pPathID = IsX360() ? "MOD" : "GAME";
		m_fmtResFilename.sprintf( "classes/%s.res", g_aRawPlayerClassNames[ iClass ] );
		if ( !g_pFullFileSystem->FileExists( m_fmtResFilename.Access(), pPathID ) &&
			g_pFullFileSystem->FileExists( "classes/default.res", pPathID ) )
		{
			m_fmtResFilename.sprintf( "classes/default.res" );
		}

		if ( m_pClassTipsListPanel )
		{
			m_pClassTipsListPanel->DeleteAllItems();

			int nScrollToItem = 0;

			// Get tip count
			const wchar_t *wzTipCount = g_pVGuiLocalize->Find( CFmtStr( "ClassTips_%d_Count", iClass ) );
			int nTipCount = wzTipCount ? _wtoi( wzTipCount ) : 0;
			for ( int iTip = 1; iTip < nTipCount+1; ++iTip )
			{
				const wchar_t *pwszText = g_pVGuiLocalize->Find( CFmtStr( "#ClassTips_%d_%d", iClass, iTip ) );
				const wchar_t *pwszTextMvM = g_pVGuiLocalize->Find( CFmtStr( "#ClassTips_%d_%d_MvM", iClass, iTip ) );
				wchar_t *pwszIcon = g_pVGuiLocalize->Find( CFmtStr( "ClassTips_%d_%d_Icon", iClass, iTip ) );
				char szIcon[MAX_PATH];

				szIcon[0] = 0;
				if ( pwszIcon )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( pwszIcon, szIcon, sizeof( szIcon ) );
				}

				// Don't load MvM tips outside the mode
				if ( pwszTextMvM )
				{
					if ( !TFGameRules()->IsMannVsMachineMode() )
						continue;

					// If we're MvM mode, remember first MvM tip
					if ( !nScrollToItem )
						nScrollToItem = iTip;
				}

				// Create a TipsItemPanel for each tip
				if ( pwszText || pwszTextMvM )
				{
					CTFClassTipsItemPanel *pClassTipsItemPanel = new CTFClassTipsItemPanel( this, "ClassTipsItemPanel", iTip );
					if ( pwszText )
					{
						pClassTipsItemPanel->SetClassTip( pwszText, szIcon );
					}
					else if ( pwszTextMvM )
					{
						pClassTipsItemPanel->SetClassTip( pwszTextMvM, szIcon );
					}

					m_pClassTipsListPanel->AddItem( NULL, pClassTipsItemPanel );
				}
			}

			if ( m_pClassTipsListPanel->GetItemCount() > 0 )
			{
				m_pClassTipsListPanel->SetFirstColumnWidth( 0 );
				m_pClassTipsListPanel->ScrollToItem( nScrollToItem );
			}
		}

		InvalidateLayout( true, true );
	}

private:

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "Resource/UI/ClassTipsList.res" );
	}

	CTFClassTipsListPanel	*m_pClassTipsListPanel;

	CFmtStr					m_fmtResFilename;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFClassMenu::CTFClassMenu( IViewPort *pViewPort )
:	CClassMenu( pViewPort )
{
	MakePopup();

	m_mouseoverButtons.RemoveAll();

	m_iClassMenuKey = BUTTON_CODE_INVALID;
	m_iCurrentClassIndex = TF_CLASS_HEAVYWEAPONS;
	m_nClassPage = 0;
	m_flLastClassPageChange = -1.f;
	Q_memset( m_nSlotClass, 0, sizeof( m_nSlotClass ) );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#endif

	m_pTFPlayerModelPanel = NULL;
	m_pSelectAClassLabel = NULL;

	m_pClassTipsPanel = new CTFClassTipsPanel( this, "ClassTipsPanel" );

	Q_memset( m_pClassButtons, 0, sizeof( m_pClassButtons ) );

#ifndef _X360
	char tempName[MAX_PATH];
	for ( int i = 0 ; i < CLASS_COUNT_IMAGES ; ++i )
	{
		Q_snprintf( tempName, sizeof( tempName ), "countImage%d", i );
		m_ClassCountImages[i] = new CTFImagePanel( this, tempName );
	}

	m_pCountLabel = NULL;

	m_pLocalPlayerImage = new CTFImagePanel( this, "localPlayerImage" );
	m_pLocalPlayerBG = new CTFImagePanel( this, "localPlayerBG" );
	m_iLocalPlayerClass = TEAM_UNASSIGNED;

	m_pClassButtons[TF_CLASS_SCOUT] = new CExImageButton( this, "scout", "", this );
	m_pClassButtons[TF_CLASS_SOLDIER] = new CExImageButton( this, "soldier", "", this );
	m_pClassButtons[TF_CLASS_PYRO] = new CExImageButton( this, "pyro", "", this );
	m_pClassButtons[TF_CLASS_DEMOMAN] = new CExImageButton( this, "demoman", "", this );
	m_pClassButtons[TF_CLASS_MEDIC] = new CExImageButton( this, "medic", "", this );
	m_pClassButtons[TF_CLASS_HEAVYWEAPONS] = new CExImageButton( this, "heavyweapons", "", this );
	m_pClassButtons[TF_CLASS_SNIPER] = new CExImageButton( this, "sniper", "", this );
	m_pClassButtons[TF_CLASS_ENGINEER] = new CExImageButton( this, "engineer", "", this );
	m_pClassButtons[TF_CLASS_SPY] = new CExImageButton( this, "spy", "", this );
	m_pClassButtons[TF_CLASS_RANDOM] = new CExImageButton( this, "random", "", this );
#endif

	m_pEditLoadoutButton = NULL;
	m_nBaseMusicGuid = -1;

	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "show_match_summary" );

	Q_memset( m_pMvmUpgradeImages, 0, sizeof( m_pMvmUpgradeImages ) );
	m_pMvmUpgradeImages[TF_CLASS_SCOUT] = new vgui::ImagePanel( this, "MvMUpgradeImageScout" );
	m_pMvmUpgradeImages[TF_CLASS_SOLDIER] = new vgui::ImagePanel( this, "MvMUpgradeImageSolider" );
	m_pMvmUpgradeImages[TF_CLASS_PYRO] = new vgui::ImagePanel( this, "MvMUpgradeImagePyro" );
	m_pMvmUpgradeImages[TF_CLASS_DEMOMAN] = new vgui::ImagePanel( this, "MvMUpgradeImageDemoman" );
	m_pMvmUpgradeImages[TF_CLASS_MEDIC] = new vgui::ImagePanel( this, "MvMUpgradeImageMedic" );
	m_pMvmUpgradeImages[TF_CLASS_HEAVYWEAPONS] = new vgui::ImagePanel( this, "MvMUpgradeImageHeavy" );
	m_pMvmUpgradeImages[TF_CLASS_SNIPER] = new vgui::ImagePanel( this, "MvMUpgradeImageSniper" );
	m_pMvmUpgradeImages[TF_CLASS_ENGINEER] = new vgui::ImagePanel( this, "MvMUpgradeImageEngineer" );
	m_pMvmUpgradeImages[TF_CLASS_SPY] = new vgui::ImagePanel( this, "MvMUpgradeImageSpy" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	for ( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		m_pClassHintIcons[i] = nullptr;
	}

	// Load the .res file
	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/ClassSelection_SC.res" );
		m_pCancelHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "CancelHintIcon" ) );
		m_pEditLoadoutHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "EditLoadoutHintIcon" ) );

		m_pClassHintIcons[TF_CLASS_SCOUT] = dynamic_cast< CSCHintIcon* >( FindChildByName( "ScoutHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_SOLDIER] = dynamic_cast< CSCHintIcon* >( FindChildByName( "SoldierHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_PYRO] = dynamic_cast< CSCHintIcon* >( FindChildByName( "PyroHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_DEMOMAN] = dynamic_cast< CSCHintIcon* >( FindChildByName( "DemomanHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_HEAVYWEAPONS] = dynamic_cast< CSCHintIcon* >( FindChildByName( "HeavyHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_MEDIC] = dynamic_cast< CSCHintIcon* >( FindChildByName( "MedicHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_SPY] = dynamic_cast< CSCHintIcon* >( FindChildByName( "SpyHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_ENGINEER] = dynamic_cast< CSCHintIcon* >( FindChildByName( "EngineerHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_SNIPER] = dynamic_cast< CSCHintIcon* >( FindChildByName( "SniperHintIcon" ) );
		m_pClassHintIcons[TF_CLASS_RANDOM] = dynamic_cast< CSCHintIcon* >( FindChildByName( "RandomHintIcon" ) );

		for ( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
		{
			if ( m_pClassHintIcons[i] )
			{
				m_pClassHintIcons[i]->SetVisible( false );
			}
		}

		SetMouseInputEnabled( false );
	}
	else
	{
		LoadControlSettings( "Resource/UI/ClassSelection.res" );
		m_pCancelHintIcon = m_pEditLoadoutHintIcon = nullptr;
		SetMouseInputEnabled( true );
	}

	m_pTFPlayerModelPanel = dynamic_cast<CTFPlayerModelPanel*>( FindChildByName("TFPlayerModel") );
	m_pSelectAClassLabel = dynamic_cast<CExLabel*>( FindChildByName( "ClassMenuSelect" ) );
	m_pEditLoadoutButton = dynamic_cast<CExButton*>( FindChildByName( "EditLoadoutButton" ) );

	const char *pTeamExtension = GetTeamNumber() == TF_TEAM_BLUE ? "blu" : "red";
	for ( int i = 0; i < ARRAYSIZE( m_pClassButtons ); ++i )
	{
		if ( !m_pClassButtons[i] )
			continue;

		if ( !IsValidTFPlayerClass( i ) && i != TF_CLASS_RANDOM )
			continue;

		m_pClassButtons[i]->SetImageSelected( CFmtStr( "class_sel_sm_%s_%s", g_aRawPlayerClassNamesShort[i], pTeamExtension ).Access() );
		if( i != TF_CLASS_RANDOM )
		{
			m_pClassButtons[i]->SetArmedSound("misc/null.wav");
		}
	}

	// Res file commands and icons describe the original nine. Repaint whatever page is up,
	// and point Random at TF_CLASS_RANDOM (the shipped res still says "select 12").
	ApplyClassPage();
}

void CTFClassMenu::PerformLayout()
{
	BaseClass::PerformLayout();

#ifndef _X360
	m_pCountLabel = dynamic_cast< CExLabel * >( FindChildByName( "CountLabel" ) );

	if ( m_pCountLabel )
	{
		m_pCountLabel->SizeToContents();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFClassMenu::GetCurrentPlayerClass()
{
	int iClass = TF_CLASS_HEAVYWEAPONS;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED )
	{
		iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	}

	return iClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton *CTFClassMenu::GetCurrentClassButton()
{
	for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
	{
		if ( m_nSlotClass[ nSlot ] == m_iCurrentClassIndex )
			return m_pClassButtons[ iRemapIndexToClass[ nSlot + 1 ] ];
	}

	if ( m_iCurrentClassIndex == TF_CLASS_RANDOM )
		return m_pClassButtons[ TF_CLASS_RANDOM ];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::ShowPanel( bool bShow )
{
	if ( bShow )
	{
		// Hide the other class menu
		if ( gViewPortInterface )
		{
			gViewPortInterface->ShowPanel( GetTeamNumber() == TF_TEAM_BLUE ? PANEL_CLASS_RED : PANEL_CLASS_BLUE, false );
		}

		// can't change class if you're on the losing team during the "bonus time" after a team has won the round
		if ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
			 C_TFPlayer::GetLocalTFPlayer() && 
			 C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TFGameRules()->GetWinningTeam()
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_SPECTATOR 
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_UNASSIGNED
			 && GetSpectatorMode() == OBS_MODE_NONE ) ||
			 TFGameRules()->State_Get() == GR_STATE_GAME_OVER ||
			( TFGameRules()->IsInTraining() && C_TFPlayer::GetLocalTFPlayer() &&
			  ( C_TFPlayer::GetLocalTFPlayer()->GetPlayerClass() == NULL || C_TFPlayer::GetLocalTFPlayer()->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) ) )
		{
			SetVisible( false );

			CHudNotificationPanel *pNotifyPanel = GET_HUDELEMENT( CHudNotificationPanel );
			if ( pNotifyPanel )
			{
				if ( C_TFPlayer::GetLocalTFPlayer() )
				{
					pNotifyPanel->SetupNotifyCustom( "#TF_CantChangeClassNow", "ico_notify_flag_moving", C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() );
				}
			}

			return;
		}

		engine->CheckPoint( "ClassMenu" );

		// Open on the page that actually contains this player's class.
		m_nClassPage = PageForClass( GetCurrentPlayerClass() );

		// Force us to reload our scheme, in case Steam Controller stuff has changed.
		InvalidateLayout( true, true );

		Activate();

		m_iClassMenuKey = gameuifuncs->GetButtonCodeForBind( "changeclass" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		SelectClass( GetCurrentPlayerClass() );
	}
	else
	{
		SetVisible( false );
		if ( m_pTFPlayerModelPanel )
		{
			m_pTFPlayerModelPanel->ClearCarriedItems();
		}
	}
}

const char *g_pszLegacyClassSelectVCDWeapons[TF_LAST_NORMAL_CLASS] =
{
	"",										// TF_CLASS_UNDEFINED = 0,
	"",										// TF_CLASS_SCOUT,				// weapons handled individually
	"",										// TF_CLASS_SNIPER,				// weapons handled individually
	"",										// TF_CLASS_SOLDIER,			// weapons handled individually
	"tf_weapon_grenadelauncher",			// TF_CLASS_DEMOMAN,
	"tf_weapon_medigun",					// TF_CLASS_MEDIC,
	"tf_weapon_minigun",					// TF_CLASS_HEAVYWEAPONS,
	"tf_weapon_flamethrower",				// TF_CLASS_PYRO,
	"",										// TF_CLASS_SPY,				// weapons handled individually
	"tf_weapon_wrench",						// TF_CLASS_ENGINEER,
	"",										// TF_CLASS_MADCAP,
};

int g_iLegacyClassSelectWeaponSlots[TF_LAST_NORMAL_CLASS] =
{
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_UNDEFINED = 0,
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_SNIPER,
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_SOLDIER,
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_DEMOMAN,
	LOADOUT_POSITION_SECONDARY,		// TF_CLASS_MEDIC,
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_HEAVYWEAPONS,
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_PYRO,
	LOADOUT_POSITION_MELEE,			// TF_CLASS_SPY,
	LOADOUT_POSITION_MELEE,			// TF_CLASS_ENGINEER,
	LOADOUT_POSITION_PRIMARY,		// TF_CLASS_MADCAP,
};

//-----------------------------------------------------------------------------
// Class pages. The row letter is the page (A, then B, ...). The column is the
// button on that page and the 1-9 key that picks it. Key 0 is Random on every
// page. TF_CLASS_UNDEFINED is an empty slot: the button stays hidden and the
// key does nothing.
//
// Add a class by writing it into a slot. This table is not filled by walking
// the class enum. pszPortrait is the class_sel short name; NULL uses the
// class's own short name. Madcap has no class_sel art yet, so B1 draws Scout's.
//-----------------------------------------------------------------------------
struct ClassMenuSlot_t
{
	int			iClass;
	const char	*pszPortrait;
};

static const ClassMenuSlot_t g_ClassMenuPages[][9] =
{
	// A
	{
		{ TF_CLASS_SCOUT,			NULL },		// A1
		{ TF_CLASS_SOLDIER,			NULL },		// A2
		{ TF_CLASS_PYRO,			NULL },		// A3
		{ TF_CLASS_DEMOMAN,			NULL },		// A4
		{ TF_CLASS_HEAVYWEAPONS,	NULL },		// A5
		{ TF_CLASS_ENGINEER,		NULL },		// A6
		{ TF_CLASS_MEDIC,			NULL },		// A7
		{ TF_CLASS_SNIPER,			NULL },		// A8
		{ TF_CLASS_SPY,				NULL },		// A9
	},
	// B
	{
		{ TF_CLASS_MADCAP,			"scout" },	// B1
		{ TF_CLASS_UNDEFINED,		NULL },		// B2
		{ TF_CLASS_UNDEFINED,		NULL },		// B3
		{ TF_CLASS_UNDEFINED,		NULL },		// B4
		{ TF_CLASS_UNDEFINED,		NULL },		// B5
		{ TF_CLASS_UNDEFINED,		NULL },		// B6
		{ TF_CLASS_UNDEFINED,		NULL },		// B7
		{ TF_CLASS_UNDEFINED,		NULL },		// B8
		{ TF_CLASS_UNDEFINED,		NULL },		// B9
	},
};

static bool ClassMenuButtonBoundTo( ButtonCode_t code, const char *pszCommand )
{
	if ( code <= BUTTON_CODE_NONE || !pszCommand || !pszCommand[0] )
		return false;

	const char *pszBind = engine->Key_BindingForKey( code );
	if ( !pszBind || !pszBind[0] )
		return false;

	const int nCmd = V_strlen( pszCommand );
	if ( V_strnicmp( pszBind, pszCommand, nCmd ) != 0 )
		return false;

	const char cNext = pszBind[ nCmd ];
	return cNext == '\0' || cNext == ' ' || cNext == '\t' || cNext == ';';
}

int CTFClassMenu::ClassPageCount( void )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE( g_ClassMenuPages[0] ) == CLASS_MENU_SLOT_COUNT );
	return ARRAYSIZE( g_ClassMenuPages );
}

int CTFClassMenu::ClassForPageSlot( int nPage, int nSlot )
{
	if ( nPage < 0 || nPage >= ClassPageCount() )
		return TF_CLASS_UNDEFINED;
	if ( nSlot < 0 || nSlot >= CLASS_MENU_SLOT_COUNT )
		return TF_CLASS_UNDEFINED;

	return g_ClassMenuPages[ nPage ][ nSlot ].iClass;
}

int CTFClassMenu::PageForClass( int iClass )
{
	if ( iClass <= TF_CLASS_UNDEFINED )
		return 0;

	const int nPages = ClassPageCount();
	for ( int nPage = 0; nPage < nPages; ++nPage )
	{
		for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
		{
			if ( g_ClassMenuPages[ nPage ][ nSlot ].iClass == iClass )
				return nPage;
		}
	}

	return 0;
}

const char *CTFClassMenu::PortraitShortName( int iClass )
{
	const int nPages = ClassPageCount();
	for ( int nPage = 0; nPage < nPages; ++nPage )
	{
		for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
		{
			if ( g_ClassMenuPages[ nPage ][ nSlot ].iClass != iClass )
				continue;

			const char *pszPortrait = g_ClassMenuPages[ nPage ][ nSlot ].pszPortrait;
			if ( pszPortrait && pszPortrait[0] )
				return pszPortrait;

			if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_CLASS_MENU_BUTTONS && g_aRawPlayerClassNamesShort[ iClass ][0] )
				return g_aRawPlayerClassNamesShort[ iClass ];
		}
	}

	if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_CLASS_MENU_BUTTONS && g_aRawPlayerClassNamesShort[ iClass ][0] )
		return g_aRawPlayerClassNamesShort[ iClass ];

	return "scout";
}

int CTFClassMenu::ClassShownOnButton( int iButton ) const
{
	if ( iButton == TF_CLASS_RANDOM )
		return TF_CLASS_RANDOM;

	for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
	{
		if ( iRemapIndexToClass[ nSlot + 1 ] == iButton )
			return m_nSlotClass[ nSlot ];
	}

	return TF_CLASS_UNDEFINED;
}

void CTFClassMenu::ApplyClassPage( void )
{
	const int nPages = ClassPageCount();
	if ( m_nClassPage < 0 || m_nClassPage >= nPages )
		m_nClassPage = 0;

	const char *pTeamExtension = GetTeamNumber() == TF_TEAM_BLUE ? "blu" : "red";

	for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
	{
		const int iAnchor = iRemapIndexToClass[ nSlot + 1 ];
		const int iClass = ClassForPageSlot( m_nClassPage, nSlot );
		m_nSlotClass[ nSlot ] = iClass;

		CExImageButton *pButton = ( iAnchor > 0 && iAnchor < ARRAYSIZE( m_pClassButtons ) ) ? m_pClassButtons[ iAnchor ] : NULL;
		if ( !pButton )
			continue;

		if ( !IsValidTFPlayerClass( iClass ) )
		{
			pButton->SetVisible( false );
			pButton->SetCommand( "" );
			continue;
		}

		pButton->SetVisible( true );
		pButton->SetCommand( CFmtStr( "select %d", iClass ) );

		const char *pszShort = PortraitShortName( iClass );
		pButton->SetImageDefault( CFmtStr( "class_sel_sm_%s_inactive", pszShort ) );
		pButton->SetImageArmed( CFmtStr( "class_sel_sm_%s_inactive", pszShort ) );
		pButton->SetImageSelected( CFmtStr( "class_sel_sm_%s_%s", pszShort, pTeamExtension ) );
	}

	if ( m_pClassButtons[ TF_CLASS_RANDOM ] )
	{
		// Shipped ClassSelection.res hardcodes "select 12", which is no longer TF_CLASS_RANDOM.
		m_pClassButtons[ TF_CLASS_RANDOM ]->SetCommand( CFmtStr( "select %d", TF_CLASS_RANDOM ) );
	}

	UpdateButtonSelectionStates( m_iCurrentClassIndex );
}

void CTFClassMenu::ChangeClassPage( int nDir, bool bFromWheel )
{
	if ( nDir == 0 )
		return;

	const int nPages = ClassPageCount();
	if ( nPages <= 1 )
		return;

	// One wheel notch is delivered as both a key and OnMouseWheeled, sometimes
	// a frame apart. Drop the second so two pages don't wrap straight back.
	if ( bFromWheel )
	{
		const float flNow = Plat_FloatTime();
		if ( m_flLastClassPageChange >= 0.f && ( flNow - m_flLastClassPageChange ) < 0.1f )
			return;
		m_flLastClassPageChange = flNow;
	}

	int nSlot = -1;
	for ( int i = 0; i < CLASS_MENU_SLOT_COUNT; ++i )
	{
		if ( m_nSlotClass[ i ] == m_iCurrentClassIndex )
		{
			nSlot = i;
			break;
		}
	}

	int nPage = m_nClassPage + ( nDir > 0 ? 1 : -1 );
	if ( nPage < 0 )
		nPage = nPages - 1;
	else if ( nPage >= nPages )
		nPage = 0;

	m_nClassPage = nPage;
	ApplyClassPage();
	CheckMvMUpgrades();

	int iClass = ( nSlot >= 0 ) ? m_nSlotClass[ nSlot ] : TF_CLASS_UNDEFINED;
	if ( !IsValidTFPlayerClass( iClass ) )
	{
		for ( int i = 0; i < CLASS_MENU_SLOT_COUNT; ++i )
		{
			if ( IsValidTFPlayerClass( m_nSlotClass[ i ] ) )
			{
				iClass = m_nSlotClass[ i ];
				break;
			}
		}
	}

	if ( IsValidTFPlayerClass( iClass ) )
		SelectClass( iClass );
}

void CTFClassMenu::CycleClassHighlight( int nDir )
{
	int choices[ CLASS_MENU_SLOT_COUNT + 1 ];
	int nChoices = 0;
	for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
	{
		if ( IsValidTFPlayerClass( m_nSlotClass[ nSlot ] ) )
			choices[ nChoices++ ] = m_nSlotClass[ nSlot ];
	}
	choices[ nChoices++ ] = TF_CLASS_RANDOM;

	int idx = -1;
	for ( int i = 0; i < nChoices; ++i )
	{
		if ( choices[ i ] == m_iCurrentClassIndex )
		{
			idx = i;
			break;
		}
	}

	if ( idx < 0 )
		idx = ( nDir > 0 ) ? 0 : ( nChoices - 1 );
	else
		idx = ( idx + ( nDir > 0 ? 1 : -1 ) + nChoices ) % nChoices;

	SelectClass( choices[ idx ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::UpdateButtonSelectionStates( int iClass )
{
	// Highlight the button currently showing this class, not the button's stock class.
	for ( int i = 0; i < ARRAYSIZE( m_pClassButtons ); ++i )
	{
		if ( !m_pClassButtons[ i ] )
			continue;

		const int iShown = ClassShownOnButton( i );
		m_pClassButtons[ i ]->SetSelected( iShown != TF_CLASS_UNDEFINED && iShown == iClass );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::SelectClass( int iClass )
{
	if ( !engine->IsInGame() )
		return;
	
	if ( !m_pTFPlayerModelPanel )
		return;

	if ( !m_pClassTipsPanel )
		return;

	if ( !m_pEditLoadoutButton )
		return;

	// Update select hint icon for Steam Controller. Hints are placed on the nine
	// stock buttons, so show the hint of the button currently displaying this class.
	for ( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		if ( m_pClassHintIcons[i] )
		{
			m_pClassHintIcons[i]->SetVisible( false );
		}
	}
	if ( iClass == TF_CLASS_RANDOM )
	{
		if ( m_pClassHintIcons[TF_CLASS_RANDOM] )
			m_pClassHintIcons[TF_CLASS_RANDOM]->SetVisible( true );
	}
	else
	{
		for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
		{
			if ( m_nSlotClass[nSlot] != iClass )
				continue;

			const int iAnchor = iRemapIndexToClass[ nSlot + 1 ];
			if ( iAnchor > 0 && iAnchor < TF_CLASS_MENU_BUTTONS && m_pClassHintIcons[iAnchor] )
				m_pClassHintIcons[iAnchor]->SetVisible( true );
			break;
		}
	}

	// Were we random? If so, we'll force our class to refresh later to prevent the
	// model panel thinking the class hasn't changed.
	const bool bClassWasRandom = m_iCurrentClassIndex == TF_CLASS_RANDOM;

	// Cache current player class
	m_iCurrentClassIndex = iClass;

	UpdateButtonSelectionStates( iClass );

	bool bRandomClass = iClass == TF_CLASS_RANDOM;
	if ( !IsValidTFPlayerClass( iClass ) && !bRandomClass )
	{
		m_pTFPlayerModelPanel->SetVisible( false );
		m_pTFPlayerModelPanel->ClearCarriedItems();
		return;
	}

	m_pTFPlayerModelPanel->SetVisible( true );
	m_pTFPlayerModelPanel->ClearCarriedItems();

	if ( bRandomClass )
	{
		m_pEditLoadoutButton->SetVisible( false );
		if ( m_pEditLoadoutHintIcon )
		{
			m_pEditLoadoutHintIcon->SetVisible( false );
		}

		MDLHandle_t hModel = mdlcache->FindMDL( "models/class_menu/random_class_icon.mdl" );
		m_pTFPlayerModelPanel->SetMDL( hModel );
		m_pTFPlayerModelPanel->SetSequence( ACT_IDLE );
		m_pTFPlayerModelPanel->InvalidateLayout( true, true ); // Updates position
		m_pTFPlayerModelPanel->SetSkin( GetTeamNumber() == TF_TEAM_RED ? 0 : 1 );
		mdlcache->Release( hModel ); // counterbalance addref from within FindMDL
	}
	else
	{
		m_pTFPlayerModelPanel->SetToPlayerClass( iClass, bClassWasRandom );

		m_pEditLoadoutButton->SetVisible( true );
		if ( m_pEditLoadoutHintIcon )
		{
			m_pEditLoadoutHintIcon->SetVisible( true );
		}

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			int iTeam = GetTeamNumber();
			m_pTFPlayerModelPanel->SetTeam( iTeam );
		}

		LoadItems();
	}

	m_pClassTipsPanel->SetClass( iClass );

	enginesound->StopSoundByGuid( m_nBaseMusicGuid );
	CBroadcastRecipientFilter filter;
	char nClassMusicStr[64];
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		sprintf( nClassMusicStr, "music.mvm_class_menu_0%i", iClass );
	}
	else
	{
		sprintf( nClassMusicStr, "music.class_menu_0%i", iClass );
	}
	CBaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, nClassMusicStr );
	m_nBaseMusicGuid = enginesound->GetGuidForLastSoundEmitted();
	

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::LoadItems()
{
	const int iClass = m_pTFPlayerModelPanel->GetPlayerClass();

	m_pTFPlayerModelPanel->ClearCarriedItems();

	static CSchemaAttributeDefHandle pAttrDef_DisableFancyLoadoutAnim( "disable fancy class select anim" );
	bool bCanUseFancyClassSelectAnimation = true;

	static CSchemaAttributeDefHandle pAttrDef_ClassSelectOverrideVCD( "class select override vcd" );
	CAttribute_String attrClassSelectOverrideVCD;

	const char *pszVCD = "class_select";

	for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
	{
		CEconItemView *pItemData = TFInventoryManager()->GetItemInLoadoutForClass( iClass, i );
		if ( pItemData && pItemData->IsValid() )
		{
			m_pTFPlayerModelPanel->AddCarriedItem( pItemData );

			// Certain items have different shapes and would interfere with our class select animations.
			bCanUseFancyClassSelectAnimation = bCanUseFancyClassSelectAnimation
											&& !pItemData->FindAttribute( pAttrDef_DisableFancyLoadoutAnim );

			// Some items want to override the class select VCD
			if ( pItemData->FindAttribute( pAttrDef_ClassSelectOverrideVCD, &attrClassSelectOverrideVCD ) )
			{
				const char *pszClassSelectOverrideVCD = attrClassSelectOverrideVCD.value().c_str();
				if ( pszClassSelectOverrideVCD && *pszClassSelectOverrideVCD )
				{
					pszVCD = pszClassSelectOverrideVCD;
				}
			}
		}
	}

	const char *pszRequired = "";
	int iHoldSlot = LOADOUT_POSITION_PRIMARY;
	if ( iClass >= 0 && iClass < TF_LAST_NORMAL_CLASS )
	{
		if ( g_pszLegacyClassSelectVCDWeapons[iClass] )
			pszRequired = g_pszLegacyClassSelectVCDWeapons[iClass];
		iHoldSlot = g_iLegacyClassSelectWeaponSlots[iClass];
	}

	m_pTFPlayerModelPanel->PlayVCD( bCanUseFancyClassSelectAnimation ? pszVCD : NULL, pszRequired );
	m_pTFPlayerModelPanel->HoldItemInSlot( iHoldSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnKeyCodePressed( KeyCode code )
{
	// Same binds that cycle weapons. Checked before the number keys so a rebind
	// of invnext onto 1-9 pages instead of joining. Mouse wheel is not fed to
	// the key-repeat handler; one notch already arrives twice in a frame.
	const bool bNextPage = ClassMenuButtonBoundTo( code, "invnext" );
	const bool bPrevPage = ClassMenuButtonBoundTo( code, "invprev" );
	if ( code != MOUSE_WHEEL_UP && code != MOUSE_WHEEL_DOWN )
	{
		m_KeyRepeat.KeyDown( code );
	}

	if ( bNextPage )
	{
		const bool bWheel = ( code == MOUSE_WHEEL_UP || code == MOUSE_WHEEL_DOWN );
		ChangeClassPage( 1, bWheel );
		return;
	}
	else if ( bPrevPage )
	{
		const bool bWheel = ( code == MOUSE_WHEEL_UP || code == MOUSE_WHEEL_DOWN );
		ChangeClassPage( -1, bWheel );
		return;
	}
	else if ( code > KEY_0 && code <= KEY_9 )
	{
		const int iButton = code - KEY_0;
		const int iClass = m_nSlotClass[ iButton - 1 ];
		if ( !IsValidTFPlayerClass( iClass ) )
			return;

		SelectClass( iClass );
		Go();
	}
	else if ( code > KEY_PAD_0 && code <= KEY_PAD_9 )
	{
		const int iButton = code - KEY_PAD_0;
		const int iClass = m_nSlotClass[ iButton - 1 ];
		if ( !IsValidTFPlayerClass( iClass ) )
			return;

		SelectClass( iClass );
		Go();
	}
	else if ( code == KEY_0 || code == KEY_PAD_0 )
	{
		SelectClass( TF_CLASS_RANDOM );
		Go();
	}
	else if( code == KEY_ENTER || code == KEY_SPACE || code == KEY_XBUTTON_A || code == KEY_XBUTTON_RTRIGGER || code == STEAMCONTROLLER_A )
	{
		Go();
	}
	else if ( ( m_iClassMenuKey != BUTTON_CODE_INVALID && m_iClassMenuKey == code ) ||
		code == KEY_XBUTTON_BACK || 
		code == KEY_XBUTTON_B ||
		code == STEAMCONTROLLER_B )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) )
		{
			ShowPanel( false );
		}
	}
	else if( code == KEY_XBUTTON_RIGHT || code == KEY_XSTICK1_RIGHT || code == STEAMCONTROLLER_DPAD_RIGHT )
	{
		CycleClassHighlight( 1 );
	}
	else if ( code == STEAMCONTROLLER_Y )
	{
		OnCommand( "openloadout" );
	}
	else if( code == KEY_XBUTTON_LEFT || code == KEY_XSTICK1_LEFT || code == STEAMCONTROLLER_DPAD_LEFT )
	{
		CycleClassHighlight( -1 );
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP || code == STEAMCONTROLLER_DPAD_UP )
	{
		// Scroll class info text up
		if ( g_lastPanel )
		{
			CExRichText *pRichText = dynamic_cast< CExRichText * >( g_lastPanel->FindChildByName( "classInfo" ) );

			if ( pRichText )
			{
				PostMessage( pRichText, new KeyValues("MoveScrollBarDirect", "delta", 1) );
			}
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN || code == STEAMCONTROLLER_DPAD_DOWN )
	{
		// Scroll class info text up
		if ( g_lastPanel )
		{
			CExRichText *pRichText = dynamic_cast< CExRichText * >( g_lastPanel->FindChildByName( "classInfo" ) );

			if ( pRichText )
			{
				PostMessage( pRichText, new KeyValues("MoveScrollBarDirect", "delta", -1) );
			}
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: Wheel is a weapon-switch bind by default. Only steal it when it
// actually is one. The same notch also arrives as a key; ChangeClassPage
// ignores that second delivery.
//-----------------------------------------------------------------------------
void CTFClassMenu::OnMouseWheeled( int delta )
{
	if ( delta != 0 )
	{
		const ButtonCode_t code = ( delta > 0 ) ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
		if ( ClassMenuButtonBoundTo( code, "invnext" ) )
		{
			ChangeClassPage( 1, true );
			return;
		}
		if ( ClassMenuButtonBoundTo( code, "invprev" ) )
		{
			ChangeClassPage( -1, true );
			return;
		}
	}

	BaseClass::OnMouseWheeled( delta );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	// Get mouse cursor position
	int aCursorPos[2];
	vgui::input()->GetCursorPos( aCursorPos[0], aCursorPos[1] );

	// Go through all buttons - if the mouse is within one, select the class it currently shows
	for ( int i = 0; i < ARRAYSIZE( m_pClassButtons ); ++i )
	{
		if ( !m_pClassButtons[ i ] || !m_pClassButtons[ i ]->IsVisible() )
			continue;

		const int iShown = ClassShownOnButton( i );
		if ( iShown == TF_CLASS_UNDEFINED || m_iCurrentClassIndex == iShown )
			continue;

		if ( m_pClassButtons[ i ]->IsWithin( aCursorPos[0], aCursorPos[1] ) )
		{
			SelectClass( iShown );
		}
	}

	//Always hide the health... this needs to be done every frame because a message from the server keeps resetting this.
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::SetCancelButtonVisible( bool bVisible )
{
	SetVisibleButton( "CancelButton", bVisible );
	if ( m_pCancelHintIcon )
	{
		m_pCancelHintIcon->SetVisible( bVisible );
	}
	
	if ( m_pSelectAClassLabel )
	{
		m_pSelectAClassLabel->SetVisible( !bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::Update()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED ) )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", true );
		}
#else
		SetCancelButtonVisible( true );

		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
		{
			SetVisibleButton( "ResetButton", true );
		}
		else
		{
			SetVisibleButton( "ResetButton", false );
		}
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", false );
		}
#else
		SetCancelButtonVisible( false );
		SetVisibleButton( "ResetButton", false );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Panel *CTFClassMenu::CreateControlByName( const char *controlName )
{
	if ( !Q_stricmp( "CIconPanel", controlName ) )
	{
		return new CIconPanel( this, "icon_panel" );
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}

//-----------------------------------------------------------------------------
// Catch the mouseover event and set the active class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnShowPage( vgui::Panel *panel, const char *pagename )
{
	for ( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		if (m_pClassButtons[i] == panel )
		{
//			SelectClass( i );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Draw nothing
//-----------------------------------------------------------------------------
void CTFClassMenu::PaintBackground( void )
{
}

//-----------------------------------------------------------------------------
// Do things that should be done often, eg number of players in the 
// selected class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnTick( void )
{
	//When a player changes teams, their class and team values don't get here 
	//necessarily before the command to update the class menu. This leads to the cancel button 
	//being visible and people cancelling before they have a class. check for class == TF_CLASS_UNDEFINED and if so
	//hide the cancel button

	if ( !IsVisible() )
		return;

#ifndef _X360
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED )
	{
		SetCancelButtonVisible( false );
		SetVisibleButton( "ResetButton", false );
	}

	UpdateClassCounts();

#endif

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnClose()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		// Clear the HIDEHUD_HEALTH bit we hackily added. Turns out prediction
		//  was restoring these bits every frame. Unfortunately, prediction
		//  is off for karts which means the spell hud item would disappear if you
		//  brought up this menu and returned.
		pLocalPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_HEALTH;
	}

	ShowPanel( false );

	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	m_KeyRepeat.Reset();

	if ( state )
	{
		engine->ServerCmd( "menuopen" );			// to the server
		engine->ClientCmd( "_cl_classmenuopen 1" );	// for other panels
		CBroadcastRecipientFilter filter;

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CBaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, "music.mvm_class_menu" );
		}
		else
		{
			CBaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, "music.class_menu" );
		}

		CheckMvMUpgrades();
	}
	else
	{
		engine->ServerCmd( "menuclosed" );	
		engine->ClientCmd( "_cl_classmenuopen 0" );
		
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CBaseEntity::StopSound( SOUND_FROM_UI_PANEL, "music.mvm_class_menu" );
		}
		else
		{
			CBaseEntity::StopSound( SOUND_FROM_UI_PANEL, "music.class_menu" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::Go()
{
	const int iClass = m_iCurrentClassIndex;
	if ( iClass == TF_CLASS_UNDEFINED )
		return;

	// Check class limits
	if ( TFGameRules() && !TFGameRules()->CanPlayerChooseClass( C_TFPlayer::GetLocalTFPlayer(), iClass ) )
		return;

#if defined( REPLAY_ENABLED )
	// Display replay recording message if appropriate
	int &nDisplayedConnectedRecording = CPlayerSpawnCache::Instance().m_Data.m_nDisplayedConnectedRecording;
	if ( g_pReplay->IsReplayEnabled() &&
		 !g_pEngineClientReplay->IsPlayingReplayDemo() &&	// FIXME: We shouldn't need this here but for some reason the engine thinks a replay is recording during demo playback, even though replay_recording has a FCVAR_DONTRECORD flag
		 !nDisplayedConnectedRecording &&
		 replay_replaywelcomedlgcount.GetInt() <= MAX_TIMES_TO_SHOW_REPLAY_WELCOME_DLG )
	{
		wchar_t wText[256];
		wchar wKeyBind[80];
		char szText[256];

		const char *pSaveReplayKey = engine->Key_LookupBinding( "save_replay" );
		if ( !pSaveReplayKey )
		{
			pSaveReplayKey = "< not bound >";
		}
		g_pVGuiLocalize->ConvertANSIToUnicode( pSaveReplayKey, wKeyBind, sizeof( wKeyBind ) );
		g_pVGuiLocalize->ConstructString_safe( wText, g_pVGuiLocalize->Find( "#Replay_ConnectRecording" ), 1, wKeyBind );
		g_pVGuiLocalize->ConvertUnicodeToANSI( wText, szText, sizeof( szText ) );

		extern ConVar replay_msgduration_connectrecording;
		g_pClientMode->DisplayReplayMessage( szText, replay_msgduration_connectrecording.GetFloat(), false, NULL, true );

		// Don't execute this clause next time the player spawns, unless the cache has been cleared
		++nDisplayedConnectedRecording;

		// Increment (archives)
		replay_replaywelcomedlgcount.SetValue( replay_replaywelcomedlgcount.GetInt() + 1 );
	}
#endif

	// This will complete any pending replay, commit if necessary, and clear - this way when the player respawns
	// we will start with a fresh replay for the new life.
	g_pClientReplayContext->OnPlayerClassChanged();

	// Change class
	BaseClass::OnCommand( CFmtStr( "joinclass %s", g_aRawPlayerClassNames[ iClass ] ).Access() );

	CBroadcastRecipientFilter filter;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		CBaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, "music.mvm_class_select" );
	}
	else
	{
		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnCommand( const char *command )
{
	if ( !V_strnicmp( command, "select", 6 ) )
	{
		const char *pClass = command + 6;
		const int iClass = atoi( pClass );

		// Avoid restarting the animation if the user selected on the same class
		if ( iClass != m_iCurrentClassIndex )
		{
			SelectClass( iClass );
		}
		else
		{
			// Ensure selection states, in case the user clicks a button and drags away
			UpdateButtonSelectionStates( iClass );
		}

		Go();
	}
	else if ( !V_strnicmp( command, "resetclass", 10 ) )
	{
		if ( TFGameRules() && !TFGameRules()->IsInHighlanderMode() )
			return;

		engine->ClientCmd( const_cast<char *>( command ) );
	}
	else if ( !V_strnicmp( command, "openloadout", 11 ) )
	{
		// Let this panel know when you've closed, so we can reload items
		EconUI()->AddPanelCloseListener( this );

		// Make the back button close, rather than go back to the econ root panel
		EconUI()->SetClosePanel( -m_iCurrentClassIndex );

		// Set team number, so the model's color will match
		EconUI()->SetDefaultTeam( GetTeamNumber() );

		// Go directly to the loadout for the selected class
		EconUI()->OpenEconUI( -m_iCurrentClassIndex );	
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Console command to select a class
//-----------------------------------------------------------------------------
void CTFClassMenu::Join_Class( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		char cmd[256];
		Q_snprintf( cmd, sizeof( cmd ), "joinclass %s", args.Arg( 1 ) );
		OnCommand( cmd );
		ShowPanel( false );
	}
}

static const char *g_sDialogVariables[] = {
	"",
	"numScout",
	"numSoldier",
	"numPyro",

	"numDemoman",
	"numHeavy",
	"numEngineer",
	
	"numMedic",
	"numSniper",
	"numSpy",
};

// Original nine in menu order, then classes added later, then Civilian.
// tf_spectatorgui.cpp indexes this through TF_LAST_NORMAL_CLASS, so the last
// entry must stay at that index. Class-select pages do not read this.
int g_ClassDefinesRemap[] = {
	0,
	TF_CLASS_SCOUT,	
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,

	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,

	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY,

	TF_CLASS_MADCAP,
	TF_CLASS_CIVILIAN,
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_ClassDefinesRemap ) == TF_LAST_NORMAL_CLASS + 1 );

static void SetClassCountDialogVariable( vgui::EditablePanel *pPanel, const char *pszVar, int classCount, int iClassLimit )
{
	if ( !pszVar || !pszVar[0] )
		return;

	if ( iClassLimit != NO_CLASS_LIMIT )
	{
		if ( classCount >= iClassLimit )
		{
			if ( classCount > 0 )
			{
				wchar_t wTemp[32];
				wchar_t wzCount[10];
				_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", classCount );
				g_pVGuiLocalize->ConstructString_safe( wTemp, g_pVGuiLocalize->Find( "TF_ClassLimitHit" ), 1, wzCount );
				pPanel->SetDialogVariable( pszVar, wTemp );
			}
			else
			{
				pPanel->SetDialogVariable( pszVar, g_pVGuiLocalize->Find( "TF_ClassLimitHit_None" ) );
			}
		}
		else
		{
			wchar_t wTemp[32];
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", classCount );
			wchar_t wzMax[10];
			_snwprintf( wzMax, ARRAYSIZE( wzMax ), L"%d", iClassLimit );
			g_pVGuiLocalize->ConstructString_safe( wTemp, g_pVGuiLocalize->Find( "TF_ClassLimitUnder" ), 2, wzCount, wzMax );
			pPanel->SetDialogVariable( pszVar, wTemp );
		}
	}
	else if ( classCount > 0 )
	{
		pPanel->SetDialogVariable( pszVar, classCount );
	}
	else
	{
		pPanel->SetDialogVariable( pszVar, "" );
	}
}

void CTFClassMenu::UpdateNumClassLabels( int iTeam )
{
#ifndef _X360
	int nTotalCount = 0;

	// count how many of each class there are
	if ( !g_TF_PR )
		return;

	if ( iTeam < FIRST_GAME_TEAM || iTeam >= TF_TEAM_COUNT ) // invalid team number
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	bool bSpectator = pLocalPlayer && pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR;

	int iLocalPlayerClass = TF_CLASS_UNDEFINED;
	if ( pLocalPlayer )
	{
		iLocalPlayerClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	}

	if ( iLocalPlayerClass == TF_CLASS_UNDEFINED )
	{
		m_iLocalPlayerClass = iLocalPlayerClass;

		if ( m_pLocalPlayerImage && m_pLocalPlayerImage->IsVisible() )
		{
			m_pLocalPlayerImage->SetVisible( false );
		}

		if ( m_pLocalPlayerBG && m_pLocalPlayerBG->IsVisible() )
		{
			m_pLocalPlayerBG->SetVisible( false );
		}
	}

	if ( bSpectator )
	{
		for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
		{
			SetDialogVariable( g_sDialogVariables[ nSlot + 1 ], "" );
		}
	}
	else
	{
		// Counts drawn on the nine buttons follow whichever class that button is showing.
		for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
		{
			const int iClass = m_nSlotClass[ nSlot ];
			if ( !IsValidTFPlayerClass( iClass ) )
			{
				SetDialogVariable( g_sDialogVariables[ nSlot + 1 ], "" );
				continue;
			}

			const int classCount = g_TF_PR->GetCountForPlayerClass( iTeam, iClass, false );
			const int iClassLimit = TFGameRules()->GetClassLimit( iClass );
			SetClassCountDialogVariable( this, g_sDialogVariables[ nSlot + 1 ], classCount, iClassLimit );
		}

		// The teammate strip lists every class, not just the page on screen.
		for ( int i = TF_FIRST_NORMAL_CLASS; i < ARRAYSIZE( g_ClassDefinesRemap ); ++i )
		{
			const int iClass = g_ClassDefinesRemap[ i ];
			if ( !IsValidTFPlayerClass( iClass ) )
				continue;

			int classCount = g_TF_PR->GetCountForPlayerClass( iTeam, iClass, false );

			if ( iClass == iLocalPlayerClass )
			{
				// The local player has their own image, so don't also fill a strip slot with them.
				if ( classCount > 0 )
					classCount--;

				if ( m_pLocalPlayerImage )
				{
					if ( !m_pLocalPlayerImage->IsVisible() )
						m_pLocalPlayerImage->SetVisible( true );

					if ( m_iLocalPlayerClass != iLocalPlayerClass )
					{
						m_iLocalPlayerClass = iLocalPlayerClass;
						m_pLocalPlayerImage->SetImage( CFmtStr( "class_sel_sm_%s_%s", PortraitShortName( iClass ), iTeam == TF_TEAM_BLUE ? "blu" : "red" ) );
					}
				}

				if ( m_pLocalPlayerBG && !m_pLocalPlayerBG->IsVisible() )
					m_pLocalPlayerBG->SetVisible( true );
			}

			if ( nTotalCount >= CLASS_COUNT_IMAGES )
				continue;

			char szImage[64];
			V_snprintf( szImage, sizeof( szImage ), "class_sel_sm_%s_%s", PortraitShortName( iClass ), iTeam == TF_TEAM_BLUE ? "blu" : "red" );
			for ( int j = 0; j < classCount; ++j )
			{
				CTFImagePanel *pImage = m_ClassCountImages[ nTotalCount ];
				if ( pImage )
				{
					pImage->SetVisible( true );
					pImage->SetImage( szImage );
				}

				nTotalCount++;
				if ( nTotalCount >= CLASS_COUNT_IMAGES )
					break;
			}
		}
	}
	
	if ( nTotalCount == 0 )
	{
		// no classes for our team yet
		if ( m_pCountLabel && m_pCountLabel->IsVisible() )
		{
			m_pCountLabel->SetVisible( false );
		}
	}
	else
	{
		if ( m_pCountLabel && !m_pCountLabel->IsVisible() )
		{
			m_pCountLabel->SetVisible( true );
		}
	}

	// turn off any unused images
	while ( nTotalCount < CLASS_COUNT_IMAGES )
	{
		CTFImagePanel *pImage = m_ClassCountImages[nTotalCount];
		if ( pImage )
		{
			pImage->SetVisible( false );
		}

		nTotalCount++;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFClassMenu::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();

	// when we are changing levels
	if ( FStrEq( pszEventName, "localplayer_changeteam" ) )
	{
		if ( IsVisible() )
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pLocalPlayer )
			{
				int iTeam = pLocalPlayer->GetTeamNumber();
				if ( iTeam != GetTeamNumber() )
				{
					ShowPanel( false );

					if ( iTeam == TF_TEAM_BLUE )
					{
						gViewPortInterface->ShowPanel( PANEL_CLASS_BLUE, true );
					}
					else
					{
						gViewPortInterface->ShowPanel( PANEL_CLASS_RED, true );
					}
				}
			}
		}
	}
	else if ( FStrEq( pszEventName, "show_match_summary" ) )
	{
		if ( IsVisible() )
		{
			ShowPanel( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFClassMenu::OnEconUIClosed()
{
	// Reload items on model panel, in case anything's changed
	LoadItems();
}

int g_nNumUpgradeIconsForLastHint = 0;

//-----------------------------------------------------------------------------
void CTFClassMenu::CheckMvMUpgrades()
{
	// Set MvM Icons invisible
	for ( int icons = 0; icons < ARRAYSIZE( m_pMvmUpgradeImages ); ++icons )
	{
		if ( m_pMvmUpgradeImages[icons] == NULL )
			continue;
		m_pMvmUpgradeImages[icons]->SetVisible( false );
	}

	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
	if ( !pStats )
		return;

	CUtlVector< CUpgradeInfo > *upgrades = pStats->GetLocalPlayerUpgrades();

	int nShowUpgradingHint = -1;
	int nNumUpgradeIconsForHint = 0;

	for ( int i = 0; i < upgrades->Count(); ++i )
	{
		vgui::Panel *pUpgradeImage = m_pMvmUpgradeImages[upgrades->Element(i).m_iPlayerClass];

		if ( !pUpgradeImage )
			continue;

		if ( !pUpgradeImage->IsVisible() )
		{
			pUpgradeImage->SetVisible( true );
			nNumUpgradeIconsForHint++;

			// Only show the hint if we've shown it 3 or less times ever
			if ( nShowUpgradingHint == -1 && tf_mvm_classupgradehelpcount.GetInt() < 3 )
			{
				int nY;
				pUpgradeImage->GetPos( nShowUpgradingHint, nY );
				nShowUpgradingHint += pUpgradeImage->GetWide() / 2;
			}
		}
	}

	// Only show the hint if there are more upgrade icon than the last time we openned the menu
	if ( nShowUpgradingHint != -1 && g_nNumUpgradeIconsForLastHint < nNumUpgradeIconsForHint )
	{
		CExplanationPopup *pPopup = dynamic_cast< CExplanationPopup* >( FindChildByName("StartExplanation") );
		if ( pPopup )
		{
			pPopup->SetCalloutInParentsX( nShowUpgradingHint );
			pPopup->Popup();

			g_nNumUpgradeIconsForLastHint = nNumUpgradeIconsForHint;
			tf_mvm_classupgradehelpcount.SetValue( tf_mvm_classupgradehelpcount.GetInt() + 1 );
		}
	}

	// Upgrade icons are pinned to the stock portraits. Hide one when that button
	// is showing a different class.
	for ( int nSlot = 0; nSlot < CLASS_MENU_SLOT_COUNT; ++nSlot )
	{
		const int iAnchor = iRemapIndexToClass[ nSlot + 1 ];
		if ( m_nSlotClass[ nSlot ] == iAnchor )
			continue;

		if ( iAnchor > 0 && iAnchor < ARRAYSIZE( m_pMvmUpgradeImages ) && m_pMvmUpgradeImages[ iAnchor ] )
			m_pMvmUpgradeImages[ iAnchor ]->SetVisible( false );
	}
}


