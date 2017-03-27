#include	"cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Route.h"
#include	"AI_Senses.h"
#include	"soundent.h"
#include	"game.h"
#include	"NPCEvent.h"
#include	"EntityList.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"basecombatweapon.h"
#include	"IEffects.h"
#include	"vstdlib/random.h"
#include	"engine/IEngineSound.h"
#include	"ammodef.h"
#include	"Sprite.h"

#define TURRET_SHOTS	2
#define TURRET_RANGE	(100 * 12)
#define TURRET_SPREAD	Vector( 0, 0, 0 )
#define TURRET_TURNRATE	30		//angles per 0.1 second
#define TURRET_MAXWAIT	15		// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN	5		// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME	0.5

typedef enum
{
//	TURRET_ANIM_NONE = 0,
	TURRET_ANIM_FIRE = 0,	
	TURRET_ANIM_SPIN,
	TURRET_ANIM_DEPLOY,
	TURRET_ANIM_RETIRE,
	TURRET_ANIM_DIE,
} TURRET_ANIM;

#define SF_MONSTER_TURRET_AUTOACTIVATE	32
#define SF_MONSTER_TURRET_STARTINACTIVE	64

#define TURRET_GLOW_SPRITE "sprites/flare3.vmt"

#define TURRET_ORIENTATION_FLOOR 0
#define TURRET_ORIENTATION_CEILING 1

class CNPC_BaseTurret : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_BaseTurret, CAI_BaseNPC );
public:
	void Spawn(void);
	virtual void Precache(void);
	void EXPORT TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	Class_T Classify( void );

	// Think functions
	void EXPORT ActiveThink(void);
	void EXPORT SearchThink(void);
	void EXPORT AutoSearchThink(void);
	void EXPORT TurretDeath(void);

	virtual void EXPORT SpinDownCall(void) { m_iSpin = 0; }
	virtual void EXPORT SpinUpCall(void) { m_iSpin = 1; }

	void EXPORT Deploy(void);
	void EXPORT Retire(void);

	void EXPORT Initialize(void);

	virtual void Ping(void);
	virtual void EyeOn(void);
	virtual void EyeOff(void);


	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );

	void Event_Killed( const CTakeDamageInfo &info );
	virtual bool ShouldFadeOnDeath( void ) { return false; }
	bool ShouldGib( const CTakeDamageInfo &info ) { return false; }

	// other functions
	void SetTurretAnim(TURRET_ANIM anim);
	int MoveTurret(void);
	virtual void Shoot(Vector &vecSrc, Vector &vecDirToEnemy) { };
  
	float m_flMaxSpin;		// Max time to spin the barrel w/o a target
	int m_iSpin;

	CSprite *m_pEyeGlow;
	int		m_eyeBrightness;

	int	m_iDeployHeight;
	int	m_iRetractHeight;
	int m_iMinPitch;

	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate
	int m_iOrientation;		// 0 = floor, 1 = Ceiling
	int	m_iOn;
	int m_fBeserk;			// Sometimes this bitch will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
							// enters its range

	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
	int m_iSearchSpeed;		// Not Used!

	// movement
	float	m_flStartYaw;
	Vector	m_vecCurAngles;
	Vector	m_vecGoalAngles;


	float	m_flPingTime;	// Time until the next ping, used when searching
	float	m_flSpinUpTime;	// Amount of time until the barrel should spin down when searching

	float   m_flDamageTime;

	int		m_iAmmoType;

	COutputEvent	m_OnActivate;
	COutputEvent	m_OnDeactivate;

	//DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

	typedef CAI_BaseNPC BaseClass;
};

BEGIN_DATADESC( CNPC_BaseTurret )

	//FIELDS
	DEFINE_FIELD( CNPC_BaseTurret, m_flMaxSpin, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_BaseTurret, m_iSpin, FIELD_INTEGER ),

	DEFINE_FIELD( CNPC_BaseTurret, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_BaseTurret, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_BaseTurret, m_iDeployHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_BaseTurret, m_iRetractHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_BaseTurret, m_iMinPitch, FIELD_INTEGER ),

	DEFINE_FIELD( CNPC_BaseTurret, m_fTurnRate, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_BaseTurret, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_BaseTurret, m_fBeserk, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_BaseTurret, m_iAutoStart, FIELD_INTEGER ),

	DEFINE_FIELD( CNPC_BaseTurret, m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_BaseTurret, m_flLastSight, FIELD_TIME ),

	DEFINE_FIELD( CNPC_BaseTurret, m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_BaseTurret, m_vecCurAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_BaseTurret, m_vecGoalAngles, FIELD_VECTOR ),

	DEFINE_FIELD( CNPC_BaseTurret, m_flPingTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BaseTurret, m_flSpinUpTime, FIELD_TIME ),

	//KEYFIELDS
	DEFINE_KEYFIELD( CNPC_BaseTurret, m_flMaxWait, FIELD_FLOAT, "maxsleep" ),
	DEFINE_KEYFIELD( CNPC_BaseTurret, m_iOrientation, FIELD_INTEGER, "orientation" ),
	DEFINE_KEYFIELD( CNPC_BaseTurret, m_iSearchSpeed, FIELD_INTEGER, "searchspeed" ),
	DEFINE_KEYFIELD( CNPC_BaseTurret, m_iBaseTurnRate, FIELD_INTEGER, "turnrate" ),
	
	//Use
	DEFINE_USEFUNC( CNPC_BaseTurret, TurretUse ),

	//Thinks
	DEFINE_THINKFUNC( CNPC_BaseTurret, ActiveThink ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, SearchThink ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, AutoSearchThink ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, TurretDeath ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, SpinDownCall ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, SpinUpCall ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, Deploy ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, Retire ),
	DEFINE_THINKFUNC( CNPC_BaseTurret, Initialize ),

	//Inputs
	DEFINE_INPUTFUNC( CNPC_BaseTurret, FIELD_VOID, "Activate",	InputActivate ),
	DEFINE_INPUTFUNC( CNPC_BaseTurret, FIELD_VOID, "Deactivate",	InputDeactivate ),

	//Outputs
	DEFINE_OUTPUT( CNPC_BaseTurret, m_OnActivate,	"OnActivate"),
	DEFINE_OUTPUT( CNPC_BaseTurret, m_OnDeactivate,	"OnDeactivate"),

