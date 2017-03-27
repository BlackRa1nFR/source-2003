//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef ICLIENTMODE_H
#define ICLIENTMODE_H

#include <vgui/VGUI.h>

class CViewSetup;
class C_BaseEntity;
class C_BasePlayer;
class CUserCmd;

namespace vgui
{
	class Panel;
	class AnimationController;
}

// Message mode types
enum
{
	MM_NONE = 0,
	MM_SAY,
	MM_SAY_TEAM,
};

class IClientMode
{
// Misc.
public:

	virtual			~IClientMode() {}

	// One time init when .dll is first loaded.
	virtual void	Init()=0;

	// One time call when dll is shutting down
	virtual void	Shutdown()=0;

	// Called when switching from one IClientMode to another.
	// This can re-layout the view and such.
	// Note that Enable and Disable are called when the DLL initializes and shuts down.
	virtual void	Enable()=0;

	// Called when it's about to go into another client mode.
	virtual void	Disable()=0;

	// Called when initializing or when the view changes.
	// This should move the viewport into the correct position.
	virtual void	Layout()=0;

	// Gets at the viewport, if there is one...
	virtual vgui::Panel *GetViewport() = 0;

	// Gets at the viewports vgui panel animation controller, if there is one...
	virtual vgui::AnimationController *GetViewportAnimationController() = 0;

	// called every time shared client dll/engine data gets changed,
	// and gives the cdll a chance to modify the data.
	virtual void	ProcessInput( bool bActive ) = 0;

	// The mode can choose to draw/not draw entities.
	virtual bool	ShouldDrawDetailObjects( ) = 0;
	virtual bool	ShouldDrawEntity(C_BaseEntity *pEnt) = 0;
	virtual bool	ShouldDrawLocalPlayer( C_BasePlayer *pPlayer ) = 0;
	virtual bool	ShouldDrawParticles( ) = 0;

	// The mode can choose to not draw fog
	virtual bool	ShouldDrawFog( void ) = 0;

	virtual void	OverrideView( CViewSetup *pSetup ) = 0;
	virtual int		KeyInput( int down, int keynum, const char *pszCurrentBinding ) = 0;
	virtual void	StartMessageMode( int iMessageModeType ) = 0;
	virtual vgui::Panel *GetMessagePanel() = 0;
	virtual void	OverrideMouseInput( int *x, int *y ) = 0;
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd ) = 0;

	virtual void	LevelInit( const char *newmap ) = 0;
	virtual void	LevelShutdown( void ) = 0;

	// Certain modes hide the view model
	virtual bool	ShouldDrawViewModel( void ) = 0;
	virtual bool	ShouldDrawCrosshair( void ) = 0;

	// Let mode override viewport for engine
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height ) = 0;

	// Called before rendering a view.
	virtual void	PreRender( CViewSetup *pSetup ) = 0;

	// Called after the world is rendered but before the hud is rendered.
	virtual void	PostRenderWorld() = 0;
	
	// Called after everything is rendered.
	virtual void	PostRender( void ) = 0;

	virtual void	ActivateInGameVGuiContext( vgui::Panel *pPanel ) = 0;
	virtual void	DeactivateInGameVGuiContext() = 0;

// Updates.
public:

	// Called every frame.
	virtual void	Update()=0;	
};	

extern IClientMode *g_pClientMode;

#endif
