//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vgui_surfacelib/FontAmalgam.h"
#include <tier0/dbg.h>
#include <vgui/VGUI.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFontAmalgam::CFontAmalgam()
{
	m_Fonts.EnsureCapacity(4);
	m_iMaxHeight = 0;
	m_iMaxWidth = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CFontAmalgam::~CFontAmalgam()
{
}

//-----------------------------------------------------------------------------
// Purpose: Data accessor
//-----------------------------------------------------------------------------
const char *CFontAmalgam::Name()
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: Data accessor
//-----------------------------------------------------------------------------
void CFontAmalgam::SetName(const char *name)
{
	strncpy(m_szName, name, sizeof(m_szName) - 1);
	m_szName[sizeof(m_szName) - 1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: adds a font to the amalgam
//-----------------------------------------------------------------------------
void CFontAmalgam::AddFont(CWin32Font *font, int lowRange, int highRange)
{
	int i = m_Fonts.AddToTail();

	m_Fonts[i].font = font;
	m_Fonts[i].lowRange = lowRange;
	m_Fonts[i].highRange = highRange;

	m_iMaxHeight = max(font->GetHeight(), m_iMaxHeight);
	m_iMaxWidth = max(font->GetMaxCharWidth(), m_iMaxWidth);
}

//-----------------------------------------------------------------------------
// Purpose: returns the font for the given character
//-----------------------------------------------------------------------------
CWin32Font *CFontAmalgam::GetFontForChar(int ch)
{
	for (int i = 0; i < m_Fonts.Count(); i++)
	{
		if (ch >= m_Fonts[i].lowRange && ch <= m_Fonts[i].highRange)
		{
			Assert(m_Fonts[i].font->IsValid());
			return m_Fonts[i].font;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of the font set
//-----------------------------------------------------------------------------
int CFontAmalgam::GetFontHeight()
{
	return m_iMaxHeight;
}

//-----------------------------------------------------------------------------
// Purpose: returns the maximum width of a character in a font
//-----------------------------------------------------------------------------
int CFontAmalgam::GetFontMaxWidth()
{
	return m_iMaxWidth;
}

//-----------------------------------------------------------------------------
// Purpose: returns the low range value of the font
//-----------------------------------------------------------------------------
int CFontAmalgam::GetFontLowRange(int i)
{
	return m_Fonts[i].lowRange;
}

//-----------------------------------------------------------------------------
// Purpose: returns the high range of the font
//-----------------------------------------------------------------------------
int CFontAmalgam::GetFontHighRange(int i)
{
	return m_Fonts[i].highRange;
}

//-----------------------------------------------------------------------------
// Purpose: returns the name of the font that is loaded
//-----------------------------------------------------------------------------
const char *CFontAmalgam::GetFontName(int i)
{	
	if( m_Fonts[i].font )
	{
		return m_Fonts[i].font->GetName();
	}
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns the name of the font that is loaded
//-----------------------------------------------------------------------------
int CFontAmalgam::GetFlags(int i)
{	
	if ( m_Fonts.Count() && m_Fonts[i].font )
	{
		return m_Fonts[i].font->GetFlags();
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of fonts this amalgam contains
//-----------------------------------------------------------------------------
int CFontAmalgam::GetCount()
{		
	return m_Fonts.Count();
}

