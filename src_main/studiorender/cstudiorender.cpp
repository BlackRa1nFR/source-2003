//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdlib.h>
#include "cstudiorender.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "optimize.h"
#include "vmatrix.h"
#include "tier0/vprof.h"
#include "studiostats.h"
#include "vstdlib/strtools.h"
#include "tier0/memalloc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------

CStudioRender g_StudioRender;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CStudioRender, IStudioRender, 
						STUDIO_RENDER_INTERFACE_VERSION, g_StudioRender );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CStudioRender::CStudioRender()
{
	m_pMaterialSystem = NULL;
	m_pMaterialSystemHardwareConfig = NULL;
	m_pStudioHdr = NULL;
	m_pStudioMeshes = NULL;
	m_pSubModel = NULL;
	m_pGlintTexture = NULL;
	m_GlintWidth = 0;
	m_GlintHeight = 0;
	m_pGlintLinearScratch = NULL;
	int i;
	for( i = 0; i < MAXSTUDIOFLEXDESC; i++ )
	{
		m_FlexWeights[i] = 0.0f;
	}
	Con_Printf = NULL;
	Con_DPrintf = NULL;

	m_pForcedMaterial = NULL;
	m_nForcedMaterialType = OVERRIDE_NORMAL;
	m_ColorMod[0] = m_ColorMod[1] = m_ColorMod[2] = 1.0f;
	m_AlphaMod = 1.0f;

	// Cache-align our important matrices
	g_pMemAlloc->PushAllocDbgInfo( __FILE__, __LINE__ );
#ifdef _WIN32
	m_BoneToWorld = (matrix3x4_t*)_mm_malloc( MAXSTUDIOBONES * sizeof(matrix3x4_t), 32 );
	m_PoseToWorld = (matrix3x4_t*)_mm_malloc( MAXSTUDIOBONES * sizeof(matrix3x4_t), 32 );
	m_PoseToDecal = (matrix3x4_t*)_mm_malloc( MAXSTUDIOBONES * sizeof(matrix3x4_t), 32 );
#elif _LINUX
	posix_memalign((void **)(&m_BoneToWorld), 32, MAXSTUDIOBONES * sizeof(matrix3x4_t));
	posix_memalign((void **)(&m_PoseToWorld), 32, MAXSTUDIOBONES * sizeof(matrix3x4_t));
	posix_memalign((void **)(&m_PoseToDecal), 32, MAXSTUDIOBONES * sizeof(matrix3x4_t));
#endif
	g_pMemAlloc->PopAllocDbgInfo();
}

CStudioRender::~CStudioRender()
{
#ifdef _WIN32
	_mm_free(m_BoneToWorld);
	_mm_free(m_PoseToWorld);
	_mm_free(m_PoseToDecal);
#elif _LINUX
	free(m_BoneToWorld);
	free(m_PoseToWorld);
	free(m_PoseToDecal);
#endif
}

void CStudioRender::InitDebugMaterials( void )
{
	m_pMaterialMRMWireframe = 
		m_pMaterialSystem->FindMaterial( "debug/debugmrmwireframe", NULL, true );
	m_pMaterialMRMWireframe->IncrementReferenceCount();

	m_pMaterialMRMNormals = 
		m_pMaterialSystem->FindMaterial( "debug/debugmrmnormals", NULL, true );
	m_pMaterialMRMWireframe->IncrementReferenceCount();

	m_pMaterialTranslucentModelHulls = 
		m_pMaterialSystem->FindMaterial( "debug/debugtranslucentmodelhulls", NULL, true );
	m_pMaterialTranslucentModelHulls->IncrementReferenceCount();

	m_pMaterialTranslucentModelHulls_ColorVar = m_pMaterialTranslucentModelHulls->FindVar( "$color", NULL );

	m_pMaterialSolidModelHulls = 
		m_pMaterialSystem->FindMaterial( "debug/debugsolidmodelhulls", NULL, true );
	m_pMaterialSolidModelHulls->IncrementReferenceCount();

	m_pMaterialSolidModelHulls_ColorVar = m_pMaterialSolidModelHulls->FindVar( "$color", NULL );

	m_pMaterialAdditiveVertexColorVertexAlpha = 
		m_pMaterialSystem->FindMaterial( "engine/additivevertexcolorvertexalpha", NULL, true );
	m_pMaterialAdditiveVertexColorVertexAlpha->IncrementReferenceCount();

	m_pMaterialModelBones = 
		m_pMaterialSystem->FindMaterial( "debug/debugmodelbones", NULL, true );
	m_pMaterialModelBones->IncrementReferenceCount();

	m_pMaterialModelEnvCubemap =
		m_pMaterialSystem->FindMaterial( "debug/env_cubemap_model", NULL, true );
	
	m_pMaterialWorldWireframe = 
		m_pMaterialSystem->FindMaterial( "debug/debugworldwireframe", NULL, true );
	m_pMaterialWorldWireframe->IncrementReferenceCount();
}

