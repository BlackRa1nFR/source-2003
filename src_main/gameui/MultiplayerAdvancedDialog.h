//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MULTIPLAYERADVANCEDDIALOG_H
#define MULTIPLAYERADVANCEDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include "TaskFrame.h"
#include "ScriptObject.h"

class CPanelListPanel;

class CMultiplayerAdvanced 
{
protected:
	void CreateControls();
	void DestroyControls();
	void GatherCurrentValues();
	void SaveValues();

	CInfoDescription *m_pDescription;

	mpcontrol_t *m_pList;

	CPanelListPanel *m_pListPanel;
};


class CMultiplayerAdvancedDialog : public CTaskFrame, public CMultiplayerAdvanced
{
	typedef CTaskFrame BaseClass;

public:
	CMultiplayerAdvancedDialog(vgui::Panel *parent);
	~CMultiplayerAdvancedDialog();

	virtual void OnCommand( const char *command );
	virtual void OnClose();

};


class CMultiplayerAdvancedPage : public vgui::PropertyPage, public CMultiplayerAdvanced
{
	typedef vgui::PropertyPage BaseClass;

public:
	CMultiplayerAdvancedPage(vgui::Panel *parent);
	~CMultiplayerAdvancedPage();

	virtual void OnResetData();
	virtual void OnApplyChanges();


};

#endif // MULTIPLAYERADVANCEDDIALOG_H
