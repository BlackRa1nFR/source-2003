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
#include "cbase.h"
#include <stdarg.h>
#include "vguicenterprint.h"
#include "ivrenderview.h"
#include <vgui/IVgui.h>
#include "VguiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar		scr_printspeed( "scr_printspeed","8" );
static ConVar		scr_centertime( "scr_centertime","2" );

//-----------------------------------------------------------------------------
// Purpose: Implements Center String printing
//-----------------------------------------------------------------------------
class CCenterStringPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
						CCenterStringPanel( vgui::VPANEL parent );
	virtual				~CCenterStringPanel( void );

	// vgui::Panel
	virtual void		ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void		Paint();
	virtual void		OnTick( void );
	virtual bool		ShouldDraw( void );

	// CVGuiCenterPrint
	virtual void		SetTextColor( int r, int g, int b, int a );
	virtual void		Print( char *fmt, ... );
	virtual void		ColorPrint( int r, int g, int b, int a, char *fmt, ... );
	virtual void		Clear( void );

private:
	vgui::HFont			m_hFont;

	char				scr_centerstring[1024];
	float				scr_centertime_start;	// for slow printing
	float				scr_centertime_off;
	int					scr_center_lines;
	int					scr_erase_lines;
	int					scr_erase_center;
	int					red, green, blue, alpha;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CCenterStringPanel::CCenterStringPanel( vgui::VPANEL parent ) : 
	BaseClass( NULL, "CCenterStringPanel" )
{
	SetParent( parent );
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	m_hFont = 0;
	SetFgColor( Color( 0, 0, 0, 0 ) );

	SetPaintBackgroundEnabled( false );

	scr_centerstring[0]=0;
	scr_centertime_start = 0.0;
	scr_centertime_off = 0.0;
	scr_center_lines = 0;
	scr_erase_lines = 0;
	scr_erase_center = 0;
	red = green = blue = 255;
	alpha = 255;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCenterStringPanel::~CCenterStringPanel( void )
{
}

void CCenterStringPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Use a large font
	m_hFont = pScheme->GetFont( "Trebuchet24" );
	assert( m_hFont );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CCenterStringPanel::Paint() 
{
	char	*start;
	int		l;
	int		x, y;
	int		remaining;

	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = ScreenHeight() * 0.35;
	else
		y = 48;

	do	
	{
		// scan the width of the line
		char line[ 40 ];
		Q_strncpy( line, start, 40 );
		l = 0;
		while ( line[l] && line[l] != '\n' )
			l++;
		line[ l ] = 0;

		l = g_pMatSystemSurface->DrawTextLen( m_hFont, line );
		
		x = ( ScreenWidth() - l ) / 2;
		
		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, red, green, blue, alpha, line );

		remaining -= strlen( line );

		y += vgui::surface()->GetFontTall( m_hFont );

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : r - 
//			g - 
//			b - 
//			a - 
//-----------------------------------------------------------------------------
void CCenterStringPanel::SetTextColor( int r, int g, int b, int a )
{
	red = r; green = g; blue = b; alpha = a;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CCenterStringPanel::Print( char *fmt, ... )
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

	Q_strncpy( scr_centerstring, data, sizeof(scr_centerstring)-1 );
	
	scr_centertime_off   = scr_centertime.GetFloat();
	scr_centertime_start = gpGlobals->curtime;

	// Count the number of lines for centering
	scr_center_lines = 1;
	
	char *str = scr_centerstring;
	while (*str)
	{
		if (*str == '\n')
		{
			scr_center_lines++;
		}
		str++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : r - 
//			g - 
//			b - 
//			a - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CCenterStringPanel::ColorPrint( int r, int g, int b, int a, char *fmt, ... )
{
	SetTextColor( r, g, b, a );
	va_list argptr;
	va_start( argptr, fmt );
	Print( fmt, argptr );
	va_end(argptr);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringPanel::Clear( void )
{
	scr_centerstring[ 0 ] = 0;
	scr_centertime_off = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringPanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
// FIXME, this has dependencies on the engine that should go away
//-----------------------------------------------------------------------------
bool CCenterStringPanel::ShouldDraw( void )
{
	if ( engine->IsDrawingLoadingImage() )
	{
		return false;
	}

	if ( scr_center_lines > scr_erase_lines )
	{
		scr_erase_lines = scr_center_lines;
	}

	float flTimeRemaining = scr_centertime_start + scr_centertime_off - gpGlobals->curtime;
	if ( flTimeRemaining <= 0 )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CCenterPrint::CCenterPrint( void )
{
	vguiCenterString = NULL;
}

void CCenterPrint::SetTextColor( int r, int g, int b, int a )
{
	if ( !vguiCenterString )
		return;
	vguiCenterString->SetTextColor( r, g, b, a );
}

void CCenterPrint::Print( char *fmt, ... )
{
	if ( !vguiCenterString )
		return;
	va_list argptr;
	va_start( argptr, fmt );
	vguiCenterString->Print( fmt, argptr );
	va_end(argptr);
}

void CCenterPrint::ColorPrint( int r, int g, int b, int a, char *fmt, ... )
{
	if ( !vguiCenterString )
		return;
	vguiCenterString->SetTextColor( r, g, b, a );
	va_list argptr;
	va_start( argptr, fmt );
	vguiCenterString->Print( fmt, argptr );
	va_end(argptr);
}

void CCenterPrint::Clear( void )
{
	if ( !vguiCenterString )
		return;
	vguiCenterString->Clear();
}

void CCenterPrint::Create( vgui::VPANEL parent )
{
	vguiCenterString = new CCenterStringPanel( parent );
}

void CCenterPrint::Destroy( void )
{
	if ( vguiCenterString )
	{
		vguiCenterString->SetParent( (vgui::Panel *)NULL );
		delete vguiCenterString;
	}
}

static CCenterPrint g_CenterString;
CCenterPrint *internalCenterPrint = &g_CenterString;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CCenterPrint, ICenterPrint, VCENTERPRINT_INTERFACE_VERSION, g_CenterString );