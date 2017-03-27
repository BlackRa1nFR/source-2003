//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vgui_surfacelib/FontManager.h"
#include <tier0/dbg.h>

#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static CFontManager s_FontManager;
//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CFontManager &FontManager()
{
	return s_FontManager;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFontManager::CFontManager()
{
	// add a single empty font, to act as an invalid font handle 0
	m_FontAmalgams.EnsureCapacity(100);
	m_FontAmalgams.AddToTail();
	m_Win32Fonts.EnsureCapacity(100);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CFontManager::~CFontManager()
{
	ClearAllFonts();
}

//-----------------------------------------------------------------------------
// Purpose: frees the fonts
//-----------------------------------------------------------------------------
void CFontManager::ClearAllFonts()
{
	// free the fonts
	for (int i = 0; i < m_Win32Fonts.Count(); i++)
	{
		delete m_Win32Fonts[i];
	}
	m_Win32Fonts.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::HFont CFontManager::CreateFont()
{
	int i = m_FontAmalgams.AddToTail();
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: adds glyphs to a font created by CreateFont()
//-----------------------------------------------------------------------------
bool CFontManager::AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange)
{
	// see if we already have the win32 font
	CWin32Font *winFont = NULL;
	for (int i = 0; i < m_Win32Fonts.Count(); i++)
	{
		if (m_Win32Fonts[i]->IsEqualTo(windowsFontName, tall, weight, blur, scanlines, flags))
		{
			winFont = m_Win32Fonts[i];
			break;
		}
	}

	// create the new win32font if we didn't find it
	if (!winFont)
	{
		i = m_Win32Fonts.AddToTail();

		m_Win32Fonts[i] = new CWin32Font();
		if (m_Win32Fonts[i]->Create(windowsFontName, tall, weight, blur, scanlines, flags))
		{
			// add to the list
			winFont = m_Win32Fonts[i];
		}
		else
		{
			// failed to create, remove
			delete m_Win32Fonts[i];
			m_Win32Fonts.Remove(i);
			return false;
		}
	}

	// add to the amalgam
	m_FontAmalgams[font].AddFont(winFont, lowRange, highRange);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: finds a font handle given a name, returns 0 if not found
//-----------------------------------------------------------------------------
vgui::HFont CFontManager::GetFontByName(const char *name)
{
	for (int i = 1; i < m_FontAmalgams.Count(); i++)
	{
		if (!stricmp(name, m_FontAmalgams[i].Name()))
		{
			return i;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: gets the windows font for the particular font in the amalgam
//-----------------------------------------------------------------------------
CWin32Font *CFontManager::GetFontForChar(vgui::HFont font, wchar_t wch)
{
	return m_FontAmalgams[font].GetFontForChar(wch);
}

//-----------------------------------------------------------------------------
// Purpose: returns the abc widths of a single character
//-----------------------------------------------------------------------------
void CFontManager::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c)
{
	CWin32Font *winFont = m_FontAmalgams[font].GetFontForChar(ch);
	if (winFont)
	{
		winFont->GetCharABCWidths(ch, a, b, c);
	}
	else
	{
		// no font for this range, just use the default width
		a = c = 0;
		b = m_FontAmalgams[font].GetFontMaxWidth();
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of a font
//-----------------------------------------------------------------------------
int CFontManager::GetFontTall(HFont font)
{
	return m_FontAmalgams[font].GetFontHeight();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFontManager::IsFontAdditive(HFont font)
{
	return ( m_FontAmalgams[font].GetFlags( 0 ) & vgui::ISurface::FONTFLAG_ADDITIVE ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: returns the pixel width of a single character
//-----------------------------------------------------------------------------
int CFontManager::GetCharacterWidth(HFont font, int ch)
{
	if (iswprint(ch))
	{
		int a, b, c;
		GetCharABCwide(font, ch, a, b, c);
		return (a + b + c);
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: returns the area of a text string, including newlines
//-----------------------------------------------------------------------------
void CFontManager::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall)
{
	wide = 0;
	tall = 0;
	
	if (!text)
		return;

	tall = GetFontTall(font);
	
	int xx = 0;
	for (int i = 0; ; i++)
	{
		wchar_t ch = text[i];
		if (ch == 0)
		{
			break;
		}
		else if (ch == '\n')
		{
			tall += GetFontTall(font);
			xx=0;
		}
		else if (ch == '&')
		{
			// underscore character, so skip
		}
		else
		{
			xx += GetCharacterWidth(font, ch);
			if (xx > wide)
			{
				wide = xx;
			}
		}
	}
}
