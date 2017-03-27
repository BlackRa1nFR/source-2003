//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VGUI_KEY_TRANSLATION_H
#define VGUI_KEY_TRANSLATION_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui/keycode.h>


// Convert from Windows scan codes to VGUI key codes.
vgui::KeyCode KeyCode_VirtualKeyToVGUI( int keyCode );

#endif // VGUI_KEY_TRANSLATION_H
