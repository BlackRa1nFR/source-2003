#if !defined(AFX_ADDEMAILDIALOG_H__92FF26E2_7E4E_4A8A_A4E9_31586BD0D7AB__INCLUDED_)
#define AFX_ADDEMAILDIALOG_H__92FF26E2_7E4E_4A8A_A4E9_31586BD0D7AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddEmailDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddEmailDialog dialog

class CAddEmailDialog : public CDialog
{
// Construction
public:
	CAddEmailDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddEmailDialog)
	enum { IDD = IDD_ADDEMAIL };
	CString	m_strAddress;
	CString	m_strMagnitude;
	CString	m_strUnits;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddEmailDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddEmailDialog)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDEMAILDIALOG_H__92FF26E2_7E4E_4A8A_A4E9_31586BD0D7AB__INCLUDED_)
