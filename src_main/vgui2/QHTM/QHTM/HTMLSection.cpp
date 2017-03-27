/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLSection.cpp
Owner:	russf@gipsysoft.com
Purpose:	HTML Display section.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <SHELLAPI.H>	//	For ShellExecute
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "defaults.h"
#include "HTMLSectionABC.h"
#include "HTMLSection.h"
#include "HTMLParse.h"
#include "HTMLSectionCreator.h"
#include "ResourceLoader.h"
#include "smallstringhash.h"

extern bool LoadTextFile( LPCTSTR pcszFilename, LPTSTR &pszBuffer, UINT &uLength );

CHTMLSection::CHTMLSection( CParentSection *psectParent, CDefaults *pDefaults )
	: CScrollContainer( psectParent )
	, m_pDocument( NULL )
	, m_uZoomLevel( pDefaults->m_nZoomLevel )
	, m_pDefaults( pDefaults )
{
}


CHTMLSection::~CHTMLSection()
{
	DestroyDocument();

}


bool CHTMLSection::SetHTMLFile( LPCTSTR pcszFilename )
{
	bool bRetVal = false;

	LPTSTR pcszHTML = NULL;
	UINT uLength = 0;
	if( LoadTextFile( pcszFilename, pcszHTML, uLength ) )
	{
		TCHAR drive[_MAX_DRIVE];
		TCHAR dir[_MAX_DIR];
		TCHAR fname[_MAX_FNAME];
		TCHAR ext[_MAX_EXT];
		_tsplitpath( pcszFilename, drive, dir, fname, ext );
		TCHAR path_buffer[_MAX_PATH];
		_tmakepath( path_buffer, drive, dir, NULL, NULL );

		SetHTML( pcszHTML, uLength, path_buffer );

		bRetVal = true;

		delete pcszHTML;
	}
	return bRetVal;
}


void CHTMLSection::SetHTML( LPCTSTR pcszHTMLText, UINT uLength, LPCTSTR pcszPathToFile )
{
	CHTMLParse html( pcszHTMLText, uLength, GetModuleHandle( NULL ), pcszPathToFile, m_pDefaults );
	CHTMLDocument *pDoc = html.Parse();

	if( pDoc )
	{
#ifdef _DEBUG
//	pDoc->Dump();
#endif
		DestroyDocument();
		m_pDocument = pDoc;
	}
}


bool CHTMLSection::SetHTML( HINSTANCE hInst, LPCTSTR pcszName )
{
	CResourceLoader rsrc( hInst );

	if( rsrc.Load( pcszName, RT_RCDATA ) || rsrc.Load( pcszName, RT_HTML ) )
	{
		CHTMLDocument *pDoc = NULL;

#ifdef _UNICODE
		LPTSTR pszHTML = reinterpret_cast<LPTSTR>( malloc( rsrc.GetSize() * sizeof( TCHAR ) + 1 ) );
		{
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)rsrc.GetData(), rsrc.GetSize(), pszHTML, rsrc.GetSize() * sizeof( TCHAR ) );
			CHTMLParse html( pszHTML, rsrc.GetSize(), hInst, NULL, m_pDefaults );
#else	//	_UNICODE
			CHTMLParse html( (LPCTSTR)rsrc.GetData(), rsrc.GetSize(), hInst, NULL, m_pDefaults );
#endif	//	_UNICODE
			pDoc = html.Parse();

#ifdef _UNICODE
			free( pszHTML );
		}
#endif	//	_UNICODE

		if( pDoc )
		{
			DestroyDocument();
			m_pDocument = pDoc;
			return true;
		}
	}
	return false;
}


void CHTMLSection::OnLayout( const CRect &rc, CDrawContext &dc )
{
	RemoveAllChildSections();

	sizeHTML.cx = sizeHTML.cy = 0;
	if( m_pDocument && rc.Width() )
	{
		sizeHTML = LayoutDocument( dc, m_pDocument
			, rc.top + m_pDocument->m_nTopMargin		//	Start YPos
			, rc.left + m_pDocument->m_nLeftMargin		//	Left
			, rc.right - m_pDocument->m_nRightMargin - 1 );	//	Right
		sizeHTML.cy -= rc.top;
		sizeHTML.cx -= rc.left;
		sizeHTML.cx += m_pDocument->m_nRightMargin;
		sizeHTML.cy += m_pDocument->m_nBottomMargin;
	}

	int nScrollPos = GetScrollPos();
	int nScrollPosH = GetScrollPosH();
	Reset( sizeHTML.cx, sizeHTML.cy );
	CScrollContainer::OnLayout( rc );

	SetPos( nScrollPos );
	SetPosH( nScrollPosH );
}


