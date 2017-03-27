//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef OP_OUTPUT_H
#define OP_OUTPUT_H
#pragma once

#include "ObjectPage.h"
#include "Resource.h"
#include "TargetNameCombo.h"
#include "MapEntity.h"
#include "ToolPickEntity.h"

#define OUTPUT_LIST_NUM_COLUMNS		6


class COP_Output;


enum SortDirection_t;


//-----------------------------------------------------------------------------
// Purpose: A little glue object that connects the entity picker tool to our dialog.
//-----------------------------------------------------------------------------
class COP_OutputPickEntityTarget : public IPickEntityTarget
{
	public:

		void AttachEntityDlg(COP_Output *pDlg) { m_pDlg = pDlg; }
		void OnNotifyPickEntity(CToolPickEntity *pTool);
	
	private:

		COP_Output *m_pDlg;
};


//-----------------------------------------------------------------------------
// Purpose: A list of connections and entites that send them.
//-----------------------------------------------------------------------------
class COutputConnection
{
public:
	CEntityConnectionList*	m_pConnList;
	CMapEntityList*			m_pEntityList;
	bool					m_bIsValid;
	bool					m_bOwnedByAll;	// Connection used by all selected entities
};


class COP_Output : public CObjectPage
{
	public:
		static CImageList *m_pImageList;
		static CEntityConnectionList*	m_pConnectionBuffer;	// Stores connections for copy/pasting
		static void EmptyCopyBuffer(void);

	public:

		DECLARE_DYNCREATE(COP_Output)

		// Construction
		COP_Output(void);
		~COP_Output(void);

		virtual void UpdateData(int Mode, PVOID pData);
		void SetSelectedItem(int nItem);
		void SetSelectedConnection(CEntityConnection *pConnection);

	protected:

		void AddEntityConnections(CMapEntity *pEntity, bool bFirst);
		void AddConnection(CEntityConnection *pConnection);
		void RemoveAllEntityConnections(void);
		void UpdateConnectionList(void);
		void ResizeColumns(void);

		// Validation functions
		bool ValidateConnections(COutputConnection *pOutputConn);
		void UpdateValidityButton(void);

		// Edit controls
		void UpdateEditControls(void);
		void EnableEditControls(bool bValue = true);
		void UpdateItemValidity(int nItem);
		void EnableTarget(bool bEnable);
		bool bSkipEditControlRefresh;

		// Functions for updating edited connections
		void UpdateEditedInputs(void);
		void UpdateEditedOutputs(void);
		void UpdateEditedTargets(void);
		void UpdateEditedDelays(void);
		void UpdateEditedFireOnce(void);
		void UpdateEditedParams(void);

		// Fuctions for reacting to combo box changes
		void OutputChanged(void);
		void InputChanged(void);
		void TargetChanged(void);

		void SortListByColumn(int nColumn, SortDirection_t eDirection);
		void SetSortColumn(int nColumn, SortDirection_t eDirection);
		void UpdateColumnHeaderText(int nColumn, bool bIsSortColumn, SortDirection_t eDirection);

		CMapEntityList*			m_pEntityList;			// Filtered m_pObject list than only includes map entities
		CMapEntityList*			m_pMapEntityList;		// List of all entities in the map.
		CEntityConnectionList*	m_pEditList;			// List of selected connections being edited 

		void UpdateEntityList();						// Generates m_pEntityList fomr m_pObjectList
		
		CEditGameClass	*m_pEditGameClass;
		CMapEntity		*m_pEntity;
		bool			m_bNoParamEdit;

		//
		// Cached data for sorting the list view.
		//
		int m_nSortColumn;												// Current column used for sorting.
		SortDirection_t m_eSortDirection[OUTPUT_LIST_NUM_COLUMNS];		// Last sort direction per column.

		bool m_bPickingEntities;

		// ########################################
		//  Message editing
		// ########################################
		void SetConnection(CEntityConnectionList *pConnectionList);

		inline void SetMapEntityList(CMapEntityList *pMapEntityList) { m_pMapEntityList = pMapEntityList; }

		void AddEntityOutputs(CMapEntity *pEntity);

		void FillInputList(void);
		void FillOutputList(void);
		void FillTargetList(void);

		void FilterInputList(void);
		void FilterOutputList(void);
		void FilterTargetList(void);
		void FilterEntityOutputs(CMapEntity *pEntity);

		void StopPicking(void);

		CClassInput*	GetInput(char *szInput, int nSize);
		CClassOutput*	GetOutput(char *szOutput, int nSize);
		CMapEntityList*	GetTarget(char *szTarget, int nSize);

		void UpdateCombosForSelectedInput(CClassInput *pInput);
		void UpdateCombosForSelectedOutput(CClassOutput *pOutput);
		void UpdateCombosForSelectedTarget(CMapEntityList *pTarget);

		CFont m_NormalFont;						// Normal font for targets combo. Used when there is a single match or no match.
		CFont m_BoldFont;						// Bold font for targets combo. Used when there are multiple matches.

		// #########################################

		//{{AFX_DATA(COP_Output)
		enum { IDD = IDD_OBJPAGE_OUTPUT };
		CListCtrl m_ListCtrl;
		CAutoSelComboBox	m_ComboOutput;
		CTargetNameComboBox m_ComboTarget;
		CAutoSelComboBox	m_ComboInput;

		CString	m_strOutput;
		CString	m_strTarget;
		CString	m_strInput;
		CString	m_strParam;
		float m_fDelay;
		BOOL m_bFireOnce;
		//}}AFX_DATA

		// ClassWizard generate virtual function overrides
		//{{AFX_VIRTUAL(COP_Output)
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);
		virtual BOOL OnInitDialog(void);
		virtual void OnDestroy(void);
		//}}AFX_VIRTUAL

		// Generated message map functions
		//{{AFX_MSG(COP_Output)
		afx_msg void OnAdd(void);
		afx_msg void OnCopy(void);
		afx_msg void OnPaste(void);
		afx_msg void OnDelete(void);
		afx_msg void OnMark(void);
		afx_msg void OnPickEntity(void);
		afx_msg void OnInputSelChange(void);
		afx_msg void OnInputEditUpdate(void);
		afx_msg void OnOutputSelChange(void);
		afx_msg void OnOutputEditUpdate(void);
		afx_msg void OnTargetSelChange(void);
		afx_msg void OnTargetEditUpdate(void);
		afx_msg void OnParamSelChange(void);
		afx_msg void OnParamEditUpdate(void);
		afx_msg void OnDelayChange(void);
		afx_msg void OnFireOnceChange(void);
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()

	private:

		COP_OutputPickEntityTarget m_PickEntityTarget;

	friend class COP_OutputPickEntityTarget;
};

#endif // OP_OUTPUT_H
