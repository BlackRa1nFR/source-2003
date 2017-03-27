//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELOPTIONSCHAT_H
#define SUBPANELOPTIONSCHAT_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>

//-----------------------------------------------------------------------------
// Purpose: Sounds property page
//-----------------------------------------------------------------------------
class CSubPanelOptionsChat : public vgui::PropertyPage
{
public:
	CSubPanelOptionsChat();
	~CSubPanelOptionsChat();

	virtual void OnResetData();
	virtual void OnApplyChanges();

private:
	typedef vgui::PropertyPage BaseClass;
};




#endif // SUBPANELOPTIONSCHAT_H
