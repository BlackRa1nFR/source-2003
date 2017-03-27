#if !defined(AFX_SCHEDULERDIALOG_H__48C2293C_E251_49EA_B294_1B2F78CB9F6B__INCLUDED_)
#define AFX_SCHEDULERDIALOG_H__48C2293C_E251_49EA_B294_1B2F78CB9F6B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SchedulerDialog.h : header file
//
#include "UtlVector.h"

/////////////////////////////////////////////////////////////////////////////
// CSchedulerDialog dialog

class CSchedulerDialog : public CDialog
{
// Construction
public:
	CSchedulerDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSchedulerDialog)
	enum { IDD = IDD_SCHEDULERDIALOG };
	CDateTimeCtrl	m_TimeControl;
	CListBox	m_TimeList;
	CTime	m_Time;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSchedulerDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSchedulerDialog)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnAddtime();
	afx_msg void OnDeltime();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHEDULERDIALOG_H__48C2293C_E251_49EA_B294_1B2F78CB9F6B__INCLUDED_)
