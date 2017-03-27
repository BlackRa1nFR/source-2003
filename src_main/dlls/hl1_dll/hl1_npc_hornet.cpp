#include	"cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Memory.h"
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
#include    "te.h"
#include    "hl1_npc_hornet.h"

int iHornetTrail;
int iHornetPuff;

LINK_ENTITY_TO_CLASS( hornet, CNPC_Hornet );

extern ConVar sk_npc_dmg_hornet;
extern ConVar sk_plr_dmg_hornet;

BEGIN_DATADESC( CNPC_Hornet )
	DEFINE_FIELD( CNPC_Hornet, m_flStopAttack, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Hornet, m_iHornetType, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Hornet, m_flFlySpeed, FIELD_FLOAT ),
	DEFINE_ENTITYFUNC( CNPC_Hornet, DieTouch ),
	DEFINE_THINKFUNC( CNPC_Hornet, StartDart ),
	DEFINE_THINKFUNC( CNPC_Hornet, StartTrack ),
	DEFINE_ENTITYFUNC( CNPC_Hornet, DartTouch ),
	DEFINE_ENTITYFUNC( CNPC_Hornet, TrackTouch ),
	DEFINE_THINKFUNC( CNPC_Hornet, TrackTarget ),
END_DATADESC()

//=========================================================
//=========================================================
void CNPC_Hornet::Spawn( void )
{
	Precache();

	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	m_takedamage	= DAMAGE_YES;
	AddFlag( FL_NPC );
	m_iHealth		= 1;// weak!
	
	if ( g_pGameRules->IsMultiplayer() )
	{
		// hornets don't live as long in multiplayer
		m_flStopAttack = gpGlobals->curtime + 3.5;
	}
	else
	{
		m_flStopAttack	= gpGlobals->curtime + 5.0;
	}

	m_flFieldOfView = 0.9; // +- 25 degrees

	if ( random->RandomInt ( 1, 5 ) <= 2 )
	{
		m_iHornetType = HORNET_TYPE_RED;
		m_flFlySpeed = HORNET_RED_SPEED;
	}
	else
	{
		m_iHornetType = HORNET_TYPE_ORANGE;
		m_flFlySpeed = HORNET_ORANGE_SPEED;
	}

	SetModel( "models/hornet.mdl" );
	UTIL_SetSize( this, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	SetTouch( DieTouch );
	SetThink( StartTrack );

	if ( GetOwnerEntity() && (GetOwnerEntity()->GetFlags() & FL_CLIENT) )
	{
		m_flDamage = sk_plr_dmg_hornet.GetFloat();
	}
	else
	{
		// no real owner, or owner isn't a client. 
		m_flDamage = sk_npc_dmg_hornet.GetFloat();
	}
	
	SetNextThink( gpGlobals->curtime + 0.1f );
	ResetSequenceInfo();
}


void CNPC_Hornet::Precache()
{
	engine->PrecacheModel("models/hornet.mdl");

	enginesound->PrecacheSound( "hornet/ag_buzz1.wav" );
	enginesound->PrecacheSound( "hornet/ag_buzz2.wav" );
	enginesound->PrecacheSound( "hornet/ag_buzz3.wav" );

	enginesound->PrecacheSound( "hornet/ag_hornethit1.wav" );
	enginesound->PrecacheSound( "hornet/ag_hornethit2.wav" );
	enginesound->PrecacheSound( "hornet/ag_hornethit3.wav" );

	iHornetPuff = engine->PrecacheModel( "sprites/muz1.vmt" );
	iHornetTrail = engine->PrecacheModel("sprites/laserbeam.vmt");
}	

//=========================================================
// hornets will never get mad at each other, no matter who the owner is.
//=========================================================
Disposition_t CNPC_Hornet::IRelationType( CBaseEntity *pTarget )
{
	if ( pTarget->GetModelIndex() == GetModelIndex() )
	{
		return D_NU;
	}

	return BaseClass::IRelationType( pTarget );
}

//=========================================================
// ID's Hornet as their owner
//=========================================================
Class_T CNPC_Hornet::Classify ( void )
{
	if ( GetOwnerEntity() && (GetOwnerEntity()->GetFlags() & FL_CLIENT) )
	{
		return CLASS_PLAYER_BIOWEAPON;
	}

	return	CLASS_ALIEN_BIOWEAPON;
}

//=========================================================
// StartDart - starts a hornet out just flying straight.
//=========================================================
void CNPC_Hornet::StartDart ( void )
{
	IgniteTrail();

	SetTouch( DartTouch );

	SetThink( SUB_Remove );
	SetNextThink( gpGlobals->curtime + 4 );
}


void CNPC_Hornet::DieTouch ( CBaseEntity *pOther )
{
	if ( !pOther || !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
	{
		return;
	}

	CPASAttenuationFilter filter( this );
	switch (random->RandomInt(0,2))
	{// buzz when you plug someone
		case 0:	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "hornet/ag_hornethit1.wav", 1, ATTN_NORM );	break;
		case 1:	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "hornet/ag_hornethit2.wav", 1, ATTN_NORM );	break;
		case 2:	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "hornet/ag_hornethit3.wav", 1, ATTN_NORM );	break;
	}
			
	CTakeDamageInfo info( this, GetOwnerEntity(), m_flDamage, DMG_BULLET );
	CalculateBulletDamageForce( &info, GetAmmoDef()->Index("Hornet"), GetAbsVelocity(), GetAbsOrigin() );
	pOther->TakeDamage( info );

	m_takedamage	= DAMAGE_NO;

	m_fEffects		|= EF_NODRAW;

	AddSolidFlags( FSOLID_NOT_SOLID );// intangible
	UTIL_Relink( this );

	UTIL_Remove( this );
	SetTouch( NULL );
}


