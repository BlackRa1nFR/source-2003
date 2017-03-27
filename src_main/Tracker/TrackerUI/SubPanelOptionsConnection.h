//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELOPTIONSCONNECTION_H
#define SUBPANELOPTIONSCONNECTION_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>

namespace vgui
{
class ComboBox;
};

//-----------------------------------------------------------------------------
// Purpose: Sounds property page
//-----------------------------------------------------------------------------
class CSubPanelOptionsConnection : public vgui::PropertyPage
{
public:
	CSubPanelOptionsConnection();
	~CSubPanelOptionsConnection();

	virtual void OnResetData();
	virtual void OnApplyChanges();

private:
	vgui::ComboBox *m_pInternetSpeed;

	typedef vgui::PropertyPage BaseClass;
};


#endif // SUBPANELOPTIONSCONNECTION_H
