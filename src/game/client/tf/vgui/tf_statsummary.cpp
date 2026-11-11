//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/ScrollableEditablePanel.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"
#include "tf_mapinfo.h"
#include "vgui_avatarimage.h"
#include "VGuiMatSurface/IMatSystemSurface.h"

#include "tf_statsummary.h"
#include <convar.h>
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "tf_gc_client.h"

using namespace vgui;

#if defined( REPLAY_ENABLED )
extern bool g_bIsReplayRewinding;
#else
bool g_bIsReplayRewinding = false;
#endif

const char *g_pszTipsClassImages[] =
{
	"",						// TF_CLASS_UNDEFINED = 0,
	"class_portraits/scout",	// TF_CLASS_SCOUT,			
	"class_portraits/sniper",// TF_CLASS_SNIPER,
	"class_portraits/soldier",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic",		// TF_CLASS_MEDIC,
	"class_portraits/heavy",	// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro",	// TF_CLASS_PYRO,
	"class_portraits/spy",		// TF_CLASS_SPY,
	"class_portraits/engineer",		// TF_CLASS_ENGINEER,		
	"class_portraits/scout",		// TF_CLASS_MADCAP,
	"class_portraits/pyro",		// TF_CLASS_MOBSTER,
	"class_portraits/sniper",	// TF_CLASS_RESUPPLY,
	"class_portraits/brick",	// TF_CLASS_BRICK,
	"class_portraits/witch",	// TF_CLASS_WITCH,
	"class_portraits/dog",		// TF_CLASS_GUARDDOG,
	"",							// TF_CLASS_CIVILIAN,
};

