//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_basenetworkable.h"

IMPLEMENT_CLIENTCLASS( C_BaseNetworkable, DT_BaseNetworkable, CBaseNetworkable );


BEGIN_RECV_TABLE_NOBASE( C_BaseNetworkable, DT_BaseNetworkable )
END_RECV_TABLE()


C_BaseNetworkable::C_BaseNetworkable()
{
	m_bDormant = true;
	index = 0xFFFF;
	m_ClientHandle = ClientEntityList().InvalidHandle();
	m_hThink = INVALID_THINK_HANDLE;
	m_DataChangeEventRef = -1;
}


C_BaseNetworkable::~C_BaseNetworkable()
{
	Term();
	ClearDataChangedEvent( m_DataChangeEventRef );
}


void C_BaseNetworkable::Init( int entnum, int iSerialNum )
{
	Assert( index == 0xFFFF );
	index = entnum;

	m_ClientHandle = ClientEntityList().AddNetworkableEntity( GetIClientUnknown(), entnum, iSerialNum );
}


void C_BaseNetworkable::Term()
{
	// Detach from the server list.
	if ( m_ClientHandle != ClientEntityList().InvalidHandle() )
	{
		// Remove from the think list.
		ClientThinkList()->RemoveThinkable( m_ClientHandle );

		ClientEntityList().RemoveEntity( GetRefEHandle() );
		
		index = 0xFFFF;
		
		// RemoveEntity should have done this.
		Assert( m_ClientHandle == ClientEntityList().InvalidHandle() );
	}
}


void C_BaseNetworkable::ClientThink()
{
}


ClientThinkHandle_t	C_BaseNetworkable::GetThinkHandle()
{
	return m_hThink;
}


void C_BaseNetworkable::SetThinkHandle( ClientThinkHandle_t hThink )
{
	m_hThink = hThink;
}


void C_BaseNetworkable::Release()
{
	delete this;
}


void C_BaseNetworkable::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	if ( state == SHOULDTRANSMIT_START )
	{
		SetDormant( false );
	}
	else if ( state == SHOULDTRANSMIT_END )
	{
		SetDormant( true );
	}
}


void C_BaseNetworkable::PreDataUpdate( DataUpdateType_t updateType )
{
	if ( AddDataChangeEvent( this, updateType, &m_DataChangeEventRef ) )
	{
		OnPreDataChanged( updateType );
	}
}


void C_BaseNetworkable::PostDataUpdate( DataUpdateType_t updateType )
{
}


void C_BaseNetworkable::OnPreDataChanged( DataUpdateType_t updateType )
{
}


void C_BaseNetworkable::OnDataChanged( DataUpdateType_t updateType )
{
}


void C_BaseNetworkable::SetDormant( bool bDormant )
{
	m_bDormant = bDormant;
}


bool C_BaseNetworkable::IsDormant( void )
{
	return m_bDormant;
}


int C_BaseNetworkable::entindex( void )
{
	return this->index;
}


void C_BaseNetworkable::ReceiveMessage( const char *msgname, int length, void *data )
{
}


void* C_BaseNetworkable::GetDataTableBasePtr()
{
	return this;
}


void C_BaseNetworkable::SetRefEHandle( const CBaseHandle &handle )
{
	m_ClientHandle = handle;
}


const CBaseHandle& C_BaseNetworkable::GetRefEHandle() const
{
	return m_ClientHandle;
}


