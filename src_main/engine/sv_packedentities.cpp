//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "sv_packedentities.h"
#include "bspfile.h"
#include "eiface.h"
#include "networkstringtablecontainerserver.h"
#include "dt_send_eng.h"
#include "changeframelist.h"
#include "sv_main.h"
#include "dt_instrumentation_server.h"
#include "sv_ents_write.h"
#include "LocalNetworkBackdoor.h"
#include "tier0/vprof.h"
#include "host.h"
#include "networkstringtableserver.h"

ConVar sv_instancebaselines( "sv_instancebaselines", "1", 0, "Enable instanced baselines. Saves network overhead." );
ConVar sv_debugmanualmode( "sv_debugmanualmode", "0", 0, "Make sure entities correctly report whether or not their network data has changed." );


class ClientPackInfo_t : public CCheckTransmitInfo
{
public:
	byte				m_PVS[ (MAX_MAP_CLUSTERS+7) / 8 ];
	byte				m_EdictBits[PAD_NUMBER( MAX_EDICTS, 8 ) / 8];
	// copied from g_AreasConnected after visibility is set up
	CUtlVector< int >	m_AreasNetworked; 
	
	int					m_ClientBit;
	int					m_ClientArea;
};


static void SV_ComputeClientPackInfo( client_t* pClient, ClientPackInfo_t *pInfo )
{
	// Grab the client's edict pointer
	pInfo->m_pClientEnt = pClient->edict;
	pInfo->m_ClientBit = 1 << (pClient - svs.clients);

	// Compute Vis for each client
	serverGameClients->ClientSetupVisibility( (edict_t *)pClient->pViewEntity,
		pInfo->m_pClientEnt, pInfo->m_PVS );

	// Compute the client area for each client
	pInfo->m_ClientArea = pClient->pViewEntity ? pClient->pViewEntity->areanum :
												pInfo->m_pClientEnt->areanum;
}


static void SV_SetupPack( client_frame_t *pPack, CFrameSnapshot *pSnapshot )
{
	Assert( pSnapshot->m_nTickNumber == host_tickcount );
	pPack->SetSnapshot( pSnapshot );
	pPack->entities.max_entities = 0;
	pPack->entities.num_entities = 0;
	memset( pPack->entity_in_pvs, 0, sizeof( pPack->entity_in_pvs ) );
}


// Returns false and calls Host_Error if the edict's pvPrivateData is NULL.
static bool SV_EnsurePrivateData(edict_t *pEdict)
{
	if(pEdict->m_pEnt)
	{
		return true;
	}
	else
	{
		Host_Error("SV_EnsurePrivateData: pEdict->pvPrivateData==NULL (ent %d).\n", pEdict - sv.edicts);
		return false;
	}
}


static inline edict_t* SV_GetEdictToTransmit( int edict, CFrameSnapshot *snapshot )
{
	// Filter out inactive players automatically
	if ( edict >= 1 && edict <= svs.maxclients )
	{
		if ( !svs.clients[edict-1].active )
		{
			return 0;
		}
	}

	// If it don't exist, fuggeddeboudit
	if ( !snapshot->m_Entities[ edict ].m_bExists )
		return 0;

	edict_t* ent = &sv.edicts[ edict ];

	// Make sure it has data to encode.
	if( !SV_EnsurePrivateData(ent) )
	{
		return 0;
	}

	return ent;
}


// This function makes sure that this entity class has an instance baseline.
// If it doesn't have one yet, it makes a new one.
void SV_EnsureInstanceBaseline( int iEdict, const void *pData, int nBytes )
{
	edict_t *pEnt = &sv.edicts[iEdict];
	ErrorIfNot( pEnt->m_pEnt,
		("SV_EnsureInstanceBaseline: edict %d missing ent", iEdict)
	);

	ServerClass *pClass = pEnt->m_pEnt->GetServerClass();

	if ( pClass->m_InstanceBaselineIndex == INVALID_STRING_INDEX )
	{
		char idString[32];
		Q_snprintf( idString, sizeof( idString ), "%d", pClass->m_ClassID );

		// Ok, make a new instance baseline so they can reference it.
		pClass->m_InstanceBaselineIndex = networkStringTableContainerServer->AddString( 
			sv.GetInstanceBaselineTable(),
			idString,	// Note we're sending a string with the ID number, not the class name.
			nBytes,
			pData );

		Assert( pClass->m_InstanceBaselineIndex != INVALID_STRING_INDEX );
	}
}