END_DATADESC()


void CNPC_BaseTurret::Spawn()
{ 
	Precache( );
	SetNextThink( gpGlobals->curtime + 1 );
	SetMoveType( MOVETYPE_FLY );
	SetSequence( 0 );
	m_flCycle			= 0;
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	m_takedamage		= DAMAGE_YES;
	AddFlag( FL_AIMTARGET );

	AddFlag( FL_NPC );
	SetUse( &CNPC_BaseTurret::TurretUse );

	if (( m_spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE ) 
		&& !( m_spawnflags & SF_MONSTER_TURRET_STARTINACTIVE ))
	{
		m_iAutoStart = true;
	}

	ResetSequenceInfo( );

	SetBoneController(0, 0);
	SetBoneController(1, 0);

	m_flFieldOfView = VIEW_FIELD_FULL;

	m_bloodColor = DONT_BLEED;
	m_flDamageTime = 0;

	if ( GetSpawnFlags() & SF_MONSTER_TURRET_STARTINACTIVE )
	{
		 SetTurretAnim( TURRET_ANIM_RETIRE );
		 m_flCycle = 0.0f;
		 m_flPlaybackRate = 0.0f;
	}
}


void CNPC_BaseTurret::Precache()
{
	m_iAmmoType = GetAmmoDef()->Index("12mmRound");	

	enginesound->PrecacheSound ("turret/tu_fire1.wav");
	enginesound->PrecacheSound ("turret/tu_ping.wav");
	enginesound->PrecacheSound ("turret/tu_active2.wav");
	enginesound->PrecacheSound ("turret/tu_die.wav");
	enginesound->PrecacheSound ("turret/tu_die2.wav");
	enginesound->PrecacheSound ("turret/tu_die3.wav");
	enginesound->PrecacheSound ("turret/tu_deploy.wav");
	enginesound->PrecacheSound ("turret/tu_spinup.wav");
	enginesound->PrecacheSound ("turret/tu_spindown.wav");
	enginesound->PrecacheSound ("turret/tu_search.wav");
	enginesound->PrecacheSound ("turret/tu_alert.wav");
}

Class_T CNPC_BaseTurret::Classify( void )
{
	if (m_iOn || m_iAutoStart)
		return	CLASS_MACHINE;
	return CLASS_NONE;
}

//=========================================================
// TraceAttack - being attacked
//=========================================================
void CNPC_BaseTurret::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo ainfo = info;

	if ( ptr->hitgroup == 10 )	
	{
		// hit armor
		if ( m_flDamageTime != gpGlobals->curtime || (random->RandomInt(0,10) < 1) )
		{
			g_pEffects->Ricochet( ptr->endpos, ptr->plane.normal );
			m_flDamageTime = gpGlobals->curtime;
		}

		ainfo.SetDamage( 0.1 );// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}

	if ( m_takedamage == DAMAGE_NO )
		return;

	//DevMsg( 1, "traceattack: %f\n", ainfo.GetDamage() );

	AddMultiDamage( info, this );
}

//=========================================================
// TakeDamage - take damage. 
//=========================================================
int CNPC_BaseTurret::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	float flDamage = inputInfo.GetDamage();

	if (!m_iOn)
		flDamage /= 10.0;

	m_iHealth -= flDamage;
	if (m_iHealth <= 0)
	{
		m_iHealth = 0;
		m_takedamage = DAMAGE_NO;
		m_flDamageTime = gpGlobals->curtime;

//		ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

		SetUse(NULL);
		SetThink(&CNPC_BaseTurret::TurretDeath);
		SetNextThink( gpGlobals->curtime + 0.1 );

		m_OnDeactivate.FireOutput(this, this);

		return 0;
	}

	if (m_iHealth <= 10)
	{
		if (m_iOn)
		{
			m_fBeserk = 1;
			SetThink(&CNPC_BaseTurret::SearchThink);
		}
	}
	return 1;
}

