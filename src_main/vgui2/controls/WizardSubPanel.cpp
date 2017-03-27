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

#include <vgui_controls/WizardPanel.h>
#include <vgui_controls/WizardSubPanel.h>
#include <vgui_controls/BuildGroup.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
WizardSubPanel::WizardSubPanel(Panel *parent, const char *panelName) : EditablePanel(parent, panelName), _wizardPanel(NULL)
{
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
WizardSubPanel::~WizardSubPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardSubPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetBgColor(GetSchemeColor("SubPanelBgColor",pScheme));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *WizardSubPanel::GetWizardData()
{
	return GetWizardPanel()->GetWizardData();
}

WizardSubPanel *WizardSubPanel::GetSiblingSubPanelByName(const char *pageName)
{
	return GetWizardPanel()->GetSubPanelByName(pageName);
}
