//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Giant walking strider thing!
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_BaseNPC.h"
#include "ai_trackpather.h"
#include "ai_pathfinder.h"
#include "ai_waypoint.h"
#include "trains.h"
#include "npcevent.h"
#include "te_particlesystem.h"
#include "shake.h"
#include "soundent.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "bone_setup.h"
#include "vcollide_parse.h"
#include "studio.h"
#include "physics_bone_follower.h"
#include "ai_navigator.h"
#include "ai_route.h"
#include "ammodef.h"
#include "npc_bullseye.h"
#include "rope.h"
#include "ai_memory.h"
#include "weapon_physcannon.h"
#include "collisionutils.h"
#include "in_buttons.h"
#include "steamjet.h"
#include "physics_prop_ragdoll.h"
#include "vehicle_base.h"

ConVar sk_strider_health( "sk_strider_health", "0" );
ConVar npc_strider_height_adj("npc_strider_height_adj", "0" );

ConVar npc_strider_shake_ropes_radius( "npc_strider_shake_ropes_radius", "1200" );
ConVar npc_strider_shake_ropes_magnitude( "npc_strider_shake_ropes_magnitude", "150" );

static const Vector g_zAxis(0,0,1);

//Animation events
#define STRIDER_AE_FOOTSTEP_LEFT		1
#define STRIDER_AE_FOOTSTEP_RIGHT		2
#define STRIDER_AE_FOOTSTEP_BACK		3
#define STRIDER_AE_FOOTSTEP_LEFTM		4
#define STRIDER_AE_FOOTSTEP_RIGHTM		5
#define STRIDER_AE_FOOTSTEP_BACKM		6
#define STRIDER_AE_FOOTSTEP_LEFTL		7
#define STRIDER_AE_FOOTSTEP_RIGHTL		8
#define STRIDER_AE_FOOTSTEP_BACKL		9
#define STRIDER_AE_WHOOSH_LEFT			11
#define STRIDER_AE_WHOOSH_RIGHT			12
#define STRIDER_AE_WHOOSH_BACK			13
#define STRIDER_AE_CREAK_LEFT			21
#define STRIDER_AE_CREAK_RIGHT			22
#define STRIDER_AE_CREAK_BACK			23
#define STRIDER_AE_SHOOTCANNON			100
#define STRIDER_AE_CANNONHIT			101
#define STRIDER_AE_SHOOTMINIGUN			105
#define STRIDER_AE_STOMPHITL			110
#define STRIDER_AE_STOMPHITR			111
#define STRIDER_AE_FLICKL				112
#define STRIDER_AE_FLICKR				113
#define STRIDER_AE_DIE					999

// UNDONE: Share properly with the client code!!!
#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3
#define STOMP_IK_SLOT					11

// can hit anything within this range
#define STRIDER_STOMP_RANGE				260

static void MoveToGround( Vector *position, CBaseEntity *ignore, const Vector &mins, const Vector &maxs );

//==================================================
// Custom Schedules
//==================================================
enum StriderSchedules
{
	SCHED_STRIDER_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_STRIDER_RANGE_ATTACK2, // Immolator
	SCHED_STRIDER_DODGE,
	SCHED_STRIDER_STOMPL,
	SCHED_STRIDER_STOMPR,
	SCHED_STRIDER_FLICKL,
	SCHED_STRIDER_FLICKR,
	SCHED_STRIDER_DIE,
};

//==================================================
// Custom Tasks
//==================================================

enum StriderTasks
{
	TASK_STRIDER_AIM = LAST_SHARED_TASK,
	TASK_STRIDER_DODGE,
	TASK_STRIDER_STOMP,
	TASK_STRIDER_BREAKDOWN,
};

enum Strider_conditions
{
	COND_STRIDER_DO_FLICK = LAST_SHARED_CONDITION,
	COND_TRACK_PATH_GO
};

//==================================================
// Custom Activities
//==================================================

int	ACT_STRIDER_LOOKL;
int	ACT_STRIDER_LOOKR;
int ACT_STRIDER_DEPLOYRA1;
int ACT_STRIDER_AIMRA1;
int ACT_STRIDER_FINISHRA1;
int ACT_STRIDER_DODGER;
int ACT_STRIDER_DODGEL;
int ACT_STRIDER_STOMPL;
int ACT_STRIDER_STOMPR;
int ACT_STRIDER_FLICKL;
int ACT_STRIDER_FLICKR;

// UNDONE: Split sleep into 3 activities (start, loop, end)
int ACT_STRIDER_SLEEP;

// These bones have physics shadows
// It allows a one-way interaction between the strider and
// the physics world
static const char *pFollowerBoneNames[] =
{
	// lower legs
	"Combine_Strider.Leg_Left_Bone1",
	"Combine_Strider.Leg_Right_Bone1",
	"Combine_Strider.Leg_Hind_Bone1",
	// upper legs
	"Combine_Strider.Leg_Left_Bone",
	"Combine_Strider.Leg_Right_Bone",
	"Combine_Strider.Leg_Hind_Bone",
};

static int g_YawControl;
static int g_PitchControl;
static int g_CannonAttachment;

class CNPC_Strider;

#define MINIGUN_MINYAW		-90
#define MINIGUN_MAXYAW		90
#define MINIGUN_MINPITCH	-45
#define MINIGUN_MAXPITCH	45

struct viewcone_t
{
	Vector origin;
	Vector axis;
	float cosAngle;
	float length;
};

class IMinigunHost
{
public:
	virtual void ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread = vec3_origin ) = 0;
	virtual void UpdateMinigunControls( float &yaw, float &pitch ) = 0;
	virtual void GetViewCone( viewcone_t &cone ) = 0;
	virtual CAI_BaseNPC *GetEntity() = 0;
};

struct animcontroller_t
{
	float	current;
	float	target;
	float	rate;

	void Update( float dt )
	{
		current = Approach( target, current, rate * dt );
	}

	void Random( float minTarget, float maxTarget, float minRate, float maxRate )
	{
		target = random->RandomFloat( minTarget, maxTarget );
		rate = random->RandomFloat( minRate, maxRate );
	}
};

class CMinigun
{
public:
	DECLARE_DATADESC();

	void Init();
	void Think( IMinigunHost *pHost, float dt );
	void ChangeState( IMinigunHost *pHost );
	CBaseEntity *FindTarget( IMinigunHost *pHost );
	void AimAtTarget( CAI_BaseNPC *pOwner, CBaseEntity *pTarget );
	void ShootAtTarget( IMinigunHost *pHost, CBaseEntity *pTarget, float shootTime );
	void SetShootDuration( float duration );
	void StopShootingForSeconds( IMinigunHost *pHost, float duration );
	void Enable( IMinigunHost *pHost, bool enable )
	{
		m_enable = enable;
		if ( !m_enable )
		{
			m_yaw.current = m_yaw.target = 0;
			m_pitch.current = m_pitch.target = 0;
			if ( pHost )
			{
				pHost->UpdateMinigunControls( m_yaw.current, m_pitch.current );
			}
		}
	}
	float	GetAimError();
	enum minigunstates_t
	{
		MINIGUN_OFF = 0,
		MINIGUN_SHOOTING = 1,
	};

private:
	bool		m_enable;
	int			m_minigunState;
	float		m_nextStateTime;
	float		m_nextShootTime;
	float		m_shootDuration;
	float		m_nextTwitchTime;
	int			m_randomState;
	EHANDLE		m_hTarget;
	animcontroller_t m_yaw;
	animcontroller_t m_pitch;
};


BEGIN_DATADESC_NO_BASE( CMinigun )

	DEFINE_FIELD( CMinigun, m_enable,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CMinigun, m_minigunState,		FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_nextStateTime,	FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_nextShootTime,	FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_shootDuration,	FIELD_FLOAT ),
	DEFINE_FIELD( CMinigun, m_nextTwitchTime,	FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_randomState,		FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_hTarget,			FIELD_EHANDLE ),

	// Silence, Classcheck!
	// DEFINE_FIELD( CMinigun, m_yaw, animcontroller_t ),
	// DEFINE_FIELD( CMinigun, m_pitch, animcontroller_t ),
	
	DEFINE_FIELD( CMinigun, m_yaw.current,		FIELD_FLOAT ),
	DEFINE_FIELD( CMinigun, m_yaw.target,		FIELD_FLOAT ),
	DEFINE_FIELD( CMinigun, m_yaw.rate,			FIELD_FLOAT ),
	DEFINE_FIELD( CMinigun, m_pitch.current,	FIELD_FLOAT ),
	DEFINE_FIELD( CMinigun, m_pitch.target,		FIELD_FLOAT ),
	DEFINE_FIELD( CMinigun, m_pitch.rate,		FIELD_FLOAT ),

END_DATADESC()


void CMinigun::Init()
{
	m_enable = true;
	m_minigunState = MINIGUN_OFF;
	m_nextStateTime = gpGlobals->curtime + 2;
	m_nextShootTime = m_nextStateTime;
	m_nextTwitchTime = m_nextStateTime;
	m_randomState = 0;
	m_yaw.current = m_yaw.target = 0;
	m_pitch.current = m_pitch.target = 0;
}