int CNPC_BaseTurret::OnTakeDamage( const CTakeDamageInfo &info )
{
	int retVal = 0;

	if (!m_takedamage)
		return 0;

	switch( m_lifeState )
	{
	case LIFE_ALIVE:
		retVal = OnTakeDamage_Alive( info );
		if ( m_iHealth <= 0 )
		{
			IPhysicsObject *pPhysics = VPhysicsGetObject();
			if ( pPhysics )
			{
				pPhysics->EnableCollisions( false );
			}
			
			Event_Killed( info );	
			Event_Dying();
		}
		return retVal;
		break;

	case LIFE_DYING:
		return OnTakeDamage_Dying( info );
	
	default:
	case LIFE_DEAD:
		return OnTakeDamage_Dead( info );
	}
}

void CNPC_BaseTurret::SetTurretAnim( TURRET_ANIM anim )
{
	/*
	if (GetSequence() != anim)
	{
		switch(anim)
		{
		case TURRET_ANIM_FIRE:
		case TURRET_ANIM_SPIN:
			if (GetSequence() != TURRET_ANIM_FIRE && GetSequence() != TURRET_ANIM_SPIN)
			{
				m_flCycle = 0;
			}
			break;
		default:
			m_flCycle = 0;
			break;
		}

		SetSequence( anim );

		ResetSequenceInfo( );

		switch(anim)
		{
		case TURRET_ANIM_RETIRE:
			m_flCycle			= 255;
			m_flPlaybackRate		= -1.0;	//play the animation backwards
			break;
		case TURRET_ANIM_DIE:
			m_flPlaybackRate		= 1.0;
			break;
		}
		//ALERT(at_console, "Turret anim #%d\n", anim);
	}
	*/

	if (GetSequence() != anim)
	{
		SetSequence( anim );

		ResetSequenceInfo( );

		switch(anim)
		{
		case TURRET_ANIM_FIRE:
		case TURRET_ANIM_SPIN:
			if (GetSequence() != TURRET_ANIM_FIRE && GetSequence() != TURRET_ANIM_SPIN)
			{
				m_flCycle = 0;
			}
			break;
		case TURRET_ANIM_RETIRE:
			m_flCycle				= 1.0;
			m_flPlaybackRate		= -1.0;	//play the animation backwards
			break;
		case TURRET_ANIM_DIE:
			m_flCycle				= 0.0;
			m_flPlaybackRate		= 1.0;
			break;
		default:
			m_flCycle = 0;
			break;
		}


	}
}

//=========================================================
// Initialize - set up the turret, initial think
//=========================================================
void CNPC_BaseTurret::Initialize(void)
{
	m_iOn = 0;
	m_fBeserk = 0;
	m_iSpin = 0;

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	if (m_iBaseTurnRate == 0) m_iBaseTurnRate = TURRET_TURNRATE;
	if (m_flMaxWait == 0) m_flMaxWait = TURRET_MAXWAIT;

	QAngle angles = GetAbsAngles();
	m_flStartYaw = angles.y;
	if (m_iOrientation == TURRET_ORIENTATION_CEILING)
	{
		angles.x = 180;
		angles.y += 180;
		if( angles.y > 360 )
			angles.y -= 360;
		SetAbsAngles( angles );

//		pev->idealpitch = 180;			//not used?

		Vector view_ofs = GetViewOffset();
		view_ofs.z = -view_ofs.z;
		SetViewOffset( view_ofs );

//		pev->effects |= EF_INVLIGHT;	//no need
	}

	m_vecGoalAngles.x = 0;

	if (m_iAutoStart)
	{
		m_flLastSight = gpGlobals->curtime + m_flMaxWait;
		SetThink(&CNPC_BaseTurret::AutoSearchThink);

		SetNextThink( gpGlobals->curtime + 0.1 );
	}
	else
	{
		SetThink( SUB_DoNothing ); 
	}
}

