//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a grab bag of visual effects entities.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "gib.h"
#include "customentity.h"
#include "beam_shared.h"
#include "decals.h"
#include "func_break.h"
#include "EntityFlame.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "model_types.h"
#include "player.h"
#include "physics.h"
#include "baseparticleentity.h"
#include "ndebugoverlay.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "env_wind_shared.h"
#include "filesystem.h"
#include "engine/IEngineSound.h"
#include "fire.h"
#include "te_effect_dispatch.h"
#include "Sprite.h"

#define	SF_GIBSHOOTER_REPEATABLE	1 // allows a gibshooter to be refired

#define SF_FUNNEL_REVERSE			1 // funnel effect repels particles instead of attracting them.

// UNDONE: This should be client-side and not use TempEnts
class CBubbling : public CBaseEntity
{
public:
	DECLARE_CLASS( CBubbling, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	
	void	FizzThink( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Input handlers.
	void	InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();

	int		m_density;
	int		m_frequency;
	int		m_bubbleModel;
	int		m_state;
};

LINK_ENTITY_TO_CLASS( env_bubbles, CBubbling );

BEGIN_DATADESC( CBubbling )

	DEFINE_KEYFIELD( CBubbling, m_flSpeed, FIELD_FLOAT, "current" ),
	DEFINE_KEYFIELD( CBubbling, m_density, FIELD_INTEGER, "density" ),
	DEFINE_KEYFIELD( CBubbling, m_frequency, FIELD_INTEGER, "frequency" ),

	DEFINE_FIELD( CBubbling, m_state, FIELD_INTEGER ),
	// Let spawn restore this!
	//	DEFINE_FIELD( CBubbling, m_bubbleModel, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( CBubbling, FizzThink ),

	DEFINE_INPUTFUNC( CBubbling, FIELD_VOID, "Activate", InputActivate ),
END_DATADESC()



#define SF_BUBBLES_STARTOFF		0x0001

void CBubbling::Spawn( void )
{
	Precache( );
	SetModel( STRING( GetModelName() ) );		// Set size

	SetSolid( SOLID_NONE );						// Remove model & collisions
	SetRenderColorA( 0 );						// The engine won't draw this model if this is set to 0 and blending is on
	m_nRenderMode = kRenderTransTexture;
	Relink();

	// HACKHACK!!! - Speed in rendercolor
	int iSpeed = abs( m_flSpeed );
	SetRenderColorR( iSpeed >> 8 );
	SetRenderColorG( iSpeed & 255 );
	SetRenderColorB( (m_flSpeed < 0) ? 1 : 0 );

	if ( !HasSpawnFlags(SF_BUBBLES_STARTOFF) )
	{
		SetThink( &CBubbling::FizzThink );
		SetNextThink( gpGlobals->curtime + 2.0 );
		m_state = 1;
	}
	else 
		m_state = 0;
}

void CBubbling::Precache( void )
{
	m_bubbleModel = engine->PrecacheModel("sprites/bubble.vmt");			// Precache bubble sprite
}


void CBubbling::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType, m_state ) )
		m_state = !m_state;

	if ( m_state )
	{
		SetThink( &CBubbling::FizzThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		SetThink( NULL );
		SetNextThink( TICK_NEVER_THINK );
	}
}


void CBubbling::InputActivate( inputdata_t &inputdata )
{
	Use( inputdata.pActivator, inputdata.pCaller, USE_ON, 0 );
}


void CBubbling::FizzThink( void )
{
	Vector center = WorldSpaceCenter();
	CPASFilter filter( center );
	te->Fizz( filter, 0.0, this, m_bubbleModel, m_density );

	if ( m_frequency > 19 )
		SetNextThink( gpGlobals->curtime + 0.5f );
	else
		SetNextThink( gpGlobals->curtime + 2.5 - (0.1 * m_frequency) );
}


// ENV_TRACER
// Fakes a tracer
class CEnvTracer : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvTracer, CPointEntity );

	void Spawn( void );
	void TracerThink( void );
	void Activate( void );

	DECLARE_DATADESC();

	Vector m_vecEnd;
	float  m_flDelay;
};

LINK_ENTITY_TO_CLASS( env_tracer, CEnvTracer );

BEGIN_DATADESC( CEnvTracer )

	DEFINE_KEYFIELD( CEnvTracer, m_flDelay, FIELD_FLOAT, "delay" ),

	DEFINE_FIELD( CEnvTracer, m_vecEnd, FIELD_POSITION_VECTOR ),

	// Function Pointers
	DEFINE_FUNCTION( CEnvTracer, TracerThink ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called after keyvalues are parsed.
//-----------------------------------------------------------------------------
void CEnvTracer::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	if (!m_flDelay)
		m_flDelay = 1;
}


//-----------------------------------------------------------------------------
// Purpose: Called after all the entities have been loaded.
//-----------------------------------------------------------------------------
void CEnvTracer::Activate( void )
{
	BaseClass::Activate();

	CBaseEntity *pEnd = gEntList.FindEntityByName( NULL, m_target, NULL );
	if (pEnd != NULL)
	{
		m_vecEnd = pEnd->GetLocalOrigin();
		SetThink( &CEnvTracer::TracerThink );
		SetNextThink( gpGlobals->curtime + m_flDelay );
	}
	else
	{
		Msg( "env_tracer: unknown entity \"%s\"\n", STRING(m_target) );
	}
}

