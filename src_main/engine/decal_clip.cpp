//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "decal_clip.h"


// --------------------------------------------------------------------------- //
// Template classes for the clipper.
// --------------------------------------------------------------------------- //
class CPlane_Top
{
public:
	static inline bool Inside( CDecalVert *pVert )					{return pVert->m_tCoords.y < 1;}
	static inline float Clip( CDecalVert *one, CDecalVert *two )	{return (1 - one->m_tCoords.y) / (two->m_tCoords.y - one->m_tCoords.y);}
};

class CPlane_Left
{
public:
	static inline bool Inside( CDecalVert *pVert )					{return pVert->m_tCoords.x > 0;}
	static inline float Clip( CDecalVert *one, CDecalVert *two )	{return one->m_tCoords.x / (one->m_tCoords.x - two->m_tCoords.x);}
};

class CPlane_Right
{
public:
	static inline bool Inside( CDecalVert *pVert )					{return pVert->m_tCoords.x < 1;}
	static inline float Clip( CDecalVert *one, CDecalVert *two )	{return (1 - one->m_tCoords.x) / (two->m_tCoords.x - one->m_tCoords.x);}
};

class CPlane_Bottom
{
public:
	static inline bool Inside( CDecalVert *pVert )					{return pVert->m_tCoords.y > 0;}
	static inline float Clip( CDecalVert *one, CDecalVert *two )	{return one->m_tCoords.y / (one->m_tCoords.y - two->m_tCoords.y);}
};



// --------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------- //
static CDecalVert g_DecalClipVerts[MAX_DECALCLIPVERT];
static CDecalVert g_DecalClipVerts2[MAX_DECALCLIPVERT];




template< class Clipper >
static inline void Intersect( Clipper &clip, CDecalVert *one, CDecalVert *two, CDecalVert *out )
{
	float t = Clipper::Clip( one, two );
	
	VectorLerp( one->m_vPos, two->m_vPos, t, out->m_vPos );
	Vector2DLerp( one->m_LMCoords, two->m_LMCoords, t, out->m_LMCoords );
	Vector2DLerp( one->m_tCoords, two->m_tCoords, t, out->m_tCoords );
}


template< class Clipper >
static inline int SHClip( CDecalVert *g_DecalClipVerts, int vertCount, CDecalVert *out, Clipper &clip )
{
	int j, outCount;
	CDecalVert *s, *p;


	outCount = 0;

	s = &g_DecalClipVerts[ vertCount-1 ];
	for ( j = 0; j < vertCount; j++ ) 
	{
		p = &g_DecalClipVerts[ j ];
		if ( Clipper::Inside( p ) ) 
		{
			if ( Clipper::Inside( s ) ) 
			{
				memcpy( out, p, sizeof(*out) );
				outCount++;
				out++;
			}
			else 
			{
				Intersect( clip, s, p, out );
				out++;
				outCount++;

				memcpy( out, p, sizeof(*out) );
				outCount++;
				out++;
			}
		}
		else 
		{
			if ( Clipper::Inside( s ) ) 
			{
				Intersect( clip, p, s, out );
				out++;
				outCount++;
			}
		}
		s = p;
	}
				
	return outCount;
}


