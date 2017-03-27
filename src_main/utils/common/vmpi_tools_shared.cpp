//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include <windows.h>
#include "vmpi.h"
#include "cmdlib.h"
#include "vmpi_tools_shared.h"
#include "vstdlib/strtools.h"
#include "mpi_stats.h"
#include "iphelpers.h"


// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //

static bool g_bReceivedDirectoryInfo = false;	// Have we gotten the qdir info yet?

static bool g_bReceivedDBInfo = false;
static CDBInfo g_DBInfo;
static unsigned long g_JobPrimaryID;

static bool g_bReceivedMulticastIP = false;
static CIPAddr g_MulticastIP;


// ----------------------------------------------------------------------------- //
// Shared dispatch code.
// ----------------------------------------------------------------------------- //

bool SharedDispatch( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	char *pInPos = &pBuf->data[2];

	switch ( pBuf->data[1] )
	{
		case VMPI_SUBPACKETID_DIRECTORIES:
		{
			Q_strncpy( basegamedir, pInPos, sizeof( basegamedir ) );
			pInPos += strlen( pInPos ) + 1;

			Q_strncpy( basedir, pInPos, sizeof( basedir ) );
			pInPos += strlen( pInPos ) + 1;

			Q_strncpy( gamedir, pInPos, sizeof( gamedir ) );
			pInPos += strlen( pInPos ) + 1;

			Q_strncpy( qdir, pInPos, sizeof( qdir ) );
			
			g_bReceivedDirectoryInfo = true;
		}
		return true;

		case VMPI_SUBPACKETID_DBINFO:
		{
			g_DBInfo = *((CDBInfo*)pInPos);
			pInPos += sizeof( CDBInfo );
			g_JobPrimaryID = *((unsigned long*)pInPos);

			g_bReceivedDBInfo = true;
		}
		return true;

		case VMPI_SUBPACKETID_MACHINE_NAME:
		{
			VMPI_SetMachineName( iSource, pInPos );
		}
		return true;

		case VMPI_SUBPACKETID_CRASH:
		{
			Warning( "\nWorker '%s' dead: %s\n", VMPI_GetMachineName( iSource ), pInPos );
		}
		return true;

		case VMPI_SUBPACKETID_MULTICAST_ADDR:
		{
			g_MulticastIP = *((CIPAddr*)pInPos);
			g_bReceivedMulticastIP = true;
		}
		return true;
	}

	return false;
}

CDispatchReg g_SharedDispatchReg( VMPI_SHARED_PACKET_ID, SharedDispatch );



// ----------------------------------------------------------------------------- //
// Module interfaces.
// ----------------------------------------------------------------------------- //

void SendQDirInfo()
{
	char cPacketID[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_DIRECTORIES };

	MessageBuffer mb;
	mb.write( cPacketID, 2 );
	mb.write( basegamedir, strlen( basegamedir ) + 1 );
	mb.write( basedir, strlen( basedir ) + 1 );
	mb.write( gamedir, strlen( gamedir ) + 1 );
	mb.write( qdir, strlen( qdir ) + 1 );

	VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );
}


void RecvQDirInfo()
{
	while ( !g_bReceivedDirectoryInfo )
		VMPI_DispatchNextMessage();
}


void SendDBInfo( const CDBInfo *pInfo, unsigned long jobPrimaryID )
{
	char cPacketInfo[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_DBINFO };
	const void *pChunks[] = { cPacketInfo, pInfo, &jobPrimaryID };
	int chunkLengths[] = { 2, sizeof( CDBInfo ), sizeof( jobPrimaryID ) };
	
	VMPI_SendChunks( pChunks, chunkLengths, ARRAYSIZE( pChunks ), VMPI_PERSISTENT );
}


void RecvDBInfo( CDBInfo *pInfo, unsigned long *pJobPrimaryID )
{
	while ( !g_bReceivedDBInfo )
		VMPI_DispatchNextMessage();

	*pInfo = g_DBInfo;
	*pJobPrimaryID = g_JobPrimaryID;
}


void SendMachineName()
{
	const char *pName = VMPI_GetLocalMachineName();
	char cPacketID[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_MACHINE_NAME };
	VMPI_Send2Chunks( cPacketID, sizeof( cPacketID ), pName, strlen( pName ) + 1, VMPI_PERSISTENT );
}


void VMPI_HandleCrash( const char *pMessage, bool bAssert )
{
	static LONG crashHandlerCount = 0;
	if ( InterlockedIncrement( &crashHandlerCount ) == 1 )
	{
		Msg( "\nFAILURE: '%s' (assert: %d)\n", pMessage, bAssert );

		// Send a message to the master.
		char crashMsg[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_CRASH };

		VMPI_Send2Chunks( 
			crashMsg, 
			sizeof( crashMsg ), 
			pMessage,
			strlen( pMessage ) + 1,
			VMPI_MASTER_ID );

		// Let the messages go out.
		Sleep( 500 );

		VMPI_Finalize();

		Sleep( 500 );
	}

	InterlockedDecrement( &crashHandlerCount );
}


void SendMulticastIP( const CIPAddr *pAddr )
{
	unsigned char packetID[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_MULTICAST_ADDR };
	VMPI_Send2Chunks( 
		packetID, sizeof( packetID ),
		pAddr, sizeof( *pAddr ),
		VMPI_PERSISTENT );
}


void RecvMulticastIP( CIPAddr *pAddr )
{
	while ( !g_bReceivedMulticastIP )
		VMPI_DispatchNextMessage();

	*pAddr = g_MulticastIP;
}
