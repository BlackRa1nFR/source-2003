//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: includes windows.h with all the unusual stuff cut out
//=============================================================================

#ifndef WINLITE_H
#define WINLITE_H
#pragma once

// 
// Prevent tons of unused windows definitions
//
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>

#undef PostMessage

#pragma warning( disable: 4800 )	// forcing value to bool 'true' or 'false' (performance warning)

#endif // WINLITE_H