bool SV_GetInstanceBaseline( ServerClass *pClass, void const **pData, int *pDatalen )
{
	if ( sv_instancebaselines.GetInt() )
	{
		ErrorIfNot( pClass->m_InstanceBaselineIndex != INVALID_STRING_INDEX,
			("SV_GetInstanceBaseline: missing instance baseline for class '%s'", pClass->m_pNetworkName)
		);
		
		*pData = networkStringTableContainerServer->GetStringUserData(
			sv.GetInstanceBaselineTable(),
			pClass->m_InstanceBaselineIndex,
			pDatalen );
		
		return *pData != NULL;
	}
	else
	{
		static char dummy[1] = {0};
		*pData = dummy;
		*pDatalen = 1;
		return true;
	}
}


//-----------------------------------------------------------------------------
// Pack the entity....
//-----------------------------------------------------------------------------

static inline void SV_PackEntity( 
	int edictIdx, 
	edict_t* ent, 
	SendTable* pSendTable,
	EntityChange_t changeType, 
	CFrameSnapshot *pSnapshot )
{
	int iSerialNum = pSnapshot->m_Entities[ edictIdx ].m_nSerialNumber;

	// Check to see if this entity specifies its changes.
	// If so, then try to early out making the fullpack
	bool bUsedPrev = false;
	if ( changeType == ENTITY_CHANGE_NONE )
	{
		// Now this may not work if we didn't previously send a packet;
		// if not, then we gotta compute it
		bUsedPrev = framesnapshot->UsePreviouslySentPacket( pSnapshot, edictIdx, iSerialNum );
	}
					  
	if ( !bUsedPrev || sv_debugmanualmode.GetInt() )
	{
		// First encode the entity's data.
		char packedData[MAX_PACKEDENTITY_DATA];
		bf_write writeBuf( "SV_PackEntity->writeBuf", packedData, sizeof( packedData ) );
		
		// (avoid constructor overhead).
		unsigned char tempData[ sizeof( CSendProxyRecipients ) * MAX_DATATABLE_PROXIES ];
		CUtlMemory< CSendProxyRecipients > recip( (CSendProxyRecipients*)tempData, pSendTable->GetNumDataTableProxies() );

		if( !SendTable_Encode( pSendTable, ent->m_pEnt, &writeBuf, NULL, edictIdx, &recip ) )
		{							 
			Host_Error( "SV_PackEntity: SendTable_Encode returned false (ent %d).\n", edictIdx );
		}

		SV_EnsureInstanceBaseline( edictIdx, packedData, writeBuf.GetNumBytesWritten() );
			
		int nFlatProps = SendTable_GetNumFlatProps( pSendTable );
		IChangeFrameList *pChangeFrame;

		// If this entity was previously in there, then it should have a valid IChangeFrameList 
		// which we can delta against to figure out which properties have changed.
		//
		// If not, then we want to setup a new IChangeFrameList.
		PackedEntity *pPrevFrame = framesnapshot->GetPreviouslySentPacket( edictIdx, pSnapshot->m_Entities[ edictIdx ].m_nSerialNumber );
		if ( pPrevFrame )
		{
			// Calculate a delta.
			bf_read bfPrev( "SV_PackEntity->bfPrev", pPrevFrame->LockData(), pPrevFrame->GetNumBytes() );
			bf_read bfNew( "SV_PackEntity->bfNew", packedData, writeBuf.GetNumBytesWritten() );
			
			int deltaProps[MAX_DATATABLE_PROPS];

			int nChanges = SendTable_CalcDelta(
				pSendTable, 
				pPrevFrame->LockData(), pPrevFrame->GetNumBits(),
				packedData,	writeBuf.GetNumBitsWritten(),
				
				deltaProps,
				ARRAYSIZE( deltaProps ),

				edictIdx
				);

			// If it's non-manual-mode, but we detect that there are no changes here, then just
			// use the previous pSnapshot if it's available (as though the entity were manual mode).
			// It would be interesting to hook here and see how many non-manual-mode entities 
			// are winding up with no changes.
			if ( nChanges == 0 )
			{
				if ( changeType == ENTITY_CHANGE_NONE )
				{
					for ( int iDeltaProp=0; iDeltaProp < nChanges; iDeltaProp++ )
					{
						Msg( "Entity %d (class '%s') reported ENTITY_CHANGE_NONE but '%s' changed.\n", 
							edictIdx,
							STRING( ent->classname ),
							pSendTable->GetProp( deltaProps[iDeltaProp] )->GetName() );

					}
				}
				else
				{
					if ( pPrevFrame->CompareRecipients( recip ) )
					{
						if ( framesnapshot->UsePreviouslySentPacket( pSnapshot, edictIdx, iSerialNum ) )
							return;
					}
				}
			}
		
			// Ok, now snag the changeframe from the previous frame and update the 'last frame changed'
			// for the properties in the delta.
			pChangeFrame = pPrevFrame->SnagChangeFrameList();
			
			ErrorIfNot( pChangeFrame && pChangeFrame->GetNumProps() == nFlatProps,
				("SV_PackEntity: SnagChangeFrameList returned null")
			);

			pChangeFrame->SetChangeTick( deltaProps, nChanges, pSnapshot->m_nTickNumber );
		}
		else
		{
			// Ok, init the change frames for the first time.
			pChangeFrame = AllocChangeFrameList( nFlatProps, pSnapshot->m_nTickNumber );
		}

		// Now make a PackedEntity and store the new packed data in there.
		PackedEntity *pCurFrame = framesnapshot->CreatePackedEntity( pSnapshot, edictIdx );
		pCurFrame->SetChangeFrameList( pChangeFrame );
		pCurFrame->m_nEntityIndex = edictIdx;
		pCurFrame->m_pSendTable = pSendTable;
		pCurFrame->AllocAndCopyPadded( packedData, writeBuf.GetNumBytesWritten(), &g_PackedDataAllocator );
		pCurFrame->SetRecipients( recip );
	}
}


