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
#if !defined( VINTERNETDLG_H )
#define VINTERNETDLG_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>
#include <VGUI_ListPanel.h>
#include <VGUI_PHandle.h>
#include "UtlVector.h"


#include "gameserver.h"
#include "CreateMultiplayerGameServerPage.h"

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
class PropertyPage;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class VInternetDlg : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	// Construction/destruction
						VInternetDlg();
	virtual				~VInternetDlg();

	virtual void		Initialize( void );

	// displays the dialog, moves it into focus, updates if it has to
	virtual void		Open( void );
	// setup
	virtual void		PerformLayout();


	// returns a pointer to a static instance of this dialog
	// valid for use only in sort functions
	static VInternetDlg *GetInstance();

	virtual void ConsoleText(const char *text);

	virtual void UpdateStatus(float fps,const char *map,int maxplayers,int numplayers);

	virtual void StartServer(const char *cmdline,const char *cvars);
	virtual void StopServer();

private:

	// called when dialog is shut down
	virtual void OnClose();

	// GUI elements
	vgui::PropertySheet *m_pDetailsSheet;
	CGameServer *m_pGameServer;
	CCreateMultiplayerGameServerPage *m_pConfigPage;

};

#endif // VINTERNETDLG_H