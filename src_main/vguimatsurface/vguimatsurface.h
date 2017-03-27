//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implementation of the VGUI ISurface interface using the 
// material system to implement it
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef VGUIMATSURFACE_H
#define VGUIMATSURFACE_H

#include <vgui/VGUI.h>

class IMaterialSystem;
class IFileSystem;

namespace vgui
{
	class IPanel;
	class ISurface;
	class IInput;
	class IVGui;
	class IInputInternal; 
}

//-----------------------------------------------------------------------------
// Globals...
//-----------------------------------------------------------------------------
extern IMaterialSystem *g_pMaterialSystem;
extern IFileSystem *g_pFileSystem;
extern vgui::IPanel *g_pIPanel;
extern vgui::IInputInternal *g_pIInput;
extern vgui::ISurface *g_pISurface;
extern vgui::IVGui *g_pIVGUI;
	    
#endif // VGUIMATSURFACE_H


