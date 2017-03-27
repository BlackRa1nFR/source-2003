// QHTMMFCView.h : interface of the CQHTMMFCView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_QHTMMFCVIEW_H__2398F340_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_)
#define AFX_QHTMMFCVIEW_H__2398F340_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef QHTM_H
#include <QHTM/QHTM.h>
#endif	//	QHTM_H

class CQHTMMFCView : public CView
{
protected: // create from serialization only
	CQHTMMFCView();
	DECLARE_DYNCREATE(CQHTMMFCView)

// Attributes
public:
	CQHTMMFCDoc* GetDocument();

// Operations
public:
	CQhtmWnd	m_pQhtmWnd;
	QHTMCONTEXT	m_printContext;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQHTMMFCView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual void OnUpdate( CView* , LPARAM , CObject* );
	protected:
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CQHTMMFCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CQHTMMFCView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFileReload();
	afx_msg BOOL OnEraseBkgnd( CDC * );
	afx_msg void OnZoomDown();
	afx_msg void OnUpdateZoomDown(CCmdUI* pCmdUI);
	afx_msg void OnZoomUp();
	afx_msg void OnUpdateZoomUp(CCmdUI* pCmdUI);
	//}}AFX_MSG
	void OnQHTMHyperlink(NMHDR*nmh, LRESULT*);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in QHTMMFCView.cpp
inline CQHTMMFCDoc* CQHTMMFCView::GetDocument()
   { return (CQHTMMFCDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QHTMMFCVIEW_H__2398F340_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_)
