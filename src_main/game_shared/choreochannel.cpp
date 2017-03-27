
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "choreochannel.h"
#include "choreoevent.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoChannel::CChoreoChannel( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CChoreoChannel::CChoreoChannel(const char *name )
{
	Init();
	SetName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Assignment
// Input  : src - 
//-----------------------------------------------------------------------------
CChoreoChannel&	CChoreoChannel::operator=( const CChoreoChannel& src )
{
	m_bActive = src.m_bActive;
	strcpy( m_szName, src.m_szName );
	for ( int i = 0; i < src.m_Events.Size(); i++ )
	{
		CChoreoEvent *e = src.m_Events[ i ];
		CChoreoEvent *newEvent = new CChoreoEvent( e->GetScene() );
		*newEvent = *e;
		AddEvent( newEvent );
		newEvent->SetChannel( this );
		newEvent->SetActor( m_pActor );
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoChannel::SetName( const char *name )
{
	assert( strlen( name ) < MAX_CHANNEL_NAME );
	strcpy( m_szName, name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoChannel::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoChannel::GetNumEvents( void )
{
	return m_Events.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoChannel::GetEvent( int event )
{
	if ( event < 0 || event >= m_Events.Size() )
	{
		return NULL;
	}

	return m_Events[ event ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannel::AddEvent( CChoreoEvent *event )
{
	m_Events.AddToTail( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannel::RemoveEvent( CChoreoEvent *event )
{
	int idx = FindEventIndex( event );
	if ( idx == -1 )
		return;

	m_Events.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoChannel::FindEventIndex( CChoreoEvent *event )
{
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		if ( event == m_Events[ i ] )
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoChannel::Init( void )
{
	m_szName[ 0 ] = 0;
	SetActor( NULL );
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoChannel::GetActor( void )
{
	return m_pActor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoChannel::SetActor( CChoreoActor *actor )
{
	m_pActor = actor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : active - 
//-----------------------------------------------------------------------------
void CChoreoChannel::SetActive( bool active )
{
	m_bActive = active;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoChannel::GetActive( void ) const
{
	return m_bActive;
}
