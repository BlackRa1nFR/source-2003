/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// menu.cpp
//
// generic menu handler
//
#include "cbase.h"
#include "text_message.h"
#include "hud_macros.h"
#include "parsemsg.h"
#include "iclientmode.h"
#include "weapon_selection.h"

#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

#define MAX_MENU_STRING	512
char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];


#include "menu.h"
#include "vgui_ScorePanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
//-----------------------------------------------------
//

DECLARE_HUDELEMENT( CHudMenu );
DECLARE_HUD_MESSAGE( CHudMenu, ShowMenu );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenu::CHudMenu( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass(NULL, "HudMenu")
{
	m_nSelectedItem = -1;

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::Init( void )
{
	HOOK_MESSAGE( ShowMenu );

	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	m_Processed.RemoveAll();
	m_nMaxPixels = 0;
	m_nHeight = 0;
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::Reset( void )
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudMenu::IsMenuOpen( void )
{
	return m_fMenuDisplayed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::OnThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenu::ShouldDraw( void )
{
	bool draw = CHudElement::ShouldDraw() && m_fMenuDisplayed;
	if ( !draw )
		return false;

	// check for if menu is set to disappear
	if ( m_flShutoffTime > 0 && m_flShutoffTime <= gpGlobals->realtime )
	{  
		// times up, shutoff
		m_fMenuDisplayed = 0;
		return false;
	}

	return draw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//			textlen - 
//			font - 
//			x - 
//			y - 
//-----------------------------------------------------------------------------
void CHudMenu::PaintString( const char *text, int textlen, HFont& font, int x, int y )
{
	surface()->DrawSetTextFont( font );
	int ch;
	surface()->DrawSetTextPos( x, y );

	for ( ch = 0; ch < textlen; ch++ )
	{
		char c = text[ ch ];
		surface()->DrawUnicodeChar( c );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::Paint()
{
	if ( !m_fMenuDisplayed )
		return;

	// center it
	int x = 20;

	Color	menuColor = m_MenuColor;
	Color itemColor = m_ItemColor;

	int c = m_Processed.Count();

	int border = 20;

	int wide = m_nMaxPixels + border;
	int tall = m_nHeight + border;

	int y = ( ScreenHeight() - tall ) * 0.5f;

	DrawBox( x - border/2, y - border/2, wide, tall, m_BoxColor, m_flSelectionAlphaOverride / 255.0f );

	//DrawTexturedBox( x - border/2, y - border/2, wide, tall, m_BoxColor, m_flSelectionAlphaOverride / 255.0f );

	menuColor[3] = menuColor[3] * ( m_flSelectionAlphaOverride / 255.0f );
	itemColor[3] = itemColor[3] * ( m_flSelectionAlphaOverride / 255.0f );

	for ( int i = 0; i < c; i++ )
	{
		ProcessedLine *line = &m_Processed[ i ];
		Assert( line );

		Color clr = line->menuitem != 0 ? itemColor : menuColor;

		bool canblur = false;
		if ( line->menuitem != 0 &&
			m_nSelectedItem >= 0 && 
			( line->menuitem == m_nSelectedItem ) )
		{
			canblur = true;
		}
		
		surface()->DrawSetTextColor( clr );

		int drawLen = line->length;
		if ( line->menuitem != 0 )
		{
			drawLen *= m_flTextScan;
		}

		surface()->DrawSetTextFont( line->menuitem != 0 ? m_hItemFont : m_hTextFont );

		PaintString( &g_szMenuString[ line->startchar ], drawLen, 
			line->menuitem != 0 ? m_hItemFont : m_hTextFont, x, y );

		if ( canblur )
		{
			// draw the overbright blur
			for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
			{
				if (fl >= 1.0f)
				{
					PaintString( &g_szMenuString[ line->startchar ], drawLen, m_hItemFontPulsing, x, y );
				}
				else
				{
					// draw a percentage of the last one
					Color col = clr;
					col[3] *= fl;
					surface()->DrawSetTextColor(col);
					PaintString( &g_szMenuString[ line->startchar ], drawLen, m_hItemFontPulsing, x, y );
				}
			}
		}

		y += line->height;
	}
}

//-----------------------------------------------------------------------------
// Purpose: selects an item from the menu
//-----------------------------------------------------------------------------
void CHudMenu::SelectMenuItem( int menu_item )
{
	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ( (menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item-1))) )
	{
		char szbuf[32];
		sprintf( szbuf, "menuselect %d\n", menu_item );
		engine->ClientCmd( szbuf );

		m_nSelectedItem = menu_item;
		// Pulse the selection
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuPulse");

		// remove the menu quickly
		m_flShutoffTime = gpGlobals->realtime + m_flOpenCloseTime;
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuClose");
	}
}

void CHudMenu::ProcessText( void )
{
	m_Processed.RemoveAll();
	m_nMaxPixels = 0;
	m_nHeight = 0;

	int i = 0;
	int startpos = i;
	int menuitem = 0;
	while ( i < MAX_MENU_STRING  )
	{
		char ch = g_szMenuString[ i ];
		if ( ch == 0 )
			break;

		if ( i == startpos && 
			( ch == '-' && g_szMenuString[ i + 1 ] == '>' ) )
		{
			// Special handling for menu item specifiers
			menuitem = atoi( &g_szMenuString[ i + 2 ] );
			i += 2;
			startpos += 2;

			continue;
		}

		// Skip to end of line
		while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' && g_szMenuString[i] != '\n' )
		{
			i++;
		}

		// Store off line
		if ( ( i - startpos ) >= 1 )
		{
			ProcessedLine line;
			line.menuitem = menuitem;
			line.startchar = startpos;
			line.length = i - startpos;
			line.pixels = 0;
			line.height = 0;

			m_Processed.AddToTail( line );
		}

		menuitem = 0;

		// Skip delimiter
		if ( g_szMenuString[i] == '\n' )
		{
			i++;
		}
		startpos = i;
	}

	// Add final block
	if ( i - startpos >= 1 )
	{
		ProcessedLine line;
		line.menuitem = menuitem;
		line.startchar = startpos;
		line.length = i - startpos;
		line.pixels = 0;
		line.height = 0;

		m_Processed.AddToTail( line );
	}

	// Now compute pixels needed
	int c = m_Processed.Count();
	for ( i = 0; i < c; i++ )
	{
		ProcessedLine *l = &m_Processed[ i ];
		Assert( l );

		int pixels = 0;
		vgui::HFont font = l->menuitem != 0 ? m_hItemFont : m_hTextFont;

		for ( int ch = 0; ch < l->length; ch++ )
		{
			pixels += surface()->GetCharacterWidth( font, g_szMenuString[ ch + l->startchar ] );
		}

		l->pixels = pixels;
		l->height = surface()->GetFontTall( font );
		if ( pixels > m_nMaxPixels )
		{
			m_nMaxPixels = pixels;
		}
		m_nHeight += l->height;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for ShowMenu message
//   takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, false if it's the last string
//		string: menu string to display
//  if this message is never received, then scores will simply be the combined totals of the players.
//-----------------------------------------------------------------------------
void CHudMenu::MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_bitsValidSlots = (short)READ_WORD();
	int DisplayTime = READ_CHAR();
	int NeedMore = READ_BYTE();

	if ( DisplayTime > 0 )
	{
		m_flShutoffTime = m_flOpenCloseTime + DisplayTime + gpGlobals->realtime;

	}
	else
	{
		m_flShutoffTime = -1;
	}

	if ( m_bitsValidSlots )
	{
		if ( !m_fWaitingForMore ) // this is the start of a new menu
		{
			Q_strncpy( g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING );
		}
		else
		{  // append to the current menu string
			strncat( g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING - strlen(g_szPrelocalisedMenuString) );
		}
		g_szPrelocalisedMenuString[MAX_MENU_STRING-1] = 0;  // ensure null termination

		if ( !NeedMore )
		{  
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuOpen");
			m_nSelectedItem = -1;
			// we have the whole string, so we can localise it now
			strcpy( g_szMenuString, hudtextmessage->BufferedLocaliseTextString( g_szPrelocalisedMenuString ) );
			ProcessText();
		}

		m_fMenuDisplayed = 1;
	}
	else
	{
		m_flShutoffTime = gpGlobals->realtime + m_flOpenCloseTime;
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuClose");
		//m_fMenuDisplayed = 0; // no valid slots means that the menu should be turned off
	}

	m_fWaitingForMore = NeedMore;
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetPaintBackgroundEnabled( false );

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	surface()->GetScreenSize(screenWide, screenTall);
	SetBounds(0, y, screenWide, screenTall - y);

	ProcessText();
}