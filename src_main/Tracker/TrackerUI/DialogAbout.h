//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGABOUT_H
#define DIALOGABOUT_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>

namespace vgui
{
class Button;
class TextEntry;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog that displays information about the current tracker install
//-----------------------------------------------------------------------------
class CDialogAbout : public vgui::Frame
{
public:
	CDialogAbout();
	~CDialogAbout();

	// activates the dialog, brings it to the front
	void Run();

private:
	// vgui overrides
	virtual void PerformLayout();

	// dialog deletes itself on close
	virtual void OnClose();

	vgui::Button *m_pClose;
	vgui::TextEntry *m_pText;

	typedef vgui::Frame BaseClass;
};


#endif // DIALOGABOUT_H
