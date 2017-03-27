//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( CD_H )
#define CD_H
#ifdef _WIN32
#pragma once
#endif

class ICDAudio
{
public:
	virtual int		Init( void ) = 0;
	virtual void	Shutdown( void ) = 0;

	virtual void	Play( int track, bool looping ) = 0;
	virtual void	Pause( void ) = 0;
	virtual void	Resume( void ) = 0;
	virtual void	Frame( void ) = 0;
};

extern ICDAudio *cdaudio;

#endif // CD_H