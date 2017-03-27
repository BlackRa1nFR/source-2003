//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DT_LOCALTRANSFER_H
#define DT_LOCALTRANSFER_H
#ifdef _WIN32
#pragma once
#endif


#include "dt_send.h"
#include "dt_recv.h"


// This sets up the ability to copy an entity with the specified SendTable directly
// into an entity with the specified RecvTable, thus avoiding compression overhead.
bool LocalTransfer_Init( SendTable *pSendTable, RecvTable *pRecvTable );

// Transfer the data from pSrcEnt to pDestEnt using the specified SendTable and RecvTable.
void LocalTransfer_TransferEntity( const SendTable *pSendTable, const void *pSrcEnt, RecvTable *pRecvTable, void *pDestEnt, int objectID );


#endif // DT_LOCALTRANSFER_H
