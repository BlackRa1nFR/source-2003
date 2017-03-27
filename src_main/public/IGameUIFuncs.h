//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IGAMEUIFUNCS_H
#define IGAMEUIFUNCS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "vgui/keycode.h"

class IGameUIFuncs
{
public:
	virtual bool		IsKeyDown( char const *keyname, bool& isdown ) = 0;
	virtual const char	*Key_NameForKey( int keynum ) = 0;
	virtual const char	*Key_BindingForKey( int keynum ) = 0;
	virtual vgui::KeyCode GetVGUI2KeyCodeForBind( const char *bind ) = 0;

	virtual void		GetVideoModes( struct vmode_s **liststart, int *count ) = 0;
	virtual void		GetCurrentVideoMode( int *wide, int *tall, int *bpp ) = 0;
	virtual void		GetCurrentRenderer( char *name, int namelen, int *windowed ) = 0;
};

#define VENGINE_GAMEUIFUNCS_VERSION "VENGINE_GAMEUIFUNCS_VERSION001"

#endif // IGAMEUIFUNCS_H
