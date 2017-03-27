//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef RECVENTLIST_H
#define RECVENTLIST_H
#ifdef _WIN32
#pragma once
#endif


#include "tier0/dbg.h"


//
// CRecvEntList just stores a list of the entity indices contained in a
// delta update packet.
//
class CRecvEntList
{
public:

			CRecvEntList();
			~CRecvEntList();
	
	void	Init( int nEnts );
	void	Term();

	int		GetNumEntities() const;

	// For an item in the array, get/set the entity index.
	int		GetEntityIndex( const int iItem ) const;
	void	SetEntityIndex( const int iItem, const int iEntity );

	// Copy a state from another CRecvEntList.
	void	CopyState( const CRecvEntList *pSrc, const int iSrcItem, const int iDestItem );


private:
	
	int		*m_pIndices;	// (was m_nEntityIndex).
	int		m_nEntities;
};


inline int CRecvEntList::GetNumEntities() const
{
	return m_nEntities;
}

inline int CRecvEntList::GetEntityIndex( const int iItem ) const
{
	Assert( iItem >= 0 && iItem < m_nEntities );
	int idx = m_pIndices[iItem];
	return idx;
}

inline void CRecvEntList::SetEntityIndex( const int iItem, const int iEntity )
{
	Assert( iItem >= 0 && iItem < m_nEntities );
	m_pIndices[iItem] = iEntity;
}

inline void CRecvEntList::CopyState( const CRecvEntList *pSrc, const int iSrcItem, const int iDestItem )
{
	SetEntityIndex( iDestItem, pSrc->GetEntityIndex( iSrcItem ) );
}


#endif // RECVENTLIST_H