CBaseEntity *CMinigun::FindTarget( IMinigunHost *pHost )
{
	static int s_offset = 0; // used to shuffle the order a bit

	viewcone_t cone;

	pHost->GetViewCone( cone );

#if 0
	Vector conePoint = cone.origin + cone.axis * cone.length;
	Vector right, up;

	pHost->GetEntity()->GetVectors( NULL, &right, &up );
	NDebugOverlay::Line( cone.origin, conePoint, 255, 0, 0, false, 10 );
	NDebugOverlay::Line( cone.origin, conePoint + up * (cone.length / cone.cosAngle), 255, 0, 0, false, 10 );
	NDebugOverlay::Line( cone.origin, conePoint - up * (cone.length / cone.cosAngle), 255, 0, 0, false, 10 );
	NDebugOverlay::Line( cone.origin, conePoint + right * (cone.length / cone.cosAngle), 255, 0, 0, false, 10 );
	NDebugOverlay::Line( cone.origin, conePoint - right * (cone.length / cone.cosAngle), 255, 0, 0, false, 10 );
#endif
	CBaseEntity *pList[64];
	s_offset++;
	int count = FindBullseyesInCone( pList, 64, cone.origin, cone.axis, cone.cosAngle, cone.length );
	if ( count )
	{
		CBaseCombatCharacter *pHostCombat = ToBaseCombatCharacter( pHost->GetEntity() );
		for ( int i = 0; i < count; i++ )
		{
			int index = (i + s_offset) % count;
			if ( pHostCombat->IRelationType( pList[index] ) != D_HT )
				continue;

			trace_t tr;
			CTraceFilterWorldOnly filter;
			UTIL_TraceLine( cone.origin, pList[index]->GetAbsOrigin(), MASK_OPAQUE, &filter, &tr );
			if ( tr.fraction < 1 || tr.startsolid )
				continue;

			return pList[index];
		}
	}
	else if( pHost->GetEntity()->GetEnemy() )
	{
		if( pHost->GetEntity()->HasCondition( COND_SEE_ENEMY ) )
		{
			return pHost->GetEntity()->GetEnemy();
		}
	}

	return NULL;
}

float CMinigun::GetAimError()
{
	return fabs(m_yaw.target-m_yaw.current) + fabs(m_pitch.target-m_pitch.current);
}


void CMinigun::AimAtTarget( CAI_BaseNPC *pOwner, CBaseEntity *pTarget )
{
	matrix3x4_t gunMatrix;
	int mingunAttachment = pOwner->LookupAttachment( "minigun" );
	pOwner->GetAttachment( mingunAttachment, gunMatrix );

	// transform the enemy into gun space
	Vector localEnemyPosition;
	VectorITransform( pTarget->GetAbsOrigin() - pOwner->GetAbsVelocity() * 0.1, gunMatrix, localEnemyPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	m_pitch.target = m_pitch.current + 0.5 * UTIL_AngleDiff( localEnemyAngles.x, 0 );
	m_yaw.target = m_yaw.current - 0.5 * UTIL_AngleDiff( localEnemyAngles.y, 0 );

#if 0
	// infinite 
	m_pitch.current = m_pitch.target;
	m_yaw.current = m_yaw.target;
	Vector out, forward, right, up;
	MatrixGetColumn( gunMatrix, 3, out );
	MatrixGetColumn( gunMatrix, 0, forward );
	MatrixGetColumn( gunMatrix, 1, right );
	MatrixGetColumn( gunMatrix, 2, up );
	NDebugOverlay::Line( out, out + forward * 200, 255, 255, 0, false, 0.1 );
	NDebugOverlay::Line( out, out + right * 200, 0, 255, 0, false, 0.1 );
	NDebugOverlay::Line( out, out + up * 200, 0, 0, 255, false, 0.1 );
	NDebugOverlay::Box( pTarget->GetAbsOrigin(), Vector(-8,-8,-8), Vector(8,8,8), 255, 255, 0, 0, 0.1 );
#endif
}

void CMinigun::ShootAtTarget( IMinigunHost *pHost, CBaseEntity *pTarget, float shootTime )
{
	if ( !pTarget )
		return;

	variant_t emptyVariant;
	pTarget->AcceptInput( "InputTargeted", pHost->GetEntity(), pHost->GetEntity(), emptyVariant, 0 );
	Enable( NULL, true );
	if ( shootTime <= 0 )
	{
		shootTime = random->RandomFloat( 4, 8 );
	}
	m_hTarget = pTarget;
	m_yaw.rate = 7200;
	m_pitch.rate = 7200;
	m_minigunState = MINIGUN_SHOOTING;
	m_shootDuration = shootTime;
	m_nextStateTime = gpGlobals->curtime + shootTime;
	m_nextShootTime = gpGlobals->curtime;
	// don't twitch while shooting
	m_nextTwitchTime = m_nextStateTime;
}

void CMinigun::SetShootDuration( float duration )
{
	if ( m_minigunState == MINIGUN_SHOOTING )
	{
		m_nextStateTime = gpGlobals->curtime + duration;
	}
}

void CMinigun::StopShootingForSeconds( IMinigunHost *pHost, float duration )
{
	float soonestShoot = gpGlobals->curtime + duration;
	if ( m_minigunState == MINIGUN_SHOOTING )
	{
		ChangeState( pHost );
		m_nextStateTime = soonestShoot;
	}
	else
	{
		if ( m_nextStateTime < soonestShoot )
		{
			m_nextStateTime = soonestShoot;
		}
		m_nextShootTime = gpGlobals->curtime + 1e4;
	}
}

void CMinigun::ChangeState( IMinigunHost *pHost )
{
	switch( m_minigunState )
	{
	case MINIGUN_OFF:
		{
			CBaseEntity *pTarget = FindTarget( pHost );
			if ( pTarget )
			{
				if( pTarget == pHost->GetEntity()->GetEnemy() )
				{
					// Shoot at enemies for 2 seconds.
					ShootAtTarget( pHost, pTarget, 2 );
				}
				else
				{
					// Otherwise, retain the behavior in place as of E3 2003.
					ShootAtTarget( pHost, pTarget, -1 );
				}
				break;
			}
		}
		// fall through when there is no target!
		
	case MINIGUN_SHOOTING:
		{
			m_yaw.rate = 360;
			m_pitch.rate = 180;
			m_nextShootTime = gpGlobals->curtime + 1e4;
			m_minigunState = MINIGUN_OFF;
			if ( m_hTarget )
			{
				variant_t emptyVariant;
				m_hTarget->AcceptInput( "InputReleased", pHost->GetEntity(), pHost->GetEntity(), emptyVariant, 0 );
			}
			m_hTarget = NULL;
			m_nextStateTime = gpGlobals->curtime + 4.0;
		}
		break;
	}
}

void CMinigun::Think( IMinigunHost *pHost, float dt )
{
	if ( !m_enable )
		return;

	if ( m_nextStateTime <= gpGlobals->curtime )
	{
		ChangeState( pHost );
	}

	if ( m_nextTwitchTime <= gpGlobals->curtime )
	{
		// invert one and randomize the other.
		// This has the effect of making the gun cross the field of
		// view more often - like he's looking around
		m_randomState = !m_randomState;
		if ( m_randomState )
		{
			m_yaw.Random( MINIGUN_MINYAW, MINIGUN_MAXYAW, 360, 720 );
			m_pitch.target = -m_pitch.target;
		}
		else
		{
			m_pitch.Random( MINIGUN_MINPITCH, MINIGUN_MAXPITCH, 270, 360 );
			m_yaw.target = -m_yaw.target;
		}
		m_nextTwitchTime = gpGlobals->curtime + random->RandomFloat( 0.3, 2 );
	}

	CBaseEntity *pTargetEnt = m_hTarget.Get();
	if ( pTargetEnt )
	{
		AimAtTarget( pHost->GetEntity(), pTargetEnt );
	}
	m_yaw.Update( dt );
	m_pitch.Update( dt );

	pHost->UpdateMinigunControls( m_yaw.current, m_pitch.current );

	if ( m_nextShootTime <= gpGlobals->curtime )
	{
		if( pTargetEnt && pTargetEnt == pHost->GetEntity()->GetEnemy() )
		{
			// Shooting at the Strider's enemy. Strafe to target!
			float flRemainingShootTime = m_nextStateTime - m_nextShootTime;
			float flFactor = flRemainingShootTime / m_shootDuration;

			Vector vecTarget = pTargetEnt->WorldSpaceCenter();

			Vector vecLine = ( vecTarget - Vector( 0, 0, 256 ) ) - vecTarget;
			
			//float flDist = VectorNormalize( vecLine ) * 0.6;
			float flDist = VectorNormalize( vecLine );

			// The target isn't the end of the line that we're going to strafe,
			// the target is about 10% of the way from the end of it. So 
			// if they do not find cover, they're definitely going to be hit 
			// a few times as the strider strafes through their position.
			//vecTarget += vecLine * (flDist * -0.1);
			vecTarget += vecLine * flDist * flFactor;

			pHost->ShootMinigun( &vecTarget, GetAimError(), VECTOR_CONE_3DEGREES );
		}
		else
		{
			const Vector *pTargetPoint = pTargetEnt ? &pTargetEnt->GetAbsOrigin() : NULL;
			pHost->ShootMinigun( pTargetPoint, GetAimError() );
		}

		m_nextShootTime = gpGlobals->curtime + 0.1;
	}
}

class CStriderNavigator : public CAI_ComponentWithOuter<CNPC_Strider, CAI_Navigator>
{
	typedef CAI_ComponentWithOuter<CNPC_Strider, CAI_Navigator> BaseClass;
public:
	CStriderNavigator( CNPC_Strider *pOuter )
	 :	BaseClass( pOuter )
	{
	}

	void MoveCalcBaseGoal( AILocalMoveGoal_t *pMoveGoal );
	bool MoveUpdateWaypoint( AIMoveResult_t *pResult );
	bool DoFindPathToPos( void );

};

//-----------------------------------------------------------------------------
// Purpose: Strider vehicle server
//-----------------------------------------------------------------------------
class CStriderServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;
// IServerVehicle
public:
	void	GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles );
	void	GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	bool	IsPassengerUsingStandardWeapons( int nRole = VEHICLE_DRIVER ) { return true; }

