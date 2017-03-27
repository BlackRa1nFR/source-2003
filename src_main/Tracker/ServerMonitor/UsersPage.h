//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef USERSPAGE_H
#define USERSPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>

namespace vgui
{
class TextEntry;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CUsersPage : public vgui::PropertyPage
{
public:
	CUsersPage();
	~CUsersPage();

	virtual void PerformLayout();

private:
	vgui::TextEntry *m_pText;

	typedef vgui::PropertyPage BaseClass;

};


#endif // USERSPAGE_H