void CStudioRender::ShutdownDebugMaterials( void )
{
	m_pMaterialMRMWireframe->DecrementReferenceCount();
	m_pMaterialMRMWireframe = NULL;

	m_pMaterialTranslucentModelHulls->DecrementReferenceCount();
	m_pMaterialTranslucentModelHulls = NULL;
	
	m_pMaterialSolidModelHulls->DecrementReferenceCount();
	m_pMaterialSolidModelHulls = NULL;
	
	m_pMaterialAdditiveVertexColorVertexAlpha->DecrementReferenceCount();
	m_pMaterialAdditiveVertexColorVertexAlpha = NULL;
	
	m_pMaterialModelBones->DecrementReferenceCount();
	m_pMaterialModelBones = NULL;
	
	m_pMaterialWorldWireframe->DecrementReferenceCount();
	m_pMaterialWorldWireframe = NULL;
}

static void ReleaseMaterialSystemObjects()
{
	g_StudioRender.UncacheGlint();
}

static void RestoreMaterialSystemObjects()
{
	g_StudioRender.PrecacheGlint();
}

bool CStudioRender::Init( CreateInterfaceFn materialSystemFactory, CreateInterfaceFn materialSystemHWConfigFactory )
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	m_CachedGamma = 2.2f;
	m_CachedTexGamma = 2.2f;
	m_CachedBrightness = 0.0f;
	m_CachedOverbright = 2;
	m_pMaterialSystem = ( IMaterialSystem * )materialSystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
	m_pMaterialSystemHardwareConfig = ( IMaterialSystemHardwareConfig * )materialSystemHWConfigFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, NULL );
	if( m_pMaterialSystem && m_pMaterialSystemHardwareConfig )
	{
		m_pMaterialSystem->AddReleaseFunc( ReleaseMaterialSystemObjects );
		m_pMaterialSystem->AddRestoreFunc( RestoreMaterialSystemObjects );
		InitDebugMaterials();
		return true;
	}
	else
	{
		return false;
	}
}

void CStudioRender::Shutdown( void )
{
	UncacheGlint();
	if( m_pMaterialSystem )
	{
		m_pMaterialSystem->RemoveReleaseFunc( ReleaseMaterialSystemObjects );
		m_pMaterialSystem->RemoveRestoreFunc( RestoreMaterialSystemObjects );
	}
	ShutdownDebugMaterials();
}

void CStudioRender::BeginFrame( void )
{
	// Cache a few values here so I don't have to in software inner loops:
	Assert( m_pMaterialSystemHardwareConfig );
	m_bSupportsHardwareLighting = m_pMaterialSystemHardwareConfig->SupportsHardwareLighting();
	m_bSupportsVertexAndPixelShaders = m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders();
	m_bSupportsOverbright = m_pMaterialSystemHardwareConfig->SupportsOverbright();


}

void CStudioRender::EndFrame( void )
{
}

//-----------------------------------------------------------------------------
// Assign unique #s for each mesh
//-----------------------------------------------------------------------------

static int R_StudioAssignMeshIDs( studiohdr_t *pStudioHdr )
{
	int id = 0;
	int i, j, k;

	assert( pStudioHdr );

	// Iterate over every body part...
	for ( i = 0; i < pStudioHdr->numbodyparts; i++ )
	{
		mstudiobodyparts_t* pBodyPart = pStudioHdr->pBodypart(i);

		// Iterate over every submodel...
		for (j = 0; j < pBodyPart->nummodels; ++j)
		{
			mstudiomodel_t* pModel = pBodyPart->pModel(j);
			
			// Iterate over all the meshes....
			for (k = 0; k < pModel->nummeshes; ++k)
			{
				mstudiomesh_t* pMesh = pModel->pMesh(k);
				pMesh->meshid = id++;
			}
		}
	}

	return id;
}

//-----------------------------------------------------------------------------
// Refresh the studiohdr since it was lost...
//-----------------------------------------------------------------------------

