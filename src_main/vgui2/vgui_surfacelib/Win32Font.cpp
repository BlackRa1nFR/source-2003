//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma warning( disable : 4244 ) // conversion from 'double' to 'float', possible loss of data

#include "vgui_surfacelib/Win32Font.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <tier0/dbg.h>


#include <vgui/ISurface.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWin32Font::CWin32Font()
{
	m_szName[0] = 0;
	m_iTall = 0;
	m_iWeight = 0;
	m_iFlags = 0;
	m_iMaxCharWidth = 0;
	m_hFont = NULL;
	m_hDC = NULL;
	m_hDIB = NULL;
	m_bAntiAliased = false;
	m_iBlur = 0;
	m_pGaussianDistribution = NULL;
	m_iScanLines = 0;
	m_bRotary = false;
	m_bAdditive = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWin32Font::~CWin32Font()
{
	if (m_hFont)
		::DeleteObject(m_hFont);

	if (m_hDC)
		::DeleteDC(m_hDC);

	if (m_hDIB)
		::DeleteObject(m_hDIB);
}

extern bool g_bFontFound = false;
//-----------------------------------------------------------------------------
// Purpose: Font iteration callback function
//			used to determine whether or not a font exists on the system
//-----------------------------------------------------------------------------
int CALLBACK FontEnumProc( const LOGFONT *lpelfe,    // logical-font data
  const TEXTMETRIC *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
)
{
	g_bFontFound = true;
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: creates the font from windows.  returns false if font does not exist in the OS.
//-----------------------------------------------------------------------------
bool CWin32Font::Create(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
{
	strncpy(m_szName, windowsFontName, sizeof(m_szName) - 1);
	m_szName[sizeof(m_szName) - 1] = 0;
	m_iTall = tall;
	m_iWeight = weight;
	m_iFlags = flags;
	m_bAntiAliased = flags & vgui::ISurface::FONTFLAG_ANTIALIAS;
	m_iDropShadowOffset = (flags & vgui::ISurface::FONTFLAG_DROPSHADOW) ? 1 : 0;
	m_iOutlineSize = (flags & vgui::ISurface::FONTFLAG_OUTLINE) ? 1 : 0;
	m_iBlur = blur;
	m_iScanLines = scanlines;
	m_bRotary = flags & vgui::ISurface::FONTFLAG_ROTARY;
	m_bAdditive = flags & vgui::ISurface::FONTFLAG_ADDITIVE;

	int charset = (flags & vgui::ISurface::FONTFLAG_SYMBOL) ? SYMBOL_CHARSET : ANSI_CHARSET;

	m_hDC = ::CreateCompatibleDC(NULL);
	Assert( m_hDC );

	// see if the font exists on the system
	LOGFONT logfont;
	logfont.lfCharSet = DEFAULT_CHARSET;
	logfont.lfPitchAndFamily = 0;
	strcpy(logfont.lfFaceName, windowsFontName);
	g_bFontFound = false;
	::EnumFontFamiliesEx(m_hDC, &logfont, &FontEnumProc, 0, 0);
	if (!g_bFontFound)
	{
		Msg("Couldn't find windows font '%s' to load\n", windowsFontName);
		m_szName[0] = 0;
		return false;
	}

	m_hFont = ::CreateFontA(tall, 0, 0, 0, 
								m_iWeight, 
								flags & vgui::ISurface::FONTFLAG_ITALIC, 
								flags & vgui::ISurface::FONTFLAG_UNDERLINE, 
								flags & vgui::ISurface::FONTFLAG_STRIKEOUT, 
								charset, 
								OUT_DEFAULT_PRECIS, 
								CLIP_DEFAULT_PRECIS, 
								m_bAntiAliased ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY, 
								DEFAULT_PITCH | FF_DONTCARE, 
								m_szName);

	if (!m_hFont)
	{
		m_szName[0] = 0;
		return false;
	}

	// set as the active font
	::SelectObject(m_hDC, m_hFont);
	::SetTextAlign(m_hDC, TA_LEFT | TA_TOP | TA_UPDATECP);

	// get info about the font
	::TEXTMETRIC tm;
	memset( &tm, 0, sizeof( tm ) );

	if ( !GetTextMetrics(m_hDC, &tm) )
	{
		m_szName[0] = 0;
		return false;
	}

	m_iHeight = tm.tmHeight + m_iDropShadowOffset + 2 * m_iOutlineSize;
	m_iMaxCharWidth = tm.tmMaxCharWidth;
	m_iAscent = tm.tmAscent;

	// code for rendering to a bitmap
	m_rgiBitmapSize[0] = tm.tmMaxCharWidth + m_iOutlineSize * 2;
	m_rgiBitmapSize[1] = tm.tmHeight + m_iDropShadowOffset + m_iOutlineSize * 2;

	::BITMAPINFOHEADER header;
	memset(&header, 0, sizeof(header));
	header.biSize = sizeof(header);
	header.biWidth = m_rgiBitmapSize[0];
	header.biHeight = -m_rgiBitmapSize[1];
	header.biPlanes = 1;
	header.biBitCount = 32;
	header.biCompression = BI_RGB;

	m_hDIB = ::CreateDIBSection(m_hDC, (BITMAPINFO*)&header, DIB_RGB_COLORS, (void**)(&m_pBuf), NULL, 0);
	::SelectObject(m_hDC, m_hDIB);

	// get char spacing
	// a is space before character (can be negative)
	// b is the width of the character
	// c is the space after the character
	memset(m_ABCWidthsCache, 0, sizeof(m_ABCWidthsCache));
	ABC abc[ABCWIDTHS_CACHE_SIZE];
	assert(ABCWIDTHS_CACHE_SIZE <= 256);
	if (::GetCharABCWidthsW(m_hDC, 0, ABCWIDTHS_CACHE_SIZE - 1, &abc[0]) || ::GetCharABCWidthsA(m_hDC, 0, ABCWIDTHS_CACHE_SIZE - 1, &abc[0]))
	{	
		// copy out into our formated structure
		for (int i = 0; i < ABCWIDTHS_CACHE_SIZE; i++)
		{
			m_ABCWidthsCache[i].a = abc[i].abcA - m_iBlur - m_iOutlineSize;
			m_ABCWidthsCache[i].b = abc[i].abcB + ((m_iBlur + m_iOutlineSize) * 2) + m_iDropShadowOffset;
			m_ABCWidthsCache[i].c = abc[i].abcC - m_iBlur - m_iDropShadowOffset - m_iOutlineSize;
		}
	}
	else
	{
		// since that failed, it must be fixed width, zero everything so a and c will be zeros, then
		// fill b with the value from TEXTMETRIC
		for (int i = 0; i < ABCWIDTHS_CACHE_SIZE; i++)
		{
			m_ABCWidthsCache[i].b = (char)tm.tmAveCharWidth;
		}
	}

	// calculate our gaussian distribution for if we're blurred
	if (m_iBlur > 1)
	{
		m_pGaussianDistribution = new float[m_iBlur * 2 + 1];
		double sigma = 0.683 * m_iBlur;
		for (int x = 0; x <= (m_iBlur * 2); x++)
		{
			int val = x - m_iBlur;
			m_pGaussianDistribution[x] = (float)(1.0f / sqrt(2 * 3.14 * sigma * sigma)) * pow(2.7, -1 * (val * val) / (2 * sigma * sigma));

			// brightening factor
			m_pGaussianDistribution[x] *= 1;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: writes the char into the specified 32bpp texture
//-----------------------------------------------------------------------------
void CWin32Font::GetCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	int a, b, c;
	GetCharABCWidths(ch, a, b, c);

	// set us up to render into our dib
	::SelectObject(m_hDC, m_hFont);

	int wide = b;
	int tall = m_iHeight;
	GLYPHMETRICS glyphMetrics;
	MAT2 mat2 = { { 0, 1}, { 0, 0}, { 0, 0}, { 0, 1}};
	int bytesNeeded = 0;

	if (m_bAntiAliased)
	{
		// try and get the glyph directly
		::SelectObject(m_hDC, m_hFont);
		bytesNeeded = ::GetGlyphOutline(m_hDC, ch, GGO_GRAY8_BITMAP, &glyphMetrics, 0, NULL, &mat2);
	}

	if (bytesNeeded > 0)
	{
		// take it
		unsigned char *lpbuf = (unsigned char *)_alloca(bytesNeeded);
		::GetGlyphOutline(m_hDC, ch, GGO_GRAY8_BITMAP, &glyphMetrics, bytesNeeded, lpbuf, &mat2);

		// rows are on DWORD boundaries
		wide = glyphMetrics.gmBlackBoxX;
		while (wide % 4 != 0)
		{
			wide++;
		}

		// see where we should start rendering
		int pushDown = m_iAscent - glyphMetrics.gmptGlyphOrigin.y;

		// set where we start copying from
		int xstart = 0;

		// don't copy the first set of pixels if the antialiased bmp is bigger than the char width
		if ((int)glyphMetrics.gmBlackBoxX >= b + 2)
		{
			xstart = (glyphMetrics.gmBlackBoxX - b) / 2;
		}

		// iterate through copying the generated dib into the texture
		for (unsigned int j = 0; j < glyphMetrics.gmBlackBoxY; j++)
		{
			for (unsigned int i = xstart; i < glyphMetrics.gmBlackBoxX; i++)
			{
				int x = rgbaX + i - xstart + m_iBlur + m_iOutlineSize;
				int y = rgbaY + j + pushDown;
				if ((x < rgbaWide) && (y < rgbaTall))
				{
					unsigned char grayscale = lpbuf[(j*wide+i)];

					float r, g, b, a;
					if (grayscale)
					{
						r = g = b = 1.0f;
						a = (grayscale + 0) / 64.0f;
						if (a > 1.0f) a = 1.0f;
					}
					else
					{
						r = g = b = a = 0.0f;
					}

					// Don't want anything drawn for tab characters.
					if (ch == '\t')
					{
						r = g = b = 0;
					}

					unsigned char *dst = &rgba[(y*rgbaWide+x)*4];
					dst[0] = (unsigned char)(r * 255.0f);
					dst[1] = (unsigned char)(g * 255.0f);
					dst[2] = (unsigned char)(b * 255.0f);
					dst[3] = (unsigned char)(a * 255.0f);
				}
			}
		}
	}
	else
	{
		// use render-to-bitmap to get our font texture
		::SetBkColor(m_hDC, RGB(0, 0, 0));
		::SetTextColor(m_hDC, RGB(255, 255, 255));
		::SetBkMode(m_hDC, OPAQUE);
		::MoveToEx(m_hDC, -a, 0, NULL);

		// render the character
		wchar_t wch = (wchar_t)ch;
		::ExtTextOutW(m_hDC, 0, 0, 0, NULL, &wch, 1, NULL);
		::SetBkMode(m_hDC, TRANSPARENT);

		if (wide > m_rgiBitmapSize[0])
		{
			wide = m_rgiBitmapSize[0];
		}
		if (tall > m_rgiBitmapSize[1])
		{
			tall = m_rgiBitmapSize[1];
		}

		// iterate through copying the generated dib into the texture
		for (int j = 0; j < tall; j++)
		{
			for (int i = 0; i < wide - m_iDropShadowOffset; i++)
			{
				int x = rgbaX + i;
				int y = rgbaY + j;
				if ((x < rgbaWide) && (y < rgbaTall))
				{
					unsigned char *src = &m_pBuf[(j*m_rgiBitmapSize[0]+i)*4];

					float r = (src[0]) / 255.0f;
					float g = (src[1]) / 255.0f;
					float b = (src[2]) / 255.0f;

					// Don't want anything drawn for tab characters.
					if (ch == '\t')
					{
						r = g = b = 0;
					}

					unsigned char *dst = &rgba[(y*rgbaWide+x)*4];
					dst[0] = (unsigned char)(r * 255.0f);
					dst[1] = (unsigned char)(g * 255.0f);
					dst[2] = (unsigned char)(b * 255.0f);
					dst[3] = (unsigned char)((r * 0.34f + g * 0.55f + b * 0.11f) * 255.0f);
				}
			}
		}
	}

	if (m_iDropShadowOffset)
	{
		ApplyDropShadowToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, wide, tall, rgba);
	}
	if( m_iOutlineSize )
	{
		ApplyOutlineToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, wide, tall, rgba);
	}

	// apply a gaussian blur to the generated texture
	ApplyGaussianBlurToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, rgba);
	// apply scanline effect
	ApplyScanlineEffectToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, rgba);
	// Apply rotary effect
	ApplyRotaryEffectToTexture(rgbaX, rgbaY, rgbaWide, rgbaTall, rgba );
}

//-----------------------------------------------------------------------------
// Purpose: Adds center line to fond
//-----------------------------------------------------------------------------
void CWin32Font::ApplyRotaryEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	if ( !m_bRotary )
		return;

	int y = rgbaTall * 0.5;

	unsigned char *line = &rgba[(rgbaX + ((y + rgbaY) * rgbaWide)) * 4];

	// Draw a line down middle
	for (int x = 0; x < rgbaWide; x++, line+=4)
	{
		line[0] = 127;
		line[1] = 127;
		line[2] = 127;
		line[3] = 255;
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds scanlines to the texture
//-----------------------------------------------------------------------------
void CWin32Font::ApplyScanlineEffectToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	if (m_iScanLines < 2)
		return;

	// darken all the areas except the scanlines
	for (int y = 0; y < rgbaTall; y++)
	{
		// skip the scan lines
		if (y % m_iScanLines == 0)
			continue;

		unsigned char *line = &rgba[(rgbaX + ((y + rgbaY) * rgbaWide)) * 4];

		// darken the other lines
		for (int x = 0; x < rgbaWide; x++, line += 4)
		{
			line[0] *= 0.7;
			line[1] *= 0.7;
			line[2] *= 0.7;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds a dropshadow the the font texture
//-----------------------------------------------------------------------------
void CWin32Font::ApplyDropShadowToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba)
{
	// walk the original image from the bottom up
	// shifting it down and right, and turning it black (the dropshadow)
	for (int y = charTall - 1; y >= m_iDropShadowOffset; y--)
	{
		for (int x = charWide - 1; x >= m_iDropShadowOffset; x--)
		{
			unsigned char *dest = &rgba[(x + rgbaX + ((rgbaY + y) * rgbaWide)) * 4];
			if (dest[3] == 0)
			{
				// there is nothing in this spot, copy in the dropshadow
				unsigned char *src = &rgba[(x + rgbaX - m_iDropShadowOffset + ((rgbaY + y - m_iDropShadowOffset) * rgbaWide)) * 4];
				dest[0] = 0;
				dest[1] = 0;
				dest[2] = 0;
				dest[3] = src[3];
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds an outline to the font texture
//-----------------------------------------------------------------------------
void CWin32Font::ApplyOutlineToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall, unsigned char *rgba)
{
	int x, y;
	for( y = 0; y < charTall; y++ )
	{
		for( x = 0; x < charWide; x++ )
		{
			unsigned char *src = &rgba[(x + rgbaX + ((rgbaY + y) * rgbaWide)) * 4];
			if( src[3] == 0 )
			{
				// We have a valid font texel.  Make all the alpha == 0 neighbors black.
				int shadowX, shadowY;
				for( shadowX = -m_iOutlineSize; shadowX <= m_iOutlineSize; shadowX++ )
				{
					for( shadowY = -m_iOutlineSize; shadowY <= m_iOutlineSize; shadowY++ )
					{
						if( shadowX == 0 && shadowY == 0 )
						{
							continue;
						}
						int testX, testY;
						testX = shadowX + x;
						testY = shadowY + y;
						if( testX < 0 || testX >= charWide ||
							testY < 0 || testY >= charTall )
						{
							continue;
						}
						unsigned char *test = &rgba[(testX + rgbaX + ((rgbaY + testY) * rgbaWide)) * 4];
						if( test[0] != 0 && test[1] != 0 && test[2] != 0 && test[3] != 0 )
						{
							src[0] = 0;
							src[1] = 0;
							src[2] = 0;
							src[3] = 255;
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: blurs the texture
//-----------------------------------------------------------------------------
void CWin32Font::ApplyGaussianBlurToTexture(int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba)
{
	// generate the gaussian field
	if (!m_pGaussianDistribution)
		return;

	// alloc a new buffer
	unsigned char *src = (unsigned char *)_alloca(rgbaWide * rgbaTall * 4);
	// copy in
	memcpy(src, rgba, rgbaWide * rgbaTall * 4);
	// incrementing destination pointer
	unsigned char *dest = rgba;
	for (int y = 0; y < rgbaTall; y++)
	{
		for (int x = 0; x < rgbaWide; x++)
		{
			// scan the source pixel
			GetBlurValueForPixel(src, m_iBlur, m_pGaussianDistribution, x, y, rgbaWide, rgbaTall, dest);

			// move to the next
			dest += 4;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the blur value for a single pixel
//-----------------------------------------------------------------------------
void CWin32Font::GetBlurValueForPixel(unsigned char *src, int blur, float *gaussianDistribution, int srcX, int srcY, int srcWide, int srcTall, unsigned char *dest)
{
	int r = 0, g = 0, b = 0, a = 0;
	
	float accum = 0.0f;

	// scan the positive x direction
	int maxX = min(srcX + blur, srcWide);
	int minX = max(srcX - blur, 0);
	for (int x = minX; x <= maxX; x++)
	{
		int maxY = min(srcY + blur, srcTall - 1);
		int minY = max(srcY - blur, 0);
		for (int y = minY; y <= maxY; y++)
		{
			unsigned char *srcPos = src + ((x + (y * srcWide)) * 4);

			// muliply by the value matrix
			float weight = gaussianDistribution[x - srcX + blur];
			float weight2 = gaussianDistribution[y - srcY + blur];
			accum += (srcPos[0] * (weight * weight2));
		}
	}

	// all the values are the same for fonts, just use the calculated alpha
	r = g = b = a = (int)accum;

	int pixelCount = 1;
//	Assert(r <= 255);
	dest[0] = min(r / pixelCount, 255);
	dest[1] = min(g / pixelCount, 255);
	dest[2] = min(b / pixelCount, 255);
	dest[3] = min(a / pixelCount, 255);
	dest[3] = 255;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the font is equivalent to that specified
//-----------------------------------------------------------------------------
bool CWin32Font::IsEqualTo(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
{
	if (!stricmp(windowsFontName, m_szName) 
		&& m_iTall == tall
		&& m_iWeight == weight
		&& m_iBlur == blur
		&& m_iFlags == flags)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true only if this font is valid for use
//-----------------------------------------------------------------------------
bool CWin32Font::IsValid()
{
	if (m_szName[0])
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: set the font to be the one to currently draw with in the gdi
//-----------------------------------------------------------------------------
void CWin32Font::SetAsActiveFont(HDC hdc)
{
	assert(IsValid());
	::SelectObject(hdc, m_hFont);
}

//-----------------------------------------------------------------------------
// Purpose: gets the abc widths for a character
//-----------------------------------------------------------------------------
void CWin32Font::GetCharABCWidths(int ch, int &a, int &b, int &c)
{
	assert(IsValid());
	if (ch < ABCWIDTHS_CACHE_SIZE)
	{
		// use the cache entry
		a = m_ABCWidthsCache[ch].a;
		b = m_ABCWidthsCache[ch].b;
		c = m_ABCWidthsCache[ch].c;
	}
	else
	{
		ABC abc;
		if (::GetCharABCWidthsW(m_hDC, ch, ch, &abc))
		{
			a = abc.abcA;
			b = abc.abcB;
			c = abc.abcC;
		}
		else
		{
			// failed to get width, just use the max width
			a = c = 0;
			b = m_iMaxCharWidth;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the height of the font, in pixels
//-----------------------------------------------------------------------------
int CWin32Font::GetHeight()
{
	assert(IsValid());
	return m_iHeight;
}

//-----------------------------------------------------------------------------
// Purpose: returns the maximum width of a character, in pixels
//-----------------------------------------------------------------------------
int CWin32Font::GetMaxCharWidth()
{
	assert(IsValid());
	return m_iMaxCharWidth;
}

//-----------------------------------------------------------------------------
// Purpose: returns the flags used to make this font, used by the dynamic resizing code
//-----------------------------------------------------------------------------
int CWin32Font::GetFlags()
{
	return m_iFlags;
}

