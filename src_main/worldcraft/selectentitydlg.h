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


#include "MapClass.h"	// dvs: For CMapObjectList


class CSelectEntityDlg : public CDialog
{
	// Construction
	public:
		CSelectEntityDlg(CMapObjectList *pList,
			CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
		//{{AFX_DATA(CSelectEntityDlg)
		enum { IDD = IDD_SELECTENTITY };
		CListBox	m_cEntities;
		//}}AFX_DATA

		CMapObjectList *m_pEntityList;
		CMapEntity *m_pFinalEntity;

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CSelectEntityDlg)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CSelectEntityDlg)
		afx_msg void OnSelchangeEntities();
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