//=========================================================
// ActiveThink - 
//=========================================================
void CNPC_BaseTurret::ActiveThink(void)
{
	int fAttack = 0;

	SetNextThink( gpGlobals->curtime + 0.1 );
	StudioFrameAdvance( );

	if ( (!m_iOn) || (GetEnemy() == NULL) )
	{
		SetEnemy( NULL );
		m_flLastSight = gpGlobals->curtime + m_flMaxWait;
		SetThink(&CNPC_BaseTurret::SearchThink);
		return;
	}
	
	// if it's dead, look for something new
	if ( !GetEnemy()->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->curtime + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->curtime > m_flLastSight)
			{	
				SetEnemy( NULL );
				m_flLastSight = gpGlobals->curtime + m_flMaxWait;
				SetThink(&CNPC_BaseTurret::SearchThink);
				return;
			}
		}
	}


	Vector vecMid = GetAbsOrigin() + GetViewOffset();
	Vector vecMidEnemy = GetEnemy()->BodyTarget( vecMid, false );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(this, GetEnemy(), vecMidEnemy );	
	
	//We want to look at the enemy's eyes so we don't jitter
	Vector	vecDirToEnemyEyes = vecMidEnemy - vecMid;

	float flDistToEnemy = vecDirToEnemyEyes.Length();

	VectorNormalize( vecDirToEnemyEyes );

	QAngle vecAnglesToEnemy;
	VectorAngles( vecDirToEnemyEyes, vecAnglesToEnemy );

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > TURRET_RANGE))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->curtime + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->curtime > m_flLastSight)
			{
				SetEnemy( NULL );
				m_flLastSight = gpGlobals->curtime + m_flMaxWait;
				SetThink(&CNPC_BaseTurret::SearchThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}
	
	//ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", 
	//	m_vecCurAngles.x, m_vecCurAngles.y,
	//	gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );

	Vector vecLOS = vecDirToEnemyEyes; //vecMid - m_vecLastSight;
	VectorNormalize( vecLOS );

	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;
	GetAttachment( 1, vecMuzzle, vecMuzzleAng );

	AngleVectors( vecMuzzleAng, &vecMuzzleDir );
	
	// Is the Gun looking at the target
	if (DotProduct(vecLOS, vecMuzzleDir) <= 0.866) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	//forward
//	NDebugOverlay::Line(vecMuzzle, vecMid + ( vecMuzzleDir * 200 ), 255,0,0, false, 0.1);
	//LOS
//	NDebugOverlay::Line(vecMuzzle, vecMid + ( vecLOS * 200 ), 0,0,255, false, 0.1);

	// fire the gun
	if (m_iSpin && ((fAttack) || (m_fBeserk)))
	{
		Vector vecOrigin;
		QAngle vecAngles;
		GetAttachment( 1, vecOrigin, vecAngles );
		Shoot(vecOrigin, vecMuzzleDir );
		SetTurretAnim(TURRET_ANIM_FIRE);
	} 
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}

	//move the gun
	if (m_fBeserk)
	{
		if (random->RandomInt(0,9) == 0)
		{
			m_vecGoalAngles.y = random->RandomFloat(0,360);
			m_vecGoalAngles.x = random->RandomFloat(0,90) - 90 * m_iOrientation;

			CTakeDamageInfo info;
			info.SetAttacker(this);
			info.SetInflictor(this);
			info.SetDamage( 1 );
			info.SetDamageType( DMG_GENERIC );

			TakeDamage( info ); // don't beserk forever
			return;
		}
	} 
	else if (fEnemyVisible)
	{
		if (vecAnglesToEnemy.y > 360)
			vecAnglesToEnemy.y -= 360;

		if (vecAnglesToEnemy.y < 0)
			vecAnglesToEnemy.y += 360;

		//ALERT(at_console, "[%.2f]", vec.x);
		
		if (vecAnglesToEnemy.x < -180)
			vecAnglesToEnemy.x += 360;

		if (vecAnglesToEnemy.x > 180)
			vecAnglesToEnemy.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...14]

		if (m_iOrientation == TURRET_ORIENTATION_FLOOR)
		{
			if (vecAnglesToEnemy.x > 90)
				vecAnglesToEnemy.x = 90;
			else if (vecAnglesToEnemy.x < m_iMinPitch)
				vecAnglesToEnemy.x = m_iMinPitch;
		}
		else
		{
			if (vecAnglesToEnemy.x < -90)
				vecAnglesToEnemy.x = -90;
			else if (vecAnglesToEnemy.x > -m_iMinPitch)
				vecAnglesToEnemy.x = -m_iMinPitch;
		}

		//DevMsg( 1, "->[%.2f]\n", vec.x);

		m_vecGoalAngles.y = vecAnglesToEnemy.y;
		m_vecGoalAngles.x = vecAnglesToEnemy.x;

	}

	SpinUpCall();
	MoveTurret();
}

//=========================================================
// SearchThink 
// This search function will sit with the turret deployed and look for a new target. 
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//=========================================================
void CNPC_BaseTurret::SearchThink(void)
{
	// ensure rethink
	SetTurretAnim(TURRET_ANIM_SPIN);
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1 );

	if (m_flSpinUpTime == 0 && m_flMaxSpin)
		m_flSpinUpTime = gpGlobals->curtime + m_flMaxSpin;

	Ping( );

	CBaseEntity *pEnemy = GetEnemy();

	// If we have a target and we're still healthy
	if (pEnemy != NULL)
	{
		if (!pEnemy->IsAlive() )
			pEnemy = NULL;// Dead enemy forces a search for new one
	}


	// Acquire Target
	if (pEnemy == NULL)
	{
		GetSenses()->Look(TURRET_RANGE);
		pEnemy = BestEnemy();

		if ( pEnemy && !FVisible( pEnemy ) )
			 pEnemy = NULL;
	}

	// If we've found a target, spin up the barrel and start to attack
	if (pEnemy != NULL)
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink(&CNPC_BaseTurret::ActiveThink);
	}
	else
	{
		// Are we out of time, do we need to retract?
 		if (gpGlobals->curtime > m_flLastSight)
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			m_flSpinUpTime = 0;
			SetThink(&CNPC_BaseTurret::Retire);
		}
		// should we stop the spin?
		else if ((m_flSpinUpTime) && (gpGlobals->curtime > m_flSpinUpTime))
		{
			SpinDownCall();
		}
		
		// generic hunt for new victims
		m_vecGoalAngles.y = (m_vecGoalAngles.y + 0.1 * m_fTurnRate);
		if (m_vecGoalAngles.y >= 360)
			m_vecGoalAngles.y -= 360;
		MoveTurret();
	}

	SetEnemy( pEnemy );
}

