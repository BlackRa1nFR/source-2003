//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Parsing of entity network packets.
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "client_class.h"
#include "cdll_engine_int.h"
#include "icliententitylist.h"
#include "cl_ents.h"
#include "con_nprint.h"
#include "protocol.h"
#include "iprediction.h"
#include "cl_entityreport.h"
#include "host.h"
#include "demo.h"
#include "UtlVector.h"
#include "FileSystem_Engine.h"
#include "const.h"
#include "dt_recv_eng.h"
#include "net_synctags.h"
#include "ISpatialPartitionInternal.h"
#include "LocalNetworkBackdoor.h"
#include "basehandle.h"
#include "dt_localtransfer.h"
#include "iprediction.h"
#include "tier0/vprof.h"
#include "vstdlib/icommandline.h"


// PostDataUpdate calls are stored in a list until all ents have been updated.
class CPostDataUpdateCall
{
public:
	int		m_iEnt;
	DataUpdateType_t	m_UpdateType;
};


// Passed around the read functions.
class CEntityReadInfo
{
public:

	frame_t			*m_pFrame;
	bool			m_bUncompressed;
	int				m_iStartBit;
	int				m_iEndBit;
	bool			m_bFinished;	// set to true when the packet is through
	
	CRecvEntList	*m_pOld;
	CRecvEntList	*m_pNew;

	int				m_OldNum;
	int				m_NewNum;

	int				m_OldIndex;
	int				m_NewIndex;
	
	UpdateType		m_UpdateType;
	int				m_UpdateFlags;		// from the subheader
	bool			m_bIsEntity;
	
	int				*m_pLocalPlayerBits;
	int				*m_pOtherPlayerBits;

	unsigned char	*m_pEntityInPVS;	

	int				m_nHeaderCount;
	int				m_nHeaderBase;

	CPostDataUpdateCall	m_PostDataUpdateCalls[MAX_EDICTS];
	int					m_nPostDataUpdateCalls;
};


ConVar cl_flushentitypacket("cl_flushentitypacket", "0", 0, "For debugging. Force the engine to flush an entity packet.");
ConVar cl_deltatrace( "cl_deltatrace", "0", 0, "For debugging, print entity creation/deletion info to console." );
ConVar cl_packettrace( "cl_packettrace", "1", 0, "For debugging, massive spew to file." );

ConVar cl_showdecodecount( "cl_showdecodecount", "0", 0, "For debugging, show # props decoded" );


// Prints important entity creation/deletion events to console
#if defined( _DEBUG )
#define TRACE_DELTA( text ) if ( cl_deltatrace.GetInt() ) { Con_Printf( text ); };
#else
#define TRACE_DELTA( funcs )
#endif



//-----------------------------------------------------------------------------
// Debug networking stuff.
//-----------------------------------------------------------------------------

// #define DEBUG_NETWORKING 1

#if defined( DEBUG_NETWORKING )
#define TRACE_PACKET( text ) if ( cl_packettrace.GetInt() ) { SpewToFile text ; };
#else
#define TRACE_PACKET( text )
#endif

#if defined( DEBUG_NETWORKING )

//-----------------------------------------------------------------------------
// Opens the recording file
//-----------------------------------------------------------------------------

static FileHandle_t OpenRecordingFile()
{
	FileHandle_t fp = 0;
	static bool s_CantOpenFile = false;
	static bool s_NeverOpened = true;
	if (!s_CantOpenFile)
	{
		fp = g_pFileSystem->Open( "cltrace.txt", s_NeverOpened ? "wt" : "at" );
		if (!fp)
		{
			s_CantOpenFile = true;			
		}
		s_NeverOpened = false;
	}
	return fp;
}

//-----------------------------------------------------------------------------
// Records an argument for a command, flushes when the command is done
//-----------------------------------------------------------------------------

void SpewToFile( char const* pFmt, ... )
{
	static CUtlVector<unsigned char> s_RecordingBuffer;

	char temp[2048];
	va_list args;

	va_start( args, pFmt );
	int len = vsprintf( temp, pFmt, args );
	va_end( args );
	assert( len < 2048 );

	int idx = s_RecordingBuffer.AddMultipleToTail( len );
	memcpy( &s_RecordingBuffer[idx], temp, len );
	if ( 1 ) //s_RecordingBuffer.Size() > 8192)
	{
		FileHandle_t fp = OpenRecordingFile();
		g_pFileSystem->Write( s_RecordingBuffer.Base(), s_RecordingBuffer.Size(), fp );
		g_pFileSystem->Close( fp );

		s_RecordingBuffer.RemoveAll();
	}
}

