//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "cstudiorender.h"

#include "studio.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "optimize.h"
#include "mathlib.h"
#include "vector.h"
#include "studiostats.h"
#include <malloc.h>
#include <xmmintrin.h>

#include "tier0/vprof.h"

typedef void (*SoftwareProcessMeshDX6Func_t)( mstudiomesh_t* pmesh, matrix3x4_t *pPoseToWorld,
	CCachedRenderData &vertexCache, CMeshBuilder& meshBuilder, int numVertices, unsigned short* pGroupToMesh, float r_blend );

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class IClientEntity;


static int boxpnt[6][4] = 
{
	{ 0, 4, 6, 2 }, // +X
	{ 0, 1, 5, 4 }, // +Y
	{ 0, 2, 3, 1 }, // +Z
	{ 7, 5, 1, 3 }, // -X
	{ 7, 3, 2, 6 }, // -Y
	{ 7, 6, 4, 5 }, // -Z
};	

static Vector	hullcolor[8] = 
{
	Vector( 1.0, 1.0, 1.0 ),
	Vector( 1.0, 0.5, 0.5 ),
	Vector( 0.5, 1.0, 0.5 ),
	Vector( 1.0, 1.0, 0.5 ),
	Vector( 0.5, 0.5, 1.0 ),
	Vector( 1.0, 0.5, 1.0 ),
	Vector( 0.5, 1.0, 1.0 ),
	Vector( 1.0, 1.0, 1.0 )
};


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

void CStudioRender::R_StudioDrawHulls( int hitboxset, bool translucent )
{
	int			i, j;
//	float		lv;
	Vector		tmp;
	Vector		p[8];
	mstudiobbox_t		*pbbox;
	IMaterialVar *colorVar;

	mstudiohitboxset_t *s = m_pStudioHdr->pHitboxSet( hitboxset );
	if ( !s )
		return;

	pbbox		= s->pHitbox( 0 );
	if ( !pbbox )
		return;

	if( translucent )
	{
		m_pMaterialSystem->Bind( m_pMaterialTranslucentModelHulls );
		colorVar = m_pMaterialTranslucentModelHulls_ColorVar;
	}
	else
	{
		m_pMaterialSystem->Bind( m_pMaterialSolidModelHulls );
		colorVar = m_pMaterialSolidModelHulls_ColorVar;
	}


	for (i = 0; i < s->numhitboxes; i++)
	{
		for (j = 0; j < 8; j++)
		{
			tmp[0] = (j & 1) ? pbbox[i].bbmin[0] : pbbox[i].bbmax[0];
			tmp[1] = (j & 2) ? pbbox[i].bbmin[1] : pbbox[i].bbmax[1];
			tmp[2] = (j & 4) ? pbbox[i].bbmin[2] : pbbox[i].bbmax[2];

			VectorTransform( tmp, m_BoneToWorld[pbbox[i].bone], p[j] );
		}

		j = (pbbox[i].group % 8);
		m_pMaterialSystem->Flush();
		if( colorVar )
		{
			if( translucent )
			{
				colorVar->SetVecValue( 0.2f * hullcolor[j][0], 0.2f * hullcolor[j][1], 0.2f * hullcolor[j][2] );
			}
			else
			{
				colorVar->SetVecValue( hullcolor[j][0], hullcolor[j][1], hullcolor[j][2] );
			}
		}
		for (j = 0; j < 6; j++)
		{
#if 0
			tmp[0] = tmp[1] = tmp[2] = 0;
			tmp[j % 3] = (j < 3) ? 1.0 : -1.0;
			// R_StudioLighting( &lv, pbbox[i].bone, 0, tmp ); // BUG: not updated
#endif

			IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh();
			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

			for (int k = 0; k < 4; ++k)
			{
				meshBuilder.Position3fv( p[boxpnt[j][k]].Base() );
				meshBuilder.AdvanceVertex();
			}
			
			meshBuilder.End();
			pMesh->Draw();
		}
	}
}

