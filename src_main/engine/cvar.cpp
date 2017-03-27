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

#include "quakedef.h"
#include "server.h"
// #include "sv_log.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "demo.h"
#include "vstdlib/ICommandLine.h"
#include "gameeventmanager.h"

extern ConVar	sv_cheats;

static CCvar g_Cvar;
CCvar *cv = &g_Cvar;

// Expose cvar interface to launcher, client .dll and game .dll
EXPOSE_SINGLE_INTERFACE( CCvar, ICvar, VENGINE_CVAR_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: This is used to hook engine ConVars
//-----------------------------------------------------------------------------
class EngineConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase(ConCommandBase *pVar)
	{
		// The engine's cvars are the "global" list and are already linked in
		//  so there is no need to call through to cv->RegisterVariable anymore.
		return true;
	}
};

static EngineConVarAccessor g_ConVarAccessor;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var_name - 
// Output : ConVar *CCvar::FindVar
//-----------------------------------------------------------------------------
const ConVar *CCvar::FindVar( const char *var_name )
{
	ConCommandBase const *var = ConCommandBase::FindCommand( var_name );
	if ( !var || var->IsCommand() )
	{
		return NULL;
	}
	
	return ( ConVar *)var;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			*starting_cvar - 
//			bFirstCompletion - 
// Output : char
//-----------------------------------------------------------------------------
char *CCvar::CompleteVariable( char *partial, char *starting_cvar, qboolean bFirstCompletion )
{
	const ConCommandBase		*var;
	int			len;
	char		*pPartial = NULL;
	
	len = Q_strlen(partial);
	
	if (!len)
		return NULL;

	var = ConCommandBase::GetCommands();

	// find the starting cvar command
	if ( starting_cvar && starting_cvar[0] )
	{
		for ( ; var; var = var->GetNext() )
		{
			if ( !stricmp(starting_cvar, var->GetName()) )
			{
				var = var->GetNext();
				break;
			}
		}

		if ( !var )
			return NULL;
	}
	else if ( bFirstCompletion )
	{
		// first time through, so look for an identical match
		for ( ; var; var = var->GetNext() )
		{
			if ( !Q_strncasecmp( partial, var->GetName(), len) )   // Found a partial match
			{
				if ( Q_strlen(var->GetName()) == len )       // If it's exact, ever, return it.
					return (char *)var->GetName();
		
				// Otherwise, store off the partial match as the best so far.
				if ( !pPartial )
				{
					pPartial = (char *)var->GetName();                 
				}
			}
		}

		return pPartial;
	}

// check variables
	for ( ; var ; var=var->GetNext())
	{
		if ( !Q_strncasecmp(partial,var->GetName(), len) )   // Found a partial match
		{
			return (char *)var->GetName();
		}
	}

	return pPartial;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var - 
//			*value - 
//-----------------------------------------------------------------------------
void CCvar::SetDirect( ConVar *var, const char *value )
{
	bool changed;
	const char *pszValue;
	char szNew[ 1024 ];

	pszValue = value;
	// This cvar's string must only contain printable characters.
	// Strip out any other crap.
	// We'll fill in "empty" if nothing is left
	if ( var->IsBitSet( FCVAR_PRINTABLEONLY ) )
	{
		const char *pS;
		char *pD;

		// Clear out new string
		szNew[0] = '\0';

		pS = pszValue;
		pD = szNew;

		// Step through the string, only copying back in characters that are printable
		while ( *pS )
		{
			if ( *pS < 32 || *pS > 127 )
			{
				pS++;
				continue;
			}

			*pD++ = *pS++;
		}

		// Terminate the new string
		*pD = '\0';

		// If it's empty, then insert a marker string
		if ( !strlen( szNew ) )
		{
			strcpy( szNew, "empty" );
		}

		// Point the value here.
		pszValue = szNew;
	}

	if ( var->IsBitSet( FCVAR_NEVER_AS_STRING ) )
	{
		changed = var->GetFloat() != (float)atof( pszValue );
	}
	else
	{
		changed = Q_strcmp( var->GetString(), pszValue ) ? true : false;
	}

	if ( var->IsBitSet( FCVAR_USERINFO ) )
	{
		// Are we not a server, but a client and have a change?
		if ( cls.state != ca_dedicated )
		{
			Info_SetValueForKey (cls.userinfo, var->GetName(), pszValue, MAX_INFO_STRING);
			if ( changed && ( cls.state >= ca_connected ) )
			{
				cls.netchan.message.WriteByte (clc_stringcmd);
				cls.netchan.message.WriteString(va("setinfo \"%s\" \"%s\"\n", var->GetName(), pszValue));
			}
		}
	}

	// Log changes to server variables
	if ( changed )
	{
		// Print to clients
		if ( var->IsBitSet( FCVAR_SERVER ) )
		{
			// CRelieableBroadcastRecipientFilter filter; TODO engine filter support
			KeyValues * event = new KeyValues( "server_cvar" );
			
			event->SetString("cvarname", var->GetName() );
						
			if ( var->IsBitSet( FCVAR_PROTECTED ) )
			{
				event->SetString("cvarvalue", "***PROTECTED***" );
			}
			else
			{
				event->SetString("cvarvalue", pszValue );
			}

			g_pGameEventManager->FireEvent( event, NULL );

/*			if ( var->IsBitSet( FCVAR_PROTECTED ) )
			{
				Log_Printf( "\"%s\" = \"%s\"\n", var->GetName(), "***PROTECTED***" );
				SV_BroadcastPrintf( "\"%s\" changed to \"%s\"\n",
					var->GetName(), "***PROTECTED***" );
			}
			else
			{
				Log_Printf( "\"%s\" = \"%s\"\n", var->GetName(), pszValue );
				SV_BroadcastPrintf( "\"%s\" changed to \"%s\"\n",
					var->GetName(), pszValue );
			} */
		}

		// Force changes down to clients (if running server)
		if ( var->IsBitSet( FCVAR_REPLICATED ) && sv.active )
		{
			SV_ReplicateConVarChange( var, pszValue );
		}
	}

	if ( var->IsBitSet( FCVAR_NEVER_AS_STRING ) )
	{
		var->SetValue( (float)atof( pszValue ) );
	}
	else
	{
		var->SetValue( pszValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the commands can be aliased to one another
//  Either game/client .dll shared with engine, or game and client dll shared and marked
//  FCVAR_REPLICATED
// Input  : *child - 
//			*parent - 
// Output : static bool
//-----------------------------------------------------------------------------
static bool AreConCommandsLinkable( ConCommandBase const *child, ConCommandBase const *parent )
{
	// Both parent and child must be marked replicated for this to work
	bool repchild = child->IsBitSet( FCVAR_REPLICATED );
	bool repparent = parent->IsBitSet( FCVAR_REPLICATED );

	char sz[ 512 ];

	if ( repchild && repparent )
	{
		// Never on protected vars
		if ( child->IsBitSet( FCVAR_PROTECTED ) || parent->IsBitSet( FCVAR_PROTECTED ) )
		{
			Q_snprintf( sz, sizeof( sz ), "FCVAR_REPLICATED can't also be FCVAR_PROTECTED (%s)\n", child->GetName() );
			Con_Printf( sz );
			return false;
		}

		// Only on ConVars
		if ( child->IsCommand() || parent->IsCommand() )
		{
			Q_snprintf( sz, sizeof( sz ), "FCVAR_REPLICATED not valid on ConCommands (%s)\n", child->GetName() );
			Con_Printf( sz );
			return false;
		}

		// One must be in client .dll and the other in the game .dll, or both in the engine
		if ( child->IsBitSet( FCVAR_EXTDLL ) && !parent->IsBitSet( FCVAR_CLIENTDLL ) )
		{
			Q_snprintf( sz, sizeof( sz ), "For FCVAR_REPLICATED, ConVar must be defined in client and game .dlls (%s)\n", child->GetName() );
			Con_Printf( sz );
			return false;
		}
		
		if ( child->IsBitSet( FCVAR_CLIENTDLL ) && !parent->IsBitSet( FCVAR_EXTDLL ) )
		{
			Q_snprintf( sz, sizeof( sz ), "For FCVAR_REPLICATED, ConVar must be defined in client and game .dlls (%s)\n", child->GetName() );
			Con_Printf( sz );
			return false;
		}

		// Allowable
		return true;
	}
	
	// Otherwise need both to allow linkage
	if ( repchild || repparent )
	{
		Q_snprintf( sz, sizeof( sz ), "Both ConVars must be marked FCVAR_REPLICATED for linkage to work (%s)\n", child->GetName() );
		Con_Printf( sz );
		return false;
	}

	if ( parent->IsBitSet( FCVAR_CLIENTDLL ) )
	{
		Q_snprintf( sz, sizeof( sz ), "Parent cvar in client.dll not allowed (%s)\n", child->GetName() );
		Con_Printf( sz );
		return false;
	}

	if ( parent->IsBitSet( FCVAR_EXTDLL ) )
	{
		Q_snprintf( sz, sizeof( sz ), "Parent cvar in server.dll not allowed (%s)\n", child->GetName() );
		Con_Printf( sz );
		return false;
	}

	if ( parent->IsBitSet( FCVAR_MATERIAL_SYSTEM ) )
	{
		Con_Printf( "Parent cvar in material system DLL not allowed (%s)\n", child->GetName() );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *variable - 
//-----------------------------------------------------------------------------
void CCvar::RegisterConCommandBase ( ConCommandBase *variable )
{
	// already linked
	if ( variable->GetNext() )
		return;

	// Already registered
	if ( variable->IsRegistered() )
		return;

	// If the variable is already defined, then setup the new variable as a proxy to it.
	ConCommandBase const *pOther = cv->FindVar ( variable->GetName() );
	if(pOther)
	{
		// See if it's a valid linkage
		if ( AreConCommandsLinkable( variable, pOther ) )
		{
			variable->m_pParent = pOther->m_pParent;
		}
		return;
	}

	// link the variable in
	ConCommandBase::AddToList( variable );
}

char const* CCvar::GetCommandLineValue( char const *pVariableName )
{
	int nLen = Q_strlen(pVariableName);
	char *pSearch = (char*)stackalloc( nLen + 2 );
	pSearch[0] = '+';
	memcpy( &pSearch[1], pVariableName, nLen + 1 );
	return CommandLine()->ParmValue( pSearch );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::RemoveHudCvars( void )
{
	ConCommandBase::RemoveFlaggedCommands( FCVAR_CLIENTDLL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCvar::IsCommand( void )
{
	ConVar			*v;

// check variables
	v = ( ConVar * )cv->FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		PrintConCommandBaseDescription( v );
		return true;
	}

	if ( v->IsBitSet( FCVAR_SPONLY ) )
	{
#ifndef SWDS
		// Connected to server?
		if ( ( cls.state != ca_dedicated ) && ( cls.state != ca_disconnected ) )
		{
			// Is it not a single player game?
			if ( cl.maxclients > 1 )
			{
				Con_Printf( "Can't set %s in multiplayer\n", v->GetName() );
				return true;
			}
		}
#endif
	}

	// Allow cheat commands in singleplayer, debug, or multiplayer with sv_cheats on
#ifndef DEBUG
	if ( v->IsBitSet( FCVAR_CHEAT ) )
	{
		if ( cl.maxclients > 1 && sv_cheats.GetInt() == 0 )
		{
			Con_Printf( "Can't use cheat cvars in multiplayer, unless the server has sv_cheats set to 1.\n" );
			return true;
		}
	}
#endif

	// Text invoking the command was typed into the console, decide what to do with it
	//  if this is a replicated ConVar, except don't worry about restrictions if playing a .dem file
	if ( v->IsBitSet( FCVAR_REPLICATED ) && !Demo_IsPlayingBack() )
	{
		bool sendToServer = ( // Typed at console
								cmd_source == src_command &&
								// Not HLDS
								cls.state != ca_dedicated );

		// If not running a server but possibly connected as a client, then
		//  if the message came from console, don't process the command
		if ( !sv.active && 
			!( sv.state == ss_loading ) &&
			 sendToServer &&
			 ( cls.state != ca_disconnected ) )
		{
			Con_Printf( "Can't change replicated ConVar %s from console of client, only server operator can change its value\n", v->GetName() );
			return true;
		}

		// FIXME:  Do we need a case where cmd_source == src_client?
		Assert( cmd_source != src_client );
	}

	// Note that we don't want the tokenized list, send down the entire string
	char remaining[1024];
	remaining[0] = 0;
	int i;
	int c = Cmd_Argc();
	for ( i = 1; i < c ; i++ )
	{
		Q_strncat( remaining, Cmd_Argv(i), sizeof( remaining ) );
		if ( i != c - 1 )
		{
			Q_strncat( remaining, " ", sizeof( remaining ) );
		}
	}

	// Now strip off any trailing spaces
	char *p = remaining + strlen( remaining ) - 1;
	while ( p >= remaining )
	{
		if ( *p != ' ' )
			break;

		*p-- = 0;
	}

	SetDirect( v, remaining );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *f - 
//-----------------------------------------------------------------------------
void CCvar::WriteVariables( FileHandle_t f )
{
	const ConCommandBase	*var;
	
	for (var = ConCommandBase::GetCommands() ; var ; var = var->GetNext())
	{
		if ( var->IsCommand() )
			continue;

		if (var->IsBitSet( FCVAR_ARCHIVE) )
		{
			g_pFileSystem->FPrintf (f, "%s \"%s\"\n", var->GetName(), ((ConVar *)var)->GetString() );
		}


	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var - 
//			*f - 
//-----------------------------------------------------------------------------
void CCvar::PrintCvar( const ConVar *var, FileHandle_t f )
{
	char szOutstr[256];   // Ouput string
	if (var->GetInt() == (int)var->GetFloat() )   // Clean up integers
		Q_snprintf(szOutstr, sizeof( szOutstr ), "%-15s : %8i",var->GetName(),(int)var->GetFloat());
	else
		Q_snprintf(szOutstr, sizeof( szOutstr ), "%-15s : %8.3f",var->GetName(),var->GetFloat());

	// Tack on archive setting
	if (var->IsBitSet( FCVAR_ARCHIVE ) )
		strcat(szOutstr,", a");

	// And server setting
	if (var->IsBitSet( FCVAR_SERVER) )
		strcat(szOutstr,", sv");

	// and info setting
	if (var->IsBitSet( FCVAR_USERINFO ) )
		strcat(szOutstr,", i");

	// and info setting
	if (var->IsBitSet( FCVAR_REPLICATED ) )
		strcat(szOutstr,", rep");

	// End the line
	strcat(szOutstr,"\n");

	// Print to console
	Con_Printf ("%s",szOutstr);
	if (f)
		g_pFileSystem->FPrintf(f,"%s",szOutstr);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void CCvar::CvarList_f
//-----------------------------------------------------------------------------
void CCvar::CvarList_f( void )
{
	const ConCommandBase	*var;			// Temporary Pointer to cvars
	int iCvars = 0;			// Number retrieved...
	int iArgs;				// Argument count
	char *partial = NULL;	// Partial cvar to search for...
							// E.eg
	int ipLen = 0;			// Length of the partial cvar
	qboolean bAOnly = false;
	qboolean bSOnly = false;

	FileHandle_t f = NULL;         // FilePointer for logging
	qboolean bLogging = false;
							// Are we logging?
	iArgs = Cmd_Argc();		// Get count

	if (iArgs >= 2)			// Check for "CvarList ?" or "CvarList xxx"
	{
		if (!Q_strcasecmp(Cmd_Argv(1),"?"))   // "CvarList ?"
		{
			Con_Printf(
				"CvarList           : List all cvars\n"
				"CvarList [Partial] : List cvars starting with 'Partial'\n"
				"CvarList log logfile [Partial] : Logs cvars to file gamedir/logfile.\n"
				"NOTE:  No relative paths allowed!");
			return;         
		}

		if (!Q_strcasecmp(Cmd_Argv(1),"log"))
		{
			char szTemp[256];
			Q_snprintf(szTemp, sizeof( szTemp ), "%s",Cmd_Argv(2));
			f = g_pFileSystem->Open(szTemp,"wt");
			if (f)
				bLogging = true;
			else
			{
				Con_Printf("Couldn't open [%s] for writing!\n", Cmd_Argv(2));
//
				g_pFileSystem->Close(f);
			}

			if (iArgs == 4)
			{
				partial = Cmd_Argv(3);
				ipLen = strlen(partial);
			}
		}
		else if ( !stricmp( Cmd_Argv(1), "-a" ) )
		{
			bAOnly = true;
		}
		else if ( !stricmp( Cmd_Argv(1), "-s" ) )
		{
			bSOnly = true;
		}
		else
		{
			partial = Cmd_Argv(1);   // Save pointer
			ipLen = strlen(partial); //  and length
		}
	}

	// Banner
	Con_Printf("CVar List\n--------------\n");

	// Loop through cvars...
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext())
	{
		// FIXME: Combine with cmdlist?
		if ( var->IsCommand() )
			continue;

		if ( bAOnly && !( var->IsBitSet( FCVAR_ARCHIVE ) ) )
			continue;

		if ( bSOnly && !( var->IsBitSet( FCVAR_SERVER ) ) )
			continue;

		if (partial)  // Partial string searching?
		{
			if (!Q_strncasecmp(var->GetName(),partial,ipLen))
			{
				cv->PrintCvar((ConVar *)var, f);
				iCvars++;
			}
		}
		else		  // List all cvars
		{
			cv->PrintCvar((ConVar *)var, f);
			iCvars++;
		}
	}
	
	// Show total and syntax help...
	if ((iArgs == 2) && partial && partial[0])
	{
		Con_Printf("--------------\n%3i CVars for [%s]\nCvarList ? for syntax\n",iCvars,partial);
	}
	else
	{
		Con_Printf("--------------\n%3i Total CVars\nCvarList ? for syntax\n",iCvars);
	}

	if (bLogging)
		g_pFileSystem->Close(f);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : int
//-----------------------------------------------------------------------------
int CCvar::CountVariablesWithFlags( int flags )
{
	int i = 0;
	const ConCommandBase *var;
	
	for ( var = ConCommandBase::GetCommands(); var; var = var->GetNext() )
	{
		if ( var->IsCommand() )
			continue;

		if ( var->IsBitSet( flags ) )
		{
			i++;
		}
	}

	return i;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::UnlinkExternals( void )
{
	ConCommandBase::RemoveFlaggedCommands( FCVAR_EXTDLL );
}


void CCvar::UnlinkMatSysVars()
{
	ConCommandBase::RemoveFlaggedCommands( FCVAR_MATERIAL_SYSTEM );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var - 
//-----------------------------------------------------------------------------
void CCvar::PrintConCommandBaseFlags( const ConCommandBase *var )
{
	bool any = false;
	if ( var->IsBitSet( FCVAR_EXTDLL ) )
	{
		Con_Printf( " game" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_CLIENTDLL ) )
	{
		Con_Printf( " client" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_LAUNCHER ) )
	{
		Con_Printf( " launcher" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_ARCHIVE ) )
	{
		Con_Printf( " archive" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_USERINFO ) )
	{
		Con_Printf( " user" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_SERVER ) )
	{
		Con_Printf( " server" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_SPONLY ) )
	{
		Con_Printf( " singleplayer" );
		any = true;
	}

	if ( var->IsBitSet( FCVAR_CHEAT ) )
	{
		Con_Printf( " cheat" );
		any = true;
	}

	if ( any )
	{
		Con_Printf( "\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var - 
//-----------------------------------------------------------------------------
void CCvar::PrintConCommandBaseDescription( const ConCommandBase *pVar )
{
	bool bMin, bMax;
	float fMin, fMax;
	const char *pStr;

	assert( pVar );

	if ( !pVar->IsCommand() )
	{
		ConVar *var = ( ConVar * )pVar;

		bMin = var->GetMin( fMin );
		bMax = var->GetMax( fMax );

		char const *value = NULL;
		char tempVal[ 32 ];

		if ( var->IsBitSet( FCVAR_NEVER_AS_STRING ) )
		{
			value = tempVal;

			if ( fabs( (float)var->GetInt() - var->GetFloat() ) < 0.000001 )
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%d", var->GetInt() );
			}
			else
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%f", var->GetFloat() );
			}
		}
		else
		{
			value = var->GetString();
		}

		if ( value )
		{
			Con_Printf( "\"%s\" = \"%s\"", var->GetName(), value );

			if ( stricmp( value, var->GetDefault() ) )
			{
				Con_Printf( " ( def. \"%s\" )", var->GetDefault() );
			}
		}

		if ( bMin )
		{
			Con_Printf( " min. %f", fMin );
		}
		if ( bMax )
		{
			Con_Printf( " max. %f", fMax );
		}

		Con_Printf( "\n" );
	}
	else
	{
		ConCommand *var = ( ConCommand * )pVar;

		Con_Printf( "\"%s\"\n", var->GetName() );
	}

	PrintConCommandBaseFlags( pVar );
	
	pStr = pVar->GetHelpText();
	if ( pStr && pStr[0] )
	{
		Con_Printf( " - %s\n", pStr );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::CvarFind_f( void )
{
	char *search;
	const ConCommandBase *var;

	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "Usage:  find <string>\n" );
		return;
	}

	// Get substring to find
	search = Cmd_Argv(1);
	
	// Loop through vars and print out findings
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext())
	{
		if ( !Q_stristr( var->GetName(), search ) &&
			 !Q_stristr( var->GetHelpText(), search ) )
			 continue;

		PrintConCommandBaseDescription( var );	
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::CvarHelp_f( void )
{
	char *search;
	const ConCommandBase *var;

	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "Usage:  help <cvarname>\n" );
		return;
	}

	// Get name of var to find
	search	= Cmd_Argv(1);

	// Search for it
	var		= ConCommandBase::FindCommand( search );
	if ( !var )
	{
		Con_Printf( "help:  no cvar or command named %s\n", search );
		return;
	}

	// Show info
	PrintConCommandBaseDescription( var );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::CvarDifferences_f( void )
{
	const ConCommandBase *var;

	// Loop through vars and print out findings
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext())
	{
		if ( var->IsCommand() )
			continue;

		if ( !stricmp( ((ConVar *)var)->GetDefault(), ((ConVar *)var)->GetString() ) )
			 continue;

		PrintConCommandBaseDescription( (ConVar *)var );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Revert all cvar values
//-----------------------------------------------------------------------------
void CCvar::CvarRevert_f( void )
{
	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "Usage:  revert <all | cvarname>\n" );
		return;
	}

	if ( !stricmp( Cmd_Argv(1), "all" ) )
	{
		ConVar::RevertAll();
	}
	else
	{
		ConVar *var = ( ConVar * )cv->FindVar( Cmd_Argv(1) );
		if ( !var )
		{
			Con_Printf( "Can't revert %s, no such cvar\n", Cmd_Argv(1) );
		}
		else
		{
			var->Revert();
			Con_Printf( "Reverted %s\n", Cmd_Argv(1) );
			PrintConCommandBaseDescription( var );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : ConCommandBase const
//-----------------------------------------------------------------------------
ConCommandBase *CCvar::GetCommands( void )
{
	return (ConCommandBase *)ConCommandBase::GetCommands();
}

//-----------------------------------------------------------------------------
// Purpose: Hook to command
//-----------------------------------------------------------------------------
void CvarList_f( void )
{
	cv->CvarList_f();
}

//-----------------------------------------------------------------------------
// Purpose: Print help text for cvar
//-----------------------------------------------------------------------------
void CvarHelp_f( void )
{
	cv->CvarHelp_f();
}

//-----------------------------------------------------------------------------
// Purpose: Search cvar names and help text for string/substring
//-----------------------------------------------------------------------------
void CvarFind_f( void )
{
	cv->CvarFind_f();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CvarRevert_f( void )
{
	cv->CvarRevert_f();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CvarDifferences_f( void )
{
	cv->CvarDifferences_f();
}

static ConCommand cvarlist( "cvarlist", ::CvarList_f );
static ConCommand help( "help", ::CvarHelp_f );
static ConCommand find( "find", ::CvarFind_f );
static ConCommand revert( "revert", ::CvarRevert_f );
static ConCommand differences( "differences", ::CvarDifferences_f );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::Init( void )
{
	// Register console variables..
	ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::Shutdown( void )
{
	// TODO, additional cleanup
}