#endif // DEBUG_NETWORKING

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *flags - 
//			entnum - 
//-----------------------------------------------------------------------------
void CL_AddInPVSFlag( unsigned char *flags, int entnum )
{
	flags[ entnum >> 3 ] |= 1 << ( entnum & 7 );
}

//-----------------------------------------------------------------------------
// Purpose: Frees the client DLL's binding to the object.
// Input  : iEnt - 
//-----------------------------------------------------------------------------
void CL_DeleteDLLEntity( int iEnt, char *reason )
{
	IClientNetworkable *pNet = entitylist->GetClientNetworkable( iEnt );

	if ( pNet )
	{
		ClientClass *pClientClass = pNet->GetClientClass();
		TRACE_DELTA( va( "%s - %s", pClientClass ? pClientClass->m_pNetworkName : "unknown", reason ) );
		CL_RecordDeleteEntity( iEnt, pClientClass );
		pNet->Release();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Has the client DLL allocate its data for the object.
// Input  : iEnt - 
//			iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
IClientNetworkable* CL_CreateDLLEntity( int iEnt, int iClass, int iSerialNum )
{
	if( iClass >= cl.m_nServerClasses )
	{
		Host_Error("CL_CreateDLLEntity: invalid class index (%d).\n", iClass);
		return false;
	}

#if defined( _DEBUG )
	IClientNetworkable *pOldNetworkable = entitylist->GetClientNetworkable( iEnt );
	Assert( !pOldNetworkable );
#endif

	ClientClass *pClientClass;
	if ( pClientClass = cl.m_pServerClasses[iClass].m_pClientClass )
	{
		TRACE_DELTA( va( "%s - Create (%i):  Enter PVS\n", pClientClass->m_pNetworkName, iEnt ) );
		CL_RecordAddEntity( iEnt );
		
		// Create the entity.
		return pClientClass->m_pCreateFn( iEnt, iSerialNum );
	}

	Assert(false);
	return false;
}

static void	SpewBitStream( unsigned char* pMem, int bit, int lastbit )
{
	int val = 0;
	char buf[1024];
	char* pTemp = buf;
	int bitcount = 0;
	int charIdx = 1;
	while( bit < lastbit )
	{
		int byte = bit >> 3;
		int bytebit = bit & 0x7;

		val |= ((pMem[byte] & bytebit) != 0) << bitcount;

		++bit;
		++bitcount;

		if (bitcount == 4)
		{
			if ((val >= 0) && (val <= 9))
				pTemp[charIdx] = '0' + val;
			else
				pTemp[charIdx] = 'A' + val - 0xA;
			if (charIdx == 1)
				charIdx = 0;
			else
			{
				charIdx = 1;
				pTemp += 2;
			}
			bitcount = 0;
			val = 0;
		}
	}
	if ((bitcount != 0) || (charIdx != 0))
	{
		if (bitcount > 0)
		{
			if ((val >= 0) && (val <= 9))
				pTemp[charIdx] = '0' + val;
			else
				pTemp[charIdx] = 'A' + val - 0xA;
		}
		if (charIdx == 1)
		{
			pTemp[0] = '0';
		}
		pTemp += 2;
	}
	pTemp[0] = '\0';

	TRACE_PACKET(( "    CL Bitstream %s\n", buf ));
}


void CL_AddPostDataUpdateCall( CEntityReadInfo &u, int iEnt, DataUpdateType_t updateType )
{
	ErrorIfNot( u.m_nPostDataUpdateCalls < MAX_EDICTS,
		("CL_AddPostDataUpdateCall: overflowed u.m_PostDataUpdateCalls") );

	u.m_PostDataUpdateCalls[u.m_nPostDataUpdateCalls].m_iEnt = iEnt;
	u.m_PostDataUpdateCalls[u.m_nPostDataUpdateCalls].m_UpdateType = updateType;
	++u.m_nPostDataUpdateCalls;
}


//-----------------------------------------------------------------------------
// Purpose: When a delta command is received from the server
//  We need to grab the entity # out of it any the bit settings, too.
//  Returns -1 if there are no more entities.
// Input  : &bRemove - 
//			&bIsNew - 
// Output : int
//-----------------------------------------------------------------------------
void CL_ParseDeltaHeader( 
	CEntityReadInfo &u
	)
{
	bool value;


	u.m_UpdateFlags = FHDR_ZERO;


#ifdef DEBUG_NETWORKING
	bf_read* pRead = MSG_GetReadBuf();
	int bit = pRead->GetNumBitsRead();
#endif

	SyncTag_Read( MSG_GetReadBuf(), "Hdr" );	

	if ( !MSG_ReadOneBit() )
	{
		int offset = MSG_ReadBitLong( DELTA_OFFSET_BITS );

		u.m_NewNum = u.m_nHeaderBase + offset;
	}
	else
	{
		u.m_NewNum = MSG_ReadBitLong( MAX_EDICT_BITS );
	}

	u.m_nHeaderBase = u.m_NewNum;

	// leave pvs flag
	value = !!MSG_ReadOneBit();
	if ( !value )
	{
		// enter pvs flag
		value = !!MSG_ReadOneBit();
		if ( value )
		{
			u.m_UpdateFlags |= FHDR_ENTERPVS;
			// Recreate flag?
			value = !!MSG_ReadOneBit();
			if ( value )
			{
				u.m_UpdateFlags |= FHDR_FORCERECREATE;
			}
		}
	}
	else
	{
		u.m_UpdateFlags |= FHDR_LEAVEPVS;

		// Force delete flag
		value = !!MSG_ReadOneBit();
		if ( value )
		{
			u.m_UpdateFlags |= FHDR_DELETE;
		}
	}

	// Output the bitstream...
#ifdef DEBUG_NETWORKING
	int lastbit = pRead->GetNumBitsRead();
	SpewBitStream( (byte *)pRead->m_pData, bit, lastbit );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Get the receive table for the specified entity
// Input  : *pEnt - 
// Output : RecvTable*
//-----------------------------------------------------------------------------
RecvTable* GetEntRecvTable( int entnum )
{
	IClientNetworkable *pNet = entitylist->GetClientNetworkable( entnum );
	if ( pNet )
		return pNet->GetClientClass()->m_pRecvTable;
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Can go from either a baseline or a previous packet_entity
//  The delta (difference) between the from and to state is used to alter the from fields
//  into the to fields.
// Input  : *from - 
//			*to - 
//			entNum - 
//			bNew - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CL_ParseDelta( 
	bf_read *pInput,
	IClientNetworkable *pDest,
	int entNum, 
	unsigned char *flags )
{
	// If ent doesn't think it's in PVS, signal that it is
	CL_AddInPVSFlag( flags, entNum );
	
	
	// The goal here is to copy the data either from 'from' or from the network stream so 'to' is a valid 
	// delta-encoded entity state.
	RecvTable *pRecvTable = GetEntRecvTable( entNum );
	if( !pRecvTable )
		Host_Error( "CL_ParseDelta: invalid recv table for ent %d.\n", entNum );

	RecvTable_Decode( pRecvTable, pDest->GetDataTableBasePtr(), pInput, entNum );
}

//-----------------------------------------------------------------------------
// Purpose: Removes existing entities from packet
// Input  : *pPacket - 
//-----------------------------------------------------------------------------
void CL_ClearPacket(CRecvEntList *pPacket)
{
	pPacket->Term();
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity index corresponds to a player slot 
// Input  : index - 
// Output : qboolean
//-----------------------------------------------------------------------------
bool CL_IsPlayerIndex( int index )
{
	return ( index >= 1 && index <= cl.maxclients ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Determine entity number of corresponding entity in old packet
// Input  : *oldp - 
//			oldindex - 
// Output : int 9999 == not in old packet
//-----------------------------------------------------------------------------
int CL_LookupOldpacketIndex( CRecvEntList *oldp, int oldindex )
{
	if ( !oldp || oldindex >= oldp->GetNumEntities() )
	{
		return 9999;
	}
	else
	{
		return oldp->GetEntityIndex( oldindex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Bad data was received, just flushes incoming delta data.
//-----------------------------------------------------------------------------
void CL_FlushEntityPacket( int endbit, CRecvEntList *packet, char const *errorString, ... )
{
	con_nprint_t np;
	char str[2048];
	va_list marker;

	// Spit out an error.
	va_start(marker, errorString);
	_vsnprintf(str, sizeof(str), errorString, marker);
	va_end(marker);
	
	Con_Printf("%s", str);

	np.fixed_width_font = false;
	np.time_to_live = 1.0;
	np.index = 0;
	np.color[ 0 ] = 1.0;
	np.color[ 1 ] = 0.2;
	np.color[ 2 ] = 0.0;
	Con_NXPrintf( &np, "WARNING:  CL_FlushEntityPacket, %s", str );

	cl.validsequence = 0;		// can't render a frame
	cl.frames[ cl.parsecountmod ].invalid = true;  // State that this data is bogus.

	MSG_GetReadBuf()->Seek( endbit );

	// Free packet memory.
	CL_ClearPacket( packet );
}



// ----------------------------------------------------------------------------- //
// Regular handles for ReadPacketEntities.
// ----------------------------------------------------------------------------- //

void CL_CopyNewEntity( 
	CEntityReadInfo &u,
	int iClass,
	int iSerialNum
	)
{
	if ( u.m_NewIndex >= MAX_EDICTS )
	{
		Host_Error ("CL_ParsePacketEntities: newindex == MAX_EDICTS");
		return;
	}

	// If it's new, make sure we have a slot for it.
	IClientNetworkable *ent = entitylist->GetClientNetworkable( u.m_NewNum );

	// Only create if it doesn't already exist
	ClientClass *pClass = cl.m_pServerClasses[iClass].m_pClientClass;
	bool bNew = false;
	if ( ent )
	{
		// Make sure we've got the same type of entity that we're about to
		// receive data for
		ClientClass *pCurrentClientClass = ent->GetClientClass();

		// Make sure we've got the same type of entity that we're about to
		// receive data for
		if ( pClass != pCurrentClientClass )
		{
			static int nWhines = 0;
			if ( ++nWhines < 4 )
			{
				Msg( "CL_CopyNewEntity:  Server / Client mismatch on entity %i (class %i)\n", u.m_NewNum, iClass );
				Msg( "  Server class %s\n", pClass->m_pNetworkName );
				Msg( "  Client class %s\n", pCurrentClientClass->m_pNetworkName );
			}
		
			CL_DeleteDLLEntity( u.m_NewNum, va( "MISMATCH DETECTED (%i)\n", u.m_NewNum ) );
			ent = 0; // force a recreate
		}
	}

	if ( !ent )
	{	
		// Ok, it doesn't exist yet, therefore this is not an "entered PVS" message.
		ent = CL_CreateDLLEntity( u.m_NewNum, iClass, iSerialNum );
		if( !ent )
		{
			Host_Error( "CL_ParsePacketEntities:  Error creating entity %s(%i)\n", cl.m_pServerClasses[iClass].m_pClientClass->m_pNetworkName, u.m_NewNum );
			return;
		}

		bNew = true;
	}

	int start_bit = MSG_GetReadBuf()->m_iCurBit;

	DataUpdateType_t updateType = bNew ? DATA_UPDATE_CREATED : DATA_UPDATE_DATATABLE_CHANGED;
	ent->PreDataUpdate( updateType );

		// Get either the static or instance baseline.
		const void *pFromData;
		int nFromBits;
		bool bUseStaticBaseline = false;

		PackedEntity *baseline = GetStaticBaseline( u.m_NewNum );
		if ( baseline && baseline->m_pRecvTable && baseline->m_pRecvTable == pClass->m_pRecvTable )
		{
			bUseStaticBaseline = true;
			pFromData = baseline->LockData();
			nFromBits = baseline->GetNumBits();
		}
		else
		{
			int nFromBytes;
			
			// Every entity must have a static or an instance baseline when we get here.
			ErrorIfNot(
				GetDynamicBaseline( iClass, &pFromData, &nFromBytes ),
				("CL_CopyNewEntity: GetDynamicBaseline(%d) failed.", iClass)
			);
			
			nFromBits = nFromBytes * 8;
		}

		// Delta from baseline
		bf_read fromBuf( "CL_CopyNewEntity->fromBuf", pFromData, PAD_NUMBER( nFromBits, 8 ) / 8, nFromBits );
		CL_ParseDelta( &fromBuf, ent, u.m_NewNum, u.m_pEntityInPVS );

		// Unlock the static baseline's data if we need to.
		if ( bUseStaticBaseline )
			baseline->UnlockData();

		// Now parse in the contents of the network stream.
		CL_ParseDelta( MSG_GetReadBuf(), ent, u.m_NewNum, u.m_pEntityInPVS );

	CL_AddPostDataUpdateCall( u, u.m_NewNum, updateType );

	u.m_pNew->SetEntityIndex( u.m_NewIndex, u.m_NewNum );

	//
	// Net stats..
	//
	int end_bit = MSG_GetReadBuf()->m_iCurBit;
	CL_RecordEntityBits( u.m_NewNum, end_bit - start_bit );
	if ( CL_IsPlayerIndex( u.m_NewNum ) )
	{
		if ( u.m_NewNum == cl.playernum + 1 )
		{
			*u.m_pLocalPlayerBits += ( end_bit - start_bit  );
		}
		else
		{
			*u.m_pOtherPlayerBits += ( end_bit - start_bit  );
		}
	}
}


void CL_CopyExistingEntity( CEntityReadInfo &u )
{
	int start_bit = MSG_GetReadBuf()->m_iCurBit;

	IClientNetworkable *pEnt = entitylist->GetClientNetworkable( u.m_NewNum );
	if ( !pEnt )
	{
		Host_Error( "CL_CopyExistingEntity: missing client entity %d.\n", u.m_NewNum );
		return;
	}

	// Read raw data from the network stream
	pEnt->PreDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );

		CL_ParseDelta( MSG_GetReadBuf(), pEnt, u.m_NewNum, u.m_pEntityInPVS	);
			
	CL_AddPostDataUpdateCall( u, u.m_NewNum, DATA_UPDATE_DATATABLE_CHANGED );

	u.m_pNew->SetEntityIndex( u.m_NewIndex, u.m_NewNum );

	int end_bit = MSG_GetReadBuf()->GetNumBitsRead();
	CL_RecordEntityBits( u.m_NewNum, end_bit - start_bit );

	if ( CL_IsPlayerIndex( u.m_NewNum ) )
	{
		if ( u.m_NewNum == cl.playernum + 1 )
		{
			*u.m_pLocalPlayerBits += ( end_bit - start_bit  );
		}
		else
		{
			*u.m_pOtherPlayerBits += ( end_bit - start_bit  );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : length - 
//			start_bit - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CL_HandleError( int endbit, int start_bit, const char *fmt, ... )
{
	char str[2048];
	va_list marker;

	// Spit out an error.
	va_start(marker, fmt);
	_vsnprintf(str, sizeof(str), fmt, marker);
	va_end(marker);

	Con_Printf( "%s\n", str );

	MSG_GetReadBuf()->Seek( endbit );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_MarkEntitiesOutOfPVS( unsigned char *pvs_flags )
{
	int highest_index = entitylist->GetHighestEntityIndex();
	// Note that we go up to and including the highest_index
	for ( int i = 0; i <= highest_index; i++ )
	{
		IClientNetworkable *ent = entitylist->GetClientNetworkable( i );
		if ( !ent )
			continue;

		// FIXME: We can remove IClientEntity here if we keep track of the
		// last frame's entity_in_pvs
		bool curstate = !ent->IsDormant();
		bool newstate = pvs_flags[ i >> 3 ] & ( 1 << ( i & 7 ) ) ? true : false;

		if ( !curstate && newstate )
		{
			// Inform the client entity list that the entity entered the PVS
			ent->NotifyShouldTransmit( SHOULDTRANSMIT_START );
		}
		else if ( curstate && !newstate )
		{
			// Inform the client entity list that the entity left the PVS
			ent->NotifyShouldTransmit( SHOULDTRANSMIT_END );

			CL_RecordLeavePVS( i );
		}
		else if (curstate && newstate)
		{
			// Inform the client entity list that the entity is still in the PVS
			ent->NotifyShouldTransmit( SHOULDTRANSMIT_STAY );
		}
	}
}


// Returns false if you should stop reading entities.
static inline bool CL_DetermineUpdateType(
	CEntityReadInfo &u 
	)
{
	if ( !u.m_bIsEntity || ( u.m_NewNum > u.m_OldNum ) )
	{
		// If we're at the last entity, preserve whatever entities followed it in the old packet.
		// If newnum > oldnum, then the server skipped sending entities that it wants to leave the state alone for.
		if ( !u.m_pOld || ( u.m_OldIndex >= u.m_pOld->GetNumEntities() ) )
		{
			assert( !u.m_bIsEntity );
			return false;
		}

		// Preserve entities until we reach newnum (ie: the server didn't send certain entities because
		// they haven't changed).
		u.m_UpdateType = PreserveEnt;
	}
	else
	{
		if( u.m_UpdateFlags & FHDR_ENTERPVS )
		{
			u.m_UpdateType = EnterPVS;
		}
		else if( u.m_UpdateFlags & FHDR_LEAVEPVS )
		{
			u.m_UpdateType = LeavePVS;
		}
		else
		{
			u.m_UpdateType = DeltaEnt;
		}
	}

	return true;
}


static inline void CL_ReadEnterPVS( 
	CEntityReadInfo &u
	)
{
	TRACE_PACKET(( "  CL Enter PVS (%d)\n", u.m_NewNum ));

	if ( u.m_UpdateFlags & FHDR_FORCERECREATE )
	{
		CL_DeleteDLLEntity( u.m_NewNum, va( "Delete (%i):  Recreate\n", u.m_NewNum ) );
	}

	int iClass = MSG_GetReadBuf()->ReadUBitLong( cl.m_nServerClassBits );

	int iSerialNum = MSG_GetReadBuf()->ReadUBitLong( NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS );

	CL_CopyNewEntity( u, iClass, iSerialNum );

	u.m_NewIndex++;
}


static inline void CL_ReadLeavePVS(CEntityReadInfo &u)
{
	// Sanity check.
	if ( u.m_bUncompressed )
	{
		cl.validsequence = 0;
		CL_HandleError( u.m_iEndBit, u.m_iStartBit, "WARNING: LeavePVS on full update" );
		u.m_bFinished = true;	// break out
		return;
	}

	if ( u.m_UpdateFlags & FHDR_DELETE )
	{
		CL_DeleteDLLEntity( u.m_OldNum, va( "Delete (%i):  Exit PVS + Destroy\n", u.m_OldNum ) );
	}

	u.m_OldIndex++;
}


static inline void CL_ReadDeltaEnt(CEntityReadInfo &u)
{
	CL_CopyExistingEntity( u );
	
	u.m_NewIndex++;
	u.m_OldIndex++;
}


static inline void CL_ReadPreserveEnt(CEntityReadInfo &u)
{
	if ( u.m_bUncompressed )  // Should never happen on a full update.
	{
		cl.validsequence = 0;
		CL_HandleError( u.m_iEndBit, u.m_iStartBit, "WARNING: PreserveEnt on full update" );
		u.m_bFinished = true;	// break out
		return;
	}
	
	// copy one of the old entities over to the new packet unchanged
	if ( u.m_NewIndex >= MAX_EDICTS )
	{
		Host_Error ("CL_ParsePacketEntities: u.m_NewIndex == MAX_EDICTS");
	}
	
	u.m_pNew->CopyState( u.m_pOld, u.m_OldIndex, u.m_NewIndex );
	u.m_NewIndex++;
	u.m_OldIndex++;
	
	// Zero overhead
	CL_RecordEntityBits( u.m_OldNum, 0 );

	CL_AddInPVSFlag( u.m_pEntityInPVS, u.m_OldNum );
}


static inline void CL_ReadEntityUpdate(
	CEntityReadInfo &u
)
{
	switch( u.m_UpdateType )
	{
		case EnterPVS:
		{
			CL_ReadEnterPVS( u );
		}
		break;

		case LeavePVS:
		{
			CL_ReadLeavePVS( u );
		}
		break;

		case DeltaEnt:
		{
			CL_ReadDeltaEnt( u );
		}
		break;

		case PreserveEnt:
		{
			CL_ReadPreserveEnt( u );
		}
		break;
	}
}


static inline void CL_ReadDeletions( bool bUncompressed, bool bFinished )
{
	if ( !bUncompressed && !bFinished )
	{
		while ( MSG_ReadOneBit() )
		{
			int idx = MSG_ReadBitLong( 10 );

			CL_DeleteDLLEntity( idx, va( "Delete (%i):  Express deletion\n", idx ));
		}
	}
}


static void CL_CallPostDataUpdates( CEntityReadInfo &u )
{
	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_POSTDATAUPDATE_START );

	for ( int i=0; i < u.m_nPostDataUpdateCalls; i++ )
	{
		CPostDataUpdateCall *pCall = &u.m_PostDataUpdateCalls[i];
	
		IClientNetworkable *pEnt = entitylist->GetClientNetworkable( pCall->m_iEnt );
		ErrorIfNot( pEnt, 
			("CL_CallPostDataUpdates: missing ent %d", pCall->m_iEnt) );

		pEnt->PostDataUpdate( pCall->m_UpdateType );
	}

	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_POSTDATAUPDATE_END );
}


// ----------------------------------------------------------------------------- //
// ----------------------------------------------------------------------------- //
void CL_ReadPacketEntities(
	int endbit,
	int headercount,
	CRecvEntList *oldp, 
	CRecvEntList *newp, 
	frame_t *frame,
	int *localplayerbits,
	int *otherplayerbits,
	bool bUncompressed,
	unsigned char entity_in_pvs[ PAD_NUMBER( MAX_EDICTS, 8 ) ]
	)
{
	CEntityReadInfo u;
	u.m_iEndBit = endbit;
	u.m_pOld = oldp;
	u.m_pNew = newp;
	u.m_pFrame = frame;
	u.m_pLocalPlayerBits = localplayerbits;
	u.m_pOtherPlayerBits = otherplayerbits;
	u.m_bUncompressed = bUncompressed;
	u.m_pEntityInPVS = entity_in_pvs;
	u.m_nHeaderBase = 0;
	u.m_nHeaderCount = headercount;
	u.m_nPostDataUpdateCalls = 0;


	memset( u.m_pEntityInPVS, 0, PAD_NUMBER( MAX_EDICTS, 8 ) );
	u.m_iStartBit = MSG_GetReadBuf()->GetNumBitsRead();

	// Loop until there are no more entities to read
	u.m_OldIndex = u.m_NewIndex = 0;
	u.m_NewNum = u.m_OldNum = -1;

	u.m_bFinished = false;
	while ( !u.m_bFinished )
	{
		u.m_nHeaderCount--;

		u.m_bIsEntity = ( u.m_nHeaderCount >= 0 ) ? true : false;

		if ( u.m_bIsEntity  )
		{
			CL_ParseDeltaHeader( u );
		}

		bool bExitOut = false;
		u.m_UpdateType = PreserveEnt;
		while( u.m_UpdateType == PreserveEnt && !u.m_bFinished )
		{
			u.m_OldNum = CL_LookupOldpacketIndex( u.m_pOld, u.m_OldIndex );

			// Figure out what kind of an update this is.
			if( !CL_DetermineUpdateType( u ) )
			{
				bExitOut = true;
				break;
			}

			CL_ReadEntityUpdate( u );
		}

		// Exit out without finishing?
		if( bExitOut )
			break;
	}

	// Now process explicit deletes.
	CL_ReadDeletions( u.m_bUncompressed, u.m_bFinished );

	CL_CallPostDataUpdates( u );

	// Something didn't parse...
	if ( MSG_IsOverflowed() )							
	{	
		Host_Error ( "CL_ParsePacketEntities:  buffer read overflow\n" );
	}

	// If we get an uncompressed packet, then the server is waiting for us to ack the validsequence
	// that we got the uncompressed packet on. So we stop reading packets here and force ourselves to
	// send the clc_move on the next frame.
	//
	// If we didn't do this, we might wind up reading another entity packet from the server and incrementing
	// our validsequence before responding to the server, in which case the server wouldn't know for sure
	// if we got the uncompressed packet or not. Then it could get stuck in a loop where it keeps resending
	// the uncompressed packet.
	if ( bUncompressed )
	{
		cl.continue_reading_packets = false;
		cl.force_send_usercmd = true;
	} 
}

//-----------------------------------------------------------------------------
// Purpose: An svc_packetentities has just been parsed, deal with the
//  rest of the data stream.  This can be a delta from the baseline or from a previous
//  client frame for this client.
// Input  : delta - 
//			*playerbits - 
// Output : void CL_ParsePacketEntities
//-----------------------------------------------------------------------------
int _CL_ParsePacketEntities ( qboolean delta, int *localplayerbits, int *otherplayerbits )
{
	int				oldpacket, newpacket;
	CRecvEntList	*oldp, *newp;
	static	CRecvEntList	dummy;
	int			from;
	int				newp_number; // # of entities in new packet.
	int				nEntsInPacket;
	frame_t			*frame;
	int				entity_count = 0;


	VPROF( "_CL_ParsePacketEntities" );
	// Paranoia...
	dummy.Term();

	// Frame # of packet we are deltaing to
	newpacket	= cl.parsecountmod; 
	frame		= &cl.frames[newpacket];


	// Packed entities for that frame
	newp		= &frame->packet_entities;

	// Retrieve size of packet.
	newp_number = MSG_ReadShort();
	// Make sure that we always have at least one slot allocated in memory.
	nEntsInPacket = max( 1, newp_number );

	// If this is a delta update, grab the sequence value and make sure it's the one we requested.
	if ( delta )
	{
		// Frame we are delta'ing from.
		from		= MSG_GetReadBuf()->ReadUBitLong( DELTAFRAME_NUMBITS );
		oldpacket	= from;
	}
	else
	{
		// Delta too old or is initial message
		oldpacket = -1;             
		// we can start recording now that we've received an uncompressed packet
		demo->SetWaitingForUpdate( false );	
	}

	// Assume valid
	frame->invalid = false;

	TRACE_PACKET(( "CL Receive (%d <-%d)\n", cls.netchan.incoming_sequence, oldpacket ));
	TRACE_PACKET(( "CL Num Ents (%d)\n", nEntsInPacket ));

	int startbit = MSG_GetReadBuf()->GetNumBitsRead();
	int length = MSG_ReadBitLong( DELTASIZE_BITS );
	
	int endbit = startbit + length;
	if ( g_pLocalNetworkBackdoor )
	{
		MSG_GetReadBuf()->Seek( endbit );
	}
	else
	{
		// Free any old data.
		CL_ClearPacket( newp );

		// Allocate space for new packet info.
		newp->Init( nEntsInPacket );

		int headercount = MSG_ReadBitLong( MAX_EDICT_BITS );

		// See if we can use the packet still
		bool uncompressed;
		if ( oldpacket != -1 || cl_flushentitypacket.GetInt() )
		{
			if ( cl_flushentitypacket.GetInt() )
			{	
				// we can't use this, it is too old
				CL_FlushEntityPacket( endbit, newp, "Forced by cvar\n" );
				cl_flushentitypacket.SetValue(0);	// Reset the cvar.
				return 0;
			}

			// DELTAFRAME_MASK determines how long you can go before you're dropped.
			// CL_UPDATE_MASK is how long you can go before asking for uncompressed updates.
			Assert( CL_UPDATE_MASK < DELTAFRAME_MASK );

			int subtracted = ( ( ( cls.netchan.incoming_sequence & DELTAFRAME_MASK ) -  oldpacket ) & DELTAFRAME_MASK );
			if ( subtracted == 0 )
			{
				Host_Error( "Update too old, connection dropped.\n" );
				return 0;
			}

			// Have we already looped around and flushed this info?
			if ( subtracted >= CL_UPDATE_MASK )
			{	
				// we can't use this, it is too old
				CL_FlushEntityPacket( endbit, newp, "Update is too old\n" );
				cl_flushentitypacket.SetValue(0);	// Reset the cvar.
				return 0;
			}

			// Otherwise, mark where we are valid to and point to the packet entities we'll be updating from.
			oldp = &cl.frames[oldpacket&CL_UPDATE_MASK].packet_entities;

			uncompressed = false;
		}
		else
		{	
			// Use a fake old value with zero values.
			oldp = &dummy;          
			
			// Flag this is a full update
			uncompressed = true;

			// Clear out the client's entity states..
			for ( int i=0; i < entitylist->GetHighestEntityIndex(); i++ )
				CL_DeleteDLLEntity( i, "uncompressed update\n" );
		}

		g_nPropsDecoded = 0;

		unsigned char entity_in_pvs[ PAD_NUMBER( MAX_EDICTS, 8 ) ];
		entity_count = headercount;
		CL_ReadPacketEntities( endbit, headercount, oldp, newp, frame, localplayerbits, otherplayerbits, uncompressed, entity_in_pvs );
 		CL_MarkEntitiesOutOfPVS( entity_in_pvs );

		if ( cl_showdecodecount.GetInt() )
		{
			Msg( "%04d props decoded\n", g_nPropsDecoded );
		}

		Assert( MSG_GetReadBuf()->GetNumBitsRead() == endbit );
		TRACE_PACKET(( "CL Msg Length (%d)\n", length ));
	}

	// Mark current delta state
	cl.validsequence = cls.netchan.incoming_sequence;

	return entity_count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : delta - 
//			*playerbits - 
// Output : void CL_ParsePacketEntities
//-----------------------------------------------------------------------------
void CL_ParsePacketEntities ( bool delta, int *localplayerbits, int *otherplayerbits )
{
	*localplayerbits = 0;
	*otherplayerbits = 0;

	// Tell prediction that we're recreating entities due to an uncompressed packet arriving
	if ( g_pClientSidePrediction && !delta )
	{
		g_pClientSidePrediction->OnReceivedUncompressedPacket();
	}

	int entity_count = _CL_ParsePacketEntities( delta, localplayerbits, otherplayerbits );

	// First update is the final signon stage where we actually receive an entity (i.e., the world at least)
	bool playingdemo = demo->IsPlayingBack();

	if ( ( cls.signon == SIGNONS - 1 ) && ( !playingdemo || entity_count > 0 ) )
	{	
		// We are done with signon sequence.
		cls.signon = SIGNONS;
		
		// Clear loading plaque.
		CL_SignonReply (); 
		
		// FIXME: Please oh please move this out of this spot...
		// It so does not belong here. Instead, we want some phase of the
		// client DLL where it knows its read in all entities
		int i;
		if( i = CommandLine()->FindParm( "-buildcubemaps" ) )
		{
			// FIXME: Sucky, figure out logic for this crap
			SpatialPartition()->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

			int numIterations = 1;
			if( CommandLine()->ParmCount() > i + 1 )
			{
				numIterations = atoi( CommandLine()->GetParm(i+1) );
			}
			if( numIterations == 0 )
			{
				numIterations = 1;
			}
			void R_BuildCubemapSamples( int numIterations );
			R_BuildCubemapSamples( numIterations );
			Cbuf_AddText( "quit\n" );

			SpatialPartition()->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
		}
	}
}

void CL_Parse_PacketEntities( void )
{
	CL_ParsePacketEntities ( false, &cls.localplayerbits, &cls.otherplayerbits );  // delta from baseline
}

void CL_Parse_DeltaPacketEntities( void )
{
	CL_ParsePacketEntities (true, &cls.localplayerbits, &cls.otherplayerbits );   // delta from previous
}
