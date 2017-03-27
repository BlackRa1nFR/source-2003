//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
//=============================================================================
	   
#include "winquake.h"
#include "keys.h"
#include "console.h"
#include "screen.h"
#include "zone.h"
#include "vid.h"
#include "game_interface.h"
#include "demo.h"
#include "cdll_engine_int.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "vgui_int.h"
#include "cvar.h"
#include "cmd.h"
#include "client.h"
#include "tier0/vcrmode.h"
#include "vgui/isurface.h"
#include "vgui_controls/controls.h"


// In other C files.
extern int giActive;

/*
key up events are sent even if in console mode
*/

char	*keybindings[256];
int		keyshift[256];		// key to map to if shift held down in console
int		key_repeats[256];	// if > 1, it is autorepeating
qboolean	keydown[256];
bool	g_KeyUpToEngine[256];	// Is a keyup event pending for the engine (ie: did the engine just process
								// a keydown event for this key?)

typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},
	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"KP_HOME",			K_KP_HOME },
	{"KP_UPARROW",		K_KP_UPARROW },
	{"KP_PGUP",			K_KP_PGUP },
	{"KP_LEFTARROW",	K_KP_LEFTARROW },
	{"KP_5",			K_KP_5 },
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW },
	{"KP_END",			K_KP_END },
	{"KP_DOWNARROW",	K_KP_DOWNARROW },
	{"KP_PGDN",			K_KP_PGDN },
	{"KP_ENTER",		K_KP_ENTER },
	{"KP_INS",			K_KP_INS },
	{"KP_DEL",			K_KP_DEL },
	{"KP_SLASH",		K_KP_SLASH },
	{"KP_MINUS",		K_KP_MINUS },
	{"KP_PLUS",			K_KP_PLUS },
	{"CAPSLOCK",		K_CAPSLOCK },

	{"MWHEELUP", K_MWHEELUP },
	{"MWHEELDOWN", K_MWHEELDOWN },

	{"PAUSE", K_PAUSE},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str)
{
	keyname_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return str[0];

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!Q_strcasecmp(str,kn->name))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, char *binding)
{
	char	*newbinding;
	int		l;
			
	if (keynum == -1)
		return;

// free old bindings
	if (keybindings[keynum])
	{
		// Exactly the same, don't re-bind and fragment memory
		if ( !strcmp( keybindings[keynum], binding ) )
			return;

		delete[] keybindings[keynum];
		keybindings[keynum] = NULL;
	}
			
// allocate memory for new binding
	l = Q_strlen (binding);	
	newbinding = (char *)new char[ l+1 ];
	Q_strcpy (newbinding, binding);
	newbinding[l] = 0;
	keybindings[keynum] = newbinding;	
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}
	if ( b == K_ESCAPE )
	{
		Con_Printf("Can't unbind ESCAPE key\n" );
		return;
	}

	Key_SetBinding (b, "");
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
	{
		if ( !keybindings[i] )
			continue;
		
		// Don't ever unbind escape or console key
		if  ( i == K_ESCAPE )	
			continue;

		if ( i == '`' )
			continue;

		Key_SetBinding (i, "");
	}
}

void Key_Escape_f (void)
{
	VGui_HideGameUI();
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c != 2 && c != 3)
	{
		Con_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
			Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b] );
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}
	if ( b == K_ESCAPE )
	{
		strcpy( cmd, "cancelselect" );
	}
	else if ( b == '`' )
	{
		strcpy( cmd, "toggleconsole" );
	}
	else
	{	
	// copy the rest of the command line
		cmd[0] = 0;		// start out with a null string
		for (i=2 ; i< c ; i++)
		{
			if (i > 2)
				Q_strncat( cmd, " ", sizeof( cmd ) );
			Q_strncat (cmd, Cmd_Argv(i), sizeof( cmd ));
		}
	}
	Key_SetBinding (b, cmd);
}

