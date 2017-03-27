//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef WIN32FONT_H
#define WIN32FONT_H
#ifdef _WIN32
#pragma once
#endif

#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <windows.h>

#ifdef GetCharABCWidths
#undef GetCharABCWidths
#endif

//-----------------------------------------------------------------------------
// Purpose: encapsulates a windows font
//-----------------------------------------------------------------------------
class CWin32Font
{
public:
	CWin32Font();
	~CWin32Font();

	// creates the font from windows.  returns false if font does not exist in the OS.
	bool Create(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags);

	// writes the char into the specified 32bpp texture
	void GetCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);

	// returns true if the font is equivalent to that specified
	bool IsEqualTo(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags);

	// returns true only if this font is valid for use
	bool IsValid();

	// gets the abc widths for a character
	void GetCharABCWidths(int ch, int &a, int &b, int &c);

	// set the font to be the one to currently draw with in the gdi
	void SetAsActiveFont(HDC hdc);

	// returns the height of the font, in pixels
	int GetHeight();

	// returns the maximum width of a character, in pixels
	int GetMaxCharWidth();

	// returns the flags used to make this font
	int GetFlags();
	
	// gets the name of this font
	const char *GetName() { return m_szName; }

private:
	void ApplyScanlineEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
	void ApplyGaussianBlurToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
	void ApplyDropShadowToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba);
	void ApplyOutlineToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba);
	void ApplyRotaryEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
	static inline void GetBlurValueForPixel(unsigned char *src, int blur, float *gaussianDistribution, int x, int y, int wide, int tall, unsigned char *dest);

	char m_szName[32];
	int m_iTall;
	int m_iWeight;
	int m_iFlags;
	bool m_bAntiAliased;
	bool m_bRotary;
	bool m_bAdditive;
	int m_iDropShadowOffset;
	int m_iOutlineSize;

	int m_iHeight;
	int m_iMaxCharWidth;
	int m_iAscent;

	// abc widths
	struct abc_t
	{
		char a;
		char b;
		char c;
		char pad;
	};
	enum { ABCWIDTHS_CACHE_SIZE = 256 };
	abc_t m_ABCWidthsCache[ABCWIDTHS_CACHE_SIZE];

	HFONT m_hFont;
	HDC m_hDC;
	HBITMAP m_hDIB;
	int m_rgiBitmapSize[2];
	unsigned char *m_pBuf;	// pointer to buffer for use when generated bitmap versions of a texture
	
	int m_iScanLines;
	int m_iBlur;
	float *m_pGaussianDistribution;
};


#endif // WIN32FONT_H
