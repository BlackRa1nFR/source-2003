/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	ParentSection.cpp
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "ParentSection.h"

CParentSection::CParentSection( CParentSection *pParent )
	: CSectionABC( pParent )
{

}

CParentSection::~CParentSection()
{

}


void CParentSection::OnDraw( CDrawContext &dc )
{
	const CRect &rcClip = dc.GetClipRect();
	const UINT uSize = m_arrSections.GetSize();

	for( UINT n = 0; n < uSize; n++ )
	{
		CSectionABC *psect = m_arrSections[ n ];
		if( rcClip.Intersect( *psect ) )
		{
			psect->OnDraw( dc );
		}
	}
}


void CParentSection::Transparent( bool bTransparent )
{
	CSectionABC::Transparent( bTransparent );

	const UINT uSize = m_arrSections.GetSize();

	for( UINT n = 0; n < uSize; n++ )
	{
		m_arrSections[ n ]->Transparent( bTransparent );
	}
}

void CParentSection::AddSection( CSectionABC *pSect )
{
	m_arrSections.Add( pSect );
}


void CParentSection::RemoveAllSections()
{
	const UINT uSize = m_arrSections.GetSize();
	for( UINT n = 0; n < uSize; n++ )
	{
		delete m_arrSections[ n ];
	}

	m_arrSections.RemoveAll();
}


CSectionABC * CParentSection::FindSectionFromPoint(const CPoint &pt) const
//
//	Given a point return pointer to a section if the pointer is within a section
//	Can return NULL if there is no section at the point.
{
	CSectionABC *pSect = NULL;
	const UINT uSize = m_arrSections.GetSize();
	for( UINT n = 0; n < uSize; n++ )
	{
		CSectionABC *psect = m_arrSections[ n ];
		if( psect->IsPointInSection( pt ) )
		{
			pSect = psect;
			CSectionABC *pSect2 = pSect->FindSectionFromPoint( pt );
			if( pSect2 )
			{
				pSect = pSect2;
			}
			break;
		}
	}
	return pSect;
}