void CStudioRender::RefreshStudioHdr( studiohdr_t* pStudioHdr, studiohwdata_t* pHardwareData )
{
	// FIXME: This whole function is bogus. We really don't want to be
	// fixing up data every time the studio header is cached in and out.
	R_StudioAssignMeshIDs( pStudioHdr );

	// Fixup the envcubemap flag. 
	// FIXME: Can we put this flag in uncacheable memory? (mod->studio.flags?)
	pStudioHdr->flags &= ~STUDIOHDR_FLAGS_USES_ENV_CUBEMAP;
	pStudioHdr->flags &= ~STUDIOHDR_FLAGS_USES_FB_TEXTURE;
	pStudioHdr->flags &= ~STUDIOHDR_FLAGS_USES_BUMPMAPPING;
	for( int i = 0; i < pHardwareData->m_NumLODs; ++i )
	{
		studioloddata_t& lod = pHardwareData->m_pLODs[i];

		for( int j = 0; j < lod.numMaterials; ++j )
		{
			if( lod.ppMaterials[j]->UsesEnvCubemap() )
			{
				pStudioHdr->flags |= STUDIOHDR_FLAGS_USES_ENV_CUBEMAP;
			}
			if( lod.ppMaterials[j]->NeedsFrameBufferTexture() )
			{
				pStudioHdr->flags |= STUDIOHDR_FLAGS_USES_FB_TEXTURE;
			}
			bool bFound;
			// FIXME: I'd rather know that the material is definitely using the bumpmap.
			// It could be in the file without actually being used.
			IMaterialVar *pBumpMatVar = lod.ppMaterials[j]->FindVar( "$bumpmap", &bFound, false );
			if( bFound && pBumpMatVar->IsDefined() && lod.ppMaterials[j]->NeedsTangentSpace() )
			{
				pStudioHdr->flags |= STUDIOHDR_FLAGS_USES_BUMPMAPPING;
			}
		}
	}
}


void CStudioRender::Mat_Stub( IMaterialSystem *pMatSys )
{
	m_pMaterialSystem = pMatSys;
}


bool CStudioRender::LoadModel( 
	studiohdr_t *pStudioHdr, 	// read from the mdl file.
	void *pVtxHdr, 				// read from the vtx file (format /*OptimizedModel::FileHeader_t)
	studiohwdata_t	*pHardwareData 
	)
{
#ifdef _DEBUG
	int bodyPartID;
	for( bodyPartID = 0; bodyPartID < pStudioHdr->numbodyparts; bodyPartID++ )
	{
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyPartID );
		int modelID;
		for( modelID = 0; modelID < pBodyPart->nummodels; modelID++ )
		{
			mstudiomodel_t *pModel = pBodyPart->pModel( modelID );
			int vertID;
			for( vertID = 0; vertID < pModel->numvertices; vertID++ )
			{
//				Vector4D *pTangentS = pModel->TangentS( vertID );
//				Assert( pTangentS->w == -1.0f || pTangentS->w == 1.0f );
			}
		}
	}
#endif

	pHardwareData->m_NumStudioMeshes = R_StudioAssignMeshIDs( pStudioHdr );
	return Mod_LoadStudioModelVertexData( pStudioHdr, pVtxHdr, pHardwareData->m_NumStudioMeshes, 
		&pHardwareData->m_NumLODs, &pHardwareData->m_pLODs );
}

void CStudioRender::UnloadModel( studiohwdata_t *pHardwareData )
{
	int i;

	for( i = 0; i < pHardwareData->m_NumLODs; i++ )
	{
		int j;
		for( j = 0; j < pHardwareData->m_pLODs[i].numMaterials; j++ )
		{
			pHardwareData->m_pLODs[i].ppMaterials[j]->DecrementReferenceCount();
		}
		delete [] pHardwareData->m_pLODs[i].ppMaterials;
		delete [] pHardwareData->m_pLODs[i].pMaterialFlags;
		pHardwareData->m_pLODs[i].ppMaterials = NULL;
		pHardwareData->m_pLODs[i].pMaterialFlags = NULL;
	}
	for( i = 0; i < pHardwareData->m_NumLODs; i++ )
	{
		R_StudioDestroyStaticMeshes( pHardwareData->m_NumStudioMeshes, &pHardwareData->m_pLODs[i].m_pMeshData );
	}
}

void CStudioRender::UpdateConfig( const StudioRenderConfig_t& config )
{
	memcpy( &m_Config, &config, sizeof( m_Config ) );
	if( config.bNormals )
	{
		m_Config.bSoftwareLighting = true;
	}
	Con_Printf = config.pConPrintf;
	Con_DPrintf = config.pConDPrintf;
	assert( Con_Printf );
	assert( Con_DPrintf );

	if( m_CachedGamma != m_Config.gamma ||
		m_CachedTexGamma != m_Config.texGamma ||
		m_CachedBrightness != m_Config.brightness ||
		m_CachedOverbright != m_Config.overbrightFactor )
	{
		BuildGammaTable( m_Config.gamma, m_Config.texGamma, m_Config.brightness, m_Config.overbrightFactor );
		m_CachedGamma = m_Config.gamma;
		m_CachedTexGamma = m_Config.texGamma;
		m_CachedBrightness = m_Config.brightness;
		m_CachedOverbright = m_Config.overbrightFactor;
	}

}

int CStudioRender::GetNumAmbientLightSamples()
{
	return 6;
}

const Vector *CStudioRender::GetAmbientLightDirections( void )
{
	static Vector boxDir[6] = 
	{
		Vector(  1,  0,  0 ), 
		Vector( -1,  0,  0 ),
		Vector(  0,  1,  0 ), 
		Vector(  0, -1,  0 ), 
		Vector(  0,  0,  1 ), 
		Vector(  0,  0, -1 ), 
	};
	return boxDir;
}

void CStudioRender::SetAmbientLightColors( const Vector *pColors, const Vector *pTotalColors )
{
	int i;
	for( i = 0; i < 6; i++ )
	{
		VectorCopy( pColors[i], m_LightBoxColors[i].AsVector3D() );
		m_LightBoxColors[i][3] = 1.0f;
	}

	// FIXME: What happens when we use the fixed function pipeline but vertex shaders 
	// are active? For the time being this only works because everything that does
	// vertex lighting does, in fact, have a vertex shader which is used to render it.
	if( m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() ||
		!m_pMaterialSystemHardwareConfig->SupportsHardwareLighting() )
	{
		m_pMaterialSystem->SetAmbientLightCube( m_LightBoxColors );
	}
	else 
	{
		// Fixed-function T&L case
		Vector4D hwLightBoxColors[6];

		// Fixup the ambient light cube so the hardware can render reasonably
		// and so any software lighting behaves similarly
		R_ComputeFixedUpAmbientCube( m_LightBoxColors, pTotalColors );
		for( i = 0; i < 6; i++ )
		{
			Vector4DMultiply( m_LightBoxColors[i], 1.0f / m_Config.overbrightFactor, hwLightBoxColors[i] );
			hwLightBoxColors[i][3] = 1.0f;
		}
		m_pMaterialSystem->SetAmbientLightCube( hwLightBoxColors );
	}
}

void CStudioRender::SetLocalLights( int numLights, const LightDesc_t *pLights )
{
	int i;
	assert( numLights <= MAXLOCALLIGHTS );
	if( numLights > MAXLOCALLIGHTS )
	{
		numLights = MAXLOCALLIGHTS;
	}
	m_NumLocalLights = numLights;
	for( i = 0; i < m_NumLocalLights; i++ )
	{
		LightDesc_t *pLight = &m_LocalLights[i];
		memcpy( pLight, &pLights[i], sizeof( LightDesc_t ) );
		pLight->m_Flags = 0;
		if( pLight->m_Attenuation0 != 0.0f )
		{
			pLight->m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0;
		}
		if( pLight->m_Attenuation1 != 0.0f )
		{
			pLight->m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1;
		}
		if( pLight->m_Attenuation2 != 0.0f )
		{
			pLight->m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2;
		}
	}

	if( m_pMaterialSystemHardwareConfig->SupportsHardwareLighting() && !m_Config.bSoftwareLighting )
	{
		for( i = 0; i < m_NumLocalLights; i++ )
		{
			m_pMaterialSystem->SetLight( i, m_LocalLights[i] );
		}
		for( ; i < m_pMaterialSystemHardwareConfig->MaxNumLights(); i++ )
		{
			LightDesc_t desc;
			desc.m_Type = MATERIAL_LIGHT_DISABLE;
			m_pMaterialSystem->SetLight( i, desc );
		}
	}
	else
	{
		for( i = 0; i < m_pMaterialSystemHardwareConfig->MaxNumLights(); i++ )
		{
			LightDesc_t desc;
			desc.m_Type = MATERIAL_LIGHT_DISABLE;
			m_pMaterialSystem->SetLight( i, desc );
		}
	}
}

void CStudioRender::SetViewState( 	
	const Vector& viewOrigin,
	const Vector& viewRight,
	const Vector& viewUp,
	const Vector& viewPlaneNormal )
{
	VectorCopy( viewOrigin, m_ViewOrigin );
	VectorCopy( viewRight, m_ViewRight );
	VectorCopy( viewUp, m_ViewUp );
	VectorCopy( viewPlaneNormal, m_ViewPlaneNormal );
}

matrix3x4_t* CStudioRender::GetPoseToWorld(int i)
{
	Assert( sizeof( matrix3x4_t ) == sizeof( matrix3x4_t ) );
	return (matrix3x4_t*)(&m_PoseToWorld[i]);
}

matrix3x4_t* CStudioRender::GetBoneToWorld(int i)
{
	Assert( sizeof( matrix3x4_t ) == sizeof( matrix3x4_t ) );
	return (matrix3x4_t*)(&m_BoneToWorld[i]);
}

matrix3x4_t* CStudioRender::GetBoneToWorldArray()
{
	Assert( sizeof( matrix3x4_t ) == sizeof( matrix3x4_t ) );
	return (matrix3x4_t*)m_BoneToWorld;
}

//-----------------------------------------------------------------------------
// Gets the model LOD; pass in the screen size of a sphere of radius 1
// that has the same origin as the model to get the LOD out...
//-----------------------------------------------------------------------------
int CStudioRender::ComputeModelLod( studiohwdata_t *pHardwareData, float unitSphereSize )
{
	// Convert the unit sphere size into the LOD metric
	if( unitSphereSize != 0.0f )
	{
		unitSphereSize = 100.0f / unitSphereSize;
	}
	
	int i;
	int numLODs = GetNumLODs( *pHardwareData );
	if ( !numLODs )
		return 0;

	bool hasShadowLOD = GetLODSwitchValue( *pHardwareData, numLODs-1 ) < 0.0f ? true : false;
	for( i = 0; i < numLODs-1; i++ )
	{
		if ( hasShadowLOD && ( i == numLODs - 1 ) )
			continue;

		if( GetLODSwitchValue( *pHardwareData, i+1 ) > unitSphereSize )
		{
			return i;
		}
	}

	// Don't ever return shadow lod as lowest lod
	if ( hasShadowLOD )	
	{
		return numLODs - 2;
	}

	return numLODs - 1;
}

	
// LOD stuff
int CStudioRender::GetNumLODs( const studiohwdata_t &hardwareData ) const
{
	return hardwareData.m_NumLODs;
}

float CStudioRender::GetLODSwitchValue( const studiohwdata_t &hardwareData, int lod ) const
{
	return hardwareData.m_pLODs[lod].m_SwitchPoint;
}

void CStudioRender::SetLODSwitchValue( studiohwdata_t &hardwareData, int lod, float switchValue )
{
	hardwareData.m_pLODs[lod].m_SwitchPoint = switchValue;
}

int CStudioRender::CalculateLOD( const Vector& modelOrigin, studiohwdata_t &hardwareData, float *pMetric )
{
	VMatrix projMat;
	Vector delta;
	float len;
	VectorSubtract( modelOrigin, m_ViewOrigin, delta );
	len = ( float )sqrt( DotProduct( delta, delta ) );
	Vector4D testPoint1( 0.0f, -0.5f, len, 1.0f );
	Vector4D testPoint2( 0.0f, 0.5f, len, 1.0f );
	
	m_pMaterialSystem->GetMatrix( MATERIAL_PROJECTION, &projMat[0][0] );
	int viewportX, viewportY, viewportWidth, viewportHeight;
	m_pMaterialSystem->GetViewport( viewportX, viewportY, viewportWidth, viewportHeight );

	Vector4D clipPos1, clipPos2;
	
	Vector4DMultiply( projMat, testPoint1, clipPos1 );
	Vector4DMultiply( projMat, testPoint2, clipPos2 );
	float metric = viewportHeight * fabs( ( clipPos2[1] / clipPos2[2] ) - ( clipPos1[1] / clipPos1[2] ) );
	if( metric != 0.0f )
	{
		metric = 100.0f / metric;
	}

	if( pMetric )
	{
		*pMetric = metric;
	}
	
	int i;
	int numLODs = GetNumLODs( hardwareData );

	bool hasShadowLOD = GetLODSwitchValue( hardwareData, numLODs-1 ) < 0.0f ? true : false;

	for( i = 0; i < numLODs-1; i++ )
	{
		if ( hasShadowLOD && ( i == numLODs - 1 ) )
			continue;

		if( GetLODSwitchValue( hardwareData, i+1 ) > metric )
		{
			return i;
		}
	}
	
	if ( hasShadowLOD )
	{
		return numLODs - 2;
	}
	return numLODs - 1;
}

//-----------------------------------------------------------------------------
// Sets the color modulation
//-----------------------------------------------------------------------------

void CStudioRender::SetColorModulation( float const* pColor )
{
	VectorCopy( pColor, m_ColorMod );
}

