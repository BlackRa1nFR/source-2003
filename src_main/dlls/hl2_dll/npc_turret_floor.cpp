//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_BaseNPC.h"
#include "AI_Senses.h"
#include "AI_Memory.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "sprite.h"
#include "hl2_dll/hl2_player.h"
#include "soundenvelope.h"
#include "physics_saverestore.h"
#include "ieffects.h"

class CTurretTipController;
extern proficiencyinfo_t g_WeaponProficiencyTable[];

#define	DISABLE_SHOT	0

//Debug visualization
ConVar	g_debug_turret( "g_debug_turret", "0" );

#define	FLOOR_TURRET_MODEL			"models/combine_turrets/floor_turret.mdl"
#define FLOOR_TURRET_GLOW_SPRITE	"sprites/glow1.vmt"
#define FLOOR_TURRET_BC_YAW			"aim_yaw"
#define FLOOR_TURRET_BC_PITCH		"aim_pitch"
#define	FLOOR_TURRET_RANGE			1200
#define	FLOOR_TURRET_MAX_WAIT		5
#define FLOOR_TURRET_SHORT_WAIT		2.0		// Used for FAST_RETIRE spawnflag
#define	FLOOR_TURRET_PING_TIME		1.0f	//LPB!!

#define	FLOOR_TURRET_VOICE_PITCH_LOW	45
#define	FLOOR_TURRET_VOICE_PITCH_HIGH	100

//Aiming variables
#define	FLOOR_TURRET_MAX_NOHARM_PERIOD		0.0f
#define	FLOOR_TURRET_MAX_GRACE_PERIOD		3.0f

//Spawnflags
#define SF_FLOOR_TURRET_AUTOACTIVATE		0x00000020
#define SF_FLOOR_TURRET_STARTINACTIVE		0x00000040
#define SF_FLOOR_TURRET_FASTRETIRE			0x00000080

//Activities
int ACT_FLOOR_TURRET_OPEN;
int ACT_FLOOR_TURRET_CLOSE;
int ACT_FLOOR_TURRET_OPEN_IDLE;
int ACT_FLOOR_TURRET_CLOSED_IDLE;
int ACT_FLOOR_TURRET_FIRE;

//Turret states
enum turretState_e
{
	TURRET_SEARCHING,
	TURRET_AUTO_SEARCHING,
	TURRET_ACTIVE,
	TURRET_SUPPRESSING,
	TURRET_DEPLOYING,
	TURRET_RETIRING,
};

//Eye states
enum eyeState_t
{
	TURRET_EYE_SEE_TARGET,			//Sees the target, bright and big
	TURRET_EYE_SEEKING_TARGET,		//Looking for a target, blinking (bright)
	TURRET_EYE_DORMANT,				//Not active
	TURRET_EYE_DEAD,				//Completely invisible
	TURRET_EYE_DISABLED,			//Turned off, must be reactivated before it'll deploy again (completely invisible)
};

//
// Floor Turret
//

class CNPC_FloorTurret : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_FloorTurret, CAI_BaseNPC );
public:
	
	CNPC_FloorTurret( void );
	~CNPC_FloorTurret( void );

	void	Precache( void );
	void	Spawn( void );
	bool	CreateVPhysics( void );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	UpdateOnRemove( void );
	virtual int	OnTakeDamage( const CTakeDamageInfo &info );

	bool	ShouldSavePhysics() { return true; }

	// Think functions
	void	Retire( void );
	void	Deploy( void );
	void	ActiveThink( void );
	void	SearchThink( void );
	void	AutoSearchThink( void );
	void	TippedThink( void );
	void	SuppressThink( void );

	Vector	GetAttackAccuracy( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget ) 
	{
		return VECTOR_CONE_10DEGREES * g_WeaponProficiencyTable[ WEAPON_PROFICIENCY_AVERAGE ].value;
	}

	// Use functions
	void	ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps() 
	{ 
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this );
		}
	}

	// Inputs
	void	InputToggle( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	
	bool	IsValidEnemy( CBaseEntity *pEnemy );

	float	MaxYawSpeed( void );

	Class_T	Classify( void ) 
	{
		if( m_bEnabled ) 
			return CLASS_MILITARY;

		return CLASS_NONE;
	}

	Vector	EyePosition( void )
	{
		Vector vecOrigin;
		QAngle vecAngles;

		GetAttachment( m_iMuzzleAttachment, vecOrigin, vecAngles );

		return vecOrigin;
	}


	Vector	EyeOffset( Activity nActivity ) { return Vector( 0, 0, 58 ); }

protected:
	
	bool	PreThink( turretState_e state );
	void	Shoot( const Vector &vecSrc, const Vector &vecDirToEnemy );
	void	SetEyeState( eyeState_t state );
	void	Ping( void );	
	void	Toggle( void );
	void	Enable( void );
	void	Disable( void );
	void	SpinUp( void );
	void	SpinDown( void );

	inline bool	OnSide( void );
	
	bool	UpdateFacing( void );

	int		m_iAmmoType;

	bool	m_bAutoStart;
	bool	m_bActive;		//Denotes the turret is deployed and looking for targets
	bool	m_bBlinkState;
	bool	m_bEnabled;		//Denotes whether the turret is able to deploy or not
	
	float	m_flShotTime;
	float	m_flLastSight;
	float	m_flThrashTime;
	float	m_flPingTime;

	QAngle	m_vecGoalAngles;

	int						m_iEyeAttachment;
	int						m_iMuzzleAttachment;
	CSprite					*m_pEyeGlow;
	CTurretTipController	*m_pMotionController;

	Vector	m_vecEnemyLKP;

	static const char		*m_pShotSounds[];

	COutputEvent m_OnDeploy;
	COutputEvent m_OnRetire;
	COutputEvent m_OnTipped;

	DECLARE_DATADESC();
};

