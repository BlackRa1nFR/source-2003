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

#ifndef EDITGROUPS_H
#define EDITGROUPS_H
#pragma once


#include "mapdoc.h"


class CVisGroup;


class CColorBox : public CStatic
{
	public:
		void SetColor(COLORREF, BOOL);
		COLORREF GetColor() { return m_c; }
		
		afx_msg void OnPaint();

	private:
		COLORREF m_c;

	protected:
		DECLARE_MESSAGE_MAP()
};


class CEditGroups : public CDialog
{
	// Construction
	public:
		CEditGroups(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
		//{{AFX_DATA(CEditGroups)
		enum { IDD = IDD_GROUPS };
		CEdit	m_cName;
		CListBox	m_cGroupList;
		//}}AFX_DATA

		CColorBox m_cColorBox;

	// operations:
		CVisGroup * GetSelectedGroup(int iSel = -1);

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CEditGroups)
		public:
		virtual BOOL DestroyWindow();
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CEditGroups)
		afx_msg void OnColor();
		afx_msg void OnChangeName();
		afx_msg void OnNew();
		afx_msg void OnRemove();
		afx_msg void OnSelchangeGrouplist();
		virtual BOOL OnInitDialog();
		afx_msg void OnClose();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

#endif // EDITGROUPS_H
