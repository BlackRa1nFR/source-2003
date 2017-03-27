//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETWORKSTRINGTABLEDEFS_H
#define NETWORKSTRINGTABLEDEFS_H
#ifdef _WIN32
#pragma once
#endif

typedef int TABLEID;

#define INVALID_STRING_TABLE -1
const unsigned short INVALID_STRING_INDEX = (unsigned short )-1;

// table index is sent in log2(MAX_TABLES) bits
#define MAX_TABLES	16  // Table id is 4 bits

#endif // NETWORKSTRINGTABLEDEFS_H
