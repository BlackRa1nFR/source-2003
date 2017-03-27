//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_SquadSlot.h"
#include "AI_BaseNPC.h"
#include "AI_Navigator.h"
#include "ndebugoverlay.h"
#include "NPC_Roller.h"
#include "explode.h"
#include "bitstring.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "decals.h"
#include "antlion_dust.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "beam_shared.h"
#include "iservervehicle.h"
#include "soundemittersystembase.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vehicle_base.h"
#include "EventQueue.h"
#include "te_effect_dispatch.h"

#define ROLLERMINE_MAX_TORQUE_FACTOR	5
extern short g_sModelIndexWExplosion;

ConVar	sk_rollermine_shock( "sk_rollermine_shock","0");

ConVar	sk_rollermine_vehicle_intercept( "sk_rollermine_vehicle_intercept","1");

#define ROLLERMINE_IDLE_SEE_DIST					2048
#define ROLLERMINE_NORMAL_SEE_DIST					2048
#define ROLLERMINE_WAKEUP_DIST						256
#define ROLLERMINE_SEE_VEHICLESONLY_BEYOND_IDLE		300		// See every other than vehicles upto this distance (i.e. old idle see dist)
#define ROLLERMINE_SEE_VEHICLESONLY_BEYOND_NORMAL	800		// See every other than vehicles upto this distance (i.e. old normal see dist)

#define ROLLERMINE_MIN_ATTACK_DIST	1
#define ROLLERMINE_MAX_ATTACK_DIST	4096

#define	ROLLERMINE_OPEN_THRESHOLD	256

#define ROLLERMINE_VEHICLE_OPEN_THRESHOLD	400
#define ROLLERMINE_VEHICLE_HOP_THRESHOLD	300

#define ROLLERMINE_HOP_DELAY				2			// Don't allow hops faster than this

#define ROLLERMINE_THROTTLE_REDUCTION		0.2

#define ROLLERMINE_REQUIRED_TO_EXPLODE_VEHICLE		4

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLERMINE_RANGE_ATTACK1 = LAST_ROLLER_SCHED,
	SCHED_ROLLERMINE_CHASE_ENEMY,
	SCHED_ROLLERMINE_BURIED_WAIT,
	SCHED_ROLLERMINE_BURIED_UNBURROW,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLERMINE_CHARGE_ENEMY = LAST_ROLLER_TASK,
	TASK_ROLLERMINE_BURIED_WAIT,
	TASK_ROLLERMINE_UNBURROW,
};


// This are little 'sound event' flags. Set the flag after you play the
// sound, and the sound will not be allowed to play until the flag is then cleared.
#define ROLLERMINE_SE_CLEAR		0x00000000
#define ROLLERMINE_SE_CHARGE	0x00000001
#define ROLLERMINE_SE_TAUNT		0x00000002
#define ROLLERMINE_SE_SHARPEN	0x00000004
#define ROLLERMINE_SE_TOSSED	0x00000008

enum rollingsoundstate_t { ROLL_SOUND_NOT_READY = 0, ROLL_SOUND_OFF, ROLL_SOUND_CLOSED, ROLL_SOUND_OPEN };

//=========================================================
//=========================================================
class CNPC_RollerMine : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_RollerMine, CAI_BaseNPC );

