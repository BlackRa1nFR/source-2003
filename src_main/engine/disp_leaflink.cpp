//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "disp_leaflink.h"


CMemoryPool g_DispLeafMPool( sizeof(CDispLeafLink), 32 );


void CDispLeafLink::Add( void *pDispInfo, 
		CDispLeafLink*& pDispHead, 
		void *pLeaf, 
		CDispLeafLink*& pLeafHead
)
{
	// Store off pointers.
	m_pDispInfo = pDispInfo;
	m_pLeaf = pLeaf;

	// Link into the displacement's list of leaves.
	if( pDispHead )
	{
		m_pPrev[LIST_DISP] = pDispHead;
		m_pNext[LIST_DISP] = pDispHead->m_pNext[LIST_DISP];
	}
	else
	{
		pDispHead = m_pPrev[LIST_DISP] = m_pNext[LIST_DISP] = this;
	}
	m_pPrev[LIST_DISP]->m_pNext[LIST_DISP] = m_pNext[LIST_DISP]->m_pPrev[LIST_DISP] = this;

	// Link into the leaf's list of displacements.
	if( pLeafHead )
	{
		m_pPrev[LIST_LEAF] = pLeafHead;
		m_pNext[LIST_LEAF] = pLeafHead->m_pNext[LIST_LEAF];
	}
	else
	{
		pLeafHead = m_pPrev[LIST_LEAF] = m_pNext[LIST_LEAF] = this;
	}
	m_pPrev[LIST_LEAF]->m_pNext[LIST_LEAF] = m_pNext[LIST_LEAF]->m_pPrev[LIST_LEAF] = this;
}

void CDispLeafLink::Remove( CDispLeafLink*& pDispHead, CDispLeafLink*& pLeafHead )
{
	// Remove from the displacement.
	m_pPrev[LIST_DISP]->m_pNext[LIST_DISP] = m_pNext[LIST_DISP];
	m_pNext[LIST_DISP]->m_pPrev[LIST_DISP] = m_pPrev[LIST_DISP];
	if( this == pDispHead )
	{
		if( m_pNext[LIST_DISP] == this )
			pDispHead = NULL;
		else
			pDispHead = m_pNext[LIST_DISP];
	}

	// Remove from the leaf.
	m_pPrev[LIST_LEAF]->m_pNext[LIST_LEAF] = m_pNext[LIST_LEAF];
	m_pNext[LIST_LEAF]->m_pPrev[LIST_LEAF] = m_pPrev[LIST_LEAF];
	if( this == pLeafHead )
	{
		if( m_pNext[LIST_LEAF] == this )
			pLeafHead = NULL;
		else
			pLeafHead = m_pNext[LIST_LEAF];
	}
}