//
// Tip controller
//

class CTurretTipController : public CPointEntity, public IMotionEvent
{
	DECLARE_CLASS( CTurretTipController, CPointEntity );
	DECLARE_DATADESC();

public:

	~CTurretTipController( void );
	void Spawn( void );
	void Activate( void );
	void Enable( bool state = true );
	void Suspend( float time );

	bool Enabled( void );

	static CTurretTipController	*CreateTipController( CNPC_FloorTurret *pOwner )
	{
		if ( pOwner == NULL )
			return NULL;

		CTurretTipController *pController = (CTurretTipController *) Create( "floorturret_tipcontroller", pOwner->GetAbsOrigin(), pOwner->GetAbsAngles() );

		if ( pController != NULL )
		{
			pController->m_pParentTurret = pOwner;
		}

		return pController;
	}

	// IMotionEvent
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

private:
	bool						m_bEnabled;
	float						m_flSuspendTime;
	Vector						m_worldGoalAxis;
	Vector						m_localTestAxis;
	IPhysicsMotionController	*m_pController;
	float						m_angularLimit;
	CNPC_FloorTurret			*m_pParentTurret;
};

//Datatable
BEGIN_DATADESC( CNPC_FloorTurret )

	DEFINE_FIELD( CNPC_FloorTurret, m_iAmmoType,	FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_FloorTurret, m_bAutoStart,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_FloorTurret, m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_FloorTurret, m_bBlinkState,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_FloorTurret, m_bEnabled,		FIELD_BOOLEAN ),

	DEFINE_FIELD( CNPC_FloorTurret, m_flShotTime,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_FloorTurret, m_flLastSight,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_FloorTurret, m_flThrashTime,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_FloorTurret, m_flPingTime,	FIELD_TIME ),

	DEFINE_FIELD( CNPC_FloorTurret, m_vecGoalAngles,FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_FloorTurret, m_iEyeAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_FloorTurret, m_iMuzzleAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_FloorTurret, m_pEyeGlow,		FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_FloorTurret, m_pMotionController,FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_FloorTurret, m_vecEnemyLKP,	FIELD_VECTOR ),

	DEFINE_THINKFUNC( CNPC_FloorTurret, Retire ),
	DEFINE_THINKFUNC( CNPC_FloorTurret, Deploy ),
	DEFINE_THINKFUNC( CNPC_FloorTurret, ActiveThink ),
	DEFINE_THINKFUNC( CNPC_FloorTurret, SearchThink ),
	DEFINE_THINKFUNC( CNPC_FloorTurret, AutoSearchThink ),
	DEFINE_THINKFUNC( CNPC_FloorTurret, TippedThink ),
	DEFINE_THINKFUNC( CNPC_FloorTurret, SuppressThink ),

	DEFINE_USEFUNC( CNPC_FloorTurret, ToggleUse ),

	// Inputs
	DEFINE_INPUTFUNC( CNPC_FloorTurret, FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( CNPC_FloorTurret, FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( CNPC_FloorTurret, FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( CNPC_FloorTurret, m_OnDeploy, "OnDeploy" ),
	DEFINE_OUTPUT( CNPC_FloorTurret, m_OnRetire, "OnRetire" ),
	DEFINE_OUTPUT( CNPC_FloorTurret, m_OnTipped, "OnTipped" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_turret_floor, CNPC_FloorTurret );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_FloorTurret::CNPC_FloorTurret( void )
{
	m_bActive			= false;
	m_pEyeGlow			= NULL;
	m_iAmmoType			= -1;
	m_bAutoStart		= false;
	m_flPingTime		= 0;
	m_flShotTime		= 0;
	m_flLastSight		= 0;
	m_bBlinkState		= false;
	m_flThrashTime		= 0;
	m_pMotionController = NULL;
	m_bEnabled			= false;

	m_vecGoalAngles.Init();
}

CNPC_FloorTurret::~CNPC_FloorTurret( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::UpdateOnRemove( void )
{
	if ( m_pMotionController != NULL )
	{
		UTIL_RemoveImmediate( m_pMotionController );
		m_pMotionController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Precache( void )
{
	engine->PrecacheModel( FLOOR_TURRET_MODEL );	
	engine->PrecacheModel( FLOOR_TURRET_GLOW_SPRITE );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the entity
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Spawn( void )
{ 
	Precache();

	SetModel( FLOOR_TURRET_MODEL );
	
	BaseClass::Spawn();

	m_HackedGunPos	= Vector( 0, 0, 12.75 );
	SetViewOffset( EyeOffset( ACT_IDLE ) );
	m_flFieldOfView	= 0.4f; // 60 degrees
	m_takedamage	= DAMAGE_EVENTS_ONLY;
	m_iHealth		= 100;
	m_iMaxHealth	= 100;

	AddFlag( FL_AIMTARGET );

	SetPoseParameter( FLOOR_TURRET_BC_YAW, 0 );
	SetPoseParameter( FLOOR_TURRET_BC_PITCH, 0 );

	m_iAmmoType = GetAmmoDef()->Index( "MediumRound" );

	m_iMuzzleAttachment = LookupAttachment( "eyes" );
	m_iEyeAttachment = 2;	// FIXME: what's the correct name?

	//Create our eye sprite
	m_pEyeGlow = CSprite::SpriteCreate( FLOOR_TURRET_GLOW_SPRITE, GetLocalOrigin(), false );
	m_pEyeGlow->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( this, m_iEyeAttachment );

	// FIXME: Do we ever need m_bAutoStart? (Sawyer)
	m_spawnflags |= SF_FLOOR_TURRET_AUTOACTIVATE;

	//Set our autostart state
	m_bAutoStart = !!( m_spawnflags & SF_FLOOR_TURRET_AUTOACTIVATE );
	m_bEnabled	 = ( ( m_spawnflags & SF_FLOOR_TURRET_STARTINACTIVE ) == false );

	//Do we start active?
	if ( m_bAutoStart && m_bEnabled )
	{
		SetThink( AutoSearchThink );
		SetEyeState( TURRET_EYE_DORMANT );
	}
	else
	{
		SetEyeState( TURRET_EYE_DISABLED );
	}

	//Stagger our starting times
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1f, 0.3f ) );

	// Activities
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_OPEN );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_CLOSE );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_CLOSED_IDLE );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_OPEN_IDLE );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_FIRE );
	
	SetUse( ToggleUse );

	CreateVPhysics();
}

