//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "NewGameDialog.h"

#include "EngineInterface.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui/ISurface.h>
#include <vgui_controls/RadioButton.h>
#include <stdio.h>
#include "ModInfo.h"

using namespace vgui;

#include "GameUI_Interface.h"
#include "taskframe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CNewGameDialog::CNewGameDialog(vgui::Panel *parent) : CTaskFrame(parent, "NewGameDialog")
{
	SetBounds(0, 0, 372, 160);
	SetSizeable( false );

	MakePopup();
	g_pTaskbar->AddTask(GetVPanel());

	//vgui::surface()->CreatePopup( GetVPanel(), false );

	SetTitle("#GameUI_NewGame", true);

	new vgui::Label( this, "HelpText", "#GameUI_NewGameHelpText" );

	// radio buttons to tab between controls
	m_pTraining = new vgui::RadioButton(this, "Training", "#GameUI_TrainingRoom");
	m_pTraining->SetSelected( true );
	m_nPlayMode = 0;
	m_pEasy = new vgui::RadioButton(this, "Easy", "#GameUI_Easy");
	m_pMedium = new vgui::RadioButton(this, "Medium", "#GameUI_Medium");
	m_pHard = new vgui::RadioButton(this, "Hard", "#GameUI_Hard");

	vgui::Button *play = new vgui::Button( this, "Play", "#GameUI_Play" );
	play->SetCommand( "Play" );

	vgui::Button *cancel = new vgui::Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	LoadControlSettings("Resource\\NewGameDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNewGameDialog::~CNewGameDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CNewGameDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Play" ) )
	{
		// Determine mode
		m_nPlayMode = 0;

		if ( m_pEasy->IsSelected() )
		{
			m_nPlayMode = 1;
		}
		else if ( m_pMedium->IsSelected() )
		{
			m_nPlayMode = 2;
		}
		else if ( m_pHard->IsSelected() )
		{
			m_nPlayMode = 3;
		}

		char mapcommand[ 512 ];
		mapcommand[ 0 ] = 0;

		// Find appropriate key
		if ( m_nPlayMode == 0 )
		{
			sprintf(mapcommand, "disconnect\nmaxplayers 1\ndeathmatch 0\nmap %s\n", ModInfo().GetTrainMap() );
		}
		else
		{
			sprintf(mapcommand, "disconnect\nmaxplayers 1\ndeathmatch 0\nskill %i\nmap %s\n", m_nPlayMode, ModInfo().GetStartMap() );
		}

		engine->ClientCmd( mapcommand );
		OnClose();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewGameDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}

