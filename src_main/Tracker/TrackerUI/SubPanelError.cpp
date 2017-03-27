//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelError.h"

#include <VGUI_Label.h>
#include <VGUI_WizardPanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelError::CSubPanelError(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_pErrorText = new Label(this, "ErrorText", "ERROR TEXT");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *errorText - 
//-----------------------------------------------------------------------------
void CSubPanelError::SetErrorText(const char *errorText)
{
	//!! need to localise
	m_pErrorText->SetText(errorText);

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Moves the error text into the center of the dialog
//-----------------------------------------------------------------------------
void CSubPanelError::PerformLayout()
{
	int wide, tall;
	GetSize(wide, tall);

	m_pErrorText->SetBounds(10, 10, wide - 20, tall - 20);

	GetWizardPanel()->SetNextButtonEnabled(false);
	GetWizardPanel()->SetFinishButtonEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Never a next panel from an error panel
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelError::GetNextSubPanel()
{
	return NULL;
}
