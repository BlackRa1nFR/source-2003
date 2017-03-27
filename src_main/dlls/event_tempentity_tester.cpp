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
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basetempentity.h"
#include "event_tempentity_tester.h"
#include "vstdlib/strtools.h"

#define TEMPENT_TEST_GAP		1.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecAngles - 
//			*single_te - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CTempEntTester::Create( const Vector &vecOrigin, const QAngle &vecAngles, const char *lifetime, const char *single_te )
{
	float life;
	char classname[ 128 ];
	if ( lifetime && lifetime[0] )
	{
		life = atoi( lifetime );
		life = max( 1.0, life );
		life = min( 1000.0, life );

		life += gpGlobals->curtime;
	}
	else
	{
		Msg( "Usage:  te <lifetime> <entname>\n" );
		return NULL;
	}

	if ( single_te && single_te[0] )
	{
		Q_strncpy( classname, single_te ,sizeof(classname));
		strlwr( classname );
	}
	else
	{
		Msg( "Usage:  te <lifetime> <entname>\n" );
		return NULL;
	}

	CTempEntTester *p = ( CTempEntTester * )CBaseEntity::CreateNoSpawn( "te_tester", vecOrigin, vecAngles );
	if ( !p )
	{
		return NULL;
	}

	Q_strncpy( p->m_szClass, classname ,sizeof(p->m_szClass));
	p->m_fLifeTime = life;

	p->Spawn();

	return p;
}

LINK_ENTITY_TO_CLASS( te_tester, CTempEntTester );

//-----------------------------------------------------------------------------
// Purpose: Called when object is being created
//-----------------------------------------------------------------------------
void CTempEntTester::Spawn( void )
{
	// Not a physical thing...
	m_fEffects |= EF_NODRAW;

	m_pCurrent = CBaseTempEntity::GetList();
	while ( m_pCurrent )
	{
		char name[ 128 ];
		Q_strncpy( name, m_pCurrent->GetName() ,sizeof(name));
		strlwr( name );
		if ( strstr( name, m_szClass ) )
		{
			break;
		}

		m_pCurrent = m_pCurrent->GetNext();
	}

	if ( !m_pCurrent )
	{
		UTIL_Remove( this );
		return;
	}

	// Think right away
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Called when object should fire itself and move on
//-----------------------------------------------------------------------------
void CTempEntTester::Think( void )
{
	// Should never happen
	if ( !m_pCurrent )
	{
		UTIL_Remove( this );
		return;
	}

	m_pCurrent->Test( GetLocalOrigin(), GetLocalAngles() );
	SetNextThink( gpGlobals->curtime + TEMPENT_TEST_GAP );

	// Time to destroy?
	if ( gpGlobals->curtime >= m_fLifeTime )
	{
		UTIL_Remove( this );
		return;
	}
}