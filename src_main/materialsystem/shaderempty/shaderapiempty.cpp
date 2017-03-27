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
// The empty implementation of the shader API
//=============================================================================

#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "IHardwareConfigInternal.h"
#include "shadersystem.h"
#include "ishaderutil.h"
#include "shaderapi.h"
#include "materialsystem/imesh.h"
#include "tier0/dbg.h"


//-----------------------------------------------------------------------------
// The empty mesh
//-----------------------------------------------------------------------------
class CEmptyMesh : public IMesh
{
public:
	CEmptyMesh();
	virtual ~CEmptyMesh();

	// Locks/ unlocks the vertex buffer, returns the index of the first vertex
	// numIndices of -1 means don't lock the index buffer...
	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyEnd( );

	// returns the # of vertices (static meshes only)
	int  NumVertices() const;

	// Helper methods to create various standard index buffer types
	void GenerateSequentialIndexBuffer( unsigned short* pIndexMemory, 
										int numIndices, int firstVertex );
	void GenerateQuadIndexBuffer( unsigned short* pIndexMemory, 
									int numIndices, int firstVertex );
	void GeneratePolygonIndexBuffer( unsigned short* pIndexMemory, 
									int numIndices, int firstVertex );
	void GenerateLineStripIndexBuffer( unsigned short* pIndexMemory, 
									int numIndices, int firstVertex );
	void GenerateLineLoopIndexBuffer( unsigned short* pIndexMemory, 
									int numIndices, int firstVertex );

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	 
	// Draws the entire mesh
	void Draw(int firstIndex, int numIndices);

	void Draw(CPrimList *pPrims, int nPrims);

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	// Spews the mesh data
	void Spew( int numVerts, int numIndices, MeshDesc_t const& desc );

	void ValidateData( int numVerts, int numIndices, MeshDesc_t const& desc );

	// gets the associated material
	IMaterial* GetMaterial();

	void SetSoftwareVertexShader( SoftwareVertexShader_t shader );
	void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder );

	void SetColorMesh( IMesh *pColorMesh )
	{
	}

private:
	enum
	{
		VERTEX_BUFFER_SIZE = 1024 * 1024
	};

	unsigned char* m_pVertexMemory;
};

//-----------------------------------------------------------------------------
// The empty shader shadow
//-----------------------------------------------------------------------------

class CShaderShadowEmpty : public IShaderShadow
{
public:
	CShaderShadowEmpty();
	virtual ~CShaderShadowEmpty();

	// Sets the default *shadow* state
	void SetDefaultState();

	// Methods related to depth buffering
	void DepthFunc( ShaderDepthFunc_t depthFunc );
	void EnableDepthWrites( bool bEnable );
	void EnableDepthTest( bool bEnable );
	void EnablePolyOffset( bool bEnable );

	// Suppresses/activates color writing 
	void EnableColorWrites( bool bEnable );
	void EnableAlphaWrites( bool bEnable );

	// Methods related to alpha blending
	void EnableBlending( bool bEnable );
	void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Alpha testing
	void EnableAlphaTest( bool bEnable );
	void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ );

	// Wireframe/filled polygons
	void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode );

	// Back face culling
	void EnableCulling( bool bEnable );
	
	// constant color + transparency
	void EnableConstantColor( bool bEnable );

	// Indicates we're preprocessing vertex data
	void EnableVertexDataPreprocess( bool bEnable );

	// Indicates the vertex format for use with a vertex shader
	// The flags to pass in here come from the VertexFormatFlags_t enum
	// If pTexCoordDimensions is *not* specified, we assume all coordinates
	// are 2-dimensional
	void VertexShaderVertexFormat( unsigned int flags, 
			int numTexCoords, int* pTexCoordDimensions, int numBoneWeights,
			int userDataSize );
	
	// Indicates we're going to light the model
	void EnableLighting( bool bEnable );
	void EnableSpecular( bool bEnable );

	// Indicates we're going to be using the ambient cube on stage0
	void EnableAmbientLightCubeOnStage0( bool bEnable );

	// vertex blending
	void EnableVertexBlend( bool bEnable );

	// per texture unit stuff
	void OverbrightValue( TextureStage_t stage, float value );
	void EnableTexture( TextureStage_t stage, bool bEnable );
	void EnableTexGen( TextureStage_t stage, bool bEnable );
	void TexGen( TextureStage_t stage, ShaderTexGenParam_t param );

	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
	void EnableCustomPixelPipe( bool bEnable );
	void CustomTextureStages( int stageCount );
	void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
		ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 );

	void VertexShaderOverbrightValue( float value );

	// indicates what per-vertex data we're providing
	void DrawFlags( unsigned int drawFlags );

	// A simpler method of dealing with alpha modulation
	void EnableAlphaPipe( bool bEnable );
	void EnableConstantAlpha( bool bEnable );
	void EnableVertexAlpha( bool bEnable );
	void EnableTextureAlpha( TextureStage_t stage, bool bEnable );

	// GR - Separate alpha blending
	void EnableBlendingSeparateAlpha( bool bEnable );
	void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Sets the vertex and pixel shaders
	void SetVertexShader( const char *pFileName, int vshIndex );
	void SetPixelShader( const char *pFileName, int pshIndex );

	// Convert from linear to gamma color space on writes to frame buffer.
	void EnableSRGBWrite( bool bEnable )
	{
	}

	bool m_IsTranslucent;
	bool m_IsAlphaTested;
	bool m_UsesAmbientCube;
	bool m_bUsesVertexAndPixelShaders;
};


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader API
//-----------------------------------------------------------------------------
class CShaderAPIEmpty : public IShaderAPI, public IHardwareConfigInternal
{
public:
	// constructor, destructor
	CShaderAPIEmpty( );
	virtual ~CShaderAPIEmpty();

	// Initialization, shutdown
	bool Init( IShaderUtil* pShaderUtil, IBaseFileSystem* pFileSystem, int nAdapter, int nFlags );
	void Shutdown( );

	// Gets the number of adapters...
	int	 GetDisplayAdapterCount() const;

	// Returns info about each adapter
	void GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const;

	// Returns the number of modes
	int	 GetModeCount( int adapter ) const;

	// Returns mode information..
	void GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const;

	// Returns the mode info for the current display device
	void GetDisplayMode( MaterialVideoMode_t& info ) const;

	void Get3DDriverInfo( Material3DDriverInfo_t &info ) const;

	// Used to clear the transition table when we know it's become invalid.
	void ClearSnapshots();

