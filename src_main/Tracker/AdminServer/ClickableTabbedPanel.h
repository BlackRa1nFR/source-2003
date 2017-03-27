//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CLICKABLETABBEDPANEL_H
#define CLICKABLETABBEDPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertySheet.h>
#include <VGUI_Frame.h>
#include <VGUI_Panel.h>
#include <VGUI_MouseCode.h>



namespace vgui
{
class Panel;
};


class CClickableTabbedPanel: public vgui2::PropertySheet 
{

public:
	CClickableTabbedPanel(vgui2::Panel *parent, const char *panelName);
	~CClickableTabbedPanel();


private:
//	void onMousePressed(vgui2::MouseCode code);


};

#endif //CLICKABLETABBEDPANEL