//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_hl2_playerlocaldata.h"
#include "dt_recv.h"

BEGIN_RECV_TABLE_NOBASE( C_HL2PlayerLocalData, DT_HL2Local )
	RecvPropFloat( RECVINFO(m_flSuitPower) )
END_RECV_TABLE()

C_HL2PlayerLocalData::C_HL2PlayerLocalData()
{
	m_flSuitPower = 0.0;
}

