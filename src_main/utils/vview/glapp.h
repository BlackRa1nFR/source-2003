//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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

#ifndef GLAPP_H
#define GLAPP_H
#pragma once


#ifdef _WINDOWS_
typedef struct
{
    int   width;
    int   height;
    int   bpp;
    int   flags;
    int   frequency;
} screen_res_t;

typedef struct
{
    HINSTANCE        hInstance;
    int              iCmdShow;
    HWND             hWnd;
    HDC              hDC;
    HGLRC            hGLRC;
    BOOL             bActive;
    BOOL             bFullScreen;
    ATOM             wndclass;
    WNDPROC          wndproc;
    int              width;
    int              height;
	int				 centerx;		// for mouse offset calculations
	int				 centery;
    int              bpp;
    BOOL             bChangeBPP;
    BOOL             bAllowSoft;
	BOOL			 bPaused;
    char            *szCmdLine;
    int              argc;
    char           **argv;
    int              glnWidth;
    int              glnHeight;
    float            gldAspect;
    float            NearClip;
    float            FarClip;
    float            fov;
    int              iResCount;
    screen_res_t    *pResolutions;
    int              iVidMode;
} windata_t;

typedef struct
{
    int  width;
    int  height;
    int  bpp;
} devinfo_t;
extern windata_t   WinData;
extern devinfo_t   DevInfo;
#endif

void GetParameters( void );
int  FindNumParameter( const char *s );
bool FindParameter( const char *s );
const char *LastParameter( void );
const char *FindParameterArg( const char *s );

void BuildModeList(void);
bool SetVideoMode(void);
bool InitOpenGL(void);
void LoadTexture(void);
void RenderScene(void);
void Error( const char *error, ... );

void ShutdownOpenGL(void);
void Cleanup(void);
void GetScreen( int &width, int &height );

extern unsigned int g_Time;


#endif // GLAPP_H
