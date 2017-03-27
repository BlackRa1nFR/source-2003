// QHTMMFCView.cpp : implementation of the CQHTMMFCView class
//

#include "stdafx.h"
#include "QHTMMFC.h"

#include "QHTMMFCDoc.h"
#include "QHTMMFCView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCView

IMPLEMENT_DYNCREATE(CQHTMMFCView, CView)

BEGIN_MESSAGE_MAP(CQHTMMFCView, CView)
	//{{AFX_MSG_MAP(CQHTMMFCView)
	ON_WM_SIZE()
	ON_COMMAND(ID_REFRESH, OnFileReload)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_ZOOM_DOWN, OnZoomDown)
	ON_UPDATE_COMMAND_UI(ID_ZOOM_DOWN, OnUpdateZoomDown)
	ON_COMMAND(ID_ZOOM_UP, OnZoomUp)
	ON_UPDATE_COMMAND_UI(ID_ZOOM_UP, OnUpdateZoomUp)
	//}}AFX_MSG_MAP
	ON_NOTIFY( QHTMN_HYPERLINK, 102, OnQHTMHyperlink )
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCView construction/destruction

CQHTMMFCView::CQHTMMFCView()
{
	// TODO: add construction code here

}

CQHTMMFCView::~CQHTMMFCView()
{
}


BOOL CQHTMMFCView::OnEraseBkgnd( CDC * )
{
	return TRUE;
}


BOOL CQHTMMFCView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCView drawing

void CQHTMMFCView::OnDraw(CDC* pDC)
{
	CQHTMMFCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCView printing


void CQHTMMFCView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo )
{
	CView::OnBeginPrinting(pDC, pInfo);

	CString path = GetDocument()->GetPathName();
	if (path.IsEmpty())
	{
		VERIFY(QHTM_PrintSetTextResource( m_printContext, AfxGetResourceHandle(), MAKEINTRESOURCE( ID_DEFAULT_HTML ) ) );
	}
	else
	{
		VERIFY(QHTM_PrintSetTextFile( m_printContext, path));
	}
	// Layout
	CRect rcLayout(0, 0, pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES));
	INT nPages;
	VERIFY(QHTM_PrintLayout( m_printContext, pDC->m_hAttribDC, &rcLayout, &nPages ));
	pInfo->SetMaxPage(nPages);
}


void CQHTMMFCView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CView::OnEndPrinting(pDC, pInfo);
	QHTM_PrintDestroyContext(m_printContext);
}


BOOL CQHTMMFCView::OnPreparePrinting(CPrintInfo* pInfo)
{
	m_printContext = QHTM_PrintCreateContext( QHTM_ZOOM_DEFAULT );
	if (!m_printContext)
	{
		AfxMessageBox( _T("Unable to Print!" ));
		return FALSE;
	}
	// TODO: call DoPreparePrinting to invoke the Print dialog box
	if (!DoPreparePrinting(pInfo))
		return FALSE;
	return CView::OnPreparePrinting(pInfo);
}


void CQHTMMFCView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	// TODO: Add your specialized code here and/or call the base class
	
	// CView::OnPrint(pDC, pInfo);
	// Calculate the area to print based on the page number.
	VERIFY( QHTM_PrintPage( m_printContext, pDC->GetSafeHdc(), pInfo->m_nCurPage - 1, &pInfo->m_rectDraw ) );
}


void CQHTMMFCView::OnUpdate( CView* , LPARAM , CObject* )
{
	// Reload from the file
	OnFileReload();
}


void CQHTMMFCView::OnFileReload() 
{
	CString path = GetDocument()->GetPathName();
	if (path.IsEmpty())
	{
		// Load something from resources.
		m_pQhtmWnd.LoadFromResource( ID_DEFAULT_HTML );
	}
	else
	{
		m_pQhtmWnd.LoadFromFile(path);
	}
}


void CQHTMMFCView::OnSize( UINT nType, int cx, int cy )
{
	CView::OnSize(nType, cx, cy);
	if( GetSafeHwnd() && m_pQhtmWnd.GetSafeHwnd() )
	{
		CRect rect;
		GetClientRect(rect);
		m_pQhtmWnd.SetWindowPos(NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER);
	}
}


BOOL CQHTMMFCView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	
	if (CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext) && 
		m_pQhtmWnd.Create(QHTM_CLASSNAME, _T("<body>&nbsp;</body>"), WS_VISIBLE, rect, this, 102, pContext))
	{
		OnFileReload();
		return TRUE;
	}
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCView diagnostics

#ifdef _DEBUG
void CQHTMMFCView::AssertValid() const
{
	CView::AssertValid();
}

void CQHTMMFCView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CQHTMMFCDoc* CQHTMMFCView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CQHTMMFCDoc)));
	return (CQHTMMFCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCView message handlers

void CQHTMMFCView::OnQHTMHyperlink(NMHDR*nmh, LRESULT*)
{
	LPNMQHTM pnm = reinterpret_cast<LPNMQHTM>( nmh );
	if( pnm->pcszLinkText )
	{
		pnm->resReturnValue = FALSE;
		//
		//	You could implement a scheme using the menu IDs instead of a sybolic name, this would
		//	mean you can add new items to the HTML without altering any code.
		//	I'll keep it simple.
		if( !_tcscmp( pnm->pcszLinkText, _T("open") ) )
		{
			AfxGetMainWnd()->PostMessage( WM_COMMAND, MAKEWPARAM( ID_FILE_OPEN, 0 ), 0 );
		}
		else if( !_tcscmp( pnm->pcszLinkText, _T("about") ) )
		{
			AfxGetMainWnd()->PostMessage( WM_COMMAND, MAKEWPARAM( ID_APP_ABOUT, 0 ), 0  );
		}
		else if( !_tcscmp( pnm->pcszLinkText, _T("print") ) )
		{
			AfxGetMainWnd()->PostMessage( WM_COMMAND, MAKEWPARAM( ID_FILE_PRINT, 0 ), 0  );
		}
		else
		{
			CString str;
			str.Format( _T("This dialog demonstrates how you can control hyperlinking.\n\n")
									_T("Shall I let the QHTM control handle the link to <a href=\"%s\">%s</a>.\n")
									_T("Click <b>Yes</b> to let QHTM resolve the link or click <b>No</b> to do nothing.")
									, pnm->pcszLinkText, pnm->pcszLinkText );
			if( AfxMessageBox( str, MB_YESNOCANCEL ) == IDYES )
			{
				pnm->resReturnValue = TRUE;
			}
		}
	}
}

void CQHTMMFCView::OnZoomDown() 
{
	m_pQhtmWnd.SetZoomLevel( m_pQhtmWnd.GetZoomLevel() - 1 );	
}

void CQHTMMFCView::OnUpdateZoomDown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( m_pQhtmWnd.GetZoomLevel()	!= QHTM_ZOOM_MIN );	
}

void CQHTMMFCView::OnZoomUp() 
{
	m_pQhtmWnd.SetZoomLevel( m_pQhtmWnd.GetZoomLevel() + 1 );		
}

void CQHTMMFCView::OnUpdateZoomUp(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( m_pQhtmWnd.GetZoomLevel()	!= QHTM_ZOOM_MAX );
}
