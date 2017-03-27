//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "dt_stack.h"
#include "dt_localtransfer.h"

void LocalTransfer_TransferEntity( 
	const SendTable *pSendTable, 
	const void *pSrcEnt, 
	RecvTable *pRecvTable, 
	void *pDestEnt,
	int objectID )
{
	// Setup the structure to traverse the source tree.
	CSendTablePrecalc *pPrecalc = pSendTable->m_pPrecalc;
	ErrorIfNot( pPrecalc, ("SendTable_Encode: Missing m_pPrecalc for SendTable %s.", pSendTable->m_pNetTableName) );
	CServerDatatableStack serverStack( pPrecalc, (unsigned char*)pSrcEnt, objectID );

	// Setup the structure to traverse the dest tree.
	CRecvDecoder *pDecoder = pRecvTable->m_pDecoder;
	ErrorIfNot( pDecoder, ("RecvTable_Decode: table '%s' missing a decoder.", pRecvTable->GetName()) );
	CClientDatatableStack clientStack( pDecoder, (unsigned char*)pDestEnt, objectID );

	// Walk through each property in each tree and transfer the data.
	int iEndProp = pPrecalc->m_Root.GetLastPropIndex();
	for ( int iProp=0; iProp <= iEndProp; iProp++ )
	{
		serverStack.SeekToProp( iProp );
		clientStack.SeekToProp( iProp );

		const SendProp *pSendProp = serverStack.GetCurProp();
		const RecvProp *pRecvProp = pDecoder->GetProp( iProp );
		if ( pRecvProp )
		{
			unsigned char *pSendBase = serverStack.GetCurStructBase();
			unsigned char *pRecvBase = clientStack.GetCurStructBase();
			if ( pSendBase && pRecvBase )
			{
				Assert( stricmp( pSendProp->GetName(), pRecvProp->GetName() ) == 0 );

				g_PropTypeFns[pRecvProp->GetType()].FastCopy( pSendProp, pRecvProp, pSendBase, pRecvBase, objectID );
			}
		}
	}
}