// Returns the SendTable that should be used with the specified edict.
SendTable* GetEntSendTable(edict_t *pEdict)
{
	if ( pEdict->m_pEnt )
	{
		ServerClass *pClass = pEdict->m_pEnt->GetServerClass();
		if ( pClass )
			return pClass->m_pTable;
	}

	return NULL;
}


// This helps handle it if they turn cl_LocalNetworkBackdoor on and off (which is useful to do for profiling).
void SV_HandleLocalNetworkBackdoorChange( client_t *pClient )
{
	if ( pClient->m_bUsedLocalNetworkBackdoor && !g_pLocalNetworkBackdoor )
	{
		pClient->delta_sequence = -1;
	}
	pClient->m_bUsedLocalNetworkBackdoor = ( g_pLocalNetworkBackdoor != NULL );
}


//-----------------------------------------------------------------------------
// Writes the compressed packet of entities to all clients
//-----------------------------------------------------------------------------
void SV_ComputeClientPacks( 
	int clientCount, 
	client_t** clients,
	CFrameSnapshot *snapshot, 
	client_frame_t **pPack
	)
{
	VPROF( "SV_ComputeClientPacks" );

	
	ClientPackInfo_t info[MAX_CLIENTS];

	// Do some setup for each client
	int	iClient;
	for (iClient = 0; iClient < clientCount; ++iClient)
	{
		ClientPackInfo_t *pInfo = &info[iClient];


		// This helps handle it if they turn cl_LocalNetworkBackdoor on and off (which is useful to do for profiling).
		SV_HandleLocalNetworkBackdoorChange( clients[iClient] );

		// This will force network string table updates for local client to go through the backdoor if it's active
		SV_CheckDirectUpdate( clients[iClient] );

		SV_ComputeClientPackInfo( clients[iClient], pInfo );

		// This is the frame we are creating, i.e., the next
		// frame after the last one that the client acknowledged
		client_frame_t& frame = clients[iClient]->frames[clients[iClient]->netchan.outgoing_sequence & SV_UPDATE_MASK];
		pPack[iClient] = &frame;

		SV_SetupPack( pPack[iClient], snapshot );

		// Clear the 'transmit edict' bits for this ent for this client.
		memset( pInfo->m_EdictBits, 0, PAD_NUMBER( sv.num_edicts, 8 ) / 8 );
		pInfo->m_pPVS = pInfo->m_PVS;
		pInfo->m_pEdictBits = pInfo->m_EdictBits;

		// Since area to area visibility is determined by each player's PVS, copy
		//  the area network lookups into the ClientPackInfo_t
		pInfo->m_AreasNetworked.RemoveAll();
		int areaCount = g_AreasNetworked.Count();
		for ( int i = 0; i < areaCount; i++ )
		{
			pInfo->m_AreasNetworked.AddToTail( g_AreasNetworked[ i ] );
		}
	}


	// Figure out which entities should be sent.
	int validEdicts[MAX_EDICTS];
	int nValidEdicts = 0;
	for ( int iEdict=0; iEdict < sv.num_edicts; iEdict++ )
	{
		edict_t* ent = SV_GetEdictToTransmit( iEdict, snapshot );
		if ( !ent || !ent->m_pEnt )
			continue;

		// Get the edict send table
		SendTable* pSendTable = GetEntSendTable( ent );
		assert( pSendTable );
		if ( !pSendTable )
			continue;

		validEdicts[nValidEdicts++] = iEdict;

		for ( int iClient=0; iClient < clientCount; iClient++ )
		{		
			ClientPackInfo_t *pInfo = &info[iClient];
			Assert( pInfo );

			int areaCount = pInfo->m_AreasNetworked.Count();

			for ( int iArea=0; iArea < areaCount; iArea++ )
			{
				// If the edict is already marked to send to this client, we don't need to call CheckTransmit on it.
				if ( pInfo->WillTransmit( iEdict ) )
					break;

				pInfo->m_iArea = pInfo->m_AreasNetworked[ iArea ];
				CServerDTITimer timer( pSendTable, SERVERDTI_SHOULDTRANSMIT );
				ent->m_pEnt->CheckTransmit( pInfo );
			}
		}
	}
		

#ifndef SWDS
	int saveTicks = cl.tickcount;
#endif

	if ( g_pLocalNetworkBackdoor )
	{
		g_pLocalNetworkBackdoor->StartEntityStateUpdate();

#ifndef SWDS
		cl.tickcount = sv.tickcount;
		g_ClientGlobalVariables.tickcount = cl.tickcount;
		g_ClientGlobalVariables.curtime = cl.gettime();
#endif
	}
	

	// Send client all active entities in the pvs
	for ( int iValidEdict=0; iValidEdict < nValidEdicts; iValidEdict++ )
	{
		int e = validEdicts[iValidEdict];
		edict_t* ent = SV_GetEdictToTransmit(e, snapshot);
		SendTable* pSendTable = GetEntSendTable( ent );

		// Check to see if the entity changed this frame...
		EntityChange_t changeType = ent->m_pEnt->DetectNetworkStateChanges();
		
		ServerDTI_RegisterNetworkStateChange( pSendTable, changeType );

		// Have we packed this entity this frame?
		bool packedThisEntity = false;
		IServerEntity *serverEntity = ent->GetIServerEntity();
		if ( serverEntity )
		{
			serverEntity->SetSentLastFrame( false );
		}

		// Compute the entity bit + byte for the entity_in_pvs bitfield
		int entityByte = e >> 3;
		byte entityBit = ( 1 << ( e & 7 ) );

		for (int i = 0; i < clientCount; ++i )
		{
			ClientPackInfo_t *pInfo = &info[i];

			if ( g_pLocalNetworkBackdoor )
			{
				// this is a bit of a hack to ensure that we get a "preview" of the
				//  packet timstamp that the server will send so that things that
				//  are encoded relative to packet time will be correct


				int serialNum = ent->m_pEnt->GetRefEHandle().GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;

				if( pInfo->WillTransmit( e ) )
				{
					CServerDTITimer timer( pSendTable, SERVERDTI_ENCODE );
					
					// If we're using the fast path for a single-player game, just pass the entity
					// directly over to the client.
					ServerClass *pClass = ent->m_pEnt->GetServerClass();
					g_pLocalNetworkBackdoor->EntState( e, serialNum, pClass->m_ClassID, pSendTable, ent->m_pEnt, changeType != ENTITY_CHANGE_NONE );

					// Indicate we've actually put the changes into the pack
					ent->m_pEnt->ResetNetworkStateChanges();
				}
				else
				{
					// Notify the client that the ent is still alive, but its ShouldTransmit returned false.
					g_pLocalNetworkBackdoor->EntityDormant( e, serialNum );
				}
			}
			else
			{
				if( !pInfo->WillTransmit( e ) )
					continue;

				// Mark that this player will have seen this entity created
				// (i.e., sent, at least once)
				ent->entity_created |= info[i].m_ClientBit;
				pPack[i]->entity_in_pvs[ entityByte ] |= entityBit;

				// Check to see if it's already been packed this frame...
				// If it hasn't, then pack it in, baby!
				if ( !packedThisEntity )
				{
					packedThisEntity = true;
					SV_PackEntity( e, ent, pSendTable, changeType, snapshot );
					ent->m_pEnt->ResetNetworkStateChanges();
				}
			}

			// We've got one more entity in this pack.
			++pPack[i]->entities.num_entities;

			// Now this is our biggest ent index in the pack
			pPack[i]->entities.max_entities = e;
		}
	}

	if ( g_pLocalNetworkBackdoor )
	{
		g_pLocalNetworkBackdoor->EndEntityStateUpdate();

#ifndef SWDS
		cl.tickcount = saveTicks;
		g_ClientGlobalVariables.tickcount = cl.tickcount;
		g_ClientGlobalVariables.curtime = cl.gettime();
#endif
	}
}