//-----------------------------------------------------------------------------

bool CNPC_FloorTurret::CreateVPhysics( void )
{
	//Spawn our physics hull
	if ( VPhysicsInitNormal( SOLID_VPHYSICS, 0, false ) == NULL )
	{
		DevMsg( "npc_turret_floor unable to spawn physics object!\n" );
	}

	//Create the motion controller
	m_pMotionController = CTurretTipController::CreateTipController( this );

	//Enable the controller
	if ( m_pMotionController != NULL )
	{
		m_pMotionController->Enable();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Retract and stop attacking
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Retire( void )
{
	if ( PreThink( TURRET_RETIRING ) )
		return;

	//Level out the turret
	m_vecGoalAngles = GetAbsAngles();
	SetNextThink( gpGlobals->curtime + 0.05f );

	//Set ourselves to close
	if ( GetActivity() != ACT_FLOOR_TURRET_CLOSE )
	{
		//Set our visible state to dormant
		SetEyeState( TURRET_EYE_DORMANT );

		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
		
		//If we're done moving to our desired facing, close up
		if ( UpdateFacing() == false )
		{
			SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSE );
			EmitSound( "NPC_FloorTurret.Retire" );

			//Notify of the retraction
			m_OnRetire.FireOutput( NULL, this );
		}
	}
	else if ( IsActivityFinished() )
	{	
		m_bActive		= false;
		m_flLastSight	= 0;

		SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSED_IDLE );

		//Go back to auto searching
		if ( m_bAutoStart )
		{
			SetThink( AutoSearchThink );
			SetNextThink( gpGlobals->curtime + 0.05f );
		}
		else
		{
			//Set our visible state to dormant
			SetEyeState( TURRET_EYE_DISABLED );
			SetThink( SUB_DoNothing );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deploy and start attacking
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Deploy( void )
{
	if ( PreThink( TURRET_DEPLOYING ) )
		return;

	m_vecGoalAngles = GetAbsAngles();

	SetNextThink( gpGlobals->curtime + 0.05f );

	//Show we've seen a target
	SetEyeState( TURRET_EYE_SEE_TARGET );

	//Open if we're not already
	if ( GetActivity() != ACT_FLOOR_TURRET_OPEN )
	{
		m_bActive = true;
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN );
		EmitSound( "NPC_FloorTurret.Deploy" );

		//Notify we're deploying
		m_OnDeploy.FireOutput( NULL, this );
	}

	//If we're done, then start searching
	if ( IsActivityFinished() )
	{
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );

		m_flShotTime  = gpGlobals->curtime + 1.0f;

		m_flPlaybackRate = 0;
		SetThink( SearchThink );

		EmitSound( "NPC_FloorTurret.Move" );
	}

	m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;	
}

//-----------------------------------------------------------------------------
// Purpose: Returns the speed at which the turret can face a target
//-----------------------------------------------------------------------------
float CNPC_FloorTurret::MaxYawSpeed( void )
{
	//TODO: Scale by difficulty?
	return 360.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the turret to face its desired angles
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::UpdateFacing( void )
{
	bool  bMoved = false;
	matrix3x4_t localToWorld;
	
	GetAttachment( m_iMuzzleAttachment, localToWorld );

	Vector vecGoalDir;
	AngleVectors( m_vecGoalAngles, &vecGoalDir );

	Vector vecGoalLocalDir;
	VectorIRotate( vecGoalDir, localToWorld, vecGoalLocalDir );

	if ( g_debug_turret.GetBool() )
	{
		Vector	vecMuzzle, vecMuzzleDir;
		QAngle	vecMuzzleAng;

		GetAttachment( m_iMuzzleAttachment, vecMuzzle, vecMuzzleAng );
		AngleVectors( vecMuzzleAng, &vecMuzzleDir );

		NDebugOverlay::Cross3D( vecMuzzle, -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMuzzle+(vecMuzzleDir*256), -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle+(vecMuzzleDir*256), 255, 255, 0, false, 0.05 );
		
		NDebugOverlay::Cross3D( vecMuzzle, -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMuzzle+(vecGoalDir*256), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 0.05 );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle+(vecGoalDir*256), 255, 0, 0, false, 0.05 );
	}

	QAngle vecGoalLocalAngles;
	VectorAngles( vecGoalLocalDir, vecGoalLocalAngles );

	// Update pitch
	float flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.x, 0.0, 0.05f * MaxYawSpeed() ) );
	
	int iPose = LookupPoseParameter( FLOOR_TURRET_BC_PITCH );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + ( flDiff / 1.5f ) );

	if ( fabs( flDiff ) > 0.1f )
	{
		bMoved = true;
	}

	// Update yaw
	flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.y, 0.0, 0.05f * MaxYawSpeed() ) );

	iPose = LookupPoseParameter( FLOOR_TURRET_BC_YAW );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + ( flDiff / 1.5f ) );

	if ( fabs( flDiff ) > 0.1f )
	{
		bMoved = true;
	}

	return bMoved;
}

