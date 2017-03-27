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

#ifndef FONT_GL_H
#define FONT_GL_H
#pragma once

struct fontchar_s
{
	int		width;
	float	u, v;
	float	u2, v2;
};

class CFontGL
{
public:
	CFontGL( void ) : m_init(0) {}
	~CFontGL( void );


	void			Init( const char *pFontName, int pointSize );
	void			Print( const char *pText, float x, float y, unsigned char r, unsigned char g, unsigned char b );

private:
	int				m_init;			// 1 if initialized, zero otherwise
	unsigned int	m_textureId;	// GL texture ID
	int				m_height;		// pixel height of the font
	fontchar_s		m_chars[256];	// per-character data
};


#endif // FONT_GL_H
