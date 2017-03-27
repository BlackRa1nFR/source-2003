#if !defined(AFX_EMAILDIALOG_H__6B8FE09E_AA54_4AE0_86C1_8F08486C344D__INCLUDED_)
#define AFX_EMAILDIALOG_H__6B8FE09E_AA54_4AE0_86C1_8F08486C344D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EmailDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEmailDialog dialog

class CEmailDialog : public CDialog
{
// Construction
public:
	CEmailDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEmailDialog)
	enum { IDD = IDD_EMAIL };
	CListBox	m_emailList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmailDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEmailDialog)
	afx_msg void OnAddemail();
	afx_msg void OnRemoveemail();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	void RepopulateList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMAILDIALOG_H__6B8FE09E_AA54_4AE0_86C1_8F08486C344D__INCLUDED_)
