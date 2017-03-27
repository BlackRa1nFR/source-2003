/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLList.cpp
Owner:	rich@woodbridgeinternalmed.com
Purpose:	A List, and List Entries
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLList::CHTMLList(	bool bOrdered )
	: CHTMLParagraphObject( CHTMLParagraphObject::knList )
	, m_bOrdered( bOrdered )
{

}


CHTMLList::~CHTMLList()
{
	for( UINT nIndex = 0; nIndex < m_arrItems.GetSize(); nIndex++ )
	{
		delete m_arrItems[ nIndex ];
	}
	m_arrItems.RemoveAll();
}

void CHTMLList::AddItem( CHTMLListItem *pItem )
{
	m_arrItems.Add( pItem );
}

#ifdef _DEBUG
void CHTMLList::Dump() const
{
	const UINT size = m_arrItems.GetSize();

	TRACENL( _T("List----------------\n") );
	TRACENL( _T(" Size (%d)\n"), size );
	TRACENL( _T(" Ordered (%s)\n"), (m_bOrdered ? _T("true") : _T("false") ));

	for( UINT nIndex = 0; nIndex < size; nIndex++ )
	{
		TRACENL( _T(" Item %d\n"), nIndex );
		m_arrItems[ nIndex ]->Dump();
	}
}
#endif	//	_DEBUG

