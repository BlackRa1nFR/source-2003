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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( VGUI_BASEPANEL_H )
#define VGUI_BASEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// Purpose: Base Panel for engine vgui panels ( can handle some drawing stuff )
//-----------------------------------------------------------------------------
class CBasePanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
					CBasePanel( vgui::Panel *parent, char const *panelName );
	virtual			~CBasePanel( void );

	// Draws text with current font at position using color values specified
	virtual int		DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, ... );

	// Determine how long in pixesl the text would occupy using the specified font
	virtual int		DrawTextLen( vgui::HFont font, char *fmt, ... );

	// should this panel be drawn this frame?
	virtual bool	ShouldDraw( void ) = 0;
	virtual void	OnTick( void );

protected:
	// Var args version...
	int DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, va_list args );
};

#endif // VGUI_BASEPANEL_H