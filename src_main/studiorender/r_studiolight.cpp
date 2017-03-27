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
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "studio.h"
#include "measure_section.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "vector.h"
#include "mathlib.h"
#include <float.h>
#include "cstudiorender.h"


/*
================
R_StudioLighting
================
*/

//-----------------------------------------------------------------------------
// Computes the ambient term
//-----------------------------------------------------------------------------

void CStudioRender::R_LightAmbient( Vector const& normal, Vector4D* pLightBoxColor, Vector &lv )
{
//	MEASURECODE( "R_LightAmbient" );

	// FIXME: change this to take in non-translated normals
	if (m_Config.fullbright != 1)
	{
		VectorScale( normal[0] > 0.f ? pLightBoxColor[0].AsVector3D() : pLightBoxColor[1].AsVector3D(), normal[0]*normal[0], lv );
		VectorMA( lv, normal[1]*normal[1], normal[1] > 0.f ? pLightBoxColor[2].AsVector3D() : pLightBoxColor[3].AsVector3D(), lv );
		VectorMA( lv, normal[2]*normal[2], normal[2] > 0.f ? pLightBoxColor[4].AsVector3D() : pLightBoxColor[5].AsVector3D(), lv );
	}
}


// REMOVED!!  See version 37 if you need it.
//void R_LightStrengthLocal( const Vector& vert, int bone, lightpos_t *light )
//void R_LightEffectsLocal( const lightpos_t *light, const float *normal, int bone, const float *src, float *dest )

//-----------------------------------------------------------------------------
// returns true if we're doing fixed-function lighting
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sets the render state when performing hardware lighting
//-----------------------------------------------------------------------------

float CStudioRender::ComputeGammaError( float f, float overbright )
{
	float gamma = LinearToVertexLight(f) * overbright;
	return (gamma - f);
}

// Fixup the ambient light cube so the hardware can render reasonably
// and so any software lighting behaves similarly
void CStudioRender::R_ComputeFixedUpAmbientCube( Vector4D* lightBoxColor, const Vector* totalBoxColor )
{
	float overbright = 1.0;
	if (m_pMaterialSystemHardwareConfig->SupportsOverbright())
		overbright = m_Config.overbrightFactor;

	for (int i = 0; i < 6; ++i)
	{
		lightBoxColor[i][0] += ComputeGammaError( totalBoxColor[i][0], m_Config.overbrightFactor );
		lightBoxColor[i][1] += ComputeGammaError( totalBoxColor[i][1], m_Config.overbrightFactor );
		lightBoxColor[i][2] += ComputeGammaError( totalBoxColor[i][2], m_Config.overbrightFactor );
	}
}


//-----------------------------------------------------------------------------
// Set up light[i].dot, light[i].falloff, and light[i].delta for all lights given 
// a vertex position "vert".
//-----------------------------------------------------------------------------

void CStudioRender::R_LightStrengthWorld( const Vector& vert, int lightcount, 
										 LightDesc_t* pDesc, lightpos_t *light )
{
//	MEASURECODE( "R_LightStrengthWorld" );

	// distance from light, angle from light's normal

	// NJS: note to self, maybe switch here based on lightcount, so multiple squareroots can be done simeltaneously?
	for ( int i = 0; i < lightcount; i++)
	{
		R_WorldLightDelta( &pDesc[i], vert, light[i].delta );

		light[i].falloff = R_WorldLightDistanceFalloff( &pDesc[i], light[i].delta );

#ifdef NEW_SOFTWARE_LIGHTING
		VectorNormalizeFast( light[i].delta );
#else
		VectorNormalize( light[i].delta );
#endif
		light[i].dot = DotProduct( light[i].delta, pDesc[i].m_Direction );
	}
}


int CStudioRender::R_LightGlintPosition( int index, const Vector& org, Vector& delta, Vector& intensity )
{
	if (index >= m_NumLocalLights)
		return false;

	R_WorldLightDelta( &m_LocalLights[index], org, delta );

	float falloff = R_WorldLightDistanceFalloff( &m_LocalLights[index], delta );

	VectorMultiply( m_LocalLights[index].m_Color, falloff, intensity );
	return true;
}

