//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include <windows.h>
#include "vmpi.h"
#include "vmpi_distribute_work.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "vmpi_dispatch.h"
#include "pacifier.h"
#include "vstdlib/random.h"
#include "mathlib.h"
#include "threadhelpers.h"
#include "threads.h"
#include "vstdlib/strtools.h"
#include "cmdlib.h"


typedef unsigned short WUIndexType;


#define MAX_DW_CALLS	8

// Subpacket IDs owned by DistributeWork.
#define DW_SUBPACKETID_MASTER_READY		0
#define DW_SUBPACKETID_WORKER_READY		1
#define DW_SUBPACKETID_MASTER_FINISHED	2
#define DW_SUBPACKETID_WU_RESULTS		4
#define DW_SUBPACKETID_WU_ASSIGNMENT	5

// The master sends a list of which work units have been completed either N times per second or
// when a certain # of work units have been completed.
#define COMPLETED_WU_BATCH_SIZE	128
#define COMPLETED_WU_INTERVAL	200 // Send WU list out 5x / second.


class CWorkerInfo
{
public:
	ProcessWorkUnitFn m_pProcessFn;

	// Threads eat up the list of WUs in here.
	CCriticalSection m_CS;
	CUtlLinkedList<int,int> m_WorkUnits;
	CUtlVector<LONG> m_WorkUnitsTaken;	// This prevents us from redoing work units we've already done.
										// Sometimes, one thread is doing a work unit, and in the meantime,
										// the master reassigns that work unit, and the other thread picks it up.
										// Then it's doing the work unit twice simultaneously (and touching all
										// the same data!)
};


class CWorkUnitInfo
{
public:
	int m_iWorkUnit;
};


class CWULookupInfo
{
public:
	int m_iWUInfo;	// Index into m_WUInfo.
	int m_iPartition;		// Which partition it's in.
	int m_iPartitionListIndex;	// Index into its partition's m_WUs.
};


class CPartitionInfo
{
public:
	int m_iPartition;	// Index into m_Partitions.
	int m_iWorker; // Who owns this partition?
	CUtlLinkedList<WUIndexType,int> m_WUs;	// Which WUs are in this partition?
};


class CMasterInfo
{
public:

	// Only used by the master.
	ReceiveWorkUnitFn m_ReceiveFn;

	CUtlLinkedList<CPartitionInfo*,int> m_Partitions;	
	CUtlVector<CWULookupInfo> m_WULookup;		// Map work unit index to CWorkUnitInfo.
	CUtlLinkedList<CWorkUnitInfo,int> m_WUInfo;	// Sorted with most elegible WU at the head.
};


class CDSInfo
{
public:
	CWorkerInfo m_WorkerInfo;
	CMasterInfo m_MasterInfo;

	bool m_bMasterReady;	// Set to true when the master is ready to go.
	bool m_bMasterFinished;
	WUIndexType m_nWorkUnits;
	char m_cPacketID;
};

CDSInfo g_DSInfo[MAX_DW_CALLS];
unsigned short g_iCurDSInfo = (unsigned short)-1;	// This is incremented each time DistributeWork is called.

// This is only valid if we're a worker and if the worker currently has threads chewing on work units.
CDSInfo *g_pCurWorkerThreadsInfo = NULL;

double g_flMPIStartTime;
CUtlVector<int>	g_wuCountByProcess;

int g_nWUs;				// How many work units there were this time around.
int g_nDuplicatedWUs;	// How many times a worker sent results for a work unit that was already completed.

// Set to true if Error() is called and we want to exit early. vrad and vvis check for this in their
// thread functions, so the workers quit early when the master is done rather than finishing up
// potentially time-consuming work units they're working on.
bool g_bVMPIEarlyExit = false;

bool g_bMasterDistributingWork = false;


CCriticalSection g_TestCS;



int SortByWUCount( const void *elem1, const void *elem2 )
{
	int a = g_wuCountByProcess[ *((const int*)elem1) ];
	int b = g_wuCountByProcess[ *((const int*)elem2) ];
	if ( a < b )
		return 1;
	else if ( a == b )
		return 0;
	else	   
		return -1;
}



