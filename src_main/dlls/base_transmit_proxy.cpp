//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "base_transmit_proxy.h"


CBaseTransmitProxy::CBaseTransmitProxy( CBaseEntity *pEnt )
{
	m_hEnt = pEnt;
	m_refCount = 0;
}


CBaseTransmitProxy::~CBaseTransmitProxy()
{
	// Unlink from our parent entity.
	if ( m_hEnt )
	{
		m_refCount = 0xFFFF; // Prevent us from deleting ourselves again.
		m_hEnt->SetTransmitProxy( NULL );
	}
}


bool CBaseTransmitProxy::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea, bool bPrevShouldTransmitResult )
{
	// Anyone implementing a transmit proxy should override this since that's the point!!
	Assert( false );
	return false;
}


void CBaseTransmitProxy::AddRef()
{
	m_refCount++;
}


void CBaseTransmitProxy::Release()
{
	if ( m_refCount == 0xFFFF )
	{
		// This means we are inside our destructor already, so we don't want to do anything here.
	}
	else if ( m_refCount <= 1 )
	{
		delete this;
	}
	else
	{
		--m_refCount;
	}
}

