//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: returns the module handle of the game dll
//			this is in its own file to protect it from tier0 PROTECTED_THINGS
//=============================================================================


#ifdef _WIN32
#include <winlite.h>
extern HMODULE win32DLLHandle;
#elif _LINUX
#include <dlopen.h>
#endif

void *GetGameModuleHandle()
{
#ifdef _WIN32
	return (void *)win32DLLHandle;
#elif _LINUX
#error "implement me!!"
#else
#error "GetGameModuleHandle() needs to be implemented"
#endif
}

