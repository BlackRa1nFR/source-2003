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
#if !defined( GAME_H )
#define GAME_H
#ifdef _WIN32
#pragma once
#else
#define HWND int
#endif

class IGame
{
public:
	virtual			~IGame( void ) { }

	virtual	bool	Init( void *pvInstance ) = 0;
	virtual bool	Shutdown( void ) = 0;

	virtual bool	CreateGameWindow( void ) = 0;

	virtual void	SleepUntilInput( int time ) = 0;

	virtual HWND	GetMainWindow( void ) = 0;
	virtual HWND	*GetMainWindowAddress( void ) = 0;

	virtual void	SetWindowXY( int x, int y ) = 0;
	virtual void	SetWindowSize( int w, int h ) = 0;

	virtual void	GetWindowRect( int *x, int *y, int *w, int *h ) = 0;

	// Not Alt-Tabbed away
	virtual bool	IsActiveApp( void ) = 0;

	virtual bool	IsMultiplayer( void ) = 0;

	virtual void	DispatchAllStoredGameMessages() = 0;
};

extern IGame *game;

#endif // GAME_H
