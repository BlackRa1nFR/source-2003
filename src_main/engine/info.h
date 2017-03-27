//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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
#ifndef INFO_H
#define INFO_H
#ifdef _WIN32
#pragma once
#endif

#include "filesystem.h"

#define	MAX_INFO_STRING			256

void			Info_SetValueForStarKey ( char *s, const char *key, const char *value, int maxsize);
// userinfo functions
const char		*Info_ValueForKey ( const char *s, const char *key );
void			Info_RemoveKey ( char *s, const char *key );
void			Info_RemovePrefixedKeys ( char *start, char prefix );
void			Info_SetValueForKey ( char *s, const char *key, const char *value, int maxsize );
void			Info_Print ( const char *s );

void			Info_WriteVars( char *s, FileHandle_t fp );

#endif // INFO_H
