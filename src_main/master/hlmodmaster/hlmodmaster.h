// HLModMaster.h : main header file for the HLModMaster application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
/////////////////////////////////////////////////////////////////////////////
// CHLModMasterApp:
// See HLModMaster.cpp for the implementation of this class
//

class CHLModMasterApp : public CWinApp
{
public:
	CHLModMasterApp();
	~CHLModMasterApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHLModMasterApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

private:
// Implementation

	//{{AFX_MSG(CHLModMasterApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
