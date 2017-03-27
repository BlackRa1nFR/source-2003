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
	virtual					~IVideoMode( void ) { }

	virtual	void			Init( void ) = 0;
	virtual void			Shutdown( void ) = 0;

	virtual struct vmode_s	*AddMode( void ) = 0;
	virtual struct vmode_s	*GetCurrentMode( void ) = 0;

	virtual bool			IsWindowedMode( void ) const = 0;

	virtual void			ReleaseFullScreen( void ) const = 0;

	virtual bool			GetInitialized( void ) const = 0;
	virtual void			SetInitialized( bool init ) = 0;

	virtual void			UpdateWindowPosition( void ) = 0;
	virtual void			GetVid( struct viddef_s *pvid ) = 0;
	virtual bool			AdjustWindowForMode( struct vmode_s *mode ) = 0;

	virtual bool			GetInModeChange( void ) const = 0;
	virtual void			SetInModeChange( bool changing ) = 0;
};

extern IVideoMode *videomode;

#endif // IVIDEOMODE_H