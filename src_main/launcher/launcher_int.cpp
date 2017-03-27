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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "stdwin.h"
#include <stdio.h>
#include "exefuncs.h"
#include "launcher_int.h"
#include "ivideomode.h"
#include "cd.h"
#include "exefuncs.h"
#include "engine_launcher_api.h"
#include "procinfo.h"
#include "IFileSystem.h"

/*
==============================
VID_GetVID

==============================
*/
void CBaseLauncher::VID_GetVID( struct viddef_s *pvid )
{
	videomode->GetVid( pvid );
}

/*
==============================
ErrorMessage

==============================
*/
void CBaseLauncher::ErrorMessage(int nLevel, const char *pszErrorMessage)
{
	videomode->Shutdown();
	::MessageBox( NULL, pszErrorMessage, "Fatal Engine Error", MB_OK );
}

/*
==============================
GetCDKey

==============================
*/
void CBaseLauncher::GetCDKey( char *pszCDKey, int *nLength, int *bDedicated )
{
	//::UTIL_GetCDKey( pszCDKey, nLength, bDedicated );
}

/*
==============================
IsValidCD

==============================
*/
int CBaseLauncher::IsValidCD(void)
{
	//return ::Launcher_IsValidCD();
	return 1;
}

/*
==============================
ChangeGameDirectory

==============================
*/
void CBaseLauncher::ChangeGameDirectory( const char *pszNewDirectory )
{
	// Reset the game directory
	FileSystem_SetGameDirectory( pszNewDirectory );

	//::Launcher_ChangeGameDirectory( pszNewDirectory );
}

/*
==============================
GetLocalizedString

==============================
*/
char *CBaseLauncher::GetLocalizedString( unsigned int resId )
{
	//return ::Launcher_GetLocalizedString( resId );
	return "dummy";
}

EXPOSE_SINGLE_INTERFACE( CBaseLauncher, IBaseLauncher, VLAUNCHER_ENGINE_API_VERSION );
