//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "usermessages.h"

void RegisterUserMessages( void );

//-----------------------------------------------------------------------------
// Purpose: Force registration on .dll load
// FIXME:  Should this be a client/server system?
//-----------------------------------------------------------------------------
CUserMessages::CUserMessages()
{
	// Game specific registration function;
	RegisterUserMessages();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::LookupUserMessage( const char *name )
{
	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		return -1;
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::GetUserMessageSize( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	UserMessage *e = &m_UserMessages[ index ];
	return e->size;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CUserMessages::GetUserMessageName( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	return m_UserMessages.GetElementName( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::IsValidIndex( int index )
{
	return m_UserMessages.IsValidIndex( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			size - -1 for variable size
//-----------------------------------------------------------------------------
void CUserMessages::Register( const char *name, int size )
{
	Assert( name );
	int idx = m_UserMessages.Find( name );
	if ( idx != m_UserMessages.InvalidIndex() )
	{
		Error( "CUserMessages::Register '%' already registered\n", name );
	}

	UserMessage entry;
	entry.size = size;

	m_UserMessages.Insert( name, entry );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			hook - 
//-----------------------------------------------------------------------------
void CUserMessages::HookMessage( const char *name, pfnUserMsgHook hook )
{
#if defined( CLIENT_DLL )
	Assert( name );
	Assert( hook );

	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		DevMsg( "CUserMessages::HookMessage:  no such message %s\n", name );
		Assert( 0 );
		return;
	}

	m_UserMessages[ idx ].clienthook = hook;
#else
	Error( "CUserMessages::HookMessage called from server code!!!\n" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::DispatchUserMessage( const char *pszName, int iSize, void *pbuf )
{
#if defined( CLIENT_DLL )
	Assert( pszName );

	int idx = LookupUserMessage( pszName );
	if ( !IsValidIndex( idx ) )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  No match for message %s\n", pszName );
		Assert( 0 );
		return false;
	}

	UserMessage *entry = &m_UserMessages[ idx ];

	if ( !entry->clienthook )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Missing client hook for %s\n", pszName );
		Assert( 0 );
		return false;
	}

	(*entry->clienthook)( pszName, iSize, pbuf );
	return true;
#else
	Error( "CUserMessages::DispatchUserMessage called from server code!!!\n" );
	return false;
#endif
}

// Singleton
static CUserMessages g_UserMessages;
// Expose to rest of .dll
CUserMessages *usermessages = &g_UserMessages;