void ShowMPIStats( 
	double flTimeSpent,
	unsigned long nBytesSent, 
	unsigned long nBytesReceived, 
	unsigned long nMessagesSent, 
	unsigned long nMessagesReceived )
{
	double flKSent = (nBytesSent + 511) / 1024;
	double flKRecv = (nBytesReceived + 511) / 1024;

	bool bOldSuppress = g_bSuppressPrintfOutput;
	g_bSuppressPrintfOutput = true;

		Msg( "\n\n--------------------------------------------------------------\n");
		Msg( "Total Time       : %.2f\n", flTimeSpent );
		Msg( "Total Bytes Sent : %dk (%.2fk/sec, %d messages)\n", (int)flKSent, flKSent / flTimeSpent, nMessagesSent );
		Msg( "Total Bytes Recv : %dk (%.2fk/sec, %d messages)\n", (int)flKRecv, flKRecv / flTimeSpent, nMessagesReceived );
		if ( g_bMPIMaster )
		{
			Msg( "Duplicated WUs   : %d (%.1f%%)\n", g_nDuplicatedWUs, (float)g_nDuplicatedWUs * 100.0f / g_nWUs );

			Msg( "WU count by proc:\n" );

			int nProcs = VMPI_GetCurrentNumberOfConnections();
			
			CUtlVector<int> sortedProcs;
			sortedProcs.SetSize( nProcs );
			for ( int i=0; i < nProcs; i++ )
				sortedProcs[i] = i;

			qsort( sortedProcs.Base(), nProcs, sizeof( int ), SortByWUCount );

			for ( i=0; i < nProcs; i++ )
			{
				if ( sortedProcs[i] == 0 )
					continue;

				const char *pMachineName = VMPI_GetMachineName( sortedProcs[i] );
				Msg( "%s", pMachineName );
				
				char formatStr[512];
				Q_snprintf( formatStr, sizeof( formatStr ), "%%%ds %d\n", 30 - strlen( pMachineName ), g_wuCountByProcess[ sortedProcs[i] ] );
				Msg( formatStr, ":" );
			}
			Msg( "\n" );
		}
		Msg( "--------------------------------------------------------------\n\n ");

	g_bSuppressPrintfOutput = bOldSuppress;
}


int FindPartitionByWorker( CMasterInfo *pMasterInfo, int iWorker )
{
	FOR_EACH_LL( pMasterInfo->m_Partitions, i )
	{
		if ( pMasterInfo->m_Partitions[i]->m_iWorker == iWorker )
			return i;
	}
	return -1;
}


void SendPartitionToWorker( CDSInfo *pInfo, CPartitionInfo *pPartition, int iWorker )
{
	// Stuff the next nWUs work units into the buffer.
	MessageBuffer mb;
	char cPacketID[2] = { pInfo->m_cPacketID, DW_SUBPACKETID_WU_ASSIGNMENT };
	mb.write( cPacketID, 2 );
	mb.write( &g_iCurDSInfo, sizeof( g_iCurDSInfo ) );
	
	FOR_EACH_LL( pPartition->m_WUs, i )
	{
		WUIndexType iWU = pPartition->m_WUs[i];
		mb.write( &iWU, sizeof( iWU ) );
	}
	
	VMPI_SendData( mb.data, mb.getLen(), iWorker );
}


CPartitionInfo* AddPartition( CDSInfo *pInfo, int iWorker )
{
	CPartitionInfo *pNew = new CPartitionInfo;
	pNew->m_iPartition = pInfo->m_MasterInfo.m_Partitions.AddToTail( pNew );
	pNew->m_iWorker = iWorker;
	return pNew;
}


int FindLargestPartition( CMasterInfo *pMasterInfo )
{
	int iLargest = pMasterInfo->m_Partitions.Head();
	FOR_EACH_LL( pMasterInfo->m_Partitions, i )
	{
		if ( pMasterInfo->m_Partitions[i]->m_WUs.Count() > pMasterInfo->m_Partitions[iLargest]->m_WUs.Count() )
			iLargest = i;
	}
	return iLargest;
}


void VMPI_DistributeWork_DisconnectHandler( int procID, const char *pReason )
{
	if ( g_bMasterDistributingWork )
	{
		Warning( "VMPI_DistributeWork_DisconnectHandler( %d )\n", procID );
	
		// Redistribute the WUs from this guy's partition to another worker.
		CDSInfo *pInfo = &g_DSInfo[g_iCurDSInfo];
		CMasterInfo *pMasterInfo = &pInfo->m_MasterInfo;
		int iPartitionLookup = FindPartitionByWorker( pMasterInfo, procID );
		if ( iPartitionLookup != -1 )
		{
			// Mark this guy's partition as unowned so another worker can get it.
			CPartitionInfo *pPartition = pMasterInfo->m_Partitions[iPartitionLookup];
			pPartition->m_iWorker = -1;
		}
	}
}


