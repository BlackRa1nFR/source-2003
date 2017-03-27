//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HELPDIALOG_H
#define HELPDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "taskframe.h"

namespace vgui
{
class Button;
class CheckButton;
class Label;
};

//-----------------------------------------------------------------------------
// Purpose: First-time tracker use help dialog
//			Tells user how to open/close tracker
//-----------------------------------------------------------------------------
class CHelpDialog : public CTaskFrame
{
public:
	CHelpDialog();
	~CHelpDialog();

protected:
	virtual void OnClose();
	virtual void OnCommand(const char *close);

private:
	vgui::Label *m_pInfoText;
	vgui::Button *m_pClose;
	vgui::CheckButton *m_pNeverShowAgain;

	typedef CTaskFrame BaseClass;
};


#endif // HELPDIALOG_H
