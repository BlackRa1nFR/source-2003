//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           ViewerSettings.cpp
// last modified:  May 29 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#include "ViewerSettings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"


ViewerSettings g_viewerSettings;



void
InitViewerSettings (void)
{
	memset (&g_viewerSettings, 0, sizeof (ViewerSettings));
	g_viewerSettings.rot[0] = -90.0f;
	g_viewerSettings.trans[3] = 50.0f;
}


bool RegReadVector( HKEY hKey, const char *szSubKey, float value[] )
{
	LONG lResult;           // Registry function result code
	char szBuff[128];       // Temp. buffer
	DWORD dwType;           // Type of key
	DWORD dwSize;           // Size of element data

	dwSize = sizeof( szBuff );

	lResult = RegQueryValueEx(
		hKey,		// handle to key
		szSubKey,	// value name
		0,			// reserved
		&dwType,    // type buffer
		(LPBYTE)szBuff,    // data buffer
		&dwSize );  // size of data buffer

	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	if (sscanf( szBuff, "(%f %f %f)", &value[0], &value[1], &value[2] ) != 3)
		return false;

	return true;
}


bool RegWriteVector( HKEY hKey, const char *szSubKey, float value[] )
{
	LONG lResult;           // Registry function result code
	char szBuff[128];       // Temp. buffer
	DWORD dwSize;           // Size of element data

	sprintf( szBuff, "(%f %f %f)", value[0], value[1], value[2] );
	dwSize = strlen( szBuff );

	lResult = RegSetValueEx(
		hKey,		// handle to key
		szSubKey,	// value name
		0,			// reserved
		REG_SZ,		// type buffer
		(LPBYTE)szBuff,    // data buffer
		dwSize );  // size of data buffer

	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	return true;
}





bool RegReadInt( HKEY hKey, const char *szSubKey, int *value )
{
	LONG lResult;           // Registry function result code
	DWORD dwType;           // Type of key
	DWORD dwSize;           // Size of element data

	dwSize = sizeof( DWORD );

	lResult = RegQueryValueEx(
		hKey,		// handle to key
		szSubKey,	// value name
		0,			// reserved
		&dwType,    // type buffer
		(LPBYTE)value,    // data buffer
		&dwSize );  // size of data buffer

	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	if (dwType != REG_DWORD)
		return false;

	return true;
}


bool RegWriteInt( HKEY hKey, const char *szSubKey, int value )
{
	LONG lResult;           // Registry function result code
	DWORD dwSize;           // Size of element data

	dwSize = sizeof( DWORD );

	lResult = RegSetValueEx(
		hKey,		// handle to key
		szSubKey,	// value name
		0,			// reserved
		REG_DWORD,		// type buffer
		(LPBYTE)&value,    // data buffer
		dwSize );  // size of data buffer

	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	return true;
}













LONG RegViewerSettingsKey( const char *filename, PHKEY phKey, LPDWORD lpdwDisposition )
{
	if (strlen( filename ) == 0)
		return ERROR_KEY_DELETED;
	
	char szFileName[1024];

	strcpy( szFileName, filename );

	// strip out bogus characters
	for (char *cp = szFileName; *cp; cp++)
	{
		if (*cp == '\\' || *cp == '/' || *cp == ':')
			*cp = '.';
	}

	char szModelKey[1024];

	sprintf( szModelKey, "Software\\Valve\\vmtedit\\%s", szFileName );

	return RegCreateKeyEx(
		HKEY_CURRENT_USER,	// handle of open key 
		szModelKey,			// address of name of subkey to open 
		0,					// DWORD ulOptions,	  // reserved 
		NULL,			// Type of value
		REG_OPTION_NON_VOLATILE, // Store permanently in reg.
		KEY_ALL_ACCESS,		// REGSAM samDesired, // security access mask 
		NULL,
		phKey,				// Key we are creating
		lpdwDisposition);    // Type of creation
}




bool LoadViewerSettings (const char *filename)
{
	LONG lResult;           // Registry function result code
	DWORD dwDisposition;    // Type of key opening event

	HKEY hModelKey;

	lResult = RegViewerSettingsKey( filename, &hModelKey, &dwDisposition);
	
	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	// First time, just set to Valve default
	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		return false;
	}

	RegReadVector( hModelKey, "Rot", g_viewerSettings.rot );
	RegReadVector( hModelKey, "Trans", g_viewerSettings.trans );

	return true;
}



bool SaveViewerSettings (const char *filename)
{
	LONG lResult;           // Registry function result code
	DWORD dwDisposition;    // Type of key opening event

	HKEY hModelKey;
	lResult = RegViewerSettingsKey( filename, &hModelKey, &dwDisposition);

	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	RegWriteVector( hModelKey, "Rot", g_viewerSettings.rot );
	RegWriteVector( hModelKey, "Trans", g_viewerSettings.trans );

	return true;
}
