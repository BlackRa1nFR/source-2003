//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Item pickup history displayed onscreen when items are picked up.
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "history_resource.h"
#include "hud_macros.h"
#include "parsemsg.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "iclientmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar hud_drawhistory_time;

#define HISTORY_PICKUP_PICK_HEIGHT		(32 + (m_iHistoryGap * 2))
#define HISTORY_PICKUP_HEIGHT_MAX		(ScreenHeight() - 64)
#define	ITEM_GUTTER_SIZE				48


DECLARE_HUDELEMENT( CHudHistoryResource );
DECLARE_HUD_MESSAGE( CHudHistoryResource, ItemPickup );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudHistoryResource::CHudHistoryResource( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudHistoryResource" )
{	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pScheme - 
//-----------------------------------------------------------------------------
void CHudHistoryResource::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( false );
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	m_hNumberFont = pScheme->GetFont( "HudNumbersSmall" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHistoryResource::Init( void )
{
	HOOK_MESSAGE( ItemPickup );

	m_iHistoryGap = 0;
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHistoryResource::Reset( void )
{
	memset( rgHistory, 0, sizeof rgHistory );
	m_iCurrentHistorySlot = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Set a new minimum size gap between history icons
//-----------------------------------------------------------------------------
void CHudHistoryResource::SetHistoryGap( int iNewHistoryGap )
{
	if ( iNewHistoryGap > m_iHistoryGap )
	{
		m_iHistoryGap = iNewHistoryGap;
	}
}

void CHudHistoryResource::AddToHistory( int iType, C_BaseCombatWeapon *weapon, int iCount /*= 0*/ )
{
	// Check to see if the pic would have to be drawn too high. If so, start again from the bottom
	if ( (((m_iHistoryGap * m_iCurrentHistorySlot) + HISTORY_PICKUP_PICK_HEIGHT) > HISTORY_PICKUP_HEIGHT_MAX) || (m_iCurrentHistorySlot >= MAX_HISTORY) )
	{	
		m_iCurrentHistorySlot = 0;
	}
	
	// default to just writing to the first slot
	HIST_ITEM *freeslot = &rgHistory[m_iCurrentHistorySlot++];  
	freeslot->type = iType;
	freeslot->iId = -1;
	freeslot->m_hWeapon = weapon;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gpGlobals->curtime + hud_drawhistory_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add a new entry to the pickup history
//-----------------------------------------------------------------------------
void CHudHistoryResource::AddToHistory( int iType, int iId, int iCount )
{
	// Ignore adds with no count
	if ( iType == HISTSLOT_AMMO && !iCount )
		return;  

	// Check to see if the pic would have to be drawn too high. If so, start again from the bottom
	if ( (((m_iHistoryGap * m_iCurrentHistorySlot) + HISTORY_PICKUP_PICK_HEIGHT) > HISTORY_PICKUP_HEIGHT_MAX) || (m_iCurrentHistorySlot >= MAX_HISTORY) )
	{	
		m_iCurrentHistorySlot = 0;
	}
	
	// default to just writing to the first slot
	HIST_ITEM *freeslot = &rgHistory[m_iCurrentHistorySlot++];  
	freeslot->type = iType;
	freeslot->iId = iId;
	freeslot->m_hWeapon = NULL;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gpGlobals->curtime + hud_drawhistory_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add a new entry to the pickup history
//-----------------------------------------------------------------------------
void CHudHistoryResource::AddToHistory( int iType, const char *szName, int iCount )
{
	if ( iType != HISTSLOT_ITEM )
		return;

	// Check to see if the pic would have to be drawn too high. If so, start again from the bottom
	if ( (((m_iHistoryGap * m_iCurrentHistorySlot) + HISTORY_PICKUP_PICK_HEIGHT) > HISTORY_PICKUP_HEIGHT_MAX) || (m_iCurrentHistorySlot >= MAX_HISTORY) )
	{
		m_iCurrentHistorySlot = 0;
	}

	// default to just writing to the first slot
	HIST_ITEM *freeslot = &rgHistory[m_iCurrentHistorySlot++];  

	// Get the item's icon
	CHudTexture *i = gHUD.GetIcon( szName );
	if ( i == NULL )
		return;  

	freeslot->iId = 1;
	freeslot->icon = i;
	freeslot->type = iType;
	freeslot->m_hWeapon  = NULL;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gpGlobals->curtime + hud_drawhistory_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Handle an item pickup event from the server
//-----------------------------------------------------------------------------
void CHudHistoryResource::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the item to the history
	AddToHistory( HISTSLOT_ITEM, szName );
}

//-----------------------------------------------------------------------------
// Purpose: If there aren't any items in the history, clear it out.
//-----------------------------------------------------------------------------
void CHudHistoryResource::CheckClearHistory( void )
{
	for ( int i = 0; i < MAX_HISTORY; i++ )
	{
		if ( rgHistory[i].type )
			return;
	}

	m_iCurrentHistorySlot = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudHistoryResource::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && m_iCurrentHistorySlot );
}

//-----------------------------------------------------------------------------
// Purpose: Draw the pickup history
//-----------------------------------------------------------------------------
void CHudHistoryResource::Paint( void )
{
	for ( int i = 0; i < MAX_HISTORY; i++ )
	{
		if ( rgHistory[i].type )
		{
			rgHistory[i].DisplayTime = min( rgHistory[i].DisplayTime, gpGlobals->curtime + hud_drawhistory_time.GetFloat() );
			if ( rgHistory[i].DisplayTime <= gpGlobals->curtime )
			{  // pic drawing time has expired
				memset( &rgHistory[i], 0, sizeof(HIST_ITEM) );
				CheckClearHistory();
				continue;
			}

			float elapsed = rgHistory[i].DisplayTime - gpGlobals->curtime;
			float scale = elapsed * 80;
			Color clr = gHUD.m_clrNormal;
			clr[3] = min( scale, 255 );

			switch ( rgHistory[i].type )
			{
				case HISTSLOT_AMMO:
				{
					CHudTexture *ic = gWR.GetAmmoIconFromWeapon( rgHistory[i].iId );
					int		iWidth	= ic ? ic->Width() : 0;

					// Draw the pic
					int ypos = ScreenHeight() - (HISTORY_PICKUP_PICK_HEIGHT + (m_iHistoryGap * i));
					int xpos = ScreenWidth() - ( iWidth + ITEM_GUTTER_SIZE );

					 // weapon isn't loaded yet so just don't draw the pic
					int iconHeight = 0;
					if ( ic  )   
					{ 
						// the dll has to make sure it has sent info the weapons you need
						ic->DrawSelf( xpos, ypos, clr );
						iconHeight = ic->Height();
					}

					//Offset the number to sit properly next to the icon
					ypos -= ( surface()->GetFontTall( m_hNumberFont ) - iconHeight ) / 2;

					vgui::surface()->DrawSetTextFont( m_hNumberFont );
					vgui::surface()->DrawSetTextColor( clr );
					vgui::surface()->DrawSetTextPos( ScreenWidth() - ( ITEM_GUTTER_SIZE * 0.85f ), ypos );

					if ( rgHistory[i].iCount )
					{
						char sz[ 32 ];
						int len = Q_snprintf( sz, sizeof( sz ), "%i", rgHistory[i].iCount );
					
						for ( int ch = 0; ch < len; ch++ )
						{
							char c = sz[ ch ];
							vgui::surface()->DrawUnicodeChar( c );
						}
					} 
				}
				break;
			case HISTSLOT_WEAP:
				{
					C_BaseCombatWeapon *pWeapon = rgHistory[i].m_hWeapon;
					if ( !pWeapon )
						return;

					if ( !pWeapon->HasAmmo() )
					{
						// if the weapon doesn't have ammo, display it as red
						clr = gHUD.m_clrCaution;	
						clr[3] = min( scale, 255 );
					}

					int ypos = ScreenHeight() - (HISTORY_PICKUP_PICK_HEIGHT + (m_iHistoryGap * i));
					int xpos = ScreenWidth() - pWeapon->GetSpriteInactive()->Width();

					pWeapon->GetSpriteInactive()->DrawSelf( xpos, ypos, clr );
				}
				break;
			case HISTSLOT_ITEM:
				{
					if ( !rgHistory[i].iId )
						continue;

					CHudTexture *ic = rgHistory[i].icon;
					if ( !ic )
						continue;

					int ypos = ScreenHeight() - (HISTORY_PICKUP_PICK_HEIGHT + (m_iHistoryGap * i));
					int xpos = ScreenWidth() - ic->Width() - 10;

					ic->DrawSelf( xpos, ypos, clr );
				}
				break;
			default:
				{
					// Unknown history type???!!!
					Assert( 0 );
				}
				break;
			}
		}
	}
}