void CStudioRender::R_WorldLightDelta( const LightDesc_t *wl, const Vector& org, Vector& delta )
{
	switch (wl->m_Type)
	{
		case MATERIAL_LIGHT_POINT:
		case MATERIAL_LIGHT_SPOT:
			VectorSubtract(wl->m_Position, org, delta );
			break;

		case MATERIAL_LIGHT_DIRECTIONAL:
			VectorMultiply(wl->m_Direction, -1, delta );
			break;
		default:
			// Bug: need to return an error
			Assert( 0 );
			break;
	}
}

//#define NO_AMBIENT_CUBE 1

#ifdef NEW_SOFTWARE_LIGHTING

// TODO: cone clipping calc's wont work for boxlight since the player asks for a single point.  Not sure what the volume is.
TEMPLATE_FUNCTION_TABLE( void, LightEffectsWorldFunctionTable, ( const lightpos_t *light, const Vector& normal, const Vector &src, Vector &dest ), 64 )
{
	enum  
	{ 
		LightType1 = ( nArgument & 0x30 ) >> 4,
		LightType2 = ( nArgument & 0x0C ) >> 2,
		LightType3 = ( nArgument & 0x03 )
	};
 
	//	MEASURECODE( "R_LightEffectsWorld" );

	#ifndef NO_AMBIENT_CUBE
		VectorCopy( src, dest );
	#else									  
		dest[0] = dest[1] = dest[2] = 0.f;
	#endif

	// FIXME: lighting effects for normal and position are independant!
	// FIXME: these can be pre-calculated per normal
	if( !g_StudioRender.GetConfig().fullbright )
	{
		float bias = g_StudioRender.GetConfig().modelLightBias * g_StudioRender.GetConfig().overbrightFactor;

		if( LightType1 != MATERIAL_LIGHT_DISABLE )
		{
			float ratio = light[0].falloff * CWorldLightAngleWrapper<LightType1>::WorldLightAngle( &g_StudioRender.m_LocalLights[0], g_StudioRender.m_LocalLights[0].m_Direction, normal, light[0].delta );

			if (ratio > 0)
			{
				// Apply lighting bias to the per-vertex portion of the lighting
				float scale = ratio * bias;
				const float* direction = (float*)&g_StudioRender.m_LocalLights[0].m_Color;
				dest[0] += ( scale * direction[0] );
				dest[1] += ( scale * direction[1] );
				dest[2] += ( scale * direction[2] );

				//VectorMA( dest, m_Config.overbrightFactor * ratio * bias, m_LocalLights[i].m_Color, dest );
			}
		}

		if( LightType2 != MATERIAL_LIGHT_DISABLE )
		{
			float ratio = light[1].falloff * CWorldLightAngleWrapper<LightType2>::WorldLightAngle( &g_StudioRender.m_LocalLights[1], g_StudioRender.m_LocalLights[1].m_Direction, normal, light[1].delta );

			if (ratio > 0)
			{
				// Apply lighting bias to the per-vertex portion of the lighting
				float scale = ratio * bias;
				const float* direction = (float*)&g_StudioRender.m_LocalLights[1].m_Color;
				dest[0] += ( scale * direction[0] );
				dest[1] += ( scale * direction[1] );
				dest[2] += ( scale * direction[2] );

				//VectorMA( dest, m_Config.overbrightFactor * ratio * bias, m_LocalLights[i].m_Color, dest );
			}
		}

		if( LightType3 != MATERIAL_LIGHT_DISABLE )
		{
			float ratio = light[2].falloff * CWorldLightAngleWrapper<LightType1>::WorldLightAngle( &g_StudioRender.m_LocalLights[2], g_StudioRender.m_LocalLights[2].m_Direction, normal, light[2].delta );

			if (ratio > 0)
			{
				// Apply lighting bias to the per-vertex portion of the lighting
				float scale = ratio * bias;
				const float* direction = (float*)&g_StudioRender.m_LocalLights[2].m_Color;
				dest[0] += ( scale * direction[0] );
				dest[1] += ( scale * direction[1] );
				dest[2] += ( scale * direction[2] );

				//VectorMA( dest, m_Config.overbrightFactor * ratio * bias, m_LocalLights[i].m_Color, dest );
			}
		}
	} 
	else
	{
		dest.x = dest.y = dest.z = 1.f;
	}
	
#ifdef NO_AMBIENT_CUBE
	if (0)
#else
	// Do normal gamma correction if we're not using fixed-function T&L
	//if (!m_pMaterialSystemHardwareConfig->SupportsHardwareLighting() ||
	//	m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders())
	if( g_StudioRender.GetSupportsHardwareLighting() || g_StudioRender.GetSupportsVertexAndPixelShaders() )
#endif
	{
		dest[0] = LinearToVertexLight( dest[0] );
		dest[1] = LinearToVertexLight( dest[1] );
		dest[2] = LinearToVertexLight( dest[2] );
	}
	else
	{
		// This should get called when we use fixed-function T&L and have to
		// render part of the model using software.
		float overbrightFactor = 1.0f;
		//if (m_pMaterialSystemHardwareConfig->SupportsOverbright())
		if( g_StudioRender.GetSupportsOverbright() )
			overbrightFactor = 1.0f / g_StudioRender.GetConfig().overbrightFactor;

		// factor in overbrighting....
		dest[0] *= overbrightFactor;
		dest[1] *= overbrightFactor;
		dest[2] *= overbrightFactor;

		// clamp
		if (dest[0] > 1.0f) dest[0] = 1.0f; else if (dest[0] < 0.0f) dest[0] = 0.0f;
		if (dest[1] > 1.0f) dest[1] = 1.0f; else if (dest[1] < 0.0f) dest[1] = 0.0f;
		if (dest[2] > 1.0f) dest[2] = 1.0f; else if (dest[2] < 0.0f) dest[2] = 0.0f;
	}

	Assert( (dest[0] >= 0.f) && (dest[0] <= 1.f) );
	Assert( (dest[1] >= 0.f) && (dest[1] <= 1.f) );
	Assert( (dest[2] >= 0.f) && (dest[2] <= 1.f) );

}


