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
#include "view.h"

#include "fx_line.h"
#include "fx_discreetline.h"
#include "fx_quad.h"
#include "clientsideeffects.h"

#include "SoundEmitterSystemBase.h"
#include "tier0/vprof.h"

//FIXME: All these functions will be moved out to FX_Main.CPP or a individual folder

/*
==================================================
FX_AddStaticLine
==================================================
*/

void FX_AddStaticLine( const Vector& start, const Vector& end, float scale, float life, const char *materialName, unsigned char flags )
{
	CFXStaticLine	*t = new CFXStaticLine( "StaticLine", start, end, scale, life, materialName, flags );
	assert( t );

	//Throw it into the list
	clienteffects->AddEffect( t );
}

/*
==================================================
FX_AddDiscreetLine
==================================================
*/

void FX_AddDiscreetLine( const Vector& start, const Vector& direction, float velocity, float length, float clipLength, float scale, float life, const char *shader )
{
	CFXDiscreetLine	*t = new CFXDiscreetLine( "Line", start, direction, velocity, length, clipLength, scale, life, shader );
	assert( t );

	//Throw it into the list
	clienteffects->AddEffect( t );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_AddQuad( const FXQuadData_t &data )
{
	CFXQuad *quad = new CFXQuad( data );

	Assert( quad != NULL );

	clienteffects->AddEffect( quad );
}
//-----------------------------------------------------------------------------
// Purpose: Parameter heavy version
//-----------------------------------------------------------------------------
void FX_AddQuad( const Vector &origin, 
				 const Vector &normal, 
				 float startSize, 
				 float endSize, 
				 float sizeBias,
				 float startAlpha, 
				 float endAlpha,
				 float alphaBias,
				 float yaw,
				 float deltaYaw,
				 const Vector &color, 
				 float lifeTime, 
				 const char *shader, 
				 unsigned int flags )
{
	FXQuadData_t data;

	//Setup the data
	data.SetAlpha( startAlpha, endAlpha );
	data.SetScale( startSize, endSize );
	data.SetFlags( flags );
	data.SetMaterial( shader );
	data.SetNormal( normal );
	data.SetOrigin( origin );
	data.SetLifeTime( lifeTime );
	data.SetColor( color[0], color[1], color[2] );
	data.SetScaleBias( sizeBias );
	data.SetAlphaBias( alphaBias );
	data.SetYaw( yaw, deltaYaw );

	//Output it
	FX_AddQuad( data );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a test effect
//-----------------------------------------------------------------------------
void CreateTestEffect( void )
{
	//NOTENOTE: Add any test effects here
	//FX_BulletPass( NULL, NULL );
}

/*
==================================================
UTIL_NearestPointOnLine
==================================================
*/

float UTIL_NearestPointOnLine( const Vector& start, const Vector& end, const Vector& target, Vector& out )
{
	Vector	lineDir, targetDir;
	float	dot;

	//Find the direction of the line
	VectorSubtract( end, start, lineDir );
	VectorNormalize( lineDir );

	//Find the direction of the target to the line's path
	VectorSubtract( target, start, targetDir );
		
	//Find the nearest point to the target on that line
	dot = DotProduct( targetDir, lineDir );
	VectorScale( lineDir, dot, out );
	VectorAdd( start, out, out );

	//Find the distance to the nearest point
	VectorSubtract( out, target, targetDir );
	
	return VectorLength( targetDir );
}

/*
==================================================
FX_PlayerTracer
==================================================
*/

#define	TRACER_BASE_OFFSET	8

void FX_PlayerTracer( Vector& start, Vector& end )
{
	VPROF_BUDGET( "FX_PlayerTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	shotDir, dStart, dEnd;
	float	length;

	//Find the direction of the tracer
	VectorSubtract( end, start, shotDir );
	length = VectorNormalize( shotDir );

	//We don't want to draw them if they're too close to us
	if ( length < 256 )
		return;

	//Randomly place the tracer along this line, with a random length
	VectorMA( start, TRACER_BASE_OFFSET + random->RandomFloat( -24.0f, 64.0f ), shotDir, dStart );
	VectorMA( dStart, ( length * random->RandomFloat( 0.1f, 0.6f ) ), shotDir, dEnd );

	//Create the line
	CFXStaticLine	*t;
	const char		*materialName;

	//materialName = ( random->RandomInt( 0, 1 ) ) ? "effects/tracer_middle" : "effects/tracer_middle2";
	materialName = "effects/spark";

	t = new CFXStaticLine( "Tracer", dStart, dEnd, random->RandomFloat( 0.5f, 0.75f ), 0.01f, materialName, 0 );
	assert( t );

	//Throw it into the list
	clienteffects->AddEffect( t );
}

/*
==================================================
FX_Tracer
==================================================
*/
// Tracer must be this close to be considered for hearing
#define	TRACER_MAX_HEAR_DIST	(6*12)
#define TRACER_SOUND_TIME_MIN	0.10f
#define TRACER_SOUND_TIME_MAX	0.10f


class CBulletWhizTimer : public CAutoGameSystem
{
public:
	void LevelInitPreEntity()
	{
		m_nextWhizTime = 0;
	}

	float	m_nextWhizTime;
};

// Client-side tracking for whiz noises
CBulletWhizTimer g_BulletWhiz;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecStart - 
//			&vecDir - 
//			iTracerType - 
//-----------------------------------------------------------------------------
void FX_TracerSound( const Vector &start, const Vector &end, int iTracerType )
{
	const char *pszSoundName = NULL;

	switch( iTracerType )
	{
	case TRACER_TYPE_DEFAULT:
		pszSoundName = "Bullets.DefaultNearmiss";
		break;

	case TRACER_TYPE_GUNSHIP:
		pszSoundName = "Bullets.GunshipNearmiss";
		break;

	case TRACER_TYPE_STRIDER:
		pszSoundName = "Bullets.StriderNearmiss";
		break;

	default:
		return;
		break;
	}

	if( pszSoundName )
	{
		//Get out shot direction and length
		float totalDist;
		Vector shotDir;

		VectorSubtract( end, start, shotDir );
		totalDist = VectorNormalize( shotDir );

		float dt = g_BulletWhiz.m_nextWhizTime - gpGlobals->curtime;

		//If needed, make a bullet whiz noise
		if ( dt <= 0 )
		{
			Vector	vNear, dStart, dEnd;

			//Find the nearest point to our view on the bullet's path
			float vDist = UTIL_NearestPointOnLine( start, end, MainViewOrigin(), vNear );

			//See if we should play a bullet whizz				
			if ( vDist <= TRACER_MAX_HEAR_DIST )
			{
				CLocalPlayerFilter filter;
				CSoundParameters params;

				if( C_BaseEntity::GetParametersForSound( pszSoundName, params ) )
				{
					enginesound->EmitSound(	filter, SOUND_FROM_WORLD, CHAN_STATIC, params.soundname, params.volume, SNDLVL_TO_ATTN(params.soundlevel), 0, params.pitch, &start, &shotDir, false);
				}

				//Don't play another bullet whiz for this client until this time has run out
				g_BulletWhiz.m_nextWhizTime = gpGlobals->curtime + random->RandomFloat( TRACER_SOUND_TIME_MIN, TRACER_SOUND_TIME_MAX );

			}
		}
	}
}


void FX_Tracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_Tracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	//Don't make small tracers
	float dist;
	Vector dir;

	VectorSubtract( end, start, dir );
	dist = VectorNormalize( dir );

	// Don't make short tracers.
	if ( dist < 256 )
	{
		return;
	}

	float length = random->RandomFloat( 64.0f, 128.0f );
	float life = ( dist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, dir, velocity, length, dist, random->RandomFloat( 0.75f, 0.9f ), life, "effects/spark" );

	if( makeWhiz )
	{
		FX_TracerSound( start, end, TRACER_TYPE_DEFAULT );	
	}
}
