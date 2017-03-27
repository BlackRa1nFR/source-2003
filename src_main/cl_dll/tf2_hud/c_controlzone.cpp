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
#include "c_controlzone.h"
#include "mapdata.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ControlZone::C_ControlZone()
{
	m_nZoneNumber = 0;

	m_pShowTriggers = cvar->FindVar("showtriggers");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ControlZone::~C_ControlZone()
{
}

//-----------------------------------------------------------------------------
// Purpose: Are we using showtriggers?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_ControlZone::ShouldDraw()
{
	if ( !m_pShowTriggers )
		return false;

	return m_pShowTriggers->GetInt() != 0 ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Update global map state based on data received
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_ControlZone::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	CMapZones *zone;
	if ( m_nZoneNumber < 1 ||
		 m_nZoneNumber > MAX_ZONES )
		 return;

	zone = &MapData().m_Zones[ m_nZoneNumber -1 ];
	zone->m_nControllingTeam = GetTeamNumber();
}

IMPLEMENT_CLIENTCLASS_DT(C_ControlZone, DT_ControlZone, CControlZone)
	RecvPropInt( RECVINFO(m_nZoneNumber )),
END_RECV_TABLE()