//-----------------------------------------------------------------------------
// Writes a delta update of a packet_entities_t to the message.
//-----------------------------------------------------------------------------

void SV_EmitPacketEntities (
	client_t *client, 
	client_frame_t *to, 
	CFrameSnapshot *to_snapshot, 
	bf_write *msg )
{
	Assert( to_snapshot->m_nTickNumber == host_tickcount );

	// See if this is a full update.
	if ( client->m_bResendNoDelta || 
		client->delta_sequence == -1 || 
		!SV_IsClientDeltaSequenceValid( client ) )
	{
		SV_CreatePacketEntities( sv_packet_nodelta, client, to, to_snapshot, msg );

		// See the definition of m_ForceWaitForAck and the comments in SV_ForceWaitForAck
		// for info about why we do this.
		client->m_ForceWaitForAck = client->netchan.outgoing_sequence;
		
		// Invalidate all the packets previous to this one.		
		SV_DereferenceUnusedSnapshots( client, to_snapshot->m_nTickNumber );
	}
	else
	{
		SV_CreatePacketEntities( sv_packet_delta, client, to, to_snapshot, msg );
	}
}

// If the table's ID is -1, writes its info into the buffer and increments curID.
void SV_MaybeWriteSendTable( SendTable *pTable, bf_write *pBuf, bool bNeedDecoder )
{
	// Already sent?
	if ( pTable->GetWriteSpawnCount() == svs.spawncount )
		return;

	pTable->SetWriteSpawnCount( svs.spawncount );

	pBuf->WriteByte( svc_sendtable );
	pBuf->WriteOneBit( bNeedDecoder );

	SendTable_SendInfo(pTable, pBuf);
}


