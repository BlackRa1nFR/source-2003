//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "Material.h"
#include "Render3D.h"
#include "StudioModel.h"
#include "ViewerSettings.h"
#include "cache.h"
#include "materialsystem/IMesh.h"
#include "TextureSystem.h"
#include "bone_setup.h"
#include "IStudioRender.h"
#include "GameData.h"
#include "GlobalFunctions.h"
#include "UtlMemory.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"

#pragma warning(disable : 4244) // double to float

Vector			g_lightvec;						// light vector in model reference frame
Vector			g_blightvec[MAXSTUDIOBONES];	// light vectors in bone reference frames
int				g_ambientlight;					// ambient world light
float			g_shadelight;					// direct world light
Vector			g_lightcolor;

IStudioRender	*StudioModel::m_pStudioRender = NULL;

//-----------------------------------------------------------------------------
// Model meshes themselves are cached to avoid redundancy. There should never be
// more than one copy of a given studio model in memory at once.
//-----------------------------------------------------------------------------
ModelCache_t CStudioModelCache::m_Cache[1024];
int CStudioModelCache::m_nItems = 0;


//-----------------------------------------------------------------------------
// Purpose: Returns an instance of a particular studio model. If the model is
//			in the cache, a pointer to that model is returned. If not, a new one
//			is created and added to the cache.
// Input  : pszModelPath - Full path of the .MDL file.
//-----------------------------------------------------------------------------
StudioModel *CStudioModelCache::CreateModel(const char *pszModelPath)
{
	//
	// First look for the model in the cache. If it's there, increment the
	// reference count and return a pointer to the cached model.
	//
	for (int i = 0; i < m_nItems; i++)
	{
		if (!stricmp(pszModelPath, m_Cache[i].pszPath))
		{
			m_Cache[i].nRefCount++;
			return(m_Cache[i].pModel);
		}
	}

	//
	// If it isn't there, try to create one.
	//
	StudioModel *pModel = new StudioModel;

	if (pModel != NULL)
	{
		bool bLoaded = pModel->LoadModel(pszModelPath);

		if (bLoaded)
		{
			bLoaded = pModel->PostLoadModel(pszModelPath);
		}

		if (!bLoaded)
		{
			delete pModel;
			pModel = NULL;
		}
	}

	//
	// If we successfully created it, add it to the cache.
	//
	if (pModel != NULL)
	{
		CStudioModelCache::AddModel(pModel, pszModelPath);
	}

	return(pModel);
}


