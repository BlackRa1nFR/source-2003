//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DATATABLE_STACK_H
#define DATATABLE_STACK_H
#ifdef _WIN32
#pragma once
#endif


#include "dt.h"
#include "dt_recv_decoder.h"


class CSendNode;


// ----------------------------------------------------------------------------- //
//
// CDatatableStack
//
// CDatatableStack is used to walk through a datatable's tree, calling proxies
// along the way to update the current data pointer.
//
// ----------------------------------------------------------------------------- //

class CDatatableStack
{
public:
	
							CDatatableStack( CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID );

	// This must be called before accessing properties.
	void Init();

	// The stack is meant to be used by calling SeekToProp with increasing property
	// numbers.
	void			SeekToProp( int iProp );

	bool			IsCurProxyValid() const;
	int				GetCurPropIndex() const;
	unsigned char*	GetCurStructBase() const;
	
	int				GetObjectID() const;


protected:

	// Derived classes must implement this. The server gets one and the client gets one.
	// It calls the proxy to move to the next datatable's data.
	virtual unsigned char*	CallPropProxy( CSendNode *pNode, int iProp, unsigned char *pStructBase ) = 0;


private:
	
	void RecurseAndCallProxies( CSendNode *pNode, unsigned char *pStructBase );


private:
	enum
	{
		MAX_ENCODESTACK_DEPTH = 64,
		MAX_PROXY_RESULTS = 64
	};

	// These point at the various values that the proxies returned. They are setup once, then 
	// the properties index them.
	unsigned char *m_pProxies[MAX_PROXY_RESULTS];
	int m_nProxies;
	CSendTablePrecalc *m_pPrecalc;
	unsigned char *m_pStructBase;

	int m_iCurProp;
	const SendProp *m_pCurProp;
	
	int m_ObjectID;

	bool m_bInitted;
};


inline bool CDatatableStack::IsCurProxyValid() const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[m_iCurProp]] != 0;
}

inline int CDatatableStack::GetCurPropIndex() const
{
	return m_iCurProp;
}

inline unsigned char* CDatatableStack::GetCurStructBase() const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[m_iCurProp]]; 
}

inline void CDatatableStack::SeekToProp( int iProp )
{
	// TODO: instead of checking this all the time, make whoever owns this object call it.
	if ( !m_bInitted )
	{
		Init();
	}

	m_iCurProp = iProp;
	m_pCurProp = m_pPrecalc->GetProp( iProp );
}

inline int CDatatableStack::GetObjectID() const
{
	return m_ObjectID;
}


// ------------------------------------------------------------------------------------ //
// The datatable stack for a RecvTable.
// ------------------------------------------------------------------------------------ //
class CClientDatatableStack : public CDatatableStack
{
public:
						CClientDatatableStack( CRecvDecoder *pDecoder, unsigned char *pStructBase, int objectID ) :
							CDatatableStack( &pDecoder->m_Precalc, pStructBase, objectID )
						{
							m_pDecoder = pDecoder;
						}

	virtual unsigned char*	CallPropProxy( CSendNode *pNode, int iProp, unsigned char *pStructBase )
	{
		const RecvProp *pProp = m_pDecoder->GetDatatableProp( iProp );

		void *pVal = NULL;

		Assert( pProp );

		pProp->GetDataTableProxyFn()( 
			&pVal,
			pStructBase + pProp->GetOffset(), 
			GetObjectID()
			);

		return (unsigned char*)pVal;
	}

public:
	
	CRecvDecoder	*m_pDecoder;
};


class CServerDatatableStack : public CDatatableStack
{
public:
						CServerDatatableStack( CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID ) :
							CDatatableStack( pPrecalc, pStructBase, objectID )
						{
							m_pPrecalc = pPrecalc;
							m_pRecipients = NULL;
						}

	virtual unsigned char*	CallPropProxy( CSendNode *pNode, int iProp, unsigned char *pStructBase )
	{
		const SendProp *pProp = m_pPrecalc->GetDatatableProp( iProp );

		CSendProxyRecipients recipients;
		recipients.SetAllRecipients();

		unsigned char *pRet = (unsigned char*)pProp->GetDataTableProxyFn()( 
			pStructBase, 
			pStructBase + pProp->GetOffset(), 
			&recipients,
			GetObjectID()
			);
	
		// Store off the recipients.
		if ( m_pRecipients && pProp->GetDataTableProxyIndex() != DATATABLE_PROXY_INDEX_NOPROXY )
			m_pRecipients->Element(pProp->GetDataTableProxyIndex()) = recipients;
	
		return pRet;
	}

	const SendProp*	GetCurProp() const;


public:
	
	CSendTablePrecalc					*m_pPrecalc;
	CUtlMemory<CSendProxyRecipients>	*m_pRecipients;
};


inline const SendProp* CServerDatatableStack::GetCurProp() const
{
	return m_pPrecalc->GetProp( GetCurPropIndex() );
}


#endif // DATATABLE_STACK_H