protected:
	CNPC_Strider *GetStrider( void );
};

//==================================================
// CNPC_Strider
//==================================================
class CNPC_Strider : public CAI_TrackPather, public IMinigunHost, public IDrivableVehicle
{
	DECLARE_CLASS( CNPC_Strider, CAI_TrackPather );
public:
	DECLARE_SERVERCLASS();

	CNPC_Strider( void );

	// CBaseEntity
	void	Precache();
	void	Spawn();
	void	Activate();
	void	UpdateOnRemove( void )
	{
		m_BoneFollowerManager.DestroyBoneFollowers();
		BaseClass::UpdateOnRemove();
	}
	void	Event_Killed( const CTakeDamageInfo &info );
	void	VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );

	// CBaseAnimating
	virtual void	CalculateIKLocks( float currentTime )
	{
		BaseClass::CalculateIKLocks( currentTime );
		if ( m_pIk && m_pIk->m_target.Count() )
		{
			// HACKHACK: Hardcoded 11???  Not a cleaner way to do this
			iktarget_t &target = m_pIk->m_target[STOMP_IK_SLOT];
			target.est.pos = m_vecHitPos;
			// yellow box at target pos - helps debugging
			//NDebugOverlay::Box( m_vecHitPos, Vector(-8,-8,-8), Vector(8,8,8), 255, 255, 0, 0, 0.1 );
		}
	}

	// CAI_BaseNPC
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	int		RangeAttack1Conditions( float flDot, float flDist );
	int		MeleeAttack1Conditions( float flDot, float flDist );
	int		MeleeAttack2Conditions( float flDot, float flDist );
	bool	InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );
	int		SelectSchedule();
	int		TranslateSchedule( int scheduleType );
	float	MaxYawSpeed();
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector ) { return false; }

	Vector	BodyTarget( const Vector &posSrc, bool bNoisy );
	void	NPCThink();
	Disposition_t IRelationType( CBaseEntity *pTarget );
	bool	IsValidEnemy( CBaseEntity *pTarget );
	void	OnStateChange( NPC_STATE oldState, NPC_STATE newState );
	void	BuildScheduleTestBits();
	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	bool	FCanCheckAttacks( void );
	bool	GetTrackPatherTarget( Vector *pPos );
	void	StartTargetHandling( CBaseEntity *pTargetEnt );
	bool	OverrideMove( float flInterval );
	void	InputFlyToPathTrack( inputdata_t &inputdata );

	// BaseCombatCharacter
	Vector Weapon_ShootPosition();

	friend class CStriderNavigator;
	CAI_Navigator *CreateNavigator()
	{
		return new CStriderNavigator( this );
	}

	void	HandleAnimEvent( animevent_t *pEvent );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	
	// IMinigunHost
	void ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread = vec3_origin );
	void UpdateMinigunControls( float &yaw, float &pitch );
	void GetViewCone( viewcone_t &cone );
	CAI_BaseNPC *GetEntity() { return this; }

	// IDrivableVehicle
public:
	virtual CBaseEntity *GetDriver( void ) { return m_hPlayer; }
	virtual void		ItemPostFrame( CBasePlayer *pPlayer )
	{
		if ( !m_isHatchOpen && (pPlayer->m_afButtonPressed & IN_USE) )
		{
			OpenHatch();
		}
		else if ( pPlayer->m_afButtonPressed & IN_JUMP )
		{
			pPlayer->LeaveVehicle();
			m_nextTouchTime = gpGlobals->curtime + 2; // debounce
		}

		// Prevent the base class using the use key
		pPlayer->m_afButtonPressed &= ~IN_USE;
	}
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool		CanEnterVehicle( CBaseEntity *pEntity ) { return true; }
	virtual bool		CanExitVehicle( CBaseEntity *pEntity ) { return true; }
	virtual void		EnterVehicle( CBasePlayer *pPlayer );
	virtual void		ExitVehicle( int iRole );
	virtual bool		PlayExitAnimation( void ) { return false; }

	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle	*GetServerVehicle() { return &m_ServerVehicle; }

protected:
	// Contained IServerVehicle
	CStriderServerVehicle	m_ServerVehicle;
	// Contained Bone Follower manager
	CBoneFollowerManager	m_BoneFollowerManager;

	// inputs
public:
	void	InputSetMinigunTime( inputdata_t &inputdata );
	void	InputSetMinigunTarget( inputdata_t &inputdata );
	void	InputSetCannonTarget( inputdata_t &inputdata );
	void	InputFlickRagdoll( inputdata_t &inputdata );
private:
	// locals
	void	UpdateMinigun();
	void	ChangeMinigunState();
	bool	AimCannonAt( CBaseEntity *pEntity, float flInterval );
	CBaseEntity *GetCannonTarget();
	Vector	LeftFootHit();
	Vector	RightFootHit();
	Vector	BackFootHit();
	void	StompHit( int followerBoneIndex );

	bool	HasScriptedEnemy() const;
	bool	IsScriptedEnemy( CBaseEntity *pTarget ) const;

	void	FootFX( const Vector &origin );
	virtual void TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance );
	Vector	CalculateStompHitPosition( CBaseEntity *pEnemy );

	virtual bool CreateVPhysics();
	void	OpenHatch();

public:
	// Implement "Big Monster" collision detection! (BMCD)
	//-----------------------------------------------------------------------------

	// Conservative collision volumes
	static Vector m_cullBoxStandMins;
	static Vector m_cullBoxStandMaxs;
	static Vector m_cullBoxCrouchMins;
	static Vector m_cullBoxCrouchMaxs;
	static float m_strideLength;

	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	virtual void SetObjectCollisionBox( void );
	void Touch( CBaseEntity *pOther );
	//-----------------------------------------------------------------------------

	Class_T	Classify( void );

	DECLARE_DATADESC();

private:

	CMinigun	m_miniGun;
	int		m_miniGunAmmo;
	float	m_nextShootTime;
	float	m_nextStompTime;
	float	m_nextTouchTime;
	float	m_ragdollTime;
	float	m_miniGunShootDuration;
	float	m_aimYaw;
	float	m_aimPitch;
	Vector	m_blastHit;
	Vector	m_blastNormal;
	Vector	m_savedViewOffset;
	CNetworkVector( m_vecHitPos );

	EHANDLE	m_hRagdoll;
	EHANDLE	m_hCannonTarget;

	int		m_BodyTargetBone;

	CNetworkHandle( CBasePlayer, m_hPlayer );

	bool	m_isHatchOpen;

	bool	m_bPathDirty;

	DEFINE_CUSTOM_AI;
};

IMPLEMENT_SERVERCLASS_ST(CNPC_Strider, DT_NPC_Strider)
	SendPropVector(SENDINFO(m_vecHitPos), -1, SPROP_COORD),
	SendPropEHandle(SENDINFO(m_hPlayer)),
END_SEND_TABLE()


Vector CNPC_Strider::m_cullBoxStandMins;
Vector CNPC_Strider::m_cullBoxStandMaxs;
Vector CNPC_Strider::m_cullBoxCrouchMins;
Vector CNPC_Strider::m_cullBoxCrouchMaxs;
float CNPC_Strider::m_strideLength = 0;

// Implement a navigator so that the strider can get closer to the ground when he's walking on a slope
// This gives him enough reach for his legs to IK to lower ground points
void LookaheadPath( const Vector &current, AI_Waypoint_t *pWaypoint, float dist, Vector &nextPos )
{
	if ( !pWaypoint )
		return;

	Vector dir = pWaypoint->GetPos() - current;
	float dist2D = dir.Length2D();
	if ( dist2D > 0.1 )
	{
		if ( dist <= dist2D )
		{
			nextPos = ((dist / dist2D) * dir) + current;
			return;
		}
	}
	nextPos = pWaypoint->GetPos();
	dist -= dist2D;
	dir = nextPos;
	
	LookaheadPath( dir, pWaypoint->GetNext(), dist, nextPos );
}

// Find the forward slope and lower the strider height when moving downward
void CStriderNavigator::MoveCalcBaseGoal( AILocalMoveGoal_t *pMoveGoal )
{
	BaseClass::MoveCalcBaseGoal( pMoveGoal );
	Vector dest = pMoveGoal->target;
	LookaheadPath( GetAbsOrigin(), pMoveGoal->pPath->GetCurWaypoint(), CNPC_Strider::m_strideLength, dest );

	//NDebugOverlay::Box( dest, Vector(-16,-16,-16), Vector(16,16,16), 0, 255, 0, 255, 0.1 );
	Vector unitDir = pMoveGoal->dir * CNPC_Strider::m_strideLength;
	unitDir.z = dest.z - GetAbsOrigin().z;
	VectorNormalize( unitDir );
	float heightAdj = unitDir.z * CNPC_Strider::m_strideLength;
	if ( heightAdj < -1 )
	{
		heightAdj = clamp( heightAdj, -192, 0 );
		pMoveGoal->target.z += heightAdj;
		pMoveGoal->flags |= AILMG_NO_STEER;
		pMoveGoal->dir = unitDir * CNPC_Strider::m_strideLength * 0.1;
		pMoveGoal->dir.z += heightAdj;
		VectorNormalize( pMoveGoal->dir );
	}
}

