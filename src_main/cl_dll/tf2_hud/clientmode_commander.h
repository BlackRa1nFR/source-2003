//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#if !defined( CLIENTMODE_COMMANDER_H )
#define CLIENTMODE_COMMANDER_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_tfbase.h"
#include <vgui/Cursor.h>
#include "hud_minimap.h"
#include <vgui_controls/Panel.h>

class IMaterial;
class CCommanderViewportPanel;
class Vector;
class CCommanderOverlayPanel;
namespace vgui
{
	class Panel;
	class AnimationController;
}

class CClientModeCommander : public ClientModeTFBase, public IMinimapClient
{
	DECLARE_CLASS( CClientModeCommander, ClientModeTFBase );

// IClientMode overrides.
public:
	
					CClientModeCommander();
	virtual			~CClientModeCommander();

	virtual void	Init( void );

	virtual void	Enable();
	virtual void	Disable();
	
	virtual void	Update();
	virtual void	Layout();

	virtual bool	ShouldDrawFog( void );
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual bool	ShouldDrawEntity(C_BaseEntity *pEnt);
	virtual bool	ShouldDrawDetailObjects( );
	virtual bool	ShouldDrawLocalPlayer( C_BasePlayer *pPlayer );
	virtual bool	ShouldDrawViewModel( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual bool	ShouldDrawParticles( );

	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height );
	virtual void	PreRender( CViewSetup *pSetup );
	virtual void	PostRenderWorld();
	virtual void	PostRender( void );
	virtual vgui::Panel		*GetViewport();
	virtual vgui::AnimationController *GetViewportAnimationController() { return NULL; }

	// Input
	virtual int		KeyInput( int down, int keynum, const char *pszCurrentBinding );

	// Makes the mouse sit over a particular world location
	void	MoveMouse( Vector& worldPos );

	// Inherited from IMinimapClient
	virtual void MinimapClicked( const Vector& clickWorldPos );

	CCommanderOverlayPanel	*GetCommanderOverlayPanel( void );

	virtual vgui::Panel *GetMinimapParent( void );

private:
	float	GetScaledSlueSpeed( void );
	void	ResetCommand( CUserCmd *cmd );
	float	Commander_ResampleMove( float in );
	void	IsometricMove( CUserCmd *cmd );
	float	GetHeightForMap( float zoom );

	// Fills in ortho parameters (and near/far Z) in pSetup for how the commander mode renders the world.
	bool			GetOrthoParameters(CViewSetup *pSetup);

private:
	CCommanderViewportPanel*	m_pViewport;

	ConVar*	m_pClear;
	ConVar*	m_pSkyBox;
	float	m_fOldClear;
	float	m_fOldSkybox;

	int		m_LastMouseX;
	int		m_LastMouseY;

	float	m_ScaledMouseSpeed;
	float	m_ScaledSlueSpeed;
	float	m_Log_BaseEto2;	// scales logarithms from base E to base 2.
};


//-----------------------------------------------------------------------------
// The panel responsible for rendering the 3D view in orthographic mode
//-----------------------------------------------------------------------------
class CCommanderViewportPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

// Panel overrides.
public:
					CCommanderViewportPanel( void );
	virtual			~CCommanderViewportPanel( void );

	void			Enable( void );
	void			Disable( void );
	void			SetCommanderView( CClientModeCommander *commander );
	void			MinimapClicked( const Vector& clickWorldPos );

	CCommanderOverlayPanel	*GetCommanderOverlayPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );
	}

private:

	CCommanderOverlayPanel	*m_pOverlayPanel;
	CClientModeCommander	*m_pCommanderView;

	vgui::HCursor	m_CursorCommander;
	vgui::HCursor	m_CursorRightMouseMove;

};

IClientMode *ClientModeCommander();

#endif // CLIENTMODE_COMMANDER_H