// Calls SV_MaybeWriteSendTable recursively.
void SV_MaybeWriteSendTable_R( SendTable *pTable, bf_write *pBuf )
{
	int i;
	SendProp *pProp;

	SV_MaybeWriteSendTable( pTable, pBuf, false );

	// Make sure we send child send tables..
	for(i=0; i < pTable->m_nProps; i++)
	{
		pProp = &pTable->m_pProps[i];

		if( pProp->m_Type == DPT_DataTable )
			SV_MaybeWriteSendTable_R( pProp->GetDataTable(), pBuf );
	}
}


// Sets up SendTable IDs and sends an svc_sendtable for each table.
void SV_WriteSendTables( ServerClass *pClasses, bf_write *pBuf )
{
	ServerClass *pCur;

	// First, we send all the leaf classes. These are the ones that will need decoders
	// on the client.
	for ( pCur=pClasses; pCur; pCur=pCur->m_pNext )
	{
		SV_MaybeWriteSendTable( pCur->m_pTable, pBuf, true );
	}

	// Now, we send their base classes. These don't need decoders on the client
	// because we will never send these SendTables by themselves.
	for ( pCur=pClasses; pCur; pCur=pCur->m_pNext )
	{
		SV_MaybeWriteSendTable_R( pCur->m_pTable, pBuf );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : crc - 
//-----------------------------------------------------------------------------
void SV_ComputeClassInfosCRC( CRC32_t* crc )
{
	ServerClass *pClasses = serverGameDLL->GetAllServerClasses();

	for ( ServerClass *pClass=pClasses; pClass; pClass=pClass->m_pNext )
	{
		CRC32_ProcessBuffer( crc, (void *)pClass->m_pNetworkName, Q_strlen( pClass->m_pNetworkName ) );
		CRC32_ProcessBuffer( crc, (void *)pClass->m_pTable->GetName(), Q_strlen(pClass->m_pTable->GetName() ) );
	}
}

// Assign each class and ID and write an svc_classinfo for each one.
void SV_WriteClassInfos(ServerClass *pClasses, bf_write *pBuf)
{
//	FileHandle_t fileHandle = g_pFileSystem->Open( "classinfos.txt", "w" );

	// Count the number of classes.
	int nClasses = 0;
	for ( ServerClass *pCount=pClasses; pCount; pCount=pCount->m_pNext )
	{
//		g_pFileSystem->FPrintf( fileHandle, "%d %s\n", nClasses, pCount->GetName() );
		++nClasses;
	}

	assert( nClasses < (1<<16) );

	sv.serverclasses = nClasses;
	sv.serverclassbits = Q_log2( sv.serverclasses ) + 1;

//	g_pFileSystem->Close( fileHandle );

	pBuf->WriteByte(svc_classinfo);
	pBuf->WriteByte(CLASSINFO_NUMCLASSES);
	pBuf->WriteShort( nClasses );

	int curID = 0;
	for ( ServerClass *pClass=pClasses; pClass; pClass=pClass->m_pNext )
	{
		pClass->m_ClassID = curID;
		curID++;

		pBuf->WriteByte(svc_classinfo);
		pBuf->WriteByte(CLASSINFO_CLASSDATA);

		pBuf->WriteUBitLong( pClass->m_ClassID, sv.serverclassbits, false );
		pBuf->WriteString( pClass->m_pNetworkName );
		pBuf->WriteString( pClass->m_pTable->GetName() );
/*
		// Write the sub networkable info.
		for ( CServerSubNetworkableDef *pCur=pClass->m_pSubNetworkableList->m_pHead; pCur; pCur=pCur->m_pNext )
		{
			pBuf->WriteOneBit( 1 );
			pBuf->WriteString( pCur->m_pSharedName );
			pBuf->WriteUBitLong( pCur->m_SubNetworkableID, 16 );
		}
		pBuf->WriteOneBit( 0 );
*/
	}

	pBuf->WriteByte(svc_classinfo);
	pBuf->WriteByte(CLASSINFO_ENDCLASSES);
}


// This is implemented for the datatable code so its warnings can include an object's classname.
const char* GetObjectClassName( int objectID )
{
	if ( objectID >= 0 && objectID < sv.num_edicts )
	{
		return STRING( sv.edicts[objectID].classname );
	}
	else
	{
		return "[unknown]";
	}
}