CDecalVert* R_DoDecalSHClip( 
	CDecalVert *pInVerts, 
	CDecalVert *pOutVerts, 
	decal_t *pDecal, 
	int nStartVerts, 
	int *pVertCount )
{
	if ( pOutVerts == NULL )
		pOutVerts = &g_DecalClipVerts[0];

	CPlane_Top top;
	CPlane_Left left;
	CPlane_Right right;
	CPlane_Bottom bottom;

	// Clip the polygon to the decal texture space
	int outCount = SHClip( pInVerts, nStartVerts, &g_DecalClipVerts2[0], top );
	outCount = SHClip( &g_DecalClipVerts2[0], outCount, &g_DecalClipVerts[0], left );
	outCount = SHClip( &g_DecalClipVerts[0], outCount, &g_DecalClipVerts2[0], right );
	outCount = SHClip( &g_DecalClipVerts2[0], outCount, pOutVerts, bottom );

	if ( outCount ) 
	{
		if ( pDecal->flags & FDECAL_CLIPTEST )
		{
			pDecal->flags &= ~FDECAL_CLIPTEST;	// We're doing the test
			
			// If there are exactly 4 verts and they are all 0,1 tex coords, then we've got an unclipped decal
			// A more precise test would be to calculate the texture area and make sure it's one, but this
			// should work as well.
			if ( outCount == 4 )
			{
				int clipped = 0;

				CDecalVert *v = pOutVerts;
				for ( int j = 0; j < outCount && !clipped; j++, v++ )
				{
					float s, t;
					s = v->m_tCoords.x;
					t = v->m_tCoords.y;
					
					if ( (s != 0.0 && s != 1.0) || (t != 0.0 && t != 1.0) )
						clipped = 1;
				}

				// We didn't need to clip this decal, it's a quad covering the full texture space, optimize
				// subsequent frames.
				if ( !clipped )
					pDecal->flags |= FDECAL_NOCLIP;
			}
		}
	}
	
	*pVertCount = outCount;
	return pOutVerts;
}

// Build the initial list of vertices from the surface verts into the global array, 'verts'.
void R_SetupDecalVertsForMSurface( 
	decal_t *pDecal, 
	int surfID, 
	Vector textureSpaceBasis[3],
	CDecalVert *pVerts )
{
	for ( int j = 0; j < MSurf_VertCount( surfID ); j++ )
	{
		int vertIndex = host_state.worldmodel->brush.vertindices[MSurf_FirstVertIndex( surfID )+j];
		Vector& v = host_state.worldmodel->brush.vertexes[vertIndex].position;
		
		pVerts[j].m_vPos = v; // Copy model space coordinates
		// garymcthack - what about m_ParentTexCoords?
		pVerts[j].m_tCoords.x = DotProduct( pVerts[j].m_vPos, textureSpaceBasis[0] ) - pDecal->dx + .5f;
		pVerts[j].m_tCoords.y = DotProduct( pVerts[j].m_vPos, textureSpaceBasis[1] ) - pDecal->dy + .5f;
		pVerts[j].m_LMCoords.Init();
	}
}


//-----------------------------------------------------------------------------
// compute the decal basis based on surface normal, and preferred saxis
//-----------------------------------------------------------------------------

#define SIN_45_DEGREES ( 0.70710678118654752440084436210485f )

void R_DecalComputeBasis( Vector const& surfaceNormal, Vector const* pSAxis, 
								 Vector* textureSpaceBasis )
{
	// s, t, textureSpaceNormal (T cross S = textureSpaceNormal(N))
	//   N     
	//   \   
	//    \     
	//     \  
	//      |---->S
	//      | 
	//		|  
	//      |T    
	// S = textureSpaceBasis[0]
	// T = textureSpaceBasis[1]
	// N = textureSpaceBasis[2]

	// Get the surface normal.
	VectorCopy( surfaceNormal, textureSpaceBasis[2] );

	if (pSAxis)
	{
		// T = S cross N
		CrossProduct( *pSAxis, textureSpaceBasis[2], textureSpaceBasis[1] );

		// Name sure they aren't parallel or antiparallel
		// In that case, fall back to the normal algorithm.
		if ( DotProduct( textureSpaceBasis[1], textureSpaceBasis[1] ) > 1e-6 )
		{
			// S = N cross T
			CrossProduct( textureSpaceBasis[2], textureSpaceBasis[1], textureSpaceBasis[0] );

			VectorNormalizeFast( textureSpaceBasis[0] );
			VectorNormalizeFast( textureSpaceBasis[1] );
			return;
		}

		// Fall through to the standard algorithm for parallel or antiparallel
	}

	// floor/ceiling?
	if( fabs( surfaceNormal[2] ) > SIN_45_DEGREES )
	{
		textureSpaceBasis[0][0] = 1.0f;
		textureSpaceBasis[0][1] = 0.0f;
		textureSpaceBasis[0][2] = 0.0f;

		// T = S cross N
		CrossProduct( textureSpaceBasis[0], textureSpaceBasis[2], textureSpaceBasis[1] );

		// S = N cross T
		CrossProduct( textureSpaceBasis[2], textureSpaceBasis[1], textureSpaceBasis[0] );
	}
	// wall
	else
	{
		textureSpaceBasis[1][0] = 0.0f;
		textureSpaceBasis[1][1] = 0.0f;
		textureSpaceBasis[1][2] = -1.0f;

		// S = N cross T
		CrossProduct( textureSpaceBasis[2], textureSpaceBasis[1], textureSpaceBasis[0] );
		// T = S cross N
		CrossProduct( textureSpaceBasis[0], textureSpaceBasis[2], textureSpaceBasis[1] );
	}

	VectorNormalizeFast( textureSpaceBasis[0] );
	VectorNormalizeFast( textureSpaceBasis[1] );
}


