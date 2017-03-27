//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "ai_utils.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_MoveMonitor )
	DEFINE_FIELD( CAI_MoveMonitor, m_vMark, FIELD_POSITION_VECTOR ), 
	DEFINE_FIELD( CAI_MoveMonitor, m_flMarkTolerance, FIELD_FLOAT )
END_DATADESC()

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_ShotRegulator )
	DEFINE_EMBEDDED( CAI_ShotRegulator, m_NextShotTimer ),
	DEFINE_FIELD( CAI_ShotRegulator, m_nShotsToTake, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_ShotRegulator, m_nMinShots, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_ShotRegulator, m_nMaxShots, FIELD_INTEGER ),
END_DATADESC()

//-----------------------------------------------------------------------------
