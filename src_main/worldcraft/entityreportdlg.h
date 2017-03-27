//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ENTITYREPORTDLG_H
#define ENTITYREPORTDLG_H
#ifdef _WIN32
#pragma once
#endif


#include <afxtempl.h>
#include "resource.h"
#include "MapDoc.h"
#include "GameData.h"


class CEntityReportDlg : public CDialog
{
	friend BOOL AddEntityToList(CMapEntity *pEntity, CEntityReportDlg *pDlg);

// Construction
public:
	CEntityReportDlg(CMapDoc *pDoc, CWnd* pParent = NULL);   // standard constructor

	void SaveToIni();

// Dialog Data
	//{{AFX_DATA(CEntityReportDlg)
	enum { IDD = IDD_ENTITYREPORT };
	CButton	m_cExact;
	CComboBox	m_cFilterClass;
	CButton	m_cFilterByClass;
	CListBox	m_cEntities;
	CEdit	m_cFilterValue;
	CEdit	m_cFilterKey;
	CButton	m_cFilterByType;
	CButton	m_cFilterByKeyvalue;
	CButton	m_cFilterByHidden;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntityReportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMapDoc *m_pDoc;
	void UpdateEntityList();

	BOOL m_bFilterByKeyvalue;
	BOOL m_bFilterByClass;
	BOOL m_bFilterByHidden;
	BOOL m_bExact;
	int m_iFilterByType;

	CString m_szFilterKey;
	CString m_szFilterValue;
	CString m_szFilterClass;

	BOOL m_bFilterTextChanged;
	DWORD m_dwFilterTime;

	// Generated message map functions
	//{{AFX_MSG(CEntityReportDlg)
	afx_msg void OnDelete();
	afx_msg void OnFilterbyhidden();
	afx_msg void OnFilterbykeyvalue();
	afx_msg void OnFilterbytype();
	afx_msg void OnChangeFilterkey();
	afx_msg void OnChangeFiltervalue();
	afx_msg void OnGoto();
	afx_msg void OnMark();
	afx_msg void OnProperties();
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeFilterclass();
	afx_msg void OnFilterbyclass();
	afx_msg void OnSelchangeFilterclass();
	afx_msg void OnExactvalue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ENTITYREPORTDLG_H
