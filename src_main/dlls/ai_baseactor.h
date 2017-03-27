//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//			groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================

#ifndef AI_BASEACTOR_H
#define AI_BASEACTOR_H

#include "ai_basehumanoid.h"
#include "ai_speech.h"
#include "AI_Interest_Target.h"
#include <limits.h>


#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// CAI_BaseActor
//
// Purpose: The base class for all facially expressive NPCS.
//
//-----------------------------------------------------------------------------
enum PoseParameter_t { POSE_END=INT_MAX };
enum FlexWeight_t { FLEX_END=INT_MAX };


class CAI_BaseActor : public CAI_BaseHumanoid
{
	DECLARE_CLASS( CAI_BaseActor, CAI_BaseHumanoid );

	//friend CPoseParameter;
	//friend CFlexWeight;

public:
	// FIXME: this method is lame, isn't there some sort of template thing that would get rid of the Outer pointer?

	void	Init( PoseParameter_t &index, const char *szName ) { index = (PoseParameter_t)LookupPoseParameter( szName ); };
	void	Set( PoseParameter_t index, float flValue ) { SetPoseParameter( (int)index, flValue ); }
	float	Get( PoseParameter_t index ) { return GetPoseParameter( (int)index ); }

	void	Init( FlexWeight_t &index, const char *szName ) { index = (FlexWeight_t)FindFlexController( szName ); };
	void	Set( FlexWeight_t index, float flValue ) { SetFlexWeight( (int)index, flValue ); }
	float	Get( FlexWeight_t index ) { return GetFlexWeight( (int)index ); }


public:
	CAI_BaseActor()
	 :	m_fLatchedPositions( 0 ),
		m_latchedEyeOrigin( vec3_origin ),
		m_latchedEyeDirection( vec3_origin ),
		m_latchedHeadDirection( vec3_origin ),
		m_flBlinktime( 0 ),
		m_hLookTarget( NULL ),
		m_iszExpressionScene( NULL_STRING ),
		m_iszIdleExpression( NULL_STRING ),
		m_iszAlertExpression( NULL_STRING ),
		m_iszCombatExpression( NULL_STRING ),
		m_iszDeathExpression( NULL_STRING ),
		m_iszExpressionOverride( NULL_STRING )
	{
	}

	virtual void			StudioFrameAdvance();

	virtual void			SetModel( const char *szModelName );

	Vector					EyePosition( );
	virtual Vector			HeadDirection2D( void );
	virtual Vector			HeadDirection3D( void );
	virtual Vector			EyeDirection2D( void );
	virtual Vector			EyeDirection3D( void );

	// CBaseFlex
	virtual	void			SetViewtarget( const Vector &viewtarget );

	// CAI_BaseNPC
	virtual float			PickRandomLookTarget( bool bExcludePlayers = false, float minTime = 1.5, float maxTime = 2.5 );
	virtual bool			HasActiveLookTargets( void );


	virtual void			AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void			AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );

	virtual void			SetHeadDirection( const Vector &vTargetPos, float flInterval );
	virtual void			MaintainLookTargets( float flInterval );
	virtual bool			ValidEyeTarget(const Vector &lookTargetPos);
	virtual bool			ValidHeadTarget(const Vector &lookTargetPos);

	//---------------------------------

	virtual	void 			OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	//---------------------------------

	virtual const char *	SelectExpressionForState( NPC_STATE state );

	void					SetExpression( const char * );
	void					ClearExpression();
	const char *			GetExpression();

	DECLARE_DATADESC();
private:
	enum
	{
		HUMANOID_LATCHED_EYE	= 0x0001,
		HUMANOID_LATCHED_HEAD	= 0x0002
	};

	//---------------------------------

	void					UpdateLatchedValues( void );

	// Input handlers.
	void InputSetExpressionOverride( inputdata_t &inputdata );

	//---------------------------------

	int						m_fLatchedPositions;
	Vector					m_latchedEyeOrigin;
	Vector 					m_latchedEyeDirection;
	Vector 					m_latchedHeadDirection;

	void					ClearHeadAdjustment( void );
	Vector					m_goalHeadDirection;

	float					m_flBlinktime;
	EHANDLE					m_hLookTarget;
	CAI_InterestTarget		m_lookQueue;
	float					m_flNextRandomLookTime;	// FIXME: move to scene

	//---------------------------------

	string_t				m_iszExpressionScene;
	EHANDLE					m_hExpressionSceneEnt;

	string_t				m_iszExpressionOverride;

protected:
	string_t				m_iszIdleExpression;
	string_t				m_iszAlertExpression;
	string_t				m_iszCombatExpression;
	string_t				m_iszDeathExpression;

private:
	//---------------------------------

	PoseParameter_t			m_ParameterBodyTransY;		// "body_trans_Y"
	PoseParameter_t			m_ParameterBodyTransX;		// "body_trans_X"
	PoseParameter_t			m_ParameterBodyLift;		// "body_lift"
	PoseParameter_t			m_ParameterBodyYaw;			// "body_yaw"
	PoseParameter_t			m_ParameterBodyPitch;		// "body_pitch"
	PoseParameter_t			m_ParameterBodyRoll;		// "body_roll"
	PoseParameter_t			m_ParameterSpineYaw;		// "spine_yaw"
	PoseParameter_t			m_ParameterSpinePitch;		// "spine_pitch"
	PoseParameter_t			m_ParameterSpineRoll;		// "spine_roll"
	PoseParameter_t			m_ParameterNeckTrans;		// "neck_trans"
	PoseParameter_t			m_ParameterHeadYaw;			// "head_yaw"
	PoseParameter_t			m_ParameterHeadPitch;		// "head_pitch"
	PoseParameter_t			m_ParameterHeadRoll;		// "head_roll"

	FlexWeight_t			m_FlexweightMoveRightLeft;	// "move_rightleft"
	FlexWeight_t			m_FlexweightMoveForwardBack;// "move_forwardback"
	FlexWeight_t			m_FlexweightMoveUpDown;		// "move_updown"
	FlexWeight_t			m_FlexweightBodyRightLeft;	// "body_rightleft"
	FlexWeight_t			m_FlexweightBodyUpDown;		// "body_updown"
	FlexWeight_t			m_FlexweightBodyTilt;		// "body_tilt"
	FlexWeight_t			m_FlexweightChestRightLeft;	// "chest_rightleft"
	FlexWeight_t			m_FlexweightChestUpDown;	// "chest_updown"
	FlexWeight_t			m_FlexweightChestTilt;		// "chest_tilt"
	FlexWeight_t			m_FlexweightHeadForwardBack;// "head_forwardback"
	FlexWeight_t			m_FlexweightHeadRightLeft;	// "head_rightleft"
	FlexWeight_t			m_FlexweightHeadUpDown;		// "head_updown"
	FlexWeight_t			m_FlexweightHeadTilt;		// "head_tilt"
};

//-----------------------------------------------------------------------------
#endif // AI_BASEACTOR_H
