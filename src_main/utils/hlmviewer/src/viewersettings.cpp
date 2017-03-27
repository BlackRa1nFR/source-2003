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



void InitViewerSettings ( const char *subkey )
{
	memset (&g_viewerSettings, 0, sizeof (ViewerSettings));
	strcpy( g_viewerSettings.registrysubkey, subkey );

	g_viewerSettings.rot[0] = -90.0f;
	g_viewerSettings.trans[2] = 50.0f;
	g_viewerSettings.renderMode = RM_TEXTURED;
	g_viewerSettings.fov = 65.0f;

	g_viewerSettings.gColor[0] = 0.85f;
	g_viewerSettings.gColor[1] = 0.85f;
	g_viewerSettings.gColor[2] = 0.69f;

	g_viewerSettings.lColor[0] = 1.0f;
	g_viewerSettings.lColor[1] = 1.0f;
	g_viewerSettings.lColor[2] = 1.0f;

	g_viewerSettings.aColor[0] = 0.3f;
	g_viewerSettings.aColor[1] = 0.3f;
	g_viewerSettings.aColor[2] = 0.3f;

	g_viewerSettings.lightrot[0] = 0.0f;
	g_viewerSettings.lightrot[1] = 180.0f;
	g_viewerSettings.lightrot[2] = 0.0f;

	g_viewerSettings.speedScale = 1.0f;

	g_viewerSettings.application_mode = 0;

	g_viewerSettings.thumbnailsize = 128;
	g_viewerSettings.highlightHitbox = -1;
	g_viewerSettings.highlightBone = -1;

	g_viewerSettings.speechapiindex = 0;

	g_viewerSettings.solveHeadTurn = 0;
	g_viewerSettings.flHeadTurn = 1.0;
	g_viewerSettings.vecHeadTarget = Vector( 0, 0, 0 );
	g_viewerSettings.vecEyeTarget = Vector( 0, 0, 0 );
}


bool RegReadVector( HKEY hKey, const char *szSubKey, Vector& value )
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

bool RegReadQAngle( HKEY hKey, const char *szSubKey, QAngle& value )
{
	Vector tmp;
	if (RegReadVector( hKey, szSubKey, tmp ))
	{
		value.Init( tmp.x, tmp.y, tmp.z );
		return true;
	}
	return false;
}


bool RegReadColor( HKEY hKey, const char *szSubKey, float value[4] )
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

	if (sscanf( szBuff, "(%f %f %f %f)", &value[0], &value[1], &value[2], &value[4] ) != 4)
		return false;

	return true;
}


bool RegWriteVector( HKEY hKey, const char *szSubKey, Vector& value )
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

bool RegWriteQAngle( HKEY hKey, const char *szSubKey, QAngle& value )
{
	Vector tmp;
	tmp.Init( value.x, value.y, value.z );
	return RegWriteVector( hKey, szSubKey, tmp );
}


bool RegWriteColor( HKEY hKey, const char *szSubKey, float value[4] )
{
	LONG lResult;           // Registry function result code
	char szBuff[128];       // Temp. buffer
	DWORD dwSize;           // Size of element data

	sprintf( szBuff, "(%f %f %f %f)", value[0], value[1], value[2], value[3] );
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




bool RegReadFloat( HKEY hKey, const char *szSubKey, float *value )
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

	*value = atof( szBuff );

	return true;
}


bool RegWriteFloat( HKEY hKey, const char *szSubKey, float value )
{
	LONG lResult;           // Registry function result code
	char szBuff[128];       // Temp. buffer
	DWORD dwSize;           // Size of element data

	sprintf( szBuff, "%f", value );
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

	sprintf( szModelKey, "Software\\Valve\\%s\\%s", g_viewerSettings.registrysubkey, szFileName );

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

bool LoadViewerSettingsInt( char const *keyname, int *value )
{
	LONG lResult;           // Registry function result code
	DWORD dwDisposition;    // Type of key opening event

	HKEY hModelKey;

	lResult = RegViewerSettingsKey( "hlfaceposer", &hModelKey, &dwDisposition);
	
	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	// First time, just set to Valve default
	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		return false;
	}

	*value = 0;
	RegReadInt( hModelKey, keyname, value );
	return true;
}

