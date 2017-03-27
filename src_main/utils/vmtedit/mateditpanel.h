//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MATEDITPANEL_H
#define MATEDITPANEL_H

#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
	class Panel;
}

// add our main window
vgui::Panel *CreateVMTEditPanel();
void CreateMaterialEditorPanel(vgui::Panel *pParent);


#endif // MATEDITPANEL_H