void CHTMLSection::OnDraw( CDrawContext &dc )
{
	HBRUSH hbr = NULL;
	if( GetBackgroundColours( dc.GetSafeHdc(), hbr ) )
	{
		FillRect( dc.GetSafeHdc(), *this, hbr );
	}
	else if( !IsTransparent() )
	{
		if( m_pDocument )
			dc.FillRect( *this, m_pDocument->m_crBack );
		else
			dc.FillRect( *this, m_pDefaults->m_crBackground );
	}
	CScrollContainer::OnDraw( dc );
}


void CHTMLSection::DestroyDocument()
//
//	Remove the child sections and destroy the document
{
	RemoveAllChildSections();

	if( m_pDocument )
	{
		delete m_pDocument;
	}
}


void CHTMLSection::RemoveAllChildSections()
//
//	Remove all child sections prior to either recreating them or destroying them
{
	RemoveAllSections();

	// Empty the list of links.
	for (UINT i = 0; i < m_arrLinks.GetSize(); ++i)
		delete m_arrLinks[i];
	//	For safety...
	m_arrLinks.RemoveAll();

	// Clear the map
	m_mapNames.RemoveAll();
	//	Clear the pagination data
	m_arrBreakSections.RemoveAll();
	m_arrPageRects.RemoveAll();
}


void CHTMLSection::LoadFromResource( UINT uID )
{
	CResourceLoader rsrc;

	VERIFY( rsrc.Load( uID, RT_RCDATA ) );
	
	CHTMLDocument *pDoc = NULL;

#ifdef _UNICODE
	LPTSTR pszHTML = reinterpret_cast<LPTSTR>( malloc( rsrc.GetSize() * sizeof( TCHAR ) + 1 ) );
	{
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)rsrc.GetData(), rsrc.GetSize(), pszHTML, rsrc.GetSize() * sizeof( TCHAR ) );
		CHTMLParse html( pszHTML, rsrc.GetSize(), GetModuleHandle( NULL ), NULL, m_pDefaults );
#else	//	_UNICODE
		CHTMLParse html( (LPCTSTR)rsrc.GetData(), rsrc.GetSize(), GetModuleHandle( NULL ), NULL, m_pDefaults );
#endif	//	_UNICODE
		pDoc = html.Parse();

#ifdef _UNICODE
		free( pszHTML );
	}
#endif	//	_UNICODE

	if( pDoc )
	{
		DestroyDocument();
		m_pDocument = pDoc;
	}

}

void CHTMLSection::OnExecuteHyperlink( LPCTSTR pcszLink )
{
	ShellExecute( g_hwnd, _T("open"), pcszLink, NULL, NULL, SW_SHOW  );
}


void CHTMLSection::GotoLink( LPCTSTR pcszSection )
{
	//
	//	If a link has not been resolved and it is a local link then
	//	we should search the page for a matching link, when found we should
	//	scroll our position to .
	if( *pcszSection == '#' )
	{
		pcszSection++;
		// Locate it in the map...
		CPoint* pPoint = m_mapNames.Lookup( StringClass(pcszSection) );
		if (pPoint)
		{
			SetPos( pPoint->y - top );
			SetPosH( pPoint->x - left );
			ForceRedraw();
			NotifyParent( 1 );
		}
	}
	else
	{
		OnExecuteHyperlink( pcszSection );
	}
}


bool CHTMLSection::OnNotify( const CSectionABC *pChild, const int nEventID )
{
	if( NotifyParent( nEventID, pChild ) )
		return true;

	//
	//	We only expect links to notify us because they are our only 
	const CHTMLSectionABC *pSection = static_cast<const CHTMLSectionABC *>( pChild );
	LPCTSTR szTarget = pSection->GetLinkTarget();
	if (szTarget)
		GotoLink(szTarget);

	return true;
}



CSize CHTMLSection::LayoutDocument( CDrawContext &dc, CHTMLDocument *pDocument, int nYPos, int nLeft, int nRight )
{
	CHTMLSectionCreator htCreate( this, dc, nYPos, nLeft, nRight, pDocument->m_crBack, false, m_uZoomLevel );
	htCreate.AddDocument( pDocument );
	return htCreate.GetSize();
}

void CHTMLSection::EnableTooltips( bool bEnable )
{
	CHTMLSectionABC::EnableTooltips( bEnable );
}

bool CHTMLSection::IsTooltipsEnabled() const
{
	return CHTMLSectionABC::IsTooltipsEnabled();
}

CHTMLSectionLink* CHTMLSection::AddLink(LPCTSTR pcszLinkTarget, LPCTSTR pcszLinkTitle, COLORREF crLink, COLORREF crHover)
{
	CHTMLSectionLink* pLink = new CHTMLSectionLink( pcszLinkTarget, pcszLinkTitle, crLink, crHover );
	m_arrLinks.Add( pLink );
	return pLink;
}

