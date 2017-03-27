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
#if !defined( ILAUNCHERDIRECTDRAW_H )
#define ILAUNCHERDIRECTDRAW_H
#ifdef _WIN32
#pragma once
#endif

class ILauncherDirectDraw
{
public:
	virtual				~ILauncherDirectDraw( void ) { }

	virtual bool		Init( void ) = 0;
	virtual void		Shutdown( void ) = 0;

	virtual void		EnumDisplayModes( int bpp ) = 0;
};

extern ILauncherDirectDraw *directdraw;


#endif // ILAUNCHERDIRECTDRAW_H