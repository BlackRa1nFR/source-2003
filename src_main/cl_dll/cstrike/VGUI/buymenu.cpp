//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "BuyMenu.h"

#include "BuySubMenu.h"
using namespace vgui;

#include "BuyMouseOverPanelButton.h"
#include "cs_shareddefs.h"
#include "cs_gamerules.h"


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuyMenu::CBuyMenu(vgui::Panel *parent) : WizardPanel(parent, "BuyMenu")
{
	SetScheme("ClientScheme");
	SetProportional( true );
	SetTitle( "#Cstrike_Buy_Menu", true);

	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );

	SetAutoDelete( false ); // we reuse this panel, don't let WizardPanel delete us
	
	// the base sub panels
	_mainMenu = new CBuySubMenu( this, "mainmenu" );
	_mainMenu->LoadControlSettings( "Resource/UI/MainBuyMenu.res" );
	_mainMenu->SetVisible( false );

	_equipMenu = new CBuySubMenu( this, "equipmenu" );
	switch( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() )
	{
	case TEAM_TERRORIST:
		_equipMenu->LoadControlSettings( "Resource/UI/BuyEquipment_TER.res" );
		break;
	case TEAM_CT:
	default:
		_equipMenu->LoadControlSettings( "Resource/UI/BuyEquipment_CT.res" );
		break;
	}
	_equipMenu->SetVisible( false );

	LoadControlSettings( "Resource/UI/BuyMenu.res" );
	ShowButtons( false );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuyMenu::~CBuyMenu()
{
}

void CBuyMenu::ActivateBuyMenu()
{
	_equipMenu->SetVisible( false );
	Run( _mainMenu );
}

void CBuyMenu::ActivateEquipmentMenu()
{
	_mainMenu->SetVisible( false );
	Run( _equipMenu );
}

void CBuyMenu::OnClose()
{
	BaseClass::OnClose();
	_mainMenu->DeleteSubPanels();
	_equipMenu->DeleteSubPanels();
	ResetHistory();
	SetVisible( false );
}