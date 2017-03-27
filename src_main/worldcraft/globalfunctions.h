//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GLOBALFUNCTIONS_H
#define GLOBALFUNCTIONS_H
#ifdef _WIN32
#pragma once
#endif

#include "MapClass.h"	// For CMapObjectList


class CMapSolid;
class CMainFrame;
class CMapWorld;
class CMapDoc;
class IWorldEditDispMgr;
class CSubdivMesh;


CMapWorld *GetActiveWorld(void);

IWorldEditDispMgr *GetActiveWorldEditDispManager( void );

//
// misc.cpp:
//
void randomize();
DWORD random();

void NotifyDuplicates(CMapSolid *pSolid);
void NotifyDuplicates(CMapObjectList *pList);

void WriteDebug(char *pszStr);
LPCTSTR GetDefaultTextureName();
void SetDefaultTextureName( const char *szTexName );
int mychdir(LPCTSTR pszDir);

const char *stristr(const char *str1, const char *str2);


//
// Message window interface.
//
typedef enum
{	mwStatus,
	mwError,
	mwWarning
} MWMSGTYPE;

void Msg(MWMSGTYPE type, LPCTSTR fmt, ...);

//
// timing functions
//
double I_FloatTime( void );
void I_BeginTime( void );
double I_EndTime( void );

// noise function
float PerlinNoise2D( float x, float y, float rockiness );
float PerlinNoise2DScaled( float x, float y, float rockiness );

void DBG(char *fmt, ...);

#endif // GLOBALFUNCTIONS_H
