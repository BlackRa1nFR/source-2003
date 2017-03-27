//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OBJECTBAR_H
#define OBJECTBAR_H
#ifdef _WIN32
#pragma once
#endif

#include "AutoSelCombo.h"
#include "WorldcraftBar.h"


class Axes2;
class BoundBox;
class CMapClass;
class Vector;

#define MAX_PREV_SEL 12


class CObjectBar : public CWorldcraftBar
{
public:
	
	CObjectBar();
	BOOL Create(CWnd *pParentWnd);
	
	static LPCTSTR GetDefaultEntityClass(void);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	void UpdateListForTool(int iTool);
	void SetupForBlockTool();
	void DoHideControls();
	CMapClass *CreateInBox(BoundBox *pBox, Axes2 *pAxes = NULL);
	BOOL GetPrefabBounds(BoundBox *pBox);

	void DoDataExchange(CDataExchange *pDX);
	
	enum 
	{
		listPrimitives,
		listPrefabs,
		listMarkers
	} ListType;

	bool IsEntityToolCreatingPrefab( void );
	bool IsEntityToolCreatingEntity( void );
	CMapClass *BuildPrefabObjectAtPoint( Vector const &HitPos );

private:
	
	//{{AFX_DATA(CMapViewBar)
	enum { IDD = IDD_OBJECTBAR };
	//}}AFX_DATA
	
	CAutoSelComboBox	m_CreateList;				// this should really be m_ItemList
	CComboBox			m_CategoryList;
	CEdit				m_Faces;
	CSpinButtonCtrl		m_FacesSpin; 

	void LoadBlockCategories( void );
	void LoadEntityCategories( void );
	void LoadPrefabCategories( void );	

	void LoadBlockItems( void );
	void LoadEntityItems( void );
	void LoadPrefabItems( void );

	int UpdatePreviousSelection( int iTool );

	int GetPrevSelIndex(DWORD dwGameID, int *piNewIndex = NULL);
	BOOL EnableFaceControl(CWnd *pWnd, BOOL bModifyWnd);
	
	int iMarkerSel;
	int iBlockSel;
	
	// previous selections:
	DWORD m_dwPrevGameID;
	struct tagprevsel
	{
		DWORD dwGameID;
		struct tagblock
		{
			CString strItem;
			CString strCategory;
		} block;
		struct tagentity
		{
			CString strItem;
			CString strCategory;
		} entity;
	} m_PrevSel[MAX_PREV_SEL];
	int m_iLastTool;
	
protected:
	
	afx_msg void UpdateControl(CCmdUI*);
	afx_msg void UpdateFaceControl(CCmdUI*);
	afx_msg void OnCreatelistSelchange();
	afx_msg void OnCategorylistSelchange();
	afx_msg void OnChangeCategory();
	
	DECLARE_MESSAGE_MAP()
};

#endif // OBJECTBAR_H
