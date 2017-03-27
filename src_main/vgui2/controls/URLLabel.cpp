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

#include <stdio.h>

#include <vgui/VGUI.h>
#include <vgui/ISystem.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include <vgui/Cursor.h>

#include <vgui_controls/URLLabel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
URLLabel::URLLabel(Panel *parent, const char *panelName, const char *text, const char *pszURL) : Label(parent, panelName, text)
{
    m_pszURL = NULL;
    m_iURLSize = 0;
    if (pszURL && strlen(pszURL) > 0)
    {
        SetURL(pszURL);
    }
}

//-----------------------------------------------------------------------------
// Purpose: unicode constructor
//-----------------------------------------------------------------------------
URLLabel::URLLabel(Panel *parent, const char *panelName, const wchar_t *wszText, const char *pszURL) : Label(parent, panelName, wszText)
{
    m_pszURL = NULL;
    m_iURLSize = 0;
    if (pszURL && strlen(pszURL) > 0)
    {
        SetURL(pszURL);
    }
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
URLLabel::~URLLabel()
{
    if (m_pszURL)
        delete [] m_pszURL;
}

//-----------------------------------------------------------------------------
// Purpose: sets the URL
//-----------------------------------------------------------------------------
void URLLabel::SetURL(const char *pszURL)
{
    int iNewURLSize = strlen(pszURL);
    if (iNewURLSize > m_iURLSize)
    {
        if (m_pszURL)
            delete [] m_pszURL;
        m_pszURL = new char [iNewURLSize + 1];
    }
    strcpy(m_pszURL, pszURL);
    m_iURLSize = iNewURLSize;
}

//-----------------------------------------------------------------------------
// Purpose: If we were left clicked on, launch the URL
//-----------------------------------------------------------------------------
void URLLabel::OnMousePressed(MouseCode code)
{
    if (code == MOUSE_LEFT)
    {
        if (m_pszURL)
	        system()->ShellExecute("open", m_pszURL); 
    }
}

//-----------------------------------------------------------------------------
// Purpose: Applies resouce settings
//-----------------------------------------------------------------------------
void URLLabel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	const char *pszURL = inResourceData->GetString("URLText", NULL);
	if (pszURL)
        SetURL(pszURL);
}


void URLLabel::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    SetCursor(dc_hand);
}

