#include	"cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Route.h"
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
#include	"hl1_ai_basenpc.h"
#include	"AI_Navigator.h"
#include	"decals.h"
#include	"effect_dispatch_data.h"
#include	"te_effect_dispatch.h"
#include	"Sprite.h"

ConVar sk_bigmomma_health_factor( "sk_bigmomma_health_factor", "1" );
ConVar sk_bigmomma_dmg_slash( "sk_bigmomma_dmg_slash", "50" );
ConVar sk_bigmomma_dmg_blast( "sk_bigmomma_dmg_blast", "100" );
ConVar sk_bigmomma_radius_blast( "sk_bigmomma_radius_blast", "250" );

extern ConVar sv_gravity;


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BIG_AE_STEP1				1		// Footstep left
#define	BIG_AE_STEP2				2		// Footstep right
#define	BIG_AE_STEP3				3		// Footstep back left
#define	BIG_AE_STEP4				4		// Footstep back right
#define BIG_AE_SACK					5		// Sack slosh
#define BIG_AE_DEATHSOUND			6		// Death sound

#define	BIG_AE_MELEE_ATTACKBR		8		// Leg attack
#define	BIG_AE_MELEE_ATTACKBL		9		// Leg attack
#define	BIG_AE_MELEE_ATTACK1		10		// Leg attack
#define BIG_AE_MORTAR_ATTACK1		11		// Launch a mortar
#define BIG_AE_LAY_CRAB				12		// Lay a headcrab
#define BIG_AE_JUMP_FORWARD			13		// Jump up and forward
#define BIG_AE_SCREAM				14		// alert sound
#define BIG_AE_PAIN_SOUND			15		// pain sound
#define BIG_AE_ATTACK_SOUND			16		// attack sound
#define BIG_AE_BIRTH_SOUND			17		// birth sound
#define BIG_AE_EARLY_TARGET			50		// Fire target early


enum
{
	SCHED_NODE_FAIL = LAST_SHARED_SCHEDULE,
	SCHED_BIG_NODE,
};

enum
{
	TASK_MOVE_TO_NODE_RANGE = LAST_SHARED_TASK,		// Move within node range
	TASK_FIND_NODE,									// Find my next node
	TASK_PLAY_NODE_PRESEQUENCE,						// Play node pre-script
	TASK_PLAY_NODE_SEQUENCE,						// Play node script
	TASK_PROCESS_NODE,								// Fire targets, etc.
	TASK_WAIT_NODE,									// Wait at the node
	TASK_NODE_DELAY,								// Delay walking toward node for a bit. You've failed to get there
	TASK_NODE_YAW,									// Get the best facing direction for this node
	TASK_CHECK_NODE_PROXIMITY,
};

// User defined conditions
//#define bits_COND_NODE_SEQUENCE			( COND_SPECIAL1 )		// pev->netname contains the name of a sequence to play

// Attack distance constants
#define	BIG_ATTACKDIST		170
#define BIG_MORTARDIST		800
#define BIG_MAXCHILDREN		6			// Max # of live headcrab children


#define bits_MEMORY_CHILDPAIR		(bits_MEMORY_CUSTOM1)
#define bits_MEMORY_ADVANCE_NODE	(bits_MEMORY_CUSTOM2)
#define bits_MEMORY_COMPLETED_NODE	(bits_MEMORY_CUSTOM3)
#define bits_MEMORY_FIRED_NODE		(bits_MEMORY_CUSTOM4)

int gSpitSprite, gSpitDebrisSprite;


Vector VecCheckSplatToss( CBaseEntity *pEntity, const Vector &vecSpot1, Vector vecSpot2, float maxHeight );
void MortarSpray( const Vector &position, const Vector &direction, int spriteModel, int count );

#define SF_INFOBM_RUN		0x0001
#define SF_INFOBM_WAIT		0x0002

//=========================================================
// Mortar shot entity
//=========================================================
class CBMortar : public CBaseAnimating
{
	DECLARE_CLASS( CBMortar, CBaseAnimating );
public:
	void Spawn( void );

	static CBMortar *Shoot( CBaseEntity *pOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void Animate( void );

	float	m_flDmgTime;

	DECLARE_DATADESC();

	int  m_maxFrame;
	int  m_iFrame;

	CSprite* pSprite;
};

LINK_ENTITY_TO_CLASS( bmortar, CBMortar );

BEGIN_DATADESC( CBMortar )
	DEFINE_FIELD( CBMortar, m_maxFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CBMortar, m_flDmgTime, FIELD_FLOAT ),
	DEFINE_FUNCTION( CBMortar, Animate ),
END_DATADESC()


// AI Nodes for Big Momma
class CInfoBM : public CPointEntity
{
	DECLARE_CLASS( CInfoBM, CPointEntity );
public:
	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );

	// name in pev->targetname
	// next in pev->target
	// radius in pev->scale
	// health in pev->health
	// Reach target in pev->message
	// Reach delay in pev->speed
	// Reach sequence in pev->netname
	
