//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGOPTIONS_H
#define DIALOGOPTIONS_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyDialog.h>

//-----------------------------------------------------------------------------
// Purpose: Preferences dialog
//-----------------------------------------------------------------------------
class CDialogOptions : public vgui::PropertyDialog
{
public:
	CDialogOptions();
	~CDialogOptions();

	void Run();

	// dialog deletes itself on close
	virtual void OnClose();

private:
	typedef vgui::Frame BaseClass;
};


#endif // DIALOGOPTIONS_H
