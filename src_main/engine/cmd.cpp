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
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "zone.h"
#include "measure_section.h"
#include "tier0/vcrmode.h"
#include "demo.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "eiface.h"
#include "server.h"
#include "sys.h"
#include "baseautocompletefilelist.h"
#include "vstdlib/ICommandLine.h"
#include "utlbuffer.h"
#include "tier0/memalloc.h"


#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char				name[ MAX_ALIAS_NAME ];
	char				*value;
} cmdalias_t;

static cmdalias_t	*cmd_alias = NULL;
static qboolean		cmd_wait;
static sizebuf_t	cmd_text;
static char			cmd_buffer[ 8192 ];

extern ConVar		sv_cheats;

static FileAssociationInfo g_FileAssociations[] =
{
	{ ".dem", "playdemo" },
	{ ".sav", "load" },
	{ ".bsp", "map" },
};

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
}


// Just translates BindToggle <key> <cvar> into: bind <key> "increment var <cvar> 0 1 1"
void Cmd_BindToggle_f()
{
	if( Cmd_Argc() <= 2 )
	{
		Con_Printf( "BindToggle <key> <cvar>: invalid syntax specified\n" );
		return;
	}

	char newCmd[2048];
	Q_snprintf( newCmd, sizeof(newCmd), "bind %s \"incrementvar %s 0 1 1\"\n", Cmd_Argv(1), Cmd_Argv(2) );

	Cbuf_InsertText( newCmd );
}


