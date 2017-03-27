//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Combine Guard - massive armored cyborg
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Motor.h"
#include "AI_Memory.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "NPCEvent.h"
#include "Player.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "soundent.h"
#include "Gib.h"
#include "shake.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_homer.h"
#include "ndebugoverlay.h"
#include "AI_BaseNPC.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

class CSprite;

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );
extern void UTIL_RotorWash( const Vector &origin, const Vector &direction, float maxDistance );

#define	COMBINEGUARD_MODEL	"models/combine_guard.mdl"

#define CGUARD_MSG_SHOT			1
#define CGUARD_MSG_SHOT_START	2

//Reference point definitions
enum 
{
	CGUARD_REF_INVALID			= 0,
	CGUARD_REF_MUZZLE,
	CGUARD_REF_LEFT_SHOULDER,
	CGUARD_REF_HUMAN_HEAD,
	CGUARD_REF_RIGHT_ARM_HIGH,
	CGUARD_REF_RIGHT_ARM_LOW,
	CGUARD_REF_LEFT_ARM_HIGH,
	CGUARD_REF_LEFT_ARM_LOW,
	CGUARD_REF_TORSO,
	CGUARD_REF_LOWER_TORSO,
	CGUARD_REF_RIGHT_THIGH_HIGH,
	CGUARD_REF_RIGHT_THIGH_LOW,
	CGUARD_REF_LEFT_THIGH_HIGH,
	CGUARD_REF_LEFT_THIGH_LOW,
	CGUARD_REF_RIGHT_SHIN_HIGH,
	CGUARD_REF_RIGHT_SHIN_LOW,
	CGUARD_REF_LEFT_SHIN_HIGH,
	CGUARD_REF_LEFT_SHIN_LOW,
	CGUARD_REF_RIGHT_SHOULDER,
	
	NUM_CGUARD_ATTACHMENTS,
};

//Bodygroup definitions
enum
{
	CGUARD_BGROUP_INVALID	= -1,
	CGUARD_BGROUP_MAIN,
	CGUARD_BGROUP_GUN,
	CGUARD_BGROUP_RIGHT_SHOULDER,
	CGUARD_BGROUP_LEFT_SHOULDER,
	CGUARD_BGROUP_HEAD,
	CGUARD_BGROUP_RIGHT_ARM,
	CGUARD_BGROUP_LEFT_ARM,
	CGUARD_BGROUP_TORSO,
	CGUARD_BGROUP_LOWER_TORSO,
	CGUARD_BGROUP_RIGHT_THIGH,
	CGUARD_BGROUP_LEFT_THIGH,
	CGUARD_BGROUP_RIGHT_SHIN,
	CGUARD_BGROUP_LEFT_SHIN,

	NUM_CGUARD_BODYGROUPS,
};

// Armor piece information structure
struct armorPiece_t
{
	DECLARE_DATADESC();

	bool	destroyed;
	int		health;
};

//Combine Guard
class CNPC_CombineGuard : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_CombineGuard, CAI_BaseNPC );
public:
	DECLARE_SERVERCLASS();

	CNPC_CombineGuard( void );

	int				OnTakeDamage( const CTakeDamageInfo &info );
	int				TranslateSchedule( int type );
	int				MeleeAttack1Conditions( float flDot, float flDist );
	int				RangeAttack1Conditions( float flDot, float flDist );

	void			Precache( void );
	void			Spawn( void );
	void			PrescheduleThink( void );
	void			TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	void			HandleAnimEvent( animevent_t *pEvent );
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );

	bool			AllArmorDestroyed( void );
	bool			IsArmorPiece( int iArmorPiece );

	float			MaxYawSpeed( void );

	Class_T			Classify( void ) { return CLASS_COMBINE; }

	Activity		NPC_TranslateActivity( Activity baseAct );

	virtual int		SelectSchedule( void );

	void			BuildScheduleTestBits( void );

	DECLARE_DATADESC();

