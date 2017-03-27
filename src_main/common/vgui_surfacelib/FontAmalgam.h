//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FONTAMALGAM_H
#define FONTAMALGAM_H
#ifdef _WIN32
#pragma once
#endif

#include "Win32Font.h"
#include "UtlVector.h"

//-----------------------------------------------------------------------------
// Purpose: object that holds a set of fonts in specified ranges
//-----------------------------------------------------------------------------
class CFontAmalgam
{
public:
	CFontAmalgam();
	~CFontAmalgam();

	const char *Name();
	void SetName(const char *name);

	// adds a font to the amalgam
	void AddFont(CWin32Font *font, int lowRange, int highRange);

	// returns the font for the given character
	CWin32Font *GetFontForChar(int ch);

	// returns the max height of the font set
	int GetFontHeight();

	// returns the maximum width of a character in a font
	int GetFontMaxWidth();

	// returns the low and high range of the first font in this set
	int GetFontLowRange(int i);
	int GetFontHighRange(int i);

	// returns the flags used to make the first font
	int GetFlags(int i);

	// returns the windows name for the font
	const char *GetFontName(int i);

	// returns the number of fonts in this amalgam
	int GetCount();

private:
	struct TFontRange
	{
		int lowRange;
		int highRange;
		CWin32Font *font;
	};

	CUtlVector<TFontRange> m_Fonts;
	char m_szName[32];
	int m_iMaxWidth;
	int m_iMaxHeight;
};


#endif // FONTAMALGAM_H
