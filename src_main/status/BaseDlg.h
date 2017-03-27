#if !defined(AFX_BASEDLG_H__7CE0A980_9540_11D3_A683_0050041C0AC1__INCLUDED_)
#define AFX_BASEDLG_H__7CE0A980_9540_11D3_A683_0050041C0AC1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BaseDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBaseDlg dialog

class CBaseDlg : public CDialog
{
// Construction
public:
	CBaseDlg(UINT id, CWnd* pParent = NULL);   // standard constructor

	virtual void RMLSetup(void);
	virtual int RMLPreIdle(void);
	virtual void RMLIdle(void);
	virtual void RMLPrePump(void);
	virtual void RMLPump(void);
	virtual void RMLPostPump(void);
	
	int RunModalLoop(DWORD dwFlags = 0);
	virtual int DoModal();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBaseDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BASEDLG_H__7CE0A980_9540_11D3_A683_0050041C0AC1__INCLUDED_)