void CStudioRender::R_InitLightEffectsWorld3()
{
	int i;

	Assert( m_NumLocalLights <= 3 && m_NumLocalLights >= 0 );

	int index = 0;

	for( i = 0; i < 3 ; i++ )
	{
		int type = i >= m_NumLocalLights ?  MATERIAL_LIGHT_DISABLE : m_LocalLights[i].m_Type;
		
		index = ( index << 2 ) + type;
	}

	Assert( index >= 0 && index < LightEffectsWorldFunctionTable::count );
	R_LightEffectsWorld3 = /*g_SoftwareLightEffectsWorldFunc[index];*/ LightEffectsWorldFunctionTable::functions[index];
}


void CStudioRender::R_LightEffectsWorld( const lightpos_t *light, const Vector& normal, const Vector &src, Vector &dest )
{
	//	MEASURECODE( "R_LightEffectsWorld" );

#ifndef NO_AMBIENT_CUBE
	VectorCopy( src, dest );
#else									  
	dest[0] = dest[1] = dest[2] = 0.f;
#endif

	// FIXME: lighting effects for normal and position are independant!
	// FIXME: these can be pre-calculated per normal
	if( !m_Config.fullbright )
	{
		float bias = m_Config.modelLightBias * m_Config.overbrightFactor;

		for (int i = 0; i < m_NumLocalLights; i++)
		{
			float ratio = R_WorldLightAngle( &m_LocalLights[i], m_LocalLights[i].m_Direction, normal, light[i].delta ) * light[i].falloff;
			if (ratio > 0)
			{
				// Apply lighting bias to the per-vertex portion of the lighting
				float scale = ratio * bias;
				const float* direction = (float*)&m_LocalLights[i].m_Color;
				dest[0] += ( scale * direction[0] );
				dest[1] += ( scale * direction[1] );
				dest[2] += ( scale * direction[2] );

				//VectorMA( dest, m_Config.overbrightFactor * ratio * bias, m_LocalLights[i].m_Color, dest );
			}
		}
	} 
	else
	{
		VectorFill( dest, 1 );
	}
	
#ifdef NO_AMBIENT_CUBE
	if (0)
#else
	// Do normal gamma correction if we're not using fixed-function T&L
	//if (!m_pMaterialSystemHardwareConfig->SupportsHardwareLighting() ||
	//	m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders())
	if( m_bSupportsHardwareLighting || m_bSupportsVertexAndPixelShaders )
#endif
	{
		dest[0] = LinearToVertexLight( dest[0] );
		dest[1] = LinearToVertexLight( dest[1] );
		dest[2] = LinearToVertexLight( dest[2] );
	}
	else
	{
		// This should get called when we use fixed-function T&L and have to
		// render part of the model using software.
		float overbrightFactor = 1.0f;
		//if (m_pMaterialSystemHardwareConfig->SupportsOverbright())
		if( m_bSupportsOverbright )
			overbrightFactor = 1.0f / m_Config.overbrightFactor;

		// factor in overbrighting....
		dest[0] *= overbrightFactor;
		dest[1] *= overbrightFactor;
		dest[2] *= overbrightFactor;


		// clamp
		if (dest[0] > 1.0f) dest[0] = 1.0f; else if (dest[0] < 0.0f) dest[0] = 0.0f;
		if (dest[1] > 1.0f) dest[1] = 1.0f; else if (dest[1] < 0.0f) dest[1] = 0.0f;
		if (dest[2] > 1.0f) dest[2] = 1.0f; else if (dest[2] < 0.0f) dest[2] = 0.0f;
	}

	Assert( (dest[0] >= 0.f) && (dest[0] <= 1.0f) );
	Assert( (dest[1] >= 0.f) && (dest[1] <= 1.0f) );
	Assert( (dest[2] >= 0.f) && (dest[2] <= 1.0f) );
}

