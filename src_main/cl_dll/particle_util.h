//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// A collection of utility functions and classes for particle systems.
//-----------------------------------------------------------------------------
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef PARTICLE_UTIL_H
#define PARTICLE_UTIL_H

#include "materialsystem/IMesh.h"
#include "particledraw.h"

extern IVEngineClient *engine;

// Lerp between two floating point numbers.
inline float FLerp(float minVal, float maxVal, float t)
{
	return minVal + (maxVal - minVal) * t;
}

inline Vector VecLerp(const Vector &minVal, const Vector &maxVal, float t)
{
	return minVal + (maxVal - minVal) * t;
}

// Get a random floating point number between the two specified numbers.
inline float FRand(float minVal, float maxVal)
{
	return minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
}

// Apply velocity and acceleration to position and acceleration to velocity.
// If you're going to keep acceleration around, you should zero it out after calling this.
inline void PhysicallySimulate(Vector &pos, Vector &velocity, const Vector &acceleration, const float fTimeDelta)
{
	pos = pos + (velocity + (acceleration*fTimeDelta*0.5f)) * fTimeDelta;
	velocity = velocity + acceleration * fTimeDelta;
}


inline Vector GetGravityVector()
{
	return Vector(0, 0, -150);
}


// Render a quad on the screen where you pass in color and size.
// Color and alpha range is 0 to 254.9
// You also get an extra texture coordinate to pass in.
inline void RenderParticle_Color255SizeNormalTCoord3(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const Vector &vNormal,
	const float tCoord
	)
{
	// Don't render totally transparent particles.
	if( alpha < 0.5f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha );

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 0, 1.0f, tCoord );
	pBuilder->Normal3f( VectorExpand(vNormal) );
	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 0, 0, tCoord );
	pBuilder->Normal3f( VectorExpand(vNormal) );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 1.0f, 0, tCoord );
	pBuilder->Normal3f( VectorExpand(vNormal) );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 1.0f, 1.0f, tCoord );
	pBuilder->Normal3f( VectorExpand(vNormal) );
 	pBuilder->AdvanceVertex();
}


// Render a quad on the screen where you pass in color and size.
// Color and alpha range is 0 to 254.9
// You also get an extra texture coordinate to pass in.
inline void RenderParticle_Color255SizeTCoord3(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const float tCoord
	)
{
	// Don't render totally transparent particles.
	if( alpha < 0.5f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha );

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 0, 1.0f, tCoord );
	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 0, 0, tCoord );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 1.0f, 0, tCoord );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 1.0f, 1.0f, tCoord );
 	pBuilder->AdvanceVertex();
}


// Render a quad on the screen where you pass in color and size.
// Color and alpha range is 0 to 254.9
// You also get an extra texture coordinate to pass in.
inline void RenderParticle_Color255SizeSpecularTCoord3(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const unsigned char *specular,
	const float tCoord
	)
{
	// Don't render totally transparent particles.
	if( alpha < 0.5f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha );

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 0, 1.0f, tCoord );
	pBuilder->Specular3ubv( specular );
	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 0, 0, tCoord );
	pBuilder->Specular3ubv( specular );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 1.0f, 0, tCoord );
	pBuilder->Specular3ubv( specular );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord3f( 0, 1.0f, 1.0f, tCoord );
	pBuilder->Specular3ubv( specular );
 	pBuilder->AdvanceVertex();
}


// Render a quad on the screen where you pass in color and size.
// Color and alpha range is 0 to 254.9
inline void RenderParticle_Color255Size(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size)
{
	// Don't render totally transparent particles.
	if( alpha < 0.5f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha );

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
 	pBuilder->AdvanceVertex();
}


// Render a quad on the screen where you pass in color and size.
// Color and alpha range is 0 to 254.9
inline void RenderParticle_Color255SizeNormal(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const Vector &vNormal )
{
	// Don't render totally transparent particles.
	if( alpha < 0.5f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha );

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
	pBuilder->Normal3fv( (float*)&vNormal );
	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 0 );
	pBuilder->Normal3fv( (float*)&vNormal );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
	pBuilder->Normal3fv( (float*)&vNormal );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
	pBuilder->Normal3fv( (float*)&vNormal );
 	pBuilder->AdvanceVertex();
}


// Render a quad on the screen where you pass in color and size.
// Color and alpha range is 0 to 254.9
// Angle is in radians.
inline void RenderParticle_Color255SizeNormalAngle(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const Vector &vNormal,
	const float angle )
{
	// Don't render totally transparent particles.
	if( alpha < 0.5f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha );

	float ca = (float)cos(angle);
	float sa = (float)sin(angle);

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x + (-ca + sa) * size, pos.y + (-sa - ca) * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
	pBuilder->Normal3fv( (float*)&vNormal );
	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x + (-ca - sa) * size, pos.y + (-sa + ca) * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 0 );
	pBuilder->Normal3fv( (float*)&vNormal );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x + (ca - sa)  * size, pos.y + (sa + ca)  * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
	pBuilder->Normal3fv( (float*)&vNormal );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x + (ca + sa)  * size, pos.y + (sa - ca)  * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
	pBuilder->Normal3fv( (float*)&vNormal );
 	pBuilder->AdvanceVertex();
}