// Think
void CEnvTracer::TracerThink( void )
{
	UTIL_Tracer( GetAbsOrigin(), m_vecEnd );

	SetNextThink( gpGlobals->curtime + m_flDelay );
}


//#################################################################################
//  >> CGibShooter
//#################################################################################
enum GibSimulation_t
{
	GIB_SIMULATE_POINT,
	GIB_SIMULATE_PHYSICS,
	GIB_SIMULATE_RAGDOLL,
};

class CGibShooter : public CBaseEntity
{
public:
	DECLARE_CLASS( CGibShooter, CBaseEntity );

	void Spawn( void );
	void Precache( void );
	void ShootThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void InitPointGib( CGib *pGib, const Vector &vecShootDir );

	virtual CGib *CreateGib( void );

	DECLARE_DATADESC();

	int		m_iGibs;
	int		m_iGibCapacity;
	int		m_iGibMaterial;
	int		m_iGibModelIndex;
	float	m_flGibVelocity;
	QAngle	m_angGibRotation;
	float	m_flVariance;
	float	m_flGibLife;
	int		m_nSimulationType;
	int		m_nMaxGibModelFrame;
	float	m_flDelay;

	bool	m_bIsSprite;

	// ----------------
	//	Inputs
	// ----------------
	void InputShoot( inputdata_t &inputdata );
};

