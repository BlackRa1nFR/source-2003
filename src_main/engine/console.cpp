// console.c

#include "quakedef.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdarg.h>
#include <fcntl.h>
#include "bspfile.h"
#ifndef SWDS
#include "io.h"
#include "keys.h"
#include "draw.h"
#endif
#include "console.h"
#include "screen.h"
#include "vid.h"
#include "zone.h"
#include "sys.h"
#include "server.h"
#include "game_interface.h"
#include "con_nprint.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#ifndef SWDS
#include "vgui_int.h"
#include "vgui_BasePanel.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Controls.h>
#include <vgui/ILocalize.h>
#endif
#include "demo.h"
#include "tier0/vprof.h"
#include "proto_version.h"
#include "vstdlib/ICommandLine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

qboolean	con_debuglog;
qboolean	con_initialized;

#ifndef SWDS
extern  bool		scr_drawloading;

ConVar		con_trace( "con_trace", "0" );
ConVar		con_notifytime( "con_notifytime","8", 0, "How long to display recent console text to the upper part of the game window" );

static ConVar con_times("contimes", "6", 0, "Number of console lines to overlay for debugging." );

bool con_refreshonprint = true;

//-----------------------------------------------------------------------------
// Purpose: Implements the console using VGUI
//-----------------------------------------------------------------------------
class CConPanel : public CBasePanel
{
public:
	enum
	{
		MAX_NOTIFY_TEXT_LINE = 256
	};

					CConPanel( vgui::Panel *parent );
	virtual			~CConPanel( void );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

	// Draws the text
	virtual void	Paint();
	// Draws the background image
	virtual void	PaintBackground();

	// Draw notify area
	virtual void	DrawNotify( void );
	// Draws debug ( Con_NXPrintf ) areas
	virtual void	DrawDebugAreas( void );
	
	int				ProcessNotifyLines( int &left, int &top, int &right, int &bottom, bool bDraw );

	// Draw helpers
	virtual int		DrawText( vgui::HFont font, int x, int y, char *fmt, ... );

	virtual bool	ShouldDraw( void );

	void			Con_NPrintf( int idx, char *msg );
	void			Con_NXPrintf( struct con_nprint_s *info, char *msg );

	void			AddToNotify( Color& clr, char const *msg );
	void			ClearNofify();

private:
	// Console font
	vgui::HFont		m_hFont;
	vgui::HFont		m_hFontFixed;

	struct CNotifyText
	{
		Color	clr;
		float		liferemaining;
		char		text[ MAX_NOTIFY_TEXT_LINE ];
	};

	CUtlVector< CNotifyText >	m_NotifyText;

	enum
	{
		MAX_DBG_NOTIFY = 128,
		DBG_NOTIFY_TIMEOUT = 4,
	};

	float da_default_color[3];

	typedef struct
	{
		char	szNotify[ MAX_NOTIFY_TEXT_LINE ];
		float	expire;
		float	color[3];
		bool	fixed_width_font;
	} da_notify_t;

	da_notify_t da_notify[ MAX_DBG_NOTIFY ];
	bool m_bDrawDebugAreas;
};

static CConPanel *g_pConPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Con_ToggleConsole_f
//-----------------------------------------------------------------------------
void Con_ToggleConsole_f (void)
{
	if (VGui_IsConsoleVisible())
	{
		// hide everything
		VGui_HideConsole();
	}
	else
	{
		// activate the console
		VGui_ShowConsole();
	}	

	SCR_EndLoadingPlaque ();
}

/*
================
Con_HideConsole_f

================
*/
void Con_HideConsole_f ( void )
{
	if (VGui_IsConsoleVisible())
	{
		// hide everything
		VGui_HideConsole();
	}
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	VGui_ClearConsole();
	Con_ClearNotify();
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	if ( g_pConPanel )
	{
		g_pConPanel->ClearNofify();
	}
}
#endif
												