bool CStriderNavigator::MoveUpdateWaypoint( AIMoveResult_t *pResult )
{
	// Note that goal & waypoint tolerances are handled in progress blockage cases (e.g., OnObstructionPreSteer)

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float 		   waypointDist = (pCurWaypoint->GetPos() - GetAbsOrigin()).Length2D();
	bool		   bIsGoal		= CurWaypointIsGoal();
	// HACKHACK: adjust this tolerance
	// The strider is following paths most of the time, but he needs a bunch of freedom
	// to adjust himself to make his feet look good.
	float		   tolerance	= 10.0;

	if ( waypointDist <= tolerance )
	{
		if ( bIsGoal )
		{
			OnNavComplete();
			*pResult = AIMR_OK;
			
		}
		else
		{
			AdvancePath();
			*pResult = AIMR_CHANGE_TYPE;
		}
		return true;
	}
	
	return false;
}

bool CStriderNavigator::DoFindPathToPos( void )
{
	if ( GetOuter()->IsOnPathTrack() )
	{
		// We set a dummy path to keep the rest of the AI happy vis-a-vis navigation state
		CAI_Path *pPath = GetPath();
		//pPath->Clear();
		pPath->SetWaypoints( new AI_Waypoint_t( pPath->ActualGoalPosition(), 0, NAV_FLY, bits_WP_TO_GOAL, NO_NODE ) );
		GetOuter()->m_bPathDirty = true;
		return true;
	}
	return BaseClass::DoFindPathToPos();
}


LINK_ENTITY_TO_CLASS( npc_strider, CNPC_Strider );

//==================================================
// CNPC_Strider::m_DataDesc
//==================================================

BEGIN_DATADESC( CNPC_Strider )

	DEFINE_EMBEDDED( CNPC_Strider, m_ServerVehicle ),
	DEFINE_EMBEDDED( CNPC_Strider, m_miniGun ),
	DEFINE_FIELD( CNPC_Strider, m_miniGunAmmo,			FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Strider, m_nextStompTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Strider, m_nextTouchTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Strider, m_nextShootTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Strider, m_ragdollTime,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_Strider, m_miniGunShootDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Strider, m_aimYaw,				FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Strider, m_aimPitch,				FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Strider, m_blastHit,				FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Strider, m_blastNormal,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Strider, m_savedViewOffset,		FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Strider, m_vecHitPos,			FIELD_VECTOR ),

	// This is reconstructed in CreateVPhysics
	// DEFINE_EMBEDDED( CNPC_Strider, m_BoneFollowerManager ),

	DEFINE_FIELD( CNPC_Strider, m_hRagdoll,				FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_Strider, m_hCannonTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_Strider, m_BodyTargetBone,		FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Strider, m_hPlayer,				FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_Strider, m_isHatchOpen,			FIELD_BOOLEAN ),	
	// DEFINE_FIELD( CNPC_Strider, m_bPathDirty, FIELD_BOOLEAN ),

	// inputs
	DEFINE_INPUTFUNC( CNPC_Strider, FIELD_FLOAT, "SetMinigunTime", InputSetMinigunTime ),
	DEFINE_INPUTFUNC( CNPC_Strider, FIELD_STRING, "SetMinigunTarget", InputSetMinigunTarget ),
	DEFINE_INPUTFUNC( CNPC_Strider, FIELD_STRING, "SetCannonTarget", InputSetCannonTarget ),
	DEFINE_INPUTFUNC( CNPC_Strider, FIELD_STRING, "FlickRagdoll", InputFlickRagdoll ),

	// Function Pointers
//	DEFINE_FUNCTION( CNPC_Strider, JumpTouch ),

END_DATADESC()

CNPC_Strider::CNPC_Strider( void )
{
	m_ServerVehicle.SetVehicle( this );
}

//==================================================
// Purpose: Spawn the creature
//==================================================

void CNPC_Strider::Spawn( void )
{
	Precache();

	m_miniGunAmmo = GetAmmoDef()->Index("CombineCannon"); 
	m_miniGun.Init();

	// UNDONE: Enable after E3 -- off by default is easier to script, harder for real levels
	//AddClassRelationship( CLASS_BULLSEYE, D_HT, -1 );
	SetModel( "models/combine_strider.mdl" );
	BaseClass::Spawn();

	//m_debugOverlays |= OVERLAY_NPC_ROUTE_BIT | OVERLAY_BBOX_BIT;
	SetHullType( HULL_LARGE_CENTERED );
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetNavType( NAV_FLY );
	m_flGroundSpeed	= 500;
	m_NPCState = NPC_STATE_NONE;
	m_bloodColor = BLOOD_COLOR_MECH;
	m_iHealth = sk_strider_health.GetFloat();
	m_flFieldOfView = -0.707; // 270 degrees for now


	AddFlag( FL_FLY );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	// BMCD: Get the conservative boxes from sequences
	ExtractBbox( SelectHeaviestSequence( ACT_WALK ), m_cullBoxStandMins, m_cullBoxStandMaxs ); 
	ExtractBbox( SelectHeaviestSequence( ACT_WALK_CROUCH ), m_cullBoxCrouchMins, m_cullBoxCrouchMaxs ); 
	// BMCD: Force collision hooks
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );

	CNPC_Strider::m_strideLength = (m_cullBoxStandMaxs.x - m_cullBoxStandMins.x) * 0.5;

	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 );

	// setup absorigin
	Relink();

	// find the ground
	Vector mins(-16,-16,-16), maxs(16,16,16);
	Vector origin = GetLocalOrigin();

	MoveToGround( &origin, this, mins, maxs );

	// move up to strider stand height
	origin.z += (GetAbsOrigin().z - GetAbsMins().z) + mins.z;

	SetLocalOrigin( origin );

	// reset the origin now that we moved it
	Relink();

	const float	STRIDER_LEAD_DISTANCE		= 120.0f;
	const float STRIDER_AVOID_DIST			= 0.0f;
	const float STRIDER_MIN_CHASE_DIST_DIFF	= 60.0f;	// Distance threshold used to determine when a target has moved enough to update our navigation to it

	InitPathingData( STRIDER_LEAD_DISTANCE, STRIDER_MIN_CHASE_DIST_DIFF, STRIDER_AVOID_DIST );
	NPCInit();

	CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_immolator" );
	if ( pWeapon )
	{
		Weapon_Equip( pWeapon );
		pWeapon->m_fEffects |= EF_NODRAW;
	}

	g_YawControl = LookupPoseParameter( "yaw" );
	g_PitchControl = LookupPoseParameter( "pitch" );
	g_CannonAttachment = LookupAttachment( "BigGun" );

	EnableServerIK();

	m_BodyTargetBone = -1;
}


//==================================================

void CNPC_Strider::Activate( void )
{
	BaseClass::Activate();

	const char *pszBodyTargetBone = "combine_strider.neck_bone";

	m_BodyTargetBone = LookupBone( pszBodyTargetBone );

	if ( m_BodyTargetBone == -1 )
		Msg( "Couldn't find npc_strider bone %s, which is used as target for others\n", pszBodyTargetBone );
}


//==================================================
// Purpose: Precache anything that needs it
//==================================================

void CNPC_Strider::Precache( void )
{
	engine->PrecacheModel( "models/combine_strider.mdl" );
	BaseClass::Precache();
}


//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::Weapon_ShootPosition( )
{
	matrix3x4_t gunMatrix;
	GetAttachment( g_CannonAttachment, gunMatrix );

	Vector vecShootPos;
	MatrixGetColumn( gunMatrix, 3, vecShootPos );

	return vecShootPos;
}

void CNPC_Strider::ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread )
{
	Vector muzzlePos, forward;
	QAngle muzzleAng;
	GetAttachment( "minigun", muzzlePos, muzzleAng );

	if ( aimError < 3 && pTarget )
	{
		forward = *pTarget - muzzlePos;
		VectorNormalize(forward);
	}
	else
	{
		AngleVectors( muzzleAng, &forward );
	}
	// exactly on target w/tracer
	FireBullets( 1, muzzlePos, forward, vecSpread, 8192, m_miniGunAmmo, 1 );
	// a couple of wild shots around it w/o tracers
	FireBullets( 2, muzzlePos, forward, vecSpread + VECTOR_CONE_3DEGREES, 8192, m_miniGunAmmo, 2 );
	g_pEffects->MuzzleFlash( muzzlePos, muzzleAng, random->RandomFloat( 2.0f, 4.0f ) , MUZZLEFLASH_TYPE_DEFAULT );

	EmitSound( "NPC_Strider.FireMinigun" );
}

void CNPC_Strider::UpdateMinigunControls( float &yaw, float &pitch ) 
{
	SetPoseParameter( "miniGunYaw", yaw );
	SetPoseParameter( "miniGunPitch", pitch );

	yaw = GetPoseParameter( "miniGunYaw" );
	pitch = GetPoseParameter( "miniGunPitch" );
}

