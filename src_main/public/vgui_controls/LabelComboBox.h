//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This is a class to create a menu that is viewable by clicking
// on a highlighted arrow button or the name of the menu, like a combobox.
// Unlike a combobox, however, the name of the menu is a Label class and
// can be made of connected Images or textImages. Do this in your constructor
// and in your perform layout functions.
// The menu items can be tied directly to thier commands and do not call SetText
// as in ComboBox classes. Therefore they do not call back into the LabelComboBox
// class. If you wish to change what is displayed in the Label in response
// to selecting items in the menu you can use a method like that used in
// OnlineStatusSelectComboBox, in which the menu commands trigger the setting 
// of a status elsewhere, and we check this status in perform layout to 
// generate the appropriate label.
//
// $NoKeywords: $
//=============================================================================

#ifndef LABELCOMBOBOX_H
#define LABELCOMBOBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Controls.h>
#include <vgui_controls/MenuButton.h>

class KeyValues;

namespace vgui
{
class TextImage;
class Border;
class IImage;
class Menu;
class LabelComboBoxButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class LabelComboBox : public Button
{
public:
	LabelComboBox(Panel *parent, const char *panelName);
	~LabelComboBox();

	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);
	
	virtual void PerformLayout();
	virtual void AddItem( const char *itemText, KeyValues *message, Panel *target );
	virtual void HideMenu();
	virtual void OnMousePressed(MouseCode code);

    virtual bool CanBeDefaultButton(void);

protected:
	TextImage *_statusText;
	IImage *_statusImage;

private:
	Menu *m_pDropDown;
	LabelComboBoxButton *m_pButton;

	typedef Button BaseClass;
};

} // namespace vgui

#endif // LABELCOMBOBOX_H
