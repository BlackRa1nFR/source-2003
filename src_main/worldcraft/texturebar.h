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

#ifndef TEXTUREBAR_H
#define TEXTUREBAR_H
#pragma once


#include "TextureBox.h"
#include "IEditorTexture.h"
#include "wndTex.h"
#include "ControlBarIDs.h"
#include "WorldcraftBar.h"


class CTextureBar : public CWorldcraftBar
{
	public:

		CTextureBar() : CWorldcraftBar() {}
		BOOL Create(CWnd *pParentWnd, int IDD = IDD_TEXTUREBAR, int iBarID = IDCB_TEXTUREBAR);

		void NotifyGraphicsChanged(void);
		void SelectTexture(LPCSTR pszTextureName);

	protected:

		void UpdateTexture(void);

		IEditorTexture *m_pCurTex;
		CTextureBox m_TextureList;
		CComboBox m_TextureGroupList;
		wndTex m_TexturePic;

		afx_msg void UpdateControl(CCmdUI *);
		afx_msg void OnBrowse(void);
		afx_msg void OnChangeTextureGroup(void);
		afx_msg void OnReplace(void);
		afx_msg void OnUpdateTexname(void);
		afx_msg void OnWindowPosChanged(WINDOWPOS *pPos);
		virtual afx_msg void OnSelChangeTexture(void);

		DECLARE_MESSAGE_MAP()
};


#endif // TEXTUREBAR_H