	// Returns true if the material system should try to use 16 bit textures.
	bool Prefer16BitTextures( ) const;
	
	// Sets the mode...
	bool SetMode( void* hwnd, MaterialVideoMode_t const& mode, int flags, int nSampleCount );

	void GetWindowSize( int &width, int &height ) const;

	// Creates/ destroys a child window
	bool AddView( void* hwnd );
	void RemoveView( void* hwnd );

	// Activates a view
	void SetView( void* hwnd );

	// Sets the default *dynamic* state
	void SetDefaultState( );

	// Returns the snapshot id for the shader state
	StateSnapshot_t	 TakeSnapshot( );

	// Returns true if the state snapshot is transparent
	bool IsTranslucent( StateSnapshot_t id ) const;
	bool IsAlphaTested( StateSnapshot_t id ) const;
	bool UsesVertexAndPixelShaders( StateSnapshot_t id ) const;
	bool UsesAmbientCube( StateSnapshot_t id ) const;

	// Gets the vertex format for a set of snapshot ids
	VertexFormat_t ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const;

	// Gets the vertex format for a set of snapshot ids
	VertexFormat_t ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const;

	// Begins a rendering pass that uses a state snapshot
	void BeginPass( StateSnapshot_t snapshot  );

	// Uses a state snapshot
	void UseSnapshot( StateSnapshot_t snapshot );

	// Use this to get the mesh builder that allows us to modify vertex data
	CMeshBuilder* GetVertexModifyBuilder();

	// Sets the color to modulate by
	void Color3f( float r, float g, float b );
	void Color3fv( float const* pColor );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( float const* pColor );