bool SaveViewerSettingsInt ( const char *keyname, int value )
{
	LONG lResult;           // Registry function result code
	DWORD dwDisposition;    // Type of key opening event

	HKEY hModelKey;
	lResult = RegViewerSettingsKey( "hlfaceposer", &hModelKey, &dwDisposition);

	if (lResult != ERROR_SUCCESS)  // Failure
		return false;

	RegWriteInt( hModelKey, keyname, value );
	return true;
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

	RegReadQAngle( hModelKey, "Rot", g_viewerSettings.rot );
	RegReadVector( hModelKey, "Trans", g_viewerSettings.trans );
	RegReadColor( hModelKey, "bgColor", g_viewerSettings.bgColor );
	RegReadColor( hModelKey, "gColor", g_viewerSettings.gColor );
	RegReadColor( hModelKey, "lColor", g_viewerSettings.lColor );
	RegReadColor( hModelKey, "aColor", g_viewerSettings.aColor );
	RegReadQAngle( hModelKey, "lightrot", g_viewerSettings.lightrot );
	RegReadInt( hModelKey, "sequence", &g_viewerSettings.sequence );
	RegReadInt( hModelKey, "overlaySequence0", &g_viewerSettings.overlaySequence[0] );
	RegReadFloat( hModelKey, "overlayWeight0", &g_viewerSettings.overlayWeight[0] );
	RegReadInt( hModelKey, "overlaySequence1", &g_viewerSettings.overlaySequence[1] );
	RegReadFloat( hModelKey, "overlayWeight1", &g_viewerSettings.overlayWeight[1] );
	RegReadInt( hModelKey, "overlaySequence2", &g_viewerSettings.overlaySequence[2] );
	RegReadFloat( hModelKey, "overlayWeight2", &g_viewerSettings.overlayWeight[2] );
	RegReadInt( hModelKey, "overlaySequence3", &g_viewerSettings.overlaySequence[3] );
	RegReadFloat( hModelKey, "overlayWeight3", &g_viewerSettings.overlayWeight[3] );
	RegReadInt( hModelKey, "renderwidth", &g_viewerSettings.width );
	RegReadInt( hModelKey, "renderheight", &g_viewerSettings.height );
	RegReadFloat( hModelKey, "speedscale", &g_viewerSettings.speedScale );
	if (g_viewerSettings.speedScale > 1.0)
		g_viewerSettings.speedScale = 1.0;
	g_viewerSettings.width = max( 200, g_viewerSettings.width );
	g_viewerSettings.height = max( 150, g_viewerSettings.height );

	RegReadInt( hModelKey, "viewermode", &g_viewerSettings.application_mode );
	RegReadInt( hModelKey, "thumbnailsize", &g_viewerSettings.thumbnailsize );

	if ( g_viewerSettings.thumbnailsize == 0 )
	{
		g_viewerSettings.thumbnailsize = 128;
	}

	RegReadInt( hModelKey, "speechapiindex", &g_viewerSettings.speechapiindex );

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

	RegWriteQAngle( hModelKey, "Rot", g_viewerSettings.rot );
	RegWriteVector( hModelKey, "Trans", g_viewerSettings.trans );
	RegWriteColor( hModelKey, "bgColor", g_viewerSettings.bgColor );
	RegWriteColor( hModelKey, "gColor", g_viewerSettings.gColor );
	RegWriteColor( hModelKey, "lColor", g_viewerSettings.lColor );
	RegWriteColor( hModelKey, "aColor", g_viewerSettings.aColor );
	RegWriteQAngle( hModelKey, "lightrot", g_viewerSettings.lightrot );
	RegWriteInt( hModelKey, "sequence", g_viewerSettings.sequence );
	RegWriteInt( hModelKey, "overlaySequence0", g_viewerSettings.overlaySequence[0] );
	RegWriteFloat( hModelKey, "overlayWeight0", g_viewerSettings.overlayWeight[0] );
	RegWriteInt( hModelKey, "overlaySequence1", g_viewerSettings.overlaySequence[1] );
	RegWriteFloat( hModelKey, "overlayWeight1", g_viewerSettings.overlayWeight[1] );
	RegWriteInt( hModelKey, "overlaySequence2", g_viewerSettings.overlaySequence[2] );
	RegWriteFloat( hModelKey, "overlayWeight2", g_viewerSettings.overlayWeight[2] );
	RegWriteInt( hModelKey, "overlaySequence3", g_viewerSettings.overlaySequence[3] );
	RegWriteFloat( hModelKey, "overlayWeight3", g_viewerSettings.overlayWeight[3] );
	RegWriteInt( hModelKey, "renderwidth", g_viewerSettings.width );
	RegWriteInt( hModelKey, "renderheight", g_viewerSettings.height );
	RegWriteFloat( hModelKey, "speedscale", g_viewerSettings.speedScale );
	RegWriteInt( hModelKey, "viewermode", g_viewerSettings.application_mode );
	RegWriteInt( hModelKey, "thumbnailsize", g_viewerSettings.thumbnailsize );
	RegWriteInt( hModelKey, "speechapiindex", g_viewerSettings.speechapiindex );




	return true;
}
