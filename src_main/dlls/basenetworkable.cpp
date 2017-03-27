//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basenetworkable.h"
#include "tier0/dbg.h"
#include "gameinterface.h"


IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseNetworkable, DT_BaseNetworkable )
END_SEND_TABLE()


CBaseNetworkable::CBaseNetworkable()
{
	m_pPev = 0;
	m_iEFlags = 0;
}


CBaseNetworkable::~CBaseNetworkable()
{
	Assert( m_pPev );	// Not sure if this check is really necessary. It's in here 
						// as a sanity check so I can catch networkables that aren't
						// calling AttachEdict.

	gEntList.RemoveEntity( GetRefEHandle() );
	DetachEdict();
}


void CBaseNetworkable::AttachEdict()
{
	Assert( !m_pPev );

	m_pPev = engine->CreateEdict();
	m_pPev->m_pEnt = this;
	m_pPev->SetFullEdict( false );
}


void CBaseNetworkable::PostConstructor( const char *pClassName )
{
	AttachEdict();
	gEntList.AddNetworkableEntity( this, entindex() );
}


void CBaseNetworkable::SetRefEHandle( const CBaseHandle &handle )
{
	m_RefEHandle = handle;
}


const CBaseHandle& CBaseNetworkable::GetRefEHandle() const
{
	return m_RefEHandle;
}


EntityChange_t CBaseNetworkable::DetectNetworkStateChanges()
{
	return ENTITY_CHANGE_AUTODETECT;
}

void CBaseNetworkable::ResetNetworkStateChanges()
{
}


int CBaseNetworkable::GetEFlags() const
{
	return m_iEFlags;
}


void CBaseNetworkable::SetEFlags( int iEFlags )
{
	m_iEFlags = iEFlags;
}


edict_t* CBaseNetworkable::GetEdict() const
{
	return (edict_t*)m_pPev;
}


bool CBaseNetworkable::ShouldTransmit( 
	const edict_t *recipient, 
	const void *pvs, int clientArea )
{
	return true;
}


void CBaseNetworkable::CheckTransmit( CCheckTransmitInfo *pInfo )
{
	if ( ShouldTransmit( pInfo->m_pClientEnt, pInfo->m_pPVS, pInfo->m_iArea ) )
		SetTransmit( pInfo );
}


void CBaseNetworkable::SetTransmit( CCheckTransmitInfo *pInfo )
{
	// Are we already marked for transmission?
	if ( pInfo->WillTransmit( entindex() ) )
		return;

	pInfo->SetTransmit( entindex() );
}


CBaseNetworkable* CBaseNetworkable::GetBaseNetworkable()
{
	return this;
}


CBaseEntity* CBaseNetworkable::GetBaseEntity()
{
	return NULL;
}


void CBaseNetworkable::DetachEdict()
{
	if ( !m_pPev )
		return;

	m_pPev->m_pEnt = 0;
	engine->RemoveEdict( (struct edict_t*)m_pPev );
	m_pPev = 0;
}


int CBaseNetworkable::entindex() const
{
	Assert( m_pPev );
	return ENTINDEX( (edict_t*)m_pPev );
}


void CBaseNetworkable::NetworkStateChanged()
{
	m_NetStateMgr.StateChanged();
}