// FIXME: This isn't currently used.
void CStudioRender::R_StudioAbsBB ( int sequence, Vector& origin )
{
	int			j;
	float		lv;
	Vector		tmp;
	Vector		p[8];
	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;

	pbones		= m_pStudioHdr->pBone( 0 );

	if ( sequence >= m_pStudioHdr->numseq )
	{
		sequence = 0;
	}

	pseqdesc	= m_pStudioHdr->pSeqdesc( sequence );

	m_pMaterialSystem->Bind( m_pMaterialAdditiveVertexColorVertexAlpha );

	for (j = 0; j < 8; j++)
	{
		p[j][0] = (j & 1) ? pseqdesc->bbmin[0] : pseqdesc->bbmax[0];
		p[j][1] = (j & 2) ? pseqdesc->bbmin[1] : pseqdesc->bbmax[1];
		p[j][2] = (j & 4) ? pseqdesc->bbmin[2] : pseqdesc->bbmax[2];

		VectorAdd( p[j], origin, p[j] );
	}

	IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 6 );

	for (j = 0; j < 6; j++)
	{
		tmp[0] = tmp[1] = tmp[2] = 0;
		tmp[j % 3] = (j < 3) ? 1.0 : -1.0;
		// R_StudioLighting( &lv, -1, 0, tmp ); // BUG: not updated
		lv = 1.0;

		meshBuilder.Color4f( 0.5, 0.5, 1.0, 1.0 );
		meshBuilder.Position3fv( p[boxpnt[j][0]].Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4f( 0.5, 0.5, 1.0, 1.0 );
		meshBuilder.Position3fv( p[boxpnt[j][1]].Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4f( 0.5, 0.5, 1.0, 1.0 );
		meshBuilder.Position3fv( p[boxpnt[j][2]].Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4f( 0.5, 0.5, 1.0, 1.0 );
		meshBuilder.Position3fv( p[boxpnt[j][3]].Base() );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();
}

void CStudioRender::R_StudioDrawBones (void)
{
	int			i, j, k;
//	float		lv;
	Vector		tmp;
	Vector		p[8];
	Vector		up, right, forward;
	Vector		a1;
	mstudiobone_t		*pbones;
	Vector		positionArray[4];

	pbones		= m_pStudioHdr->pBone( 0 );

	for (i = 0; i < m_pStudioHdr->numbones; i++)
	{
		if (pbones[i].parent == -1)
			continue;

		k = pbones[i].parent;

		a1[0] = a1[1] = a1[2] = 1.0;
		up[0] = m_BoneToWorld[i][0][3] - m_BoneToWorld[k][0][3];
		up[1] = m_BoneToWorld[i][1][3] - m_BoneToWorld[k][1][3];
		up[2] = m_BoneToWorld[i][2][3] - m_BoneToWorld[k][2][3];
		if (up[0] > up[1])
			if (up[0] > up[2])
				a1[0] = 0.0;
			else
				a1[2] = 0.0;
		else
			if (up[1] > up[2])
				a1[1] = 0.0;
			else
				a1[2] = 0.0;
		CrossProduct( up, a1, right );
		VectorNormalize( right );
		CrossProduct( up, right, forward );
		VectorNormalize( forward );
		VectorScale( right, 2.0, right );
		VectorScale( forward, 2.0, forward );

		for (j = 0; j < 8; j++)
		{
			p[j][0] = m_BoneToWorld[k][0][3];
			p[j][1] = m_BoneToWorld[k][1][3];
			p[j][2] = m_BoneToWorld[k][2][3];

			if (j & 1)
			{
				VectorSubtract( p[j], right, p[j] );
			}
			else
			{
				VectorAdd( p[j], right, p[j] );
			}

			if (j & 2)
			{
				VectorSubtract( p[j], forward, p[j] );
			}
			else
			{
				VectorAdd( p[j], forward, p[j] );
			}

			if (j & 4)
			{ 
			}
			else
			{
				VectorAdd( p[j], up, p[j] );
			}
		}

		VectorNormalize( up );
		VectorNormalize( right );
		VectorNormalize( forward );

		m_pMaterialSystem->Bind( m_pMaterialModelBones );
		
		for (j = 0; j < 6; j++)
		{
			switch( j)
			{
			case 0:	VectorCopy( right, tmp ); break;
			case 1:	VectorCopy( forward, tmp ); break;
			case 2:	VectorCopy( up, tmp ); break;
			case 3:	VectorScale( right, -1, tmp ); break;
			case 4:	VectorScale( forward, -1, tmp ); break;
			case 5:	VectorScale( up, -1, tmp ); break;
			}
			// R_StudioLighting( &lv, -1, 0, tmp );  // BUG: not updated

			IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh();
			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

			for (int k = 0; k < 4; ++k)
			{
				meshBuilder.Position3fv( p[boxpnt[j][k]].Base() );
				meshBuilder.AdvanceVertex();
			}
			
			meshBuilder.End();
			pMesh->Draw();
		}
	}
}


int CStudioRender::R_StudioRenderModel( int skin, int body, int hitboxset, void /*IClientEntity*/ *pEntity,
	IMaterial **ppMaterials, int *pMaterialFlags, int flags, IMesh **ppColorMeshes )
{
	VPROF("CStudioRender::R_StudioRenderModel");

	// BUG: This method is crap, though less crap then before.  It should just sort 
	// the materials though it'll need to sort at render time as "skin" 
	// can change what materials a given mesh may use

	int numTrianglesRendered = 0;

	int nDrawGroup = flags & STUDIORENDER_DRAW_GROUP_MASK;
	if (nDrawGroup != STUDIORENDER_DRAW_TRANSLUCENT_ONLY)
	{
		m_bDrawTranslucentSubModels = false;
		numTrianglesRendered += R_StudioRenderFinal( skin, body, hitboxset, pEntity, 
			ppMaterials, pMaterialFlags, ppColorMeshes );
	}
	if (nDrawGroup != STUDIORENDER_DRAW_OPAQUE_ONLY)
	{
		m_bDrawTranslucentSubModels = true;
		numTrianglesRendered += R_StudioRenderFinal( skin, body, hitboxset, pEntity, 
			ppMaterials, pMaterialFlags, ppColorMeshes );
	}
	return numTrianglesRendered;
}


/*
================
R_StudioRenderFinal
inputs:
outputs: returns the number of triangles rendered.
================
*/
int CStudioRender::R_StudioRenderFinal( 
	int skin, int body, int hitboxset, void /*IClientEntity*/ *pClientEntity,
	IMaterial **ppMaterials, int *pMaterialFlags, IMesh **ppColorMeshes )
{
//	MEASURE_TIMED_STAT(STUDIO_STATS_RENDER_FINAL);
	VPROF("CStudioRender::R_StudioRenderFinal");

	int i;
	int numTrianglesRendered = 0;

	if( m_Config.drawEntities == 1 )
	{
		for (i=0 ; i < m_pStudioHdr->numbodyparts ; i++) 
		{
			int model = R_StudioSetupModel( i, body, &m_pSubModel, m_pStudioHdr );

			// Set up flex
			m_VertexCache.SetBodyPart( i );
			m_VertexCache.SetModel( model );

			numTrianglesRendered += R_StudioDrawPoints( skin, pClientEntity, 
				ppMaterials, pMaterialFlags, ppColorMeshes );
		}
		return numTrianglesRendered;
	}
	else if (m_Config.drawEntities == 2)
	{
		R_StudioDrawBones( );
	}
	else if (m_Config.drawEntities == 3)
	{
		R_StudioDrawHulls( hitboxset, false );
	}
#if 0
	else if (m_Config.drawEntities == 4)
	{
		R_StudioDrawHulls( hitboxset, true );
	}
#endif

	return numTrianglesRendered;
}



// define NONORMALUPDATES


// draw normal
// FIXME: not used!
void CStudioRender::DrawNormal( const Vector& pos, float scale, const Vector& normal, const Vector& color )
{
	Vector tmp;

	m_pMaterialSystem->Bind( m_pMaterialWorldWireframe );
	IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( pos, scale, normal, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draw shadows
//-----------------------------------------------------------------------------
void CStudioRender::DrawShadows( int body, int flags )
{
	VPROF("CStudioRender::DrawShadows");

	IMaterial* pForcedMat = m_pForcedMaterial;

	// Here, we have to redraw the model one time for each shadow
	for (int i = 0; i < m_ShadowState.Count(); ++i )
	{
		ForcedMaterialOverride( m_ShadowState[i].m_pMaterial );
		R_StudioRenderModel( 0, body, 0, m_ShadowState[i].m_pProxyData,
			NULL, NULL, flags );
	}

	// Restore the previous forced material
	ForcedMaterialOverride( pForcedMat );
}


static matrix3x4_t *ComputeSkinMatrix( mstudioboneweight_t &boneweights, matrix3x4_t *pPoseToWorld, matrix3x4_t &result )
{
	float flWeight0, flWeight1, flWeight2, flWeight3;

	switch( boneweights.numbones )
	{
	default:
	case 1:
		return &pPoseToWorld[boneweights.bone[0]];

	case 2:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];

			// NOTE: Inlining here seems to make a fair amount of difference
			result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1;
			result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1;
			result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1;
			result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1;
			result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1;
			result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1;
			result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1;
			result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1;
			result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1;
			result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1;
			result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1;
			result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1;
		}
		return &result;

	case 3:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];
			flWeight2 = boneweights.weight[2];

			result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1 + boneMat2[0][0] * flWeight2;
			result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1 + boneMat2[0][1] * flWeight2;
			result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1 + boneMat2[0][2] * flWeight2;
			result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1 + boneMat2[0][3] * flWeight2;
			result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1 + boneMat2[1][0] * flWeight2;
			result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1 + boneMat2[1][1] * flWeight2;
			result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1 + boneMat2[1][2] * flWeight2;
			result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1 + boneMat2[1][3] * flWeight2;
			result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1 + boneMat2[2][0] * flWeight2;
			result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1 + boneMat2[2][1] * flWeight2;
			result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1 + boneMat2[2][2] * flWeight2;
			result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1 + boneMat2[2][3] * flWeight2;
		}
		return &result;

	case 4:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			matrix3x4_t &boneMat3 = pPoseToWorld[boneweights.bone[3]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];
			flWeight2 = boneweights.weight[2];
			flWeight3 = boneweights.weight[3];

			result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1 + boneMat2[0][0] * flWeight2 + boneMat3[0][0] * flWeight3;
			result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1 + boneMat2[0][1] * flWeight2 + boneMat3[0][1] * flWeight3;
			result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1 + boneMat2[0][2] * flWeight2 + boneMat3[0][2] * flWeight3;
			result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1 + boneMat2[0][3] * flWeight2 + boneMat3[0][3] * flWeight3;
			result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1 + boneMat2[1][0] * flWeight2 + boneMat3[1][0] * flWeight3;
			result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1 + boneMat2[1][1] * flWeight2 + boneMat3[1][1] * flWeight3;
			result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1 + boneMat2[1][2] * flWeight2 + boneMat3[1][2] * flWeight3;
			result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1 + boneMat2[1][3] * flWeight2 + boneMat3[1][3] * flWeight3;
			result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1 + boneMat2[2][0] * flWeight2 + boneMat3[2][0] * flWeight3;
			result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1 + boneMat2[2][1] * flWeight2 + boneMat3[2][1] * flWeight3;
			result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1 + boneMat2[2][2] * flWeight2 + boneMat3[2][2] * flWeight3;
			result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1 + boneMat2[2][3] * flWeight2 + boneMat3[2][3] * flWeight3;
		}
		return &result;
	}

	Assert(0);
	return NULL;
}


static matrix3x4_t *ComputeSkinMatrixSSE( mstudioboneweight_t &boneweights, matrix3x4_t *pPoseToWorld, matrix3x4_t &result )
{
	// NOTE: pPoseToWorld, being cache aligned, doesn't need explicit initialization
#ifdef _WIN32
	switch( boneweights.numbones )
	{
	default:
	case 1:
		return &pPoseToWorld[boneweights.bone[0]];

	case 2:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm6, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm7, dword ptr[eax + 4]	; boneweights.weight[1]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edi, DWORD PTR [result]

				// Fill xmm6, and 7 with all the bone weights
				shufps	xmm6, xmm6, 0
				shufps	xmm7, xmm7, 0

				// Load up all rows of the three matrices
				movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [eax + 16]
				movaps	xmm3, XMMWORD PTR [ecx + 16]
				movaps	xmm4, XMMWORD PTR [eax + 32]
				movaps	xmm5, XMMWORD PTR [ecx + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm6
				mulps	xmm1, xmm7
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7
				mulps	xmm4, xmm6
				mulps	xmm5, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm4, xmm5

				movaps	XMMWORD PTR [edi], xmm0
				movaps	XMMWORD PTR [edi + 16], xmm2
				movaps	XMMWORD PTR [edi + 32], xmm4
			}
		}
		return &result;

	case 3:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm5, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm6, dword ptr[eax + 4]	; boneweights.weight[1]
				movss	xmm7, dword ptr[eax + 8]	; boneweights.weight[2]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edx, DWORD PTR [boneMat2]
				mov		edi, DWORD PTR [result]

				// Fill xmm5, 6, and 7 with all the bone weights
				shufps	xmm5, xmm5, 0
				shufps	xmm6, xmm6, 0
				shufps	xmm7, xmm7, 0

				// Load up the first row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [edx]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
				mulps	xmm1, xmm6
				mulps	xmm2, xmm7

				addps	xmm0, xmm1
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi], xmm0
				
				// Load up the second row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 16]
				movaps	xmm1, XMMWORD PTR [ecx + 16]
				movaps	xmm2, XMMWORD PTR [edx + 16]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
				mulps	xmm1, xmm6
				mulps	xmm2, xmm7

				addps	xmm0, xmm1
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 16], xmm0	

				// Load up the third row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 32]
				movaps	xmm1, XMMWORD PTR [ecx + 32]
				movaps	xmm2, XMMWORD PTR [edx + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
				mulps	xmm1, xmm6
				mulps	xmm2, xmm7

				addps	xmm0, xmm1
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 32], xmm0	
			}
		}
		return &result;

	case 4:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			matrix3x4_t &boneMat3 = pPoseToWorld[boneweights.bone[3]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm4, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm5, dword ptr[eax + 4]	; boneweights.weight[1]
				movss	xmm6, dword ptr[eax + 8]	; boneweights.weight[2]
				movss	xmm7, dword ptr[eax + 12]	; boneweights.weight[3]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edx, DWORD PTR [boneMat2]
				mov		esi, DWORD PTR [boneMat3]
				mov		edi, DWORD PTR [result]

				// Fill xmm5, 6, and 7 with all the bone weights
				shufps	xmm4, xmm4, 0
				shufps	xmm5, xmm5, 0
				shufps	xmm6, xmm6, 0
				shufps	xmm7, xmm7, 0

				// Load up the first row of the four matrices
				movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [edx]
				movaps	xmm3, XMMWORD PTR [esi]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
				mulps	xmm1, xmm5
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi], xmm0
				
				// Load up the second row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 16]
				movaps	xmm1, XMMWORD PTR [ecx + 16]
				movaps	xmm2, XMMWORD PTR [edx + 16]
				movaps	xmm3, XMMWORD PTR [esi + 16]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
				mulps	xmm1, xmm5
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 16], xmm0	

				// Load up the third row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 32]
				movaps	xmm1, XMMWORD PTR [ecx + 32]
				movaps	xmm2, XMMWORD PTR [edx + 32]
				movaps	xmm3, XMMWORD PTR [esi + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
				mulps	xmm1, xmm5
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 32], xmm0	
			}
		}
		return &result;
	}
