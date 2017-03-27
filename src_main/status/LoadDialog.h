#if !defined(AFX_LOADDIALOG_H__88EB9B97_98D8_4E56_AD7F_2A5287F815EA__INCLUDED_)
#define AFX_LOADDIALOG_H__88EB9B97_98D8_4E56_AD7F_2A5287F815EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoadDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoadDialog dialog

#include "UtlVector.h"

class CStatusDlg;
class CLoadDialog : public CDialog
{
// Construction
public:
	CLoadDialog(CStatusDlg* pParent = NULL);   // standard constructor

	int				GetNumFiles( void );
	char const		*GetFile( int index );

	int				GetFlags();

// Dialog Data
	//{{AFX_DATA(CLoadDialog)
	enum { IDD = IDD_LOADFILE };
	CStatic	m_staticNumFiles;
	CString	m_strEndDate;
	CString	m_strStartDate;
	CString	m_strThreshold;
	CString	m_strModName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoadDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoadDialog)
	afx_msg void OnChoosefiles();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int				DetermineFlags( void );

	struct FilenameList
	{
		char	fn[ 128 ];
	};

	CUtlVector< FilenameList > m_FileNames;

	CStatusDlg		*m_pStatus;
	int				m_nFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOADDIALOG_H__88EB9B97_98D8_4E56_AD7F_2A5287F815EA__INCLUDED_)
