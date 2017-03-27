//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CHOREOACTOR_H
#define CHOREOACTOR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CChoreoChannel;

//-----------------------------------------------------------------------------
// Purpose: The actor is the atomic element of a scene
//  A scene can have one or more actors, who have multiple events on one or 
//  more channels
//-----------------------------------------------------------------------------
class CChoreoActor
{
public:

	// Construction
					CChoreoActor( void );
					CChoreoActor( const char *name );
	// Assignment
	CChoreoActor&	operator = ( const CChoreoActor& src );

	// Accessors
	void			SetName( const char *name );
	const char		*GetName( void );

	// Iteration
	int				GetNumChannels( void );
	CChoreoChannel	*GetChannel( int channel );

	// Manipulate children
	void			AddChannel( CChoreoChannel *channel );
	void			RemoveChannel( CChoreoChannel *channel );
	int				FindChannelIndex( CChoreoChannel *channel );
	void			SwapChannels( int c1, int c2 );

	void			SetFacePoserModelName( const char *name );
	char const		*GetFacePoserModelName( void ) const;

	void						SetActive( bool active );
	bool						GetActive( void ) const;

private:
	// Clear structure out
	void			Init( void );

	enum
	{
		MAX_ACTOR_NAME = 128,
		MAX_FACEPOSER_MODEL_NAME = 128
	};

	char			m_szName[ MAX_ACTOR_NAME ];
	char			m_szFacePoserModelName[ MAX_FACEPOSER_MODEL_NAME ];

	// Children
	CUtlVector < CChoreoChannel * >	m_Channels;

	bool			m_bActive;
};

#endif // CHOREOACTOR_H