bool CNPC_Strider::FCanCheckAttacks( void )
{
	// Strider has direct and indirect attacks, so he's always checking
	// as long as the enemy is in front of him.
	if( FInViewCone( GetEnemy() ) )
	{
		return true;
	}

	return false;
}

void CNPC_Strider::StartTargetHandling( CBaseEntity *pTargetEnt )
{
	CPathTrack *pTrack = dynamic_cast<CPathTrack *>(pTargetEnt);
	if ( pTrack )
	{
		SetTrack( pTargetEnt );
		return;
	}
	BaseClass::StartTargetHandling( pTargetEnt );
}

void CNPC_Strider::InputFlyToPathTrack( inputdata_t &inputdata )
{
	BaseClass::InputFlyToPathTrack( inputdata );

	CBaseEntity *pTrackTarget = GetDestPathTarget();
	if ( pTrackTarget )
	{
		SetCondition( COND_TRACK_PATH_GO );
		ClearForcedMove();
	}
}

bool CNPC_Strider::OverrideMove( float flInterval )
{
	if ( GetNavigator()->IsGoalActive() && IsOnPathTrack() )
	{
		CAI_Path *pPath = GetNavigator()->GetPath();
		Vector curGoal = GetDesiredPosition();
		UpdateTrackNavigation();

		if ( m_bPathDirty )
		{
			Assert( pPath->CurWaypointIsGoal() );

			CBaseEntity *pTrackTarget = GetDestPathTarget();
			Assert( pTrackTarget );

			AI_Waypoint_t *pWaypoint = pPath->GetCurWaypoint();
			pPath->ResetGoalPosition( pTrackTarget->GetAbsOrigin() );
			pWaypoint->SetPos( pTrackTarget->GetAbsOrigin() );
		}

		if ( m_bPathDirty || curGoal != GetDesiredPosition() )
		{
			AI_Waypoint_t *pWaypoints = GetPathfinder()->BuildLocalRoute( GetAbsOrigin(), 
																		  GetDesiredPosition(), 
																		  pPath->GetTarget(), 
																		  bits_WP_DONT_SIMPLIFY, 
																		  NO_NODE, 
																		  bits_BUILD_FLY, 
																		  0.0 );

			if ( !pWaypoints )
			{
				TaskFail( FAIL_NO_ROUTE );
				m_bPathDirty = false; // set here rather than above to ease debugging conditions in case of failure
				return true;
			}
			m_bPathDirty = false;
			if ( !pPath->CurWaypointIsGoal() )
				pPath->Advance();
			pPath->PrependWaypoints(  pWaypoints );
		}
	}

	return false;
}

bool CNPC_Strider::GetTrackPatherTarget( Vector *pPos )
{
	if ( GetNavigator()->IsGoalActive() )
	{
		*pPos = GetNavigator()->GetPath()->ActualGoalPosition();
		return true;
	}
	return false;
}


void CNPC_Strider::GetViewCone( viewcone_t &cone )
{
	cone.origin = EyePosition();
	GetVectors( &cone.axis, NULL, NULL );
	cone.cosAngle = 0.5; // 60 degree cone
	cone.length = 2048;
}

void CNPC_Strider::InputSetMinigunTime( inputdata_t &inputdata )
{
	m_miniGunShootDuration = inputdata.value.Float();
	m_miniGun.SetShootDuration( m_miniGunShootDuration );
}

void CNPC_Strider::InputSetMinigunTarget( inputdata_t &inputdata )
{
	CBaseEntity *pTargetEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), inputdata.pActivator );

	m_miniGun.ShootAtTarget( this, pTargetEntity, m_miniGunShootDuration );
	m_miniGunShootDuration = 0;
}

bool CNPC_Strider::InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	return true;
}

void CNPC_Strider::InputSetCannonTarget( inputdata_t &inputdata )
{
	// clear old target
	m_hCannonTarget = NULL;

	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), inputdata.pActivator );

	if ( pTarget )
	{
		m_hCannonTarget = pTarget;
		m_miniGun.StopShootingForSeconds( this, 10 );
	}
}

void CNPC_Strider::InputFlickRagdoll( inputdata_t &inputdata )
{
	if ( m_hRagdoll.Get() )
	{
		SetCondition( COND_STRIDER_DO_FLICK );
	}
}

void CNPC_Strider::NPCThink(void)
{
	if ( m_hRagdoll.Get() )
	{
		m_nextStompTime = gpGlobals->curtime + 5;
	}

	BaseClass::NPCThink();
	
	m_miniGun.Think( this, 0.1 );

	// update follower bones
	m_BoneFollowerManager.UpdateBoneFollowers();
}

bool CNPC_Strider::CreateVPhysics()
{
	BaseClass::CreateVPhysics();

	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames );
	return true;
}