#elif _LINUX
#warning "ComputeSkinMatrixSSE C implementation only"
	return ComputeSkinMatrix( boneweights, pPoseToWorld, result );
#else
#error
#endif
	Assert(0);
	return NULL;
}


//-----------------------------------------------------------------------------
// Computes lighting
//-----------------------------------------------------------------------------
static lightpos_t lightpos[MAXLOCALLIGHTS];

inline void CStudioRender::R_ComputeLightAtPoint( const Vector &pos, const Vector &norm, Vector &color )
{
	Vector	ambientvalues;

	// Set up lightpos[i].dot, lightpos[i].falloff, and lightpos[i].delta for all lights
	R_LightStrengthWorld( pos, lightpos );

	// calculate ambientvalues from the ambient cube given a normal.
	R_LightAmbient( norm, ambientvalues );

	// Calculate color given lightpos_t lightpos, a normal, and the ambient
	// color from the ambient cube calculated above.
	R_LightEffectsWorld( lightpos, norm, ambientvalues, color );

}

#ifdef NEW_SOFTWARE_LIGHTING

inline void CStudioRender::R_ComputeLightAtPoint3( const Vector &pos, const Vector &norm, Vector &color )
{
	Vector	ambientvalues;

	// Set up lightpos[i].dot, lightpos[i].falloff, and lightpos[i].delta for all lights
	R_LightStrengthWorld( pos, lightpos );

	// calculate ambientvalues from the ambient cube given a normal.
	R_LightAmbient( norm, ambientvalues );

	// Calculate color given lightpos_t lightpos, a normal, and the ambient
	// color from the ambient cube calculated above.
	Assert(R_LightEffectsWorld3);

	//R_LightEffectsWorld( lightpos, norm, ambientvalues, color );
	R_LightEffectsWorld3( lightpos, norm, ambientvalues, color );
}

#endif

//-----------------------------------------------------------------------------
// Optimized for low-end hardware
//-----------------------------------------------------------------------------
#pragma warning (disable:4701)

// NOTE: I'm using this crazy wrapper because using straight template functions
// doesn't appear to work with function tables 
template< int nHasTangentSpace, int nDoFlex, int nHasSSE, int nLighting > 
class CProcessMeshWrapper
{
public:
	static void R_PerformLighting( const Vector &forward, float fIllum, 
		const Vector &pos, const Vector &norm, float r_blend, CMeshBuilder& meshBuilder )
	{
		Vector color;
		if (nLighting == LIGHTING_SOFTWARE)
		{
			#ifdef NEW_SOFTWARE_LIGHTING
				g_StudioRender.R_ComputeLightAtPoint3( pos, norm, color );
			#else
				g_StudioRender.R_ComputeLightAtPoint( pos, norm, color );
			#endif
			meshBuilder.Color4f( color.x, color.y, color.z, r_blend );
		}
		else if (nLighting == LIGHTING_MOUTH)
		{
			if (fIllum != 0.0f)
			{
				g_StudioRender.R_ComputeLightAtPoint( pos, norm, color );
				g_StudioRender.R_MouthLighting( fIllum, norm, forward, color );
				meshBuilder.Color4f( color.x, color.y, color.z, r_blend );
			}
			else
			{
				meshBuilder.Color4ub( 0, 0, 0, r_blend * 255.0f );
			}
		}
	}

	static void R_TransformVert( const Vector *pSrcPos, const Vector *pSrcNorm, const Vector4D *pSrcTangentS,
		matrix3x4_t *pSkinMat, VectorAligned &pos, VectorAligned &norm, Vector4DAligned &tangentS )
	{
		// NOTE: Could add SSE stuff here, if we knew what SSE stuff could make it faster

		pos.x =		pSrcPos->x * (*pSkinMat)[0][0]	+ pSrcPos->y * (*pSkinMat)[0][1]	+ pSrcPos->z * (*pSkinMat)[0][2] + (*pSkinMat)[0][3];
		norm.x =	pSrcNorm->x * (*pSkinMat)[0][0] + pSrcNorm->y * (*pSkinMat)[0][1]	+ pSrcNorm->z * (*pSkinMat)[0][2];

		pos.y =		pSrcPos->x * (*pSkinMat)[1][0]	+ pSrcPos->y * (*pSkinMat)[1][1]	+ pSrcPos->z * (*pSkinMat)[1][2] + (*pSkinMat)[1][3];
		norm.y =	pSrcNorm->x * (*pSkinMat)[1][0] + pSrcNorm->y * (*pSkinMat)[1][1]	+ pSrcNorm->z * (*pSkinMat)[1][2];

		pos.z =		pSrcPos->x * (*pSkinMat)[2][0]	+ pSrcPos->y * (*pSkinMat)[2][1]	+ pSrcPos->z * (*pSkinMat)[2][2] + (*pSkinMat)[2][3];
		norm.z =	pSrcNorm->x * (*pSkinMat)[2][0] + pSrcNorm->y * (*pSkinMat)[2][1]	+ pSrcNorm->z * (*pSkinMat)[2][2];

		if (nHasTangentSpace)
		{
			tangentS.x = pSrcTangentS->x * (*pSkinMat)[0][0] + pSrcTangentS->y * (*pSkinMat)[0][1]	+ pSrcTangentS->z * (*pSkinMat)[0][2];
			tangentS.y = pSrcTangentS->x * (*pSkinMat)[1][0] + pSrcTangentS->y * (*pSkinMat)[1][1]	+ pSrcTangentS->z * (*pSkinMat)[1][2];
			tangentS.z = pSrcTangentS->x * (*pSkinMat)[2][0] + pSrcTangentS->y * (*pSkinMat)[2][1]	+ pSrcTangentS->z * (*pSkinMat)[2][2];
			tangentS.w = pSrcTangentS->w;
		}
	}

