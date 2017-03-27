//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "sv_ents_write.h"
#include "sv_main.h"
#include "edict.h"
#include "eiface.h"
#include "dt_send.h"
#include "server_class.h"
#include "utllinkedlist.h"
#include "convar.h"
#include "dt_send_eng.h"
#include "dt.h"
#include "net_synctags.h"
#include "dt_instrumentation_server.h"
#include "LocalNetworkBackdoor.h"


extern ConVar g_CV_DTWatchEnt;


//-----------------------------------------------------------------------------
// Delta timing stuff.
//-----------------------------------------------------------------------------

static ConVar		sv_deltatime( "sv_deltatime", "0", 0, "Enable profiling of CalcDelta calls" );
static ConVar		sv_deltaprint( "sv_deltaprint", "0", 0, "Print accumulated CalcDelta profiling data (only if sv_deltatime is on)" );


class CChangeTrack
{
public:
	char		*m_pName;
	int			m_nChanged;
	int			m_nUnchanged;
	
	CCycleCount	m_Count;
	CCycleCount	m_EncodeCount;
};


static CUtlLinkedList<CChangeTrack*, int> g_Tracks;


// These are the main variables used by the SV_CreatePacketEntities function.
// The function is split up into multiple smaller ones and they pass this structure around.
class CEntityWriteInfo
{
public:
	sv_delta_t		m_Type;
	client_t		*m_pClient;
	bf_write		*m_pBuf;

	PackedEntity	*m_pOldPack;
	PackedEntity	*m_pNewPack;

	CFrameSnapshot	*m_pFromSnapshot;
	CFrameSnapshot	*m_pToSnapshot;
	
	// For each entity handled in the to packet, mark that's it has already been deleted if that's the case
	unsigned char	m_DeletionFlags[ PAD_NUMBER( MAX_EDICTS, 8 ) ];
	
	client_frame_t	*m_pFrom;
	client_frame_t	*m_pTo;

	int				m_nHeaderCount;

	// For compressing offsets into delta header
	int				m_nHeaderBase;
	int				m_nHeaderBits;

	// Some profiling data
	int				m_nTotalGap;
	int				m_nTotalGapCount;
};



//-----------------------------------------------------------------------------
// Delta timing helpers.
//-----------------------------------------------------------------------------

CChangeTrack* GetChangeTrack( const char *pName )
{
	FOR_EACH_LL( g_Tracks, i )
	{
		CChangeTrack *pCur = g_Tracks[i];

		if ( stricmp( pCur->m_pName, pName ) == 0 )
			return pCur;
	}

	CChangeTrack *pCur = new CChangeTrack;
	pCur->m_pName = new char[strlen(pName)+1];
	strcpy( pCur->m_pName, pName );
	pCur->m_nChanged = pCur->m_nUnchanged = 0;
	
	g_Tracks.AddToTail( pCur );
	
	return pCur;
}


void PrintChangeTracks()
{
	Con_Printf( "\n\n" );
	Con_Printf( "------------------------------------------------------------------------\n" );
	Con_Printf( "CalcDelta MS / %% time / Encode MS / # Changed / # Unchanged / Class Name\n" );
	Con_Printf( "------------------------------------------------------------------------\n" );

	CCycleCount total, encodeTotal;
	FOR_EACH_LL( g_Tracks, i )
	{
		CChangeTrack *pCur = g_Tracks[i];
		CCycleCount::Add( pCur->m_Count, total, total );
		CCycleCount::Add( pCur->m_EncodeCount, encodeTotal, encodeTotal );
	}

	FOR_EACH_LL( g_Tracks, j )
	{
		CChangeTrack *pCur = g_Tracks[j];
	
		Con_Printf( "%6.2fms       %5.2f    %6.2fms    %4d        %4d          %s\n", 
			pCur->m_Count.GetMillisecondsF(),
			pCur->m_Count.GetMillisecondsF() * 100.0f / total.GetMillisecondsF(),
			pCur->m_EncodeCount.GetMillisecondsF(),
			pCur->m_nChanged, 
			pCur->m_nUnchanged, 
			pCur->m_pName
			);
	}

	Con_Printf( "\n\n" );
	Con_Printf( "Total CalcDelta MS: %.2f\n\n", total.GetMillisecondsF() );
	Con_Printf( "Total Encode    MS: %.2f\n\n", encodeTotal.GetMillisecondsF() );
}


