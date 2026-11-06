//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: PDA Weapon
//
//=============================================================================

#ifndef TF_WEAPON_PDA_H
#define TF_WEAPON_PDA_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#ifdef CLIENT_DLL
	#include "tf_hud_base_build_menu.h"
	#include <vgui_controls/EditablePanel.h>
	#include <vgui_controls/Label.h>
	#include <vgui_controls/ImagePanel.h>
	#include <game_controls/IconPanel.h>
	#include <vgui/tf_controls.h>
#endif

// Client specific.
#if defined( CLIENT_DLL ) 
	#define CTFWeaponPDA C_TFWeaponPDA
	#define CTFWeaponPDA_Engineer_Build	C_TFWeaponPDA_Engineer_Build
	#define CTFWeaponPDA_Engineer_Destroy	C_TFWeaponPDA_Engineer_Destroy
	#define CTFWeaponPDA_Spy		C_TFWeaponPDA_Spy
	#define CTFWeaponPDA_Spy_Build	C_TFWeaponPDA_Spy_Build
	#define CTFWeaponPDA_Spellbook	C_TFWeaponPDA_Spellbook
	
	#define CTFWeaponPDAExpansion_Dispenser		C_TFWeaponPDAExpansion_Dispenser
	#define CTFWeaponPDAExpansion_Teleporter	C_TFWeaponPDAExpansion_Teleporter
#endif

class CTFWeaponPDA : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFWeaponPDA, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponPDA();

	virtual void	Spawn();

#if !defined( CLIENT_DLL )
		virtual void	Precache();
		virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
#else
		virtual float	CalcViewmodelBob( void );
		virtual CHudBaseBuildMenu *GetBuildMenu() const { return NULL; }
#endif

	virtual bool	ShouldShowControlPanels( void );

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual int		GetWeaponID( void ) const						{ return TF_WEAPON_PDA; }
	virtual bool	ShouldDrawCrosshair( void )						{ return false; }
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }
#ifdef CLIENT_DLL
	virtual void	OnDataChanged( DataUpdateType_t type ) OVERRIDE;
	virtual void	UpdateOnRemove() OVERRIDE;
#endif

	virtual const char *GetPanelName() { return "pda_panel"; }

	virtual bool	CanInspect() const OVERRIDE { return false; }


public:	
	CTFWeaponInfo	*m_pWeaponInfo;

private:
#ifdef CLIENT_DLL
	void HideBuildMenu() const;
#endif

	CTFWeaponPDA( const CTFWeaponPDA & ) {}
};

class CTFWeaponPDA_Engineer_Build : public CTFWeaponPDA
{
public:
	DECLARE_CLASS( CTFWeaponPDA_Engineer_Build, CTFWeaponPDA );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual const char *GetPanelName() { return ""; }
	virtual int		GetWeaponID( void ) const { return TF_WEAPON_PDA_ENGINEER_BUILD; }
#ifdef CLIENT_DLL
	virtual CHudBaseBuildMenu *GetBuildMenu() const OVERRIDE;
#endif
};

#ifdef CLIENT_DLL

extern ConVar tf_build_menu_controller_mode;

#endif
class CTFWeaponPDA_Engineer_Destroy : public CTFWeaponPDA
{
public:
	DECLARE_CLASS( CTFWeaponPDA_Engineer_Destroy, CTFWeaponPDA );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual const char *GetPanelName() { return ""; }
	virtual int		GetWeaponID( void ) const { return TF_WEAPON_PDA_ENGINEER_DESTROY; }
#ifdef CLIENT_DLL
	virtual CHudBaseBuildMenu *GetBuildMenu() const OVERRIDE;
#endif

	virtual bool	VisibleInWeaponSelection( void );
};

class CTFWeaponPDA_Spy : public CTFWeaponPDA
{
public:
	DECLARE_CLASS( CTFWeaponPDA_Spy, CTFWeaponPDA );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual const char *GetPanelName() { return ""; }
	virtual int		GetWeaponID( void ) const { return TF_WEAPON_PDA_SPY; }
#ifdef CLIENT_DLL
	virtual CHudBaseBuildMenu *GetBuildMenu() const OVERRIDE;
	virtual bool Deploy( void );
#endif

