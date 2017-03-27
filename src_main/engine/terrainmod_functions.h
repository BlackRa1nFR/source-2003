//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:  Terrain modification classes.  One per mod type.
//
//=============================================================================

#ifndef TERRAINMOD_FUNCTIONS_H
#define TERRAINMOD_FUNCTIONS_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "terrainmod.h"
#include "vmatrix.h"

//-----------------------------------------------------------------------------
// Terrain Modification Sphere
//-----------------------------------------------------------------------------
class CTerrainMod_Sphere : public ITerrainMod
{
public:

	// ITerrainMod overrides.
	void	Init( const CTerrainModParams &params );
	bool	ApplyMod( Vector &vec, Vector const &vOriginalPos );
	bool	ApplyModAtMorphTime( Vector &vecTargetPos, const Vector&vecOriginalPos, 
		                         float flCurrentTime, float flMorphTime );
	void	GetBBox( Vector &bbMin, Vector &bbMax );

private:

	Vector	m_vCenter;
	Vector	m_BBMin;
	Vector	m_BBMax;

	float	m_flRadius;
};

//-----------------------------------------------------------------------------
// Terrain Modification Suck
//-----------------------------------------------------------------------------
class CTerrainMod_Suck : public ITerrainMod
{
public:

	// ITerrainMod overrides.
	void	Init( const CTerrainModParams &params );
	bool	ApplyMod( Vector &vec, Vector const &vOriginalPos );
	bool	ApplyModAtMorphTime( Vector &vecTargetPos, const Vector&vecOriginalPos, 
		                         float flCurrentTime, float flMorphTime );
	void	GetBBox( Vector &bbMin, Vector &bbMax );

private:

	Vector			m_vCenter;
	Vector			m_BBMin;
	Vector			m_BBMax;

	float			m_flRadius;
	float			m_flStrength;

	bool			m_bSuckToNormal;
	bool			m_bStayAboveOriginal;

	// If using m_bSuckToNormal, vector positions are transformed so that 
	// CTerrainModParams::m_vNormal lies along (0,0,1).
	VMatrix			m_mNormalTransform;
	VMatrix			m_mInvNormalTransform;
};

//-----------------------------------------------------------------------------
// Terrain Modification Axial-Aligned Bounding Box
//-----------------------------------------------------------------------------
class CTerrainMod_AABB : public ITerrainMod
{
public:

	// ITerrainMod interface implementation.
	void	Init( const CTerrainModParams &params );
	bool	ApplyMod( Vector &vecTargetPos, const Vector &vecOriginalPos );
	bool	ApplyModAtMorphTime( Vector &vecTargetPos, const Vector&vecOriginalPos, 
		                         float flCurrentTime, float flMorphTime );
	void	GetBBox( Vector &bbMin, Vector &bbMax );

private:

	Vector		m_vecCenter;
	Vector		m_vecBBMin;
	Vector		m_vecBBMax;

	float		m_flMorphTime;
};

// Setup a terrain mod given the parameters.
ITerrainMod *SetupTerrainMod( TerrainModType type, const CTerrainModParams &params );

#endif // TERRAINMOD_FUNCTIONS_H
