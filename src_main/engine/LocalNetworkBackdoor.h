//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef LOCALNETWORKBACKDOOR_H
#define LOCALNETWORKBACKDOOR_H
#ifdef _WIN32
#pragma once
#endif


class SendTable;

void LocalNetworkBackDoor_DirectStringTableUpdate( int tableId, int entryIndex, char const *string,
	int userdatalength, const void *userdata );

// This class facilitates a fast path for networking when running a single-player game.
// Instead of the server bit-packing entities, delta'ing them, encoding deltas, then decoding the states,
// it just hands the server entity's data to the client, which copies the data over directly.
class ILocalNetworkBackdoor
{
public:
	// These start and end a block of entity updates. EntState() is called in between these for 
	// each entity the server wants the client to receive.
	virtual void StartEntityStateUpdate() = 0;
	virtual void EndEntityStateUpdate() = 0;

	// This tells the client that the entity is still alive but it went dormant for this client.
	virtual void EntityDormant( int iEnt, int iSerialNum ) = 0;

	// This tells the client the full entity state for a specific entity.
	virtual void EntState( 
		int iEnt, 
		int iSerialNum, 
		int classID, 
		const SendTable *pSendTable, 
		const void *pSourceEnt,
		bool bChanged	// If this is false, then the client doesn't have to copy the state unless it doesn't have the ent yet.
		) = 0;
};


// The client will set this if it decides to use the fast path.
extern ILocalNetworkBackdoor *g_pLocalNetworkBackdoor;


#endif // LOCALNETWORKBACKDOOR_H
