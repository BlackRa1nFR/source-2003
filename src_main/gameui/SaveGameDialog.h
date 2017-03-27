//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SAVEGAMEDIALOG_H
#define SAVEGAMEDIALOG_H
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
class CSaveGameDialog : public CTaskFrame
{
public:

	static const int SAVEGAME_MAPNAME_LEN;
	static const int SAVEGAME_COMMENT_LEN;
	static const int SAVEGAME_ELAPSED_LEN;


	CSaveGameDialog(vgui::Panel *parent);
	~CSaveGameDialog();
	virtual void	OnCommand( const char *command );
	virtual void	OnClose();

protected:
	typedef CTaskFrame BaseClass;

	void			CreateSavedGamesList( void );
	void			ScanSavedGames( void );
	bool			ParseSaveData( char const* pszFileName, char const* pszShortName, KeyValues *kv );
	const char		*FindSaveSlot();

	vgui::ListPanel		*m_pGameList;
};


#endif // SAVEGAMEDIALOG_H
