//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELOPTIONSPERSONAL_H
#define SUBPANELOPTIONSPERSONAL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>

//-----------------------------------------------------------------------------
// Purpose: Personal details property page, part of CDialogOptions
//-----------------------------------------------------------------------------
class CSubPanelOptionsPersonal : public vgui::PropertyPage
{
public:
	CSubPanelOptionsPersonal();
	~CSubPanelOptionsPersonal();

	virtual void OnResetData();
	virtual void OnApplyChanges();

private:
	typedef vgui::PropertyPage BaseClass;
};



#endif // SUBPANELOPTIONSPERSONAL_H