//-----------------------------------------------------------------------------
// Purpose: Entity wasn't dealt with in packet, but it has been deleted, we'll flag
//  the entity for destruction
// Input  : type - 
//			entnum - 
//			*from - 
//			*to - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
static bool SV_NeedsExplicitDestroy( sv_delta_t type, int entnum, CFrameSnapshot *from, CFrameSnapshot *to )
{
	// Never on uncompressed packet
	if ( type != sv_packet_delta )
	{
		return false;
	}

	if( !to->m_Entities[entnum].m_bExists )
	{
		// Not in old, but is in new.
		if( from->m_Entities[ entnum ].m_bExists )
		{
			return true;
		}

		// Serial numbers are different. This means that the entity was deleted 
		// in between the frames we're deltaing between, and it doesn't exist in either frame.
		if( from->m_Entities[entnum].m_nSerialNumber != to->m_Entities[entnum].m_nSerialNumber )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Creates a delta header for the entity
//-----------------------------------------------------------------------------
static inline void SV_UpdateHeaderDelta( 
	CEntityWriteInfo &u,
	int entnum,
	int nHeaderBits
	)
{
	// Profiling info
	u.m_nTotalGap += entnum - u.m_nHeaderBase;
	u.m_nTotalGapCount++;

	// Keep track of number of headers so we can tell the client
	u.m_nHeaderCount++;
	
	// Cache of header bits needed
	u.m_nHeaderBits = nHeaderBits;

	u.m_nHeaderBase = entnum;
}


//
// Write the delta header. Also update the header delta info if bUpdateHeaderDelta is true.
//
// There are some cases where you want to tenatively write a header, then possibly back it out.
// In these cases:
// - pass in false for bUpdateHeaderDelta
// - store the return value from SV_WriteDeltaHeader
// - call SV_UpdateHeaderDelta ONLY if you want to keep the delta header it wrote
//
static inline int SV_WriteDeltaHeader(
	CEntityWriteInfo &u,
	int entnum,
	int flags,
	bool bUpdateHeaderDelta = true
	)
{
	bf_write *pBuf = u.m_pBuf;

	int startbit = pBuf->GetNumBitsWritten();
	int offset = entnum - u.m_nHeaderBase;

	SyncTag_Write( u.m_pBuf, "Hdr" );

	if ( offset > DELTA_OFFSET_MAX )
	{
		pBuf->WriteOneBit( 1 );
		pBuf->WriteUBitLong(entnum, MAX_EDICT_BITS);
	}
	else
	{
		pBuf->WriteOneBit( 0 );
		pBuf->WriteUBitLong( offset, DELTA_OFFSET_BITS );
	}

	pBuf->WriteOneBit( flags & FHDR_LEAVEPVS ? 1 : 0 );
	if ( !(flags & FHDR_LEAVEPVS) )
	{
		pBuf->WriteOneBit( flags & FHDR_ENTERPVS ? 1 : 0 );
		if ( flags & FHDR_ENTERPVS )
		{
			pBuf->WriteOneBit( flags & FHDR_FORCERECREATE ? 1 : 0 );
		}
	}
	else
	{
		pBuf->WriteOneBit( flags & FHDR_DELETE );
	}

	int nHeaderBits = pBuf->GetNumBitsWritten() - startbit;;
	if ( bUpdateHeaderDelta )
		SV_UpdateHeaderDelta( u, entnum, nHeaderBits );

	return nHeaderBits;
}


// Calculates the delta between the two states and writes the delta and the new properties
// into u.m_pBuf. Returns false if the states are the same.
//
// Also uses the IFrameChangeList in pTo to come up with a smaller set of properties to delta against.
// It deltas against any properties that have changed since iFromFrame.
// If iFromFrame is -1, then it deltas all properties.
static void SV_CalcDeltaAndWriteProps( 
	CEntityWriteInfo &u, 
	
	const void *pFromData,
	int nFromBits, 

	PackedEntity *pTo
	)
{
	// Calculate the delta props.
	int deltaProps[MAX_DATATABLE_PROPS];
	void *pToData = pTo->LockData();

	int nDeltaProps = SendTable_CalcDelta(
		pTo->m_pSendTable, 
		
		pFromData,
		nFromBits,
		
		pToData,
		pTo->GetNumBits(),
		
		deltaProps,
		ARRAYSIZE( deltaProps ),
		
		pTo->m_nEntityIndex	);

	

	// Cull out props given what the proxies say.
	int culledProps[MAX_DATATABLE_PROPS];
	
	int nCulledProps = 0;
	if ( nDeltaProps )
	{
		nCulledProps = SendTable_CullPropsFromProxies(
			pTo->m_pSendTable,
			deltaProps, 
			nDeltaProps,
			u.m_pClient - svs.clients,
			
			NULL,
			-1,

			pTo->GetRecipients(),
			pTo->GetNumRecipients(),

			culledProps,
			ARRAYSIZE( culledProps ) );
	}

	
	// Write the properties.
	SendTable_WritePropList( 
		pTo->m_pSendTable,
		
		pToData,				// object data
		pTo->GetNumBits(),

		u.m_pBuf,				// output buffer

		pTo->m_nEntityIndex,
		culledProps,
		nCulledProps );

	pTo->UnlockData();
}


// NOTE: to optimize this, it could store the bit offsets of each property in the packed entity.
// It would only have to store the offsets for the entities for each frame, since it only reaches 
// into the current frame's entities here.
static inline void SV_WritePropsFromPackedEntity( 
	CEntityWriteInfo &u, 
	PackedEntity *pFrom,
	PackedEntity *pTo,
	const int *pCheckProps,
	const int nCheckProps
	)
{
	CServerDTITimer timer( pTo->m_pSendTable, SERVERDTI_WRITE_DELTA_PROPS );
	if ( g_bServerDTIEnabled )
	{
		IServerEntity *pEnt = sv.edicts[pTo->m_nEntityIndex].GetIServerEntity();
		IServerEntity *pClientEnt = sv.edicts[u.m_pClient - svs.clients + 1].GetIServerEntity();
		if ( pEnt && pClientEnt )
		{
			float flDist = (pEnt->GetAbsOrigin() - pClientEnt->GetAbsOrigin()).Length();
			ServerDTI_AddEntityEncodeEvent( pTo->m_pSendTable, flDist );
		}
	}

	void *pToData = pTo->LockData();

	// Cull out the properties that their proxies said not to send to this client.
	int culledProps[MAX_DATATABLE_PROPS];

	int nCulledProps = SendTable_CullPropsFromProxies( 
		pTo->m_pSendTable, 
		pCheckProps, 
		nCheckProps, 
		u.m_pClient - svs.clients,
		
		pFrom->GetRecipients(),
		pFrom->GetNumRecipients(),
		
		pTo->GetRecipients(),
		pTo->GetNumRecipients(),

		culledProps, 
		ARRAYSIZE( culledProps )
		);
	
	SendTable_WritePropList(
		pTo->m_pSendTable, 
		pToData,
		pTo->GetNumBits(),
		u.m_pBuf, 
		pTo->m_nEntityIndex,
		
		culledProps,
		nCulledProps
		);

	pTo->UnlockData();
}


//-----------------------------------------------------------------------------
// Purpose: See if the entity needs a "hard" reset ( i.e., and explicit creation tag )
//  This should only occur if the entity slot deleted and re-created an entity faster than
//  the last two updates toa  player.  Should never or almost never occur.  You never know though.
// Input  : type - 
//			entnum - 
//			*from - 
//			*to - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
static bool SV_NeedsExplicitCreate( sv_delta_t type, int entnum, CFrameSnapshot *from, CFrameSnapshot *to )
{
	// Never on uncompressed packet
	if ( type != sv_packet_delta )
	{
		return false;
	}

	// Server thinks the entity was continues, but the serial # changed, so we might need to destroy and recreate it
	CFrameSnapshotEntry *pFromEnt = &from->m_Entities[entnum];
	CFrameSnapshotEntry *pToEnt = &to->m_Entities[entnum];

	bool bNeedsExplicitCreate = !pFromEnt->m_bExists || pFromEnt->m_nSerialNumber != pToEnt->m_nSerialNumber;
	if ( !bNeedsExplicitCreate )
	{
		// If it doesn't need explicit create, then the classnames should match.
		// This assert is analagous to the "Server / Client mismatch" one on the client.
		static int nWhines = 0;
		if ( pFromEnt->m_ClassName != pToEnt->m_ClassName )
		{
			if ( ++nWhines < 4 )
			{
				Msg( "ERROR in SV_NeedsExplicitCreate: ent %d from/to classname (%s/%s) mismatch.\n", entnum, STRING( pFromEnt->m_ClassName ), STRING( pToEnt->m_ClassName ) );
			}
		}
	}

	return bNeedsExplicitCreate;
}


static inline void SV_DetermineUpdateType( 
	CEntityWriteInfo &u,
	UpdateType &updateType,
	bool &recreate,
	int const oldEntity,
	int const newEntity
	)
{
	recreate = false;

	// Figure out how we want to update the entity.
	if( newEntity < oldEntity )
	{
		// If the entity was not in the old packet (oldnum == 9999), then 
		// delta from the baseline since this is a new entity.
		recreate = SV_NeedsExplicitCreate( u.m_Type, newEntity, u.m_pFromSnapshot, u.m_pToSnapshot );
		updateType = EnterPVS;
	}
	else if( newEntity > oldEntity )
	{
		// If the entity was in the old list, but is not in the new list 
		// (newnum == 9999), then construct a special remove message.
		updateType = LeavePVS;
	}
	else
	{
		SendTable *pFromTable = u.m_pOldPack->m_pSendTable;
		SendTable *pToTable   = u.m_pNewPack->m_pSendTable;
		
		assert( u.m_pToSnapshot->m_Entities[ newEntity ].m_bExists );

		recreate = SV_NeedsExplicitCreate( u.m_Type, newEntity, u.m_pFromSnapshot, u.m_pToSnapshot );
		
		if ( recreate )
		{
			updateType = EnterPVS;
		}
		else
		{
			// These should be the same! If they're not, then it should detect an explicit create message.
			ErrorIfNot( pFromTable == pToTable,
				("SV_DetermineUpdateType: pFromTable (%s) != pToTable (%s) for ent %d.\n", pFromTable->GetName(), pToTable->GetName(), newEntity)
			);
			
			// We can early out with the delta bits if we are using the same pack handles...
			if ( u.m_pOldPack == u.m_pNewPack )
			{
				updateType = PreserveEnt;
			}
			else
			{
				int iStartBit = u.m_pBuf->GetNumBitsWritten();

				// Write a header.
				int nHeaderBits = SV_WriteDeltaHeader( u, newEntity, FHDR_ZERO, false );
				
				int checkProps[MAX_DATATABLE_PROPS];
				int nCheckProps = u.m_pNewPack->GetPropsChangedAfterTick( u.m_pFromSnapshot->m_nTickNumber, checkProps, ARRAYSIZE( checkProps ) );

				if ( nCheckProps > 0 )
				{
					SV_WritePropsFromPackedEntity( u, u.m_pOldPack, u.m_pNewPack, checkProps, nCheckProps );

					// Ok, we want to keep the header we wrote.
					SV_UpdateHeaderDelta( u, newEntity, nHeaderBits );

					// If the numbers are the same, then the entity was in the old and new packet.
					// Just delta compress the differences.
					updateType = DeltaEnt;
				}
				else
				{
					u.m_pBuf->SeekToBit( iStartBit ); // Remove any changes SV_CalcDelta wrote.
					updateType = PreserveEnt;
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Dereferences all snapshots before the starting snapshot
//-----------------------------------------------------------------------------
void SV_DereferenceUnusedSnapshots( client_t *client, int start )
{
	if ( client->frames && (start >= 0) )
	{
		for ( int j = 0 ; j < SV_UPDATE_BACKUP; j++ )
		{
			client_frame_t *pFrame = &client->frames[ j ];
			
			CFrameSnapshot *pSnapshot = pFrame->GetSnapshot();
			if ( pSnapshot && pSnapshot->m_nTickNumber < start )
				pFrame->SetSnapshot( NULL );
		}
	}
}


bool SV_IsClientDeltaSequenceValid( client_t *pClient )
{
	client_frame_t* fromframe = &pClient->frames[pClient->delta_sequence & SV_UPDATE_MASK];
	return fromframe->GetSnapshot() != NULL;
}



static inline void SV_WritePacketEntitiesHeader(
	CEntityWriteInfo &u,
	bf_write &savepos
)
{
	// See if this is a full update.
	u.m_pFrom = NULL;		// Entity packet for that frame.
	u.m_pFromSnapshot = NULL;
	if ( u.m_Type == sv_packet_delta )
	{
		// This is the frame that we are going to delta update from
		u.m_pFrom      = &u.m_pClient->frames[u.m_pClient->delta_sequence & SV_UPDATE_MASK];
		u.m_pFromSnapshot =  u.m_pFrom->GetSnapshot();

		u.m_pBuf->WriteByte  ( svc_deltapacketentities );   // This is a delta
		u.m_pBuf->WriteShort ( u.m_pTo->GetNumEntities() );          // This is how many ents are in the new packet.
		u.m_pBuf->WriteUBitLong( u.m_pClient->delta_sequence, DELTAFRAME_NUMBITS );    // This is the sequence # that we are updating from.

		// Dereference all snapshots outside of the range needed by this client...
		SV_DereferenceUnusedSnapshots( u.m_pClient, u.m_pFromSnapshot->m_nTickNumber );
	}
	else
	{
		u.m_pBuf->WriteByte ( svc_packetentities );           // Just a packet update.
		u.m_pBuf->WriteShort ( u.m_pTo->GetNumEntities() );            // This is the # of entities we are sending.
	}

	TRACE_PACKET(( "SV Num Ents (%d)\n", u.m_pTo->num_entities ));

	// Store off current position so we can say how long rest of packet is ( so
	//  client can flush the packet easily )
	savepos = *u.m_pBuf;
	u.m_pBuf->WriteUBitLong ( 0, DELTASIZE_BITS );
	// Save room for number of headers to parse, too
	u.m_pBuf->WriteUBitLong( 0, MAX_EDICT_BITS );
}

static inline ServerClass* GetEntServerClass(edict_t *pEdict)
{
	return pEdict->m_pEnt->GetServerClass();
}


//-----------------------------------------------------------------------------
// Gets the next non-zero bit in the entity_in_pvs bitfield
//-----------------------------------------------------------------------------
enum
{
	ENTITY_SENTINEL = 9999
};

static inline int SV_NextEntity( int currentEntity, client_frame_t *pPack )
{
	++currentEntity;

	int byte = currentEntity >> 3;
	int bit = currentEntity & 0x7;

	while (currentEntity <= pPack->entities.max_entities)
	{
		// Early out for lots of zeros in a row
		if (bit == 0)
		{
			// 32 bit boundary...
			if ((byte & 0x3) == 0)
			{
				if (*(int*)(pPack->entity_in_pvs + byte) == 0)
				{
					byte += 4;
					currentEntity += 32;
					continue;
				}
			}

			// 8 bit boundary
			if (pPack->entity_in_pvs[byte] == 0)
			{
				currentEntity += 8;
				++byte;
				continue;
			}
		}

		// One bit at a time...
		for (int i = bit; i < 8; ++i)
		{
			if ( pPack->entity_in_pvs[byte] & (1 << i) )
				return currentEntity;
			++currentEntity;
		}

		bit = 0;
		++byte;
	}

	// Sentinel/end of list....
	return ENTITY_SENTINEL;
}


static inline void SV_WriteEnterPVS( 
	CEntityWriteInfo &u,
	bool bRecreate,
	int &newEntity
	)
{
	TRACE_PACKET(( "  SV Enter PVS (%d) %s\n", newEntity, pNewPack->m_pSendTable->m_pNetTableName ) );

	int headerflags = FHDR_ENTERPVS;

	if ( bRecreate )
	{
		headerflags |= FHDR_FORCERECREATE;
	}

	SV_WriteDeltaHeader( u, newEntity, headerflags );

	ServerClass *pClass = GetEntServerClass( &sv.edicts[ newEntity ] );
	if ( !pClass )
	{
		Host_Error("SV_CreatePacketEntities: GetEntServerClass failed for ent %d.\n", newEntity);
	}
	
	TRACE_PACKET(( "  SV Enter Class %s\n", pClass->m_pNetworkName ) );

	if ( pClass->m_ClassID >= sv.serverclasses )
	{
		Con_Printf( "pClass->m_ClassID(%i) >= %i\n", pClass->m_ClassID, sv.serverclasses );
		Assert( 0 );
	}

	u.m_pBuf->WriteUBitLong( pClass->m_ClassID, sv.serverclassbits );
	
	// Write some of the serial number's bits.
	int sn = sv.edicts[newEntity].m_pEnt->GetRefEHandle().GetSerialNumber();
	sn &= (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
	u.m_pBuf->WriteUBitLong( (unsigned short)sn, NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS );

	// Get the baseline.
	// Since the ent is in the fullpack, then it must have either a static or an instance baseline.
	PackedEntity *pBaseline = &sv.baselines.entities[ newEntity ];
	const void *pFromData;
	int nFromBits;

	bool bUseStaticBaseline = false;
	if ( pBaseline->m_pSendTable && pBaseline->m_pSendTable == u.m_pNewPack->m_pSendTable )
	{
		pFromData = pBaseline->LockData();
		nFromBits = pBaseline->GetNumBits();
		bUseStaticBaseline = true;
	}
	else
	{
		// Since the ent is in the fullpack, then it must have either a static or an instance baseline.
		int nFromBytes;
		if ( !SV_GetInstanceBaseline( pClass, &pFromData, &nFromBytes ) )
		{
			Error( "SV_WriteEnterPVS: missing instance baseline for '%s'.", pClass->m_pNetworkName );
		}

		ErrorIfNot( pFromData,
			("SV_WriteEnterPVS: missing pFromData for '%s'.", pClass->m_pNetworkName)
		);
		
		nFromBits = nFromBytes * 8;	// NOTE: this isn't the EXACT number of bits but that's ok since it's
									// only used to detect if we overran the buffer (and if we do, it's probably
									// by more than 7 bits).
	}

	SV_CalcDeltaAndWriteProps( u, pFromData, nFromBits, u.m_pNewPack );

	// Unlock the data if we used a static baseline.
	if ( bUseStaticBaseline )
	{
		pBaseline->UnlockData();
	}

	newEntity = SV_NextEntity( newEntity, u.m_pTo );
}


static inline void SV_WriteLeavePVS( CEntityWriteInfo &u, int &oldEntity )
{
	int headerflags = FHDR_LEAVEPVS;
	bool deleteentity = SV_NeedsExplicitDestroy( u.m_Type, oldEntity, u.m_pFromSnapshot, u.m_pToSnapshot );
	if ( deleteentity )
	{
		// Mark that we handled deletion of this index
		u.m_DeletionFlags[ oldEntity >> 3 ] |= ( 1 << ( oldEntity & 7 ) );

		headerflags |= FHDR_DELETE;
	}

	TRACE_PACKET( ( "  SV Leave PVS (%d) %s %s\n", oldEntity, 
		deleteentity ? "deleted" : "left pvs",
		u.m_pOldPack->m_pSendTable->m_pNetTableName ) );

	SV_WriteDeltaHeader( u, oldEntity, headerflags );

	oldEntity = SV_NextEntity( oldEntity, u.m_pFrom );
}


static inline void SV_WriteDeltaEnt( CEntityWriteInfo &u, int &oldEntity, int &newEntity )
{
	TRACE_PACKET( ( "  SV Delta PVS (%d %d) %s\n", newEntity, oldEntity, pOldPack->m_pSendTable->m_pNetTableName ) );

	// NOTE: it was already written in DetermineUpdateType. By doing it this way, we avoid an expensive
	// (non-byte-aligned) copy of the data.

	oldEntity = SV_NextEntity( oldEntity, u.m_pFrom );
	newEntity = SV_NextEntity( newEntity, u.m_pTo );
}


static inline void SV_PreserveEnt( CEntityWriteInfo &u, int &oldEntity, int &newEntity )
{
	TRACE_PACKET( ( "  SV Preserve PVS (%d) %s\n", oldEntity, u.m_pOldPack->m_pSendTable->m_pNetTableName ) );

	// updateType is preserveEnt. The client will detect this because our next entity will have a newnum
	// that is greater than oldnum, in which case the client just keeps the current entity alive.
	oldEntity = SV_NextEntity( oldEntity, u.m_pFrom );
	newEntity = SV_NextEntity( newEntity, u.m_pTo );
}


static inline void SV_WriteEntityUpdate(
	CEntityWriteInfo &u,
	UpdateType &updateType,
	bool bRecreate,
	int &oldEntity,
	int &newEntity
	)
{
	switch( updateType )
	{
		case EnterPVS:
		{
			SV_WriteEnterPVS( u, bRecreate, newEntity );
		}
		break;

		case LeavePVS:
		{
			SV_WriteLeavePVS( u, oldEntity );
		}
		break;

		case DeltaEnt:
		{
			SV_WriteDeltaEnt( u, oldEntity, newEntity );
		}
		break;

		case PreserveEnt:
		{
			SV_PreserveEnt( u, oldEntity, newEntity );
		}
		break;
	}
}


static inline void SV_WriteDeletions( CEntityWriteInfo &u )
{
	if( u.m_Type != sv_packet_delta )
		return;

	int i;
	int player_mask = ( 1 << ( u.m_pClient - svs.clients ) );
	for ( i = 0; i < sv.num_edicts; i++ )
	{
		edict_t *ent = &sv.edicts[ i ];
		
		// Check conditions
		if ( // Packet update didn't clear it out expressly
			!( u.m_DeletionFlags[ i >> 3 ] & ( 1 << ( i & 7 ) ) ) &&
			// Player got the entity at some point (no get entity ever == never created on client, of course)
			( ent->entity_created & player_mask ) &&
			// Looks like it should be gone
			SV_NeedsExplicitDestroy( u.m_Type, i, u.m_pFromSnapshot, u.m_pToSnapshot ) )
		{
			TRACE_PACKET( ( "  SV Explicit Destroy (%d)\n", i ) );

			u.m_pBuf->WriteOneBit(1);
			u.m_pBuf->WriteUBitLong( i, MAX_EDICT_BITS );
		}
	}
	// No more entities..
	u.m_pBuf->WriteOneBit(0); 
}


/*
=============
SV_CreatePacketEntities

Computes either a compressed, or uncompressed delta buffer for the client.
Returns the size IN BITS of the message buffer created.
=============
*/

int SV_CreatePacketEntities( 
	sv_delta_t type, 
	client_t *client, 
	client_frame_t *to, 
	CFrameSnapshot *to_snapshot, 
	bf_write *pBuf
	)
{
	// Setup the CEntityWriteInfo structure.
	CEntityWriteInfo u;
	u.m_Type = type;
	u.m_pClient = client;
	u.m_pBuf = pBuf;
	u.m_pTo = to;
	u.m_pToSnapshot = to_snapshot;
	memset( u.m_DeletionFlags, 0, sizeof( u.m_DeletionFlags ) );
	u.m_nHeaderCount = 0;
	u.m_nHeaderBase = 0;
	u.m_nTotalGap = 0;
	u.m_nTotalGapCount = 0;

	// Write the header.
	bf_write savepos;
	savepos.SetDebugName( "savepos" );

	SV_WritePacketEntitiesHeader( u, savepos );

	// Don't work too hard if we're using the optimized single-player mode.
	if ( !g_pLocalNetworkBackdoor )
	{
		// Iterate through the in PVS bitfields until we find an entity 
		// that was either in the old pack or the new pack
		int oldEntity = u.m_pFrom ? SV_NextEntity( -1, u.m_pFrom ) : ENTITY_SENTINEL;
		int newEntity = SV_NextEntity( -1, u.m_pTo );

		while ( (oldEntity != ENTITY_SENTINEL) || (newEntity != ENTITY_SENTINEL) )
		{
			u.m_pNewPack = (newEntity != ENTITY_SENTINEL) ? framesnapshot->GetPackedEntity( u.m_pToSnapshot, newEntity ) : 0;
			u.m_pOldPack = (oldEntity != ENTITY_SENTINEL) ? framesnapshot->GetPackedEntity( u.m_pFromSnapshot, oldEntity ) : 0;

			// Figure out how we want to write this entity.
			bool bRecreate;
			UpdateType updateType;
			SV_DetermineUpdateType( u, updateType, bRecreate, oldEntity, newEntity );
			
			SV_WriteEntityUpdate( u, updateType, bRecreate, oldEntity, newEntity );
		}
	}

	// Now write out the express deletions
	SV_WriteDeletions( u );

	// Fill in length now
	int length = u.m_pBuf->GetNumBitsWritten() - savepos.GetNumBitsWritten();
	savepos.WriteUBitLong( length, DELTASIZE_BITS );
	savepos.WriteUBitLong( u.m_nHeaderCount, MAX_EDICT_BITS );

	return u.m_pBuf->GetNumBitsWritten();
}