	// Faster versions of color
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const* rgb );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const* rgba );

	// Sets the lights
	void SetLight( int lightNum, LightDesc_t& desc );
	void SetAmbientLight( float r, float g, float b );
	void SetAmbientLightCube( Vector4D cube[6] );

	// Get the lights
	int GetMaxLights( void ) const;
	const LightDesc_t& GetLight( int lightNum ) const;

	// Render state for the ambient light cube (vertex shaders)
	void SetVertexShaderStateAmbientLightCube();

	void SetSkinningMatrices();

	// Render state for the ambient light cube (pixel shaders)
	void BindAmbientLightCubeToStage0( );

	// Lightmap texture binding
	void BindLightmap( TextureStage_t stage );
	void BindLightmapAlpha( TextureStage_t stage )
	{
	}
	void BindBumpLightmap( TextureStage_t stage );
	void BindWhite( TextureStage_t stage );
	void BindBlack( TextureStage_t stage );
	void BindGrey( TextureStage_t stage );
	void BindSyncTexture( TextureStage_t stage, int texture );
	void BindFBTexture( TextureStage_t stage );
	void CopyRenderTargetToTexture( int texID );


	// Special system flat normal map binding.
	void BindFlatNormalMap( TextureStage_t stage );
	void BindNormalizationCubeMap( TextureStage_t stage );

	// Set the number of bone weights
	void SetNumBoneWeights( int numBones );

	// Flushes any primitives that are buffered
	void FlushBufferedPrimitives();

	// Creates/destroys Mesh
	IMesh* CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh );
	IMesh* CreateStaticMesh( VertexFormat_t fmt, bool bSoftwareVertexShader );
	void DestroyStaticMesh( IMesh* mesh );

	// Gets the dynamic mesh; note that you've got to render the mesh
	// before calling this function a second time. Clients should *not*
	// call DestroyStaticMesh on the mesh returned by this call.
	IMesh* GetDynamicMesh( IMaterial* pMaterial, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride );

	// Renders a single pass of a material
	void RenderPass( );

	// Page flip
	void SwapBuffers( );

	// stuff related to matrix stacks
	void MatrixMode( MaterialMatrixMode_t matrixMode );
	void PushMatrix();
	void PopMatrix();
	void LoadMatrix( float *m );
	void MultMatrix( float *m );
	void MultMatrixLocal( float *m );
	void GetMatrix( MaterialMatrixMode_t matrixMode, float *dst );
	void LoadIdentity( void );
	void LoadCameraToWorld( void );
	void Ortho( double left, double top, double right, double bottom, double zNear, double zFar );
	void PerspectiveX( double fovx, double aspect, double zNear, double zFar );
	void PickMatrix( int x, int y, int width, int height );
	void Rotate( float angle, float x, float y, float z );
	void Translate( float x, float y, float z );
	void Scale( float x, float y, float z );
	void ScaleXY( float x, float y );

	void Viewport( int x, int y, int width, int height );
	void GetViewport( int& x, int& y, int& width, int& height ) const;

	// Fog methods...
	void FogMode( MaterialFogMode_t fogMode );
	void FogStart( float fStart );
	void FogEnd( float fEnd );
	void SetFogZ( float fogZ );
	float GetFogStart( void ) const;
	float GetFogEnd( void ) const;
	void FogColor3f( float r, float g, float b );
	void FogColor3fv( float const* rgb );
	void FogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void FogColor3ubv( unsigned char const* rgb );

	virtual void SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void GetSceneFogColor( unsigned char *rgb );
	virtual void SceneFogMode( MaterialFogMode_t fogMode );
	virtual MaterialFogMode_t GetSceneFogMode( );

	void SetHeightClipZ( float z );
	void SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode );

	void SetClipPlane( int index, const float *pPlane );
	void EnableClipPlane( int index, bool bEnable );
	
	void DestroyVertexBuffers();

	// Sets the vertex and pixel shaders
	void SetVertexShaderIndex( int vshIndex );
	void SetPixelShaderIndex( int pshIndex );

	// Sets the constant register for vertex and pixel shaders
	void SetVertexShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false );
	void SetPixelShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false );

	// Cull mode
	void CullMode( MaterialCullMode_t cullMode );

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
	void ForceDepthFuncEquals( bool bEnable );

	// Forces Z buffering on or off
	void OverrideDepthEnable( bool bEnable, bool bDepthEnable );

	// Sets the shade mode
	void ShadeMode( ShaderShadeMode_t mode );

	// Binds a particular material to render with
	void Bind( IMaterial* pMaterial );

	// Returns the nearest supported format
	ImageFormat GetNearestSupportedFormat( ImageFormat fmt ) const;
 	ImageFormat GetNearestRenderTargetFormat( ImageFormat fmt ) const;

	void ClearColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );

	// Sets the texture state
	void BindTexture( TextureStage_t stage, unsigned int textureNum );

	void SetRenderTarget( unsigned int textureNum, unsigned int ztexture );

	ImageFormat GetBackBufferFormat() const;
	void GetBackBufferDimensions( int& width, int& height ) const;

	// Indicates we're going to be modifying this texture
	// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
	// all use the texture specified by this function.
	void ModifyTexture( unsigned int textureNum );

	// Texture management methods
	void TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int width, int height, 
							 ImageFormat srcFormat, void *imageData );
	void TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int width, int height,
							 ImageFormat srcFormat, int srcStride, void *imageData );

	bool TexLock( int level, int cubeFaceID, int xOffset, int yOffset, 
									int width, int height, CPixelWriter& writer );
	void TexUnlock( );
	
	// These are bound to the texture, not the texture environment
	void TexMinFilter( ShaderTexFilterMode_t texFilterMode );
	void TexMagFilter( ShaderTexFilterMode_t texFilterMode );
	void TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode );
	void TexSetPriority( int priority );	
	void SetLightmapTextureIdRange( int firstId, int lastId );

	void CreateTexture( unsigned int textureNum, int width, int height,
		ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags );
	void CreateDepthTexture( unsigned int textureNum, ImageFormat renderFormat, int width, int height );
	void DeleteTexture( unsigned int textureNum );
	bool IsTexture( unsigned int textureNum );
	bool IsTextureResident( unsigned int textureNum );

	// stuff that isn't to be used from within a shader
	void DepthRange( float zNear, float zFar );
	void ClearBuffers( bool bClearColor, bool bClearDepth, int renderTargetWidth, int renderTargetHeight );
	void ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat );

	// Point size
	void PointSize( float size );

	// Selection mode methods
	int SelectionMode( bool selectionMode );
	void SelectionBuffer( unsigned int* pBuffer, int size );
	void ClearSelectionNames( );
	void LoadSelectionName( int name );
	void PushSelectionName( int name );
	void PopSelectionName();

	void FlushHardware();
	void ResetRenderState();

	// Can we download textures?
	virtual bool CanDownloadTextures() const;

	// Are we using graphics?
	virtual bool IsUsingGraphics() const;

	// Board-independent calls, here to unify how shaders set state
	// Implementations should chain back to IShaderUtil->BindTexture(), etc.

	// Use this to spew information about the 3D layer 
	void SpewDriverInfo() const;

	// Use this to begin and end the frame
	void BeginFrame();
	void EndFrame();

	// returns current time
	double CurrentTime() const;

	// Get the current camera position in world space.
	void GetWorldSpaceCameraPosition( float * pPos ) const;

	// Members of IMaterialSystemHardwareConfig
	bool HasAlphaBuffer() const;
	bool HasStencilBuffer() const;
	bool HasARBMultitexture() const;
	bool HasNVRegisterCombiners() const;
	bool HasTextureEnvCombine() const;
	int	 GetFrameBufferColorDepth() const;
	int  GetNumTextureUnits() const;
	bool HasIteratorsPerTextureUnit() const;
	bool HasSetDeviceGammaRamp() const;
	bool SupportsCompressedTextures() const;
	bool SupportsVertexAndPixelShaders() const;
	bool SupportsPixelShaders_1_4() const;
	bool SupportsPixelShaders_2_0() const;
	bool SupportsVertexShaders_2_0() const;
	bool SupportsPartialTextureDownload() const;
	bool SupportsStateSnapshotting() const;
	int  MaximumAnisotropicLevel() const;
	int  MaxTextureWidth() const;
	int  MaxTextureHeight() const;
	int  MaxTextureAspectRatio() const;
	int  GetDXSupportLevel() const;
	const char *GetShaderDLLName() const
	{
		return "UNKNOWN";
	}
	int	 TextureMemorySize() const;
	bool SupportsOverbright() const;
	bool SupportsCubeMaps() const;
	bool SupportsMipmappedCubemaps() const;
	bool SupportsNonPow2Textures() const;
	int  GetNumTextureStages() const;
	int	 NumVertexShaderConstants() const;
	int	 NumPixelShaderConstants() const;
	int	 MaxNumLights() const;
	bool SupportsHardwareLighting() const;
	int	 MaxBlendMatrices() const;
	int	 MaxBlendMatrixIndices() const;
	int	 MaxVertexShaderBlendMatrices() const;
	int	 MaxUserClipPlanes() const;
	bool UseFastClipping() const
	{
		return false;
	}
	bool UseFastZReject() const
	{
		return false;
	}
	const char *GetHWSpecificShaderDLLName() const;
	bool NeedsAAClamp() const
	{
		return false;
	}
	bool SupportsSpheremapping() const;
	bool HasFastZReject() const;

	bool ReadPixelsFromFrontBuffer() const;
	bool PreferDynamicTextures() const;
	bool HasProjectedBumpEnv() const;
	void SetSoftwareVertexShader( SoftwareVertexShader_t shader );
	void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder );
	void ForceHardwareSync( void );
	void DrawDebugText( int desiredLeft, int desiredTop, 
						MaterialRect_t *pActualRect,
						const char *pText );
	
	int GetCurrentNumBones( void ) const;
	int GetCurrentLightCombo( void ) const;
	int GetCurrentFogType( void ) const;

	void RecordString( const char *pStr );

	void EvictManagedResources();

	void SetTextureTransformDimension( int textureStage, int dimension, bool projected );
	void DisableTextureTransform( int textureStage )
	{
	}
	void SetBumpEnvMatrix( int textureStage, float m00, float m01, float m10, float m11 );

	// Gets the lightmap dimensions
	virtual void GetLightmapDimensions( int *w, int *h );

	// Adds/removes precompiled shader dictionaries
	virtual ShaderDLL_t AddShaderDLL( );
	virtual void RemoveShaderDLL( ShaderDLL_t hShaderDLL );
	virtual void AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict );
	virtual void SetShaderDLL( ShaderDLL_t hShaderDLL );
	virtual void SyncToken( const char *pToken );

	// Level of anisotropic filtering
	virtual void SetAnisotropicLevel( int nAnisotropyLevel );

	bool SupportsHDR() const
	{
		return true;
	}
	virtual void EnableSRGBRead( TextureStage_t stage, bool bEnable )
	{
	}
	virtual bool NeedsATICentroidHack() const
	{
		return false;
	}
private:
	enum
	{
		TRANSLUCENT = 0x1,
		ALPHATESTED = 0x2,
		AMBIENT_CUBE = 0x4,
		VERTEX_AND_PIXEL_SHADERS = 0x8,
	};

	CEmptyMesh m_Mesh;
};


//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------

