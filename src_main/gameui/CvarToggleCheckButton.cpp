//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "CvarToggleCheckButton.h"
#include "EngineInterface.h"
#include <vgui/IVGui.h>
#include <KeyValues.h>
#include "IGameUIFuncs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

CCvarToggleCheckButton::CCvarToggleCheckButton( Panel *parent, const char *panelName, const char *text, 
	char const *cvarname )
 : CheckButton( parent, panelName, text )
{
	m_pszCvarName = cvarname ? strdup( cvarname ) : NULL;

	if (m_pszCvarName)
	{
		Reset();
	}
	AddActionSignalTarget( this );
}

CCvarToggleCheckButton::~CCvarToggleCheckButton()
{
	free( m_pszCvarName );
}

void CCvarToggleCheckButton::Paint()
{
	if ( !m_pszCvarName || !m_pszCvarName[ 0 ] ) 
	{
		BaseClass::Paint();
		return;
	}

	// Look up current value
//	bool value = engine->pfnGetCvarFloat( m_pszCvarName ) > 0.0f ? true : false;
	ConVar const *var = cvar->FindVar( m_pszCvarName );
	if ( !var )
		return;
	bool value = var->GetBool();
	
	if (value != m_bStartValue)
	//if ( value != IsSelected() )
	{
		SetSelected( value );
		m_bStartValue = value;
	}
	BaseClass::Paint();
}

void CCvarToggleCheckButton::ApplyChanges()
{
	m_bStartValue = IsSelected();
//	engine->Cvar_SetValue( m_pszCvarName, m_bStartValue ? 1.0f : 0.0f );
	ConVar *var = (ConVar *)cvar->FindVar( m_pszCvarName );
	if ( !var )
		return;
	var->SetValue(m_bStartValue);

}

void CCvarToggleCheckButton::Reset()
{
//	m_bStartValue = engine->pfnGetCvarFloat( m_pszCvarName ) > 0.0f ? true : false;
	ConVar const *var = cvar->FindVar( m_pszCvarName );
	if ( !var )
		return;
	m_bStartValue = var->GetBool();
	SetSelected(m_bStartValue);
}

bool CCvarToggleCheckButton::HasBeenModified()
{
	return IsSelected() != m_bStartValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CCvarToggleCheckButton::SetSelected( bool state )
{
	BaseClass::SetSelected( state );

	if ( !m_pszCvarName || !m_pszCvarName[ 0 ] ) 
		return;
/*
	// Look up current value
	bool value = state;

	engine->Cvar_SetValue( m_pszCvarName, value ? 1.0f : 0.0f );*/
}


void CCvarToggleCheckButton::OnButtonChecked()
{
	if (HasBeenModified())
	{
		PostActionSignal(new KeyValues("ControlModified"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t CCvarToggleCheckButton::m_MessageMap[] =
{
	MAP_MESSAGE( CCvarToggleCheckButton, "CheckButtonChecked", OnButtonChecked),
};

IMPLEMENT_PANELMAP( CCvarToggleCheckButton, BaseClass );