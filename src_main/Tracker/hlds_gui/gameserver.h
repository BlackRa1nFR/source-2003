//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( CGAMESERVER_H )
#define CGAMESERVER_H
#ifdef _WIN32
#pragma once
#endif


#include "engine_hlds_api.h"

#include <VGUI_Frame.h>
#include <VGUI_ListPanel.h>
#include <VGUI_PHandle.h>
#include <VGUI_PropertyPage.h>
#include "UtlVector.h"



namespace vgui
{
class Label;
class Font;
class ListPanel;
class Button;
class ComboBox;
class QueryBox;
class TextEntry;
class CheckButton;
class PropertySheet;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CGameServer : public vgui::PropertyPage
{
	typedef vgui::PropertyPage BaseClass;

public:
	// Construction/destruction
						CGameServer( vgui::Panel *parent, const char *name);
	virtual				~CGameServer();

	// displays the dialog, moves it into focus, updates if it has to
	virtual void		Open( void );
	// setup
	virtual void		PerformLayout();

	virtual void Stop();

	virtual void ConsoleText(const char *text);

	virtual void UpdateStatus(float fps,const char *map,int maxplayers,int numplayers);

	virtual void Start(const char *cmdline,const char *cvars);
private:

	// called when dialog is shut down
	virtual void OnCommand(const char *command);
	void OnSendCommand();

	void SetLabelText(const char *textEntryName, const char *text);


	DECLARE_PANELMAP();

private:
	// thread vars
	bool start;
		
	// the engine
	IDedicatedServerAPI *engine;


	// GUI elements
	vgui::TextEntry *m_ConsoleTextEntry;
	vgui::TextEntry *m_CommandTextEntry;
	vgui::Button *m_SubmitButton;
	vgui::Button *m_QuitButton;

};

#endif // CGAMESERVER_H