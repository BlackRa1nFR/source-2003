//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include <stdarg.h>
#include "exefuncs.h"
#include "launcher_int.h"

/*
==============================
VID_GetVID

==============================
*/
void CBaseLauncher::VID_GetVID( struct viddef_s *pvid )
{
}

/*
==============================
ErrorMessage

==============================
*/
void CBaseLauncher::ErrorMessage(int nLevel, const char *pszErrorMessage)
{
}

/*
==============================
GetCDKey

==============================
*/
void CBaseLauncher::GetCDKey( char *pszCDKey, int *nLength, int *bDedicated )
{
}

/*
==============================
IsValidCD

==============================
*/
int CBaseLauncher::IsValidCD(void)
{
	return 1;
}

/*
==============================
ChangeGameDirectory

==============================
*/
void CBaseLauncher::ChangeGameDirectory( const char *pszNewDirectory )
{
}

/*
==============================
GetLocalizedString

==============================
*/
char *CBaseLauncher::GetLocalizedString( unsigned int resId )
{
	return "String";
}

EXPOSE_SINGLE_INTERFACE( CBaseLauncher, IBaseLauncher, VLAUNCHER_ENGINE_API_VERSION );
