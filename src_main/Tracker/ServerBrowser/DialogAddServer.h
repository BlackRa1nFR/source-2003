//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGADDSERVER_H
#define DIALOGADDSERVER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

class IGameList;

//-----------------------------------------------------------------------------
// Purpose: Dialog which lets the user add a server by IP address
//-----------------------------------------------------------------------------
class CDialogAddServer : public vgui::Frame
{
public:
	CDialogAddServer(vgui::Panel *parent, IGameList *gameList);
	~CDialogAddServer();

private:
	virtual void OnClose();
	virtual void OnCommand(const char *command);

	void OnOK();

	IGameList *m_pGameList;

	typedef vgui::Frame BaseClass;
};


#endif // DIALOGADDSERVER_H