	static void R_StudioSoftwareProcessMesh( mstudiomesh_t* pmesh, matrix3x4_t *pPoseToWorld,
		CCachedRenderData &vertexCache, CMeshBuilder& meshBuilder, int numVertices, unsigned short* pGroupToMesh, float r_blend )
	{
		Vector color;
		Vector4D *pStudioTangentS;
		VectorAligned norm, pos;
		Vector4DAligned tangentS;
		Vector *pSrcPos;
		Vector *pSrcNorm;
		Vector4D *pSrcTangentS = NULL;
#ifdef _WIN32		
		__declspec(align(16)) matrix3x4_t temp;
		__declspec(align(16)) matrix3x4_t *pSkinMat;
#elif _LINUX
		__attribute__((aligned(16))) matrix3x4_t temp;
		__attribute__((aligned(16))) matrix3x4_t *pSkinMat;
#endif
		int ntemp[4];

		Assert( numVertices > 0 );

		// Gets at the vertex data
		mstudiovertex_t *pVertices = pmesh->Vertex(0);
		if (nHasTangentSpace)
		{
			pStudioTangentS = pmesh->TangentS( 0 );
			Assert( pStudioTangentS->w == -1.0f || pStudioTangentS->w == 1.0f );
		}

		// Mouth related stuff...
		float fIllum = 1.0f;
		Vector forward;
		if (nLighting == LIGHTING_MOUTH)
		{
			g_StudioRender.R_MouthComputeLightingValues( fIllum, forward );
		}

		#ifdef NEW_SOFTWARE_LIGHTING
			if(nLighting == LIGHTING_SOFTWARE  )
			{
				g_StudioRender.R_InitLightEffectsWorld3();
			}
			// In debug, clear it out to ensure we aren't accidentially calling 
			// the last set R_LightEffectsWorld3.
			#ifdef _DEBUG
				else
				{
					g_StudioRender.R_LightEffectsWorld3 = NULL;
				}
			#endif
		#endif

		// Precaches the data
		for ( int i = 0; i < 4; ++i )
		{
			ntemp[i] = pGroupToMesh[i];

			if (nHasSSE)
			{
				char *pMem = (char*)&pVertices[ntemp[i]];
				_mm_prefetch( pMem, _MM_HINT_T0 );
				_mm_prefetch( pMem + 32, _MM_HINT_T0 );

				if (nHasTangentSpace)
				{
					_mm_prefetch( (char*)&pStudioTangentS[ntemp[i]], _MM_HINT_T0 );
				}
			}
		}

		int n, idx;
		for ( int j=0; j < numVertices; ++j )
		{
			idx = j & 0x3;
			n = ntemp[idx];

			mstudiovertex_t &vert = pVertices[n];

			ntemp[idx] = pGroupToMesh[j+4];

			// Compute the skinning matrix
			if (nHasSSE)
			{
				pSkinMat = ComputeSkinMatrixSSE( vert.m_BoneWeights, pPoseToWorld, temp );
			}
			else
			{
				pSkinMat = ComputeSkinMatrix( vert.m_BoneWeights, pPoseToWorld, temp );
			}
			
			// transform into world space
			if (nDoFlex && vertexCache.IsVertexFlexed(n))
			{
				CachedVertex_t* pFlexedVertex = vertexCache.GetFlexVertex(n);
				pSrcPos = &pFlexedVertex->m_Position;
				pSrcNorm = &pFlexedVertex->m_Normal;
				VectorNormalize( pFlexedVertex->m_Normal );

				if (nHasTangentSpace)
				{
					pSrcTangentS = &pFlexedVertex->m_TangentS;
					Assert( pSrcTangentS->w == -1.0f || pSrcTangentS->w == 1.0f );
				}
			}
			else
			{
				pSrcPos = &vert.m_vecPosition;
				pSrcNorm = &vert.m_vecNormal;

				if (nHasTangentSpace)
				{
					pSrcTangentS = &pStudioTangentS[n];
					Assert( pSrcTangentS->w == -1.0f || pSrcTangentS->w == 1.0f );
				}
			}

			// Transform the vert into world space
			R_TransformVert( pSrcPos, pSrcNorm, pSrcTangentS, pSkinMat, pos, norm, tangentS );

			// Compute lighting
			R_PerformLighting( forward, fIllum, pos, norm, r_blend, meshBuilder );

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Normal3fv( norm.Base() );
			meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );

			if (nHasTangentSpace)
			{
				Assert( tangentS.w == -1.0f || tangentS.w == 1.0f );
				meshBuilder.UserData( tangentS.Base() );
			}

	#ifdef _DEBUG
			// Set these to something so that ValidateData won't barf.
			meshBuilder.BoneWeight( 0, 0.0f );
			meshBuilder.BoneWeight( 1, 0.0f );
			meshBuilder.BoneWeight( 2, 0.0f );
			meshBuilder.BoneWeight( 3, 0.0f );
			meshBuilder.BoneMatrix( 0, 0 );
			meshBuilder.BoneMatrix( 1, 0 );
			meshBuilder.BoneMatrix( 2, 0 );
			meshBuilder.BoneMatrix( 3, 0 );
	#endif

			meshBuilder.AdvanceVertex();

			if (nHasSSE)
			{
				_mm_prefetch( (char*)&pVertices[ntemp[idx]], _MM_HINT_T0 );
				_mm_prefetch( (char*)&pVertices[ntemp[idx]] + 32, _MM_HINT_T0 );

				if (nHasTangentSpace)
				{
					_mm_prefetch( (char*)&pStudioTangentS[ntemp[idx]], _MM_HINT_T0 );
				}
			}
		}
	}
};


//-----------------------------------------------------------------------------
// Draws the mesh as tristrips using software
//-----------------------------------------------------------------------------
typedef CProcessMeshWrapper< false, false, false, LIGHTING_HARDWARE >	ProcessMesh000H_t;
typedef CProcessMeshWrapper< false, false, false, LIGHTING_SOFTWARE >	ProcessMesh000S_t;
typedef CProcessMeshWrapper< false, false, false, LIGHTING_MOUTH >		ProcessMesh000M_t;

typedef CProcessMeshWrapper< false, false, true, LIGHTING_HARDWARE >	ProcessMesh001H_t;
typedef CProcessMeshWrapper< false, false, true, LIGHTING_SOFTWARE >	ProcessMesh001S_t;
typedef CProcessMeshWrapper< false, false, true, LIGHTING_MOUTH >		ProcessMesh001M_t;

typedef CProcessMeshWrapper< false, true, false, LIGHTING_HARDWARE >	ProcessMesh010H_t;
typedef CProcessMeshWrapper< false, true, false, LIGHTING_SOFTWARE >	ProcessMesh010S_t;
typedef CProcessMeshWrapper< false, true, false, LIGHTING_MOUTH >		ProcessMesh010M_t;

typedef CProcessMeshWrapper< false, true, true, LIGHTING_HARDWARE >		ProcessMesh011H_t;
typedef CProcessMeshWrapper< false, true, true, LIGHTING_SOFTWARE >		ProcessMesh011S_t;
typedef CProcessMeshWrapper< false, true, true, LIGHTING_MOUTH >		ProcessMesh011M_t;

typedef CProcessMeshWrapper< true, false, false, LIGHTING_HARDWARE >	ProcessMesh100H_t;
typedef CProcessMeshWrapper< true, false, false, LIGHTING_SOFTWARE >	ProcessMesh100S_t;
typedef CProcessMeshWrapper< true, false, false, LIGHTING_MOUTH >		ProcessMesh100M_t;

typedef CProcessMeshWrapper< true, false, true, LIGHTING_HARDWARE >		ProcessMesh101H_t;
typedef CProcessMeshWrapper< true, false, true, LIGHTING_SOFTWARE >		ProcessMesh101S_t;
typedef CProcessMeshWrapper< true, false, true, LIGHTING_MOUTH >		ProcessMesh101M_t;

typedef CProcessMeshWrapper< true, true, false, LIGHTING_HARDWARE >		ProcessMesh110H_t;
typedef CProcessMeshWrapper< true, true, false, LIGHTING_SOFTWARE >		ProcessMesh110S_t;
typedef CProcessMeshWrapper< true, true, false, LIGHTING_MOUTH >		ProcessMesh110M_t;

typedef CProcessMeshWrapper< true, true, true, LIGHTING_HARDWARE >		ProcessMesh111H_t;
typedef CProcessMeshWrapper< true, true, true, LIGHTING_SOFTWARE >		ProcessMesh111S_t;
typedef CProcessMeshWrapper< true, true, true, LIGHTING_MOUTH >			ProcessMesh111M_t;

static SoftwareProcessMeshDX6Func_t g_SoftwareProcessFunc[] =
{
	ProcessMesh000H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh000S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh000M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh001H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh001S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh001M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh010H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh010S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh010M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh011H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh011S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh011M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh100H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh100S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh100M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh101H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh101S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh101M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh110H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh110S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh110M_t::R_StudioSoftwareProcessMesh,

	ProcessMesh111H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh111S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh111M_t::R_StudioSoftwareProcessMesh,
};


void CStudioRender::R_StudioSoftwareProcessMesh( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
		int numVertices, unsigned short* pGroupToMesh, StudioModelLighting_t lighting, bool doFlex, float r_blend,
		bool bNeedsTangentSpace )
{
	// FIXME: Use function pointers to simplify this?!?
	int idx	= bNeedsTangentSpace * 12 + doFlex * 6 + MathLib_SSEEnabled() * 3 + lighting;
	g_SoftwareProcessFunc[idx](pmesh, m_PoseToWorld, m_VertexCache, meshBuilder, numVertices, pGroupToMesh, r_blend ); 
}

