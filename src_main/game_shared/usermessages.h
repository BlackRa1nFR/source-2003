//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef USERMESSAGES_H
#define USERMESSAGES_H
#ifdef _WIN32
#pragma once
#endif

#include "utldict.h"

// Client dispatch function for usermessages
typedef void (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

//-----------------------------------------------------------------------------
// Purpose: Interface for registering and dispatching usermessages
// Shred code creates same ordered list on client/server
//-----------------------------------------------------------------------------
class CUserMessages
{
public:
	CUserMessages();

	// Returns -1 if not found, otherwise, returns appropriate index
	int		LookupUserMessage( const char *name );
	int		GetUserMessageSize( int index );
	const char *GetUserMessageName( int index );
	bool	IsValidIndex( int index );

	// Server only
	void	Register( const char *name, int size );

	// Client only
	void	HookMessage( const char *name, pfnUserMsgHook hook );
	bool	DispatchUserMessage( const char *pszName, int iSize, void *pbuf );

private:

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	struct UserMessage
	{
		// byte size of message, or -1 for variable sized
		int				size;	
		// Client only dispatch function for message
		pfnUserMsgHook	clienthook;
	};

	CUtlDict< UserMessage, int >	m_UserMessages;
};

extern CUserMessages *usermessages;

#endif // USERMESSAGES_H
