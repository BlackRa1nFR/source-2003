//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "LoadingDialog.h"
//#include "GameUISurface.h"
#include "EngineInterface.h"

#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/ISystem.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include "vstdlib/icommandline.h"

#include "ModInfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLoadingDialog::CLoadingDialog(vgui::Panel *parent) : Frame(parent, "LoadingDialog")
{
	SetSize(416, 100);
	SetTitle("#GameUI_Loading", true);
	m_bShowingSecondaryProgress = false;
	m_flSecondaryProgress = 0.0f;
	m_flLastSecondaryProgressUpdateTime = 0.0f;
	m_flSecondaryProgressStartTime = 0.0f;

	m_pProgress = new ProgressBar(this, "Progress");
	m_pProgress2 = new ProgressBar(this, "Progress2");
	m_pInfoLabel = new Label(this, "InfoLabel", "");
	m_pCancelButton = new Button(this, "CancelButton", "Cancel");
	m_pHTML = new HTML(this, "BannerAd");
	m_pTimeRemainingLabel = new Label(this, "TimeRemainingLabel", "");
	m_pCancelButton->SetCommand("Cancel");

	m_pInfoLabel->SetBounds(20, 32, 392, 24);
	m_pProgress->SetBounds(20, 64, 300, 24); 
	m_pCancelButton->SetBounds(330, 64, 72, 24);
	m_pHTML->SetBounds(24, 204, 340, 56);
	m_pHTML->SetScrollbarsEnabled(false);

	m_pInfoLabel->SetTextColorState(Label::CS_DULL);

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	m_pProgress2->SetVisible(false);

	// get steam content sponsor from registry
	if (CommandLine()->FindParm("-steam") || CommandLine()->FindParm("-showplatform"))
	{
		char sponsorURL[1024];
		if (system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Steam\\LastContentProviderURL", sponsorURL, sizeof(sponsorURL)))
		{
			m_pHTML->OpenURL(sponsorURL);
		}
		LoadControlSettings("Resource/LoadingDialog.res");
	}
	else if ( ModInfo().IsSinglePlayerOnly() )
	{
		LoadControlSettings("Resource/LoadingDialogNoBannerSingle.res");
	}
	else
	{
		LoadControlSettings("Resource/LoadingDialogNoBanner.res");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLoadingDialog::~CLoadingDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the loading screen, initializing and making it visible
//-----------------------------------------------------------------------------
void CLoadingDialog::DisplayProgressBar(const char *resourceType, const char *resourceName)
{
	SetTitle("#GameUI_Loading", true);
	vgui::surface()->RestrictPaintToSinglePanel(GetVPanel());
	vgui::surface()->SetModalPanel(GetVPanel());
	BaseClass::Activate();

	m_pProgress->SetVisible(true);
	if ( !ModInfo().IsSinglePlayerOnly() )
	{
		m_pInfoLabel->SetVisible(true);
	}
	m_pInfoLabel->SetText("");
	
	m_pCancelButton->SetText("Cancel");
	m_pCancelButton->SetCommand("Cancel");
}

//-----------------------------------------------------------------------------
// Purpose: Turns dialog into error display
//-----------------------------------------------------------------------------
void CLoadingDialog::DisplayError(const char *failureReason, const char *extendedReason)
{
	LoadControlSettings("Resource/LoadingDialogError.res");

	SetTitle("#GameUI_Disconnected", true);
	vgui::surface()->RestrictPaintToSinglePanel(GetVPanel());
	vgui::surface()->SetModalPanel(GetVPanel());
	BaseClass::Activate();
	
	m_pProgress->SetVisible(false);
	m_pHTML->SetVisible(false);
	m_pInfoLabel->SetVisible(true);
	m_pCancelButton->SetText("Close");
	m_pCancelButton->SetCommand("Close");

	if ( extendedReason && strlen( extendedReason ) > 0 ) 
	{
		wchar_t compositeReason[256], finalMsg[512], formatStr[256];
		if ( extendedReason[0] == '#' )
		{
			wcsncpy(compositeReason, localize()->Find(extendedReason), sizeof( compositeReason ) / sizeof( wchar_t ) );
		}
		else
		{
			vgui::localize()->ConvertANSIToUnicode(extendedReason, compositeReason, sizeof( compositeReason ));
		}

		if ( failureReason[0] == '#' )
		{
			wcsncpy(formatStr, localize()->Find(failureReason), sizeof( formatStr ) / sizeof( wchar_t ) );
		}
		else
		{
			vgui::localize()->ConvertANSIToUnicode(failureReason, formatStr, sizeof( formatStr ));
		}

		vgui::localize()->ConstructString(finalMsg, sizeof( finalMsg ), formatStr, 1, compositeReason);
		m_pInfoLabel->SetText(finalMsg);
	}
	else
	{
		m_pInfoLabel->SetText(failureReason);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets status info text
//-----------------------------------------------------------------------------
void CLoadingDialog::SetStatusText(const char *statusText)
{
	m_pInfoLabel->SetText(statusText);
}

//-----------------------------------------------------------------------------
// Purpose: updates time remaining
//-----------------------------------------------------------------------------
void CLoadingDialog::OnThink()
{
	BaseClass::OnThink();
	if (m_bShowingSecondaryProgress)
	{
		// calculate the time remaining string
		wchar_t unicode[512];
		if (ProgressBar::ConstructTimeRemainingString(unicode, sizeof(unicode), m_flSecondaryProgressStartTime, (float)system()->GetFrameTime(), m_flSecondaryProgress, m_flLastSecondaryProgressUpdateTime, true))
		{
			m_pTimeRemainingLabel->SetText(unicode);
		}
		else
		{
			m_pTimeRemainingLabel->SetText("");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadingDialog::PerformLayout()
{
	MoveToCenterOfScreen();
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadingDialog::SetProgressPoint(int progressPoint)
{
	m_pProgress->SetProgress((float)progressPoint / (m_iRangeMax - m_iRangeMin));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadingDialog::SetProgressRange(int min, int max)
{
	m_iRangeMin = min;
	m_iRangeMax = max;
}

//-----------------------------------------------------------------------------
// Purpose: sets and shows the secondary progress bar
//-----------------------------------------------------------------------------
void CLoadingDialog::SetSecondaryProgress(float progress)
{
	// don't show the progress if we've jumped right to completion
	if (!m_bShowingSecondaryProgress && progress > 0.99f)
		return;

	// if we haven't yet shown secondary progress then reconfigure the dialog
	if (!m_bShowingSecondaryProgress)
	{
		LoadControlSettings("Resource/LoadingDialogDualProgress.res");
		m_bShowingSecondaryProgress = true;
		m_pProgress2->SetVisible(true);
		m_flSecondaryProgressStartTime = (float)system()->GetFrameTime();
	}

	// if progress has increased then update the progress counters
	if (progress > m_flSecondaryProgress)
	{
		m_pProgress2->SetProgress(progress);
		m_flSecondaryProgress = progress;
		m_flLastSecondaryProgressUpdateTime = (float)system()->GetFrameTime();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadingDialog::SetSecondaryProgressText(const char *statusText)
{
	SetControlString("SecondaryProgressLabel", statusText);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadingDialog::OnClose()
{
	// remove any rendering restrictions
	vgui::surface()->RestrictPaintToSinglePanel(NULL);
	vgui::surface()->SetModalPanel(NULL);

	BaseClass::OnClose();

	// delete ourselves
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: command handler
//-----------------------------------------------------------------------------
void CLoadingDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "Cancel"))
	{
		// disconnect from the server
		engine->ClientCmd("disconnect\n");

		// close
		Close();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Maps ESC to quiting loading
//-----------------------------------------------------------------------------
void CLoadingDialog::OnKeyCodePressed(KeyCode code)
{
	if (code == KEY_ESCAPE)
	{
		OnCommand("Cancel");
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}