static void R_SlowTransformVert( const Vector *pSrcPos, const Vector *pSrcNorm, const Vector4D *pSrcTangentS,
	matrix3x4_t *pSkinMat, VectorAligned &pos, VectorAligned &norm, Vector4DAligned &tangentS )
{
	pos.x =		pSrcPos->x * (*pSkinMat)[0][0]	+ pSrcPos->y * (*pSkinMat)[0][1]	+ pSrcPos->z * (*pSkinMat)[0][2] + (*pSkinMat)[0][3];
	norm.x =	pSrcNorm->x * (*pSkinMat)[0][0] + pSrcNorm->y * (*pSkinMat)[0][1]	+ pSrcNorm->z * (*pSkinMat)[0][2];

	pos.y =		pSrcPos->x * (*pSkinMat)[1][0]	+ pSrcPos->y * (*pSkinMat)[1][1]	+ pSrcPos->z * (*pSkinMat)[1][2] + (*pSkinMat)[1][3];
	norm.y =	pSrcNorm->x * (*pSkinMat)[1][0] + pSrcNorm->y * (*pSkinMat)[1][1]	+ pSrcNorm->z * (*pSkinMat)[1][2];

	pos.z =		pSrcPos->x * (*pSkinMat)[2][0]	+ pSrcPos->y * (*pSkinMat)[2][1]	+ pSrcPos->z * (*pSkinMat)[2][2] + (*pSkinMat)[2][3];
	norm.z =	pSrcNorm->x * (*pSkinMat)[2][0] + pSrcNorm->y * (*pSkinMat)[2][1]	+ pSrcNorm->z * (*pSkinMat)[2][2];

	tangentS.x = pSrcTangentS->x * (*pSkinMat)[0][0] + pSrcTangentS->y * (*pSkinMat)[0][1]	+ pSrcTangentS->z * (*pSkinMat)[0][2];
	tangentS.y = pSrcTangentS->x * (*pSkinMat)[1][0] + pSrcTangentS->y * (*pSkinMat)[1][1]	+ pSrcTangentS->z * (*pSkinMat)[1][2];
	tangentS.z = pSrcTangentS->x * (*pSkinMat)[2][0] + pSrcTangentS->y * (*pSkinMat)[2][1]	+ pSrcTangentS->z * (*pSkinMat)[2][2];
	tangentS.w = pSrcTangentS->w;
}

