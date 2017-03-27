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

#ifndef TITLEWND_H
#define TITLEWND_H
#pragma once


class CTitleWnd : public CWnd
{
	public:

		static CTitleWnd *CreateTitleWnd(CWnd *pwndParent, UINT uID);

		void SetTitle(LPCTSTR pszTitle);

	private:

		char m_szTitle[256];
		static CFont m_FontNormal;
		static CFont m_FontActive;

	protected:

		CTitleWnd(void);

		void OnMouseButton(void);

		bool m_bMouseOver;
		bool m_bMenuOpen;

		afx_msg void OnPaint();
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);

		DECLARE_MESSAGE_MAP()
};



#endif // TITLEWND_H
