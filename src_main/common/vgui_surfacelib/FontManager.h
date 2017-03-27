//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FONTMANAGER_H
#define FONTMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "vgui_surfacelib/FontAmalgam.h"

#ifdef CreateFont
#undef CreateFont
#endif

class CWin32Font;

using vgui::HFont;

//-----------------------------------------------------------------------------
// Purpose: Creates and maintains list of actively used fonts
//-----------------------------------------------------------------------------
class CFontManager
{
public:
	CFontManager();
	~CFontManager();

	// clears the current font list, frees any resources
	void ClearAllFonts();

	HFont CreateFont();
	bool AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange);
	HFont GetFontByName(const char *name);
	void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c);
	int GetFontTall(HFont font);
	int GetCharacterWidth(HFont font, int ch);
	void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall);

	CWin32Font *GetFontForChar(HFont, wchar_t wch);
	bool IsFontAdditive(HFont font);

private:
	CUtlVector<CFontAmalgam> m_FontAmalgams;
	CUtlVector<CWin32Font *> m_Win32Fonts;
};

// singleton accessor
extern CFontManager &FontManager();


#endif // FONTMANAGER_H
