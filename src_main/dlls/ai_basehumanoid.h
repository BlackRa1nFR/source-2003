//========= Copyright (c) 1996-2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_BASEHUMANOID_H
#define AI_BASEHUMANOID_H

#include "ai_speech.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_navigator.h"

class CAI_HumanoidNavigator;
class CAI_HumanoidMotor;

//-----------------------------------------------------------------------------
// CLASS: CAI_BaseHumanoid
//
// Purpose: The home of fancy human animation transition code
//
//-----------------------------------------------------------------------------

class CAI_BaseHumanoid : public CAI_BehaviorHost<CAI_ExpressiveNPC>
{
public:
	DECLARE_CLASS( CAI_BaseHumanoid, CAI_BehaviorHost<CAI_ExpressiveNPC> );

	CAI_BaseHumanoid();

	float MaxYawSpeed( void );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	bool HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	// --------------------------------

	// void	SetMoveSequence( float flGoalDistance );
	void	BuildMoveScript(  float flGoalDistance );
	void	BuildMoveScript( );
	int		BuildTurnScript( int i, int j );
	void	BuildTurnScript( );
	int 	BuildInsertNode( int i, float flTime );

	Activity GetTransitionActivity( void );

	// --------------------------------
	
	virtual CAI_Motor *CreateMotor();
	virtual CAI_Navigator *CreateNavigator();
	
	// --------------------------------

	/*
	struct movementpiece_t
	{
		float	flSequenceWeight1;
		float	flSequenceWeight2;
		float	flVelocity;
		float	flAngle;
		float	flAnglularVelocity;
		float	dist;
	};
	*/

	struct AI_Movementscript_t
	{
	public:
		AI_Movementscript_t( )
		{
			Init( );
		};

		void Init( void )
		{
			memset( this, 0, sizeof(*this) );
		};

		float	flTime;	// time till next entry
		float	flElapsedTime;	// time since first entry

		float	flDist;

		float	flMaxVelocity;

		// float	flVelocity;

		float	flYaw;
		float	flAngularVelocity;

		bool	bLooping;
		int		nFlags;

		AI_Waypoint_t *pWaypoint;

	public:
		AI_Movementscript_t *pNext;
		AI_Movementscript_t *pPrev;

		// int		iSequence1;
		// int		iSequence2;

		// movementpiece_t start;
		// movementpiece_t end;

		Vector	vecLocation;
		// Vector  vecFacing;

	};
	
	int					m_iScript;
	// int					m_nMaxScript;
	CUtlVector <AI_Movementscript_t>	m_scriptMove;
	CUtlVector <AI_Movementscript_t>	m_scriptTurn;
	float				m_flEntryDist;
	float				m_flExitDist;
	bool				m_bMoveSeqFinished;
};


#endif
