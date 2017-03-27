//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Large vehicle what delivers combine troops.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_BaseNPC.h"
#include "soundenvelope.h"
#include "CBaseHelicopter.h"
#include "AI_Schedule.h"
#include "engine/IEngineSound.h"
#include "smoke_trail.h"
#include "IEffects.h"
#include "props.h"
#include "TemplateEntities.h"
#include "baseanimating.h"
#include "AI_Senses.h"
#include "entitylist.h"
#include "ammodef.h"
#include "ndebugoverlay.h"
#include "npc_combines.h"
#include "soundent.h"
#include "mapentities.h"

#define DROPSHIP_ACCEL_RATE				300

// Timers
#define DROPSHIP_DEPLOY_TIME			2		// Time after dropping off troops before lifting off
#define DROPSHIP_TIME_BETWEEN_MINES		1.0f
#define DROPSHIP_PAUSE_B4_TROOP_UNLOAD	2.0f

// Special actions
#define DROPSHIP_TROOP_GRID				64
#define DROPSHIP_DEFAULT_SOLDIERS		4

// Movement
#define DROPSHIP_BUFF_TIME				0.3f
#define DROPSHIP_MAX_LAND_TILT			2.5f
#define DROPSHIP_CONTAINER_HEIGHT		130.0f
#define DROPSHIP_MAX_SPEED				(60 * 17.6) // 120 miles per hour.

// Pathing data
#define	DROPSHIP_LEAD_DISTANCE			800.0f
#define	DROPSHIP_MIN_CHASE_DIST_DIFF	128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it
#define	DROPSHIP_AVOID_DIST				256.0f

#define	CRATE_BBOX_MIN		(Vector( -100, -80, -60 ))
#define CRATE_BBOX_MAX		(Vector( 100, 80, 80 ))

enum DROP_STATES {
	DROP_IDLE = 0,
	DROP_NEXT,
};

enum CRATE_TYPES {
	CRATE_ROLLER_HOPPER = 0,
	CRATE_SOLDIER,
	CRATE_NONE,
};

//=====================================
// Animation Events
//=====================================
#define AE_DROPSHIP_RAMP_OPEN	1		// the tailgate is open.

//=====================================
// Custom activities
//=====================================
// Without Cargo
Activity ACT_DROPSHIP_FLY_IDLE;			// Flying. Vertical aspect 
Activity ACT_DROPSHIP_FLY_IDLE_EXAGG;	// Exaggerated version of the flying idle
// With Cargo
Activity ACT_DROPSHIP_FLY_IDLE_CARGO;	// Flying. Vertical aspect 
Activity ACT_DROPSHIP_DESCEND_IDLE;		// waiting to touchdown
Activity ACT_DROPSHIP_DEPLOY_IDLE;		// idle on the ground with door open. Troops are leaving.
Activity ACT_DROPSHIP_LIFTOFF;			// transition back to FLY IDLE

enum
{
	LANDING_NO = 0,

	// Dropoff
	LANDING_LEVEL_OUT,		// Heading to a point above the dropoff point
	LANDING_DESCEND,		// Descending from to the dropoff point
	LANDING_TOUCHDOWN,
	LANDING_UNLOADING,
	LANDING_UNLOADED,
	LANDING_LIFTOFF,

	// Pickup
	LANDING_SWOOPING,		// Swooping down to the target
};


//=============================================================================
//=============================================================================
class CNPC_CombineDropship : public CBaseHelicopter
{
	DECLARE_CLASS( CNPC_CombineDropship, CBaseHelicopter );

	~CNPC_CombineDropship();

	// Setup
	void	Spawn( void );
	void	Precache( void );

	void	Activate( void );

	// Thinking/init
	void	InitializeRotorSound( void );
	void	StopLoopingSounds();
	void	PrescheduleThink( void );

	// Flight/sound
	void	Flight( void );
	virtual void OnReachedTarget( CBaseEntity *pTarget );
	float	GetAltitude( void );
	void	DoRotorWash( void );
	void	UpdateRotorSoundPitch( int iPitch );
	void	UpdatePickupNavigation( void );
	void	CalculateSoldierCount( int iSoldiers );

	// Combat
	void	SpawnTroops( void );
	void	DoCombatStuff( void );
	void	DropMine( void );
	void	FireCannonRound( void );
	void	StartCannon( void );
	void	StopCannon( void );
	void	Hunt( void );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo ) { return 0; };	// don't die

	// Input handlers.
	void	InputLandLeave( inputdata_t &inputdata );
	void	InputLandTake( inputdata_t &inputdata );
	void	InputDropMines( inputdata_t &inputdata );
	void	InputPickup( inputdata_t &inputdata );
	void	LandCommon( void );

	Class_T Classify( void ) { return CLASS_COMBINE; }

private:
	// Timers
	float	m_flTimeTakeOff;
	float	m_flDropDelay;			// delta between each mine
	float	m_flTroopDeployPause;
	float	m_flTimeNextAttack;
	float	m_flLastTime;
	
	// States and counters
	int		m_iMineCount;		// index for current mine # being deployed
	int		m_totalMinesToDrop;	// total # of mines to drop as a group (based upon triggered input)
	int		m_soldiersToDrop;
	int		m_iDropState;
	bool	m_bDropMines;		// signal to drop mines
	int		m_iLandState; 
	float	m_engineThrust;		// for tracking sound volume/pitch
	bool	m_bIsFiring;
	float	m_existPitch;
	float	m_existRoll;
	bool	m_leaveCrate;
	int		m_iCrateType;
	float	m_flLandingSpeed;

	QAngle	m_vecAngAcceleration;
	
	// Misc Vars
	EHANDLE		m_hContainer;
	EHANDLE		m_hPickupTarget;
	int			m_iContainerMoveType;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	// Strings
	string_t m_sNPCTemplate;
	string_t m_sNPCTemplateData;	

	// Sounds
	CSoundPatch		*m_pCannonSound;

	// Outputs
	COutputEvent	m_OnFinishedDropoff;
	COutputEvent	m_OnFinishedPickup;
};

