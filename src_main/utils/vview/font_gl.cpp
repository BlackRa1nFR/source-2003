//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "font_gl.h"

// Your app must define these functions
extern unsigned int TextureID_Alloc( void );
extern void TextureID_Free( unsigned int textureId );
extern void TextureID_Bind( unsigned int textureId );


CFontGL::~CFontGL( void )
{
	if ( m_init )
	{
		TextureID_Free( m_textureId );
	}
}

void CFontGL::Init( const char *pFontName, int pointSize )
{
	int i;

	// UNDONE: Do this before mucking with the desktop or create it with a different DC 
	// so that it doesn't have problems on Voodoo Graphics.
	HDC hDC = CreateCompatibleDC( NULL );

	SetMapMode( hDC, MM_TEXT );

	int height = MulDiv(pointSize, GetDeviceCaps( hDC, LOGPIXELSY), 72 );

	HFONT hFont = CreateFont(
	  height,               // logical height of font
	  0,                // logical average character width
	  0,           // angle of escapement
	  0,          // base-line orientation angle
	  FW_DONTCARE,              // font weight
	  0,           // italic attribute flag
	  0,        // underline attribute flag
	  0,        // strikeout attribute flag
	  ANSI_CHARSET,          // character set identifier
	  OUT_DEFAULT_PRECIS,  // output precision
	  CLIP_DEFAULT_PRECIS,    // clipping precision
	  DEFAULT_QUALITY,          // output quality
	  DEFAULT_PITCH,   // pitch and family
	  pFontName          // pointer to typeface name string
	);

	if ( hFont )
	{
		HGDIOBJ oldFont = SelectObject( hDC, hFont );

		// get the exact pixel height of this font
		TEXTMETRIC textMetric;
		GetTextMetrics( hDC, &textMetric );
		height = textMetric.tmHeight;
		m_height = height;

		// width 0 characters are considered non-printable
		int widths[256];
		// read the widths from the truetype font
		GetCharWidth( hDC, 0, 255, widths );

		int textArea = 0;
		for ( i = 0; i < 256; i++ )
		{
			if ( i > 31 && isprint(i) && widths[i] > 0 )
			{
				textArea += (height) * (widths[i] + 1);
			}
			else
			{
				widths[i] = 0;
			}
		}

		// compute the smallest square power of 2 bitmap that can hold this font
		int bitmapSize = 32;
		int bitmapArea = 32*32;
		
		while ( bitmapArea < textArea )
		{
			bitmapSize *= 2;
			bitmapArea = bitmapSize * bitmapSize;
		}

		// create a DIB
		BITMAPINFO bmi;

		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = bitmapSize;
		bmi.bmiHeader.biHeight = bitmapSize;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		
		char *pBits = NULL;
		HBITMAP hDIB = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (void**)&pBits, 0, 0 );

		HGDIOBJ oldDIB = SelectObject( hDC, hDIB );

		// Generate texture id.
		m_textureId = TextureID_Alloc();

		// Render the text to the DIB
		SetTextColor( hDC, (COLORREF)0x00FFFFFF );	// white text 0x00bbggrr
		SetBkColor( hDC, (COLORREF)0x00000000 );	// black background

		// fill background with black
		PatBlt( hDC, 0, 0, bitmapSize, bitmapSize, BLACKNESS );

		int x = 0, y = 0;

		float oobitmapSize = 1.0f / (float)bitmapSize;

		char tmp[4];
		tmp[1] = 0;

		// render each printable character to the DIB (wrap letters)
		for ( i = 0; i < 256; i++ )
		{
			fontchar_s *pLetter = m_chars + i;

			if ( widths[i] )
			{
				tmp[0] = (char)i;
				
				if ( (x + widths[i]) >= bitmapSize )
				{
					x = 0;
					y += height;
				}
				TextOut( hDC, x, y, tmp, 1 );

				// generate texture coordinates for this letter
				pLetter->u = x * oobitmapSize;
				pLetter->u2 = (x+widths[i]) * oobitmapSize;

				// flip v's, the DIB is upside down
				pLetter->v = 1.0f - (y * oobitmapSize);	
				pLetter->v2 = 1.0f - ((y+height-1) * oobitmapSize);

				x += widths[i] + 1;
				if ( (x + widths[i]) >= bitmapSize )
				{
					x = 0;
					y += height;
				}
				pLetter->width = widths[i];

			}
			else
			{
				pLetter->width = 0;
			}
		}

		// add the alpha channel 
		unsigned char *pOut = (unsigned char *)pBits;
		for ( i = 0; i < bitmapSize*bitmapSize; i++ )
		{
			if ( pOut[0] || pOut[1] || pOut[2] )
			{
				// filled pixel
				pOut[3] = 255U;
			}
			else
			{
				// empty pixel
				pOut[3] = 0;
			}
			pOut += 4;
		}

		// upload the font texture to GL
		glBindTexture( GL_TEXTURE_2D, m_textureId );
		glTexImage2D( GL_TEXTURE_2D, 0, 4, bitmapSize, bitmapSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pBits );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

		// clean up the DC
		SelectObject( hDC, oldDIB );
		SelectObject( hDC, oldFont );

		DeleteObject( hFont );
		DeleteObject( hDIB );
		m_init = true;
	}
	DeleteDC( hDC );
}


void CFontGL::Print( const char *pText, float x, float y, unsigned char r, unsigned char g, unsigned char b )
{
	if ( !m_init )
		return;

	// draw the string
	fontchar_s *pLetter;
	unsigned char letter;

	glColor3ub( r, g, b );
	glEnable( GL_ALPHA_TEST );
	TextureID_Bind( m_textureId );
	do 
	{
		letter = *pText++;
		pLetter = m_chars + letter;
		if ( letter && pLetter->width )
		{
			glBegin( GL_QUADS );

			glTexCoord2f( pLetter->u, pLetter->v );
			glVertex2f( x, y );

			glTexCoord2f( pLetter->u, pLetter->v2 );
			glVertex2f( x, y + m_height );

			glTexCoord2f( pLetter->u2, pLetter->v2 );
			glVertex2f( x + pLetter->width, y + m_height );

			glTexCoord2f( pLetter->u2, pLetter->v );
			glVertex2f( x + pLetter->width, y );

			glEnd();

			x += pLetter->width + 1;
		}
	} while ( letter );

	glDisable( GL_ALPHA_TEST );
	glEnable( GL_TEXTURE_2D );
	// draw the whole font
#if 0
	glColor3ub( r, g, b );
	glEnable( GL_ALPHA_TEST );
	glBindTexture( GL_TEXTURE_2D, m_textureId );
	glBegin( GL_QUADS );

	glTexCoord2f( 0, 1 );
	glVertex2f( x, y );

	glTexCoord2f( 0, 0 );
	glVertex2f( x, y + 256 );

	glTexCoord2f( 1, 0 );
	glVertex2f( x + 256, y + 256 );

	glTexCoord2f( 1, 1 );
	glVertex2f( x + 256, y );

	glEnd();
	glDisable( GL_ALPHA_TEST );
#endif
}


