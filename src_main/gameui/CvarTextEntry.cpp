//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "CvarTextEntry.h"
#include "EngineInterface.h"
#include <vgui/IVGui.h>
#include "IGameUIFuncs.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static const int MAX_CVAR_TEXT = 64;

CCvarTextEntry::CCvarTextEntry( Panel *parent, const char *panelName, char const *cvarname )
 : TextEntry( parent, panelName)
{
	m_pszCvarName = cvarname ? strdup( cvarname ) : NULL;
	m_pszStartValue[0] = 0;

	if ( m_pszCvarName )
	{
		Reset();
	}

	AddActionSignalTarget( this );
}

CCvarTextEntry::~CCvarTextEntry()
{
	if ( m_pszCvarName )
	{
		free( m_pszCvarName );
	}
}

void CCvarTextEntry::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	if (GetMaximumCharCount() < 0 || GetMaximumCharCount() > MAX_CVAR_TEXT)
	{
		SetMaximumCharCount(MAX_CVAR_TEXT - 1);
	}
}

void CCvarTextEntry::ApplyChanges()
{
	if ( !m_pszCvarName )
		return;

	char szText[ MAX_CVAR_TEXT ];
	GetText( szText, MAX_CVAR_TEXT );

	if ( !szText[ 0 ] )
		return;

	char szCommand[ 256 ];
	sprintf( szCommand, "%s \"%s\"\n", m_pszCvarName, szText );
	engine->ClientCmd( szCommand );
	strcpy( m_pszStartValue, szText );
}

void CCvarTextEntry::Reset()
{
//	char *value = engine->pfnGetCvarString( m_pszCvarName );
	ConVar const *var = cvar->FindVar( m_pszCvarName );
	if ( !var )
		return;
	const char *value = var->GetString();
	if ( value && value[ 0 ] )
	{
		SetText( value );
		strcpy( m_pszStartValue, value );
	}
}

bool CCvarTextEntry::HasBeenModified()
{
	char szText[ MAX_CVAR_TEXT ];
	GetText( szText, MAX_CVAR_TEXT );

	return stricmp( szText, m_pszStartValue );
}


void CCvarTextEntry::OnTextChanged()
{
	if ( !m_pszCvarName )
		return;
	
	if (HasBeenModified())
	{
		PostActionSignal(new KeyValues("ControlModified"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t CCvarTextEntry::m_MessageMap[] =
{
	MAP_MESSAGE( CCvarTextEntry, "TextChanged", OnTextChanged ),	// custom message
};

IMPLEMENT_PANELMAP( CCvarTextEntry, BaseClass );