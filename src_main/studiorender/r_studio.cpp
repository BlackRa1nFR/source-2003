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
// r_studio.c: routines for setting up to draw 3DStudio models 
#include "studio.h"
#include "optimize.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "model_types.h"
#include "cstudiorender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// garymcthack - this should go elsewhere
#define MAX_NUM_BONE_INDICES 4

void CStudioRender::ForcedMaterialOverride( IMaterial *newMaterial, OverrideType_t nOverrideType )
{
	m_pForcedMaterial = newMaterial;
	m_nForcedMaterialType = nOverrideType;
}

IMaterial* CStudioRender::R_StudioSetupSkin( int index, IMaterial **ppMaterials,  
	void /*IClientEntity*/ *pClientEntity )
{
	if( m_Config.bWireframe )
	{
		m_pMaterialSystem->Bind( m_pMaterialMRMWireframe );
		return m_pMaterialMRMWireframe;
	}

	if( m_Config.bShowEnvCubemapOnly )
	{
		m_pMaterialSystem->Bind( m_pMaterialModelEnvCubemap );
		return m_pMaterialModelEnvCubemap;
	}

	IMaterial *pMaterial;
	if ( !m_pForcedMaterial )
	{
		pMaterial = ppMaterials[index];
	}
	else
	{
		pMaterial = m_pForcedMaterial;
		if (m_nForcedMaterialType == OVERRIDE_BUILD_SHADOWS)
		{
			// Connect the original material up to the shadow building material
			// Also bind the original material so its proxies are in the correct state
			bool bFound;
			IMaterialVar *pOriginalMaterialVar = pMaterial->FindVar("$translucent_material", &bFound );
			Assert( bFound );
			IMaterial *pOriginalMaterial = ppMaterials[index];
			if (pOriginalMaterial && (pOriginalMaterial->IsTranslucent() || pOriginalMaterial->IsAlphaTested()))
			{
				m_pMaterialSystem->Bind( pOriginalMaterial, pClientEntity );
				pOriginalMaterialVar->SetMaterialValue( pOriginalMaterial );
			}
			else
			{
				pOriginalMaterialVar->SetMaterialValue( NULL );
			}
		}
	}

	if( !pMaterial )
	{
		Assert( 0 );
		return 0;
	}

	// Try to set the alpha based on the blend
	pMaterial->AlphaModulate( m_AlphaMod );

	// Try to set the color based on the colormod
	pMaterial->ColorModulate( m_ColorMod[0], m_ColorMod[1], m_ColorMod[2] );

	m_pMaterialSystem->Bind( pMaterial, pClientEntity );
	return pMaterial;
}


//=============================================================================

static const char *GetTextureName( studiohdr_t *phdr, OptimizedModel::FileHeader_t *pVtxHeader, 
								   int lodID, int inMaterialID )
{
	OptimizedModel::MaterialReplacementListHeader_t *materialReplacementList = 
		pVtxHeader->pMaterialReplacementList( lodID );
	int i;
	for( i = 0; i < materialReplacementList->numReplacements; i++ )
	{
		OptimizedModel::MaterialReplacementHeader_t *materialReplacement =
			materialReplacementList->pMaterialReplacement( i );
		if( materialReplacement->materialID == inMaterialID )
		{
			const char *str = materialReplacement->pMaterialReplacementName();
			return str;
		}
	}
	return phdr->pTexture( inMaterialID )->pszName();
}