public:

	~CNPC_RollerMine( void );

	void	Spawn( void );
	bool	CreateVPhysics();
	virtual const Vector& WorldAlignMins( ) const;
	virtual const Vector& WorldAlignMaxs( ) const;
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	void	SpikeTouch( CBaseEntity *pOther );
	void	ShockTouch( CBaseEntity *pOther );
	void	CloseTouch( CBaseEntity *pOther );
	void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void	Precache( void );
	void	OnPhysGunPickup( CBasePlayer *pPhysGunUser );
	void	OnPhysGunDrop( CBasePlayer *pPhysGunUser, bool wasLaunched );
	void	StopLoopingSounds( void );
	void	PrescheduleThink();
	bool	ShouldSavePhysics()	{ return true; }
	void	OnRestore();

	int		RangeAttack1Conditions ( float flDot, float flDist );
	int		SelectSchedule( void );

	bool	OverrideMove( float flInterval ) { return true; }
	bool	IsValidEnemy( CBaseEntity *pEnemy );
	bool	IsPlayerVehicle( CBaseEntity *pEntity );
	bool	IsShocking() { return gpGlobals->curtime < m_flShockTime ? true : false; }
	void	UpdateRollingSound();
	void	UpdatePingSound();
	void	StopRollingSound();
	void	StopPingSound();
	float	RollingSpeed();
	void	DrawDebugGeometryOverlays()
	{
		if (m_debugOverlays & OVERLAY_BBOX_BIT) 
		{
			float dist = GetSenses()->GetDistLook();
			Vector range(dist, dist, 64);
			NDebugOverlay::Box( GetAbsOrigin(), -range, range, 255, 0, 0, 0, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
	// UNDONE: Put this in the qc file!
	Vector EyePosition() 
	{
		Vector eye = GetAbsOrigin();
		eye.z += WorldAlignMaxs().z;
		return eye;
	}

	int		OnTakeDamage( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	Class_T	Classify() 
	{ 
		return m_bHeld ? CLASS_PLAYER_ALLY : CLASS_COMBINE; 
	}

	virtual bool ShouldGoToIdleState() 
	{ 
		return gpGlobals->curtime > m_flGoIdleTime ? true : false;
	}

	virtual	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	// Vehicle interception
	bool	EnemyInVehicle( void );
	Vector	VehicleVelocity( CBaseEntity *pVehicle );
	float	VehicleHeading( CBaseEntity *pVehicle );

	NPC_STATE SelectIdealState();

	// Vehicle sticking
	void		StickToVehicle( CBaseEntity *pOther );
	void		AnnounceArrivalToOthers( CBaseEntity *pOther );
	void		UnstickFromVehicle( void );
	CBaseEntity *GetVehicleStuckTo( void );
	void		InputConstraintBroken( inputdata_t &inputdata );
	void		InputRespondToChirp( inputdata_t &inputdata );
	void		InputRespondToExplodeChirp( inputdata_t &inputdata );

protected:
	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

	bool	BecomePhysical();
	void	WakeNeighbors();
	bool	WakeupMine( CAI_BaseNPC *pNPC );

	void	Open( void );
	void	Close( void );
	void	Explode( void );
	void	PreDetonate( void );
	void	Hop( float height );

	bool	IsActive() { return m_flActiveTime > gpGlobals->curtime ? false : true; }
	CSoundPatch					*m_pRollSound;
	CSoundPatch					*m_pPingSound;

	CRollerController			m_RollerController;
	IPhysicsMotionController	*m_pMotionController;

	float	m_flSeeVehiclesOnlyBeyond;
	float	m_flActiveTime;	//If later than the current time, this will force the mine to be active
	float	m_flChargeTime;
	float	m_flGoIdleTime;
	float	m_flShockTime;
	float	m_flForwardSpeed;
	int		m_iSoundEventFlags;
	rollingsoundstate_t m_rollingSoundState;

	bool	m_bIsOpen;
	bool	m_bHeld;		//Whether or not the player is holding the mine
	EHANDLE m_hVehicleStuckTo;
	float	m_flPreventUnstickUntil;
	float	m_flNextHop;
	bool	m_bStartBuried;
	bool	m_bBuried;
	bool	m_bIsPrimed;
	bool	m_wakeUp;

	// Constraint used to stick us to a vehicle
	IPhysicsConstraint *m_pConstraint;
};

LINK_ENTITY_TO_CLASS( npc_rollermine, CNPC_RollerMine );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CNPC_RollerMine )

	DEFINE_SOUNDPATCH( CNPC_RollerMine, m_pRollSound ),
	DEFINE_SOUNDPATCH( CNPC_RollerMine, m_pPingSound ),
	DEFINE_EMBEDDED( CNPC_RollerMine, m_RollerController ),
	DEFINE_PHYSPTR( CNPC_RollerMine, m_pMotionController ),

	DEFINE_FIELD( CNPC_RollerMine, m_flSeeVehiclesOnlyBeyond, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_RollerMine, m_flActiveTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerMine, m_flChargeTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerMine, m_flGoIdleTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerMine, m_flShockTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerMine, m_flForwardSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_RollerMine, m_bIsOpen, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerMine, m_bHeld, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerMine, m_hVehicleStuckTo, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_RollerMine, m_flPreventUnstickUntil, FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerMine, m_flNextHop, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_RollerMine, m_bIsPrimed, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerMine, m_iSoundEventFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_RollerMine, m_rollingSoundState, FIELD_INTEGER ),

	DEFINE_KEYFIELD( CNPC_RollerMine,	m_bStartBuried, FIELD_BOOLEAN, "StartBuried" ),
	DEFINE_FIELD( CNPC_RollerMine, m_bBuried, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerMine, m_wakeUp, FIELD_BOOLEAN ),

	DEFINE_PHYSPTR( CNPC_RollerMine, m_pConstraint ),

	DEFINE_INPUTFUNC( CNPC_RollerMine, FIELD_VOID, "ConstraintBroken", InputConstraintBroken ),
	DEFINE_INPUTFUNC( CNPC_RollerMine, FIELD_VOID, "RespondToChirp", InputRespondToChirp ),
	DEFINE_INPUTFUNC( CNPC_RollerMine, FIELD_VOID, "RespondToExplodeChirp", InputRespondToExplodeChirp ),

	// Function Pointers
	DEFINE_ENTITYFUNC( CNPC_RollerMine, SpikeTouch ),
	DEFINE_ENTITYFUNC( CNPC_RollerMine, ShockTouch ),
	DEFINE_ENTITYFUNC( CNPC_RollerMine, CloseTouch ),
	DEFINE_THINKFUNC( CNPC_RollerMine, Explode ),
	DEFINE_THINKFUNC( CNPC_RollerMine, PreDetonate ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_RollerMine::~CNPC_RollerMine( void )
{
	if ( m_pMotionController != NULL )
	{
		physenv->DestroyMotionController( m_pMotionController );
		m_pMotionController = NULL;
	}

	UnstickFromVehicle();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Precache( void )
{
	engine->PrecacheModel( "models/roller.mdl" );
	engine->PrecacheModel( "models/roller_spikes.mdl" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// NOTE: We're re-implementing these for the rollermine because it's troublesome
// It's actually a SOLID_VPHYSICS that uses AI navigation, and at the moment,
// it's the only one. Since its SOLID_VPHYSICS describes a sphere, it's reasonable
// to have WorldAlignMins/Maxs == EntitySpaceMins/Maxs
//-----------------------------------------------------------------------------
const Vector& CNPC_RollerMine::WorldAlignMins( ) const
{
	return EntitySpaceMins();
}

const Vector& CNPC_RollerMine::WorldAlignMaxs( ) const
{
	return EntitySpaceMaxs();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Spawn( void )
{
	Precache();

	SetSolid( SOLID_VPHYSICS );

	BaseClass::Spawn();

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD );

	m_pRollSound = NULL;

	m_bIsOpen = true;
	Close();
	
	m_flFieldOfView		= -1.0f;
	m_flForwardSpeed	= -1200;
	m_bloodColor		= DONT_BLEED;
	SetHullType(HULL_SMALL_CENTERED);
	SetHullSizeNormal();

	m_flActiveTime		= 0;

	m_bBuried = m_bStartBuried;
	if ( m_bStartBuried )
	{
		trace_t	tr;
		AI_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, MAX_TRACE_LENGTH ), MASK_NPCSOLID, &tr );

		// Move into the ground layer
		Vector	buriedPos = tr.endpos - ( Vector( 0, 0, GetHullHeight() ) * 0.65f );
		SetAbsOrigin( buriedPos );
		SetMoveType( MOVETYPE_NONE );

		SetSchedule( SCHED_ROLLERMINE_BURIED_WAIT );
	}

	NPCInit();

	m_takedamage = DAMAGE_EVENTS_ONLY;
	SetDistLook( ROLLERMINE_IDLE_SEE_DIST );
	m_flSeeVehiclesOnlyBeyond = ROLLERMINE_SEE_VEHICLESONLY_BEYOND_IDLE;

	//Suppress superfluous warnings from animation system
	m_flGroundSpeed = 20;
	m_NPCState		= NPC_STATE_NONE;

	m_rollingSoundState = ROLL_SOUND_OFF;

	m_pConstraint = NULL;
	m_hVehicleStuckTo = NULL;
	m_flPreventUnstickUntil = 0;
	m_flNextHop = 0;
}

bool CNPC_RollerMine::WakeupMine( CAI_BaseNPC *pNPC )
{
	if ( pNPC && pNPC->m_iClassname == m_iClassname && pNPC != this )
	{
		CNPC_RollerMine *pMine = dynamic_cast<CNPC_RollerMine *>(pNPC);
		if ( pMine )
		{
			if ( pMine->m_NPCState == NPC_STATE_IDLE )
			{
				pMine->m_wakeUp = false;
				pMine->m_IdealNPCState = NPC_STATE_ALERT;
				return true;
			}
		}
	}

	return false;
}


void CNPC_RollerMine::WakeNeighbors()
{
	if ( !m_wakeUp || !IsActive() )
		return;
	m_wakeUp = false;

	if ( m_pSquad )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			WakeupMine( pSquadMember );
		}
		return;
	}

	CBaseEntity *entityList[64];
	Vector range(ROLLERMINE_WAKEUP_DIST,ROLLERMINE_WAKEUP_DIST,64);
	int boxCount = UTIL_EntitiesInBox( entityList, ARRAYSIZE(entityList), GetAbsOrigin()-range, GetAbsOrigin()+range, FL_NPC );
	//NDebugOverlay::Box( GetAbsOrigin(), -range, range, 255, 0, 0, 64, 10.0 );
	int wakeCount = 0;
	while ( boxCount > 0 )
	{
		boxCount--;
		CAI_BaseNPC *pNPC = entityList[boxCount]->MyNPCPointer();
		if ( WakeupMine( pNPC ) )
		{
			wakeCount++;
			if ( wakeCount >= 2 )
				return;
		}
	}
}

void CNPC_RollerMine::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if ( NewState == NPC_STATE_IDLE )
	{
		SetDistLook( ROLLERMINE_IDLE_SEE_DIST );
		m_flDistTooFar = ROLLERMINE_IDLE_SEE_DIST;
		m_flSeeVehiclesOnlyBeyond = ROLLERMINE_SEE_VEHICLESONLY_BEYOND_IDLE;
		
		m_RollerController.m_vecAngular = vec3_origin;
		m_wakeUp = true;
	}
	else
	{
		if ( OldState == NPC_STATE_IDLE )
		{
			// wake the neighbors!
			WakeNeighbors();
		}
		SetDistLook( ROLLERMINE_NORMAL_SEE_DIST );
		m_flDistTooFar = ROLLERMINE_NORMAL_SEE_DIST;
		m_flSeeVehiclesOnlyBeyond = ROLLERMINE_SEE_VEHICLESONLY_BEYOND_NORMAL;
	}
	BaseClass::OnStateChange( OldState, NewState );
}

NPC_STATE CNPC_RollerMine::SelectIdealState( void )
{
	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_ENEMY_TOO_FAR ) )
		{
			ClearEnemyMemory();
			SetEnemy( NULL );
			m_IdealNPCState = NPC_STATE_ALERT;
			m_flGoIdleTime = gpGlobals->curtime + 10;
			return m_IdealNPCState;
		}
	}
	return BaseClass::SelectIdealState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::BecomePhysical( void )
{
	VPhysicsDestroyObject();

	RemoveSolidFlags( FSOLID_NOT_SOLID );
	Relink();

	//Setup the physics controller on the roller
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false );

	if ( pPhysicsObject == NULL )
		return false;

	m_pMotionController = physenv->CreateMotionController( &m_RollerController );
	m_pMotionController->AttachObject( pPhysicsObject );

	SetMoveType( MOVETYPE_VPHYSICS );

	return true;
}

