
#include "dt_stack.h"


CDatatableStack::CDatatableStack( CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID )
{
	m_pPrecalc = pPrecalc;

	m_pStructBase = pStructBase;
	m_ObjectID = objectID;
	m_nProxies = 0;
	
	m_iCurProp = 0;
	m_pCurProp = NULL;

	m_bInitted = false;
}


void CDatatableStack::Init()
{
	Assert( m_nProxies == 0 );

	// Walk down the tree and call all the datatable proxies as we hit them.
	RecurseAndCallProxies( &m_pPrecalc->m_Root, m_pStructBase );

	m_bInitted = true;
}


void CDatatableStack::RecurseAndCallProxies( CSendNode *pNode, unsigned char *pStructBase )
{
	if ( m_nProxies >= MAX_PROXY_RESULTS )
		Error( "CDatatableStack::RecurseAndCallProxies - overflowed" );
	
	// Remember where the game code pointed us for this datatable's data so 
	m_pProxies[m_nProxies] = pStructBase;
	++m_nProxies;

	for ( int iChild=0; iChild < pNode->GetNumChildren(); iChild++ )
	{
		CSendNode *pCurChild = pNode->GetChild( iChild );
		
		unsigned char *pNewStructBase = NULL;
		if ( pStructBase )
		{
			pNewStructBase = CallPropProxy( pCurChild, pCurChild->m_iDatatableProp, pStructBase );
		}

		RecurseAndCallProxies( pCurChild, pNewStructBase );
	}
}