void TransferWUs( CDSInfo *pInfo, CPartitionInfo *pFrom, CPartitionInfo *pTo, int nWUs, bool bReverse )
{
	CMasterInfo *pMasterInfo = &pInfo->m_MasterInfo;

	for ( int i=0; i < nWUs; i++ )
	{
		int iHead = pFrom->m_WUs.Head();
		int iWU = pFrom->m_WUs[iHead];
		pFrom->m_WUs.Remove( iHead );

		pMasterInfo->m_WULookup[iWU].m_iPartition = pTo->m_iPartition;
		if ( bReverse )
			pMasterInfo->m_WULookup[iWU].m_iPartitionListIndex = pTo->m_WUs.AddToHead( (WUIndexType)iWU );
		else
			pMasterInfo->m_WULookup[iWU].m_iPartitionListIndex = pTo->m_WUs.AddToTail( (WUIndexType)iWU );
	}
}


void AssignWUsToWorker( CDSInfo *pInfo, int iWorker )
{
	CMasterInfo *pMasterInfo = &pInfo->m_MasterInfo;

	// Get rid of this worker's old partition.
	int iPrevious = FindPartitionByWorker( pMasterInfo, iWorker );
	if ( iPrevious != -1 )
	{
		delete pMasterInfo->m_Partitions[iPrevious];
		pMasterInfo->m_Partitions.Remove( iPrevious );
	}

	if ( g_iVMPIVerboseLevel >= 1 )
		Msg( "A" );

	// Any partitions yet?
	if ( pMasterInfo->m_Partitions.Count() == 0 )
	{
		// None. Assign all of the work units to this worker.
		CPartitionInfo *pPartition = new CPartitionInfo;
		int iPartition = pMasterInfo->m_Partitions.AddToTail( pPartition );

		pPartition->m_iWorker = iWorker;
		for ( WUIndexType i=0; i < pInfo->m_nWorkUnits; i++ )
		{
			pMasterInfo->m_WULookup[i].m_iPartition = iPartition;
			pMasterInfo->m_WULookup[i].m_iPartitionListIndex = pPartition->m_WUs.AddToTail( i );
		}

		// Now send this guy the WU list in his partition.
		SendPartitionToWorker( pInfo, pPartition, iWorker );
	}
	else
	{
		// Find an unowned partition.
		int iUnowned = FindPartitionByWorker( pMasterInfo, -1 );
		if ( iUnowned == -1 )
		{
			// Find the largest remaining partition.
			int iLargest = FindLargestPartition( pMasterInfo );
			CPartitionInfo *pPartition = pMasterInfo->m_Partitions[iLargest];
				
				int iOldWorker = pPartition->m_iWorker;

				// Split it in twain.
				int nNew1 = max( 0, pPartition->m_WUs.Count() / 2 - 1 );
				int nNew2 = pPartition->m_WUs.Count() - nNew1;

				// Send half to the old worker.
				if ( nNew1 )
				{
					CPartitionInfo *pNew1 = AddPartition( pInfo, iOldWorker );
					TransferWUs( pInfo, pPartition, pNew1, nNew1, false );
					SendPartitionToWorker( pInfo, pNew1, iOldWorker );
				}

				// Send half to the new worker.
				CPartitionInfo *pNew2 = AddPartition( pInfo, iWorker );
				TransferWUs( pInfo, pPartition, pNew2, nNew2, false );
				SendPartitionToWorker( pInfo, pNew2, iWorker );

			Assert( pPartition->m_WUs.Count() == 0 );
			delete pPartition;
			pMasterInfo->m_Partitions.Remove( iLargest );
		}
		else
		{
			CPartitionInfo *pPartition = pMasterInfo->m_Partitions[iUnowned];
			pPartition->m_iWorker = iWorker;
			SendPartitionToWorker( pInfo, pPartition, iWorker );
		}
	}
}


