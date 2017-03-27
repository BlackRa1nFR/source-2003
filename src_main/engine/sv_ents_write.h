//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: This module holds all the code the server uses to encode entity
//          deltas.
//
// $NoKeywords: $
//=============================================================================

#ifndef SV_ENTS_WRITE_H
#define SV_ENTS_WRITE_H
#ifdef _WIN32
#pragma once
#endif


#include "server.h"
#include "framesnapshot.h"


int SV_CreatePacketEntities( 
	sv_delta_t type, 
	client_t *client, 
	client_frame_t *to, 
	CFrameSnapshot *to_snapshot, 
	bf_write *pBuf );

void SV_DereferenceUnusedSnapshots( client_t *client, int start );

// Returns false if the client's current delta_sequence can't be used as a 'from' frame for a 
// delta packet. This can happen when an uncompressed packet is sent. It invalidates all the 
// frames older than host_framecount because you can't delta from those frames anymore.
bool SV_IsClientDeltaSequenceValid( client_t *pClient );


#endif // SV_ENTS_WRITE_H
