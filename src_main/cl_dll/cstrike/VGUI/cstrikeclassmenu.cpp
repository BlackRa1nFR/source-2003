//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "CStrikeClassMenu.h"

#include <KeyValues.h>
#include <FileSystem.h>
#include <vgui_controls/Button.h>
#include <vgui/IVGUI.h>

#include "hud.h" // for gEngfuncs
#include "cs_gamerules.h"

using namespace vgui;

const char *GetResourceFile( int team )
{
	switch( team )
	{
	case TEAM_TERRORIST:
		return "Resource/UI/ClassMenu_TER.res";
	case TEAM_CT:
	default:
		return "Resource/UI/ClassMenu_CT.res";
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCSClassMenu::CCSClassMenu(vgui::Panel *parent) : CClassMenu(parent)
{
	m_iFirst = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCSClassMenu::~CCSClassMenu()
{
}


//-----------------------------------------------------------------------------
// Purpose: Updates the menu with new information, DON'T call the base class for this one
//-----------------------------------------------------------------------------
void CCSClassMenu::Update( int *validClasses, int team )
{
	LoadControlSettings( GetResourceFile( team ) );

	if ( C_CSPlayer::GetLocalCSPlayer()->PlayerClass() == CS_CLASS_NONE )
	{
		SetVisibleButton( "CancelButton", false );
	}
	else
	{
		SetVisibleButton( "CancelButton", true ); 
	}
	
	// if we don't have the new models installed,
	// turn off the militia and spetsnaz buttons
	SetVisibleButton( "militia", false );
	SetVisibleButton( "spetsnaz", false );
}

//-----------------------------------------------------------------------------
// Purpose: magic override to allow vgui to create mouse over buttons for us, and to use our class res files, not TFC's one
//-----------------------------------------------------------------------------
Panel *CCSClassMenu::CreateControlByName(const char *controlName)
{
	if( !stricmp( "MouseOverPanelButton", controlName ) )
	{
		ClassHelperPanel *classPanel = new ClassHelperPanel( this, NULL );
		classPanel->SetVisible( false );

		int x,y,wide,tall;
		m_pPanel->GetBounds( x, y, wide, tall );
		classPanel->SetBounds( x, y, wide, tall );

		MouseOverPanelButton *newButton = new MouseOverPanelButton( this, NULL, classPanel );
		classPanel->SetAssociatedButton( newButton ); // class panel will use this to lookup the .res file

		if ( m_iFirst )
		{
			m_iFirst = 0;
			m_pFirstButton = newButton; // this first button's page is triggered when the panel is set visible
		}
		return newButton;
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the visibility of a button by name
//-----------------------------------------------------------------------------
void CCSClassMenu::SetVisibleButton(const char *textEntryName, bool state)
{
	Button *entry = dynamic_cast<Button *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetVisible(state);
	}
}


//-----------------------------------------------------------------------------
// Purpose: causes the class panel to load the resource file for its class
//-----------------------------------------------------------------------------
void CCSClassMenu::ClassHelperPanel::ApplySchemeSettings( IScheme *pScheme )
{
	Assert( GetAssociatedButton() );
	LoadControlSettings( GetClassPage( GetAssociatedButton()->GetName() ) );

	BaseClass::ApplySchemeSettings( pScheme );
}


//-----------------------------------------------------------------------------
// Purpose: returns the filename of the class file for this class
//-----------------------------------------------------------------------------
const char *CCSClassMenu::ClassHelperPanel::GetClassPage( const char *className )
{
	static char classPanel[ _MAX_PATH ];
	Q_snprintf( classPanel, sizeof( classPanel ), "classes/%s.res", className );

	if ( vgui::filesystem()->FileExists( classPanel ) )
	{
	}
	else if (vgui::filesystem()->FileExists( "classes/default.res" ) )
	{
		Q_snprintf( classPanel, sizeof( classPanel ), "classes/default.res" );
	}
	else
	{
		return NULL;
	}

	return classPanel;
}
