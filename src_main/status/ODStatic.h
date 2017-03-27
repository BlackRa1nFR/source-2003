#if !defined ( ODSTATICH )
#define ODSTATICH
#pragma once
/////////////////////////////////////////////////////////
// CODStatic Definition

class CODStatic : public CStatic
{
// Construction
public:
	void SetWindowText( CString& strNewText );
	void SetOffsets(int cx, int cy);
	void SetCentered(BOOL bCenter);
	void SetFontSize(int nSize, int nWeight = FW_NORMAL);
	void SetBkColor(COLORREF clr);
	void SetTextColor(COLORREF clr);
	void SetTransparent(BOOL bTransparent);
	void ForcePaint();
	CODStatic();

// Attributes
public:
	CSize m_szOffsets;
	CString strText;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CODStatic)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CODStatic();
	void SetWindowText(LPCTSTR lpszString );
	// Generated message map functions
protected:
	BOOL m_bCenterText;
	COLORREF m_clrBgnd;
	BOOL m_bTransparent;
	CFont m_hStaticFont;
	COLORREF m_clrText;
	//{{AFX_MSG(CODStatic)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnNcPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif