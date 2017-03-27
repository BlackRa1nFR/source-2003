//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "materialsystem/IMesh.h"
#include "mat_stub.h"
#include "imageloader.h"

// Hook the engine's mat_stub cvar.
ConVar mat_stub( "mat_stub", "0" );
extern ConVar gl_clear;

// ---------------------------------------------------------------------------------------- //
// IMaterialSystem and IMesh stub classes.
// ---------------------------------------------------------------------------------------- //

class CDummyMesh : public IMesh
{
public:

	// Locks/ unlocks the mesh, providing space for numVerts and numIndices.
	// numIndices of -1 means don't lock the index buffer...
	virtual void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
	{
		int i;
		static float dummyFloat[65536];
		static unsigned short dummyShort[65536];
		static unsigned char dummyChar[65536];
		desc.m_VertexSize_Position = 0;
		desc.m_VertexSize_BoneWeight = 0;
		desc.m_VertexSize_BoneMatrixIndex = 0;
		desc.m_VertexSize_Normal = 0;
		desc.m_VertexSize_Color = 0;
		desc.m_VertexSize_Specular = 0;
		for( i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		{
			desc.m_VertexSize_TexCoord[i] = 0;
		}
		desc.m_VertexSize_TangentS = 0;
		desc.m_VertexSize_TangentT = 0;
		desc.m_VertexSize_TangentSxT = 0;
		desc.m_VertexSize_UserData = 0;
		desc.m_ActualVertexSize = 0;

		// The first vertex index
		desc.m_FirstVertex = 0;

		// Number of bone weights per vertex...
		desc.m_NumBoneWeights = 0;

		// Pointers to our current vertex data
		desc.m_pPosition = dummyFloat;
		desc.m_pBoneWeight = dummyFloat;
#ifdef NEW_SKINNING
		desc.m_pBoneMatrixIndex = dummyFloat;
#else
		desc.m_pBoneMatrixIndex = dummyChar;
#endif

		desc.m_pNormal = dummyFloat;
		desc.m_pColor = dummyChar;
		desc.m_pSpecular = dummyChar;
		for( i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		{
			desc.m_pTexCoord[i] = dummyFloat;
		}

		desc.m_pTangentS = dummyFloat;
		desc.m_pTangentT = dummyFloat;
		desc.m_pTangentSxT = dummyFloat;

		// user data
		desc.m_pUserData = dummyFloat;

		// Pointers to the index data
		desc.m_pIndices = dummyShort;
	}

	// Unlocks the mesh, indicating	how many verts and indices we actually used
	virtual void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
	{
	}

	// Locks mesh for modifying
	virtual void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
	{
	}
	virtual void ModifyEnd()
	{
	}

	// Helper methods to create various standard index buffer types
	virtual void GenerateSequentialIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex )
	{
	}
	virtual void GenerateQuadIndexBuffer( unsigned short* pIndexMemory, 
											int numIndices, int firstVertex )
	{
	}
	virtual void GeneratePolygonIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex )
	{
	}
	virtual void GenerateLineStripIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex )
	{
	}
	virtual void GenerateLineLoopIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex )
	{
	}

	// returns the # of vertices (static meshes only)
	virtual int NumVertices() const
	{
		return 0;
	}

	// Sets/gets the primitive type
	virtual void SetPrimitiveType( MaterialPrimitiveType_t type )
	{
	}
	
	// Draws the mesh
	virtual void Draw( int firstIndex = -1, int numIndices = 0 )
	{
	}
	
	virtual void SetColorMesh( IMesh *pColorMesh )
	{
	}

	// Draw a list of (lists of) primitives. Batching your lists together that use
	// the same lightmap, material, vertex and index buffers with multipass shaders
	// can drastically reduce state-switching overhead.
	// NOTE: this only works with STATIC meshes.
	virtual void Draw( CPrimList *pLists, int nLists )
	{
	}

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder )
	{
	}

	// Spews the mesh data
	virtual void Spew( int numVerts, int numIndices, const MeshDesc_t& desc )
	{
	}

	// Call this in debug mode to make sure our data is good.
	virtual void ValidateData( int numVerts, int numIndices, const MeshDesc_t& desc )
	{
	}

	virtual void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
	{
	}
};



