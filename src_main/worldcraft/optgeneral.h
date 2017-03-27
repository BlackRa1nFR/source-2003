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

#ifndef OPTGENERAL_H
#define OPTGENERAL_H
#pragma once

#include "Resource.h"


class COPTGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(COPTGeneral)

// Construction
public:
	COPTGeneral();
	~COPTGeneral();

// Dialog Data
	//{{AFX_DATA(COPTGeneral)
	enum { IDD = IDD_OPTIONS_MAIN };
	CButton	m_cLoadWinPos;
	CButton	m_cIndependentWin;
	CSpinButtonCtrl	m_UndoSpin;
	int		m_iUndoLevels;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COPTGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COPTGeneral)
	virtual BOOL OnInitDialog(void);
	afx_msg void OnIndependentwindows(void);
	//}}AFX_MSG

	BOOL OnApply();

	DECLARE_MESSAGE_MAP()
};


#endif // OPTGENERAL_H
