#include "npc_roller.h"
#include "soundenvelope.h"

//=========================================================
//=========================================================
class CNPC_RollerBuddy : public CNPC_Roller
{
	DECLARE_CLASS( CNPC_RollerBuddy, CNPC_Roller );
	DECLARE_DATADESC();

public:
	void Spawn( void );
	void GatherConditions( void );
	void PrescheduleThink( void );

	void FindMaster( void );

	void Precache( void );

	DEFINE_CUSTOM_AI;

	virtual int	SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	int m_iMode;

	Class_T	Classify( void ) { return CLASS_CONSCRIPT; }

public:
	void SetMaster( CBasePlayer *pPlayer );
	void ToggleBuddyMode( bool fAcknowledge );
	void CommandMoveToLocation( const Vector &vecLocation, CBaseEntity *pCommandEnt );

	void PowerOn( void );
	void PowerOff( void );

private:
	Vector m_vecCommandLocation;
	EHANDLE m_hCommandEntity;
	CBasePlayer *m_pMaster;
};

