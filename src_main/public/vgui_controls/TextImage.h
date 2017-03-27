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

#ifndef TEXTIMAGE_H
#define TEXTIMAGE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Image.h>

class KeyValues;

namespace vgui
{

class Panel;
class App;

//-----------------------------------------------------------------------------
// Purpose: Image that handles drawing of a text string
//-----------------------------------------------------------------------------
class TextImage : public Image
{
public:	
	TextImage(const char *text);
	TextImage(const wchar_t *wszText);
	~TextImage();

public:
	// takes the string and looks it up in the localization file to convert it to unicode
	virtual void SetText(const char *text);
	// sets unicode text directly
	virtual void SetText(const wchar_t *text);
	// get the full text in the image
	virtual void GetText(char *buffer, int bufferSize);
	virtual void GetText(wchar_t *buffer, int bufferLength);
	// get the text in it's unlocalized form
	virtual void GetUnlocalizedText(char *buffer, int bufferSize);

	// set the font of the text
	virtual void SetFont(vgui::HFont font);
	// get the font of the text
	virtual vgui::HFont GetFont();

	// set the width of the text to be drawn
	// use this function if the textImage is in another window to cause 
	// the text to be truncated to the width of the window (elipsis added)
	void SetDrawWidth(int width);
	// get the width of the text to be drawn
	void GetDrawWidth(int &width); 

    void ResizeImageToContent();

	// set the size of the image
	virtual void SetSize(int wide,int tall);

	// get the full size of a text string
	virtual void GetContentSize(int &wide, int &tall);

	// gets the size of a specified piece of text
	virtual void GetTextSize(int &wide, int &tall, const wchar_t *text);

	// get the full size of the text string taking into account newlines
	virtual void GetTextSizeWrapped(int &wide, int &tall, const wchar_t *text);

	// draws the text
	virtual void Paint();

protected:
	// truncate the _text string to fit into the draw width
	void SizeText(wchar_t *tempText, int stringLength);

private:

	void ReloadFont();

	wchar_t *_utext;	// unicode version of the text
	short _textBufferLen;	// size of the text buffer
	short _textLen;		// length of the text string
	vgui::HFont _font;	// font of the text string
	int _drawWidth;		// this is the width of the window we are drawing into. 
						// if there is not enough room truncate the txt	and add an elipsis
	unsigned short _unlocalizedTextSymbol;
};

} // namespace vgui

#endif // TEXTIMAGE_H
