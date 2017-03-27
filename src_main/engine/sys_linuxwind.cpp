//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Linux support for the IGame interface
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "winquake.h"
#include "iengine.h"
#include <stdlib.h>

#include "engine_launcher_api.h"
#include "dll_state.h"
#include "ivideomode.h"
#include "igame.h"

#define HWND int
#define UINT unsigned int
#define WPARAM int
#define LPARAM int

#include "profile.h"
#include "server.h"
#include "cdll_int.h"

void ForceReloadProfile( void );

void ClearIOStates( void );
//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame : public IGame
{
public:
					CGame( void );
	virtual			~CGame( void );

	bool			Init( void *pvInstance );
	bool			Shutdown( void );

	bool			CreateGameWindow( void );

	void			SleepUntilInput( int time );
	HWND			GetMainWindow( void );
	HWND			*GetMainWindowAddress( void );

	void			SetWindowXY( int x, int y );
	void			SetWindowSize( int w, int h );
	void			GetWindowRect( int *x, int *y, int *w, int *h );

	bool			IsActiveApp( void );
	bool			IsMultiplayer( void );
	virtual void		DispatchAllStoredGameMessages();

private:
	void			SetActiveApp( bool fActive );

private:
	bool m_bActiveApp;
	static const char CLASSNAME[];

};

static CGame g_Game;
IGame *game = ( IGame * )&g_Game;

const char CGame::CLASSNAME[] = "Valve001";

#include "conprint.h"

void CGame::SleepUntilInput( int time )
{
	sleep(time);
}

bool CGame::CreateGameWindow( void )
{
	return true;
}

CGame::CGame( void )
{
	m_bActiveApp = true;
}

CGame::~CGame( void )
{
}

bool CGame::Init( void *pvInstance )
{
	return true;
}

bool CGame::Shutdown( void )
{
	return true;
}

HWND CGame::GetMainWindow( void )
{
	return 0;
}

HWND *CGame::GetMainWindowAddress( void )
{
	return NULL;
}

void CGame::SetWindowXY( int x, int y )
{
}

void CGame::SetWindowSize( int w, int h )
{
}

void CGame::GetWindowRect( int *x, int *y, int *w, int *h )
{
	if ( x )
	{
		*x = 0;
	}
	if ( y )
	{
		*y = 0;
	}
	if ( w )
	{
		*w = 0;
	}
	if ( h )
	{
		*h = 0;
	}
}

bool CGame::IsActiveApp( void )
{
	return m_bActiveApp;
}

void CGame::SetActiveApp( bool active )
{
	m_bActiveApp = active;
}

bool CGame::IsMultiplayer( void )
{
	return true;
}

void CGame::DispatchAllStoredGameMessages()
{
}
