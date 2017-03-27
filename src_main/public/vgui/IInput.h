//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_IINPUT_H
#define VGUI_IINPUT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "interface.h"

namespace vgui
{

enum MouseCode;
enum KeyCode;
class Cursor;
typedef unsigned long HCursor;

class IInput : public IBaseInterface
{
public:
	virtual void SetMouseFocus(VPANEL newMouseFocus) = 0;
	virtual void SetMouseCapture(VPANEL panel) = 0;

	// returns the string name of a scan code
	virtual void GetKeyCodeText(KeyCode code, char *buf, int buflen) = 0;

	// focus
	virtual VPANEL GetFocus() = 0;
	virtual VPANEL GetMouseOver() = 0;		// returns the panel the mouse is currently over, ignoring mouse capture

	// mouse state
	virtual void SetCursorPos(int x, int y) = 0;
	virtual void GetCursorPos(int &x, int &y) = 0;
	virtual bool WasMousePressed(MouseCode code) = 0;
	virtual bool WasMouseDoublePressed(MouseCode code) = 0;
	virtual bool IsMouseDown(MouseCode code) = 0;

	// cursor override
	virtual void SetCursorOveride(HCursor cursor) = 0;
	virtual HCursor GetCursorOveride() = 0;

	// key state
	virtual bool WasMouseReleased(MouseCode code) = 0;
	virtual bool WasKeyPressed(KeyCode code) = 0;
	virtual bool IsKeyDown(KeyCode code) = 0;
	virtual bool WasKeyTyped(KeyCode code) = 0;
	virtual bool WasKeyReleased(KeyCode code) = 0;
	
	virtual VPANEL GetAppModalSurface() = 0;
	// set the modal dialog panel.
	// all events will go only to this panel and its children.
	virtual void SetAppModalSurface(VPANEL panel) = 0;
	// release the modal dialog panel
	// do this when your modal dialog finishes.
	virtual void ReleaseAppModalSurface() = 0;

	virtual void GetCursorPosition( int &x, int &y ) = 0;

};

#define VGUI_INPUT_INTERFACE_VERSION "VGUI_Input004"

} // namespace vgui


#endif // VGUI_IINPUT_H