void CHTMLSection::AddNamedSection(LPCTSTR name, const CPoint& point)
{
	StringClass string(name);
	m_mapNames.SetAt(string, point);
}

void CHTMLSection::AddBreakSection(int i)
{
	// Don't allow duplicates...
	const UINT nArrSize = m_arrBreakSections.GetSize();
	if (nArrSize > 0)
	{
		const int nLastSection = m_arrBreakSections[nArrSize - 1];
		if (nLastSection == i)
			return;
	}
	m_arrBreakSections.Add(i);
}


void CHTMLSection::GetDefaultMargins( LPRECT lprect ) const
{
	if( m_pDocument )
	{
		lprect->left = m_pDocument->m_nLeftMargin;
		lprect->top = m_pDocument->m_nTopMargin;
		lprect->right = m_pDocument->m_nRightMargin;
		lprect->bottom= m_pDocument->m_nBottomMargin;
	}
	else
	{
		*lprect = m_pDefaults->m_rcMargins;
	}
}

void CHTMLSection::SetDefaultMargins( LPCRECT lprect )
{
	m_pDefaults->m_rcMargins = *lprect;

	if( m_pDocument )
	{
		//	Too early, need to have set HTML first.
		ASSERT( m_pDocument );

		m_pDocument->m_nLeftMargin = lprect->left;
		m_pDocument->m_nTopMargin = lprect->top;
		m_pDocument->m_nRightMargin = lprect->right;
		m_pDocument->m_nBottomMargin = lprect->bottom;
	}
}

// Returns number of pages
int CHTMLSection::Paginate(const CRect& rcPage)
{
	/*	Strategy:
			Using the page break data, attempt to place
			as much as possible on each page, and determine
			the extrema of the rectangle for each page.
			If an object does fit on the remainder of the page,
			AND it is larger than a page, split it onto the current
			page, otherwise, wait until the next page.
	*/
	//	Empty the pagination data
	m_arrPageRects.RemoveAll();
	const int nPageHeight = rcPage.Height();
	const int nPageBreaks = m_arrBreakSections.GetSize();

	// Handle the simple case where the whole thing fits on the first page
	if (GetHeight() <= nPageHeight)
	{
		m_arrPageRects.Add(rcPage);
	}
	else
	{
		int nBreakIndex;
		UINT nPageTop = 0, nPageBottom = 0;

		for (nBreakIndex = 0; nBreakIndex < nPageBreaks; ++nBreakIndex)
		{
			// See if the next break will fit on this page
			const int nSectionIndex = m_arrBreakSections[nBreakIndex] - 1;
			if (nSectionIndex < 0)
				continue;
			if ((UINT)nSectionIndex >= GetSectionCount())
				break;
			CSectionABC* psect = GetSectionAt(nSectionIndex);
			const UINT nNextY = psect->bottom;
			// See if it will fit. 
			if (nNextY < nPageTop + nPageHeight)
			{
				// It fits... keep going...
				nPageBottom = max(nPageBottom, nNextY);
			}
			else
			{
				// If the page is empty, or the object is taller
				// than the page height, split the object!
				if (nPageBottom == nPageTop || psect->Height() > nPageHeight)
				{
					// maximum page:
					nPageBottom = nPageTop + nPageHeight;
				}
				// Create a new page
				CRect rect(rcPage.left, nPageTop, rcPage.right, nPageBottom);
				m_arrPageRects.Add(rect);
				nPageBottom++;
				nPageTop = nPageBottom;
			}
		}
		// Handle the remainder...
		while (nPageBottom < (UINT)GetHeight())	// bottom of this
		{
			nPageBottom = nPageTop + nPageHeight;
			CRect rect(rcPage.left, nPageTop, rcPage.right, nPageBottom);
			m_arrPageRects.Add(rect);
			nPageBottom++;
			nPageTop = nPageBottom;
		}
	}
	return m_arrPageRects.GetSize();
}

CRect CHTMLSection::GetPageRect(UINT nPage) const
{
	ASSERT(nPage < m_arrPageRects.GetSize());

	return m_arrPageRects[nPage];
}


const StringClass & CHTMLSection::GetTitle() const
{
	static const StringClass g_strNoTitle;

	if( m_pDocument )
		return m_pDocument->m_strTitle;
	return g_strNoTitle;
}


void CHTMLSection::ResetMeasuringKludge()
{
	if( m_pDocument )
		m_pDocument->ResetMeasuringKludge();
}


void CHTMLSection::SetZoomLevel( UINT uZoomLevel )
{
	ResetMeasuringKludge();
	m_uZoomLevel = uZoomLevel;
}