static CShaderAPIEmpty g_ShaderAPIEmpty;
static CShaderShadowEmpty g_ShaderShadow;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IShaderAPI, 
									SHADERAPI_INTERFACE_VERSION, g_ShaderAPIEmpty )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderShadowEmpty, IShaderShadow, 
								SHADERSHADOW_INTERFACE_VERSION, g_ShaderShadow )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IMaterialSystemHardwareConfig, 
				MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, g_ShaderAPIEmpty )


//-----------------------------------------------------------------------------
// The main GL Shader util interface
//-----------------------------------------------------------------------------
IShaderUtil* g_pShaderUtil;

IShaderUtil* ShaderUtil()
{
	return g_pShaderUtil;
}
  

//-----------------------------------------------------------------------------
//
// The empty mesh...
//
//-----------------------------------------------------------------------------

CEmptyMesh::CEmptyMesh()
{
	m_pVertexMemory = new unsigned char[VERTEX_BUFFER_SIZE];
}

CEmptyMesh::~CEmptyMesh()
{
	delete[] m_pVertexMemory;
}

// Locks/ unlocks the vertex buffer, returns the index of the first vertex
// numIndices of -1 means don't lock the index buffer...
void CEmptyMesh::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	// Who cares about the data?
	desc.m_pPosition = (float*)m_pVertexMemory;
	desc.m_pNormal = (float*)m_pVertexMemory;
	desc.m_pColor = m_pVertexMemory;
	int i;
	for ( i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i)
		desc.m_pTexCoord[i] = (float*)m_pVertexMemory;
	desc.m_pIndices = (unsigned short*)m_pVertexMemory;

	desc.m_pBoneWeight = (float*)m_pVertexMemory;
	desc.m_pBoneMatrixIndex = (unsigned char*)m_pVertexMemory;
	desc.m_pTangentS = (float*)m_pVertexMemory;
	desc.m_pTangentT = (float*)m_pVertexMemory;
	desc.m_pUserData = (float*)m_pVertexMemory;
	desc.m_NumBoneWeights = 2;

	desc.m_VertexSize_Position = 0;
	desc.m_VertexSize_BoneWeight = 0;
	desc.m_VertexSize_BoneMatrixIndex = 0;
	desc.m_VertexSize_Normal = 0;
	desc.m_VertexSize_Color = 0;
	for( i=0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		desc.m_VertexSize_TexCoord[i] = 0;
	desc.m_VertexSize_TangentS = 0;
	desc.m_VertexSize_TangentT = 0;
	desc.m_VertexSize_UserData = 0;
	desc.m_ActualVertexSize = 0;	// Size of the vertices.. Some of the m_VertexSize_ elements above

	desc.m_FirstVertex = 0;
}

void CEmptyMesh::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
}

void CEmptyMesh::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	// Who cares about the data?
	desc.m_pPosition = (float*)m_pVertexMemory;
	desc.m_pNormal = (float*)m_pVertexMemory;
	desc.m_pColor = m_pVertexMemory;
	int i;
	for ( i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i)
		desc.m_pTexCoord[i] = (float*)m_pVertexMemory;
	desc.m_pIndices = (unsigned short*)m_pVertexMemory;

	desc.m_pBoneWeight = (float*)m_pVertexMemory;
	desc.m_pBoneMatrixIndex = (unsigned char*)m_pVertexMemory;
	desc.m_pTangentS = (float*)m_pVertexMemory;
	desc.m_pTangentT = (float*)m_pVertexMemory;
	desc.m_pUserData = (float*)m_pVertexMemory;
	desc.m_NumBoneWeights = 2;

	desc.m_VertexSize_Position = 0;
	desc.m_VertexSize_BoneWeight = 0;
	desc.m_VertexSize_BoneMatrixIndex = 0;
	desc.m_VertexSize_Normal = 0;
	desc.m_VertexSize_Color = 0;
	for( i=0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		desc.m_VertexSize_TexCoord[i] = 0;
	desc.m_VertexSize_TangentS = 0;
	desc.m_VertexSize_TangentT = 0;
	desc.m_VertexSize_UserData = 0;
	desc.m_ActualVertexSize = 0;	// Size of the vertices.. Some of the m_VertexSize_ elements above

	desc.m_FirstVertex = 0;
}

void CEmptyMesh::ModifyEnd( )
{
}

// returns the # of vertices (static meshes only)
int CEmptyMesh::NumVertices() const
{
	return 0;
}

// Helper methods to create various standard index buffer types
void CEmptyMesh::GenerateSequentialIndexBuffer( unsigned short* pIndexMemory, 
											int numIndices, int firstVertex )
{
}

void CEmptyMesh::GenerateQuadIndexBuffer( unsigned short* pIndexMemory, 
										int numIndices, int firstVertex )
{
}

void CEmptyMesh::GeneratePolygonIndexBuffer( unsigned short* pIndexMemory, 
								int numIndices, int firstVertex )
{
}

void CEmptyMesh::GenerateLineStripIndexBuffer( unsigned short* pIndexMemory, 
								int numIndices, int firstVertex )
{
}

void CEmptyMesh::GenerateLineLoopIndexBuffer( unsigned short* pIndexMemory, 
								int numIndices, int firstVertex )
{
}

// Sets the primitive type
void CEmptyMesh::SetPrimitiveType( MaterialPrimitiveType_t type )
{
}

// Draws the entire mesh
void CEmptyMesh::Draw( int firstIndex, int numIndices )
{
}

void CEmptyMesh::Draw(CPrimList *pPrims, int nPrims)
{
}

// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
void CEmptyMesh::CopyToMeshBuilder( 
	int iStartVert,		// Which vertices to copy.
	int nVerts, 
	int iStartIndex,	// Which indices to copy.
	int nIndices, 
	int indexOffset,	// This is added to each index.
	CMeshBuilder &builder )
{
}

// Spews the mesh data
void CEmptyMesh::Spew( int numVerts, int numIndices, MeshDesc_t const& desc )
{
}

void CEmptyMesh::ValidateData( int numVerts, int numIndices, MeshDesc_t const& desc )
{
}

// gets the associated material
IMaterial* CEmptyMesh::GetMaterial()
{
	// umm. this don't work none
	Assert(0);
	return 0;
}

void CEmptyMesh::SetSoftwareVertexShader( SoftwareVertexShader_t shader )
{
}

void CEmptyMesh::CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
{
}


//-----------------------------------------------------------------------------
// The shader shadow interface
//-----------------------------------------------------------------------------
CShaderShadowEmpty::CShaderShadowEmpty()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bUsesVertexAndPixelShaders = false;
}

