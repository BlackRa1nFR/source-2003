//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_BHMOVEMENT_H
#define AI_BHMOVEMENT_H

#if defined( _WIN32 )
#pragma once
#endif

#include "ai_baseactor.h"
#include "ai_motor.h"
#include "ai_navigator.h"
#include "ai_basehumanoid.h"


//-----------------------------------------------------------------------------

class CAI_HumanoidNavigator : public CAI_ComponentWithOuter<CAI_BaseHumanoid, CAI_Navigator>
{
	typedef CAI_ComponentWithOuter<CAI_BaseHumanoid, CAI_Navigator> BaseClass;
public:
	CAI_HumanoidNavigator( CAI_BaseHumanoid *pOuter )
	 :	BaseClass( pOuter )
	{
		m_bResetActivity = false;
	}

protected:
	AIMoveResult_t MoveNormal();
	void SetActivity( Activity NewActivity );
	bool m_bResetActivity;
};

//-----------------------------------------------------------------------------

class CAI_HumanoidMotor : public CAI_ComponentWithOuter<CAI_BaseHumanoid, CAI_Motor>
{
	typedef CAI_ComponentWithOuter<CAI_BaseHumanoid, CAI_Motor> BaseClass;
public:
	CAI_HumanoidMotor( CAI_BaseHumanoid *pOuter )
	 :	BaseClass( pOuter )
	{
		m_iStandLayer = -1;
	}

public:
	void		MoveStop();

protected:
	AIMotorMoveResult_t MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult );

	int	m_iStandLayer;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------	




#endif // AI_BHMOVEMENT_H
