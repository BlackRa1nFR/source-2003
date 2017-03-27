//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#ifndef AI_MOVEPROBE_H
#define AI_MOVEPROBE_H

#include "ai_component.h"
#include "ai_navtype.h"
#include "ai_movetypes.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Set of basic tools for probing box movements through space.
//			No moves actually take place
//-----------------------------------------------------------------------------

enum AI_TestGroundMoveFlags_t
{
	AITGM_DEFAULT = 0,
	AITGM_IGNORE_FLOOR = 0x01,
	AITGM_IGNORE_INITIAL_STAND_POS = 0x02,
};

class CAI_MoveProbe : public CAI_Component
{
public:

	CAI_MoveProbe(CAI_BaseNPC *pOuter)
	 : 	CAI_Component( pOuter )
	{
	}
	
	// ----------------------------------------------------
	// Queries & probes
	// ----------------------------------------------------
	bool				MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMove) const;
	bool				MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, bool fCheckStandPositions, AIMoveTrace_t* pMove) const;
	bool				CheckStandPosition( const Vector &vecStart, unsigned int collisionMask ) const;
	bool				FloorPoint( const Vector &vecStart, unsigned int collisionMask, float flStartZ, float flEndZ, Vector *pVecResult ) const;

	// --------------------------------
	// Tracing tools
	// --------------------------------
	void				TraceLine( const Vector &vecStart, const Vector &vecEnd, unsigned int mask, 
								   bool bUseCollisionGroup, trace_t *pResult ) const;

	void 				TraceHull( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
								   const Vector &hullMax, unsigned int mask, 
								   trace_t *ptr ) const;
	
	void 				TraceHull( const Vector &vecStart, const Vector &vecEnd, unsigned int mask, 
								   trace_t *ptr ) const;

	// --------------------------------
	// Checks a ground-based movement
	// --------------------------------
	bool				TestGroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
										unsigned int collisionMask, unsigned flags, AIMoveTrace_t *pMoveTrace ) const;

private:
	CBaseEntity *		CheckStep( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, StepGroundTest_t groundTest, Vector *pVecResult, Vector *pHitNormal ) const; // public just to satisfy CAI_Node

	// these check connections between positions in space, regardless of routes
	void				GroundMoveLimit( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, bool fCheckStep, AIMoveTrace_t* pMoveTrace ) const;
	void				FlyMoveLimit( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMoveTrace) const;
	void				JumpMoveLimit( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMoveTrace) const;
	void				ClimbMoveLimit( const Vector &vecStart, const Vector &vecEnd, const CBaseEntity *pTarget, AIMoveTrace_t* pMoveTrace) const;

	// A floorPoint that is useful only in the contect of iterative movement
	bool				IterativeFloorPoint( const Vector &vecStart, unsigned int collisionMask, Vector *pVecResult ) const;
	bool 				IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
	Vector 				CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float gravity, float *pminHeight, float maxHorzVelocity, Vector *vecApex ) const;

	// Common services provided by CAI_BaseNPC, Convenience methods to simplify code
	float				StepHeight() const;
	bool				CanStandOn( CBaseEntity *pSurface ) const;

	DECLARE_SIMPLE_DATADESC();
};

// ----------------------------------------------------------------------------

inline bool CAI_MoveProbe::MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMove) const
{
	return MoveLimit( navType, vecStart, vecEnd, collisionMask, pTarget, true, pMove);
}

//=============================================================================

#endif // AI_MOVEPROBE_H
