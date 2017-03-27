//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "choreoactor.h"
#include "choreochannel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoActor::CChoreoActor( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CChoreoActor::CChoreoActor( const char *name )
{
	Init();
	SetName( name );
}

//-----------------------------------------------------------------------------
// Purpose: // Assignment
// Input  : src - 
// Output : CChoreoActor&
//-----------------------------------------------------------------------------
CChoreoActor& CChoreoActor::operator=( const CChoreoActor& src )
{
	m_bActive = src.m_bActive;

	strcpy( m_szName, src.m_szName );
	strcpy( m_szFacePoserModelName, src.m_szFacePoserModelName );

	for ( int i = 0; i < src.m_Channels.Size(); i++ )
	{
		CChoreoChannel *c = src.m_Channels[ i ];
		CChoreoChannel *newChannel = new CChoreoChannel();
		newChannel->SetActor( this );
		*newChannel = *c;
		AddChannel( newChannel );
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoActor::Init( void )
{
	m_szName[ 0 ] = 0;
	m_szFacePoserModelName[ 0 ] = 0;
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoActor::SetName( const char *name )
{
	assert( strlen( name ) < MAX_ACTOR_NAME );
	strcpy( m_szName, name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoActor::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoActor::GetNumChannels( void )
{
	return m_Channels.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : channel - 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoActor::GetChannel( int channel )
{
	if ( channel < 0 || channel >= m_Channels.Size() )
	{
		return NULL;
	}

	return m_Channels[ channel ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoActor::AddChannel( CChoreoChannel *channel )
{
	m_Channels.AddToTail( channel );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoActor::RemoveChannel( CChoreoChannel *channel )
{
	int idx = FindChannelIndex( channel );
	if ( idx == -1 )
		return;

	m_Channels.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : c1 - 
//			c2 - 
//-----------------------------------------------------------------------------
void CChoreoActor::SwapChannels( int c1, int c2 )
{
	CChoreoChannel *pch1, *pch2;

	pch1 = m_Channels[ c1 ];
	pch2 = m_Channels[ c2 ];

	if ( !pch1 || !pch2 )
		return;

	m_Channels.Remove( c1 );
	m_Channels.InsertBefore( c1, pch2 );
	m_Channels.Remove( c2 );
	m_Channels.InsertBefore( c2, pch1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoActor::FindChannelIndex( CChoreoChannel *channel )
{
	for ( int i = 0; i < m_Channels.Size(); i++ )
	{
		if ( channel == m_Channels[ i ] )
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoActor::SetFacePoserModelName( const char *name )
{
	strcpy( m_szFacePoserModelName, name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CChoreoActor::GetFacePoserModelName( void ) const
{
	return m_szFacePoserModelName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : active - 
//-----------------------------------------------------------------------------
void CChoreoActor::SetActive( bool active )
{
	m_bActive = active;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoActor::GetActive( void ) const
{
	return m_bActive;
}
