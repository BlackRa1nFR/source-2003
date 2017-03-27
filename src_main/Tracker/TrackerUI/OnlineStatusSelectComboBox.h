//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: A class to handle the online status combo box, we display
//          a status icon, the user's username, and their online status
//          Clicking on the box or the arrow button in the box triggers
//          a dropdown menu to change online status and update the text in the box
//
// $NoKeywords: $
//=============================================================================

#ifndef ONLINESTATUSSELECTCOMBOBOX_H
#define ONLINESTATUSSELECTCOMBOBOX_H
#pragma once

#include <VGUI_LabelComboBox.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class COnlineStatusSelectComboBox : public LabelComboBox
{
public:
	COnlineStatusSelectComboBox(Panel *parent, const char *panelName);
	~COnlineStatusSelectComboBox();

	virtual void PerformLayout();

private:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	typedef LabelComboBox BaseClass;
};

#endif // ONLINESTATUSSELECTCOMBOBOX_H