private:
	bool			m_fOffBalance;
	float			m_flNextClobberTime;

	int				GetReferencePointForBodyGroup( int bodyGroup );

	void			InitArmorPieces( void );
	void			DamageArmorPiece( int pieceID, float damage, const Vector &vecOrigin, const Vector &vecDir );
	void			DestroyArmorPiece( int pieceID );
	void			Shove( void );
	void			FireRangeWeapon( void );

	float			GetLegDamage( void );

	bool			AimGunAt( CBaseEntity *pEntity, float flInterval );

	CSprite			*m_pGlowSprite[NUM_CGUARD_ATTACHMENTS];

	//static const char	*m_pScaredSounds[]; 

	armorPiece_t	m_armorPieces[NUM_CGUARD_BODYGROUPS];

	int				m_nGibModel;

	float			m_flGlowTime;
	float			m_flLastRangeTime;

	//Aiming
	float			m_aimYaw;
	float			m_aimPitch;

	int				m_YawControl;
	int				m_PitchControl;
	int				m_MuzzleAttachment;

	DEFINE_CUSTOM_AI;
};

ConVar	sk_combineguard_health( "sk_combineguard_health", "0" );

//FIXME: Make a cvar
#define	CGUARD_DEFAULT_ARMOR_HEALTH		50

#define	COMBINEGUARD_MELEE1_RANGE		92
#define	COMBINEGUARD_MELEE1_CONE		0.5f

#define	COMBINEGUARD_RANGE1_RANGE		1024
#define	COMBINEGUARD_RANGE1_CONE		0.0f

#define	CGUARD_GLOW_TIME				0.5f

IMPLEMENT_SERVERCLASS_ST(CNPC_CombineGuard, DT_NPC_CombineGuard)
END_SEND_TABLE()

//Data description
BEGIN_DATADESC_NO_BASE( armorPiece_t )
	DEFINE_FIELD( armorPiece_t, destroyed,	FIELD_BOOLEAN ),
	DEFINE_FIELD( armorPiece_t, health,		FIELD_INTEGER ),
END_DATADESC()


BEGIN_DATADESC( CNPC_CombineGuard )

	DEFINE_FIELD( CNPC_CombineGuard, m_fOffBalance, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGuard, m_flNextClobberTime, FIELD_TIME ),
	DEFINE_ARRAY( CNPC_CombineGuard, m_pGlowSprite, FIELD_CLASSPTR, NUM_CGUARD_ATTACHMENTS ),
	DEFINE_EMBEDDED_AUTO_ARRAY( CNPC_CombineGuard, m_armorPieces ),
	DEFINE_FIELD( CNPC_CombineGuard, m_nGibModel,	FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGuard, m_flGlowTime,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineGuard, m_flLastRangeTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineGuard, m_aimYaw,		FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGuard, m_aimPitch,	FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGuard, m_YawControl,	FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGuard, m_PitchControl, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGuard, m_MuzzleAttachment, FIELD_INTEGER ),

END_DATADESC()

//Schedules	
enum CombineGuardSchedules 
{	
	SCHED_CGUARD_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_COMBINEGUARD_CLOBBERED,
	SCHED_COMBINEGUARD_TOPPLE,
	SCHED_COMBINEGUARD_HELPLESS,
};

//Tasks	
enum CombineGuardTasks 
{	
	TASK_CGUARD_RANGE_ATTACK1 = LAST_SHARED_TASK,
	TASK_COMBINEGUARD_SET_BALANCE,
};

//Conditions 
enum CombineGuardConditions
{	
	COND_COMBINEGUARD_CLOBBERED = LAST_SHARED_CONDITION,
};

//Activities
int	ACT_CGUARD_IDLE_TO_ANGRY;
int ACT_COMBINEGUARD_CLOBBERED;
int ACT_COMBINEGUARD_TOPPLE;
int ACT_COMBINEGUARD_GETUP;
int ACT_COMBINEGUARD_HELPLESS;

//Animation events
#define	CGUARD_AE_SHOVE					11
#define	CGUARD_AE_FIRE					12
#define	CGUARD_AE_FIRE_START			13
#define	CGUARD_AE_GLOW					14
#define CGUARD_AE_LEFTFOOT				20 // footsteps
#define CGUARD_AE_RIGHTFOOT				21 // footsteps
#define CGUARD_AE_SHAKEIMPACT			22 // hard body impact that makes screenshake.


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_CombineGuard::CNPC_CombineGuard( void )
{
}

