//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OP_GROUPS_H
#define OP_GROUPS_H
#ifdef _WIN32
#pragma once
#endif


#include "resource.h"
#include "ObjectPage.h"


class COP_Groups : public CObjectPage
{
	DECLARE_DYNCREATE(COP_Groups)

// Construction
public:
	COP_Groups();
	~COP_Groups();

	virtual bool SaveData(void);
	virtual void UpdateData(int Mode, PVOID pData);

	void SetMultiEdit(bool b);
	void UpdateGrouplist();

	CMapClass *pUpdateObject;

// Dialog Data
	//{{AFX_DATA(COP_Groups)
	enum { IDD = IDD_OBJPAGE_GROUPS };
	CListBox	m_cGroups;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COP_Groups)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COP_Groups)
	afx_msg void OnEditgroups();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	afx_msg void OnSetFocus(CWnd *pOld);
	
	DECLARE_MESSAGE_MAP()

};


#endif // OP_GROUPS_H