/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	memset( &cmd_text, 0, sizeof( cmd_text ) );

	cmd_text.data		= (unsigned char *)cmd_buffer;
	cmd_text.maxsize	= sizeof( cmd_buffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Cbuf_Shutdown( void )
{
	// TODO:  Cleanup
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText( char *text )
{
	int		l;
	
	l = Q_strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Con_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	
	SZ_Write (&cmd_text, text, Q_strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
HACKHACK -- copy of this below
============
*/
void Cbuf_InsertText( char *text )
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if ( templen )
	{
		temp = (char *)new char[ templen ];
		Q_memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = NULL;	// shut up compiler
		
// add the entire text of the file
	Cbuf_AddText (text);
	
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		delete[] temp;
	}
}


// HACKHACK - Copy of the above function
// Same as above, but inserts newlines around the text
void Cbuf_InsertTextLines (char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = (char *)new char[templen];
		Q_memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = NULL;	// shut up compiler
		
// add the entire text of the file
	Cbuf_AddText ("\n");
	Cbuf_AddText (text);
	Cbuf_AddText ("\n");
	
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		delete[] temp;
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;
	
	while (cmd_text.cursize)
	{
	// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}
			
				
		memcpy (line, text, i);
		line[i] = 0;
		
	// delete the text from the command buffer and move remaining commands down
	// this is necessary because commands (exec, alias) can insert data at the
	// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			Q_memcpy (text, text+i, cmd_text.cursize);
		}

	// execute the command line
		Cmd_ExecuteString (line, src_command);

#ifndef SWDS
		demo->RecordCommand( line );
#endif
		
		if (cmd_wait)
		{	
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *param - 
// Output : static char const
//-----------------------------------------------------------------------------
static char const *Cmd_TranslateFileAssociation( char const *fullcmd, char const *param )
{
	static char sz[ 512 ];
	Q_strncpy( sz, param, sizeof( sz ) );
	char temp[ 512 ];
	Q_strncpy( temp, param, sizeof( temp ) );

	COM_FixSlashes( temp );

	Q_strlower( temp );

	int c = ARRAYSIZE( g_FileAssociations );
	for ( int i = 0; i < c; i++ )
	{
		FileAssociationInfo& info = g_FileAssociations[ i ];

		if ( !Q_stristr( temp, info.extension ) )
		{
			continue;
		}

		// User already specified this command, don't get in the way
		if ( Q_stristr( fullcmd, va( "+%s", info.command_to_issue ) ) )
		{
			continue;
		}

		Q_strncpy( sz, temp, sizeof( sz ) );
		COM_FileBase( sz, temp );

		Q_snprintf( sz, sizeof( sz ), "+%s %s", info.command_to_issue, temp );
	
		break;
	}

	return sz;
}

//-----------------------------------------------------------------------------
// Purpose: Adds command line parameters as script statements
// Commands lead with a +, and continue until a - or another +
// Also automatically converts .dem, .bsp, and .sav files to +playdemo etc command line options
// hl2 +cmd amlev1
// hl2 -nosound +cmd amlev1
// Output : void Cmd_StuffCmds_f
//-----------------------------------------------------------------------------
void Cmd_StuffCmds_f (void)
{
	int		i, j;
	char	*build, c;
		
	if (Cmd_Argc () != 1)
	{
		Con_Printf ("stuffcmds : execute command line parameters\n");
		return;
	}

	// Build up a command line
	CUtlBuffer buf( 0, 0, true );

	// build the combined string to parse from
	int numparams = CommandLine()->ParmCount();
	if ( numparams <= 1 )
	{
		return;
	}

	for ( i = 1 ; i < numparams ; i++ )
	{
		if ( !CommandLine()->GetParm( i ) )
			continue;		

		char const *translated = Cmd_TranslateFileAssociation( CommandLine()->GetCmdLine(), CommandLine()->GetParm( i ) );
		buf.PutString( translated );
		if ( i != numparams - 1 )
		{
			buf.PutString( " " );
		}
	}
	
	int s = buf.TellPut();

// pull out the commands
	build = ( char * )new char [ s + 1 ];
	build[0] = 0;
	
	char *text = (char *)buf.Base();

	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
			Q_strcat (build, text+i);
			Q_strcat (build, "\n");
			text[j] = c;
			i = j - 1;
		}
	}
	
	if (build[0])
	{
		Cbuf_InsertText (build);
	}

	delete[] build;
}

void Cmd_MemDump_f(void)
{
	g_pMemAlloc->DumpStats();
}

/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;
	int		mark;
	char *s;
	char	fileName[MAX_OSPATH];

	if (Cmd_Argc () < 2)
	{
		Con_Printf ("exec <filename> [path id]: execute a script file\n");
		return;
	}

	s = Cmd_Argv(1);
	if (!s)
		return;

	// Optional path ID. * means no path ID.
	const char *pPathID = NULL;
	if ( Cmd_Argc() >= 3 )
		pPathID = Cmd_Argv(2);
	else
		pPathID = "*";

	mark = Hunk_LowMark ();

	// Has an extension already?
	Q_snprintf( fileName, sizeof( fileName ), "//%s/cfg/%s", pPathID, s );

	// Ensure it has an extension
	COM_DefaultExtension( fileName, ".cfg", sizeof( fileName ) );

	f = (char *)COM_LoadHunkFile (fileName);
	if (!f)
	{
		if ( !strstr( s, "autoexec.cfg" ) && !strstr( s, "joystick.cfg" ) && !strstr( s, "game.cfg" ) )
			Con_Printf ("couldn't exec %s\n",s);
		return;
	}
	g_pVCR->Hook_Cmd_Exec(&f);

	Con_DPrintf ("execing %s\n",s);
	
	Cbuf_InsertTextLines( f );
	Hunk_FreeToLowMark (mark);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Con_Printf ("%s ",Cmd_Argv(i));
	Con_Printf ("\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
// Output : char *CopyString
//-----------------------------------------------------------------------------
char *CopyString (char *in)
{
	char	*out;
	
	out = (char *)new char[ Q_strlen(in)+1 ];
	strcpy (out, in);
	return out;
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Con_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != c)
			strcat (cmd, " ");
	}
	strcat (cmd, "\n");

	// if the alias allready exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			if ( !strcmp( a->value, cmd ) )		// Re-alias the same thing
				return;

			delete[] a->value;
			break;
		}
	}

	if (!a)
	{
		a = (cmdalias_t *)new cmdalias_t;
		a->next = cmd_alias;
		cmd_alias = a;
	}
	strcpy (a->name, s);	

	a->value = CopyString (cmd);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

#define	MAX_ARGS		80

static	int			cmd_argc;
static	char		*cmd_argv[MAX_ARGS];
static	char		*cmd_null_string = "";
static	char		*cmd_args = NULL;

cmd_source_t	cmd_source;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Cmd_Init
//-----------------------------------------------------------------------------

void Cmd_ForwardToServerProxy()
{
	Cmd_ForwardToServer();
}

static ConCommand stuffcmds("stuffcmds",Cmd_StuffCmds_f);

CON_COMMAND_AUTOCOMPLETEFILE( exec, Cmd_Exec_f, "Execute script file.", "cfg", cfg );

//static ConCommand exec("exec",Cmd_Exec_f);
static ConCommand echo("echo",Cmd_Echo_f);
static ConCommand alias("alias",Cmd_Alias_f);
static ConCommand cmd("cmd", Cmd_ForwardToServerProxy);
static ConCommand wait("wait", Cmd_Wait_f);
static ConCommand BindToggle( "BindToggle", Cmd_BindToggle_f );
static ConCommand mem_dump( "mem_dump", Cmd_MemDump_f );


void Cmd_Init( void )
{
	Sys_CreateFileAssociations( ARRAYSIZE( g_FileAssociations ), g_FileAssociations );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Cmd_Shutdown( void )
{
	// TODO, cleanup
	while ( cmd_alias )
	{
		cmdalias_t *next = cmd_alias->next;
		delete cmd_alias;
		cmd_alias = next;
	}
}

/*
============
Cmd_Argc
============
*/
int	Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char *Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= (unsigned)cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

/*
============
Cmd_Args
============
*/
char *Cmd_Args (void)
{
	return cmd_args;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString (char *text)
{
	int		i;
	
// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
	{
		delete[] cmd_argv[i];
	}
		
	cmd_argc = 0;
	cmd_args = NULL;
	
	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;
	
		if (cmd_argc == 1)
			 cmd_args = text;
			
		text = COM_Parse (text);
		if (!text)
			return;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = (char *)new char[ Q_strlen(com_token)+1 ];
			Q_strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}
	
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text, cmd_source_t src)
{	
	cmdalias_t		*a;

	cmd_source = src;
	Cmd_TokenizeString (text);
			
// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0], a->name))
		{
			Cbuf_InsertText (a->value);
			return;
		}
	}
	
// check ConCommands
	ConCommandBase const *pCommand = ConCommandBase::FindCommand( cmd_argv[ 0 ] );
	if ( pCommand && pCommand->IsCommand() )
	{
		bool isServerCommand = ( pCommand->IsBitSet( FCVAR_EXTDLL ) && 
								// Typed at console
								cmd_source == src_command &&
								// Not HLDS
								cls.state != ca_dedicated );

		// Hook to allow game .dll to figure out who type the message on a listen server
		if ( serverGameClients )
		{
			// We're actually the server, so set it up locally
			if ( sv.active )
			{
				serverGameClients->SetCommandClient( -1 );
						
#ifndef SWDS
				// Special processing for listen server player
				if ( isServerCommand )
				{
					serverGameClients->SetCommandClient( cl.playernum );
				}
#endif
			}
			// We're not the server, but we've been a listen server (game .dll loaded)
			//  forward this command tot he server instead of running it locally if we're still
			//  connected
			// Otherwise, things like "say" won't work unless you quit and restart
			else if ( isServerCommand )
			{
				if ( cls.state == ca_active || 
					cls.state == ca_connected )
				{
					Cmd_ForwardToServer();
					return;
				}
			}
		}

		// Allow cheat commands in singleplayer, debug, or multiplayer with sv_cheats on
#ifndef _DEBUG
		if ( pCommand->IsBitSet( FCVAR_CHEAT ) )
		{
			if ( cl.maxclients > 1 && sv_cheats.GetInt() == 0 )
			{
				Msg( "Can't use cheat commands in multiplayer, unless the server has sv_cheats set to 1.\n" );
				return;
			}
		}
#endif

		(( ConCommand * )pCommand )->Dispatch();
		return;
	}

	// check cvars
	if ( cv->IsCommand() )
	{
		return;
	}

	// forward the command line to the server, so the entity DLL can parse it
	if ( cmd_source == src_command )
	{
		if ( cls.state == ca_active || 
			cls.state == ca_connected )
		{
			Cmd_ForwardToServer();
			return;
		}
	}
	
	Msg("Unknown command \"%s\"\n", Cmd_Argv(0));
}


/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (bool bReliable)
{
	char str[1024];

	bf_write *pBuf = bReliable ? &cls.netchan.message : &cls.datagram;

	if (cls.state != ca_connected &&
		cls.state != ca_active)
	{
		if ( stricmp( Cmd_Argv(0), "setinfo" ) )
		{
			Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		}
		return;
	}
	
#ifndef SWDS
	if ( Demo_IsPlayingBack() )
		return;		// not really connected
#endif

	// YWB 6/3/98 Don't forward if this is a dedicated server?
	if (isDedicated)
		return;

	pBuf->WriteByte (clc_stringcmd);
	
	str[0] = 0;
	if (Q_strcasecmp(Cmd_Argv(0), "cmd") != 0)
	{
		strcat(str, Cmd_Argv(0));
		strcat(str, " ");
	}
	
	if (Cmd_Argc() > 1)
		strcat(str, Cmd_Args());
	else
		strcat(str, "\n");

	pBuf->WriteString(str);
}


/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/

int Cmd_CheckParm (char *parm)
{
	int i;
	
	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! Q_strcasecmp (parm, Cmd_Argv (i)))
			return i;
			
	return 0;
}
