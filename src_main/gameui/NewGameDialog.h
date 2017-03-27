//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NEWGAMEDIALOG_H
#define NEWGAMEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "taskframe.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNewGameDialog : public CTaskFrame
{
public:
	CNewGameDialog(vgui::Panel *parent);
	~CNewGameDialog();

	virtual void OnCommand( const char *command );
	virtual void OnClose();
protected:
	typedef vgui::Frame BaseClass;

	int				m_nPlayMode;

	vgui::RadioButton	*m_pTraining;
	vgui::RadioButton	*m_pEasy;
	vgui::RadioButton	*m_pMedium;
	vgui::RadioButton	*m_pHard;
};


#endif // NEWGAMEDIALOG_H