//-----------------------------------------------------------------------------
// Purpose: Adds the model to the cache, setting the reference count to one.
// Input  : pModel - Model to add to the cache.
//			pszModelPath - The full path of the .MDL file, which is used as a
//				key in the model cache.
// Output : Returns TRUE if the model was successfully added, FALSE if we ran
//			out of memory trying to add the model to the cache.
//-----------------------------------------------------------------------------
BOOL CStudioModelCache::AddModel(StudioModel *pModel, const char *pszModelPath)
{
	//
	// Copy the model pointer.
	//
	m_Cache[m_nItems].pModel = pModel;

	//
	// Allocate space for and copy the model path.
	//
	m_Cache[m_nItems].pszPath = new char [strlen(pszModelPath) + 1];
	if (m_Cache[m_nItems].pszPath != NULL)
	{
		strcpy(m_Cache[m_nItems].pszPath, pszModelPath);
	}
	else
	{
		return(FALSE);
	}

	m_Cache[m_nItems].nRefCount = 1;

	m_nItems++;

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Advances the animation of all models in the cache for the given interval.
// Input  : flInterval - delta time in seconds.
//-----------------------------------------------------------------------------
void CStudioModelCache::AdvanceAnimation(float flInterval)
{
	for (int i = 0; i < m_nItems; i++)
	{
		m_Cache[i].pModel->AdvanceFrame(flInterval);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Increments the reference count on a model in the cache. Called by
//			client code when a pointer to the model is copied, making that
//			reference independent.
// Input  : pModel - Model for which to increment the reference count.
//-----------------------------------------------------------------------------
void CStudioModelCache::AddRef(StudioModel *pModel)
{
	for (int i = 0; i < m_nItems; i++)
	{
		if (m_Cache[i].pModel == pModel)
		{
			m_Cache[i].nRefCount++;
			return;
		}
	}	
}


//-----------------------------------------------------------------------------
// Purpose: Called by client code to release an instance of a model. If the
//			model's reference count is zero, the model is freed.
// Input  : pModel - Pointer to the model to release.
//-----------------------------------------------------------------------------
void CStudioModelCache::Release(StudioModel *pModel)
{
	for (int i = 0; i < m_nItems; i++)
	{
		if (m_Cache[i].pModel == pModel)
		{
			m_Cache[i].nRefCount--;
			ASSERT(m_Cache[i].nRefCount >= 0);

			//
			// If this model is no longer referenced, free it and remove it
			// from the cache.
			//
			if (m_Cache[i].nRefCount <= 0)
			{
				//
				// Free the path, which was allocated by AddModel.
				//
				delete [] m_Cache[i].pszPath;
				delete m_Cache[i].pModel;

				//
				// Decrement the item count and copy the last element in the cache over
				// this element.
				//
				m_nItems--;

				m_Cache[i].pModel = m_Cache[m_nItems].pModel;
				m_Cache[i].pszPath = m_Cache[m_nItems].pszPath;
				m_Cache[i].nRefCount = m_Cache[m_nItems].nRefCount;
			}

			break;
		}
	}	
}

//-----------------------------------------------------------------------------
// Releases/restores models in case of task switch
//-----------------------------------------------------------------------------

// Used to release/restore static meshes
CUtlVector<StudioModel*> StudioModel::s_StudioModels;

void StudioModel::ReleaseStudioModels()
{
	for (int i = s_StudioModels.Size(); --i >= 0; )
	{
		s_StudioModels[i]->ReleaseStaticMesh();
	}
}

void StudioModel::RestoreStudioModels()
{
	for (int i = s_StudioModels.Size(); --i >= 0; )
	{
		s_StudioModels[i]->RestoreStaticMesh();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads up the IStudioRender interface.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StudioModel::Initialize()
{
	extern CreateInterfaceFn g_MaterialSystemClientFactory;
	extern CreateInterfaceFn g_MaterialSystemFactory;

	if (( !g_MaterialSystemFactory ) || ( !g_MaterialSystemClientFactory ))
	{
		// Don't report this, it should have been reported earlier.
		return false;
	}

	CSysModule *studioRenderDLL = NULL;
	studioRenderDLL = Sys_LoadModule( "StudioRender.dll" );
	if( !studioRenderDLL )
	{
		Msg( mwWarning, "Can't load StudioRender.dll" );
		return false;
	}

	CreateInterfaceFn studioRenderFactory = Sys_GetFactory( "StudioRender.dll" );
	if (!studioRenderFactory )
	{
		Msg( mwWarning, "Can't get studio render factory" );
		return false;
	}

	m_pStudioRender = ( IStudioRender * )studioRenderFactory( STUDIO_RENDER_INTERFACE_VERSION, NULL );
	if (!m_pStudioRender)
	{
		Msg( mwWarning, "Can't get version %s of StudioRender.dll", STUDIO_RENDER_INTERFACE_VERSION );
		return false;
	}

	if( !m_pStudioRender->Init( g_MaterialSystemFactory, g_MaterialSystemClientFactory ) )
	{
		Msg( mwWarning, "Can't initialize StudioRender.dll" );
		m_pStudioRender = NULL;
	}
	UpdateStudioRenderConfig( false, false );

	return true;
}


void StudioModel::Shutdown( void )
{
	if( m_pStudioRender )
	{
		m_pStudioRender->Shutdown();
		m_pStudioRender = NULL;
	}
}

#define	MAXPRINTMSG	4096
static void StudioRender_Warning( const char *fmt, ... )
{
	// This caused a crash in release build...
#ifdef _DEBUG
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

//	Msg( mwWarning, "StudioRender: %s", msg );
	OutputDebugString( msg );
#endif
}

void StudioModel::UpdateStudioRenderConfig( bool bFlat, bool bWireframe )
{
	StudioRenderConfig_t config;
	memset( &config, 0, sizeof( config ) );
	config.pConPrintf = StudioRender_Warning;
	config.pConDPrintf = StudioRender_Warning;
	config.fEyeShiftX = 0.0f;
	config.fEyeShiftY = 0.0f;
	config.fEyeShiftZ = 0.0f;
	config.fEyeSize = 0;
	config.gamma = 2.2f;
	config.texGamma = 2.2f;
	config.brightness = 0.0f;
	config.overbrightFactor = 1.0f;
	config.modelLightBias = 1.0f;
	config.eyeGloss = 0;
	config.drawEntities = 1;
	config.skin = 0;
#if 0
	// uncomment this when lighting is enabled.
	if( bFlat )
	{
		config.fullbright = 2;
	}
	else
#endif
	{
		config.fullbright = 1;
	}
	config.bEyeMove = true;
	config.bSoftwareSkin = false;
	config.bNoHardware = false;
	config.bNoSoftware = false;
	config.bTeeth = false;
	config.bEyes = true;
	config.bFlex = true;
	config.bSoftwareLighting = true;
	if( bWireframe )
	{
		config.bWireframe = true;
	}
	else
	{
		config.bWireframe = false;
	}
	config.bNormals = false;
	config.bUseAmbientCube = true;
	config.bShowEnvCubemapOnly = false;
	config.bStaticLighting = false;
	m_pStudioRender->UpdateConfig( config );
}

void StudioModel::UpdateViewState( const Vector& viewOrigin,
				 				   const Vector& viewRight,
								   const Vector& viewUp,
								   const Vector& viewPlaneNormal )
{
	if( m_pStudioRender )
	{
		m_pStudioRender->SetViewState( viewOrigin, viewRight, viewUp, viewPlaneNormal );
		m_pStudioRender->SetEyeViewTarget( viewOrigin );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
StudioModel::StudioModel(void) : m_pModelName(0)
{
	int i;

	ZeroVector(m_origin);
	ZeroVector(m_angles);
	m_sequence = 0;
	m_cycle = 0;
	m_bodynum = 0;
	m_skinnum = 0;

	for (i = 0; i < sizeof(m_controller) / sizeof(m_controller[0]); i++)
	{
		m_controller[i] = 0;
	}

	for (i = 0; i < sizeof(m_poseParameter) / sizeof(m_poseParameter[0]); i++)
	{
		m_poseParameter[i] = 0;
	}

	m_mouth = 0;

	m_pStudioHdr = NULL;
	m_pModel = NULL;
	memset( &m_HardwareData, 0, sizeof( m_HardwareData ) );

	s_StudioModels.AddToTail( this );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees dynamically allocated data.
//-----------------------------------------------------------------------------
StudioModel::~StudioModel(void)
{
	FreeModel();
	if (m_pModelName)
		delete[] m_pModelName;

	int i = s_StudioModels.Find(this);
	if (i >= 0)
		s_StudioModels.FastRemove( i );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the Euler angles for the model.
// Input  : fAngles - A pointer to engine PITCH, YAW, and ROLL angles.
//-----------------------------------------------------------------------------
void StudioModel::SetAngles(QAngle& pfAngles)
{
	m_angles[PITCH] = pfAngles[PITCH];
	m_angles[YAW] = pfAngles[YAW];
	m_angles[ROLL] = pfAngles[ROLL];
}


void StudioModel::AdvanceFrame( float dt )
{
	if (dt > 0.1)
		dt = 0.1f;

	float t = Studio_Duration( m_pStudioHdr, m_sequence, m_poseParameter );

	if (t > 0)
	{
		m_cycle += dt / t;

		// wrap
		m_cycle -= (int)(m_cycle);
	}
	else
	{
		m_cycle = 0;
	}
}




void StudioModel::SetUpBones ( void )
{
	int					i;

	mstudiobone_t		*pbones;

	static Vector		pos[MAXSTUDIOBONES];
	matrix3x4_t			bonematrix;
	static Quaternion	q[MAXSTUDIOBONES];
	
	CalcPose( m_pStudioHdr, NULL, pos, q, m_sequence, m_cycle, m_poseParameter, BONE_USED_BY_ANYTHING );

	pbones = m_pStudioHdr->pBone( 0 );

	matrix3x4_t cameraTransform;
	AngleMatrix( m_angles, cameraTransform );
	cameraTransform[0][3] = m_origin[0];
	cameraTransform[1][3] = m_origin[1];
	cameraTransform[2][3] = m_origin[2];
	
	for (i = 0; i < m_pStudioHdr->numbones; i++) 
	{
		if (CalcProceduralBone( m_pStudioHdr, i, m_pStudioRender->GetBoneToWorldArray() ))
		{
			continue;
		}

		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) 
		{
			ConcatTransforms( cameraTransform, bonematrix, *m_pStudioRender->GetBoneToWorld( i ) );
		} 
		else 
		{
			ConcatTransforms (*m_pStudioRender->GetBoneToWorld( pbones[i].parent ), bonematrix, *m_pStudioRender->GetBoneToWorld( i ) );
		}
	}
}



/*
/*
================
StudioModel::SetupLighting
	set some global variables based on entity position
inputs:
outputs:
	g_ambientlight
	g_shadelight
================
*/
void StudioModel::SetupLighting ( )
{
}


/*
=================
StudioModel::SetupModel
	based on the body part, figure out which mesh it should be using.
inputs:
	currententity
outputs:
	pstudiomesh
	pmdl
=================
*/

void StudioModel::SetupModel ( int bodypart )
{
	int index;

	if (bodypart > m_pStudioHdr->numbodyparts)
	{
		// Con_DPrintf ("StudioModel::SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	mstudiobodyparts_t   *pbodypart = m_pStudioHdr->pBodypart( bodypart );

	index = m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	m_pModel = pbodypart->pModel( index );
}

/*
================
StudioModel::DrawModel
inputs:
	currententity
	r_entorigin
================
*/
void StudioModel::DrawModel( CRender3D *pRender )
{
	if (!m_pStudioHdr)
		return;

	if (m_pStudioHdr->numbodyparts == 0)
		return;

	UpdateStudioRenderConfig( pRender->GetCurrentRenderMode() == RENDER_MODE_FLAT,
		pRender->GetCurrentRenderMode() == RENDER_MODE_WIREFRAME ); // garymcthack - should really only do this once a frame and at init time.
	
	MaterialSystemInterface()->MatrixMode(MATERIAL_MODEL);
	MaterialSystemInterface()->PushMatrix ();
	MaterialSystemInterface()->LoadIdentity();

	SetUpBones ( );

	SetupLighting( );
	
	m_pStudioRender->SetAlphaModulation( 1.0f );

	DrawModelInfo_t info;
	info.m_pStudioHdr = m_pStudioHdr;
	info.m_pHardwareData = &m_HardwareData;
	info.m_Decals = STUDIORENDER_DECAL_INVALID;
	info.m_Skin = m_skinnum;
	info.m_Body = m_bodynum;
	info.m_HitboxSet = 0;

	info.m_pClientEntity = NULL;
	info.m_Lod = -1;
	info.m_ppColorMeshes = NULL;

	m_pStudioRender->DrawModel( info, m_origin, NULL, NULL, STUDIORENDER_DRAW_ENTIRE_MODEL );

	MaterialSystemInterface()->MatrixMode(MATERIAL_MODEL);
	MaterialSystemInterface()->PopMatrix ();

	// Force the render mode to flush itself.
	RenderMode_t saveRenderMode = pRender->GetCurrentRenderMode();
	pRender->SetRenderMode( saveRenderMode, true );
}


//-----------------------------------------------------------------------------
// It's translucent if all its materials are translucent
//-----------------------------------------------------------------------------

bool StudioModel::IsTranslucent()
{
	// garymcthack - shouldn't crack hardwaredata
	int lodID;
	for( lodID = 0; lodID < m_HardwareData.m_NumLODs; lodID++ )
	{
		for (int i = 0; i < m_HardwareData.m_pLODs[lodID].numMaterials; ++i)
		{
			if (!m_HardwareData.m_pLODs[lodID].ppMaterials[i]->IsTranslucent())
				return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Frees the model data and releases textures from OpenGL.
//-----------------------------------------------------------------------------
void StudioModel::FreeModel(void)
{
	m_pStudioRender->UnloadModel( &m_HardwareData );
	delete [] m_pStudioHdr;

	m_pStudioHdr = NULL;
	m_pModel = NULL;
	memset( &m_HardwareData, 0, sizeof( m_HardwareData ) );
}


bool StudioModel::LoadModel(const char *modelname)
{
	FILE *fp;
	long size;
	void *buffer;

	// Load the MDL file.
	assert( !m_pStudioHdr );

	if( !m_pStudioRender )
	{
		return false;
	}

	if (!modelname)
		return false;

	// In the case of restore, m_pModelName == modelname
	if (m_pModelName != modelname)
	{
		// Copy over the model name; we'll need it later...
		if (m_pModelName)
			delete[] m_pModelName;
		m_pModelName = new char[strlen(modelname) + 1];
		strcpy( m_pModelName, modelname );
	}

	// load the model
	if( (fp = fopen( modelname, "rb" )) == NULL)
		return false;

	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	buffer = new unsigned char[size];
	if (!buffer)
	{
		fclose (fp);
		return false;
	}

	fread( buffer, size, 1, fp );
	fclose( fp );

	byte				*pin;
	studiohdr_t			*phdr;
	mstudiotexture_t	*ptexture;

	pin = (byte *)buffer;
	phdr = (studiohdr_t *)pin;
	ptexture = (mstudiotexture_t *)(pin + phdr->textureindex);

	if (strncmp ((const char *) buffer, "IDST", 4) &&
		strncmp ((const char *) buffer, "IDSQ", 4))
	{
		delete [] buffer;
		return false;
	}

	if (!strncmp ((const char *) buffer, "IDSQ", 4) && !m_pStudioHdr)
	{
		delete [] buffer;
		return false;
	}

	Studio_ConvertStudioHdrToNewVersion( phdr );
	if (phdr->version != STUDIO_VERSION)
	{
		delete [] buffer;
		return false;
	}

	m_pStudioHdr = (studiohdr_t *)buffer;

	// Load the VTX file.
	char* pExtension;
	if (MaterialSystemHardwareConfig()->SupportsVertexAndPixelShaders())
	{
		pExtension = ".dx80.vtx";
	}
	else
	{
		if( MaterialSystemHardwareConfig()->MaxBlendMatrices() > 2 )
		{
			pExtension = ".dx7_3bone.vtx";
		}
		else
		{
			pExtension = ".dx7_2bone.vtx";
		}
	}

	int vtxFileNameLen = strlen( modelname ) - strlen( ".mdl" ) + strlen( pExtension ) + 1;
	char *vtxFileName = ( char * )_alloca( vtxFileNameLen );
	strcpy( vtxFileName, modelname );
	strcpy( vtxFileName + strlen( vtxFileName ) - 4, pExtension );
	assert( ( int )strlen( vtxFileName ) == vtxFileNameLen - 1 );
	
	CUtlMemory<unsigned char> tmpVtxMem; // This goes away when we leave this scope.
	
	if( (fp = fopen( vtxFileName, "rb" )) == NULL)
	{
		// fallback
		pExtension = ".dx7_2bone.vtx";
		vtxFileNameLen = strlen( modelname ) - strlen( ".mdl" ) + strlen( pExtension ) + 1;
		vtxFileName = ( char * )_alloca( vtxFileNameLen );
		strcpy( vtxFileName, modelname );
		strcpy( vtxFileName + strlen( vtxFileName ) - 4, pExtension );
		assert( ( int )strlen( vtxFileName ) == vtxFileNameLen - 1 );
		if( (fp = fopen( vtxFileName, "rb" )) == NULL)
		{
			Msg( mwWarning, "Can't find vtx file: %s\n", vtxFileName );
			delete [] m_pStudioHdr;
			m_pStudioHdr = NULL;
			return false;
		}
	}

	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	tmpVtxMem.EnsureCapacity( size );
	
	fread( tmpVtxMem.Base(), size, 1, fp );
	fclose( fp );

	if( !m_pStudioRender->LoadModel( m_pStudioHdr, tmpVtxMem.Base(), &m_HardwareData ) )
	{
		Msg( mwWarning, "error loading model: %s\n", modelname );
		delete [] m_pStudioHdr;
		m_pStudioHdr = NULL;
		return false;
	}

	return true;
}



bool StudioModel::PostLoadModel(const char *modelname)
{
	if (m_pStudioHdr == NULL)
	{
		return(false);
	}

	SetSequence (0);

	for (int n = 0; n < m_pStudioHdr->numbodyparts; n++)
	{
		if (SetBodygroup (n, 0) < 0)
		{
			return false;
		}
	}

	SetSkin (0);


/*
	Vector mins, maxs;
	ExtractBbox (mins, maxs);
	if (mins[2] < 5.0f)
		m_origin[2] = -mins[2];
*/
	return true;
}

//-----------------------------------------------------------------------------
// Release, restore static mesh in case of task switch
//-----------------------------------------------------------------------------

void StudioModel::ReleaseStaticMesh()
{
	FreeModel(); 
}

void StudioModel::RestoreStaticMesh()
{
	if (LoadModel(m_pModelName))
	{
		PostLoadModel( m_pModelName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int StudioModel::GetSequenceCount( void )
{
	return m_pStudioHdr->numseq;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//			szName - 
//-----------------------------------------------------------------------------
void StudioModel::GetSequenceName( int nIndex, char *szName )
{
	if (nIndex < m_pStudioHdr->numseq)
	{
		strcpy(szName, m_pStudioHdr->pSeqdesc(nIndex)->pszLabel());
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the index of the current sequence.
//-----------------------------------------------------------------------------
int StudioModel::GetSequence( )
{
	return m_sequence;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the current sequence by index.
//-----------------------------------------------------------------------------
int StudioModel::SetSequence( int iSequence )
{
	if (iSequence > m_pStudioHdr->numseq)
		return m_sequence;

	m_sequence = iSequence;
	m_cycle = 0;

	return m_sequence;
}


//-----------------------------------------------------------------------------
// Purpose: Rotates the given bounding box by the given angles and computes the
//			bounds of the rotated box. This is used to take the rotation angles
//			into consideration when returning the bounding box. Note that this
//			can produce a larger than optimal bounding box.
// Input  : Mins - 
//			Maxs - 
//			Angles - 
//-----------------------------------------------------------------------------
void StudioModel::RotateBbox(Vector &Mins, Vector &Maxs, const QAngle &Angles)
{
	Vector Points[8];

	//
	// Generate the corner points of the box.
	//
	Points[0][0] = Mins[0];
	Points[0][1] = Mins[1];
	Points[0][2] = Mins[2];

	Points[1][0] = Mins[0];
	Points[1][1] = Mins[1];
	Points[1][2] = Maxs[2];

	Points[2][0] = Mins[0];
	Points[2][1] = Maxs[1];
	Points[2][2] = Mins[2];

	Points[3][0] = Mins[0];
	Points[3][1] = Maxs[1];
	Points[3][2] = Maxs[2];

	Points[4][0] = Maxs[0];
	Points[4][1] = Mins[1];
	Points[4][2] = Mins[2];

	Points[5][0] = Maxs[0];
	Points[5][1] = Mins[1];
	Points[5][2] = Maxs[2];

	Points[6][0] = Maxs[0];
	Points[6][1] = Maxs[1];
	Points[6][2] = Mins[2];

	Points[7][0] = Maxs[0];
	Points[7][1] = Maxs[1];
	Points[7][2] = Maxs[2];

	//
	// Rotate the corner points by the specified angles, in the same
	// order that our Render code uses.
	//
	matrix4_t fMatrix;
	IdentityMatrix(fMatrix);

	RotateZ(fMatrix, Angles[YAW]);
	RotateY(fMatrix, Angles[PITCH]);
	RotateX(fMatrix, Angles[ROLL]);

	matrix3x4_t fMatrix2;
	for (int nRow = 0; nRow < 3; nRow++)
	{
		for (int nCol = 0; nCol < 3; nCol++)
		{
			fMatrix2[nRow][nCol] = fMatrix[nRow][nCol];
		}
	}

	Vector RotatedPoints[8];
	for (int i = 0; i < 8; i++)
	{
		VectorRotate(Points[i], fMatrix2, RotatedPoints[i]);
	}

	//
	// Calculate the new mins and maxes.
	//
	for (i = 0; i < 8; i++)
	{
		for (int nDim = 0; nDim < 3; nDim++)
		{
			if ((i == 0) || (RotatedPoints[i][nDim] < Mins[nDim]))
			{
				Mins[nDim] = RotatedPoints[i][nDim];
			}

			if ((i == 0) || (RotatedPoints[i][nDim] > Maxs[nDim]))
			{
				Maxs[nDim] = RotatedPoints[i][nDim];
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mins - 
//			maxs - 
//-----------------------------------------------------------------------------
void StudioModel::ExtractBbox(Vector &mins, Vector &maxs)
{
	mstudioseqdesc_t	*pseqdesc;

	pseqdesc = m_pStudioHdr->pSeqdesc( 0 );
	
	mins[0] = pseqdesc[ m_sequence ].bbmin[0];
	mins[1] = pseqdesc[ m_sequence ].bbmin[1];
	mins[2] = pseqdesc[ m_sequence ].bbmin[2];

	maxs[0] = pseqdesc[ m_sequence ].bbmax[0];
	maxs[1] = pseqdesc[ m_sequence ].bbmax[1];
	maxs[2] = pseqdesc[ m_sequence ].bbmax[2];

	RotateBbox(mins, maxs, m_angles);
}


void StudioModel::ExtractClippingBbox( Vector& mins, Vector& maxs )
{
	mins[0] = m_pStudioHdr->view_bbmin[0];
	mins[1] = m_pStudioHdr->view_bbmin[1];
	mins[2] = m_pStudioHdr->view_bbmin[2];

	maxs[0] = m_pStudioHdr->view_bbmax[0];
	maxs[1] = m_pStudioHdr->view_bbmax[1];
	maxs[2] = m_pStudioHdr->view_bbmax[2];
}


void StudioModel::ExtractMovementBbox( Vector& mins, Vector& maxs )
{
	mins[0] = m_pStudioHdr->hull_min[0];
	mins[1] = m_pStudioHdr->hull_min[1];
	mins[2] = m_pStudioHdr->hull_min[2];

	maxs[0] = m_pStudioHdr->hull_max[0];
	maxs[1] = m_pStudioHdr->hull_max[1];
	maxs[2] = m_pStudioHdr->hull_max[2];
}


void StudioModel::GetSequenceInfo( float *pflFrameRate, float *pflGroundSpeed )
{
	float t = Studio_Duration( m_pStudioHdr, m_sequence, m_poseParameter );

	if (t > 0)
	{
		*pflFrameRate = 1.0 / t;
		*pflGroundSpeed = 0; // sqrt( pseqdesc->linearmovement[0]*pseqdesc->linearmovement[0]+ pseqdesc->linearmovement[1]*pseqdesc->linearmovement[1]+ pseqdesc->linearmovement[2]*pseqdesc->linearmovement[2] );
		// *pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 1.0;
		*pflGroundSpeed = 0.0;
	}
}

void StudioModel::SetOrigin( float x, float y, float z )
{
	m_origin[0] = x;
	m_origin[1] = y;
	m_origin[2] = z;
}


int StudioModel::SetBodygroup( int iGroup, int iValue )
{
	if (!m_pStudioHdr)
		return 0;

	if (iGroup > m_pStudioHdr->numbodyparts)
		return -1;

	mstudiobodyparts_t *pbodypart = m_pStudioHdr->pBodypart( iGroup );

	if ((pbodypart->base == 0) || (pbodypart->nummodels == 0))
	{
		return -1;
	}

	int iCurrent = (m_bodynum / pbodypart->base) % pbodypart->nummodels;

	if (iValue >= pbodypart->nummodels)
		return iCurrent;

	m_bodynum = (m_bodynum - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));

	return iValue;
}


int StudioModel::SetSkin( int iValue )
{
	if (!m_pStudioHdr)
		return 0;

	if (iValue >= m_pStudioHdr->numskinfamilies)
	{
		return m_skinnum;
	}

	m_skinnum = iValue;

	return iValue;
}