	DECLARE_DATADESC();

	int		m_preSequence;
	int		m_curSequence;
	float   m_flRadius;
	float   m_flDelay;
	string_t m_iszReachTarget;
	string_t m_iszReachSequence;
	string_t m_iszPreSequence;
	const char *m_iszNameHack;

	COutputEvent m_OnAnimationEvent;
};

LINK_ENTITY_TO_CLASS( info_bigmomma, CInfoBM );

BEGIN_DATADESC( CInfoBM )
	DEFINE_FIELD( CInfoBM, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CInfoBM, m_flDelay, FIELD_FLOAT ),
	DEFINE_KEYFIELD( CInfoBM, m_iszReachTarget, FIELD_STRING, "reachtarget" ),
	DEFINE_KEYFIELD( CInfoBM, m_iszReachSequence, FIELD_STRING, "reachsequence" ),
	DEFINE_KEYFIELD( CInfoBM, m_iszPreSequence, FIELD_STRING, "presequence" ),
	DEFINE_OUTPUT( CInfoBM, m_OnAnimationEvent, "OnAnimationEvent" ),
END_DATADESC()


void CInfoBM::Spawn( void )
{
	BaseClass::Spawn();

//	Msg( "Name %s\n", STRING( GetEntityName() ) );
}

bool CInfoBM::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if (FStrEq( szKeyName, "radius"))
	{
		m_flRadius = atof( szValue );
		return true;
	}
	else if (FStrEq( szKeyName, "reachdelay" ))
	{
		m_flDelay = atof( szValue);
		return true;
	}
	else if (FStrEq( szKeyName, "health" ))
	{
		m_iHealth = atoi( szValue );
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue );
}

// UNDONE:	
//
#define BIG_CHILDCLASS		"monster_babycrab"


class CNPC_BigMomma : public CHL1BaseNPC
{
	DECLARE_CLASS( CNPC_BigMomma, CHL1BaseNPC );
public:

	void Spawn( void );
	void Precache( void );

	static const char *pChildDieSounds[];
	static const char *pSackSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackSounds[];
	static const char *pAttackHitSounds[];
	static const char *pBirthSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pFootSounds[];
	
	Class_T	Classify( void ) { return CLASS_ALIEN_MONSTER; };
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int	OnTakeDamage( const CTakeDamageInfo &info );
	void HandleAnimEvent( animevent_t *pEvent );
	void LayHeadcrab( void );
	void LaunchMortar( void );
	void DeathNotice( CBaseEntity *pevChild );

	int MeleeAttack1Conditions( float flDot, float flDist );	// Slash
	int MeleeAttack2Conditions( float flDot, float flDist );	// Lay a crab
	int RangeAttack1Conditions( float flDot, float flDist );	// Mortar launch


	BOOL CanLayCrab( void ) 
	{ 
		if ( m_crabTime < gpGlobals->curtime && m_crabCount < BIG_MAXCHILDREN )
		{
			// Don't spawn crabs inside each other
			Vector mins = GetAbsOrigin() - Vector( 32, 32, 0 );
			Vector maxs = GetAbsOrigin() + Vector( 32, 32, 0 );

			CBaseEntity *pList[2];
			int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_NPC );
			for ( int i = 0; i < count; i++ )
			{
				if ( pList[i] != this )	// Don't hurt yourself!
					return COND_NONE;
			}
			return COND_CAN_MELEE_ATTACK2;
		}

		return COND_NONE;
	}

	void Activate ( void );

	void NodeReach( void );
	void NodeStart( string_t iszNextNode );
	bool ShouldGoToNode( void );
	
	const char *GetNodeSequence( void )
	{
		CInfoBM *pTarget = (CInfoBM*)GetTarget();

		if ( pTarget && FClassnameIs( pTarget, "info_bigmomma" ) )
		{
			return STRING( pTarget->m_iszReachSequence );	// netname holds node sequence
		}
		
		return NULL;
	}


	const char *GetNodePresequence( void )
	{
		CInfoBM *pTarget = (CInfoBM *)GetTarget();

		if ( pTarget && FClassnameIs( pTarget, "info_bigmomma" ) )
		{
			return STRING( pTarget->m_iszPreSequence );
		}
		return NULL;
	}

	float GetNodeDelay( void )
	{
		CInfoBM *pTarget = (CInfoBM *)GetTarget();

		if ( pTarget && FClassnameIs( pTarget, "info_bigmomma" ) )
		{
			return pTarget->m_flDelay;	// Speed holds node delay
		}
		return 0;
	}

	float GetNodeRange( void )
	{
		CInfoBM *pTarget = (CInfoBM *)GetTarget();

		if ( pTarget && FClassnameIs( pTarget, "info_bigmomma" ) )
		{
			return pTarget->m_flRadius;	// Scale holds node delay
		}

		return 1e6;
	}

	float GetNodeYaw( void )
	{
		CBaseEntity *pTarget = GetTarget();

		if ( pTarget )
		{
			if ( pTarget->GetAbsAngles().y != 0 )
				 return pTarget->GetAbsAngles().y;
		}
		
		return GetAbsAngles().y;
	}
	
	// Restart the crab count on each new level
	void OnRestore( void )
	{
		BaseClass::OnRestore();
		m_crabCount = 0;
	}

	virtual void SetObjectCollisionBox( void )
	{
		SetAbsMins( GetAbsOrigin() + Vector( -95, -95, 0 ) );
		SetAbsMaxs( GetAbsOrigin() + Vector( 95, 95, 190 ) );
	}

	int SelectSchedule( void );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	float MaxYawSpeed( void );

	DECLARE_DATADESC();

	DEFINE_CUSTOM_AI;

	/*	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void		RunTask( Task_t *pTask );
	void		StartTask( Task_t *pTask );
	Schedule_t	*GetSchedule( void );
	Schedule_t	*GetScheduleOfType( int Type );
	void		TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	void SetYawSpeed( void );

	CUSTOM_SCHEDULES;
*/
private:
	float	m_nodeTime;
	float	m_crabTime;
	float	m_mortarTime;
	float	m_painSoundTime;
	int		m_crabCount;
	float	m_flDmgTime;

	bool	m_bDoneWithPath;

	string_t m_iszTarget;
	string_t m_iszNetName;
	float	m_flWait;

	Vector m_vTossDir;

	
};


