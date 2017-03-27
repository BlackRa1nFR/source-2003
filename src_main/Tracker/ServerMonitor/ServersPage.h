//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERSPAGE_H
#define SERVERSPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>
#include "UtlVector.h"

#include "ServerInfoPanel.h"

//-----------------------------------------------------------------------------
// Purpose: Property page containing the list of servers
//-----------------------------------------------------------------------------
class CServersPage : public vgui::PropertyPage
{
public:
	CServersPage();
	~CServersPage();

	virtual void PerformLayout();

private:
	CUtlVector<CServerInfoPanel *> m_Panels;

	typedef vgui::PropertyPage BaseClass;
};


#endif // SERVERSPAGE_H