CShaderShadowEmpty::~CShaderShadowEmpty()
{
}

// Sets the default *shadow* state
void CShaderShadowEmpty::SetDefaultState()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_UsesAmbientCube = false;
	m_bUsesVertexAndPixelShaders = false;
}

// Methods related to depth buffering
void CShaderShadowEmpty::DepthFunc( ShaderDepthFunc_t depthFunc )
{
}

void CShaderShadowEmpty::EnableDepthWrites( bool bEnable )
{
}

void CShaderShadowEmpty::EnableDepthTest( bool bEnable )
{
}

void CShaderShadowEmpty::EnablePolyOffset( bool bEnable )
{
}


// Suppresses/activates color writing 
void CShaderShadowEmpty::EnableColorWrites( bool bEnable )
{
}

// Suppresses/activates alpha writing 
void CShaderShadowEmpty::EnableAlphaWrites( bool bEnable )
{
}

// Methods related to alpha blending
void CShaderShadowEmpty::EnableBlending( bool bEnable )
{
	m_IsTranslucent = bEnable;
}

void CShaderShadowEmpty::BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
}

// A simpler method of dealing with alpha modulation
void CShaderShadowEmpty::EnableAlphaPipe( bool bEnable )
{
}

void CShaderShadowEmpty::EnableConstantAlpha( bool bEnable )
{
}

void CShaderShadowEmpty::EnableVertexAlpha( bool bEnable )
{
}

void CShaderShadowEmpty::EnableTextureAlpha( TextureStage_t stage, bool bEnable )
{
}


// Alpha testing
void CShaderShadowEmpty::EnableAlphaTest( bool bEnable )
{
	m_IsAlphaTested = bEnable;
}

void CShaderShadowEmpty::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ )
{
}


// Wireframe/filled polygons
void CShaderShadowEmpty::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{
}


// Back face culling
void CShaderShadowEmpty::EnableCulling( bool bEnable )
{
}


// constant color + transparency
void CShaderShadowEmpty::EnableConstantColor( bool bEnable )
{
}


// Indicates we're preprocessing vertex data
void CShaderShadowEmpty::EnableVertexDataPreprocess( bool bEnable )
{
}

// Indicates the vertex format for use with a vertex shader
// The flags to pass in here come from the VertexFormatFlags_t enum
// If pTexCoordDimensions is *not* specified, we assume all coordinates
// are 2-dimensional
void CShaderShadowEmpty::VertexShaderVertexFormat( unsigned int flags, 
		int numTexCoords, int* pTexCoordDimensions, int numBoneWeights,
		int userDataSize )
{
}

// Indicates we're going to light the model
void CShaderShadowEmpty::EnableLighting( bool bEnable )
{
}

void CShaderShadowEmpty::EnableSpecular( bool bEnable )
{
}

// Indicates we're going to be using the ambient cube on stage0
void CShaderShadowEmpty::EnableAmbientLightCubeOnStage0( bool bEnable )
{
	m_UsesAmbientCube = bEnable;
}

// Activate/deactivate skinning
void CShaderShadowEmpty::EnableVertexBlend( bool bEnable )
{
}

// per texture unit stuff
void CShaderShadowEmpty::OverbrightValue( TextureStage_t stage, float value )
{
}

void CShaderShadowEmpty::EnableTexture( TextureStage_t stage, bool bEnable )
{
}

void CShaderShadowEmpty::EnableCustomPixelPipe( bool bEnable )
{
}

void CShaderShadowEmpty::CustomTextureStages( int stageCount )
{
}

void CShaderShadowEmpty::CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
	ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 )
{
}

void CShaderShadowEmpty::EnableTexGen( TextureStage_t stage, bool bEnable )
{
}

void CShaderShadowEmpty::TexGen( TextureStage_t stage, ShaderTexGenParam_t param )
{
}

void CShaderShadowEmpty::VertexShaderOverbrightValue( float value )
{
}

// Sets the vertex and pixel shaders
void CShaderShadowEmpty::SetVertexShader( const char *pShaderName, int vshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowEmpty::EnableBlendingSeparateAlpha( bool bEnable )
{
}
void CShaderShadowEmpty::SetPixelShader( const char *pShaderName, int pshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowEmpty::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
}
// indicates what per-vertex data we're providing
void CShaderShadowEmpty::DrawFlags( unsigned int drawFlags )
{
}



//-----------------------------------------------------------------------------
//
// Shader API Empty
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

CShaderAPIEmpty::CShaderAPIEmpty() 
{
}

CShaderAPIEmpty::~CShaderAPIEmpty()
{
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------

bool CShaderAPIEmpty::Init( IShaderUtil* pShaderUtil, IBaseFileSystem* pFileSystem, int nAdapter, int nFlags )
{
	// So others can access it
	g_pShaderUtil = pShaderUtil;

	return true;
}


//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------

void CShaderAPIEmpty::Shutdown( )
{
}


// Gets the number of adapters...
int	 CShaderAPIEmpty::GetDisplayAdapterCount() const
{
	return 0;
}

// Returns info about each adapter
void CShaderAPIEmpty::GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const
{
}

// Returns the number of modes
int	 CShaderAPIEmpty::GetModeCount( int adapter ) const
{
	return 0;
}

// Returns mode information..
void CShaderAPIEmpty::GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const
{
}

// Returns the mode info for the current display device
void CShaderAPIEmpty::GetDisplayMode( MaterialVideoMode_t& info ) const
{
}

void CShaderAPIEmpty::Get3DDriverInfo( Material3DDriverInfo_t &info ) const
{
}

bool CShaderAPIEmpty::Prefer16BitTextures( ) const
{
	return true;
}

// Can we download textures?
bool CShaderAPIEmpty::CanDownloadTextures() const
{
	return false;
}

// Are we using graphics?
bool CShaderAPIEmpty::IsUsingGraphics() const
{
	return false;
}


// Used to clear the transition table when we know it's become invalid.
void CShaderAPIEmpty::ClearSnapshots()
{
}

// Sets the mode...
bool CShaderAPIEmpty::SetMode( void* hwnd, MaterialVideoMode_t const& mode, int flags, int nSampleCount )
{
	return true;
}

void CShaderAPIEmpty::GetWindowSize( int &width, int &height ) const
{
	width = 0;
	height = 0;
}

// Creates/ destroys a child window
bool CShaderAPIEmpty::AddView( void* hwnd )
{
	return true;
}

void CShaderAPIEmpty::RemoveView( void* hwnd )
{
}

// Activates a view
void CShaderAPIEmpty::SetView( void* hwnd )
{
}

// Members of IMaterialSystemHardwareConfig
bool CShaderAPIEmpty::HasAlphaBuffer() const
{
	return false;
}

bool CShaderAPIEmpty::HasStencilBuffer() const
{
	return false;
}

bool CShaderAPIEmpty::HasARBMultitexture() const
{
	return true;
}

bool CShaderAPIEmpty::HasNVRegisterCombiners() const
{
	return false;
}

bool CShaderAPIEmpty::HasTextureEnvCombine() const
{
	return false;
}

int	 CShaderAPIEmpty::GetFrameBufferColorDepth() const
{
	return 0;
}

int  CShaderAPIEmpty::GetNumTextureUnits() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 60))
		return 1;
	if (( ShaderUtil()->GetConfig().dxSupportLevel >= 60 ) && ( ShaderUtil()->GetConfig().dxSupportLevel < 80 ))
		return 2;
	return 4;
}

