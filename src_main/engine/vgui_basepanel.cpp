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
#include "quakedef.h"
#include "vgui_int.h"
#include <vgui_controls/Panel.h>
#include "vgui_BasePanel.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui/IPanel.h>
#include <vgui/ILocalize.h>
#include "EngineStats.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePanel::CBasePanel( vgui::Panel *parent, char const *panelName )
: vgui::Panel( parent, panelName )
{
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePanel::~CBasePanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Draws colored text to a vgui panel
// Input  : *font - font to use
//			x - position of text
//			y - 
//			r - color of text
//			g - 
//			b - 
//			a - alpha ( 0 = opaque, 255 = transparent )
//			*fmt - va_* text string
//			... - 
// Output : int - horizontal # of pixels drawn
//-----------------------------------------------------------------------------

int CBasePanel::DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, va_list argptr )
{
	static int count = 0;

	int len;
	char data[ 1024 ];

	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawSetTextColor( r, g, b, a );

#ifdef _WIN32
	len = _vsnprintf(data, 1024, fmt, argptr);
#else
	len = vsprintf(data, fmt, argptr);
#endif

	if ( len == -1 )
		return x;

	vgui::surface()->DrawSetTextFont( font );

	wchar_t unicodeString[1024];
	vgui::localize()->ConvertANSIToUnicode(data, unicodeString, sizeof(unicodeString)  );

	int pixels = DrawTextLen( font, data );

	vgui::surface()->DrawPrintText( unicodeString, len );

	return x + pixels;
}

int CBasePanel::DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, ... )
{
	va_list argptr;
	va_start( argptr, fmt );
	int ret = DrawColoredText( font, x, y, r, g, b, a, fmt, argptr );
	va_end(argptr);
	return ret;
}


//-----------------------------------------------------------------------------
// Purpose: Determine length of text string
// Input  : *font - 
//			*fmt - 
//			... - 
// Output : 
//-----------------------------------------------------------------------------
int	CBasePanel::DrawTextLen( vgui::HFont font, char *fmt, ... )
{
	va_list argptr;
	char data[ 1024 ];
	int len;

	va_start(argptr, fmt);
#ifdef _WIN32
	len = _vsnprintf(data, 1024, fmt, argptr);
#else
	len = vsprintf(data, fmt, argptr);
#endif
	va_end(argptr);

	int i;
	int x = 0;

	vgui::surface()->DrawSetTextFont( font );

	for ( i = 0 ; i < len; i++ )
	{
		int a, b, c;
		vgui::surface()->GetCharABCwide( font, data[i], a, b, c );
		// Ignore a
		if ( i != 0 )
			x += a;
		x += b;
		if ( i != len - 1 )
			x += c;
	}

	return x;
}

void CBasePanel::OnTick()
{
	SetVisible( ShouldDraw() );
}

