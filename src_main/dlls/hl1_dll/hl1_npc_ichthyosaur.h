#ifndef NPC_ICHTHYOSAUR_H
#define NPC_ICHTHYOSAUR_H


#include "hl1_ai_basenpc.h"

#define SEARCH_RETRY	16

#define ICHTHYOSAUR_SPEED 150

#define EYE_MAD		0
#define EYE_BASE	1
#define EYE_CLOSED	2
#define EYE_BACK	3
#define EYE_LOOK	4


//
// CNPC_Ichthyosaur
//

class CNPC_Ichthyosaur : public CHL1BaseNPC
{
	DECLARE_CLASS( CNPC_Ichthyosaur, CHL1BaseNPC );
public:

	void	Precache( void );
	void	Spawn( void );
	Class_T Classify ( void );
	void	NPCThink ( void );
	void	Swim ( void );
	void	StartTask(const Task_t *pTask);
	void	RunTask( const Task_t *pTask );
	int		MeleeAttack1Conditions ( float flDot, float flDist );
	void	BiteTouch( CBaseEntity *pOther );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		TranslateSchedule( int scheduleType );
//	int		SelectSchedule( void ) ;

	Vector  DoProbe( const Vector &Probe );
	bool    ProbeZ( const Vector &position, const Vector &probe, float *pFraction);

	float	GetGroundSpeed ( void );

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

private:
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];

	Vector m_SaveVelocity;
	float m_idealDist;

	float m_flBlink;

	float m_flEnemyTouched;
	bool  m_bOnAttack;

	float m_flMaxSpeed;
	float m_flMinSpeed;
	float m_flMaxDist;

	float m_flNextAlert;

	CBeam *m_pBeam;

	//Save the info from that run
	Vector m_vecLastMoveTarget;
	bool m_bHasMoveTarget;
};

LINK_ENTITY_TO_CLASS( monster_ichthyosaur, CNPC_Ichthyosaur );

BEGIN_DATADESC( CNPC_Ichthyosaur )
	// Function Pointers
	DEFINE_ENTITYFUNC( CNPC_Ichthyosaur, BiteTouch ),
END_DATADESC()


#endif //NPC_ICHTHYOSAUR_H