bool DistributeWorkDispatch( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	unsigned short iCurDW = *((unsigned short*)&pBuf->data[2]);
	if ( iCurDW >= ARRAYSIZE( g_DSInfo ) )
		Error( "Got an invalid DistributeWork packet (id: %d, sub: %d) (iCurDW: %d).", pBuf->data[0], pBuf->data[1], iCurDW );

	CDSInfo *pInfo = &g_DSInfo[iCurDW];
	CWorkerInfo *pWorkerInfo = &pInfo->m_WorkerInfo;
	CMasterInfo *pMasterInfo = &pInfo->m_MasterInfo;
		
	pBuf->setOffset( 4 );
	switch ( pBuf->data[1] )
	{
		case DW_SUBPACKETID_MASTER_READY:
		{
			pInfo->m_bMasterReady = true;
			return true;
		}

		case DW_SUBPACKETID_WORKER_READY:
		{
			if ( iCurDW > g_iCurDSInfo || !g_bMPIMaster )
				Error( "State incorrect on master for DW_SUBPACKETID_WORKER_READY packet from %s.", VMPI_GetMachineName( iSource ) );

			if ( iCurDW == g_iCurDSInfo )
			{
				// Ok, give this guy some WUs.
				AssignWUsToWorker( pInfo, iSource );
			}

			return true;
		}

		case DW_SUBPACKETID_MASTER_FINISHED:
		{
			pInfo->m_bMasterFinished = true;
			return true;
		}
		
		case DW_SUBPACKETID_WU_ASSIGNMENT:
		{
			if ( iCurDW == g_iCurDSInfo )
			{
				if ( ((pBuf->getLen() - pBuf->getOffset()) % 2) != 0 )
				{
					Error( "DistributeWork: invalid work units packet from master" );
				}

				// Parse out the work unit indices.
				CCriticalSectionLock csLock( &pWorkerInfo->m_CS );
				csLock.Lock();

					pWorkerInfo->m_WorkUnits.Purge();

					int nIndices = (pBuf->getLen() - pBuf->getOffset()) / 2;
					for ( int i=0; i < nIndices; i++ )
					{
						unsigned short iWU;
						pBuf->read( &iWU, sizeof( iWU ) );

						// Add the index to the list.
						pWorkerInfo->m_WorkUnits.AddToTail( iWU );
					}
				
				csLock.Unlock();
			}

			return true;
		}

		case DW_SUBPACKETID_WU_RESULTS:
		{
			// We only care about work results for the iteration we're in.
			if ( iCurDW != g_iCurDSInfo )
				return true;

			unsigned short iWorkUnit;
			pBuf->read( &iWorkUnit, sizeof( iWorkUnit ) );
			if ( iWorkUnit >= pInfo->m_nWorkUnits )
			{
				Error( "DistributeWork: got an invalid work unit index (%d for WU count of %d).", iWorkUnit, pInfo->m_nWorkUnits );
			}
			
			// Ignore it if we already got the results for this work unit.
			CWULookupInfo *pLookup = &pMasterInfo->m_WULookup[iWorkUnit];
			if ( pLookup->m_iWUInfo == -1 )
			{
				g_nDuplicatedWUs++;
				if ( g_iVMPIVerboseLevel >= 1 )
					Msg( "*" );
				
			}
			else
			{
				if ( g_iVMPIVerboseLevel >= 1 )
					Msg( "-" );

				g_wuCountByProcess[iSource]++;

				// Mark this WU finished and remove it from the list of pending WUs.
				pMasterInfo->m_WUInfo.Remove( pLookup->m_iWUInfo );
				pLookup->m_iWUInfo = -1;	

				// Let the master process the incoming WU data.
				pMasterInfo->m_ReceiveFn( iWorkUnit, pBuf, iSource );


				// Get rid of the WU from its partition.
				int iPartition = pLookup->m_iPartition;
				CPartitionInfo *pPartition = pMasterInfo->m_Partitions[iPartition];
				pPartition->m_WUs.Remove( pLookup->m_iPartitionListIndex );


				// Give the worker some new work if need be.
				if ( pPartition->m_WUs.Count() == 0 )
				{
					int iPartitionWorker = pPartition->m_iWorker;
					delete pPartition;
					pMasterInfo->m_Partitions.Remove( iPartition );
			
					// If there are any more WUs remaining, give the worker from this partition some more of them.		
					if ( pMasterInfo->m_WUInfo.Count() > 0 )
					{
						AssignWUsToWorker( pInfo, iPartitionWorker );
					}
				}


				UpdatePacifier( (float)(pInfo->m_nWorkUnits - pMasterInfo->m_WUInfo.Count()) / pInfo->m_nWorkUnits );
			}

			return true;
		}

		default:
		{
			return false;
		}
	}
}


