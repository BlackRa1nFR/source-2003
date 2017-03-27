
//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Expose functions from sv_main.cpp.
//
// $NoKeywords: $
//=============================================================================

#ifndef SV_MAIN_H
#define SV_MAIN_H


#include "edict.h"
#include "server.h"
#include "packed_entity.h"
#include "utlvector.h"


// This is extra spew to the files cltrace.txt + svtrace.txt
//#define DEBUG_NETWORKING 1

#if defined( DEBUG_NETWORKING )
#define TRACE_PACKET( text ) if ( sv_packettrace.GetInt() ) { SpewToFile text ; };
#else
#define TRACE_PACKET( text )
#endif


class ServerClass;

// Call this to free data and wipe the client_t structure.
void SV_ClearClientStructure(client_t *cl);

// Call the specified function for each client in svs.clients (0 to maxclients).
void SV_MapOverClients(void (*fn)(client_t *));

// Clear all client net channels.
// Also gets rid of all fake clients (bots).
void SV_InactivateClients( void );

// Get the client's index (into svs.clients).
int SV_GetClientIndex(client_t *cl);

// Builds an alternate copy of the datatable for any classes that have datatables with props excluded.
void SV_InitSendTables( ServerClass *pClasses );
void SV_TermSendTables( ServerClass *pClasses );

// Get the instance baseline for a class. 
bool SV_GetInstanceBaseline( ServerClass *pClass, void const **pData, int *pDatalen );

// Write svc_time message to specified client
void SV_WriteServerTimestamp( bf_write& msg, client_t *client );

/*
=============
Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
void SV_ResetPVS( byte* pvs );
void SV_AddOriginToPVS( const Vector& origin );

void SV_ClearBaselines();

extern CGlobalVars g_ServerGlobalVariables;
extern PackedDataAllocator g_PackedDataAllocator;

// Which areas are we going to transmit (usually 1, but with portals you can see into multiple other areas).
extern CUtlVector<int> g_AreasNetworked;


#endif


