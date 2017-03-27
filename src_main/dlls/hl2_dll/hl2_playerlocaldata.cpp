//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "mathlib.h"
#include "entitylist.h"


BEGIN_SEND_TABLE_NOBASE( CHL2PlayerLocalData, DT_HL2Local )
	SendPropFloat( SENDINFO(m_flSuitPower), 10, SPROP_UNSIGNED | SPROP_ROUNDUP, 0.0, 100.0 ),
END_SEND_TABLE()

BEGIN_SIMPLE_DATADESC( CHL2PlayerLocalData )
	DEFINE_FIELD( CHL2PlayerLocalData, m_flSuitPower, FIELD_FLOAT ),
END_DATADESC()


CHL2PlayerLocalData::CHL2PlayerLocalData()
{
	m_flSuitPower = 0.0;
}

