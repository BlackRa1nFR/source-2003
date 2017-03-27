//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "terrainmod_functions.h"
#include "mathlib.h"
#include "convar.h"

// This cvar is here for now because it is shared by the client and the server.
ConVar disp_modlimit_side( "disp_modlimit", "80" );
ConVar disp_modlimit_up( "disp_modlimit_up", "80" );
ConVar disp_modlimit_down( "disp_modlimit_down", "20" );

inline void LockDist( Vector &vec, Vector const &vOriginalPos, bool bStayAboveOriginal )
{
	// Clamp the distance in the XY plane.
	Vector vTo;
	VectorSubtract( vec, vOriginalPos, vTo );
	vTo.z = 0;
	float len = vTo.Length();
	if( len > disp_modlimit_side.GetFloat() )
	{
		vTo *= (disp_modlimit_side.GetFloat() / len);
		vec.x = vOriginalPos.x + vTo.x;
		vec.y = vOriginalPos.y + vTo.y; // leave Z intact
	}

	// Clamp the upwards distance.
	if( vec.z - vOriginalPos.z > disp_modlimit_up.GetFloat() )
		vec.z = vOriginalPos.z + disp_modlimit_up.GetFloat();

	// Clamp the downwards distance.
	float fMaxLowerDist = bStayAboveOriginal ? 0 : disp_modlimit_down.GetFloat();
	if( vOriginalPos.z - vec.z > fMaxLowerDist )
		vec.z = vOriginalPos.z - fMaxLowerDist;
}

// ----------------------------------------------------------------------------- //
// CTerrainMod_Sphere
// ----------------------------------------------------------------------------- //
void CTerrainMod_Sphere::Init( CTerrainModParams const &params )
{
	m_vCenter = params.m_vCenter;
	m_flRadius = params.m_flRadius;

	m_BBMin.Init( m_vCenter.x - m_flRadius, m_vCenter.y - m_flRadius, m_vCenter.z - m_flRadius );
	m_BBMax.Init( m_vCenter.x + m_flRadius, m_vCenter.y + m_flRadius, m_vCenter.z + m_flRadius );
}

bool CTerrainMod_Sphere::ApplyMod( Vector &vecTargetPos, Vector const &vecOriginalPos )
{
	if( vecTargetPos.x < m_BBMin.x || vecTargetPos.y < m_BBMin.y || vecTargetPos.z < m_BBMin.z ||
		vecTargetPos.x > m_BBMax.x || vecTargetPos.y > m_BBMax.y || vecTargetPos.z > m_BBMax.z )
	{
		return false;
	}

	Vector vDir = vecTargetPos - m_vCenter;

	float flDist = vDir.Length();
	if( flDist < m_flRadius )
	{
		VectorNormalize( vDir );
		vecTargetPos = vecTargetPos + vDir * m_flRadius;
		
		LockDist( vecTargetPos, vecOriginalPos, false );
		return true;
	}
	else
	{
		return false;
	}
}

bool CTerrainMod_Sphere::ApplyModAtMorphTime( Vector &vecTargetPos, const Vector &vecOriginalPos, 
										      float flCurrentTime, float flMorphTime )
{
	return false;
}

void CTerrainMod_Sphere::GetBBox( Vector &vecBBMin, Vector &vecBBMax )
{
	vecBBMin = m_BBMin;
	vecBBMax = m_BBMax;
}

// ----------------------------------------------------------------------------- //
// CTerrainMod_Suck
// ----------------------------------------------------------------------------- //
void CTerrainMod_Suck::Init( CTerrainModParams const &params )
{
	m_vCenter = params.m_vCenter;
	m_flRadius = params.m_flRadius;
	m_flStrength = params.m_flStrength;
	m_bStayAboveOriginal = !!(params.m_Flags & CTerrainModParams::TMOD_STAYABOVEORIGINAL);

	float rad = max( m_flRadius, 300 );
	m_BBMin.Init( m_vCenter.x - rad, m_vCenter.y - rad, m_vCenter.z - rad );
	m_BBMax.Init( m_vCenter.x + rad, m_vCenter.y + rad, m_vCenter.z + rad );

	if( params.m_Flags & CTerrainModParams::TMOD_SUCKTONORMAL )
	{
		m_bSuckToNormal = true;

		QAngle vAngles;
		Vector vecs[3];
		vecs[0] = params.m_vNormal;
		VectorAngles( vecs[0], vAngles );
		AngleVectors( vAngles, NULL, &vecs[1], &vecs[2] );

		m_mNormalTransform.Init(
			-vecs[2].x, -vecs[2].y, -vecs[2].z, 0,
			-vecs[1].x, -vecs[1].y, -vecs[1].z, 0,
			vecs[0].x, vecs[0].y, vecs[0].z, 0,
			0, 0, 0, 1 );

		m_mInvNormalTransform = m_mNormalTransform.Transpose();

		m_vCenter = m_mNormalTransform * m_vCenter;
	}
	else
	{
		m_bSuckToNormal = false;
	}
}

