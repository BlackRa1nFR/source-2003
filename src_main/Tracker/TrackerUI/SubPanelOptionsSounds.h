//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELOPTIONSSOUNDS_H
#define SUBPANELOPTIONSSOUNDS_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>

//-----------------------------------------------------------------------------
// Purpose: Sounds property page
//-----------------------------------------------------------------------------
class CSubPanelOptionsSounds : public vgui::PropertyPage
{
public:
	CSubPanelOptionsSounds();
	~CSubPanelOptionsSounds();

	virtual void OnResetData();
	virtual void OnApplyChanges();

private:
	typedef vgui::PropertyPage BaseClass;
};


#endif // SUBPANELOPTIONSSOUNDS_H
