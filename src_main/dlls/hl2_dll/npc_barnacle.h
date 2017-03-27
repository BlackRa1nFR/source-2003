//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef NPC_BARNACLE_H
#define NPC_BARNACLE_H
#ifdef _WIN32
#pragma once
#endif

#include "AI_BaseNPC.h"
#include "studio.h"

#define BARNACLE_PULL_SPEED			8
#define BARNACLE_KILL_VICTIM_DELAY	5 // how many seconds after pulling prey in to gib them. 

// Tongue
#define BARNACLE_TONGUE_POINTS	8

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BARNACLE_AE_PUKEGIB	2
#define	BARNACLE_AE_BITE	3

//-----------------------------------------------------------------------------
// Purpose: This is the entity we place at the top & bottom of the tongue, to create a vphysics spring
//-----------------------------------------------------------------------------
class CBarnacleTongueTip : public CBaseAnimating
{
	DECLARE_CLASS( CBarnacleTongueTip, CBaseAnimating );
public:
	DECLARE_DATADESC();

	~CBarnacleTongueTip( void )
	{
		physenv->DestroySpring( m_pSpring );
	}

	void	Spawn( void );
	void	Precache( void );

	bool						CreateSpring( CBaseAnimating *pTongueRoot );
	static CBarnacleTongueTip	*CreateTongueTip( CBaseAnimating *pTongueRoot, const Vector &vecOrigin, const QAngle &vecAngles );
	static CBarnacleTongueTip	*CreateTongueRoot( const Vector &vecOrigin, const QAngle &vecAngles );

public:
	IPhysicsSpring			*m_pSpring;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Barnacle : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Barnacle, CAI_BaseNPC );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_Barnacle();
	~CNPC_Barnacle();

	void			Spawn( void );
	void			Activate( void );
	void			InitTonguePosition( void );
	void			Precache( void );
	CBaseEntity*	TongueTouchEnt ( float *pflLength );
	Class_T			Classify ( void );
	void			SetObjectCollisionBox( void );
	void			HandleAnimEvent( animevent_t *pEvent );
	void			BarnacleThink ( void );
	void			WaitTillDead ( void );
	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			LiftPrey( void );
	void			SwallowPrey( void );
	void			AttachTongueToTarget( CBaseEntity *pTouchEnt, Vector vecGrabPos );
	void			RemoveRagdoll( bool bDestroyRagdoll );
	void			LostPrey( bool bRemoveRagdoll );
	void			UpdateTongue( void );
	void			BitePrey( void );

	CNetworkVar( float, m_flAltitude );
	int				m_cGibs;				// barnacle loads up on gibs each time it kills something.
	bool			m_fTongueExtended;
	bool			m_bLiftingPrey;			// true when the prey's on the tongue and being lifted to the mouth
	bool			m_bSwallowingPrey;		// if it's a human, true while the barnacle chews it and swallows it whole. 
	float			m_flDigestFinish;		// time at which we've finished digesting something we chewed
	float			m_flVictimHeight;
	int				m_iGrabbedBoneIndex;
	bool			m_bPlayedPullSound;

	// Tongue spline points
	CNetworkVar( Vector, m_vecRoot );
	CNetworkVar( Vector, m_vecTip );

private:
	// Tongue tip & root
	CHandle<CBarnacleTongueTip>			m_hTongueRoot;
	CHandle<CBarnacleTongueTip>			m_hTongueTip;
	CHandle<CRagdollProp>				m_hRagdoll;
	matrix3x4_t							m_pRagdollBones[MAXSTUDIOBONES];
	IPhysicsConstraint					*m_pConstraint;

	DEFINE_CUSTOM_AI;
};

#endif // NPC_BARNACLE_H