/*
============
Key_CountBindings

Count number of lines of bindings we'll be writing
============
*/
int  Key_CountBindings ( void )
{
	int		i;
	int		c = 0;

	for ( i = 0 ; i < 256 ; i++ )
	{
		if (!keybindings[i])
			continue;

		if (!*keybindings[i])
			continue;

		c++;
	}

	return c;
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FileHandle_t f)
{
	int		i;

	for ( i = 0 ; i < 256 ; i++ )
	{
		if (!keybindings[i])
			continue;

		if (!*keybindings[i])
			continue;

		g_pFileSystem->FPrintf (f, "bind \"%s\" \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
	}
}

/*
============
Key_NameForBinding

Returns the keyname to which a binding string is bound.  E.g., if 
TAB is bound to +use then searching for +use will return "TAB"
============
*/
const char *Key_NameForBinding( const char *pBinding )
{
	int i;

	for (i=0 ; i<256 ; i++)
	{
		if (keybindings[i])
		{
			if (*keybindings[i])
			{
				if ( keybindings[i][0] == '+' )
				{
					if ( !Q_strncasecmp( keybindings[i]+1, (char *)pBinding, Q_strlen(pBinding) ) )
						return Key_KeynumToString(i);
				}
				else
				{
					if ( !Q_strncasecmp( keybindings[i], (char *)pBinding, Q_strlen(pBinding) ) )
						return Key_KeynumToString(i);
				}

			}
		}
	}

	return NULL;
}

const char *Key_BindingForKey( int keynum )
{
	if ( keynum < 0 || keynum > 255 )
		return NULL;

	if ( !keybindings[ keynum ] )
		return NULL;

	return keybindings[ keynum ];
}

void Key_ListBoundKeys_f( void )
{
	int i;

	for (i=0 ; i<256 ; i++)
	{
		char const *binding = Key_BindingForKey( i );
		if ( !binding || !binding[0] )
			continue;

		Con_Printf( "\"%s\" = \"%s\"\n",
			Key_KeynumToString( i ), binding );

	}
}

void Key_FindBinding_f( void )
{
	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "usage:  key_findbinding substring\n" );
		return;
	}

	char const *substring = Cmd_Argv(1);
	if ( !substring || !substring[ 0 ] )
	{
		Con_Printf( "usage:  key_findbinding substring\n" );
		return;
	}

	int i;

	for (i=0 ; i<256 ; i++)
	{
		char const *binding = Key_BindingForKey( i );
		if ( !binding || !binding[0] )
			continue;

		if ( Q_strstr( binding, substring ) )
		{
			Con_Printf( "\"%s\" = \"%s\"\n",
				Key_KeynumToString( i ), binding );
		}
	}
}

//
// register our functions
//
static ConCommand bind("bind",Key_Bind_f);
static ConCommand unbind("unbind",Key_Unbind_f);
static ConCommand unbindall("unbindall",Key_Unbindall_f);
static ConCommand escape("escape", Key_Escape_f);
static ConCommand key_listboundkeys( "key_listboundkeys", Key_ListBoundKeys_f );
static ConCommand key_findbinding( "key_findbinding", Key_FindBinding_f );

/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
	{
		keyshift[i] = i;
	}
	for (i='a' ; i<='z' ; i++)	
	{
		keyshift[i] = i - 'a' + 'A';
	}
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Key_Shutdown( void )
{
}

//////////////////////////////////////////////////
/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, bool down)
{
	char	*kb;
	char	cmd[1024];

	// Don't handle key ups if the key's already up. This can happen when we alt-tab back to the engine.
	if ( keydown[key] == false && 
		 down == false )
	{
		return;
	}

	keydown[key] = down;

	if (keydown[K_CTRL] && keydown[K_ALT] && keydown[K_DEL])
	{
		Error ("ctrl-alt-del pressed");
	}

	if ( key == K_F1 )
	{
		if ( keydown[ K_SHIFT ] )
		{
			VGui_DebugSystemKeyPressed();
			return;
		}
	}

	if (!down)
	{
		key_repeats[key] = 0;
	}

// update auto-repeat status
	if (down)
	{
		key_repeats[key]++;
		if ( key != K_BACKSPACE &&
			 key != K_PAUSE &&
			 key != K_PGUP &&
			 key != K_KP_PGUP &&
			 key != K_PGDN && 
			 key != K_KP_PGDN && 
			 key_repeats[key] > 1)
		{
			return;	// ignore most autorepeats
		}
			
		if (key >= 200 && !keybindings[key])
		{
			Con_Printf ("%s is unbound.\n", Key_KeynumToString (key) );
		}
	}

	// Does VGUI want a widdew keypwess?
	if ( !down && g_KeyUpToEngine[key] )
	{
		// (The key is being let up, and the engine got the keydown, so make sure the engine 
		// gets the matching keyup).
		g_KeyUpToEngine[key] = false;
	}
	else
	{
		if ( vgui::surface()->NeedKBInput() )
			return;
	}

	if ( down )
		g_KeyUpToEngine[key] = true;

	if ( g_ClientDLL && g_ClientDLL->IN_KeyEvent( down ? 1 : 0, key, keybindings[key] ) == 0 )
	{
		return;
	}

	// key up events only generate commands if the game key binding is
	// a button command (leading + sign).  These will occur even in console mode,
	// to keep the character from continuing an action started before a console
	// switch.  Button commands include the kenum as a parameter, so multiple
	// downs can be matched with ups
	//
	if (!down)
	{
		kb = keybindings[key];
		if (kb && kb[0] == '+')
		{
			Q_snprintf (cmd, sizeof( cmd ), "-%s %i\n", kb+1, key);
			Cbuf_AddText (cmd);
		}
		if (keyshift[key] != key)
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
			{
				Q_snprintf (cmd, sizeof( cmd ), "-%s %i\n", kb+1, key);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

	if ( key == K_ESCAPE && VGui_GameUIKeyPressed() )
	{
		// game ui handled it, so don't pass on
		return;
	}

	//
	// Send to the interpreter
	//
	kb = keybindings[key];
	if (kb)
	{
		if (kb[0] == '+')
		{	// button commands add keynum as a parm
			Q_snprintf (cmd, sizeof( cmd ), "%s %i\n", kb, key);
			Cbuf_AddText (cmd);
		}
		else
		{
			Cbuf_AddText (kb);
			Cbuf_AddText ("\n");
		}
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
	{
		keydown[i] = false;
		key_repeats[i] = 0;
		g_KeyUpToEngine[i] = false;
	}
}

