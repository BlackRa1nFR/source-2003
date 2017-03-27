//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUYMOUSEOVERHTMLBUTTON_H
#define BUYMOUSEOVERHTMLBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/KeyValues.h>
#include <FileSystem.h>
#include "MouseOverHTMLButton.h"
#include "hud.h"

//-----------------------------------------------------------------------------
// Purpose: Triggers a new html page when the mouse goes over the button
//-----------------------------------------------------------------------------
class BuyMouseOverHTMLButton : public MouseOverHTMLButton
{
public:
	BuyMouseOverHTMLButton(vgui::Panel *parent, const char *panelName, vgui::HTML *html, const char *page) :
	  MouseOverHTMLButton( parent, panelName, html, page)	{ SetAddHotKey( false );}

private:
	void ApplySettings( KeyValues *resourceData ) 
	{
		MouseOverHTMLButton::ApplySettings( resourceData );

		KeyValues *kv =resourceData->FindKey("cost", false);
		if( kv ) // if this button has a cost defined for it
		{
			if( kv->GetInt() > gHUD.m_accountBalance.GetBalance() )
			{
				SetEnabled( false );
			}
		}
		SetPage( GetItemPage(resourceData->GetName()) );
	}

	const char *GetItemPage( const char *className )
	{
		char localPath[ _MAX_PATH ];
		static char classHTML[ _MAX_PATH ];

		_snprintf( classHTML, sizeof( classHTML ), "classes/%s.html", className);

		if ( vgui::filesystem()->FileExists( classHTML ) )
		{
			vgui::filesystem()->GetLocalPath ( classHTML, localPath, sizeof( localPath ) );
		}
		else if (vgui::filesystem()->FileExists( "classes/default.html" ) )
		{
			vgui::filesystem()->GetLocalPath ( "classes/default.html", localPath, sizeof( localPath ) );
		}
		else
		{
			return NULL;
		}

		_snprintf( classHTML, sizeof( classHTML ), "file:///%s", localPath);

		return classHTML;
	}


};


#endif // BUYMOUSEOVERHTMLBUTTON_H
