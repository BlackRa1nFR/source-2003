//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef DT_COMMON_ENG_H
#define DT_COMMON_ENG_H
#ifdef _WIN32
#pragma once
#endif

class CClientState;

// For shortcutting when server and client have the same game .dll
//  data
void DataTable_CreateClientTablesFromServerTables();
void DataTable_CreateClientClassInfosFromServerClasses( CClientState *pState );

#endif // DT_COMMON_ENG_H
