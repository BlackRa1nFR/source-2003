/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
File:	sectionabc.cpp
Owner:	russf@gipsysoft.com
Purpose:	base class for all sections
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "SectionABC.h"
#include "ParentSection.h"
#include "Utils.h"
#include "TipWindow.h"

//REVIEW - russf - bit of a kludge
extern CSectionABC *g_pSectMouseDowned;
extern void CancelMouseDowns();		
extern CSectionABC *g_pSectHighlight;
extern void CancelHighlight();

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

enum { g_knTipWaitTime = 400 };

CTipWindow *CSectionABC::m_pCurrentTip = NULL;
CSectionABC *CSectionABC::m_pTippedWindow = NULL;

static LONG g_lLastMouseMove = 0;

CSectionABC::CSectionABC( CParentSection *psectParent )
	: m_psectParent( psectParent )
	, m_bMouseInSection( false )
	, m_bLeftMouseDown( false )
	,	m_nTipTimerID( knNullTimerId )
	, m_bTransparent( false )
{
	Empty();
	if( m_psectParent )
	{
		m_psectParent->AddSection( this );
	}
}


CSectionABC::~CSectionABC()
{
	if( SectionHasToolTip() )
	{
		KillTip();
		m_pTippedWindow = NULL;
	}

	//
	//	If we are the section that was mousedowned on then we cancel it
	if( g_pSectMouseDowned == this )
		CancelMouseDowns();

	if( g_pSectHighlight == this )
		CancelHighlight();
}


void CSectionABC::OnMouseMove( const CPoint & )
{
	g_lLastMouseMove = GetMessageTime();
}


void CSectionABC::OnDestroy()
{
	if( SectionHasToolTip() )
	{
		KillTip();
	}
}


bool CSectionABC::OnSetMouseCursor( const CPoint &pt )
{
	//
	//	Find a section the point falls in.
	//	If we succeed at that then pass the event onto the section, if
	//		it gets handled in the section then do nothing otherwise we set our own section cursor.
	CSectionABC *pSect = FindSectionFromPoint( pt );
	bool bRetVal = false;
	if( pSect )
	{
		pSect->SetCursor( pt );
		bRetVal = true;
	}
	return bRetVal;
}


void CSectionABC::OnDraw( CDrawContext & /*dc*/ )
{
}


void CSectionABC::OnLayout( const CRect &rc )
{
	CRect::operator = ( rc );
}


CSectionABC * CSectionABC::FindSectionFromPoint(const CPoint & /*pt*/) const
//
//	Given a point return pointer to a section if the pointer is within a section
//	Can return NULL if there is no section at the point.
{
	return NULL;
}


void CSectionABC::SetCursor( const CPoint & )
{
	m_cursor.Set();
}


void CSectionABC::OnMouseLeftDown( const CPoint &pt )
{
	CSectionABC *pSect = FindSectionFromPoint( pt );
	if( pSect )
	{
		pSect->OnMouseLeftDown( pt );
	}
	else
	{
		m_bLeftMouseDown = true;
	}
}

void CSectionABC::OnMouseRightDown( const CPoint &pt )
{
	CSectionABC *pSect = FindSectionFromPoint( pt );
	if( pSect )
	{
		pSect->OnMouseRightDown( pt );
	}
}

void CSectionABC::OnMouseRightUp( const CPoint &pt )
{
	CSectionABC *pSect = FindSectionFromPoint( pt );
	if( pSect )
	{
		pSect->OnMouseRightUp( pt );
	}
}


void CSectionABC::OnMouseLeftUp( const CPoint & )
{
	m_bLeftMouseDown = false;
}
	

void CSectionABC::OnStopMouseCapture()
{
	//
	//	Do nothing
}


void CSectionABC::OnStopMouseDown()
{
	m_bLeftMouseDown = false;
}


void CSectionABC::ForceRedraw()
//
//	Force the section to be re-drawn.
{
	ForceRedraw( *this );
}


void CSectionABC::ForceRedraw( const CRect &rc )
//
//	Force the section to be re-drawn.
{
	if( m_psectParent )
	{
		m_psectParent->ForceRedraw( rc );
	}
}


void CSectionABC::DrawNow()
{
	if( m_psectParent )
	{
		m_psectParent->DrawNow();
	}
}


