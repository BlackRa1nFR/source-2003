#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <float.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "studio.h"
#include "studiomdl.h"
#include "bone_setup.h"
#include "vstdlib/strtools.h"
#include "vmatrix.h"

void ValidateBoneWeights( const s_source_t *pSrc );


s_source_t* GetModelLODSource( const char *pModelName, 
								const LodScriptData_t& scriptLOD, bool* pFound )
{
	// When doing LOD replacement, ignore all path + extension information
	char* pTempBuf = (char*)_alloca( strlen(pModelName) + 1 );

	// Strip off extensions for the source...
	strcpy( pTempBuf, pModelName ); 
	char* pDot = strrchr( pTempBuf, '.' );
	if (pDot)
		*pDot = 0;

	for( int i = 0; i < scriptLOD.modelReplacements.Size(); i++ )
	{
		// FIXME: Should we strip off path information?
//		char* pSlash = strrchr( pTempBuf1, '\\' );
//		char* pSlash2 = strrchr( pTempBuf1, '/' );
//		if (pSlash2 > pSlash)
//			pSlash = pSlash2;
//		if (!pSlash)
//			pSlash = pTempBuf1;

		if( !stricmp( pTempBuf, scriptLOD.modelReplacements[i].GetSrcName() ) )
		{
			*pFound = true;
			return scriptLOD.modelReplacements[i].m_pSource;
		}
	}

	*pFound = false;
	return 0;
}

#define POSITION_EPSILON ( .05f )
#define TEXCOORD_EPSILON ( .05f )
#define NORMAL_EPSILON ( 5.0f ) // in degrees
#define BONEWEIGHT_EPSILON ( .05f )