bool CShaderAPIEmpty::HasIteratorsPerTextureUnit() const
{
	return false;
}

bool CShaderAPIEmpty::HasSetDeviceGammaRamp() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsCompressedTextures() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsVertexAndPixelShaders() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 80))
		return false;

	return true;
}

bool CShaderAPIEmpty::SupportsPixelShaders_1_4() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 81))
		return false;

	return true;
}

bool CShaderAPIEmpty::SupportsPixelShaders_2_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

bool CShaderAPIEmpty::SupportsVertexShaders_2_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

bool CShaderAPIEmpty::SupportsPartialTextureDownload() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsStateSnapshotting() const
{
	return true;
}

int  CShaderAPIEmpty::MaximumAnisotropicLevel() const
{
	return 0;
}

void CShaderAPIEmpty::SetAnisotropicLevel( int nAnisotropyLevel )
{
}

int  CShaderAPIEmpty::MaxTextureWidth() const
{
	// Should be big enough to cover all cases
	return 16384;
}

int  CShaderAPIEmpty::MaxTextureHeight() const
{
	// Should be big enough to cover all cases
	return 16384;
}

int  CShaderAPIEmpty::MaxTextureAspectRatio() const
{
	// Should be big enough to cover all cases
	return 16384;
}


int	 CShaderAPIEmpty::TextureMemorySize() const
{
	// fake it
	return 64 * 1024 * 1024;
}

int  CShaderAPIEmpty::GetDXSupportLevel() const 
{ 
	if (ShaderUtil()->GetConfig().dxSupportLevel != 0)
		return ShaderUtil()->GetConfig().dxSupportLevel;
	return 90; 
}

bool CShaderAPIEmpty::SupportsOverbright() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsCubeMaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

bool CShaderAPIEmpty::SupportsNonPow2Textures() const
{
	return true;
}

bool CShaderAPIEmpty::SupportsMipmappedCubemaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

int  CShaderAPIEmpty::GetNumTextureStages() const
{
	return 4;
}

int	 CShaderAPIEmpty::NumVertexShaderConstants() const
{
	return 128;
}

int	 CShaderAPIEmpty::NumPixelShaderConstants() const
{
	return 8;
}

int	 CShaderAPIEmpty::MaxNumLights() const
{
	return 4;
}

bool CShaderAPIEmpty::SupportsSpheremapping() const
{
	return false;
}

bool CShaderAPIEmpty::HasFastZReject() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsHardwareLighting() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

int	 CShaderAPIEmpty::MaxBlendMatrices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
	{
		return 1;
	}

	return 0;
}

int	 CShaderAPIEmpty::MaxBlendMatrixIndices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
	{
		return 1;
	}

	return 0;
}

int	 CShaderAPIEmpty::MaxVertexShaderBlendMatrices() const
{
	return 0;
}

int	CShaderAPIEmpty::MaxUserClipPlanes() const
{
	return 0;
}

const char *CShaderAPIEmpty::GetHWSpecificShaderDLLName() const
{
	return 0;
}

// Sets the default *dynamic* state
void CShaderAPIEmpty::SetDefaultState()
{
}


// Returns the snapshot id for the shader state
StateSnapshot_t	 CShaderAPIEmpty::TakeSnapshot( )
{
	StateSnapshot_t id = 0;
	if (g_ShaderShadow.m_IsTranslucent)
		id |= TRANSLUCENT;
	if (g_ShaderShadow.m_IsAlphaTested)
		id |= ALPHATESTED;
	if (g_ShaderShadow.m_UsesAmbientCube)
		id |= AMBIENT_CUBE;
	if (g_ShaderShadow.m_bUsesVertexAndPixelShaders)
		id |= VERTEX_AND_PIXEL_SHADERS;
	return id;
}

// Returns true if the state snapshot is transparent
bool CShaderAPIEmpty::IsTranslucent( StateSnapshot_t id ) const
{
	return (id & TRANSLUCENT) != 0; 
}

bool CShaderAPIEmpty::IsAlphaTested( StateSnapshot_t id ) const
{
	return (id & ALPHATESTED) != 0; 
}

bool CShaderAPIEmpty::UsesAmbientCube( StateSnapshot_t id ) const
{
	return (id & AMBIENT_CUBE) != 0; 
}

bool CShaderAPIEmpty::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	return (id & VERTEX_AND_PIXEL_SHADERS) != 0; 
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIEmpty::ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return 0;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIEmpty::ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return 0;
}

// Uses a state snapshot
void CShaderAPIEmpty::UseSnapshot( StateSnapshot_t snapshot )
{
}

// Sets the color to modulate by
void CShaderAPIEmpty::Color3f( float r, float g, float b )
{
}

void CShaderAPIEmpty::Color3fv( float const* pColor )
{
}

void CShaderAPIEmpty::Color4f( float r, float g, float b, float a )
{
}

void CShaderAPIEmpty::Color4fv( float const* pColor )
{
}

// Faster versions of color
void CShaderAPIEmpty::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIEmpty::Color3ubv( unsigned char const* rgb )
{
}

void CShaderAPIEmpty::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
}

void CShaderAPIEmpty::Color4ubv( unsigned char const* rgba )
{
}

// The shade mode
void CShaderAPIEmpty::ShadeMode( ShaderShadeMode_t mode )
{
}

// Binds a particular material to render with
void CShaderAPIEmpty::Bind( IMaterial* pMaterial )
{
}

// Cull mode
void CShaderAPIEmpty::CullMode( MaterialCullMode_t cullMode )
{
}