void R_SetupDecalTextureSpaceBasis(
	decal_t *pDecal,
	Vector &vSurfNormal,
	IMaterial *pMaterial,
	Vector textureSpaceBasis[3],
	float decalWorldScale[2] )
{
	// Compute the non-scaled decal basis
	R_DecalComputeBasis( vSurfNormal,
		(pDecal->flags & FDECAL_USESAXIS) ? &pDecal->saxis : 0,
		textureSpaceBasis );

	// world width of decal = ptexture->width / pDecal->scale
	// world height of decal = ptexture->height / pDecal->scale
	// scale is inverse, scales world space to decal u/v space [0,1]
	// OPTIMIZE: Get rid of these divides
	decalWorldScale[0] = pDecal->scale / pMaterial->GetMappingWidth();
	decalWorldScale[1] = pDecal->scale / pMaterial->GetMappingHeight();
	
	VectorScale( textureSpaceBasis[0], decalWorldScale[0], textureSpaceBasis[0] );
	VectorScale( textureSpaceBasis[1], decalWorldScale[1], textureSpaceBasis[1] );
}


// Figure out where the decal maps onto the surface.
void R_SetupDecalClip( 
	CDecalVert* &pOutVerts,
	decal_t *pDecal,
	Vector &vSurfNormal,
	IMaterial *pMaterial,
	Vector textureSpaceBasis[3],
	float decalWorldScale[2] )
{
//	if ( pOutVerts == NULL )
//		pOutVerts = &g_DecalClipVerts[0];

	R_SetupDecalTextureSpaceBasis( pDecal, vSurfNormal, pMaterial, textureSpaceBasis, decalWorldScale );

	// Generate texture coordinates for each vertex in decal s,t space
	// probably should pre-generate this, store it and use it for decal-decal collisions
	// as in R_DecalsIntersect()
	pDecal->dx = DotProduct( pDecal->position, textureSpaceBasis[0] );
	pDecal->dy = DotProduct( pDecal->position, textureSpaceBasis[1] );
}


//-----------------------------------------------------------------------------
// Generate clipped vertex list for decal pdecal projected onto polygon psurf
//-----------------------------------------------------------------------------
CDecalVert* R_DecalVertsClip( 
	CDecalVert *pOutVerts, 
	decal_t *pDecal, 
	int surfID, 
	IMaterial *pMaterial, 
	int *pVertCount )
{
	float decalWorldScale[2];
	Vector textureSpaceBasis[3]; 

	// Figure out where the decal maps onto the surface.
	R_SetupDecalClip( 
		pOutVerts,
		pDecal,
		MSurf_Plane( surfID ).normal,
		pMaterial,
		textureSpaceBasis, decalWorldScale );

	// Build the initial list of vertices from the surface verts.
	R_SetupDecalVertsForMSurface( pDecal, surfID, textureSpaceBasis, g_DecalClipVerts );

	return R_DoDecalSHClip( g_DecalClipVerts, pOutVerts, pDecal, MSurf_VertCount( surfID ), pVertCount );
}