//=========================================================
// AutoSearchThink - 
//=========================================================
void CNPC_BaseTurret::AutoSearchThink(void)
{
	// ensure rethink
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.3 );

	// If we have a target and we're still healthy

	CBaseEntity *pEnemy = GetEnemy();

	if (pEnemy != NULL)
	{
		if (!pEnemy->IsAlive() )
		{
			pEnemy = NULL;	
		}
	}

	// Acquire Target

	if (pEnemy == NULL)
	{
		GetSenses()->Look( TURRET_RANGE );
		pEnemy = BestEnemy();

		if ( pEnemy && !FVisible( pEnemy ) )
			 pEnemy = NULL;
	}

	if (pEnemy != NULL)
	{
		SetThink(&CNPC_BaseTurret::Deploy);
		CPASAttenuationFilter filter( this );
		enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_alert.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
	}

	SetEnemy( pEnemy );
}

extern short g_sModelIndexSmoke;

//=========================================================
// TurretDeath - I die as I have lived, beyond my means
//=========================================================
void CNPC_BaseTurret::TurretDeath(void)
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1 );

	if (m_lifeState != LIFE_DEAD)
	{
		m_lifeState = LIFE_DEAD;

		CPASAttenuationFilter filter( this );

		switch( random->RandomInt(0,2) )
		{
		case 0: 
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_die.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
			break;
		case 1:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_die2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
			break;
		case 2:
		default:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_die3.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
			break;
		}
	
		enginesound->StopSound( entindex(), CHAN_STATIC, "turret/tu_active2.wav" );

		if (m_iOrientation == TURRET_ORIENTATION_FLOOR)
			m_vecGoalAngles.x = -14;
		else
			m_vecGoalAngles.x = 90;//-90;

		SetTurretAnim(TURRET_ANIM_DIE); 

		EyeOn( );	
	}

	EyeOff( );

	if (m_flDamageTime + random->RandomFloat( 0, 2 ) > gpGlobals->curtime)
	{
		// lots of smoke
		Vector pos( random->RandomFloat( GetAbsMins().x, GetAbsMaxs().x ),
			     random->RandomFloat( GetAbsMins().y, GetAbsMaxs().y ),
				 GetAbsOrigin().z );
		
		CBroadcastRecipientFilter filter;
		te->Smoke( filter, 0.0, &pos,
			g_sModelIndexSmoke,
			2.5,
			10 );
	}
	
	if (m_flDamageTime + random->RandomFloat( 0, 5 ) > gpGlobals->curtime)
	{
		Vector vecSrc = Vector( random->RandomFloat( GetAbsMins().x, GetAbsMaxs().x ), random->RandomFloat( GetAbsMins().y, GetAbsMaxs().y ), random->RandomFloat( GetAbsMins().z, GetAbsMaxs().z ) );
		g_pEffects->Sparks( vecSrc );
	}

	if (IsSequenceFinished() && !MoveTurret() && m_flDamageTime + 5 < gpGlobals->curtime)
	{
		m_flPlaybackRate = 0;
		SetThink( NULL );
	}
}

//=========================================================
// Deploy - go active
//=========================================================
void CNPC_BaseTurret::Deploy(void)
{
	SetNextThink( gpGlobals->curtime + 0.1 );
	StudioFrameAdvance( );

	if (GetSequence() != TURRET_ANIM_DEPLOY)
	{
		m_iOn = 1;
		SetTurretAnim(TURRET_ANIM_DEPLOY);
		CPASAttenuationFilter filter( this );
		enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
		m_OnActivate.FireOutput(this, this);
	}

	if (IsSequenceFinished())
	{
		Vector curmins, curmaxs;
		curmins = WorldAlignMins();
		curmaxs = WorldAlignMaxs();

		curmaxs.z = m_iDeployHeight;
		curmins.z = -m_iDeployHeight;

		SetCollisionBounds( curmins, curmaxs );
		Relink();

		m_vecCurAngles.x = 0;

		QAngle angles = GetAbsAngles();

		if (m_iOrientation == TURRET_ORIENTATION_CEILING)
		{
			m_vecCurAngles.y = UTIL_AngleMod( angles.y + 180 );
		}
		else
		{
			m_vecCurAngles.y = UTIL_AngleMod( angles.y );
		}

		SetTurretAnim(TURRET_ANIM_SPIN);
		m_flPlaybackRate = 0;
		SetThink(&CNPC_BaseTurret::SearchThink);
	}

	m_flLastSight = gpGlobals->curtime + m_flMaxWait;
}

