#if !defined(AFX_STATUSWINDOW_H__055C0961_A200_4122_909D_5E42E883ACAF__INCLUDED_)
#define AFX_STATUSWINDOW_H__055C0961_A200_4122_909D_5E42E883ACAF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StatusWindow.h : header file
//

#include "UtlVector.h"

class CStatusDlg;

/////////////////////////////////////////////////////////////////////////////
// CStatusWindow window

class CStatusWindow : public CWnd
{
// Construction
public:
	CStatusWindow( CStatusDlg *pParent );
	
	CStatusWindow::Create( DWORD dwStyle );

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusWindow)
	//}}AFX_VIRTUAL

// Implementation
public:
	void ColorPrintf( COLORREF rgb, char const *fmt, ... );
	void Printf( char const *fmt, ... );
	virtual ~CStatusWindow();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStatusWindow)
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void		RemoveOldStrings( void );

	class CData
	{
	public:
		CData();
		~CData();

		void SetText( COLORREF col, char const *text );
		char const *GetText( void ) const;
		COLORREF GetColor( void ) const;
		float		GetStartTime( void ) const;

	private:
		enum
		{
			MAX_TEXT = 128,
		};

		COLORREF	clr;
		char		linetext[ MAX_TEXT ];
		float		starttime;
	};


	CUtlVector< CData >	m_Text;

	int			m_nFontHeight;
	CFont		m_hSmallFont;

	CStatusDlg	*m_pStatus;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUSWINDOW_H__055C0961_A200_4122_909D_5E42E883ACAF__INCLUDED_)
