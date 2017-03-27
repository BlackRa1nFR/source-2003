//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VCOLLIDE_PARSE_PRIVATE_H
#define VCOLLIDE_PARSE_PRIVATE_H
#ifdef _WIN32
#pragma once
#endif

#include "vcollide_parse.h"

#define MAX_KEYVALUE	1024

class IVPhysicsKeyParser;

const char			*ParseKeyvalue( const char *pBuffer, char *key, char *value );
IVPhysicsKeyParser	*CreateVPhysicsKeyParser( const char *pKeyData );
void				DestroyVPhysicsKeyParser( IVPhysicsKeyParser * );

#endif // VCOLLIDE_PARSE_PRIVATE_H
