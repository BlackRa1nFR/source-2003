//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// All the code associated with adding decals to studio models
//=============================================================================

#include "cstudiorender.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "mathlib.h"
#include "optimize.h"
#include "cmodel.h"
#include "materialsystem/imaterialvar.h"

#include "tier0/vprof.h"
//-----------------------------------------------------------------------------
// Decal triangle clip flags
//-----------------------------------------------------------------------------

enum
{
	DECAL_CLIP_MINUSU	= 0x1,
	DECAL_CLIP_MINUSV	= 0x2,
	DECAL_CLIP_PLUSU	= 0x4,
	DECAL_CLIP_PLUSV	= 0x8,
};


//-----------------------------------------------------------------------------
// Triangle clipping state
//-----------------------------------------------------------------------------

struct DecalClipState_t
{
	// Number of used vertices
	int m_VertCount;

	// Indices into the clip verts array of the used vertices
	int m_Indices[2][7];

	// Helps us avoid copying the m_Indices array by using double-buffering
	bool m_Pass;

	// Add vertices we've started with and had to generate due to clipping
	int m_ClipVertCount;
	DecalVertex_t	m_ClipVerts[16];

	// Union of the decal triangle clip flags above for each vert
	int m_ClipFlags[16];
};


//-----------------------------------------------------------------------------
//
// Lovely decal code begins here... ABANDON ALL HOPE YE WHO ENTER!!!
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Create, destroy list of decals for a particular model
//-----------------------------------------------------------------------------
StudioDecalHandle_t CStudioRender::CreateDecalList( studiohwdata_t *pHardwareData )
{
	StudioDecalHandle_t handle = m_DecalList.AddToTail();
	m_DecalList[handle].m_pHardwareData = pHardwareData;
	m_DecalList[handle].m_pLod = new DecalLod_t[pHardwareData->m_NumLODs];

	for (int i = pHardwareData->m_NumLODs; --i >= 0; )
	{
		m_DecalList[handle].m_pLod[i].m_FirstMaterial = m_DecalMaterial.InvalidIndex();
	}

	return handle;
}

void CStudioRender::DestroyDecalList( StudioDecalHandle_t handle )
{
	if (handle != STUDIORENDER_DECAL_INVALID)
	{
		// Clean up 
		for (int i = m_DecalList[handle].m_pHardwareData->m_NumLODs; --i >= 0; )
		{
			// Blat out all geometry associated with all materials
			unsigned short mat = m_DecalList[handle].m_pLod[i].m_FirstMaterial;
			unsigned short next;
			while (mat != m_DecalMaterial.InvalidIndex())
			{
				next = m_DecalMaterial.Next(mat);

				m_DecalMaterial.Remove(mat);

				mat = next;
			}
		}

		delete[] m_DecalList[handle].m_pLod;
		m_DecalList.Remove( handle );
	}
}


//-----------------------------------------------------------------------------
// Transformation/Rotation for decals
//-----------------------------------------------------------------------------

inline bool CStudioRender::IsFrontFacing( const Vector& norm, mstudioboneweight_t *pboneweight )
{
	// NOTE: This only works to rotate normals if there's no scale in the
	// pose to world transforms. If we ever add scale, we'll need to
	// multiply by the inverse transpose of the pose to decal

	float z;
	if (pboneweight->numbones == 1)
	{
		z = DotProduct( norm.Base(), m_PoseToDecal[pboneweight->bone[0]][2] );
	}
	else
	{
		float zbone;

		z = 0;
		for (int i = 0; i < pboneweight->numbones; i++)
		{
			zbone = DotProduct( norm.Base(), m_PoseToDecal[pboneweight->bone[i]][2] );
			z += zbone * pboneweight->weight[i];
		}
	}

	return ( z >= 0.0f );
}

inline bool CStudioRender::TransformToDecalSpace( DecalBuildInfo_t& build, const Vector& pos, 
						mstudioboneweight_t *pboneweight, Vector2D& uv )
{
	// NOTE: This only works to rotate normals if there's no scale in the
	// pose to world transforms. If we ever add scale, we'll need to
	// multiply by the inverse transpose of the pose to world

	if (pboneweight->numbones == 1)
	{
		uv.x = DotProduct( pos.Base(), m_PoseToDecal[pboneweight->bone[0]][0] ) + 
			m_PoseToDecal[pboneweight->bone[0]][0][3];
		uv.y = DotProduct( pos.Base(), m_PoseToDecal[pboneweight->bone[0]][1] ) + 
			m_PoseToDecal[pboneweight->bone[0]][1][3];
	}
	else
	{
		uv.x = uv.y = 0;
		float ubone, vbone;
		for (int i = 0; i < pboneweight->numbones; i++)
		{
			ubone = DotProduct( pos.Base(), m_PoseToDecal[pboneweight->bone[i]][0] ) + 
				m_PoseToDecal[pboneweight->bone[i]][0][3];
			vbone = DotProduct( pos.Base(), m_PoseToDecal[pboneweight->bone[i]][1] ) + 
				m_PoseToDecal[pboneweight->bone[i]][1][3];

			uv.x += ubone * pboneweight->weight[i];
			uv.y += vbone * pboneweight->weight[i];
		}
	}

	if (!build.m_NoPokeThru)
		return true;

	// No poke thru? do culling....
	float z;
	if (pboneweight->numbones == 1)
	{
		z = DotProduct( pos.Base(), m_PoseToDecal[pboneweight->bone[0]][2] ) + 
			m_PoseToDecal[pboneweight->bone[0]][2][3];
	}
	else
	{
		z = 0;
		float zbone;
		for (int i = 0; i < pboneweight->numbones; i++)
		{
			zbone = DotProduct( pos.Base(), m_PoseToDecal[pboneweight->bone[i]][2] ) + 
				m_PoseToDecal[pboneweight->bone[i]][2][3];
			z += zbone * pboneweight->weight[i];
		}
	}

	return (fabs(z) < build.m_Radius );
}


