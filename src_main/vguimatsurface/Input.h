//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Methods related to input
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef INPUT_H
#define INPUT_H

//-----------------------------------------------------------------------------
// Initializes the input system
//-----------------------------------------------------------------------------
void InitInput();


//-----------------------------------------------------------------------------
// Hooks input listening up to a window
//-----------------------------------------------------------------------------
void InputAttachToWindow(void *hwnd);
void InputDetachFromWindow(void *hwnd);

// If input isn't hooked, this forwards messages to vgui.
void InputHandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam );


//-----------------------------------------------------------------------------
// Enables/disables input (enabled by default)
//-----------------------------------------------------------------------------
void EnableInput( bool bEnable );


#endif	// INPUT_H