void CStudioRender::R_StudioSoftwareProcessMesh_Normals( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
		int numVertices, unsigned short* pGroupToMesh, StudioModelLighting_t lighting, bool doFlex, float r_blend,
		bool bNeedsTangentSpace )
{
#ifdef _WIN32
	__declspec(align(16)) matrix3x4_t temp;
	__declspec(align(16)) matrix3x4_t *pSkinMat;
#elif _LINUX
	__attribute__((aligned(16))) matrix3x4_t temp;
	__attribute__((aligned(16))) matrix3x4_t *pSkinMat;
#endif
	Vector *pSrcPos;
	Vector *pSrcNorm;
	Vector4D *pSrcTangentS = NULL;
	Vector4D *pStudioTangentS;
	VectorAligned norm, pos;
	Vector4DAligned tangentS;

	// Gets at the vertex data
	mstudiovertex_t *pVertices = pmesh->Vertex(0);
	pStudioTangentS = pmesh->TangentS( 0 );
	Assert( pStudioTangentS->w == -1.0f || pStudioTangentS->w == 1.0f );

	for ( int j=0; j < numVertices; j++ )
	{
		int n = pGroupToMesh[j];

		mstudiovertex_t &vert = pVertices[n];

		pSkinMat = ComputeSkinMatrix( vert.m_BoneWeights, m_PoseToWorld, temp );

		// transform into world space
		if (m_VertexCache.IsVertexFlexed(n))
		{
			CachedVertex_t* pFlexedVertex = m_VertexCache.GetFlexVertex(n);
			pSrcPos = &pFlexedVertex->m_Position;
			pSrcNorm = &pFlexedVertex->m_Normal;
			VectorNormalize( pFlexedVertex->m_Normal );

			pSrcTangentS = &pFlexedVertex->m_TangentS;
			Assert( pSrcTangentS->w == -1.0f || pSrcTangentS->w == 1.0f );
		}
		else
		{
			pSrcPos = &vert.m_vecPosition;
			pSrcNorm = &vert.m_vecNormal;

			pSrcTangentS = &pStudioTangentS[n];
			Assert( pSrcTangentS->w == -1.0f || pSrcTangentS->w == 1.0f );
		}

		// Transform the vert into world space
		R_SlowTransformVert( pSrcPos, pSrcNorm, pSrcTangentS, pSkinMat, pos, norm, tangentS );

		meshBuilder.Position3fv( pos.Base() );
		meshBuilder.Normal3f( 1.0f, 0.0f, 0.0f );
		meshBuilder.BoneWeight( 0, 1.0f );
		meshBuilder.BoneWeight( 1, 0.0f );
		meshBuilder.BoneWeight( 2, 0.0f );
		meshBuilder.BoneWeight( 3, 0.0f );
		meshBuilder.BoneMatrix( 0, 0 );
		meshBuilder.BoneMatrix( 1, 0 );
		meshBuilder.BoneMatrix( 2, 0 );
		meshBuilder.BoneMatrix( 3, 0 );
		meshBuilder.AdvanceVertex();

		Vector normalPos;
		normalPos = pos + norm * 2.0f;
		meshBuilder.Position3fv( normalPos.Base() );
		meshBuilder.Normal3f( 1.0f, 0.0f, 0.0f );
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

#pragma warning (default:4701)

//-----------------------------------------------------------------------------
// Processes a flexed mesh to be hw skinned
//-----------------------------------------------------------------------------
void CStudioRender::R_StudioProcessFlexedMesh( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
							int numVertices, unsigned short* pGroupToMesh )
{
	mstudiovertex_t *pVertices	= pmesh->Vertex(0);
	Vector4D *pstudiotangentS	= pmesh->TangentS( 0 );
	Assert( pstudiotangentS->w == -1.0f || pstudiotangentS->w == 1.0f );

	for ( int j=0; j < numVertices ; j++)
	{
		int n = pGroupToMesh[j];
		mstudiovertex_t &vert = pVertices[n];

		// Here, we are doing HW skinning, so we need to simply copy over the flex
		if (m_VertexCache.IsVertexFlexed(n))
		{
			CachedVertex_t* pFlexedVertex = m_VertexCache.GetFlexVertex(n);
			VectorNormalize( pFlexedVertex->m_Normal.Base() );
			meshBuilder.Position3fv( pFlexedVertex->m_Position.Base() );
			meshBuilder.Normal3fv( pFlexedVertex->m_Normal.Base() );
			Assert( pFlexedVertex->m_TangentS.w == -1.0f || pFlexedVertex->m_TangentS.w == 1.0f );
			meshBuilder.UserData( pFlexedVertex->m_TangentS.Base() );
		}
		else
		{
			meshBuilder.Position3fv( vert.m_vecPosition.Base() );
			meshBuilder.Normal3fv( vert.m_vecNormal.Base() );
			Assert( pstudiotangentS[n].w == -1.0f || pstudiotangentS[n].w == 1.0f );
			meshBuilder.UserData( pstudiotangentS[n].Base() );
		}
		meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );

		// FIXME: For now, flexed hw-skinned meshes can only have one bone
		// The data must exist in the 0th hardware matrix
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
// Restores the static mesh
//-----------------------------------------------------------------------------
void CStudioRender::R_StudioRestoreMesh( mstudiomesh_t* pmesh, 
									studiomeshgroup_t* pMeshData )
{
	mstudiovertex_t *pVertices	= pmesh->Vertex(0);
	Vector4D *pstudiotangentS	= pmesh->TangentS( 0 );

	CMeshBuilder meshBuilder;
	meshBuilder.BeginModify( pMeshData->m_pMesh );
	for ( int j=0; j < meshBuilder.NumVertices() ; j++)
	{
		meshBuilder.SelectVertex(j);
		int n = pMeshData->m_pGroupIndexToMeshIndex[j];
		mstudiovertex_t &vert = pVertices[n];

		meshBuilder.Position3fv( vert.m_vecPosition.Base() );
		meshBuilder.Normal3fv( vert.m_vecNormal.Base() );
		meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );
		Assert( pstudiotangentS[n].w == -1.0f || pstudiotangentS[n].w == 1.0f );
		meshBuilder.UserData( pstudiotangentS[n].Base() );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
	}
	meshBuilder.EndModify();
}


//-----------------------------------------------------------------------------
// Draws a mesh using hardware + software skinning
//-----------------------------------------------------------------------------

int CStudioRender::R_StudioDrawGroupHWSkin( studiomeshgroup_t* pGroup, IMesh* pMesh, IMesh* pColorMesh )
{
	MEASURE_TIMED_STAT( STUDIO_STATS_HW_SKIN_TIME );

	int numTrianglesRendered = 0;

	if( m_pStudioHdr->numbones == 1 )
	{
		m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
		m_pMaterialSystem->LoadMatrix( (float*)m_MaterialPoseToWorld[0] );
	}

	for (int j = 0; j < pGroup->m_NumStrips; ++j)
	{
		OptimizedModel::StripHeader_t* pStrip = &pGroup->m_pStripData[j];

		// Reset bone state if we're hardware skinning
		m_pMaterialSystem->SetNumBoneWeights( pStrip->numBones );

		for (int k = 0; k < pStrip->numBoneStateChanges; ++k)
		{
			OptimizedModel::BoneStateChangeHeader_t* pStateChange = pStrip->pBoneStateChange(k);
			if (pStateChange->newBoneID < 0)
				break;

			MaterialMatrixMode_t bone = MATERIAL_MODEL_MATRIX(pStateChange->hardwareID);

			StudioStats().IncrementCountedStat( STUDIO_STATS_MODEL_NUM_BONE_CHANGES, 1 );

			m_pMaterialSystem->MatrixMode( bone );
			m_pMaterialSystem->LoadMatrix( (float*)m_MaterialPoseToWorld[pStateChange->newBoneID] );
		}

		pMesh->SetPrimitiveType( pStrip->flags & OptimizedModel::STRIP_IS_TRISTRIP ? 
			MATERIAL_TRIANGLE_STRIP : MATERIAL_TRIANGLES );

//		g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_PRIMITIVES, pGroup->m_pUniqueTris[j] );
//		g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_DRAW_CALLS, 1 );

		if( m_Config.bStaticLighting && 
			m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			pMesh->SetColorMesh( pColorMesh );
		}
		else
		{
			pMesh->SetColorMesh( NULL );
		}
		pMesh->Draw( pStrip->indexOffset, pStrip->numIndices );
		pMesh->SetColorMesh( NULL );
		numTrianglesRendered += pGroup->m_pUniqueTris[j];
	}

	return numTrianglesRendered;
}

int CStudioRender::R_StudioDrawGroupSWSkin( studiomeshgroup_t* pGroup, IMesh* pMesh )
{
	int numTrianglesRendered = 0;
	
	// Disable skinning
	m_pMaterialSystem->SetNumBoneWeights( 0 );

	for (int j = 0; j < pGroup->m_NumStrips; ++j)
	{
		OptimizedModel::StripHeader_t* pStrip = &pGroup->m_pStripData[j];

		// Choose our primitive type
		pMesh->SetPrimitiveType( pStrip->flags & OptimizedModel::STRIP_IS_TRISTRIP ? 
			MATERIAL_TRIANGLE_STRIP : MATERIAL_TRIANGLES );

		pMesh->Draw( pStrip->indexOffset, pStrip->numIndices );
		numTrianglesRendered += pGroup->m_pUniqueTris[j];
	}

	return numTrianglesRendered;
}

//-----------------------------------------------------------------------------
// Draws the mesh as tristrips using hardware
//-----------------------------------------------------------------------------

int CStudioRender::R_StudioDrawStaticMesh( mstudiomesh_t* pmesh, 
				studiomeshgroup_t* pGroup, StudioModelLighting_t lighting, 
				float r_blend, IMaterial* pMaterial, IMesh **ppColorMeshes  )
{
//	MEASURE_TIMED_STAT( STUDIO_STATS_STATIC_MESH_TIME );

	bool vertexShaderMouthLighting = (lighting == LIGHTING_MOUTH) && 
		m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders();

	bool doSoftwareLighting = (m_Config.bSoftwareSkin != 0) ||
		(m_Config.bNormals != 0) ||
		(m_Config.bSoftwareLighting != 0) ||
		((lighting != LIGHTING_HARDWARE) && !vertexShaderMouthLighting ) ||
		( pMaterial ? pMaterial->NeedsSoftwareSkinning() : false );

	bool bNeedsTangentSpace = pMaterial ? pMaterial->NeedsTangentSpace() : false;

	int numTrianglesRendered = 0;

	// software lighting case
	if (doSoftwareLighting)
	{
		if( m_Config.bNoSoftware )
			return 0;

		m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
		m_pMaterialSystem->LoadIdentity();

		CMeshBuilder meshBuilder;
		IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh(false, 0, pGroup->m_pMesh);
		meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, pGroup->m_NumVertices, 0 );

		R_StudioSoftwareProcessMesh( pmesh, meshBuilder, 
			pGroup->m_NumVertices, pGroup->m_pGroupIndexToMeshIndex, lighting, false, r_blend, bNeedsTangentSpace );
		meshBuilder.End();

		numTrianglesRendered = R_StudioDrawGroupSWSkin( pGroup, pMesh );
		return numTrianglesRendered;
	}

	// Set up mouth material for rendering
	if (vertexShaderMouthLighting)
	{
		R_MouthSetupVertexShader( pMaterial );
	}

	// Needed when we switch back and forth between hardware + software lighting
	if (pGroup->m_MeshNeedsRestore)
	{
		R_StudioRestoreMesh(pmesh, pGroup);
		pGroup->m_MeshNeedsRestore = false;
	}

	// Draw it baby
	if( ppColorMeshes && ( pGroup->m_ColorMeshID != -1 ) )
	{
		numTrianglesRendered = R_StudioDrawGroupHWSkin( pGroup, pGroup->m_pMesh, ppColorMeshes[pGroup->m_ColorMeshID] );
	}
	else
	{
		numTrianglesRendered = R_StudioDrawGroupHWSkin( pGroup, pGroup->m_pMesh, NULL );
	}
	return numTrianglesRendered;
}


//-----------------------------------------------------------------------------
// Draws a dynamic mesh
//-----------------------------------------------------------------------------

int CStudioRender::R_StudioDrawDynamicMesh( mstudiomesh_t* pmesh, 
				studiomeshgroup_t* pGroup, StudioModelLighting_t lighting, 
				float r_blend, IMaterial* pMaterial )
{
	int numTrianglesRendered = 0;
	bool doFlex = ((pGroup->m_Flags & MESHGROUP_IS_FLEXED) != 0) &&
		m_Config.bFlex;

	// skin it and light it, but only if we need to.
	bool vertexShaderMouthLighting = (lighting == LIGHTING_MOUTH) && 
		m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders();

	bool doSoftwareLighting = (m_Config.bSoftwareLighting != 0) ||
		((lighting != LIGHTING_HARDWARE) && !vertexShaderMouthLighting );

	bool swSkin = doSoftwareLighting ||
		((pGroup->m_Flags & MESHGROUP_IS_HWSKINNED) == 0) ||
		m_Config.bSoftwareSkin ||
		( pMaterial ? pMaterial->NeedsSoftwareSkinning() : false );

	if (!doFlex && !swSkin && !m_Config.bNormals )
	{
		return R_StudioDrawStaticMesh( pmesh, pGroup, lighting, r_blend, pMaterial, NULL );
	}

#ifdef _DEBUG
	float mat[16];
	m_pMaterialSystem->GetMatrix( MATERIAL_MODEL, mat );
#endif

#ifdef _DEBUG
	const char *pDebugMaterialName = NULL;
	if( pMaterial )
	{
		pDebugMaterialName = pMaterial->GetName();
	}
#endif
	
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->LoadIdentity();

	if (doFlex)
	{
		R_StudioFlexVerts( pmesh );
	}

	IMesh* pMesh;
	bool bNeedsTangentSpace = pMaterial ? pMaterial->NeedsTangentSpace() : false;

	CMeshBuilder meshBuilder;
	pMesh = m_pMaterialSystem->GetDynamicMesh(false, 0, pGroup->m_pMesh);
	meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, pGroup->m_NumVertices, 0 );

	if (vertexShaderMouthLighting)
	{
		R_MouthSetupVertexShader( pMaterial );
	}

	if (swSkin)
	{
		R_StudioSoftwareProcessMesh( pmesh, meshBuilder, pGroup->m_NumVertices, 
			pGroup->m_pGroupIndexToMeshIndex, lighting, doFlex, r_blend, bNeedsTangentSpace );
	}
	else if (doFlex)
	{
		R_StudioProcessFlexedMesh( pmesh, meshBuilder, pGroup->m_NumVertices,
			pGroup->m_pGroupIndexToMeshIndex );
	}

	meshBuilder.End( );

	// Draw it baby
	if ( !swSkin )
	{
		numTrianglesRendered = R_StudioDrawGroupHWSkin( pGroup, pMesh );
	}
	else
	{
		numTrianglesRendered = R_StudioDrawGroupSWSkin( pGroup, pMesh );
	}

#if 1
	if( m_Config.bNormals )
	{
		m_pMaterialSystem->SetNumBoneWeights( 0 );
		m_pMaterialSystem->Bind( m_pMaterialMRMNormals );
		
		CMeshBuilder meshBuilder;
		pMesh = m_pMaterialSystem->GetDynamicMesh( false );
		meshBuilder.Begin( pMesh, MATERIAL_LINES, pGroup->m_NumVertices );

		R_StudioSoftwareProcessMesh_Normals( pmesh, meshBuilder, pGroup->m_NumVertices, 
			pGroup->m_pGroupIndexToMeshIndex, lighting, doFlex, r_blend, bNeedsTangentSpace );
		meshBuilder.End( );

		pMesh->Draw();
		m_pMaterialSystem->Bind( pMaterial );
	}
#endif
	
	return numTrianglesRendered;
}


