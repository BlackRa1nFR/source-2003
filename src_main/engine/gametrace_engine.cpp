//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "gametrace.h"
#include "server.h"
#include "eiface.h"


void CGameTrace::SetEdict( edict_t *pEdict )
{
	m_pEnt = serverGameEnts->EdictToBaseEntity( pEdict );
}


edict_t* CGameTrace::GetEdict() const
{
	return serverGameEnts->BaseEntityToEdict( m_pEnt );
}