BEGIN_DATADESC( CGibShooter )

	DEFINE_KEYFIELD( CGibShooter, m_iGibs, FIELD_INTEGER, "m_iGibs" ),
	DEFINE_KEYFIELD( CGibShooter, m_flGibVelocity, FIELD_FLOAT, "m_flVelocity" ),
	DEFINE_KEYFIELD( CGibShooter, m_flVariance, FIELD_FLOAT, "m_flVariance" ),
	DEFINE_KEYFIELD( CGibShooter, m_flGibLife, FIELD_FLOAT, "m_flGibLife" ),
	DEFINE_KEYFIELD( CGibShooter, m_nSimulationType, FIELD_INTEGER, "Simulation" ),
	DEFINE_KEYFIELD( CGibShooter, m_flDelay, FIELD_FLOAT, "delay" ),
	DEFINE_KEYFIELD( CGibShooter, m_angGibRotation, FIELD_VECTOR, "gibangles" ),
	DEFINE_FIELD( CGibShooter, m_bIsSprite, FIELD_BOOLEAN ),

	DEFINE_FIELD( CGibShooter, m_iGibCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_nMaxGibModelFrame, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( CGibShooter, FIELD_VOID,	"Shoot", InputShoot ),

	// Function Pointers
	DEFINE_FUNCTION( CGibShooter, ShootThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( gibshooter, CGibShooter );


void CGibShooter::Precache ( void )
{
	if ( g_Language == LANGUAGE_GERMAN )
	{
		m_iGibModelIndex = engine->PrecacheModel ("models/germanygibs.mdl");
	}
	else
	{
		m_iGibModelIndex = engine->PrecacheModel ("models/gibs/hgibs.mdl");
	}
}


void CGibShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGibShooter::ShootThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for shooting gibs.
//-----------------------------------------------------------------------------
void CGibShooter::InputShoot( inputdata_t &inputdata )
{
	SetThink( &CGibShooter::ShootThink );
	SetNextThink( gpGlobals->curtime );
}


void CGibShooter::Spawn( void )
{
	Precache();

	SetSolid( SOLID_NONE );
	m_fEffects |= EF_NODRAW;

	if ( m_flDelay == 0 )
	{
		m_flDelay = 0.1;
	}

	if ( m_flGibLife == 0 )
	{
		m_flGibLife = 25;
	}

	m_iGibCapacity = m_iGibs;

	m_nMaxGibModelFrame = modelinfo->GetModelFrameCount( modelinfo->GetModel( m_iGibModelIndex ) );
	Relink();
}

CGib *CGibShooter::CreateGib ( void )
{
	ConVar const *hgibs = cvar->FindVar( "violence_hgibs" );
	if ( hgibs && !hgibs->GetInt() )
		return NULL;

	CGib *pGib = CREATE_ENTITY( CGib, "gib" );
	pGib->Spawn( "models/gibs/hgibs.mdl" );
	pGib->SetBloodColor( BLOOD_COLOR_RED );

	if ( m_nMaxGibModelFrame <= 1 )
	{
		DevWarning( 2, "GibShooter Body is <= 1!\n" );
	}

	pGib->m_nBody = random->RandomInt ( 1, m_nMaxGibModelFrame - 1 );// avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}


void CGibShooter::InitPointGib( CGib *pGib, const Vector &vecShootDir )
{
	if ( pGib )
	{
		pGib->SetLocalOrigin( GetAbsOrigin() );
		pGib->SetAbsVelocity( vecShootDir * m_flGibVelocity );

		QAngle angVel( random->RandomFloat ( 100, 200 ), random->RandomFloat ( 100, 300 ), 0 );
		pGib->SetLocalAngularVelocity( angVel );

		float thinkTime = ( pGib->GetNextThink() - gpGlobals->curtime );

		pGib->m_lifeTime = (m_flGibLife * random->RandomFloat( 0.95, 1.05 ));	// +/- 5%
		
		if ( pGib->m_lifeTime < thinkTime )
		{
			pGib->SetNextThink( gpGlobals->curtime + pGib->m_lifeTime );
			pGib->m_lifeTime = 0;
		}
		UTIL_Relink( pGib );

		if ( m_bIsSprite == true )
		{
			pGib->SetSprite( CSprite::SpriteCreate( STRING( GetModelName() ), pGib->GetAbsOrigin(), false ) );

			CSprite *pSprite = (CSprite*)pGib->GetSprite();

			if ( pSprite )
			{
				pSprite->SetAttachment( pGib, 0 );
				pSprite->SetOwnerEntity( pGib );

				pSprite->SetScale( pGib->m_flModelScale );
				pSprite->SetTransparency( m_nRenderMode, m_clrRender->r, m_clrRender->g, m_clrRender->b, m_clrRender->a, m_nRenderFX );
				pSprite->AnimateForTime( 5, m_flGibLife + 1 ); //This framerate is totally wrong
				pSprite->Relink();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGibShooter :: ShootThink ( void )
{
	SetNextThink( gpGlobals->curtime + m_flDelay );

	Vector vecShootDir, vForward,vRight,vUp;
	AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
	vecShootDir = vForward;
	vecShootDir = vecShootDir + vRight * random->RandomFloat( -1, 1) * m_flVariance;
	vecShootDir = vecShootDir + vForward * random->RandomFloat( -1, 1) * m_flVariance;
	vecShootDir = vecShootDir + vUp * random->RandomFloat( -1, 1) * m_flVariance;

	VectorNormalize( vecShootDir );
	
	switch (m_nSimulationType)
	{
		case GIB_SIMULATE_RAGDOLL:
		{
			// UNDONE: Assume a mass of 200 for now
			Vector flForce = vecShootDir * m_flGibVelocity * 200;
			CreateRagGib( STRING( GetModelName() ), GetAbsOrigin(), GetAbsAngles(), flForce);
			break;
		}
		case GIB_SIMULATE_PHYSICS:
		{
			CGib *pGib = CreateGib();

			if ( pGib )
			{
				pGib->SetAbsOrigin( GetAbsOrigin() );
				pGib->SetAbsAngles( m_angGibRotation );

				pGib->m_lifeTime = (m_flGibLife * random->RandomFloat( 0.95, 1.05 ));	// +/- 5%

				pGib->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
				IPhysicsObject *pPhysicsObject = pGib->VPhysicsInitNormal( SOLID_VPHYSICS, pGib->GetSolidFlags(), false );
				pGib->SetMoveType( MOVETYPE_VPHYSICS );	  
				
				if ( pPhysicsObject )
				{
					// Set gib velocity
					Vector vVel		= vecShootDir * m_flGibVelocity;
					pPhysicsObject->AddVelocity(&vVel, NULL);
				}
				else
				{
					InitPointGib( pGib, vecShootDir );
				}
			}
			break;
		}
		case GIB_SIMULATE_POINT:
		{
			CGib *pGib = CreateGib();
			pGib->SetAbsAngles( m_angGibRotation );
			
			InitPointGib( pGib, vecShootDir );
			break;
		}
	}

	if ( --m_iGibs <= 0 )
	{
		if ( HasSpawnFlags(SF_GIBSHOOTER_REPEATABLE) )
		{
			m_iGibs = m_iGibCapacity;
			SetThink ( NULL );
			SetNextThink( gpGlobals->curtime );
		}
		else
		{
			SetThink ( &CGibShooter::SUB_Remove );
			SetNextThink( gpGlobals->curtime );
		}
	}
}

#define	SF_SHOOTER_FLAMING	(1<<1)

class CEnvShooter : public CGibShooter
{
public:
	DECLARE_CLASS( CEnvShooter, CGibShooter );

	void		Precache( void );
	bool		KeyValue( const char *szKeyName, const char *szValue );

	CGib		*CreateGib( void );
	
	DECLARE_DATADESC();

public:

	int m_nSkin;
	float m_flGibScale;
};

BEGIN_DATADESC( CEnvShooter )

	DEFINE_KEYFIELD( CEnvShooter, m_nSkin, FIELD_INTEGER, "skin" ),
	DEFINE_KEYFIELD( CEnvShooter, m_flGibScale, FIELD_FLOAT ,"scale" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( env_shooter, CEnvShooter );

bool CEnvShooter::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "shootmodel"))
	{
		m_bIsSprite = false;
		SetModelName( AllocPooledString(szValue) );

		//Adrian - not pretty...
		if ( Q_stristr( szValue, ".vmt" ) )
			 m_bIsSprite = true;
	}

	else if (FStrEq(szKeyName, "shootsounds"))
	{
		int iNoise = atoi(szValue);
		switch( iNoise )
		{
		case 0:
			m_iGibMaterial = matGlass;
			break;
		case 1:
			m_iGibMaterial = matWood;
			break;
		case 2:
			m_iGibMaterial = matMetal;
			break;
		case 3:
			m_iGibMaterial = matFlesh;
			break;
		case 4:
			m_iGibMaterial = matRocks;
			break;
		
		default:
		case -1:
			m_iGibMaterial = matNone;
			break;
		}
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


void CEnvShooter :: Precache ( void )
{
	m_iGibModelIndex = engine->PrecacheModel( STRING( GetModelName() ) );
}


CGib *CEnvShooter :: CreateGib ( void )
{
	CGib *pGib = CREATE_ENTITY( CGib, "gib" );

	if ( m_bIsSprite == true )
	{
		//HACK HACK
		pGib->Spawn( "" );
	}
	else
	{
		pGib->Spawn( STRING( GetModelName() ) );
	}
	
	int bodyPart = 0;

	if ( m_nMaxGibModelFrame > 1 )
	{
		bodyPart = random->RandomInt( 0, m_nMaxGibModelFrame-1 );
	}

	pGib->m_nBody = bodyPart;
	pGib->SetBloodColor( DONT_BLEED );
	pGib->m_material = m_iGibMaterial;

	pGib->m_nRenderMode = m_nRenderMode;
	pGib->m_clrRender = m_clrRender;
	pGib->m_nRenderFX = m_nRenderFX;
	pGib->m_flModelScale = m_flGibScale;
	pGib->m_nSkin = m_nSkin;
	pGib->m_lifeTime = gpGlobals->curtime + m_flGibLife;
		
	// Spawn a flaming gib
	if ( HasSpawnFlags( SF_SHOOTER_FLAMING ) )
	{
		// Tag an entity flame along with us
		CEntityFlame *pFlame = CEntityFlame::Create( pGib );
		if ( pFlame != NULL )
		{
			pFlame->SetLifetime( pGib->m_lifeTime );
		}
	}

	return pGib;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTestEffect : public CBaseEntity
{
public:
	DECLARE_CLASS( CTestEffect, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	// bool	KeyValue( const char *szKeyName, const char *szValue );
	void Think( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int		m_iLoop;
	int		m_iBeam;
	CBeam	*m_pBeam[24];
	float	m_flBeamTime[24];
	float	m_flStartTime;
};


LINK_ENTITY_TO_CLASS( test_effect, CTestEffect );

void CTestEffect::Spawn( void )
{
	Precache( );
}

void CTestEffect::Precache( void )
{
	engine->PrecacheModel( "sprites/lgtning.vmt" );
}

void CTestEffect::Think( void )
{
	int i;
	float t = (gpGlobals->curtime - m_flStartTime);

	if (m_iBeam < 24)
	{
		CBeam *pbeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 10 );

		trace_t	tr;

		Vector vecSrc = GetAbsOrigin();
		Vector vecDir = Vector( random->RandomFloat( -1.0, 1.0 ), random->RandomFloat( -1.0, 1.0 ),random->RandomFloat( -1.0, 1.0 ) );
		VectorNormalize( vecDir );
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 128, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

		pbeam->PointsInit( vecSrc, tr.endpos );
		// pbeam->SetColor( 80, 100, 255 );
		pbeam->SetColor( 255, 180, 100 );
		pbeam->SetWidth( 10.0 );
		pbeam->SetScrollRate( 12 );
		
		m_flBeamTime[m_iBeam] = gpGlobals->curtime;
		m_pBeam[m_iBeam] = pbeam;
		m_iBeam++;

#if 0
		Vector vecMid = (vecSrc + tr.endpos) * 0.5;
		CBroadcastRecipientFilter filter;
		TE_DynamicLight( filter, 0.0,
			vecMid, 255, 180, 100, 3, 2.0, 0.0 );
#endif
	}

	if (t < 3.0)
	{
		for (i = 0; i < m_iBeam; i++)
		{
			t = (gpGlobals->curtime - m_flBeamTime[i]) / ( 3 + m_flStartTime - m_flBeamTime[i]);
			m_pBeam[i]->SetBrightness( 255 * t );
			// m_pBeam[i]->SetScrollRate( 20 * t );
		}
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		for (i = 0; i < m_iBeam; i++)
		{
			UTIL_Remove( m_pBeam[i] );
		}
		m_flStartTime = gpGlobals->curtime;
		m_iBeam = 0;
		SetNextThink( TICK_NEVER_THINK );
	}
}


void CTestEffect::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetNextThink( gpGlobals->curtime + 0.1f );
	m_flStartTime = gpGlobals->curtime;
}



// Blood effects
class CBlood : public CPointEntity
{
public:
	DECLARE_CLASS( CBlood, CPointEntity );

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	inline	int		Color( void ) { return m_Color; }
	inline	float 	BloodAmount( void ) { return m_flAmount; }

	inline	void SetColor( int color ) { m_Color = color; }
	
	// Input handlers
	void InputEmitBlood( inputdata_t &inputdata );

	Vector	Direction( void );
	Vector	BloodPosition( CBaseEntity *pActivator );

	DECLARE_DATADESC();

	Vector m_vecSprayDir;
	float m_flAmount;
	int m_Color;

private:
};

LINK_ENTITY_TO_CLASS( env_blood, CBlood );

BEGIN_DATADESC( CBlood )

	DEFINE_KEYFIELD( CBlood, m_vecSprayDir, FIELD_VECTOR, "spraydir" ),
	DEFINE_KEYFIELD( CBlood, m_flAmount, FIELD_FLOAT, "amount" ),
	DEFINE_FIELD( CBlood, m_Color, FIELD_INTEGER ),

	DEFINE_INPUTFUNC( CBlood, FIELD_VOID, "EmitBlood", InputEmitBlood ),

END_DATADESC()


#define SF_BLOOD_RANDOM		0x0001
#define SF_BLOOD_STREAM		0x0002
#define SF_BLOOD_PLAYER		0x0004
#define SF_BLOOD_DECAL		0x0008
#define SF_BLOOD_CLOUD		0x0010
#define SF_BLOOD_DROPS		0x0020
#define SF_BLOOD_GORE		0x0040


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlood::Spawn( void )
{
	// Convert spraydir from angles to a vector
	QAngle angSprayDir = QAngle( m_vecSprayDir.x, m_vecSprayDir.y, m_vecSprayDir.z );
	AngleVectors( angSprayDir, &m_vecSprayDir );

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetColor( BLOOD_COLOR_RED );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKeyName - 
//			szValue - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBlood::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "color"))
	{
		int color = atoi(szValue);
		switch ( color )
		{
			case 1:
			{
				SetColor( BLOOD_COLOR_YELLOW );
				break;
			}
		}
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


Vector CBlood::Direction( void )
{
	if ( HasSpawnFlags( SF_BLOOD_RANDOM ) )
		return UTIL_RandomBloodVector();
	
	return m_vecSprayDir;
}


Vector CBlood::BloodPosition( CBaseEntity *pActivator )
{
	if ( HasSpawnFlags( SF_BLOOD_PLAYER ) )
	{
		edict_t *pPlayer;

		if ( pActivator && pActivator->IsPlayer() )
		{
			pPlayer = pActivator->edict();
		}
		else
			pPlayer = engine->PEntityOfEntIndex( 1 );
		if ( pPlayer )
		{
			CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pPlayer );
			if ( player )
			{
				return (player->EyePosition()) + Vector( random->RandomFloat(-10,10), random->RandomFloat(-10,10), random->RandomFloat(-10,10) );
			}
		}
	}

	return GetLocalOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags )
{
	CEffectData	data;

	data.m_vOrigin = pos;
	data.m_vNormal = dir;
	data.m_flScale = (float)amount;
	data.m_fFlags = flags;
	data.m_nColor = (unsigned char)color;

	DispatchEffect( "bloodspray", data );
}



//-----------------------------------------------------------------------------
// Purpose: Input handler for triggering the blood effect.
//-----------------------------------------------------------------------------
void CBlood::InputEmitBlood( inputdata_t &inputdata )
{
	if ( HasSpawnFlags( SF_BLOOD_STREAM ) )
	{
		UTIL_BloodStream( BloodPosition(inputdata.pActivator), Direction(), Color(), BloodAmount() );
	}
	else
	{
		UTIL_BloodDrips( BloodPosition(inputdata.pActivator), Direction(), Color(), BloodAmount() );
	}

	if ( HasSpawnFlags( SF_BLOOD_DECAL ) )
	{
		Vector forward = Direction();
		Vector start = BloodPosition( inputdata.pActivator );
		trace_t tr;

		UTIL_TraceLine( start, start + forward * BloodAmount() * 2, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &tr, Color() );
		}
	}

	//
	// New-fangled blood effects.
	//
	if ( HasSpawnFlags( SF_BLOOD_CLOUD | SF_BLOOD_DROPS | SF_BLOOD_GORE ) )
	{
		int nFlags = 0;
		if (HasSpawnFlags(SF_BLOOD_CLOUD))
		{
			nFlags |= FX_BLOODSPRAY_CLOUD;
		}

		if (HasSpawnFlags(SF_BLOOD_DROPS))
		{
			nFlags |= FX_BLOODSPRAY_DROPS;
		}

		if (HasSpawnFlags(SF_BLOOD_GORE))
		{
			nFlags |= FX_BLOODSPRAY_GORE;
		}

		UTIL_BloodSpray(GetAbsOrigin(), Direction(), Color(), BloodAmount(), nFlags);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Console command for emitting the blood spray effect from an NPC.
//-----------------------------------------------------------------------------
void CC_BloodSpray( void )
{
	CBaseEntity *pEnt = NULL;
	while ((pEnt = gEntList.FindEntityGeneric(pEnt, engine->Cmd_Argv(1))) != NULL)
	{
		Vector forward;
		pEnt->GetVectors(&forward, NULL, NULL);
		UTIL_BloodSpray( (forward * 4 ) + ( pEnt->EyePosition() + pEnt->WorldSpaceCenter() ) * 0.5f, forward, BLOOD_COLOR_RED, 4, FX_BLOODSPRAY_ALL );
	}
}

static ConCommand bloodspray( "bloodspray", CC_BloodSpray, "blood", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CEnvFunnel : public CBaseEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CEnvFunnel, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int		m_iSprite;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( env_funnel, CEnvFunnel );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CEnvFunnel )

//	DEFINE_FIELD( CEnvFunnel, m_iSprite,	FIELD_INTEGER ),

END_DATADESC()



void CEnvFunnel :: Precache ( void )
{
	m_iSprite = engine->PrecacheModel ( "sprites/flare6.vmt" );
}

void CEnvFunnel::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBroadcastRecipientFilter filter;
	te->LargeFunnel( filter, 0.0,
		&GetAbsOrigin(), m_iSprite, HasSpawnFlags( SF_FUNNEL_REVERSE ) ? 1 : 0 );

	SetThink( &CEnvFunnel::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

void CEnvFunnel::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	m_fEffects |= EF_NODRAW;
}

//=========================================================
// Beverage Dispenser
// overloaded m_iHealth, is now how many cans remain in the machine.
//=========================================================
class CEnvBeverage : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvBeverage, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	// Input handlers.
	void	InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();

public:
	bool	m_CanInDispenser;
	int		m_nBeverageType;
};

void CEnvBeverage :: Precache ( void )
{
	engine->PrecacheModel( "models/can.mdl" );
}

BEGIN_DATADESC( CEnvBeverage )
	DEFINE_FIELD( CEnvBeverage, m_CanInDispenser, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEnvBeverage, m_nBeverageType, FIELD_INTEGER ),

	DEFINE_INPUTFUNC( CEnvBeverage, FIELD_VOID, "Activate", InputActivate ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_beverage, CEnvBeverage );


bool CEnvBeverage::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "beveragetype"))
	{
		m_nBeverageType = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CEnvBeverage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_CanInDispenser || m_iHealth <= 0 )
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseAnimating *pCan = (CBaseAnimating *)CBaseEntity::Create( "item_sodacan", GetLocalOrigin(), GetLocalAngles(), this );	

	if ( m_nBeverageType == 6 )
	{
		// random
		pCan->m_nSkin = random->RandomInt( 0, 5 );
	}
	else
	{
		pCan->m_nSkin = m_nBeverageType;
	}

	m_CanInDispenser = true;
	m_iHealth -= 1;

	//SetThink (SUB_Remove);
	//SetNextThink( gpGlobals->curtime );
}

void CEnvBeverage::InputActivate( inputdata_t &inputdata )
{
	Use( inputdata.pActivator, inputdata.pCaller, USE_ON, 0 );
}

void CEnvBeverage::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	m_fEffects |= EF_NODRAW;
	m_CanInDispenser = false;

	if ( m_iHealth == 0 )
	{
		m_iHealth = 10;
	}
}

//=========================================================
// Soda can
//=========================================================
class CItemSoda : public CBaseAnimating
{
public:
	DECLARE_CLASS( CItemSoda, CBaseAnimating );

	void	Spawn( void );
	void	Precache( void );
	void	CanThink ( void );
	void	CanTouch ( CBaseEntity *pOther );

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CItemSoda )

	// Function Pointers
	DEFINE_FUNCTION( CItemSoda, CanThink ),
	DEFINE_FUNCTION( CItemSoda, CanTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( item_sodacan, CItemSoda );

void CItemSoda :: Precache ( void )
{
	engine->PrecacheModel( "models/can.mdl" );
}

void CItemSoda::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_FLYGRAVITY );

	SetModel ( "models/can.mdl" );
	UTIL_SetSize ( this, Vector ( 0, 0, 0 ), Vector ( 0, 0, 0 ) );
	
	SetThink (&CItemSoda::CanThink);
	SetNextThink( gpGlobals->curtime + 0.5f );
}

void CItemSoda::CanThink ( void )
{
	EmitSound( "ItemSoda.Bounce" );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER );
	UTIL_SetSize ( this, Vector ( -8, -8, 0 ), Vector ( 8, 8, 8 ) );
	Relink();

	SetThink ( NULL );
	SetTouch ( &CItemSoda::CanTouch );
}

void CItemSoda::CanTouch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	// spoit sound here

	pOther->TakeHealth( 1, DMG_GENERIC );// a bit of health.

	if ( GetOwnerEntity() )
	{
		// tell the machine the can was taken
		CEnvBeverage *bev = (CEnvBeverage *)GetOwnerEntity();
		bev->m_CanInDispenser = false;
	}

	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	m_fEffects |= EF_NODRAW;
	SetTouch ( NULL );
	SetThink ( &CItemSoda::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
	Relink();
}


//=========================================================
// func_precipitation - temporary snow solution for first HL2 
// technology demo
//=========================================================

class CPrecipitation : public CBaseEntity
{
public:
	DECLARE_CLASS( CPrecipitation, CBaseEntity );

	DECLARE_SERVERCLASS();

	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( func_precipitation, CPrecipitation );

// Just send the normal entity crap
IMPLEMENT_SERVERCLASS_ST( CPrecipitation, DT_Precipitation)
END_SEND_TABLE()


void CPrecipitation::Spawn( void )							   
{
	Precache();
	SetSolid( SOLID_NONE );							// Remove model & collisions
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );		// Set size

	m_nRenderMode = kRenderEnvironmental;
}


//-----------------------------------------------------------------------------
// EnvWind - global wind info
//-----------------------------------------------------------------------------
class CEnvWind : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvWind, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	void	WindThink( void );
	bool	ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

private:
	CNetworkVarEmbedded( CEnvWindShared, m_EnvWindShared );
};

LINK_ENTITY_TO_CLASS( env_wind, CEnvWind );

BEGIN_DATADESC( CEnvWind )

	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_iMinWind, FIELD_INTEGER, "minwind" ),
	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_iMaxWind, FIELD_INTEGER, "maxwind" ),
	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_iMinGust, FIELD_INTEGER, "mingust" ),
	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_iMaxGust, FIELD_INTEGER, "maxgust" ),
	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_flMinGustDelay, FIELD_FLOAT, "mingustdelay" ),
	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_flMaxGustDelay, FIELD_FLOAT, "maxgustdelay" ),
	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_iGustDirChange, FIELD_INTEGER, "gustdirchange" ),
