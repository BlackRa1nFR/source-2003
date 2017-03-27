//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The main vstdlib library interfaces
//=============================================================================
	   
#ifndef VSTDLIB_H
#define VSTDLIB_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------
#ifdef VSTDLIB_DLL_EXPORT
#define VSTDLIB_INTERFACE	DLL_EXPORT
#define VSTDLIB_OVERLOAD	DLL_GLOBAL_EXPORT
#define VSTDLIB_CLASS		DLL_CLASS_EXPORT
#define VSTDLIB_GLOBAL		DLL_GLOBAL_EXPORT
#else
#define VSTDLIB_INTERFACE	DLL_IMPORT
#define VSTDLIB_OVERLOAD	DLL_GLOBAL_IMPORT
#define VSTDLIB_CLASS		DLL_CLASS_IMPORT
#define VSTDLIB_GLOBAL		DLL_GLOBAL_IMPORT
#endif
 

#endif // VSTDLIB_H