//-----------------------------------------------------------------------------
// Projects a decal onto a mesh
//-----------------------------------------------------------------------------
void CStudioRender::ProjectDecalOntoMesh( DecalBuildInfo_t& build )
{
	float invRadius = (build.m_Radius != 0.0f) ? 1.0f / build.m_Radius : 1.0f;

	DecalBuildVertexInfo_t* pVertexInfo = build.m_pVertexInfo;

	// For this to work, the plane and intercept must have been transformed
	// into pose space. Also, we'll not be bothering with flexes.
	for ( int j=0; j < build.m_pMesh->numvertices; ++j )
	{
		mstudiovertex_t &vert = *build.m_pMesh->Vertex(j);

		// No decal vertex yet...
		pVertexInfo[j].m_VertexIndex = 0xFFFF;

		// We need to know if the normal is pointing in the negative direction
		// if so, blow off all triangles connected to that vertex.
		pVertexInfo[j].m_FrontFacing = IsFrontFacing( vert.m_vecNormal, &vert.m_BoneWeights );
		if (!pVertexInfo[j].m_FrontFacing)
		{
			continue;
		}

		bool inValidArea = TransformToDecalSpace( build, vert.m_vecPosition, &vert.m_BoneWeights, pVertexInfo[j].m_UV );
		pVertexInfo[j].m_InValidArea = inValidArea;

		pVertexInfo[j].m_UV *= invRadius * 0.5f;
		pVertexInfo[j].m_UV[0] += 0.5f;
		pVertexInfo[j].m_UV[1] += 0.5f;
	}
}


//-----------------------------------------------------------------------------
// Computes clip flags
//-----------------------------------------------------------------------------
inline int ComputeClipFlags( Vector2D const& uv )
{
	// Otherwise we gotta do the test
	int flags = 0;

	if (uv.x < 0.0f)
		flags |= DECAL_CLIP_MINUSU;
	else if (uv.x > 1.0f)
		flags |= DECAL_CLIP_PLUSU;

	if (uv.y < 0.0f)
		flags |= DECAL_CLIP_MINUSV;
	else if (uv.y > 1.0f )
		flags |= DECAL_CLIP_PLUSV;

	return flags;
}

inline int CStudioRender::ComputeClipFlags( DecalBuildVertexInfo_t* pVertexInfo, int i )
{
	return ::ComputeClipFlags( pVertexInfo[i].m_UV );
}


//-----------------------------------------------------------------------------
// Creates a new vertex where the edge intersects the plane
//-----------------------------------------------------------------------------
static int IntersectPlane( DecalClipState_t& state, int start, int end, 
						    int normalInd, float val )
{
	DecalVertex_t& startVert = state.m_ClipVerts[start];
	DecalVertex_t& endVert = state.m_ClipVerts[end];

	Vector2D dir;
	Vector2DSubtract( endVert.m_TexCoord, startVert.m_TexCoord, dir );
	Assert( dir[normalInd] != 0.0f );
	float t = (val - startVert.m_TexCoord[normalInd]) / dir[normalInd];
				 
	// Allocate a clipped vertex
	DecalVertex_t& out = state.m_ClipVerts[state.m_ClipVertCount];
	int newVert = state.m_ClipVertCount++;

	// The clipped vertex has no analogue in the original mesh
	out.m_MeshVertexIndex = 0xFFFF;
	out.m_Mesh = 0xFFFF;
	out.m_Model = 0xFFFF;
	out.m_Body = 0xFFFF;

	// Interpolate position
	out.m_Position[0] = startVert.m_Position[0] * (1.0 - t) + endVert.m_Position[0] * t;
	out.m_Position[1] = startVert.m_Position[1] * (1.0 - t) + endVert.m_Position[1] * t;
	out.m_Position[2] = startVert.m_Position[2] * (1.0 - t) + endVert.m_Position[2] * t;

	// Interpolate normal
	out.m_Normal[0] = startVert.m_Position[0] * (1.0 - t) + endVert.m_Position[0] * t;
	out.m_Normal[1] = startVert.m_Position[1] * (1.0 - t) + endVert.m_Position[1] * t;
	out.m_Normal[2] = startVert.m_Position[2] * (1.0 - t) + endVert.m_Position[2] * t;
	VectorNormalize( out.m_Normal );

	// Interpolate texture coord
	Vector2DLerp( startVert.m_TexCoord, endVert.m_TexCoord, t, out.m_TexCoord );

	// Compute the clip flags baby...
	state.m_ClipFlags[newVert] = ComputeClipFlags( out.m_TexCoord );

	return newVert;
}

//-----------------------------------------------------------------------------
// Clips a triangle against a plane, use clip flags to speed it up
//-----------------------------------------------------------------------------

static void ClipTriangleAgainstPlane( DecalClipState_t& state, int normalInd, int flag, float val )
{
	// FIXME: Could compute the & of all the clip flags of all the verts
	// as we go through the loop to do another early out

	// Ye Olde Sutherland-Hodgman clipping algorithm
	int outVertCount = 0;
	int start = state.m_Indices[state.m_Pass][state.m_VertCount - 1];
	bool startInside = (state.m_ClipFlags[start] & flag) == 0;
	for (int i = 0; i < state.m_VertCount; ++i)
	{
		int end = state.m_Indices[state.m_Pass][i];

		bool endInside = (state.m_ClipFlags[end] & flag) == 0;
		if (endInside)
		{
			if (!startInside)
			{
				int clipVert = IntersectPlane( state, start, end, normalInd, val );
				state.m_Indices[!state.m_Pass][outVertCount++] = clipVert;
			}
			state.m_Indices[!state.m_Pass][outVertCount++] = end;
		}
		else
		{
			if (startInside)
			{
				int clipVert = IntersectPlane( state, start, end, normalInd, val );
				state.m_Indices[!state.m_Pass][outVertCount++] = clipVert;
			}
		}
		start = end;
		startInside = endInside;
	}

	state.m_Pass = !state.m_Pass;
	state.m_VertCount = outVertCount;
}


//-----------------------------------------------------------------------------
// Converts a mesh index to a DecalVertex_t
//-----------------------------------------------------------------------------
void CStudioRender::ConvertMeshVertexToDecalVertex( DecalBuildInfo_t& build, 
									int meshIndex, DecalVertex_t& decalVertex )
{
	// Copy over the data;
	// get the texture coords from the decal planar projection

	Assert( meshIndex < MAXSTUDIOVERTS );
	
	VectorCopy( *build.m_pMesh->Position(meshIndex), decalVertex.m_Position );
	VectorCopy( *build.m_pMesh->Normal(meshIndex), decalVertex.m_Normal );
	Vector2DCopy( build.m_pVertexInfo[meshIndex].m_UV, decalVertex.m_TexCoord );
	decalVertex.m_MeshVertexIndex = meshIndex;
	decalVertex.m_Mesh = build.m_Mesh;
	Assert (decalVertex.m_Mesh < 100 );
	decalVertex.m_Model = build.m_Model;
	decalVertex.m_Body = build.m_Body;
}


