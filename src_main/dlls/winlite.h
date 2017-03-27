// winlite.h
// this is windows.h with all the unusual stuff cut out

#ifndef __WINLITE_H
#define __WINLITE_H

// Prevent tons of unused windows definitions
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include "windows.h"

#include "basetypes.h"


#endif // __WINLITE_H