/*
=================
R_StudioLoadMaterials
=================
*/
typedef IMaterial *IMaterialPtr;
void CStudioRender::LoadMaterials( studiohdr_t *phdr, OptimizedModel::FileHeader_t *pVtxHeader,
								   studioloddata_t &lodData, int lodID )
{
	lodData.numMaterials = phdr->numtextures;
	if( lodData.numMaterials == 0 )
	{
		lodData.ppMaterials = NULL;
		return;
	}

	lodData.ppMaterials = new IMaterialPtr[lodData.numMaterials];
	Assert( lodData.ppMaterials );

	lodData.pMaterialFlags = new int[lodData.numMaterials];
	Assert( lodData.pMaterialFlags );

	int					i, j;

	Assert( phdr );

	// clear this flag since I think older versions of studiomdl didn't set the flag
	// to zero by default.
	phdr->flags &= ~STUDIOHDR_FLAGS_USES_ENV_CUBEMAP;
	phdr->flags &= ~STUDIOHDR_FLAGS_USES_FB_TEXTURE;
	phdr->flags &= ~STUDIOHDR_FLAGS_USES_BUMPMAPPING;
	// get index of each material
	if( phdr->textureindex != 0 )
	{
		for( i = 0; i < phdr->numtextures; i++ )
		{
			char szPath[256];
			IMaterial *pMaterial = NULL;
			bool found = false;
			// search through all specified directories until a valid material is found
			for( j = 0; j < phdr->numcdtextures && !found; j++ )
			{
				strcpy( szPath, phdr->pCdtexture( j ) );
				const char *textureName = GetTextureName( phdr, pVtxHeader, lodID, i );
				strcat( szPath, textureName );

				pMaterial = m_pMaterialSystem->FindMaterial( szPath, &found, false );
			}
			if( !found )
			{
				// hack - if it isn't found, go through the motions of looking for it again
				// so that the materialsystem will give an error.
				for( j = 0; j < phdr->numcdtextures; j++ )
				{
					strcpy( szPath, phdr->pCdtexture( j ) );
					const char *textureName = GetTextureName( phdr, pVtxHeader, lodID, i );
					strcat( szPath, textureName );
					m_pMaterialSystem->FindMaterial( szPath, NULL, true );
				}
			}

			lodData.ppMaterials[i] = pMaterial;
			
			// Increment the reference count for the material.
			if( pMaterial )
			{
				pMaterial->IncrementReferenceCount();
			}

			if( pMaterial->UsesEnvCubemap() )
			{
				phdr->flags |= STUDIOHDR_FLAGS_USES_ENV_CUBEMAP;
			}
			if( pMaterial->NeedsFrameBufferTexture() )
			{
				phdr->flags |= STUDIOHDR_FLAGS_USES_FB_TEXTURE;
			}
			bool bFound;
			// FIXME: I'd rather know that the material is definitely using the bumpmap.
			// It could be in the file without actually being used.
			IMaterialVar *pBumpMatVar = pMaterial->FindVar( "$bumpmap", &bFound, false );
			if( bFound && pBumpMatVar->IsDefined() && pMaterial->NeedsTangentSpace() )
			{
				phdr->flags |= STUDIOHDR_FLAGS_USES_BUMPMAPPING;
			}

			// FIXME: hack, needs proper client side material system interface
			lodData.pMaterialFlags[i] = 0;
			IMaterialVar *clientShaderVar = pMaterial->FindVar( "$clientShader", &found, false );
			if( found )
			{
				if (stricmp( clientShaderVar->GetStringValue(), "MouthShader") == 0)
				{
					lodData.pMaterialFlags[i] = 1;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Adds a vertex to the meshbuilder...
//-----------------------------------------------------------------------------
bool CStudioRender::R_AddVertexToMesh( CMeshBuilder& meshBuilder, 
	OptimizedModel::Vertex_t* pVertex, mstudiomesh_t* pMesh, bool hwSkin )
{
	bool ok = true;
	int idx = pVertex->origMeshVertID;

	mstudiovertex_t &vert = *pMesh->Vertex(idx);

	meshBuilder.Position3fv( vert.m_vecPosition.Base() );
	meshBuilder.Normal3fv( vert.m_vecNormal.Base() );
	meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );

#ifdef _DEBUG
//	float w = pMesh->TangentS( idx )->w;
//	Assert( w == 1.0f || w == -1.0f );
#endif
	// send down tangent S as a 4D userdata vect.
	meshBuilder.UserData( (*pMesh->TangentS(idx)).Base() );

	// Just in case we get hooked to a material that wants per-vertex color
	meshBuilder.Color4ub( 255, 255, 255, 255 );

	if (hwSkin)
	{
		// sum up weights..
		int i;

		// We have to do this because since we're potentially dropping bones
		// to get them to fit in hardware, we'll need to renormalize based on
		// the actual total.
		mstudioboneweight_t *pBoneWeight = pMesh->BoneWeights(idx);

		// NOTE: We use pVertex->numbones because that's the number of bones actually influencing this
		// vertex. Note that pVertex->numBones is not necessary the *desired* # of bones influencing this
		// vertex; we could have collapsed some of those bones out. pBoneWeight->numbones stures the desired #
		float totalWeight = 0;
		for (i = 0; i < pVertex->numBones; ++i)
		{
			totalWeight += pBoneWeight->weight[pVertex->boneWeightIndex[i]];
		}

		// The only way we should not add up to 1 is if there's more than 3 *desired* bones
		// and more than 1 *actual* bone (we can have 0	vertex bones in the case of static props
		if ( (pVertex->numBones > 0) && (pBoneWeight->numbones <= 3) && fabs(totalWeight - 1.0f) > 1e-5 )
		{
			ok = false;
			totalWeight = 1.0f;
		}

		// Fix up the static prop case
		if ( totalWeight == 0.0f )
		{
			totalWeight = 1.0f;
		}

		float invTotalWeight = 1.0f / totalWeight;

		// It is essential to iterate over all actual bones so that the bone indices
		// are set correctly, even though the last bone weight is computed in a shader program
		for (i = 0; i < pVertex->numBones; ++i)
		{
			if (pVertex->boneID[i] == -1)
			{
				meshBuilder.BoneWeight( i, 0.0f );
				meshBuilder.BoneMatrix( i, BONE_MATRIX_INDEX_INVALID );
			}
			else
			{
				float weight = pBoneWeight->weight[pVertex->boneWeightIndex[i]];
				meshBuilder.BoneWeight( i, weight * invTotalWeight );
				meshBuilder.BoneMatrix( i, pVertex->boneID[i] );
			}
		}
		for( ; i < MAX_NUM_BONE_INDICES; i++ )
		{
			meshBuilder.BoneWeight( i, 0.0f );
			meshBuilder.BoneMatrix( i, BONE_MATRIX_INDEX_INVALID );
		}
	}
	else
	{
		for (int i = 0; i < MAX_NUM_BONE_INDICES/*meshBuilder.NumBoneWeights()*/; ++i)
		{
			meshBuilder.BoneWeight( i, (i == 0) ? 1.0f : 0.0f );
			meshBuilder.BoneMatrix( i, BONE_MATRIX_INDEX_INVALID );
		}
	}

	meshBuilder.AdvanceVertex();
	return ok;
}


//-----------------------------------------------------------------------------
// Builds the group
//-----------------------------------------------------------------------------

void CStudioRender::R_StudioBuildMeshGroup( studiomeshgroup_t* pMeshGroup,
		OptimizedModel::StripGroupHeader_t *pStripGroup, mstudiomesh_t* pMesh,
		studiohdr_t *pStudioHdr )
{
	// We have to do this here because of skinning; there may be any number of
	// materials that are applied to this mesh.
	// Copy over all the vertices + indices in this strip group
	pMeshGroup->m_pMesh = m_pMaterialSystem->CreateStaticMesh( MATERIAL_VERTEX_FORMAT_MODEL, false );
	pMeshGroup->m_ColorMeshID = -1;
	pMeshGroup->m_pColorMesh = NULL;

	bool hwSkin = (pMeshGroup->m_Flags & MESHGROUP_IS_HWSKINNED) != 0;

	// This mesh could have tristrips or trilists in it
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMeshGroup->m_pMesh, MATERIAL_HETEROGENOUS, 
		hwSkin ? pStripGroup->numVerts : 0, pStripGroup->numIndices );

	int i;
	bool ok = true;
	if (hwSkin)
	{
		for (i = 0; i < pStripGroup->numVerts; ++i)
		{
			if (!R_AddVertexToMesh( meshBuilder, pStripGroup->pVertex(i), pMesh, hwSkin ))
				ok = false;
		}
	}

	for (i = 0; i < pStripGroup->numIndices; ++i)
	{
		meshBuilder.Index( *pStripGroup->pIndex(i) );
		meshBuilder.AdvanceIndex();
	}

	meshBuilder.End();

	// Copy over the strip indices. We need access to the indices for decals
	pMeshGroup->m_pIndices = new unsigned short[ pStripGroup->numIndices ];
	memcpy( pMeshGroup->m_pIndices, pStripGroup->pIndex(0), 
		pStripGroup->numIndices * sizeof(unsigned short) );

	// Compute the number of non-degenerate trianges in each strip group
	// for statistics gathering
    pMeshGroup->m_pUniqueTris = new int[ pStripGroup->numStrips ];

	for (i = 0; i < pStripGroup->numStrips; ++i )
	{
		int numUnique = 0;
		if (pStripGroup->pStrip(i)->flags & OptimizedModel::STRIP_IS_TRISTRIP) 
		{
			int last[2] = {-1, -1};
			int curr = pStripGroup->pStrip(i)->indexOffset;
			int end = curr + pStripGroup->pStrip(i)->numIndices;
			while (curr != end)
			{
				int idx = *pStripGroup->pIndex(curr);
				if (idx != last[0] && idx != last[1] && last[0] != last[1] && last[0] != -1)
					++numUnique;
				last[0] = last[1];
				last[1] = idx;
				++curr;
			}
		}
		else
			numUnique = pStripGroup->pStrip(i)->numIndices / 3;
		pMeshGroup->m_pUniqueTris[i] = numUnique;
	}

	if (!ok)
	{
		mstudiomodel_t* pModel = pMesh->pModel();
		Con_Printf("Bad data found in model \"%s\" (bad bone weights)\n", pModel->name);
	}
}

//-----------------------------------------------------------------------------
// Builds the strip data
//-----------------------------------------------------------------------------
void CStudioRender::R_StudioBuildMeshStrips( studiomeshgroup_t* pMeshGroup,
							OptimizedModel::StripGroupHeader_t *pStripGroup )
{
	// FIXME: This is bogus
	// Compute the amount of memory we need to store the strip data
	int i;
	int stripDataSize = 0;
	for( i = 0; i < pStripGroup->numStrips; ++i )
	{
		stripDataSize += sizeof(OptimizedModel::StripHeader_t);
		stripDataSize += pStripGroup->pStrip(i)->numBoneStateChanges *
			sizeof(OptimizedModel::BoneStateChangeHeader_t);
	}

	pMeshGroup->m_pStripData = (OptimizedModel::StripHeader_t*)malloc(stripDataSize);

	// Copy over the strip info
	int boneStateChangeOffset = pStripGroup->numStrips * sizeof(OptimizedModel::StripHeader_t);
	for( i = 0; i < pStripGroup->numStrips; ++i )
	{
		memcpy( &pMeshGroup->m_pStripData[i], pStripGroup->pStrip(i),
			sizeof( OptimizedModel::StripHeader_t ) );

		// Fixup the bone state change offset, since we have it right after the strip data
		pMeshGroup->m_pStripData[i].boneStateChangeOffset = boneStateChangeOffset -
			i * sizeof(OptimizedModel::StripHeader_t);

		// copy over bone state changes
		int boneWeightSize = pMeshGroup->m_pStripData[i].numBoneStateChanges * 
			sizeof(OptimizedModel::BoneStateChangeHeader_t);

		if (boneWeightSize != 0)
		{
			unsigned char* pBoneStateChange = (unsigned char*)pMeshGroup->m_pStripData + boneStateChangeOffset;
			memcpy( pBoneStateChange, pStripGroup->pStrip(i)->pBoneStateChange(0), boneWeightSize);

			boneStateChangeOffset += boneWeightSize;
		}
	}
	pMeshGroup->m_NumStrips = pStripGroup->numStrips;
}

//-----------------------------------------------------------------------------
// Creates a single mesh
//-----------------------------------------------------------------------------

void CStudioRender::R_StudioCreateSingleMesh(mstudiomesh_t* pMesh, 
	OptimizedModel::MeshHeader_t* pVtxMesh, int numBones, studiomeshdata_t* pMeshData,
	studiohdr_t *pStudioHdr )
{
	// Here are the cases where we don't use any meshes at all...
	// In the case of eyes, we're just gonna use dynamic buffers
	// because it's the fastest solution (prevents lots of locks)

	// Each strip group represents a locking group, it's a set of vertices
	// that are locked together, and, potentially, software light + skinned together
	pMeshData->m_NumGroup = pVtxMesh->numStripGroups;
	pMeshData->m_pMeshGroup = new studiomeshgroup_t[pVtxMesh->numStripGroups];

	for (int i = 0; i < pVtxMesh->numStripGroups; ++i )
	{
		OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup(i);
		studiomeshgroup_t* pMeshGroup = &pMeshData->m_pMeshGroup[i];

		pMeshGroup->m_MeshNeedsRestore = false;

		// Set the flags...
		pMeshGroup->m_Flags = 0;
		if (pStripGroup->flags & OptimizedModel::STRIPGROUP_IS_FLEXED)
			pMeshGroup->m_Flags |= MESHGROUP_IS_FLEXED;
		if (pStripGroup->flags & OptimizedModel::STRIPGROUP_IS_HWSKINNED)
			pMeshGroup->m_Flags |= MESHGROUP_IS_HWSKINNED;

		// Build the vertex + index buffers
		R_StudioBuildMeshGroup( pMeshGroup, pStripGroup, pMesh, pStudioHdr );

		// Copy over the tristrip and triangle list data
		R_StudioBuildMeshStrips( pMeshGroup, pStripGroup );

		// Build the mapping from strip group vertex idx to actual mesh idx
		pMeshGroup->m_pGroupIndexToMeshIndex = new unsigned short[pStripGroup->numVerts + 4];
		pMeshGroup->m_NumVertices = pStripGroup->numVerts;
		for (int j = 0; j < pStripGroup->numVerts; ++j)
		{
			pMeshGroup->m_pGroupIndexToMeshIndex[j] = pStripGroup->pVertex(j)->origMeshVertID;
		}

		// Extra copies are for precaching...
		pMeshGroup->m_pGroupIndexToMeshIndex[pStripGroup->numVerts] = 
			pMeshGroup->m_pGroupIndexToMeshIndex[pStripGroup->numVerts+1] =
			pMeshGroup->m_pGroupIndexToMeshIndex[pStripGroup->numVerts+2] =
			pMeshGroup->m_pGroupIndexToMeshIndex[pStripGroup->numVerts+3] = pMeshGroup->m_pGroupIndexToMeshIndex[pStripGroup->numVerts - 1];
	}
}

/*
int CStudioRender::CalculateNumVerticesForWholeModel( studiohdr_t *pStudioHdr, 
													   OptimizedModel::FileHeader_t* pVtxHdr )
{
	int totalVerts = 0;
	
	bool hwSkin = (pMeshGroup->m_Flags & MESHGROUP_IS_HWSKINNED) != 0;

	// Iterate over every body part...
	for ( i = 0; i < pStudioHdr->numbodyparts; i++ )
	{
		mstudiobodyparts_t* pBodyPart = pStudioHdr->pBodypart(i);
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart(i);

		// Iterate over every submodel...
		for (j = 0; j < pBodyPart->nummodels; ++j)
		{
			mstudiomodel_t* pModel = pBodyPart->pModel(j);
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel(j);
			
			OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( lodID );
			// Iterate over all the meshes....
			for (k = 0; k < pModel->nummeshes; ++k)
			{
				Assert( pModel->nummeshes == pVtxLOD->numMeshes );
				mstudiomesh_t* pMesh = pModel->pMesh(k);
				OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh(k);

				for( l = 0; l < pVtxMesh->numStripGroups; l++ )
				{
					totalVerts += pStripGroup->numVerts;
				}
			}
		}
	}
	return totalVerts;
}
*/

//-----------------------------------------------------------------------------
// Creates static meshes
//-----------------------------------------------------------------------------

bool CStudioRender::R_StudioCreateStaticMeshes(const char *pModelName, studiohdr_t *pStudioHdr, 
								OptimizedModel::FileHeader_t* pVtxHdr,
								int numStudioMeshes, studiomeshdata_t **ppStudioMeshes,
								int lodID )
{
	if (!pVtxHdr)
	{
		Con_DPrintf("Error! Model %s has unreadable .vtx file\n", pModelName );
		return false;
	}
	if (!pStudioHdr)
	{
		Con_DPrintf("Error! Model %s has unreadable .mdl file\n", pModelName );
		return false;
	}
	if( pVtxHdr->checkSum != pStudioHdr->checksum )
	{								  
		Con_DPrintf("Error! Model %s .vtx file out of synch with .mdl\n", pModelName );
		return false;
	}

	
	Assert( !*ppStudioMeshes );
	*ppStudioMeshes = new studiomeshdata_t[numStudioMeshes];

	int					i, j, k;

	// Iterate over every body part...
	for ( i = 0; i < pStudioHdr->numbodyparts; i++ )
	{
		mstudiobodyparts_t* pBodyPart = pStudioHdr->pBodypart(i);
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart(i);

		// Iterate over every submodel...
		for (j = 0; j < pBodyPart->nummodels; ++j)
		{
			mstudiomodel_t* pModel = pBodyPart->pModel(j);
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel(j);
			
			OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( lodID );
			// Iterate over all the meshes....
			for (k = 0; k < pModel->nummeshes; ++k)
			{
				Assert( pModel->nummeshes == pVtxLOD->numMeshes );
				mstudiomesh_t* pMesh = pModel->pMesh(k);
				OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh(k);

				Assert( pMesh->meshid < numStudioMeshes );
				R_StudioCreateSingleMesh( pMesh, pVtxMesh, pVtxHdr->maxBonesPerVert, 
					&((*ppStudioMeshes)[pMesh->meshid]), pStudioHdr );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Destroys static meshes
//-----------------------------------------------------------------------------

void CStudioRender::R_StudioDestroyStaticMeshes( int numStudioMeshes, studiomeshdata_t **ppStudioMeshes )
{
	if( !*ppStudioMeshes)
		return;

	// Iterate over every body mesh...
	for ( int i = 0; i < numStudioMeshes; ++i )
	{
		studiomeshdata_t* pMesh = &((*ppStudioMeshes)[i]);

		for (int j = 0; j < pMesh->m_NumGroup; ++j)
		{
			studiomeshgroup_t* pGroup = &pMesh->m_pMeshGroup[j];
			if (pGroup->m_pGroupIndexToMeshIndex)
			{
				delete[] pGroup->m_pGroupIndexToMeshIndex;
				pGroup->m_pGroupIndexToMeshIndex = 0;
			}

			if (pGroup->m_pUniqueTris)
			{
				delete [] pGroup->m_pUniqueTris;
				pGroup->m_pUniqueTris = 0;
			}

			if (pGroup->m_pIndices)
			{
				delete [] pGroup->m_pIndices;
				pGroup->m_pIndices = 0;
			}

			if (pGroup->m_pMesh)
			{
				m_pMaterialSystem->DestroyStaticMesh( pGroup->m_pMesh );
				pGroup->m_pMesh = 0;
			}

			if (pGroup->m_pStripData)
			{
				free( pGroup->m_pStripData );
				pGroup->m_pStripData = 0;
			}
		}

		if (pMesh->m_pMeshGroup)
		{
			delete[] pMesh->m_pMeshGroup;
			pMesh->m_pMeshGroup = 0;
		}
	}

	if (*ppStudioMeshes)
	{
		delete 	*ppStudioMeshes;
		*ppStudioMeshes = 0;
	}
}


/*
=================
R_StudioUnloadMaterials
=================
*/
void CStudioRender::UnloadMaterials( int numMaterials, IMaterial **ppMaterials )
{
	int					i;

	for( i = 0; i < numMaterials; i++ )
	{
		if( ppMaterials[i] )
		{
			ppMaterials[i]->DecrementReferenceCount();
		}
	}
	delete [] ppMaterials;
}


//=============================================================================


/*
=================
R_StudioSetupModel
	based on the body part, figure out which mesh it should be using.
inputs:
outputs:
	pstudiomesh
	pmdl
=================
*/
int CStudioRender::R_StudioSetupModel ( int bodypart, int entity_body, mstudiomodel_t **ppSubModel, 
									   studiohdr_t *pStudioHdr ) const
{
	int index;
	mstudiobodyparts_t   *pbodypart;

	if (bodypart > pStudioHdr->numbodyparts)
	{
		Con_DPrintf ("R_StudioSetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	pbodypart = pStudioHdr->pBodypart( bodypart );

	index = entity_body / pbodypart->base;
	index = index % pbodypart->nummodels;

	Assert( ppSubModel );
	*ppSubModel = pbodypart->pModel( index );
	return index;
}

