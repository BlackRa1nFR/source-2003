#if !defined(AFX_PROJECTIONWND_H__EC5D015A_C288_4EC3_9D3D_130E409702ED__INCLUDED_)
#define AFX_PROJECTIONWND_H__EC5D015A_C288_4EC3_9D3D_130E409702ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProjectionWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProjectionWnd window

class CProjectionWnd : public CWnd
{
// Construction
public:
	CProjectionWnd();
	BOOL Create( DWORD dwStyle, CWnd *pParent, UINT id );

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProjectionWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CProjectionWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CProjectionWnd)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	COLORREF	m_clrBgnd;
	COLORREF	m_clrText;
	CFont		m_hStaticFont;
	CSize		m_szOffsets;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROJECTIONWND_H__EC5D015A_C288_4EC3_9D3D_130E409702ED__INCLUDED_)
