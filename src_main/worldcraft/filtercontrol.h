//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FILTERCONTROL_H
#define FILTERCONTROL_H
#pragma once


#include "resource.h"
#include "GroupList.h"
#include "WorldcraftBar.h"


class CFilterControl : public CWorldcraftBar
{
public:
	CFilterControl() : CWorldcraftBar() { bInitialized = FALSE; }
	BOOL Create(CWnd *pParentWnd);

	void UpdateGroupList();

	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

private:
	//{{AFX_DATA(CFilterControl)
	enum { IDD = IDD_MAPVIEWBAR };
	//}}AFX_DATA

	CGroupList m_cGroupBox;
	CImageList m_cGroupImageList;
	BOOL bInitialized;

protected:
	//{{AFX_MSG(CFilterControl)
	afx_msg void OnEditGroups();
	afx_msg void OnMarkMembers();
	afx_msg void UpdateControl(CCmdUI *);
	afx_msg void UpdateControlGroups(CCmdUI *pCmdUI);
	afx_msg void OnActivate(UINT nState, CWnd*, BOOL);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnWindowPosChanged(WINDOWPOS *pPos);
	afx_msg void OnEndlabeleditGrouplist(NMHDR*, LRESULT*);
	afx_msg void OnBegindragGrouplist(NMHDR*, LRESULT*);
	afx_msg void OnMousemoveGrouplist(NMHDR*, LRESULT*);
	afx_msg void OnEnddragGrouplist(NMHDR*, LRESULT*);
	afx_msg LRESULT OnListToggleState(WPARAM wParam, LPARAM lParam);
	afx_msg void OnShowAllGroups(void);
	//}}AFX_MSG

	CImageList *m_pDragImageList;

	DECLARE_MESSAGE_MAP()
};


#endif // FILTERCONTROL_H