//	DEFINE_KEYFIELD( CEnvWind, m_EnvWindShared.m_iszGustSound, FIELD_STRING, "gustsound" ),

// Just here to quiet down classcheck
	// DEFINE_FIELD( CEnvWind, m_EnvWindShared, CEnvWindShared ),

	DEFINE_FIELD( CEnvWind, m_EnvWindShared.m_iWindDir, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvWind, m_EnvWindShared.m_flWindSpeed, FIELD_FLOAT ),

	// Function Pointers
	DEFINE_FUNCTION( CEnvWind, WindThink ),

END_DATADESC()


BEGIN_SEND_TABLE_NOBASE(CEnvWindShared, DT_EnvWindShared)
	// These are parameters that are used to generate the entire motion
	SendPropInt		(SENDINFO(m_iMinWind),		10, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxWind),		10, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMinGust),		10, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxGust),		10, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_flMinGustDelay), 0, SPROP_NOSCALE),		// NOTE: Have to do this, so it's *exactly* the same on client
	SendPropFloat	(SENDINFO(m_flMaxGustDelay), 0, SPROP_NOSCALE),
	SendPropInt		(SENDINFO(m_iGustDirChange), 9, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iWindSeed),		32, SPROP_UNSIGNED ),

	// These are related to initial state
	SendPropInt		(SENDINFO(m_iInitialWindDir),9, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_flInitialWindSpeed),0, SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_flStartTime),	 0, SPROP_NOSCALE ),

	// Sound related
