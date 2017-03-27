//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Dynamic light at the end of a spotlight
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "spotlightend.h"

LINK_ENTITY_TO_CLASS(spotlight_end, CSpotlightEnd);

IMPLEMENT_SERVERCLASS_ST(CSpotlightEnd, DT_SpotlightEnd)
	SendPropFloat(SENDINFO(m_flLightScale), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Radius), 0, SPROP_NOSCALE),
	SendPropVector(SENDINFO(m_vSpotlightDir), -1, SPROP_NORMAL),
	SendPropVector(SENDINFO(m_vSpotlightOrg), -1, SPROP_COORD),
END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CSpotlightEnd )

	DEFINE_FIELD( CSpotlightEnd, m_flLightScale, FIELD_FLOAT ),
	DEFINE_FIELD( CSpotlightEnd, m_Radius, FIELD_FLOAT ),
	DEFINE_FIELD( CSpotlightEnd, m_vSpotlightDir, FIELD_VECTOR ),
	DEFINE_FIELD( CSpotlightEnd, m_vSpotlightOrg, FIELD_POSITION_VECTOR ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CSpotlightEnd::Spawn( void )
{
	Precache();
	m_flLightScale  = 100;
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_FLY );
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	Relink();
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}
