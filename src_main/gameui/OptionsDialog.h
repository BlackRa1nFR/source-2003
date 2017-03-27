//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

namespace vgui
{
class Button;
class CheckButton;
class Label;
};

#include <vgui_controls/PropertyDialog.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class COptionsDialog : public vgui::PropertyDialog
{
public:
	COptionsDialog(vgui::Panel *parent);
	~COptionsDialog();

	void Run();
	virtual void Activate();

protected:
	virtual void SetTitle(const char *title, bool surfaceTitle);
	virtual void OnClose();

	typedef vgui::PropertyDialog BaseClass;
};


#endif // OPTIONSDIALOG_H