#else

void CStudioRender::R_LightEffectsWorld( const lightpos_t *light, const Vector& normal, const Vector &src, Vector &dest )
{
	//	MEASURECODE( "R_LightEffectsWorld" );

#ifndef NO_AMBIENT_CUBE
	VectorCopy( src, dest );
#else									  
	dest[0] = dest[1] = dest[2] = 0.f;
#endif

	// FIXME: lighting effects for normal and position are independant!
	// FIXME: these can be pre-calculated per normal
	if( !m_Config.fullbright )
	{
		float bias = m_Config.modelLightBias * m_Config.overbrightFactor;

		for (int i = 0; i < m_NumLocalLights; i++)
		{
			float ratio = R_WorldLightAngle( &m_LocalLights[i], m_LocalLights[i].m_Direction, normal, light[i].delta ) * light[i].falloff;
			if (ratio > 0)
			{
				// Apply lighting bias to the per-vertex portion of the lighting
				float scale = ratio * bias;
				const float* direction = (float*)&m_LocalLights[i].m_Color;
				dest[0] += ( scale * direction[0] );
				dest[1] += ( scale * direction[1] );
				dest[2] += ( scale * direction[2] );

				//VectorMA( dest, m_Config.overbrightFactor * ratio * bias, m_LocalLights[i].m_Color, dest );
			}
		}
	} 
	else
	{
		VectorFill( dest, 1 );
	}
	
#ifdef NO_AMBIENT_CUBE
	if (0)
#else
	// Do normal gamma correction if we're not using fixed-function T&L
	//if (!m_pMaterialSystemHardwareConfig->SupportsHardwareLighting() ||
	//	m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders())
	if( m_bSupportsHardwareLighting || m_bSupportsVertexAndPixelShaders )
#endif
	{
		dest[0] = LinearToVertexLight( dest[0] );
		dest[1] = LinearToVertexLight( dest[1] );
		dest[2] = LinearToVertexLight( dest[2] );
	}
	else
	{
		// This should get called when we use fixed-function T&L and have to
		// render part of the model using software.
		float overbrightFactor = 1.0f;
		//if (m_pMaterialSystemHardwareConfig->SupportsOverbright())
		if( m_bSupportsOverbright )
			overbrightFactor = 1.0f / m_Config.overbrightFactor;

		// factor in overbrighting....
		dest[0] *= overbrightFactor;
		dest[1] *= overbrightFactor;
		dest[2] *= overbrightFactor;


		// clamp
		if (dest[0] > 1.0f) dest[0] = 1.0f; else if (dest[0] < 0.0f) dest[0] = 0.0f;
		if (dest[1] > 1.0f) dest[1] = 1.0f; else if (dest[1] < 0.0f) dest[1] = 0.0f;
		if (dest[2] > 1.0f) dest[2] = 1.0f; else if (dest[2] < 0.0f) dest[2] = 0.0f;
	}

	Assert( (dest[0] >= 0.f) && (dest[0] <= 1.0f) );
	Assert( (dest[1] >= 0.f) && (dest[1] <= 1.0f) );
	Assert( (dest[2] >= 0.f) && (dest[2] <= 1.0f) );
}