// Render a quad on the screen where you pass in color and size.
inline void RenderParticle_ColorSize(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size
	)
{
	// Don't render totally transparent particles.
	if( alpha < 0.001f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x * 254.9f );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y * 254.9f );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z * 254.9f );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha * 254.9f );

	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
 	pBuilder->AdvanceVertex();
}


inline void RenderParticle_ColorSizeAngle(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const float angle)
{
	// Don't render totally transparent particles.
	if(alpha < 0.001f)
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x * 254.9f );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y * 254.9f );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z * 254.9f );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha * 254.9f );

	float ca = (float)cos(angle);
	float sa = (float)sin(angle);

	pBuilder->Position3f( pos.x + (-ca + sa) * size, pos.y + (-sa - ca) * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x + (-ca - sa) * size, pos.y + (-sa + ca) * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 0, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x + (ca - sa)  * size, pos.y + (sa + ca)  * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x + (ca + sa)  * size, pos.y + (sa - ca)  * size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
 	pBuilder->AdvanceVertex();
}

inline void RenderParticle_ColorSizeAngles(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size,
	const QAngle &angles)
{
	// Don't render totally transparent particles.
	if(alpha < 0.001f)
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x * 254.9f );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y * 254.9f );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z * 254.9f );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha * 254.9f );

	Vector vNorm,vWidth,vHeight;
	AngleVectors(angles,&vNorm,&vWidth,&vHeight);

	Vector vVertex = pos;
	pBuilder->Position3f( vVertex.x , vVertex.y , vVertex.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3f( VectorExpand(vNorm) );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
 	pBuilder->AdvanceVertex();

	vVertex = vVertex + vWidth*size;
	pBuilder->Position3f( vVertex.x, vVertex.y, vVertex.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3f( VectorExpand(vNorm) );
	pBuilder->TexCoord2f( 0, 0, 0 );
 	pBuilder->AdvanceVertex();

	vVertex = vVertex + vHeight*size;
	pBuilder->Position3f( vVertex.x, vVertex.y , vVertex.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3f( VectorExpand(vNorm) );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
 	pBuilder->AdvanceVertex();

	vVertex = vVertex - vWidth*size;
	pBuilder->Position3f( vVertex.x, vVertex.y, vVertex.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3f( VectorExpand(vNorm) );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
 	pBuilder->AdvanceVertex();
}

inline float GetAlphaDistanceFade(
	const Vector &pos,
	const float fadeNearDist,
	const float fadeFarDist)
{
	if(-pos.z > fadeFarDist)
	{
		return 1;
	}
	else if(-pos.z > fadeNearDist)
	{
		return (-pos.z - fadeNearDist) / (fadeFarDist - fadeNearDist);
	}
	else
	{
		return 0;
	}
}


inline Vector WorldGetLightForPoint(const Vector &vPos, bool bClamp)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return Vector(1,1,1);
	#else
		extern Vector g_vecRenderOrigin;
		return engine->GetLightForPoint(vPos, bClamp);
	#endif
}


// This class triggers events at a specified rate. Just call NextEvent() and do an event until it 
// returns false. For example, if you want to spawn particles 10 times per second, do this:
// pTimer->SetRate(10);
// float tempDelta = fTimeDelta;
// while(pTimer->NextEvent(tempDelta))
//		spawn a particle
class TimedEvent
{
public:
				TimedEvent()
				{
					m_TimeBetweenEvents = -1;
					m_fNextEvent = 0;
				}

	// Rate is in events per second (ie: rate of 15 will trigger 15 events per second).
	inline void	Init(float rate)			
	{
		m_TimeBetweenEvents = 1.0f / rate;
		m_fNextEvent = 0;
	}
	
	inline void ResetRate(float rate)
	{
		m_TimeBetweenEvents = 1.0f / rate;
	}

	inline bool NextEvent(float &curDelta)
	{
		// If this goes off, you didn't call Init().
		Assert( m_TimeBetweenEvents != -1 );

		if(curDelta >= m_fNextEvent)
		{
			curDelta -= m_fNextEvent;
			
			m_fNextEvent = m_TimeBetweenEvents;
			return true;
		}
		else
		{
			m_fNextEvent -= curDelta;
			return false;
		}
	}

private:
	float		m_TimeBetweenEvents;
	float		m_fNextEvent;	// When the next event should be triggered.
};


#endif



