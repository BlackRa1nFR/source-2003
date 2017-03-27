//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef STEAMPASSWORDDIALOG_H
#define STEAMPASSWORDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "taskframe.h"

namespace vgui
{
	class RadioButton;
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSteamPasswordDialog : public CTaskFrame
{
public:
	CSteamPasswordDialog( vgui::Panel *parent, const char *szAccountName, const char *szUserName );
	~CSteamPasswordDialog();

	virtual void OnCommand( const char *command );
	virtual void OnClose();
protected:
	typedef CTaskFrame BaseClass;

	vgui::TextEntry *m_pPassword;

private:

	bool _isValidPassword_TempTestImp( char *szPassword );
	char m_szAccountName[256];
	char m_szUserName[256];

};


#endif // STEAMPASSWORDDIALOG_H
