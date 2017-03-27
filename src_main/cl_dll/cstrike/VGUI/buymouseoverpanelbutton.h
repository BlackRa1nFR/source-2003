//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUYMOUSEOVERPANELBUTTON_H
#define BUYMOUSEOVERPANELBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <KeyValues.h>
#include <FileSystem.h>
#include <game_controls/MouseOverPanelButton.h>
#include "hud.h"
#include "c_cs_player.h"
#include "cs_gamerules.h"
//#include "Exports.h"

//-----------------------------------------------------------------------------
// Purpose: Triggers a new panel when the mouse goes over the button
//-----------------------------------------------------------------------------
class BuyMouseOverPanelButton : public MouseOverPanelButton
{
private:
	typedef MouseOverPanelButton BaseClass;
public:
	BuyMouseOverPanelButton(vgui::Panel *parent, const char *panelName, vgui::Panel *panel) :
	  MouseOverPanelButton( parent, panelName, panel)	
	  {
		m_iPrice = 0;
		m_iASRestrict = 0;
		m_iDEUseOnly = 0;
	  }

	virtual void ApplySettings( KeyValues *resourceData ) 
	{
		BaseClass::ApplySettings( resourceData );

		KeyValues *kv = resourceData->FindKey( "cost", false );
		if( kv ) // if this button has a cost defined for it
		{
			m_iPrice = kv->GetInt(); // save the price away
		}

		kv = resourceData->FindKey( "as_restrict", false );
		if( kv ) // if this button has a map limitation for it
		{
			m_iASRestrict = kv->GetInt(); // save the as_restrict away
		}
		
		kv = resourceData->FindKey( "de_useonly", false );
		if( kv ) // if this button has a map limitation for it
		{
			m_iDEUseOnly = kv->GetInt(); // save the de_useonly away
		}

		SetPriceState();
		SetMapTypeState();
	}

	int GetASRestrict() { return m_iASRestrict; }

	int GetDEUseOnly() { return m_iDEUseOnly; }

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		SetPriceState();
		SetMapTypeState();
	}
	
	void SetPriceState()
	{
		if ( m_iPrice &&  m_iPrice > C_CSPlayer::GetLocalCSPlayer()->GetAccount() )
		{
			SetEnabled( false );
		}
		else
		{
			SetEnabled( true );
		}
	}

	void SetMapTypeState()
	{
		if ( CSGameRules()->IsVIPMap() )
		{
			if ( m_iASRestrict )
			{
				SetEnabled( false );
			}
		}

		if ( !CSGameRules()->IsBombDefuseMap() )
		{
			if ( m_iDEUseOnly )
			{
				SetEnabled( false );
			}
		}
	}

private:

	int m_iPrice;
	int m_iASRestrict;
	int m_iDEUseOnly;
};


#endif // BUYMOUSEOVERPANELBUTTON_H
