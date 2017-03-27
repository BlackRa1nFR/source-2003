//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Item pickup history displayed onscreen when items are picked up.
//
// $NoKeywords: $
//=============================================================================

#ifndef HISTORY_RESOURCE_H
#define HISTORY_RESOURCE_H
#pragma once

#include "hudelement.h"
#include "ehandle.h"

#include <vgui_controls/Panel.h>

#define MAX_HISTORY 12
enum 
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};

namespace vgui
{
	class IScheme;
}

class C_BaseCombatWeapon;

//-----------------------------------------------------------------------------
// Purpose: Used to draw the history of ammo / weapon / item pickups by the player
//-----------------------------------------------------------------------------
class CHudHistoryResource : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudHistoryResource, vgui::Panel );
private:
	struct HIST_ITEM 
	{
		int type;
		float DisplayTime;  // the time at which this item should be removed from the history
		int iCount;
		int iId;
		CHandle< C_BaseCombatWeapon > m_hWeapon;

		CHudTexture *icon;
	};

	HIST_ITEM rgHistory[MAX_HISTORY];

public:

	CHudHistoryResource( const char *pElementName );

	// CHudElement overrides
	virtual void Init( void );
	virtual void Reset( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void	AddToHistory( int iType, int iId, int iCount = 0 );
	void	AddToHistory( int iType, const char *szName, int iCount = 0 );
	void	AddToHistory( int iType, C_BaseCombatWeapon *weapon, int iCount = 0 );
	void	MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );

	void	CheckClearHistory( void );
	void	SetHistoryGap( int iNewHistoryGap );

public:
	int		m_iHistoryGap;
	int		m_iCurrentHistorySlot;

	vgui::HFont	m_hNumberFont;
};

#endif // HISTORY_RESOURCE_H
