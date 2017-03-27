//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LOADGAMEDIALOG_H
#define LOADGAMEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "taskframe.h"

namespace vgui
{
	class ListPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLoadGameDialog : public CTaskFrame
{
public:
	static const int SAVEGAME_MAPNAME_LEN;
	static const int SAVEGAME_COMMENT_LEN;
	static const int SAVEGAME_ELAPSED_LEN;

	CLoadGameDialog(vgui::Panel *parent);
	~CLoadGameDialog();

	virtual void	OnCommand( const char *command );
	virtual void	OnClose();
protected:

	void			CreateSavedGamesList( void );
	void			ScanSavedGames( void );
	bool			ParseSaveData( char const* pszFileName, char const* pszShortName, KeyValues *kv );

	vgui::ListPanel		*m_pGameList;
	typedef CTaskFrame BaseClass;

};


#endif // LOADGAMEDIALOG_H
