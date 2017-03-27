//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <game_controls/CommandMenu.h>
#include <vgui_controls/MenuItem.h>
#include "ConVar.h"

CommandMenu::CommandMenu( Panel *parent, const char *panelName, IViewPort * viewport) : Menu( parent, panelName )
{ 
	if ( !viewport )
		return;

	m_ViewPort = viewport; 
	SetVisible( false );
	m_CurrentMenu = this;

	m_MenuKeys = NULL;
}

bool CommandMenu::LoadFromFile( const char * fileName)	// load menu from KeyValues
{
	KeyValues * kv = new KeyValues(fileName);

	if  ( !kv->LoadFromFile( filesystem(), fileName, "GAME" ) )
		return false;

	bool ret = LoadFromKeyValues( kv );

	kv->deleteThis();
	return ret;
}

/*CommandMenu * CommandMenu::Factory(Panel *parent, const char *panelName, IViewPort * viewport, IFileSystem * pFileSytem) // overwrite
{
	// use old structurs
	return new CommandMenu( parent, panelName, viewport?viewport:m_ViewPort, pFileSytem?pFileSytem:m_FileSystem);
}*/

CommandMenu::~CommandMenu()
{
	ClearMenu();
}

void CommandMenu::OnMessage(KeyValues *params, VPANEL fromPanel)
{
	char type[32];

	Q_strcpy( type, params->GetString("type") );

	if ( !type[0] )
	{
		BaseClass::OnMessage( params, fromPanel );
		return;
	}

	if ( Q_strcmp( type, "command" ) == 0 )
	{
		m_ViewPort->GetClientDllInterface()->ClientCmd( params->GetString("typedata") );
	}
	else if ( Q_strcmp( type, "toggle" ) == 0 )
	{
		ConVar * convar = (ConVar*) m_ViewPort->GetClientDllInterface()->FindCVar( params->GetString("typedata") );
		if ( convar )
		{
			// toggel cvar 

			if ( convar->GetInt() )	
			{
				convar->SetValue( 0 );
			}
			else
			{
				convar->SetValue( 1 );
			}
		}
		else
		{
			Msg("CommandComboBox::OnMessage: cvar %s not found.\n", params->GetString("typedata") );
		}
	}
	else if ( Q_strcmp( type, "custom" ) == 0 )
	{
		OnCustomItem( params ); // let derived class decide what to do
	}
	else
	{
		Msg("CommandComboBox::OnMessage: unknown menu type %s.\n", type );
	}

	PostMessage( GetParent(), new KeyValues("CommandMenuClosed") );
	
	UpdateMenu();
	
	BaseClass::OnMessage( params, fromPanel );
}

void CommandMenu::StartNewSubMenu(KeyValues * params)
{
	CommandMenuItem menuitem;
	menuitem.menu = m_CurrentMenu;

	Menu * menu = new Menu( this, params->GetString("name") );	// create new menu

	menuitem.itemnr = m_CurrentMenu->AddCascadingMenuItem( params->GetString("label"), this, menu, params ); // add to current menu as item

	m_MenuItems.AddToTail( menuitem ); // add to global list

	m_pMenuStack.Push( m_CurrentMenu ); // remember current menu
	
	m_CurrentMenu = menu; // continue adding items in new menu
}

void CommandMenu::FinishSubMenu()
{
	m_CurrentMenu = m_pMenuStack.Top(); // get menu one level above
	m_pMenuStack.Pop(); // remove it from stack
}

void CommandMenu::AddMenuCommandItem(KeyValues * params)
{
	CommandMenuItem menuitem;	// create new menuItem
	menuitem.menu = m_CurrentMenu;	// save the current menu context
	menuitem.itemnr = m_CurrentMenu->AddMenuItem( params->GetString("label"), params->MakeCopy(), this, params ); // add it
	m_MenuItems.AddToTail( menuitem ); // add to global list
}

void CommandMenu::AddMenuToggleItem(KeyValues * params)
{
	CommandMenuItem menuitem;	// create new menuItem
	menuitem.menu = m_CurrentMenu;	// save the current menu context
	menuitem.itemnr = m_CurrentMenu->AddCheckableMenuItem( params->GetString("label"), params->MakeCopy(), this, params  ); // add it
	m_MenuItems.AddToTail( menuitem ); // add to global list
}

void CommandMenu::AddMenuCustomItem(KeyValues * params)
{
	CommandMenuItem menuitem;	// create new menuItem
	menuitem.menu = m_CurrentMenu;	// save the current menu context
	menuitem.itemnr = AddCustomItem( params, m_CurrentMenu );
	m_MenuItems.AddToTail( menuitem ); // add to global list
}

void CommandMenu::ClearMenu()
{
	SetVisible( false );
	m_pMenuStack.Clear();
	m_MenuItems.RemoveAll(); 
	// DeleteAllItems();
	MarkForDeletion();

	if ( m_MenuKeys )
	{
		m_MenuKeys->deleteThis();
		m_MenuKeys = NULL;
	}

}

void CommandMenu::RebuildMenu()
{
	if ( !m_MenuKeys )
		return;	

	m_pMenuStack.Clear();
	m_MenuItems.RemoveAll(); 
	DeleteAllItems();
	
	LoadFromKeyValues( m_MenuKeys ); // and reload respecting new team, mapname etc.
}

