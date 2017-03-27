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

#ifndef MESSAGEWND_H
#define MESSAGEWND_H
#pragma once

#include "GlobalFunctions.h"


const int MAX_MESSAGE_WND_LINES = 5000;

enum
{
	MESSAGE_WND_MESSAGE_LENGTH = 150
};

class CMessageWnd : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMessageWnd)
protected:
	CMessageWnd();           // protected constructor used by dynamic creation

	struct MWMSGSTRUCT
	{
		MWMSGTYPE type;
		TCHAR szMsg[MESSAGE_WND_MESSAGE_LENGTH];
		int MsgLen;	// length of message w/o 0x0
	} ;

// Attributes
public:
	void AddMsg(MWMSGTYPE type, TCHAR* msg);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessageWnd)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMessageWnd();

	void CalculateScrollSize();
	CArray<MWMSGSTRUCT, MWMSGSTRUCT&> MsgArray;

	CFont Font;
	int iCharWidth;	// calculated in first paint
	int iNumMsgs;

	// Generated message map functions
	//{{AFX_MSG(CMessageWnd)
	afx_msg void OnPaint();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // MESSAGEWND_H
