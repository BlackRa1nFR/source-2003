//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef OP_MODEL_H
#define OP_MODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "ObjectPage.h"


class GDclass;
class CMapStudioModel;


class COP_Model : public CObjectPage
{
	DECLARE_DYNCREATE(COP_Model)

public:
	COP_Model();
	~COP_Model();

	virtual bool SaveData(void);
	virtual void UpdateData(int Mode, PVOID pData);
	void UpdateForClass(LPCTSTR pszClass);

	GDclass *pObjClass;

	//{{AFX_DATA(COP_Model)
	enum { IDD = IDD_OBJPAGE_MODEL };
	CComboBox m_ComboSequence;
	CSliderCtrl m_ScrollBarFrame;
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COP_Model)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	CMapStudioModel *GetModelHelper(void);
	void UpdateFrameText(float flFrame);

	// Generated message map functions
	//{{AFX_MSG(COP_Model)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangeSequence(void);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // OP_MODEL_H