void CStudioRender::SetAlphaModulation( float alpha )
{
	m_AlphaMod = alpha;
}

//-----------------------------------------------------------------------------
// Computes PoseToWorld from BoneToWorld
//-----------------------------------------------------------------------------
void CStudioRender::ComputePoseToWorld( studiohdr_t *pStudioHdr )
{ 

	// convert bone to world transformations into pose to world transformations
	for (int i = 0; i < pStudioHdr->numbones; i++)
	{
		mstudiobone_t *pCurBone = pStudioHdr->pBone(i);

		// Pretransform

		if( !( pCurBone->flags & ( BONE_SCREEN_ALIGN_SPHERE | BONE_SCREEN_ALIGN_CYLINDER )))
		{

			ConcatTransforms( m_BoneToWorld[ i ], pCurBone->poseToBone, m_PoseToWorld[ i ] );
		}
		// If this bone is screen aligned, then generate a PoseToWorld matrix that billboards the bone
		else 
		{
			ScreenAlignBone( pStudioHdr, i );
		} 	
	}
}

//-----------------------------------------------------------------------------
// Generates the PoseToBone Matrix nessecary to align the given bone with the 
// world.
//-----------------------------------------------------------------------------
#ifdef _WIN32
	#pragma warning (disable: 4701)	// warning C4701: local variable 'vX' may be used without having been initialized
#endif

void CStudioRender::ScreenAlignBone( studiohdr_t *pStudioHdr, int i )
{
	mstudiobone_t *pCurBone = pStudioHdr->pBone(i);

	// Grab the world translation:
	Vector vT( m_BoneToWorld[i][0][3], m_BoneToWorld[i][1][3], m_BoneToWorld[i][2][3] );

	//m_pMaterialSystem->GetMatrix( MATERIAL_VIEW, &matView );

	// Construct the coordinate frame:
	// Initialized to get rid of compiler 
	Vector vX, vY, vZ;

	if( pCurBone->flags & BONE_SCREEN_ALIGN_SPHERE )
	{
		vX = m_ViewOrigin - vT;		    VectorNormalize(vX);
		vZ = Vector(0,0,1);
		vY = vZ.Cross(vX);				VectorNormalize(vY);
		vZ = vX.Cross(vY);				VectorNormalize(vZ);
	} 
	else if( pCurBone->flags & BONE_SCREEN_ALIGN_CYLINDER )
	{
		vX.Init(m_BoneToWorld[i][0][0], m_BoneToWorld[i][1][0], m_BoneToWorld[i][2][0] );
		vZ = m_ViewOrigin - vT;			VectorNormalize(vZ);
		vY = vZ.Cross(vX);				VectorNormalize(vY);
		vZ = vX.Cross(vY);				VectorNormalize(vZ);
	}

	matrix3x4_t matBoneBillboard( 
		vX.x, vY.x, vZ.x, vT.x, 
		vX.y, vY.y, vZ.y, vT.y, 
		vX.z, vY.z, vZ.z, vT.z );
	ConcatTransforms( matBoneBillboard, pCurBone->poseToBone, m_PoseToWorld[ i ] );
}

#ifdef _WIN32
	#pragma warning (default: 4701)	
#endif

//-----------------------------------------------------------------------------
// Shadow state (affects the models as they are rendered)
//-----------------------------------------------------------------------------
void CStudioRender::AddShadow( IMaterial* pMaterial, void* pProxyData )
{
	int i = m_ShadowState.AddToTail();
	ShadowState_t& state = m_ShadowState[i];
	state.m_pMaterial = pMaterial;
	state.m_pProxyData = pProxyData;
}

void CStudioRender::ClearAllShadows()
{
	m_ShadowState.RemoveAll();
}


//-----------------------------------------------------------------------------
// Main model rendering entry point
//-----------------------------------------------------------------------------
//static ConVar r_accuratemodeltime( "r_accuratemodeltime", "0" );
//const bool bAccurateModelTime = true;

