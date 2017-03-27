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

#ifndef MATSURFACE_CURSOR_H
#define MATSURFACE_CURSOR_H

#ifdef _WIN32
#pragma once
#endif

#include "vguimatsurface/IMatSystemSurface.h"
#include <vgui/Cursor.h>

//-----------------------------------------------------------------------------
// Initializes cursors
//-----------------------------------------------------------------------------
void InitCursors();


//-----------------------------------------------------------------------------
// Selects a cursor
//-----------------------------------------------------------------------------
void CursorSelect(vgui::HCursor hCursor);


//-----------------------------------------------------------------------------
// Activates the current cursor
//-----------------------------------------------------------------------------
void ActivateCurrentCursor();


//-----------------------------------------------------------------------------
// handles mouse movement
//-----------------------------------------------------------------------------
void CursorSetPos(void *hwnd, int x, int y);
void CursorGetPos(void *hwnd, int &x, int &y);


//-----------------------------------------------------------------------------
// Cursor callbacks
//-----------------------------------------------------------------------------
void CursorSetMouseCallbacks( GetMouseCallback_t getFunc, SetMouseCallback_t setFunc );


//-----------------------------------------------------------------------------
// Purpose: prevents vgui from changing the cursor
//-----------------------------------------------------------------------------
void LockCursor( bool bEnable );


//-----------------------------------------------------------------------------
// Purpose: unlocks the cursor state
//-----------------------------------------------------------------------------
bool IsCursorLocked();


#endif // MATSURFACE_CURSOR_H





