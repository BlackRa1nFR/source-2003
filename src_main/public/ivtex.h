//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IVTEX_H
#define IVTEX_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IVTex
{
public:
	virtual int VTex( int argc, char **argv ) = 0;
};

#define IVTEX_VERSION_STRING "VTEX001"

#endif // IVTEX_H