//=========================================================
// Retire - stop being active
//=========================================================
void CNPC_BaseTurret::Retire(void)
{
	// make the turret level
	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;

	SetNextThink( gpGlobals->curtime + 0.1 );

	StudioFrameAdvance( );

	EyeOff( );

	if (!MoveTurret())
	{
		if (m_iSpin)
		{
			SpinDownCall();
		}
		else if (GetSequence() != TURRET_ANIM_RETIRE)
		{
			SetTurretAnim(TURRET_ANIM_RETIRE);
			CPASAttenuationFilter filter( this );
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, 120 );
			m_OnDeactivate.FireOutput(this, this);
		}
		//else if (IsSequenceFinished()) 
		else if( GetSequence() == TURRET_ANIM_RETIRE && m_flCycle <= 0.0 )
		{	
			m_iOn = 0;
			m_flLastSight = 0;
			//SetTurretAnim(TURRET_ANIM_NONE);

			Vector curmins, curmaxs;
			curmins = WorldAlignMins();
			curmaxs = WorldAlignMaxs();

			curmaxs.z = m_iRetractHeight;
			curmins.z = -m_iRetractHeight;

			SetCollisionBounds( curmins, curmaxs );

			Relink();

			if (m_iAutoStart)
			{
				SetThink(&CNPC_BaseTurret::AutoSearchThink);	
				SetNextThink( gpGlobals->curtime + 0.1 );
			}
			else
			{
				SetThink( SUB_DoNothing );
			}
		}
	}
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}
}

//=========================================================
// Ping - make the pinging noise every second while searching
//=========================================================
void CNPC_BaseTurret::Ping(void)
{	
	if (m_flPingTime == 0)
		m_flPingTime = gpGlobals->curtime + 1;
	else if (m_flPingTime <= gpGlobals->curtime)
	{
		m_flPingTime = gpGlobals->curtime + 1;
		CPASAttenuationFilter filter( this );
		enginesound->EmitSound( filter, entindex(), CHAN_ITEM, "turret/tu_ping.wav", 1, ATTN_NORM );

		EyeOn( );
	}
	else if (m_eyeBrightness > 0)
	{
		EyeOff( );
	}
}

//=========================================================
// MoveTurret - handle turret rotation
// returns 1 if the turret moved.
//=========================================================
int CNPC_BaseTurret::MoveTurret(void)
{
	int bMoved = 0;
	
	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.1 * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		if (m_iOrientation == TURRET_ORIENTATION_FLOOR)
			SetBoneController(1, m_vecCurAngles.x);
		else
			SetBoneController(1, -m_vecCurAngles.x);		

		bMoved = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);
		
		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if (flDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10)
			{
				m_fTurnRate += m_iBaseTurnRate;
			}
		}
		else if (m_fTurnRate > 45)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;

		if (m_vecCurAngles.y < 0)
			m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360)
			m_vecCurAngles.y -= 360;

		if (flDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;

		QAngle angles = GetAbsAngles();

		//ALERT(at_console, "%.2f -> %.2f\n", m_vecCurAngles.y, y);

		if (m_iOrientation == TURRET_ORIENTATION_FLOOR)
			SetBoneController(0, m_vecCurAngles.y - angles.y );
		else 
			SetBoneController(0, angles.y - 180 - m_vecCurAngles.y );
		bMoved = 1;
	}

	if (!bMoved)
		m_fTurnRate = m_iBaseTurnRate;

	//DevMsg(1, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x, 
	//	m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y);

	return bMoved;
}

void CNPC_BaseTurret::TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_iOn ) )
		return;

	if (m_iOn)
	{
		SetEnemy( NULL );
		SetNextThink( gpGlobals->curtime + 0.1 );
		m_iAutoStart = FALSE;// switching off a turret disables autostart
		//!!!! this should spin down first!!BUGBUG
		SetThink(&CNPC_BaseTurret::Retire);
	}
	else 
	{
		SetNextThink( gpGlobals->curtime + 0.1 ); // turn on delay

		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if ( m_spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE )
		{
			m_iAutoStart = TRUE;
		}
		
		SetThink(&CNPC_BaseTurret::Deploy);
	}
}

void CNPC_BaseTurret::InputDeactivate( inputdata_t &inputdata )
{
	if( m_iOn && m_lifeState == LIFE_ALIVE )
	{
		SetEnemy( NULL );
		SetNextThink( gpGlobals->curtime + 0.1 );
		m_iAutoStart = FALSE;// switching off a turret disables autostart
		//!!!! this should spin down first!!BUGBUG
		SetThink(&CNPC_BaseTurret::Retire);
	}
}

void CNPC_BaseTurret::InputActivate( inputdata_t &inputdata )
{
	if( !m_iOn && m_lifeState == LIFE_ALIVE )
	{
		SetNextThink( gpGlobals->curtime + 0.1 ); // turn on delay

		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if ( m_spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE )
		{
			m_iAutoStart = TRUE;
		}
		
		SetThink(&CNPC_BaseTurret::Deploy);
	}
}


//=========================================================
// EyeOn - turn on light on the turret
//=========================================================
void CNPC_BaseTurret::EyeOn(void)
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness != 255)
		{
			m_eyeBrightness = 255;
		}
		m_pEyeGlow->SetBrightness( m_eyeBrightness );
	}
}

