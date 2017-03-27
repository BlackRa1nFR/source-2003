//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ICREATEVFONT_H
#define ICREATEVFONT_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class ICreateVFont
{
public:
	virtual bool CreateVFont( char *gamedir,
		char *pOutName, 
		int maxdimension, 
		char *pFontName, 
		int pointSize, 
		bool bItalic, 
		int weight, 
		bool bUnderline, 
		bool bSymbol, 
		int darkborder, 
		int *bordercolor,
		bool additive,
		bool overbright ) = 0;
};

#define ICREATEVFONT_VERSION_STRING "CREATEVFONT001"

#endif // ICREATEVFONT_H