float ApplyGain( float flPercent )
{
	if( flPercent < 0.5f )
	{
		return pow( 2*flPercent, 2.32193f ) * 0.5f; // 2.32193 = log(0.2) / log(0.5)
	}
	else
	{
		return 1 - 0.5f * pow( 2 - 2*flPercent, 2.32193f );
	}
}
					  
bool CTerrainMod_Suck::ApplyMod( Vector &vecTargetPos, Vector const &vecOriginalPos )
{
	if( vecTargetPos.x < m_BBMin.x || vecTargetPos.y < m_BBMin.y || vecTargetPos.z < m_BBMin.z ||
		vecTargetPos.x > m_BBMax.x || vecTargetPos.y > m_BBMax.y || vecTargetPos.z > m_BBMax.z )
	{
		return false;
	}

	Vector vPos, vOriginalPos;
	if( m_bSuckToNormal )
	{
		// put in a space where +z moves along the normal
		m_mNormalTransform.V3Mul( vecTargetPos, vPos );
		m_mNormalTransform.V3Mul( vecOriginalPos, vOriginalPos );
	}
	else
	{
		vPos = vecTargetPos;
		vOriginalPos = vecOriginalPos;
	}

	Vector vDir = vPos - m_vCenter;
	float flDist = vDir.AsVector2D().Length();
	if( flDist < m_flRadius && vPos.z < m_vCenter.z )
	{
		float flPercent = (m_flRadius - flDist) / m_flRadius;
		flPercent = ApplyGain( flPercent ); // (smooth out near 0 and 1)

		vPos.z += m_flStrength * flPercent;
		vPos.z = min( vPos.z, m_vCenter.z );

		// Clamp the distance.
		LockDist( vPos, vOriginalPos, m_bStayAboveOriginal );
		
		// Transform back to the original space.
		if( m_bSuckToNormal )		
			m_mInvNormalTransform.V3Mul( vPos, vecTargetPos );
		else
			vecTargetPos = vPos;

		return true;
	}
	else
	{
		return false;
	}
}

bool CTerrainMod_Suck::ApplyModAtMorphTime( Vector &vecTargetPos, const Vector &vecOriginalPos, 
										    float flCurrentTime, float flMorphTime )
{
	return false;
}

void CTerrainMod_Suck::GetBBox( Vector &vecBBMin, Vector &vecBBMax )
{
	vecBBMin = m_BBMin;
	vecBBMax = m_BBMax;
}

//-----------------------------------------------------------------------------
// Terrain modification axial-aligned bounding box functions.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Initialize the axial-aligned bounding box terrain modifier.
//-----------------------------------------------------------------------------
void CTerrainMod_AABB::Init( const CTerrainModParams &params )
{
	// Get the center of the modifier.
	VectorCopy( params.m_vCenter, m_vecCenter );

	m_vecBBMin = params.m_vecMin;
	m_vecBBMax = params.m_vecMax;

	// Get the modifier morph time.
	m_flMorphTime = params.m_flMorphTime;
}

//-----------------------------------------------------------------------------
// Purpose: Apply the terrain modifier fully.  Move from the original position to
//          the target position.
//-----------------------------------------------------------------------------
bool CTerrainMod_AABB::ApplyMod( Vector &vecTargetPos, const Vector &vecOriginalPos )
{
	// Check to see if the target position or the current position of the vertex
	// is inside of the modifier bounding area.
	if( vecTargetPos.x < m_vecBBMin.x || vecTargetPos.y < m_vecBBMin.y || vecTargetPos.z < m_vecBBMin.z ||
		vecTargetPos.x > m_vecBBMax.x || vecTargetPos.y > m_vecBBMax.y || vecTargetPos.z > m_vecBBMax.z )
	{
		return false;
	}

	// For a quick and dirty test, just push the vertex straight up/down to the bottom of the bounding
	// box.
	vecTargetPos.z = m_vecCenter.z;
	return true;	
}

//-----------------------------------------------------------------------------
// Purpose: Apply the terrain modifier over time.  Move fromt the original position
//          toward the target position.
//-----------------------------------------------------------------------------
bool CTerrainMod_AABB::ApplyModAtMorphTime( Vector &vecTargetPos, const Vector &vecOriginalPos, 
										    float flCurrentTime, float flMorphTime )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return the bounding box of the axial-aligned bounding box terrain
//          modifier.
//-----------------------------------------------------------------------------
void CTerrainMod_AABB::GetBBox( Vector &vecBBMin, Vector &vecBBMax )
{
	VectorCopy( m_vecBBMin, vecBBMin );
	VectorCopy( m_vecBBMax, vecBBMax );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ITerrainMod* SetupTerrainMod( TerrainModType type, CTerrainModParams const &params )
{
	switch ( type )
	{
	case TMod_Sphere:
		{
			static CTerrainMod_Sphere mod;
			mod.Init( params );
			return &mod;
		}
	case TMod_Suck:
		{
			static CTerrainMod_Suck mod;
			mod.Init( params );
			return &mod;
		}
	case TMod_AABB:
		{
			static CTerrainMod_AABB mod;
			mod.Init( params );
			return &mod;
		}
	default:
		{
			return NULL;
		}
	}
}

