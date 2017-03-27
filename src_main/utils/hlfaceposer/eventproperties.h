//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef EVENTPROPERTIES_H
#define EVENTPROPERTIES_H
#ifdef _WIN32
#pragma once
#endif

#include "basedialogparams.h"

class CChoreoScene;
class CChoreoEvent;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CEventParams : public CBaseDialogParams
{
	// e.g. CChoreoEvent::GESTURE
	int				m_nType;

	// Event descriptive name
	char			m_szName[ 256 ];

	// Expression name/wav name/gesture name/look at name
	char			m_szParameters[ 256 ];
	char			m_szParameters2[ 256 ];

	CChoreoScene	*m_pScene;

	float			m_flStartTime;
	float			m_flEndTime;
	bool			m_bHasEndTime;

	CChoreoEvent	*m_pEvent;

	bool			m_bFixedLength;

	bool			m_bResumeCondition;

	bool			m_bUsesTag;
	char			m_szTagName[ 256 ];
	char			m_szTagWav[ 256 ];

	// For Lookat events
	int				pitch;
	int				yaw;
	bool			usepitchyaw;
};

int EventProperties( CEventParams *params );

#endif // EVENTPROPERTIES_H
