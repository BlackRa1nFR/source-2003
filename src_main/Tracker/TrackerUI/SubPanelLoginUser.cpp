//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelLoginUser.h"
#include "SubPanelCreateUser.h"
#include "Tracker.h"

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>
#include <VGUI_KeyValues.h>
#include <VGUI_TextEntry.h>
#include <VGUI_WizardPanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSubPanelLoginUser::CSubPanelLoginUser(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_pEmailEdit = new TextEntry(this, "EmailEdit");
	m_pPassword  = new TextEntry(this, "PasswordEdit");

	m_pEmailEdit->AddActionSignalTarget(this);
	m_pPassword->AddActionSignalTarget(this);

	LoadControlSettings("Friends/SubPanelLoginUser.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelLoginUser::PerformLayout()
{
	GetWizardPanel()->SetFinishButtonEnabled(false);
	GetWizardPanel()->SetNextButtonEnabled(VerifyEntriesAreValid());

	GetWizardPanel()->SetTitle("#TrackerUI_Friends_LoginExistingUserTitle", true);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelLoginUser::GetNextSubPanel()
{
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelLoginUser2"));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelLoginUser::OnNextButton()
{
	// write the data into the doc
	KeyValues *doc = GetWizardData();

	char buf[256];
	m_pEmailEdit->GetText(0, buf, 255);
	doc->SetString("email", buf);

	m_pPassword->GetText(0, buf, 255);
	doc->SetString("password", buf);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelLoginUser::VerifyEntriesAreValid()
{
	char buf[256];

	// make sure the email address is valid
	m_pEmailEdit->GetText(0, buf, 255);
	if (!CSubPanelCreateUser::IsEmailAddressValid(buf))
		return false;

	// make sure some password is typed in
	m_pPassword->GetText(0, buf, 255);
	if (strlen(buf) < 1)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the text in any of the text entries changes
// Input  : 
//-----------------------------------------------------------------------------
void CSubPanelLoginUser::OnTextChanged()
{
	GetWizardPanel()->SetNextButtonEnabled(VerifyEntriesAreValid());
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelLoginUser::m_MessageMap[] =
{
	MAP_MESSAGE( CSubPanelLoginUser, "TextChanged", OnTextChanged ),
};
IMPLEMENT_PANELMAP(CSubPanelLoginUser, Panel);


