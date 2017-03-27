#ifndef NPC_BARNEY_H
#define NPC_BARNEY_H

#include "hl1_npc_talker.h"

//=========================================================
//=========================================================
class CNPC_Barney : public CHL1NPCTalker
{
	DECLARE_CLASS( CNPC_Barney, CHL1NPCTalker );
public:
	
	DECLARE_DATADESC();

	virtual void ModifyOrAppendCriteria( AI_CriteriaSet& set );

	void	Precache( void );
	void	Spawn( void );
	void	TalkInit( void );

	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );

	int		GetSoundInterests ( void );
	Class_T  Classify ( void );
	void	AlertSound( void );
	void    SetYawSpeed ( void );

	bool    CheckRangeAttack1 ( float flDot, float flDist );
	void    BarneyFirePistol ( void );
	
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	Event_Killed( const CTakeDamageInfo &info );

	void    PainSound ( void );
	void	DeathSound( void );

	void	HandleAnimEvent( animevent_t *pEvent );
	int		TranslateSchedule( int scheduleType );
	int		SelectSchedule( void );

	void	DeclineFollowing( void );

	int		RangeAttack1Conditions( float flDot, float flDist );

	NPC_STATE CNPC_Barney::SelectIdealState ( void );

	bool	m_fGunDrawn;
	float	m_flPainTime;
	float	m_flCheckAttackTime;
	bool	m_fLastAttackCheck;
	
	int		m_iAmmoType;

	enum
	{
		SCHED_BARNEY_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_BARNEY_ENEMY_DRAW,
		SCHED_BARNEY_FACE_TARGET,
		SCHED_BARNEY_IDLE_STAND,
		SCHED_BARNEY_STOP_FOLLOWING,
	};

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_BARNEY_H