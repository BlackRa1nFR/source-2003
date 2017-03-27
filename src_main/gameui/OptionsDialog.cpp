//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "OptionsDialog.h"

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>

#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui_controls/PropertySheet.h>

#include "OptionsSubKeyboard.h"
#include "OptionsSubMouse.h"
#include "OptionsSubAudio.h"
#include "OptionsSubVideo.h"
#include "OptionsSubVoice.h"
#include "OptionsSubAdvanced.h"
#include "OptionsSubMultiplayer.h"
#include "MultiplayerAdvancedDialog.h"
#include "ModInfo.h"

using namespace vgui;

#include "Taskbar.h"
extern CTaskbar *g_pTaskbar;
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
COptionsDialog::COptionsDialog(vgui::Panel *parent) : PropertyDialog(parent, "OptionsDialog")
{
	SetBounds(0, 0, 512, 406);
	SetSizeable( false );

	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_Options", true);

	// debug timing code, this function takes too long
//	double s4 = system()->GetCurrentTime();

	// add the multiplayer tab first if we're a multiplayer-only game
	if (ModInfo().IsMultiplayerOnly() && !ModInfo().IsSinglePlayerOnly() )
	{
		AddPage(new COptionsSubMultiplayer(this), "#GameUI_Multiplayer");
	}

	AddPage(new COptionsSubKeyboard(this), "#GameUI_Keyboard");
	AddPage(new COptionsSubMouse(this), "#GameUI_Mouse");
	AddPage(new COptionsSubAudio(this), "#GameUI_Audio");
	AddPage(new COptionsSubVideo(this), "#GameUI_Video");
	AddPage(new COptionsSubVoice(this), "#GameUI_Voice");
	AddPage(new COptionsSubAdvanced(this), "#GameUI_Advanced");

//	double s5 = system()->GetCurrentTime();
//	ivgui()->DPrintf("COptionsDialog::COptionsDialog(): %.3fms\n", (float)(s5 - s4) * 1000.0f);

	if ( ModInfo().IsSinglePlayerOnly() ) 
	{
		AddPage(new CMultiplayerAdvancedPage(this), "#GameUI_AdvancedNoEllipsis");
	}

	if (!ModInfo().IsMultiplayerOnly() && !ModInfo().IsSinglePlayerOnly() )
	{
		AddPage(new COptionsSubMultiplayer(this), "#GameUI_Multiplayer");
	}

	SetApplyButtonVisible(true);

	GetPropertySheet()->SetTabWidth(72);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
COptionsDialog::~COptionsDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Overrides the base class so it can setup the taskbar title properly
//-----------------------------------------------------------------------------
void COptionsDialog::SetTitle(const char *title, bool surfaceTitle)
{
	BaseClass::SetTitle(title,surfaceTitle);
	if (g_pTaskbar)
	{
		wchar_t *wTitle;
		wchar_t w_szTitle[1024];

		wTitle = vgui::localize()->Find(title);

		if(!wTitle)
		{
			vgui::localize()->ConvertANSIToUnicode(title,w_szTitle,sizeof(w_szTitle));
			wTitle = w_szTitle;
		}

		g_pTaskbar->SetTitle(GetVPanel(), wTitle);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Brings the dialog to the fore
//-----------------------------------------------------------------------------
void COptionsDialog::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

//-----------------------------------------------------------------------------
// Purpose: Opens the dialog
//-----------------------------------------------------------------------------
void COptionsDialog::Run()
{
	SetTitle("#GameUI_Options", true);
	Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Deletes the dialog on close
//-----------------------------------------------------------------------------
void COptionsDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}
