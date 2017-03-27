//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "gametrace.h"



bool CGameTrace::DidHitWorld() const
{
	return m_pEnt == ClientEntityList().GetBaseEntity( 0 );
}


bool CGameTrace::DidHitNonWorldEntity() const
{
	return m_pEnt != NULL && !DidHitWorld();
}


int CGameTrace::GetEntityIndex() const
{
	if ( m_pEnt )
		return m_pEnt->entindex();
	else
		return -1;
}

