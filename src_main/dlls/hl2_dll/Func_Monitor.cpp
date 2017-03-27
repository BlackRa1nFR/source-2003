//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_monitor.h"
#include "point_camera.h"


BEGIN_DATADESC( CFuncMonitor )

	DEFINE_FIELD( CFuncMonitor, m_TargetIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncMonitor, m_pCamera, FIELD_CLASSPTR ),

	// Outputs
	DEFINE_INPUTFUNC( CFuncMonitor, FIELD_STRING, "SetCamera", InputSetCamera ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( func_monitor, CFuncMonitor );


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//-----------------------------------------------------------------------------
void CFuncMonitor::Activate()
{
	BaseClass::Activate();
	SetCameraByName(STRING(m_target));
}


CPointCamera *CFuncMonitor::GetCamera()
{
	return m_pCamera;
}


void CFuncMonitor::SetCameraByName(const char *szName)
{
	CBaseEntity *pBaseEnt = gEntList.FindEntityByName( NULL, szName, NULL );
	if( pBaseEnt )
	{
		m_pCamera = dynamic_cast<CPointCamera *>( pBaseEnt );
		if( m_pCamera )
		{
			m_TargetIndex = pBaseEnt->entindex();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMonitor::InputSetCamera(inputdata_t &inputdata)
{
	SetCameraByName(inputdata.value.String());
}


IMPLEMENT_SERVERCLASS_ST( CFuncMonitor, DT_FuncMonitor )
	SendPropInt( SENDINFO( m_TargetIndex ), 16, SPROP_UNSIGNED ),
END_SEND_TABLE()
