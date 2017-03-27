//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Wraps pointers to basic vgui interfaces
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_INTERNAL_H
#define VGUI_INTERNAL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "interface.h"

class IFileSystem;

namespace vgui
{

bool VGui_InternalLoadInterfaces( CreateInterfaceFn *factoryList, int numFactories );

// <vgui/IInputInternal.h> header
class IInputInternal *input();

// <vgui/IScheme.h> header
class ISchemeManager *scheme();

// <vgui/ISurface.h> header
class ISurface *surface();

// <vgui/ISystem.h> header
class ISystem *system();

// <vgui/IVGui.h> header
class IVGui *ivgui();

// "FileSystem.h" header
IFileSystem *filesystem();

// <vgui/ILocalize.h> header
class ILocalize *localize();

// <vgui/IPanel.h> header
class IPanel *ipanel();

// methods
void vgui_strcpy(char *dst, int dstLen, const char *src);
} // namespace vgui




#endif // VGUI_INTERNAL_H