void CSectionABC::Scroll( int cx, int cy, const CRect &rc )
{
#ifdef _WINDOWS
	CRect rcUpdate;
	(void)::ScrollWindowEx( g_hwnd, cx, cy, &rc, &rc, NULL, &rcUpdate, SW_INVALIDATE );
	(void)InvalidateRect( g_hwnd, rcUpdate, FALSE );
#endif	//	_WINDOWS
}

void CSectionABC::Transparent( bool bTransparent )
{
	m_bTransparent = bTransparent;
}



bool CSectionABC::NotifyParent( int nEvent, const CSectionABC *pChild /*= NULL*/ )
//
//	Send our parent a notify event. If pChild is NULL then this is used as child
{
	if( m_psectParent )
	{
		return m_psectParent->OnNotify( pChild ? pChild : this, nEvent );
	}
	else
	{
		TRACE( _T("CSectionABC::NotifyParent not sent becuase no parent\n") );
	}

	return false;
}

bool CSectionABC::OnNotify( const CSectionABC * pChild, const int nEvent )
{
	if( GetParent() )
	{
		return GetParent()->OnNotify( pChild, nEvent );
	}
	return false;
}

void CSectionABC::OnMouseEnter()
{
	m_bMouseInSection = true;
	RestTipTimer( true );
}


void CSectionABC::OnMouseLeave()
{
	RestTipTimer( false );
	m_bMouseInSection = false;
}


void CSectionABC::KillTip()
{
	if( m_pCurrentTip )
	{
		if( !IsControlPressed() )
		{
			//
			//	We copy into temporary just in case we get a recursive call.
			CTipWindow *pWndTip = m_pCurrentTip;
			m_pCurrentTip = NULL;
			pWndTip -> DestroyWindow();
		}
	}	
}

void	CSectionABC::KillTip( TipKillReason /*nKillReason*/ )
{
	CSectionABC::KillTip();
}

void CSectionABC::RestTipTimer( bool bReStart )
{
	if( m_pTippedWindow )
	{
		KillTip();
	}

	if( m_nTipTimerID != knNullTimerId )
	{
		const int nTipTimerID = m_nTipTimerID;
		m_nTipTimerID = knNullTimerId;

		UnregisterTimerEvent( nTipTimerID );
	}

	if( bReStart && GetTipText().GetLength() )
	{
		m_nTipTimerID = RegisterTimerEvent( this, g_knTipWaitTime );
	}
}

void CSectionABC::OnTimer( int nTimerID )
{
	if( m_nTipTimerID == nTimerID && !IsMouseDown() && IsMouseInSection() && GetMessageTime() - g_lLastMouseMove >= g_knTipWaitTime && !m_pCurrentTip )
	{
		RestTipTimer( false );
		StringClass strTip = GetTipText();
		if( !IsControlPressed() && strTip.GetLength() && GetActiveWindow() )
		{
			CPoint pt;
			GetMousePoint( pt );
			m_pCurrentTip = CreateTipWindow( strTip, pt );
			if( m_pCurrentTip )
			{
				m_pTippedWindow = this;
			}
		}
	}
}

CTipWindow *CSectionABC::CreateTipWindow(  LPCTSTR pcszTip, CPoint &pt  )
{
	return new CTipWindow( pcszTip, pt );
}


StringClass CSectionABC::GetTipText() const
{
	return m_strTipText;
}


void CSectionABC::SetTipText( LPCTSTR pcszTipText )
{
	m_strTipText = pcszTipText;
}

void CSectionABC::SetTipText( const StringClass &strTipText )
{
	m_strTipText = strTipText;
}


void CSectionABC::OnMouseWheel( int nDelta )
{
	//
	//	Default simply calls it's parent
	if( m_psectParent )
	{
		m_psectParent->OnMouseWheel( nDelta );
	}
}


int CSectionABC::RegisterTimerEvent( CSectionABC *pSect, int nInterval )
{
	if( GetParent() )
		return GetParent()->RegisterTimerEvent( pSect, nInterval );
	return 0;
}


void CSectionABC::UnregisterTimerEvent( const int nTimerEventID )
{
	if( GetParent() )
		GetParent()->UnregisterTimerEvent( nTimerEventID );
}