//	SendPropInt		(SENDINFO(m_iszGustSound),	10, SPROP_UNSIGNED ),
END_SEND_TABLE()

// This table encodes the CBaseEntity data.
IMPLEMENT_SERVERCLASS_ST_NOBASE(CEnvWind, DT_EnvWind)
	SendPropDataTable(SENDINFO_DT(m_EnvWindShared), &REFERENCE_SEND_TABLE(DT_EnvWindShared)),
END_SEND_TABLE()

void CEnvWind :: Precache ( void )
{
//	if (m_iszGustSound)
//		enginesound->PrecacheSound( STRING( m_iszGustSound ) );
}

void CEnvWind::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	m_fEffects |= EF_NODRAW;

	m_EnvWindShared.Init( entindex(), 0, gpGlobals->frametime, GetLocalAngles().y, 0 );

	// We never need to re-transmit (unless an input event happens)
	NetworkStateManualMode( true );
	NetworkStateChanged( );

	SetThink( &CEnvWind::WindThink );
	SetNextThink( gpGlobals->curtime );
	Relink();
}

bool CEnvWind::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	return true;
}

void CEnvWind::WindThink( void )
{
	SetNextThink( m_EnvWindShared.WindThink( gpGlobals->curtime ) );
}



//==================================================
// CEmbers
//==================================================

