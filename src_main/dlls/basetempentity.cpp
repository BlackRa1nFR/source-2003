//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "mathlib.h"
#include "basetempentity.h"


IMPLEMENT_SERVERCLASS_ST_NOBASE(CBaseTempEntity, DT_BaseTempEntity)
END_SEND_TABLE()




// Global list of temp entity event classes
CBaseTempEntity *CBaseTempEntity::s_pTempEntities = NULL;

//-----------------------------------------------------------------------------
// Purpose: Returns head of list
// Output : CBaseTempEntity * -- head of list
//-----------------------------------------------------------------------------
CBaseTempEntity *CBaseTempEntity::GetList( void )
{
	return s_pTempEntities;
}

//-----------------------------------------------------------------------------
// Purpose: Creates temp entity, sets name, adds to global list
// Input  : *name - 
//-----------------------------------------------------------------------------
CBaseTempEntity::CBaseTempEntity( const char *name )
{
	m_pszName = name;
	Assert( m_pszName );

	// Add to list
	m_pNext			= s_pTempEntities;
	s_pTempEntities = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseTempEntity::~CBaseTempEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the name of this temp entity
// Output : const char *
//-----------------------------------------------------------------------------
const char *CBaseTempEntity::GetName( void )
{
	return m_pszName ? m_pszName : "Unnamed";
}

//-----------------------------------------------------------------------------
// Purpose: Get next temp ent in chain
// Output : CBaseTempEntity *
//-----------------------------------------------------------------------------
CBaseTempEntity *CBaseTempEntity::GetNext( void )
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTempEntity::Precache( void )
{
	// Nothing...
}

//-----------------------------------------------------------------------------
// Purpose: Default test implementation. Should only be called by derived classes
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CBaseTempEntity::Test( const Vector& current_origin, const QAngle& current_angles )
{
	Vector origin, forward;

	Msg( "%s\n", m_pszName );
	AngleVectors( current_angles, &forward );

	VectorMA( current_origin, 20, forward, origin );

	CBroadcastRecipientFilter filter;

	Create( filter, 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Called at startup to allow temp entities to precache any models/sounds that they need
//-----------------------------------------------------------------------------
void CBaseTempEntity::PrecacheTempEnts( void )
{
	CBaseTempEntity *te = GetList();
	while ( te )
	{
		te->Precache();
		te = te->GetNext();
	}
}