//=========================================================
// StartTrack - starts a hornet out tracking its target
//=========================================================
void CNPC_Hornet :: StartTrack ( void )
{
	IgniteTrail();

	SetTouch( TrackTouch );
	SetThink( TrackTarget );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void TE_BeamFollow( IRecipientFilter& filter, float delay,
	int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth, 
	float fadeLength,float r, float g, float b, float a );

void CNPC_Hornet::IgniteTrail( void )
{
	Vector vColor;

	if ( m_iHornetType == HORNET_TYPE_RED )
		 vColor = Vector ( 179, 39, 14 );
	else
		 vColor = Vector ( 255, 128, 0 );

	CBroadcastRecipientFilter filter;
	TE_BeamFollow( filter, 0.0,
		entindex(),
		iHornetTrail,
		0,
		1,
		2,
		0.5,
		0.5,
		vColor.x,
		vColor.y,
		vColor.z,
		128 );
}

//=========================================================
// Tracking Hornet hit something
//=========================================================
void CNPC_Hornet::TrackTouch ( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
	{
		return;
	}

	if ( pOther == GetOwnerEntity() || pOther->GetModelIndex() == GetModelIndex() )
	{// bumped into the guy that shot it.
		//SetSolid( SOLID_NOT );
		return;
	}

	int nRelationship = IRelationType( pOther );
	if ( (nRelationship == D_FR || nRelationship == D_NU || nRelationship == D_LI) )
	{
		// hit something we don't want to hurt, so turn around.
		Vector vecVel = GetAbsVelocity();

		VectorNormalize( vecVel );

		vecVel.x *= -1;
		vecVel.y *= -1;

		SetAbsOrigin( GetAbsOrigin() + vecVel * 4 ); // bounce the hornet off a bit.
		SetAbsVelocity( vecVel * m_flFlySpeed );

		return;
	}

	DieTouch( pOther );
}

void CNPC_Hornet::DartTouch( CBaseEntity *pOther )
{
	DieTouch( pOther );
}

//=========================================================
// Hornet is flying, gently tracking target
//=========================================================
void CNPC_Hornet::TrackTarget ( void )
{
	Vector	vecFlightDir;
	Vector	vecDirToEnemy;
	float	flDelta;
	Vector  vecEnemyLKP;

	StudioFrameAdvance( );

	if (gpGlobals->curtime > m_flStopAttack)
	{
		SetTouch( NULL );
		SetThink( SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	// UNDONE: The player pointer should come back after returning from another level
	if ( GetEnemy() == NULL )
	{// enemy is dead.
		GetSenses()->Look( 512 );
		SetEnemy( BestEnemy() );
	}
	
	if ( GetEnemy() != NULL && FVisible( GetEnemy() ))
	{
		vecEnemyLKP = GetEnemy()->BodyTarget( GetAbsOrigin() );
	}
	else
	{
		vecEnemyLKP = GetEnemyLKP() + GetAbsVelocity() * m_flFlySpeed * 0.1;
	}

	vecDirToEnemy = vecEnemyLKP - GetAbsOrigin();
	VectorNormalize( vecDirToEnemy );

	if ( GetAbsVelocity().Length() < 0.1 )
		vecFlightDir = vecDirToEnemy;
	else 
	{
		vecFlightDir = GetAbsVelocity();
		VectorNormalize( vecFlightDir );
	}

	SetAbsVelocity( vecFlightDir + vecDirToEnemy );

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	flDelta = DotProduct ( vecFlightDir, vecDirToEnemy );
	
	if ( flDelta < 0.5 )
	{// hafta turn wide again. play sound
		CPASAttenuationFilter filter( this );
		switch (random->RandomInt(0,2))
		{
		case 0:	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );	break;
		case 1:	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );	break;
		case 2:	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );	break;
		}
	}

	if ( flDelta <= 0 && m_iHornetType == HORNET_TYPE_RED )
	{// no flying backwards, but we don't want to invert this, cause we'd go fast when we have to turn REAL far.
		flDelta = 0.25;
	}

	Vector vecVel = GetAbsVelocity();
	VectorNormalize( vecVel );

	if ( GetOwnerEntity() && (GetOwnerEntity()->GetFlags() & FL_NPC) )
	{
		// random pattern only applies to hornets fired by monsters, not players. 

		vecVel.x += random->RandomFloat ( -0.10, 0.10 );// scramble the flight dir a bit.
		vecVel.y += random->RandomFloat ( -0.10, 0.10 );
		vecVel.z += random->RandomFloat ( -0.10, 0.10 );
	}

	SetAbsVelocity( vecVel );
	
	switch ( m_iHornetType )
	{
		case HORNET_TYPE_RED:
			SetAbsVelocity( GetAbsVelocity() * ( m_flFlySpeed * flDelta ) );// scale the dir by the ( speed * width of turn )
			SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1, 0.3 ) );
			break;
		case HORNET_TYPE_ORANGE:
			SetAbsVelocity( GetAbsVelocity() * m_flFlySpeed );// do not have to slow down to turn.
			SetNextThink( gpGlobals->curtime + 0.1f );// fixed think time
			break;
	}

	QAngle angNewAngles;
	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );
	
	SetSolid( SOLID_BBOX );

	// if hornet is close to the enemy, jet in a straight line for a half second.
	// (only in the single player game)
	if ( GetEnemy() != NULL && !g_pGameRules->IsMultiplayer() )
	{
		if ( flDelta >= 0.4 && ( GetAbsOrigin() - vecEnemyLKP ).Length() <= 300 )
		{
			CPVSFilter filter( GetAbsOrigin() );
			te->Sprite( filter, 0.0,
				&GetAbsOrigin(), // pos
				iHornetPuff,	// model
				0.2,				//size
				128				// brightness
			);

			CPASAttenuationFilter filter2( this );
			switch ( random->RandomInt(0,2))
			{
			case 0:	enginesound->EmitSound( filter2, entindex(), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );	break;
			case 1:	enginesound->EmitSound( filter2, entindex(), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );	break;
			case 2:	enginesound->EmitSound( filter2, entindex(), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );	break;
			}
			SetAbsVelocity( GetAbsVelocity() * 2 );
			SetNextThink( gpGlobals->curtime + 1.0f );
			// don't attack again
			m_flStopAttack = gpGlobals->curtime;
		}
	}
}