void PreDistributeWorkSync( CDSInfo *pInfo )
{
	if ( g_bMPIMaster )
	{
		// Send a message telling all the workers we're ready to go on this DistributeWork call.
		char id[2] = { pInfo->m_cPacketID, DW_SUBPACKETID_MASTER_READY };
		VMPI_Send2Chunks( id, sizeof( id ), &g_iCurDSInfo, sizeof( g_iCurDSInfo ), VMPI_PERSISTENT );
	}
	else
	{
		// 
		Msg( "PreDistributeWorkSync: waiting for master\n" );

		// Wait for the master's message saying it's ready to go.
		while ( !pInfo->m_bMasterReady )
		{
			VMPI_DispatchNextMessage();
		}

		Msg( "PreDistributeWorkSync: master ready\n" );

		// Now tell the master we're ready.
		char id[2] = { pInfo->m_cPacketID, DW_SUBPACKETID_WORKER_READY };
		VMPI_Send2Chunks( id, sizeof( id ), &g_iCurDSInfo, sizeof( g_iCurDSInfo ), VMPI_MASTER_ID );
	}
}


void DistributeWork_Master( CDSInfo *pInfo, ReceiveWorkUnitFn receiveFn )
{
	CMasterInfo *pMasterInfo = &pInfo->m_MasterInfo;
	pMasterInfo->m_ReceiveFn = receiveFn;

	pMasterInfo->m_WULookup.SetCount( pInfo->m_nWorkUnits );
	for ( int i=0; i < pInfo->m_nWorkUnits; i++ )
	{
		CWorkUnitInfo info;
		info.m_iWorkUnit = i;
		
		pMasterInfo->m_WULookup[i].m_iWUInfo = pMasterInfo->m_WUInfo.AddToTail( info );
		pMasterInfo->m_WULookup[i].m_iPartition = -222222;
	}


	// The workers know what to do. Now wait for the results.
	g_bMasterDistributingWork = true;
		while ( pMasterInfo->m_WUInfo.Count() > 0 )
		{
			VMPI_DispatchNextMessage( 200 );
		}
	g_bMasterDistributingWork = false;
		
	// Tell all workers to move on
	char cPacket[2] = { pInfo->m_cPacketID, DW_SUBPACKETID_MASTER_FINISHED };
	VMPI_Send2Chunks( cPacket, sizeof( cPacket ), &g_iCurDSInfo, sizeof( g_iCurDSInfo ), VMPI_PERSISTENT );
}


void VMPI_WorkerThread( int iThread, void *pUserData )
{
	CDSInfo *pInfo = (CDSInfo*)pUserData;
	CWorkerInfo *pWorkerInfo = &pInfo->m_WorkerInfo;

	MessageBuffer mb;

	char cPacketID[2] = { pInfo->m_cPacketID, DW_SUBPACKETID_WU_RESULTS };
	mb.write( cPacketID, sizeof( cPacketID ) );
	mb.write( &g_iCurDSInfo, sizeof( g_iCurDSInfo ) );

	while ( !pInfo->m_bMasterFinished )
	{
		unsigned short iWU = 0;

		CCriticalSectionLock csLock( &pWorkerInfo->m_CS );
		csLock.Lock();

			// Quit out when there are no more work units.
			if ( pWorkerInfo->m_WorkUnits.Count() == 0 )
			{
				// Wait until there are some WUs to do. This should probably use event handles.
				VMPI_Sleep( 10 );
				continue;
			}
			else
			{
				iWU = (unsigned short)pWorkerInfo->m_WorkUnits[pWorkerInfo->m_WorkUnits.Head()];
				pWorkerInfo->m_WorkUnits.Remove( pWorkerInfo->m_WorkUnits.Head() );
			}
	
		csLock.Unlock();

		// Make sure not to do the same work unit twice (especially simultaneously by multiple threads!)
		if ( InterlockedIncrement( &pWorkerInfo->m_WorkUnitsTaken[iWU] ) == 1 )
		{
			// Process this WU and send the results to the master.
			mb.setLen( 4 );
			mb.write( &iWU, sizeof( iWU ) );

			pWorkerInfo->m_pProcessFn( iThread, iWU, &mb );

			
			// TODO: this fixes the bug that Iikka and Dario reported. What would happen is that some of the
			// workers thought they had sent a work unit to the master, but the master never received the message.
			// Apparently, when there are multiple threads, this call screws up somehow. This is a temporary fix
			// until I verify that this is the place where the problem is.
			CCriticalSectionLock csLock( &g_TestCS );
			csLock.Lock();

				VMPI_SendData( mb.data, mb.getLen(), VMPI_MASTER_ID );

			csLock.Unlock();
		}
	}

	Msg( "Worker thread exiting.\n" );
}