/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con_debuglog = CommandLine()->FindParm( "-condebug" ) != 0;
	if ( con_debuglog && CommandLine()->FindParm( "-conclearlog" ) )
	{
		g_pFileSystem->RemoveFile( "console.log", "GAME" );
	}

	con_initialized = true;
}

/*
================
Con_Shutdown
================
*/
void Con_Shutdown (void)
{
	con_initialized = false;
}

/*
================
Con_DebugLog
================
*/
void Con_DebugLog(const char *file, const char *fmt, ...)
{
    va_list argptr; 
    static char data[1024];
    
    va_start(argptr, fmt);
    Q_vsnprintf(data, 1024, fmt, argptr);
    va_end(argptr);

	FileHandle_t fh = g_pFileSystem->Open( file, "a" );
	if (fh != FILESYSTEM_INVALID_HANDLE )
	{
		g_pFileSystem->Write( data, strlen(data), fh );
		g_pFileSystem->Close( fh );
	}
}

#define	MAXPRINTMSG	4096
static bool g_fIsDebugPrint = false;

#ifndef SWDS
/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
static bool g_fColorPrintf = false;
// FIXME: make a buffer size safe vsprintf?
void Con_ColorPrint( Color& clr, char const *msg )
{
	// also echo to debugging console
	if(!con_trace.GetInt())
	{
		Sys_Printf ("%s", msg);	// also echo to debugging console
	}

	// Add to redirected message
	if ( SV_RedirectActive() )
	{
		SV_RedirectAddText( msg );
		return;
	}

	// log all messages to file
	if (con_debuglog)
		Con_DebugLog("console.log", "%s", msg);

	if (!con_initialized)
	{
		return;
	}
		
	if (cls.state == ca_dedicated)
	{
		return;		// no graphics mode
	}

	bool convisible = Con_IsVisible();
	bool indeveloper = ( developer.GetInt() > 0 );
	bool debugprint = g_fIsDebugPrint;

	if ( g_fColorPrintf )
	{
		VGui_ColorPrintf( clr, msg );
	}
	else
	{
		// write it out to the vgui console no matter what
		if ( g_fIsDebugPrint )
		{
			// Don't spew debug stuff to actual console once in game, unless console isn't up
			if ( cls.state != ca_active || 
				!convisible )
			{
				VGui_ConDPrintf( msg );
			}
		}
		else
		{
			VGui_ConPrintf( msg );
		}
	}

	// Only write to notify if it's non-debug or we are running with developer set > 0
	// Buf it it's debug then make sure we don't have the console down
	if ( ( !debugprint || indeveloper ) && 
		!( debugprint && convisible ) )
	{
		if ( g_pConPanel )
		{
			g_pConPanel->AddToNotify( clr, msg );
		}
	}
}
#endif

void Con_Print (const char *msg)
{
	if ( !msg || !msg[0] )
		return;

#ifdef SWDS
	Sys_Printf( "%s", msg );
#else
	Color clr(255, 255, 255, 255 );
	Con_ColorPrint( clr, msg );
#endif
}

void Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	Q_vsnprintf (msg,sizeof( msg ), fmt,argptr);
	va_end (argptr);

#ifdef SWDS
	Sys_Printf( "%s", msg );
#else
	Color clr(255, 255, 255, 255 );
	Con_ColorPrint( clr, msg );
#endif
}

#ifndef SWDS
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : clr - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void Con_ColorPrintf( Color& clr, const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	Q_vsnprintf (msg,sizeof( msg ), fmt,argptr);
	va_end (argptr);

	g_fColorPrintf = true;
	Con_ColorPrint( clr, msg );
	g_fColorPrintf = false;
}
#endif

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf(msg,sizeof( msg ), fmt,argptr);
	va_end (argptr);
	
	g_fIsDebugPrint = true;

#ifdef SWDS
	Sys_Printf( "%s", msg );
#else
	Color clr( 196, 181, 80, 255 );
	Con_ColorPrint ( clr, msg );