static void MoveToGround( Vector *position, CBaseEntity *ignore, const Vector &mins, const Vector &maxs )
{
	trace_t tr;
	// Find point on floor where enemy would stand at chasePosition
	Vector floor = *position;
	floor.z -= 1024;
	AI_TraceHull( *position, floor, mins, maxs, MASK_NPCSOLID, ignore, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1 )
	{
		position->z = tr.endpos.z;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Compute the nav position for the strider's origin relative to this enemy.
//-----------------------------------------------------------------------------
void CNPC_Strider::TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance )
{
	tolerance = 64;
	if ( ! (pEnemy->GetFlags() & FL_ONGROUND) )
	{
		MoveToGround( &chasePosition, pEnemy, pEnemy->WorldAlignMins(), pEnemy->WorldAlignMaxs() );
	}
	// move down to enemy's feet for enemy origin at chasePosition
	chasePosition.z += pEnemy->GetAbsMins().z - pEnemy->GetAbsOrigin().z;
	// move up to strider stand height
	chasePosition.z += (GetAbsOrigin().z - GetAbsMins().z) + npc_strider_height_adj.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CNPC_Strider::Event_Killed( const CTakeDamageInfo &info )
{
	// no more riding on top
	m_nextTouchTime = gpGlobals->curtime + 1e6;

	m_BoneFollowerManager.DestroyBoneFollowers();
	if ( m_hPlayer.Get() )
	{
		m_hPlayer->LeaveVehicle();
	}

	CEntityMessageFilter filter( this, "CNPC_Strider" );
	MessageBegin( filter, 0 );
		WRITE_BYTE( STRIDER_MSG_DEAD );
	MessageEnd();
	BaseClass::Event_Killed( info );
}


Vector CNPC_Strider::CalculateStompHitPosition( CBaseEntity *pEnemy )
{
	Vector skewerPosition, footPosition;
	QAngle angles;
	GetAttachment( "left skewer", skewerPosition, angles );
	GetAttachment( "left foot", footPosition, angles );
	
	Vector vecStabPos = ( pEnemy->WorldSpaceCenter() + pEnemy->EyePosition() ) * 0.5f;

	return vecStabPos - skewerPosition + footPosition;
}

CBaseEntity *CNPC_Strider::GetCannonTarget()
{
	CBaseEntity *pTarget = m_hCannonTarget;
	if ( !pTarget )
	{
		pTarget = GetEnemy();
	}

	return pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Aim Gun at a target
// Output : Returns true if you hit the target, false if not there yet
//-----------------------------------------------------------------------------
bool CNPC_Strider::AimCannonAt( CBaseEntity *pEntity, float flInterval )
{
	if ( !pEntity )
		return true;

	matrix3x4_t gunMatrix;
	GetAttachment( g_CannonAttachment, gunMatrix );

	// transform the enemy into gun space
	m_vecHitPos = pEntity->GetAbsOrigin();
	Vector localEnemyPosition;
	VectorITransform( pEntity->GetAbsOrigin(), gunMatrix, localEnemyPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	Vector unitAngles = Vector( localEnemyAngles.x, localEnemyAngles.y, localEnemyAngles.z ); 
	float angleDiff = VectorNormalize(unitAngles);
	const float aimSpeed = 16;

	// Exponentially approach the target
	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed = max(yawSpeed,5);
	pitchSpeed = max(pitchSpeed,5);

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( g_YawControl, m_aimYaw );
	SetPoseParameter( g_PitchControl, m_aimPitch );

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = GetPoseParameter( g_PitchControl );
	m_aimYaw = GetPoseParameter( g_YawControl );

	// UNDONE: Zero out any movement past the limit and go ahead and fire if the strider hit its 
	// target except for clamping.  Need to clamp targets to limits and compare?
	if ( angleDiff < 1 )
		return true;

	return false;
}


//==================================================
// Purpose: Handle any pre-task needs
// Input:	pTask - task that is about to start
//==================================================
void CNPC_Strider::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STRIDER_AIM:
		{
			SetIdealActivity( (Activity)ACT_STRIDER_AIMRA1 );
			m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			m_aimYaw = 0;
			m_aimPitch = 0;
			// clear out the previous shooting
			SetPoseParameter( g_YawControl, m_aimYaw );
			SetPoseParameter( g_PitchControl, m_aimPitch );
			matrix3x4_t gunMatrix;
			GetAttachment( g_CannonAttachment, gunMatrix );
			Vector vecShootPos;
			MatrixGetColumn( gunMatrix, 3, vecShootPos );
		
			// tell the client side effect to complete

			CEntityMessageFilter filter( this, "CNPC_Strider" );
			filter.MakeReliable();

			MessageBegin( filter, 0 );
			WRITE_BYTE( STRIDER_MSG_STREAKS );
			WRITE_VEC3COORD( vecShootPos );
			MessageEnd();
			CPASAttenuationFilter filter2( this, "NPC_Strider.Charge" );
			EmitSound( filter2, entindex(), "NPC_Strider.Charge" );

			//	CPVSFilter filter( vecShootPos );
			//te->StreakSphere( filter, 0, 0, 150, 100, entindex(), g_CannonAttachment );
		}
		break;

	case TASK_STRIDER_DODGE:
		break;

	case TASK_STRIDER_STOMP:
		{
			m_nextStompTime = gpGlobals->curtime + 5;
			Activity stompAct = (Activity) ( pTask->flTaskData > 0 ? ACT_STRIDER_STOMPR : ACT_STRIDER_STOMPL );
			ResetIdealActivity( stompAct );
		}
		break;

	case TASK_RANGE_ATTACK2:
		Msg("BOOM!\n");
		CBaseCombatWeapon *pWeapon;
		pWeapon = GetActiveWeapon();

		if( pWeapon )
		{
			pWeapon->PrimaryAttack();
		}
		else
		{
			TaskFail("no primary weapon");
		}

		TaskComplete();
		break;

	case TASK_STRIDER_BREAKDOWN:
		SetIdealActivity( (Activity)ACT_STRIDER_SLEEP );
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//==================================================
// Purpose:	Run the current task
// Input:	pTask - current task
//==================================================

void CNPC_Strider::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STRIDER_AIM:
		{
			// BUGBUG: Need the real flInterval here, not just 0.1
			AimCannonAt( GetCannonTarget(), 0.1 );
			if ( gpGlobals->curtime > m_flWaitFinished )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_STRIDER_DODGE:
		TaskComplete();
		break;

	case TASK_STRIDER_STOMP:
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		if ( GetEnemy() )
		{
			m_vecHitPos = CalculateStompHitPosition( GetEnemy() );
		}
		break;

	case TASK_STRIDER_BREAKDOWN:
		if ( IsActivityFinished() )
		{
			// UNDONE: Fix this bug!
			//Assert(!IsMarkedForDeletion());
			if ( !IsMarkedForDeletion() )
			{
				CTakeDamageInfo info;
				CreateServerRagdoll( this, 0, info, COLLISION_GROUP_NONE );
				TaskComplete();
				UTIL_Remove(this);
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;		
	}
}

//==================================================
// Purpose: Return the class of this NPC
// Output:	The class we are
//==================================================

Class_T CNPC_Strider::Classify( void )
{
	return CLASS_COMBINE;
}


bool CNPC_Strider::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	// Let normal hitbox code handle rays
	if ( mask & CONTENTS_HITBOX )
	{
		return BaseClass::TestCollision( ray, mask, trace );
	}


	return TestCollisionBox( ray, mask, trace, WorldAlignMins() + GetAbsOrigin(), WorldAlignMaxs() + GetAbsOrigin() );
}

// BMCD: Use the conservative boxes for rough culling
void CNPC_Strider::SetObjectCollisionBox( void )
{
	// Set up the conservative culling box
	// UNDONE: use crouch when crouched
	SetAbsMins( GetAbsOrigin() + m_cullBoxStandMins );
	SetAbsMaxs( GetAbsOrigin() + m_cullBoxStandMaxs );
}


void CNPC_Strider::Touch( CBaseEntity *pOther )
{
	// debounce players who just jumped off
	if ( gpGlobals->curtime < m_nextTouchTime )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	if ( !pPlayer )
		return;

	Vector mins, maxs;
	mins = WorldAlignMins() + GetAbsOrigin();
	maxs = WorldAlignMaxs() + GetAbsOrigin();
	mins += Vector(8,8,8);
	maxs -= Vector(8,8,8);
	float height = maxs.z - mins.z;
	mins.z += height;
	maxs.z += height;
	
	// did player land on top?
	if ( !IsBoxIntersectingBox( pPlayer->GetAbsMins(), pPlayer->GetAbsMaxs(), mins, maxs ) )
		return; // no.

	pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_DRIVER);
#if 0
	static bool g_Draw = true;
	if ( g_Draw )
	{
		Vector center = 0.5 * mins + 0.5 * maxs;
		NDebugOverlay::Box( center, mins-center, maxs-center, 255, 0, 0, 32, 10 );
		g_Draw = false;
	}
	Msg("STRIDER KILL!\n");
#endif
}

int CNPC_Strider::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( m_hCannonTarget.Get() != NULL )
	{
		return COND_CAN_RANGE_ATTACK1;
	}

	// E3: HACKHACK: Don't shoot cannon unless told to
	return COND_NONE;

	if (gpGlobals->curtime < m_nextShootTime)
	{
		return COND_NONE;
	}
	// too close, but don't change schedules/movement
	if ( flDist < 1200 )
	{
		return COND_NONE;
	}
	return BaseClass::RangeAttack1Conditions( flDot, flDist );
}


void CNPC_Strider::OpenHatch()
{
	EmitSound( "NPC_Strider.OpenHatch" );
	m_isHatchOpen = true;

	CSteamJet *pJet= (CSteamJet *)CreateEntityByName("env_steam");
	pJet->Spawn();
	pJet->SetAbsOrigin( GetAbsOrigin() + Vector(0,0,24) );
	pJet->m_bEmit = true;
	pJet->m_SpreadSpeed = 10;
	pJet->m_Speed = 40;
	pJet->m_Rate = 15;
	pJet->m_StartSize = 5;
	pJet->m_EndSize	= 15;
	pJet->m_JetLength = 800;
	pJet->SetRenderColor( 18, 18, 18, 255 );
	//NOTENOTE: Set pJet->m_clrRender->a here for translucency
	pJet->SetParent(this);
	pJet->SetLocalAngles( QAngle(-90,0,0) );// point up
	pJet->SetLifetime(3.0);
}


int CNPC_Strider::MeleeAttack1Conditions( float flDot, float flDist )
{
	if (gpGlobals->curtime < m_nextStompTime)
	{
		return COND_NONE;
	}

	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy || pEnemy->Classify() == CLASS_BULLSEYE )
		return COND_NONE;

	if ( flDist <= STRIDER_STOMP_RANGE )
	{
		Vector right;
		GetVectors( NULL, &right, NULL );
		// strider will cross his feet, but only 6ft over
		if ( DotProduct( pEnemy->GetAbsOrigin() - GetAbsOrigin(), right ) <= 72 )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}

	// too far, but don't change schedules/movement
	return COND_NONE;
}

int CNPC_Strider::MeleeAttack2Conditions( float flDot, float flDist )
{
	// HACKHACK: Disabled until we get a good right-leg animation
	return COND_NONE;
}

int CNPC_Strider::SelectSchedule()
{
	CBaseEntity *pTrackTarget = GetDestPathTarget();
	if ( pTrackTarget && HasCondition( COND_TRACK_PATH_GO ) )
		SetTarget( pTrackTarget );

	switch ( m_NPCState )
	{
	case NPC_STATE_SCRIPT:
		return BaseClass::SelectSchedule();
	case NPC_STATE_DEAD:
		return SCHED_STRIDER_DIE;
	default:
		if ( !HasCondition( COND_NEW_ENEMY ) )
		{
			if ( m_hRagdoll.Get() && (gpGlobals->curtime > m_ragdollTime  || HasCondition( COND_STRIDER_DO_FLICK ) ) )
			{
				return SCHED_STRIDER_FLICKL;
			}
		}
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			return SCHED_STRIDER_RANGE_ATTACK1;
		}
		if ( HasCondition( COND_CAN_RANGE_ATTACK2 ) )
		{
			return SCHED_STRIDER_RANGE_ATTACK2;
		}
		break;
	}

	if ( pTrackTarget )
		return SCHED_TARGET_CHASE;
	return SCHED_IDLE_STAND;
}

int CNPC_Strider::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_STRIDER_RANGE_ATTACK1;
	case SCHED_RANGE_ATTACK2:
		return SCHED_STRIDER_RANGE_ATTACK2;
	case SCHED_MELEE_ATTACK1:
		return SCHED_STRIDER_STOMPL;
	case SCHED_MELEE_ATTACK2:
		return SCHED_STRIDER_STOMPR;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


float CNPC_Strider::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_90_LEFT:
	case ACT_90_RIGHT:
		return 10;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 10;

	case ACT_WALK:
		return 10;

	default:
		return 10;		// should be zero, but this makes it easy to see when he's turning with default AI code
		break;
	}
}


int CNPC_Strider::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// don't take damage from my own weapons!!!
	if ( info.GetInflictor() && info.GetInflictor()->GetOwnerEntity() == this )
		return 0;

	if ( m_isHatchOpen && m_hPlayer.Get() && info.GetAttacker() == m_hPlayer.Get() )
	{
		return BaseClass::OnTakeDamage_Alive( info );
	}

	if ( !(info.GetDamageType() & DMG_BLAST) )
		return 0;

	if ( !info.GetInflictor() )
		return 0;

	if ( info.GetMaxDamage() < 50 )
		return 0;

	// UNDONE: Take this kind of damage (large explosion?)
	return 0;
}

