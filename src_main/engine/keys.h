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
//=============================================================================
	   
#ifndef KEYS_H
#define KEYS_H
#pragma once

#include "keydefs.h"
#include "filesystem.h"

extern	char		*keybindings[256];
extern	int			key_repeats[256];

void		Key_Event (int key, bool down);
void		Key_ClearStates (void);

void		Key_Init (void);
void		Key_Shutdown( void );
void		Key_WriteBindings (FileHandle_t f);
int			Key_CountBindings ( void );
void		Key_SetBinding (int keynum, char *binding);
const char	*Key_BindingForKey( int keynum );
const char	*Key_NameForBinding( const char *pBinding );
char		*Key_KeynumToString (int keynum);
int			Key_StringToKeynum (char *str);


#endif
