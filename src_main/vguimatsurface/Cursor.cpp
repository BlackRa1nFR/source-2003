//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Methods associated with the cursor
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#define OEMRESOURCE //for OCR_* cursor junk
#include <windows.h>
#include "tier0/dbg.h"
#include "tier0/vcrmode.h"
#include "cursor.h"

using namespace vgui;
static HICON s_pDefaultCursor[20];
static HICON s_hCurrentCursor = NULL;
static GetMouseCallback_t s_GetMouseFunc = NULL;
static SetMouseCallback_t s_SetMouseFunc = NULL;
static bool s_bCursorLocked = false; 
static bool s_bCursorVisible = true;


//-----------------------------------------------------------------------------
// Initializes cursors
//-----------------------------------------------------------------------------
void InitCursors()
{
	// load up all default cursors
	s_pDefaultCursor[dc_none]     = NULL;
	s_pDefaultCursor[dc_arrow]    =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_NORMAL);
	s_pDefaultCursor[dc_ibeam]    =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_IBEAM);
	s_pDefaultCursor[dc_hourglass]=(HICON)LoadCursor(NULL, (LPCTSTR)OCR_WAIT);
	s_pDefaultCursor[dc_crosshair]=(HICON)LoadCursor(NULL, (LPCTSTR)OCR_CROSS);
	s_pDefaultCursor[dc_up]       =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_UP);
	s_pDefaultCursor[dc_sizenwse] =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_SIZENWSE);
	s_pDefaultCursor[dc_sizenesw] =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_SIZENESW);
	s_pDefaultCursor[dc_sizewe]   =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_SIZEWE);
	s_pDefaultCursor[dc_sizens]   =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_SIZENS);
	s_pDefaultCursor[dc_sizeall]  =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_SIZEALL);
	s_pDefaultCursor[dc_no]       =(HICON)LoadCursor(NULL, (LPCTSTR)OCR_NO);
	s_pDefaultCursor[dc_hand]     =(HICON)LoadCursor(NULL, (LPCTSTR)32649);

	s_bCursorLocked = false;
	s_bCursorVisible = true;
	s_hCurrentCursor = s_pDefaultCursor[dc_arrow];
	s_GetMouseFunc = NULL;
	s_SetMouseFunc = NULL;
}


//-----------------------------------------------------------------------------
// Cursor callbacks
//-----------------------------------------------------------------------------
void CursorSetMouseCallbacks( GetMouseCallback_t getFunc, SetMouseCallback_t setFunc )
{
	s_GetMouseFunc = getFunc;
	s_SetMouseFunc = setFunc;
}


//-----------------------------------------------------------------------------
// Selects a cursor
//-----------------------------------------------------------------------------
void CursorSelect(HCursor hCursor)
{
	if (s_bCursorLocked)
		return;

	s_bCursorVisible = true;
	switch (hCursor)
	{
	case dc_user:
	case dc_none:
		s_bCursorVisible = false;
		break;

	case dc_arrow:
	case dc_ibeam:
	case dc_hourglass:
	case dc_crosshair:
	case dc_up:
	case dc_sizenwse:
	case dc_sizenesw:
	case dc_sizewe:
	case dc_sizens:
	case dc_sizeall:
	case dc_no:
	case dc_hand:
		s_hCurrentCursor = s_pDefaultCursor[hCursor];
		break;

	default:
		s_bCursorVisible = false;
		Assert(0);
		break;
	}

	ActivateCurrentCursor();
}


//-----------------------------------------------------------------------------
// Activates the current cursor
//-----------------------------------------------------------------------------
void ActivateCurrentCursor()
{
	if (s_bCursorVisible)
	{
		::SetCursor(s_hCurrentCursor);
	}
	else
	{
		::SetCursor(NULL);
	}
}


//-----------------------------------------------------------------------------
// Purpose: prevents vgui from changing the cursor
//-----------------------------------------------------------------------------
void LockCursor( bool bEnable )
{
	s_bCursorLocked = bEnable;
	ActivateCurrentCursor();
}


//-----------------------------------------------------------------------------
// Purpose: unlocks the cursor state
//-----------------------------------------------------------------------------
bool IsCursorLocked()
{
	return s_bCursorLocked;
}


//-----------------------------------------------------------------------------
// handles mouse movement
//-----------------------------------------------------------------------------
void CursorSetPos(void *hwnd, int x, int y)
{
	POINT pt;
	pt.x = x; pt.y = y;
	ClientToScreen((HWND)hwnd, &pt);

	if (s_SetMouseFunc)
	{
		s_SetMouseFunc( pt.x, pt.y );
	}
	else
	{
		SetCursorPos( pt.x, pt.y );
	}
}

void CursorGetPos(void *hwnd, int &x, int &y)
{
	POINT pt;
	if (s_GetMouseFunc)
	{	
		s_GetMouseFunc(x, y);
		pt.x = x;
		pt.y = y;
	}
	else
	{
		// Default implementation
		g_pVCR->Hook_GetCursorPos( &pt );
	}
	g_pVCR->Hook_ScreenToClient((HWND)hwnd, &pt);
	x = pt.x; y = pt.y;
}






