//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VMTEDIT_H
#define VMTEDIT_H

#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
	class ISurface;
	class IVGui;
}

class IFileSystem;
class IMaterialSystem;
class IMatSystemSurface;


extern IFileSystem *g_pFileSystem;
extern IMaterialSystem *g_pMaterialSystem;
extern IMatSystemSurface *g_pMatSystemSurface;
extern vgui::ISurface *g_pVGuiSurface;
extern vgui::IVGui	*g_pVGui;


#endif // VMTEDIT_H
