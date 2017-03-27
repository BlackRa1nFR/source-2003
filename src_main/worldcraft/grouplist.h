//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GROUPLIST_H
#define GROUPLIST_H
#pragma once


#define GROUPLIST_TOGGLESTATE_MSG	"GroupList_ToggleState"


class CGroupList : public CListCtrl
{
// Construction
public:
	CGroupList();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupList)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGroupList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGroupList)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	CPoint m_ptLDown;
	CImageList *m_pDragImageList;
	int m_iLastDropItem;
	int m_iDragItem;
};


#endif // GROUPLIST_H

