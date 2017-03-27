//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Teleports a named entity to a given position and restores
//			it's physics state
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#define	SF_TELEPORT_TO_SPAWN_POS	0x00000001

class CPointTeleport : public CBaseEntity
{
	DECLARE_CLASS( CPointTeleport, CBaseEntity );
public:
	void	Activate( void );

	void InputTeleport( inputdata_t &inputdata );

	Vector m_vSaveOrigin;
	QAngle m_vSaveAngles;

	DECLARE_DATADESC();
};


LINK_ENTITY_TO_CLASS( point_teleport, CPointTeleport );


BEGIN_DATADESC( CPointTeleport )

	DEFINE_FIELD( CPointTeleport, m_vSaveOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CPointTeleport, m_vSaveAngles, FIELD_VECTOR ),

	DEFINE_INPUTFUNC( CPointTeleport, FIELD_VOID, "Teleport", InputTeleport ),

END_DATADESC()




//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointTeleport::Activate( void )
{
	CBaseEntity *pTarget = GetNextTarget();
	if (pTarget)
	{
		if ( pTarget->GetMoveParent() != NULL )
		{
			Warning("ERROR: (%s) can't teleport object (%s) as it has a parent!\n",GetDebugName(),pTarget->GetDebugName());

			return;
		}
		if (m_spawnflags & SF_TELEPORT_TO_SPAWN_POS)
		{
			m_vSaveOrigin = pTarget->GetAbsOrigin();
			m_vSaveAngles = pTarget->GetAbsAngles();
		}
		else
		{
			m_vSaveOrigin = GetAbsOrigin();
			m_vSaveAngles = GetAbsAngles();
		}
	}
	else
	{
		Warning("ERROR: (%s) given no target.  Deleting.\n",GetDebugName());
		UTIL_Remove(this);
		return;
	}
	BaseClass::Activate();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointTeleport::InputTeleport( inputdata_t &inputdata )
{
	CBaseEntity *pTarget = GetNextTarget();
	if (pTarget)
	{
		// If teleport object is in a movement hierarchy, remove it first
		if ( pTarget->GetMoveParent() != NULL )
		{
			Warning("ERROR: (%s) can't teleport object (%s) as it has a parent (%s)!\n",GetDebugName(),pTarget->GetDebugName(),pTarget->GetMoveParent()->GetDebugName());
			return;
		}

		pTarget->Teleport( &m_vSaveOrigin, &m_vSaveAngles, NULL );
	}
}