class CDummyMaterialSystem : public IMaterialSystem
{
private:
	IMaterialSystem *m_pRealMaterialSystem;
public:
	CDummyMaterialSystem()
	{
		m_pRealMaterialSystem = 0;
	}

	virtual void	SetRealMaterialSystem( IMaterialSystem *pSys )
	{
		m_pRealMaterialSystem = pSys;
	}

	
	// Call this to initialize the material system
	// returns a method to create interfaces in the shader dll
	virtual CreateInterfaceFn	Init( char const* pShaderDLL, 
									  IMaterialProxyFactory *pMaterialProxyFactory,
									  CreateInterfaceFn fileSystemFactory,
									  CreateInterfaceFn cvarFactory )
	{
		return NULL;
	}

	virtual void				Shutdown( )
	{
	}

	virtual IMaterialSystemHardwareConfig *GetHardwareConfig( const char *pVersion, int *returnCode )
	{
		return NULL;
	}

	// Gets the number of adapters...
	virtual int					GetDisplayAdapterCount() const
	{
		return 0;
	}

	// Returns info about each adapter
	virtual void				GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const
	{
	}

	// Returns the number of modes
	virtual int					GetModeCount( int adapter ) const
	{
		return 0;
	}

	// Returns mode information..
	virtual void				GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const
	{
	}

	// Returns the mode info for the current display device
	virtual void				GetDisplayMode( MaterialVideoMode_t& mode ) const
	{
	}
 
	// Sets the mode...
	virtual bool				SetMode( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSuperSamples = 0 )
	{
		return true;
	}

	// Creates/ destroys a child window
	virtual bool				AddView( void* hwnd )
	{
		return false;
	}
	virtual void				RemoveView( void* hwnd )
	{
	}

	// Sets the view
	virtual void				SetView( void* hwnd )
	{
	}

	// return true if lightmaps need to be redownloaded
	// Call this before rendering each frame with the current config
	// for the material system.
	// Will do whatever is necessary to get the material system into the correct state
	// upon configuration change. .doesn't much else otherwise.
	virtual bool				UpdateConfig( MaterialSystem_Config_t *config, bool forceUpdate )
	{
		return false;
	}
	
	// This is the interface for knowing what materials are available
	// is to use the following functions to get a list of materials.  The
	// material names will have the full path to the material, and that is the 
	// only way that the directory structure of the materials will be seen through this
	// interface.
	// NOTE:  This is mostly for worldcraft to get a list of materials to put
	// in the "texture" browser.in Worldcraft
	virtual MaterialHandle_t	FirstMaterial() const
	{
		return 0;
	}

	// returns InvalidMaterial if there isn't another material.
	// WARNING: you must call GetNextMaterial until it returns NULL, 
	// otherwise there will be a memory leak.
	virtual MaterialHandle_t	NextMaterial( MaterialHandle_t h ) const
	{
		return 0;
	}

	// This is the invalid material
	virtual MaterialHandle_t	InvalidMaterial() const
	{
		return m_pRealMaterialSystem->InvalidMaterial();
	}

	// Returns a particular material
	virtual IMaterial*			GetMaterial( MaterialHandle_t h ) const
	{
		return m_pRealMaterialSystem->GetMaterial( h );
	}

	// Find a material by name.
	// The name of a material is a full path to 
	// the vmt file starting from "hl2/materials" (or equivalent) without
	// a file extension.
	// eg. "dev/dev_bumptest" refers to somethign similar to:
	// "d:/hl2/hl2/materials/dev/dev_bumptest.vmt"
	virtual IMaterial *			FindMaterial( char const* pMaterialName, bool *pFound, bool complain = true )
	{
		return m_pRealMaterialSystem->FindMaterial( pMaterialName, pFound, complain );
	}

