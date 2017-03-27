//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "icvar.h"


// ------------------------------------------------------------------------------------------- //
// ConVar stuff.
// ------------------------------------------------------------------------------------------- //
static ICvar *s_pCVar;

class CShaderLibConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
		// Mark for easy removal
		pCommand->AddFlags( FCVAR_MATERIAL_SYSTEM );

		// Unlink from client .dll only list
		pCommand->SetNext( 0 );

		// Link to engine's list instead
		s_pCVar->RegisterConCommandBase( pCommand );

		char const *pValue = s_pCVar->GetCommandLineValue( pCommand->GetName() );
		if( pValue && !pCommand->IsCommand() )
		{
			( ( ConVar * )pCommand )->SetValue( pValue );
		}
		return true;
	}

};

CShaderLibConVarAccessor g_ConVarAccessor;


void InitShaderLibCVars( CreateInterfaceFn cvarFactory )
{
	s_pCVar = (ICvar*)cvarFactory( VENGINE_CVAR_INTERFACE_VERSION, NULL );
	if ( s_pCVar )
	{
		ConCommandBaseMgr::OneTimeInit( &g_ConVarAccessor );
	}
}