//-----------------------------------------------------------------------------
// Adds a vertex to the list of vertices for this material
//-----------------------------------------------------------------------------
inline unsigned short CStudioRender::AddVertexToDecal( DecalBuildInfo_t& build, int meshIndex )
{
	DecalBuildVertexInfo_t* pVertexInfo = build.m_pVertexInfo;

	// If we've never seen this vertex before, we need to add a new decal vert
	if (pVertexInfo[meshIndex].m_VertexIndex == 0xFFFF)
	{
		DecalVertexList_t& decalVertexList = build.m_pDecalMaterial->m_Vertices;
		int v = decalVertexList.AddToTail();

		// Copy over the data;
		ConvertMeshVertexToDecalVertex( build, meshIndex, build.m_pDecalMaterial->m_Vertices[v] );

#ifdef _DEBUG
		// Make sure clipped vertices are in the right range...
		if (build.m_UseClipVert)
		{
			Assert( (decalVertexList[v].m_TexCoord[0] >= -1e-3) && (decalVertexList[v].m_TexCoord[0] - 1.0f < 1e-3) );
			Assert( (decalVertexList[v].m_TexCoord[1] >= -1e-3) && (decalVertexList[v].m_TexCoord[1] - 1.0f < 1e-3) );
		}
#endif

		// Store off the index of this vertex so we can reference it again
		pVertexInfo[meshIndex].m_VertexIndex = build.m_VertexCount;
		++build.m_VertexCount;
		if (build.m_FirstVertex == decalVertexList.InvalidIndex())
			build.m_FirstVertex = v;
	}

	return pVertexInfo[meshIndex].m_VertexIndex;
}


//-----------------------------------------------------------------------------
// Adds a vertex to the list of vertices for this material
//-----------------------------------------------------------------------------
inline unsigned short CStudioRender::AddVertexToDecal( DecalBuildInfo_t& build, DecalVertex_t& vert )
{
	// This creates a unique vertex
	DecalVertexList_t& decalVertexList = build.m_pDecalMaterial->m_Vertices;

	// Try to see if the clipped vertex already exists in our decal list...
	// Only search for matches with verts appearing in the current decal
	unsigned short i;
	unsigned short vertexCount = 0;
	for ( i = build.m_FirstVertex; i != decalVertexList.InvalidIndex(); 
		i = decalVertexList.Next(i), ++vertexCount )
	{
		// Only bother to check against clipped vertices
		if ( decalVertexList[i].GetMesh( build.m_pStudioHdr ) )
			continue;

		// They must have the same position, and normal
		// texcoord will fall right out if the positions match
		Vector temp;
		VectorSubtract( decalVertexList[i].m_Position, vert.m_Position, temp );
		if ( (fabs(temp[0]) > 1e-3) || (fabs(temp[1]) > 1e-3) || (fabs(temp[2]) > 1e-3) )
			continue;

		VectorSubtract( decalVertexList[i].m_Normal, vert.m_Normal, temp );
		if ( (fabs(temp[0]) > 1e-3) || (fabs(temp[1]) > 1e-3) || (fabs(temp[2]) > 1e-3) )
			continue;

		return vertexCount;
	}

	// This path is the path taken by clipped vertices
	Assert( (vert.m_TexCoord[0] >= -1e-3) && (vert.m_TexCoord[0] - 1.0f < 1e-3) );
	Assert( (vert.m_TexCoord[1] >= -1e-3) && (vert.m_TexCoord[1] - 1.0f < 1e-3) );

	// Must create a new vertex...
	unsigned short idx = decalVertexList.AddToTail(vert);
	if (build.m_FirstVertex == decalVertexList.InvalidIndex())
		build.m_FirstVertex = idx;
	Assert( vertexCount == build.m_VertexCount );
	return build.m_VertexCount++;
}


//-----------------------------------------------------------------------------
// Adds the clipped triangle to the decal
//-----------------------------------------------------------------------------
void CStudioRender::AddClippedDecalToTriangle( DecalBuildInfo_t& build, DecalClipState_t& clipState )
{
	// FIXME: Clipped vertices will almost always be shared. We
	// need a way of associating clipped vertices with edges so we can share
	// the clipped vertices quickly
	Assert( clipState.m_VertCount <= 7 );

	// Yeah baby yeah!!	Add this sucka
	int i;
	unsigned short indices[7];
	for ( i = 0; i < clipState.m_VertCount; ++i)
	{
		// First add the vertices
		int vertIdx = clipState.m_Indices[clipState.m_Pass][i];
		if (vertIdx < 3)
		{
			indices[i] = AddVertexToDecal( build, clipState.m_ClipVerts[vertIdx].m_MeshVertexIndex );
		}
		else
		{
			indices[i] = AddVertexToDecal( build, clipState.m_ClipVerts[vertIdx] );
		}
	}

	// Add a trifan worth of triangles
	for ( i = 1; i < clipState.m_VertCount - 1; ++i)
	{
		build.m_pDecalMaterial->m_Indices.AddToTail( indices[0] );
		build.m_pDecalMaterial->m_Indices.AddToTail( indices[i] );
		build.m_pDecalMaterial->m_Indices.AddToTail( indices[i+1] );
	}
}


