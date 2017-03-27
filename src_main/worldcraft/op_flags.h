//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#pragma once


#include "ObjectPage.h"


class GDclass;


class COP_Flags : public CObjectPage
{
	DECLARE_DYNCREATE(COP_Flags)

// Construction
public:
	COP_Flags();
	~COP_Flags();

	virtual bool SaveData(void);
	virtual void UpdateData(int Mode, PVOID pData);
	void UpdateForClass(LPCTSTR pszClass);

	GDclass *pObjClass;

// Dialog Data
	//{{AFX_DATA(COP_Flags)
	enum { IDD = IDD_OBJPAGE_FLAGS };
	CCheckListBox m_CheckList;
	int m_nItemBitNum[24];

		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COP_Flags)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COP_Flags)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
