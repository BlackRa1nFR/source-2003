//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CHOREOCHANNEL_H
#define CHOREOCHANNEL_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CChoreoEvent;
class CChoreoActor;

//-----------------------------------------------------------------------------
// Purpose: A channel is owned by an actor and contains zero or more events
//-----------------------------------------------------------------------------
class CChoreoChannel
{
public:
	// Construction
					CChoreoChannel( void );
					CChoreoChannel( const char *name );

	// Assignment
	CChoreoChannel&	operator=(const CChoreoChannel& src );

	// Accessors
	void			SetName( const char *name );
	const char		*GetName( void );

	// Iterate children
	int				GetNumEvents( void );
	CChoreoEvent	*GetEvent( int event );

	// Manipulate children
	void			AddEvent( CChoreoEvent *event );
	void			RemoveEvent( CChoreoEvent *event );
	int				FindEventIndex( CChoreoEvent *event );

	CChoreoActor	*GetActor( void );
	void			SetActor( CChoreoActor *actor );

	void						SetActive( bool active );
	bool						GetActive( void ) const;

private:
	// Initialize fields
	void			Init( void );

	enum
	{
		MAX_CHANNEL_NAME = 128,
	};

	CChoreoActor	*m_pActor;

	// Channels are just named
	char			m_szName[ MAX_CHANNEL_NAME ];

	// All of the events for this channel
	CUtlVector < CChoreoEvent * > m_Events;

	bool			m_bActive;
};

#endif // CHOREOCHANNEL_H
