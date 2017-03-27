//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Basic header for using vgui
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_H
#define VGUI_H

#ifdef _WIN32
#pragma once
#endif

#define null 0L

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#pragma warning( disable: 4800 )	// disables 'performance warning converting int to bool'
#pragma warning( disable: 4786 )	// disables 'identifier truncated in browser information' warning
#pragma warning( disable: 4355 )	// disables 'this' : used in base member initializer list
#pragma warning( disable: 4097 )	// warning C4097: typedef-name 'BaseClass' used as synonym for class-name
#pragma warning( disable: 4514 )	// warning C4514: 'Color::Color' : unreferenced inline function has been removed
#pragma warning( disable: 4100 )	// warning C4100: 'code' : unreferenced formal parameter
#pragma warning( disable: 4127 )	// warning C4127: conditional expression is constant

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

//!! hack alert
// this is done so that the linker can tell the difference between the two versions
// of vgui it uses in the engine, vgui.lib and vgui2.lib
#define vgui	vgui2

// do this in GOLDSRC only!!!
//#define Assert assert

namespace vgui
{
// handle to an internal vgui panel
// this is the only handle to a panel that is valid across dll boundaries
typedef unsigned int VPANEL;

// handles to vgui objects
// NULL values signify an invalid value
typedef unsigned long HScheme;
typedef unsigned long HTexture;
typedef unsigned long HCursor;
typedef unsigned long HPanel;
const HPanel INVALID_PANEL = 0xffffffff;
typedef unsigned long HFont;
const HFont INVALID_FONT = 0; // the value of an invalid font handle
}

#include "vstdlib/strtools.h"


#endif // VGUI_H