LINK_ENTITY_TO_CLASS( npc_combineguard, CNPC_CombineGuard );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::Precache( void )
{
	engine->PrecacheModel( COMBINEGUARD_MODEL );

	engine->PrecacheModel("sprites/blueflare1.vmt");

	BaseClass::Precache();

//	m_nGibModel = engine->PrecacheModel( "models/gibs/metalgibs.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::InitArmorPieces( void )
{
	//Turn on all body groups
	for ( int i = 1; i < NUM_CGUARD_BODYGROUPS; i++ )
	{
		SetBodygroup( i, true );

		m_armorPieces[i].health		= CGUARD_DEFAULT_ARMOR_HEALTH;


		if( IsArmorPiece( i ) )
		{
			m_armorPieces[i].destroyed	= false;
		}
		else
		{
			// Start all non-armor bodygroups as destroyed so
			// that the code to pick and destroy random armor pieces
			// doesn't destroy pieces that aren't represented visually
			// (such as the head, torso, etc).
			m_armorPieces[i].destroyed	= true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::Spawn( void )
{
	Precache();

	SetModel( COMBINEGUARD_MODEL );

	SetHullType(HULL_LARGE);
	
	SetNavType(NAV_GROUND);
	m_NPCState				= NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_YELLOW );
	m_iHealth				= 100;
	m_iMaxHealth			= m_iHealth;
	m_flFieldOfView			= 0.1f;
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	m_flGlowTime			= gpGlobals->curtime;
	m_flLastRangeTime		= gpGlobals->curtime;
	m_aimYaw				= 0;
	m_aimPitch				= 0;

	m_flNextClobberTime		= gpGlobals->curtime;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 );

	//DEBUG: Attach sprites to all the reference points
	for ( int i = 1; i < NUM_CGUARD_ATTACHMENTS; i++ )
	{
		m_pGlowSprite[i] = CSprite::SpriteCreate( "sprites/blueflare1.vmt", GetAbsOrigin(), false );
		
		Assert( m_pGlowSprite[i] );

		if ( m_pGlowSprite[i] == NULL )
			return;

		m_pGlowSprite[i]->TurnOff();
		m_pGlowSprite[i]->SetTransparency( kRenderGlow, 16, 16, 16, 255, kRenderFxNoDissipation );
		m_pGlowSprite[i]->SetScale( 1.0f );
		m_pGlowSprite[i]->SetAttachment( this, i );
	}

	NPCInit();
	
	//Create and init all armor pieces
	InitArmorPieces();
	
	//Turn the gun on
	SetBodygroup( 1, true );

	m_YawControl		= LookupPoseParameter( "aim_yaw" );
	m_PitchControl		= LookupPoseParameter( "aim_pitch" );
	m_MuzzleAttachment	= LookupAttachment( "muzzle" );

	BaseClass::Spawn();

	m_fOffBalance = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( HasCondition( COND_COMBINEGUARD_CLOBBERED ) )
	{
		Msg( "CLOBBERED!\n" );
	}

	for ( int i = 1; i < NUM_CGUARD_ATTACHMENTS; i++ )
	{
		if ( m_pGlowSprite[i] == NULL )
			continue;

		if ( m_flGlowTime > gpGlobals->curtime )
		{
			float	brightness;
			float	perc = ( m_flGlowTime - gpGlobals->curtime ) / CGUARD_GLOW_TIME;

			m_pGlowSprite[i]->TurnOn();

			//Random brightness
			brightness = perc * 92.0f;
			m_pGlowSprite[i]->SetTransparency( kRenderGlow, brightness, brightness, brightness, 255, kRenderFxNoDissipation );
			
			//Random scale
			m_pGlowSprite[i]->SetScale( perc * 1.0f );
		}
		else
		{
			m_pGlowSprite[i]->TurnOff();
		}
	}

	//HACK: Aim at the player
	if ( GetEnemy() != NULL )
	{
		AimGunAt( GetEnemy(), 0.1f );
	}

	/*	
	int	id = CGUARD_REF_MUZZLE;
	m_pGlowSprite[id]->TurnOn();
	m_pGlowSprite[id]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pGlowSprite[id]->SetScale( 0.5f );
	*/

	Vector	vecDamagePoint;
	QAngle	vecDamageAngles;

	for ( i = 1; i < NUM_CGUARD_BODYGROUPS; i++ )
	{
		if ( m_armorPieces[i].destroyed )
		{
			int	referencePoint = GetReferencePointForBodyGroup( i );

			if ( referencePoint == CGUARD_REF_INVALID )
				continue;

			GetAttachment( referencePoint, vecDamagePoint, vecDamageAngles );

#if 0
			if ( random->RandomInt( 0, 4 ) == 0 )
			{
				if ( random->RandomInt( 0, 800 ) == 0 )
				{
					ExplosionCreate( vecDamagePoint, vecDamageAngles, this, 0.5f, 0, false );
				}
				else
				{
					/*//FIXME: Smoke function was changed under our nose so it doesn't work
					float	scale = random->RandomInt( 4, 12 );

					vecDamagePoint[2] -= random->RandomFloat( 24, 32 );
					g_pEffects->Smoke( vecDamagePoint, random->RandomInt( 15, 30 ), scale, 10);
					*/
				}
			}
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::HandleAnimEvent( animevent_t *pEvent )
{
	
	switch ( pEvent->event )
	{
	case CGUARD_AE_SHAKEIMPACT:
		UTIL_RotorWash( WorldSpaceCenter(), Vector( 0, 0, -1 ), 128 );
		UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
		break;

	case CGUARD_AE_LEFTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepLeft" );
		}
		break;

	case CGUARD_AE_RIGHTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepRight" );
		}
		break;
	
	case CGUARD_AE_SHOVE:
		Shove();
		break;

	case CGUARD_AE_FIRE:
		{
			m_flLastRangeTime = gpGlobals->curtime + 6.0f;
			FireRangeWeapon();
			
			EmitSound( "NPC_CombineGuard.Fire" );

			CEntityMessageFilter emfilter( this, "CNPC_CombineGuard" );

			MessageBegin( emfilter, 0 );
				WRITE_BYTE( CGUARD_MSG_SHOT );
			MessageEnd();
		}
		break;

	case CGUARD_AE_FIRE_START:
		{
			EmitSound( "NPC_CombineGuard.Charging" );

			CEntityMessageFilter emfilter( this, "CNPC_CombineGuard" );
			MessageBegin( emfilter, 0 );
				WRITE_BYTE( CGUARD_MSG_SHOT_START );
			MessageEnd();
		}
		break;

	case CGUARD_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + CGUARD_GLOW_TIME;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::Shove( void )
{
	if ( GetEnemy() == NULL )
		return;

	CBaseEntity *pHurt = NULL;

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );

	float		flDist	= ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).Length();
	Vector2D	v2LOS	= ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).AsVector2D();
	Vector2DNormalize(v2LOS);
	float		flDot	= DotProduct2D (v2LOS , forward.AsVector2D() );

	flDist -= GetEnemy()->WorldAlignSize().x * 0.5f;

	if ( flDist < COMBINEGUARD_MELEE1_RANGE && flDot >= COMBINEGUARD_MELEE1_CONE )
	{
		Vector vStart = GetAbsOrigin();
		vStart.z += WorldAlignSize().z * 0.5;

		Vector vEnd = GetEnemy()->GetAbsOrigin();
		vEnd.z += GetEnemy()->WorldAlignSize().z * 0.5;

		pHurt = CheckTraceHullAttack( vStart, vEnd, Vector(-16,-16,0), Vector(16,16,24), 25, DMG_CLUB );
	}

	if ( pHurt )
	{
		pHurt->ViewPunch( QAngle(-20,0,20) );
				
		//Shake the screen
		UTIL_ScreenShake( pHurt->GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START );
		
		color32 white = {255,255,255,64};
		UTIL_ScreenFade( pHurt, white, 0.5f, 0.1f, FFADE_IN );

		// If the player, throw him around
		if ( pHurt->IsPlayer() )
		{
			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );
			pHurt->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CombineGuard::SelectSchedule( void )
{
	if( HasCondition( COND_COMBINEGUARD_CLOBBERED ) )
	{
		ClearCondition( COND_COMBINEGUARD_CLOBBERED );

		if( m_fOffBalance )
		{
			//Randomly destroy an armor piece
			int iArmorPiece;
			do 
			{

				iArmorPiece = random->RandomInt( CGUARD_BGROUP_RIGHT_SHOULDER, CGUARD_BGROUP_LEFT_SHIN );

			} while( m_armorPieces[ iArmorPiece ].destroyed  );

			DestroyArmorPiece( iArmorPiece );

			Msg("DESTROYING PIECE:%d\n", iArmorPiece );

			if( AllArmorDestroyed() )
			{
				//go into die schedule
				Msg(" NO!!!!!!!! I'M DEADZ0R!!\n" );
				return SCHED_COMBINEGUARD_HELPLESS;
			}
			else
			{
				return SCHED_COMBINEGUARD_TOPPLE;
			}
		}
		else
		{
			return SCHED_COMBINEGUARD_CLOBBERED;
		}
	}

	//Otherwise just walk around a little
	//return SCHED_PATROL_WALK;
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_COMBINEGUARD_SET_BALANCE:
		if( pTask->flTaskData == 0.0 )
		{
			m_fOffBalance = false;
		}
		else
		{
			m_fOffBalance = true;
		}

		TaskComplete();

		break;

	case TASK_CGUARD_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		SetActivity( ACT_RANGE_ATTACK1 );
		return;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CGUARD_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );

			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}
		}
		return;
	}

	BaseClass::RunTask( pTask );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_CombineGuard::GetLegDamage( void )
{
	float damageTotal = 0.0f;

	if ( m_armorPieces[CGUARD_BGROUP_RIGHT_THIGH].destroyed )
	{
		damageTotal += 0.25f;
	}

	if ( m_armorPieces[CGUARD_BGROUP_LEFT_THIGH].destroyed )
	{
		damageTotal += 0.25f;
	}

	if ( m_armorPieces[CGUARD_BGROUP_RIGHT_SHIN].destroyed )
	{
		damageTotal += 0.25f;
	}

	if ( m_armorPieces[CGUARD_BGROUP_LEFT_SHIN].destroyed )
	{
		damageTotal += 0.25f;
	}

	return damageTotal;
}

#define TRANSLATE_SCHEDULE( type, in, out ) { if ( type == in ) return out; }

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseAct - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CNPC_CombineGuard::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_RUN )
	{
		float	legDamage = GetLegDamage();

		if ( legDamage > 0.75f )
		{
			//TODO: LIMP
			return (Activity) ACT_WALK;
		}
		else if ( legDamage > 0.5f )
		{
			return (Activity) ACT_WALK;
		}
	}

	if( baseAct == ACT_RUN )
	{
		// Don't run at all.
		return (Activity)ACT_WALK;
	}
	
	//Translate our idle if we're angry
	if ( baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE )
	{
		return (Activity) ACT_IDLE_ANGRY;
	}

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose: override/translate a schedule by type
// Input  : Type - schedule type
// Output : int - translated type
//-----------------------------------------------------------------------------
int CNPC_CombineGuard::TranslateSchedule( int type ) 
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_CGUARD_RANGE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bodyGroup - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_CombineGuard::GetReferencePointForBodyGroup( int bodyGroup )
{
	switch ( bodyGroup )
	{
	case CGUARD_BGROUP_MAIN:
	case CGUARD_BGROUP_GUN:
		return CGUARD_REF_INVALID;
		break;

	case CGUARD_BGROUP_RIGHT_SHOULDER:
		return CGUARD_REF_RIGHT_SHOULDER;
		break;

	case CGUARD_BGROUP_LEFT_SHOULDER:
		return CGUARD_REF_LEFT_SHOULDER;
		break;

	//FIXME: This is a special case and should not continue to emit sparks/smoke!
	case CGUARD_BGROUP_HEAD:
		return CGUARD_REF_HUMAN_HEAD;
		break;

	case CGUARD_BGROUP_RIGHT_ARM:
		if ( random->RandomInt( 0, 1 ) )
			return CGUARD_REF_RIGHT_ARM_LOW;
		else
			return CGUARD_REF_RIGHT_ARM_HIGH;
		
		break;

	case CGUARD_BGROUP_LEFT_ARM:
		if ( random->RandomInt( 0, 1 ) )
			return CGUARD_REF_LEFT_ARM_LOW;
		else
			return CGUARD_REF_LEFT_ARM_HIGH;

		break;

	case CGUARD_BGROUP_TORSO:
		return CGUARD_REF_TORSO;
		break;

	case CGUARD_BGROUP_RIGHT_THIGH:
		if ( random->RandomInt( 0, 1 ) )
			return CGUARD_REF_RIGHT_THIGH_LOW;
		else
			return CGUARD_REF_RIGHT_THIGH_HIGH;

		break;

	case CGUARD_BGROUP_LEFT_THIGH:
		if ( random->RandomInt( 0, 1 ) )
			return CGUARD_REF_LEFT_THIGH_LOW;
		else
			return CGUARD_REF_LEFT_THIGH_HIGH;

		break;

	case CGUARD_BGROUP_RIGHT_SHIN:
		if ( random->RandomInt( 0, 1 ) )
			return CGUARD_REF_RIGHT_SHIN_LOW;
		else
			return CGUARD_REF_RIGHT_SHIN_HIGH;
		break;

	case CGUARD_BGROUP_LEFT_SHIN:
		if ( random->RandomInt( 0, 1 ) )
			return CGUARD_REF_LEFT_SHIN_LOW;
		else
			return CGUARD_REF_LEFT_SHIN_HIGH;
		break;

	case CGUARD_BGROUP_LOWER_TORSO:
		return CGUARD_REF_LOWER_TORSO;
		break;
	}

	return CGUARD_REF_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pieceID - 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::DestroyArmorPiece( int pieceID )
{
	int	refPoint = GetReferencePointForBodyGroup( pieceID );

	if ( refPoint == CGUARD_REF_INVALID )
		return;

	Vector	vecDamagePoint;
	QAngle	vecDamageAngles;

	GetAttachment( refPoint, vecDamagePoint, vecDamageAngles );

	Vector	vecVelocity, vecSize;

	vecVelocity.Random( -1.0, 1.0 );
	vecVelocity *= random->RandomInt( 16, 64 );

	vecSize = Vector( 32, 32, 32 );

	CPVSFilter filter( vecDamagePoint );
	te->BreakModel( filter, 0.0, &vecDamagePoint, &vecSize, &vecVelocity, m_nGibModel, 100, 0, 2.5, BREAK_METAL );

	//ExplosionCreate( vecDamagePoint, vecDamageAngles, this, 16.0f, 0, false );
	
	m_armorPieces[pieceID].destroyed = true;
	SetBodygroup( pieceID, false );

	//For interrupting our behavior
	SetCondition( COND_LIGHT_DAMAGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pieceID - 
//			damage - 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::DamageArmorPiece( int pieceID, float damage, const Vector &vecOrigin, const Vector &vecDir )
{
	//Destroyed pieces take no damage
	if ( m_armorPieces[pieceID].destroyed )
	{
		if ( ( random->RandomInt( 0, 8 ) == 0 ) && ( pieceID != CGUARD_BGROUP_HEAD ) )
		{
			g_pEffects->Ricochet( vecOrigin, (vecDir*-1.0f) );
		}

		return;
	}

	//Take the damage
	m_armorPieces[pieceID].health -= damage;

	//See if we've destroyed this piece
	if ( m_armorPieces[pieceID].health <= 0.0f )
	{
		DestroyArmorPiece( pieceID );
		return;
	}

	// Otherwise just spark
	g_pEffects->Sparks( vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAttacker - 
//			flDamage - 
//			&vecDir - 
//			*ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType )
{
	Vector	vecDamagePoint = ptr->endpos;

	//Approximate explosive damage
	if ( bitsDamageType & ( DMG_BLAST ) )
	{
		Vector	vecOrigin;
		QAngle  vecAngles;
		float	flNearestDist = 99999999.0f;
		float	flDist;
		int		nReferencePoint;
		int		nNearestGroup = CGUARD_BGROUP_MAIN;
		float	flAdjustedDamage = flDamage;

		//Check all pieces to find the nearest one
		for ( int i = 1; i < NUM_CGUARD_BODYGROUPS; i++ )
		{
			if ( m_armorPieces[i].destroyed )
			{
				//Lose 10% (dodgy falloff approximation)
				flAdjustedDamage *= 0.9f;
				continue;
			}

			nReferencePoint = GetReferencePointForBodyGroup( i );

			if ( nReferencePoint == CGUARD_REF_INVALID )
				continue;

			GetAttachment( nReferencePoint, vecOrigin, vecAngles );

			flDist = ( vecOrigin - ptr->endpos ).Length();
			
			//See if found a closer piece
			if ( flDist < flNearestDist )
			{
				flNearestDist = flDist;
				nNearestGroup = i;
			}
		}

		//If we found one, damage it
		if ( nNearestGroup != CGUARD_BGROUP_MAIN )
		{
			DamageArmorPiece( nNearestGroup, flAdjustedDamage, vecDamagePoint, vecDir );
			return;
		}
	}
	
	//Damage the hitgroup
	if ( ptr->hitgroup != CGUARD_BGROUP_MAIN )
	{
		DamageArmorPiece( ptr->hitgroup, flDamage, vecDamagePoint, vecDir );
	}
	else
	{
		//UTIL_Ricochet( vecDamagePoint, (vecDir*-1.0f) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
int	CNPC_CombineGuard::OnTakeDamage( const CTakeDamageInfo &info )
{
	if( info.GetDamageType() & DMG_CRUSH && info.GetInflictor() )
	{
		IPhysicsObject *pPhysObject;

		pPhysObject = info.GetInflictor()->VPhysicsGetObject();

		if( pPhysObject )
		{
			Msg(" HIT by DMG: %f\n", info.GetDamage() );

			// Smacked by a physics object
			if( info.GetDamage() >= 50.0 && gpGlobals->curtime > m_flNextClobberTime )
			{
				// Hit hard enough to knock me
				Msg(" DONK \n" );
				SetCondition( COND_COMBINEGUARD_CLOBBERED );

				// !!!HACKHACK - stop from being hit twice by the same object
				m_flNextClobberTime = gpGlobals->curtime + 0.5;
			}


/*
			Vector vecOrigin;
			Vector vecVelocity;

			pPhysObject->GetVelocity( &vecVelocity, NULL );
			pPhysObject->GetPosition( &vecOrigin, NULL );

			NDebugOverlay::Line( vecOrigin, vecOrigin + vecVelocity * 255, 0, 0, 255, false, 1.0 );			
*/
			
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_CombineGuard::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > COMBINEGUARD_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < COMBINEGUARD_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_CombineGuard::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > COMBINEGUARD_RANGE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < COMBINEGUARD_RANGE1_CONE )
		return COND_NOT_FACING_ATTACK;

	if ( m_flLastRangeTime > gpGlobals->curtime )
		return COND_TOO_FAR_TO_ATTACK;
		//return COND_NOT_FACING_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::FireRangeWeapon( void )
{
	//FIXME: E3 HACK - For now just trigger our target
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->Classify() == CLASS_BULLSEYE ) )
	{
		// DMG_GENERIC because we don't want to generate physics force
		GetEnemy()->TakeDamage( CTakeDamageInfo( this, this, 1.0f, DMG_GENERIC ) );
	}

	Vector vecSrc, vecAiming;

	GetVectors( &vecAiming, NULL, NULL );
	vecSrc = WorldSpaceCenter() + vecAiming * 64;
	
	Vector	impactPoint	= vecSrc + ( vecAiming * MAX_TRACE_LENGTH );

	trace_t	tr;
	AI_TraceLine( vecSrc, impactPoint, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	//vecDir = tr.endpos - vecTracerSrc;
	//flTracerDist = VectorNormalize( vecDir );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 2000 ), true, "GunshipTracer" );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 3000 ), true, "GunshipTracer" );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 4000 ), true, "GunshipTracer" );

	CreateConcussiveBlast( tr.endpos, tr.plane.normal, this, 1.0 );
}

#define	DEBUG_AIMING	0

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGuard::AimGunAt( CBaseEntity *pEntity, float flInterval )
{
	if ( !pEntity )
		return true;

	matrix3x4_t gunMatrix;
	GetAttachment( m_MuzzleAttachment, gunMatrix );

	// transform the enemy into gun space
	Vector localEnemyPosition;
	VectorITransform( pEntity->GetAbsOrigin(), gunMatrix, localEnemyPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float	targetYaw = m_aimYaw + localEnemyAngles.y;
	float	targetPitch = m_aimPitch + localEnemyAngles.x;
	
	QAngle	unitAngles = localEnemyAngles;
	float	angleDiff = sqrt( localEnemyAngles.y * localEnemyAngles.y + localEnemyAngles.x * localEnemyAngles.x );
	const float aimSpeed = 1;

	// Exponentially approach the target
	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed	= max(yawSpeed,15);
	pitchSpeed	= max(pitchSpeed,15);

	m_aimYaw	= UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch	= UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( m_YawControl, m_aimYaw );
	SetPoseParameter( m_PitchControl, m_aimPitch );

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch	= GetPoseParameter( m_PitchControl );
	m_aimYaw	= GetPoseParameter( m_YawControl );

	// UNDONE: Zero out any movement past the limit and go ahead and fire if the strider hit its 
	// target except for clamping.  Need to clamp targets to limits and compare?
	if ( angleDiff < 1 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_CombineGuard::MaxYawSpeed( void ) 
{ 	
	if ( GetActivity() == ACT_RANGE_ATTACK1 )
	{
		return 1.0f;
	}

	return 60.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_CombineGuard::BuildScheduleTestBits( void )
{
	//Interrupt any schedule if I get clobbered
	SetCustomInterruptCondition( COND_COMBINEGUARD_CLOBBERED );
}

//-----------------------------------------------------------------------------
// Purpose: Not every bodygroup is a piece of armor that can be destroyed. 
//			this function checks whether a bodygroup is a piece of armor.
// Input  : iArmorPiece- the bodygroup to check.
//-----------------------------------------------------------------------------
bool CNPC_CombineGuard::IsArmorPiece( int iArmorPiece )
{
	switch( iArmorPiece )
	{
	case CGUARD_BGROUP_MAIN:
	case CGUARD_BGROUP_GUN:
	case CGUARD_BGROUP_HEAD:
	case CGUARD_BGROUP_RIGHT_ARM:
	case CGUARD_BGROUP_LEFT_ARM:
	case CGUARD_BGROUP_TORSO:
	case CGUARD_BGROUP_LOWER_TORSO:
	//case CGUARD_BGROUP_RIGHT_THIGH:
	//case CGUARD_BGROUP_LEFT_THIGH:
	case CGUARD_BGROUP_RIGHT_SHIN:
	case CGUARD_BGROUP_LEFT_SHIN:
		return false;
		break;

	default:
		return true;
		break;
	}

/*
	This requires every visible armor piece be broken.

	switch( iArmorPiece )
	{
	case CGUARD_BGROUP_INVALID:
	case CGUARD_BGROUP_MAIN:
	case CGUARD_BGROUP_GUN:
	case CGUARD_BGROUP_HEAD:
		return false;
		break;

	default:
		return true;
		break;
	}
*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
bool CNPC_CombineGuard::AllArmorDestroyed( void )
{
	int i;

	for( i = CGUARD_BGROUP_RIGHT_SHOULDER ; i <= CGUARD_BGROUP_LEFT_SHIN ; i++ )
	{
		if( !m_armorPieces[ i ].destroyed )
		{
			// Found an intact piece of armor. It's not possible
			// that the whole suit is destroyed.
			return false;
		}
	}

	// We fell through. All pieces destroyed.
	return true;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_combineguard, CNPC_CombineGuard )

	//Interaction	DECLARE_INTERACTION( g_interactionAntlionAttacked )

	//Tasks	
	DECLARE_TASK( TASK_CGUARD_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_COMBINEGUARD_SET_BALANCE )
	
	//Conditions	
	DECLARE_CONDITION( COND_COMBINEGUARD_CLOBBERED )

	//Activities	
	DECLARE_ACTIVITY( ACT_CGUARD_IDLE_TO_ANGRY )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_CLOBBERED )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_TOPPLE )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_GETUP )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_HELPLESS )

	//==================================================
	// SCHED_CGUARD_RANGE_ATTACK1
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_CGUARD_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"
		"		TASK_CGUARD_RANGE_ATTACK1	0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_NO_PRIMARY_AMMO"
	)



	//==================================================
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINEGUARD_CLOBBERED,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_COMBINEGUARD_SET_BALANCE			1"
		"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_COMBINEGUARD_CLOBBERED"
		"		TASK_COMBINEGUARD_SET_BALANCE			0"
		"	"
		"	Interrupts"
	)


	//==================================================
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINEGUARD_TOPPLE,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_TOPPLE"
		"		TASK_WAIT						1"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_GETUP"
		"		TASK_COMBINEGUARD_SET_BALANCE	0"
		"	"
		"	Interrupts"
	)


	//==================================================
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINEGUARD_HELPLESS,

		"	Tasks"
		"	TASK_STOP_MOVING				0"
		"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_TOPPLE"
		"	TASK_WAIT						2"
		"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_HELPLESS"
		"	TASK_WAIT_INDEFINITE			0"
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()