//-----------------------------------------------------------------------------
// Purpose: Turret will continue to fire on a target's position when it loses sight of it
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SuppressThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_SUPPRESSING ) )
		return;

	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );

	//Look for a new enemy
	GetSenses()->Look( FLOOR_TURRET_RANGE );
	
	CBaseEntity *pEnemy = BestEnemy();

	SetEnemy( pEnemy );

	//If we've acquired an enemy, start firing at it
	if ( pEnemy != NULL )
	{
		SetThink( ActiveThink );
		return;
	}

	//See if we're done suppressing
	if ( gpGlobals->curtime > m_flLastSight )
	{
		// Should we look for a new target?
		ClearEnemyMemory();
		SetEnemy( NULL );
		SetThink( SearchThink );
		m_vecGoalAngles = GetAbsAngles();
		
		SpinDown();

		if ( m_spawnflags & SF_FLOOR_TURRET_FASTRETIRE )
		{
			// Retire quickly in this case. (The case where we saw the player, but he hid again).
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_SHORT_WAIT;
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;
		}

		return;
	}

	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_vecEnemyLKP;

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;	

	//We want to look at the enemy's eyes so we don't jitter
	Vector	vecDirToEnemyEyes = vecMidEnemy - vecMid;
	VectorNormalize( vecDirToEnemyEyes );

	QAngle vecAnglesToEnemy;
	VectorAngles( vecDirToEnemyEyes, vecAnglesToEnemy );

	//Draw debug info
	if ( g_debug_turret.GetBool() )
	{
		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMidEnemy, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, vecMidEnemy, 0, 255, 0, false, 0.05f );
	}

	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;
	
	GetAttachment( m_iMuzzleAttachment, vecMuzzle, vecMuzzleAng );
	AngleVectors( vecMuzzleAng, &vecMuzzleDir );
	
	if ( m_flShotTime < gpGlobals->curtime )
	{
		//Fire the gun
		if ( DotProduct( vecDirToEnemy, vecMuzzleDir ) >= 0.9848 ) // 10 degree slop
		{
			SetActivity( ACT_RESET );
			SetActivity( (Activity) ACT_FLOOR_TURRET_FIRE );
			
			//Fire the weapon
#if !DISABLE_SHOT
			Shoot( vecMuzzle, vecMuzzleDir );
#endif
		} 
	}
	else
	{
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
	}

	//If we can see our enemy, face it
	m_vecGoalAngles.y = vecAnglesToEnemy.y;
	m_vecGoalAngles.x = vecAnglesToEnemy.x;

	//Turn to face
	UpdateFacing();
}

