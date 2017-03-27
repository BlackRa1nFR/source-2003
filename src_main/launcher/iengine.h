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
// $NoKeywords: $
//=============================================================================
#ifndef IENGINE_H
#define IENGINE_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class IEngine
{
public:
	virtual			~IEngine( void ) { }

	virtual bool	LoadModule( void ) = 0;
	virtual void	UnloadModule( void ) = 0;
	virtual CreateInterfaceFn GetFactory( void ) = 0;

	virtual	bool	Load( CreateInterfaceFn launcherFactory ) = 0;
	virtual void	Unload( void ) = 0;
	virtual	void	SetState( int iState ) = 0;
	virtual int		GetState( void ) = 0;
	virtual int		Frame( void ) = 0;

	virtual bool	GetInEngineLoad( void ) = 0;
	virtual void	SetInEngineLoad( bool inload ) = 0;
	virtual double	GetFrameTime( void ) = 0;
	virtual double	GetCurTime( void ) = 0;

	virtual void	TrapKey_Event( int key, bool down ) = 0;
	virtual void	TrapMouse_Event( int buttons, bool down ) = 0;

	virtual void	StartTrapMode( void ) = 0;
	virtual bool	IsTrapping( void ) = 0;
	virtual bool	CheckDoneTrapping( int& buttons, int& key ) = 0;
};

extern IEngine *eng;

#endif // IENGINE_H