#include "glquake.h"
#include "dt_stack.h"
#include "utllinkedlist.h"
#include "server.h"
#include "server_class.h"
#include "eiface.h"

extern CUtlLinkedList< CClientSendTable*, unsigned short > g_ClientSendTables;
extern CUtlLinkedList< CRecvDecoder *, unsigned short > g_RecvDecoders;

RecvTable* FindRecvTable( const char *pName );

static char *CopyString( const char *in )
{
	int len = strlen( in );
	char *n = new char[ len + 1 ];
	strcpy( n, in );
	return n;
}

void DataTable_SetupReceiveTableFromSendTable( SendTable *sendTable, bool bNeedsDecoder )
{
	CClientSendTable *pClientSendTable = new CClientSendTable;
	SendTable *pTable = &pClientSendTable->m_SendTable;
	g_ClientSendTables.AddToTail( pClientSendTable );

	// Read the name.
	pTable->m_pNetTableName = CopyString( sendTable->m_pNetTableName );

	// Create a decoder for it if necessary.
	if ( bNeedsDecoder )
	{
		// Make a decoder for it.
		CRecvDecoder *pDecoder = new CRecvDecoder;
		g_RecvDecoders.AddToTail( pDecoder );
		
		RecvTable *pRecvTable = FindRecvTable( pTable->m_pNetTableName );
		if ( !pRecvTable )
		{
			DataTable_Warning( "No matching RecvTable for SendTable '%s'.\n", pTable->m_pNetTableName );
			return;
		}

		pRecvTable->m_pDecoder = pDecoder;
		pDecoder->m_pTable = pRecvTable;

		pDecoder->m_pClientSendTable = pClientSendTable;
		pDecoder->m_Precalc.m_pSendTable = pClientSendTable->GetSendTable();
		pClientSendTable->GetSendTable()->m_pPrecalc = &pDecoder->m_Precalc;

		// Initialize array properties.
		SetupArrayProps_R<RecvTable, RecvTable::PropType>( pRecvTable );
	}

	// Read the property list.
	pTable->m_nProps = sendTable->m_nProps;
	pTable->m_pProps = pTable->m_nProps ? new SendProp[ pTable->m_nProps ] : 0;
	pClientSendTable->m_Props.SetSize( pTable->m_nProps );

	for ( int iProp=0; iProp < pTable->m_nProps; iProp++ )
	{
		CClientSendProp *pClientProp = &pClientSendTable->m_Props[iProp];
		SendProp *pProp = &pTable->m_pProps[iProp];
		const SendProp *pSendTableProp = &sendTable->m_pProps[ iProp ];

		pProp->m_Type = (SendPropType)pSendTableProp->m_Type;
		pProp->m_pVarName = CopyString( pSendTableProp->GetName() );
		pProp->SetFlags( pSendTableProp->GetFlags() );

		if ( pProp->m_Type == DPT_DataTable )
		{
			pClientProp->SetTableName( CopyString( pSendTableProp->GetDataTable()->m_pNetTableName ) );
		}
		else
		{
			if ( pProp->IsExcludeProp() )
			{
				pProp->m_pExcludeDTName = CopyString( pSendTableProp->GetExcludeDTName() );
			}
			else if ( pProp->GetType() == DPT_String )
			{
				pProp->m_StringBufferLen = pSendTableProp->m_StringBufferLen;
			}
			else if ( pProp->GetType() == DPT_Array )
			{
				pProp->SetNumElements( pSendTableProp->GetNumElements() );
			}
			else
			{
				pProp->m_fLowValue = pSendTableProp->m_fLowValue;
				pProp->m_fHighValue = pSendTableProp->m_fHighValue;
				pProp->m_nBits = pSendTableProp->m_nBits;
			}
		}
	}
}

// If the table's ID is -1, writes its info into the buffer and increments curID.
void DataTable_MaybeCreateReceiveTable( CUtlVector< SendTable * >& visited, SendTable *pTable, bool bNeedDecoder )
{
	// Already sent?
	if ( visited.Find( pTable ) != visited.InvalidIndex() )
		return;

	visited.AddToTail( pTable );

	DataTable_SetupReceiveTableFromSendTable( pTable, bNeedDecoder );
}


void DataTable_MaybeCreateReceiveTable_R( CUtlVector< SendTable * >& visited, SendTable *pTable )
{
	int i;

	DataTable_MaybeCreateReceiveTable( visited, pTable, false );

	// Make sure we send child send tables..
	for(i=0; i < pTable->m_nProps; i++)
	{
		SendProp *pProp = &pTable->m_pProps[i];

		if( pProp->m_Type == DPT_DataTable )
		{
			DataTable_MaybeCreateReceiveTable_R( visited, pProp->GetDataTable() );
		}
	}
}

void DataTable_CreateClientTablesFromServerTables()
{
	if ( !serverGameDLL )
	{
		Sys_Error( "DataTable_CreateClientTablesFromServerTables:  No serverGameDLL loaded!" );
	}

	ServerClass *pClasses = serverGameDLL->GetAllServerClasses();
	ServerClass *pCur;

	CUtlVector< SendTable * > visited;

	// First, we send all the leaf classes. These are the ones that will need decoders
	// on the client.
	for ( pCur=pClasses; pCur; pCur=pCur->m_pNext )
	{
		DataTable_MaybeCreateReceiveTable( visited, pCur->m_pTable, true );
	}

	// Now, we send their base classes. These don't need decoders on the client
	// because we will never send these SendTables by themselves.
	for ( pCur=pClasses; pCur; pCur=pCur->m_pNext )
	{
		DataTable_MaybeCreateReceiveTable_R( visited, pCur->m_pTable );
	}
}

void DataTable_CreateClientClassInfosFromServerClasses( CClientState *pState )
{
	ServerClass *pClasses = serverGameDLL->GetAllServerClasses();

	// Count the number of classes.
	int nClasses = 0;
	for ( ServerClass *pCount=pClasses; pCount; pCount=pCount->m_pNext )
	{
		++nClasses;
	}

	// Remove old
	if ( pState->m_pServerClasses )
	{
		delete [] pState->m_pServerClasses;
	}

	Assert( nClasses > 0 );

	pState->m_nServerClasses = nClasses;
	if(!(pState->m_pServerClasses = new C_ServerClassInfo[ pState->m_nServerClasses ]))
	{
		Host_EndGame("CL_ParseClassInfo: can't allocate %d C_ServerClassInfos.\n", pState->m_nServerClasses);
		return;
	}

	pState->m_nServerClassBits = Q_log2( pState->m_nServerClasses ) + 1;
	Assert( pState->m_nServerClassBits > 0 );
	// Make sure it's bounded!!!
	Assert( pState->m_nServerClassBits <= 16 );

	// Now fill in the entries
	int curID = 0;
	for ( ServerClass *pClass=pClasses; pClass; pClass=pClass->m_pNext )
	{
		Assert( pClass->m_ClassID >= 0 && pClass->m_ClassID < nClasses );

		pClass->m_ClassID = curID++;

		pState->m_pServerClasses[ pClass->m_ClassID ].m_ClassName = CopyString( pClass->m_pNetworkName );
		pState->m_pServerClasses[ pClass->m_ClassID ].m_DatatableName = CopyString( pClass->m_pTable->GetName() );
	}
}