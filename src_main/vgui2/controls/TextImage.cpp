//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implementation of vgui::TextImage control
//
// $NoKeywords: $
//=============================================================================

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>

#include <vgui_controls/TextImage.h>
#include <vgui_controls/Controls.h>

#include "tier0/dbg.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// enable this define if you want unlocalized strings logged to files unfound.txt and unlocalized.txt
// #define LOG_UNLOCALIZED_STRINGS

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TextImage::TextImage(const char *text) : Image()
{
	_utext = NULL;
	_textBufferLen = 0;
	_font = INVALID_FONT; 
	_unlocalizedTextSymbol = INVALID_STRING_INDEX;
	_drawWidth = 0;
	_textBufferLen = 0;
	_textLen = 0;

	SetText(text); // set the text.
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TextImage::TextImage(const wchar_t *wszText) : Image()
{
	_utext = NULL;
	_textBufferLen = 0;
	_font = INVALID_FONT;
	_unlocalizedTextSymbol = INVALID_STRING_INDEX;
	_drawWidth = 0;
	_textBufferLen = 0;
	_textLen = 0;

	SetText(wszText); // set the text.
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
TextImage::~TextImage()
{
	delete [] _utext;
}

//-----------------------------------------------------------------------------
// Purpose: takes the string and looks it up in the localization file to convert it to unicode
//-----------------------------------------------------------------------------
void TextImage::SetText(const char *text)
{
	if (!text)
	{
		text = "";
	}

	// check for localization
	if (*text == '#')
	{
		// try lookup in localization tables
		_unlocalizedTextSymbol = localize()->FindIndex(text + 1);
		
		if (_unlocalizedTextSymbol != INVALID_STRING_INDEX)
		{
			wchar_t *unicode = localize()->GetValueByIndex(_unlocalizedTextSymbol);
			SetText(unicode);
			return;
		}
		else
		{
			// could not find string
			// debug code for logging unlocalized strings
#if defined(LOG_UNLOCALIZED_STRINGS)
			if (*text)
			{
				// write out error to unfound.txt log file
				static bool first = true;
				FILE *f;
				if (first)
				{
					first = false;
					f = fopen("unfound.txt", "wt");
				}
				else
				{
					f = fopen("unfound.txt", "at");
				}

				if (f)
				{
					fprintf(f, "\"%s\"\n", text);
					fclose(f);
				}
			}
#endif // LOG_UNLOCALIZED_STRINGS
		}
	}
	else
	{
		// debug code for logging unlocalized strings
#if defined(LOG_UNLOCALIZED_STRINGS)
		if (text[0])
		{
			// setting a label to be ANSI text, write out error to unlocalized.txt log file
			static bool first = true;
			FILE *f;
			if (first)
			{
				first = false;
				f = fopen("unlocalized.txt", "wt");
			}
			else
			{
				f = fopen("unlocalized.txt", "at");
			}
			if (f)
			{
				fprintf(f, "\"%s\"\n", text);
				fclose(f);
			}
		}
#endif // LOG_UNLOCALIZED_STRINGS
	}

	// convert the ansi string to unicode and use that
	wchar_t unicode[1024];
	localize()->ConvertANSIToUnicode(text, unicode, sizeof(unicode));
	SetText(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the width that the text can be.
//-----------------------------------------------------------------------------
void TextImage::SetDrawWidth(int width)
{
	_drawWidth = width;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the width that the text can be.
//-----------------------------------------------------------------------------
void TextImage::GetDrawWidth(int &width)
{
	width = _drawWidth;
}

//-----------------------------------------------------------------------------
// Purpose: sets unicode text directly
//-----------------------------------------------------------------------------
void TextImage::SetText(const wchar_t *unicode)
{
	// reallocate the buffer if necessary
	_textLen = (short)wcslen(unicode);
	if (_textLen >= _textBufferLen)
	{
		delete [] _utext;
		_textBufferLen = (short)(_textLen + 1);
		_utext = new wchar_t[_textBufferLen];
	}

	// store the text as unicode
	wcscpy(_utext, unicode);
/*
	int wide, tall;
	// get the size of the _text
	GetTextSize(wide, tall, _utext);
	// set the size of the Image to the size of the text.
	SetSize(wide, tall);
*/	
}

//-----------------------------------------------------------------------------
// Purpose: Gets the text in the textImage
//-----------------------------------------------------------------------------
void TextImage::GetText(char *buffer, int bufferSize)
{
	localize()->ConvertUnicodeToANSI(_utext, buffer, bufferSize);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the text in the textImage
//-----------------------------------------------------------------------------
void TextImage::GetText(wchar_t *buffer, int bufferLength)
{
	wcsncpy(buffer, _utext, bufferLength);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextImage::GetUnlocalizedText(char *buffer, int bufferSize)
{
	if (_unlocalizedTextSymbol != INVALID_STRING_INDEX)
	{
		const char *text = localize()->GetNameByIndex(_unlocalizedTextSymbol);
		buffer[0] = '#';
		strncpy(buffer + 1, text, bufferSize - 1);
		buffer[bufferSize-1] = 0;
	}
	else
	{
		GetText(buffer, bufferSize);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Changes the current font
//-----------------------------------------------------------------------------
void TextImage::SetFont(HFont font)
{
	_font = font;
	/*
	// changing the font chould change the size of the image....
	int wide, tall;
	GetTextSize(wide,tall, _utext); // get the size of the text
	SetSize(wide,tall);	// set the image to the size of the text	
*/	
}

//-----------------------------------------------------------------------------
// Purpose: Gets the font of the text.
//-----------------------------------------------------------------------------
HFont TextImage::GetFont()
{
	return _font;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the size of the TextImage. This is directly tied to drawWidth
//-----------------------------------------------------------------------------
void TextImage::SetSize(int wide,int tall)
{
	Image::SetSize(wide,tall);
	_drawWidth = wide;
}

//-----------------------------------------------------------------------------
// Purpose: Draw the Image on screen.
//-----------------------------------------------------------------------------
void TextImage::Paint()
{
	int wide, tall;
	GetSize(wide, tall);
		
	if (!_utext || _font == INVALID_FONT )
		return;

	DrawSetTextColor(GetColor());
	HFont font = GetFont();
	DrawSetTextFont(font);

	int yAdd = surface()->GetFontTall(font);
	int x = 0, y = 0;

	int px, py;
	GetPos(px, py);

	// resize our _text string to one that fits inside _drawWidth 
	wchar_t tempText[1024];
	wcsncpy(tempText, _utext, (sizeof(tempText) / sizeof(wchar_t)) - 1);
	SizeText(tempText, _textLen);

	for (int i = 0; ; i++)
	{
		wchar_t ch = tempText[i];

		int a,b,c;

		surface()->GetCharABCwide(font, ch, a, b, c);
		int len = a + b + c;

		if (ch == 0)
		{
			break;
		}
		else if (ch == '\r')
		{
		}		
		else if (ch == '\n')
		{
			x = 0;
			y += yAdd;
		}
		else if (ch == ' ')
		{
			wchar_t nextch;
			nextch = tempText[i + 1];

			surface()->GetCharABCwide(font, ' ',a,b,c);

			if (!(nextch == 0) && !(nextch == '\n') && !(nextch == '\r'))
			{
				x += a+b+c;
				if (x > wide)
				{
					x = 0;
					y += yAdd;
				}
			}
		}
		else
		{
			int ctr;
			for (ctr = 1;; ctr++)
			{
				ch=tempText[i+ctr];
				if ((ch==0) || (ch=='\n') || (ch=='\r') || (ch==' '))
				{
					break;
				}
				else
				{
					int a,b,c;
					surface()->GetCharABCwide(font, ch,a,b,c);
					len += a+b+c;
				}
			}

			for (int j = 0; j < ctr; j++)
			{
				ch = tempText[i+j];

				// special rendering for & symbols
				if (ch == '&')
				{
					j++;
					ch = tempText[i+j];

					if (j >= ctr)
					{
						continue;
					}

					// double '&' means draw a single '&'
					if (ch != '&')
					{
						//!! disabled until fixed
#ifdef VGUI_DRAW_HOTKEYS_ENABLED
						DrawPrintChar(x, y, '_');
#endif
					}
				}

				surface()->GetCharABCwide(font, ch, a, b, c);
				surface()->DrawSetTextPos(x + px, y + py);
				surface()->DrawUnicodeChar(ch);
				x += a + b + c;
			}
			
			i += ctr - 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the tempText string to a string that fits in _drawWidth
// Input:	tempText, the string to truncate if necessary.
//			This can destroy hotkey chars on screen
//-----------------------------------------------------------------------------
void TextImage::SizeText(wchar_t *tempText, int textBufferLen)
{
	// calculate width of textstring in pixels
	int wide, tall;
	GetTextSize(wide, tall, _utext);

	if( _font == INVALID_FONT )
		return;

	int startChar;
	if (_drawWidth < wide)
	{
		// elipsis size
		int dotWidth = surface()->GetCharacterWidth(GetFont(), '.');
		dotWidth *= 3;
		
		// now figure out how many chars to remove
		startChar = textBufferLen - 1;
		bool addElipsis = true;
		while (wide + dotWidth > _drawWidth)
		{	
			if (startChar < 0)
			{
				addElipsis = false;
				break;
			}
			
			int charWidth = 0;
			int a, b, c;

			// don't count & chars, those aren't displayed if there's just one
			if (tempText[startChar] != '&')
			{
				surface()->GetCharABCwide(GetFont(), tempText[startChar], a, b, c);
				charWidth = (a + b + c);
			}
			else  // they are displayed if there are 2 in a row
			{
				if (startChar-1 > 0)
				{
					if (tempText[startChar-1] == '&') 
					{
						surface()->GetCharABCwide(GetFont(), tempText[startChar], a, b, c);
						charWidth = (a + b + c);
						startChar--; // skip the other & 
					}
				}
			}

			wide -= charWidth;
			if (wide < 0)
			{
				addElipsis = false;
				break;
			}

			startChar--;
		}
		
		if (startChar < 0)
		{
			startChar = 0;
		}

		if (addElipsis)
		{
			// make sure we always keep the first character
			if (startChar == 0)
			{
				if (tempText[startChar] == '&') // we have a hotkey here.
				{
					if (tempText[startChar+1] == '&') // this is a character '&&' = &
						startChar = 2;
					else 
						startChar = 1;
				}
				else   // we want to keep the first char always
					startChar = 1;	
			}
			if (startChar == 1) // lets make sure startChar[0] is a visible char
			{
				// take into account a space at the Start
				// windows doesn't let you Start strings like this
				// the column headers in the serverbrowser Start with spaces for example
				if ( (tempText[0] == '&') || (tempText[0] == ' ')	)
				{
					startChar = 2;
				}
			}

			tempText[startChar] = '.';
			tempText[startChar+1] = '.';
			tempText[startChar+2] =	'.';
			tempText[startChar+3] = '\0';
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the text size taking into account newlines
//-----------------------------------------------------------------------------
void TextImage::GetTextSizeWrapped(int &wideo, int &tallo, const wchar_t *text)
{
	wideo = 0;
	tallo = 0;

	if (!text || _font == INVALID_FONT )
		return;

	int wide,tall;
	GetSize(wide,tall);
	HFont font = GetFont();
	int yAdd = surface()->GetFontTall(font);
	int x = 0, y = 0;
	
	for(int i = 0; ; i++)
	{
		wchar_t ch = text[i];

		int a,b,c;

		surface()->GetCharABCwide(font, ch,a,b,c);
		int len=a+b+c;

		if(ch==0)
		{
			break;
		}
		else
		if(ch=='\r')
		{
		}		
		else
		if(ch=='\n')
		{
			x=0;
			y+=yAdd;
		}
		else
		if(ch==' ')
		{
			wchar_t nextch;
			nextch = text[ i + 1 ];

			surface()->GetCharABCwide(font, ' ',a,b,c);

			if ( !( nextch==0 ) && !(nextch=='\n') && !(nextch=='\r') )
			{
				x+=a+b+c;
				if(x>wide)
				{
					x=0;
					y+=yAdd;
				}
			}
		}
		else
		{
			int ctr;
			for(ctr=1;;ctr++)
			{
				ch=text[i+ctr];
				if( (ch==0) || (ch=='\n') || (ch=='\r') || (ch==' ') )
				{
					break;
				}
				else
				{
					int a,b,c;
					surface()->GetCharABCwide(font, ch,a,b,c);
					len+=a+b+c;
				}
			}

			if((x+len)>wide)
			{
				x=0;
				y+=yAdd;
			}

			for(int j=0;j<ctr;j++)
			{
				ch=_utext[i+j];
				surface()->GetCharABCwide(font, ch,a,b,c);

				if((x+a+b+c)>wideo)
				{
					wideo=x+a+b+c;
				}
				if((y+yAdd)>tallo)
				{
					tallo=y+yAdd;
				}

				x+=a+b+c;
			}
			
			i+=ctr-1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the size of a text string in pixels
//-----------------------------------------------------------------------------
void TextImage::GetTextSize(int &wide, int &tall, const wchar_t *text)
{
	wide = 0;
	tall = 0;
	int maxWide=0;

	HFont font = GetFont();
	if ( font == INVALID_FONT )
		return;

	tall = surface()->GetFontTall(font);

	int textLen = wcslen(text);
	for (int i = 0; i < textLen; i++)
	{
		int a, b, c;
		surface()->GetCharABCwide(font, text[i], a, b, c);
		wide += (a + b + c);

		if (text[i] == '\n')
		{
			tall += surface()->GetFontTall(font);
			if(wide>maxWide) 
			{
				maxWide=wide;
			}
			wide=0; // new line, wide is reset...
		}
	}
	if(wide<maxWide)
	{ // maxWide only gets set if a newline is in the label
		wide=maxWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the size of the text in the image
//-----------------------------------------------------------------------------
void TextImage::GetContentSize(int &wide, int &tall)
{
	GetTextSize(wide, tall, _utext);
}

//-----------------------------------------------------------------------------
// Purpose: Resize the text image to the content size
//-----------------------------------------------------------------------------
void TextImage::ResizeImageToContent()
{
    int wide, tall;
    GetContentSize(wide, tall);
    SetSize(wide, tall);
}

