//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef NPC_MANHACK_H
#define NPC_MANHACK_H
#ifdef _WIN32
#pragma once
#endif

#include "AI_BaseNPC_PhysicsFlyer.h"
#include "sprite.h"
#include "SpriteTrail.h"

// Start with the engine off and folded up.
#define SF_MANHACK_PACKED_UP	0x00000400
#define SF_MANHACK_LIMIT_SPEED	0x00000800


//-----------------------------------------------------------------------------
// Attachment points.
//-----------------------------------------------------------------------------
#define	MANHACK_GIB_HEALTH				30
#define	MANHACK_INACTIVE_HEALTH			25
#define	MANHACK_MAX_SPEED				400
#define MANHACK_BURST_SPEED				550
#define MANHACK_NPC_BURST_SPEED			800

//-----------------------------------------------------------------------------
// Movement parameters.
//-----------------------------------------------------------------------------
#define MANHACK_WAYPOINT_DISTANCE		25	// Distance from waypoint that counts as arrival.

class CSprite;

//-----------------------------------------------------------------------------
// Manhack 
//-----------------------------------------------------------------------------
class CNPC_Manhack : public CAI_BasePhysicsFlyingBot
{
	DECLARE_CLASS( CNPC_Manhack, CAI_BasePhysicsFlyingBot );
	DECLARE_SERVERCLASS();

	public:
		CNPC_Manhack();
		~CNPC_Manhack();

		Class_T			Classify(void);

		bool			CorpseGib( const CTakeDamageInfo &info );
		void			Ragdoll(void);
		void			Event_Dying(void);
		void			Event_Killed( const CTakeDamageInfo &info );
		int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
		int				OnTakeDamage_Dying( const CTakeDamageInfo &info );
		void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr);
		void			TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance );

		void			UpdateOnRemove( void );
		void			KillSprites( float flDelay );

		void			OnStateChange( NPC_STATE OldState, NPC_STATE NewState );


		Activity		NPC_TranslateActivity( Activity baseAct );
		virtual int		TranslateSchedule( int scheduleType );
		int				MeleeAttack1Conditions ( float flDot, float flDist );
		void			HandleAnimEvent( animevent_t *pEvent );

		bool			OverrideMove(float flInterval);
		void			MoveToTarget(float flInterval, const Vector &MoveTarget);
		void			MoveExecute_Alive(float flInterval);
		void			MoveExecute_Dead(float flInterval);
		int				MoveCollisionMask(void);

		void			TurnHeadRandomly( float flInterval );

		void			CrashTouch( CBaseEntity *pOther );

		void			StartEngine( bool fStartSound );

		virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true ) { return WorldSpaceCenter(); }

		void			CheckCollisions(float flInterval);
		virtual void	GatherEnemyConditions( CBaseEntity *pEnemy );
		void			PlayFlySound(void);
		virtual void	StopLoopingSounds(void);

		void			Precache(void);
		void			RunTask( const Task_t *pTask );
		void			Spawn(void);
		void			StartTask( const Task_t *pTask );

		int				SelectSchedule ( void );

		void			BladesInit();
		void			SoundInit( void );

		bool			HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt);

		void			PostNPCInit( void );

		void			PrescheduleThink( void );

		void			SpinBlades(float flInterval);

		void			Slice( CBaseEntity *pHitEntity, float flInterval, trace_t &tr );
		void			Bump( CBaseEntity *pHitEntity, float flInterval, trace_t &tr );
		void			Splash( const Vector &vecSplashPos );

		float			ManhackMaxSpeed( void );
		virtual void	VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
		void			HitPhysicsObject( CBaseEntity *pOther );
		virtual void	ClampMotorForces( Vector &linear, AngularImpulse &angular );

		
void			Ignite( float flFlameLifetime ) { return; }
 
		DEFINE_CUSTOM_AI;

		DECLARE_DATADESC();

	protected:

		bool IsFlyingActivity( Activity baseAct );

		//
		// Movement variables.
		//

		Vector			m_vForceVelocity;		// Someone forced me to move

		Vector			m_vTargetBanking;

		Vector			m_vForceMoveTarget;		// Will fly here
		float			m_fForceMoveTime;		// If time is less than this
		Vector			m_vSwarmMoveTarget;		// Will fly here
		float			m_fSwarmMoveTime;		// If time is less than this
		float			m_fEnginePowerScale;	// scale all thrust by this amount (usually 1.0!)

		float			m_flNextEngineSoundTime;
		float			m_flEngineStallTime;
		float			m_flIgnoreDamageTime;
		float			m_flNextBurstTime;
		float			m_flWaterSuspendTime;
		int				m_nLastSpinSound;

		// Death
		float			m_fSparkTime;
		float			m_fSmokeTime;

		bool			m_bDirtyPitch; // indicates whether we want the sound pitch updated.(sjb)
		bool			m_bWasClubbed;

		CSoundPatch		*m_pEngineSound1;
		CSoundPatch		*m_pEngineSound2;
		CSoundPatch		*m_pBladeSound;

		bool			m_bBladesActive;
		float			m_flBladeSpeed;

		int				m_iEyeAttachment;
		CSprite			*m_pEyeGlow;
		
		// Re-enable for light trails
		//CSpriteTrail	*m_pLightTrail;

		int				m_iLightAttachment;
		CSprite			*m_pLightGlow;

		int				m_iPanel1;
		int				m_iPanel2;
		int				m_iPanel3;
		int				m_iPanel4;
};

#endif	//NPC_MANHACK_H
