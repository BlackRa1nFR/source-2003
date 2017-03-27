//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines an autoselection combo box that color codes the text
//			based on whether the current selection represents a single entity,
//			multiple entities, or an unresolved entity targetname.
//
//			The fonts are as follows:
//
//			Single entity		black, normal weight
//			Multiple entities	black, bold
//			Unresolved			red, normal weight
//
// $NoKeywords: $
//=============================================================================

#ifndef TARGETNAMECOMBO_H
#define TARGETNAMECOMBO_H
#ifdef _WIN32
#pragma once
#endif

#include <afxtempl.h>
#include "AutoSelCombo.h"


class CMapEntityList;


class CTargetNameComboBox : public CAutoSelComboBox
{
	typedef CAutoSelComboBox BaseClass;

	public:

		CTargetNameComboBox(void);
		~CTargetNameComboBox(void);

		void SetEntityList(CMapEntityList *pEntityList);
		void SetTargetComboText(const char *szText);

		virtual void OnUpdateText(void);

	protected:

		// Called to update the font and text color as selection changes
		void UpdateForCurSel(int nIndex);

		afx_msg BOOL OnSelChange(void);
		afx_msg BOOL OnCloseUp(void);
		afx_msg BOOL OnDropDown(void);

		void FreeSubLists(void);

		CMapEntityList *m_pEntityList;
		CTypedPtrList<CPtrList, CMapEntityList *> m_SubLists;

		CFont m_NormalFont;						// Normal font used when there is a single match or no match.
		CFont m_BoldFont;						// Bold font used when there are multiple matches.
		bool m_bBold;

		DECLARE_MESSAGE_MAP()
};


#endif // TARGETNAMECOMBO_H
