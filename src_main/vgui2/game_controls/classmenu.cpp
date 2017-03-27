//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include <game_controls/ClassMenu.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>


#include "IGameUIFuncs.h" // for key bindings
extern IGameUIFuncs *gameuifuncs; // for key binding details
#include <game_controls/IViewPort.h>

#include <stdlib.h> // MAX_PATH define

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClassMenu::CClassMenu(vgui::Panel *parent) : Frame(parent, "ClassMenu")
{
	m_pFirstButton = NULL;
	m_iScoreBoardKey = KEY_NONE; // this is looked up in Activate()
	m_iFirst = 1;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this map
	m_pPanel = new Panel( this, "ClassInfo" );

	LoadControlSettings("Resource/UI/ClassMenu.res");
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClassMenu::~CClassMenu()
{
}


Panel *CClassMenu::CreateControlByName(const char *controlName)
{
	if( !stricmp( "MouseOverPanelButton", controlName ) )
	{
		ClassHelperPanel *classPanel = new ClassHelperPanel( this, NULL );
		classPanel->SetVisible( false );

		int x,y,wide,tall;
		m_pPanel->GetBounds( x, y, wide, tall );
		classPanel->SetBounds( x, y, wide, tall );

		MouseOverPanelButton *newButton = new MouseOverPanelButton( this, NULL, classPanel );
		m_pClassButtons.AddToTail( newButton );		
		classPanel->SetButton( newButton );
		if ( m_iFirst )
		{
			m_iFirst = 0;
			m_pFirstButton = newButton;
		}
		return newButton;
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CClassMenu::OnCommand( const char *command)
{
	if ( stricmp( command, "vguicancel" ) )
	{
		gViewPortInterface->GetClientDllInterface()->ClientCmd( const_cast<char *>( command ));
	}
	Close();
	gViewPortInterface->HideBackGround();
	BaseClass::OnCommand(command);
}


//-----------------------------------------------------------------------------
// Purpose: shows the class menu
//-----------------------------------------------------------------------------
void CClassMenu::Activate( )
{
	BaseClass::Activate();

	SetVisible( true );
	// load a default class page
	if ( m_pFirstButton )
	{
		m_pFirstButton->ShowPage();
	}
	
	if ( m_iScoreBoardKey == KEY_NONE ) 
	{
		m_iScoreBoardKey = gameuifuncs->GetVGUI2KeyCodeForBind( "showscores" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates the set of valid classes to let the user see
//-----------------------------------------------------------------------------
void CClassMenu::Update( int *validClasses, int numClasses)
{
	int i = 0;
	int teamNumber = gViewPortInterface->GetClientDllInterface()->TeamNumber();
	if (validClasses[teamNumber] == -1) // civilian button only
	{
		for( i = 0; i < m_pClassButtons.Count(); i++ )
		{
			if( !stricmp(m_pClassButtons[i]->GetName(), "civilian") )
			{
				m_pClassButtons[i]->SetEnabled(true);
			}
			else
			{
				m_pClassButtons[i]->SetEnabled(false);
			}
		}
	}
	else
	{
		for( i = 0; i < m_pClassButtons.Count(); i++ )
		{
			// Is it an illegal class?
			if ( m_pClassButtons[i]->GetClass()==-1 || (validClasses[0] & m_pClassButtons[i]->GetClass() ) || 
				 (validClasses[teamNumber] & m_pClassButtons[i]->GetClass() ) )
			{
					m_pClassButtons[i]->SetEnabled(false);
			}
			else
			{
					m_pClassButtons[i]->SetEnabled(true);
			}
		}
	}
			/*
			TODO: work out what this does and see if we need to do it...

	// Only check current class if they've got autokill on
	bool bAutoKill = CVAR_GET_FLOAT( "hud_classautokill" ) != 0;
	if ( bAutoKill )
	{	
		// Is it the player's current class?
		if ( (gViewPort->IsRandomPC() && m_iPlayerClass == PC_RANDOM) || (!gViewPort->IsRandomPC() && (m_iPlayerClass == g_iPlayerClass)) )
			return true;
	}



*/

	//TODO: reposition the list to put the visible ones at the top???
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CClassMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CClassMenu::OnKeyCodePressed(KeyCode code)
{
	if ( m_iScoreBoardKey!=KEY_NONE && m_iScoreBoardKey == code )
	{
		if ( !gViewPortInterface->IsScoreBoardVisible() )
		{
			gViewPortInterface->HideBackGround();
			gViewPortInterface->ShowScoreBoard();
			SetVisible( false );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: causes the class panel to load the resource file for its class
//-----------------------------------------------------------------------------
void CClassMenu::ClassHelperPanel::ApplySchemeSettings(IScheme *pScheme)
{
	const char *name = GetName();
	LoadControlSettings( GetClassPage( name ));
#ifdef _TFC
	if ( m_pButton )
	{
			// map string names to illegal class ints, used in Update()
		if ( !stricmp("scout", name ) )
		{
			m_pButton->SetClass( TF_ILL_SCOUT, -1 );
		}
		else if ( !stricmp("sniper", name ) )
		{
			m_pButton->SetClass( TF_ILL_SNIPER, -1 );
		}
		else if ( !stricmp("soldier", name ) )
		{
			m_pButton->SetClass( TF_ILL_SOLDIER, -1 );
		}
		else if ( !stricmp("demoman", name ) )
		{
			m_pButton->SetClass( TF_ILL_DEMOMAN, -1 );
		}
		else if ( !stricmp("medic", name ) )
		{
			m_pButton->SetClass( TF_ILL_MEDIC, -1 );
		}
		else if ( !stricmp("hwguy", name ) )
		{
			m_pButton->SetClass( TF_ILL_HVYWEP, -1 );
		}
		else if ( !stricmp("pyro", name ) )
		{
			m_pButton->SetClass( TF_ILL_PYRO, -1 );
		}
		else if ( !stricmp("spy", name ) )
		{
			m_pButton->SetClass( TF_ILL_SPY, -1 );
		}
		else if ( !stricmp("engineer", name) )
		{
			m_pButton->SetClass( TF_ILL_ENGINEER, -1 );
		}
		else if ( !stricmp("randompc", name ) )
		{
			m_pButton->SetClass( TF_ILL_RANDOMPC, -1 );// this is the 11th entry...
		}
		else
		{
			m_pButton->SetClass( -1, -1 );
		}
	}
#endif

	BaseClass::ApplySchemeSettings(pScheme);
}


//-----------------------------------------------------------------------------
// Purpose: returns the filename of the class file for this class
//-----------------------------------------------------------------------------
const char *CClassMenu::ClassHelperPanel::GetClassPage( const char *className )
{
	static char classPanel[ _MAX_PATH ];
	_snprintf( classPanel, sizeof( classPanel ), "classes/short_%s.res", className);

	if ( vgui::filesystem()->FileExists( classPanel ) )
	{
	}
	else if (vgui::filesystem()->FileExists( "classes/default.res" ) )
	{
		_snprintf ( classPanel, sizeof( classPanel ), "classes/default.res" );
	}
	else
	{
		return NULL;
	}

	return classPanel;
}