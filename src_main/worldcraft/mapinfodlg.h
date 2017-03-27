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

#ifndef MAPINFODLG_H
#define MAPINFODLG_H
#pragma once


class CMapInfoDlg : public CDialog
{
	// Construction
	public:
		CMapInfoDlg(CMapWorld *, CWnd* pParent = NULL);   // standard constructor

		void CountEntity(CMapEntity *pEntity);
		void CountFace(CMapFace *pFace);
		void CountSolid(CMapSolid *pSolid);
		void CountTexture(IEditorTexture *pTex);

	// Dialog Data
		//{{AFX_DATA(CMapInfoDlg)
		enum { IDD = IDD_MAPINFO };
		CStatic	m_Faces;
		CStatic	m_Solids;
		CStatic	m_SolidEntities;
		CStatic	m_PointEntities;
		CStatic	m_TextureMemory;
		CStatic	m_UniqueTextures;
		CListBox m_WadsUsed;
		//}}AFX_DATA

		CMapWorld *pWorld;

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapInfoDlg)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

		UINT m_uSolidCount;
		UINT m_uPointEntityCount;
		UINT m_uSolidEntityCount;
		UINT m_uFaceCount;
		UINT m_uUniqueTextures;
		UINT m_uTextureMemory;

		IEditorTexture *m_pTextures[1024];

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CMapInfoDlg)
		virtual BOOL OnInitDialog();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

#endif // MAPINFODLG_H
