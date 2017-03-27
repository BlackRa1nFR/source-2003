//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TF_PLAYERANIMSTATE_H
#define TF_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "studio.h"

class CCSPlayer;

#if defined( CLIENT_DLL )
	class C_CSPlayer;
	#define CCSPlayer C_CSPlayer
#endif

class CPlayerAnimState
{
public:
	enum
	{
		TURN_NONE = 0,
		TURN_LEFT,
		TURN_RIGHT
	};

						CPlayerAnimState( CCSPlayer *outer );

	Activity			BodyYawTranslateActivity( Activity activity );

	void				Update( float eyeYaw, float eyePitch );

	const QAngle&		GetRenderAngles();
				
	void				GetPoseParameters( float poseParameter[MAXSTUDIOPOSEPARAM] );

	CCSPlayer			*GetOuter() const;

private:
	void				GetOuterAbsVelocity( Vector& vel );
	void				SetOuterPoseParameter( int iParam, float flValue );

	int					ConvergeAngles( float goal,float maxrate, float dt, float& current );

	void				EstimateYaw();
	void				ComputePoseParam_MoveYaw();
	void				ComputePoseParam_BodyPitch();
	void				ComputePoseParam_BodyYaw();

	void				ComputePlaybackRate();


private:

	// The player's eye yaw and pitch angles.
	float m_flEyeYaw;
	float m_flEyePitch;

	CCSPlayer			*m_pOuter;

	float				m_flGaitYaw;
	float				m_flStoredCycle;

	// The following variables are used for tweaking the yaw of the upper body when standing still and
	//  making sure that it smoothly blends in and out once the player starts moving
	// Direction feet were facing when we stopped moving
	float				m_flGoalFeetYaw;
	float				m_flCurrentFeetYaw;

	float				m_flCurrentTorsoYaw;

	// To check if they are rotating in place
	float				m_flLastYaw;
	// Time when we stopped moving
	float				m_flLastTurnTime;

	// One of the above enums
	int					m_nTurningInPlace;

	QAngle				m_angRender;
};

#endif // TF_PLAYERANIMSTATE_H
