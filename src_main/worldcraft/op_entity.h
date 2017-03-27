//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef OP_ENTITY_H
#define OP_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "AutoSelCombo.h"
#include "ListBoxEx.h"
#include "AngleBox.h"
#include "GameData.h"
#include "KeyValues.h"
#include "MapFace.h"
#include "ObjectPage.h"
#include "ToolPickAngles.h"
#include "ToolPickEntity.h"
#include "ToolPickFace.h"


class CEditGameClass;
class COP_Entity;


//-----------------------------------------------------------------------------
// Purpose: A little glue object that connects the angles picker tool to our dialog.
//-----------------------------------------------------------------------------
class CPickAnglesTarget : public IPickAnglesTarget
{
	public:

		void AttachEntityDlg(COP_Entity *pDlg) { m_pDlg = pDlg; }
		void OnNotifyPickAngles(const Vector &vecPos);
	
	private:

		COP_Entity *m_pDlg;
};


//-----------------------------------------------------------------------------
// Purpose: A little glue object that connects the entity picker tool to our dialog.
//-----------------------------------------------------------------------------
class CPickEntityTarget : public IPickEntityTarget
{
	public:

		void AttachEntityDlg(COP_Entity *pDlg) { m_pDlg = pDlg; }
		void OnNotifyPickEntity(CToolPickEntity *pTool);
	
	private:

		COP_Entity *m_pDlg;
};


//-----------------------------------------------------------------------------
// Purpose: A little glue object that connects the face picker tool to our dialog.
//-----------------------------------------------------------------------------
class CPickFaceTarget : public IPickFaceTarget
{
	public:

		void AttachEntityDlg(COP_Entity *pDlg) { m_pDlg = pDlg; }
		void OnNotifyPickFace(CToolPickFace *pTool);
	
	private:

		COP_Entity *m_pDlg;
};


class COP_Entity : public CObjectPage
{
		DECLARE_DYNCREATE(COP_Entity)

	// Construction
	public:

		COP_Entity();
		~COP_Entity();

		//
		// Interface for property sheet.
		//
		virtual bool SaveData(void);
		virtual void UpdateData(int Mode, PVOID pData);
		virtual void RememberState(void);

		//
		// Interface for custom edit control.
		//
		void SetNextVar(int cmd);

		//{{AFX_DATA(COP_Entity)
		enum { IDD = IDD_OBJPAGE_ENTITYKV };
		CAngleCombo m_AngleEdit;
		CAngleCombo m_SmartAngleEdit;
		CEdit m_cValue;
		CListBox m_VarList;
		CEdit m_cKey;
		CAutoSelComboBox m_cClasses;
		CEdit m_Comments;
		CEdit m_KeyValueHelpText;
		//}}AFX_DATA

		// ClassWizard generate virtual function overrides
		//{{AFX_VIRTUAL(COP_Entity)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	protected:

		void LoadClassList();
		void SetSmartedit(bool bSet);
		void SetupForMode();
		void RemoveBlankKeys();

		void EnableAnglesControl(bool bEnable);

		void CreateSmartControls(GDinputvariable *pVar);
		void DestroySmartControls(void);

		void UpdateClass(const char *pszClass);
		void UpdateKeyValue(const char *szKey, const char *szValue);

		virtual void UpdatePickFaceText(CToolPickFace *pTool);
		void GetFaceIDListsForKey(CMapFaceIDList &FullFaces, CMapFaceIDList &PartialFaces, const char *pszKey);
		void GetFaceListsForKey(CMapFaceList &FullFaces, CMapFaceList &PartialFaces, const char *pszKey);
		void ApplyKeyValueToObject(CEditGameClass *pObject, const char *pszKey, const char *pszValue);

		// Generated message map functions
		//{{AFX_MSG(COP_Entity)
		afx_msg void OnAddkeyvalue();
		afx_msg BOOL OnApply(void);
		afx_msg void OnBrowse(void);
		virtual BOOL OnInitDialog();
		afx_msg void OnSelchangeKeyvalues();
		afx_msg void OnRemovekeyvalue();
		afx_msg void OnSelChangeAngleEdit(void);
		afx_msg void OnChangeAngleedit();
		afx_msg void OnSmartedit();
		afx_msg void OnSelchangeClass();
		afx_msg void OnChangeKeyorValue();
		afx_msg void OnCopy();
		afx_msg void OnPaste();
		afx_msg void OnSetfocusKey();
		afx_msg void OnKillfocusKey();
		afx_msg LRESULT OnChangeAngleBox(WPARAM, LPARAM);
		afx_msg void OnChangeSmartcontrol();
		afx_msg void OnChangeSmartcontrolSel();
		afx_msg void OnPickFaces(void);
		afx_msg void OnPickColor();
		afx_msg void OnMark();
		afx_msg void OnEntityHelp(void);
		afx_msg void OnPickAngles(void);
		afx_msg void OnPickEntity(void);
		//}}AFX_MSG

		void BrowseTextures( const char *szFilter );
		void MergeObjectKeyValues(CEditGameClass *pEdit);
		void MergeKeyValue(char const *pszKey);
		void SetCurKey(LPCTSTR pszKey);
		void GetCurKey(CString& strKey);

		void OnShowPropertySheet(BOOL bShow, UINT nStatus);
		void StopPicking(void);

		DECLARE_MESSAGE_MAP()

	private:

		CString m_szOldKeyName;
		bool m_bWantSmartedit;
		bool m_bEnableControlUpdate;	// Whether to reflect changes to the edit control into other controls.

		CAngleBox m_Angle;
		CAngleBox m_SmartAngle;

		CButton m_cPickColor;
		bool m_bSmartedit;
		int m_nNewKeyCount;
		GDclass *m_pObjClass;
		BYTE m_VarMap[GD_MAX_VARIABLES];
		
		CWnd *m_pSmartControl;			// current smartedit control
		CString m_strLastKey;			// Active key when SaveData was called.

		KeyValues m_kv;					// Our kv storage. Holds merged keyvalues for multiselect.
		KeyValues m_kvAdded;			// Corresponding keys set to value "1" if they were added

		GDIV_TYPE m_eEditType;			// The type of the currently selected key when SmartEdit is enabled.

		bool	 m_bIgnoreKVChange;			// Set to ignore Windows notifications when setting up controls.
		bool	 m_bChangingKeyName;

		bool m_bPicking;					// A picking tool is currently active.
		ToolID_t m_ToolPrePick;				// The tool that was active before we activated the picking tool.

		CPickAnglesTarget m_PickAnglesTarget;
		CPickEntityTarget m_PickEntityTarget;
		CPickFaceTarget m_PickFaceTarget;

	friend class CPickAnglesTarget;
	friend class CPickEntityTarget;
	friend class CPickFaceTarget;
};

#endif // OP_ENTITY_H
