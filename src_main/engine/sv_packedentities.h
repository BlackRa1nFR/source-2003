//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SV_PACKEDENTITIES_H
#define SV_PACKEDENTITIES_H
#ifdef _WIN32
#pragma once
#endif


#include "server.h"
#include "framesnapshot.h"
#include "server_class.h"


void SV_ComputeClientPacks( 
	int clientCount, 
	client_t** clients,
	CFrameSnapshot *snapshot, 
	client_frame_t **pPack );

void SV_EmitPacketEntities( client_t *client, client_frame_t *to, CFrameSnapshot *to_snapshot, bf_write *msg );

void SV_WriteSendTables( ServerClass *pClasses, bf_write *pBuf );

void SV_WriteClassInfos( ServerClass *pClasses, bf_write *pBuf );

void SV_ComputeClassInfosCRC( CRC32_t* crc );


#endif // SV_PACKEDENTITIES_H