	virtual ITexture *			FindTexture( char const* pTextureName, bool *pFound, bool complain = true )
	{
		return m_pRealMaterialSystem->FindTexture( pTextureName, pFound, complain );
	}
	virtual void				BindLocalCubemap( ITexture *pTexture )
	{
	}
	
	// pass in an ITexture (that is build with "rendertarget" "1") or
	// pass in NULL for the regular backbuffer.
	virtual void				SetRenderTarget( ITexture *pTexture )
	{
	}
	
	virtual ITexture *			GetRenderTarget( void )
	{
		return NULL;
	}
	
	virtual void				GetRenderTargetDimensions( int &width, int &height) const
	{
		width = 256;
		height = 256;
	}
	
	// Get the total number of materials in the system.  These aren't just the used
	// materials, but the complete collection.
	virtual int					GetNumMaterials( ) const
	{
		return m_pRealMaterialSystem->GetNumMaterials();
	}

	// Remove any materials from memory that aren't in use as determined
	// by the IMaterial's reference count.
	virtual void				UncacheUnusedMaterials( )
	{
		m_pRealMaterialSystem->UncacheUnusedMaterials();
	}

	// uncache all materials. .  good for forcing reload of materials.
	virtual void				UncacheAllMaterials( )
	{
		m_pRealMaterialSystem->UncacheAllMaterials();
	}

	// Load any materials into memory that are to be used as determined
	// by the IMaterial's reference count.
	virtual void				CacheUsedMaterials( )
	{
		m_pRealMaterialSystem->CacheUsedMaterials( );
	}
	
	// Force all textures to be reloaded from disk.
	virtual void				ReloadTextures( )
	{
	}
	
	// Allows us to override the depth buffer setting of a material
	virtual void	OverrideDepthEnable( bool bEnable, bool bEnableValue )
	{
	}

	//
	// lightmap allocation stuff
	//

	// To allocate lightmaps, sort the whole world by material twice.
	// The first time through, call AllocateLightmap for every surface.
	// that has a lightmap.
	// The second time through, call AllocateWhiteLightmap for every 
	// surface that expects to use shaders that expect lightmaps.
	virtual void				BeginLightmapAllocation( )
	{
	}
	// returns the sorting id for this surface
	virtual int 				AllocateLightmap( int width, int height, 
		                                          int offsetIntoLightmapPage[2],
												  IMaterial *pMaterial )
	{
		return 0;
	}
	// returns the sorting id for this surface
	virtual int					AllocateWhiteLightmap( IMaterial *pMaterial )
	{
		return 0;
	}
	virtual void				EndLightmapAllocation( )
	{
	}

	// lightmaps are in linear color space
	// lightmapPageID is returned by GetLightmapPageIDForSortID
	// lightmapSize and offsetIntoLightmapPage are returned by AllocateLightmap.
	// You should never call UpdateLightmap for a lightmap allocated through
	// AllocateWhiteLightmap.
	virtual void				UpdateLightmap( int lightmapPageID, int lightmapSize[2],
												int offsetIntoLightmapPage[2], 
												float *pFloatImage, float *pFloatImageBump1,
												float *pFloatImageBump2, float *pFloatImageBump3 )
	{
	}
	// Force the lightmaps updated with UpdateLightmap to be sent to the hardware.
	virtual void				FlushLightmaps( )
	{
	}

	// fixme: could just be an array of ints for lightmapPageIDs since the material
	// for a surface is already known.
	virtual int					GetNumSortIDs( )
	{
		return 0;
	}
//	virtual int					GetLightmapPageIDForSortID( int sortID ) = 0;
	virtual void				GetSortInfo( MaterialSystem_SortInfo_t *sortInfoArray )
	{
	}

	virtual void				BeginFrame( )
	{
	}
	virtual void				EndFrame( )
	{
	}
	
	// Bind a material is current for rendering.
	virtual void				Bind( IMaterial *material, void *proxyData = 0 )
	{
	}
	// Bind a lightmap page current for rendering.  You only have to 
	// do this for materials that require lightmaps.
	virtual void				BindLightmapPage( int lightmapPageID )
	{
	}