void CommandMenu::UpdateMenu()
{
	char type[16];

	int num = m_MenuItems.Count();

	for (int i=0; i < num; i++)
	{
		CommandMenuItem menuitem = m_MenuItems.Element(i);
		KeyValues * keys = menuitem.menu->GetItemUserData( menuitem.itemnr );

		if ( !keys )
			continue;

		Q_strncpy( type,  keys->GetString("type"), 16 );

		// let custom menu items update themself
				
		if ( Q_strcmp( type, "custom" ) == 0 )
		{
			// let derived class modify the menu item
			UpdateCustomItem( keys,  menuitem.menu->GetMenuItem(menuitem.itemnr) ); 
			continue; 
		}

		// update toggle buttons

		if ( Q_strcmp( type, "toggle" ) == 0 )
		{
			// set toggle state equal to cvar state
			ConVar * convar = (ConVar*) m_ViewPort->GetClientDllInterface()->FindCVar( keys->GetString("typedata") );

			if ( convar )
			{
				menuitem.menu->SetMenuItemChecked( menuitem.itemnr, convar->GetInt() !=0 );
			}
		}
	}
}

bool CommandMenu::CheckRules(KeyValues * key)
{
	char rule[16];
	char ruledata[256];
	
	Q_strncpy( rule,  key->GetString("rule"), 16 );
	
	if ( !rule[0] )
	{
		return true; // no rule defined, show item
	}

	if ( Q_strcmp( rule, "none") == 0 )
	{
		return true; // no rule defined, show item
	}

	Q_strncpy( ruledata,  key->GetString("ruledata"), 255 );

	if ( Q_strcmp( rule, "team") == 0 )
	{
		// if team is same as specified in rule, show item
		return ( Q_strcmp( m_CurrentTeam, ruledata ) == 0 );
	}
	else if ( Q_strcmp( rule, "map") == 0 )
	{
		// if team is same as specified in rule, show item
		return ( Q_strcmp( m_CurrentMap, ruledata ) == 0 );
	}
	else if ( Q_strcmp( rule, "class") == 0 )
	{
		// TODO 
		Msg("CommandMenu::CheckRules: TODO implement class.\n" );
		return false;
	}
	else
	{
		Msg("CommandMenu::CheckRules: unknown menu rule: %s.\n", rule );
		return false;
	}	
}

int CommandMenu::AddCustomItem( KeyValues * params, Menu * menu )
{
	return 0;
}

void CommandMenu::UpdateCustomItem(KeyValues * params, MenuItem * item )
{
	return;
}

void CommandMenu::OnCustomItem(KeyValues * params)
{
	return;
}

KeyValues *	CommandMenu::GetKeyValues()
{
	return m_MenuKeys;
}

bool CommandMenu::LoadFromKeyValues( KeyValues * params )
{
	char mapname[256];

	if ( !params )
		return false;

	Q_snprintf( m_CurrentTeam, 4, "%i", m_ViewPort->GetClientDllInterface()->TeamNumber() );

	Q_strncpy(mapname, gViewPortInterface->GetClientDllInterface()->GetLevelName(), 256 );

	int length = Q_strlen( mapname ); // mapname = "maps/blabla.bsp""

	if ( length > 9 )
	{
		Q_strncpy(m_CurrentMap,	mapname + 5, 255); // remove maps/
		m_CurrentMap[ length - 9 ] = 0; // remove .bsp
	}

	m_MenuKeys = params->MakeCopy(); // save keyvalues

	bool ret = LoadFromKeyValuesInternal(m_MenuKeys, 0);

	UpdateMenu();

	return ret;
}

bool CommandMenu::LoadFromKeyValuesInternal(KeyValues * key, int depth)
{
	char type[16];
	KeyValues * subkey = NULL;

	if ( depth > 100 )
	{
		Msg("CommandMenu::LoadFromKeyValueInternal: depth > 100.\n");
		return false;
	}

	// KeyValues *subkey = params->GetFirstSubKey();
		
	Q_strncpy( type,  key->GetString("type"), 16 );	 // get type

	if  (Q_strcmp(type, "custom") == 0) 
	{
		AddMenuCustomItem( key ); // do whatever custom item wants to
		return true;
	}
	
	if ( CheckRules( key ) )
	{		
		// rules OK add subkey

		if ( Q_strcmp(type, "command") == 0 )
		{
			if ( CheckRules( key ))
				AddMenuCommandItem( key );
		}
		else if ( Q_strcmp(type, "toggle") == 0 )
		{
			if ( CheckRules( key ) )
				AddMenuToggleItem( key );
		}
		else if ( Q_strcmp(type, "menu") == 0 )
		{
			// iterate through all menu items

			subkey = key->GetFirstSubKey();

			while ( subkey )
			{
				if ( subkey->GetDataType() == KeyValues::TYPE_NONE )
				{
					LoadFromKeyValuesInternal( subkey, depth+1 ); // recursive call
				}

				subkey = subkey->GetNextKey();
			}
		}
		else if ( Q_strcmp(type, "submenu") == 0 )
		{
			StartNewSubMenu( key );	// create submenu

			// iterate through all subkeys

			subkey = key->GetFirstSubKey();

			while ( subkey )
			{
				if ( subkey->GetDataType() == KeyValues::TYPE_NONE )
				{
					LoadFromKeyValuesInternal( subkey, depth+1 ); // recursive call
				}

				subkey = subkey->GetNextKey();
			}

			FinishSubMenu(); // go one level back
		}
		else
		{
			Msg("CommandMenu::LoadFromKeyValue: unknown menu type: %s.\n", type );
		}
	}

	return true;
}