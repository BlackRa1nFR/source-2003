//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines a window for displaying a single texture within.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef WNDTEX_H
#define WNDTEX_H
#pragma once


class wndTex : public CStatic
{
	public:

		wndTex() : m_pTexture(NULL)
		{
		}

		void SetTexture(IEditorTexture *pTex);

		inline IEditorTexture *GetTexture(void)
		{
			return(m_pTexture);
		}

	protected:

		IEditorTexture *m_pTexture;

		afx_msg void OnPaint();
		
		DECLARE_MESSAGE_MAP();
};


#endif // WNDTEX_H
