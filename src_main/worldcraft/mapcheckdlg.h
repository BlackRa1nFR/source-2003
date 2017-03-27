// MapCheckDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapCheckDlg dialog

#include "mapdoc.h"

class CMapCheckDlg : public CDialog
{
// Construction
public:
	CMapCheckDlg(CWnd* pParent = NULL);   // standard constructor

// operations
	void DoCheck();
	
// Dialog Data
	//{{AFX_DATA(CMapCheckDlg)
	enum { IDD = IDD_MAPCHECK };
	CButton	m_cFixAll;
	CButton	m_Fix;
	CButton	m_Go;
	CEdit	m_Description;
	CListBox	m_Errors;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapCheckDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL bChecked;
	void KillErrorList();

	// Generated message map functions
	//{{AFX_MSG(CMapCheckDlg)
	afx_msg void OnGo();
	afx_msg void OnSelchangeErrors();
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnFix();
	afx_msg void OnFixall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void Fix(int iSel, UpdateBox &ub);
};
