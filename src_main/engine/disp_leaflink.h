//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DISP_LEAFLINK_H
#define DISP_LEAFLINK_H
#ifdef _WIN32
#pragma once
#endif


#include <stdio.h>
#include "mempool.h"
#include "basetypes.h"


// You can allocate CDispLeafLinks out of here.
extern CMemoryPool g_DispLeafMPool;



// This links a displacement into a leaf.
class CDispLeafLink
{
public:
	
	void			Add( 
		void *pDispInfo, 
		CDispLeafLink*& pDispHead, 
		void *pLeaf, 
		CDispLeafLink*& pLeafHead
		);
	
	void			Remove( CDispLeafLink*& pDispHead, CDispLeafLink*& pLeafHead );


public:

	enum
	{
		LIST_LEAF=0,		// the list that's chained into leaves.
		LIST_DISP=1,		// the list that's chained into displacements
		NUM_LISTS=2
	};	

	// It uses void*'s here because this link structure is used between
	// IDispInfo/mleaf's as well as CDispCollTree/cleaf_t.
	void			*m_pDispInfo;
	void			*m_pLeaf;

	CDispLeafLink	*m_pPrev[NUM_LISTS];
	CDispLeafLink	*m_pNext[NUM_LISTS];
};


// Helper class for iterating lists of displacements, either in 
// the displacements or in leaves.
class CDispIterator
{
public:
					CDispIterator( CDispLeafLink *pListHead, int iList );
	bool			IsValid();
	CDispLeafLink*	Inc();

private:
	CDispLeafLink	*m_pHead;
	CDispLeafLink	*m_pCur;
	int				m_iList;
};



// ---------------------------------------------------------------------- //
// Inlines
// ---------------------------------------------------------------------- //

inline CDispIterator::CDispIterator( CDispLeafLink *pListHead, int iList )
{
	m_pCur = m_pHead = pListHead;
	m_iList = iList;
}

inline bool CDispIterator::IsValid()
{
	return !!m_pCur;
}

inline CDispLeafLink* CDispIterator::Inc()
{
	CDispLeafLink *pRet = m_pCur;

	m_pCur = m_pCur->m_pNext[m_iList];
	if( m_pCur == m_pHead )
		m_pCur = NULL;

	return pRet;
}


#endif // DISP_LEAFLINK_H
