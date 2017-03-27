//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#ifndef AI_TRACKPATHER_H
#define AI_TRACKPATHER_H

#include "ai_basenpc.h"

#if defined( _WIN32 )
#pragma once
#endif

class CPathTrack;

//------------------------------------------------------------------------------

class CAI_TrackPather : public CAI_BaseNPC
{
	DECLARE_CLASS( CAI_TrackPather, CAI_BaseNPC );
	DECLARE_DATADESC();
public:

	bool			IsOnPathTrack()							{ return (m_pCurrentPathTarget != NULL); }

protected:	
	void			InitPathingData( float flBaseLead, float flTargetDistance, float flAvoidDistance );
	virtual	void	UpdateTrackNavigation( void );
	virtual bool	GetTrackPatherTarget( Vector *pPos ) { return false; }
	virtual CBaseEntity *GetTrackPatherTargetEnt()	{ return NULL; }

	const Vector &	GetDesiredPosition() const				{ return m_vecDesiredPosition; 	}
	void 			SetDesiredPosition( const Vector &v )	{ m_vecDesiredPosition = v; 	}
	const Vector &	GetGoalOrientation() const				{ return m_vecGoalOrientation; 	}
	void 			SetGoalOrientation( const Vector &v )	{ m_vecGoalOrientation = v; 	}
	CBaseEntity *	GetCurPathTarget();
	float			GetPathLeadBias() const 				{ return m_flPathLeadBias; 		}
	Vector 			GetPathGoalPoint( float leadBias );

	bool			CurPathTargetIsDest()					{ return ( m_pDestPathTarget == m_pCurrentPathTarget ); }

	virtual bool	HasReachedTarget( void ) 				{ return (WorldSpaceCenter() - m_vecDesiredPosition).Length() < 128; }
	virtual void	OnReachedTarget( CBaseEntity *pTarget ) {}

	bool			SetTrack( string_t strTrackName );
	bool			SetTrack( CBaseEntity *pGoalEnt );

	CPathTrack *	GetDestPathTarget()						{ return m_pDestPathTarget;		}

	bool			IsInForcedMove() const					{ return m_bForcedMove;			}
	void			ClearForcedMove()						{ m_bForcedMove = false;		}

	void			OnSave( void );
	void			OnRestore( void );

	virtual void	InputSetTrack( inputdata_t &inputdata );
	virtual void	InputFlyToPathTrack( inputdata_t &inputdata );
	virtual void	InputStartPatrol( inputdata_t &inputdata );
	virtual void	InputStartPatrolBreakable( inputdata_t &inputdata );

private:
	virtual	void	UpdateDesiredPosition( void );
	virtual	void	UpdateTargetPosition( void );
	virtual	void	UpdatePathCorner( void );
	virtual void	UpdatePatrol( void );
	CPathTrack		*BestPointOnPath( const Vector &targetPos, float avoidRadius, bool visible = false );
	CPathTrack		*FindNextPathPoint( CPathTrack *pStart, CPathTrack *pDest );
	bool			ArrivedAtPathPoint( CPathTrack *pPathPoint, float goalThreshold );

	//---------------------------------

	Vector			m_vecDesiredPosition;
	Vector			m_vecGoalOrientation; // orientation of the goal entity.

	CPathTrack		*m_pCurrentPathTarget;
	CPathTrack		*m_pDestPathTarget;
	CPathTrack		*m_pLastPathTarget;

	string_t		m_strCurrentPathName;
	string_t		m_strDestPathName;
	string_t		m_strLastPathName;

	Vector			m_vecLastGoalCheckPosition;	// Last position checked for moving towards
	float			m_flPathUpdateTime;			// Next time to update our path
	float			m_flEnemyPathUpdateTime;	// Next time to update our enemies position
	float			m_flPathLeadBias;			// Distance to lead or trail our desired position
	bool			m_bForcedMove;				// Means the destination point must be reached regardless of enemy position
	bool			m_bPatrolling;				// If set, move back and forth along the current track until we see an enemy
	bool			m_bPatrolBreakable;			// It set, I'll stop patrolling if I see an enemy

	// Derived class pathing data
	float			m_flPathBaseLead;			// Base lead distance added to mapmaker's tweaked m_flPathLeadBias
	float			m_flTargetDistanceThreshold;// Distance threshold used to determine when a target has moved enough to update our navigation to it
	float			m_flAvoidDistance;			// 
};

//------------------------------------------------------------------------------

#endif // AI_TRACKPATHER_H
