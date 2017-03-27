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
#if !defined( IVIDEOMODE_H )
#define IVIDEOMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "vmodes.h"

class IVideoMode
{
public:	
	virtual char const		*GetName( void ) = 0;

	virtual	bool			Init( void *pvInstance ) = 0;
	virtual void			Shutdown( void ) = 0;

	virtual struct vmode_s	*AddMode( void ) = 0;

	virtual struct vmode_s	*GetCurrentMode( void ) = 0;
	virtual struct vmode_s	*GetMode( int num ) = 0;
	virtual int				GetModeCount( void ) = 0;

	virtual bool			IsWindowedMode( void ) const = 0;

	virtual bool			GetInitialized( void ) const = 0;
	virtual void			SetInitialized( bool init ) = 0;

	virtual void			UpdateWindowPosition( void ) = 0;

	virtual void			FlipScreen( void ) = 0;

	virtual void			RestoreVideo( void ) = 0;
	virtual void			ReleaseVideo( void ) = 0;
};

extern IVideoMode *videomode;

void VideoMode_Create();
void VideoMode_GetVideoModes( struct vmode_s **liststart, int *count );
void VideoMode_GetCurrentVideoMode( int *wide, int *tall, int *bpp );
void VideoMode_GetCurrentRenderer( char *name, int namelen, int *windowed );

#endif // IVIDEOMODE_H