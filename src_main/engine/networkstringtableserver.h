//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETWORKSTRINGTABLESERVER_H
#define NETWORKSTRINGTABLESERVER_H
#ifdef _WIN32
#pragma once
#endif

void SV_CreateNetworkStringTables( void );

// Called after level loading has completed
void SV_WriteNetworkStringTablesToBuffer( bf_write *msg);
void SV_UpdateStringTables( struct client_s *cl, bf_write *msg );
void SV_UpdateAcknowledgedFramecount( client_t *cl );
void SV_CheckDirectUpdate( client_t *cl );

void SV_PrintStringTables( void );

#endif // NETWORKSTRINGTABLESERVER_H