ClassDetails_t g_PerClassStatDetails[15] =
{
	{ TFSTAT_POINTSSCORED,			ALL_CLASSES,					"#TF_ClassRecord_MostPoints", "#TF_ClassRecord_Alt_MostPoints" },
	{ TFSTAT_KILLS,					ALL_CLASSES,					"#TF_ClassRecord_MostKills", "#TF_ClassRecord_Alt_MostKills" },
	{ TFSTAT_KILLASSISTS,			ALL_CLASSES,					"#TF_ClassRecord_MostAssists", "#TF_ClassRecord_Alt_MostAssists" },
	{ TFSTAT_CAPTURES,				ALL_CLASSES,					"#TF_ClassRecord_MostCaptures", "#TF_ClassRecord_Alt_MostCaptures" },
	{ TFSTAT_DEFENSES,				ALL_CLASSES,					"#TF_ClassRecord_MostDefenses", "#TF_ClassRecord_Alt_MostDefenses" },
	{ TFSTAT_DAMAGE,				ALL_CLASSES,					"#TF_ClassRecord_MostDamage", "#TF_ClassRecord_Alt_MostDamage" },
	{ TFSTAT_BUILDINGSDESTROYED,	ALL_CLASSES,					"#TF_ClassRecord_MostDestruction", "#TF_ClassRecord_Alt_MostDestruction" },
	{ TFSTAT_DOMINATIONS,			ALL_CLASSES,					"#TF_ClassRecord_MostDominations", "#TF_ClassRecord_Alt_MostDominations" },
	{ TFSTAT_PLAYTIME,				ALL_CLASSES,					"#TF_ClassRecord_LongestLife", "#TF_ClassRecord_Alt_LongestLife" },
	{ TFSTAT_HEALING,				MAKESTATFLAG(TF_CLASS_MEDIC) | MAKESTATFLAG(TF_CLASS_ENGINEER) | MAKESTATFLAG(TF_CLASS_HEAVYWEAPONS),	"#TF_ClassRecord_MostHealing", "#TF_ClassRecord_Alt_MostHealing" },
	{ TFSTAT_INVULNS,				MAKESTATFLAG(TF_CLASS_MEDIC),		"#TF_ClassRecord_MostInvulns", "#TF_ClassRecord_Alt_MostInvulns" },
	{ TFSTAT_MAXSENTRYKILLS,		MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostSentryKills", "#TF_ClassRecord_Alt_MostSentryKills" },
	{ TFSTAT_TELEPORTS,				MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostTeleports", "#TF_ClassRecord_Alt_MostTeleports" },
	{ TFSTAT_HEADSHOTS,				MAKESTATFLAG(TF_CLASS_SNIPER) | MAKESTATFLAG(TF_CLASS_SPY),		"#TF_ClassRecord_MostHeadshots", "#TF_ClassRecord_Alt_MostHeadshots" },
	{ TFSTAT_BACKSTABS,				MAKESTATFLAG(TF_CLASS_SPY),			"#TF_ClassRecord_MostBackstabs", "#TF_ClassRecord_Alt_MostBackstabs" },
};

ClassDetails_t g_PerClassMVMStatDetails[12] =
{
	{ TFSTAT_POINTSSCORED,			ALL_CLASSES,					"#TF_ClassRecord_MostPoints", "#TF_ClassRecord_Alt_MostPoints" },
	{ TFSTAT_KILLS,					ALL_CLASSES,					"#TF_ClassRecord_MostKills", "#TF_ClassRecord_Alt_MostKills" },
	{ TFSTAT_KILLASSISTS,			ALL_CLASSES,					"#TF_ClassRecord_MostAssists", "#TF_ClassRecord_Alt_MostAssists" },
	{ TFSTAT_DEFENSES,				ALL_CLASSES,					"#TF_ClassRecord_MostDefenses", "#TF_ClassRecord_Alt_MostDefenses" },
	{ TFSTAT_DAMAGE,				ALL_CLASSES,					"#TF_ClassRecord_MostDamage", "#TF_ClassRecord_Alt_MostDamage" },
	{ TFSTAT_PLAYTIME,				ALL_CLASSES,					"#TF_ClassRecord_LongestLife", "#TF_ClassRecord_Alt_LongestLife" },
	{ TFSTAT_HEALING,				MAKESTATFLAG(TF_CLASS_MEDIC) | MAKESTATFLAG(TF_CLASS_ENGINEER) | MAKESTATFLAG(TF_CLASS_HEAVYWEAPONS),	"#TF_ClassRecord_MostHealing", "#TF_ClassRecord_Alt_MostHealing" },
	{ TFSTAT_INVULNS,				MAKESTATFLAG(TF_CLASS_MEDIC),		"#TF_ClassRecord_MostInvulns", "#TF_ClassRecord_Alt_MostInvulns" },
	{ TFSTAT_MAXSENTRYKILLS,		MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostSentryKills", "#TF_ClassRecord_Alt_MostSentryKills" },
	{ TFSTAT_TELEPORTS,				MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostTeleports", "#TF_ClassRecord_Alt_MostTeleports" },
	{ TFSTAT_HEADSHOTS,				MAKESTATFLAG(TF_CLASS_SNIPER) | MAKESTATFLAG(TF_CLASS_SPY),		"#TF_ClassRecord_MostHeadshots", "#TF_ClassRecord_Alt_MostHeadshots" },
	{ TFSTAT_BACKSTABS,				MAKESTATFLAG(TF_CLASS_SPY),			"#TF_ClassRecord_MostBackstabs", "#TF_ClassRecord_Alt_MostBackstabs" },
};

CTFStatsSummaryPanel *g_pTFStatsSummaryPanel = NULL;

CUtlVector<CTFStatsSummaryPanel *> g_vecStatPanels;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UpdateStatSummaryPanels( CUtlVector<ClassStats_t> &vecClassStats )
{
	for ( int i = 0; i < g_vecStatPanels.Count(); i++ )
	{
		g_vecStatPanels[i]->SetStats( vecClassStats );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the global stats summary panel
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel *GStatsSummaryPanel()
{
	if ( NULL == g_pTFStatsSummaryPanel )
	{
		g_pTFStatsSummaryPanel = new CTFStatsSummaryPanel();
	}
	return g_pTFStatsSummaryPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Destroys the global stats summary panel
//-----------------------------------------------------------------------------
void DestroyStatsSummaryPanel()
{
	if ( NULL != g_pTFStatsSummaryPanel )
	{
		g_pTFStatsSummaryPanel->MarkForDeletion();
		g_pTFStatsSummaryPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel::CTFStatsSummaryPanel() 
  : BaseClass( NULL, "TFStatsSummary", vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
  ,	m_bShowingLeaderboard( false )
  , m_bLoadingCommunityMap( false )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::Init( void )
{
	m_bControlsLoaded = false;
	m_bClassScrollBuilt = false;
	m_bStatFrameLifted = false;
	m_bClassChartsSynced = false;
	m_bBarAnchorsReady = false;
	m_bInteractive = false;
	m_bEmbedded = false;
	m_xStartLHBar = 0;
	m_xStartRHBar = 0;
	m_iBarHeight = 1;
	m_iBarMaxWidth = 1;

	m_pPlayerData = new vgui::EditablePanel( this, "statdata" );
	m_pInteractiveHeaders = new vgui::EditablePanel( m_pPlayerData, "InteractiveHeaders" );
	m_pNonInteractiveHeaders = new vgui::EditablePanel( m_pPlayerData, "NonInteractiveHeaders" );
	m_pBarChartComboBoxA = new vgui::ComboBox( m_pInteractiveHeaders, "BarChartComboA", 10, false );
	m_pBarChartComboBoxB = new vgui::ComboBox( m_pInteractiveHeaders, "BarChartComboB", 10, false );
	m_pClassComboBox = new vgui::ComboBox( m_pInteractiveHeaders, "ClassCombo", 10, false );	
	m_pTipImage = new CTFImagePanel( this, "TipImage" );
	m_pTipText = new vgui::Label( this, "TipText", "" );
	m_pMapInfoPanel = NULL;
	m_pMainBackground = NULL;
	m_pLeaderboardTitle = NULL;
	m_pContributedPanel = NULL;

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
	m_bShowBackButton = false;
#else
	m_pNextTipButton = new vgui::Button( this, "NextTipButton", "" );	
	m_pResetStatsButton = new vgui::Button( this, "ResetStatsButton", "" );
	m_pCloseButton = new vgui::Button( this, "CloseButton", "" );	
#endif

	m_pBarChartComboBoxA->AddActionSignalTarget( this );
	m_pBarChartComboBoxB->AddActionSignalTarget( this );
	m_pClassComboBox->AddActionSignalTarget( this );

	ListenForGameEvent( "server_spawn" );

	Reset();

	g_vecStatPanels.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel::CTFStatsSummaryPanel( vgui::Panel *parent ) : BaseClass( parent, "TFStatsSummary", 
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel::~CTFStatsSummaryPanel()
{
	g_vecStatPanels.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ShowModal()
{
#ifdef _X360
	m_bInteractive = false;
	m_bShowBackButton = true;
#else
	// we are in interactive mode, enable controls
	m_bInteractive = true;
#endif

	SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	UpdateDialog();
	SetVisible( true );
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetupForEmbedded( void )
{
	m_bInteractive = true;
	m_bEmbedded = true;

	UpdateDialog();

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::PerformLayout()
{
	BaseClass::PerformLayout();

#ifndef _X360
	if ( m_pTipImage && m_pTipText )
	{
		int iX,iY;
		m_pTipImage->GetPos(iX,iY);
		int iTX, iTY;
		m_pTipText->GetPos(iTX, iTY);
		m_pTipText->SetPos( iX + m_pTipImage->GetWide() + XRES(8), iTY );
	}

	if ( m_pNextTipButton )
	{
		m_pNextTipButton->SizeToContents();
	}

	if ( m_pResetStatsButton )
	{
		m_pResetStatsButton->SizeToContents();
	}
#endif

	// LoadControlSettings applies "c+" positions, but they are not real until
	// this layout pass. The loading screen has room for every class, so those
	// rows are spaced through the report. The shorter stats page still scrolls.
	if ( m_bControlsLoaded && m_pPlayerData && !m_bClassScrollBuilt )
	{
		Panel *pLeft = m_pPlayerData->FindChildByName( "ClassBarBG1A", false );
		if ( pLeft && pLeft->GetXPos() > 0 && m_pPlayerData->GetWide() > 100 )
		{
			bool bBuilt = m_bEmbedded ? SetupClassStatScroll() : SetupClassStatRows();
			if ( bBuilt )
				m_bClassScrollBuilt = true;
		}
	}

	// The loading screen heading moves over the records column, and the
	// points / playtime frame grows up into the space that frees. The
	// embedded stats page keeps the res-file layout.
	if ( m_bControlsLoaded && !m_bEmbedded )
	{
		if ( m_bClassScrollBuilt && !m_bStatFrameLifted )
			LiftPerformanceReport();
		if ( m_bClassScrollBuilt && m_bStatFrameLifted && SpreadClassStatRows() && !m_bClassChartsSynced )
		{
			// Spread owns Y. Draw the bars once those positions are real so a
			// chart pass from before this layout cannot leave them stacked.
			m_bClassChartsSynced = true;
			UpdateBarCharts();
		}
		AlignMapHeadingToRecords();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnThink()
{
	BaseClass::OnThink();

	if ( m_bShowingLeaderboard )
	{
		UpdateLeaderboard();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnCommand( const char *command )
{
	if ( 0 == Q_stricmp( command, "vguicancel" ) )
	{
		m_bInteractive = false;
		UpdateDialog();
		SetVisible( false );
		SetParent( (VPANEL) NULL );

#ifdef _X360
		SetDefaultSelections();
		m_bShowBackButton = true;
#endif
	}
#ifndef _X360
	else if ( 0 == Q_stricmp( command, "resetstatsbutton" ) )
	{
		QueryBox *qb = new QueryBox( "#GameUI_Confirm", "#TF_ConfirmResetStats" );
		if (qb != NULL)
		{
			qb->SetOKCommand(new KeyValues("DoResetStats") );
			qb->AddActionSignalTarget(this);
			qb->MoveToFront();
			qb->DoModal();
		}
	}
#endif
	else if ( 0 == Q_stricmp( command, "nexttip" ) )
	{
		UpdateTip();
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::Reset()
{
	m_aClassStats.RemoveAll();

	SetDefaultSelections();
}

//-----------------------------------------------------------------------------
// Purpose: Sets all user-controllable dialog settings to default values
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetDefaultSelections()
{
	m_iSelectedClass = TF_CLASS_UNDEFINED;
	m_statBarGraph[0] = TFSTAT_POINTSSCORED;
	m_displayBarGraph[0]= SHOW_MAX;
	m_statBarGraph[1] = TFSTAT_PLAYTIME;
	m_displayBarGraph[1] = SHOW_TOTAL;

	m_pBarChartComboBoxA->ActivateItemByRow( 0 );
	m_pBarChartComboBoxB->ActivateItemByRow( 10 );
}


//-----------------------------------------------------------------------------
// Purpose: Set the background image based on the current mode
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateMainBackground( void )
{
	if ( IsPC() )
	{
		m_pMainBackground = dynamic_cast<ImagePanel *>( FindChildByName( "MainBackground" ) );
		if ( m_pMainBackground )
		{
			const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GTFGCClientSystem()->GetLiveMatchGroup() );

			// determine if we're in widescreen or not and select the appropriate image
			int screenWide, screenTall;
			surface()->GetScreenSize( screenWide, screenTall );
			float aspectRatio = (float)screenWide/(float)screenTall;
			bool bIsWidescreen = aspectRatio >= 1.5999f;

			if ( g_bIsReplayRewinding )
			{
				m_pMainBackground->SetImage( bIsWidescreen ? "../console/rewind_background_widescreen" : "../console/rewind_background" );
			}
			else if ( engine->IsLoadingDemo() || engine->IsPlayingDemo() || engine->IsSkippingPlayback() )
			{
				m_pMainBackground->SetImage( bIsWidescreen ? "../console/replay_loading_widescreen" : "../console/replay_loading" );
			}
			else if ( pMatchDesc && pMatchDesc->GetMapLoadBackgroundOverride( bIsWidescreen ) ) // Use match override if we have one
			{
				m_pMainBackground->SetImage( pMatchDesc->GetMapLoadBackgroundOverride( bIsWidescreen ) );
			}
			else
			{
				m_pMainBackground->SetImage( bIsWidescreen ? "../console/background01_widescreen" : "../console/background01" );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );

	// A previous pass moved the class rows under a scroller. Delete that
	// before the res file runs, so those rows are created again as children
	// of StatData. MarkForDeletion is too late: the build group would still
	// find the doomed panels and then not replace them.
	ReleaseClassStatScroll();
	m_bClassScrollBuilt = false;
	m_bStatFrameLifted = false;
	m_bClassChartsSynced = false;
	m_bBarAnchorsReady = false;

	if ( m_bEmbedded )
	{
		LoadControlSettings( "Resource/UI/StatSummary_Embedded.res" );
	}
	else
	{
		LoadControlSettings( "Resource/UI/StatSummary.res" );
	}
	m_bControlsLoaded = true;

	// set the background image
	UpdateMainBackground();

	m_pMapInfoPanel = dynamic_cast< EditablePanel *>( FindChildByName( "MapInfo" ) );
	m_vecLeaderboardEntries.RemoveAll();
	if ( m_pMapInfoPanel )
	{
		for ( int i = 0; i < 10; ++ i )
		{
			vgui::EditablePanel *pEntryUI = new vgui::EditablePanel( m_pMapInfoPanel, "LeaderboardEntry" );
			pEntryUI->ApplySchemeSettings( pScheme );
			pEntryUI->LoadControlSettings( "Resource/UI/LeaderboardEntry.res" );
			m_vecLeaderboardEntries.AddToTail( pEntryUI );
		}
	}

	// get the dimensions and position of a left-hand bar and a right-hand bar so we can do bar sizing later
	Panel *pLHBar = m_pPlayerData->FindChildByName( "ClassBar1A" );
	Panel *pRHBar = m_pPlayerData->FindChildByName( "ClassBar1B" );
	if ( pLHBar && pRHBar )
	{
		int y;
		pLHBar->GetBounds( m_xStartLHBar, y, m_iBarMaxWidth, m_iBarHeight );
		pRHBar->GetBounds( m_xStartRHBar, y, m_iBarMaxWidth, m_iBarHeight );
	}

	// The stock list has one row per original class. The loading screen spaces
	// the extra classes into the taller report. The embedded stats page still
	// scrolls, because that panel was not made taller.
	// Built from PerformLayout, once "c+" positions are real.

	// fill the combo box selections appropriately
	InitBarChartComboBox( m_pBarChartComboBoxA );
	InitBarChartComboBox( m_pBarChartComboBoxB );

	// fill the class names in the class combo box
	HFont hFont = scheme()->GetIScheme( GetScheme() )->GetFont( "ScoreboardSmall", true );
	m_pClassComboBox->SetFont( hFont );
	m_pClassComboBox->RemoveAll();
	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "class", TF_CLASS_UNDEFINED );
	m_pClassComboBox->AddItem( "#StatSummary_Label_AsAnyClass", pKeyValues );
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue;
		pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "class", iClass );
		m_pClassComboBox->AddItem( g_aPlayerClassNames[iClass], pKeyValues );
	}
	m_pClassComboBox->ActivateItemByRow( 0 );

	if ( m_pMapInfoPanel )
	{
		m_pContributedPanel = dynamic_cast< vgui::EditablePanel* >( m_pMapInfoPanel->FindChildByName( "ContributedLabel" ) );
	}

	SetDefaultSelections();
	UpdateDialog();

	if ( !m_bEmbedded )
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnKeyCodePressed( KeyCode code )
{
	if ( IsX360() )
	{
		if ( code == KEY_XBUTTON_A || code == STEAMCONTROLLER_A )
		{
			OnCommand(  "nexttip" )	;
		}
		else if ( code == KEY_XBUTTON_B || code == STEAMCONTROLLER_B )
		{
			OnCommand( "vguicancel" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets stats to use
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetStats( CUtlVector<ClassStats_t> &vecClassStats ) 
{
	m_aClassStats = vecClassStats; 
	if ( m_bControlsLoaded )
	{
		UpdateDialog();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ClearMapLabel()
{
	SetDialogVariable( "maplabel", "" );
	SetDialogVariable( "maptype", "" );

	vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "OnYourWayLabel" ) );
	if ( pLabel && pLabel->IsVisible() )
	{
		pLabel->SetVisible( false );
	}

	pLabel = dynamic_cast<Label *>( FindChildByName( "MapType" ) );
	if ( pLabel && pLabel->IsVisible() )
	{
		pLabel->SetVisible( false );
	}

	if ( m_pContributedPanel )
	{
		m_pContributedPanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ShowMapInfo( bool bShowMapInfo, bool bIsMVM /*= false*/, bool bBackgroundOverride /*= false*/ )
{
	if ( m_pMainBackground )
	{
		m_pMainBackground->SetVisible( !bShowMapInfo );
	}
	m_pPlayerData->SetVisible( bIsMVM || !bShowMapInfo );
	m_pNextTipButton->SetVisible( m_bInteractive && !bShowMapInfo );
	m_pResetStatsButton->SetVisible( m_bInteractive && !bShowMapInfo );

	if ( m_pMapInfoPanel )
	{
		m_pMapInfoPanel->SetVisible( bShowMapInfo );
		vgui::Panel* pInfoBG = m_pMapInfoPanel->FindChildByName( "InfoBG" );
		if ( pInfoBG )
		{
			pInfoBG->SetVisible( bShowMapInfo && !bIsMVM && !bBackgroundOverride );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnMapLoad( const char *pMapName )
{
	if ( g_bIsReplayRewinding || engine->IsLoadingDemo() || engine->IsPlayingDemo() || engine->IsSkippingPlayback() )
		return;

	bool bWidescreenBackground = false;

	bool bIsMVM = ( pMapName && !Q_strncmp( pMapName, "mvm_", 4 ) );
	bool bIsMVMBackground = false;
	const char *pszBackgroundOverride = NULL;
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GTFGCClientSystem()->GetLiveMatchGroup() );
	if ( pMatchDesc )
	{
		int screenWide, screenTall;
		surface()->GetScreenSize( screenWide, screenTall );
		float aspectRatio = (float)screenWide/(float)screenTall;
		bool bWideScreen = aspectRatio >= 1.5999f;

		// Check if there's a widescreen override
		if( bWideScreen )
		{
			pszBackgroundOverride = pMatchDesc->GetMapLoadBackgroundOverride( true );
			if ( pszBackgroundOverride )
			{
				// Success!  We're done
				bWidescreenBackground = true;
			}
		}
		
		if ( !bWideScreen && !pszBackgroundOverride )
		{
			pszBackgroundOverride = pMatchDesc->GetMapLoadBackgroundOverride( false );
		}
		
	}
	
	if ( bIsMVM && !pszBackgroundOverride )
	{
		// this will preserve the current behavior for non-matchmaking servers
		pszBackgroundOverride = "mvm_background_map";
		bIsMVMBackground = true;
	}

	bool bIsCommunityMap = false;
	const char *pAuthors = NULL;
	
	const MapDef_t *pMapInfo = GetItemSchema()->GetMasterMapDefByName( pMapName );
	if ( pMapInfo )
	{
		bIsCommunityMap = pMapInfo->IsCommunityMap();
		pAuthors = pMapInfo->pszAuthorsLocKey;
	}
	
	ShowMapInfo( true, bIsMVM, ( pszBackgroundOverride != NULL ) );

	m_xStartLeaderboard = 0;
	m_yStartLeaderboard = 0;

	// If we're loading a background map, don't display anything
	// HACK: Client doesn't get gpGlobals->eLoadType, so just do string compare for now.
	if ( Q_stristr( pMapName, "background") )
	{
		ClearMapLabel();
	}
	else
	{
		// set the map name in the UI
		wchar_t wzMapName[255]=L"";
		g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( pMapName ), wzMapName, sizeof( wzMapName ) );

		SetDialogVariable( "maplabel", wzMapName );
		SetDialogVariable( "maptype", g_pVGuiLocalize->Find( GetMapType( pMapName ) ) );

		vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "OnYourWayLabel" ) );
		if ( pLabel && !pLabel->IsVisible() )
		{
			pLabel->SetVisible( true );
		}

		pLabel = dynamic_cast<Label *>( FindChildByName( "MapType" ) );
		if ( pLabel && !pLabel->IsVisible() )
		{
			pLabel->SetVisible( true );
		}

		ImagePanel *pMapImage = m_pMapInfoPanel ? dynamic_cast< ImagePanel *>( m_pMapInfoPanel->FindChildByName( "MapImage" ) ) : NULL;
		if ( pMapImage )
		{
			// load the map image (if it exists for the current map)
			char szMapImage[ MAX_PATH ];
			Q_snprintf( szMapImage, sizeof( szMapImage ), "VGUI/maps/menu_photos_%s", pMapName );
			Q_strlower( szMapImage );

			IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
			if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) && ( !pszBackgroundOverride || bIsMVMBackground ) )
			{
				// take off the vgui/ at the beginning when we set the image
				Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_photos_%s", pMapName );
				Q_strlower( szMapImage );
				pMapImage->SetImage( szMapImage );
				pMapImage->SetVisible( true );
			}
			else
			{
				pMapImage->SetVisible( false );
			}
		}

		ImagePanel *pBackgroundImage = m_pMapInfoPanel ? dynamic_cast< ImagePanel *>( m_pMapInfoPanel->FindChildByName( "Background" ) ) : NULL;
		if ( pBackgroundImage )
		{
			const char* pszBackgroundImage = pszBackgroundOverride ? pszBackgroundOverride : "stamp_background_map";

			pBackgroundImage->SetImage( pszBackgroundImage );

			// Resize to accomodate the background image coming in
			if ( bWidescreenBackground )
			{
				pBackgroundImage->SetWide( GetWide() );
			}
			else 
			{
				pBackgroundImage->SetWide( GetTall() * ( 4.f / 3.f ) );
			}

		}

		if ( bIsMVM )
		{
			UpdateClassDetails( true );
			m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", "" );
			m_pMapInfoPanel->SetDialogVariable( "title", "" );
			m_pMapInfoPanel->SetDialogVariable( "authors", "" );

			FOR_EACH_VEC( m_vecLeaderboardEntries, i )
			{
				EditablePanel *pContainer = dynamic_cast< EditablePanel* >( m_vecLeaderboardEntries[i] );
				if ( pContainer )
				{
					pContainer->SetVisible( false );
				}
			}
		}
		else
		{
			m_pLeaderboardTitle = NULL;
			// add authors
			if ( m_pMapInfoPanel )
			{
				if ( bIsCommunityMap )
				{
					m_pMapInfoPanel->SetDialogVariable( "title", g_pVGuiLocalize->Find( "#TF_MapAuthors_Community_Title" ) );
					m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", "" );
					m_pMapInfoPanel->SetDialogVariable( "authors", g_pVGuiLocalize->Find( pAuthors ) ); 
					m_pLeaderboardTitle = m_pMapInfoPanel->FindChildByName( "MapLeaderboardTitle" );
				}
				else
				{
					m_pMapInfoPanel->SetDialogVariable( "title", g_pVGuiLocalize->Find( "#TF_DuelLeaderboard_Title" ) );
					m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", "" );
					m_pMapInfoPanel->SetDialogVariable( "authors", "" );
					m_pLeaderboardTitle = m_pMapInfoPanel->FindChildByName( "Title" );
				}
			}
			if ( m_pLeaderboardTitle )
			{
				m_pLeaderboardTitle->GetPos( m_xStartLeaderboard, m_yStartLeaderboard );
				m_yStartLeaderboard += m_pLeaderboardTitle->GetTall();
			}

			//  request leaderboard data
			m_bShowingLeaderboard = true;
			if ( bIsCommunityMap )
			{
				MapInfo_RefreshLeaderboard( pMapName );
			}
			else
			{
				Leaderboards_Refresh();
			}
			m_bLoadingCommunityMap = bIsCommunityMap;

			if ( m_pContributedPanel && steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamFriends() )
			{
				int iDonationAmount = MapInfo_GetDonationAmount( steamapicontext->SteamUser()->GetSteamID().GetAccountID(), pMapName );
				m_pContributedPanel->SetVisible( iDonationAmount != 0 );
				if ( iDonationAmount != 0 )
				{
					m_pContributedPanel->SetDialogVariable( "playername", steamapicontext->SteamFriends()->GetPersonaName() );
				}
			}

			UpdateLeaderboard();
		}
	}
}

void CTFStatsSummaryPanel::UpdateLeaderboard()
{
	if ( m_pMapInfoPanel == NULL || steamapicontext == NULL || steamapicontext->SteamUserStats() == NULL || steamapicontext->SteamUser() == NULL )
		return;

	const int kMaxVisible_Supporters = 5;
	const int kIdeallyNumVisible_Supporters = 3;
	const int kMaxVisible_DuelWins = 10;
	const int kIdeallyNumVisible_DuelWins = 5;

	//  retrieve scores
	CUtlVector< LeaderboardEntry_t* > scores;
	bool bVisible = true;
	int iNumLeaderboardEntries = 0;
	if ( m_bLoadingCommunityMap )
	{
		bVisible = MapInfo_GetLeaderboardInfo( engine->GetLevelName(), scores, iNumLeaderboardEntries, kIdeallyNumVisible_Supporters );
		wchar_t wzNumEntriesString[256];
		_snwprintf( wzNumEntriesString, ARRAYSIZE( wzNumEntriesString ), L"%i", iNumLeaderboardEntries );
		wchar_t wzTitle[256];
		g_pVGuiLocalize->ConstructString_safe( wzTitle, g_pVGuiLocalize->Find( "#TF_MapDonators_Title" ), 1, wzNumEntriesString );
		m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", wzTitle );
	}
	else
	{
		bVisible = Leaderboards_GetDuelWins( scores, false );
		if ( bVisible && scores.Count() < kIdeallyNumVisible_DuelWins )
		{
			bVisible = Leaderboards_GetDuelWins( scores, true ) && scores.Count() > 0;
		}
		// show old stats
		m_pPlayerData->SetVisible( bVisible == false );
		// The report and the class tip travel together. Hiding the tip here
		// is what left the bottom of the loading screen empty.
		if ( !m_bEmbedded )
		{
			const bool bShowReport = ( bVisible == false );
			if ( m_pTipText )
				m_pTipText->SetVisible( bShowReport );
			if ( m_pTipImage && !bShowReport )
				m_pTipImage->SetVisible( false );
		}
		if ( m_pMapInfoPanel )
		{
			vgui::Panel* pInfoBG = m_pMapInfoPanel->FindChildByName( "InfoBG" );
			if ( pInfoBG )
			{
				pInfoBG->SetVisible( bVisible );
			}
		}
	}

	const int kMaxVisible = m_bLoadingCommunityMap ? kMaxVisible_Supporters : kMaxVisible_DuelWins;

	// try to show local player in relation to the people in the list
	if ( bVisible && scores.Count() > 0 && steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUserStats() )
	{
		int iLocalPlayerIdx = -1;
		CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
		FOR_EACH_VEC( scores, i )
		{
			const LeaderboardEntry_t *leaderboardEntry = scores[i];
			if ( leaderboardEntry->m_steamIDUser == localSteamID )
			{
				iLocalPlayerIdx = i;
				break;
			}
		}
		// local player is in the list, but is outside the visible range
		// so we want to move them to the last spot
		// and move the closest person above them as well
		if ( iLocalPlayerIdx >= kMaxVisible )
		{
			LeaderboardEntry_t *entryLocalPlayer = scores[iLocalPlayerIdx];
			LeaderboardEntry_t *closestPlayer = scores[iLocalPlayerIdx - 1];
			scores[kMaxVisible - 1] = entryLocalPlayer;
			scores[kMaxVisible - 2] = closestPlayer;
		}
	}

	// set avatars and names
	int x = m_xStartLeaderboard;
	int y = m_yStartLeaderboard;
	FOR_EACH_VEC( m_vecLeaderboardEntries, i )
	{
		EditablePanel *pContainer = dynamic_cast< EditablePanel* >( m_vecLeaderboardEntries[i] );
		if ( pContainer )
		{
			bool bIsEntryVisible = bVisible && i < scores.Count() && i < kMaxVisible;
			pContainer->SetVisible( bIsEntryVisible );
			pContainer->SetPos( x, y );
			y += pContainer->GetTall();
			if ( bIsEntryVisible )
			{
				const LeaderboardEntry_t *leaderboardEntry = scores[i];
				const CSteamID &steamID = leaderboardEntry->m_steamIDUser;
				pContainer->SetDialogVariable( "username", CFmtStr( "%d. %s - %d", leaderboardEntry->m_nGlobalRank, InventoryManager()->PersonaName_Get( steamID.GetAccountID() ), leaderboardEntry->m_nScore ) );
				CAvatarImagePanel *pAvatar = dynamic_cast< CAvatarImagePanel* >( pContainer->FindChildByName( "AvatarImage" ) );
				if ( pAvatar )
				{
					pAvatar->SetShouldDrawFriendIcon( false );
					pAvatar->SetPlayer( steamID, k_EAvatarSize32x32 );
				}
			}								
		}			
	}

	if ( m_pLeaderboardTitle )
	{
		bool bShowTitle = bVisible && scores.Count() > 0;
		if ( m_pLeaderboardTitle->IsVisible() != bShowTitle )
		{
			m_pLeaderboardTitle->SetVisible( bShowTitle );
		}
	}

	m_bShowingLeaderboard = bVisible;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateDialog()
{
	UpdateMainBackground();

	if ( g_bIsReplayRewinding || engine->IsLoadingDemo() || engine->IsPlayingDemo() || engine->IsSkippingPlayback() )
	{
		// hide all of the various panels for the other loadscreen modes
		if ( IsPC() )
		{
			ClearMapLabel();

			m_pPlayerData->SetVisible( false );
			m_pNextTipButton->SetVisible( false );
			m_pResetStatsButton->SetVisible( false );
			m_pInteractiveHeaders->SetVisible( false );
			m_pNonInteractiveHeaders->SetVisible( false );
			m_pTipText->SetVisible( false );
			m_pTipImage->SetVisible( false );

			if ( m_pMapInfoPanel )
			{
				m_pMapInfoPanel->SetVisible( false );

				vgui::Panel* pInfoBG = m_pMapInfoPanel->FindChildByName( "InfoBG" );
				if ( pInfoBG )
				{
					pInfoBG->SetVisible( false );
				}
			}
		}

		return;
	}

	RandomSeed( Plat_MSTime() );

	m_iTotalSpawns = 0;

	// if we don't have stats for any class, add empty stat entries for them 
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue; // Ignore the civilian.

		int j;
		for ( j = 0; j < m_aClassStats.Count(); j++ )
		{
			if ( m_aClassStats[j].iPlayerClass == iClass )
			{
				m_iTotalSpawns += m_aClassStats[j].iNumberOfRounds;
				break;
			}
		}
		if ( j == m_aClassStats.Count() )
		{
			ClassStats_t stats;
			stats.iPlayerClass = iClass;
			m_aClassStats.AddToTail( stats );
		}
	}

	ClearMapLabel();

#ifdef _X360
	if ( m_pFooter )
	{
		m_pFooter->ShowButtonLabel( "nexttip", m_bShowBackButton );
		m_pFooter->ShowButtonLabel( "back", m_bShowBackButton );
	}
#endif

	// fill out bar charts
	UpdateBarCharts();
	// fill out class details
	UpdateClassDetails();
	// update the tip
	UpdateTip();
	// show or hide controls depending on if we're interactive or not
	UpdateControls();		
}

//-----------------------------------------------------------------------------
// Purpose: Updates bar charts
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateBarCharts()
{
	// sort the class stats by the selected stat for right-hand bar chart
	m_aClassStats.Sort( &CTFStatsSummaryPanel::CompareClassStats );

	// loop for left & right hand charts
	for ( int iChart = 0; iChart < 2; iChart++ )
	{
		float flMax = 0;
		for ( int i = 0; i < m_aClassStats.Count(); i++ )
		{
			// get max value of stat being charted so we know how to scale the graph
			float flVal = GetDisplayValue( m_aClassStats[i], m_statBarGraph[iChart], m_displayBarGraph[iChart] );
			flMax = MAX( flVal, flMax );
		}

		// draw the bar chart value for each player class
		// TODO: Fix up after the civilian becomes playable.
		int iChartBar = 0;
		for ( int i = 0; i < m_aClassStats.Count(); i++ )
		{	
			int iClass = m_aClassStats[i].iPlayerClass;
			if ( iClass == TF_CLASS_CIVILIAN )
			{
				continue;
			}
			if ( 0 == iChart )
			{
				// if this is the first chart, set the class label for each class
				m_pPlayerData->SetDialogVariable( CFmtStr( "class%d", iChartBar+1 ), g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] ) );
			}
			// draw the bar for this class
			DisplayBarValue( iChart, iChartBar++, m_aClassStats[i], m_statBarGraph[iChart], m_displayBarGraph[iChart], flMax );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates class details
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateClassDetails( bool bIsMVM )
{
	vgui::Label *pTitle = assert_cast< vgui::Label* >( FindChildByName( "RecordsLabel1", true ) );
	if ( pTitle )
	{
		pTitle->SetText( bIsMVM ? "#StatSummary_Label_BestMVMMoments" : "#StatSummary_Label_BestMoments" );
	}

	const wchar_t *wzWithClassFmt = g_pVGuiLocalize->Find( "#StatSummary_ScoreAsClassFmt" );
	const wchar_t *wzWithoutClassFmt = L"%s1";

	ClassDetails_t *pStatDetails = ( bIsMVM ? g_PerClassMVMStatDetails : g_PerClassStatDetails );
	int nArraySize = ( bIsMVM ? ARRAYSIZE( g_PerClassMVMStatDetails ) : ARRAYSIZE( g_PerClassStatDetails ) );

	// display the record for each stat
	int iRow = 0;
	for ( int i = 0; i < nArraySize; i++ )
	{
		TFStatType_t statType = pStatDetails[i].statType;

		int iClass = TF_CLASS_UNDEFINED;
		int iMaxVal = 0;

		// if there is a selected class, and if this stat should not be shown for this class, skip this stat
		if ( m_iSelectedClass != TF_CLASS_UNDEFINED && ( 0 == ( pStatDetails[i].iFlagsClass & MAKESTATFLAG( m_iSelectedClass ) ) ) )
			continue;

		if ( m_iSelectedClass == TF_CLASS_UNDEFINED )
		{
			// if showing best from any class, look through all player classes to determine the max value of this stat
			for ( int j = 0; j  < m_aClassStats.Count(); j++ )
			{
				RoundStats_t *pRoundStats = &( bIsMVM ? m_aClassStats[j].maxMVM : m_aClassStats[j].max );
				if ( pRoundStats->m_iStat[statType] > iMaxVal )
				{
					// remember max value and class that has max value
					iMaxVal = pRoundStats->m_iStat[statType];
					iClass = m_aClassStats[j].iPlayerClass;
				}
			}
		}
		else
		{
			// show best from selected class
			iClass = m_iSelectedClass;
			for ( int j = 0; j  < m_aClassStats.Count(); j++ )
			{
				if ( m_aClassStats[j].iPlayerClass == iClass )
				{
					RoundStats_t *pRoundStats = &( bIsMVM ? m_aClassStats[j].maxMVM : m_aClassStats[j].max );
					iMaxVal = pRoundStats->m_iStat[statType];
					break;
				}
			}
		}

		wchar_t wzStatNum[32];
		wchar_t wzStatVal[128];
		if ( TFSTAT_PLAYTIME == statType )
		{
			// playtime gets displayed as a time string
			g_pVGuiLocalize->ConvertANSIToUnicode( FormatSeconds( iMaxVal ), wzStatNum, sizeof( wzStatNum ) );
		}
		else
		{
			// all other stats are just shown as a #
			swprintf_s( wzStatNum, ARRAYSIZE( wzStatNum ), L"%d", iMaxVal );
		}

		if ( TF_CLASS_UNDEFINED == m_iSelectedClass && iMaxVal > 0 )
		{
			// if we are doing a cross-class view (no single selected class) and the max value is non-zero, show "# (as <class>)"
			wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] );
			g_pVGuiLocalize->ConstructString_safe( wzStatVal, wzWithClassFmt, 2, wzStatNum, wzLocalizedClassName );
		}
		else
		{
			// just show the value
			g_pVGuiLocalize->ConstructString_safe( wzStatVal, wzWithoutClassFmt, 1, wzStatNum );
		}				

		// set the label
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dlabel", iRow+1 ), g_pVGuiLocalize->Find( pStatDetails[i].szResourceName ) );
		// set the value 
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dvalue", iRow+1 ), wzStatVal );

		iRow++;
	}

	// if there are any leftover rows for the selected class, fill out the remaining rows with blank labels and values
	for ( ; iRow < 15; iRow ++ )
	{
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dlabel", iRow+1 ), "" );
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dvalue", iRow+1 ), "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the tip
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateTip()
{
	int iTipClass = TF_CLASS_UNDEFINED;

	SetDialogVariable( "tiptext", g_TFTips.GetRandomTip( iTipClass ) );

	if ( m_pTipImage )
	{
		if ( iTipClass > TF_CLASS_UNDEFINED && iTipClass <= TF_CLASS_ENGINEER )
		{
			m_pTipImage->SetVisible( true );
			m_pTipImage->SetImage( g_pszTipsClassImages[iTipClass] );
		}
		else
		{
			m_pTipImage->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shows or hides controls
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateControls()
{
	// show or hide controls depending on what mode we're in
#ifndef _X360
	bool bShowPlayerData = ( m_bInteractive || m_iTotalSpawns > 0 );
#else
	bool bShowPlayerData = ( m_bInteractive || m_bShowBackButton || m_iTotalSpawns > 0 );
#endif
	m_pPlayerData->SetVisible( bShowPlayerData );
	m_pInteractiveHeaders->SetVisible( m_bInteractive );
	m_pNonInteractiveHeaders->SetVisible( !m_bInteractive );
	// A new player has no spawns, so bShowPlayerData is false on the loading
	// screen, and UpdateLeaderboard then shows the report anyway. Keep the tip.
	const bool bShowTip = bShowPlayerData || ( !m_bEmbedded && !m_bInteractive );
	m_pTipText->SetVisible( bShowTip );
	if ( m_pTipImage && !bShowTip )
		m_pTipImage->SetVisible( false );

	if ( !IsX360() )
	{
		if ( !m_bInteractive )
		{
			char szTemp[128];

			// update our non-interactive headers to match the current combo box selections
			Label *pLabel = dynamic_cast<Label *>( m_pNonInteractiveHeaders->FindChildByName( "BarChartLabelA" ) );
			if ( pLabel && m_pBarChartComboBoxA )
			{
				m_pBarChartComboBoxA->GetItemText( m_pBarChartComboBoxA->GetActiveItem(), szTemp, sizeof( szTemp ) );
				pLabel->SetText( szTemp );
			}

			pLabel = dynamic_cast<Label *>( m_pNonInteractiveHeaders->FindChildByName( "BarChartLabelB" ) );
			if ( pLabel && m_pBarChartComboBoxB )
			{
				m_pBarChartComboBoxB->GetItemText( m_pBarChartComboBoxB->GetActiveItem(), szTemp, sizeof( szTemp ) );
				pLabel->SetText( szTemp );
			}

			pLabel = dynamic_cast<Label *>( m_pNonInteractiveHeaders->FindChildByName( "OverallRecordLabel" ) );
			if ( pLabel && m_pClassComboBox )
			{
				m_pClassComboBox->GetItemText( m_pClassComboBox->GetActiveItem(), szTemp, sizeof( szTemp ) );
				pLabel->SetText( szTemp );
			}
		}
	}

#ifndef _X360
	m_pNextTipButton->SetVisible( m_bInteractive );
	m_pResetStatsButton->SetVisible( m_bInteractive );
	m_pCloseButton->SetVisible( m_bInteractive && !m_bEmbedded );
#endif
}

//-----------------------------------------------------------------------------
// The time/score rows are reparented under a scroller. Dialog variables are
// only pushed to StatData's direct children, so forward them down to the labels.
//-----------------------------------------------------------------------------
static void ForwardClassStatDialogVariables( Panel *pPanel, KeyValues *pDialogVariables )
{
	if ( !pPanel || !pDialogVariables )
		return;

	for ( int i = 0; i < pPanel->GetChildCount(); i++ )
	{
		Panel *pChild = pPanel->GetChild( i );
		if ( !pChild )
			continue;

		// OnDialogVariablesChanged is protected. The message map is what calls it.
		ipanel()->SendMessage( pChild->GetVPanel(), pDialogVariables, pPanel->GetVPanel() );

		ForwardClassStatDialogVariables( pChild, pDialogVariables );
	}
}

class CTFClassStatScroll : public vgui::ScrollableEditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFClassStatScroll, vgui::ScrollableEditablePanel );

public:
	CTFClassStatScroll( Panel *pParent, EditablePanel *pChild, const char *pszName )
		: BaseClass( pParent, pChild, pszName )
	{
	}

	MESSAGE_FUNC_PARAMS( OnDialogVariablesChanged, "DialogVariables", dialogVariables );
};

void CTFClassStatScroll::OnDialogVariablesChanged( KeyValues *dialogVariables )
{
	ForwardClassStatDialogVariables( this, dialogVariables );
}

//-----------------------------------------------------------------------------
// Purpose: Drop the scroller created by SetupClassStatScroll.
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ReleaseClassStatScroll()
{
	if ( !m_pPlayerData )
		return;

	Panel *pScroll = m_pPlayerData->FindChildByName( "ClassStatScroll", false );
	if ( !pScroll )
		return;

	pScroll->SetParent( (Panel *)NULL );
	delete pScroll;
}

//-----------------------------------------------------------------------------
// "%class10%" has to be registered the same way Label::ApplySettings does,
// or SetDialogVariable never substitutes it.
//-----------------------------------------------------------------------------
static void SetClassStatLabelVar( Label *pLabel, const char *pszVarFmt, int iRow )
{
	char szVar[64];
	V_snprintf( szVar, sizeof( szVar ), pszVarFmt, iRow );

	char szToken[96];
	V_snprintf( szToken, sizeof( szToken ), "%%%s%%", szVar );

	wchar_t unicodeVar[128];
	g_pVGuiLocalize->ConvertANSIToUnicode( szToken, unicodeVar, sizeof( unicodeVar ) );

	char szLocalized[128];
	V_snprintf( szLocalized, sizeof( szLocalized ), "#var_%s", szToken );
	g_pVGuiLocalize->AddString( szLocalized + 1, unicodeVar, "" );
	pLabel->SetText( szLocalized );
}

//-----------------------------------------------------------------------------
// Purpose: Grow the points / playtime frame upward. The bottom stays put so
//		the class tip still fits under the list. The records column is not moved.
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::LiftPerformanceReport()
{
	if ( m_bStatFrameLifted || !m_pPlayerData )
		return;

	Panel *pAvg = m_pPlayerData->FindChildByName( "AveragesBG", false );
	if ( !pAvg || pAvg->GetTall() < 20 )
		return;

	const int nLift = scheme()->GetProportionalScaledValueEx( GetScheme(), 64 );
	if ( nLift < 1 )
		return;

	// The map heading is moved over the records panel, so this frame can
	// grow into the space that heading used to occupy.
	const int nMinY = scheme()->GetProportionalScaledValueEx( GetScheme(), 28 );
	int x, y, w, h;
	pAvg->GetBounds( x, y, w, h );
	// Still the pre-layout value. Leave the flag clear so the next pass can lift.
	if ( y < nMinY )
		return;
	int nUse = nLift;
	if ( y - nUse < nMinY )
		nUse = y - nMinY;
	if ( nUse < 1 )
	{
		m_bStatFrameLifted = true;
		return;
	}

	pAvg->SetBounds( x, y - nUse, w, h + nUse );

	Panel *pMove[] =
	{
		m_pPlayerData->FindChildByName( "AveragesLabel", false ),
		m_pPlayerData->FindChildByName( "BarChartLabelA", true ),
		m_pPlayerData->FindChildByName( "BarChartLabelB", true ),
		m_pBarChartComboBoxA,
		m_pBarChartComboBoxB,
	};
	for ( int i = 0; i < ARRAYSIZE( pMove ); i++ )
	{
		Panel *pPanel = pMove[i];
		if ( !pPanel )
			continue;
		int px, py;
		pPanel->GetPos( px, py );
		pPanel->SetPos( px, py - nUse );
	}

	// Class rows are not moved here. SpreadClassStatRows places every class
	// between these headers and the tip, using the frame this just grew.
	m_bStatFrameLifted = true;
}

//-----------------------------------------------------------------------------
// Purpose: Center "on your way to" / map name / map type on the records panel.
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::AlignMapHeadingToRecords()
{
	if ( !m_pPlayerData )
		return;

	Panel *pRecords = m_pPlayerData->FindChildByName( "RecordsBG", false );
	if ( !pRecords || pRecords->GetWide() < 20 )
		return;

	int nCenter = pRecords->GetXPos() + pRecords->GetWide() / 2;
	Panel *pParent = pRecords->GetParent();
	if ( pParent && pParent != this )
	{
		int px, py;
		pParent->GetPos( px, py );
		nCenter += px;
	}
	// Not laid out yet. A center this small would pin the heading to the photo.
	if ( nCenter < pRecords->GetWide() )
		return;

	const char *pszLabels[] = { "OnYourWayLabel", "MapLabel", "MapType" };
	for ( int i = 0; i < ARRAYSIZE( pszLabels ); i++ )
	{
		Panel *pLabel = FindChildByName( pszLabels[i], false );
		if ( !pLabel || pLabel->GetWide() < 20 )
			continue;
		int lx, ly;
		pLabel->GetPos( lx, ly );
		pLabel->SetPos( nCenter - pLabel->GetWide() / 2, ly );
	}
}

//-----------------------------------------------------------------------------
// Y of pPanel in pAncestor's space. False if pPanel is not under pAncestor.
//-----------------------------------------------------------------------------
static bool YInAncestor( Panel *pAncestor, Panel *pPanel, int &outY )
{
	outY = 0;
	for ( Panel *p = pPanel; p; p = p->GetParent() )
	{
		if ( p == pAncestor )
			return true;
		outY += p->GetYPos();
	}
	return false;
}

static int CountStatSummaryClasses()
{
	int nNeeded = 0;
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue;
		nNeeded++;
	}
	return nNeeded;
}

//-----------------------------------------------------------------------------
// Copy one stock class-row control, including its image, so the extra classes
// are real StatData children and dialog variables reach them.
//-----------------------------------------------------------------------------
static bool CloneClassStatControl( EditablePanel *pParent, Panel *pTemplate, const char *pszName, const char *pszLabelText )
{
	if ( !pParent || !pTemplate || !pszName )
		return false;
	if ( pParent->FindChildByName( pszName, false ) )
		return true;

	KeyValues *pKV = new KeyValues( "Control" );
	pTemplate->GetSettings( pKV );
	pKV->SetString( "fieldName", pszName );
	if ( pszLabelText )
		pKV->SetString( "labelText", pszLabelText );

	const char *pszClass = pKV->GetString( "ControlName", NULL );
	Panel *pNew = ( pszClass && pszClass[0] ) ? pParent->CreateControlByName( pszClass ) : NULL;
	if ( !pNew )
	{
		if ( dynamic_cast< Label * >( pTemplate ) )
			pNew = new Label( pParent, pszName, "" );
		else if ( dynamic_cast< ImagePanel * >( pTemplate ) )
			pNew = new ImagePanel( pParent, pszName );
		else
		{
			pKV->deleteThis();
			return false;
		}
	}
	else
	{
		pNew->SetParent( pParent );
	}

	pNew->SetScheme( pTemplate->GetScheme() );
	pNew->SetProportional( pTemplate->IsProportional() );
	pNew->ApplySettings( pKV );
	pKV->deleteThis();

	Label *pSrcLabel = dynamic_cast< Label * >( pTemplate );
	Label *pDstLabel = dynamic_cast< Label * >( pNew );
	if ( pSrcLabel && pDstLabel )
	{
		pDstLabel->SetFont( pSrcLabel->GetFont() );
		pDstLabel->SetFgColor( pSrcLabel->GetFgColor() );
	}
	else
	{
		ImagePanel *pSrcImg = dynamic_cast< ImagePanel * >( pTemplate );
		ImagePanel *pDstImg = dynamic_cast< ImagePanel * >( pNew );
		if ( pSrcImg && pDstImg )
		{
			pDstImg->SetFillColor( pSrcImg->GetFillColor() );
			if ( !pDstImg->GetImage() && pSrcImg->GetImage() )
				pDstImg->SetImage( pSrcImg->GetImage() );
		}
	}

	pNew->SetZPos( pTemplate->GetZPos() );
	pNew->SetMouseInputEnabled( false );
	pNew->SetVisible( true );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Remember the track each bar grows from. StatData space, not a
//		scroller. A bar DisplayBarValue already shrank is not the track.
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::CaptureClassBarAnchors()
{
	if ( m_bBarAnchorsReady || !m_pPlayerData )
		return;

	Panel *pBGL = m_pPlayerData->FindChildByName( "ClassBarBG1A", false );
	Panel *pBGR = m_pPlayerData->FindChildByName( "ClassBarBG1B", false );
	Panel *pBarL = m_pPlayerData->FindChildByName( "ClassBar1A", false );
	Panel *pBarR = m_pPlayerData->FindChildByName( "ClassBar1B", false );
	if ( !pBarL || !pBarR )
		return;

	// DisplayBarValue shrinks the fill. A bar that still matches its track is
	// the resource width; otherwise the track image is the unscaled size.
	const bool bBarFull = pBarL->GetWide() > 8 && pBarR->GetXPos() > pBarL->GetXPos()
		&& ( !pBGL || pBarL->GetWide() >= pBGL->GetWide() - 2 );
	Panel *pLeft = ( bBarFull || !pBGL || pBGL->GetWide() <= 8 ) ? pBarL : pBGL;
	Panel *pRight = ( bBarFull || !pBGR || pBGR->GetWide() <= 8 ) ? pBarR : pBGR;
	if ( pLeft->GetXPos() <= 0 || pLeft->GetWide() <= 8 || pRight->GetXPos() <= pLeft->GetXPos() )
		return;

	int y;
	pLeft->GetBounds( m_xStartLHBar, y, m_iBarMaxWidth, m_iBarHeight );
	pRight->GetBounds( m_xStartRHBar, y, m_iBarMaxWidth, m_iBarHeight );
	if ( pBarL->GetTall() > 0 )
		m_iBarHeight = pBarL->GetTall();
	if ( m_iBarHeight < 1 )
		m_iBarHeight = 1;
	m_bBarAnchorsReady = m_iBarMaxWidth > 1;
}

//-----------------------------------------------------------------------------
// Purpose: Loading-screen class rows. Extra classes are cloned onto StatData
//		(no scrollbar). Returns false while "c+" positions are still zero.
//-----------------------------------------------------------------------------
bool CTFStatsSummaryPanel::SetupClassStatRows()
{
	if ( !m_pPlayerData )
		return false;

	// A previous build of this screen parented the rows under a scroller.
	ReleaseClassStatScroll();

	Label *pLabel1 = dynamic_cast< Label * >( m_pPlayerData->FindChildByName( "ClassLabel1", false ) );
	if ( !pLabel1 )
		return false;

	int nExisting = 0;
	for ( int i = 1; i < 64; i++ )
	{
		if ( !m_pPlayerData->FindChildByName( CFmtStr( "ClassLabel%d", i ), false ) )
			break;
		nExisting = i;
	}
	if ( nExisting < 1 )
		return false;

	Panel *pLeft = m_pPlayerData->FindChildByName( "ClassBarBG1A", false );
	if ( !pLeft )
		pLeft = m_pPlayerData->FindChildByName( "ClassBar1A", false );
	if ( !pLeft )
		return false;

	// "c+" values read as 0 until layout. Cloning off those stacks every name
	// on the map photo.
	if ( pLeft->GetXPos() <= 0 || pLabel1->GetXPos() <= pLeft->GetXPos() )
		return false;
	if ( pLabel1->GetTall() < 1 )
		return false;

	Label *pLabel2 = dynamic_cast< Label * >( m_pPlayerData->FindChildByName( "ClassLabel2", false ) );
	if ( pLabel2 && pLabel2->GetYPos() <= pLabel1->GetYPos() )
		return false;

	const int nNeeded = CountStatSummaryClasses();
	if ( nNeeded > nExisting )
	{
		struct ClassStatControl_t
		{
			const char *pszPattern;
			bool bLabel;
			const char *pszVar;
		};
		const ClassStatControl_t kControls[] =
		{
			{ "ClassBarBG%dA",		false,	NULL },
			{ "ClassBarBG%dB",		false,	NULL },
			{ "ClassBar%dA",		false,	NULL },
			{ "ClassBar%dB",		false,	NULL },
			{ "ClassBarLabel%dA",	true,	"classbarlabel%dA" },
			{ "ClassBarLabel%dB",	true,	"classbarlabel%dB" },
			{ "ClassLabel%d",		true,	"class%d" },
		};

		for ( int iRow = nExisting + 1; iRow <= nNeeded; iRow++ )
		{
			const int iColorRow = ( ( iRow - 1 ) % nExisting ) + 1;
			for ( int iCtl = 0; iCtl < ARRAYSIZE( kControls ); iCtl++ )
			{
				char szName[64];
				V_snprintf( szName, sizeof( szName ), kControls[iCtl].pszPattern, iRow );

				char szColorName[64];
				V_snprintf( szColorName, sizeof( szColorName ), kControls[iCtl].pszPattern, iColorRow );
				Panel *pTemplate = m_pPlayerData->FindChildByName( szColorName, false );
				if ( !pTemplate )
				{
					V_snprintf( szColorName, sizeof( szColorName ), kControls[iCtl].pszPattern, 1 );
					pTemplate = m_pPlayerData->FindChildByName( szColorName, false );
				}
				if ( !pTemplate )
					continue;

				const char *pszLabelText = NULL;
				char szToken[96];
				if ( kControls[iCtl].bLabel && kControls[iCtl].pszVar )
				{
					char szVar[64];
					V_snprintf( szVar, sizeof( szVar ), kControls[iCtl].pszVar, iRow );
					V_snprintf( szToken, sizeof( szToken ), "%%%s%%", szVar );
					pszLabelText = szToken;
				}

				CloneClassStatControl( m_pPlayerData, pTemplate, szName, pszLabelText );
			}
		}
	}

	CaptureClassBarAnchors();
	return m_bBarAnchorsReady;
}

//-----------------------------------------------------------------------------
// Purpose: Vanilla chart, stretched to the taller report. Class name stays on
//		its X (centered between the bars). Rows run from just under the column
//		headers to just above the class tip. Safe to call every layout.
//-----------------------------------------------------------------------------
bool CTFStatsSummaryPanel::SpreadClassStatRows()
{
	if ( !m_pPlayerData )
		return false;

	Panel *pAvg = m_pPlayerData->FindChildByName( "AveragesBG", false );
	Label *pLabel1 = dynamic_cast< Label * >( m_pPlayerData->FindChildByName( "ClassLabel1", false ) );
	if ( !pAvg || !pLabel1 || pLabel1->GetTall() < 1 || pAvg->GetTall() < 20 )
		return false;

	int nRows = 0;
	for ( int i = 1; i < 64; i++ )
	{
		if ( !m_pPlayerData->FindChildByName( CFmtStr( "ClassLabel%d", i ), false ) )
			break;
		nRows = i;
	}
	if ( nRows < 1 )
		return false;

	int nFrameY = pAvg->GetYPos();
	int nFrameH = pAvg->GetTall();
	if ( nFrameY < scheme()->GetProportionalScaledValueEx( GetScheme(), 20 ) )
		return false;

	const int nGap = scheme()->GetProportionalScaledValueEx( GetScheme(), 3 );
	int nHeaderBottom = -1;
	Panel *pHeaderLabels[2];
	if ( m_bInteractive )
	{
		pHeaderLabels[0] = m_pBarChartComboBoxA;
		pHeaderLabels[1] = m_pBarChartComboBoxB;
	}
	else
	{
		pHeaderLabels[0] = m_pNonInteractiveHeaders ? m_pNonInteractiveHeaders->FindChildByName( "BarChartLabelA", false ) : NULL;
		pHeaderLabels[1] = m_pNonInteractiveHeaders ? m_pNonInteractiveHeaders->FindChildByName( "BarChartLabelB", false ) : NULL;
	}
	for ( int i = 0; i < 2; i++ )
	{
		int y;
		if ( !pHeaderLabels[i] || !pHeaderLabels[i]->IsVisible() )
			continue;
		if ( !YInAncestor( m_pPlayerData, pHeaderLabels[i], y ) )
			continue;
		const int nBottom = y + pHeaderLabels[i]->GetTall();
		if ( nBottom > nHeaderBottom )
			nHeaderBottom = nBottom;
	}
	// Headers not laid out yet. Placing rows now stacks them on the title.
	if ( nHeaderBottom < nFrameY )
		return false;

	int yStart = nHeaderBottom + nGap;
	int yLimit = nFrameY + nFrameH - nGap;

	int nTipTop = -1;
	int nTipY = 0;
	int nDataY = 0;
	if ( YInAncestor( this, m_pPlayerData, nDataY ) )
	{
		if ( m_pTipText && m_pTipText->IsVisible() && YInAncestor( this, m_pTipText, nTipY ) )
			nTipTop = nTipY - nDataY;
		if ( m_pTipImage && m_pTipImage->IsVisible() && YInAncestor( this, m_pTipImage, nTipY ) )
		{
			const int nImgTop = nTipY - nDataY;
			if ( nTipTop < 0 || nImgTop < nTipTop )
				nTipTop = nImgTop;
		}
	}
	// The portrait sits inside the report. Keep the last class above it.
	if ( nTipTop > yStart + pLabel1->GetTall() && nTipTop < nFrameY + nFrameH + nGap * 4 )
		yLimit = nTipTop - nGap;

	const int rowTall = pLabel1->GetTall();
	if ( yLimit - yStart < rowTall )
		return false;

	int pitch = ( nRows > 1 ) ? ( yLimit - yStart - rowTall ) / ( nRows - 1 ) : 0;
	if ( nRows > 1 && pitch < 1 )
		return false;
	// Names have to stay readable. Even spacing wins when the frame is tall
	// enough; otherwise sit them on the label height instead of overlapping.
	if ( nRows > 1 && pitch < rowTall )
		pitch = rowTall;

	static const char *kRowPatterns[] =
	{
		"ClassLabel%d",
		"ClassBarBG%dA", "ClassBarBG%dB",
		"ClassBar%dA", "ClassBar%dB",
		"ClassBarLabel%dA", "ClassBarLabel%dB",
	};

	for ( int iRow = 1; iRow <= nRows; iRow++ )
	{
		Panel *pName = m_pPlayerData->FindChildByName( CFmtStr( "ClassLabel%d", iRow ), false );
		if ( !pName )
			continue;
		const int nBaseY = pName->GetYPos();
		const int nNewY = yStart + ( iRow - 1 ) * pitch;

		for ( int iCtl = 0; iCtl < ARRAYSIZE( kRowPatterns ); iCtl++ )
		{
			Panel *pRow = m_pPlayerData->FindChildByName( CFmtStr( kRowPatterns[iCtl], iRow ), false );
			if ( !pRow )
				continue;
			int x, y;
			pRow->GetPos( x, y );
			pRow->SetPos( x, nNewY + ( y - nBaseY ) );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Embedded stats page. That panel is still the short res-file frame,
//		so the extra classes scroll. The loading screen does not use this.
//		Returns false when positions are not ready yet.
//-----------------------------------------------------------------------------
bool CTFStatsSummaryPanel::SetupClassStatScroll()
{
	if ( !m_pPlayerData )
		return false;

	if ( m_pPlayerData->FindChildByName( "ClassStatScroll", false ) )
		return true;

	Label *pLabel1 = dynamic_cast< Label * >( m_pPlayerData->FindChildByName( "ClassLabel1", false ) );
	Label *pLabel2 = dynamic_cast< Label * >( m_pPlayerData->FindChildByName( "ClassLabel2", false ) );
	if ( !pLabel1 )
		return false;

	int nExisting = 0;
	for ( int i = 1; i < 64; i++ )
	{
		if ( !m_pPlayerData->FindChildByName( CFmtStr( "ClassLabel%d", i ), false ) )
			break;
		nExisting = i;
	}
	if ( nExisting < 1 )
		return false;

	int nNeeded = 0;
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue;
		nNeeded++;
	}
	if ( nNeeded <= nExisting )
		return true;

	int y0 = pLabel1->GetYPos();
	int pitch = pLabel1->GetTall();
	if ( pLabel2 )
		pitch = pLabel2->GetYPos() - y0;
	if ( pitch < 1 )
		pitch = pLabel1->GetTall();

	Label *pLabelLast = dynamic_cast< Label * >( m_pPlayerData->FindChildByName( CFmtStr( "ClassLabel%d", nExisting ), false ) );
	int viewH = ( pLabelLast ? ( pLabelLast->GetYPos() - y0 ) : ( nExisting - 1 ) * pitch ) + pLabel1->GetTall();
	int contentH = ( nNeeded - 1 ) * pitch + pLabel1->GetTall();

	Panel *pLeft = m_pPlayerData->FindChildByName( "ClassBarBG1A", false );
	if ( !pLeft )
		pLeft = m_pPlayerData->FindChildByName( "ClassBar1A", false );
	if ( !pLeft )
		return false;

	// "c+" values read as 0 until layout. Reparenting off those puts every
	// class name to the left of the performance report.
	if ( pLeft->GetXPos() <= 0 || pLabel1->GetXPos() <= pLeft->GetXPos() )
		return false;
	if ( pLabel2 && pLabel2->GetYPos() <= y0 )
		return false;
	if ( pLabel1->GetTall() < 1 || viewH < 1 )
		return false;

	const int nScrollbarWide = 16;
	int parentW = m_pPlayerData->GetWide();
	int scrollX = pLeft->GetXPos();
	// Class labels are 300px so long names do not clip, which would run the
	// scrollbar through the records column. Stop at that column when it is showing.
	int scrollRight = pLabel1->GetXPos() + 120 + nScrollbarWide;
	Panel *pRecords = m_pPlayerData->FindChildByName( "RecordsBG", false );
	if ( pRecords && pRecords->IsVisible() && pRecords->GetXPos() > pLabel1->GetXPos() + 48 )
		scrollRight = pRecords->GetXPos() - 2;
	int scrollW = scrollRight - scrollX;
	if ( scrollX < 0 )
		scrollX = 0;
	if ( scrollX + scrollW > parentW )
		scrollW = parentW - scrollX;
	if ( scrollW < nScrollbarWide + 32 || viewH < 1 )
		return false;

	EditablePanel *pChild = new EditablePanel( NULL, "ClassStatList" );
	pChild->SetPaintBackgroundEnabled( false );
	pChild->SetMouseInputEnabled( false );
	pChild->SetBounds( 0, 0, scrollW - nScrollbarWide, contentH );

	CTFClassStatScroll *pScroll = new CTFClassStatScroll( m_pPlayerData, pChild, "ClassStatScroll" );
	pScroll->SetBounds( scrollX, y0, scrollW, viewH );
	pScroll->SetPaintBackgroundEnabled( false );
	pScroll->SetFgColor( Color( 210, 190, 150, 255 ) );
	pScroll->SetZPos( 8 );

	struct ClassStatControl_t
	{
		const char *pszPattern;
		bool bLabel;
		const char *pszVar;
	};
	static const ClassStatControl_t kControls[] =
	{
		{ "ClassBarBG%dA",		false,	NULL },
		{ "ClassBarBG%dB",		false,	NULL },
		{ "ClassBar%dA",		false,	NULL },
		{ "ClassBar%dB",		false,	NULL },
		{ "ClassBarLabel%dA",	true,	"classbarlabel%dA" },
		{ "ClassBarLabel%dB",	true,	"classbarlabel%dB" },
		{ "ClassLabel%d",		true,	"class%d" },
	};

	for ( int iRow = 1; iRow <= nExisting; iRow++ )
	{
		for ( int iCtl = 0; iCtl < ARRAYSIZE( kControls ); iCtl++ )
		{
			char szName[64];
			V_snprintf( szName, sizeof( szName ), kControls[iCtl].pszPattern, iRow );
			Panel *pPanel = m_pPlayerData->FindChildByName( szName, false );
			if ( !pPanel )
				continue;
			int x, y;
			pPanel->GetPos( x, y );
			pPanel->SetParent( pChild );
			pPanel->SetPos( x - scrollX, y - y0 );
			pPanel->SetMouseInputEnabled( false );
		}
	}

	for ( int iRow = nExisting + 1; iRow <= nNeeded; iRow++ )
	{
		const int iColorRow = ( ( iRow - 1 ) % nExisting ) + 1;
		for ( int iCtl = 0; iCtl < ARRAYSIZE( kControls ); iCtl++ )
		{
			char szTemplate[64];
			char szName[64];
			V_snprintf( szTemplate, sizeof( szTemplate ), kControls[iCtl].pszPattern, 1 );
			V_snprintf( szName, sizeof( szName ), kControls[iCtl].pszPattern, iRow );

			Panel *pTemplate = pChild->FindChildByName( szTemplate, false );
			if ( !pTemplate )
				continue;

			char szColorName[64];
			V_snprintf( szColorName, sizeof( szColorName ), kControls[iCtl].pszPattern, iColorRow );
			Panel *pColorSrc = pChild->FindChildByName( szColorName, false );
			if ( !pColorSrc )
				pColorSrc = pTemplate;

			int x, y, w, h;
			pTemplate->GetBounds( x, y, w, h );
			const int yRow = y + ( iRow - 1 ) * pitch;

			if ( kControls[iCtl].bLabel )
			{
				Label *pSrc = dynamic_cast< Label * >( pTemplate );
				if ( !pSrc || !kControls[iCtl].pszVar )
					continue;
				Label *pNew = new Label( pChild, szName, "" );
				pNew->SetBounds( x, yRow, w, h );
				pNew->SetFont( pSrc->GetFont() );
				// Every stock row is west-aligned. There is no getter for it.
				pNew->SetContentAlignment( Label::a_west );
				pNew->SetZPos( pSrc->GetZPos() );
				pNew->SetMouseInputEnabled( false );
				SetClassStatLabelVar( pNew, kControls[iCtl].pszVar, iRow );
			}
			else
			{
				ImagePanel *pSrc = dynamic_cast< ImagePanel * >( pColorSrc );
				if ( !pSrc )
					pSrc = dynamic_cast< ImagePanel * >( pTemplate );
				if ( !pSrc )
					continue;
				ImagePanel *pNew = new ImagePanel( pChild, szName );
				pNew->SetBounds( x, yRow, w, h );
				pNew->SetFillColor( pSrc->GetFillColor() );
				pNew->SetZPos( pSrc->GetZPos() );
				pNew->SetMouseInputEnabled( false );
			}
		}
	}

	int nLabelRoom = ( scrollW - nScrollbarWide ) - 2;
	for ( int iRow = 1; iRow <= nNeeded; iRow++ )
	{
		char szName[64];
		V_snprintf( szName, sizeof( szName ), "ClassLabel%d", iRow );
		Label *pName = dynamic_cast< Label * >( pChild->FindChildByName( szName, false ) );
		if ( !pName )
			continue;
		int x, y, w, h;
		pName->GetBounds( x, y, w, h );
		if ( x + w > nLabelRoom && nLabelRoom > x )
			pName->SetWide( nLabelRoom - x );
	}

	pScroll->InvalidateLayout( true, true );

	Panel *pLHBar = pChild->FindChildByName( "ClassBar1A", false );
	Panel *pRHBar = pChild->FindChildByName( "ClassBar1B", false );
	if ( pLHBar && pRHBar )
	{
		int y;
		pLHBar->GetBounds( m_xStartLHBar, y, m_iBarMaxWidth, m_iBarHeight );
		pRHBar->GetBounds( m_xStartRHBar, y, m_iBarMaxWidth, m_iBarHeight );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes a bar chart combo box
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::InitBarChartComboBox( ComboBox *pComboBox )
{
	struct BarChartComboInit_t
	{
		TFStatType_t statType;
		StatDisplay_t statDisplay;
		const char *szName;
	};

	BarChartComboInit_t initData[] =
	{
		{ TFSTAT_POINTSSCORED, SHOW_MAX, "#StatSummary_StatTitle_MostPoints" },
		{ TFSTAT_POINTSSCORED, SHOW_AVG, "#StatSummary_StatTitle_AvgPoints" },
		{ TFSTAT_KILLS, SHOW_MAX, "#StatSummary_StatTitle_MostKills" },
		{ TFSTAT_KILLS, SHOW_AVG, "#StatSummary_StatTitle_AvgKills" },
		{ TFSTAT_CAPTURES, SHOW_MAX, "#StatSummary_StatTitle_MostCaptures" },
		{ TFSTAT_CAPTURES, SHOW_AVG, "#StatSummary_StatTitle_AvgCaptures" },
		{ TFSTAT_KILLASSISTS, SHOW_MAX, "#StatSummary_StatTitle_MostAssists" },
		{ TFSTAT_KILLASSISTS, SHOW_AVG, "#StatSummary_StatTitle_AvgAssists" },
		{ TFSTAT_DAMAGE, SHOW_MAX, "#StatSummary_StatTitle_MostDamage" },
		{ TFSTAT_DAMAGE, SHOW_AVG, "#StatSummary_StatTitle_AvgDamage" },
		{ TFSTAT_PLAYTIME, SHOW_TOTAL, "#StatSummary_StatTitle_TotalPlaytime" },
		{ TFSTAT_PLAYTIME, SHOW_MAX, "#StatSummary_StatTitle_LongestLife" },
	};

	// set the font
	HFont hFont = scheme()->GetIScheme( GetScheme() )->GetFont( "ScoreboardVerySmall", true );
	pComboBox->SetFont( hFont );
	pComboBox->RemoveAll();
	// add all the options to the combo box
	for ( int i=0; i < ARRAYSIZE( initData ); i++ )
	{
		KeyValues *pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "stattype", initData[i].statType );
		pKeyValues->SetInt( "statdisplay", initData[i].statDisplay );
		pComboBox->AddItem(  g_pVGuiLocalize->Find( initData[i].szName ), pKeyValues );
	}
	pComboBox->SetNumberOfEditLines( ARRAYSIZE( initData ) );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function that sets the specified dialog variable to
//		"<value> (as <localized class name>)"
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetValueAsClass( const char *pDialogVariable, int iValue, int iPlayerClass )
{
	if ( iValue > 0 )
	{
		wchar_t *wzScoreAsClassFmt = g_pVGuiLocalize->Find( "#StatSummary_ScoreAsClassFmt" );
		wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[iPlayerClass] );
		wchar_t wzVal[16];
		wchar_t wzMsg[128];
		swprintf( wzVal, ARRAYSIZE( wzVal ), L"%d", iValue );
		g_pVGuiLocalize->ConstructString_safe( wzMsg, wzScoreAsClassFmt, 2, wzVal, wzLocalizedClassName );
		m_pPlayerData->SetDialogVariable( pDialogVariable, wzMsg );
	}
	else
	{
		m_pPlayerData->SetDialogVariable( pDialogVariable, "0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the specified bar chart item to the specified value, in range 0->1
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::DisplayBarValue( int iChart, int iBar, ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay, float flMaxValue )
{
	const char *szControlSuffix = ( 0 == iChart ? "A" : "B" );
	// Embedded rows live under the scroller. Loading-screen rows are direct
	// children. Either way a recursive search finds them.
	Panel *pBar = m_pPlayerData->FindChildByName( CFmtStr( "ClassBar%d%s", iBar+1, szControlSuffix ), true );
	Label *pLabel = dynamic_cast<Label*>( m_pPlayerData->FindChildByName( CFmtStr( "classbarlabel%d%s", iBar+1, szControlSuffix ), true ) );
	if ( !pBar || !pLabel )
		return;

	// get the stat value
	float flValue = GetDisplayValue( stats, statType, statDisplay );
	// calculate the bar size to draw, in the range of 0.0->1.0
	float flBarRange = SafeCalcFraction( flValue, flMaxValue );
	// calculate the # of pixels of bar width to draw
	int iBarWidth = MAX( (int) ( flBarRange * (float) m_iBarMaxWidth ), 1 );

	// Get the text label to draw for this bar.  For values of 0, draw nothing, to minimize clutter
	const char *szLabel = ( flValue > 0 ? RenderValue( flValue, statType, statDisplay ) : "" );
	// draw the label outside the bar if there's room
	bool bLabelOutsideBar = true;
	const int iLabelSpacing = 4;
	HFont hFont = pLabel->GetFont();
	int iLabelWidth = UTIL_ComputeStringWidth( hFont, szLabel );
	if ( iBarWidth + iLabelWidth + iLabelSpacing > m_iBarMaxWidth )
	{
		// if there's not room outside the bar for the label, draw it inside the bar
		bLabelOutsideBar = false;
	}

	int xBar,yBar,xLabel,yLabel;
	pBar->GetPos( xBar,yBar );
	pLabel->GetPos( xLabel,yLabel );

	m_pPlayerData->SetDialogVariable( CFmtStr( "classbarlabel%d%s", iBar+1, szControlSuffix ), szLabel );
	if ( 1 == iChart )	
	{
		// drawing code for RH bar chart
		xBar = m_xStartRHBar;
		pBar->SetBounds( xBar, yBar, iBarWidth, m_iBarHeight );
		if ( bLabelOutsideBar )
		{	
			pLabel->SetPos( xBar + iBarWidth + iLabelSpacing, yLabel );
		}
		else
		{
			pLabel->SetPos( xBar + iBarWidth - ( iLabelWidth + iLabelSpacing ), yLabel );
		}
	}
	else
	{
		// drawing code for LH bar chart
		xBar = m_xStartLHBar + m_iBarMaxWidth - iBarWidth;
		pBar->SetBounds( xBar, yBar, iBarWidth, m_iBarHeight );
		if ( bLabelOutsideBar )
		{	
			pLabel->SetPos( xBar - ( iLabelWidth + iLabelSpacing ), yLabel );
		}
		else
		{
			pLabel->SetPos( xBar + iLabelSpacing, yLabel );
		}
	}		
}

//-----------------------------------------------------------------------------
// Purpose: Calculates a fraction and guards from divide by 0.  (Returns 0 if 
//			denominator is 0.)
//-----------------------------------------------------------------------------
float CTFStatsSummaryPanel::SafeCalcFraction( float flNumerator, float flDemoninator )
{
	if ( 0 == flDemoninator )
		return 0;
	return flNumerator / flDemoninator;
}

//-----------------------------------------------------------------------------
// Purpose: Formats # of seconds into a string
//-----------------------------------------------------------------------------
const char *FormatSeconds( int seconds )
{
	static char string[64];

	int hours = 0;
	int minutes = seconds / 60;

	if ( minutes > 0 )
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;

		if ( hours > 0 )
		{
			minutes -= (hours * 60);
		}
	}

	if ( hours > 0 )
	{
		Q_snprintf( string, sizeof(string), "%2i:%02i:%02i", hours, minutes, seconds );
	}
	else
	{
		Q_snprintf( string, sizeof(string), "%02i:%02i", minutes, seconds );
	}

	return string;
}

//-----------------------------------------------------------------------------
// Purpose: Static sort function that sorts in descending order by play time
//-----------------------------------------------------------------------------
int __cdecl CTFStatsSummaryPanel::CompareClassStats( const ClassStats_t *pStats0, const ClassStats_t *pStats1 )
{
	// sort stats first by right-hand bar graph
	TFStatType_t statTypePrimary = GStatsSummaryPanel()->m_statBarGraph[1];
	StatDisplay_t statDisplayPrimary = GStatsSummaryPanel()->m_displayBarGraph[1];
	// then by left-hand bar graph
	TFStatType_t statTypeSecondary = GStatsSummaryPanel()->m_statBarGraph[0];
	StatDisplay_t statDisplaySecondary = GStatsSummaryPanel()->m_displayBarGraph[0];

	float flValPrimary0 = GetDisplayValue( (ClassStats_t &) *pStats0, statTypePrimary, statDisplayPrimary );
	float flValPrimary1 = GetDisplayValue( (ClassStats_t &) *pStats1, statTypePrimary, statDisplayPrimary );
	float flValSecondary0 = GetDisplayValue( (ClassStats_t &) *pStats0, statTypeSecondary, statDisplaySecondary );
	float flValSecondary1 = GetDisplayValue( (ClassStats_t &) *pStats1, statTypeSecondary, statDisplaySecondary );

	// sort in descending order by primary stat value
	if ( flValPrimary1 > flValPrimary0 )
		return 1;
	if ( flValPrimary1 < flValPrimary0 )
		return -1;

	// if primary stat values are equal, sort in descending order by secondary stat value
	if ( flValSecondary1 > flValSecondary0 )
		return 1;
	if ( flValSecondary1 < flValSecondary0 )
		return -1;

	// if primary & secondary stats are equal, sort by class for consistent sort order
	return ( pStats1->iPlayerClass - pStats0->iPlayerClass );
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );
	
	if ( m_pBarChartComboBoxA == pComboBox || m_pBarChartComboBoxB == pComboBox )
	{
		// a bar chart combo box changed, update the bar charts

		KeyValues *pUserDataA = m_pBarChartComboBoxA->GetActiveItemUserData();
		KeyValues *pUserDataB = m_pBarChartComboBoxB->GetActiveItemUserData();
		if ( !pUserDataA || !pUserDataB )
			return;
		m_statBarGraph[0] = (TFStatType_t) pUserDataA->GetInt( "stattype" );
		m_displayBarGraph[0] = (StatDisplay_t) pUserDataA->GetInt( "statdisplay" );
		m_statBarGraph[1] = (TFStatType_t) pUserDataB->GetInt( "stattype" );
		m_displayBarGraph[1] = (StatDisplay_t) pUserDataB->GetInt( "statdisplay" );
		UpdateBarCharts();
	}	
	else if ( m_pClassComboBox == pComboBox )
	{
		// the class selection combo box changed, update class details

		KeyValues *pUserData = m_pClassComboBox->GetActiveItemUserData();
		if ( !pUserData )
			return;

		m_iSelectedClass = pUserData->GetInt( "class", TF_CLASS_UNDEFINED );

		UpdateClassDetails();
	}

}

//-----------------------------------------------------------------------------
// Purpose: Command target handler called from reset stats confirmation query box
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::DoResetStats()
{
#ifndef _X360
	// reset the stats
	engine->ClientCmd( "resetplayerstats" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns the stat value for specified display type
//-----------------------------------------------------------------------------
float CTFStatsSummaryPanel::GetDisplayValue( ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay )
{
	switch ( statDisplay )
	{
	case SHOW_MAX:
		return stats.max.m_iStat[statType];
		break;
	case SHOW_TOTAL:
		return stats.accumulated.m_iStat[statType];
		break;
	case SHOW_AVG:
		return SafeCalcFraction( stats.accumulated.m_iStat[statType], stats.iNumberOfRounds );
		break;
	default:
		AssertOnce( false );
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the text representation of this value
//-----------------------------------------------------------------------------
const char *CTFStatsSummaryPanel::RenderValue( float flValue, TFStatType_t statType, StatDisplay_t statDisplay )
{
	static char szValue[64];
	if ( TFSTAT_PLAYTIME == statType )
	{
		// the playtime stat is shown in seconds
		return FormatSeconds( (int) flValue );
	}
	else if ( SHOW_AVG == statDisplay )
	{	
		// if it's an average, render as a float w/2 decimal places
		Q_snprintf( szValue, ARRAYSIZE( szValue ), "%.2f", flValue );
	}
	else
	{
		// otherwise, render as an integer
		Q_snprintf( szValue, ARRAYSIZE( szValue ), "%d", (int) flValue );
	}

	return szValue;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	// when we are changing levels and 
	if ( 0 == Q_strcmp( pEventName, "server_spawn" ) )
	{
		if ( !m_bInteractive )
		{
			const char *pMapName = event->GetString( "mapname" );
			if ( pMapName )
			{
				OnMapLoad( pMapName );
			}	
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are activated during level load
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnActivate()
{
	ClearMapLabel();

	m_bShowingLeaderboard = false;
	m_bLoadingCommunityMap = false;
	ShowMapInfo( false );

#ifdef _X360
	m_bShowBackButton = false;
#endif

	UpdateDialog();
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are deactivated at end of level load
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnDeactivate()
{
	ClearMapLabel();
}

CON_COMMAND( showstatsdlg, "Shows the player stats dialog" )
{
#ifdef _DEBUG
	GStatsSummaryPanel()->InvalidateLayout( false, true );
#endif
	GStatsSummaryPanel()->ShowModal();
#ifdef _DEBUG
	GStatsSummaryPanel()->OnMapLoad( "cp_coldfront" );
#endif
}