int CStudioRender::DrawModel( DrawModelInfo_t& info, Vector const& origin, int *pLodUsed, float *pMetric, int flags )
{
	VPROF( "CStudioRender::DrawModel ");

	// Set to zero in case we don't render anything.
	info.m_ActualTriCount = info.m_TextureMemoryBytes = 0;
	
	if( !info.m_pStudioHdr || !info.m_pHardwareData || 
		!info.m_pHardwareData->m_NumLODs || !info.m_pHardwareData->m_pLODs )
	{
		return 0;
	} 

	if( flags & STUDIORENDER_DRAW_ACCURATETIME )
	{
		VPROF("STUDIORENDER_DRAW_ACCURATETIME");

		// Flush the material system before timing this model:
		m_pMaterialSystem->Flush(true);
		m_pMaterialSystem->Flush(true);
	}

	info.m_RenderTime.Start();

	int lod = info.m_Lod;
	int lastlod = info.m_pHardwareData->m_NumLODs - 1;

	if ( lod == USESHADOWLOD )
	{
		lod = lastlod;
	} 
	else if ( lod == -1 )
	{
		lod = CalculateLOD( origin, *info.m_pHardwareData, pMetric );

		// make sure we have a valid lod
		if ( info.m_pStudioHdr->flags & STUDIOHDR_FLAGS_HASSHADOWLOD )
		{
			lastlod--;
		}

		lod = clamp( lod, 0, lastlod );
	}
	else
	{
		lod = clamp( lod, 0, lastlod );
		if( pMetric )
			*pMetric = 0.0f;
	}

	// Disable flex if we're told to...
	bool flexConfig = m_Config.bFlex;
	if (flags & STUDIORENDER_DRAW_NO_FLEXES)
		m_Config.bFlex = false;

	if( pLodUsed )
	{
		*pLodUsed = lod;
	}

	info.m_Lod = lod;
//	StudioStats().IncrementCountedStat( STUDIO_STATS_ACTUAL_MODEL_TRIS, actualTrisRendered );
//	StudioStats().IncrementCountedStat( STUDIO_STATS_EFFECTIVE_MODEL_TRIS, effectiveTrisRendered );

	// Bone to world must be set before calling drawmodel; it uses that here
	ComputePoseToWorld( info.m_pStudioHdr );

	// Preserve the matrices if we're skinning
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->PushMatrix();
	m_pMaterialSystem->LoadIdentity();

	m_VertexCache.StartModel();

	m_pStudioHdr = info.m_pStudioHdr;
	m_pStudioMeshes = info.m_pHardwareData->m_pLODs[lod].m_pMeshData;
	int retVal = R_StudioRenderModel( info.m_Skin, info.m_Body, info.m_HitboxSet, info.m_pClientEntity,
		info.m_pHardwareData->m_pLODs[lod].ppMaterials, 
		info.m_pHardwareData->m_pLODs[lod].pMaterialFlags, flags, info.m_ppColorMeshes );

	// FIXME: Should this occur in a separate call?
	// Draw all the decals on this model
	if ((flags & STUDIORENDER_DRAW_GROUP_MASK) != STUDIORENDER_DRAW_TRANSLUCENT_ONLY)
	{
		DrawDecal( info.m_Decals, info.m_pStudioHdr, lod, info.m_Body );
	}

	// Draw shadows
	DrawShadows( info.m_Body, flags );

	// Restore the matrices if we're skinning
	m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	m_pMaterialSystem->PopMatrix();

	// Restore the flex config
	m_Config.bFlex = flexConfig;

	if( flags & STUDIORENDER_DRAW_ACCURATETIME )
	{
		VPROF("STUDIORENDER_DRAW_ACCURATETIME");

		// Make sure this model is completely drawn before ending the timer:
		m_pMaterialSystem->Flush(true);
		m_pMaterialSystem->Flush(true);
	}

	info.m_RenderTime.End();

	GetPerfStats( info );

	return retVal;
}

bool CStudioRender::Mod_LoadStudioModelVertexData( studiohdr_t *pStudioHdr, void *pVtxBuffer,
									    int numStudioMeshes, 
										int	*numLODs,
										studioloddata_t	**ppLODs )
{
	assert( pStudioHdr );
	assert( pVtxBuffer );
	if( !pStudioHdr || !pVtxBuffer )
	{
		return false;
	}
	
	// NOTE: This must be called *after* Mod_LoadStudioModel
	OptimizedModel::FileHeader_t* pVertexHdr = (OptimizedModel::FileHeader_t*)pVtxBuffer; 

	// Create static meshes
	assert( pVertexHdr->numLODs );
	*numLODs = pVertexHdr->numLODs;
	*ppLODs = new studioloddata_t[*numLODs];
	memset( *ppLODs, 0, sizeof( studioloddata_t ) * *numLODs );
	int lodID;
	for( lodID = 0; lodID < pVertexHdr->numLODs; lodID++ )
	{
		if( !R_StudioCreateStaticMeshes( pStudioHdr->name, pStudioHdr, pVertexHdr,
			numStudioMeshes, &((*ppLODs)[lodID].m_pMeshData), lodID ) )
		{
			return false;
		}
		// garymcthack - need to check for NULL here.
		(*ppLODs)[lodID].m_SwitchPoint = pVertexHdr->pBodyPart( 0 )->pModel( 0 )->pLOD( lodID )->switchPoint;

		// Load materials
		LoadMaterials( pStudioHdr, pVertexHdr, (*ppLODs)[lodID], lodID );
	}

	return true;
}

