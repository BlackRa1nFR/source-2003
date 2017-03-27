//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <game_controls/VGUITextWindow.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Button.h>

#include <game_controls/IViewPort.h>

using namespace vgui;

void UpdateCursorState();

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVGUITextWindow::CVGUITextWindow(vgui::Panel *parent) : Frame(parent, "VGUITextWindow")
{
	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pMessage = new RichText(this, "Message");

	m_pOK = new Button(this, "ok", "#PropertyDialog_OK");
	m_pOK->SetCommand("okay");

	LoadControlSettings("Resource/UI/TextWindow.res");
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVGUITextWindow::~CVGUITextWindow()
{
}

void CVGUITextWindow::OnCommand( const char *command)
{
    if (!stricmp(command, "okay"))
    {
		Close();
		gViewPortInterface->HideBackGround();

	}

	BaseClass::OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: shows the MOTD
//-----------------------------------------------------------------------------
void CVGUITextWindow::Activate( const char *title, const char *msg )
{
	BaseClass::Activate();

	SetTitle( title, false );
	m_pMessage->SetText( msg );
	m_pMessage->GotoTextStart();
	SetVisible( true );
}

void CVGUITextWindow::Activate( const wchar_t *title, const wchar_t *msg )
{
	BaseClass::Activate();

	SetTitle( title, false );
	m_pMessage->SetText( msg );
	m_pMessage->GotoTextStart();
	SetVisible( true );
}
