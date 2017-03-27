#if !defined(AFX_WARNDIALOG_H__BFF9F0FA_8223_4ED8_B4F9_0230512166F6__INCLUDED_)
#define AFX_WARNDIALOG_H__BFF9F0FA_8223_4ED8_B4F9_0230512166F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WarnDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWarnDialog dialog

class CWarnDialog : public CDialog
{
// Construction
public:
	CWarnDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWarnDialog)
	enum { IDD = IDD_WARNDIALOG };
	CString	m_sMinPlayers;
	CString	m_sWarnEmail;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWarnDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWarnDialog)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WARNDIALOG_H__BFF9F0FA_8223_4ED8_B4F9_0230512166F6__INCLUDED_)