void CNPC_RollerMine::OnRestore()
{
	BaseClass::OnRestore();
	if ( m_pMotionController )
	{
		m_pMotionController->SetEventHandler( &m_RollerController );
	}
}

bool CNPC_RollerMine::CreateVPhysics()
{
	if ( m_bBuried )
	{
		VPhysicsInitStatic();
		return true;
	}
	else
	{
		return BecomePhysical();
	}
}	

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerMine::RangeAttack1Conditions( float flDot, float flDist )
{
	if( HasCondition( COND_SEE_ENEMY ) == false )
		return COND_NONE;

	if ( EnemyInVehicle() )
		return COND_CAN_RANGE_ATTACK1;

	if( flDist > ROLLERMINE_MAX_ATTACK_DIST  )
		return COND_TOO_FAR_TO_ATTACK;
	
	if (flDist < ROLLERMINE_MIN_ATTACK_DIST )
		return COND_TOO_CLOSE_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerMine::SelectSchedule ( void )
{
	if ( m_bBuried )
	{
		if ( HasCondition(COND_NEW_ENEMY) || HasCondition(COND_LIGHT_DAMAGE) )
			return SCHED_ROLLERMINE_BURIED_UNBURROW;

		return SCHED_ROLLERMINE_BURIED_WAIT;
	}

	//If we're held, don't try and do anything
	if ( ( m_bHeld ) || !IsActive() || m_hVehicleStuckTo )
		return SCHED_ALERT_STAND;

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:

		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			return SCHED_ROLLERMINE_RANGE_ATTACK1;
		
		return SCHED_ROLLERMINE_CHASE_ENEMY;
		break;

	default:
		break;
	}

	return BaseClass::SelectSchedule();
}

