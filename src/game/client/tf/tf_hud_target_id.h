//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_TARGET_ID_H
#define TF_HUD_TARGET_ID_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_spectatorgui.h"
#include "c_tf_player.h"
#include "IconPanel.h"

class CFloatingHealthIcon;
class CAvatarImagePanel;

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)
#define MAX_ID_STRING			256
#define MAX_PREPEND_STRING		32

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetID : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTargetID, vgui::EditablePanel );
public:
	CTargetID( const char *pElementName );
	
	virtual void	LevelShutdown( void );
	void			Reset( void );
	void			VidInit( void );
	virtual bool	ShouldDraw( void );
	virtual void	PerformLayout( void );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );

	void			UpdateID( void );

	virtual int		CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer );
	virtual wchar_t	*GetPrepend( void ) { return NULL; }

	int				GetTargetIndex( void ) { return m_iTargetEntIndex; }

	virtual int		GetRenderGroupPriority( void );

	virtual void	FireGameEvent( IGameEvent * event );

	virtual	bool	DrawHealthIcon();
	virtual	C_TFPlayer *GetTargetForSteamAvatar( C_TFPlayer *pTFPlayer );
private:

	bool IsValidIDTarget( int nEntIndex, float flOldTargetRetainFOV, float &flNewTargetRetainFOV );
	void UpdateFloatingHealthIconVisibility( bool bVisible );

protected:
	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	float			m_flLastChangeTime;
	float			m_flTargetRetainFOV;
	int				m_iTargetEntIndex;
	bool			m_bLayoutOnUpdate;

	vgui::Label				*m_pTargetNameLabel;
	vgui::Label				*m_pTargetDataLabel;
	CTFImagePanel			*m_pBGPanel;
	vgui::EditablePanel		*m_pMoveableSubPanel;
	CIconPanel				*m_pMoveableIcon;
	vgui::ImagePanel		*m_pMoveableSymbolIcon;
	vgui::Label				*m_pMoveableKeyLabel;
	CIconPanel				*m_pMoveableIconBG;
	CTFSpectatorGUIHealth	*m_pTargetHealth;
	vgui::ImagePanel		*m_pTargetAmmoIcon;
	vgui::ImagePanel		*m_pTargetKillStreakIcon;
	CAvatarImagePanel		*m_pAvatarImage;

	int				m_iRenderPriority;
	int				m_nOriginalY;
	Color			m_LabelColorDefault;

	bool			m_bArenaPanelVisible;

	CFloatingHealthIcon		*m_pFloatingHealthIcon;
	int						m_iLastScannedEntIndex;

	CPanelAnimationVarAliasType( int, m_iXOffset, "x_offset", "20", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iYOffset, "y_offset", "20", "proportional_int" );
};

class CMainTargetID : public CTargetID
{
	DECLARE_CLASS_SIMPLE( CMainTargetID, CTargetID );
public:
	CMainTargetID( const char *pElementName ) : CTargetID( pElementName ) {}

	virtual bool ShouldDraw( void );
};

class CSpectatorTargetID : public CTargetID
{
	DECLARE_CLASS_SIMPLE( CSpectatorTargetID, CTargetID );
public:
	CSpectatorTargetID( const char *pElementName ) : CTargetID( pElementName ) {}

	virtual bool ShouldDraw( void );
	virtual int	CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void PerformLayout( void );

	virtual	bool	DrawHealthIcon()	{ return true; }
private:
	vgui::Panel		*m_pBGPanel_Spec_Blue;
	vgui::Panel		*m_pBGPanel_Spec_Red;
																					
};

//-----------------------------------------------------------------------------
// Purpose: Second target ID that's used for displaying a second target below the primary
//-----------------------------------------------------------------------------
class CSecondaryTargetID : public CTargetID
{
	DECLARE_CLASS_SIMPLE( CSecondaryTargetID, CTargetID );
public:
	CSecondaryTargetID( const char *pElementName );

	virtual bool	ShouldDraw( void );
	virtual int		CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer );
	virtual wchar_t	*GetPrepend( void ) { return m_wszPrepend; }

	virtual	bool	DrawHealthIcon() { return true; }
private:
	wchar_t		m_wszPrepend[ MAX_PREPEND_STRING ];

	bool m_bWasHidingLowerElements;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CFloatingHealthIcon : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CFloatingHealthIcon, vgui::EditablePanel );
public:
	CFloatingHealthIcon( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	virtual void Paint( void );
	virtual void PostChildPaint( void );
	virtual bool IsVisible( void );

	virtual void SetVisible( bool state );

	void		Reset( void );
	void		SetEntity( C_BaseEntity *pEntity );
	C_BaseEntity *GetEntity( void ) { return m_hEntity; }
	void		SetTeamStatus( bool bState ) { m_bTeamStatus = bState; }
	void		SetShowAmmo( bool bState ) { m_bShowAmmo = bState; }

	static CFloatingHealthIcon* AddFloatingHealthIcon( C_BaseEntity *pEntity );

	bool		CalculatePosition();

private:
	bool		GetCrossBounds( int &x, int &y, int &w, int &h );
	void		PaintAmmoCorners( void );

	CTFSpectatorGUIHealth	*m_pTargetHealth;
	CHandle< C_BaseEntity >	m_hEntity;
	float					m_flPrevHealth;
	int						m_nPrevMaxBuffed;
	int						m_nPrevLevel;
	bool					m_bTeamStatus;
	bool					m_bShowAmmo;
	bool					m_bPositionValid;
	int						m_iAnchorX;
	int						m_iAnchorY;
	int						m_iHealthClipW;
	int						m_iHealthClipH;
	vgui::HFont				m_hAmmoFont;
	Color					m_clrAmmo;
};

//-----------------------------------------------------------------------------
// Purpose: One health cross per teammate when the class can see them without aiming.
//-----------------------------------------------------------------------------
class CTeamStatusIcons : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTeamStatusIcons, vgui::EditablePanel );
public:
	CTeamStatusIcons( const char *pElementName );
	virtual ~CTeamStatusIcons();

	virtual void	LevelShutdown( void );
	virtual bool	ShouldDraw( void ) { return false; }
	virtual void	OnTick( void );

	bool			Covers( C_BaseEntity *pEnt ) const;
	static bool		CoversEntity( C_BaseEntity *pEnt );

private:
	void			DestroyIcons( void );

	CFloatingHealthIcon	*m_pIcons[MAX_PLAYERS_ARRAY_SAFE];
};

#endif // TF_HUD_TARGET_ID_H
