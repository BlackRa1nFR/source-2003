//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEAUTOCOMPLETEFILELIST_H
#define BASEAUTOCOMPLETEFILELIST_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

//-----------------------------------------------------------------------------
// Purpose: Simple helper class for doing autocompletion of all files in a specific directory by extension
//-----------------------------------------------------------------------------
class CBaseAutoCompleteFileList
{
public:
	CBaseAutoCompleteFileList( char const *cmdname, char const *subdir, char const *extension )
	{
		m_pszCommandName	= cmdname;
		m_pszSubDir			= subdir;
		m_pszExtension		= extension;
	}

	int AutoCompletionFunc( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );

private:
	char const	*m_pszCommandName;
	char const	*m_pszSubDir;
	char const	*m_pszExtension;
};

#define DECLARE_AUTOCOMPLETION_FUNCTION( command, subdirectory, extension )							\
	static int g_##command##_CompletionFunc( char const *partial,									\
			char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )		\
	{																								\
		static CBaseAutoCompleteFileList command##Complete( #command, subdirectory, #extension );	\
		return command##Complete.AutoCompletionFunc( partial, commands );							\
	}

#define AUTOCOMPLETION_FUNCTION( command )	\
	g_##command##_CompletionFunc

//-----------------------------------------------------------------------------
// Purpose: Utility to quicky generate a simple console command with file name autocompletion
//-----------------------------------------------------------------------------
#define CON_COMMAND_AUTOCOMPLETEFILE( name, func, description, subdirectory, extension )				\
   DECLARE_AUTOCOMPLETION_FUNCTION( name, subdirectory, extension )										\
   static ConCommand name##_command( #name, func, description, 0, AUTOCOMPLETION_FUNCTION( name ) ); 

#endif // BASEAUTOCOMPLETEFILELIST_H
