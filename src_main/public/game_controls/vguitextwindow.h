//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUITEXTWINDOW_H
#define VGUITEXTWINDOW_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>

namespace vgui
{
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: displays the MOTD
//-----------------------------------------------------------------------------
class CVGUITextWindow : public vgui::Frame
{
private:
	typedef vgui::Frame BaseClass;

public:
	CVGUITextWindow(vgui::Panel *parent);
	virtual ~CVGUITextWindow();

	void Activate(const char *title,const char *message);
	void Activate(const wchar_t *title,const wchar_t *message);
private:	
	// vgui overrides
	virtual void OnCommand( const char *command);

	vgui::RichText *m_pMessage;
	vgui::Button *m_pOK;
};


#endif // VGUITEXTWINDOW_H
