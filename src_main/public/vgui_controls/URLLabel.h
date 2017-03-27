//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef URLLABEL_H
#define URLLABEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Label.h>

namespace vgui
{

class URLLabel : public Label
{
public:	
	URLLabel(Panel *parent, const char *panelName, const char *text, const char *pszURL);
	URLLabel(Panel *parent, const char *panelName, const wchar_t *wszText, const char *pszURL);
    ~URLLabel();

    void SetURL(const char *pszURL);

protected:
	virtual void OnMousePressed(MouseCode code);
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings(IScheme *pScheme);

private:
    char    *m_pszURL;
    int     m_iURLSize;

	typedef Label BaseClass;
};

}

#endif // URLLABEL_H
