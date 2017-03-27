//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// cvar.c -- dynamic variable tracking
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#if !defined( CVAR_H )
#define CVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "icvar.h"
#include "filesystem.h"

class ConVar;
class ConCommandBase;

class CCvar : public ICvar
{
public:
	// Try to register cvar
	virtual void			RegisterConCommandBase ( ConCommandBase *variable );

	virtual char const*		GetCommandLineValue( char const *pVariableName );

	// Try to find the cvar pointer by name
	virtual const ConVar	*FindVar ( const char *var_name );
	
	virtual ConCommandBase	*GetCommands( void );

public:	// Internal interface
	void			Init( void );
	void			Shutdown( void );

	// Removes all cvars with the FCVAR_CLIENTDLL bit set
	void			RemoveHudCvars( void );
	void			UnlinkExternals( void );
	void			UnlinkMatSysVars();	// Unlink ones with FCVAR_MATERIAL_SYSTEM.
	
	bool			IsCommand (void);


	// Writes lines containing "set variable value" for all variables
	// with the archive flag set to true.
	void 			WriteVariables( FileHandle_t f );
	// Same, for profile flag.
	void			WriteProfileVariables( FileHandle_t f );  
	// Returns the # of cvars with the server flag set.
	int				CountVariablesWithFlags( int flags );

	// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
	// command.  Returns true if the command was a variable reference that
	// was handled. (print or change)
	char			*CompleteVariable ( char *partial, char *starting_cvar, qboolean bFirstCompletion );

	// Lists cvars to console
	void			CvarList_f( void );

	// Prints help text for cvar
	void			CvarHelp_f( void );

	// Search help and names of cvars
	void			CvarFind_f( void );

	// Revert all cvar values
	void			CvarRevert_f( void );

	// Revert all cvar values
	void			CvarDifferences_f( void );
private:
	// just like Cvar_set, but optimizes out the search
	void			SetDirect ( ConVar *var, const char *value );

	// Print cvar to file, if specified, console otherwise
	void			PrintCvar( const ConVar *var, FileHandle_t f );

	void			PrintConCommandBaseDescription( const ConCommandBase *var );

	void			PrintConCommandBaseFlags( const ConCommandBase *var );
};

// For the engine, point to full interface, dlls only get the ICvar stuff
extern CCvar *cv;


#endif // CVAR_H