//=========================================================
// EyeOn - turn off light on the turret
//=========================================================
void CNPC_BaseTurret::EyeOff(void)
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness > 0)
		{
			m_eyeBrightness = max( 0, m_eyeBrightness - 30 );
			m_pEyeGlow->SetBrightness( m_eyeBrightness );
		}
	}
}

void CNPC_BaseTurret::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	SetMoveType( MOVETYPE_FLY );
}

//===

class CNPC_MiniTurret : public CNPC_BaseTurret
{
	DECLARE_CLASS( CNPC_MiniTurret, CNPC_BaseTurret );

public:
	void Spawn( );
	void Precache(void);

	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
};

class CNPC_Turret : public CNPC_BaseTurret
{
	DECLARE_CLASS( CNPC_Turret, CNPC_BaseTurret );

public:
	void Spawn(void);
	void Precache(void);

	// Think functions
	void SpinUpCall(void);
	void SpinDownCall(void);

	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

private:
	int m_iStartSpin;
};

LINK_ENTITY_TO_CLASS( monster_turret, CNPC_Turret );
LINK_ENTITY_TO_CLASS( monster_miniturret, CNPC_MiniTurret );

ConVar	sk_turret_health ( "sk_turret_health","50");	
ConVar	sk_miniturret_health ( "sk_miniturret_health","40");
ConVar	sk_sentry_health ( "sk_sentry_health","40");

void CNPC_Turret::Spawn()
{ 
	Precache( );
	SetModel( "models/turret.mdl" );
	m_iHealth 			= sk_turret_health.GetFloat();
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin =		TURRET_MAXSPIN;

	Vector view_ofs( 0, 0, 12.75 );
	SetViewOffset( view_ofs );

	CNPC_BaseTurret::Spawn( );

	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -90;
	UTIL_SetSize(this, Vector(-32, -32, -m_iRetractHeight), Vector(32, 32, m_iRetractHeight));
	
	SetThink(&CNPC_BaseTurret::Initialize);	

	m_pEyeGlow = CSprite::SpriteCreate( TURRET_GLOW_SPRITE, GetAbsOrigin(), FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( this, 2 );

	m_eyeBrightness = 0;

	SetNextThink( gpGlobals->curtime + 0.3 ); 
}

void CNPC_Turret::Precache()
{
	CNPC_BaseTurret::Precache( );
	engine->PrecacheModel ("models/turret.mdl");	
	engine->PrecacheModel (TURRET_GLOW_SPRITE);

	engine->PrecacheModel( "sprites/xspark4.vmt" );

	//precache sounds
}

void CNPC_Turret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CPASAttenuationFilter filter( this );

	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, m_iAmmoType, 1 );

	enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, "turret/tu_fire1.wav", TURRET_MACHINE_VOLUME, 0.6 );

	m_fEffects |= EF_MUZZLEFLASH;
}

void CNPC_Turret::SpinUpCall(void)
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1 );

	// Are we already spun up? If not start the two stage process.
	if (!m_iSpin)
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
		// for the first pass, spin up the the barrel
		if (!m_iStartSpin)
		{
			SetNextThink( gpGlobals->curtime + 1.0 );	//spinup delay
			CPASAttenuationFilter filter( this );
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_spinup.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
			m_iStartSpin = 1;
			m_flPlaybackRate = 0.1;
		}
		// after the barrel is spun up, turn on the hum
		else if (m_flPlaybackRate >= 1.0)
		{
			SetNextThink( gpGlobals->curtime + 0.1 );// retarget delay
			CPASAttenuationFilter filter( this );
			enginesound->EmitSound( filter, entindex(), CHAN_STATIC, "turret/tu_active2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
			SetThink(&CNPC_BaseTurret::ActiveThink);
			m_iStartSpin = 0;
			m_iSpin = 1;
		} 
		else
		{
			m_flPlaybackRate += 0.075;
		}
	}

	if (m_iSpin)
	{
		SetThink(&CNPC_BaseTurret::ActiveThink);
	}
}

