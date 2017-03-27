//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "engine/IEngineSound.h"
#include "particles_simple.h"
#include "dlight.h"
#include "iefx.h"
#include "clientsideeffects.h"
#include "ClientEffectPrecacheSystem.h"
#include "cdll_util.h"
#include "glow_overlay.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"

//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheMuzzleFlash )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash1" )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash2" )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash3" )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash4" )
CLIENTEFFECT_MATERIAL( "effects/bluemuzzle" )
CLIENTEFFECT_MATERIAL( "effects/gunshipmuzzle" )
CLIENTEFFECT_MATERIAL( "effects/gunshiptracer" )
CLIENTEFFECT_REGISTER_END()

//Whether or not we should emit a dynamic light
static ConVar muzzleflash_light( "muzzleflash_light", "1", FCVAR_ARCHIVE );

extern void FX_TracerSound( const Vector &start, const Vector &end, int iTracerType );

//===================================================================
//===================================================================
class CImpactOverlay : public CWarpOverlay
{
public:
	
	virtual bool Update( void )
	{
		m_flLifetime += gpGlobals->frametime;
		
		static float flTotalLifetime = 0.1f;
		static Vector vColor( 0.4, 0.25, 0 );

		if ( m_flLifetime < flTotalLifetime )
		{
			float flColorScale = 1.0f - ( m_flLifetime / flTotalLifetime );

			for( int i=0; i < m_nSprites; i++ )
			{
				m_Sprites[i].m_vColor = m_vBaseColors[i] * flColorScale;
				
				m_Sprites[i].m_flHorzSize += 1.0f * gpGlobals->frametime;
				m_Sprites[i].m_flVertSize += 1.0f * gpGlobals->frametime;
			}
	
			return true;
		}
	
		return false;
	}

public:

	float	m_flLifetime;
	Vector	m_vBaseColors[CGlowOverlay::MAX_SPRITES];

};

//-----------------------------------------------------------------------------
// Purpose: Play random ricochet sound
// Input  : *pos - 
//-----------------------------------------------------------------------------
void FX_RicochetSound( const Vector& pos )
{
	Vector org = pos;
	CLocalPlayerFilter filter;
 	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "FX_RicochetSound.Ricochet", &org );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			scale - 