BEGIN_DATADESC( CNPC_BigMomma )
	DEFINE_FIELD( CNPC_BigMomma, m_nodeTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BigMomma, m_crabTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BigMomma, m_mortarTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BigMomma, m_painSoundTime, FIELD_TIME ),
	DEFINE_KEYFIELD( CNPC_BigMomma, m_iszNetName, FIELD_STRING, "netname" ),
	DEFINE_FIELD( CNPC_BigMomma, m_flWait, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BigMomma, m_iszTarget, FIELD_STRING ),
END_DATADESC()


LINK_ENTITY_TO_CLASS ( monster_bigmomma, CNPC_BigMomma );

const char *CNPC_BigMomma::pChildDieSounds[] = 
{
	"gonarch/gon_childdie1.wav",
	"gonarch/gon_childdie2.wav",
	"gonarch/gon_childdie3.wav",
};

const char *CNPC_BigMomma::pSackSounds[] = 
{
	"gonarch/gon_sack1.wav",
	"gonarch/gon_sack2.wav",
	"gonarch/gon_sack3.wav",
};

const char *CNPC_BigMomma::pDeathSounds[] = 
{
	"gonarch/gon_die1.wav",
};

const char *CNPC_BigMomma::pAttackSounds[] = 
{
	"gonarch/gon_attack1.wav",
	"gonarch/gon_attack2.wav",
	"gonarch/gon_attack3.wav",
};
const char *CNPC_BigMomma::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CNPC_BigMomma::pBirthSounds[] = 
{
	"gonarch/gon_birth1.wav",
	"gonarch/gon_birth2.wav",
	"gonarch/gon_birth3.wav",
};

const char *CNPC_BigMomma::pAlertSounds[] = 
{
	"gonarch/gon_alert1.wav",
	"gonarch/gon_alert2.wav",
	"gonarch/gon_alert3.wav",
};

const char *CNPC_BigMomma::pPainSounds[] = 
{
	"gonarch/gon_pain2.wav",
	"gonarch/gon_pain4.wav",
	"gonarch/gon_pain5.wav",
};

const char *CNPC_BigMomma::pFootSounds[] = 
{
	"gonarch/gon_step1.wav",
	"gonarch/gon_step2.wav",
	"gonarch/gon_step3.wav",
};