void CNPC_Turret::SpinDownCall(void)
{
	if (m_iSpin)
	{
		CPASAttenuationFilter filter( this );

		SetTurretAnim(TURRET_ANIM_SPIN);

		if ( m_flPlaybackRate == 1.0)
		{
			enginesound->StopSound( entindex(), CHAN_WEAPON, "turret/tu_active2.wav" );
			enginesound->EmitSound( filter, entindex(), CHAN_ITEM, "turret/tu_spindown.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, PITCH_NORM + random->RandomInt(-10,10) );
		}
		m_flPlaybackRate -= 0.02;
		if (m_flPlaybackRate <= 0)
		{
			m_flPlaybackRate = 0;
			m_iSpin = 0;
		}
	}
}

void CNPC_MiniTurret::Spawn()
{ 
	Precache( );

	SetModel( "models/miniturret.mdl" );
	m_iHealth 			= sk_miniturret_health.GetFloat();
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin = 0;

	Vector view_ofs( 0, 0, 12.75 );
	SetViewOffset( view_ofs );

	CNPC_BaseTurret::Spawn( );
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -90;
	UTIL_SetSize(this, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetThink(&CNPC_MiniTurret::Initialize);	
	SetNextThink(gpGlobals->curtime + 0.3); 

	if (( m_spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE ) && !( m_spawnflags & SF_MONSTER_TURRET_STARTINACTIVE ))
	{
		m_iAutoStart = true;
	}
}


void CNPC_MiniTurret::Precache()
{
	CNPC_BaseTurret::Precache( );

	m_iAmmoType = GetAmmoDef()->Index("9mmRound");

	engine->PrecacheModel ("models/miniturret.mdl");	
	enginesound->PrecacheSound("weapons/hks1.wav");
	enginesound->PrecacheSound("weapons/hks2.wav");
	enginesound->PrecacheSound("weapons/hks3.wav");
}

void CNPC_MiniTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, m_iAmmoType, 1 );

	CPASAttenuationFilter filter( this );

	enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, "turret/tu_fire1.wav", TURRET_MACHINE_VOLUME, 1 );
	
	m_fEffects |= EF_MUZZLEFLASH;
}

//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CNPC_Sentry : public CNPC_BaseTurret
{
	DECLARE_CLASS( CNPC_Sentry, CNPC_BaseTurret );

public:
	void Spawn( );
	void Precache(void);

	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int OnTakeDamage_Alive(const CTakeDamageInfo &info);
	void Event_Killed( const CTakeDamageInfo &info );
	void SentryTouch( CBaseEntity *pOther );

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CNPC_Sentry )
	DEFINE_ENTITYFUNC( CNPC_Sentry, SentryTouch ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( monster_sentry, CNPC_Sentry );

void CNPC_Sentry::Precache()
{
	BaseClass::Precache( );

	m_iAmmoType = GetAmmoDef()->Index("9mmRound");

	engine->PrecacheModel ("models/sentry.mdl");
	enginesound->PrecacheSound("weapons/hks1.wav");
	enginesound->PrecacheSound("weapons/hks2.wav");
	enginesound->PrecacheSound("weapons/hks3.wav");
}

void CNPC_Sentry::Spawn()
{ 
	Precache( );
	SetModel( "models/sentry.mdl" );
	m_iHealth			= sk_sentry_health.GetFloat();
	m_HackedGunPos		= Vector( 0, 0, 48 );


	SetViewOffset( Vector(0,0,48) );

	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;

	BaseClass::Spawn();

	SetSequence( TURRET_ANIM_RETIRE );
	m_flCycle = 0.0;
	m_flPlaybackRate = 0.0;

	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch	= -60;

	UTIL_SetSize(this, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetTouch(&CNPC_Sentry::SentryTouch);
	SetThink(&CNPC_Sentry::Initialize);	

	SetNextThink(gpGlobals->curtime + 0.3); 
}

void CNPC_Sentry::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, m_iAmmoType, 1 );

	CPASAttenuationFilter filter( this );

	switch(random->RandomInt(0,2))
	{
	case 0: 
		enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, "weapons/hks1.wav", TURRET_MACHINE_VOLUME, 1 );
		break;
	case 1:
		enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, "weapons/hks2.wav", TURRET_MACHINE_VOLUME, 1 );
		break;
	case 2:
	default:
		enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, "weapons/hks3.wav", TURRET_MACHINE_VOLUME, 1 );
		break;
	}

	SETBITS( m_fEffects, EF_MUZZLEFLASH );
}

int CNPC_Sentry::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	if (!m_iOn)
	{
		SetThink( &CNPC_Sentry::Deploy );
		SetUse( NULL );
		SetNextThink( gpGlobals->curtime + 0.1 );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

void CNPC_Sentry::Event_Killed( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );

	switch( random->RandomInt(0,2) )
	{
	case 0: 
		enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_die.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
		break;
	case 1:
		enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_die2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
		break;
	case 2:
	default:
		enginesound->EmitSound( filter, entindex(), CHAN_BODY, "turret/tu_die3.wav", TURRET_MACHINE_VOLUME, ATTN_NORM );
		break;
	}

	enginesound->StopSound( entindex(), CHAN_STATIC, "turret/tu_active2.wav" );

	SetSolidFlags( FSOLID_NOT_STANDABLE );

	Vector vecSrc;
	QAngle vecAng;
	GetAttachment( 2, vecSrc, vecAng );
	
	te->Smoke( filter, 0.0, &vecSrc,
		g_sModelIndexSmoke,
		2.5,
		10 );

	g_pEffects->Sparks( vecSrc );

	BaseClass::Event_Killed( info );
}


void CNPC_Sentry::SentryTouch( CBaseEntity *pOther )
{
	//trigger the sentry to turn on if a monster or player touches it
	if ( pOther && (pOther->IsPlayer() || FBitSet ( pOther->GetFlags(), FL_NPC )) )
	{
		CTakeDamageInfo info;
		info.SetAttacker( pOther );
		info.SetInflictor( pOther );
		info.SetDamage( 0 );

		TakeDamage(info);
	}
}