LINK_ENTITY_TO_CLASS( npc_combinedropship, CNPC_CombineDropship );

BEGIN_DATADESC( CNPC_CombineDropship )

	DEFINE_FIELD( CNPC_CombineDropship, m_flTimeTakeOff, FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineDropship, m_flDropDelay, FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineDropship, m_flTroopDeployPause, FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineDropship, m_flTimeNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineDropship, m_flLastTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineDropship, m_iMineCount, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineDropship, m_totalMinesToDrop, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineDropship, m_soldiersToDrop, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineDropship, m_iDropState, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineDropship, m_bDropMines, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineDropship, m_iLandState, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineDropship, m_engineThrust, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineDropship, m_bIsFiring, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineDropship, m_existPitch, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineDropship, m_existRoll, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineDropship, m_leaveCrate, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( CNPC_CombineDropship, m_iCrateType, FIELD_INTEGER, "CrateType" ),
	DEFINE_FIELD( CNPC_CombineDropship, m_flLandingSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineDropship, m_vecAngAcceleration,FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CombineDropship, m_hContainer, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_CombineDropship, m_hPickupTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_CombineDropship, m_iContainerMoveType, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineDropship, m_sNPCTemplateData, FIELD_STRING ),
	DEFINE_KEYFIELD( CNPC_CombineDropship, m_sNPCTemplate, FIELD_STRING,	"NPCTemplate" ),
	DEFINE_SOUNDPATCH( CNPC_CombineDropship, m_pCannonSound ),
	
	DEFINE_INPUTFUNC( CNPC_CombineDropship, FIELD_INTEGER, "LandLeaveCrate", InputLandLeave ),
	DEFINE_INPUTFUNC( CNPC_CombineDropship, FIELD_INTEGER, "LandTakeCrate", InputLandTake ),
	DEFINE_INPUTFUNC( CNPC_CombineDropship, FIELD_INTEGER, "DropMines", InputDropMines ),
	DEFINE_INPUTFUNC( CNPC_CombineDropship, FIELD_STRING, "Pickup", InputPickup ),
	
	DEFINE_OUTPUT( CNPC_CombineDropship, m_OnFinishedDropoff, "OnFinishedDropoff" ),
	DEFINE_OUTPUT( CNPC_CombineDropship, m_OnFinishedPickup, "OnFinishedPickup" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose : Destructor
//------------------------------------------------------------------------------
CNPC_CombineDropship::~CNPC_CombineDropship(void)
{
	if ( m_hContainer )
	{
		UTIL_Remove( m_hContainer );		// get rid of container
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after spawning on map load or on a load from save game.
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::Activate( void )
{
	BaseClass::Activate();
	
	// check for valid template
	if (!m_sNPCTemplateData)
	{
		//
		// This must be the first time we're activated, not a load from save game.
		// Look up the template in the template database.
		//
		if (!m_sNPCTemplate)
		{
			Warning( "npc_template_maker %s has no template NPC!\n", GetEntityName() );
			return;
		}
		else
		{
			m_sNPCTemplateData = Templates_FindByTargetName(STRING(m_sNPCTemplate));
			if ( m_sNPCTemplateData == NULL_STRING )
			{
				Warning( "Template NPC %s not found!\n", STRING(m_sNPCTemplate) );
				return;
			}
		}
	}

}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Spawn( void )
{
	Precache( );

	SetModel( "models/combine_dropship.mdl" );
	BaseClass::Spawn();

	ExtractBbox( SelectHeaviestSequence( ACT_DROPSHIP_DEPLOY_IDLE ), m_cullBoxMins, m_cullBoxMaxs ); 
	BaseClass::Spawn();

	InitPathingData( DROPSHIP_LEAD_DISTANCE, DROPSHIP_MIN_CHASE_DIST_DIFF, DROPSHIP_AVOID_DIST );

	// create the correct bin for the ship to carry
	switch ( m_iCrateType )
	{
	case CRATE_ROLLER_HOPPER:
		break;
	case CRATE_SOLDIER:
		m_hContainer = CreateEntityByName( "prop_dynamic" );
		if ( m_hContainer )
		{
			m_hContainer->SetModel( "models/props_junk/trashdumpster02.mdl" );
			m_hContainer->SetLocalOrigin( GetAbsOrigin() );
			m_hContainer->SetLocalAngles( GetLocalAngles() );
			m_hContainer->SetAbsAngles( GetAbsAngles() );
			m_hContainer->SetParent(this, 0);
			m_hContainer->SetOwnerEntity(this);
			m_hContainer->SetSolid( SOLID_VPHYSICS );
			m_hContainer->Spawn();
		}
		break;

	case CRATE_NONE:
	default:
		break;
	}

	m_iHealth = 100;
	m_flFieldOfView = 0.5; // 60 degrees
	m_iContainerMoveType = MOVETYPE_NONE;

	//InitBoneControllers();
	InitCustomSchedules();

	if ( m_hContainer )
	{
		SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_CARGO );
	}
	else
	{
		SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_EXAGG );
	}

	m_flMaxSpeed = DROPSHIP_MAX_SPEED;
	m_flMaxSpeedFiring = BASECHOPPER_MAX_FIRING_SPEED;
	m_hPickupTarget = NULL;

	//!!!HACKHACK
	// This tricks the AI code that constantly complains that the vehicle has no schedule.
	SetSchedule( SCHED_IDLE_STAND );

	m_iLandState = LANDING_NO;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Precache( void )
{
	// Models
	engine->PrecacheModel("models/combine_dropship.mdl");
	engine->PrecacheModel("models/props_junk/trashdumpster02.mdl");

	// Sounds
	enginesound->PrecacheSound("npc/combine_gunship/engine_whine.wav");
	enginesound->PrecacheSound("npc/combine_gunship/gunship_fire.wav");

	BaseClass::Precache();

}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Flight( void )
{
	// Only run pose params in some flight states
	bool bRunPoseParams = ( m_iLandState == LANDING_NO || 
							m_iLandState == LANDING_LEVEL_OUT || 
							m_iLandState == LANDING_LIFTOFF ||
							m_iLandState == LANDING_SWOOPING );

	if ( bRunPoseParams )
	{
		if( GetFlags() & FL_ONGROUND )
		{
			//This would be really bad.
			RemoveFlag( FL_ONGROUND );
		}

		// NDebugOverlay::Line(GetLocalOrigin(), GetDesiredPosition(), 0,0,255, true, 0.1);

		Vector deltaPos = GetDesiredPosition() - GetLocalOrigin();

		// calc desired acceleration
		float dt = 1.0f;

		Vector	accel;
		float	accelRate = DROPSHIP_ACCEL_RATE;
		float	maxSpeed = m_flMaxSpeed;

		if ( m_lifeState == LIFE_DYING )
		{
			accelRate *= 5.0;
			maxSpeed *= 5.0;
		}

		float flDist = min( GetAbsVelocity().Length() + accelRate, maxSpeed );

		// Only decelerate to our goal if we're going to hit it
		if ( deltaPos.Length() > flDist * dt )
		{
			float scale = flDist * dt / deltaPos.Length();
			deltaPos = deltaPos * scale;
		}
		
		// If we're swooping, floor it
		if ( m_iLandState == LANDING_SWOOPING )
		{
			VectorNormalize( deltaPos );
			deltaPos *= maxSpeed;
		}
		
		// calc goal linear accel to hit deltaPos in dt time.
		accel.x = 2.0 * (deltaPos.x - GetAbsVelocity().x * dt) / (dt * dt);
		accel.y = 2.0 * (deltaPos.y - GetAbsVelocity().y * dt) / (dt * dt);
		accel.z = 2.0 * (deltaPos.z - GetAbsVelocity().z * dt + 0.5 * 384 * dt * dt) / (dt * dt);
		
		//NDebugOverlay::Line(GetLocalOrigin(), GetLocalOrigin() + deltaPos, 255,0,0, true, 0.1);
		//NDebugOverlay::Line(GetLocalOrigin(), GetLocalOrigin() + accel, 0,255,0, true, 0.1);

		// don't fall faster than 0.2G or climb faster than 2G
		if ( m_iLandState != LANDING_SWOOPING )
		{
			accel.z = clamp( accel.z, 384 * 0.2, 384 * 2.0 );
		}

		Vector forward, right, up;
		GetVectors( &forward, &right, &up );

		Vector goalUp = accel;
		VectorNormalize( goalUp );

		// calc goal orientation to hit linear accel forces
		float goalPitch = RAD2DEG( asin( DotProduct( forward, goalUp ) ) );
		float goalYaw = UTIL_VecToYaw( m_vecDesiredFaceDir );
		float goalRoll = RAD2DEG( asin( DotProduct( right, goalUp ) ) );

		// clamp goal orientations
		goalPitch = clamp( goalPitch, -45, 60 );
		goalRoll = clamp( goalRoll, -45, 45 );

		// calc angular accel needed to hit goal pitch in dt time.
		dt = 0.6;
		QAngle goalAngAccel;
		goalAngAccel.x = 2.0 * (AngleDiff( goalPitch, AngleNormalize( GetLocalAngles().x ) ) - GetLocalAngularVelocity().x * dt) / (dt * dt);
		goalAngAccel.y = 2.0 * (AngleDiff( goalYaw, AngleNormalize( GetLocalAngles().y ) ) - GetLocalAngularVelocity().y * dt) / (dt * dt);
		goalAngAccel.z = 2.0 * (AngleDiff( goalRoll, AngleNormalize( GetLocalAngles().z ) ) - GetLocalAngularVelocity().z * dt) / (dt * dt);

		goalAngAccel.x = clamp( goalAngAccel.x, -300, 300 );
		//goalAngAccel.y = clamp( goalAngAccel.y, -60, 60 );
		goalAngAccel.y = clamp( goalAngAccel.y, -120, 120 );
		goalAngAccel.z = clamp( goalAngAccel.z, -300, 300 );

		// limit angular accel changes to simulate mechanical response times
		dt = 0.1;
		QAngle angAccelAccel;
		angAccelAccel.x = (goalAngAccel.x - m_vecAngAcceleration.x) / dt;
		angAccelAccel.y = (goalAngAccel.y - m_vecAngAcceleration.y) / dt;
		angAccelAccel.z = (goalAngAccel.z - m_vecAngAcceleration.z) / dt;

		angAccelAccel.x = clamp( angAccelAccel.x, -1000, 1000 );
		angAccelAccel.y = clamp( angAccelAccel.y, -1000, 1000 );
		angAccelAccel.z = clamp( angAccelAccel.z, -1000, 1000 );

		m_vecAngAcceleration += angAccelAccel * 0.1;

		// Msg( "pitch %6.1f (%6.1f:%6.1f)  ", goalPitch, GetLocalAngles().x, m_vecAngVelocity.x );
		// Msg( "roll %6.1f (%6.1f:%6.1f) : ", goalRoll, GetLocalAngles().z, m_vecAngVelocity.z );
		// Msg( "%6.1f %6.1f %6.1f  :  ", goalAngAccel.x, goalAngAccel.y, goalAngAccel.z );
		// Msg( "%6.0f %6.0f %6.0f\n", angAccelAccel.x, angAccelAccel.y, angAccelAccel.z );

		ApplySidewaysDrag( right );
		ApplyGeneralDrag();
		
		QAngle angVel = GetLocalAngularVelocity();
		angVel += m_vecAngAcceleration * 0.1;

		//angVel.y = clamp( angVel.y, -60, 60 );
		//angVel.y = clamp( angVel.y, -120, 120 );
		angVel.y = clamp( angVel.y, -120, 120 );

		SetLocalAngularVelocity( angVel );

		m_flForce = m_flForce * 0.8 + (accel.z + fabs( accel.x ) * 0.1 + fabs( accel.y ) * 0.1) * 0.1 * 0.2;

		Vector vecImpulse = m_flForce * up;
		
		if ( m_lifeState == LIFE_DYING )
		{
			vecImpulse.z = -38.4;  // 64ft/sec
		}
		else
		{
			vecImpulse.z -= 38.4;  // 32ft/sec
		}
		
		// Find our acceleration direction
		Vector	vecAccelDir = vecImpulse;
		VectorNormalize( vecAccelDir );

		// Find our current velocity
		Vector	vecVelDir = GetAbsVelocity();
		VectorNormalize( vecVelDir );

		// Level out our plane of movement
		vecAccelDir.z	= 0.0f;
		vecVelDir.z		= 0.0f;
		forward.z		= 0.0f;
		right.z			= 0.0f;

		// Find out how "fast" we're moving in relation to facing and acceleration
		float speed = m_flForce * DotProduct( vecVelDir, vecAccelDir );// * DotProduct( forward, vecVelDir );

		// Use the correct pose params
		char *sBodyAccel;
		char *sBodySway;
		if ( m_hContainer || m_iLandState == LANDING_SWOOPING )
		{
			sBodyAccel = "cargo_body_accel";
			sBodySway = "cargo_body_sway";
			SetPoseParameter( "body_accel", 0 );
			SetPoseParameter( "body_sway", 0 );
		}
		else
		{
			sBodyAccel = "body_accel";
			sBodySway = "body_sway";
			SetPoseParameter( "cargo_body_accel", 0 );
			SetPoseParameter( "cargo_body_sway", 0 );
		}

		// Apply the acceleration blend to the fins
		float finAccelBlend = SimpleSplineRemapVal( speed, -60, 60, -1, 1 );
		float curFinAccel = GetPoseParameter( sBodyAccel );
		
		curFinAccel = UTIL_Approach( finAccelBlend, curFinAccel, 0.5f );

		SetPoseParameter( sBodyAccel, curFinAccel );

		speed = m_flForce * DotProduct( vecVelDir, right );

		// Apply the spin sway to the fins
		float finSwayBlend = SimpleSplineRemapVal( speed, -60, 60, -1, 1 );
		float curFinSway = GetPoseParameter( sBodySway );

		curFinSway = UTIL_Approach( finSwayBlend, curFinSway, 0.5f );
		SetPoseParameter( sBodySway, curFinSway );

		// Add in our velocity pulse for this frame
		ApplyAbsVelocityImpulse( vecImpulse );

		//Msg("FinAccel: %f, Finsway: %f\n", curFinAccel, curFinSway );
	}
	else
	{
		SetPoseParameter( "body_accel", 0 );
		SetPoseParameter( "body_sway", 0 );
		SetPoseParameter( "cargo_body_accel", 0 );
		SetPoseParameter( "cargo_body_sway", 0 );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::OnReachedTarget( CBaseEntity *pTarget )
{
	if ( pTarget )
	{
		variant_t emptyVariant;
		pTarget->AcceptInput( "InPass", this, this, emptyVariant, 0 );
	}
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter( this );
	m_pRotorSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, "npc/combine_gunship/engine_whine.wav", 0.45 );
	m_pCannonSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, "npc/combine_gunship/gunship_fire.wav", 0.2  );

	controller.Play( m_pCannonSound, 0.0, 100 );

	m_engineThrust = 1.0f;

	BaseClass::InitializeRotorSound();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::StopLoopingSounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pCannonSound );
	m_pCannonSound = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSoldiers - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::CalculateSoldierCount( int iSoldiers )
{
	m_soldiersToDrop = iSoldiers;
	if ( m_soldiersToDrop < 0 || m_soldiersToDrop > 6 )
	{
		m_soldiersToDrop = DROPSHIP_DEFAULT_SOLDIERS;
	}
}

//------------------------------------------------------------------------------
// Purpose : Leave crate being carried
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputLandLeave( inputdata_t &inputdata )
{
	CalculateSoldierCount( inputdata.value.Int() );
	m_leaveCrate = true;
	LandCommon();
}

//------------------------------------------------------------------------------
// Purpose : Take crate being carried to next point
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputLandTake( inputdata_t &inputdata )
{
	CalculateSoldierCount( inputdata.value.Int() );
	m_leaveCrate = false;
	LandCommon();
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::LandCommon( void )
{
	// If we don't have a crate, we're not able to land
	if ( !m_hContainer )
		return;

	//Msg( "Landing\n" );

	m_iLandState = LANDING_LEVEL_OUT;
	UTIL_SetSize( this, CRATE_BBOX_MIN, CRATE_BBOX_MAX );
	SetLocalAngularVelocity( vec3_angle );
}

//------------------------------------------------------------------------------
// Purpose : Drop mine inputs... done this way so generic path_corners can be used
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputDropMines( inputdata_t &inputdata )
{
	m_totalMinesToDrop = inputdata.value.Int();
	if ( m_totalMinesToDrop >= 1 )	// catch bogus values being passed in
	{
		m_bDropMines = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pick up a specified object
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputPickup( inputdata_t &inputdata )
{
	// Can't pickup if we're already carrying something
	if ( m_hContainer )
	{
		Warning("npc_combinedropship %s was told to pickup, but is already carrying something.\n", STRING(GetEntityName()) );
		return;
	}

	string_t iszTargetName = inputdata.value.StringID();
	if ( iszTargetName == NULL_STRING )
	{
		Warning("npc_combinedropship %s tried to pickup with no specified pickup target.\n", STRING(GetEntityName()) );
		return;
	}
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, iszTargetName, NULL );
	if ( !pTarget )
	{
		Warning("npc_combinedropship %s couldn't find pickup target named %s\n", STRING(GetEntityName()), STRING(iszTargetName) );
		return;
	}

	// Start heading to the point
	m_hPickupTarget = pTarget;
	// Disable collisions to my target
	m_hPickupTarget->SetOwnerEntity(this);
	if ( m_NPCState == NPC_STATE_IDLE )
	{
		SetState( NPC_STATE_ALERT );
	}
	m_iLandState = LANDING_SWOOPING;
	m_flLandingSpeed = GetAbsVelocity().Length();
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	// keep track of think time deltas for burn calc below
	float dt = gpGlobals->curtime - m_flLastTime;
	m_flLastTime = gpGlobals->curtime;

	switch( m_iLandState )
	{
	case LANDING_NO:
		{
			if ( IsActivityFinished() && (GetActivity() != ACT_DROPSHIP_FLY_IDLE_EXAGG && GetActivity() != ACT_DROPSHIP_FLY_IDLE_CARGO) )
			{
				if ( m_hContainer )
				{
					SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_CARGO );
				}
				else
				{
					SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_EXAGG );
				}
			}

			DoRotorWash();
		}
		break;

	case LANDING_LEVEL_OUT:
		{
			// Approach the drop point
			Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
			float flDistance = vecToTarget.Length();

			// If we're slowing, make it look like we're slowing
			/*
			if ( IsActivityFinished() && GetActivity() != ACT_DROPSHIP_DESCEND_IDLE )
			{
				SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
			}
			*/

			// Are we there yet?
			float flSpeed = GetAbsVelocity().Length();
			if ( flDistance < 70 && flSpeed < 100 )
			{
				m_flLandingSpeed = flSpeed;
				m_iLandState = LANDING_DESCEND;
				// save off current angles so we can work them out over time
				QAngle angles = GetLocalAngles();
				m_existPitch = angles.x;
				m_existRoll = angles.z;

			}

			DoRotorWash();
		}
		break;

	case LANDING_DESCEND:
		{
			float	flAltitude;

			SetLocalAngularVelocity( vec3_angle );

			// Ensure we land on the drop point
			Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
			float flDistance = vecToTarget.Length();
			float flRampedSpeed = m_flLandingSpeed * (flDistance / 70);
			Vector vecVelocity = (flRampedSpeed / flDistance) * vecToTarget;
			vecVelocity.z = -75;
			SetAbsVelocity( vecVelocity );

			flAltitude = GetAltitude();			

			if ( IsActivityFinished() && GetActivity() != ACT_DROPSHIP_DESCEND_IDLE )
			{
				SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
			}

			if ( flAltitude < 72 )
			{
				QAngle angles = GetLocalAngles();

				// Level out quickly.
				angles.x = UTIL_Approach( 0.0, angles.x, 0.2 );
				angles.z = UTIL_Approach( 0.0, angles.z, 0.2 );

				SetLocalAngles( angles );
			}
			else
			{
				// randomly move as if buffeted by ground effects
				// gently flatten ship from starting pitch/yaw
				m_existPitch = UTIL_Approach( 0.0, m_existPitch, 1 );
				m_existRoll = UTIL_Approach( 0.0, m_existRoll, 1 );

				QAngle angles = GetLocalAngles();
				angles.x = m_existPitch + ( sin( gpGlobals->curtime * 3.5f ) * DROPSHIP_MAX_LAND_TILT );
				angles.z = m_existRoll + ( sin( gpGlobals->curtime * 3.75f ) * DROPSHIP_MAX_LAND_TILT );
				SetLocalAngles( angles );

				// figure out where to face (nav point)
				Vector targetDir = GetDesiredPosition() - GetAbsOrigin();
//				NDebugOverlay::Cross3D( m_pGoalEnt->GetAbsOrigin(), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 20 );

				QAngle targetAngles = GetAbsAngles();
				targetAngles.y += UTIL_AngleDiff(UTIL_VecToYaw( targetDir ), targetAngles.y);
				// orient ship towards path corner on the way down
				angles = GetAbsAngles();
				angles.y = UTIL_Approach(targetAngles.y, angles.y, 2 );
				SetAbsAngles( angles );
			}

			if ( flAltitude <= 0.5f )
			{
				m_iLandState = LANDING_TOUCHDOWN;

				// upon landing, make sure ship is flat
				QAngle angles = GetLocalAngles();
				angles.x = 0;
				angles.z = 0;
				SetLocalAngles( angles );

				// TODO: Release cargo anim
				SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
			}

			DoRotorWash();

			// place danger sounds 1 foot above ground to get troops to scatter if they are below dropship
			Vector vecBottom = GetAbsOrigin();
			vecBottom.z += WorldAlignMins().z;
			Vector vecSpot = vecBottom + Vector(0, 0, -1) * (GetAltitude() - 12 );
			CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.2, this, 0 );
			CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, vecSpot, 400, 0.2, this, 1 );
//			NDebugOverlay::Cross3D( vecSpot, -Vector(4,4,4), Vector(4,4,4), 255, 0, 255, false, 10.0f );

			// now check to see if player is below us, if so, cause heat damage to them (i.e. get them to move)
			trace_t tr;
			Vector vecBBoxMin = CRATE_BBOX_MIN;		// use flat box for check
			vecBBoxMin.z = -5;
			Vector vecBBoxMax = CRATE_BBOX_MAX;
			vecBBoxMax.z = 5;
			Vector pEndPoint = vecBottom + Vector(0, 0, -1) * ( GetAltitude() - 12 );
			AI_TraceHull( vecBottom, pEndPoint, vecBBoxMin, vecBBoxMax, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction < 1.0f )
			{
				if ( tr.GetEntityIndex() == 1 )			// player???
				{
					CTakeDamageInfo info( this, this, 20 * dt, DMG_BURN );
					CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
					pPlayer->TakeDamage( info );
				}
			}

		}
		break;

	case LANDING_TOUCHDOWN:
		{
			if ( IsActivityFinished() && ( GetActivity() != ACT_DROPSHIP_DESCEND_IDLE ) )
			{
				SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
			}

			m_iLandState = LANDING_UNLOADING;
			m_flTroopDeployPause = gpGlobals->curtime + DROPSHIP_PAUSE_B4_TROOP_UNLOAD;
			m_flTimeTakeOff = m_flTroopDeployPause + DROPSHIP_DEPLOY_TIME;
		}
		break;

	case LANDING_UNLOADING:
		{
			// pause before dropping troops
			if ( gpGlobals->curtime > m_flTroopDeployPause )
			{
				if ( m_hContainer )	// don't drop troops if we don't have a crate any more
				{
					SpawnTroops();
					m_flTroopDeployPause = m_flTimeTakeOff + 2;	// only drop once
				}
			}
			// manage engine wash and volume
			if ( m_flTimeTakeOff - gpGlobals->curtime < 0.5f )
			{
				m_engineThrust = UTIL_Approach( 1.0f, m_engineThrust, 0.1f );
				DoRotorWash();
			}
			else
			{
				float idleVolume = 0.2f;
				m_engineThrust = UTIL_Approach( idleVolume, m_engineThrust, 0.04f );
				if ( m_engineThrust > idleVolume ) 
				{
					DoRotorWash();				// make sure we're kicking up dust/water as long as engine thrust is up
				}
			}

			if( gpGlobals->curtime > m_flTimeTakeOff )
			{
				m_iLandState = LANDING_LIFTOFF;
				SetActivity( (Activity)ACT_DROPSHIP_LIFTOFF );
				m_engineThrust = 1.0f;			// ensure max volume once we're airborne
				if ( m_bIsFiring )
				{
					StopCannon();				// kill cannon sounds if they are on
				}

				// detach container from ship
				if ( m_hContainer && m_leaveCrate )
				{
					m_hContainer->SetParent(NULL);
					m_hContainer->SetMoveType( (MoveType_t)m_iContainerMoveType );

					// If the container has a physics object, remove it's shadow
					IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
					if ( pPhysicsObject )
					{
						pPhysicsObject->RemoveShadowController();
					}

					m_hContainer = NULL;
				}
			}
		}
		break;

	case LANDING_LIFTOFF:
		{
			// give us some clearance before changing back to larger hull -- keeps ship from getting stuck on
			// things like the player, etc since we "pop" the hull...
			if ( GetAltitude() > 120 )		
			{
				m_OnFinishedDropoff.FireOutput( this, this );

				m_iLandState = LANDING_NO;

				// change bounding box back to normal ship hull
				Vector vecBBMin, vecBBMax;
				ExtractBbox( SelectHeaviestSequence( ACT_DROPSHIP_DEPLOY_IDLE ), vecBBMin, vecBBMax ); 
				UTIL_SetSize( this, vecBBMin, vecBBMax );
				Relink();
			}
		}
		break;

	case LANDING_SWOOPING:
		{
			// Did we lose our pickup target?
			if ( !m_hPickupTarget )
			{
				m_iLandState = LANDING_NO;
			}
			else
			{
				// Decrease altitude and speed to hit the target point.
				Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
				float flDistance = vecToTarget.Length();

				// Start cheating when we get near it
				if ( flDistance < 50 )
				{
					/*
					if ( flDistance > 10 )
					{
						// Cheat and ensure we touch the target
						float flSpeed = GetAbsVelocity().Length();
						Vector vecVelocity = vecToTarget;
						VectorNormalize( vecVelocity );
						SetAbsVelocity( vecVelocity * min(flSpeed,flDistance) );
					}
					else
					*/
					{
						// Grab the target
						m_hContainer = m_hPickupTarget;
						m_hPickupTarget = NULL;
						m_iContainerMoveType = m_hContainer->GetMoveType();

						// If the container has a physics object, move it to shadow
						IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
						if ( pPhysicsObject )
						{
							pPhysicsObject->SetShadow( Vector(1e4,1e4,1e4), AngularImpulse(1e4,1e4,1e4), false, false );
							pPhysicsObject->UpdateShadow( GetAbsOrigin(), GetAbsAngles(), false, 0 );
						}

						int iIndex = 0;//LookupAttachment("Cargo");
						/*
						Vector vecOrigin;
						QAngle vecAngles;
						GetAttachment( iIndex, vecOrigin, vecAngles );
						m_hContainer->SetAbsOrigin( vecOrigin );
						m_hContainer->SetAbsAngles( vec3_angle );
						*/
						m_hContainer->SetAbsOrigin( GetAbsOrigin() );
						m_hContainer->SetParent(this, iIndex);
						m_hContainer->SetMoveType( MOVETYPE_PUSH );
						m_hContainer->RemoveFlag( FL_ONGROUND );
						m_hContainer->Relink();
						m_hContainer->SetAbsAngles( vec3_angle );

						m_OnFinishedPickup.FireOutput( this, this );
						m_iLandState = LANDING_NO;
					}
				}
			}

			DoRotorWash();
		}
		break;
	}

	DoCombatStuff();

	if ( GetActivity() != GetIdealActivity() )
	{
		//Msg( "setactivity" );
		SetActivity( GetIdealActivity() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define DROPSHIP_WASH_ALTITUDE 1024.0

void CNPC_CombineDropship::DoRotorWash( void )
{
	Vector	vecForward;
	GetVectors( &vecForward, NULL, NULL );

	Vector vecRotorHub = GetAbsOrigin() + vecForward * -64;

	DrawRotorWash( DROPSHIP_WASH_ALTITUDE, vecRotorHub );
}


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------

void CNPC_CombineDropship::SpawnTroops( void )
{
	int				i;
//	char			szAttachmentName[ 32 ];
	Vector			vecLocation;
	QAngle			vecAngles;
	QAngle			vecSpawnAngles;

//	memset( szAttachmentName, 0, 32 );

	vecSpawnAngles = GetLocalAngles();
	vecSpawnAngles.y = UTIL_AngleMod( vecSpawnAngles.y - 180 );
	vecSpawnAngles.x = 0;
	vecSpawnAngles.z = 0;

	for( i = 1 ; i <= m_soldiersToDrop ; i++ )
	{

//		Q_snprintf( szAttachmentName,sizeof(szAttachmentName), "spot%d", i );
//		GetAttachment( szAttachmentName, vecLocation, vecAngles );

		vecLocation = GetAbsOrigin();
		vecAngles = GetAbsAngles();

		// troops spawn behind vehicle at all times
		Vector shipDir, shipLeft;
		AngleVectors( vecAngles, &shipDir, &shipLeft, NULL );
		vecLocation -= shipDir * 250;

		// set spawn position for spawning in formation
		switch( i )
		{
		case 1:
			vecLocation -= shipLeft * DROPSHIP_TROOP_GRID;
			break;
		case 3:
			vecLocation += shipLeft * DROPSHIP_TROOP_GRID;
			break;
		case 4:
			vecLocation -= shipDir * DROPSHIP_TROOP_GRID - shipLeft * DROPSHIP_TROOP_GRID;
			break;
		case 5:
			vecLocation -= shipDir * DROPSHIP_TROOP_GRID;
			break;
		case 6:
			vecLocation -= shipDir * DROPSHIP_TROOP_GRID + shipLeft * DROPSHIP_TROOP_GRID;
		}

		// spawn based upon template
		CAI_BaseNPC	*pEnt = NULL;
		CBaseEntity *pEntity = NULL;
		MapEntity_ParseEntity( pEntity, STRING(m_sNPCTemplateData), NULL );
		if ( pEntity != NULL )
		{
			pEnt = (CAI_BaseNPC *)pEntity;
		}
		else
		{
			Warning("Dropship could not create template NPC\n" );
			return;
		}

		pEnt->SetLocalOrigin( vecLocation );
		pEnt->SetLocalAngles( vecSpawnAngles );
		DispatchSpawn( pEnt );

		pEnt->m_NPCState = NPC_STATE_IDLE;
		pEnt->SetOwnerEntity( this );
		pEnt->Activate();
	}
}


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CNPC_CombineDropship::GetAltitude( void )
{
	trace_t tr;
	Vector vecBottom = GetAbsOrigin();

	// Uneven terrain causes us problems, so trace our box down
	AI_TraceEntity( this, vecBottom, vecBottom - Vector(0,0,4096), MASK_SHOT, &tr );

	float flAltitude = ( 4096 * tr.fraction );
	//Msg(" Altitude: %.3f\n", flAltitude );
	return flAltitude;
}


//-----------------------------------------------------------------------------
// Purpose: Drop rollermine from dropship
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::DropMine( void )
{
	CBaseEntity *pMine = CreateEntityByName("npc_rollermine");

	if ( pMine->pev )
	{
		pMine->SetAbsOrigin( GetAbsOrigin() );
		pMine->Spawn();
	}
	else
	{
		Warning( "NULL Ent in Rollermine Create!\n" );
	}
}

//------------------------------------------------------------------------------
// Purpose : Fly towards our pickup target
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdatePickupNavigation( void )
{
	Vector vecPickup = m_hPickupTarget->WorldSpaceCenter();
	SetDesiredPosition( vecPickup );

	//NDebugOverlay::Cross3D( GetDesiredPosition(), -Vector(32,32,32), Vector(32,32,32), 0, 255, 255, true, 0.1f );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Hunt( void )
{
	// If we have a pickup target, fly to it
	if ( m_hPickupTarget )
	{
		UpdatePickupNavigation();
	}
	else if ( m_iLandState == LANDING_NO )
	{
		UpdateTrackNavigation();
	}

	// look for enemy
	GetSenses()->Look( 4092 );
	ChooseEnemy();

	// don't face player ever, only face nav points
	Vector desiredDir = GetDesiredPosition() - GetAbsOrigin();
	VectorNormalize( desiredDir ); 
	// Face our desired position.
	m_vecDesiredFaceDir = desiredDir;

	Flight();

	UpdatePlayerDopplerShift( );

}
//-----------------------------------------------------------------------------
// Purpose: do all of the stuff related to having an enemy, attacking, etc.
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::DoCombatStuff( void )
{
	// Handle mines
	if ( m_bDropMines )
	{
		switch( m_iDropState )
		{
		case DROP_IDLE:
			{
				m_iMineCount = m_totalMinesToDrop - 1;

				DropMine();
				// setup next individual drop time
				m_flDropDelay = gpGlobals->curtime + DROPSHIP_TIME_BETWEEN_MINES;
				// get ready to drop next mine, unless we're only supposed to drop 1
				if ( m_iMineCount )
				{
					m_iDropState = DROP_NEXT;
				}
				else
				{
					m_bDropMines = false;		// no more...
				}
				break;
			}
		case DROP_NEXT:
			{
				if ( gpGlobals->curtime > m_flDropDelay )		// time to drop next mine?
				{
					DropMine();
					m_flDropDelay = gpGlobals->curtime + DROPSHIP_TIME_BETWEEN_MINES;

					m_iMineCount--;
					if ( !m_iMineCount )
					{
						m_iDropState = DROP_IDLE;
						m_bDropMines = false;		// reset flag
					}
				}
				break;
			}
		}
	}

#if 0
	// Handle Guns -- fire only as cover fire when deploying troops
	if( HasCondition( COND_SEE_ENEMY) && gpGlobals->curtime > m_flTimeNextAttack && 
		( m_iLandState == LANDING_UNLOADING || m_iLandState == LANDING_DESCEND || m_iLandState == LANDING_TOUCHDOWN ) )
	{
		FireCannonRound();
	}
	else
	{
		if ( m_bIsFiring )
		{
			StopCannon();
		}
	}
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateRotorSoundPitch( int iPitch )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	float rotorPitch = 0.2 + m_engineThrust * 0.8;
	if ( m_pRotorSound )
	{
		controller.SoundChangePitch( m_pRotorSound, iPitch + rotorPitch, 0.1 );
		controller.SoundChangeVolume( m_pRotorSound, m_engineThrust * 0.5f, 0.1 );
	}
}

//------------------------------------------------------------------------------
// Purpose: Fire a round from the cannon
// Notes:	Only call this if you have an enemy.
//------------------------------------------------------------------------------
void CNPC_CombineDropship::FireCannonRound( void )
{

	Vector vecPenetrate;
	trace_t tr;

	QAngle vecAimAngle;
	Vector vecToEnemy, vecEnemyTarget;
	Vector vecMuzzle;
	Vector vecAimDir;

	// HACKHACK until we actually get a muzzle attachment
//	GetAttachment( "muzzle", vecMuzzle, vecAimAngle );
	vecMuzzle = GetAbsOrigin();
	vecAimAngle = GetAbsAngles();

	Vector shipDir, shipLeft;
	AngleVectors( vecAimAngle, &shipDir );
	vecMuzzle.z += 72;
	vecMuzzle += shipDir * 96;

	vecEnemyTarget = GetEnemy()->BodyTarget( vecMuzzle, false );
	
	// Aim with the muzzle's attachment point.
	VectorSubtract( vecEnemyTarget, vecMuzzle, vecToEnemy );

	QAngle enemyAngles;
	VectorAngles(vecToEnemy, enemyAngles );
	float diff = fabs( UTIL_AngleDiff( enemyAngles.y, vecAimAngle.y ));	

	// only allow 90 degrees of freedom when firing
	if ( diff > 45 )
	{
		StopCannon();
		m_flTimeNextAttack = gpGlobals->curtime + 0.05;
		return;	
	}

	if ( !m_bIsFiring )				// start up sound
	{
		StartCannon();
	}

	VectorNormalize( vecToEnemy );

	// If the gun is wildly off target, stop firing!
	// FIXME  - this should use a vector pointing 
	// to the enemy's location PLUS the stitching 
	// error! (sjb) !!!BUGBUG
//	AngleVectors( vecAimAngle, &vecAimDir );	

/*	if( DotProduct( vecToEnemy, vecAimDir ) < 0.980 )
	{
		StopCannonBurst();
		m_flTimeNextAttack = gpGlobals->curtime + 0.1;
		return;
	}
*/
	//Add a muzzle flash
	g_pEffects->MuzzleFlash( vecMuzzle, vecAimAngle, random->RandomFloat( 5.0f, 7.0f ) , MUZZLEFLASH_TYPE_GUNSHIP );

	m_flTimeNextAttack = gpGlobals->curtime + 0.05;

	// Cheat. Fire directly at the target, plus noise.
	int ammoType = GetAmmoDef()->Index("CombineCannon"); 
	FireBullets( 1, vecMuzzle, vecToEnemy, VECTOR_CONE_2DEGREES, 8192, ammoType, 1, -1, -1, 1 /* override damage */ );
}

//------------------------------------------------------------------------------
// Purpose : The proper way to begin the gunship cannon firing at the enemy.
// Input   : 
//		   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::StartCannon( void )
{
	m_bIsFiring = true;

	// Start up the cannon sound.
	if ( m_pCannonSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume(m_pCannonSound, 0.5, 0.0);
	}

}

//------------------------------------------------------------------------------
// Purpose : The proper way to cease the gunship cannon firing. 
// Input   : 
//		   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::StopCannon( void )
{
	m_bIsFiring = false;

	// Stop the cannon sound.
	if ( m_pCannonSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume(m_pCannonSound, 0.0, 0.1);
	}
}



AI_BEGIN_CUSTOM_NPC( npc_combinedropship, CNPC_CombineDropship )

	DECLARE_ACTIVITY( ACT_DROPSHIP_FLY_IDLE );
	DECLARE_ACTIVITY( ACT_DROPSHIP_FLY_IDLE_EXAGG );
	DECLARE_ACTIVITY( ACT_DROPSHIP_DESCEND_IDLE );
	DECLARE_ACTIVITY( ACT_DROPSHIP_DEPLOY_IDLE );
	DECLARE_ACTIVITY( ACT_DROPSHIP_LIFTOFF );

	DECLARE_ACTIVITY( ACT_DROPSHIP_FLY_IDLE_CARGO );

AI_END_CUSTOM_NPC()



