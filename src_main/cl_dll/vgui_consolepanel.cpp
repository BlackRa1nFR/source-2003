//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "iconsole.h"

class CConPanel;

class CConsole : public IConsole
{
private:
	CConPanel *conPanel;
public:
	CConsole( void )
	{
		conPanel = NULL;
	}
	
	void Create( vgui::VPANEL parent )
	{
		/*
		conPanel = new CConPanel( parent );
		*/
	}

	void Destroy( void )
	{
		/*
		if ( conPanel )
		{
			conPanel->SetParent( (vgui::Panel *)NULL );
			delete conPanel;
		}
		*/
	}
};

static CConsole g_Console;
IConsole *console = ( IConsole * )&g_Console;