#endif

/*
  light_direction (light_pos - vertex_pos)
*/

#ifdef NEW_SOFTWARE_LIGHTING

// TODO: move cone calcs to position
// TODO: cone clipping calc's wont work for boxlight since the player asks for a single point.  Not sure what the volume is.
TEMPLATE_FUNCTION_TABLE( float, WorldLightDistanceFalloffFunctionTable, ( const LightDesc_t *wl, const Vector& delta ), 8)
{
	Assert( nArgument != 0 );

	float dist2 = DotProduct( delta, delta );


	// Cull out light beyond this radius
	if (wl->m_Range != 0.f)
	{
		if (dist2 > wl->m_Range * wl->m_Range)
			return 0.0f;
	}

	// The general purpose equation:
	float fTotal = FLT_EPSILON;

	if( nArgument & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 )
		fTotal = wl->m_Attenuation0;

	if( nArgument & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 )
	{
		fTotal += wl->m_Attenuation1 * FastSqrt( dist2 );
	}

	if( nArgument & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 )
	{
		fTotal += wl->m_Attenuation2 * dist2;

	}

	return 1.f / fTotal;
}

float FASTCALL CStudioRender::R_WorldLightDistanceFalloff( const LightDesc_t *wl, const Vector& delta )
{
	// Ensure no invalid flags are set
	Assert( ! ( wl->m_Flags & ~(LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0|LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1|LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2) ) );
	// Clear 'em for release mode just in case.
	int flags = wl->m_Flags & (LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0|LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1|LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2);
	return WorldLightDistanceFalloffFunctionTable::functions[flags](wl, delta);
}

#else

float FASTCALL CStudioRender::R_WorldLightDistanceFalloff( const LightDesc_t *wl, const Vector& delta )
{
	float dist;
	float dist2 = DotProduct( delta, delta );

	// Cull out light beyond this radius
	if (wl->m_Range != 0)
	{
		if (dist2 > wl->m_Range * wl->m_Range)
			return 0.0f;
	}

	switch( wl->m_Flags )
	{
	case LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0:
		// no falloff.
		return 1.0 / wl->m_Attenuation0;

	case LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1:
		// linear falloff.
		dist = FastSqrt( dist2 );
		return 1.0f / ( wl->m_Attenuation1 * dist );

	case LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2:
		// 1/(d*d) falloff
		return InvRSquared(delta);

	default:
		// general case.
		dist = FastSqrt( dist2 );
		return 1.0 / (wl->m_Attenuation0 + wl->m_Attenuation1 * dist + wl->m_Attenuation2 * dist2);
	}
}

/*
  light_normal (lights normal translated to same space as other normals)
  surface_normal
  light_direction_normal | (light_pos - vertex_pos) |
*/
float CStudioRender::R_WorldLightAngle( const LightDesc_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta )
{
	float dot, dot2, ratio = 0;

	switch (wl->m_Type)	
	{
		case MATERIAL_LIGHT_POINT:
			dot = DotProduct( snormal, delta );
			if (dot < 0)
				return 0;
			return dot;

		case MATERIAL_LIGHT_SPOT:
			dot = DotProduct( snormal, delta );
			if (dot < 0)
				return 0;

			dot2 = -DotProduct (delta, lnormal);
			if (dot2 <= wl->m_PhiDot)
				return 0; // outside light cone

			ratio = dot;
			if (dot2 >= wl->m_ThetaDot)
				return ratio;	// inside inner cone

			if ((wl->m_Falloff == 1) || (wl->m_Falloff == 0))
			{
				ratio *= (dot2 - wl->m_PhiDot) / (wl->m_ThetaDot - wl->m_PhiDot);
			}
			else
			{
				ratio *= pow((dot2 - wl->m_PhiDot) / (wl->m_ThetaDot - wl->m_PhiDot), wl->m_Falloff );
			}
			return ratio;

		case MATERIAL_LIGHT_DIRECTIONAL:
			dot2 = -DotProduct( snormal, lnormal );
			if (dot2 < 0)
				return 0;
			return dot2;

		default:
			// Bug: need to return an error
			break;
	} 
	return 0;
}
/*
float FASTCALL CStudioRender::R_WorldLightDistanceFalloff( const LightDesc_t *wl, const Vector& delta )
{
	Assert( wl->m_Flags != 0 );

	float dist2 = DotProduct( delta, delta );

	// Cull out light beyond this radius
	if (wl->m_Range != 0.f)
	{
		if (dist2 > wl->m_Range * wl->m_Range)
			return 0.0f;
	}

	switch( wl->m_Flags )
	{	
	// Don't have to worry about zero case on each of these... the whole point is that they aren't zero
	case LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0:
		// no falloff.
		return 1.f / wl->m_Attenuation0;

	case LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1:
		// linear falloff.
		return 1.f / ( wl->m_Attenuation1 * FastSqrt( dist2 ) );

	case LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2:
		// 1/(d*d) falloff
		return 1.f / (wl->m_Attenuation2 * dist2);

	default:
		// general case.
		return 1.f / (wl->m_Attenuation0 + wl->m_Attenuation1 * FastSqrt( dist2 ) + wl->m_Attenuation2 * dist2);
	}
}
*/
#endif

