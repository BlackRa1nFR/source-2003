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
#if !defined( CLIENTMODE_HLNORMAL_H )
#define CLIENTMODE_HLNORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeHLNormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeHLNormal, ClientModeShared );

	ClientModeHLNormal();
	~ClientModeHLNormal();

// ClientModeNormal overrides.
public:
	
	virtual void	Init( void );
	virtual void	Enable();
	virtual void	Disable();
	virtual void	Layout();
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual vgui::Panel *GetViewport();
	virtual vgui::AnimationController *GetViewportAnimationController();
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	Update();

private:
	CHudViewport *m_pViewport;
	vgui::HCursor m_hCursorNone;
};

extern IClientMode *GetClientModeNormal();

#endif // CLIENTMODE_HLNORMAL_H