//-----------------------------------------------------------------------------
// Sets the material vars for the vertex shader
//-----------------------------------------------------------------------------

void CStudioRender::SetEyeMaterialVars( IMaterial* pMaterial, mstudioeyeball_t* peyeball, 
		Vector const& eyeOrigin, const matrix3x4_t& irisTransform, const matrix3x4_t& glintTransform )
{
	if (!pMaterial)
		return;

/*
	Warning( "eyeOrigin: %f %f %f\n", eyeOrigin[0], eyeOrigin[1], eyeOrigin[2] );
	Warning( "irisTransform: " );
	int i, j;
	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j < 4; j++ )
		{
			Warning( "%f ", irisTransform[i][j] );
		}
		Warning( "\n" );
	}
	Warning( "glintTransform: " );
	for( i = 0; i < 3; i++ )
	{
		for( j = 0; j < 4; j++ )
		{
			Warning( "%f ", glintTransform[i][j] );
		}
		Warning( "\n" );
	}
*/
	
	bool found;
	IMaterialVar* pVar = pMaterial->FindVar( "$eyeorigin", &found, false );
	if (found)
	{
		pVar->SetVecValue( eyeOrigin.Base(), 3 );
	}

	pVar = pMaterial->FindVar( "$eyeup", &found, false );
	if (found)
	{
		pVar->SetVecValue( peyeball->up.Base(), 3 );
	}

	pVar = pMaterial->FindVar( "$irisu", &found, false );
	if (found)
	{
		pVar->SetVecValue( irisTransform[0], 4 );
	}

	pVar = pMaterial->FindVar( "$irisv", &found, false );
	if (found)
	{
		pVar->SetVecValue( irisTransform[1], 4 );
	}

	pVar = pMaterial->FindVar( "$glintu", &found, false );
	if (found)
	{
		pVar->SetVecValue( glintTransform[0], 4 );
	}

	pVar = pMaterial->FindVar( "$glintv", &found, false );
	if (found)
	{
		pVar->SetVecValue( glintTransform[1], 4 );
	}
}



int CStudioRender::R_StudioDrawEyeball( mstudiomesh_t* pmesh, studiomeshdata_t* pMeshData,
	StudioModelLighting_t lighting, IMaterial *pMaterial )
{
#ifndef IHVTEST
	MEASURE_TIMED_STAT( STUDIO_STATS_EYE_TIME );
	int numTrianglesRendered = 0;

	if( !m_Config.bEyes )
		return 0;

	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->LoadIdentity();

#ifdef _DEBUG
	float mat[16];
	m_pMaterialSystem->GetMatrix( MATERIAL_MODEL, mat );
#endif
	
	// FIXME: We could compile a static vertex buffer in this case
	// if there's no flexed verts.
	mstudiovertex_t *pVertices = pmesh->Vertex(0);

	R_StudioFlexVerts( pmesh );

	m_pMaterialSystem->SetNumBoneWeights( 0 );

	mstudioeyeball_t *peyeball = m_pSubModel->pEyeball(pmesh->materialparam);
	
	if( !m_Config.bWireframe )
	{
		// Compute the glint procedural texture
		bool found;
		IMaterialVar* pGlintVar = pMaterial->FindVar( "$glint", &found, false );
		if (found)
		{
			R_StudioEyeballGlint( &m_EyeballState[pmesh->materialparam], pGlintVar, 
				m_ViewRight, m_ViewUp, m_ViewOrigin );
		}
	}

	// We'll need this to compute normals
	Vector org;
	VectorTransform( peyeball->org, m_BoneToWorld[peyeball->bone], org );
//	Warning( "org: %f %f %f\n", peyeball->org[0], peyeball->org[1], peyeball->org[2] );
//	Warning( "peyeball->bone: %d\n", ( int )peyeball->bone );
/*
	{
		Warning( "m_BoneToWorld[peyeball->bone]\n" );
		int i, j;
		for( i = 0; i < 3; i++ )
		{
			for( j = 0; j < 4; j++ )
			{
				Warning( "%f ", m_BoneToWorld[peyeball->bone][i][j] );
			}
			Warning( "\n" );
		}
	}
	*/


	// Compute the glint projection
	matrix3x4_t glintMat;
	ComputeGlintTextureProjection( &m_EyeballState[pmesh->materialparam], m_ViewRight, m_ViewUp, glintMat );

	// Sets the material vars for the vertex shader
	if( !m_Config.bWireframe )
		SetEyeMaterialVars( pMaterial, peyeball, org, m_EyeballState[pmesh->materialparam].mat, glintMat );

	m_VertexCache.SetupComputation( pmesh );

	Vector	ambientvalues;
	static lightpos_t lightpos[MAXLOCALLIGHTS];
	Vector4D lightvalues;
	Vector position, normal;

	// Render the puppy
	CMeshBuilder meshBuilder;

	// Draw all the various mesh groups...
	for ( int j = 0; j < pMeshData->m_NumGroup; ++j )
	{
		studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];

		IMesh* pMesh = m_pMaterialSystem->GetDynamicMesh(false, 0, pGroup->m_pMesh);

		// garymcthack!  need to look at the strip flags to figure out what it is.
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, pmesh->numvertices, 0 );
//		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, pmesh->numvertices, 0 );
		
		for ( int i=0 ; i < pGroup->m_NumVertices ; ++i)
		{
			int n = pGroup->m_pGroupIndexToMeshIndex[i];
			mstudiovertex_t	&vert = pVertices[n];

			CachedVertex_t* pWorldVert = m_VertexCache.CreateWorldVertex(n);

			// transform into world space
			if (m_VertexCache.IsVertexFlexed(n))
			{
				CachedVertex_t* pFlexVert = m_VertexCache.GetFlexVertex(n);
				R_StudioTransform( pFlexVert->m_Position, &vert.m_BoneWeights, pWorldVert->m_Position );
				R_StudioRotate( pFlexVert->m_Normal, &vert.m_BoneWeights, pWorldVert->m_Normal );
				VectorNormalize( pWorldVert->m_Normal.Base() );
				Assert( pWorldVert->m_Normal.x >= -1.05f && pWorldVert->m_Normal.x <= 1.05f );
				Assert( pWorldVert->m_Normal.y >= -1.05f && pWorldVert->m_Normal.y <= 1.05f );
				Assert( pWorldVert->m_Normal.z >= -1.05f && pWorldVert->m_Normal.z <= 1.05f );
			}
			else
			{
				R_StudioTransform( vert.m_vecPosition, &vert.m_BoneWeights, pWorldVert->m_Position );
				R_StudioRotate( vert.m_vecNormal, &vert.m_BoneWeights, pWorldVert->m_Normal );
				Assert( pWorldVert->m_Normal.x >= -1.05f && pWorldVert->m_Normal.x <= 1.05f );
				Assert( pWorldVert->m_Normal.y >= -1.05f && pWorldVert->m_Normal.y <= 1.05f );
				Assert( pWorldVert->m_Normal.z >= -1.05f && pWorldVert->m_Normal.z <= 1.05f );
			}

			// Don't bother to light in software when we've got vertex + pixel shaders.
			if (m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() &&
				!m_Config.bSoftwareLighting)
			{
				meshBuilder.Normal3fv( pWorldVert->m_Normal.Base() );
			}
			else
			{
				R_StudioEyeballNormal( peyeball, org, pWorldVert->m_Position, pWorldVert->m_Normal );
#ifdef _DEBUG
				// This isn't really used, but since the meshbuilder checks for messed up
				// normals, let's do this here in debug mode.
				meshBuilder.Normal3fv( pWorldVert->m_Normal.Base() );
#endif

				// Set up light[i].dot, light[i].falloff, and light[i].delta for all lights
				R_LightStrengthWorld( pWorldVert->m_Position, lightpos );
				// calculate ambientvalues from the ambient cube given a normal.
				R_LightAmbient( pWorldVert->m_Normal, ambientvalues );
				// Calculate m_lightvalues[n] given lightpos_t lightpos, a normal, and the ambient
				// color from the ambient cube calculated above.
				R_LightEffectsWorld( lightpos, pWorldVert->m_Normal, ambientvalues, lightvalues.AsVector3D() );
				lightvalues[3] = m_AlphaMod;

				meshBuilder.Color4fv( lightvalues.Base() );
			}

			meshBuilder.Position3fv( pWorldVert->m_Position.Base() );
			meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );

			// FIXME: For now, flexed hw-skinned meshes can only have one bone
			// The data must exist in the 0th hardware matrix
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

		meshBuilder.End();
		pMesh->Draw();
		numTrianglesRendered += pGroup->m_pUniqueTris[j];

		if( m_Config.bNormals )
		{
			m_pMaterialSystem->SetNumBoneWeights( 0 );
			m_pMaterialSystem->Bind( m_pMaterialMRMNormals );
			
			CMeshBuilder meshBuilder;
			pMesh = m_pMaterialSystem->GetDynamicMesh( false );
			meshBuilder.Begin( pMesh, MATERIAL_LINES, pGroup->m_NumVertices );

			bool doFlex = true;
			bool r_blend = false;
			bool bNeedsTangentSpace = false;
			R_StudioSoftwareProcessMesh_Normals( pmesh, meshBuilder, pGroup->m_NumVertices, 
				pGroup->m_pGroupIndexToMeshIndex, lighting, doFlex, r_blend, bNeedsTangentSpace );
			meshBuilder.End( );

			pMesh->Draw();
			m_pMaterialSystem->Bind( pMaterial );
		}
		
	}

	return numTrianglesRendered;