bool CompareNormalFuzzy( Vector n1, Vector n2 )
{ 
	static float epsilon = cos( DEG2RAD( NORMAL_EPSILON ) );
	VectorNormalize( n1 );
	VectorNormalize( n2 );
	float dot = DotProduct( n1, n2 );
	if( dot >= epsilon )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ComparePositionFuzzy( const Vector &p1, const Vector &p2 )
{
	Vector delta = p1 - p2;
	float distSquared = DotProduct( delta, delta );
	if( distSquared < ( POSITION_EPSILON * POSITION_EPSILON ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CompareTexCoordsFuzzy( const Vector2D &t1, const Vector2D &t2 )
{
	if( fabs( t2[0] - t1[0] ) > TEXCOORD_EPSILON ||
		fabs( t2[1] - t1[1] ) > TEXCOORD_EPSILON )
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool CompareBoneWeights( const s_boneweight_t &b1, const s_boneweight_t &b2 )
{
	return( memcmp( &b1, &b2, sizeof( b1 ) ) == 0 );
}

bool CompareBoneWeightsFuzzy( const s_boneweight_t &b1, const s_boneweight_t &b2 )
{
	if( b1.numbones != b2.numbones )
	{
		return false;
	}
	int i;
	for( i = 0; i < b1.numbones; i++ )
	{
		if( b1.bone[i] != b2.bone[i] )
		{
			return false;
		}
		if( fabs( b1.weight[i] - b2.weight[i] ) > BONEWEIGHT_EPSILON )
		{
			return false;
		}
	}
	return true;
}

int FindMaterialByName( const char *pMaterialName )
{
	int i;
	int allocLen = strlen( pMaterialName ) + 1;
	char *pBaseName = ( char * )_alloca( allocLen );
	ExtractFileBase( ( char * )pMaterialName, pBaseName, allocLen );

	for( i = 0; i < g_numtextures; i++ )
	{
		if( stricmp( pBaseName, g_texture[i].name ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

static void GetLODSources( CUtlVector<s_source_t *> &lods, const s_model_t *pSrcModel )
{
	int lodID;
	int numLODs;

 	numLODs = g_ScriptLODs.Size();

	for( lodID = 0; lodID < numLODs; lodID++ )
	{
		LodScriptData_t& scriptLOD = g_ScriptLODs[lodID];

		bool found;
		s_source_t* pSource = GetModelLODSource( pSrcModel->filename, scriptLOD, &found );
		if (!pSource && (!found))
		{
			pSource = pSrcModel->source;
		}

		lods[lodID] = pSource;

		/*
		if (lods[lodID])
		{
			ValidateBoneWeights( lods[lodID] );
		}
		*/
	}
}

static s_mesh_t *FindMeshByMaterial( s_source_t *pSrc, int materialID )
{
	int lodMeshID = -1;
	int m;
	for( m = 0; m < pSrc->nummeshes; m++ )
	{
//		if( pSrc->texmap[pSrc->meshindex[m]] == materialID )
		if( pSrc->meshindex[m] == materialID )
		{
			lodMeshID = m;
			break;
		}
	}
	if( lodMeshID == -1 )
	{
		// this mesh/material doesn't exist at this lod.
		return NULL;
	}
	else
	{
		return &pSrc->mesh[pSrc->meshindex[lodMeshID]];
	}
}

static void ValidateBoneWeight( const s_boneweight_t &boneWeight, const s_source_t *pSrc )
{
#ifdef _DEBUG
	int i;
	if( boneWeight.weight[0] == 1.0f )
	{
		assert( boneWeight.numbones == 1 );
	}
	for( i = 0; i < boneWeight.numbones; i++ )
	{
		assert( boneWeight.bone[i] >= 0 && boneWeight.bone[i] < g_numbones );
	}

	float weight = 0.0f;
	for( i = 0; i < boneWeight.numbones; i++ )
	{
		weight += boneWeight.weight[i] ;
	}
	assert( fabs( weight - 1.0f ) < 1e-3 );
#endif
}

static void SortBoneWeightByWeight( s_boneweight_t &boneWeight );
static void CopyVertsToTempArrays( const s_source_t *pSrc, const s_mesh_t *pSrcMesh, CUtlVector<Vector> &verts,
								  CUtlVector<s_boneweight_t> &boneWeights, 
								  CUtlVector<s_vertexinfo_t> &vertexInfo, 
								  CUtlVector<Vector> &normals,
								  CUtlVector<Vector2D> &texcoords, s_mesh_t *pDstMesh )
{
	// just copy the verts over instead of searching for them.
	int srcVertID;
	for( srcVertID = 0; srcVertID < pSrcMesh->numvertices; srcVertID++ )
	{
		int dstVertID = verts.AddToTail();
		boneWeights.AddToTail();
		vertexInfo.AddToTail();
		normals.AddToTail();
		texcoords.AddToTail();
		verts[dstVertID] = pSrc->vertex[pSrcMesh->vertexoffset + srcVertID];
		boneWeights[dstVertID] = pSrc->globalBoneweight[pSrcMesh->vertexoffset + srcVertID];
		vertexInfo[dstVertID] = pSrc->vertexInfo[pSrcMesh->vertexoffset + srcVertID];
		ValidateBoneWeight( boneWeights[dstVertID], pSrc );
		SortBoneWeightByWeight( boneWeights[dstVertID] );
		ValidateBoneWeight( boneWeights[dstVertID], pSrc );
		normals[dstVertID] = pSrc->normal[pSrcMesh->vertexoffset + srcVertID];
		texcoords[dstVertID] = pSrc->texcoord[pSrcMesh->vertexoffset + srcVertID];
	}
	pDstMesh->numvertices = pSrcMesh->numvertices;
}

static void CopyFacesToTempArray( const s_source_t *pSrc, const s_mesh_t *pSrcMesh, CUtlVector<s_face_t> &faces, s_mesh_t *pDstMesh )
{
	int srcFaceID;
	for( srcFaceID = 0; srcFaceID < pSrcMesh->numfaces; srcFaceID++ )
	{
		int srcID = srcFaceID + pSrcMesh->faceoffset;
		s_face_t *pSrcFace = &pSrc->face[srcID];
		s_face_t *pDstFace = &faces[faces.AddToTail()];
		pDstFace->a = pSrcFace->a;
		pDstFace->b = pSrcFace->b;
		pDstFace->c = pSrcFace->c;
		pDstMesh->numfaces++;
	}
}

// return -1 if there is no match
// the index returned is mesh relative!
static int FindVertexWithinMesh( const s_boneweight_t &boneWeight, const Vector &position, 
							     const Vector &normal, const Vector2D &texCoord,
								 const s_mesh_t *pMesh, const s_source_t *pSrc, 
								 bool ignoreBoneWeight )
{
	int i;
	int closestIndex = -1;
	float closestDistSquared = FLT_MAX;
	for( i = 0; i < pMesh->numvertices; i++ )
	{
		// see if the position is reasonable
		if( !ComparePositionFuzzy( position, pSrc->vertex[i + pMesh->vertexoffset] ) )
		{
			continue;
		}
		// see if the normal is reasonable
		if( !CompareNormalFuzzy( normal, pSrc->normal[i + pMesh->vertexoffset] ) )
		{
			// nope, bad normal
			continue;
		}
		// see if the texcoord is reasonable
		if( !CompareTexCoordsFuzzy( texCoord, pSrc->texcoord[i + pMesh->vertexoffset] ) )
		{
			continue;
		}

		if( !ignoreBoneWeight )
		{
			if( !CompareBoneWeightsFuzzy( boneWeight, pSrc->globalBoneweight[i + pMesh->vertexoffset] ) )
			{
				continue;
			}
		}

		Vector delta = position - pSrc->vertex[i + pMesh->vertexoffset];
		float distSquared = DotProduct( delta, delta );
		if( distSquared < closestDistSquared )
		{
			closestDistSquared = distSquared;
			closestIndex = i;
		}
	}
	return closestIndex;
}

// return -1 if there is no match
// the index returned is model relative!
static int FindVertexWithinModel( const s_boneweight_t &boneWeight, const Vector &position, 
								  const Vector &normal, const Vector2D &texCoord,
								  const s_source_t *pSrc, bool ignoreNormalAndTexCoord,
								  bool ignoreBoneWeight )
{
	int i;
	int closestIndex = -1;
	float closestDistSquared = FLT_MAX;
	for( i = 0; i < pSrc->numvertices; i++ )
	{
		// see if the position is reasonable
		if( !ComparePositionFuzzy( position, pSrc->vertex[i] ) )
		{
			continue;
		}
		if( !ignoreNormalAndTexCoord )
		{
			// see if the normal is reasonable
			if( !CompareNormalFuzzy( normal, pSrc->normal[i] ) )
			{
				continue;
			}
			// see if the texcoord is reasonable
			if( !CompareTexCoordsFuzzy( texCoord, pSrc->texcoord[i] ) )
			{
				continue;
			}
		}
		if( !ignoreBoneWeight )
		{
			if( !CompareBoneWeightsFuzzy( boneWeight, pSrc->globalBoneweight[i] ) )
			{
				continue;
			}
		}
		Vector delta = position - pSrc->vertex[i];
		float distSquared = DotProduct( delta, delta );
		if( distSquared < closestDistSquared )
		{
			closestDistSquared = distSquared;
			closestIndex = i;
		}
	}
	return closestIndex;
}

//-----------------------------------------------------------------------------
// Modify the bone weights in all of the vertices....
//-----------------------------------------------------------------------------

static void RemapBoneWeights( const CUtlVector<int> &boneMap, s_boneweight_t &boneWeight, 
							  const s_source_t *pSrc )
{
	for( int i = 0; i < boneWeight.numbones; i++ )
	{
		assert( boneWeight.bone[i] >= 0 && boneWeight.bone[i] < boneMap.Size() );
		boneWeight.bone[i] = boneMap[boneWeight.bone[i]];
	}
}

static void SortBoneWeightByIndex( s_boneweight_t &boneWeight )
{
	int j;
	// bubble sort the bones by index. . .put the smallest weight first.
	for( j = boneWeight.numbones; j > 1; j-- )
	{
		int k;
		for( k = 0; k < j - 1; k++ )
		{
			if( boneWeight.bone[k] > boneWeight.bone[k+1] )
			{
				// swap
				int tmpBone;
				float tmpWeight;
				tmpBone = boneWeight.bone[k];
				tmpWeight = boneWeight.weight[k];
				boneWeight.bone[k] = boneWeight.bone[k+1];
				boneWeight.weight[k] = boneWeight.weight[k+1];
				boneWeight.bone[k+1] = tmpBone;
				boneWeight.weight[k+1] = tmpWeight;
			}
		}
	}
}

static void CollapseBoneWeights( s_boneweight_t &boneWeight )
{
	int i;
	SortBoneWeightByIndex( boneWeight );
	for( i = 0; i < boneWeight.numbones-1; i++ )
	{
		if( boneWeight.bone[i] == boneWeight.bone[i+1] )
		{
			// add i+1's weight to i since they have the same index
			boneWeight.weight[i] += boneWeight.weight[i+1];
			// remove i+1
			int j;
			for( j = i+1; j < boneWeight.numbones-1; j++ )
			{
				boneWeight.bone[j] = boneWeight.bone[j+1];
				boneWeight.weight[j] = boneWeight.weight[j+1];
			}
			boneWeight.numbones--;

			// Gotta step back one, may have many bones collapsing into one
			--i;
		}
	}
}

static void SortBoneWeightByWeight( s_boneweight_t &boneWeight )
{
	int j;
	// bubble sort the bones by weight. . .put the largest weight first.
	for( j = boneWeight.numbones; j > 1; j-- )
	{
		int k;
		for( k = 0; k < j - 1; k++ )
		{
			if( boneWeight.weight[k] < boneWeight.weight[k+1] )
			{
				// swap
				int tmpBone;
				float tmpWeight;
				tmpBone = boneWeight.bone[k];
				tmpWeight = boneWeight.weight[k];
				boneWeight.bone[k] = boneWeight.bone[k+1];
				boneWeight.weight[k] = boneWeight.weight[k+1];
				boneWeight.bone[k+1] = tmpBone;
				boneWeight.weight[k+1] = tmpWeight;
			}
			else if( boneWeight.weight[k] == boneWeight.weight[k+1] )
			{
				// fixme: may want to sort by index if the weights are the same.
			}
		}
	}
}

static void CalculateIdealVert( int srcVertID, const s_mesh_t *pSrcMesh,
							    CUtlVector<Vector> &verts,
								CUtlVector<s_boneweight_t> &boneWeights, 
								CUtlVector<s_vertexinfo_t> &vertexInfo,
								CUtlVector<Vector> &normals,
								CUtlVector<Vector2D> &texcoords, const s_source_t *pSrc,
								s_mesh_t *pDstMesh, const s_source_t *pTopLODSrc,
								Vector &idealPos, Vector2D &idealTexCoords,
								Vector &idealNormal, s_boneweight_t &idealBoneWeight,
								s_vertexinfo_t &idealVertexInfo )
{
	// see if this vert already exists in the temp arrays, if not, then add it.
	int srcID = srcVertID + pSrcMesh->vertexoffset;
	
	s_source_t tmpSrc;
	tmpSrc.localBoneweight = boneWeights.Base();
	tmpSrc.vertexInfo = vertexInfo.Base();
	tmpSrc.vertex = verts.Base();
	tmpSrc.normal = normals.Base();
	tmpSrc.tangentS = NULL;
	tmpSrc.texcoord = texcoords.Base();
	tmpSrc.numvertices = verts.Size();
	// dstMeshVertID is mesh relative
	int dstMeshVertID = FindVertexWithinMesh( pSrc->globalBoneweight[srcID],
		pSrc->vertex[srcID],
		pSrc->normal[srcID],
		pSrc->texcoord[srcID],
		pDstMesh, &tmpSrc, true /* ignoreBoneWeight */ );

	if( dstMeshVertID != -1 )
	{
		assert( dstMeshVertID >= 0 && dstMeshVertID < pDstMesh->numvertices );
		// found in the mesh already.
		idealPos = verts[pDstMesh->vertexoffset + dstMeshVertID];
		idealBoneWeight = boneWeights[pDstMesh->vertexoffset + dstMeshVertID];
		idealVertexInfo = vertexInfo[pDstMesh->vertexoffset + dstMeshVertID];
		idealNormal = normals[pDstMesh->vertexoffset + dstMeshVertID];
		idealTexCoords = texcoords[pDstMesh->vertexoffset + dstMeshVertID];
		ValidateBoneWeight( idealBoneWeight, pTopLODSrc );
		return;
	}

	// didn't find the vert within the temp mesh that we are building, so now
	// we need to find it in the whole model for the top LOD and make a new
	// vert for this mesh (material) that has the same values.
	int srcModelVertID = FindVertexWithinModel( pSrc->globalBoneweight[srcID],
		pSrc->vertex[srcID],
		pSrc->normal[srcID],
		pSrc->texcoord[srcID],
		pTopLODSrc, 
		false /* ignoreNormalAndTexCoord */, 
		true /* ignoreBoneWeight */ );
	if( srcModelVertID != -1 )
	{
		idealPos = pTopLODSrc->vertex[srcModelVertID];
		idealBoneWeight = pTopLODSrc->globalBoneweight[srcModelVertID];
		idealVertexInfo = pTopLODSrc->vertexInfo[srcModelVertID];
		idealNormal = pTopLODSrc->normal[srcModelVertID];
		idealTexCoords = pTopLODSrc->texcoord[srcModelVertID];
		ValidateBoneWeight( idealBoneWeight, pTopLODSrc);
		return;
	}

	// Our normal and texcoord must be too far, so search ignoring it
	// so that we can find decent bone weights
	srcModelVertID = FindVertexWithinModel( pSrc->globalBoneweight[srcID],
		pSrc->vertex[srcID],
		pSrc->normal[srcID],
		pSrc->texcoord[srcID],
		pTopLODSrc, 
		true /* ignoreNormalAndTexCoord */, 
		true /* ignoreBoneWeight */ );
	if( srcModelVertID != -1 )
	{
		idealPos = pTopLODSrc->vertex[srcModelVertID];
		idealBoneWeight = pTopLODSrc->globalBoneweight[srcModelVertID];
		idealVertexInfo = pTopLODSrc->vertexInfo[srcModelVertID];
		idealNormal = pSrc->normal[srcID];
		idealTexCoords = pSrc->texcoord[srcID];
		ValidateBoneWeight( idealBoneWeight, pTopLODSrc );
		return;
	}
		
	printf( "LOD vertex mismatch.  All verts in lower LODs must exist in the top LOD: position %f %f %f file: %s.\n",
		pSrc->vertex[srcID].x, pSrc->vertex[srcID].y, pSrc->vertex[srcID].z, pSrc->filename );
	assert( 0 );
	exit( -1 );
}


static bool FuzzyFloatCompare( float f1, float f2, float epsilon )
{
	if( fabs( f1 - f2 ) < epsilon )
	{
		return true;
	}
	else
	{
		return false;
	}
}
								
static int FindOrCreateVertInTmpMesh( CUtlVector<Vector> &verts,
									   CUtlVector<s_boneweight_t> &boneWeights, 
									   CUtlVector<s_vertexinfo_t> &vertexInfo,
								       CUtlVector<Vector> &normals,
								       CUtlVector<Vector2D> &texcoords,
									   const Vector &idealPos,
									   const Vector2D &idealTexCoords,
									   const Vector &idealNormal,
									   const s_boneweight_t &idealBoneWeight,
									   const s_vertexinfo_t &idealVertexInfo,
									   s_mesh_t *pDstMesh )
{
	int meshVertID;
	for( meshVertID = 0; meshVertID < pDstMesh->numvertices; meshVertID++ )
	{
		int modelVertID = pDstMesh->vertexoffset + meshVertID;
		if( ComparePositionFuzzy( verts[modelVertID], idealPos ) &&
			CompareTexCoordsFuzzy( texcoords[modelVertID], idealTexCoords ) &&
			CompareNormalFuzzy( normals[modelVertID], idealNormal ) &&
			CompareBoneWeightsFuzzy( boneWeights[modelVertID], idealBoneWeight ) )
		{
			return meshVertID;
		}
	}

	// doh!  didn't find it. . add it.
	int i = verts.AddToTail();
	boneWeights.AddToTail();
	vertexInfo.AddToTail();
	normals.AddToTail();
	texcoords.AddToTail();
	verts[i] = idealPos;
	boneWeights[i] = idealBoneWeight;
	vertexInfo[i] = idealVertexInfo;
	normals[i] = idealNormal;
	texcoords[i] = idealTexCoords;
	pDstMesh->numvertices++;
	return meshVertID;
}

static void PrintBonesUsedInLOD( s_source_t *pSrc )
{
	printf( "PrintBonesUsedInLOD\n" );
	int i;
	for( i = 0; i < pSrc->numvertices; i++ )
	{
		Vector &pos = pSrc->vertex[i];
		Vector &norm = pSrc->normal[i];
		Vector2D &texcoord = pSrc->texcoord[i];
		printf( "pos: %f %f %f norm: %f %f %f texcoord: %f %f\n",
			pos[0], pos[1], pos[2], norm[0], norm[1], norm[2], texcoord[0], texcoord[1] );
		s_boneweight_t *pBoneWeight = &pSrc->globalBoneweight[i];
		int j;
		for( j = 0; j < pBoneWeight->numbones; j++ )
		{
			int globalBoneID = pBoneWeight->bone[j];
			const char *pBoneName = g_bonetable[globalBoneID].name;
			printf( "vert: %d bone: %d boneid: %d weight: %f name: \"%s\"\n", i, ( int )j, ( int )pBoneWeight->bone[j], 
				( float )pBoneWeight->weight[j], pBoneName );
		}
		printf( "\n" );
		fflush( stdout );
	}
}

static void	MarkBonesUsedByLod( s_source_t *pSrc, int lodID )
{
	int i;
	for( i = 0; i < pSrc->numvertices; i++ )
	{
		Vector &pos = pSrc->vertex[i];
		Vector &norm = pSrc->normal[i];
		Vector2D &texcoord = pSrc->texcoord[i];
		s_boneweight_t *pBoneWeight = &pSrc->globalBoneweight[i];
		int j;
		for( j = 0; j < pBoneWeight->numbones; j++ )
		{
			int globalBoneID = pBoneWeight->bone[j];
			s_bonetable_t *pBone = &g_bonetable[globalBoneID];
			pBone->flags |= ( BONE_USED_BY_VERTEX_LOD0 << lodID );
		}
	}
}

void ValidateBoneWeights( const s_source_t *pSrc )
{
	int i;
	for( i = 0; i < pSrc->numvertices; i++ )
	{
		Vector &pos = pSrc->vertex[i];
		Vector &norm = pSrc->normal[i];
		Vector2D &texcoord = pSrc->texcoord[i];
		s_boneweight_t *pBoneWeight = &pSrc->globalBoneweight[i];
		int j;
		for( j = 0; j < pBoneWeight->numbones; j++ )
		{
			int globalBoneID;
			globalBoneID = pBoneWeight->bone[j];
			const char *pBoneName = g_bonetable[globalBoneID].name;
			ValidateBoneWeight( *pBoneWeight, pSrc );
		}
	}
}

static void PrintSBoneWeight( s_boneweight_t *pBoneWeight, const s_source_t *pSrc )
{
	int j;
	for( j = 0; j < pBoneWeight->numbones; j++ )
	{
		int globalBoneID;
		globalBoneID = pBoneWeight->bone[j];
		const char *pBoneName = g_bonetable[globalBoneID].name;
		printf( "bone: %d boneid: %d weight: %f name: \"%s\"\n", ( int )j, ( int )pBoneWeight->bone[j], 
			( float )pBoneWeight->weight[j], pBoneName );
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
// Inputs:
// Outputs:
//-----------------------------------------------------------------------------



static void CreateLODVertsInTopLOD( const CUtlVector<int> &boneMap, const s_source_t *pTopLODSrc, 
								   s_source_t *pSrc, 
								  const s_mesh_t *pSrcMesh, CUtlVector<Vector> &verts,
								  CUtlVector<s_boneweight_t> &boneWeights, 
								  CUtlVector<s_vertexinfo_t> &vertexInfo, 
								  CUtlVector<Vector> &normals,
								  CUtlVector<Vector2D> &texcoords, s_mesh_t *pDstMesh, int lodID )
{
	ValidateBoneWeights( pTopLODSrc );
	int srcVertID;
	for( srcVertID = 0; srcVertID < pSrcMesh->numvertices; srcVertID++ )
	{
		Vector idealPos;
		Vector2D idealTexCoords;
		Vector idealNormal;
		s_boneweight_t idealBoneWeight;
		s_vertexinfo_t idealVertexInfo;
	
		CalculateIdealVert( srcVertID, pSrcMesh, verts, boneWeights, vertexInfo, normals, texcoords, 
			pSrc, pDstMesh, pTopLODSrc, idealPos, idealTexCoords, idealNormal, idealBoneWeight, idealVertexInfo );

		// Now we should have an ideal vertex to work with including boneweights.

//		s_boneweight_t &boneWeight = pSrc->boneweight[srcVertID];
		// Now remap the bones if necessary, merge those that are redundantly represented after
		// the merge.
		ValidateBoneWeight( idealBoneWeight, pTopLODSrc );
		RemapBoneWeights( boneMap, idealBoneWeight, pTopLODSrc );

		ValidateBoneWeight( idealBoneWeight, pTopLODSrc );
		CollapseBoneWeights( idealBoneWeight );

		ValidateBoneWeight( idealBoneWeight, pTopLODSrc );
		SortBoneWeightByWeight( idealBoneWeight );

		ValidateBoneWeight( idealBoneWeight, pTopLODSrc );

		//		PrintSBoneWeight( &idealBoneWeight, pSrc );

		// Go ahead and stick the bones weights into the lod src since the boneweights there are invalid
		// since the lods aren't physiqued.
		pSrc->globalBoneweight[srcVertID + pSrcMesh->vertexoffset] = idealBoneWeight;

		// search to see if we already have the vert that we need in the mesh.  
		// If not, then create a new vert.
		int meshVertID = FindOrCreateVertInTmpMesh( verts, boneWeights, vertexInfo, normals, texcoords, idealPos, idealTexCoords,
			idealNormal, idealBoneWeight, idealVertexInfo, pDstMesh );
		pSrc->topLODMeshVertIndex[srcVertID + pSrcMesh->vertexoffset] = meshVertID;
	}
}

static void PrintSourceVerts( s_source_t *pSrc )
{
	int i;
	for( i = 0; i < pSrc->numvertices; i++ )
	{
		printf( "v %d ", i );
		printf( "pos: %f %f %f ", pSrc->vertex[i][0], pSrc->vertex[i][1], pSrc->vertex[i][2] );
		printf( "norm: %f %f %f ", pSrc->normal[i][0], pSrc->normal[i][1], pSrc->normal[i][2] );
		printf( "texcoord: %f %f\n", pSrc->texcoord[i][0], pSrc->texcoord[i][1] );
		int j;
		for( j = 0; j < pSrc->globalBoneweight[i].numbones; j++ )
		{
			printf( "\t%d: %d %f\n", j, ( int )pSrc->globalBoneweight[i].bone[j], 
				pSrc->globalBoneweight[i].weight[j] );
		}
		fflush( stdout );
	}
}

static void StompSourceWithTempVertexArrays( s_source_t *pSrc, CUtlVector<Vector> &verts,
								  CUtlVector<s_boneweight_t> &boneWeights, 
								  CUtlVector<s_vertexinfo_t> &vertexInfo, 
								  CUtlVector<Vector> &normals,
								  CUtlVector<Vector2D> &texcoords, CUtlVector<s_face_t> &faces,
								  CUtlVector<s_mesh_t> &meshes )
{
	free( pSrc->globalBoneweight );
	free( pSrc->vertexInfo );
	delete [] pSrc->vertex;
	delete [] pSrc->normal;
	free( pSrc->texcoord );
	free( pSrc->face );
	
	assert( boneWeights.Size() == verts.Size() );
	assert( normals.Size() == verts.Size() );
	assert( texcoords.Size() == verts.Size() );
	
	pSrc->globalBoneweight = (s_boneweight_t *)kalloc( boneWeights.Size(), sizeof( s_boneweight_t ) );
	pSrc->vertexInfo = (s_vertexinfo_t *)kalloc( vertexInfo.Size(), sizeof( s_vertexinfo_t ) );
	pSrc->vertex = new Vector[verts.Size()];
	pSrc->normal = new Vector[normals.Size()];
	pSrc->tangentS = new Vector4D[normals.Size()];
	pSrc->texcoord = (Vector2D *)kalloc( texcoords.Size(), sizeof( Vector2D ) );
	pSrc->numvertices = verts.Size();
	pSrc->face = (s_face_t *)kalloc( faces.Size(), sizeof( s_face_t ));
	pSrc->numfaces = faces.Size();
	
	memcpy( pSrc->globalBoneweight, boneWeights.Base(), pSrc->numvertices * sizeof( s_boneweight_t ) );
	memcpy( pSrc->vertexInfo, vertexInfo.Base(), pSrc->numvertices * sizeof( s_vertexinfo_t ) );
	memcpy( pSrc->vertex, verts.Base(), pSrc->numvertices * sizeof( Vector ) );
	memcpy( pSrc->normal, normals.Base(), pSrc->numvertices * sizeof( Vector ) );
	memcpy( pSrc->texcoord, texcoords.Base(), pSrc->numvertices * sizeof( Vector2D ) );
	memcpy( pSrc->face, faces.Base(), pSrc->numfaces * sizeof( s_face_t ) );
	
	memcpy( pSrc->mesh, meshes.Base(), meshes.Size() * sizeof( s_mesh_t ) );
}

// This fills out boneMap, which is a mapping from src bone to src bone replacement (or to itself
// if there is no bone replacement.
static void BuildBoneLODMapping( CUtlVector<int> &boneMap,
								 int lodID )
{
	boneMap.AddMultipleToTail( g_numbones );

	assert( lodID < g_ScriptLODs.Size() );
	LodScriptData_t& scriptLOD = g_ScriptLODs[lodID];
	int i;
	for( i = 0; i < g_numbones; i++ )
	{
		boneMap[i] = i;
	}

	for( i = 0; i < scriptLOD.boneReplacements.Size(); i++ )
	{
		const char *src, *dst;
		src = scriptLOD.boneReplacements[i].GetSrcName();
		dst = scriptLOD.boneReplacements[i].GetDstName();
		int j = findGlobalBone( src );
		int k = findGlobalBone( dst );

		if ( j != -1 && k != -1)
		{
			boneMap[j] = k;
		}
		else
		{
			printf( "ERROR: couldn't replace bone \"%s\" with \"%s\"\n", src, dst );
		}
	}
}

static void UnifyModelLODs( s_model_t *pSrcModel )
{
	CUtlVector<s_source_t *> lods;
	int numLODs = g_ScriptLODs.Size();
	lods.AddMultipleToTail( numLODs );
	
	if( stricmp( pSrcModel->name, "blank" ) == 0 )
	{
		return;
	}
	
	GetLODSources( lods, pSrcModel );
	
	// These hold temp info for the model so that we can stomp over the current verts
	// for the model when we are done.
	CUtlVector<s_boneweight_t> boneWeights;
	CUtlVector<s_vertexinfo_t> vertexInfo;
	CUtlVector<Vector> verts;
	CUtlVector<Vector> normals;
	CUtlVector<Vector2D> texcoords;
	CUtlVector<s_face_t> faces;
	CUtlVector<s_mesh_t> meshes;
	
	int lodID;
	for( lodID = 0; lodID < numLODs; lodID++ )
	{
		if( lods[lodID] )
		{
			lods[lodID]->topLODMeshVertIndex = new int[lods[lodID]->numvertices];
		}
	}

	meshes.AddMultipleToTail( MAXSTUDIOSKINS );
	assert( meshes.Size() == MAXSTUDIOSKINS );
	memset( meshes.Base(), 0, sizeof( s_mesh_t ) * meshes.Size() );
	int meshID;
	for( meshID = 0; meshID < pSrcModel->source->nummeshes; meshID++ )
	{
		s_mesh_t *pDstMesh = &meshes[pSrcModel->source->meshindex[meshID]];
		
		pDstMesh->numvertices = 0;
		pDstMesh->vertexoffset = verts.Size();
		pDstMesh->numfaces = 0;
		pDstMesh->faceoffset = faces.Size();
		
		for( lodID = 0; lodID < numLODs; lodID++ )
		{
			s_source_t *pSrc = lods[lodID];
			if( !pSrc )
			{
				continue;
			}
//			ValidateBoneWeights( pSrc );
			// figure out the meshID for this lod.
			s_mesh_t *pSrcMesh = FindMeshByMaterial( pSrc, lods[0]->meshindex[meshID] );
			if( !pSrcMesh )
			{
				continue;
			}

			CUtlVector<int> boneMap;
			BuildBoneLODMapping( boneMap, lodID );
			ValidateBoneWeights( lods[0] );
			
			if( lodID == 0 )
			{
				CopyVertsToTempArrays( pSrc, pSrcMesh, verts, boneWeights, vertexInfo, normals, texcoords, pDstMesh );
				// only fix up the faces for the highest lod since the lowest ones are going
				// to be reprocessed later.
				CopyFacesToTempArray( pSrc, pSrcMesh, faces, pDstMesh );
			}
			else
			{
				CreateLODVertsInTopLOD( boneMap, lods[0], pSrc, pSrcMesh, verts, boneWeights, vertexInfo, normals, texcoords, pDstMesh, lodID );

				MarkBonesUsedByLod( pSrc, lodID );
			}
		}
	}

	// stick everything that we just built into the highest lod.
	// We don't really save anything about the lower lods at this point. . we'll refind their verts later.
	// The point of all this is to make sure that the highest LOD has all of the verts that are needed
	// for all LODs.
	StompSourceWithTempVertexArrays( lods[0], verts, boneWeights, vertexInfo, normals, texcoords, faces, meshes );
//	PrintSourceVerts( lods[0] );
}

// Force the vertex array for a model to have all of the vertices that are needed
// for all of the LODs of the model.
void UnifyLODs( void )
{
	int modelID;

	// todo: need to fixup the firstref/lastref stuff . . do we really need it anymore?
	for( modelID = 0; modelID < g_nummodelsbeforeLOD; modelID++ )
	{
		UnifyModelLODs( g_model[modelID] );
	}
}

static int g_NumBonesInLOD[MAX_NUM_LODS];

static void PrintSpaces( int numSpaces )
{
	int i;
	for( i = 0; i < numSpaces; i++ )
	{
		printf( " " );
	}
}

static void SpewBoneInfo( int globalBoneID, int depth )
{
	s_bonetable_t *pBone = &g_bonetable[globalBoneID];
	if( g_bPrintBones )
	{
		PrintSpaces( depth * 2 );
		printf( "%d \"%s\" ", depth, pBone->name );
	}
	int i;
	for( i = 0; i < 8; i++ )
	{
		if( pBone->flags & ( BONE_USED_BY_VERTEX_LOD0 << i ) )
		{
			if( g_bPrintBones )
			{
				printf( "lod%d ", i );
			}
			g_NumBonesInLOD[i]++;
		}
	}
	if( g_bPrintBones )
	{
		printf( "\n" );	
	}
	
	int j;
	for( j = 0; j < g_numbones; j++ )
	{
		s_bonetable_t *pBone = &g_bonetable[j];
		if( pBone->parent == globalBoneID )
		{
			SpewBoneInfo( j, depth + 1 );
		}
	}
}

void SpewBoneUsageStats( void )
{
	memset( g_NumBonesInLOD, 0, sizeof( int ) * MAX_NUM_LODS );
	if( g_numbones == 0 )
	{
		return;
	}
	SpewBoneInfo( 0, 0 );
	if( g_bPrintBones )
	{
		int i;
		for( i = 0; i < g_ScriptLODs.Count(); i++ )
		{
			printf( "\t%d bones used in lod %d\n", g_NumBonesInLOD[i], i );
		}
	}
}

void MarkParentBoneLODs( void )
{
	int i;
	for( i = 0; i < g_numbones; i++ )
	{
		int flags = g_bonetable[i].flags;
		flags &= BONE_USED_BY_VERTEX_MASK;
		int globalBoneID = g_bonetable[i].parent;
		while( globalBoneID != -1 )
		{
			g_bonetable[globalBoneID].flags |= flags;
			globalBoneID = g_bonetable[globalBoneID].parent;
		}
	}
}

static void LoadModelLODSource( s_model_t *pSrcModel )
{
	CUtlVector<s_source_t *> lods;
	int numLODs = g_ScriptLODs.Size();
	lods.AddMultipleToTail( numLODs );
	
	if( stricmp( pSrcModel->name, "blank" ) == 0 )
	{
		return;
	}
	
	GetLODSources( lods, pSrcModel );
}

void LoadLODSources( void )
{
	g_nummodelsbeforeLOD = g_nummodels;
	for( int modelID = 0; modelID < g_nummodelsbeforeLOD; modelID++ )
	{
		LoadModelLODSource( g_model[modelID] );
	}
}

static void ReplaceBonesRecursive( int globalBoneID, bool replaceThis, 
								   CUtlVector<CLodScriptReplacement_t> &boneReplacements, 
								   const char *replacementName )
{
	if( replaceThis )
	{
		CLodScriptReplacement_t &boneReplacement = boneReplacements[boneReplacements.AddToTail()];
		boneReplacement.SetSrcName( g_bonetable[globalBoneID].name );
		boneReplacement.SetDstName( replacementName );
	}

	// find children and recurse.
	int i;
	for( i = 0; i < g_numbones; i++ )
	{
		if( g_bonetable[i].parent == globalBoneID )
		{
			ReplaceBonesRecursive( i, true, boneReplacements, replacementName );
		}
	}
}

static void ConvertSingleBoneTreeCollapseToReplaceBones( CLodScriptReplacement_t &boneTreeCollapse, 
														 CUtlVector<CLodScriptReplacement_t> &boneReplacements )
{
	// find the bone that we are starting with.
	int i = findGlobalBone( boneTreeCollapse.GetSrcName() );
	if (i != -1)
	{
		ReplaceBonesRecursive( i, false, boneReplacements, g_bonetable[i].name );
		return;
	}
	printf( "WARNING: couldn't find bone %s for bonetreecollapse, skipping\n", boneTreeCollapse.GetSrcName() );
}

void ConvertBoneTreeCollapsesToReplaceBones( void )
{
	int i;
	for( i = 0; i < g_ScriptLODs.Size(); i++ )
	{
		LodScriptData_t& lod = g_ScriptLODs[i];
		int j;
		for( j = 0; j < lod.boneTreeCollapses.Size(); j++ )
		{
			ConvertSingleBoneTreeCollapseToReplaceBones( lod.boneTreeCollapses[j], 
				lod.boneReplacements );
		}
	}
}