#endif

	g_fIsDebugPrint = false;
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];
		
	va_start (argptr,fmt);
	Q_vsnprintf(msg,sizeof( msg ), fmt,argptr);
	va_end (argptr);

#ifndef SWDS
	int			temp;
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;
#endif
	g_fIsDebugPrint = true;
	Con_Printf ("%s", msg);
	g_fIsDebugPrint = false;
#ifndef SWDS
	scr_disabled_for_loading = temp;
#endif
}

#ifndef SWDS
bool Con_IsVisible()
{
	return (VGui_IsConsoleVisible());	
}



void Con_NPrintf( int idx, char *fmt, ... )
{
	va_list argptr; 
	char outtext[ 4096 ];

	va_start(argptr, fmt);
    Q_vsnprintf( outtext, sizeof( outtext ), fmt, argptr);
    va_end(argptr);

	g_pConPanel->Con_NPrintf( idx, outtext );

}

void Con_NXPrintf( struct con_nprint_s *info, char *fmt, ... )
{
	va_list argptr; 
	char outtext[ 4096 ];

	va_start(argptr, fmt);
    Q_vsnprintf( outtext, sizeof( outtext ), fmt, argptr);
    va_end(argptr);

	g_pConPanel->Con_NXPrintf( info, outtext );
}