//-----------------------------------------------------------------------------
// Purpose: Allows the turret to fire on targets if they're visible
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::ActiveThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_ACTIVE ) )
		return;

	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );

	//If we've become inactive, go back to searching
	if ( ( m_bActive == false ) || ( GetEnemy() == NULL ) )
	{
		SetEnemy( NULL );
		m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;
		SetThink( SearchThink );
		m_vecGoalAngles = GetAbsAngles();
		return;
	}
	
	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = GetEnemy()->BodyTarget( vecMid );

	//Store off our last seen location
	UpdateEnemyMemory( GetEnemy(), vecMidEnemy, true );

	//Look for our current enemy
	bool bEnemyInFOV = FInViewCone( GetEnemy() );
	bool bEnemyVisible = FVisible( GetEnemy() ) && GetEnemy()->IsAlive();	

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;	
	float	flDistToEnemy = VectorNormalize( vecDirToEnemy );

	//We want to look at the enemy's eyes so we don't jitter
	Vector	vecDirToEnemyEyes = GetEnemy()->WorldSpaceCenter() - vecMid;
	VectorNormalize( vecDirToEnemyEyes );

	QAngle vecAnglesToEnemy;
	VectorAngles( vecDirToEnemyEyes, vecAnglesToEnemy );

	GetEnemies()->RefreshMemories();

	//Draw debug info
	if ( g_debug_turret.GetBool() )
	{
		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( GetEnemy()->WorldSpaceCenter(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, GetEnemy()->WorldSpaceCenter(), 0, 255, 0, false, 0.05 );

		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMidEnemy, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, vecMidEnemy, 0, 255, 0, false, 0.05f );
	}

	//See if they're past our FOV of attack
	if ( bEnemyInFOV == false )
	{
		// Should we look for a new target?
		ClearEnemyMemory();
		SetEnemy( NULL );
		
		if ( m_spawnflags & SF_FLOOR_TURRET_FASTRETIRE )
		{
			// Retire quickly in this case. (The case where we saw the player, but he hid again).
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_SHORT_WAIT;
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;
		}

		SetThink( SearchThink );
		m_vecGoalAngles = GetAbsAngles();
		
		SpinDown();

		return;
	}

	//Current enemy is not visible
	if ( ( bEnemyVisible == false ) || ( flDistToEnemy > FLOOR_TURRET_RANGE ))
	{
		m_flLastSight = gpGlobals->curtime + 2.0f;

		ClearEnemyMemory();
		SetEnemy( NULL );
		SetThink( SuppressThink );

		return;
	}

	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;
	
	GetAttachment( m_iMuzzleAttachment, vecMuzzle, vecMuzzleAng );
	AngleVectors( vecMuzzleAng, &vecMuzzleDir );
	
	if ( m_flShotTime < gpGlobals->curtime )
	{
		//Fire the gun
		if ( DotProduct( vecDirToEnemy, vecMuzzleDir ) >= 0.9848 ) // 10 degree slop
		{
			SetActivity( ACT_RESET );
			SetActivity( (Activity) ACT_FLOOR_TURRET_FIRE );
			
			//Fire the weapon
#if !DISABLE_SHOT
			Shoot( vecMuzzle, vecMuzzleDir );
#endif
		} 
	}
	else
	{
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
	}

	//If we can see our enemy, face it
	if ( bEnemyVisible )
	{
		m_vecGoalAngles.y = vecAnglesToEnemy.y;
		m_vecGoalAngles.x = vecAnglesToEnemy.x;
	}

	//Turn to face
	UpdateFacing();
}

//-----------------------------------------------------------------------------
// Purpose: Target doesn't exist or has eluded us, so search for one
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SearchThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_SEARCHING ) )
		return;

	SetNextThink( gpGlobals->curtime + 0.05f );

	SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );

	//If our enemy has died, pick a new enemy
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		SetEnemy( NULL );
	}

	//Acquire the target
	if ( GetEnemy() == NULL )
	{
		GetSenses()->Look( FLOOR_TURRET_RANGE );
		SetEnemy( BestEnemy() );
	}

	//If we've found a target, spin up the barrel and start to attack
	if ( GetEnemy() != NULL )
	{
		//Give players a grace period
		if ( GetEnemy()->IsPlayer() )
		{
			m_flShotTime  = gpGlobals->curtime + 0.5f;
		}
		else
		{
			m_flShotTime  = gpGlobals->curtime + 0.1f;
		}

		m_flLastSight = 0;
		SetThink( ActiveThink );
		SetEyeState( TURRET_EYE_SEE_TARGET );

		SpinUp();
		EmitSound( "NPC_FloorTurret.Activate" );
		return;
	}

	//Are we out of time and need to retract?
 	if ( gpGlobals->curtime > m_flLastSight )
	{
		//Before we retrace, make sure that we are spun down.
		m_flLastSight = 0;
		SetThink( Retire );
		return;
	}
	
	//Display that we're scanning
	m_vecGoalAngles.x = GetAbsAngles().x + ( sin( gpGlobals->curtime * 1.0f ) * 15.0f );
	m_vecGoalAngles.y = GetAbsAngles().y + ( sin( gpGlobals->curtime * 2.0f ) * 60.0f );

	//Turn and ping
	UpdateFacing();
	Ping();
}