#define	bitsSF_EMBERS_START_ON	0x00000001
#define	bitsSF_EMBERS_TOGGLE	0x00000002

// UNDONE: This is a brush effect-in-volume entity, move client side.
class CEmbers : public CBaseEntity
{
public:
	DECLARE_CLASS( CEmbers, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	
	void	EmberUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CNetworkVar( int, m_nDensity );
	CNetworkVar( int, m_nLifetime );
	CNetworkVar( int, m_nSpeed );

	CNetworkVar( bool, m_bEmit );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( env_embers, CEmbers );

//Data description
BEGIN_DATADESC( CEmbers )

	DEFINE_KEYFIELD( CEmbers, m_nDensity,	FIELD_INTEGER, "density" ),
	DEFINE_KEYFIELD( CEmbers, m_nLifetime,	FIELD_INTEGER, "lifetime" ),
	DEFINE_KEYFIELD( CEmbers, m_nSpeed,		FIELD_INTEGER, "speed" ),

	DEFINE_FIELD( CEmbers, m_bEmit,	FIELD_BOOLEAN ),

	//Function pointers
	DEFINE_FUNCTION( CEmbers, EmberUse ),

END_DATADESC()


//Data table
IMPLEMENT_SERVERCLASS_ST( CEmbers, DT_Embers )
	SendPropInt(	SENDINFO( m_nDensity ),		32,	SPROP_UNSIGNED ),
	SendPropInt(	SENDINFO( m_nLifetime ),	32,	SPROP_UNSIGNED ),
	SendPropInt(	SENDINFO( m_nSpeed ),		32,	SPROP_UNSIGNED ),
	SendPropInt(	SENDINFO( m_bEmit ),		2,	SPROP_UNSIGNED ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbers::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	SetSolid( SOLID_NONE );
	SetRenderColorA( 0 );
	m_nRenderMode	= kRenderTransTexture;

	Relink();
	SetUse( &CEmbers::EmberUse );

	//Start off if we're targetted (unless flagged)
	m_bEmit = ( HasSpawnFlags( bitsSF_EMBERS_START_ON ) || ( !GetEntityName() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbers::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CEmbers::EmberUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//If we're not toggable, only allow one use
	if ( !HasSpawnFlags( bitsSF_EMBERS_TOGGLE ) )
	{
		SetUse( NULL );
	}

	//Handle it
	switch ( useType )
	{
	case USE_OFF:
		m_bEmit = false;
		break;

	case USE_ON:
		m_bEmit = true;
		break;

	case USE_SET:
		m_bEmit = !!(int)value;
		break;

	default:
	case USE_TOGGLE:
		m_bEmit = !m_bEmit;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPhysicsWire : public CBaseEntity
{
public:
	DECLARE_CLASS( CPhysicsWire, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );

	DECLARE_DATADESC();

protected:

	bool SetupPhysics( void );

	int		m_nDensity;
};

LINK_ENTITY_TO_CLASS( env_physwire, CPhysicsWire );

BEGIN_DATADESC( CPhysicsWire )

	DEFINE_KEYFIELD( CPhysicsWire, m_nDensity,	FIELD_INTEGER, "Density" ),
//	DEFINE_KEYFIELD( CPhysicsWire, m_frequency, FIELD_INTEGER, "frequency" ),

//	DEFINE_FIELD( CPhysicsWire, m_flFoo, FIELD_FLOAT ),

	// Function Pointers
//	DEFINE_FUNCTION( CPhysicsWire, WireThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsWire::Spawn( void )
{
	BaseClass::Spawn();

	Precache();

//	if ( SetupPhysics() == false )
//		return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsWire::Precache( void )
{
	BaseClass::Precache();
}

class CPhysBallSocket;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPhysicsWire::SetupPhysics( void )
{
/*
	CPointEntity	*anchorEnt, *freeEnt;
	CPhysBallSocket *socket;

	char	anchorName[256];
	char	freeName[256];

	int		iAnchorName, iFreeName;

	anchorEnt = (CPointEntity *) CreateEntityByName( "info_target" );

	if ( anchorEnt == NULL )
		return false;

	//Create and connect all segments
	for ( int i = 0; i < m_nDensity; i++ )
	{
		// Create other end of our link
		freeEnt = (CPointEntity *) CreateEntityByName( "info_target" );

		// Create a ballsocket and attach the two
		//socket = (CPhysBallSocket *) CreateEntityByName( "phys_ballsocket" );

		Q_snprintf( anchorName,sizeof(anchorName), "__PWIREANCHOR%d", i );
		Q_snprintf( freeName,sizeof(freeName), "__PWIREFREE%d", i+1 );

		iAnchorName = MAKE_STRING( anchorName );
		iFreeName	= MAKE_STRING( freeName );
		
		//Fake the names
		//socket->m_nameAttach1 = anchorEnt->m_iGlobalname	= iAnchorName;
		//socket->m_nameAttach2 = freeEnt->m_iGlobalname	= iFreeName
		
		//socket->Activate();

		//The free ent is now the anchor for the next link
		anchorEnt = freeEnt;
	}
*/

	return true;
}

//
// Muzzle flash
//

class CEnvMuzzleFlash : public CPointEntity
{
	DECLARE_CLASS( CEnvMuzzleFlash, CPointEntity );

public:
	// Input handlers
	void	InputFire( inputdata_t &inputdata );
	
	DECLARE_DATADESC();

	float	m_flScale;
};

BEGIN_DATADESC( CEnvMuzzleFlash )

	DEFINE_KEYFIELD( CEnvMuzzleFlash, m_flScale, FIELD_FLOAT, "scale" ),

	DEFINE_INPUTFUNC( CEnvMuzzleFlash, FIELD_VOID, "Fire", InputFire ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( env_muzzleflash, CEnvMuzzleFlash );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvMuzzleFlash::InputFire( inputdata_t &inputdata )
{
	g_pEffects->MuzzleFlash( GetAbsOrigin(), GetAbsAngles(), m_flScale, MUZZLEFLASH_TYPE_DEFAULT );
}


//=========================================================
// Splash!
//=========================================================
class CEnvSplash : public CPointEntity 
{
	DECLARE_CLASS( CEnvSplash, CPointEntity );

public:
	// Input handlers
	void	InputSplash( inputdata_t &inputdata );
	
protected:

	float	m_flScale;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CEnvSplash )
	DEFINE_KEYFIELD( CEnvSplash,	m_flScale, FIELD_FLOAT, "scale" ),

	DEFINE_INPUTFUNC( CEnvSplash, FIELD_VOID, "Splash", InputSplash ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_splash, CEnvSplash );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvSplash::InputSplash( inputdata_t &inputdata )
{
	CEffectData	data;

	data.m_vOrigin = GetAbsOrigin();
	data.m_vNormal = Vector( 0, 0, 1 );

	data.m_flScale = m_flScale;

	DispatchEffect( "watersplash", data );
}