	// Used for WorldCraft lighting preview mode.
	virtual void				SetForceBindWhiteLightmaps( bool bForce )
	{
	}
	
	// inputs are between 0 and 1
	virtual void				DepthRange( float zNear, float zFar )
	{
	}

	virtual void				ClearBuffers( bool bClearColor, bool bClearDepth )
	{
	}

	// read to a unsigned char rgb image.
	virtual void				ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat )
	{
	}

	// Sets lighting
	virtual void				SetAmbientLight( float r, float g, float b )
	{
	}
	virtual void				SetLight( int lightNum, LightDesc_t& desc )
	{
	}

	// The faces of the cube are specified in the same order as cubemap textures
	virtual void				SetAmbientLightCube( Vector4D cube[6] )
	{
	}
	
	// Blit the backbuffer to the framebuffer texture
	virtual void				CopyRenderTargetToTexture( ITexture * )
	{
	}
	
	virtual void				SetFrameBufferCopyTexture( ITexture *pTexture )
	{
	}

	virtual	ITexture *          GetFrameBufferCopyTexture( void )
	{
		return NULL;
	}

	// do we need this?
	virtual void				Flush( bool flushHardware = false )
	{
	}

	//
	// end vertex array api
	//

	//
	// Debugging tools
	//
	virtual void				DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose )
	{
	}
	virtual void				DebugPrintUsedTextures( void )
	{
	}
	virtual void				ToggleSuppressMaterial( char const* pMaterialName )
	{
	}
	virtual void				ToggleDebugMaterial( char const* pMaterialName )
	{
	}

	//
	// Debugging text output
	//
	virtual void				DrawDebugText( int desiredLeft, int desiredTop, 
											   MaterialRect_t *pActualRect,									   
											   const char *fmt, ... )
	{
	}

	// matrix api
	virtual void				MatrixMode( MaterialMatrixMode_t matrixMode )
	{
	}
	virtual void				PushMatrix( void )
	{
	}
	virtual void				PopMatrix( void )
	{
	}
	virtual void				LoadMatrix( float * )
	{
	}
	virtual void				MultMatrix( float * )
	{
	}
	virtual void				MultMatrixLocal( float * )
	{
	}
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
	{
	}
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, VMatrix *pMatrix )
	{
	}
	virtual void				LoadIdentity( void )
	{
	}
	virtual void				Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
	{
	}
	virtual void				PerspectiveX( double fovx, double aspect, double zNear, double zFar )
	{
	}
	virtual void				PickMatrix( int x, int y, int width, int height )
	{
	}
	virtual void				Rotate( float angle, float x, float y, float z )
	{
	}
	virtual void				Translate( float x, float y, float z )
	{
	}
	virtual void				Scale( float x, float y, float z )
	{
	}
	// end matrix api

	// Sets/gets the viewport
	virtual void				Viewport( int x, int y, int width, int height )
	{
	}
	virtual void				GetViewport( int& x, int& y, int& width, int& height ) const
	{
	}

	// Point size
	virtual void				PointSize( float size )
	{
	}

	// The cull mode
	virtual void				CullMode( MaterialCullMode_t cullMode )
	{
	}

	// end matrix api

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
//	virtual void				ForceDepthFuncEquals( bool bEnable ) = 0;
//	virtual void				RenderZOnlyWithHeightClip( bool bEnable ) = 0;
	// This could easily be extended to a general user clip plane
	virtual void				SetHeightClipMode( enum MaterialHeightClipMode_t )
	{
	}
	// garymcthack : fog z is always used for heightclipz for now.
	virtual void				SetHeightClipZ( float z )
	{
	}
	
	// Fog methods...
	virtual void				FogMode( MaterialFogMode_t fogMode )
	{
	}
	MaterialFogMode_t			GetFogMode( void )
	{
		return MATERIAL_FOG_NONE;
	}
	virtual void				FogStart( float fStart )
	{
	}
	virtual void				FogEnd( float fEnd )
	{
	}
	virtual void				SetFogZ( float fogZ )
	{
	}

	virtual void				FogColor3f( float r, float g, float b )
	{
	}
	virtual void				FogColor3fv( float const* rgb )
	{
	}
	virtual void				FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
	{
	}
	virtual void				FogColor3ubv( unsigned char const* rgb )
	{
	}

	// Sets the number of bones for skinning
	virtual void				SetNumBoneWeights( int numBones ) 
	{
	}
	virtual IMaterialProxyFactory *GetMaterialProxyFactory()
	{
		return NULL;
	}
	
	// Read the page size of an existing lightmap by sort id (returned from AllocateLightmap())
	virtual void				GetLightmapPageSize( int lightmap, int *width, int *height ) const
	{
		m_pRealMaterialSystem->GetLightmapPageSize( lightmap, width, height );
	}
	
	/// FIXME: This stuff needs to be cleaned up and abstracted.
	// Stuff that gets exported to the launcher through the engine
	virtual void				SwapBuffers( )
	{
	}

	// Use this to spew information about the 3D layer 
	virtual void				SpewDriverInfo() const
	{
	}

	// Creates/destroys Mesh
	virtual IMesh* CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh )
	{
		IMesh *pMesh = ( CDummyMesh * )new CDummyMesh;
		return pMesh;
	}
	virtual IMesh* CreateStaticMesh( MaterialVertexFormat_t fmt, bool bSoftwareVertexShader )
	{
		IMesh *pMesh = ( CDummyMesh * )new CDummyMesh;
		return pMesh;
	}
	virtual void DestroyStaticMesh( IMesh* mesh )
	{
		delete mesh;
	}

	// Gets the dynamic mesh associated with the currently bound material
	// note that you've got to render the mesh before calling this function 
	// a second time. Clients should *not* call DestroyStaticMesh on the mesh 
	// returned by this call.
	// Use buffered = false if you want to not have the mesh be buffered,
	// but use it instead in the following pattern:
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		Draw partial
	//		Draw partial
	//		Draw partial
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		etc
	// Use Vertex or Index Override to supply a static vertex or index buffer
	// to use in place of the dynamic buffers.
	//
	// If you pass in a material in pAutoBind, it will automatically bind the
	// material. This can be helpful since you must bind the material you're
	// going to use BEFORE calling GetDynamicMesh.
	virtual IMesh* GetDynamicMesh( 
		bool buffered = true, 
		IMesh* pVertexOverride = 0,	
		IMesh* pIndexOverride = 0, 
		IMaterial *pAutoBind = 0 )
	{
		static CDummyMesh dummyMesh;
		return &dummyMesh;
	}
		
	// Selection mode methods
	virtual int  SelectionMode( bool selectionMode )
	{
		return 0;
	}
	virtual void SelectionBuffer( unsigned int* pBuffer, int size )
	{
	}
	virtual void ClearSelectionNames( )
	{
	}
	virtual void LoadSelectionName( int name )
	{
	}
	virtual void PushSelectionName( int name )
	{
	}
	virtual void PopSelectionName()
	{
	}
	
	// Installs a function to be called when we need to release vertex buffers + textures
	virtual void AddReleaseFunc( MaterialBufferReleaseFunc_t func )
	{
	}
	virtual void RemoveReleaseFunc( MaterialBufferReleaseFunc_t func )
	{
	}

	// Installs a function to be called when we need to restore vertex buffers
	virtual void AddRestoreFunc( MaterialBufferRestoreFunc_t func )
	{
	}
	virtual void RemoveRestoreFunc( MaterialBufferRestoreFunc_t func )
	{
	}

	// Stuff for probing properties of shaders.
	virtual int					GetNumShaders( void ) const 
	{
		return 0;
	}
	virtual const char *		GetShaderName( int shaderID ) const
	{
		return NULL;
	}
	virtual int					GetNumShaderParams( int shaderID ) const
	{
		return 0;
	}
	virtual const char *		GetShaderParamName( int shaderID, int paramID ) const
	{
		return NULL;
	}
	virtual const char *		GetShaderParamHelp( int shaderID, int paramID ) const
	{
		return NULL;
	}
	virtual ShaderParamType_t	GetShaderParamType( int shaderID, int paramID ) const
	{
		return ( enum ShaderParamType_t )0;
	}
	virtual const char *		GetShaderParamDefault( int shaderID, int paramID ) const
	{
		return NULL;
	}

	// Reloads materials
	virtual void	ReloadMaterials( const char *pSubString = NULL )
	{
	}

	virtual void	ResetMaterialLightmapPageInfo()
	{
	}

	// Creates a render target
	// If depth == true, a depth buffer is also allocated. If not, then
	// the screen's depth buffer is used.
	virtual ITexture*	CreateRenderTargetTexture( int w, int h, ImageFormat format, bool depth = false )
	{
		return NULL;
	}

	virtual ITexture *CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags )
	{
		return NULL;
	}

	// Methods that use VMatrix
	virtual void		LoadMatrix( const VMatrix& matrix )
	{
	}

	// Sets the Clear Color for ClearBuffer....
	virtual void		ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
	{
	}
	virtual void		ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
	{
	}
	virtual void SetInStubMode( bool b )
	{
	}

	// Create new materials
	virtual IMaterial	*CreateMaterial()
	{
		return NULL;
	}

	void GetBackBufferDimensions( int &w, int &h ) const
	{
		w = 1024;
		h = 768;
	}
	
	ImageFormat GetBackBufferFormat( void ) const
	{
		return IMAGE_FORMAT_RGBA8888;
	}
	
	// FIXME: This is a hack required for NVidia, can they fix in drivers?
	virtual void	DrawScreenSpaceQuad( IMaterial* pMaterial ) {}

	// FIXME: Test interface
	virtual bool Connect( CreateInterfaceFn factory ) { return true; }
	virtual void Disconnect() {}
	virtual void *QueryInterface( const char *pInterfaceName ) { return NULL; }
	virtual InitReturnVal_t Init() { return INIT_OK; }
	virtual void SetShaderAPI( const char *pShaderAPIDLL ) {}
	virtual void SetAdapter( int nAdapter, int nFlags ) {}

	// Release temporary HW memory...
	virtual void ReleaseTempTextureMemory() {}
	virtual void Get3DDriverInfo( Material3DDriverInfo_t &info ) const {}
	virtual ITexture* CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat format, bool depth = false, bool bClampTexCoords = true, bool bAutoMipMap = false )
	{
		return NULL;
	}
	virtual void SyncToken( const char *pToken ) {}
};