//-----------------------------------------------------------------------------
// Clips the triangle to +/- radius
//-----------------------------------------------------------------------------
bool CStudioRender::ClipDecal( DecalBuildInfo_t& build, int i1, int i2, int i3, int *pClipFlags )
{
	int i;

	DecalClipState_t clipState;
	clipState.m_VertCount = 3;
	ConvertMeshVertexToDecalVertex( build, i1, clipState.m_ClipVerts[0] );
	ConvertMeshVertexToDecalVertex( build, i2, clipState.m_ClipVerts[1] );
	ConvertMeshVertexToDecalVertex( build, i3, clipState.m_ClipVerts[2] );
	clipState.m_ClipVertCount = 3;

	for ( i = 0; i < 3; ++i)
	{
		clipState.m_ClipFlags[i] = pClipFlags[i];
		clipState.m_Indices[0][i] = i;
	}
	clipState.m_Pass = 0;

	// Clip against each plane
	ClipTriangleAgainstPlane( clipState, 0, DECAL_CLIP_MINUSU, 0.0f );
	if (clipState.m_VertCount < 3)
		return false;

	ClipTriangleAgainstPlane( clipState, 0, DECAL_CLIP_PLUSU, 1.0f );
	if (clipState.m_VertCount < 3)
		return false;

	ClipTriangleAgainstPlane( clipState, 1, DECAL_CLIP_MINUSV, 0.0f );
	if (clipState.m_VertCount < 3)
		return false;

	ClipTriangleAgainstPlane( clipState, 1, DECAL_CLIP_PLUSV, 1.0f );
	if (clipState.m_VertCount < 3)
		return false;

	// Only add the clipped decal to the triangle if it's one bone
	// otherwise just return if it was clipped
	if ( build.m_UseClipVert )
	{
		AddClippedDecalToTriangle( build, clipState );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Adds a decal to a triangle, but only if it should
//-----------------------------------------------------------------------------
void CStudioRender::AddTriangleToDecal( DecalBuildInfo_t& build, int i1, int i2, int i3 )
{
	DecalBuildVertexInfo_t* pVertexInfo = build.m_pVertexInfo;

	// All must be front-facing for a decal to be added
	// FIXME: Could make it work if not all are front-facing, need clipping for that
	if ((!pVertexInfo[i1].m_FrontFacing) || (!pVertexInfo[i2].m_FrontFacing) || 
		(!pVertexInfo[i3].m_FrontFacing) )
	{
		return;
	}

	// This is used to prevent poke through; if the points are too far away
	// from the contact point, then don't add the decal
	if ((!pVertexInfo[i1].m_InValidArea) && (!pVertexInfo[i2].m_InValidArea) && 
		(!pVertexInfo[i3].m_InValidArea) )
	{
		return;
	}

	// Clip to +/- radius
	int clipFlags[3];

	clipFlags[0] = ComputeClipFlags( pVertexInfo, i1 );
	clipFlags[1] = ComputeClipFlags( pVertexInfo, i2 );
	clipFlags[2] = ComputeClipFlags( pVertexInfo, i3 );

	// Cull... The result is non-zero if they're all outside the same plane
	if ( (clipFlags[0] & (clipFlags[1] & clipFlags[2]) ) != 0)
		return;

	bool doClip = true;
	
	// Trivial accept for skinned polys... if even one vert is inside
	// the draw region, accept
	if ((!build.m_UseClipVert) && ( !clipFlags[0] || !clipFlags[1] || !clipFlags[2] ))
		doClip = false;

	// Trivial accept... no clip flags set means all in
	// Don't clip if we have more than one bone... we'll need to do skinning
	// and we can't clip the bone indices
	// We *do* want to clip in the one bone case though; useful for large
	// static props.
	if ( doClip && ( clipFlags[0] || clipFlags[1] || clipFlags[2] ))
	{
		bool validTri = ClipDecal( build, i1, i2, i3, clipFlags );

		// Don't add the triangle if we culled the triangle or if 
		// we had one or less bones
		if (build.m_UseClipVert || (!validTri))
			return;
	}

	// Add the vertices to the decal since there was no clipping
	build.m_pDecalMaterial->m_Indices.AddToTail(AddVertexToDecal(build, i1));
	build.m_pDecalMaterial->m_Indices.AddToTail(AddVertexToDecal(build, i2));
	build.m_pDecalMaterial->m_Indices.AddToTail(AddVertexToDecal(build, i3));
}


//-----------------------------------------------------------------------------
// Adds a decal to a mesh 
//-----------------------------------------------------------------------------
void CStudioRender::AddDecalToMesh( DecalBuildInfo_t& build )
{
	// Don't add to the mesh if the mesh has a translucent material
	if (build.m_SuppressTlucDecal)
	{
		short *pSkinRef	= m_pStudioHdr->pSkinref( 0 );
		IMaterial *pMaterial = build.m_ppMaterials[pSkinRef[build.m_pMesh->material]];
		if (pMaterial->IsTranslucent())
			return;
	}

	build.m_pVertexInfo = (DecalBuildVertexInfo_t*)_alloca( build.m_pMesh->numvertices * sizeof(DecalBuildVertexInfo_t) );

	// Project all vertices for this group into decal space
	// Note we do this work at a mesh level instead of a model level
	// because vertices are not shared across mesh boundaries
	ProjectDecalOntoMesh( build );

	// Draw all the various mesh groups...
	for ( int j = 0; j < build.m_pMeshData->m_NumGroup; ++j )
	{
		studiomeshgroup_t* pGroup = &build.m_pMeshData->m_pMeshGroup[j];

		// Must add decal to each strip in the strip group
		// We do this so we can re-use all of the bone state change
		// info associated with the strips
		for (int k = 0; k < pGroup->m_NumStrips; ++k)
		{
			OptimizedModel::StripHeader_t* pStrip = &pGroup->m_pStripData[k];
			if (pStrip->flags & OptimizedModel::STRIP_IS_TRISTRIP)
			{
				for (int i = 0; i < pStrip->numIndices - 2; ++i)
				{
					int idx = pStrip->indexOffset + i;

					bool ccw = (i & 0x1) == 0;
					int i1 = pGroup->MeshIndex(idx);
					int i2 = pGroup->MeshIndex(idx+1+ccw);
					int i3 = pGroup->MeshIndex(idx+2-ccw);

					AddTriangleToDecal( build, i1, i2, i3 );
				}
			}
			else
			{
				for (int i = 0; i < pStrip->numIndices; i += 3)
				{
					int idx = pStrip->indexOffset + i;

					int i1 = pGroup->MeshIndex(idx);
					int i2 = pGroup->MeshIndex(idx+1);
					int i3 = pGroup->MeshIndex(idx+2);

					AddTriangleToDecal( build, i1, i2, i3 );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Adds a decal to a mesh 
//-----------------------------------------------------------------------------
void CStudioRender::AddDecalToModel( DecalBuildInfo_t& buildInfo )
{
	// FIXME: We need to do some high-level culling to figure out exactly
	// which meshes we need to add the decals to
	// Turns out this solution may also be good for mesh sorting
	// we need to know the center of each mesh, could also store a
	// bounding radius for each mesh and test the ray against each sphere.

	for ( int i = 0; i < m_pSubModel->nummeshes; ++i)
	{
		buildInfo.m_Mesh = i;
		buildInfo.m_pMesh = m_pSubModel->pMesh(i);
		buildInfo.m_pMeshData = &m_pStudioMeshes[buildInfo.m_pMesh->meshid];
		Assert(buildInfo.m_pMeshData);

		AddDecalToMesh( buildInfo );
	}
}


//-----------------------------------------------------------------------------
// Computes the pose to decal plane transform 
//-----------------------------------------------------------------------------
bool CStudioRender::ComputePoseToDecal( const Ray_t& ray, const Vector& up )
{
	// Create a transform that projects world coordinates into a 
	// basis for the decal
	matrix3x4_t worldToDecal;
	Vector decalU, decalV, decalN;

	// Get the z axis
	VectorMultiply( ray.m_Delta, -1.0f, decalN );
	if (VectorNormalize( decalN ) == 0.0f)
		return false;

	// Deal with the u axis
	CrossProduct( up, decalN, decalU );
	if ( VectorNormalize( decalU ) < 1e-3 )
	{
		// if up parallel or antiparallel to ray, deal...
		Vector fixup( up.y, up.z, up.x );
		CrossProduct( fixup, decalN, decalU );
		if ( VectorNormalize( decalU ) < 1e-3 )
			return false;
	}

	CrossProduct( decalN, decalU, decalV );

	// Since I want world-to-decal, I gotta take the inverse of the decal
	// to world. Assuming post-multiplying column vectors, the decal to world = 
	//		[ Ux Vx Nx | ray.m_Start[0] ]
	//		[ Uy Vy Ny | ray.m_Start[1] ]
	//		[ Uz Vz Nz | ray.m_Start[2] ]

	VectorCopy( decalU.Base(), worldToDecal[0] );
	VectorCopy( decalV.Base(), worldToDecal[1] );
	VectorCopy( decalN.Base(), worldToDecal[2] );

	worldToDecal[0][3] = -DotProduct( ray.m_Start.Base(), worldToDecal[0] );
	worldToDecal[1][3] = -DotProduct( ray.m_Start.Base(), worldToDecal[1] );
	worldToDecal[2][3] = -DotProduct( ray.m_Start.Base(), worldToDecal[2] );

	// Compute transforms from pose space to decal plane space
	for ( int i = 0; i < m_pStudioHdr->numbones; i++)
	{
		ConcatTransforms( worldToDecal, m_PoseToWorld[i], m_PoseToDecal[i] );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Gets the list of triangles for a particular material and lod
//-----------------------------------------------------------------------------

int CStudioRender::GetDecalMaterial( DecalLod_t& decalLod, IMaterial* pDecalMaterial )
{
	// Grab the material for this lod...
	unsigned short j;
	for ( j = decalLod.m_FirstMaterial; j != m_DecalMaterial.InvalidIndex(); j = m_DecalMaterial.Next(j) )
	{
		if (m_DecalMaterial[j].m_pMaterial == pDecalMaterial)
		{
			return j;
		}
	}

	// If we got here, this must be the first time we saw this material
	j = m_DecalMaterial.Alloc( true );
	
	// Link it into the list of data for this lod
	if (decalLod.m_FirstMaterial != m_DecalMaterial.InvalidIndex() )
		m_DecalMaterial.LinkBefore( decalLod.m_FirstMaterial, j );
	decalLod.m_FirstMaterial = j;

	m_DecalMaterial[j].m_pMaterial = pDecalMaterial;

	return j;
}


//-----------------------------------------------------------------------------
// Removes a decal and associated vertices + indices from the history list
//-----------------------------------------------------------------------------
void CStudioRender::RetireDecal( DecalHistoryList_t& historyList )
{
	Assert( historyList.Count() );
	DecalHistory_t& decalHistory = historyList[ historyList.Head() ];

	// Find the decal material for the decal to remove
	DecalMaterial_t& material = m_DecalMaterial[decalHistory.m_Material];

	DecalVertexList_t& vertices = material.m_Vertices;
	Decal_t& decalToRemove = material.m_Decals[decalHistory.m_Decal];
	
	// Now clear out the vertices referenced by the indices....
	unsigned short next; 
	unsigned short vert = vertices.Head();
	Assert( vertices.Count() >= decalToRemove.m_VertexCount );
	int vertsToRemove = decalToRemove.m_VertexCount;
	while ( vertsToRemove > 0)
	{
		// blat out the vertices
		next = vertices.Next(vert);
		vertices.Remove(vert);
		vert = next;

		--vertsToRemove;
	}

	// FIXME: This does a memmove. How expensive is it?
	material.m_Indices.RemoveMultiple( 0, decalToRemove.m_IndexCount );

	// Remove the decal
	material.m_Decals.Remove(decalHistory.m_Decal);

	// Clear the decal out of the history
	historyList.Remove( historyList.Head() );
}


//-----------------------------------------------------------------------------
// Adds a decal to the history list
//-----------------------------------------------------------------------------
int CStudioRender::AddDecalToMaterialList( DecalMaterial_t* pMaterial )
{
	DecalList_t& decalList = pMaterial->m_Decals;
	return decalList.AddToTail();
}


#define MAX_DECAL_INDICES_PER_MODEL 2048

inline bool CStudioRender::ShouldRetireDecal(DecalMaterial_t* pDecalMaterial, DecalHistoryList_t const& decalHistory )
{
	// Check to see if we should retire the decal
	return ( decalHistory.Count() >= m_Config.maxDecalsPerModel ) ||
		(pDecalMaterial->m_Indices.Count() > MAX_DECAL_INDICES_PER_MODEL);
}

//-----------------------------------------------------------------------------
// Add decals to a decal list by doing a planar projection along the ray
//-----------------------------------------------------------------------------
void CStudioRender::AddDecal( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr, const Ray_t& ray, 
	const Vector& decalUp, IMaterial* pDecalMaterial, float radius, int body, bool noPokethru, int maxLODToDecal )
{
	// For each lod, build the decal list
	DecalModelList_t& list = m_DecalList[handle];
	m_pStudioHdr = pStudioHdr;

	// Bone to world must be set before calling AddDecal; it uses that here
	ComputePoseToWorld( m_pStudioHdr );

	// Compute transforms from pose space to decal plane space
	if (!ComputePoseToDecal( ray, decalUp ))
		return;

	// Since we're adding this to a studio model, check the decal to see if 
	// there's an alternate form used for static props...
	bool found;
	IMaterialVar* pModelMaterialVar = pDecalMaterial->FindVar( "$modelmaterial", &found, false );
	if (found)
	{
		IMaterial* pModelMaterial = m_pMaterialSystem->FindMaterial( pModelMaterialVar->GetStringValue(), &found, false );
		if (found)
			pDecalMaterial = pModelMaterial;		
	}

	// Get dynamic information from the material (fade start, fade time)
	float fadeStartTime	= 0.0f;
	float fadeDuration = 0.0f;
	int flags = 0;

	/*
	IMaterialVar* decalVar = pDecalMaterial->FindVar( "$decalFadeDuration", &found, false );
	if ( found  )
	{
		flags |= DECAL_DYNAMIC;
		fadeDuration = decalVar->GetFloatValue();
		decalVar = pDecalMaterial->FindVar( "$decalFadeTime", &found, false );
		fadeStartTime = found ? decalVar->GetFloatValue() : 0.0f;
		fadeStartTime += cl.time;
	}

	// Is this a second-pass decal?
	decalVar = pDecalMaterial->FindVar( "$decalSecondPass", &found, false );
	if ( found  )
		flags |= DECAL_SECONDPASS;
	*/

	// This sucker is state needed only when building decals
	DecalBuildInfo_t buildInfo;
	buildInfo.m_Radius = radius;
	buildInfo.m_NoPokeThru = noPokethru;
	buildInfo.m_pStudioHdr = pStudioHdr;

	// Find out which LODs we're defacing
	int iMaxLOD;
	if ( maxLODToDecal == ADDDECAL_TO_ALL_LODS )
	{
		iMaxLOD = list.m_pHardwareData->m_NumLODs;
	}
	else 
	{
		iMaxLOD = min( list.m_pHardwareData->m_NumLODs, maxLODToDecal );
	}

	// Gotta do this for all LODs
	for (int i = iMaxLOD; --i >= 0; )
	{
		// Grab the list of all decals using the same material for this lod...
		int materialIdx = GetDecalMaterial( list.m_pLod[i], pDecalMaterial );
		buildInfo.m_pDecalMaterial = &m_DecalMaterial[materialIdx];

		// Check to see if we should retire the decal
		while( ShouldRetireDecal( buildInfo.m_pDecalMaterial, list.m_pLod[i].m_DecalHistory ) )
		{
			RetireDecal( list.m_pLod[i].m_DecalHistory );
		}

		// Grab the meshes for this lod
		m_pStudioMeshes = list.m_pHardwareData->m_pLODs[i].m_pMeshData;

		// Clip decals when there's no flexes or bones
 		buildInfo.m_UseClipVert = ( m_pStudioHdr->numbones <= 1 ) && ( m_pStudioHdr->numflexdesc == 0 );

		// Don't decal on meshes that are translucent if it's twopass
		buildInfo.m_SuppressTlucDecal = (m_pStudioHdr->flags & STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS) != 0;
		buildInfo.m_ppMaterials = list.m_pHardwareData->m_pLODs[i].ppMaterials;

		// Set up info needed for vertex sharing
		buildInfo.m_FirstVertex = buildInfo.m_pDecalMaterial->m_Vertices.InvalidIndex();
		buildInfo.m_VertexCount = 0;

		int prevIndexCount = buildInfo.m_pDecalMaterial->m_Indices.Count();

		// Step over all body parts + add decals to em all!
		for ( int k=0 ; k < m_pStudioHdr->numbodyparts ; k++) 
		{
			// Grab the model for this body part
			int model = R_StudioSetupModel( k, body, &m_pSubModel, m_pStudioHdr );
			buildInfo.m_Body = k;
			buildInfo.m_Model = model;
			AddDecalToModel( buildInfo );
		}

		// Add this to the list of decals in this material
		if (buildInfo.m_VertexCount)
		{
			int decalIndexCount = buildInfo.m_pDecalMaterial->m_Indices.Count() - prevIndexCount;
			Assert(decalIndexCount > 0);

			int decalIndex = AddDecalToMaterialList( buildInfo.m_pDecalMaterial );
			Decal_t& decal = buildInfo.m_pDecalMaterial->m_Decals[decalIndex];
			decal.m_VertexCount = buildInfo.m_VertexCount;
			decal.m_IndexCount = decalIndexCount;
			decal.m_FadeStartTime = fadeStartTime;
			decal.m_FadeDuration = fadeDuration; 
			decal.m_Flags = flags;

			// Add this decal to the history...
			int h = list.m_pLod[i].m_DecalHistory.AddToTail();
			list.m_pLod[i].m_DecalHistory[h].m_Material = materialIdx;
			list.m_pLod[i].m_DecalHistory[h].m_Decal = decalIndex;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove all the decals on a model
//-----------------------------------------------------------------------------
void CStudioRender::RemoveAllDecals( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr )
{
	DestroyDecalList( handle );
}

//-----------------------------------------------------------------------------
//
// This code here is all about rendering the decals
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inner loop for rendering decals that have a single bone
//-----------------------------------------------------------------------------

void CStudioRender::DrawSingleBoneDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial )
{
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->LoadMatrix( (float*)m_MaterialPoseToWorld[0] );

	// We don't got no bones, so yummy yummy yum, just copy the data out
	// Static props should go though this code path

	DecalVertexList_t& verts = decalMaterial.m_Vertices; 
	for ( unsigned short i = verts.Head(); i != verts.InvalidIndex(); i = verts.Next(i) )
	{
		DecalVertex_t& vertex = verts[i];
		
		meshBuilder.Position3fv( vertex.m_Position.Base() );
		meshBuilder.Normal3fv( vertex.m_Normal.Base() );
		meshBuilder.TexCoord2fv( 0, vertex.m_TexCoord.Base() );
		meshBuilder.Color4ub( 255, 255, 255, 255 );

		meshBuilder.BoneWeight( 0, 1.0f );
		meshBuilder.BoneWeight( 1, 0.0f );
		meshBuilder.BoneWeight( 2, 0.0f );
		meshBuilder.BoneWeight( 3, 0.0f );
		meshBuilder.BoneMatrix( 0, 0 );
		meshBuilder.BoneMatrix( 1, 0 );
		meshBuilder.BoneMatrix( 2, 0 );
		meshBuilder.BoneMatrix( 3, 0 );

		meshBuilder.AdvanceVertex();
	}
}

void CStudioRender::DrawSingleBoneFlexedDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial )
{
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->LoadMatrix( (float*)m_MaterialPoseToWorld[0] );

	// We don't got no bones, so yummy yummy yum, just copy the data out
	// Static props should go though this code path
	DecalVertexList_t& verts = decalMaterial.m_Vertices; 
	for ( unsigned short i = verts.Head(); i != verts.InvalidIndex(); i = verts.Next(i) )
	{
		DecalVertex_t& vertex = verts[i];

		// Clipped verts shouldn't come through here, only static props should use clipped
		Assert ( vertex.m_MeshVertexIndex >= 0 );

		m_VertexCache.SetBodyModelMesh( vertex.m_Body, vertex.m_Model, vertex.m_Mesh );
		if (m_VertexCache.IsVertexFlexed( vertex.m_MeshVertexIndex ))
		{
			CachedVertex_t* pFlexedVertex = m_VertexCache.GetFlexVertex( vertex.m_MeshVertexIndex );
			meshBuilder.Position3fv( pFlexedVertex->m_Position.Base() );
			meshBuilder.Normal3fv( pFlexedVertex->m_Normal.Base() );
		}
		else
		{
			meshBuilder.Position3fv( vertex.m_Position.Base() );
			meshBuilder.Normal3fv( vertex.m_Normal.Base() );
		}

		meshBuilder.TexCoord2fv( 0, vertex.m_TexCoord.Base() );
		meshBuilder.Color4ub( 255, 255, 255, 255 );

		meshBuilder.BoneWeight( 0, 1.0f );
		meshBuilder.BoneWeight( 1, 0.0f );
		meshBuilder.BoneWeight( 2, 0.0f );
		meshBuilder.BoneWeight( 3, 0.0f );
		meshBuilder.BoneMatrix( 0, 0 );
		meshBuilder.BoneMatrix( 1, 0 );
		meshBuilder.BoneMatrix( 2, 0 );
		meshBuilder.BoneMatrix( 3, 0 );

		meshBuilder.AdvanceVertex();
	}
}


//-----------------------------------------------------------------------------
// Inner loop for rendering decals that have multiple bones
//-----------------------------------------------------------------------------
void CStudioRender::DrawMultiBoneDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial, studiohdr_t *pStudioHdr )
{
	// Use the identity matrix because we're going to be doing software skinning
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->LoadIdentity( );

	DecalVertexList_t& verts = decalMaterial.m_Vertices; 
	for ( unsigned short i = verts.Head(); i != verts.InvalidIndex(); i = verts.Next(i) )
	{
		DecalVertex_t& vertex = verts[i];
		
		int n = vertex.m_MeshVertexIndex;

		Assert( n < MAXSTUDIOVERTS );

		mstudiomesh_t *pMesh = vertex.GetMesh( pStudioHdr );
		Assert( pMesh );

		m_VertexCache.SetBodyModelMesh( vertex.m_Body, vertex.m_Model, vertex.m_Mesh );
		if (m_VertexCache.IsVertexPositionCached( n ))
		{
			CachedVertex_t* pCachedVert = m_VertexCache.GetWorldVertex( n );
			meshBuilder.Position3fv( pCachedVert->m_Position.Base() );
			meshBuilder.Normal3fv( pCachedVert->m_Normal.Base() );
		}
		else
		{
			// Prevent the computation of this again....
			m_VertexCache.SetupComputation(pMesh);
			CachedVertex_t* pCachedVert = m_VertexCache.CreateWorldVertex( n );

			mstudioboneweight_t* pBoneWeights = pMesh->BoneWeights( n );
			R_StudioTransform( *pMesh->Position(n), pBoneWeights, pCachedVert->m_Position );
			R_StudioRotate( *pMesh->Normal(n), pBoneWeights, pCachedVert->m_Normal );

			// Add a little extra offset for hardware skinning; in that case
			// we're doing software skinning for decals and it might not be quite right
			VectorMA( pCachedVert->m_Position, 0.1, pCachedVert->m_Normal, pCachedVert->m_Position );

			meshBuilder.Position3fv( pCachedVert->m_Position.Base() );
			meshBuilder.Normal3fv( pCachedVert->m_Normal.Base() );
		}

		meshBuilder.TexCoord2fv( 0, vertex.m_TexCoord.Base() );
		meshBuilder.Color4ub( 255, 255, 255, 255 );

		meshBuilder.BoneWeight( 0, 1.0f );
		meshBuilder.BoneWeight( 1, 0.0f );
		meshBuilder.BoneWeight( 2, 0.0f );
		meshBuilder.BoneWeight( 3, 0.0f );
		meshBuilder.BoneMatrix( 0, 0 );
		meshBuilder.BoneMatrix( 1, 0 );
		meshBuilder.BoneMatrix( 2, 0 );
		meshBuilder.BoneMatrix( 3, 0 );

		meshBuilder.AdvanceVertex();
	}
}

void CStudioRender::DrawMultiBoneFlexedDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial, studiohdr_t *pStudioHdr )
{
	// Use the identity matrix because we're going to be doing software skinning
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->LoadIdentity( );

	DecalVertexList_t& verts = decalMaterial.m_Vertices; 
	for ( unsigned short i = verts.Head(); i != verts.InvalidIndex(); i = verts.Next(i) )
	{
		DecalVertex_t& vertex = verts[i];
		
		int n = vertex.m_MeshVertexIndex;

		mstudiomesh_t *pMesh = vertex.GetMesh( pStudioHdr );
		Assert( pMesh );

		mstudioboneweight_t* pBoneWeights = pMesh->BoneWeights( n );

		m_VertexCache.SetBodyModelMesh( vertex.m_Body, vertex.m_Model, vertex.m_Mesh );

		if (m_VertexCache.IsVertexPositionCached( n ))
		{
			CachedVertex_t* pCachedVert = m_VertexCache.GetWorldVertex( n );
			meshBuilder.Position3fv( pCachedVert->m_Position.Base() );
			meshBuilder.Normal3fv( pCachedVert->m_Normal.Base() );
		}
		else
		{
			// Prevent the computation of this again....
			m_VertexCache.SetupComputation(pMesh);
			CachedVertex_t* pCachedVert = m_VertexCache.CreateWorldVertex( n );

			if (m_VertexCache.IsVertexFlexed( n ))
			{
				CachedVertex_t* pFlexedVertex = m_VertexCache.GetFlexVertex( n );
				R_StudioTransform( pFlexedVertex->m_Position, pBoneWeights, pCachedVert->m_Position );
				R_StudioRotate( pFlexedVertex->m_Normal, pBoneWeights, pCachedVert->m_Normal );
			}
			else
			{
				Assert( pMesh );
				R_StudioTransform( *pMesh->Position(n), pBoneWeights, pCachedVert->m_Position );
				R_StudioRotate( *pMesh->Normal(n), pBoneWeights, pCachedVert->m_Normal );
			}

			// Add a little extra offset for hardware skinning; in that case
			// we're doing software skinning for decals and it might not be quite right
			VectorMA( pCachedVert->m_Position, 0.1, pCachedVert->m_Normal, pCachedVert->m_Position );

			meshBuilder.Position3fv( pCachedVert->m_Position.Base() );
			meshBuilder.Normal3fv( pCachedVert->m_Normal.Base() );
		}

		meshBuilder.TexCoord2fv( 0, vertex.m_TexCoord.Base() );
		meshBuilder.Color4ub( 255, 255, 255, 255 );

		meshBuilder.BoneWeight( 0, 1.0f );
		meshBuilder.BoneWeight( 1, 0.0f );
		meshBuilder.BoneWeight( 2, 0.0f );
		meshBuilder.BoneWeight( 3, 0.0f );
		meshBuilder.BoneMatrix( 0, 0 );
		meshBuilder.BoneMatrix( 1, 0 );
		meshBuilder.BoneMatrix( 2, 0 );
		meshBuilder.BoneMatrix( 3, 0 );

		meshBuilder.AdvanceVertex();
	}
}


//-----------------------------------------------------------------------------
// Draws all the decals using a particular material
//-----------------------------------------------------------------------------
void CStudioRender::DrawDecalMaterial( DecalMaterial_t& decalMaterial, studiohdr_t *pStudioHdr )
{
	// It's possible for the index count to become zero due to decal retirement
	int indexCount = decalMaterial.m_Indices.Count();
	if ( indexCount == 0 )
		return;

	// Bind the decal material
	if (!m_Config.bWireframeDecals)
		m_pMaterialSystem->Bind( decalMaterial.m_pMaterial );
	else
		m_pMaterialSystem->Bind( m_pMaterialMRMWireframe );

	// Use a dynamic mesh...
	IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh();

	int vertexCount = decalMaterial.m_Vertices.Count();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, vertexCount, indexCount );

	// FIXME: Could make static meshes for these?
	// But don't make no static meshes for decals that fade, at least

	// Two possibilities: no/one bones, we let the hardware do all transformation
	// or, more than one bone, we do software skinning.
	if (m_pStudioHdr->numbones <= 1)
	{
		if (m_pStudioHdr->numflexdesc == 0)
			DrawSingleBoneDecals( meshBuilder, decalMaterial );
		else
			DrawSingleBoneFlexedDecals( meshBuilder, decalMaterial );
	}
	else
	{
		if (m_pStudioHdr->numflexdesc == 0)
			DrawMultiBoneDecals( meshBuilder, decalMaterial, pStudioHdr );
		else
			DrawMultiBoneFlexedDecals( meshBuilder, decalMaterial, pStudioHdr );
	}

	// Set the indices
	// This is a little tricky. Because we can retire decals, the indices
	// for each decal start at 0. We output all the vertices in order of
	// each decal, and then fix up the indices based on how many vertices
	// we wrote out for the decals
	unsigned short decal = decalMaterial.m_Decals.Head();
	int indicesRemaining = decalMaterial.m_Decals[decal].m_IndexCount;
	int vertexOffset = 0;
	for ( int i = 0; i < indexCount; ++i)
	{
		meshBuilder.Index( decalMaterial.m_Indices[i] + vertexOffset ); 
		meshBuilder.AdvanceIndex();
		if (--indicesRemaining <= 0)
		{
			vertexOffset += decalMaterial.m_Decals[decal].m_VertexCount;
			decal = decalMaterial.m_Decals.Next(decal); 
			if (decal != decalMaterial.m_Decals.InvalidIndex())
			{
				indicesRemaining = decalMaterial.m_Decals[decal].m_IndexCount;
			}
#ifdef _DEBUG
			else
			{
				Assert( i + 1 == indexCount );
			}
#endif
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draws all the decals on a particular model
//-----------------------------------------------------------------------------
void CStudioRender::DrawDecal( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr, int lod, int body )
{
	VPROF("CStudioRender::DrawDecal");

	if (handle == STUDIORENDER_DECAL_INVALID)
		return;

	// All decal vertex data is are stored in pose space
	// So as long as the pose-to-world transforms are set, we're all ready!

	// FIXME: Body stuff isn't hooked in at all for decals

	// Get the decal list for this lod
	DecalModelList_t const& list = m_DecalList[handle];
	m_pStudioHdr = pStudioHdr;

	// Compute all the bone matrices the way the material system wants it
	for ( int i = 0; i < m_pStudioHdr->numbones; i++)
	{
		BoneMatToMaterialMat( m_PoseToWorld[i], m_MaterialPoseToWorld[i] );
	}

	// Gotta do this for all LODs
	// Draw each set of decals using a particular material
	unsigned short mat = list.m_pLod[lod].m_FirstMaterial;
	for ( ; mat != m_DecalMaterial.InvalidIndex(); mat = m_DecalMaterial.Next(mat))
	{
		DecalMaterial_t& decalMaterial = m_DecalMaterial[mat];
		DrawDecalMaterial( decalMaterial, pStudioHdr );
	}
}

