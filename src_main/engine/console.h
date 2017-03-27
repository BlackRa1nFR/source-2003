#ifndef CONSOLE_H
#define CONSOLE_H
#pragma once

#include <vgui/VGUI.h>

#ifndef CONPRINT_H
#include "conprint.h"
#endif

namespace vgui
{
	class Panel;
}

typedef int qboolean;
typedef unsigned char byte;

//
// console
//
extern qboolean con_initialized;

void Con_Init (void);
void Con_Shutdown (void);   // Free overlay line buffer.
void Con_ClearNotify (void);

bool Con_IsVisible();

vgui::Panel* Con_GetConsolePanel();


#endif			// CONSOLE_H