//-----------------------------------------------------------------------------
// Purpose: Watch for a target to wander into our view
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::AutoSearchThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_AUTO_SEARCHING ) )
		return;

	//Spread out our thinking
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.2f, 0.4f ) );

	//If the enemy is dead, find a new one
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		SetEnemy( NULL );
	}

	//Acquire Target
	if ( GetEnemy() == NULL )
	{
		GetSenses()->Look( FLOOR_TURRET_RANGE );
		SetEnemy( BestEnemy() );
	}

	//Deploy if we've got an active target
	if ( GetEnemy() != NULL )
	{
		SetThink( Deploy );
		EmitSound( "NPC_FloorTurret.Alert" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire!
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Shoot( const Vector &vecSrc, const Vector &vecDirToEnemy )
{
	if ( GetEnemy() != NULL )
	{
		Vector vecDir = GetActualShootTrajectory( vecSrc );

		FireBullets( 1, vecSrc, vecDir, VECTOR_CONE_PRECALCULATED, MAX_COORD_RANGE, m_iAmmoType, 1, -1, -1, 5, NULL );
		EmitSound( "NPC_FloorTurret.ShotSounds" );
		m_fEffects |= EF_MUZZLEFLASH;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( m_NPCState == NPC_STATE_DEAD )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: The turret has been tipped over and will thrash for awhile
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::TippedThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.05f );

	StudioFrameAdvance();

	//See if we should continue to thrash
	if ( gpGlobals->curtime < m_flThrashTime )
	{
		if ( m_flShotTime < gpGlobals->curtime )
		{
			Vector vecMuzzle, vecMuzzleDir;
			QAngle vecMuzzleAng;
			GetAttachment( m_iMuzzleAttachment, vecMuzzle, vecMuzzleAng );

			AngleVectors( vecMuzzleAng, &vecMuzzleDir );
			
#if !DISABLE_SHOT
			Shoot( vecMuzzle, vecMuzzleDir );
#endif
			m_flShotTime = gpGlobals->curtime + 0.05f;
		}

		m_vecGoalAngles.x = GetAbsAngles().x + random->RandomFloat( -60, 60 );
		m_vecGoalAngles.y = GetAbsAngles().y + random->RandomFloat( -60, 60 );

		UpdateFacing();
	}
	else
	{
		//Face forward
		m_vecGoalAngles = GetAbsAngles();

		//Set ourselves to close
		if ( GetActivity() != ACT_FLOOR_TURRET_CLOSE )
		{
			SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
			
			//If we're done moving to our desired facing, close up
			if ( UpdateFacing() == false )
			{
				//Make any last death noises and anims
				EmitSound( "NPC_FloorTurret.Die" );
				SpinDown();

				SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSE );
				EmitSound( "NPC_FloorTurret.Retract" );

				CTakeDamageInfo	info;
				info.SetDamage( 1 );
				info.SetDamageType( DMG_CRUSH );
				Event_Killed( info );
			}
		}
		else if ( IsActivityFinished() )
		{	
			m_bActive		= false;
			m_flLastSight	= 0;

			SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSED_IDLE );

			//Try to look straight
			if ( UpdateFacing() == false )
			{
				m_OnTipped.FireOutput( this, this );
				SetEyeState( TURRET_EYE_DEAD );
				SetThink( NULL );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the turret is upright enough to function
// Output : Returns true if the turret is tipped over
//-----------------------------------------------------------------------------
inline bool CNPC_FloorTurret::OnSide( void )
{
	Vector	up;
	GetVectors( NULL, NULL, &up );

	return ( DotProduct( up, Vector(0,0,1) ) < 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the turret is currently in
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::PreThink( turretState_e state )
{
	//Animate
	StudioFrameAdvance();

	//See if we've tipped
	if ( OnSide() == false )
	{
		//Debug visualization
		if ( g_debug_turret.GetBool() )
		{
			Vector	up;
			GetVectors( NULL, NULL, &up );

			NDebugOverlay::Line( GetAbsOrigin()+(up*32), GetAbsOrigin()+(up*128), 0, 255, 0, false, 2.0f );
			NDebugOverlay::Cross3D( GetAbsOrigin()+(up*32), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 2.0f );
			NDebugOverlay::Cross3D( GetAbsOrigin()+(up*128), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 2.0f );
		}
	}
	else
	{
		//Thrash around for a bit
		m_flThrashTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 2.5f );
		SetNextThink( gpGlobals->curtime + 0.05f );

		SetThink( TippedThink );

		//Stop being targetted
		SetState( NPC_STATE_DEAD );
		m_lifeState = LIFE_DEAD;

		//Disable the tip controller
		m_pMotionController->Enable( false );

		SetEyeState( TURRET_EYE_SEE_TARGET );

		SpinUp();
		EmitSound( "NPC_FloorTurret.Alarm" );

		//Debug visualization
		if ( g_debug_turret.GetBool() )
		{
			Vector	up;
			GetVectors( NULL, NULL, &up );

			NDebugOverlay::Line( GetAbsOrigin()+(up*32), GetAbsOrigin()+(up*128), 255, 0, 0, false, 2.0f );
			NDebugOverlay::Cross3D( GetAbsOrigin()+(up*32), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 2.0f );
			NDebugOverlay::Cross3D( GetAbsOrigin()+(up*128), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 2.0f );
		}

		//Interrupt current think function
		return true;
	}

	//Do not interrupt current think function
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the glowing eye attached to the turret
// Input  : state - state the eye should be in
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SetEyeState( eyeState_t state )
{
	//Must have a valid eye to affect
	if ( m_pEyeGlow == NULL )
		return;

	//Set the state
	switch( state )
	{
	default:
	case TURRET_EYE_SEE_TARGET: //Fade in and scale up
		m_pEyeGlow->SetColor( 255, 0, 0 );
		m_pEyeGlow->SetBrightness( 164, 0.1f );
		m_pEyeGlow->SetScale( 0.4f, 0.1f );
		break;

	case TURRET_EYE_SEEKING_TARGET: //Ping-pongs
		
		//Toggle our state
		m_bBlinkState = !m_bBlinkState;
		m_pEyeGlow->SetColor( 255, 128, 0 );

		if ( m_bBlinkState )
		{
			//Fade up and scale up
			m_pEyeGlow->SetScale( 0.25f, 0.1f );
			m_pEyeGlow->SetBrightness( 164, 0.1f );
		}
		else
		{
			//Fade down and scale down
			m_pEyeGlow->SetScale( 0.2f, 0.1f );
			m_pEyeGlow->SetBrightness( 64, 0.1f );
		}

		break;

	case TURRET_EYE_DORMANT: //Fade out and scale down
		m_pEyeGlow->SetColor( 0, 255, 0 );
		m_pEyeGlow->SetScale( 0.1f, 0.5f );
		m_pEyeGlow->SetBrightness( 64, 0.5f );
		break;

	case TURRET_EYE_DEAD: //Fade out slowly
		m_pEyeGlow->SetColor( 255, 0, 0 );
		m_pEyeGlow->SetScale( 0.1f, 3.0f );
		m_pEyeGlow->SetBrightness( 0, 3.0f );
		break;

	case TURRET_EYE_DISABLED:
		m_pEyeGlow->SetColor( 0, 255, 0 );
		m_pEyeGlow->SetScale( 0.1f, 1.0f );
		m_pEyeGlow->SetBrightness( 0, 1.0f );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make a pinging noise so the player knows where we are
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Ping( void )
{
	//See if it's time to ping again
	if ( m_flPingTime > gpGlobals->curtime )
		return;

	//Ping!
	EmitSound( "NPC_FloorTurret.Ping" );

	SetEyeState( TURRET_EYE_SEEKING_TARGET );

	m_flPingTime = gpGlobals->curtime + FLOOR_TURRET_PING_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the turret's state
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Toggle( void )
{
	//This turret is on its side, it can't function
	if ( OnSide() || ( IsAlive() == false ) )
		return;

	//Toggle the state
	if ( m_bEnabled )
	{
		Disable();
	}
	else 
	{
		Enable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable the turret and deploy
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Enable( void )
{
	if ( IsAlive() == false )
		return;

	m_bEnabled = true;

	// if the turret is flagged as an autoactivate turret, re-enable its ability open self.
	if ( m_spawnflags & SF_FLOOR_TURRET_AUTOACTIVATE )
	{
		m_bAutoStart = true;
	}

	SetThink( Deploy );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: Retire the turret until enabled again
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Disable( void )
{
	if ( IsAlive() == false )
		return;

	m_bEnabled = false;
	m_bAutoStart = false;

	SetEnemy( NULL );
	SetThink( Retire );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the turret's state via input function
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: Allow players and npc's to turn the turret on and off
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	switch( useType )
	{
	case USE_OFF:
		Disable();
		break;
	case USE_ON:
		Enable();
		break;
	case USE_SET:
		break;
	case USE_TOGGLE:
		Toggle( );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sweeten any impact damage we take
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	IPhysicsObject *pObj = VPhysicsGetObject();

	if ( pObj != NULL )
	{
		VPhysicsGetObject()->ApplyForceOffset( vecDir * info.GetDamage() * 650, ptr->endpos );
	}

	//NOTENOTE: Damage is consumed
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CNPC_FloorTurret::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo	newInfo = info;

	if ( info.GetDamageType() & (DMG_SLASH|DMG_CLUB) )
	{
		newInfo.ScaleDamage( 50.0f );
		
		if ( m_pMotionController != NULL )
		{
			m_pMotionController->Suspend( 2.0f );
		}
	}
	else if ( info.GetDamageType() & DMG_BLAST )
	{
		newInfo.ScaleDamage( 4.0f );
		
		if ( m_pMotionController != NULL )
		{
			m_pMotionController->Suspend( 2.0f );
		}
	}

	//Cap it
	if ( newInfo.GetDamage() > 1000 )
	{
		newInfo.SetDamage( 1000 );
	}

	VPhysicsTakeDamage( newInfo );

	return BaseClass::OnTakeDamage( newInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SpinUp( void )
{
}

#define	FLOOR_TURRET_MIN_SPIN_DOWN	1.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SpinDown( void )
{
}

// 
// Tip controller
//

LINK_ENTITY_TO_CLASS( floorturret_tipcontroller, CTurretTipController );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CTurretTipController )

	DEFINE_FIELD( CTurretTipController, m_bEnabled,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CTurretTipController, m_flSuspendTime,	FIELD_TIME ),
	DEFINE_FIELD( CTurretTipController, m_worldGoalAxis,	FIELD_VECTOR ),
	DEFINE_FIELD( CTurretTipController, m_localTestAxis,	FIELD_VECTOR ),
	DEFINE_PHYSPTR( CTurretTipController, m_pController ),
	DEFINE_FIELD( CTurretTipController, m_angularLimit,		FIELD_FLOAT ),
	DEFINE_FIELD( CTurretTipController, m_pParentTurret,	FIELD_CLASSPTR ),

END_DATADESC()



CTurretTipController::~CTurretTipController()
{
	if ( m_pController )
	{
		physenv->DestroyMotionController( m_pController );
		m_pController = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTurretTipController::Spawn( void )
{
	m_bEnabled = true;

	// align the object's local Z axis
	m_localTestAxis.Init( 0, 0, 1 );
	// with the world's Z axis
	m_worldGoalAxis.Init( 0, 0, 1 );

	// recover from up to 15 degrees / sec angular velocity
	m_angularLimit	= 25;
	m_flSuspendTime	= 0;

	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTurretTipController::Activate( void )
{
	BaseClass::Activate();

	if ( m_pParentTurret == NULL )
	{
		UTIL_Remove(this);
		return;
	}

	IPhysicsObject *pPhys = m_pParentTurret->VPhysicsGetObject();

	if ( pPhys == NULL )
	{
		UTIL_Remove(this);
		return;
	}

	//Setup the motion controller
	if ( !m_pController )
	{
		m_pController = physenv->CreateMotionController( (IMotionEvent *)this );
		m_pController->AttachObject( pPhys );
	}
	else
	{
		m_pController->SetEventHandler( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Actual simulation for tip controller
//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CTurretTipController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if ( Enabled() == false )
	{
		linear.Init();
		angular.Init();

		return SIM_NOTHING;
	}

	Vector currentAxis;
	matrix3x4_t matrix;

	// get the object's local to world transform
	pObject->GetPositionMatrix( matrix );

	// Get the object's local test axis in world space
	VectorRotate( m_localTestAxis, matrix, currentAxis );
	Vector rotationAxis = CrossProduct( currentAxis, m_worldGoalAxis );

	// atan2() is well defined, so do a Dot & Cross instead of asin(Cross)
	float cosine = DotProduct( currentAxis, m_worldGoalAxis );
	float sine = VectorNormalize( rotationAxis );
	float angle = atan2( sine, cosine );

	angle = RAD2DEG(angle);
	rotationAxis *= angle;
	VectorIRotate( rotationAxis, matrix, angular );
	float invDeltaTime = (1/deltaTime);
	angular *= invDeltaTime * invDeltaTime;

	AngularImpulse angVel;
	pObject->GetVelocity( NULL, &angVel );
	angular -= angVel;

	float len = VectorNormalize( angular );

	if ( len > m_angularLimit * invDeltaTime )
	{
		len = m_angularLimit * invDeltaTime;
	}

	angular *= len;

	linear.Init();

	return SIM_LOCAL_ACCELERATION;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTurretTipController::Enable( bool state )
{
	m_bEnabled = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CTurretTipController::Suspend( float time )
{
	m_flSuspendTime = gpGlobals->curtime + time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTurretTipController::Enabled( void )
{
	if ( m_flSuspendTime > gpGlobals->curtime )
		return false;

	return m_bEnabled;
}