static CDummyMaterialSystem g_DummyMaterialSystem;

CDummyMaterialSystem* GetStubMaterialSystem()
{
	return &g_DummyMaterialSystem;
}


// ---------------------------------------------------------------------------------------- //
// CMatStubHandler implementation.
// ---------------------------------------------------------------------------------------- //

CMatStubHandler::CMatStubHandler()
{
	if ( mat_stub.GetInt() )
	{
		m_pOldMaterialSystem = materials;

		// Replace all material system pointers with the stub.
		GetStubMaterialSystem()->SetRealMaterialSystem( materials );
		materials->SetInStubMode( true );
		materials = GetStubMaterialSystem();
		engine->Mat_Stub( materials );
	}
	else
	{
		m_pOldMaterialSystem = 0;
	}
}


CMatStubHandler::~CMatStubHandler()
{
	End();
}


void CMatStubHandler::End()
{
	// Put back the original material system pointer.
	if ( m_pOldMaterialSystem )
	{
		materials = m_pOldMaterialSystem;
		materials->SetInStubMode( false );
		engine->Mat_Stub( materials );
		m_pOldMaterialSystem = 0;
		if( gl_clear.GetBool() )
		{
			materials->ClearBuffers( true, true );
		}
	}
}


bool IsMatStubEnabled()
{
	return mat_stub.GetBool();
}
