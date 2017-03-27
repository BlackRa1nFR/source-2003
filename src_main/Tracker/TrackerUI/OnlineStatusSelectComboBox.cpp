//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "OnlineStatus.h"
#include "OnlineStatusSelectComboBox.h"
#include "TrackerDoc.h"
#include "ServerSession.h"

#include <VGUI_TextImage.h>
#include <VGUI_IScheme.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : parent panel, name of panel
//-----------------------------------------------------------------------------
COnlineStatusSelectComboBox::COnlineStatusSelectComboBox(Panel *parent, const char *panelName) : vgui::LabelComboBox(parent, panelName)
{
	_statusText = new TextImage("offline");

	AddImage(_statusImage, 6);
	SetTextImageIndex(1);
	SetImagePreOffset(1, 4);
	AddImage(_statusText, 6);
	SetText("< unknown >");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COnlineStatusSelectComboBox::~COnlineStatusSelectComboBox()
{
}
  

//-----------------------------------------------------------------------------
// Purpose: Sets the colors of elements in the gui
// Input  : 
//-----------------------------------------------------------------------------
void COnlineStatusSelectComboBox::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	Button::ApplySchemeSettings(pScheme);
	SetFgColor(GetSchemeColor("StatusSelectFgColor"));
	_statusText->SetColor(GetSchemeColor("StatusSelectFgColor2"));

	SetBgColor(GetSchemeColor("BuddyListBgColor", GetBgColor()));

	SetDefaultColor(GetFgColor(), GetBgColor());

	SetArmedColor(GetSchemeColor("ButtonArmedFgColor", GetFgColor()), GetBgColor());
	SetDepressedColor(GetSchemeColor("ButtonDepressedFgColor", GetFgColor()), GetBgColor());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COnlineStatusSelectComboBox::PerformLayout()
{
	char szStatus[64];

	// set our own text images
	SetText(GetDoc()->Data()->GetString("User/UserName", "< unknown >"));
	_snprintf( szStatus, sizeof( szStatus ) - 1, "#TrackerUI_%s", COnlineStatus::IDToString(ServerSession().GetStatus()));
	szStatus[ sizeof( szStatus ) - 1 ] = '\0';
	_statusText->SetText(szStatus);

	// load in the appropriate icon.
	char buf[128];
	sprintf(buf, "friends/icon_%s", COnlineStatus::IDToString(ServerSession().GetStatus()));
	_statusImage = scheme()->GetImage(scheme()->GetDefaultScheme(), buf);

	if (!_statusImage)
	{
		_statusImage = scheme()->GetImage(scheme()->GetDefaultScheme(), "resource/icon_blank");
	}

	if (_statusImage)
	{
		// add it into Start of the image list
		Label::SetImageAtIndex(0, _statusImage, 6);
	}

	BaseClass::PerformLayout();
}