void DistributeWork_Worker( CDSInfo *pInfo, ProcessWorkUnitFn processFn )
{
	Msg( "VMPI_DistributeWork call %d started.\n", g_iCurDSInfo+1 );

	CWorkerInfo *pWorkerInfo = &pInfo->m_WorkerInfo;
	pWorkerInfo->m_pProcessFn = processFn;
	pWorkerInfo->m_WorkUnitsTaken.SetSize( pInfo->m_nWorkUnits );
	for ( int i=0; i < pWorkerInfo->m_WorkUnitsTaken.Count(); i++ )
		pWorkerInfo->m_WorkUnitsTaken[i] = 0;
	
	// Start a couple threads to do the work.
	RunThreads_Start( VMPI_WorkerThread, pInfo );
	Msg( "RunThreads_Start finished successfully.\n" );
	
	g_pCurWorkerThreadsInfo = pInfo;

		while ( !pInfo->m_bMasterFinished )
		{
			VMPI_DispatchNextMessage();
		}

	// Close the threads.
	g_pCurWorkerThreadsInfo = NULL;
	Msg( "Calling RunThreads_End...\n" );
	RunThreads_End();

	Msg( "VMPI_DistributeWork call %d finished.\n", g_iCurDSInfo+1 );
}


// This is called by VMPI_Finalize in case it's shutting down due to an Error() call.
// In this case, it's important that the worker threads here are shut down before VMPI shuts
// down its sockets.
void DistributeWork_Cancel()
{
	if ( g_pCurWorkerThreadsInfo )
	{
		Msg( "\nDistributeWork_Cancel saves the day!\n" );
		g_pCurWorkerThreadsInfo->m_bMasterFinished = true;
		g_bVMPIEarlyExit = true;
		RunThreads_End();
	}
}


// Returns time it took to finish the work.
double DistributeWork( 
	int nWorkUnits,					// how many work units to dole out
	char cPacketID,
	ProcessWorkUnitFn processFn,	// workers implement this to process a work unit and send results back
	ReceiveWorkUnitFn receiveFn		// the master implements this to receive a work unit
	)
{
	++g_iCurDSInfo;
	if ( g_iCurDSInfo == 0 )
	{
		// Register our disconnect handler so we can deal with it if clients bail out.
		if ( g_bMPIMaster )
		{
			VMPI_AddDisconnectHandler( VMPI_DistributeWork_DisconnectHandler );
		}
	}
	else if ( g_iCurDSInfo >= ARRAYSIZE( g_DSInfo ) )
	{
		Error( "DistributeWork: called more than %d times.\n", ARRAYSIZE( g_DSInfo ) );
	}

	CDSInfo *pInfo = &g_DSInfo[g_iCurDSInfo];
	
	pInfo->m_cPacketID = cPacketID;
	pInfo->m_nWorkUnits = nWorkUnits;

	// Make all the workers wait until the master is ready.
	PreDistributeWorkSync( pInfo );

	g_nWUs = nWorkUnits;
	g_nDuplicatedWUs = 0;

	// Setup stats info.
	g_flMPIStartTime = Plat_FloatTime();
	g_wuCountByProcess.SetCount( 512 );
	memset( g_wuCountByProcess.Base(), 0, sizeof( int ) * g_wuCountByProcess.Count() );
	
	unsigned long nBytesSentStart = g_nBytesSent;
	unsigned long nBytesReceivedStart = g_nBytesReceived;
	unsigned long nMessagesSentStart = g_nMessagesSent;
	unsigned long nMessagesReceivedStart = g_nMessagesReceived;

	if ( g_bMPIMaster )
	{
		DistributeWork_Master( pInfo, receiveFn );
	}
	else 
	{
		DistributeWork_Worker( pInfo, processFn );
	}

	double flTimeSpent = Plat_FloatTime() - g_flMPIStartTime;
	ShowMPIStats( 
		flTimeSpent,
		g_nBytesSent - nBytesSentStart,
		g_nBytesReceived - nBytesReceivedStart,
		g_nMessagesSent - nMessagesSentStart,
		g_nMessagesReceived - nMessagesReceivedStart
		);
	
	return flTimeSpent;
}

