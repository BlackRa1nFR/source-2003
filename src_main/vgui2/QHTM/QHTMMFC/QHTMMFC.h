// QHTMMFC.h : main header file for the QHTMMFC application
//

#if !defined(AFX_QHTMMFC_H__2398F338_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_)
#define AFX_QHTMMFC_H__2398F338_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCApp:
// See QHTMMFC.cpp for the implementation of this class
//

class CQHTMMFCApp : public CWinApp
{
public:
	CQHTMMFCApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQHTMMFCApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	virtual int DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt);

// Implementation
	//{{AFX_MSG(CQHTMMFCApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QHTMMFC_H__2398F338_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_)
