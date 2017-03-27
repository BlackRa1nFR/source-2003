//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CVARNEGATECHECKBUTTON_H
#define CVARNEGATECHECKBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/CheckButton.h>

class CCvarNegateCheckButton : public vgui::CheckButton
{
public:
	CCvarNegateCheckButton( vgui::Panel *parent, const char *panelName, const char *text, 
		char const *cvarname );
	~CCvarNegateCheckButton();

	virtual void	SetSelected( bool state );
	virtual void	Paint();

	void			Reset();
	void			ApplyChanges();
	bool			HasBeenModified();

private:
	typedef vgui::CheckButton BaseClass;
	
	DECLARE_PANELMAP();
	void			OnButtonChecked();

	char			*m_pszCvarName;
	bool			m_bStartState;
};

#endif // CVARNEGATECHECKBUTTON_H
