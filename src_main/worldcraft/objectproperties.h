//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OBJECTPROPERTIES_H
#define OBJECTPROPERTIES_H
#pragma once

#include "MapClass.h"
#include "EditGameClass.h"
#include "ObjectPage.h"
#include "OP_Groups.h"


class COP_Entity;
class COP_Flags;
class COP_Output;
class COP_Input;
class COP_Model;

enum LayoutType_t;


//
// Wrap map classes in CObject so we can use runtime identification
// to pass edit data to the object pages.
//
class editCMapClass : public CObject, public CMapClass
{
	DECLARE_DYNAMIC(editCMapClass)

public:
	// kludge:
	MAPCLASSTYPE GetType() { return NULL; }
	BOOL IsMapClass(MAPCLASSTYPE Type) { return FALSE; }
};


class editCEditGameClass : public CObject, public CEditGameClass
{
	DECLARE_DYNAMIC(editCEditGameClass)
};


class CObjectProperties : public CPropertySheet
{
	DECLARE_DYNAMIC(CObjectProperties)

public:

	CObjectProperties(void);
	virtual ~CObjectProperties(void);

	CObjectProperties(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CObjectProperties(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

	inline void UpdateGrouplist(void);

	PVOID GetEditObject(CRuntimeClass *pType);
	PVOID GetEditObjectFromMapObject(CMapClass *pobj, CRuntimeClass *pType);
	void CopyDataToEditObjects(CMapClass *pobj);
	void DontEdit(CMapClass *pObject);

	void SetPageToOutput(CEntityConnection *pConnection);

	BOOL SetupPages(void);
	void CreatePages(void);
	void SaveData(void);
	void AbortData(void);
	void RefreshData(void);

	void LoadDataForPages(int iPage = -1);
	void LoadData(CMapObjectList *pObjects, bool bSave);

	void SetOutputButtonState(int nState);
	void SetInputButtonState(int nState);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectProperties)
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

protected:

	LayoutType_t GetLayout(bool &bEntity, bool &bGroups, bool &bFlags, bool &bModel);

	void CreateButtons(void);

	void UpdateOutputButton(void);
	void UpdateInputButton(void);

	void SaveActivePage(void);
	void RestoreActivePage(void);

	//
	// Pages.
	//
	COP_Entity *m_pEntity;
	COP_Groups *m_pGroups;
	COP_Flags *m_pFlags;
	COP_Output *m_pOutput;
	COP_Input *m_pInput;
	COP_Model *m_pModel;

	//
	// Input/output connection buttons
	//
	CButton *m_pApplyButton;
	CButton *m_pCancelButton;
	CButton *m_pInputButton;
	CButton *m_pOutputButton;

	HICON m_hIconOutputGood;
	HICON m_hIconOutputBad;
	HICON m_hIconInputGood;
	HICON m_hIconInputBad;

	CPropertyPage *m_pDummy;		// 
	bool m_bDummy;					// 
	CMapObjectList m_DstObjects;	// List of objects to save data to.
	CObjectPage **m_ppPages;		// Pointers to the pages that are currently in use
	int m_nPages;					// Number of pages currently in use
	CObjectPage *m_pLastActivePage;	// Used for restoring the last active page when changing selection.


	//{{AFX_MSG(CObjectProperties)
	afx_msg void OnApply(void);
	afx_msg void OnCancel(void);
	afx_msg void OnInputs(void);
	afx_msg void OnOutputs(void);
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

friend CObjectPage;

};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CObjectProperties::UpdateGrouplist(void)
{
	if (m_pGroups != NULL)
	{
		m_pGroups->UpdateGrouplist();
	}
}


#endif // OBJECTPROPERTIES_H
