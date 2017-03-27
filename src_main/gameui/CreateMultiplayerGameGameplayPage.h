//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CREATEMULTIPLAYERGAMEGAMEPLAYPAGE_H
#define CREATEMULTIPLAYERGAMEGAMEPLAYPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CPanelListPanel;
class CDescription;
class mpcontrol_t;

//-----------------------------------------------------------------------------
// Purpose: server options page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameGameplayPage : public vgui::PropertyPage
{
public:
	CCreateMultiplayerGameGameplayPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameGameplayPage();

protected:
	virtual void OnApplyChanges();

private:
	void LoadGameOptionsList();
	void GatherCurrentValues();

	CDescription *m_pDescription;
	mpcontrol_t *m_pList;
	CPanelListPanel *m_pOptionsList;
};


#endif // CREATEMULTIPLAYERGAMEGAMEPLAYPAGE_H
