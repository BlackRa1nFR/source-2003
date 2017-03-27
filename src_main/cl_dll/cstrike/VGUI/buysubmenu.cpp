//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "BuySubMenu.h"

#include <KeyValues.h>
#include <vgui_controls/WizardPanel.h>
#include <FileSystem.h>
#include <game_controls/IViewPort.h>

#include "BuyMouseOverPanelButton.h"
#include "cs_gamerules.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuySubMenu::CBuySubMenu(vgui::Panel *parent, const char *name) : WizardSubPanel(parent, name)
{
	_nextPanel = NULL;
	m_pFirstButton = NULL;
	
	m_pPanel = new Panel( this, "ItemInfo" );// info window about these items
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuySubMenu::~CBuySubMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: magic override to allow vgui to create mouse over buttons for us
//-----------------------------------------------------------------------------
Panel *CBuySubMenu::CreateControlByName( const char *controlName )
{
	if( !stricmp( "MouseOverPanelButton", controlName ) )
	{
		ClassHelperPanel *classPanel = new ClassHelperPanel( this, NULL );
		classPanel->SetVisible( false );

		int x,y,wide,tall;
		m_pPanel->GetBounds( x, y, wide, tall );
		classPanel->SetBounds( x, y, wide, tall );

		BuyMouseOverPanelButton *newButton = new BuyMouseOverPanelButton( this, NULL, classPanel );
		classPanel->SetAssociatedButton( newButton ); // class panel will use this to lookup the .res file

		if( !m_pFirstButton )
		{
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
// Purpose: Make the first buttons page get displayed when the menu becomes visible
//-----------------------------------------------------------------------------
void CBuySubMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );
	if( state && m_pFirstButton )
	{
		m_pFirstButton->ShowPage();
	}
	for( int i = 0; i< GetChildCount(); i++ ) // get all the buy buttons to performlayout
	{
		BuyMouseOverPanelButton *buyButton = dynamic_cast<BuyMouseOverPanelButton *>(GetChild(i));
		if ( buyButton )
		{
			buyButton->InvalidateLayout();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CBuySubMenu::OnCommand( const char *command)
{
	if ( strstr( command, ".res" ) ) // if its a .res file then its a new menu
	{ 
		char tmpname[256];
		Q_strncpy(tmpname, command, sizeof( tmpname ) );
		
		char *res = strstr( tmpname, ".res" );
		*res = '\0';

		switch( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() )
		{
		case TEAM_TERRORIST:
			strcat(tmpname, "_TER.res");
			break;
		case TEAM_CT:
		default:
			strcat(tmpname, "_CT.res");
			break;
		}

		int i;
		// check the cache
		for ( i = 0; i < m_SubMenus.Count(); i++ )
		{
			if ( !stricmp( m_SubMenus[i].filename, tmpname ) )
			{
				_nextPanel = m_SubMenus[i].panel;
				Assert( _nextPanel );
				_nextPanel->InvalidateLayout(); // force it to reset it prices
				break;
			}
		}	

		if ( i == m_SubMenus.Count() )
		{
			// not there, add a new entry
			SubMenuEntry_t newEntry;
			memset( &newEntry, 0x0, sizeof( newEntry ) );

			CBuySubMenu *newMenu = new CBuySubMenu( this );
			newMenu->LoadControlSettings( tmpname );
			_nextPanel = newMenu;
			Q_strncpy( newEntry.filename, tmpname, sizeof( newEntry.filename ) );
			newEntry.panel = newMenu;
			m_SubMenus.AddToTail( newEntry );
		}

		GetWizardPanel()->OnNextButton();
	}
	else // otherwise it must be a client command
	{
		GetWizardPanel()->Close();
		gViewPortInterface->HideBackGround();
		
		if ( Q_stricmp( command, "vguicancel" ) != 0 )
			engine->ClientCmd( command );

		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Causes the panel to delete itself when it closes
//-----------------------------------------------------------------------------
void CBuySubMenu::DeleteSubPanels()
{
	if ( _nextPanel )
	{
		_nextPanel->SetVisible( false );
		_nextPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: return the next panel to show
//-----------------------------------------------------------------------------
vgui::WizardSubPanel *CBuySubMenu::GetNextSubPanel()
{ 
	return _nextPanel;
}

//-----------------------------------------------------------------------------
// Purpose: causes the class panel to load the resource file for its class
//-----------------------------------------------------------------------------
void CBuySubMenu::ClassHelperPanel::ApplySchemeSettings( IScheme *pScheme )
{
	Assert( GetAssociatedButton() );
	
	const char *pClassName;
	if ( ( CSGameRules()->IsVIPMap() && GetAssociatedButton()->GetASRestrict() ) ||
         ( !CSGameRules()->IsBombDefuseMap() && GetAssociatedButton()->GetDEUseOnly() ) )
	{
		pClassName = "not_available";
	}
	else
	{
		pClassName = GetAssociatedButton()->GetName();
	}

	const char *pClassPage = GetClassPage( pClassName );
	if ( !pClassPage )
		Error( "CBuySubMenu::ClassHelperPanel::ApplySchemeSettings - missing class page '%s'", pClassName );		

	LoadControlSettings( pClassPage );
	BaseClass::ApplySchemeSettings( pScheme );
}


//-----------------------------------------------------------------------------
// Purpose: returns the filename of the class file for this class
//-----------------------------------------------------------------------------
const char *CBuySubMenu::ClassHelperPanel::GetClassPage( const char *className )
{
	static char classPanel[ _MAX_PATH ];
	Q_snprintf( classPanel, sizeof( classPanel ), "classes/%s.res", className);

	if ( vgui::filesystem()->FileExists( classPanel ) )
	{
	}
	else if (vgui::filesystem()->FileExists( "classes/default.res" ) )
	{
		Q_snprintf ( classPanel, sizeof( classPanel ), "classes/default.res" );
	}
	else
	{
		return NULL;
	}

	return classPanel;
}