//=========================================================
// Spawn
//=========================================================
void CNPC_BigMomma::Spawn()
{
	Precache( );

	SetModel( "models/big_mom.mdl" );
	UTIL_SetSize( this, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	SetNavType( NAV_GROUND );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	
	m_bloodColor = BLOOD_COLOR_GREEN;
	m_iHealth = 150 * sk_bigmomma_health_factor.GetFloat();

	SetHullType( HULL_WIDE_HUMAN );
	SetHullSizeNormal();

	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 );

//	pev->view_ofs		= Vector ( 0, 0, 128 );// position of the eyes relative to monster's origin.
	m_flFieldOfView	= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;

	SetRenderColor( 255, 255, 255, 255 );

	m_bDoneWithPath = false;

	m_nodeTime = 0.0f;

	m_iszTarget = m_iszNetName;

	NPCInit();

	BaseClass::Spawn();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_BigMomma::Precache()
{
	engine->PrecacheModel("models/big_mom.mdl");

	PRECACHE_SOUND_ARRAY( pChildDieSounds );
	PRECACHE_SOUND_ARRAY( pSackSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pBirthSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pFootSounds );

	UTIL_PrecacheOther( BIG_CHILDCLASS );

	// TEMP: Squid
	engine->PrecacheModel("sprites/mommaspit.vmt");// spit projectile.
	gSpitSprite = engine->PrecacheModel("sprites/mommaspout.vmt");// client side spittle.
	gSpitDebrisSprite = engine->PrecacheModel("sprites/mommablob.vmt" );

	enginesound->PrecacheSound( "bullchicken/bc_acid1.wav" );
	enginesound->PrecacheSound( "bullchicken/bc_spithit1.wav" );
	enginesound->PrecacheSound( "bullchicken/bc_spithit2.wav" );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_BigMomma::MaxYawSpeed ( void )
{
	float ys = 90.0f;

	switch ( GetActivity() )
	{
	case ACT_IDLE:
		ys = 100.0f;
		break;
	default:
		ys = 90.0f;
	}
	
	return ys;
}

void CNPC_BigMomma::Activate( void )
{
	if ( GetTarget() == NULL )
		 Remember( bits_MEMORY_ADVANCE_NODE );	// Start 'er up

	BaseClass::Activate();
}

void CNPC_BigMomma::NodeStart( string_t iszNextNode )
{
	m_iszTarget = iszNextNode;

	const char *pTargetName = STRING( m_iszTarget );

	CBaseEntity *pTarget = NULL;

	if ( pTargetName )
		 pTarget = gEntList.FindEntityByName( NULL, pTargetName, NULL );

	if ( pTarget == NULL )
	{
		//Msg( "BM: Finished the path!!\n" );
		m_bDoneWithPath = true;
		return;
	}

	SetTarget( pTarget );
}


void CNPC_BigMomma::NodeReach( void )
{
	CInfoBM *pTarget = (CInfoBM*)GetTarget();

	Forget( bits_MEMORY_ADVANCE_NODE );

	if ( !pTarget )
		return;

	if ( pTarget->m_iHealth >= 1 )
		 m_iMaxHealth = m_iHealth = pTarget->m_iHealth * sk_bigmomma_health_factor.GetFloat();

	if ( !HasMemory( bits_MEMORY_FIRED_NODE ) )
	{
		if ( pTarget )
		{
			 pTarget->m_OnAnimationEvent.FireOutput( this, this );
		}
	}

	Forget( bits_MEMORY_FIRED_NODE );

	m_iszTarget = pTarget->m_target;
	
	if ( pTarget->m_iHealth == 0 )
		 Remember( bits_MEMORY_ADVANCE_NODE );	// Move on if no health at this node
	else
	{
		 GetNavigator()->ClearGoal();
	}
}


void CNPC_BigMomma::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo dmgInfo = info;
	float flDamage = dmgInfo.GetDamage();

	if ( ptr->hitbox <= 9  )
	{
		// didn't hit the sack?
		if ( m_flDmgTime != gpGlobals->curtime || (RandomInt( 0, 10 ) < 1) )
		{
			CEffectData data;
			
			data.m_vOrigin = ptr->endpos;
			data.m_vAngles = GetAbsAngles();
			
			data.m_vNormal = ( vecDir  ) * -1;

			DispatchEffect( "ManhackSparks", data );


			m_flDmgTime = gpGlobals->curtime;
		}

		flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	else 
	{
		SpawnBlood( ptr->endpos + ptr->plane.normal * 15, m_bloodColor, 100 );

		if ( gpGlobals->curtime > m_painSoundTime )
		{
			m_painSoundTime = gpGlobals->curtime + RandomInt(1, 3);
						
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1.0f, ATTN_NORM, 0, 100 );
			
			enginesound->EmitSound( filter, entindex(), CHAN_VOICE, pPainSounds[ random->RandomInt(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}


int CNPC_BigMomma::OnTakeDamage( const CTakeDamageInfo &info )
{
	float flDamage = info.GetDamage();

	// Don't take any acid damage -- BigMomma's mortar is acid
	if ( info.GetDamageType() & DMG_ACID )
	{
		CTakeDamageInfo subInfo = info;
		subInfo.SetDamage( 0 );
		return BaseClass::OnTakeDamage( subInfo ); 
	}
	
	//Hacky McHack
	if ( GetTask()->iTask == TASK_PLAY_NODE_PRESEQUENCE || GetTask()->iTask == TASK_PLAY_NODE_SEQUENCE )
		 return 0;

	if ( m_bDoneWithPath == false )
	{
		if ( m_iHealth <= flDamage )
		{
			m_iHealth = flDamage + 1;
			Remember( bits_MEMORY_ADVANCE_NODE );
			//Msg( "BM: Finished node health!!!\n" );
		}
	}

	return BaseClass::OnTakeDamage( info );
}

bool CNPC_BigMomma::ShouldGoToNode( void )
{
	if ( HasMemory( bits_MEMORY_ADVANCE_NODE ) )
	{
		if ( m_nodeTime < gpGlobals->curtime )
			 return true;
	}
	return false;
}


int CNPC_BigMomma::SelectSchedule( void )
{
	if ( ShouldGoToNode() )
	{
		return SCHED_BIG_NODE;
	}

	return BaseClass::SelectSchedule();
}


void CNPC_BigMomma::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHECK_NODE_PROXIMITY:
		{

		}

		break;
	case TASK_FIND_NODE:
		{
			CBaseEntity *pTarget = GetTarget();

			if ( !HasMemory( bits_MEMORY_ADVANCE_NODE ) )
			{
				if ( pTarget )
					 m_iszTarget = pTarget->m_target;
			}

			NodeStart( m_iszTarget );
			TaskComplete();
			//Msg( "BM: Found node %s\n", STRING( m_iszTarget ) );
		}
		break;

	case TASK_NODE_DELAY:
		m_nodeTime = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		//Msg( "BM: FAIL! Delay %.2f\n", pTask->flTaskData );
		break;

	case TASK_PROCESS_NODE:
		//Msg( "BM: Reached node %s\n", STRING( m_iszTarget ) );
		NodeReach();
		TaskComplete();
		break;

	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
		{
			const char *pSequence = NULL;
			int	iSequence;

			if ( pTask->iTask == TASK_PLAY_NODE_SEQUENCE )
				pSequence = GetNodeSequence();
			else
				pSequence = GetNodePresequence();

			//Msg( "BM: Playing node sequence %s\n", pSequence );

			if ( pSequence ) //ugh
			{
				iSequence = LookupSequence( pSequence );

				if ( iSequence != -1 )
				{
					SetIdealActivity( ACT_DO_NOT_DISTURB );
					SetSequence( iSequence );
					m_flCycle = 0.0f;

					ResetSequenceInfo();
					//Msg( "BM: Sequence %s %f\n", GetNodeSequence(), gpGlobals->curtime );
					return;
				}
			}
			TaskComplete();
		}
		break;

	case TASK_NODE_YAW:
		GetMotor()->SetIdealYaw( GetNodeYaw() );
		TaskComplete();
		break;

	case TASK_WAIT_NODE:
		m_flWait = gpGlobals->curtime + GetNodeDelay();

		/*if ( GetTarget() && GetTarget()->GetSpawnFlags() & SF_INFOBM_WAIT )
			Msg( "BM: Wait at node %s forever\n", STRING( m_iszTarget) );
		else
			Msg( "BM: Wait at node %s for %.2f\n", STRING( m_iszTarget ), GetNodeDelay() );*/
		break;


	case TASK_MOVE_TO_NODE_RANGE:
		{
			CBaseEntity *pTarget = GetTarget();

			if ( !pTarget )
				TaskFail( FAIL_NO_TARGET );
			else
			{
				if ( ( pTarget->GetAbsOrigin() - GetAbsOrigin() ).Length() < GetNodeRange() )
					TaskComplete();
				else
				{
					Activity act = ACT_WALK;
					if ( pTarget->GetSpawnFlags() & SF_INFOBM_RUN )
						 act = ACT_RUN;

					AI_NavGoal_t goal( GOALTYPE_TARGETENT, vec3_origin, act );

					if ( !GetNavigator()->SetGoal( goal ) )
					{
						TaskFail( NO_TASK_FAILURE );
					}
				}
			}
		}
		//Msg( "BM: Moving to node %s\n", STRING( m_iszTarget ) );

		break;

	case TASK_MELEE_ATTACK1:
		{

		// Play an attack sound here

		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0f, ATTN_NORM, 0, PITCH_NORM );

		BaseClass::StartTask( pTask );
		}
		
		break;

	default: 
		BaseClass::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_BigMomma::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHECK_NODE_PROXIMITY:
		{
			float distance;

			if ( GetTarget() == NULL )
				TaskFail( FAIL_NO_TARGET );
			else
			{
				if ( GetNavigator()->IsGoalActive() )
				{
					distance = ( GetTarget()->GetAbsOrigin() - GetAbsOrigin() ).Length2D();
					// Set the appropriate activity based on an overlapping range
					// overlap the range to prevent oscillation
					if ( distance < GetNodeRange() )
					{
						//Msg( "BM: Reached node PROXIMITY!!\n" );
						TaskComplete();
						GetNavigator()->ClearGoal();		// Stop moving
					}
				}
				else
					TaskComplete();
			}
		}

		break;

	case TASK_WAIT_NODE:
		if ( GetTarget() != NULL && (GetTarget()->GetSpawnFlags() & SF_INFOBM_WAIT) )
			return;

		if ( gpGlobals->curtime > m_flWaitFinished )
			TaskComplete();
		//Msg( "BM: The WAIT is over!\n" );
		break;

	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:

		if ( IsSequenceFinished() )
		{
			CBaseEntity *pTarget = NULL;

			if (  GetTarget() )
				 pTarget = gEntList.FindEntityByName( NULL, STRING( GetTarget()->m_target ), NULL );

			if ( pTarget )
			{
				 SetActivity( ACT_IDLE );
				 TaskComplete();
			}
		}
		
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_BigMomma::HandleAnimEvent( animevent_t *pEvent )
{
	CPASAttenuationFilter filter( this );

	Vector vecFwd, vecRight, vecUp;
	QAngle angles;
	angles = GetAbsAngles();
	AngleVectors( angles, &vecFwd, &vecRight, &vecUp );

	switch( pEvent->event )
	{
		case BIG_AE_MELEE_ATTACKBR:
		case BIG_AE_MELEE_ATTACKBL:
		case BIG_AE_MELEE_ATTACK1:
		{
			Vector center = GetAbsOrigin() + vecFwd * 128;
			Vector mins = center - Vector( 64, 64, 0 );
			Vector maxs = center + Vector( 64, 64, 64 );

			CBaseEntity *pList[8];
			int count = UTIL_EntitiesInBox( pList, 8, mins, maxs, FL_NPC | FL_CLIENT );
			CBaseEntity *pHurt = NULL;

			for ( int i = 0; i < count && !pHurt; i++ )
			{
				if ( pList[i] != this )
				{
					if ( pList[i]->GetOwnerEntity() != this )
					{
						pHurt = pList[i];
					}
				}
			}
					
			if ( pHurt )
			{
				CTakeDamageInfo info( this, this, 15, DMG_CLUB | DMG_SLASH );
				CalculateMeleeDamageForce( &info, (pHurt->GetAbsOrigin() - GetAbsOrigin()), pHurt->GetAbsOrigin() );
				pHurt->TakeDamage( info );
				QAngle newAngles = angles;
				newAngles.x = 15;
				if ( pHurt->IsPlayer() )
				{
					((CBasePlayer *)pHurt)->SetPunchAngle( newAngles );
				}
				switch( pEvent->event )
				{
					case BIG_AE_MELEE_ATTACKBR:
//						pHurt->pev->velocity = pHurt->pev->velocity + (vecFwd * 150) + Vector(0,0,250) - (vecRight * 200);
						pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + (vecFwd * 150) + Vector(0,0,250) - (vecRight * 200) );
					break;

					case BIG_AE_MELEE_ATTACKBL:
						pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + (vecFwd * 150) + Vector(0,0,250) + (vecRight * 200) );
					break;

					case BIG_AE_MELEE_ATTACK1:
						pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + (vecFwd * 220) + Vector(0,0,200) );
					break;
				}

				pHurt->RemoveFlag( FL_ONGROUND );
				enginesound->EmitSound( filter, entindex(), CHAN_BODY, pAttackHitSounds[ random->RandomInt(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RandomInt(-5,5) );
			}
		}
		break;
		
		case BIG_AE_SCREAM:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, pAlertSounds[ random->RandomInt(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;
		
		case BIG_AE_PAIN_SOUND:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, pPainSounds[ random->RandomInt(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;
		
		case BIG_AE_ATTACK_SOUND:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, pAttackSounds[ random->RandomInt(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;

		case BIG_AE_BIRTH_SOUND:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, pBirthSounds[ random->RandomInt(0,ARRAYSIZE(pBirthSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;

		case BIG_AE_SACK:
			if ( RandomInt(0,100) < 30 )
			{
				enginesound->EmitSound( filter, entindex(), CHAN_BODY, pSackSounds[ random->RandomInt(0,ARRAYSIZE(pSackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			}
			break;

		case BIG_AE_DEATHSOUND:
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, pDeathSounds[ random->RandomInt(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;

		case BIG_AE_STEP1:		// Footstep left
		case BIG_AE_STEP3:		// Footstep back left
			enginesound->EmitSound( filter, entindex(), CHAN_ITEM, pFootSounds[ random->RandomInt(0,ARRAYSIZE(pFootSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;

		case BIG_AE_STEP4:		// Footstep back right
		case BIG_AE_STEP2:		// Footstep right
			enginesound->EmitSound( filter, entindex(), CHAN_BODY, pFootSounds[ random->RandomInt(0,ARRAYSIZE(pFootSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
			break;

		case BIG_AE_MORTAR_ATTACK1:
			LaunchMortar();
			break;

		case BIG_AE_LAY_CRAB:
			LayHeadcrab();
			break;

		case BIG_AE_JUMP_FORWARD:
			RemoveFlag( FL_ONGROUND );
			SetAbsOrigin(GetAbsOrigin() + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			SetAbsVelocity(vecFwd * 200 + vecUp * 500 );
			break;

		case BIG_AE_EARLY_TARGET:
			{
				CInfoBM *pTarget = (CInfoBM*) GetTarget();

				if ( pTarget )
				{
					pTarget->m_OnAnimationEvent.FireOutput( this, this );
				}
				
				Remember( bits_MEMORY_FIRED_NODE );
			}
			break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}


void CNPC_BigMomma::LayHeadcrab( void )
{
	CBaseEntity *pChild = CBaseEntity::Create( BIG_CHILDCLASS, GetAbsOrigin(), GetAbsAngles(), this );

	pChild->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	pChild->SetOwnerEntity( this );

	// Is this the second crab in a pair?
	if ( HasMemory( bits_MEMORY_CHILDPAIR ) )
	{
		m_crabTime = gpGlobals->curtime + RandomFloat( 5, 10 );
		Forget( bits_MEMORY_CHILDPAIR );
	}
	else
	{
		m_crabTime = gpGlobals->curtime + RandomFloat( 0.5, 2.5 );
		Remember( bits_MEMORY_CHILDPAIR );
	}

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,100), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	UTIL_DecalTrace( &tr, "Splash" );

	CPASAttenuationFilter filter( this );
	enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, pBirthSounds[ random->RandomInt(0,ARRAYSIZE(pBirthSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + random->RandomInt(-5,5) );
	m_crabCount++;
}

void CNPC_BigMomma::DeathNotice( CBaseEntity *pevChild )
{
	if ( m_crabCount > 0 )		// Some babies may cross a transition, but we reset the count then
	{
		m_crabCount--;
	}
	if ( IsAlive() )
	{
		// Make the "my baby's dead" noise!
		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pChildDieSounds ), 1.0f, ATTN_NORM, 0, PITCH_NORM );

		enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, pBirthSounds[ random->RandomInt(0,ARRAYSIZE(pBirthSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
	}
}


void CNPC_BigMomma::LaunchMortar( void )
{
	m_mortarTime = gpGlobals->curtime + RandomFloat( 2, 15 );
	
	Vector startPos = GetAbsOrigin();
	startPos.z += 180;

	CPASAttenuationFilter filter( this );
	enginesound->EmitSound( filter, entindex(), CHAN_WEAPON, pSackSounds[ random->RandomInt(0,ARRAYSIZE(pSackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + random->RandomInt(-5,5) );
	CBMortar *pBomb = CBMortar::Shoot( this, startPos, m_vTossDir );
	pBomb->SetGravity( 1.0 );
	MortarSpray( startPos, Vector(0,0,10), gSpitSprite, 24 );
}

int CNPC_BigMomma::MeleeAttack1Conditions( float flDot, float flDist )
{
	if (flDot >= 0.7)
	{
		if ( flDist > BIG_ATTACKDIST )
			 return COND_TOO_FAR_TO_ATTACK;
		else
			 return COND_CAN_MELEE_ATTACK1;
	}
	else
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_NONE;
}


// Lay a crab
int CNPC_BigMomma::MeleeAttack2Conditions( float flDot, float flDist )
{
	return CanLayCrab();
}


Vector VecCheckSplatToss( CBaseEntity *pEnt, const Vector &vecSpot1, Vector vecSpot2, float maxHeight )
{
	trace_t			tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecGrenadeVel;
	Vector			vecTemp;
	float			flGravity = sv_gravity.GetFloat();

	// calculate the midpoint and apex of the 'triangle'
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0,0,maxHeight), MASK_SOLID_BRUSHONLY, pEnt, COLLISION_GROUP_NONE, &tr );
	vecApex = tr.endpos;
	
	UTIL_TraceLine(vecSpot1, vecApex, MASK_SOLID, pEnt, COLLISION_GROUP_NONE, &tr );
	if (tr.fraction != 1.0)
	{
		// fail!
		return vec3_origin;
	}

	// Don't worry about actually hitting the target, this won't hurt us!

	// How high should the grenade travel (subtract 15 so the grenade doesn't hit the ceiling)?
	float height = (vecApex.z - vecSpot1.z) - 15;

	//HACK HACK
	if ( height < 0 )
		 height *= -1;

	// How fast does the grenade need to travel to reach that height given gravity?
	float speed = sqrt( 2 * flGravity * height );

	// How much time does it take to get there?
	float time = speed / flGravity;
	vecGrenadeVel = (vecSpot2 - vecSpot1);
	vecGrenadeVel.z = 0;
	
	// Travel half the distance to the target in that time (apex is at the midpoint)
	vecGrenadeVel = vecGrenadeVel * ( 0.5 / time );
	// Speed to offset gravity at the desired height
	vecGrenadeVel.z = speed;

	return vecGrenadeVel;
}

// Mortar launch
int CNPC_BigMomma::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > BIG_MORTARDIST )
		 return COND_TOO_FAR_TO_ATTACK;

	if ( flDist <= BIG_MORTARDIST && m_mortarTime < gpGlobals->curtime )
	{
		CBaseEntity *pEnemy = GetEnemy();

		if ( pEnemy )
		{
			Vector startPos = GetAbsOrigin();
			startPos.z += 180;

			m_vTossDir = VecCheckSplatToss( this, startPos, pEnemy->BodyTarget( GetAbsOrigin() ), random->RandomFloat( 150, 500 ) );

			if ( m_vTossDir != vec3_origin )
				return COND_CAN_RANGE_ATTACK1;
		}
	}
	
	return COND_NONE;
}

// ---------------------------------
//
// Mortar
//
// ---------------------------------
void MortarSpray( const Vector &position, const Vector &direction, int spriteModel, int count )
{
	CPVSFilter filter( position );
	
	te->SpriteSpray( filter, 0.0, &position, &direction, spriteModel, 200, 80, count );
}


// UNDONE: right now this is pretty much a copy of the squid spit with minor changes to the way it does damage
void CBMortar:: Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetClassname( "bmortar" );
	
	SetSolid( SOLID_BBOX );

	pSprite = CSprite::SpriteCreate( "sprites/mommaspit.vmt", GetAbsOrigin(), true ); 

	if ( pSprite )
	{
		pSprite->SetAttachment( this, 0 );
		pSprite->m_flSpriteFramerate = 5;

		pSprite->m_nRenderMode = kRenderTransAlpha;
		pSprite->SetBrightness( 255 );

		m_iFrame = 0;

		pSprite->SetScale( 2.5f );
	}

	UTIL_SetSize( this, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float)modelinfo->GetModelFrameCount( GetModel() ) - 1;
	m_flDmgTime = gpGlobals->curtime + 0.4;
}

void CBMortar::Animate( void )
{
	SetNextThink( gpGlobals->curtime + 0.1 );

	Vector vVelocity = GetAbsVelocity();

	VectorNormalize( vVelocity );

	if ( gpGlobals->curtime > m_flDmgTime )
	{
		m_flDmgTime = gpGlobals->curtime + 0.2;
		MortarSpray( GetAbsOrigin() + Vector( 0, 0, 15 ), -vVelocity, gSpitSprite, 3 );
	}
	if ( m_iFrame++ )
	{
		if ( m_iFrame > m_maxFrame )
		{
			m_iFrame = 0;
		}
	}
}

CBMortar *CBMortar::Shoot( CBaseEntity *pOwner, Vector vecStart, Vector vecVelocity )
{
	CBMortar *pSpit = CREATE_ENTITY( CBMortar, "bmortar" ); 
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit, vecStart );
	pSpit->SetAbsVelocity( vecVelocity );
	pSpit->SetOwnerEntity( pOwner );
	pSpit->SetThink ( &CBMortar::Animate );
	pSpit->SetNextThink( gpGlobals->curtime + 0.1 );

	return pSpit;
}


void CBMortar::Touch( CBaseEntity *pOther )
{
	trace_t tr;
	int		iPitch;

	// splat sound
	iPitch = random->RandomFloat( 90, 110 );

	EmitSound( "NPC_BigMomma.SpitTouch1" );

	switch ( random->RandomInt( 0, 1 ) )
	{
	case 0:
		EmitSound( "NPC_BigMomma.SpitHit1" );
		break;
	case 1:
		EmitSound( "NPC_BigMomma.SpitHit2" );
		break;
	}

	if ( pOther->IsBSPModel() )
	{
		// make a splat on the wall
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 10, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		UTIL_DecalTrace( &tr, "Splash" );
	}
	else
	{
		tr.endpos = GetAbsOrigin();

		Vector vVelocity = GetAbsVelocity();
		VectorNormalize( vVelocity );

		tr.plane.normal = -1 * vVelocity;
	}
	// make some flecks
	MortarSpray( tr.endpos + Vector( 0, 0, 15 ), tr.plane.normal, gSpitSprite, 24 );

	CBaseEntity *pOwner = GetOwnerEntity();

	RadiusDamage( CTakeDamageInfo( this, pOwner, sk_bigmomma_dmg_blast.GetFloat(), DMG_ACID ), GetAbsOrigin(), sk_bigmomma_radius_blast.GetFloat(), CLASS_NONE );
		
	UTIL_Remove( pSprite );
	UTIL_Remove( this );
}


AI_BEGIN_CUSTOM_NPC( monster_bigmomma, CNPC_BigMomma )

	DECLARE_TASK( TASK_MOVE_TO_NODE_RANGE )
	DECLARE_TASK( TASK_FIND_NODE )
	DECLARE_TASK( TASK_PLAY_NODE_PRESEQUENCE )
	DECLARE_TASK( TASK_PLAY_NODE_SEQUENCE )
	DECLARE_TASK( TASK_PROCESS_NODE )
	DECLARE_TASK( TASK_WAIT_NODE )
	DECLARE_TASK( TASK_NODE_DELAY )
	DECLARE_TASK( TASK_NODE_YAW )
	DECLARE_TASK( TASK_CHECK_NODE_PROXIMITY )

	
	//=========================================================
	// > SCHED_BIG_NODE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NODE_FAIL,

		"	Tasks"
		"		TASK_NODE_DELAY						3"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_BIG_NODE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BIG_NODE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_NODE_FAIL"
		"		TASK_STOP_MOVING					0"
		"		TASK_FIND_NODE						0"
		"		TASK_PLAY_NODE_PRESEQUENCE			0"
		"		TASK_MOVE_TO_NODE_RANGE				0"
		"		TASK_CHECK_NODE_PROXIMITY			0"
		"		TASK_STOP_MOVING					0"
		"		TASK_NODE_YAW						0"
		"		TASK_FACE_IDEAL						0"
		"		TASK_WAIT_NODE						0"
		"		TASK_PLAY_NODE_SEQUENCE				0"
		"		TASK_PROCESS_NODE					0"
			
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()

	