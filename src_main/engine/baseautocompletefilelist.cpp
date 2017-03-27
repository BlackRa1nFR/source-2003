//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "glquake.h"
#include "sys.h"
#include "baseautocompletefilelist.h"

//-----------------------------------------------------------------------------
// Purpose: Fills in a list of commands based on specified subdirectory and extension into the format:
//  commandname subdir/filename.ext
//  commandname subdir/filename2.ext
// Returns number of files in list for autocompletion
//-----------------------------------------------------------------------------
int CBaseAutoCompleteFileList::AutoCompletionFunc( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	char const *cmdname = m_pszCommandName;

	char *substring = (char *)partial;
	if ( Q_strstr( partial, cmdname ) )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
	}

	int current = 0;

	// Search the directory structure.
	char searchpath[MAX_QPATH];
	if ( m_pszSubDir && m_pszSubDir[0] && Q_strcasecmp( m_pszSubDir, "NULL" ) )
	{
		Q_snprintf(searchpath,sizeof(searchpath),"%s/*.%s", m_pszSubDir, m_pszExtension );
	}
	else
	{
		Q_snprintf(searchpath,sizeof(searchpath),"*.%s", m_pszExtension );
	}

	char const *findfn = Sys_FindFirst( searchpath, NULL );
	while ( findfn )
	{
		char sz[ MAX_QPATH ];
		Q_snprintf( sz, sizeof( sz ), "%s", findfn );

		bool add = false;
		// Insert into lookup
		if ( substring[0] )
		{
			if ( !Q_strncasecmp( findfn, substring, strlen( substring ) ) )
			{
				add = true;
			}
		}
		else
		{
			add = true;
		}

		if ( add )
		{
			Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s", cmdname, findfn );
			// Remove .dem
			commands[ current ][ strlen( commands[ current ] ) - 4 ] = 0;
			current++;
		}

		findfn = Sys_FindNext( NULL );

		// Too many
		if ( current >= COMMAND_COMPLETION_MAXITEMS )
			break;
	}

	Sys_FindClose();

	return current;
}