void CShaderAPIEmpty::ForceDepthFuncEquals( bool bEnable )
{
}

// Forces Z buffering on or off
void CShaderAPIEmpty::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
}

// Sets the lights
void CShaderAPIEmpty::SetLight( int lightNum, LightDesc_t& desc )
{
}

void CShaderAPIEmpty::SetAmbientLight( float r, float g, float b )
{
}

void CShaderAPIEmpty::SetAmbientLightCube( Vector4D cube[6] )
{
}

// Get lights
int CShaderAPIEmpty::GetMaxLights( void ) const
{
	return 0;
}

const LightDesc_t& CShaderAPIEmpty::GetLight( int lightNum ) const
{
	static LightDesc_t blah;
	return blah;
}

// Render state for the ambient light cube (vertex shaders)
void CShaderAPIEmpty::SetVertexShaderStateAmbientLightCube()
{
}

void CShaderAPIEmpty::SetSkinningMatrices()
{
}

// Render state for the ambient light cube (pixel shaders)
void CShaderAPIEmpty::BindAmbientLightCubeToStage0( )
{
}



// Lightmap texture binding
void CShaderAPIEmpty::BindLightmap( TextureStage_t stage )
{
}

void CShaderAPIEmpty::BindBumpLightmap( TextureStage_t stage )
{
}

void CShaderAPIEmpty::BindWhite( TextureStage_t stage )
{
}

void CShaderAPIEmpty::BindBlack( TextureStage_t stage )
{
}

void CShaderAPIEmpty::BindGrey( TextureStage_t stage )
{
}

// Gets the lightmap dimensions
void CShaderAPIEmpty::GetLightmapDimensions( int *w, int *h )
{
	g_pShaderUtil->GetLightmapDimensions( w, h );
}

// Special system flat normal map binding.
void CShaderAPIEmpty::BindFlatNormalMap( TextureStage_t stage )
{
}

void CShaderAPIEmpty::BindNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPIEmpty::BindSyncTexture( TextureStage_t stage, int texture )
{
}

void CShaderAPIEmpty::BindFBTexture( TextureStage_t stage )
{
}

void CShaderAPIEmpty::CopyRenderTargetToTexture( int texID )
{
}

// Flushes any primitives that are buffered
void CShaderAPIEmpty::FlushBufferedPrimitives()
{
}

IMesh* CShaderAPIEmpty::CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh )
{
	return &m_Mesh;
}

// Creates/destroys Mesh
IMesh* CShaderAPIEmpty::CreateStaticMesh( VertexFormat_t fmt, bool bSoftwareVertexShader )
{
	return &m_Mesh;
}

void CShaderAPIEmpty::DestroyStaticMesh( IMesh* mesh )
{
}

// Gets the dynamic mesh; note that you've got to render the mesh
// before calling this function a second time. Clients should *not*
// call DestroyStaticMesh on the mesh returned by this call.
IMesh* CShaderAPIEmpty::GetDynamicMesh( IMaterial* pMaterial, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	return &m_Mesh;
}


// Begins a rendering pass that uses a state snapshot
void CShaderAPIEmpty::BeginPass( StateSnapshot_t snapshot  )
{
}

// Renders a single pass of a material
void CShaderAPIEmpty::RenderPass( )
{
}

// Page flip
void CShaderAPIEmpty::SwapBuffers( )
{
}

// stuff related to matrix stacks
void CShaderAPIEmpty::MatrixMode( MaterialMatrixMode_t matrixMode )
{
}

void CShaderAPIEmpty::PushMatrix()
{
}

void CShaderAPIEmpty::PopMatrix()
{
}

void CShaderAPIEmpty::LoadMatrix( float *m )
{
}

void CShaderAPIEmpty::MultMatrix( float *m )
{
}

void CShaderAPIEmpty::MultMatrixLocal( float *m )
{
}

void CShaderAPIEmpty::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
}

void CShaderAPIEmpty::LoadIdentity( void )
{
}

void CShaderAPIEmpty::LoadCameraToWorld( void )
{
}

void CShaderAPIEmpty::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
}

void CShaderAPIEmpty::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
}

void CShaderAPIEmpty::PickMatrix( int x, int y, int width, int height )
{
}

void CShaderAPIEmpty::Rotate( float angle, float x, float y, float z )
{
}

void CShaderAPIEmpty::Translate( float x, float y, float z )
{
}

void CShaderAPIEmpty::Scale( float x, float y, float z )
{
}

void CShaderAPIEmpty::ScaleXY( float x, float y )
{
}

// Fog methods...
void CShaderAPIEmpty::FogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPIEmpty::FogStart( float fStart )
{
}

void CShaderAPIEmpty::FogEnd( float fEnd )
{
}

void CShaderAPIEmpty::SetFogZ( float fogZ )
{
}

float CShaderAPIEmpty::GetFogStart( void ) const
{
	return 0.0;
}

float CShaderAPIEmpty::GetFogEnd( void ) const
{
	return 0.0;
}

void CShaderAPIEmpty::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIEmpty::GetSceneFogColor( unsigned char *rgb )
{
	rgb[0] = 0;
	rgb[1] = 0;
	rgb[2] = 0;
}

void CShaderAPIEmpty::SceneFogMode( MaterialFogMode_t fogMode )
{
}

MaterialFogMode_t CShaderAPIEmpty::GetSceneFogMode( )
{
	return MATERIAL_FOG_NONE;
}

void CShaderAPIEmpty::FogColor3f( float r, float g, float b )
{
}

void CShaderAPIEmpty::FogColor3fv( float const* rgb )
{
}

void CShaderAPIEmpty::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIEmpty::FogColor3ubv( unsigned char const* rgb )
{
}

void CShaderAPIEmpty::Viewport( int x, int y, int width, int height )
{
}

void CShaderAPIEmpty::GetViewport( int& x, int& y, int& width, int& height ) const
{
}

// Sets the vertex and pixel shaders
void CShaderAPIEmpty::SetVertexShaderIndex( int vshIndex )
{
}

void CShaderAPIEmpty::SetPixelShaderIndex( int pshIndex )
{
}

// Sets the constant register for vertex and pixel shaders
void CShaderAPIEmpty::SetVertexShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
}

void CShaderAPIEmpty::SetPixelShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
}


// Returns the nearest supported format
ImageFormat CShaderAPIEmpty::GetNearestSupportedFormat( ImageFormat fmt ) const
{
	return fmt;
}

ImageFormat CShaderAPIEmpty::GetNearestRenderTargetFormat( ImageFormat fmt ) const
{
	return fmt;
}