	virtual bool			CanBeSelected( void ) OVERRIDE;
	virtual bool			VisibleInWeaponSelection( void ) OVERRIDE;

	virtual void			ItemPreFrame( void );					// called each frame by the player PreThink
	virtual void			ItemBusyFrame( void );					// called each frame by the player PostThink
	virtual void			ItemHolsterFrame( void );			// called each frame by the player PreThink, if the weapon is holstered

	void	CheckDisguiseTimer( void );
	void	ProcessDisguiseImpulse( void );
};

// Witch spellbook. Its own PDA, so it is not the Spy disguise kit.
class CTFWeaponPDA_Spellbook : public CTFWeaponPDA
{
public:
	DECLARE_CLASS( CTFWeaponPDA_Spellbook, CTFWeaponPDA );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual const char *GetPanelName() { return ""; }
	virtual int		GetWeaponID( void ) const { return TF_WEAPON_PDA_SPELLBOOK; }

#ifdef CLIENT_DLL
	virtual CHudBaseBuildMenu *GetBuildMenu() const OVERRIDE;
	virtual bool Deploy( void );
#endif
};

#ifdef CLIENT_DLL

using namespace vgui;

//-----------------------------------------------------------------------------
// Spell picker. Same keys as the Spy disguise menu, spell icons instead of classes.
//-----------------------------------------------------------------------------
class CHudMenuSpellbook : public CHudBaseBuildMenu
{
	DECLARE_CLASS_SIMPLE( CHudMenuSpellbook, EditablePanel );

public:
	CHudMenuSpellbook( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	SetVisible( bool state );

	int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority( void ) { return 50; }
	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_IN_GAME_HUD : GAME_ACTION_SET_NONE; }

private:
	void SetSelectedItem( int iSlot );
	void UpdateSpellItems( void );
	void ToggleSelectionIcons( bool bGroup );
	void FindToggleBinding( void );
	void ChooseSlot( int iSlot );
	void PlaceOnScreen( void );

	EditablePanel *m_pClassItems_Red[9];
	CIconPanel *m_pKeyIcons_Red[9];
	CExLabel *m_pKeyLabels_Red[9];
	CExLabel *m_pKeyLabelsNew_Red[9];
	ImagePanel *m_pSpellIcons_Red[9];
	CExLabel *m_pCostLabels_Red[9];

	EditablePanel *m_pClassItems_Blue[9];
	CIconPanel *m_pKeyIcons_Blue[9];
	CExLabel *m_pKeyLabels_Blue[9];
	CExLabel *m_pKeyLabelsNew_Blue[9];
	ImagePanel *m_pSpellIcons_Blue[9];
	CExLabel *m_pCostLabels_Blue[9];

	CIconPanel *m_pKeyIcons_Category[3];
	CExLabel *m_pKeyLabels_Category[3];

	EditablePanel *m_pActiveSelection;

	int m_iShowingTeam;
	int m_iSelectedItem;
	int m_iGroupSelection;
	bool m_bInConsoleMode;
};

#endif // CLIENT_DLL

// ********************************************************************************************
// PDA Expansion Slots
class CTFWeaponPDAExpansion_Dispenser : public CTFWearable
{
	DECLARE_CLASS( CTFWeaponPDAExpansion_Dispenser, CTFWearable );

public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	virtual void		Equip( CBasePlayer *pOwner );
	virtual void		UnEquip( CBasePlayer *pOwner );
};

class CTFWeaponPDAExpansion_Teleporter : public CTFWearable
{
	DECLARE_CLASS( CTFWeaponPDAExpansion_Teleporter, CTFWearable );

public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	virtual void		Equip( CBasePlayer *pOwner );
	virtual void		UnEquip( CBasePlayer *pOwner );
};


#endif // TF_WEAPON_PDA_H
