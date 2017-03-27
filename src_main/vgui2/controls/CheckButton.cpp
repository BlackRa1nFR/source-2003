//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdarg.h>
#include <stdio.h>

#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>

#include <vgui_controls/Image.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Check box image
//-----------------------------------------------------------------------------
class CheckImage : public TextImage
{
public:
	CheckImage(CheckButton *CheckButton) : TextImage( "g" )
	{
		_CheckButton = CheckButton;

		SetSize(20, 13);
	}

	virtual void Paint()
	{
		DrawSetTextFont(GetFont());

		// draw background
		if (_CheckButton->IsEnabled())
		{
			DrawSetTextColor(_bgColor);
		}
		else
		{
			DrawSetTextColor(_CheckButton->GetBgColor());
		}
		DrawPrintChar(0, 1, 'g');
	
		// draw border box
		DrawSetTextColor(_borderColor1);
		DrawPrintChar(0, 1, 'e');
		DrawSetTextColor(_borderColor2);
		DrawPrintChar(0, 1, 'f');

		// draw selected check
		if (_CheckButton->IsSelected())
		{
			DrawSetTextColor(_checkColor);
			DrawPrintChar(0, 2, 'b');
		}
	}

	Color _borderColor1;
	Color _borderColor2;
	Color _checkColor;

	Color _bgColor;

private:
	CheckButton *_CheckButton;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CheckButton::CheckButton(Panel *parent, const char *panelName, const char *text) : ToggleButton(parent, panelName, text)
{
 	SetContentAlignment(a_west);

	// create the image
	_checkBoxImage = new CheckImage(this);

	SetTextImageIndex(1);
	SetImageAtIndex(0, _checkBoxImage, CHECK_INSET);

	_selectedFgColor = Color( 196, 181, 80, 255 );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CheckButton::~CheckButton()
{
	delete _checkBoxImage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CheckButton::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("FgColor", pScheme));
	_checkBoxImage->_bgColor = GetSchemeColor("CheckBgColor", Color(62, 70, 55, 255), pScheme);
	_checkBoxImage->_borderColor1 = GetSchemeColor("CheckButtonBorder1", Color(20, 20, 20, 255), pScheme);
	_checkBoxImage->_borderColor2 = GetSchemeColor("CheckButtonBorder2", Color(90, 90, 90, 255), pScheme);
	_checkBoxImage->_checkColor = GetSchemeColor("CheckButtonCheck", Color(20, 20, 20, 255), pScheme);
	_selectedFgColor = GetSchemeColor("BrightControlText", GetSchemeColor("ControlText",pScheme), pScheme);

	SetContentAlignment(Label::a_west);

	_checkBoxImage->SetFont( pScheme->GetFont("Marlett", IsProportional()) );
	_checkBoxImage->ResizeImageToContent();
	SetImageAtIndex(0, _checkBoxImage, CHECK_INSET);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IBorder *CheckButton::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Check the button
//-----------------------------------------------------------------------------
void CheckButton::SetSelected(bool state)
{
	// send a message saying we've been checked
	KeyValues *msg = new KeyValues("CheckButtonChecked", "state", (int)state);
	PostActionSignal(msg);

	BaseClass::SetSelected(state);
}

//-----------------------------------------------------------------------------
// Purpose: Gets a different foreground text color if we are selected
//-----------------------------------------------------------------------------
Color CheckButton::GetButtonFgColor()
{
	if (IsSelected())
	{
		return _selectedFgColor;
	}

	return BaseClass::GetButtonFgColor();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CheckButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	SetTextColorState(CS_NORMAL);

	// use our own text object instead of the labels
	const char *labelText =	inResourceData->GetString("labelText", NULL);
	if (labelText)
	{
		BaseClass::SetText("");
		SetText(labelText);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CheckButton::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);

	char buf[256];
	GetText(buf, 255);
	outResourceData->SetString("labelText", buf);
}

//-----------------------------------------------------------------------------
// Purpose: Sets new text
//-----------------------------------------------------------------------------
void CheckButton::SetText(const wchar_t *text)
{
	BaseClass::SetText(text);
	SetHotkey(CalculateHotkey(text));
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets new text
//-----------------------------------------------------------------------------
void CheckButton::SetText(const char *text)
{
	BaseClass::SetText(text);
	SetHotkey(CalculateHotkey(text));
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CheckButton::OnCheckButtonChecked(Panel *panel)
{
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CheckButton::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( CheckButton, "CheckButtonChecked", OnCheckButtonChecked, "panel" ),	// custom message
};
IMPLEMENT_PANELMAP(CheckButton, ToggleButton);