//-----------------------------------------------------------------------------
// Compute the lighting at a point and normal
//-----------------------------------------------------------------------------

void CStudioRender::ComputeLighting( const Vector* pAmbient, int lightCount,
		LightDesc_t* pLights, Vector const& pt, Vector const& normal, Vector& lighting )
{
	// Set up lightpos[i].dot, lightpos[i].falloff, and lightpos[i].delta for all lights
	static lightpos_t lightpos[MAXLOCALLIGHTS];
	R_LightStrengthWorld( pt, lightCount, pLights, lightpos );

	// FIXME: Need to copy this off into a vector4D array
	int i;
	Vector4D ambient4D[6];
	for ( i = 0; i < 6; ++i )
	{
		VectorCopy( pAmbient[i], ambient4D[i].AsVector3D() );
	}

	// calculate ambientvalues from the ambient cube given a normal.
	R_LightAmbient( normal, ambient4D, lighting );

	for ( i = 0; i < lightCount; ++i)
	{
		float ratio = R_WorldLightAngle( &pLights[i], pLights[i].m_Direction, 
			normal, lightpos[i].delta ) * lightpos[i].falloff;
		if (ratio > 0)
		{
			// Apply lighting bias to the per-vertex portion of the lighting
			VectorMA( lighting, ratio, pLights[i].m_Color, lighting );
		}
	}
}


#if 0
void CStudioRender::ComputeLightingSpotlight( const Vector &pos, const Vector &norm, 
											  lightpos_t &lightpos, LightDesc_t &lightDesc,
											  Vector4D &color )
{
#if 0
	// Set up lightpos[i].dot, lightpos[i].falloff, and lightpos[i].delta for all lights
	R_LightStrengthWorld( pos, lightpos );
	// calculate ambientvalues from the ambient cube given a normal.
	R_LightAmbient( norm, ambientvalues );
	// Calculate color given lightpos_t lightpos, a normal, and the ambient
	// color from the ambient cube calculated above.
	R_LightEffectsWorld( lightpos, norm, ambientvalues, color.AsVector3D() );
	color[3] = r_blend;
#endif

//----
//	R_LightStrengthWorld( pos, lightpos );
//void CStudioRender::R_LightStrengthWorld( const Vector& vert, int lightcount, 
//										 LightDesc_t* pDesc, lightpos_t *light )
//----
	R_WorldLightDelta( &lightDesc, pos, lightpos.delta );

	lightpos.falloff = R_WorldLightDistanceFalloff( &lightDesc, lightpos.delta );

	VectorNormalize( lightpos.delta );
	lightpos.dot = DotProduct( lightpos.delta, lightDesc.m_Direction );

//---
//	R_LightEffectsWorld( lightpos, norm, ambientvalues, color.AsVector3D() );
//---
	float ratio = R_WorldLightAngle( &lightDesc, lightDesc.m_Direction, norm, lightpos.delta ) * lightpos.falloff;
	if (ratio > 0)
	{
		// Apply lighting bias to the per-vertex portion of the lighting
		VectorMA( color.AsVector3D(), m_Config.overbrightFactor * ratio * m_Config.modelLightBias, 
			lightDesc.m_Color, color.AsVector3D() );
	}
}
#endif
