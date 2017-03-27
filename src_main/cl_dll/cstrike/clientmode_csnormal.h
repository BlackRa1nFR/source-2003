//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef CS_CLIENTMODE_H
#define CS_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui/Cursor.h>
#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include "counterstrikeviewport.h"

class CBuyMenu;


class ClientModeCSNormal : public ClientModeShared
{
DECLARE_CLASS( ClientModeCSNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeCSNormal();
	virtual			~ClientModeCSNormal();

	virtual void	Init( void );

	virtual void	Enable();
	virtual void	Disable();
	
	virtual void	Update();
	virtual void	Layout();
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual bool	ShouldDrawViewModel( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height );
	virtual void	PreRender( CViewSetup *pSetup );
	virtual void	PostRenderWorld();
	virtual void	PostRender( void );
	virtual int		KeyInput( int down, int keynum, const char *pszCurrentBinding );

	virtual vgui::AnimationController *GetViewportAnimationController();

	virtual vgui::Panel* GetViewport() { return &m_Viewport; }
	virtual vgui::Panel* GetMinimapParent( void );

public:
	
	void			ReloadScheme( void );


public:

	CBuyMenu *		m_pBuyMenu;

private:

	vgui::HCursor			m_CursorNone;
	CounterStrikeViewport	m_Viewport;

};


extern IClientMode *GetClientModeNormal();
extern ClientModeCSNormal* GetClientModeCSNormal();


#endif // CS_CLIENTMODE_H
