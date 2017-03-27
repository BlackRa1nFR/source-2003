//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "recventlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CRecvEntList::CRecvEntList()
{
	m_pIndices = 0;
	m_nEntities = 0;
}

CRecvEntList::~CRecvEntList()
{
	Term();
}

void CRecvEntList::Init( int nEnts )
{
	Term();
	Assert( nEnts != 0 );
	m_pIndices = new int[nEnts];

// Set the indices so we can detect if they've been initialized yet.	
#if defined( _DEBUG )	
	for ( int i=0; i < nEnts; i++ )
		m_pIndices[i] = -3333;
#endif

	m_nEntities = nEnts;
}

void CRecvEntList::Term()
{
	if ( m_pIndices )
	{
		delete [] m_pIndices;
		m_pIndices = 0;
	}

	m_nEntities = 0;
}