//			entityIndex - 
//-----------------------------------------------------------------------------
void FX_MuzzleEffect( const Vector &origin, const QAngle &angles, float scale, int entityIndex, unsigned char *pFlashColor )
{
	VPROF_BUDGET( "FX_MuzzleEffect", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash" );
	pSimple->SetSortOrigin( origin );
	
	SimpleParticle *pParticle;
	Vector			forward, offset;
	int				alpha, color;

	AngleVectors( angles, &forward );
	float flScale = random->RandomFloat( scale-0.25f, scale+0.25f );

	if ( flScale < 0.5f )
	{
		flScale = 0.5f;
	}
	else if ( flScale > 8.0f )
	{
		flScale = 8.0f;
	}

	//
	// Flash
	//

	int i;
	for ( i = 1; i < 9; i++ )
	{
		offset = origin + (forward * (i*2.0f*scale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/muzzleflash%d", random->RandomInt(1,4) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.1f;

		pParticle->m_vecVelocity.Init();

		if ( !pFlashColor )
		{
			pParticle->m_uchColor[0]	= 255;
			pParticle->m_uchColor[1]	= 255;
			pParticle->m_uchColor[2]	= 255;
		}
		else
		{
			pParticle->m_uchColor[0]	= pFlashColor[0];
			pParticle->m_uchColor[1]	= pFlashColor[1];
			pParticle->m_uchColor[2]	= pFlashColor[2];
		}

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= (random->RandomFloat( 6.0f, 9.0f ) * (12-(i))/9) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	//
	// Smoke
	//

	for ( i = 0; i < 4; i++ )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "particle/particle_smokegrenade" ), origin );
			
		if ( pParticle == NULL )
			return;

		alpha = random->RandomInt( 32, 84 );
		color = random->RandomInt( 64, 164 );

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= random->RandomFloat( 0.5f, 1.0f );

		pParticle->m_vecVelocity.Random( -0.5f, 0.5f );
		pParticle->m_vecVelocity += forward;
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity	*= random->RandomFloat( 16.0f, 32.0f );
		pParticle->m_vecVelocity[2] += random->RandomFloat( 4.0f, 16.0f );

		pParticle->m_uchColor[0]	= color;
		pParticle->m_uchColor[1]	= color;
		pParticle->m_uchColor[2]	= color;
		pParticle->m_uchStartAlpha	= alpha;
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_uchStartSize	= random->RandomInt( 4, 8 ) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}


	// 
	// Dynamic light
	//

	if ( muzzleflash_light.GetInt() )
	{
		dlight_t *dl = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + entityIndex );

		dl->origin	= origin;

		dl->color.r = 255;
		dl->color.g = 192;
		dl->color.b = 64;
		dl->color.exponent = 5;

		dl->radius	= 100;
		dl->decay	= dl->radius / 0.05f;
		dl->die		= gpGlobals->curtime + 0.05f;
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&velocity - 
//			scale - 
//			numParticles - 
//			*pColor - 
//			iAlpha - 
//			*pMaterial - 
//			flRoll - 
//			flRollDelta - 
//-----------------------------------------------------------------------------
CSmartPtr<CSimpleEmitter> FX_Smoke( const Vector &origin, const Vector &velocity, float scale, int numParticles, float flDietime, unsigned char *pColor, int iAlpha, const char *pMaterial, float flRoll, float flRollDelta )
{
	VPROF_BUDGET( "FX_Smoke", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_Smoke" );
	pSimple->SetSortOrigin( origin );

	SimpleParticle *pParticle;

	// Smoke
	for ( int i = 0; i < numParticles; i++ )
	{
		PMaterialHandle hMaterial = pSimple->GetPMaterial( pMaterial );
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, origin );			
		if ( pParticle == NULL )
			return NULL;

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime = flDietime;
		pParticle->m_vecVelocity = velocity;
		for( int i = 0; i < 3; ++i )
		{
			pParticle->m_uchColor[i] = pColor[i];
		}
		pParticle->m_uchStartAlpha	= iAlpha;
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_uchStartSize	= scale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;
		pParticle->m_flRoll	= flRoll;
		pParticle->m_flRollDelta = flRollDelta;
	}

	return pSimple;
}

//-----------------------------------------------------------------------------
// Purpose: Smoke puffs
//-----------------------------------------------------------------------------
void FX_Smoke( const Vector &origin, const QAngle &angles, float scale, int numParticles, unsigned char *pColor, int iAlpha )
{
	VPROF_BUDGET( "FX_Smoke", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector vecVelocity;
	Vector vecForward;

	// Smoke
	for ( int i = 0; i < numParticles; i++ )
	{
		// Velocity
		AngleVectors( angles, &vecForward );
		vecVelocity.Random( -0.5f, 0.5f );
		vecVelocity += vecForward;
		VectorNormalize( vecVelocity );
		vecVelocity	*= random->RandomFloat( 16.0f, 32.0f );
		vecVelocity[2] += random->RandomFloat( 4.0f, 16.0f );

		// Color
		unsigned char particlecolor[3];
		if ( !pColor )
		{
			int color = random->RandomInt( 64, 164 );
			particlecolor[0] = color;
			particlecolor[1] = color;
			particlecolor[2] = color;
		}
		else
		{
			int color[3][2];
			for( int i = 0; i < 3; ++i )
			{
				color[i][0] = pColor[i] - 64;
				color[i][1] = pColor[i] + 64;

				if ( color[i][0] < 0 ) { color[i][0] = 0; }
				if ( color[i][1] > 255 ) { color[i][1] = 255; }
			}

			particlecolor[0] = random->RandomInt( color[0][0], color[0][1] );
			particlecolor[1] = random->RandomInt( color[1][0], color[1][1] );
			particlecolor[2] = random->RandomInt( color[2][0], color[2][1] );
		}

		// Alpha
		int alpha = iAlpha;
		if ( alpha == -1 )
		{
			alpha = random->RandomInt( 10, 25 );
		}

		// Scale
		int iSize = random->RandomInt( 4, 8 ) * scale;

		// Roll
		float flRoll = random->RandomInt( 0, 360 );
		float flRollDelta = random->RandomFloat( -4.0f, 4.0f );

		//pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;

		FX_Smoke( origin, vecVelocity, iSize, 1, random->RandomFloat( 0.5f, 1.0f ), particlecolor, alpha, "particle/particle_smokegrenade", flRoll, flRollDelta );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Shockwave for gunship bullet impacts!
// Input  : &pos - 
//
// NOTES:	-Don't draw this effect when the viewer is very far away.
//-----------------------------------------------------------------------------
void FX_GunshipImpact( const Vector &pos )
{
	VPROF_BUDGET( "FX_GunshipImpact", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	if ( CImpactOverlay *pOverlay = new CImpactOverlay )
	{
		pOverlay->m_flLifetime	= 0;
		pOverlay->m_vPos		= pos + Vector( 0, 0, 1 ); // Doesn't show up on terrain if you don't do this(sjb)
		pOverlay->m_nSprites	= 1;
		
		pOverlay->m_vBaseColors[0].Init( 0.25f, 0.25f, 0.5f );

		pOverlay->m_Sprites[0].m_flHorzSize = 0.01f;
		pOverlay->m_Sprites[0].m_flVertSize = 0.01f;

		pOverlay->Activate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : data - 
//-----------------------------------------------------------------------------
void GunshipImpactCallback( const CEffectData & data )
{
	Vector vecPosition;

	vecPosition = data.m_vOrigin;

	FX_GunshipImpact( vecPosition );
}
DECLARE_CLIENT_EFFECT( "GunshipImpact", GunshipImpactCallback );


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			scale - 
//			entityIndex - 
//-----------------------------------------------------------------------------
void FX_GunshipMuzzleEffect( const Vector &origin, const QAngle &angles, float scale, int entityIndex, unsigned char *pFlashColor )
{
	VPROF_BUDGET( "FX_GunshipMuzzleEffect", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash" );
	pSimple->SetSortOrigin( origin );
	
	SimpleParticle *pParticle;
	Vector			forward, offset;

	AngleVectors( angles, &forward );

	//
	// Flash
	//
	offset = origin;

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/gunshipmuzzle" ), offset );
		
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= 0.15f;

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchStartSize	= random->RandomFloat( 40.0, 50.0 );
	pParticle->m_uchEndSize		= pParticle->m_uchStartSize;

	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= 0.15f;

	pParticle->m_uchColor[0]	= 255;
	pParticle->m_uchColor[1]	= 255;
	pParticle->m_uchColor[2]	= 255;

	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchEndAlpha	= 255;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_GunshipTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_GunshipTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	//Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	//Don't make small tracers
	if ( totalDist <= 256 )
		return;

	float length = random->RandomFloat( 128.0f, 256.0f );
	float life = ( totalDist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, shotDir, velocity, length, totalDist, 5.0f, life, "effects/gunshiptracer" );

	if( makeWhiz )
	{
		FX_TracerSound( start, end, TRACER_TYPE_GUNSHIP );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_StriderTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_StriderTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	//Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	//Don't make small tracers
	if ( totalDist <= 256 )
		return;

	float length = random->RandomFloat( 128.0f, 256.0f );
	float life = ( totalDist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, shotDir, velocity, length, totalDist, 5.0f, life, "effects/bluespark" );

	if( makeWhiz )
	{
		FX_TracerSound( start, end, TRACER_TYPE_STRIDER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_GaussTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_GaussTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	//Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	//Don't make small tracers
	if ( totalDist <= 256 )
		return;

	float length = random->RandomFloat( 250.0f, 500.0f );
	float life = ( totalDist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, shotDir, velocity, length, totalDist, random->RandomFloat( 5.0f, 8.0f ), life, "effects/spark" );
}