// Sets the texture state
void CShaderAPIEmpty::BindTexture( TextureStage_t stage, unsigned int textureNum )
{
}

void CShaderAPIEmpty::SetRenderTarget( unsigned int textureNum, unsigned int ztexture )
{
}

void CShaderAPIEmpty::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIEmpty::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
}

ImageFormat CShaderAPIEmpty::GetBackBufferFormat() const
{
	return IMAGE_FORMAT_RGB888;
}

void CShaderAPIEmpty::GetBackBufferDimensions( int& width, int& height ) const
{
	width = 1024;
	height = 768;
}

// Indicates we're going to be modifying this texture
// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
// all use the texture specified by this function.
void CShaderAPIEmpty::ModifyTexture( unsigned int textureNum )
{
}

// Texture management methods
void CShaderAPIEmpty::TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int width, int height, 
						 ImageFormat srcFormat, void *imageData )
{
}

void CShaderAPIEmpty::TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int width, int height,
						 ImageFormat srcFormat, int srcStride, void *imageData )
{
}

bool CShaderAPIEmpty::TexLock( int level, int cubeFaceID, int xOffset, int yOffset, 
								int width, int height, CPixelWriter& writer )
{
	return false;
}

void CShaderAPIEmpty::TexUnlock( )
{
}


// These are bound to the texture, not the texture environment
void CShaderAPIEmpty::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{
}

void CShaderAPIEmpty::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{
}

void CShaderAPIEmpty::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
}

void CShaderAPIEmpty::TexSetPriority( int priority )
{
}

void CShaderAPIEmpty::SetLightmapTextureIdRange( int firstId, int lastId )
{
}

void CShaderAPIEmpty::CreateTexture( unsigned int textureNum, int width, int height,
		ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags )
{
}

void CShaderAPIEmpty::CreateDepthTexture( unsigned int textureNum, ImageFormat renderFormat, int width, int height )
{
}

void CShaderAPIEmpty::DeleteTexture( unsigned int textureNum )
{
}

bool CShaderAPIEmpty::IsTexture( unsigned int textureNum )
{
	return true;
}

bool CShaderAPIEmpty::IsTextureResident( unsigned int textureNum )
{
	return false;
}

// stuff that isn't to be used from within a shader
void CShaderAPIEmpty::DepthRange( float zNear, float zFar )
{
}

void CShaderAPIEmpty::ClearBuffers( bool bClearColor, bool bClearDepth, int renderTargetWidth, int renderTargetHeight )
{
}

void CShaderAPIEmpty::ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat )
{
}

void CShaderAPIEmpty::FlushHardware()
{
}

void CShaderAPIEmpty::ResetRenderState()
{
}

// Set the number of bone weights
void CShaderAPIEmpty::SetNumBoneWeights( int numBones )
{
}

// Point size...
void CShaderAPIEmpty::PointSize( float size )
{
}

// Selection mode methods
int CShaderAPIEmpty::SelectionMode( bool selectionMode )
{
	return 0;
}

void CShaderAPIEmpty::SelectionBuffer( unsigned int* pBuffer, int size )
{
}

void CShaderAPIEmpty::ClearSelectionNames( )
{
}

void CShaderAPIEmpty::LoadSelectionName( int name )
{
}

void CShaderAPIEmpty::PushSelectionName( int name )
{
}

void CShaderAPIEmpty::PopSelectionName()
{
}


// Use this to get the mesh builder that allows us to modify vertex data
CMeshBuilder* CShaderAPIEmpty::GetVertexModifyBuilder()
{
	return 0;
}

// Board-independent calls, here to unify how shaders set state
// Implementations should chain back to IShaderUtil->BindTexture(), etc.

// Use this to spew information about the 3D layer 
void CShaderAPIEmpty::SpewDriverInfo() const
{
	Warning("Empty shader\n");
}

// Use this to begin and end the frame
void CShaderAPIEmpty::BeginFrame()
{
}

void CShaderAPIEmpty::EndFrame()
{
}

// returns the current time in seconds....
double CShaderAPIEmpty::CurrentTime() const
{
	return Sys_FloatTime();
}

// Get the current camera position in world space.
void CShaderAPIEmpty::GetWorldSpaceCameraPosition( float * pPos ) const
{
}

void CShaderAPIEmpty::SetSoftwareVertexShader( SoftwareVertexShader_t shader )
{
}

void CShaderAPIEmpty::CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
{
}

void CShaderAPIEmpty::ForceHardwareSync( void )
{
}

void CShaderAPIEmpty::DrawDebugText( int desiredLeft, int desiredTop, 
									 MaterialRect_t *pActualRect, const char *pText )
{
}

void CShaderAPIEmpty::SetHeightClipZ( float z )
{
}

void CShaderAPIEmpty::SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode )
{
}

void CShaderAPIEmpty::SetClipPlane( int index, const float *pPlane )
{
}

void CShaderAPIEmpty::EnableClipPlane( int index, bool bEnable )
{
}

int CShaderAPIEmpty::GetCurrentNumBones( void ) const
{
	return 0;
}

int CShaderAPIEmpty::GetCurrentLightCombo( void ) const
{
	return 0;
}

int CShaderAPIEmpty::GetCurrentFogType( void ) const
{
	return 0;
}

void CShaderAPIEmpty::RecordString( const char *pStr )
{
}

bool CShaderAPIEmpty::ReadPixelsFromFrontBuffer() const
{
	return true;
}

bool CShaderAPIEmpty::PreferDynamicTextures() const
{
	return false;
}

bool CShaderAPIEmpty::HasProjectedBumpEnv() const
{
	return true;
}

void CShaderAPIEmpty::DestroyVertexBuffers()
{
}

void CShaderAPIEmpty::EvictManagedResources()
{
}

void CShaderAPIEmpty::SetTextureTransformDimension( int textureStage, int dimension, bool projected )
{
}

void CShaderAPIEmpty::SetBumpEnvMatrix( int textureStage, float m00, float m01, float m10, float m11 )
{
}

// Adds/removes precompiled shader dictionaries
ShaderDLL_t CShaderAPIEmpty::AddShaderDLL( )
{
	return 0;
}

void CShaderAPIEmpty::RemoveShaderDLL( ShaderDLL_t hShaderDLL ) 
{
}

void CShaderAPIEmpty::AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict )
{
}

void CShaderAPIEmpty::SetShaderDLL( ShaderDLL_t hShaderDLL )
{
}

void CShaderAPIEmpty::SyncToken( const char *pToken )
{
}

