#if !defined(AFX_JOBWATCHDLG_H__761BDEEF_D549_4F10_817C_1C1FAF9FCA47__INCLUDED_)
#define AFX_JOBWATCHDLG_H__761BDEEF_D549_4F10_817C_1C1FAF9FCA47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// JobWatchDlg.h : header file
//


#include "idle_dialog.h"
#include "resource.h"
#include "utlvector.h"
#include "mysql_wrapper.h"
#include "GraphControl.h"
#include "window_anchor_mgr.h"


/////////////////////////////////////////////////////////////////////////////
// CJobWatchDlg dialog

class CJobWatchDlg : public CIdleDialog
{
// Construction
public:
	CJobWatchDlg( CWnd* pParent = NULL);   // standard constructor
	virtual ~CJobWatchDlg();

// Dialog Data
	//{{AFX_DATA(CJobWatchDlg)
	enum { IDD = IDD_JOB_WATCH };
	CListBox	m_Workers;
	CEdit	m_TextOutput;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJobWatchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	virtual void OnIdle();
	

	// This is our connection to the mysql database.
	IMySQL*	GetMySQL()	{ return m_pSQL; }
	IMySQL	*m_pSQL;

	CWindowAnchorMgr	m_AnchorMgr;


	bool GetCurJobWorkerID( unsigned long &id );
	void FillGraph();
	bool GetCurrentWorkerText( CUtlVector<char> &text, unsigned long &curMessageIndex );

	CGraphControl	m_GraphControl;
	unsigned long	m_JobID;
	unsigned long	m_CurMessageIndex;
	int				m_CurGraphTime;

	// Generated message map functions
	//{{AFX_MSG(CJobWatchDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangeWorkers();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JOBWATCHDLG_H__761BDEEF_D549_4F10_817C_1C1FAF9FCA47__INCLUDED_)
