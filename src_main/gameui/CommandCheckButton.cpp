//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "CommandCheckButton.h"
#include "EngineInterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


using namespace vgui;

CCommandCheckButton::CCommandCheckButton( Panel *parent, const char *panelName, const char *text, const char *downcmd, const char *upcmd )
 : CheckButton( parent, panelName, text )
{
	m_pszDown = downcmd ? strdup( downcmd ) : NULL;
	m_pszUp = upcmd ? strdup( upcmd ) : NULL;
}

CCommandCheckButton::~CCommandCheckButton()
{
	free( m_pszDown );
	free( m_pszUp );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CCommandCheckButton::SetSelected( bool state )
{
	BaseClass::SetSelected( state );

	if ( IsSelected() && m_pszDown )
	{
		engine->ClientCmd( m_pszDown );
		engine->ClientCmd( "\n" );
	}
	else if ( !IsSelected() && m_pszUp )
	{
		engine->ClientCmd( m_pszUp );
		engine->ClientCmd( "\n" );
	}
}