//-----------------------------------------------------------------------------
// Purpose: Creates the console panel
// Input  : *parent - 
//-----------------------------------------------------------------------------
CConPanel::CConPanel( vgui::Panel *parent )
: CBasePanel( parent, "CConPanel" )
{
	// Full screen assumed
	SetSize( vid.width, vid.height );
	SetPos( 0, 0 );
	SetVisible( true );
	SetCursor( null );

	da_default_color[0] = 1.0;
	da_default_color[1] = 1.0;
	da_default_color[2] = 1.0;

	m_bDrawDebugAreas = false;

	g_pConPanel = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConPanel::~CConPanel( void )
{
}

void CConPanel::Con_NPrintf( int idx, char *msg )
{
	if ( idx < 0 || idx >= MAX_DBG_NOTIFY )
		return;

    Q_snprintf( da_notify[idx].szNotify, sizeof( da_notify[idx].szNotify ), msg );

	// Reset values
	da_notify[idx].expire = realtime + DBG_NOTIFY_TIMEOUT;
	VectorCopy( da_default_color, da_notify[idx].color );
	da_notify[idx].fixed_width_font = false;
	m_bDrawDebugAreas = true;
}

void CConPanel::Con_NXPrintf( struct con_nprint_s *info, char *msg )
{
	if ( !info )
		return;

	if ( info->index < 0 || info->index >= MAX_DBG_NOTIFY )
		return;

    Q_snprintf( da_notify[ info->index ].szNotify, sizeof( da_notify[ info->index ].szNotify ), msg );

	// Reset values
	da_notify[ info->index ].expire = realtime + info->time_to_live;
	VectorCopy( info->color, da_notify[ info->index ].color );
	da_notify[ info->index ].fixed_width_font = info->fixed_width_font;
	m_bDrawDebugAreas = true;
}

static void safestrncat( char *text, int maxlength, char const *add, int addchars )
{
	int curlen = Q_strlen( text );
	if ( curlen >= maxlength )
		return;

	char *p = text + curlen;
	while ( curlen++ < maxlength && 
		--addchars >= 0 )
	{
		*p++ = *add++;
	}
	*p = 0;
}

void CConPanel::AddToNotify( Color& clr, char const *msg )
{
	// This is a game message, and should appear in the notify buffer, even if developer == 0
	if ( msg[0] == 1 || 
		 msg[0] == 2 )
	{
		msg++;
	}

	// Nothing left
	if ( !msg[0] )
		return;

	CNotifyText *current = NULL;

	int slot = m_NotifyText.Count() - 1;
	if ( slot < 0 )
	{
		slot = m_NotifyText.AddToTail();
		current = &m_NotifyText[ slot ];
		current->clr = clr;
		current->text[ 0 ] = 0;
		current->liferemaining = con_notifytime.GetFloat();;
	}
	else
	{
		current = &m_NotifyText[ slot ];
		current->clr = clr;
	}

	Assert( current );

	char const *p = msg;
	while ( *p )
	{
		char *nextreturn = Q_strstr( p, "\n" );
		if ( nextreturn != NULL )
		{
			int copysize = nextreturn - p + 1;
			safestrncat( current->text, sizeof( current->text ), p, copysize );

			// Add a new notify, but don't add a new one if the previous one was empty...
			if ( current->text[0] && current->text[0] != '\n' )
			{
				slot = m_NotifyText.AddToTail();
				current = &m_NotifyText[ slot ];
			}
			// Clear it
			current->clr = clr;
			current->text[ 0 ] = 0;
			current->liferemaining = con_notifytime.GetFloat();
			// Skip return character
			p += copysize;
			continue;
		}

		// Append it
		safestrncat( current->text, sizeof( current->text ), p, Q_strlen( p ) );
		current->clr = clr;
		current->liferemaining = con_notifytime.GetFloat();
		break;
	}

	while ( m_NotifyText.Count() > 0 &&
		( m_NotifyText.Count() >= con_times.GetInt() ) )
	{
		m_NotifyText.Remove( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConPanel::ClearNofify()
{
	m_NotifyText.RemoveAll();
}

void CConPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Console font
	m_hFont = pScheme->GetFont( "DefaultSmall", false );
	m_hFontFixed = pScheme->GetFont( "DefaultFixed", false );
}

int CConPanel::DrawText( vgui::HFont font, int x, int y, char *fmt, ... )
{
	va_list argptr;
	char data[ 1024 ];
	int len;

	va_start(argptr, fmt);
#ifdef _WIN32
	len = _vsnprintf(data, 1024, fmt, argptr);
#else
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
#endif
	va_end(argptr);

	len = DrawColoredText( font, 
		x, 
		y, 
		255, 
		255,
		255, 
		255, 
		data );

	return len;
}


//-----------------------------------------------------------------------------
// called when we're ticked...
//-----------------------------------------------------------------------------
bool CConPanel::ShouldDraw()
{
	bool bVisible = false;

	if ( m_bDrawDebugAreas )
	{
		bVisible = true;
	}

	// Should be invisible if there's no notifys and the console is up.
	// and if the launcher isn't active
	if ( !Con_IsVisible() )
	{
		int i;
		int c = m_NotifyText.Count();
		for ( i = c - 1; i >= 0; i-- )
		{
			CNotifyText *notify = &m_NotifyText[ i ];

			notify->liferemaining -= host_frametime;

			if ( notify->liferemaining <= 0.0f )
			{
				m_NotifyText.Remove( i );
				continue;
			}
			
			bVisible = true;
		}
	}
	else
	{
		bVisible = true;
	}

	return bVisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConPanel::DrawNotify( void )
{
	int x = 8;
	int y = 5;

	if ( !m_hFontFixed )
		return;

	vgui::surface()->DrawSetTextFont( m_hFontFixed );

	int fontTall = vgui::surface()->GetFontTall( m_hFontFixed ) + 1;

	Color clr;

	int c = m_NotifyText.Count();
	for ( int i = 0; i < c; i++ )
	{
		CNotifyText *notify = &m_NotifyText[ i ];

		float timeleft = notify->liferemaining;
	
		clr = notify->clr;

		if ( timeleft < .5f )
		{
			float f = clamp( timeleft, 0.0f, .5f ) / .5f;

			clr[3] = (int)( f * 255.0f );

			if ( i == 0 && f < 0.2f )
			{
				y -= fontTall * ( 1.0f - f / 0.2f );
			}
		}
		else
		{
			clr[3] = 255;
		}
		DrawColoredText( m_hFontFixed, x, y, clr[0], clr[1], clr[2], clr[3], notify->text );

		y += fontTall;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar con_nprint_bgalpha( "con_nprint_bgalpha", "128" );
ConVar con_nprint_bgborder( "con_nprint_bgborder", "5" );

void CConPanel::DrawDebugAreas( void )
{
	if ( !m_bDrawDebugAreas )
		return;

	// Find the top and bottom of all the nprint text so we can draw a box behind it.
	int left=99999, top=99999, right=-99999, bottom=-99999;
	if ( con_nprint_bgalpha.GetInt() )
	{
		// First, figure out the bounds of all the con_nprint text.
		if ( ProcessNotifyLines( left, top, right, bottom, false ) )
		{
			int b = con_nprint_bgborder.GetInt();

			// Now draw a box behind it.
			vgui::surface()->DrawSetColor( 0, 0, 0, con_nprint_bgalpha.GetInt() );
			vgui::surface()->DrawFilledRect( left-b, top-b, right+b, bottom+b );
		}
	}
	
	// Now draw the text.
	if ( ProcessNotifyLines( left, top, right, bottom, true ) == 0 )
	{
		// Have all notifies expired?
		m_bDrawDebugAreas = false;
	}
}


int CConPanel::ProcessNotifyLines( int &left, int &top, int &right, int &bottom, bool bDraw )
{
	int count = 0;
	int y = 20;
	
	for ( int i = 0; i < MAX_DBG_NOTIFY; i++ )
	{
		if ( realtime < da_notify[i].expire )
		{
			int len;
			int x;

			vgui::HFont font = da_notify[i].fixed_width_font ? m_hFontFixed : m_hFont ;

			int fontTall = vgui::surface()->GetFontTall( m_hFontFixed ) + 1;

			len = DrawTextLen( font, da_notify[i].szNotify );
			x = vid.width - 10 - len;

			if ( y + fontTall > (int)vid.height - 20 )
				return count;

			count++;
			int y = 20 + 10 * i;

			if ( bDraw )
			{
				DrawColoredText( font, x, y, 
					da_notify[i].color[0] * 255, 
					da_notify[i].color[1] * 255, 
					da_notify[i].color[2] * 255,
					255,
					da_notify[i].szNotify );
			}

			// Extend the bounds.
			left = min( left, x );
			top = min( top, y );
			right = max( right, x+len );
			bottom = max( bottom, y+fontTall );

			y += fontTall;
		}
	}

	return count;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConPanel::Paint()
{
	VPROF( "CConPanel::Paint" );
	
	DrawDebugAreas();

	DrawNotify();	// only draw notify in game
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConPanel::PaintBackground()
{
	if ( !Con_IsVisible() )
		return;

	int wide = GetWide();
	char ver[ 100 ];
	Q_snprintf(ver, sizeof( ver ), "Source Engine %i/%s (build %d - days until 9/30/03)", PROTOCOL_VERSION, gpszVersionString, build_number() );

	vgui::surface()->DrawSetTextColor( Color( 255, 255, 255, 255 ) );
	int x = wide - DrawTextLen( m_hFont, ver ) - 2;
	DrawText( m_hFont, x, 0, ver );
}

//-----------------------------------------------------------------------------
// Purpose: Creates the Console VGUI object
//-----------------------------------------------------------------------------
static CConPanel *conPanel = NULL;

void Con_CreateConsolePanel( vgui::Panel *parent )
{
	conPanel = new CConPanel( parent );
	if(conPanel)
	{
		conPanel->SetVisible(false);
	}
}

vgui::Panel* Con_GetConsolePanel()
{
	return conPanel;
}

static ConCommand toggleconsole("toggleconsole", Con_ToggleConsole_f);
static ConCommand hideconsole("hideconsole", Con_HideConsole_f);
static ConCommand clear("clear", Con_Clear_f);

#endif // SWDS
