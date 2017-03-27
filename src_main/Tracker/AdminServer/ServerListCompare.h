//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERLISTCOMPARE_H
#define SERVERLISTCOMPARE_H
#ifdef _WIN32
#pragma once
#endif

// these functions are common to most server lists in sorts
int __cdecl PasswordCompare(const void *elem1, const void *elem2 );
int __cdecl PingCompare(const void *elem1, const void *elem2 );
int __cdecl PlayersCompare(const void *elem1, const void *elem2 );
int __cdecl MapCompare(const void *elem1, const void *elem2 );
int __cdecl GameCompare(const void *elem1, const void *elem2 );
int __cdecl ServerNameCompare(const void *elem1, const void *elem2 );

#endif // SERVERLISTCOMPARE_H