void CStudioRender::GetPerfStats( DrawModelInfo_t &info, CUtlBuffer *pSpewBuf ) const
{
	Assert( info.m_Lod >= 0 );
	info.m_ActualTriCount = info.m_TextureMemoryBytes = 0;
	studiomeshdata_t *pStudioMeshes = info.m_pHardwareData->m_pLODs[info.m_Lod].m_pMeshData;
	
//	Warning( "\n\n\n" );
	int numMaterialStateChanges = 0;
	int numBoneStateChangeBatches = 0;
	int numBoneStateChanges = 0;
	// Iterate over every submodel...
	IMaterial **ppMaterials = info.m_pHardwareData->m_pLODs[info.m_Lod].ppMaterials;
	
	if ( info.m_Skin >= info.m_pStudioHdr->numskinfamilies )
	{
		info.m_Skin = 0;
	}
	short *pSkinRef	= info.m_pStudioHdr->pSkinref( info.m_Skin * info.m_pStudioHdr->numskinref );
	
	int i;
	for (i=0 ; i < info.m_pStudioHdr->numbodyparts ; i++) 
	{
		mstudiomodel_t *pModel = NULL;
		R_StudioSetupModel( i, info.m_Body, &pModel, info.m_pStudioHdr );
		
		// Iterate over all the meshes.... each mesh is a new material
		int k;
		for( k = 0; k < pModel->nummeshes; ++k )
		{
			numMaterialStateChanges++;
			mstudiomesh_t *pMesh = pModel->pMesh(k);
			IMaterial *pMaterial = ppMaterials[pSkinRef[pMesh->material]];
			Assert( pMaterial );
			if( pSpewBuf )
			{
				pSpewBuf->Printf( "\tmaterial: %s\n", pMaterial->GetName() );
			}
			int numPasses = m_pForcedMaterial ? m_pForcedMaterial->GetNumPasses() : pMaterial->GetNumPasses();
			if( pSpewBuf )
			{
				pSpewBuf->Printf( "\t\tnumPasses: %d\n", numPasses );
			}
			int bytes = pMaterial->GetTextureMemoryBytes();
			info.m_TextureMemoryBytes += bytes;
			if( pSpewBuf )
			{
				pSpewBuf->Printf( "\t\ttexture memory: %d\n", bytes );
			}
			studiomeshdata_t *pMeshData = &pStudioMeshes[pMesh->meshid];
			
			// Iterate over all stripgroups
			int stripGroupID;
			for( stripGroupID = 0; stripGroupID < pMeshData->m_NumGroup; stripGroupID++ )
			{
				studiomeshgroup_t *pMeshGroup = &pMeshData->m_pMeshGroup[stripGroupID];
				bool bIsFlexed = ( pMeshGroup->m_Flags & MESHGROUP_IS_FLEXED ) != 0;
				bool bIsHWSkinned = ( pMeshGroup->m_Flags & MESHGROUP_IS_HWSKINNED ) != 0;
				
				// Iterate over all strips. . . each strip potentially changes bones states.
				int stripID;
				for( stripID = 0; stripID < pMeshGroup->m_NumStrips; stripID++ )
				{
					OptimizedModel::StripHeader_t *pStripData = &pMeshGroup->m_pStripData[stripID];
					numBoneStateChangeBatches++;
					numBoneStateChanges += pStripData->numBoneStateChanges;
					if( pStripData->flags & OptimizedModel::STRIP_IS_TRILIST )
					{
						// TODO: need to factor in bIsFlexed and bIsHWSkinned
						int numTris = pStripData->numIndices / 3;
						if( pSpewBuf )
						{
							pSpewBuf->Printf( "\t\t%s%s", bIsFlexed ? "flexed " : "",
								bIsHWSkinned ? "" : "swskinned " );
							pSpewBuf->Printf( "tris: %d ", numTris );
							pSpewBuf->Printf( "bone changes: %d bones/strip: %d\n", pStripData->numBoneStateChanges,
								( int )pStripData->numBones );
						}
						info.m_ActualTriCount += numTris * numPasses;
					}
					else if( pStripData->flags & OptimizedModel::STRIP_IS_TRISTRIP )
					{
						Assert( 0 ); // FIXME: fill this in when we start using strips again.
					}
					else
					{
						Assert( 0 );
					}
				}
			}
		}
	}	
}