Vector CNPC_Strider::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	if ( m_BodyTargetBone != -1 )
	{
		Vector position;
		QAngle angles;
		GetBonePosition( m_BodyTargetBone, position, angles );
		return position;
	}
	return BaseClass::BodyTarget( posSrc, bNoisy );
}

void CNPC_Strider::FootFX( const Vector &origin )
{
	trace_t tr;
	AI_TraceLine( origin, origin - Vector(0,0,100), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	float yaw = random->RandomInt(0,120);
	for ( int i = 0; i < 3; i++ )
	{
		Vector dir = UTIL_YawToVector( yaw + i*120 ) * 10;
		dir.z = 0.25;
		g_pEffects->Dust( tr.endpos, dir, 12, 50 );
	}
	UTIL_ScreenShake( tr.endpos, 20.0, 1.0, 0.5, 1000, SHAKE_START, false );
	
	if ( npc_strider_shake_ropes_radius.GetInt() )
	{
		CRopeKeyframe::ShakeRopes( tr.endpos, npc_strider_shake_ropes_radius.GetFloat(), npc_strider_shake_ropes_magnitude.GetFloat() );
	}

	//
	// My feet are scary things! NOTE: We might want to make danger sounds as the feet move
	// through the air. Then soldiers could run from the feet, which would look cool.
	//
	CSoundEnt::InsertSound( SOUND_DANGER, tr.endpos, 512, 1.0f );
}

Vector CNPC_Strider::LeftFootHit()
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "left foot", footPosition, angles );
	CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
	EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition );

	FootFX( footPosition );

	return footPosition;
}

Vector CNPC_Strider::RightFootHit()
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "right foot", footPosition, angles );
	CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
	EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition );
	FootFX( footPosition );

	return footPosition;
}


Vector CNPC_Strider::BackFootHit()
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "back foot", footPosition, angles );
	CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
	EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition );
	FootFX( footPosition );

	return footPosition;
}


bool CNPC_Strider::HasScriptedEnemy() const
{
	return ( m_hCannonTarget.Get() != NULL );
}


bool CNPC_Strider::IsScriptedEnemy( CBaseEntity *pTarget ) const
{
	CBaseEntity *pCannonTarget = m_hCannonTarget;
	if ( pCannonTarget && pCannonTarget == pTarget )
	{
		return true;
	}

	return false;
}


Disposition_t CNPC_Strider::IRelationType( CBaseEntity *pTarget )
{
	if ( IsScriptedEnemy( pTarget ) )
		return D_HT;

	return BaseClass::IRelationType( pTarget );
}

bool CNPC_Strider::IsValidEnemy( CBaseEntity *pTarget )
{
	if ( HasScriptedEnemy() )
	{
		return IsScriptedEnemy(pTarget);
	}

	CBaseCombatCharacter *pEnemy = ToBaseCombatCharacter( pTarget );
	if ( pEnemy )
	{
		// only attack a bullseye if explicitly told to
		if ( pEnemy->Classify() == CLASS_BULLSEYE )
			return false;

		// if the player is riding, he can't be an enemy
		if ( pEnemy == m_hPlayer.Get() )
			return false;

		// if they have a weapon, fry 'em
		// otherwise, if they are a non-weapon using NPC, then fry them too!
		// UNDONE: Check for innate attacks?
		if ( pEnemy->GetActiveWeapon() ||
			(pEnemy->MyNPCPointer() && !(pEnemy->MyNPCPointer()->CapabilitiesGet() & bits_CAP_USE_WEAPONS) ) )
		{
			if ( !pEnemy->IsPlayer() || HasMemory( bits_MEMORY_PROVOKED ) )
			{
				return true;
			}
		}
		// player or human with no weapon, ignore
		return false;
	}
	return BaseClass::IsValidEnemy(pTarget);
}


void CNPC_Strider::OnStateChange( NPC_STATE oldState, NPC_STATE newState )
{
	if ( oldState == NPC_STATE_SCRIPT )
	{
		m_miniGun.Enable( this, true );
	}
	else if ( newState == NPC_STATE_SCRIPT )
	{
		m_miniGun.Enable( this, false );
	}
}


void CNPC_Strider::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();
	if (m_NPCState != NPC_STATE_SCRIPT)
	{
		SetCustomInterruptCondition( COND_STRIDER_DO_FLICK );
	}
}


void CNPC_Strider::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	switch ( iTracerType )
	{
	case TRACER_LINE:
		{
			float flTracerDist;
			Vector vecDir;
			Vector vecEndPos;

			vecDir = tr.endpos - vecTracerSrc;

			flTracerDist = VectorNormalize( vecDir );

			UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 2000 ), true, "StriderTracer" );
		}
		break;

	default:
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		break;
	}
}


static Vector GetAttachmentPositionInSpaceOfBone( studiohdr_t *pStudioHdr, const char *pAttachmentName, int outputBoneIndex )
{
	int attachment = Studio_FindAttachment( pStudioHdr, pAttachmentName );

	Vector localAttach;
	mstudioattachment_t *pAttachment = pStudioHdr->pAttachment(attachment);
	MatrixGetColumn( pAttachment->local, 3, localAttach );

	matrix3x4_t inputToOutputBone;
	Studio_CalcBoneToBoneTransform( pStudioHdr, pAttachment->bone, outputBoneIndex, inputToOutputBone );
	
	Vector out;
	VectorTransform( localAttach, inputToOutputBone, out );

	return out;
}


void CNPC_Strider::StompHit( int followerBoneIndex )
{
	studiohdr_t *pStudioHdr = GetModelPtr();
	physfollower_t *bone = m_BoneFollowerManager.GetBoneFollower( followerBoneIndex );

	const char *pBoneNames[] = {"left skewer", "right skewer" };
	Vector localHit = GetAttachmentPositionInSpaceOfBone( pStudioHdr, pBoneNames[followerBoneIndex], bone->boneIndex );
	IPhysicsObject *pLegPhys = bone->hFollower->VPhysicsGetObject();

	// now transform into the worldspace of the current position of the leg's physics
	matrix3x4_t legToWorld;
	pLegPhys->GetPositionMatrix( legToWorld );
	Vector hitPosition;
	VectorTransform( localHit, legToWorld, hitPosition );

	//NDebugOverlay::Box( hitPosition, Vector(-16,-16,-16), Vector(16,16,16), 0, 255, 0, 255, 1.0 );

	CAI_BaseNPC *pNPC = GetEnemy() ? GetEnemy()->MyNPCPointer() : NULL;
	if ( pNPC )
	{
		Vector delta = pNPC->GetAbsOrigin() - hitPosition;
		if ( delta.Length() <= STRIDER_STOMP_RANGE )
		{
			// DVS E3 HACK: Assume we stab our victim midway between their eyes and their center.
			Vector vecStabPos = ( pNPC->WorldSpaceCenter() + pNPC->EyePosition() ) * 0.5f;
			hitPosition = pNPC->GetAbsOrigin();

			Vector footPosition;
			QAngle angles;
			GetAttachment( "left foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Skewer" );
			EmitSound( filter, 0, "NPC_Strider.Skewer", &hitPosition );

			CTakeDamageInfo damageInfo( this, this, 500, DMG_CRUSH );
			Vector forward;
			pNPC->GetVectors( &forward, NULL, NULL );
			damageInfo.SetDamagePosition( hitPosition );
			damageInfo.SetDamageForce( -50 * 300 * forward );
			pNPC->TakeDamage( damageInfo );

			if ( !pNPC->IsAlive() )
			{
				Vector vecBloodDelta = footPosition - vecStabPos;
				vecBloodDelta.z = 0; // effect looks better 
				VectorNormalize( vecBloodDelta );
				UTIL_BloodSpray( vecStabPos + vecBloodDelta * 4, vecBloodDelta, BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_ALL );
				UTIL_BloodSpray( vecStabPos + vecBloodDelta * 4, vecBloodDelta, BLOOD_COLOR_RED, 11, FX_BLOODSPRAY_DROPS );
				CBaseEntity *pRagdoll = CreateServerRagdollAttached( pNPC, vec3_origin, -1, COLLISION_GROUP_DEBRIS, pLegPhys, this, bone->boneIndex, vecStabPos, -1, localHit );
				if ( pRagdoll )
				{
					m_hRagdoll = pRagdoll;
					m_ragdollTime = gpGlobals->curtime + 10;
					UTIL_Remove( pNPC );
				}
			}
		}
	}
}


extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