#if 0
#define	ROLLERMINE_DETECTION_RADIUS		350
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::DetectedEnemyInProximity( void )
{
	CBaseEntity *pEnt = NULL;
	CBaseEntity *pBestEnemy = NULL;
	float		flBestDist = MAX_TRACE_LENGTH;

	while ( ( pEnt = gEntList.FindEntityInSphere( pEnt, GetAbsOrigin(), ROLLERMINE_DETECTION_RADIUS ) ) != NULL )
	{
		if ( IRelationType( pEnt ) != D_HT )
			continue;

		float distance = ( pEnt->GetAbsOrigin() - GetAbsOrigin() ).Length();
		
		if ( distance >= flBestDist )
			continue;

		pBestEnemy = pEnt;
		flBestDist = distance;
	}

	if ( pBestEnemy != NULL )
	{
		SetEnemy( pBestEnemy );
		return true;
	}

	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERMINE_UNBURROW:
		
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
			Relink();
			SetMoveType( MOVETYPE_NOCLIP );
			SetAbsVelocity( Vector( 0, 0, 256 ) );
			Open();

			trace_t	tr;
			AI_TraceLine( GetAbsOrigin()+Vector(0,0,1), GetAbsOrigin()-Vector(0,0,64), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction < 1.0f )
			{
				UTIL_CreateAntlionDust( tr.endpos + Vector(0,0,24), GetLocalAngles() );		
			}
		}

		return;
		break;

	case TASK_ROLLERMINE_BURIED_WAIT:
		if ( HasCondition( COND_SEE_ENEMY ) )
		{
			TaskComplete();
		}
		break;

	case TASK_STOP_MOVING:

		//Stop turning
		m_RollerController.m_vecAngular = vec3_origin;
		
		TaskComplete();
		return;
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

			if ( pPhysicsObject == NULL )
			{
				assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}

			pPhysicsObject->Wake();
		}
		break;

	case TASK_ROLLER_WAIT_FOR_PHYSICS:
		if( m_bIsOpen )
		{
			Close();
		}

		BaseClass::StartTask( pTask );
		break;

	case TASK_ROLLERMINE_CHARGE_ENEMY:
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			
			if ( pPhysicsObject == NULL )
			{
				assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}
			
			pPhysicsObject->Wake();

			m_flChargeTime = gpGlobals->curtime;
		}

		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERMINE_UNBURROW:
		{	
			Vector	vCenter = WorldSpaceCenter();

			trace_t	tr;
			AI_TraceEntity( this, vCenter, vCenter, MASK_NPCSOLID, &tr );

			if ( tr.fraction == 1 && tr.allsolid != 1 && (tr.startsolid != 1) )
			{
				if ( BecomePhysical() )
				{
					Hop( 256 );
					m_bBuried = false;
					TaskComplete();
					m_IdealNPCState = NPC_STATE_ALERT;
				}
			}
		}

		return;
		break;

	case TASK_ROLLERMINE_BURIED_WAIT:
		if ( HasCondition( COND_SEE_ENEMY ) || HasCondition( COND_LIGHT_DAMAGE ) )
		{
			TaskComplete();
		}
		break;

	case TASK_RUN_PATH:
	case TASK_WALK_PATH:

		if ( m_bHeld || m_hVehicleStuckTo )
		{
			TaskFail( "Player interrupted by grabbing" );
			break;
		}

		// Start turning early
		if( (GetLocalOrigin() - GetNavigator()->GetCurWaypointPos() ).Length() <= 64 )
		{
			if( GetNavigator()->CurWaypointIsGoal() )
			{
				// Hit the brakes a bit.
				float yaw = UTIL_VecToYaw( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() );
				Vector vecRight;
				AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

				m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, -m_flForwardSpeed * 5 );

				TaskComplete();
				return;
			}

			GetNavigator()->AdvancePath();	
		}

		{
			float yaw = UTIL_VecToYaw( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() );

			Vector vecRight;
			Vector vecToPath; // points at the path
			AngleVectors( QAngle( 0, yaw, 0 ), &vecToPath, &vecRight, NULL );

			// figure out if the roller is turning. If so, cut the throttle a little.
			float flDot;
			Vector vecVelocity;

			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			
			if ( pPhysicsObject == NULL )
			{
				assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}
			
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );

			VectorNormalize( vecVelocity );

			vecVelocity.z = 0;

			flDot = DotProduct( vecVelocity, vecToPath );

			m_RollerController.m_vecAngular = vec3_origin;

			if( flDot > 0.25 && flDot < 0.7 )
			{
				// Feed a little torque backwards into the axis perpendicular to the velocity.
				// This will help get rid of momentum that would otherwise make us overshoot our goal.
				Vector vecCompensate;

				vecCompensate.x = vecVelocity.y;
				vecCompensate.y = -vecVelocity.x;
				vecCompensate.z = 0;

				m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			}

			m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed );
		}
		break;

	case TASK_ROLLER_WAIT_FOR_PHYSICS:
		BaseClass::RunTask( pTask );
		break;

	case TASK_ROLLERMINE_CHARGE_ENEMY:
		{
			if ( !GetEnemy() )
			{
				TaskFail( FAIL_NO_ENEMY );
				break;
			}

			if ( m_bHeld || m_hVehicleStuckTo )
			{
				TaskComplete();
				break;
			}

			CBaseEntity *pEnemy = GetEnemy();
			Vector vecTargetPosition = pEnemy->GetAbsOrigin();

			// If we're chasing a vehicle, try and get ahead of it
			if ( EnemyInVehicle() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( pEnemy );
				Vector vecVehicleVelocity = VehicleVelocity( pPlayer->GetVehicle()->GetVehicleEnt() );
				Vector vecNewTarget = vecTargetPosition + (vecVehicleVelocity * 1.0) + Vector(0,0,64);

				// If we're closer to the projected point than the vehicle, use it
				if ( (GetAbsOrigin() - vecTargetPosition).LengthSqr() > (GetAbsOrigin() - vecNewTarget).LengthSqr() )
				{
					// Only use this position if it's clear
					if ( enginetrace->GetPointContents( vecTargetPosition ) == CONTENTS_EMPTY )
					{
						vecTargetPosition = vecNewTarget;
					}
				}
			}
			//NDebugOverlay::Box( vecTargetPosition, -Vector(20,20,20), Vector(20,20,20), 255,255,255, 0.1, 0.1 );

			float flTorqueFactor;
			float yaw = UTIL_VecToYaw( vecTargetPosition - GetLocalOrigin() );
			Vector vecRight;

			AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

			//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin() + (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()), 0,255,0, true, 0.1 );

			float flDot;

			// Figure out whether to continue the charge.
			// (Have I overrun the target?)			
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

			if ( pPhysicsObject == NULL )
			{
//				Assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}

			Vector vecVelocity, vecToTarget;

			pPhysicsObject->GetVelocity( &vecVelocity, NULL );
			VectorNormalize( vecVelocity );

			vecToTarget = vecTargetPosition - GetLocalOrigin();

			VectorNormalize( vecToTarget );

			flDot = DotProduct( vecVelocity, vecToTarget );

			// more torque the longer the roller has been going.
			flTorqueFactor = 1 + (gpGlobals->curtime - m_flChargeTime) * 2;

			if( flTorqueFactor < 1 )
			{
				flTorqueFactor = 1;
			}
			else if( flTorqueFactor > ROLLERMINE_MAX_TORQUE_FACTOR)
			{
				flTorqueFactor = ROLLERMINE_MAX_TORQUE_FACTOR;
			}

			Vector vecCompensate;

			vecCompensate.x = vecVelocity.y;
			vecCompensate.y = -vecVelocity.x;
			vecCompensate.z = 0;
			VectorNormalize( vecCompensate );

			m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed  * flTorqueFactor );
		
			// Taunt when I get closer
			if( !(m_iSoundEventFlags & ROLLERMINE_SE_TAUNT) && UTIL_DistApprox( GetLocalOrigin(), vecTargetPosition ) <= 400 )
			{
				m_iSoundEventFlags |= ROLLERMINE_SE_TAUNT; // Don't repeat.

				EmitSound( "NPC_RollerMine.Taunt" );
			}

			// Jump earlier when chasing a vehicle
			float flThreshold = ROLLERMINE_OPEN_THRESHOLD;
			if ( EnemyInVehicle() )
			{
				flThreshold = ROLLERMINE_VEHICLE_OPEN_THRESHOLD;
			}

			// Open the spikes if i'm close enough to cut the enemy!!
  			if( ( m_bIsOpen == false ) && ( ( UTIL_DistApprox( GetAbsOrigin(), vecTargetPosition ) <= flThreshold ) || !IsActive() ) )
			{
				Open();
			}
			else if ( m_bIsOpen )
			{
				float flDistance = UTIL_DistApprox( GetAbsOrigin(), GetEnemy()->GetAbsOrigin() );
				if ( flDistance >= flThreshold )
				{
					// Otherwise close them if the enemy is getting away!
					Close();
				}
				else if ( EnemyInVehicle() && flDistance < ROLLERMINE_VEHICLE_HOP_THRESHOLD )
				{
					// Keep trying to hop when we're ramming a vehicle, so we're visible to the player
					if ( vecVelocity.x != 0 && vecVelocity.y != 0 && flTorqueFactor > 3 && flDot > 0.0 )
					{
						Hop( 300 );
					}
				}
			}

			// If we drive past, close the blades and make a new plan.
			if ( !EnemyInVehicle() )
			{
				if( vecVelocity.x != 0 && vecVelocity.y != 0 )
				{
					if( gpGlobals->curtime - m_flChargeTime > 1.0 && flTorqueFactor > 1 &&  flDot < 0.0 )
					{
						if( m_bIsOpen )
						{
							Close();
						}

						TaskComplete();
					}
				}
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Open( void )
{
	if ( m_bIsOpen == false )
	{
		SetModel( "models/roller_spikes.mdl" );
		EmitSound( "NPC_RollerMine.OpenSpikes" );

		SetTouch( ShockTouch );
		m_bIsOpen = true;

		// Don't hop if we're constrained
		if ( !m_pConstraint )
		{
			if ( EnemyInVehicle() )
			{
				Hop( 256 );
			}
			else
			{
				Hop( 128 );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Close( void )
{
	if ( m_bIsOpen && !IsShocking() )
	{
		SetModel( "models/roller.mdl" );

		SetTouch( NULL );
		m_bIsOpen = false;

		m_iSoundEventFlags = ROLLERMINE_SE_CLEAR;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::SpikeTouch( CBaseEntity *pOther )
{
	if ( m_bHeld )
		return;

	// primed guys always blow up
	if ( !m_bIsPrimed )
	{
		Disposition_t disp = IRelationType(pOther);
		if ( ( pOther->m_iClassname == m_iClassname ) || (disp != D_HT && disp != D_FR) )
			return;
	}

	Explode();
	EmitSound( "NPC_RollerMine.Warn" );
}

void CNPC_RollerMine::CloseTouch( CBaseEntity *pOther )
{
	if ( IsShocking() )
		return;

	Disposition_t disp = IRelationType(pOther);

	if ( (disp == D_HT || disp == D_FR) )
	{
		ShockTouch( pOther );
		return;
	}
	Close();
}


bool CNPC_RollerMine::IsPlayerVehicle( CBaseEntity *pEntity )
{
	IServerVehicle *pVehicle = pEntity->GetServerVehicle();
	if ( pVehicle )
	{
		CBasePlayer *pPlayer = pVehicle->GetPassenger();
		if ( pPlayer )
		{
			Disposition_t disp = IRelationType(pPlayer);

			if ( disp == D_HT || disp == D_FR )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::ShockTouch( CBaseEntity *pOther )
{
	if ( m_bHeld || m_hVehicleStuckTo || gpGlobals->curtime < m_flShockTime )
		return;

	// error?
	Assert( !m_bIsPrimed );

	Disposition_t disp = IRelationType(pOther);
	if ( ( pOther->m_iClassname == m_iClassname ) || (disp != D_HT && disp != D_FR) )
		return;

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	// Calculate a collision force
	Vector impulse = GetAbsOrigin() - pOther->GetAbsOrigin();
	impulse.z = 0;
	VectorNormalize( impulse );
	impulse.z = 0.75;
	VectorNormalize( impulse );
	impulse *= 600;

	// jump up at a 30 degree angle away from the guy we hit
	SetTouch( CloseTouch );
	Vector vel;
	pPhysics->SetVelocity( &impulse, NULL );
	EmitSound( "NPC_RollerMine.Shock" );
	m_flShockTime = gpGlobals->curtime + 1.25;

	// Calculate physics force
	Vector out;
	CalcClosestPointOnAABB( pOther->GetAbsMins(), pOther->GetAbsMaxs(), GetAbsOrigin(), out );
	Vector vecForce = ( -impulse * pPhysics->GetMass() * 10 );
	CTakeDamageInfo	info( this, this, vecForce, out, sk_rollermine_shock.GetFloat(), DMG_SHOCK );
	pOther->TakeDamage( info );

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rollermine_shock.vmt", 4 );

	if ( pBeam != NULL )
	{
		pBeam->EntsInit( pOther, this );
		CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(pOther);
		if ( pAnimating )
		{
			int startAttach = pAnimating->LookupAttachment("beam_damage" );
			pBeam->SetStartAttachment( startAttach );
		}
		pBeam->SetEndAttachment( 1 );
		pBeam->SetWidth( 4 );
		pBeam->SetEndWidth( 4 );
		pBeam->SetBrightness( 255 );
		pBeam->SetColor( 255, 255, 255 );
		pBeam->LiveForTime( 0.5f );
		pBeam->RelinkBeam();
		pBeam->SetNoise( 3 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Make sure we don't keep hitting the same entity
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	if ( pEvent->deltaCollisionTime < 0.5 && (pOther == this) )
		return;

	BaseClass::VPhysicsCollision( index, pEvent );

	// If we've just hit a vehicle, we want to stick to it
	if ( m_bHeld || m_hVehicleStuckTo || !IsPlayerVehicle( pOther ) )
		return;

	StickToVehicle( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::StickToVehicle( CBaseEntity *pOther )
{
	IPhysicsObject *pOtherPhysics = pOther->VPhysicsGetObject();
	if ( !pOtherPhysics )
		return;

	// Don't stick to the wheels
	if ( pOtherPhysics->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL )
		return;

	// Destroy our constraint. This can happen if we had our constraint broken
	// and we still haven't cleaned up our constraint.
	UnstickFromVehicle();

	// We've hit the vehicle that the player's in.
	// Stick to it and slow it down.
	m_hVehicleStuckTo = pOther;

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	// Constrain us to the vehicle
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( pOtherPhysics, pPhysics );
	fixed.constraint.Defaults();
	fixed.constraint.forceLimit	= ImpulseScale( pPhysics->GetMass(), 100 );
	fixed.constraint.torqueLimit = 0;
	m_pConstraint = physenv->CreateFixedConstraint( pOtherPhysics, pPhysics, NULL, fixed );
	m_pConstraint->SetGameData( (void *)this );

	// Kick the vehicle so the player knows we've arrived
	Vector impulse = GetAbsOrigin() - pOther->GetAbsOrigin();
	impulse.z = 0;
	VectorNormalize( impulse );
	impulse.z = 0.75;
	VectorNormalize( impulse );
	impulse *= 600;
	Vector vecForce = -impulse * pPhysics->GetMass() * 10;
	pOtherPhysics->ApplyForceOffset( vecForce, GetAbsOrigin() );

	// Get the velocity at the point we're sticking to
	Vector vecVelocity;
	pOtherPhysics->GetVelocityAtPoint( GetAbsOrigin(), vecVelocity );
	AngularImpulse angNone( 0.0f, 0.0f, 0.0f );
	pPhysics->SetVelocity( &vecVelocity, &angNone );

	// Make sure we're spiky
	Open();

	// Reduce four wheeled vehicle's max speeds
	CPropVehicleDriveable *pVehicle = dynamic_cast<CPropVehicleDriveable *>(pOther);
	if ( pVehicle )
	{
		// Add a speed reduction to the vehicle.
		pVehicle->GetPhysics()->AddThrottleReduction( ROLLERMINE_THROTTLE_REDUCTION );
	}

	m_flPreventUnstickUntil = gpGlobals->curtime + 0.3;

	AnnounceArrivalToOthers( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Tell other rollermines on the vehicle that I've just arrived
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::AnnounceArrivalToOthers( CBaseEntity *pOther )
{
	// Now talk to any other rollermines stuck to the same vehicle
	CBaseEntity *entityList[64];
	Vector range(256,256,256);
	CUtlVector<CNPC_RollerMine*> aRollersOnVehicle;
	aRollersOnVehicle.AddToTail( this );
	int boxCount = UTIL_EntitiesInBox( entityList, ARRAYSIZE(entityList), GetAbsOrigin()-range, GetAbsOrigin()+range, FL_NPC );
	for ( int i = 0; i < boxCount; i++ )
	{
		CAI_BaseNPC *pNPC = entityList[i]->MyNPCPointer();
		if ( pNPC && pNPC->m_iClassname == m_iClassname && pNPC != this )
		{
			// Found another rollermine
			CNPC_RollerMine *pMine = dynamic_cast<CNPC_RollerMine*>(pNPC);
			Assert( pMine );

			// Is he stuck to the same vehicle?
			if ( pMine->GetVehicleStuckTo() == pOther )
			{
				aRollersOnVehicle.AddToTail( pMine );
			}
		}
	}

	// See if we've got enough rollers on the vehicle to start being mean
	int iRollers = aRollersOnVehicle.Count();
	if ( iRollers >= ROLLERMINE_REQUIRED_TO_EXPLODE_VEHICLE )
	{
		// Alert the others
		EmitSound( "NPC_RollerMine.ExplodeChirp" );

		// Tell everyone to explode shortly
		for ( i = 0; i < iRollers; i++ )
		{
			variant_t emptyVariant;
			g_EventQueue.AddEvent( aRollersOnVehicle[i], "RespondToExplodeChirp", RandomFloat(2,5), NULL, NULL );
		}
	}
	else if ( iRollers > 1 )
	{
		// Chirp to the others
		EmitSound( "NPC_RollerMine.Chirp" );

		// Tell the others to respond (skip first slot, because that's me)
		for ( i = 1; i < iRollers; i++ )
		{
			variant_t emptyVariant;
			g_EventQueue.AddEvent( aRollersOnVehicle[i], "RespondToChirp", RandomFloat(2,3), NULL, NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Physics system has just told us our constraint has been broken
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputConstraintBroken( inputdata_t &inputdata )
{
	// Prevent rollermines being dislodged right as they stick
	if ( m_flPreventUnstickUntil > gpGlobals->curtime )
		return;

	// We can't delete it here safely
	UnstickFromVehicle();
	Close();

	// dazed
	m_RollerController.m_vecAngular.Init();
	m_flActiveTime = gpGlobals->curtime + 3;
}

//-----------------------------------------------------------------------------
// Purpose: Respond to another rollermine that's chirped at us
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputRespondToChirp( inputdata_t &inputdata )
{
	EmitSound( "NPC_RollerMine.ChirpRespond" );
}

//-----------------------------------------------------------------------------
// Purpose: Respond to another rollermine's signal to detonate
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputRespondToExplodeChirp( inputdata_t &inputdata )
{
	EmitSound( "NPC_RollerMine.ExplodeChirpRespond" );

	Explode();
}

//-----------------------------------------------------------------------------
// Purpose: If we were stuck to a vehicle, remove ourselves
//-----------------------------------------------------------------------------
void CNPC_RollerMine::UnstickFromVehicle( void )
{
	if ( m_pConstraint )
	{
		// Get the vehicle we're stuck to, and if it's a four wheeled vehicle, remove our speed reduction
		CBaseEntity *pEntity = GetVehicleStuckTo();
		CPropVehicleDriveable *pVehicle = dynamic_cast<CPropVehicleDriveable *>(pEntity);
		if ( pVehicle )
		{
			// Remove our speed reduction to the vehicle.
			pVehicle->GetPhysics()->RemoveThrottleReduction( ROLLERMINE_THROTTLE_REDUCTION );
		}

		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}

	m_hVehicleStuckTo = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_RollerMine::GetVehicleStuckTo( void )
{
	if ( !m_pConstraint )
		return NULL;

	return m_hVehicleStuckTo;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::OnPhysGunPickup( CBasePlayer *pPhysGunUser )
{
	//Stop turning
	m_RollerController.m_vecAngular = vec3_origin;

	UnstickFromVehicle();

	m_bHeld = true;
	m_RollerController.Off();
	EmitSound( "NPC_RollerMine.Held" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::OnPhysGunDrop( CBasePlayer *pPhysGunUser, bool wasLaunched )
{
	m_bHeld = false;
	m_flActiveTime = gpGlobals->curtime + 1.0f;
	m_RollerController.On();
	
	// explode on contact if launched from the physgun
	if ( wasLaunched )
	{
		if ( m_bIsOpen )
		{
			m_bIsPrimed = true;
			SetTouch( SpikeTouch );
			// enable world/prop touch too
			VPhysicsGetObject()->SetCallbackFlags( VPhysicsGetObject()->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH|CALLBACK_GLOBAL_TOUCH_STATIC );
		}
		EmitSound( "NPC_RollerMine.Tossed" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : float
//-----------------------------------------------------------------------------
int CNPC_RollerMine::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !(info.GetDamageType() & DMG_BURN) )
	{
		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			AngularImpulse	angVel;
			angVel.Random( -400.0f, 400.0f );
			VPhysicsGetObject()->AddVelocity( NULL, &angVel );
			m_RollerController.m_vecAngular *= 0.8f;

			VPhysicsTakeDamage( info );
		}
		SetCondition( COND_LIGHT_DAMAGE );
	}

	if ( info.GetDamageType() & (DMG_BURN|DMG_BLAST) )
	{
		if ( info.GetAttacker() && info.GetAttacker()->m_iClassname != m_iClassname )
		{
			SetThink( PreDetonate );
			SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1f, 0.5f ) );
		}
		else
		{
			// dazed
			m_RollerController.m_vecAngular.Init();
			m_flActiveTime = gpGlobals->curtime + 3;
			Hop( 300 );
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the roller to hop into the air
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Hop( float height )
{
	if ( m_flNextHop > gpGlobals->curtime )
		return;

	if ( GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysObj = VPhysicsGetObject();
		pPhysObj->ApplyForceCenter( Vector(0,0,1) * height * pPhysObj->GetMass() );
		
		AngularImpulse	angVel;
		angVel.Random( -400.0f, 400.0f );
		pPhysObj->AddVelocity( NULL, &angVel );

		m_flNextHop = gpGlobals->curtime + ROLLERMINE_HOP_DELAY;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes warning noise before actual explosion occurs
//-----------------------------------------------------------------------------
void CNPC_RollerMine::PreDetonate( void )
{
	Hop( 300 );

	SetTouch( NULL );
	SetThink( Explode );
	SetNextThink( gpGlobals->curtime + 0.5f );

	EmitSound( "NPC_RollerMine.Hurt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Explode( void )
{
	m_takedamage = DAMAGE_NO;

	//FIXME: Hack to make thrown mines more deadly and fun
	float expDamage = m_bIsPrimed ? 100 : 25;

	// Underwater explosion?
	if ( UTIL_PointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		CEffectData	data;
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = expDamage;
		data.m_flScale = 128;
		data.m_fFlags = ( SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE );
		DispatchEffect( "WaterSurfaceExplosion", data );
	}
	else
	{
		ExplosionCreate( WorldSpaceCenter(), GetLocalAngles(), this, expDamage, 128, true );
	}

	CTakeDamageInfo	info( this, this, 1, DMG_GENERIC );
	Event_Killed( info );

	// Remove myself a frame from now to avoid doing it in the middle of running AI
	SetThink( SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

const float MAX_ROLLING_SPEED = 720;

float CNPC_RollerMine::RollingSpeed()
{
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( !m_hVehicleStuckTo && !m_bHeld && pPhysics && !pPhysics->IsAsleep() )
	{
		AngularImpulse angVel;
		pPhysics->GetVelocity( NULL, &angVel );
		float rollingSpeed = angVel.Length() - 90;
		rollingSpeed = clamp( rollingSpeed, 1, MAX_ROLLING_SPEED );
		rollingSpeed *= (1/MAX_ROLLING_SPEED);
		return rollingSpeed;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::PrescheduleThink()
{
	// Are we underwater?
	if ( UTIL_PointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		// As soon as we're far enough underwater, detonate
		Vector vecAboveMe = GetAbsOrigin() + Vector(0,0,64);
		if ( UTIL_PointContents( vecAboveMe ) & MASK_WATER )
		{
			Explode();
			return;
		}
	}

	UpdateRollingSound();
	UpdatePingSound();
	BaseClass::PrescheduleThink();
}

void CNPC_RollerMine::UpdateRollingSound()
{
	if ( m_rollingSoundState == ROLL_SOUND_NOT_READY )
		return;

	rollingsoundstate_t soundState = ROLL_SOUND_OFF;
	float rollingSpeed = RollingSpeed();
	if ( rollingSpeed > 0 )
	{
		soundState = m_bIsOpen ? ROLL_SOUND_OPEN : ROLL_SOUND_CLOSED;
	}


	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CSoundParameters params;
	switch( soundState )
	{
	case ROLL_SOUND_CLOSED:
		CBaseEntity::GetParametersForSound( "NPC_RollerMine.Roll", params );
		break;
	case ROLL_SOUND_OPEN:
		CBaseEntity::GetParametersForSound( "NPC_RollerMine.RollWithSpikes", params );
		break;

	case ROLL_SOUND_OFF:
		// no sound
		break;
	}

	// start the new sound playing if necessary
	if ( m_rollingSoundState != soundState )
	{
		StopRollingSound();

		m_rollingSoundState = soundState;

		if ( m_rollingSoundState == ROLL_SOUND_OFF )
			return;

		CPASAttenuationFilter filter( this );
		m_pRollSound = controller.SoundCreate( filter, entindex(), params.channel, params.soundname, params.soundlevel );
		controller.Play( m_pRollSound, params.volume, params.pitch );
		m_rollingSoundState = soundState;
	}

	if ( m_pRollSound )
	{
		// for tuning
		//Msg("SOUND: %s, VOL: %.1f\n", m_rollingSoundState == ROLL_SOUND_CLOSED ? "CLOSED" : "OPEN ", rollingSpeed );
		controller.SoundChangePitch( m_pRollSound, params.pitchlow + (params.pitchhigh - params.pitchlow) * rollingSpeed, 0.1 );
		controller.SoundChangeVolume( m_pRollSound, params.volume * rollingSpeed, 0.1 );
	}
}


void CNPC_RollerMine::StopRollingSound()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRollSound );
	m_pRollSound = NULL;
}

void CNPC_RollerMine::UpdatePingSound()
{
	float pingSpeed = 0;
	if ( m_bIsOpen && !IsShocking() && !m_bHeld )
	{
		CBaseEntity *pEnemy = GetEnemy();
		if ( pEnemy )
		{
			pingSpeed = EnemyDistance( pEnemy );
			pingSpeed = clamp( pingSpeed, 1, ROLLERMINE_OPEN_THRESHOLD );
			pingSpeed *= (1.0f/ROLLERMINE_OPEN_THRESHOLD);
		}
	}

	if ( pingSpeed > 0 )
	{
		pingSpeed = 1-pingSpeed;
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CSoundParameters params;
		CBaseEntity::GetParametersForSound( "NPC_RollerMine.Ping", params );
		if ( !m_pPingSound )
		{
			CPASAttenuationFilter filter( this );
			m_pPingSound = controller.SoundCreate( filter, entindex(), params.channel, params.soundname, params.soundlevel );
			controller.Play( m_pPingSound, params.volume, 101 );
		}

		controller.SoundChangePitch( m_pPingSound, params.pitchlow + (params.pitchhigh - params.pitchlow) * pingSpeed, 0.1 );
		controller.SoundChangeVolume( m_pPingSound, params.volume, 0.1 );
		//Msg("PING: %.1f\n", pingSpeed );

	}
	else
	{
		StopPingSound();
	}
}


void CNPC_RollerMine::StopPingSound()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pPingSound );
	m_pPingSound = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::StopLoopingSounds( void )
{
	StopRollingSound();
	StopPingSound();
	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::IsValidEnemy( CBaseEntity *pEnemy )
{
	// If the enemy's over the vehicle detection range, and it's not a player in a vehicle, ignore it
	if ( pEnemy )
	{
		float flDistance = GetAbsOrigin().DistTo( pEnemy->GetAbsOrigin() );
		if ( flDistance >= m_flSeeVehiclesOnlyBeyond )
		{
			if ( pEnemy->IsPlayer() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( pEnemy );
				if ( pPlayer->IsInAVehicle() )
				{
					// If we're buried, we only care when they're heading directly towards us.
					if ( m_bBuried )
						return ( VehicleHeading( pPlayer->GetVehicle()->GetVehicleEnt() ) > DOT_20DEGREE );
						
					// If we're not buried, chase him as long as he's not heading away from us
					return ( VehicleHeading( pPlayer->GetVehicle()->GetVehicleEnt() ) > 0 );
				}
			}

			return false;
		}
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_RollerMine::VehicleVelocity( CBaseEntity *pVehicle )
{
	AngularImpulse angVel;
	Vector vecVelocity;

	// If it doesn't have a physics model, just use standard movement
	IPhysicsObject *pVehiclePhysics = pVehicle->VPhysicsGetObject();
	if ( !pVehiclePhysics )
	{
		vecVelocity = GetAbsVelocity();
	}
	else
	{
		pVehiclePhysics->GetVelocity( &vecVelocity, &angVel );
	}

	return vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::EnemyInVehicle( void )
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( pEnemy && pEnemy->IsPlayer() && ((CBasePlayer *)pEnemy)->IsInAVehicle() )
		return ( sk_rollermine_vehicle_intercept.GetInt() > 0 );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_RollerMine::VehicleHeading( CBaseEntity *pVehicle )
{
	Vector vecVelocity = VehicleVelocity( pVehicle );
	float flSpeed = VectorNormalize( vecVelocity );
	Vector vecToMine = GetAbsOrigin() - pVehicle->GetAbsOrigin();
	VectorNormalize( vecToMine );

	// If it's not moving, consider it moving towards us, but not directly
	// This will enable already active rollers to chase the vehicle if it's stationary.
	if ( flSpeed < 10 )
		return 0.1;

	return DotProduct( vecVelocity, vecToMine );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecDir - 
//			*ptr - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_BULLET )
	{
		CTakeDamageInfo newInfo( info );
		newInfo.SetDamageForce( info.GetDamageForce() * 20 );
		BaseClass::TraceAttack( newInfo, vecDir, ptr );
		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_rollermine, CNPC_RollerMine )

	//Tasks
	DECLARE_TASK( TASK_ROLLERMINE_CHARGE_ENEMY )
	DECLARE_TASK( TASK_ROLLERMINE_BURIED_WAIT )
	DECLARE_TASK( TASK_ROLLERMINE_UNBURROW )

	//Schedules
	//==================================================
	// SCHED_ANTLION_CHASE_ENEMY_BURROW
	//==================================================

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_BURIED_WAIT,

		"	Tasks"
		"		TASK_ROLLERMINE_BURIED_WAIT		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_BURIED_UNBURROW,

		"	Tasks"
		"		TASK_ROLLERMINE_UNBURROW		0"
		"	"
		"	Interrupts"
	)
	
	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ROLLERMINE_CHARGE_ENEMY	0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_TOO_FAR"
	)
	
	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ROLLERMINE_RANGE_ATTACK1"
		"		TASK_SET_TOLERANCE_DISTANCE		24"
		"		TASK_GET_PATH_TO_ENEMY			0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_ENEMY_TOO_FAR"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_TASK_FAILED"
	)

AI_END_CUSTOM_NPC()
