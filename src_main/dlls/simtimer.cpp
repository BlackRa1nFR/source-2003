//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "simtimer.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CBaseSimTimer )
	DEFINE_FIELD( CBaseSimTimer,	m_next,			FIELD_TIME	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CSimTimer, CBaseSimTimer )
	DEFINE_FIELD( CSimTimer,		m_interval,		FIELD_FLOAT	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CRandSimTimer, CBaseSimTimer )
	DEFINE_FIELD( CRandSimTimer,		m_minInterval,		FIELD_FLOAT	),
	DEFINE_FIELD( CRandSimTimer,		m_maxInterval,		FIELD_FLOAT	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CRandStopwatchBase, CBaseSimTimer )
	DEFINE_FIELD( CRandStopwatchBase, m_fIsRunning,	FIELD_BOOLEAN	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CRandStopwatch, CRandStopwatchBase )
	DEFINE_FIELD( CRandSimTimer,		m_minInterval,		FIELD_FLOAT	),
	DEFINE_FIELD( CRandSimTimer,		m_maxInterval,		FIELD_FLOAT	),
END_DATADESC()

//-----------------------------------------------------------------------------
