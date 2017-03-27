//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "sentence.h"
#include "hud_closecaption.h"
#include "vstdlib/strtools.h"
#include <vgui_controls/Controls.h>
#include <vgui/IVgui.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "iclientmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CC_BOTTOM_FRAC .9f
#define CC_HEIGHT		60
#define CC_INSET		4

#define CAPTION_LINGER_TIME	2.0f

static ConVar closecaption( "closecaption", "0", FCVAR_ARCHIVE, "Enable close captioning." );
static ConVar cc_linger_time( "cc_linger_time", "2.0", FCVAR_ARCHIVE, "Close caption linger time." );
//static ConVar cc_language( "cc_language", "english", FCVAR_ARCHIVE, "Close caption language." );

//-----------------------------------------------------------------------------
// Purpose: Helper for sentence.cpp
// Input  : *ansi - 
//			*unicode - 
//			unicodeBufferSize - 
// Output : int
//-----------------------------------------------------------------------------
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize)
{
	return vgui::localize()->ConvertANSIToUnicode( ansi, unicode, unicodeBufferSize );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCloseCaptionItem
{
public:
	CCloseCaptionItem( 
		C_BaseFlex *actor, 
		CSentence *sentence, 
		CCloseCaptionPhrase *phrase,
		float curtime, 
		int frame, 
		double updatetime )
	{
		SetActor( actor );
		m_pPhrase = NULL;
		SetPhrase( phrase );
		SetSentence( sentence );
		SetSentenceTime( curtime );
		SetLastUpdateFrame( frame );
		SetLastUpdateTime( updatetime );
	}

	CCloseCaptionItem( const CCloseCaptionItem& src )
	{
		m_hActor = src.m_hActor;
		m_pSentence = src.m_pSentence;

		SetPhrase( src.GetPhrase() );

		m_nLastUpdateFrame = src.m_nLastUpdateFrame;
		m_flUpdateTime = src.m_flUpdateTime;
		m_flSentenceTime = src.m_flSentenceTime;
		m_flLingerTime = src.m_flLingerTime;
	}

	~CCloseCaptionItem( void )
	{
		delete m_pPhrase;
	}

	void SetPhrase( const CCloseCaptionPhrase *phrase )
	{
		m_flLingerTime = CAPTION_LINGER_TIME;
		delete m_pPhrase;
		if ( !phrase )
		{
			m_pPhrase = NULL;
		}
		else
		{
			// Copy it
			m_pPhrase = new CCloseCaptionPhrase( *phrase );

			// Check for lingering
			int tokenCount = m_pPhrase->CountTokens();
			for ( int i = 0; i < tokenCount; i++ )
			{
				const wchar_t *token = m_pPhrase->GetToken( i );

				if ( !token )
					continue;

				wchar_t cmd[ 256 ];
				wchar_t args[ 256 ];

				if ( !m_pPhrase->SplitCommand( token, cmd, args ) )
					continue;

				if ( wcscmp( cmd, L"linger" ) )
					continue;

				char converted[ 128 ];
				vgui::localize()->ConvertUnicodeToANSI( args, converted, sizeof( converted ) );

				float linger_time = (float)Q_atof( converted );

				if ( linger_time > m_flLingerTime )
				{
					m_flLingerTime = linger_time;
				}
			}
		}
	}

	const CCloseCaptionPhrase *GetPhrase( void ) const
	{
		return m_pPhrase;
	}

	const C_BaseFlex *GetActor( void ) const
	{
		return m_hActor;
	}

	void SetActor( const C_BaseFlex *actor )
	{
		m_hActor = (C_BaseFlex *)actor;
	}

	void SetSentence( const CSentence *sentence )
	{
		m_pSentence = sentence;
	}

	const CSentence *GetSentence( void ) const
	{
		return m_pSentence;
	}

	void SetLastUpdateFrame( int frame )
	{
		m_nLastUpdateFrame = frame;
	}

	int GetLastUpdateFrame( void ) const
	{
		return m_nLastUpdateFrame;
	}

	void SetLastUpdateTime( double t )
	{
		m_flUpdateTime = t;
	}

	double GetLastUpdateTime( void ) const
	{
		return m_flUpdateTime;
	}

	void SetSentenceTime( float t )
	{
		m_flSentenceTime = t;
	}

	float GetSentenceTime( void ) const
	{
		return m_flSentenceTime;
	}

	float GetLingerTime( void ) const
	{
		return m_flLingerTime;
	}

	void SetLingerTime( float t )
	{
		m_flLingerTime = t;
	}

private:
	int					m_nLastUpdateFrame;
	double				m_flUpdateTime;
	float				m_flSentenceTime;
	float				m_flLingerTime;

	CCloseCaptionPhrase	*m_pPhrase;	

	CSentence const		*m_pSentence;
	CHandle< C_BaseFlex > m_hActor;
};

struct VisibleStreamItem
{
	int					height;
	int					width;
	CCloseCaptionItem	*item;
};

DECLARE_HUDELEMENT( CHudCloseCaption );

CHudCloseCaption::CHudCloseCaption( const char *pElementName )
: CHudElement( pElementName ), vgui::Panel( NULL, "HudCloseCaption" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void CHudCloseCaption::LevelInit( void )
{
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	CreateFonts();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCloseCaption::Paint( void )
{
	wrect_t rcOutput;
	rcOutput.left = 0;
	rcOutput.right = ScreenWidth();
	rcOutput.bottom = ScreenHeight() * CC_BOTTOM_FRAC;
	rcOutput.top = rcOutput.bottom - CC_HEIGHT;

	wrect_t rcText = rcOutput;

	rcText.left += CC_INSET;
	rcText.right -= CC_INSET;

	int avail_width = rcText.right - rcText.left;

	int totalheight = 0;
	int i;
	CUtlVector< VisibleStreamItem > visibleitems;
	int c = m_Items.Count();
	int maxwidth = 0;

	for  ( i = 0; i < c; i++ )
	{
		CCloseCaptionItem *item = m_Items[ i ];
		float curtime = item->GetSentenceTime();
		float start = item->GetPhrase()->GetStartTime();

		if ( curtime < start )
			continue;

		float end = item->GetPhrase()->GetEndTime();

		if ( curtime > end )
		{
			// Check lingering
			if ( curtime > end + item->GetLingerTime() )
				continue;
		}

		int itemheight, itemwidth;
		ComputeStreamSize( avail_width, item->GetPhrase(), itemwidth, itemheight );

		totalheight += itemheight;
		if ( itemwidth > maxwidth )
		{
			maxwidth = itemwidth;
		}

		VisibleStreamItem si;
		si.height = itemheight;
		si.width = itemwidth;
		si.item = item;

		visibleitems.AddToTail( si );
	}

	rcText.bottom -= 2;
	rcText.top = rcText.bottom - totalheight;

	/*
	rcText.right = rcText.left + maxwidth + 10;
	vgui::surface()->DrawSetColor( vgui::Color( 0, 0, 0, 192 ) );
	vgui::surface()->DrawFilledRect( rcText.left, rcText.top, rcText.right, rcText.bottom );
	vgui::surface()->DrawSetColor( vgui::Color( 200, 245, 150, 255 ) );
	vgui::surface()->DrawOutlinedRect( rcText.left, rcText.top, rcText.right, rcText.bottom );
	*/

	// Now draw them
	c = visibleitems.Count();
	for ( i = 0; i < c; i++ )
	{
		VisibleStreamItem *si = &visibleitems[ i ];

		int height = si->height;
		CCloseCaptionItem *item = si->item;
	
		rcText.bottom = rcText.top + height;

		wrect_t rcOut = rcText;

		rcOut.right = rcOut.left + si->width + 10;
		
		DrawStream( rcOut, item->GetPhrase() );

		rcText.top += height;
		rcText.bottom += height;

		if ( rcText.top >= rcOutput.bottom )
			break;
	}
}

void CHudCloseCaption::OnTick( void )
{
	SetVisible( closecaption.GetBool() );

	int framecount = gpGlobals->framecount;
	float frametime = gpGlobals->absoluteframetime;

	// Do other thinking here
	int c = m_Items.Count();
	int i;
	for  ( i = c - 1; i >= 0; i-- )
	{
		CCloseCaptionItem *item = m_Items[ i ];
		if ( item->GetLastUpdateFrame() == framecount )
			continue;

		// See if it's a lingerer
		float lt = item->GetLingerTime();
		
		lt -= frametime;

		if ( lt > 0.0f )
		{
			item->SetLingerTime( lt );
			continue;
		}

		delete item;
		m_Items.Remove( i );
	}
}

void CHudCloseCaption::Reset( void )
{
	while ( m_Items.Count() > 0 )
	{
		CCloseCaptionItem *i = m_Items[ 0 ];
		delete i;
		m_Items.Remove( 0 );
	}
}

void CHudCloseCaption::Process( C_BaseFlex *actor, float curtime, CSentence* sentence )
{
	if ( !closecaption.GetBool() )
	{
		Reset();
		return;
	}

	int i;
	int c = m_Items.Count();
	bool found = false;
	CCloseCaptionItem *item = NULL;

	int framecount = gpGlobals->framecount;
	double realtime = (double)engine->Time();

	for ( i = 0; i < c; i++ )
	{
		item = m_Items[ i ];
		Assert( item );
		if ( item->GetSentence() != sentence )
			continue;

		found = true;
		item->SetSentenceTime( curtime );
		item->SetLastUpdateTime( realtime );
		item->SetLastUpdateFrame( framecount );
	}

	if ( found )
		return;

	// Create new entries
	c = sentence->GetCloseCaptionPhraseCount( CC_ENGLISH );
	for ( i = 0; i < c; i++ )
	{	
		CCloseCaptionPhrase *phrase = sentence->GetCloseCaptionPhrase( CC_ENGLISH, i );
		Assert( phrase );
		item = new CCloseCaptionItem( 
			actor, 
			sentence, 
			phrase, 
			curtime, 
			framecount, 
			realtime );
		m_Items.AddToTail( item );
	}
}

void CHudCloseCaption::CreateFonts( void )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	fontSet[ FONT_NORMAL ]				= pScheme->GetFont( "CloseCaption_Normal" );
	fontSet[ FONT_BOLD ]				= pScheme->GetFont( "CloseCaption_Bold" );
	fontSet[ FONT_ITALIC ]				= pScheme->GetFont( "CloseCaption_Italic" );
	fontSet[ FONT_BOLD | FONT_ITALIC ]	= pScheme->GetFont( "CloseCaption_BoldItalic" );

	Assert( fontSet[ FONT_NORMAL ] );
	Assert( fontSet[ FONT_BOLD ] );
	Assert( fontSet[ FONT_ITALIC ] );
	Assert( fontSet[ FONT_BOLD | FONT_ITALIC ] );
}


void CHudCloseCaption::ComputeStreamSize( int available_width, const CCloseCaptionPhrase *phrase, int& itemwidth, int& itemheight )
{
	int c = phrase->CountTokens();
	int i;

	int usedw = 0;
	int fontheight = vgui::surface()->GetFontTall( fontSet[ FONT_NORMAL ] );
	int height = fontheight+1;

	int italic = 0;
	int bold = 0;
	int maxwidth = 0;

	for ( i = 0; i < c; i++ )
	{
		const wchar_t *token = phrase->GetToken( i );
		if ( !token )
			continue;

		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( phrase->SplitCommand( token, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"cr" ) )
			{
				usedw = 0;
				height += ( fontheight + 1 );
			}

			if ( !wcscmp( cmd, L"I" ) )
			{
				italic = !italic;
			}

			if ( !wcscmp( cmd, L"B" ) )
			{
				bold = !bold;
			}

			continue;
		}

		if ( italic )
		{
			italic = FONT_ITALIC;
		}

		if ( bold )
		{
			bold = FONT_BOLD;
		}

		int w, h;
	
		vgui::HFont useF = FindFont( bold | italic );
		
		wchar_t sz[ 1024 ];
		swprintf( sz, L"%s ", token );

		vgui::surface()->GetTextSize( useF, sz, w, h );

		if ( usedw + w > available_width )
		{
			usedw = 0;
			height += ( h + 1 );
		}
		usedw += w;

		if ( usedw > maxwidth )
		{
			maxwidth = usedw;
		}
	}

	itemheight = height;
	itemwidth = maxwidth;
}

void CHudCloseCaption::DrawStream( wrect_t &rcText, const CCloseCaptionPhrase *phrase )
{
	vgui::surface()->DrawSetColor( GetBgColor() );
	vgui::surface()->DrawFilledRect( rcText.left, rcText.top, rcText.right, rcText.bottom );

	int c = phrase->CountTokens();
	int i;

	int fontheight = vgui::surface()->GetFontTall( fontSet[ FONT_NORMAL ] );
	int x = 0;
	int y = 0;

	int available_width = rcText.right - rcText.left;

	color32 clr;
	clr.r = 255;
	clr.b = 255;
	clr.g = 255;
	clr.a = 255;

	CUtlVector< color32 > colorStack;
	colorStack.AddToTail( clr );

	int italic = 0;
	int bold = 0;

	for ( i = 0; i < c; i++ )
	{
		const wchar_t *token = phrase->GetToken( i );
		if ( !token )
			continue;

		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( phrase->SplitCommand( token, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"cr" ) )
			{
				x = 0;
				y += ( fontheight + 1 );
			}

			if ( !wcscmp( cmd, L"clr" ) )
			{
				if ( args[0] == 0 && colorStack.Count()>= 2)
				{
					colorStack.Remove( colorStack.Count() - 1 );
				}
				else
				{
					int r, g, b;
					color32 newcolor;
					if ( 3 == swscanf( args, L"%i,%i,%i", &r, &g, &b ) )
					{
						newcolor.r = r;
						newcolor.g = g;
						newcolor.b = b;
						newcolor.a = 255;
						colorStack.AddToTail( newcolor );
					}
				}
			}

			if ( !wcscmp( cmd, L"I" ) )
			{
				italic = !italic;
			}

			if ( !wcscmp( cmd, L"B" ) )
			{
				bold = !bold;
			}

			continue;
		}

		int w, h;

		if ( italic )
		{
			italic = FONT_ITALIC;
		}

		if ( bold )
		{
			bold = FONT_BOLD;
		}
		vgui::HFont useF = FindFont( bold | italic );
		
		wchar_t sz[ 1024 ];
		swprintf( sz, L"%s ", token );

		vgui::surface()->GetTextSize( useF, sz, w, h );

		if ( x + w > available_width )
		{
			x = 0;
			y += ( h + 1 );
		}

		wrect_t rcOut;
		rcOut.left = rcText.left + x + 5;
		rcOut.right = rcOut.left + w;
		rcOut.top = rcText.top + y;
		rcOut.bottom = rcOut.top + fontheight + 1;

		color32 useColor = colorStack[ colorStack.Count() - 1 ];

		vgui::surface()->DrawSetTextFont( useF );
		vgui::surface()->DrawSetTextPos( rcOut.left, rcOut.top );
		vgui::surface()->DrawSetTextColor( Color( useColor.r, useColor.g, useColor.b, useColor.a ) );
		vgui::surface()->DrawPrintText( sz, wcslen( sz ) );

		x += w;
	}
}