#endif // IHVTEST
#ifdef IHVTEST
	return 0;
#endif // IHVTEST
}

//-----------------------------------------------------------------------------
// Draws normal meshes
//-----------------------------------------------------------------------------

int CStudioRender::R_StudioDrawMesh( mstudiomesh_t* pmesh, studiomeshdata_t* pMeshData,
									 StudioModelLighting_t lighting, IMaterial *pMaterial, 
									 IMesh **ppColorMeshes )
{
	MEASURE_TIMED_STAT( STUDIO_STATS_DRAW_MESH );

	int numTrianglesRendered = 0;

	// Draw all the various mesh groups...
	for ( int j = 0; j < pMeshData->m_NumGroup; ++j )
	{
		studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];

		// We can use the hardware if the mesh is not flexed 
		// and is hw skinned. Otherwise, we gotta do some expensive locks
		if ( ((pGroup->m_Flags & MESHGROUP_IS_FLEXED) == 0) && 
			 ((pGroup->m_Flags & MESHGROUP_IS_HWSKINNED) != 0) && 
			 !m_Config.bNormals )
		{
			if (!m_Config.bNoHardware)
			{
				numTrianglesRendered += R_StudioDrawStaticMesh( pmesh, pGroup, lighting, m_AlphaMod, pMaterial, ppColorMeshes );
			}
		}
		else
		{
			if (!m_Config.bNoSoftware)
			{
				numTrianglesRendered += R_StudioDrawDynamicMesh( pmesh, pGroup, lighting, m_AlphaMod, pMaterial );
			}
		}
	}
	return numTrianglesRendered;
}


//-----------------------------------------------------------------------------
// Inserts translucent mesh into list
//-----------------------------------------------------------------------------

template< class T >
void InsertRenderable( int mesh, T val, int count, int* pIndices, T* pValList )
{
	// Compute insertion point...
	int i;
	for ( i = count; --i >= 0; )
	{
		if (val < pValList[i])
			break;

		// Shift down
		pIndices[i + 1] = pIndices[i];
		pValList[i+1] = pValList[i];
	}

	// Insert at insertion point
	++i;
	pValList[i] = val;
	pIndices[i] = mesh;
}


//-----------------------------------------------------------------------------
// Sorts the meshes
//-----------------------------------------------------------------------------

int CStudioRender::SortMeshes( int* pIndices, IMaterial **ppMaterials, 
	short* pskinref, Vector const& vforward, Vector const& r_origin )
{
	int numMeshes = 0;
	if (m_bDrawTranslucentSubModels)
	{
//		float* pDist = (float*)_alloca( m_pSubModel->nummeshes * sizeof(float) );

		// Sort each model piece by it's center, if it's translucent
		for (int i = 0; i < m_pSubModel->nummeshes; ++i)
		{
			// Don't add opaque materials
			mstudiomesh_t*	pmesh = m_pSubModel->pMesh(i);
			IMaterial *pMaterial = ppMaterials[pskinref[pmesh->material]];
			if( !pMaterial || !pMaterial->IsTranslucent() )
				continue;

			// FIXME: put the "center" of the mesh into delta
//			Vector delta;
//			VectorSubtract( delta, r_origin, delta );
//			float dist = DotProduct( delta, vforward );

			// Add it to our lists
//			InsertRenderable( i, dist, numMeshes, pIndices, pDist );

			// One more mesh
			++numMeshes;
		}
	}
	else
	{
		IMaterial** ppMat = (IMaterial**)_alloca( m_pSubModel->nummeshes * sizeof(IMaterial*) );

		// Sort by material type
		for (int i = 0; i < m_pSubModel->nummeshes; ++i)
		{
			mstudiomesh_t*	pmesh = m_pSubModel->pMesh(i);
			IMaterial *pMaterial = ppMaterials[pskinref[pmesh->material]];
			if( !pMaterial )
				continue;

			// Don't add translucent materials
			if (( !m_Config.bWireframe ) && pMaterial->IsTranslucent() )
				continue;

			// Add it to our lists
			InsertRenderable( i, pMaterial, numMeshes, pIndices, ppMat );

			// One more mesh
			++numMeshes;
		}
	}

	return numMeshes;
}

/*
================
R_StudioDrawPoints
	General clipped case

inputs:
	pfinalverts	- points to buffer area.

outputs:
	returns the number of triangles rendered
================
*/

#pragma warning (disable:4189)

int CStudioRender::R_StudioDrawPoints( int skin, void /*IClientEntity*/ *pClientEntity, 
	IMaterial **ppMaterials, int *pMaterialFlags, IMesh **ppColorMeshes )
{
	int			i;
	int numTrianglesRendered = 0;

//	MEASURECODE( "R_StudioDrawPoints" );

#if 0 // garymcthack
	if (m_pSubModel->numfaces == 0)
		return 0;
#endif

	// happens when there's a model load failure
	if (m_pStudioMeshes == 0)
		return 0;

	if( m_Config.bWireframe && m_bDrawTranslucentSubModels )
	{
		return 0;
	}
	
	// Con_DPrintf("%d: %d %d\n", pimesh->numFaces, pimesh->numVertices, pimesh->numNormals );
	if (m_Config.skin)
	{
		skin = m_Config.skin;
		if (skin >= m_pStudioHdr->numskinfamilies)
			skin = 0;
	}

	// get skinref array
	short *pskinref	= m_pStudioHdr->pSkinref( 0 );
	if ( skin > 0 && skin < m_pStudioHdr->numskinfamilies )
	{
		pskinref += ( skin * m_pStudioHdr->numskinref );
	}

	// Compute all the bone matrices the way the material system wants it
	for ( i = 0; i < m_pStudioHdr->numbones; i++)
	{
		BoneMatToMaterialMat( m_PoseToWorld[i], m_MaterialPoseToWorld[i] );
	}

	// this has to run here becuase it effects flex targets
	for ( i = 0; i < m_pSubModel->numeyeballs; i++)
	{
		R_StudioEyeballPosition( m_pSubModel->pEyeball(i), &m_EyeballState[i] );
	}

	// FIXME: Activate sorting on a mesh level
//	int* pIndices = (int*)_alloca( m_pSubModel->nummeshes * sizeof(int) ); 
//	int numMeshes = SortMeshes( pIndices, ppMaterials, pskinref, vforward, r_origin );

	// draw each mesh
	for ( i = 0; i < m_pSubModel->nummeshes; ++i)
	{
		mstudiomesh_t	*pmesh		= m_pSubModel->pMesh(i);
		studiomeshdata_t *pMeshData = &m_pStudioMeshes[pmesh->meshid];
		Assert(pMeshData);

		IMaterial* pMaterial = R_StudioSetupSkin( pskinref[ pmesh->material ], ppMaterials, pClientEntity );
		if( !pMaterial )
			continue;

#ifdef _DEBUG
		char const *materialName = pMaterial->GetName();
#endif

		int materialFlags = (!m_pForcedMaterial) ? pMaterialFlags[pskinref[pmesh->material]] : 0;
		bool translucent = pMaterial->IsTranslucent();

		if( !m_Config.bWireframe )
		{
			if (( m_bDrawTranslucentSubModels && !translucent ) ||
				( !m_bDrawTranslucentSubModels && translucent ))
			{
				continue;
			}
		}

		StudioModelLighting_t lighting = R_StudioComputeLighting( pMaterial, materialFlags );
		if ((lighting == LIGHTING_MOUTH) && !m_Config.bTeeth)
			continue;

		// Set up flex data
		m_VertexCache.SetMesh( i );

		// The following are special cases that can't be covered with
		// the normal static/dynamic methods due to optimization reasons
		switch (pmesh->materialtype)
		{
		case 1:	// eyeballs
			if( m_bDrawTranslucentSubModels )
				break;

			numTrianglesRendered += R_StudioDrawEyeball( pmesh, pMeshData, lighting, pMaterial );
			break;

		default:
			numTrianglesRendered += R_StudioDrawMesh( pmesh, pMeshData, lighting, pMaterial, ppColorMeshes );
			break;
		}
	}

	// Reset this state so it doesn't hose other parts of rendering
	m_pMaterialSystem->SetNumBoneWeights( 0 );

	return numTrianglesRendered;
}

#pragma warning (default:4189)
