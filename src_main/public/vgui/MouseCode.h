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

#ifndef MOUSECODE_H
#define MOUSECODE_H

#include <vgui/VGUI.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: names mouse button inputs
//-----------------------------------------------------------------------------
enum MouseCode
{
	MOUSE_LEFT = 0,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_LAST,
};

} // namespace vgui

#endif // MOUSECODE_H