void CNPC_Strider::HandleAnimEvent( animevent_t *pEvent )
{
	Vector footPosition;
	QAngle angles;

	switch( pEvent->event )
	{
	case STRIDER_AE_SHOOTCANNON:
		{
			m_nextShootTime = gpGlobals->curtime + 5;
			trace_t tr;
			matrix3x4_t gunMatrix;
			GetAttachment( g_CannonAttachment, gunMatrix );

			Vector vecShootPos;
			MatrixGetColumn( gunMatrix, 3, vecShootPos );
			Vector vecShootDir;
			MatrixGetColumn( gunMatrix, 0, vecShootDir );

			AI_TraceLine( vecShootPos, vecShootPos + vecShootDir * 1024, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			m_blastHit = tr.endpos;
			m_blastHit += tr.plane.normal * 16;
			m_blastNormal = tr.plane.normal;

			// tell the client side effect to complete
			CEntityMessageFilter filter( this, "CNPC_Strider" );
			filter.MakeReliable();

			MessageBegin( filter, 0 );
				WRITE_BYTE( STRIDER_MSG_BIG_SHOT );
				WRITE_VEC3COORD( tr.endpos );
			MessageEnd();
			CPASAttenuationFilter filter2( this, "NPC_Strider.Shoot" );
			EmitSound( filter2, entindex(), "NPC_Strider.Shoot");
			
			// Clear this guy now that we've shot him
			m_hCannonTarget = NULL;
		}
		break;
	case STRIDER_AE_CANNONHIT:
		CreateConcussiveBlast( m_blastHit, m_blastNormal, this, 2.5 );
		break;

	case STRIDER_AE_SHOOTMINIGUN:
		ShootMinigun( NULL, 0 );
		break;

	case STRIDER_AE_STOMPHITL:
		StompHit( 0 );
		break;
	case STRIDER_AE_STOMPHITR:
		StompHit( 1 );
		break;
	case STRIDER_AE_FLICKL:
	case STRIDER_AE_FLICKR:
		{
			CBaseEntity *pRagdoll = m_hRagdoll;
			if ( pRagdoll )
			{
				CPASAttenuationFilter filter( pRagdoll, "NPC_Strider.RagdollDetach" );
				EmitSound( filter, pRagdoll->entindex(), "NPC_Strider.RagdollDetach" );
				DetachAttachedRagdoll( pRagdoll );
			}
			m_hRagdoll = NULL;
		}
		break;

	case STRIDER_AE_FOOTSTEP_LEFT:
	case STRIDER_AE_FOOTSTEP_LEFTM:
	case STRIDER_AE_FOOTSTEP_LEFTL:
		LeftFootHit();
		break;
	case STRIDER_AE_FOOTSTEP_RIGHT:
	case STRIDER_AE_FOOTSTEP_RIGHTM:
	case STRIDER_AE_FOOTSTEP_RIGHTL:
		RightFootHit();
		break;
	case STRIDER_AE_FOOTSTEP_BACK:
	case STRIDER_AE_FOOTSTEP_BACKM:
	case STRIDER_AE_FOOTSTEP_BACKL:
		BackFootHit();
		break;
	case STRIDER_AE_WHOOSH_LEFT:
		{
			GetAttachment( "left foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_WHOOSH_RIGHT:
		{
			GetAttachment( "right foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_WHOOSH_BACK:
		{
			GetAttachment( "back foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_CREAK_LEFT:
	case STRIDER_AE_CREAK_BACK:
	case STRIDER_AE_CREAK_RIGHT:
		{
			EmitSound( "NPC_Strider.Creak" );
		}
		break;
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

void CNPC_Strider::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	if ( !HasMemory( bits_MEMORY_PROVOKED ) )
	{
		// if the player threw this in the last 1 seconds
		CBasePlayer *pPlayer = WasLaunchedByPlayer( pEvent->pObjects[!index], gpGlobals->curtime, 1 );
		if ( pPlayer )
		{
			GetEnemies()->ClearMemory( pPlayer );
			Remember( bits_MEMORY_PROVOKED );
			SetCondition( COND_LIGHT_DAMAGE );
		}
	}

	BaseClass::VPhysicsShadowCollision( index, pEvent );
}

void CNPC_Strider::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	bool ricochetBullets = true;

	if ( m_hPlayer.Get() && info.GetAttacker() == m_hPlayer.Get() )
	{
		ricochetBullets = m_isHatchOpen ? false : true;
	}
	else if ( info.GetAttacker()->IsPlayer() )
	{
		if ( !HasMemory( bits_MEMORY_PROVOKED ) )
		{
			GetEnemies()->ClearMemory( info.GetAttacker() );
			Remember( bits_MEMORY_PROVOKED );
			SetCondition( COND_LIGHT_DAMAGE );
		}
	}

	if ( (info.GetDamageType() & DMG_BULLET) && ricochetBullets )
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
		info.SetDamage( 0.01 );
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_strider, CNPC_Strider )

	DECLARE_TASK( TASK_STRIDER_AIM )
	DECLARE_TASK( TASK_STRIDER_DODGE )
	DECLARE_TASK( TASK_STRIDER_STOMP )
	DECLARE_TASK( TASK_STRIDER_BREAKDOWN )

	DECLARE_ACTIVITY( ACT_STRIDER_LOOKL )
	DECLARE_ACTIVITY( ACT_STRIDER_LOOKR )
	DECLARE_ACTIVITY( ACT_STRIDER_DEPLOYRA1 )
	DECLARE_ACTIVITY( ACT_STRIDER_AIMRA1 )
	DECLARE_ACTIVITY( ACT_STRIDER_FINISHRA1 )
	DECLARE_ACTIVITY( ACT_STRIDER_DODGER )
	DECLARE_ACTIVITY( ACT_STRIDER_DODGEL )
	DECLARE_ACTIVITY( ACT_STRIDER_STOMPL )
	DECLARE_ACTIVITY( ACT_STRIDER_STOMPR )
	DECLARE_ACTIVITY( ACT_STRIDER_FLICKL )
	DECLARE_ACTIVITY( ACT_STRIDER_FLICKR )
	DECLARE_ACTIVITY( ACT_STRIDER_SLEEP )

	DECLARE_CONDITION( COND_STRIDER_DO_FLICK )
	DECLARE_CONDITION( COND_TRACK_PATH_GO )

	//=========================================================
	// Attack (Deploy/shoot/finish)
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_FACE_ENEMY			0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_DEPLOYRA1"
		"		TASK_STRIDER_AIM		1.25"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_RANGE_ATTACK1"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_FINISHRA1"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Attack (Deploy/shoot/finish)
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_RANGE_ATTACK2,

		"	Tasks"
		"		TASK_FACE_ENEMY			0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_DEPLOYRA1"
		"		TASK_RANGE_ATTACK2		0"
		"		TASK_WAIT				5" // let the immolator work its magic
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_FINISHRA1"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Dodge Incoming missile
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_DODGE,

		"	Tasks"
		"		TASK_STRIDER_DODGE		0"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Break down and die
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_DIE,

		"	Tasks"
		"		TASK_STRIDER_BREAKDOWN		0"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// Stomp on an enemy
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_STOMPL,

		"	Tasks"
		"		TASK_STRIDER_STOMP		0"
		"	"
		"	Interrupts"
	);		

	// Stomp on an enemy
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_STOMPR,

		"	Tasks"
		"		TASK_STRIDER_STOMP		1"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_FLICKL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_FLICKL"
		"	"
		"	Interrupts"
	)
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_FLICKR,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_FLICKR"
		"	"
		"	Interrupts"
	)
AI_END_CUSTOM_NPC()



// Strider vehicle stuff:
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Strider::EnterVehicle( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Remove any player who may be in the vehicle at the moment
	ExitVehicle(VEHICLE_DRIVER);

	m_hPlayer = pPlayer;

	m_savedViewOffset = pPlayer->GetViewOffset();
	pPlayer->SetViewOffset( vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Strider::ExitVehicle( int iRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	pPlayer->SetViewOffset( m_savedViewOffset );
}

//-----------------------------------------------------------------------------
// Purpose: Crane rotates around with +left and +right, and extends/retracts 
//			the cable with +forward and +back.
//-----------------------------------------------------------------------------
void CNPC_Strider::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// process input
#if 0
	if ( ucmd->buttons & IN_MOVELEFT )
	{
	}
	else if ( ucmd->buttons & IN_MOVERIGHT )
	{
	}
#endif
}

//========================================================================================================================================
// CRANE VEHICLE SERVER VEHICLE
//========================================================================================================================================
CNPC_Strider *CStriderServerVehicle::GetStrider( void )
{
	return (CNPC_Strider*)GetDrivableVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStriderServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = GetPassenger( nRole );
	Assert( pPlayer );
	*pAbsAngles = pPlayer->EyeAngles();
	int eyeAttachmentIndex = GetStrider()->LookupAttachment("vehicle_driver_eyes");
	QAngle tmp;
	GetStrider()->GetAttachment( eyeAttachmentIndex, *pAbsOrigin, tmp );
}

//-----------------------------------------------------------------------------
// Purpose: Where does this passenger exit the vehicle?
//-----------------------------------------------------------------------------
void CStriderServerVehicle::GetPassengerExitPoint( int nRole, Vector *pExitPoint, QAngle *pAngles )
{ 
	Assert( nRole == VEHICLE_DRIVER ); 

	// Get our attachment point
	Vector vehicleExitOrigin;
	QAngle vehicleExitAngles;
	GetStrider()->GetAttachment( "vehicle_driver_exit", vehicleExitOrigin, vehicleExitAngles );
	*pAngles = vehicleExitAngles;

	// Make sure it's clear
	trace_t tr;
	AI_TraceHull( vehicleExitOrigin + Vector(0,0,64), vehicleExitOrigin, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID, GetStrider(), COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid )
	{
		*pExitPoint = tr.endpos;
		return;
	}

	// Worst case, jump out on top
	*pExitPoint = GetStrider()->GetAbsOrigin();
	pExitPoint->z += GetStrider()->WorldAlignMaxs()[2] + 50.0f;
	
	// pop out behind this guy a bit
	Vector forward;
	GetStrider()->GetVectors( &forward, NULL, NULL );
	*pExitPoint -= forward * 64;
}
