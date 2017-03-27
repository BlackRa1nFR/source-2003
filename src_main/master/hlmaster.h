// HLMaster.h : main header file for the HLMASTER application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
/////////////////////////////////////////////////////////////////////////////
// CHLMasterApp:
// See HLMaster.cpp for the implementation of this class
//

class CHLMasterApp : public CWinApp
{
public:
	CHLMasterApp();
	~CHLMasterApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHLMasterApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

private:
// Implementation

	//{{AFX_MSG(CHLMasterApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
