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
// The dx8 implementation of the shader API
//=============================================================================
	    
/*
DX9 todo:
-make the transforms in the older shaders match the transforms in lightmappedgeneric
-fix polyoffset for hardware that doesn't support D3DRS_SLOPESCALEDEPTHBIAS and D3DRS_DEPTHBIAS
	- code is there, I think matrix offset just needs tweaking
-fix forcehardwaresync - implement texture locking for hardware that doesn't support async query
-get the format for GetAdapterModeCount and EnumAdapterModes from somewhere (shaderapidx8.cpp, GetModeCount, GetModeInfo)
-record frame sync objects (allocframesyncobjects, free framesync objects, ForceHardwareSync)
-Need to fix ENVMAPMASKSCALE, BUMPOFFSET in lightmappedgeneric*.cpp and vertexlitgeneric*.cpp
fix this:
		// FIXME: This also depends on the vertex format and whether or not we are static lit in dx9
	#ifndef SHADERAPIDX9
		if (m_DynamicState.m_VertexShader != shader) // garymcthack
	#endif // !SHADERAPIDX9
unrelated to dx9:
mat_fullbright 1 doesn't work properly on alpha materials in testroom_standards
*/
#include <windows.h>	    


#include "shaderapidx8.h"
#include "ShaderShadowDX8.h"
#include "locald3dtypes.h"												   
#include "UtlVector.h"
#include "IHardwareConfigInternal.h"
#include "UtlStack.h"
#include "IShaderUtil.h"
#include "ShaderAPIDX8_Global.h"
#include "materialsystem/IMaterialSystem.h"
#include "IMaterialInternal.h"
#include "IMeshDX8.h"
#include "ColorFormatDX8.h"
#include "TextureDX8.h"
#include <malloc.h>
#include "interface.h"
#include "CMaterialSystemStats.h"
#include "UtlRBTree.h"
#include "UtlSymbol.h"
#include "vstdlib/strTools.h"
#include "Recording.h"
#include <crtmemdebug.h>
#include "VertexShaderDX8.h"
#include "FileSystem.h"
#include "mathlib.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "worldsize.h"
#include "TransitionTable.h"
#include "tier0/vcrmode.h"
#include "tier0/vprof.h"
#include "utlbuffer.h"
#include "vertexdecl.h"
#include "vstdlib/icommandline.h"
#include <KeyValues.h>

#include "../stdshaders/common_hlsl_cpp_consts.h" // hack hack hack!


// Define this if you want to use a stubbed d3d.
//#define STUBD3D

#ifdef STUBD3D
#include "stubd3ddevice.h"
#endif // STUBD3D

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _WIN32
#pragma warning (disable:4189)
#endif


//-----------------------------------------------------------------------------
// Some important enumerations
//-----------------------------------------------------------------------------
#define TEXTURE_AMBIENT_CUBE 0xFFFFFFFE

enum
{
	NUM_TEXTURE_UNITS = 4,
	MAX_NUM_LIGHTS = 2,

	// Hopefully, no one else will use this id..!
	AMBIENT_CUBE_TEXTURE_SIZE = 8,
	NUM_AMBIENT_CUBE_TEXTURES = 4,
};


//-----------------------------------------------------------------------------
// Standard vertex shader constants
//-----------------------------------------------------------------------------
enum
{
	// Standard vertex shader constants
	VERTEX_SHADER_CAMERA_POS = 2,
	VERTEX_SHADER_LIGHT_INDEX = 3,
	VERTEX_SHADER_MODELVIEWPROJ = 4,
	VERTEX_SHADER_VIEWPROJ = 8,
	VERTEX_SHADER_MODELVIEW = 12,
	VERTEX_SHADER_FOG_PARAMS = 16,
	VERTEX_SHADER_AMBIENT_LIGHT = 21,
	VERTEX_SHADER_LIGHTS = 27,
	VERTEX_SHADER_CLIP_DIRECTION = 37,
	VERTEX_SHADER_MODEL = 42,
};


//-----------------------------------------------------------------------------
// These board states change with high frequency; are not shadowed
//-----------------------------------------------------------------------------
struct TextureStageState_t
{
	int						m_BoundTexture;
	D3DTEXTUREADDRESS		m_UTexWrap;
	D3DTEXTUREADDRESS		m_VTexWrap;
	D3DTEXTUREFILTERTYPE	m_MagFilter;
	D3DTEXTUREFILTERTYPE	m_MinFilter;
	D3DTEXTUREFILTERTYPE	m_MipFilter;
	int						m_nAnisotropyLevel;
	D3DTEXTURETRANSFORMFLAGS m_TextureTransformFlags;
	bool					m_TextureEnable;
	float					m_BumpEnvMat00;
	float					m_BumpEnvMat01;
	float					m_BumpEnvMat10;
	float					m_BumpEnvMat11;
	bool					m_bSRGBReadEnabled;
};

enum TransformType_t
{
	TRANSFORM_IS_IDENTITY = 0,
	TRANSFORM_IS_CAMERA_TO_WORLD,
	TRANSFORM_IS_GENERAL,
};

enum TransformDirtyBits_t
{
	STATE_CHANGED_VERTEX_SHADER = 0x1,
	STATE_CHANGED_FIXED_FUNCTION = 0x2,
	STATE_CHANGED = 0x3
};

enum
{
	MAXUSERCLIPPLANES = 6,
	MAX_NUM_RENDERSTATES = ( D3DRS_BLENDOPALPHA+1 ),
};

struct DynamicState_t
{
	// Constant color
	unsigned int	m_ConstantColor;

	// point size
	float			m_PointSize;

	// Normalize normals?
	bool			m_NormalizeNormals;

	// Viewport state
	D3DVIEWPORT	m_Viewport;

	// Transform state
	D3DXMATRIX		m_Transform[NUM_MATRIX_MODES];
	unsigned char	m_TransformType[NUM_MATRIX_MODES];
	unsigned char	m_TransformChanged[NUM_MATRIX_MODES];

	// Ambient light color
	D3DCOLOR		m_Ambient;
	D3DLIGHT		m_Lights[MAX_NUM_LIGHTS];
	LightDesc_t		m_LightDescs[MAX_NUM_LIGHTS];
	bool			m_LightEnable[MAX_NUM_LIGHTS];
	Vector4D		m_AmbientLightCube[6];
	unsigned char	m_LightChanged[MAX_NUM_LIGHTS];
	unsigned char	m_LightEnableChanged[MAX_NUM_LIGHTS];
	VertexShaderLightTypes_t	m_LightType[MAX_NUM_LIGHTS];
	int				m_NumLights;

	// Shade mode
	D3DSHADEMODE	m_ShadeMode;

	// Clear color
	D3DCOLOR		m_ClearColor;

	// Fog
	D3DCOLOR		m_FogColor;
	bool			m_FogEnable;
	bool			m_FogHeightEnabled;
	D3DFOGMODE		m_FogMode;
	float			m_FogStart;
	float			m_FogEnd;
	float			m_FogZ;

	float			m_HeightClipZ;
	MaterialHeightClipMode_t m_HeightClipMode;	
	
	// user clip planes
	int				m_UserClipPlaneEnabled;
	int				m_UserClipPlaneChanged;
	D3DXPLANE		m_UserClipPlaneWorld[MAXUSERCLIPPLANES];
	D3DXPLANE		m_UserClipPlaneProj[MAXUSERCLIPPLANES];

	bool			m_FastClipEnabled;
	D3DXPLANE		m_FastClipPlane;

	// Cull mode
	D3DCULL			m_CullMode;

	// Skinning
	D3DVERTEXBLENDFLAGS	m_VertexBlend;
	int					m_NumBones;

	// Pixel and vertex shader constants...
	Vector4D*	m_pPixelShaderConstant;
	Vector4D*	m_pVertexShaderConstant;

	// Is the material reflection values divided?
	float		m_MaterialOverbrightVal;
	float		m_VertexShaderOverbright;

	// Texture stage state
	TextureStageState_t m_TextureStage[NUM_TEXTURE_STAGES];

	DWORD m_RenderState[MAX_NUM_RENDERSTATES];

	IDirect3DVertexDeclaration9 *m_pVertexDecl;

	DynamicState_t() {}

private:
	DynamicState_t( DynamicState_t const& );
};



//-----------------------------------------------------------------------------
// The DX8 implementation of the shader API
//-----------------------------------------------------------------------------
class CShaderAPIDX8 : public IShaderAPIDX8, public IHardwareConfigInternal
{
public:
	virtual void EnableSRGBRead( TextureStage_t stage, bool bEnable );

	// constructor, destructor
	CShaderAPIDX8( );
	virtual ~CShaderAPIDX8();

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

	// Returns true if the material system should try to use 16 bit textures.
	virtual bool Prefer16BitTextures( ) const;
	
	// Sets the mode...
	bool SetMode( void* hwnd, MaterialVideoMode_t const& mode, int flags, int nSuperSamples = 0 );

	// Creates/ destroys a child window
	bool AddView( void* hwnd );
	void RemoveView( void* hwnd );

	// Activates a view
	void SetView( void* hwnd );

	// Sets the default render state
	void SetDefaultState();

	// Methods to ask about particular state snapshots
	virtual bool IsTranslucent( StateSnapshot_t id ) const;
	virtual bool IsAlphaTested( StateSnapshot_t id ) const;
	virtual bool UsesVertexAndPixelShaders( StateSnapshot_t id ) const;

	// returns true if the state uses an ambient cube
	bool UsesAmbientCube( StateSnapshot_t id ) const;

	// Computes the vertex format for a particular set of snapshot ids
	VertexFormat_t ComputeVertexFormat( int num, StateSnapshot_t* pIds ) const;
	VertexFormat_t ComputeVertexUsage( int num, StateSnapshot_t* pIds ) const;

	// Page flip
	void SwapBuffers( );

	// Uses a state snapshot
	void UseSnapshot( StateSnapshot_t snapshot );

	// Color state
	void Color3f( float r, float g, float b );
	void Color4f( float r, float g, float b, float a );
	void Color3fv( float const* c );
	void Color4fv( float const* c );

	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const* pColor );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const* pColor );

	// Set the number of bone weights
	void SetNumBoneWeights( int numBones );

	// Sets the vertex and pixel shaders
	virtual void SetVertexShaderIndex( int vshIndex = -1 );
	virtual void SetPixelShaderIndex( int pshIndex = 0 );

	// Matrix state
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

	// Viewport methods
	void Viewport( int x, int y, int width, int height );
	void GetViewport( int& x, int& y, int& width, int& height ) const;

	// Binds a particular material to render with
	void Bind( IMaterial* pMaterial );
	IMaterialInternal* GetBoundMaterial();

	// Adds/removes precompiled shader dictionaries
	virtual ShaderDLL_t AddShaderDLL( );
	virtual void RemoveShaderDLL( ShaderDLL_t hShaderDLL );
	virtual void AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict );
	virtual void SetShaderDLL( ShaderDLL_t hShaderDLL );

	// Level of anisotropic filtering
	virtual void SetAnisotropicLevel( int nAnisotropyLevel );

	virtual void	SyncToken( const char *pToken );

	// Point size...
	void PointSize( float size );

	// Cull mode
	void CullMode( MaterialCullMode_t cullMode );

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
	void ForceDepthFuncEquals( bool bEnable );

	// Turns off Z buffering
	void OverrideDepthEnable( bool bEnable, bool bDepthEnable );

	void SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode );
	void SetHeightClipZ( float z );

	void SetClipPlane( int index, const float *pPlane );
	void EnableClipPlane( int index, bool bEnable );
	
	void SetFastClipPlane(const float *pPlane);
	void EnableFastClip(bool bEnable);
	
	// The shade mode
	void ShadeMode( ShaderShadeMode_t mode );

	// Vertex blend state
	void SetVertexBlendState( int numBones );

	// Creates/destroys Mesh
	IMesh* CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh );
	IMesh* CreateStaticMesh( VertexFormat_t format, bool bSoftwareVertexShader );
	void DestroyStaticMesh( IMesh* mesh );

	// Gets the dynamic mesh
	IMesh* GetDynamicMesh( IMaterial* pMaterial, bool buffered,
		IMesh* pVertexOverride, IMesh* pIndexOverride );

	// Draws the mesh
	void DrawMesh( IMeshDX8* mesh );

	// modifies the vertex data when necessary
	void ModifyVertexData( );

	// Draws
	void BeginPass( StateSnapshot_t snapshot  );
	void RenderPass( );

	virtual void DestroyVertexBuffers();

	void SetVertexDecl( VertexFormat_t vertexFormat, bool bHasColorMesh );

	// Sets the constant register for vertex and pixel shaders
	void SetVertexShaderConstant( int var, float const* pVec, int numVecs = 1, bool bForce = false );
	void SetPixelShaderConstant( int var, float const* pVec, int numVecs = 1, bool bForce = false );

	// Returns the nearest supported format
	ImageFormat GetNearestSupportedFormat( ImageFormat fmt ) const;
	ImageFormat GetNearestRenderTargetFormat( ImageFormat format ) const;

	// This is only used for statistics gathering..
	void SetLightmapTextureIdRange( int firstId, int lastId );

	// stuff that shouldn't be used from within a shader 
    void ModifyTexture( unsigned int textureNum );
	void BindTexture( TextureStage_t stage, unsigned int textureNum );
	void DeleteTexture( unsigned int textureNum );
	bool IsTexture( unsigned int textureNum );
	bool IsTextureResident( unsigned int textureNum );

	// Set the render target to a texID.
	// Set to SHADER_RENDERTARGET_BACKBUFFER if you want to use the regular framebuffer.
	void SetRenderTarget( unsigned int colorTexture = SHADER_RENDERTARGET_BACKBUFFER, 
		unsigned int depthTexture = SHADER_RENDERTARGET_DEPTHBUFFER );
	ImageFormat GetBackBufferFormat() const;
	void GetBackBufferDimensions( int& width, int& height ) const;

	// These are bound to the texture, not the texture environment
	void TexMinFilter( ShaderTexFilterMode_t texFilterMode );
	void TexMagFilter( ShaderTexFilterMode_t texFilterMode );
	void TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode );
	void TexSetPriority( int priority );

	void CreateTexture( unsigned int textureNum, int width, 
		int height, ImageFormat dstImageFormat, int numMipLevels, int numCopies, int creationFlags );
	void CreateDepthTexture( unsigned int textureNum, ImageFormat renderTargetFormat, int width, int height );
	void TexImage2D( int level, int cubeFaceID, ImageFormat dstFormat, int width, int height, 
							 ImageFormat srcFormat, void *imageData );
	void TexSubImage2D( int level, int cubeFaceID, int xOffset, int yOffset, int width, int height,
							 ImageFormat srcFormat, int srcStride, void *imageData );

	bool TexLock( int level, int cubeFaceID, int xOffset, int yOffset, 
									int width, int height, CPixelWriter& writer );
	void TexUnlock( );

	// stuff that isn't to be used from within a shader
	// what's the best way to hide this? subclassing?
	void DepthRange( float zNear, float zFar );
	void ClearBuffers( bool bClearColor, bool bClearDepth, int renderTargetWidth, int renderTargetHeight );
	void ClearColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat );

	// We need to make the ambient cube...
	void CreateAmbientCubeTexture();
	void ReleaseAmbientCubeTexture();

	// Gets the current buffered state... (debug only)
	void GetBufferedState( BufferedState_t& state );

	// Buffered primitives
	void FlushBufferedPrimitives( );

	// Make sure we finish drawing everything that has been requested 
	void FlushHardware();
	
	// Use this to begin and end the frame
	void BeginFrame();
	void EndFrame();

	// Use this to spew information about the 3D layer 
	void SpewDriverInfo() const;

	// Used to clear the transition table when we know it's become invalid.
	void ClearSnapshots();

	// returns the D3D interfaces....
	IDirect3DDevice* D3DDevice()	
	{ 
		return m_pD3DDevice; 
	}
	IDirect3D*		  D3D()			
	{ 
		return m_pD3D; 
	}

	virtual int GetActualNumTextureUnits() const;

	// Inherited from IMaterialSystemHardwareConfig
	bool HasAlphaBuffer() const;
	bool HasStencilBuffer() const;
	bool HasARBMultitexture() const;
	bool HasNVRegisterCombiners() const;
	bool HasTextureEnvCombine() const;
	int	 GetFrameBufferColorDepth() const;
	int  GetNumTextureUnits() const;
	int  GetNumTextureStages() const;
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
	int	 MaxTextureAspectRatio() const;
	int	 TextureMemorySize() const;
	bool SupportsOverbright() const;
	bool SupportsMipmapping() const;
	bool SupportsCubeMaps() const;
	bool SupportsMipmappedCubemaps() const;
	bool SupportsNonPow2Textures() const;
	int	 NumVertexShaderConstants() const;
	int	 NumPixelShaderConstants() const;
	int	 MaxNumLights() const;
	bool SupportsHardwareLighting() const;
	int	 MaxBlendMatrices() const;
	int	 MaxBlendMatrixIndices() const;
	int	 MaxVertexShaderBlendMatrices() const;
	int	 GetDXSupportLevel() const;
	const char *GetShaderDLLName() const;
	int	 MaxUserClipPlanes() const;
	bool UseFastClipping() const;
	int  GetAnisotropicLevel() const;
	bool UseForcedTrilinear() const;
	bool UseForcedBilinear() const;
	bool UseFastZReject() const;
	void UpdateFastClipUserClipPlane( void );
	bool ReadPixelsFromFrontBuffer() const;
	bool PreferDynamicTextures() const;
	bool HasProjectedBumpEnv() const;

	// GR - HDR stuff
	bool SupportsHDR() const;
	bool NeedsAAClamp() const;
	bool SupportsSpheremapping() const;
	virtual bool HasFastZReject() const;
	virtual bool NeedsATICentroidHack() const;
	virtual const char *GetHWSpecificShaderDLLName() const;

	// returns the current time in seconds....
	double CurrentTime() const;

	// Get the current camera position in world space.
	void GetWorldSpaceCameraPosition( float* pPos ) const;

	// Fog methods
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

	void SceneFogMode( MaterialFogMode_t fogMode );
	MaterialFogMode_t GetSceneFogMode( );
	void SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void GetSceneFogColor( unsigned char *rgb );
	
	// Selection mode methods
	int  SelectionMode( bool selectionMode );
	void SelectionBuffer( unsigned int* pBuffer, int size );
	void ClearSelectionNames( );
	void LoadSelectionName( int name );
	void PushSelectionName( int name );
	void PopSelectionName();
	bool IsInSelectionMode() const;
	void RegisterSelectionHit( float minz, float maxz );
	void WriteHitRecord();

    // lightmap stuff
	void BindLightmap( TextureStage_t stage );
	void BindLightmapAlpha( TextureStage_t stage );
	void BindBumpLightmap( TextureStage_t stage );
	void BindWhite( TextureStage_t stage );
	void BindBlack( TextureStage_t stage );
	void BindGrey( TextureStage_t stage );
	void BindFlatNormalMap( TextureStage_t stage );
	void BindSyncTexture( TextureStage_t stage, int texture );
	void BindFBTexture( TextureStage_t stage );
	void BindNormalizationCubeMap( TextureStage_t stage );

	// Gets the lightmap dimensions
	virtual void GetLightmapDimensions( int *w, int *h );

	// Use this to get the mesh builder that allows us to modify vertex data
	CMeshBuilder* GetVertexModifyBuilder();

	// Helper to get at the texture state stage
	TextureStageState_t& TextureStage( int stage ) { return m_DynamicState.m_TextureStage[stage]; }
	TextureStageState_t const& TextureStage( int stage ) const { return m_DynamicState.m_TextureStage[stage]; }

	void SetAmbientLight( float r, float g, float b );
	void SetLight( int lightNum, LightDesc_t& desc );
	void SetAmbientLightCube( Vector4D colors[6] );

	int GetMaxLights( void ) const;
	const LightDesc_t& GetLight( int lightNum ) const;

	void SetVertexShaderStateAmbientLightCube();
	void BindAmbientLightCubeToStage0( );

	void CopyRenderTargetToTexture( int texID );

	// Returns the cull mode (for fill rate computation)
	D3DCULL GetCullMode() const;

	// Gets the window size
	void GetWindowSize( int& width, int& height ) const;

	// Applies Z Bias
	void ApplyZBias( ShadowState_t const& shaderState );

	// Applies texture enable
	void ApplyTextureEnable( ShadowState_t const& state, int stage );

	// Applies cull enable
	void ApplyCullEnable( bool bEnable );

	// Applies alpha blending
	void ApplyAlphaBlend( bool bEnable, D3DBLEND srcBlend, D3DBLEND destBlend );

	// Applies alpha texture op
	void ApplyColorTextureStage( int stage, D3DTEXTUREOP op, int arg1, int arg2 );
	void ApplyAlphaTextureStage( int stage, D3DTEXTUREOP op, int arg1, int arg2 );

	// Sets the material rendering state
	void SetMaterialProperty( float overbrightVal );

	// Sets texture stage stage + render stage state
	void SetSamplerState( int stage, D3DSAMPLERSTATETYPE state, DWORD val );
	void SetTextureStageState( int stage, D3DTEXTURESTAGESTATETYPE state, DWORD val );
	void SetRenderStateForce( D3DRENDERSTATETYPE state, DWORD val );
	void SetRenderState( D3DRENDERSTATETYPE state, DWORD val );

	// Can we download textures?
	virtual bool CanDownloadTextures() const;

	// Are we using graphics?
	virtual bool IsUsingGraphics() const;

	// Call this when another app is initializing or finished initializing
	void OtherAppInitializing( bool initializing );

	// Initializes the C1 constant (has the overbright term)
	void SetVertexShaderConstantC1( float overbright );

	void ForceHardwareSync( void );

	void DrawDebugText( int desiredLeft, int desiredTop, 
					    MaterialRect_t *pActualRect,
						const char *pText );

	void EvictManagedResources();
	// Gets at a particular transform
	inline D3DXMATRIX& GetTransform( int i )
	{
		return *m_pMatrixStack[i]->GetTop();
	}

	int GetCurrentNumBones( void ) const;
	int GetCurrentLightCombo( void ) const;
	int GetCurrentFogType( void ) const;

	void RecordString( const char *pStr );

	virtual bool IsRenderingMesh() const { return m_pRenderMesh != 0; }

	void SetTextureTransformDimension( int textureStage, int dimension, bool projected );
	void DisableTextureTransform( int textureMatrix );
	void SetBumpEnvMatrix( int textureStage, float m00, float m01, float m10, float m11 );
private:
	CShaderAPIDX8( CShaderAPIDX8 const& );

	enum
	{
		INVALID_TRANSITION_OP = 0xFFFF
	};

	struct Texture_t
	{
		Texture_t()
		{
			m_Flags = 0;
		}
		
		int m_BindId;

		// FIXME: Compress this info
		D3DTEXTUREADDRESS m_UTexWrap;
		D3DTEXTUREADDRESS m_VTexWrap;
		D3DTEXTUREFILTERTYPE m_MagFilter;
		D3DTEXTUREFILTERTYPE m_MinFilter;
		D3DTEXTUREFILTERTYPE m_MipFilter;
	    unsigned char m_NumLevels;
		unsigned char m_SwitchNeeded;	// Do we need to advance the current copy?
		unsigned char m_NumCopies;		// copies are used to optimize procedural textures
		unsigned char m_CurrentCopy;	// the current copy we're using...

		// stats stuff
		int m_SizeBytes;
		int m_SizeTexels;
		int m_LastBoundFrame;

		enum Flags_t
		{
			IS_DEPTH_STENCIL = 1
		};
		
		unsigned int m_Flags;
		typedef IDirect3DBaseTexture *IDirect3DBaseTexturePtr; // garymcthack
		typedef IDirect3DBaseTexture **IDirect3DBaseTexturePtrPtr; // garymcthack
		IDirect3DBaseTexturePtr &GetTexture( void )
		{
			Assert( m_NumCopies == 1 );
			Assert( !( m_Flags & IS_DEPTH_STENCIL ) );
			return m_pTexture;
		}
		IDirect3DBaseTexturePtr &GetTexture( int copy )
		{
			Assert( m_NumCopies > 1 );
			Assert( !( m_Flags & IS_DEPTH_STENCIL ) );
			return m_ppTexture[copy];
		}
		IDirect3DBaseTexturePtrPtr &GetTextureArray( void )
		{
			Assert( m_NumCopies > 1 );
			Assert( !( m_Flags & IS_DEPTH_STENCIL ) );
			return m_ppTexture;
		}
		typedef IDirect3DSurface *IDirect3DSurfacePtr; // garymcthack
		IDirect3DSurfacePtr &GetDepthStencilSurface( void )
		{
			Assert( m_NumCopies == 1 );
			Assert( m_Flags & IS_DEPTH_STENCIL );
			return m_pDepthStencilSurface;
		}
	private:
		union
		{
			IDirect3DBaseTexture*  m_pTexture;		// used when there's one copy
			IDirect3DBaseTexture** m_ppTexture;	// used when there are more than one copies
			IDirect3DSurface*		m_pDepthStencilSurface; // used when there's one depth stencil surface
		};
	};

	// Allow us to override support level based on device + vendor id
	struct DeviceSupportLevels_t
	{
		int m_nVendorId;
		int m_nDeviceIdMin;
		int m_nDeviceIdMax;
		int m_nDXSupportLevel;
		int m_nCreateQuery;		// Whether the card supports CreateQuery or not.  -1 = detect, 1 = yes, 0 = no.
		char m_pShaderDLL[32];
		bool m_bFastZReject;
		bool m_bFastClipping;
		bool m_bNeedsATICentroidHack;
	};

	enum CompressedTextureState_t
	{
		COMPRESSED_TEXTURES_ON,
		COMPRESSED_TEXTURES_OFF,
		COMPRESSED_TEXTURES_NOT_INITIALIZED
	};

	struct HardwareCaps_t
	{
		// NOTE: The difference between texture units and texture stages
		// is that texture units represents the number of simultaneous textures
		// if there's more stages than units, it usually is because there's
		// some sort of emulation of extra hardware features going on.
		int  m_NumTextureUnits;
		int  m_NumTextureStages;
		bool m_HasSetDeviceGammaRamp;
		bool m_SupportsVertexShaders;
		bool m_SupportsVertexShaders_2_0;
		bool m_SupportsPixelShaders;
		bool m_SupportsPixelShaders_1_4;
		bool m_SupportsPixelShaders_2_0;
		CompressedTextureState_t m_SupportsCompressedTextures;
		bool m_bSupportsAnisotropicFiltering;
		int	 m_nMaxAnisotropy;
		int  m_MaxTextureWidth;
		int  m_MaxTextureHeight;
		int	 m_MaxTextureAspectRatio;
		int  m_MaxPrimitiveCount;
		bool m_ZBiasSupported;
		bool m_SlopeScaledDepthBiasSupported;
		bool m_SupportsMipmapping;
		bool m_SupportsOverbright;
		bool m_SupportsCubeMaps;
		int  m_NumPixelShaderConstants;
		int  m_NumVertexShaderConstants;
		int  m_TextureMemorySize;
		int  m_MaxNumLights;
		bool m_SupportsHardwareLighting;
		int  m_MaxBlendMatrices;
		int  m_MaxBlendMatrixIndices;
		int	 m_MaxVertexShaderBlendMatrices;
		bool m_SupportsMipmappedCubemaps;
		bool m_SupportsNonPow2Textures;
		int  m_DXSupportLevel;
		bool m_PreferDynamicTextures;
		bool m_HasProjectedBumpEnv;
		int m_MaxUserClipPlanes;
		// GR
		bool m_SupportsSRGB;
		bool m_SupportsFatTextures;
		bool m_SupportsHDR;
		bool m_bSupportsSpheremapping;
		bool m_UseFastClipping;
		char m_pShaderDLL[32];
		bool m_bFastZReject;
		bool m_bNeedsATICentroidHack;
	};

	// Mode setting utility methods
	bool ValidateMode( void* hwnd, const MaterialVideoMode_t &mode, int flags ) const;
	bool DoesModeRequireDeviceChange( void* hwnd, const MaterialVideoMode_t &mode, int flags ) const;
	bool CanRenderWindowed( ) const;
	void SetPresentParameters( MaterialVideoMode_t const& mode, int flags, int nSampleCount );

	// Changes the window size
	bool ResizeWindow( const MaterialVideoMode_t &mode, int flags, int nSampleCount );

	// Finds a child window
	int  FindView( void* hwnd ) const;

	// Initialize, shutdown the D3DDevice....
	bool InitDevice( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSampleCount );
	void ShutdownDevice( );

	// Creates the D3D Device
	bool CreateD3DDevice( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSampleCount );

	// Alloc and free objects that are necessary for frame syncing
	void AllocFrameSyncObjects( void );
	void FreeFrameSyncObjects( void );
	
	// Creates the matrix stack
	void CreateMatrixStacks();

	// Initializes the render state
	void InitRenderState( );

	// Resets all dx renderstates to dx default so that our shadows are correct.
	void ResetDXRenderState( );
	
	// Resets the render state
	void ResetRenderState( );

	// Initializes vertex and pixel shaders
	void InitVertexAndPixelShaders( );

	// Determines hardware capabilities
	bool DetermineHardwareCaps( );

	// Discards the vertex and index buffers
	void DiscardVertexBuffers();

	// Deals with lost devices
	void CheckDeviceLost();

	// Computes the fill rate
	void ComputeFillRate();

	// Takes a snapshot
	virtual StateSnapshot_t TakeSnapshot( );

	// Gets the matrix stack from the matrix mode
	int GetMatrixStack( MaterialMatrixMode_t mode ) const;

	// Flushes the matrix state, returns false if we don't need to
	// do any more work
	bool MatrixIsChanging( TransformType_t transform = TRANSFORM_IS_GENERAL );

	// Updates the matrix transform state
	void UpdateMatrixTransform( TransformType_t transform = TRANSFORM_IS_GENERAL );

	// Sets the vertex shader modelView state..
	const D3DXMATRIX &GetProjectionMatrix( void );
	void SetVertexShaderViewProj();
	void SetVertexShaderModelViewProjAndModelView();

	FORCEINLINE void UpdateVertexShaderFogParams( void )
	{
		MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_FOG_MODE);

		if( m_Caps.m_SupportsPixelShaders )
		{
			float ooFogRange = 1.0f / ( m_VertexShaderFogParams[1] - m_VertexShaderFogParams[0] );
			m_VertexShaderFogParams[2] = m_DynamicState.m_FogZ;
			m_VertexShaderFogParams[3] = ooFogRange;

			float fogParams[4];
			fogParams[0] = ooFogRange * m_VertexShaderFogParams[1];
			fogParams[1] = 1.0f;

			float clipDirection[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			switch( m_DynamicState.m_HeightClipMode )
			{
				case MATERIAL_HEIGHTCLIPMODE_DISABLE:
					clipDirection[0] = 1.0f;
					fogParams[2] = ( float )COORD_EXTENT;
					break;
				case MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT:
					clipDirection[0] = -1.0f;
					fogParams[2] = m_DynamicState.m_HeightClipZ;
					break;
				case MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT:
					clipDirection[0] = 1.0f;
					fogParams[2] = m_DynamicState.m_HeightClipZ;
					break;

				NO_DEFAULT;
				//default:
				//	Assert( 0 );
				//	break;
			}
		
			// $const = $cClipDirection * $cHeightClipZ;
			clipDirection[1] = clipDirection[0] * fogParams[2];
			
			fogParams[3] = m_VertexShaderFogParams[3];
			float vertexShaderCameraPos[4];
			vertexShaderCameraPos[0] = m_WorldSpaceCameraPositon[0];
			vertexShaderCameraPos[1] = m_WorldSpaceCameraPositon[1];
			vertexShaderCameraPos[2] = m_WorldSpaceCameraPositon[2];
			vertexShaderCameraPos[3] = m_DynamicState.m_FogZ;  // waterheight

			// cFogEndOverFogRange, cFogOne, cHeightClipZ, cOOFogRange
			SetVertexShaderConstant( VERTEX_SHADER_FOG_PARAMS, fogParams, 1 );

			// cClipDirection, unused, unused, unused
			SetVertexShaderConstant( VERTEX_SHADER_CLIP_DIRECTION, clipDirection, 1 );

			// eyepos.x eyepos.y eyepos.z cWaterZ
			SetVertexShaderConstant( VERTEX_SHADER_CAMERA_POS, vertexShaderCameraPos );
		}
	}

	// Compute stats info for a texture
	void ComputeStatsInfo( unsigned int textureIdx, bool isCubeMap );

	// Gets the texture 
	IDirect3DBaseTexture* GetD3DTexture( int idx );

	// For procedural textures
	void AdvanceCurrentCopy( int textureIdx );

	// Finds a texture
	bool FindTexture( int id, int& idx );

	// Deletes a D3D texture
	void DeleteD3DTexture( int idx );

	// Unbinds a texture
	void UnbindTexture( int textureIdx );

	// Releases all D3D textures
	void ReleaseAllTextures();

	// Deletes all textures
	void DeleteAllTextures();

	// Gets the currently modified texture index
	int GetModifyTextureIndex() const;

	// Gets the bind id
	int GetBoundTextureBindId( int textureStage ) const;

	// Sets the texture state
	void SetTextureState( int stage, int textureIdx, bool force = false );

	// Grab/release the render targets
	void AcquireRenderTargets();
	void ReleaseRenderTargets();

	// Gets the texture being modified
	IDirect3DBaseTexture* GetModifyTexture();
	IDirect3DBaseTexture** GetModifyTextureReference();

	// returns true if we're using texture coordinates at a given stage
	bool IsUsingTextureCoordinates( int stage, int flags ) const;

	// Returns true if the board thinks we're generating spheremap coordinates
	bool IsSpheremapRenderStateActive( int stage ) const;

	// Returns true if we're modulating constant color into the vertex color
	bool IsModulatingVertexColor() const;

	// Generates spheremap coordinates
	void GenerateSpheremapCoordinates( CMeshBuilder& builder, int stage );

	// Modulates the vertex color
	void ModulateVertexColor( CMeshBuilder& meshBuilder );

	// Recomputes ambient light cube
	void RecomputeAmbientLightCube( );

	// Debugging spew
	void SpewBoardState();

	// Compute and save the world space camera position.
	void CacheWorldSpaceCameraPosition();

	// Compute and save the projection atrix with polyoffset built in if we need it.
	void CachePolyOffsetProjectionMatrix();

	// Releases/reloads resources when other apps want some memory
	void ReleaseResources();
	void ReacquireResources();

	// Vertex shader helper functions
	int  FindVertexShader( VertexFormat_t fmt, char const* pFileName ) const;
	int  FindPixelShader( char const* pFileName ) const;

	// Returns copies of the front and back buffers
	IDirect3DSurface* GetFrontBufferImage( ImageFormat& format );
	IDirect3DSurface* GetBackBufferImage( ImageFormat& format );

	// Flips an image vertically
	void FlipVertical( int width, int height, ImageFormat format, unsigned char *pData );

	// used to determine if we're deactivated
	bool IsDeactivated() const { return m_OtherAppInitializing || m_DisplayLost; }

	// Commits transforms and lighting
	void CommitStateChanges();

	// Commits transforms that have to be dealt with on a per pass basis (ie. projection matrix for polyoffset)
	void CommitPerPassStateChanges( StateSnapshot_t id );

	// Commits user clip planes
	void CommitUserClipPlanes();

	// transform commit
	bool VertexShaderTransformChanged( int i );
	bool FixedFunctionTransformChanged( int i );

	void UpdateVertexShaderMatrix( int iMatrix );
	void UpdateAllVertexShaderMatrices( void );
	void SetVertexShaderStateSkinningMatrices();
	void CommitVertexShaderTransforms();
	void CommitPerPassVertexShaderTransforms();

	void UpdateFixedFunctionMatrix( int iMatrix );
	void SetFixedFunctionStateSkinningMatrices();
	void CommitFixedFunctionTransforms();
	void CommitPerPassFixedFunctionTransforms();

	void SetSkinningMatrices();
	
	// lighting commit
	bool VertexShaderLightingChanged( int i );
	bool VertexShaderLightingEnableChanged( int i );
	bool FixedFunctionLightingChanged( int i );
	bool FixedFunctionLightingEnableChanged( int i );
	VertexShaderLightTypes_t ComputeLightType( int i ) const;
	void SortLights( int* index );
	void CommitVertexShaderLighting();
	void CommitFixedFunctionLighting();

	// Saves/restores all bind indices...
	void StoreBindIndices( int* pBindId );
	void RestoreBindIndices( int* pBindId );

	// Compute the effective DX support level based on all the other caps
	void ComputeDXSupportLevel();

	// Read dx support levels
	void ReadDXSupportLevels();

	// Gets the hardware-specific info for the current card
	const DeviceSupportLevels_t *GetCardSpecificInfo();

	// Gets the surface associated with a texture (refcount of surface is increased)
	IDirect3DSurface* GetTextureSurface( unsigned int tex );
	IDirect3DSurface* GetDepthTextureSurface( unsigned int tex );

	// Main D3D interface
	IDirect3D*			m_pD3D;
	IDirect3DDevice*	m_pD3DDevice;

	// The HWND
	HWND m_HWnd;
	int	m_nWindowWidth;
	int m_nWindowHeight;

	// "normal" back buffer and depth buffer.  Need to keep this around so that we
	// know what to set the render target to when we are done rendering to a texture.
	IDirect3DSurface *m_pBackBufferSurface;
	IDirect3DSurface *m_pZBufferSurface;
	
	//
	// State needed at the time of rendering (after snapshots have been applied)
	//

	// Interface for the D3DXMatrixStack
	ID3DXMatrixStack*	m_pMatrixStack[NUM_MATRIX_MODES];

	// Current matrix mode
	D3DTRANSFORMSTATETYPE m_MatrixMode;
	int m_CurrStack;

	// The current camera position in world space.
	Vector4D m_WorldSpaceCameraPositon;

	// The current projection matrix with polyoffset baked into it.
	D3DXMATRIX		m_CachedPolyOffsetProjectionMatrix;
	D3DXMATRIX		m_CachedPolyOffsetProjectionMatrixWithClip;
	D3DXMATRIX		m_CachedFastClipProjectionMatrix;

	// The texture stage state that changes frequently
	DynamicState_t m_DynamicState;

	// Render data
	IMeshDX8* m_pRenderMesh;
	IMaterialInternal* m_pMaterial;

	// Members related to capabilities
	HardwareCaps_t m_Caps;

	// Mode info
	MaterialVideoMode_t		m_Mode;
	D3DPRESENT_PARAMETERS	m_PresentParameters;
	int						m_DeviceSupportsCreateQuery;

	int			m_ModeFlags;

	bool		m_OtherAppInitializing;
	bool		m_DisplayLost;
	bool		m_IsResizing;
	bool		m_SoftwareTL;

	// The current view hwnd
	HWND		m_ViewHWnd;

	// Render-to-texture stuff...
	bool		m_UsingTextureRenderTarget;
	int			m_ViewportMaxWidth;
	int			m_ViewportMaxHeight;

	// Which device are we using?
	UINT		m_DisplayAdapter;
	D3DDEVTYPE	m_DeviceType;

	// Ambient cube map ok?
	int m_CachedAmbientLightCube;

	// The current frame
	int m_CurrentFrame;

	// The texture we're currently modifying
	int m_ModifyTextureIdx;
	char m_ModifyTextureLockedLevel;
	unsigned char m_ModifyTextureLockedFace;

	// lightmap texture id range
	int m_FirstLightmapId;
	int m_LastLightmapId;

	inline int GetTextureCount( void )
	{
		return m_Textures.Count();
	}

	inline Texture_t& GetTexture( int index )
	{
		return m_Textures[ index ];
	}

	// Various support levels
	CUtlVector< DeviceSupportLevels_t >	m_DeviceSupportLevels;

	// Stores all textures...
	CUtlVector< Texture_t >	m_Textures;

	// Mesh builder used to modify vertex data
	CMeshBuilder m_ModifyBuilder;

	float			m_VertexShaderFogParams[4];

	// Shadow state transition table
	CTransitionTable m_TransitionTable;
	StateSnapshot_t m_nCurrentSnapshot;

	// Depth test override...
	bool	m_bOverrideMaterialIgnoreZ;
	bool 	m_bIgnoreZValue;

	// Selection name stack
	CUtlStack< int >	m_SelectionNames;
	bool	m_InSelectionMode;
	unsigned int*	m_pSelectionBufferEnd;
	unsigned int*	m_pSelectionBuffer;
	unsigned int*	m_pCurrSelectionRecord;
	float	m_SelectionMinZ;
	float	m_SelectionMaxZ;
	int		m_NumHits;

	// Vendor + device id
	int m_nVendorId;
	int m_nDeviceId;
	GUID m_DeviceGUID;

	// fog
	unsigned char m_SceneFogColor[3];
	MaterialFogMode_t m_SceneFogMode;

	// Debug font stuff.
	ID3DXFont *m_pDebugFont;
	IDirect3DQuery9 *m_pFrameSyncQueryObject;
};


//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------
static CShaderAPIDX8 g_ShaderAPIDX8;
IShaderAPIDX8 *g_pShaderAPIDX8 = &g_ShaderAPIDX8;
IMaterialSystemHardwareConfig *g_pHardwareConfig = &g_ShaderAPIDX8;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIDX8, IShaderAPI, 
									SHADERAPI_INTERFACE_VERSION, g_ShaderAPIDX8 )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIDX8, IMaterialSystemHardwareConfig, 
				MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, g_ShaderAPIDX8 )


//-----------------------------------------------------------------------------
// Stats interface
//-----------------------------------------------------------------------------
static CMaterialSystemStatsDX8 g_Stats;
CMaterialSystemStatsDX8 *g_pStats = &g_Stats;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CMaterialSystemStatsDX8, IMaterialSystemStats, 
							MATERIAL_SYSTEM_STATS_INTERFACE_VERSION, g_Stats )


//-----------------------------------------------------------------------------
// Accessors for major interfaces
//-----------------------------------------------------------------------------
IShaderUtil* g_pShaderUtil = 0;
IBaseFileSystem* g_pFileSystem = 0;

IDirect3DDevice* D3DDevice()
{
	return g_ShaderAPIDX8.D3DDevice();
}

IDirect3D* D3D()  
{
	return g_ShaderAPIDX8.D3D();
}

 
//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CShaderAPIDX8::CShaderAPIDX8() : 
	m_pD3D(0), m_pD3DDevice(0), m_Textures( 32 ), m_DisplayLost(false),
	m_CurrStack(-1), m_ModifyTextureIdx(-1), m_pRenderMesh(0), m_OtherAppInitializing(false),
	m_IsResizing(false), m_CurrentFrame(0), m_CachedAmbientLightCube(STATE_CHANGED),
	m_InSelectionMode(false), m_SelectionMinZ(FLT_MAX), m_SelectionMaxZ(FLT_MIN),
	m_pSelectionBuffer(0), m_pSelectionBufferEnd(0), m_ModifyTextureLockedLevel(-1),
	m_pDebugFont( NULL ), m_pBackBufferSurface(0), m_pZBufferSurface(0)
{
	memset( m_pMatrixStack, 0, sizeof(ID3DXMatrixStack*) * NUM_MATRIX_MODES );
	memset( &m_DynamicState, 0, sizeof(m_DynamicState) );
	memset( &m_Mode, 0, sizeof(m_Mode) );
	memset( &m_Caps, 0, sizeof(m_Caps) );
	m_FirstLightmapId = m_LastLightmapId = -1;
	m_Mode.m_Format = (ImageFormat)-1;
	m_DynamicState.m_HeightClipMode = MATERIAL_HEIGHTCLIPMODE_DISABLE;
	m_nWindowHeight = m_nWindowWidth = 0;

	m_SceneFogColor[0] = 0;
	m_SceneFogColor[1] = 0;
	m_SceneFogColor[2] = 0;
	m_SceneFogMode = MATERIAL_FOG_NONE;

	// FIXME: This is kind of a hack to deal with DX8 worldcraft startup.
	// We can at least have this much texture 
	m_Caps.m_MaxTextureWidth = m_Caps.m_MaxTextureHeight = 256;

	// We haven't yet detected whether we support CreateQuery or not yet.
	m_DeviceSupportsCreateQuery = -1;
}

CShaderAPIDX8::~CShaderAPIDX8()
{
	if (m_DynamicState.m_pPixelShaderConstant)
		delete[] m_DynamicState.m_pPixelShaderConstant;
	if (m_DynamicState.m_pVertexShaderConstant)
		delete[] m_DynamicState.m_pVertexShaderConstant;
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::Init( IShaderUtil* pShaderUtil, IBaseFileSystem* pFileSystem, int nAdapter, int nAdapterFlags )
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	m_pFrameSyncQueryObject = NULL;
	
	// So others can access it
	g_pShaderUtil = pShaderUtil;
	g_pFileSystem = pFileSystem;

	// Read dx support levels
	ReadDXSupportLevels();

	// Create the main interface
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D)
        return false;

	// Set up hardware information for this adapter...
	m_DeviceType = (nAdapterFlags & MATERIAL_INIT_REFERENCE_RASTERIZER) ? 
		D3DDEVTYPE_REF : D3DDEVTYPE_HAL;
	m_DisplayAdapter = nAdapter;
	if ( m_DisplayAdapter >= (UINT)GetDisplayAdapterCount() )
	{
		m_DisplayAdapter = 0;
	}

	return DetermineHardwareCaps( );
}


//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void CShaderAPIDX8::Shutdown( )
{
	if (m_pD3D)
	{
		ShutdownDevice();
		m_pD3D->Release();
		m_pD3D = 0;
	}
}


//-----------------------------------------------------------------------------
// Gets the number of adapters...
//-----------------------------------------------------------------------------
int CShaderAPIDX8::GetDisplayAdapterCount() const
{
	Assert( m_pD3D );
	return m_pD3D->GetAdapterCount( );
}


//-----------------------------------------------------------------------------
// Returns info about each adapter
//-----------------------------------------------------------------------------
void CShaderAPIDX8::GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const
{
	Assert( m_pD3D && (adapter < GetDisplayAdapterCount()) );
	
	HRESULT hr;
	D3DADAPTER_IDENTIFIER ident;
	hr = m_pD3D->GetAdapterIdentifier( adapter, D3DENUM_WHQL_LEVEL, &ident );
	Assert( !FAILED(hr) );

	Q_strncpy( info.m_pDriverName, ident.Driver, MATERIAL_ADAPTER_NAME_LENGTH );
	Q_strncpy( info.m_pDriverDescription, ident.Description, MATERIAL_ADAPTER_NAME_LENGTH );
}


//-----------------------------------------------------------------------------
// Returns the number of modes
//-----------------------------------------------------------------------------
int	 CShaderAPIDX8::GetModeCount( int adapter ) const
{
	Assert( m_pD3D && (adapter < GetDisplayAdapterCount()) );
	// fixme - what format should I use here?
	return m_pD3D->GetAdapterModeCount( adapter, D3DFMT_X8R8G8B8 );
}


//-----------------------------------------------------------------------------
// Returns mode information..
//-----------------------------------------------------------------------------
void CShaderAPIDX8::GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const
{
	Assert( m_pD3D && (adapter < GetDisplayAdapterCount()) );
	Assert( mode < GetModeCount( adapter ) );

	HRESULT hr;
	D3DDISPLAYMODE d3dInfo;
	// fixme - what format should I use here?
	hr = m_pD3D->EnumAdapterModes( adapter, D3DFMT_X8R8G8B8, mode, &d3dInfo );
	Assert( !FAILED(hr) );

	info.m_Width = d3dInfo.Width;
	info.m_Height = d3dInfo.Height;
	info.m_Format = D3DFormatToImageFormat( d3dInfo.Format );
	info.m_RefreshRate = d3dInfo.RefreshRate;
}


//-----------------------------------------------------------------------------
// Returns the mode info for the current display device
//-----------------------------------------------------------------------------
void CShaderAPIDX8::GetDisplayMode( MaterialVideoMode_t& info ) const
{
	Assert( m_pD3D );

	HRESULT hr;
	D3DDISPLAYMODE mode;
	hr = m_pD3D->GetAdapterDisplayMode( m_DisplayAdapter, &mode );
	Assert( !FAILED(hr) );

	info.m_Width = mode.Width;
	info.m_Height = mode.Height;
	info.m_Format = D3DFormatToImageFormat( mode.Format );
	info.m_RefreshRate = mode.RefreshRate;
}

bool CShaderAPIDX8::Prefer16BitTextures( void ) const
{
	return ::Prefer16BitTextures();
}


//-----------------------------------------------------------------------------
// Validates the mode...
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::ValidateMode( void* hwnd,
							const MaterialVideoMode_t & mode, int flags ) const
{
	// validate...
	if (m_DisplayAdapter >= (int)m_pD3D->GetAdapterCount())
		return false;

	if (flags & MATERIAL_VIDEO_MODE_WINDOWED)
	{
		// make sure the window fits within the current video mode
		MaterialVideoMode_t displayMode;
		GetDisplayMode( displayMode );

		if ((mode.m_Width >= displayMode.m_Width) ||
			(mode.m_Height >= displayMode.m_Height))
			return false;
	}
	else
	{
		// If the width and height == 0, then we just use whatever the current
		// video mode is
		if ((mode.m_Width == 0) && (mode.m_Height == 0))
			return true;

		// make sure we support that display mode
		MaterialVideoMode_t testMode;
		int count = GetModeCount( m_DisplayAdapter );
		for (int i = 0; i < count; ++i)
		{
			GetModeInfo( m_DisplayAdapter, i, testMode );

			// Found a match, we're ok
			if ((testMode.m_Width == mode.m_Width) &&
				(testMode.m_Height == mode.m_Height) &&
				(testMode.m_Format == mode.m_Format) &&
				((mode.m_RefreshRate == 0) || (testMode.m_RefreshRate == mode.m_RefreshRate)) )
			{
				return true;
			}
		}

		// not found baby
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Do we have to change the device, or can we do something simpler...
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::DoesModeRequireDeviceChange( void* hwnd, 
							const MaterialVideoMode_t &mode, int flags ) const
{
	if (m_HWnd != (HWND)hwnd)
		return true;

	if ((flags & MATERIAL_VIDEO_MODE_RESIZING) != (m_ModeFlags & MATERIAL_VIDEO_MODE_RESIZING))
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Can we render windowed?
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::CanRenderWindowed( ) const
{
	// DX9 only supports dx7 drivers and above.  All existing dx7 can render in windowed mode.
	return true;
}


//-----------------------------------------------------------------------------
// Computes the supersample flags
//-----------------------------------------------------------------------------
static D3DMULTISAMPLE_TYPE ComputeMultisampleType( int nSampleCount )
{
	switch (nSampleCount)
	{
	case 2: return D3DMULTISAMPLE_2_SAMPLES;
	case 3: return D3DMULTISAMPLE_3_SAMPLES;
	case 4: return D3DMULTISAMPLE_4_SAMPLES;
	case 5: return D3DMULTISAMPLE_5_SAMPLES;
	case 6: return D3DMULTISAMPLE_6_SAMPLES;
	case 7: return D3DMULTISAMPLE_7_SAMPLES;
	case 8: return D3DMULTISAMPLE_8_SAMPLES;
	case 9: return D3DMULTISAMPLE_9_SAMPLES;
	case 10: return D3DMULTISAMPLE_10_SAMPLES;
	case 11: return D3DMULTISAMPLE_11_SAMPLES;
	case 12: return D3DMULTISAMPLE_12_SAMPLES;
	case 13: return D3DMULTISAMPLE_13_SAMPLES;
	case 14: return D3DMULTISAMPLE_14_SAMPLES;
	case 15: return D3DMULTISAMPLE_15_SAMPLES;
	case 16: return D3DMULTISAMPLE_16_SAMPLES;

	default:
	case 0:
	case 1:
		return D3DMULTISAMPLE_NONE;
	}
}


//-----------------------------------------------------------------------------
// Sets the present parameters
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetPresentParameters( const MaterialVideoMode_t &mode, int flags, int nSampleCount )
{
	D3DDISPLAYMODE d3ddm;
	HRESULT hr = m_pD3D->GetAdapterDisplayMode( m_DisplayAdapter, &d3ddm );
	if (FAILED(hr))
		return;

	bool windowed = (flags & MATERIAL_VIDEO_MODE_WINDOWED) != 0;
	bool resizing = (flags & MATERIAL_VIDEO_MODE_RESIZING) != 0;

	ZeroMemory( &m_PresentParameters, sizeof(m_PresentParameters) );
	m_PresentParameters.Windowed   = windowed;
	m_PresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
//	m_PresentParameters.SwapEffect = D3DSWAPEFFECT_FLIP;
//	m_PresentParameters.SwapEffect = D3DSWAPEFFECT_COPY;
    m_PresentParameters.EnableAutoDepthStencil = TRUE;
    m_PresentParameters.AutoDepthStencilFormat = FindNearestSupportedDepthFormat( D3DFMT_D24S8 );
	m_PresentParameters.hDeviceWindow = m_HWnd;

	// If we can't use CreateQuery() to check for the end of a frame, then we have to lock the backbuffer
	m_PresentParameters.Flags = 0;

#ifdef D3D_BUG_TRACKED_DOWN
	if( m_DeviceSupportsCreateQuery == 0 )
#else
	// NJS: Until we track down the D3D bug, basically force a lockable backbuffer unless createQuery is already detected.
	if( m_DeviceSupportsCreateQuery != 1 )
#endif
	{
		m_PresentParameters.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	}

	if (!windowed)
	{
		bool useDefault = (mode.m_Width == 0) || (mode.m_Height == 0);
		m_PresentParameters.BackBufferWidth = useDefault ? d3ddm.Width : mode.m_Width;
		m_PresentParameters.BackBufferHeight = useDefault ? d3ddm.Height : mode.m_Height;
		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 && g_pHardwareConfig->SupportsHDR() )
		{
			// hack to get dest alpha
			m_PresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
		}
		else
		{
			m_PresentParameters.BackBufferFormat = useDefault ? d3ddm.Format : ImageFormatToD3DFormat( mode.m_Format );
		}
		m_PresentParameters.BackBufferCount = 1;

		if (flags & MATERIAL_VIDEO_MODE_NO_WAIT_FOR_VSYNC)
			m_PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		else
			m_PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		m_PresentParameters.FullScreen_RefreshRateInHz = useDefault || !mode.m_RefreshRate ? 
			D3DPRESENT_RATE_DEFAULT : mode.m_RefreshRate; 
	}
	else
	{
		// NJS: We are seeing a lot of time spent in present in some cases when this isn't set.
		m_PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		/*
		if (flags & MATERIAL_VIDEO_MODE_NO_WAIT_FOR_VSYNC)
			m_PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		else
			m_PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_TWO;
		*/

		if (resizing)
		{
			// When in resizing windowed mode, 
			// we want to allocate enough memory to deal with any resizing...
			m_PresentParameters.BackBufferWidth = d3ddm.Width;
			m_PresentParameters.BackBufferHeight = d3ddm.Height;
		}
		else
		{
			m_PresentParameters.BackBufferWidth = mode.m_Width;
			m_PresentParameters.BackBufferHeight = mode.m_Height;
		}
		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 && g_pHardwareConfig->SupportsHDR() )
		{
			// hack to get dest alpha
			m_PresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
		}
		else
		{
			m_PresentParameters.BackBufferFormat = d3ddm.Format;
		}
		m_PresentParameters.BackBufferCount = 1;
	}

	if (flags & MATERIAL_VIDEO_MODE_ANTIALIAS)
	{
		DWORD nQualityLevel;
		D3DMULTISAMPLE_TYPE multiSampleType = ComputeMultisampleType( nSampleCount );
		hr = m_pD3D->CheckDeviceMultiSampleType( m_DisplayAdapter, m_DeviceType, 
			m_PresentParameters.BackBufferFormat, m_PresentParameters.Windowed, 
			multiSampleType, &nQualityLevel 
			);

		if (!FAILED(hr))
		{
			m_PresentParameters.MultiSampleType = multiSampleType;
			m_PresentParameters.MultiSampleQuality = nQualityLevel - 1;
		}
	}
}


ImageFormat CShaderAPIDX8::GetBackBufferFormat() const
{
	return D3DFormatToImageFormat( m_PresentParameters.BackBufferFormat );
}

void CShaderAPIDX8::GetBackBufferDimensions( int& width, int& height ) const
{
	width = m_nWindowWidth; //m_PresentParameters.BackBufferWidth;
	height = m_nWindowHeight; //m_PresentParameters.BackBufferHeight;
}


//-----------------------------------------------------------------------------
// Changes the window size
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::ResizeWindow( const MaterialVideoMode_t &mode, int flags, int nSampleCount ) 
{
	// We don't need to do crap if the window was set up to set up
	// to be resizing...
	if (flags & MATERIAL_VIDEO_MODE_RESIZING)
		return false;

	// not fully implemented yet...
	Assert(0);

    // Release all objects in video memory
    ShaderUtil()->ReleaseShaderObjects();

    // Reset the device
	SetPresentParameters( mode, flags, nSampleCount );

	RECORD_COMMAND( DX8_RESET, 1 );
	RECORD_STRUCT( &m_PresentParameters, sizeof(m_PresentParameters) );

    HRESULT hr = m_pD3DDevice->Reset( &m_PresentParameters );
	if (FAILED(hr))
		return false;

	GetDisplayMode( m_Mode );

    // Initialize the app's device-dependent objects
    ShaderUtil()->RestoreShaderObjects();

	return true;
}



//-----------------------------------------------------------------------------
// Methods for interprocess communication to release resources
//-----------------------------------------------------------------------------

#define MATERIAL_SYSTEM_WINDOW_ID		0xFEEDDEAD

#define RELEASE_MESSAGE		0x5E740DE0
#define REACQUIRE_MESSAGE	0x5E740DE1

static BOOL CALLBACK EnumChildWindowsProc( HWND hWnd, LPARAM lParam )
{
	int windowId = GetWindowLong( hWnd, GWL_USERDATA );
	if (windowId == MATERIAL_SYSTEM_WINDOW_ID)
	{
		COPYDATASTRUCT copyData;
		copyData.dwData = lParam;
		copyData.cbData = 0;
		copyData.lpData = 0;
		
		SendMessage(hWnd, WM_COPYDATA, 0, (LPARAM)&copyData);
	}
	return TRUE;
}

static BOOL CALLBACK EnumWindowsProc( HWND hWnd, LPARAM lParam )
{
	EnumChildWindows( hWnd, EnumChildWindowsProc, lParam );
	return TRUE;
}

static void SendIPCMessage( bool release )
{
	// Gotta send this to all windows, since we don't know which ones
	// are material system apps... 
	DWORD msg = release ? RELEASE_MESSAGE : REACQUIRE_MESSAGE;
	EnumWindows( EnumWindowsProc, msg );
}

//-----------------------------------------------------------------------------
// Adds a hook to let us know when other instances are setting the mode
//-----------------------------------------------------------------------------

#ifdef STRICT
#define WINDOW_PROC WNDPROC
#else
#define WINDOW_PROC FARPROC
#endif
 
HWND g_HWndCookie;

static LRESULT CALLBACK ShaderDX8WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	// FIXME: Should these IPC messages tell when an app has focus or not?
	// If so, we'd want to totally disable the shader api layer when an app
	// doesn't have focus.

	// Look for the special IPC message that tells us we're trying to set
	// the mode....
	switch(msg)
	{
	case WM_COPYDATA:
		{
			COPYDATASTRUCT* pData = (COPYDATASTRUCT*)lParam;

			// that number is our magic cookie number
			if (pData->dwData == RELEASE_MESSAGE)
			{
				g_ShaderAPIDX8.OtherAppInitializing(true);
			}
			else if (pData->dwData == REACQUIRE_MESSAGE)  
			{
				g_ShaderAPIDX8.OtherAppInitializing(false);
			}
		}
		break;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

static HWND GetTopmostParentWindow( HWND hWnd )
{
	// Find the parent window...
	HWND hParent = GetParent(hWnd);
	while (hParent)
	{
		hWnd = hParent;
		hParent = GetParent(hWnd);
	}

	return hWnd;
}

static void InstallWindowHook( HWND hWnd )
{
	HWND hParent = GetTopmostParentWindow(hWnd);

	// Attach a child window to the parent; we're gonna store special info there
	// We can't use the USERDATA, cause other apps may want to use this.
	HINSTANCE hInst = (HINSTANCE)GetWindowLong( hParent, GWL_HINSTANCE );
	WNDCLASS		wc;
	memset( &wc, 0, sizeof( wc ) );
    wc.style         = CS_NOCLOSE | CS_PARENTDC;
    wc.lpfnWndProc   = ShaderDX8WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "shaderdx8";

	RegisterClass( &wc );

	// Create the window
	g_HWndCookie = CreateWindow( "shaderdx8", "shaderdx8", WS_CHILD, 
		0, 0, 0, 0, hParent, NULL, hInst, NULL );

	// Marks it as a material system window
	SetWindowLong( g_HWndCookie, GWL_USERDATA, MATERIAL_SYSTEM_WINDOW_ID );
}

static void RemoveWindowHook( HWND hWnd )
{
	if (g_HWndCookie)
	{
		DestroyWindow( g_HWndCookie ); 
		g_HWndCookie = 0;
	}
}


//-----------------------------------------------------------------------------
// Can we download textures?
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::CanDownloadTextures() const
{
	if( IsDeactivated() )
		return false;

	return (m_pD3DDevice != NULL);
}


//-----------------------------------------------------------------------------
// Are we using graphics?
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::IsUsingGraphics() const
{
	return (m_pD3DDevice != NULL);
}


//-----------------------------------------------------------------------------
// Grab/release the render targets
//-----------------------------------------------------------------------------
void CShaderAPIDX8::AcquireRenderTargets()
{
	if (!m_pBackBufferSurface)
	{
		m_pD3DDevice->GetRenderTarget( 0, &m_pBackBufferSurface );
		Assert( m_pBackBufferSurface );
	}

	if (!m_pZBufferSurface)
	{
		m_pD3DDevice->GetDepthStencilSurface( &m_pZBufferSurface );
		Assert( m_pZBufferSurface );
	}
}

void CShaderAPIDX8::ReleaseRenderTargets()
{
	if (m_pBackBufferSurface)
	{
		m_pBackBufferSurface->Release();
		m_pBackBufferSurface = NULL;
	}

	if (m_pZBufferSurface)
	{
		m_pZBufferSurface->Release();
		m_pZBufferSurface = NULL;
	}
}


void CShaderAPIDX8::AllocFrameSyncObjects( void )
{
	if( m_DeviceSupportsCreateQuery == 0 )
	{
		m_pFrameSyncQueryObject = NULL;
		return;
	}

	// FIXME FIXME FIXME!!!!!  Need to record this.
	HRESULT hr = D3DDevice()->CreateQuery( D3DQUERYTYPE_EVENT, &m_pFrameSyncQueryObject );
	if( hr == D3DERR_NOTAVAILABLE )
	{
		Warning( "D3DQUERYTYPE_EVENT not available on this driver\n" );
		Assert( m_pFrameSyncQueryObject == NULL );
	}
	else
	{
		Assert( hr == D3D_OK );
		Assert( m_pFrameSyncQueryObject );
		m_pFrameSyncQueryObject->Issue( D3DISSUE_END );
	}
}

void CShaderAPIDX8::FreeFrameSyncObjects( void )
{
	// FIXME FIXME FIXME!!!!!  Need to record this.
	if( !m_pFrameSyncQueryObject )
	{
		return;
	}
	m_pFrameSyncQueryObject->Release();
	m_pFrameSyncQueryObject = NULL;
}

//-----------------------------------------------------------------------------
// Initialize, shutdown the Device....
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::InitDevice( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSampleCount )
{
	bool restoreNeeded = false;

	if (m_pD3DDevice)
	{
		ReleaseResources();
		restoreNeeded = true;
	}

	FreeFrameSyncObjects();

	ShutdownDevice();
	
	// windowed
	if (!CreateD3DDevice( (HWND)hwnd, mode, flags, nSampleCount ))
		return false;

	AcquireRenderTargets();

	// Hook up our own windows proc to get at messages to tell us when
	// other instances of the material system are trying to set the mode
	InstallWindowHook( (HWND)hwnd );

	m_Caps.m_TextureMemorySize = ComputeTextureMemorySize( m_DeviceGUID, m_DeviceType );

	CreateMatrixStacks();
	
	// Hide the cursor
	RECORD_COMMAND( DX8_SHOW_CURSOR, 1 );
	RECORD_INT( false );

	m_pD3DDevice->ShowCursor( false );

	// Initialize the shader manager
	ShaderManager()->Init( ( flags & MATERIAL_VIDEO_MODE_PRELOAD_SHADERS ) != 0 );

	// Initialize the shader shadow
	ShaderShadow()->Init();

	// Initialize the mesh manager
	MeshMgr()->Init();

	// Initialize the transition table.
	m_TransitionTable.Init( GetDXSupportLevel() >= 90 );

	// Initialize the render state
	InitRenderState( );

	// Create the ambient texture
	CreateAmbientCubeTexture();

	// Clear the z and color buffers
	ClearBuffers( true, true, -1, -1 );

	if (restoreNeeded)
		ReacquireResources();

	AllocFrameSyncObjects();

#if 0
	// create a surface to copy the backbuffer into.
	D3DSURFACE_DESC backbufferDesc;
	m_pBackBufferSurface->GetDesc( &backbufferDesc );
	HRESULT hr = m_pD3DDevice->CreateTexture( backbufferDesc.Width, backbufferDesc.Height,
		1, 0, backbufferDesc.Format, D3DPOOL_DEFAULT, &m_pFBTexture );
	Assert( !FAILED( hr ) );
#endif
	
	RECORD_COMMAND( DX8_BEGIN_SCENE, 0 );

	m_pD3DDevice->BeginScene();

	return true;
}

void CShaderAPIDX8::ShutdownDevice( )
{
	if (m_pD3DDevice)
	{
		// Deallocate all textures
		DeleteAllTextures();

		// Release render targets
		ReleaseRenderTargets();

		// Free objects that are used for frame syncing.
		FreeFrameSyncObjects();

		for (int i = 0; i < NUM_MATRIX_MODES; ++i)
		{
			if (m_pMatrixStack[i])
			{
				int ref = m_pMatrixStack[i]->Release();
				Assert( ref == 0 );
			}
		}

		// Shutdown the transition table.
		m_TransitionTable.Shutdown();

		MeshMgr()->Shutdown();

		ShaderManager()->Shutdown();

		ReleaseAllVertexDecl( );

		RECORD_COMMAND( DX8_DESTROY_DEVICE, 0 );

		m_pD3DDevice->Release();
#ifdef STUBD3D
		delete ( CStubD3DDevice * )m_pD3DDevice;
#endif
		m_pD3DDevice = 0;

		RemoveWindowHook( m_HWnd );

		m_HWnd = 0;
	}
}


//-----------------------------------------------------------------------------
// Sets the mode...
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::SetMode( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSampleCount )
{
	Assert( m_pD3D );

	// If we can't render windowed, we'll automatically switch to fullscreen...
	if (!CanRenderWindowed())
	{
		flags &= ~(MATERIAL_VIDEO_MODE_WINDOWED	| MATERIAL_VIDEO_MODE_RESIZING);
	}

	if (!ValidateMode( hwnd, mode, flags ))
		return false;

	bool ok;
	if (!DoesModeRequireDeviceChange(hwnd, mode, flags))
	{
		ok = ResizeWindow( mode, flags, nSampleCount );
	}
	else
	{
		ok = InitDevice( hwnd, mode, flags, nSampleCount );
	}

	if (!ok)
		return false;

	if (flags & MATERIAL_VIDEO_MODE_WINDOWED)
	{
		// FIXME: Resize the window...?
		if ((mode.m_Width != 0) && (mode.m_Height != 0))
		{
		/*
			SetWindowPos( m_HWnd, HWND_NOTOPMOST,
						  m_rcWindowBounds.left, m_rcWindowBounds.top,
						  ( m_rcWindowBounds.right - m_rcWindowBounds.left ),
						  ( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
						  SWP_SHOWWINDOW );
		*/
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Find view
//-----------------------------------------------------------------------------

int CShaderAPIDX8::FindView( void* hwnd ) const
{
	/* FIXME: Is this necessary?
	// Look for the view in the list of views
	for (int i = m_Views.Count(); --i >= 0; )
	{
		if (m_Views[i].m_HWnd == (HWND)hwnd)
			return i;
	}
	*/
	return -1;
}

//-----------------------------------------------------------------------------
// Creates a child window
//-----------------------------------------------------------------------------

bool CShaderAPIDX8::AddView( void* hwnd )
{
	/*
	// If we haven't created a device yet
	if (!m_pD3DDevice)
		return false;

	// Make sure no duplicate hwnds...
	if (FindView(hwnd) >= 0)
		return false;

	// In this case, we need to create the device; this is our
	// default swap chain. This here says we're gonna use a part of the
	// existing buffer and just grab that.
	int view = m_Views.AddToTail();
	m_Views[view].m_HWnd = (HWND)hwnd;
//	memcpy( &m_Views[view].m_PresentParamters, m_PresentParameters, sizeof(m_PresentParamters) );

	HRESULT hr;
	hr = m_pD3DDevice->CreateAdditionalSwapChain( &m_PresentParameters,
		&m_Views[view].m_pSwapChain );
	return !FAILED(hr);
	*/

	return true;
}

void CShaderAPIDX8::RemoveView( void* hwnd )
{
	/*
	// Look for the view in the list of views
	int i = FindView(hwnd);
	if (i >= 0)
	{
// FIXME		m_Views[i].m_pSwapChain->Release();
		m_Views.FastRemove(i);
	}
	*/
}

//-----------------------------------------------------------------------------
// Activates a child window
//-----------------------------------------------------------------------------

void CShaderAPIDX8::SetView( void* hwnd )
{
	int x, y, width, height;
	GetViewport( x, y, width, height );

	// Get the window (*not* client) rect of the view window
	m_ViewHWnd = (HWND)hwnd;
	GetWindowSize( m_nWindowWidth, m_nWindowHeight );

	// Reset the viewport (takes into account the view rect)
	// Don't need to set the viewport if it's not ready
	Viewport( x, y, width, height );
}


//-----------------------------------------------------------------------------
// Computes device creation paramters
//-----------------------------------------------------------------------------

static DWORD ComputeDeviceCreationFlags( D3DCAPS& caps, D3DADAPTER_IDENTIFIER& ident, int flags )
{
	// Find out what type of device to make
	bool pureDeviceSupported = (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) != 0;
	bool hardwareVertexProcessing = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;

	if (flags & MATERIAL_VIDEO_MODE_SOFTWARE_TL)
		hardwareVertexProcessing = false;

	DWORD deviceCreationFlags;
	if (hardwareVertexProcessing)
	{
		deviceCreationFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
		if (pureDeviceSupported)
			deviceCreationFlags |= D3DCREATE_PUREDEVICE;
	}
	else
	{
		deviceCreationFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}
	deviceCreationFlags |= D3DCREATE_FPU_PRESERVE;

	return deviceCreationFlags;
}


//-----------------------------------------------------------------------------
// Creates the D3D Device
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::CreateD3DDevice( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSampleCount )
{
	// Get some caps....
	HRESULT hr;
	D3DCAPS caps;
	D3DADAPTER_IDENTIFIER ident;

	hr = m_pD3D->GetDeviceCaps( m_DisplayAdapter, m_DeviceType, &caps );
	if (FAILED(hr))
		return false;

	hr = m_pD3D->GetAdapterIdentifier( m_DisplayAdapter, 0, &ident );
	if (FAILED(hr))
		return false;

	m_HWnd = (HWND)hwnd;
	m_DisplayLost = false;
	m_ModeFlags = flags;
	m_SoftwareTL = (flags & MATERIAL_VIDEO_MODE_SOFTWARE_TL) != 0;

	// What texture formats do we support?
	GetDisplayMode( m_Mode );
    InitializeColorInformation( m_DisplayAdapter, m_DeviceType, m_Mode.m_Format );
	if ( D3DSupportsCompressedTextures() )
		m_Caps.m_SupportsCompressedTextures = COMPRESSED_TEXTURES_ON;
	else
		m_Caps.m_SupportsCompressedTextures = COMPRESSED_TEXTURES_OFF;

	DWORD deviceCreationFlags = ComputeDeviceCreationFlags( caps, ident, flags );
	SetPresentParameters( mode, flags, nSampleCount );

	// Create that device!
	RECT windowRect;
	GetClientRect( m_HWnd, &windowRect );

	RECORD_COMMAND( DX8_CREATE_DEVICE, 6 );
	RECORD_INT( windowRect.right - windowRect.left );
	RECORD_INT( windowRect.bottom - windowRect.top );
	RECORD_INT( m_DisplayAdapter );
	RECORD_INT( m_DeviceType );
	RECORD_INT( deviceCreationFlags );
	RECORD_STRUCT( &m_PresentParameters, sizeof(m_PresentParameters) );
	
	// Tell all other instances of the material system to let go of memory
	SendIPCMessage( true );

	IDirect3DDevice *pD3DDevice = NULL;
	hr = m_pD3D->CreateDevice( m_DisplayAdapter, m_DeviceType,
		m_HWnd, deviceCreationFlags, &m_PresentParameters, &pD3DDevice );

	if (FAILED(hr))
	{
		// try again, other applications may be taking their time
		Sleep(1000);
		hr = m_pD3D->CreateDevice( m_DisplayAdapter, m_DeviceType,
			m_HWnd, deviceCreationFlags, &m_PresentParameters, &pD3DDevice );

		// in this case, we actually are allocating too much memory....
		// This will cause us to use less buffers...
		if (FAILED(hr) && m_PresentParameters.Windowed)
		{
			m_PresentParameters.SwapEffect = D3DSWAPEFFECT_COPY; 
			m_PresentParameters.BackBufferCount = 0;
			hr = m_pD3D->CreateDevice( m_DisplayAdapter, m_DeviceType,
				m_HWnd, deviceCreationFlags, &m_PresentParameters, &pD3DDevice );
		}
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Do I need to detect whether this device supports CreateQuery before creating it?
	if( m_DeviceSupportsCreateQuery == -1 )
	{
		IDirect3DQuery9 *pQueryObject = NULL;

		// Detect whether query is supported by creating and releasing:
		HRESULT hr = pD3DDevice->CreateQuery( D3DQUERYTYPE_EVENT, &pQueryObject );
		
		if( pQueryObject ) 
		{
			pQueryObject->Release();
			m_DeviceSupportsCreateQuery = 1;
		} else
		{
			m_DeviceSupportsCreateQuery = 0;
		}

		// If it's not available, revert to z buffer locking by recreating the device

		// NJS: Note: What I've found is that if you create a d3d device, destroy it, 
		// and create another D3D device immedietely after (or even waiting for a while)
		// that in fullscreen, the new device always returns device lost, even after
		// being restored.  For now, we get around this by defaulting the initial device
		// to contain a lockable z buffer.
#ifdef D3D_BUG_TRACKED_DOWN
		if( FAILED(hr) )
		{
			m_DeviceSupportsCreateQuery = 0;
			Msg( "Using Lockable Back Buffer.\n" );
			//pD3DDevice->Reset( &m_PresentParameters );
			pD3DDevice->Release();

			// Give the system a little time to recover:
			
			Sleep(1000);

			{
				DWORD deviceCreationFlags = ComputeDeviceCreationFlags( caps, ident, flags );
				SetPresentParameters( mode, flags, nSampleCount );

				hr = m_pD3D->CreateDevice( m_DisplayAdapter, m_DeviceType,
					m_HWnd, deviceCreationFlags, &m_PresentParameters, &pD3DDevice );

				if (FAILED(hr))
				{
					// try again, other applications may be taking their time
					Sleep(1000);
					hr = m_pD3D->CreateDevice( m_DisplayAdapter, m_DeviceType,
						m_HWnd, deviceCreationFlags, &m_PresentParameters, &pD3DDevice );

					// in this case, we actually are allocating too much memory....
					// This will cause us to use less buffers...
					if (FAILED(hr) && m_PresentParameters.Windowed)
					{
						m_PresentParameters.SwapEffect = D3DSWAPEFFECT_COPY; 
						m_PresentParameters.BackBufferCount = 0;
						hr = m_pD3D->CreateDevice( m_DisplayAdapter, m_DeviceType,
							m_HWnd, deviceCreationFlags, &m_PresentParameters, &pD3DDevice );
					}

					if (FAILED(hr))
					{
						return false;
					}
				}
			}
			

		} 
		else
		{
			Msg( "Using CreateQuery.\n" );

			m_DeviceSupportsCreateQuery = 1;
		}
#endif

	}

#ifdef STUBD3D
	m_pD3DDevice = new CStubD3DDevice( pD3DDevice, g_pFileSystem );
#else
	m_pD3DDevice = pD3DDevice;
#endif
	
//	CheckDeviceLost();

	// Tell all other instances of the material system it's ok to grab memory
	SendIPCMessage( false );

	m_IsResizing = ((flags & MATERIAL_VIDEO_MODE_WINDOWED) != 0) &&
					((flags & MATERIAL_VIDEO_MODE_RESIZING) != 0);

	// This is our current view.
	SetView( m_HWnd );

	if( flags & MATERIAL_VIDEO_MODE_QUIT_AFTER_PRELOAD )
	{
		exit( 0 );
	}

	return !FAILED(hr);
}


//-----------------------------------------------------------------------------
// Gets the window size
//-----------------------------------------------------------------------------
void CShaderAPIDX8::GetWindowSize( int& width, int& height ) const
{
	RECT rect;
	GetClientRect( m_HWnd, &rect );
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
}


//-----------------------------------------------------------------------------
// Creates the matrix stack
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CreateMatrixStacks()
{
	BEGIN_D3DX_ALLOCATION();

	for (int i = 0; i < NUM_MATRIX_MODES; ++i)
	{
		HRESULT hr = D3DXCreateMatrixStack( 0, &m_pMatrixStack[i] );
		Assert( hr == D3D_OK );
	}

	END_D3DX_ALLOCATION();
}



//-----------------------------------------------------------------------------
// Read dx support levels
//-----------------------------------------------------------------------------
static inline int ReadHexValue( KeyValues *pVal, const char *pName )
{
	const char *pString = pVal->GetString( pName, NULL );
	if (!pString)
	{
		return -1;
	}

	char *pTemp;
	int nVal = strtol( pString, &pTemp, 16 );
	return (pTemp != pString) ? nVal : -1;
}


//-----------------------------------------------------------------------------
// Read dx support levels
//-----------------------------------------------------------------------------
#define SUPPORT_CFG_FILE "dxsupport.cfg"

void CShaderAPIDX8::ReadDXSupportLevels()
{
	m_DeviceSupportLevels.RemoveAll();

	if( CommandLine()->CheckParm( "-ignoredxsupportcfg" ) )
	{
		return;
	}
	
	KeyValues * cfg = new KeyValues("dxsupport");
	if (!cfg->LoadFromFile( g_pFileSystem, SUPPORT_CFG_FILE, "EXECUTABLE_PATH" ))
		return;

	KeyValues *pGroup = cfg->GetFirstSubKey();
	for( pGroup = cfg->GetFirstSubKey(); pGroup; pGroup = pGroup->GetNextKey() )
	{
		DeviceSupportLevels_t info;
		info.m_nVendorId = ReadHexValue( pGroup, "VendorID" );
		if ( info.m_nVendorId == -1 )
			continue;

		info.m_nDeviceIdMin = ReadHexValue( pGroup, "MinDeviceID" );
		if ( info.m_nDeviceIdMin == -1 )
			continue;

		info.m_nDeviceIdMax = ReadHexValue( pGroup, "MaxDeviceID" );
		if ( info.m_nDeviceIdMax == -1 )
			continue;

		info.m_nDXSupportLevel = pGroup->GetInt( "DXLevel", 0 );
		const char *pShaderDLL = pGroup->GetString( "ShaderDLL", NULL );
		if ( pShaderDLL )
		{
			Q_strncpy( info.m_pShaderDLL, pShaderDLL, 32 );
		}
		else
		{
			info.m_pShaderDLL[0] = 0;
		}

		info.m_bFastZReject = pGroup->GetInt( "FastZReject", 0 ) != 0;
		info.m_bFastClipping = pGroup->GetInt( "FastClip", 0 ) != 0;
		info.m_nCreateQuery = pGroup->GetInt( "CreateQuery", -1 ) != 0;
		info.m_bNeedsATICentroidHack = pGroup->GetInt( "CentroidHack", 0 ) != 0;

		m_DeviceSupportLevels.AddToTail( info );
	}

	cfg->deleteThis();
}


  
//-----------------------------------------------------------------------------
// Gets the hardware-specific info for the current card
//-----------------------------------------------------------------------------
const CShaderAPIDX8::DeviceSupportLevels_t *CShaderAPIDX8::GetCardSpecificInfo()
{
	int nCount = m_DeviceSupportLevels.Count();
	for ( int i = 0; i < nCount; ++ i)
	{
		const DeviceSupportLevels_t &info = m_DeviceSupportLevels[i];
		if ( ( info.m_nVendorId == m_nVendorId ) &&
			 ( info.m_nDeviceIdMax >= m_nDeviceId ) &&
			 ( info.m_nDeviceIdMin <= m_nDeviceId ) )
		{
			return &info;
		}
	}

	return NULL;
}



//-----------------------------------------------------------------------------
// Compute the effective DX support level based on all the other caps
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ComputeDXSupportLevel()
{
	// NOTE: Support level is actually DX level * 10 + subversion
	// So, 70 = DX7, 80 = DX8, 81 = DX8 w/ 1.4 pixel shaders, etc.
	// NOTE: 82 = NVidia nv3x cards, which can't run dx9 fast	
	const CPUInformation& pi = GetCPUInformation();
	double CPUSpeedMhz = pi.m_Speed / 1000000.0;

	// FIXME: Improve this!! There should be a whole list of features
	// we require in order to be considered a DX7 board, DX8 board, etc.
	if ( m_Caps.m_SupportsVertexShaders_2_0 && m_Caps.m_SupportsPixelShaders_2_0 && (m_Caps.m_MaxUserClipPlanes > 0) )
	{
		// DX9 requires a 1.4Ghz CPU or higher: (some slack provided)
		if( CPUSpeedMhz >= 1350 )
		{
			m_Caps.m_DXSupportLevel = 90;
			return;
		}

		Msg("Graphics Subsystem Supports DX9, but processor is too slow.");
	}

	if (m_Caps.m_SupportsPixelShaders && m_Caps.m_SupportsVertexShaders)
	{
		// DX8 requires an 866Mhz or higher CPU: (some slack provided)
		if( CPUSpeedMhz >= 850 )
		{
			if (m_Caps.m_SupportsPixelShaders_1_4)
			{
				m_Caps.m_DXSupportLevel = 81;
				return;
			}

			m_Caps.m_DXSupportLevel = 80;
			return;
		}

		Msg("Graphics Card Supports DX8, but processor is too slow.");
	}

	if (
		m_Caps.m_SupportsHardwareLighting && 
		m_Caps.m_SupportsCubeMaps &&
		(m_Caps.m_MaxBlendMatrices >= 2)
		)
	{
		if (m_Caps.m_MaxBlendMatrices >= 3)
		{
			m_Caps.m_DXSupportLevel = 71;
			return;
		}

		m_Caps.m_DXSupportLevel = 70;
		return;
	}

	// FIXME:  Why would we require compressed textures for dx6?
	if (
//		m_Caps.m_SupportsCompressedTextures && 
		(m_Caps.m_NumTextureUnits >= 2) &&
		m_Caps.m_SupportsMipmapping
		)
	{
		m_Caps.m_DXSupportLevel = 60;
		return;
	}

	Assert( 0 ); // we don't support this!
	m_Caps.m_DXSupportLevel = 50;
}


// Use this when recording *.rec files that need to be run on more than one kind of 
// hardware, etc.
//#define DX8_COMPATABILITY_MODE

//-----------------------------------------------------------------------------
// Determine capabilities
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::DetermineHardwareCaps( )
{
	// NOTE: When getting the caps, we want to be limited by the hardware
	// even if we're running with software T&L...
	D3DCAPS caps;
	D3DADAPTER_IDENTIFIER ident;
	HRESULT hr;
	hr = m_pD3D->GetDeviceCaps( m_DisplayAdapter, m_DeviceType, &caps );
	if ( FAILED(hr) )
		return false;

	hr = m_pD3D->GetAdapterIdentifier( m_DisplayAdapter, 0, &ident );
	if ( FAILED(hr) )
		return false;

	m_nVendorId = ident.VendorId;
	m_nDeviceId = ident.DeviceId;
	m_DeviceGUID = ident.DeviceIdentifier;

	m_Caps.m_pShaderDLL[0] = 0;

	// NOTE: Normally, the number of texture units == the number of texture
	// stages except for NVidia hardware, which reports more stages than units.
	// The reason for this is because they expose the inner hardware pixel
	// pipeline through the extra stages. The only thing we use stages for
	// in the hardware is for configuring the color + alpha args + ops.
	m_Caps.m_NumTextureUnits = caps.MaxSimultaneousTextures;
	m_Caps.m_NumTextureStages = caps.MaxTextureBlendStages;
	if (m_Caps.m_NumTextureUnits > NUM_SIMULTANEOUS_TEXTURES)
		m_Caps.m_NumTextureUnits = NUM_SIMULTANEOUS_TEXTURES;
	if (m_Caps.m_NumTextureStages > NUM_TEXTURE_STAGES)
		m_Caps.m_NumTextureStages = NUM_TEXTURE_STAGES;

	m_Caps.m_PreferDynamicTextures = ( caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES ) ? 1 : 0;

	m_Caps.m_HasProjectedBumpEnv = ( caps.TextureCaps & D3DPTEXTURECAPS_NOPROJECTEDBUMPENV ) == 0;

	m_Caps.m_HasSetDeviceGammaRamp = (caps.Caps2 & D3DCAPS2_CANCALIBRATEGAMMA) != 0;
	m_Caps.m_SupportsVertexShaders = ((caps.VertexShaderVersion >> 8) & 0xFF) >= 1;
	m_Caps.m_SupportsPixelShaders = ((caps.PixelShaderVersion >> 8) & 0xFF) >= 1;

#ifdef DX8_COMPATABILITY_MODE
	m_Caps.m_SupportsPixelShaders_1_4 = false;
	m_Caps.m_SupportsPixelShaders_2_0 = false;
	m_Caps.m_SupportsVertexShaders_2_0 = false;
	m_Caps.m_SupportsMipmappedCubemaps = false;
#else
	m_Caps.m_SupportsPixelShaders_1_4 = (caps.PixelShaderVersion & 0xffff) >= 0x0104;
	m_Caps.m_SupportsPixelShaders_2_0 = (caps.PixelShaderVersion & 0xffff) >= 0x0200;
	m_Caps.m_SupportsVertexShaders_2_0 = ( caps.VertexShaderVersion & 0xffff ) >= 0x0200;
	m_Caps.m_SupportsMipmappedCubemaps = ( caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP ) ? true : false;
#endif
	if ( D3DSupportsCompressedTextures() )
		m_Caps.m_SupportsCompressedTextures = COMPRESSED_TEXTURES_ON;
	else
		m_Caps.m_SupportsCompressedTextures = COMPRESSED_TEXTURES_OFF;
	m_Caps.m_bSupportsAnisotropicFiltering = (caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0;
//		(caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC);
	m_Caps.m_nMaxAnisotropy = m_Caps.m_bSupportsAnisotropicFiltering ? caps.MaxAnisotropy : 0; 

	m_Caps.m_SupportsCubeMaps = ( caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP ) ? true : false;
	m_Caps.m_SupportsNonPow2Textures = ( caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL ) ? true : false;

	// garymcthack - debug runtime doesn't let you use more than 96 constants.
	// NEED TO TEST THIS ON DX9 TO SEE IF IT IS FIXED!
	// NOTE: Initting more constants than we are ever going to use may cause the 
	// driver to try to keep track of them.. I'm forcing this to 96 so that this doesn't happen.
#if 0
	m_Caps.m_NumVertexShaderConstants = caps.MaxVertexShaderConst;
#else
	m_Caps.m_NumVertexShaderConstants = ( caps.MaxVertexShaderConst > 100 ) ? 100 : 96;
#endif

	if( m_Caps.m_SupportsPixelShaders )
	{
		if( m_Caps.m_SupportsPixelShaders_2_0 )
		{
			m_Caps.m_NumPixelShaderConstants = 32;
		}
		else
		{
			m_Caps.m_NumPixelShaderConstants = 8;
		}
	}
	else
	{
		m_Caps.m_NumPixelShaderConstants = 0;
	}

	m_Caps.m_SupportsHardwareLighting = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
	m_Caps.m_MaxNumLights = caps.MaxActiveLights;
	if (m_Caps.m_MaxNumLights > MAX_NUM_LIGHTS)
		m_Caps.m_MaxNumLights = MAX_NUM_LIGHTS;

	m_Caps.m_MaxTextureWidth = caps.MaxTextureWidth;
	m_Caps.m_MaxTextureHeight = caps.MaxTextureHeight;
	m_Caps.m_MaxTextureAspectRatio = caps.MaxTextureAspectRatio;
	if (m_Caps.m_MaxTextureAspectRatio == 0)
		m_Caps.m_MaxTextureAspectRatio = max( m_Caps.m_MaxTextureWidth, m_Caps.m_MaxTextureHeight);
	m_Caps.m_MaxPrimitiveCount = caps.MaxPrimitiveCount;
	m_Caps.m_MaxBlendMatrices = caps.MaxVertexBlendMatrices;
	m_Caps.m_MaxBlendMatrixIndices = caps.MaxVertexBlendMatrixIndex;

	m_Caps.m_MaxVertexShaderBlendMatrices = (caps.MaxVertexShaderConst - VERTEX_SHADER_MODEL) / 3;
	if (m_Caps.m_MaxVertexShaderBlendMatrices > NUM_MODEL_TRANSFORMS)
		m_Caps.m_MaxVertexShaderBlendMatrices = NUM_MODEL_TRANSFORMS;

	bool addSupported = (caps.TextureOpCaps & D3DTEXOPCAPS_ADD) != 0;
	bool modSupported = (caps.TextureOpCaps & D3DTEXOPCAPS_MODULATE2X) != 0;

	// FIXME: What's the default value for this?
	m_Caps.m_bFastZReject = false;

	m_Caps.m_bNeedsATICentroidHack = false;

	m_Caps.m_SupportsMipmapping = true;
	m_Caps.m_SupportsOverbright = true;

	// Thank you to all you driver writers who actually correctly return caps
	if ((!modSupported) || (!addSupported))
	{
		m_Caps.m_SupportsOverbright = false;
	}
 
	// Some cards, like ATI Rage Pro doesn't support mipmapping (looks like ass)
	// NOTE: This is necessary because ATI assumes that the wrap/clamp
	// mode is shared among all texture units
	if (Q_stristr(ident.Description, "Rage Pro"))
	{
		m_Caps.m_SupportsMipmapping = false;
		m_Caps.m_NumTextureUnits = 1;
	}

	// Voodoo 3 doesn't support mod 2x (although caps says it does!)
	// Same is true of the G400
	if ((Q_stristr(ident.Description, "Voodoo 3")) ||
		(Q_stristr(ident.Description, "G400")))
	{
		m_Caps.m_SupportsOverbright = false;
	}

	// Check if ZBias is supported...
	m_Caps.m_ZBiasSupported = (caps.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS) != 0;
	m_Caps.m_SlopeScaledDepthBiasSupported = (caps.RasterCaps & D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS ) != 0;

	// Spheremapping supported?
	m_Caps.m_bSupportsSpheremapping = (caps.VertexProcessingCaps & D3DVTXPCAPS_TEXGEN_SPHEREMAP) != 0;

	// How many user clip planes?
	m_Caps.m_MaxUserClipPlanes = caps.MaxUserClipPlanes;
	if( m_Caps.m_MaxUserClipPlanes > MAXUSERCLIPPLANES )
	{
		m_Caps.m_MaxUserClipPlanes = MAXUSERCLIPPLANES;
	}

	m_Caps.m_UseFastClipping = false;

	if( m_Caps.m_MaxUserClipPlanes == 0 )
	{
		m_Caps.m_UseFastClipping = true;
	}

	// GR - query for SRGB support as needed for our DX 9 stuff
	m_Caps.m_SupportsSRGB = ( m_pD3D->CheckDeviceFormat( m_DisplayAdapter, m_DeviceType,
		D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_SRGBREAD | D3DUSAGE_QUERY_SRGBWRITE,
		D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8 ) == S_OK);

	// GR - check for support of 16-bit per channel textures
	m_Caps.m_SupportsFatTextures = ( m_pD3D->CheckDeviceFormat( m_DisplayAdapter, m_DeviceType,
		D3DFMT_X8R8G8B8, 0,
		D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16 ) == S_OK);

	// GR - check if HDR rendering is supported
	m_Caps.m_SupportsHDR = m_Caps.m_SupportsPixelShaders_2_0 &&
		m_Caps.m_SupportsVertexShaders_2_0 &&
		m_Caps.m_SupportsFatTextures &&
		(caps.Caps3 & D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD) &&
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) &&
		m_Caps.m_SupportsSRGB;
	
	// Compute the effective DX support level based on all the other caps
	ComputeDXSupportLevel();

	// Hardware-specific overrides
	int nForcedDXLevel = CommandLine()->ParmValue( "-dxlevel", 0 );
	const DeviceSupportLevels_t *pInfo = GetCardSpecificInfo();
	if ( pInfo )
	{
		m_Caps.m_UseFastClipping = pInfo->m_bFastClipping;
		m_Caps.m_bFastZReject = pInfo->m_bFastZReject;
		m_Caps.m_bNeedsATICentroidHack = pInfo->m_bNeedsATICentroidHack;
		m_DeviceSupportsCreateQuery = pInfo->m_nCreateQuery;

		strcpy( m_Caps.m_pShaderDLL, pInfo->m_pShaderDLL );
		if( nForcedDXLevel > 0 )
		{
			m_Caps.m_DXSupportLevel = max( nForcedDXLevel, m_Caps.m_DXSupportLevel );
		}
		else if ( pInfo->m_nDXSupportLevel > 0 )
		{
			m_Caps.m_DXSupportLevel = pInfo->m_nDXSupportLevel; 
		}
	}

	// HACK HACK HACK!!
	// If we aren't using stdshader_hdr_dx9.dll, don't bother trying to use hdr.
	const char *pParam = CommandLine()->ParmValue( "-shader" );
	if( !pParam || !Q_stristr( pParam, "_hdr_" ) )
	{
		m_Caps.m_SupportsHDR = false;
	}
	return true;
}							   


void CShaderAPIDX8::Get3DDriverInfo( Material3DDriverInfo_t &info ) const
{
	HRESULT hr;
	D3DCAPS caps;
	D3DADAPTER_IDENTIFIER ident;
	
	RECORD_COMMAND( DX8_GET_DEVICE_CAPS, 0 );

	RECORD_COMMAND( DX8_GET_ADAPTER_IDENTIFIER, 2 );
	RECORD_INT( m_DisplayAdapter );
	RECORD_INT( 0 );

	hr = m_pD3DDevice->GetDeviceCaps( &caps );
	hr = m_pD3D->GetAdapterIdentifier( m_DisplayAdapter, D3DENUM_WHQL_LEVEL, &ident );

	Q_strncpy( info.m_pDriverName, ident.Driver, MATERIAL_ADAPTER_NAME_LENGTH );
	Q_strncpy( info.m_pDriverDescription, ident.Description, MATERIAL_ADAPTER_NAME_LENGTH );
	Q_snprintf( info.m_pDriverVersion, MATERIAL_ADAPTER_NAME_LENGTH,
		"%ld.%ld.%ld.%ld", 
		( long )( ident.DriverVersion.HighPart>>16 ), 
		( long )( ident.DriverVersion.HighPart & 0xffff ), 
		( long )( ident.DriverVersion.LowPart>>16 ), 
		( long )( ident.DriverVersion.LowPart & 0xffff ) );
	info.m_VendorID = ident.VendorId;
	info.m_DeviceID = ident.DeviceId;
	info.m_SubSysID = ident.SubSysId;
	info.m_Revision = ident.Revision;
	info.m_WHQLLevel = ident.WHQLLevel;
}


//-----------------------------------------------------------------------------
// Use this to spew information about the 3D layer 
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SpewDriverInfo() const
{
	HRESULT hr;
	D3DCAPS caps;
	D3DADAPTER_IDENTIFIER ident;
	
	RECORD_COMMAND( DX8_GET_DEVICE_CAPS, 0 );

	RECORD_COMMAND( DX8_GET_ADAPTER_IDENTIFIER, 2 );
	RECORD_INT( m_DisplayAdapter );
	RECORD_INT( 0 );

	hr = m_pD3DDevice->GetDeviceCaps( &caps );
	hr = m_pD3D->GetAdapterIdentifier( m_DisplayAdapter, D3DENUM_WHQL_LEVEL, &ident );

	Warning("Shader API DX8 Driver Info:\n\nDriver : %s Version : %d\n", 
		ident.Driver, ident.DriverVersion );
	Warning("Driver Description :  %s\n", ident.Description );
	Warning("Chipset version %d %d %d %d\n\n", 
		ident.VendorId, ident.DeviceId, ident.SubSysId, ident.Revision );

	Warning("Display mode : %d x %d (%s)\n", 
		m_Mode.m_Width, m_Mode.m_Height, ShaderUtil()->ImageFormatInfo(m_Mode.m_Format).m_pName );
	Warning("Vertex Shader Version : %d.%d Pixel Shader Version : %d.%d\n",
		(caps.VertexShaderVersion >> 8) && 0xFF, caps.VertexShaderVersion & 0xFF,
		(caps.PixelShaderVersion >> 8) & 0xFF, caps.PixelShaderVersion & 0xFF);
	Warning("\nDevice Caps :\n");
	Warning("CANBLTSYSTONONLOCAL %s CANRENDERAFTERFLIP %s HWRASTERIZATION %s\n",
		(caps.DevCaps & D3DDEVCAPS_CANBLTSYSTONONLOCAL) ? " Y " : " N ",
		(caps.DevCaps & D3DDEVCAPS_CANRENDERAFTERFLIP) ? " Y " : " N ",
		(caps.DevCaps & D3DDEVCAPS_HWRASTERIZATION) ? " Y " : "*N*" );
	Warning("HWTRANSFORMANDLIGHT %s NPATCHES %s PUREDEVICE %s\n",
		(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ? " Y " : " N ",
		(caps.DevCaps & D3DDEVCAPS_NPATCHES) ? " Y " : " N ",
		(caps.DevCaps & D3DDEVCAPS_PUREDEVICE) ? " Y " : " N " );
	Warning("SEPARATETEXTUREMEMORIES %s TEXTURENONLOCALVIDMEM %s TEXTURESYSTEMMEMORY %s\n",
		(caps.DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES) ? "*Y*" : " N ",
		(caps.DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) ? " Y " : " N ",
		(caps.DevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) ? " Y " : " N " );
	Warning("TEXTUREVIDEOMEMORY %s TLVERTEXSYSTEMMEMORY %s TLVERTEXVIDEOMEMORY %s\n",
		(caps.DevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) ? " Y " : "*N*",
		(caps.DevCaps & D3DDEVCAPS_TLVERTEXSYSTEMMEMORY) ? " Y " : "*N*",
		(caps.DevCaps & D3DDEVCAPS_TLVERTEXVIDEOMEMORY) ? " Y " : " N " );

	Warning("\nPrimitive Caps :\n");
	Warning("BLENDOP %s CLIPPLANESCALEDPOINTS %s CLIPTLVERTS %s\n",
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_BLENDOP) ? " Y " : " N ",
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPPLANESCALEDPOINTS) ? " Y " : " N ",
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS) ? " Y " : " N " );
	Warning("COLORWRITEENABLE %s MASKZ %s TSSARGTEMP %s\n",
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE) ? " Y " : " N ",
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_MASKZ) ? " Y " : "*N*",
		(caps.PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP) ? " Y " : " N " );

	Warning("\nRaster Caps :\n");
	Warning("FOGRANGE %s FOGTABLE %s FOGVERTEX %s ZFOG %s WFOG %s\n",
		(caps.RasterCaps & D3DPRASTERCAPS_FOGRANGE) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_ZFOG) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_WFOG) ? " Y " : " N " );
	Warning("MIPMAPLODBIAS %s WBUFFER %s ZBIAS %s ZTEST %s\n",
		(caps.RasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_WBUFFER) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS) ? " Y " : " N ",
		(caps.RasterCaps & D3DPRASTERCAPS_ZTEST) ? " Y " : "*N*" );

	Warning("Size of Texture Memory : %d kb\n", m_Caps.m_TextureMemorySize / 1024 );
	Warning("Max Texture Dimensions : %d x %d\n", 
		caps.MaxTextureWidth, caps.MaxTextureHeight );
	if (caps.MaxTextureAspectRatio != 0)
		Warning("Max Texture Aspect Ratio : *%d*\n", caps.MaxTextureAspectRatio );
	Warning("Max Textures : %d Max Stages : %d\n", 
		caps.MaxSimultaneousTextures, caps.MaxTextureBlendStages );

	Warning("\nTexture Caps :\n");
	Warning("ALPHA %s CUBEMAP %s MIPCUBEMAP %s SQUAREONLY %s\n",
		(caps.TextureCaps & D3DPTEXTURECAPS_ALPHA) ? " Y " : " N ",
		(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) ? " Y " : " N ",
		(caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP) ? " Y " : " N ",
		(caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) ? "*Y*" : " N " );

	Warning( "vendor id: 0x%x\n", m_nVendorId );
	Warning( "device id: 0x%x\n", m_nDeviceId );
}


//-----------------------------------------------------------------------------
// Inherited from IMaterialSystemHardwareConfig
//-----------------------------------------------------------------------------

bool CShaderAPIDX8::HasAlphaBuffer() const
{
	return (m_Mode.m_Format == IMAGE_FORMAT_BGRA8888);
}

bool CShaderAPIDX8::HasStencilBuffer() const
{
	return false;
}

bool CShaderAPIDX8::HasARBMultitexture() const
{
	return (GetNumTextureUnits() > 1);
}

bool CShaderAPIDX8::HasNVRegisterCombiners() const
{
	return false;
}

bool CShaderAPIDX8::HasTextureEnvCombine() const
{
	return true;
}

int	 CShaderAPIDX8::GetFrameBufferColorDepth() const
{
	return ShaderUtil()->ImageFormatInfo(m_Mode.m_Format).m_NumBytes;
}

int  CShaderAPIDX8::GetNumTextureUnits() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel != 0) && ( GetDXSupportLevel() <= 71 ))
		return 2;

	int configNumTextureUnits = ShaderUtil()->GetConfig().numTextureUnits;
	if( configNumTextureUnits <= 0 )
	{
		return m_Caps.m_NumTextureUnits;
	}
	else
	{
		return configNumTextureUnits < m_Caps.m_NumTextureUnits ? 
			configNumTextureUnits : m_Caps.m_NumTextureUnits;
	}
}

int CShaderAPIDX8::GetActualNumTextureUnits() const
{
	return m_Caps.m_NumTextureUnits;
}

int  CShaderAPIDX8::GetNumTextureStages() const
{
	if ( (ShaderUtil()->GetConfig().dxSupportLevel != 0) && (GetDXSupportLevel() <= 71) )
	{
		return 2;
	}

	return m_Caps.m_NumTextureStages;
}

bool CShaderAPIDX8::HasIteratorsPerTextureUnit() const
{
	return true;
}

bool CShaderAPIDX8::HasSetDeviceGammaRamp()	const
{
	return m_Caps.m_HasSetDeviceGammaRamp;
}

bool CShaderAPIDX8::SupportsCompressedTextures() const
{
	Assert( m_Caps.m_SupportsCompressedTextures != COMPRESSED_TEXTURES_NOT_INITIALIZED );
	return m_Caps.m_SupportsCompressedTextures == COMPRESSED_TEXTURES_ON;
}

bool CShaderAPIDX8::SupportsVertexAndPixelShaders() const 
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel != 0) && (GetDXSupportLevel() < 80))
		return false;

	return m_Caps.m_SupportsPixelShaders;
}

bool CShaderAPIDX8::SupportsPixelShaders_1_4() const 
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel != 0) && (GetDXSupportLevel() < 81))
		return false;

	return m_Caps.m_SupportsPixelShaders_1_4;
}

bool CShaderAPIDX8::SupportsPixelShaders_2_0() const 
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel != 0) &&
	    (GetDXSupportLevel() < 90))
		return false;

	return m_Caps.m_SupportsPixelShaders_2_0;
}

bool CShaderAPIDX8::SupportsVertexShaders_2_0() const 
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel != 0) &&
	    (GetDXSupportLevel() < 90))
		return false;

	return m_Caps.m_SupportsVertexShaders_2_0;
}

bool CShaderAPIDX8::SupportsPartialTextureDownload() const
{
	return true;
}

bool CShaderAPIDX8::SupportsStateSnapshotting()	const
{
	return true;
}

int CShaderAPIDX8::MaximumAnisotropicLevel() const
{
	return m_Caps.m_nMaxAnisotropy;
}

int  CShaderAPIDX8::MaxTextureWidth() const
{
	return m_Caps.m_MaxTextureWidth;
}

int  CShaderAPIDX8::MaxTextureHeight() const
{
	return m_Caps.m_MaxTextureHeight;
}

int	 CShaderAPIDX8::MaxTextureAspectRatio() const
{
	return m_Caps.m_MaxTextureAspectRatio;
}

int	 CShaderAPIDX8::TextureMemorySize() const
{
	return m_Caps.m_TextureMemorySize;
}

bool CShaderAPIDX8::SupportsOverbright() const
{
	return m_Caps.m_SupportsOverbright;
}

bool CShaderAPIDX8::SupportsMipmapping() const
{
	return m_Caps.m_SupportsMipmapping;
}

bool CShaderAPIDX8::SupportsCubeMaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 70))
		return false;

	return m_Caps.m_SupportsCubeMaps;
}

bool CShaderAPIDX8::SupportsMipmappedCubemaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 70))
		return false;

	return m_Caps.m_SupportsMipmappedCubemaps;
}

bool CShaderAPIDX8::SupportsNonPow2Textures() const
{
	return m_Caps.m_SupportsNonPow2Textures;
}

int	 CShaderAPIDX8::NumVertexShaderConstants() const
{
	return m_Caps.m_NumVertexShaderConstants;
}

int	 CShaderAPIDX8::NumPixelShaderConstants() const
{
	return m_Caps.m_NumPixelShaderConstants;
}

int	CShaderAPIDX8::MaxNumLights() const
{
	return m_Caps.m_MaxNumLights;
}

bool CShaderAPIDX8::SupportsHardwareLighting() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 70))
		return false;

	return m_Caps.m_SupportsHardwareLighting;
}

int	 CShaderAPIDX8::MaxBlendMatrices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 70))
	{
		return 1;
	}

	return m_Caps.m_MaxBlendMatrices;
}

int	 CShaderAPIDX8::MaxBlendMatrixIndices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 70))
	{
		return 1;
	}

	return m_Caps.m_MaxBlendMatrixIndices;
}

int	 CShaderAPIDX8::MaxVertexShaderBlendMatrices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 70))
	{
		return 1;
	}

	return m_Caps.m_MaxVertexShaderBlendMatrices;
}

int	 CShaderAPIDX8::GetDXSupportLevel() const
{
	if ( ShaderUtil()->GetConfig().dxSupportLevel != 0 )
	{
		return min( ShaderUtil()->GetConfig().dxSupportLevel, m_Caps.m_DXSupportLevel );
	}

	return m_Caps.m_DXSupportLevel;
}

bool CShaderAPIDX8::UseFastZReject() const
{
	return m_Caps.m_bFastZReject;
}

const char *CShaderAPIDX8::GetShaderDLLName() const
{
	return m_Caps.m_pShaderDLL ? m_Caps.m_pShaderDLL : "DEFAULT";
}

//  This is not yet implemented
bool CShaderAPIDX8::NeedsAAClamp() const 
{
#pragma message( "REMINDER: CShaderAPIDX8::NeedsAAClamp() -- Not yet implemented" )
	return false;
}

bool CShaderAPIDX8::ReadPixelsFromFrontBuffer() const
{
	// GR - in DX 9.0a can blit from MSAA back buffer
	return false;
}

bool CShaderAPIDX8::PreferDynamicTextures() const
{
	// For now, disable this feature.
	return false;
//	return m_Caps.m_PreferDynamicTextures;
}

bool CShaderAPIDX8::HasProjectedBumpEnv() const
{
	return m_Caps.m_HasProjectedBumpEnv;
}

bool CShaderAPIDX8::SupportsSpheremapping() const
{
	return m_Caps.m_bSupportsSpheremapping;
}

bool CShaderAPIDX8::HasFastZReject() const
{
	return m_Caps.m_bFastZReject;
}

bool CShaderAPIDX8::NeedsATICentroidHack() const
{
	return m_Caps.m_bNeedsATICentroidHack;
}

int CShaderAPIDX8::MaxUserClipPlanes() const
{
	return m_Caps.m_MaxUserClipPlanes;
}

bool CShaderAPIDX8::UseFastClipping() const
{
	return m_Caps.m_UseFastClipping;
}

bool CShaderAPIDX8::SupportsHDR() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (GetDXSupportLevel() < 90))
		return false;

//	if (m_Mode.m_Format != IMAGE_FORMAT_BGRA8888)
//		return false;

	return m_Caps.m_SupportsHDR;
}

const char *CShaderAPIDX8::GetHWSpecificShaderDLLName()	const
{
	return m_Caps.m_pShaderDLL[0] ? m_Caps.m_pShaderDLL : NULL;
}

//-----------------------------------------------------------------------------
// Initialize vertex and pixel shaders
//-----------------------------------------------------------------------------
void CShaderAPIDX8::InitVertexAndPixelShaders( )
{
	// Allocate space for the pixel and vertex shader constants...
	if (m_Caps.m_SupportsPixelShaders)
	{
		if (m_DynamicState.m_pPixelShaderConstant)
			delete[] m_DynamicState.m_pPixelShaderConstant;
		m_DynamicState.m_pPixelShaderConstant = new Vector4D[m_Caps.m_NumPixelShaderConstants];

		if (m_DynamicState.m_pVertexShaderConstant)
			delete[] m_DynamicState.m_pVertexShaderConstant;
		m_DynamicState.m_pVertexShaderConstant = new Vector4D[m_Caps.m_NumVertexShaderConstants];

		// Blat out all constants...
		int i;
		Vector4D zero( 0.0f, 0.0f, 0.0f, 0.0f );
		for ( i = 0; i < m_Caps.m_NumVertexShaderConstants; ++i )
		{
			SetVertexShaderConstant( i, zero.Base(), 1, true );
		}
		for ( i = 0; i < m_Caps.m_NumPixelShaderConstants; ++i )
		{
			SetPixelShaderConstant( i, zero.Base(), 1, true );
		}

		// Set a couple standard constants....
		Vector4D standardVertexShaderConstant( 0.0f, 1.0f, 2.0f, 0.5f );
		SetVertexShaderConstant( 0, standardVertexShaderConstant.Base(), 1 );

		// [$cThree, unused, unused, unused]
		Vector4D standardVertexShaderConstant2( 3.0f, 0.0f, 0.0f, 0.0f );
		SetVertexShaderConstant( 39, standardVertexShaderConstant2.Base(), 1 );

		// These point to the lighting and the transforms
		standardVertexShaderConstant.Init( VERTEX_SHADER_LIGHTS,
			VERTEX_SHADER_LIGHTS + 5, 
            // Use COLOR instead of UBYTE4 since Geforce3 does not support it
            // vConst.w should be 3, but due to about hack, mul by 255 and add epsilon
			765.01f, // ubyte4 crap
			VERTEX_SHADER_MODEL );

		SetVertexShaderConstant( VERTEX_SHADER_LIGHT_INDEX, 
			standardVertexShaderConstant.Base(), 1 );
	}

	// Set up the vertex and pixel shader stuff
	ShaderManager()->ResetShaderState();
}


//-----------------------------------------------------------------------------
// Set the overbrighting value
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetVertexShaderConstantC1( float overbright )
{
	if (SupportsVertexAndPixelShaders())
	{
		// Set a couple standard constants....
		if (overbright != m_DynamicState.m_VertexShaderOverbright)
		{
			Vector4D standardVertexShaderConstant;

			standardVertexShaderConstant.Init( 1.0f/2.2f, overbright, 1.0f / 3.0f, 1.0f / overbright );
			SetVertexShaderConstant( 1, standardVertexShaderConstant.Base(), 1 );
			m_DynamicState.m_VertexShaderOverbright = overbright;
		}
	}
}


//-----------------------------------------------------------------------------
// We need to make the ambient cube...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CreateAmbientCubeTexture()
{
	if (m_Caps.m_SupportsCubeMaps)
	{
		CreateTexture( TEXTURE_AMBIENT_CUBE, AMBIENT_CUBE_TEXTURE_SIZE,
			AMBIENT_CUBE_TEXTURE_SIZE, IMAGE_FORMAT_RGB888, 1, NUM_AMBIENT_CUBE_TEXTURES, 
			TEXTURE_CREATE_CUBEMAP | TEXTURE_CREATE_MANAGED );
	}
}

void CShaderAPIDX8::ReleaseAmbientCubeTexture()
{
	if (m_Caps.m_SupportsCubeMaps)
	{
		DeleteTexture( TEXTURE_AMBIENT_CUBE );
	}
}


//-----------------------------------------------------------------------------
// Initialize render state
//-----------------------------------------------------------------------------
void CShaderAPIDX8::InitRenderState( )
{ 
	// Set the default shadow state
	ShaderShadow()->SetDefaultState();

	// Grab a snapshot of this state; we'll be using it to set the board
	// state to something well defined.
	m_TransitionTable.TakeDefaultStateSnapshot( );

	ResetRenderState( );
}


//-----------------------------------------------------------------------------
// Returns true if the state snapshot is translucent
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::IsTranslucent( StateSnapshot_t id ) const
{
	return m_TransitionTable.GetSnapshot(id).m_AlphaBlendEnable;
}


//-----------------------------------------------------------------------------
// Returns true if the state snapshot is alpha tested
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::IsAlphaTested( StateSnapshot_t id ) const
{
	return m_TransitionTable.GetSnapshot(id).m_AlphaTestEnable;
}


//-----------------------------------------------------------------------------
// Returns true if the state snapshot uses shaders
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	return m_TransitionTable.GetSnapshotShader(id).m_VertexShader != INVALID_VERTEX_SHADER;
}


//-----------------------------------------------------------------------------
// returns true if the state uses an ambient cube
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::UsesAmbientCube( StateSnapshot_t id ) const
{
	return m_TransitionTable.GetSnapshot(id).m_AmbientCubeOnStage0;
}


//-----------------------------------------------------------------------------
// Takes a snapshot
//-----------------------------------------------------------------------------
StateSnapshot_t CShaderAPIDX8::TakeSnapshot( )
{
	return m_TransitionTable.TakeSnapshot();
}



//-----------------------------------------------------------------------------
// Sets the texture stage state
//-----------------------------------------------------------------------------
inline void CShaderAPIDX8::SetSamplerState( int stage, D3DSAMPLERSTATETYPE state, DWORD val )
{
	if( IsDeactivated() )
		return;

	RECORD_SAMPLER_STATE( stage, state, val ); 

	HRESULT	hr = m_pD3DDevice->SetSamplerState( stage, state, val );
	Assert( !FAILED(hr) );
}

inline void CShaderAPIDX8::SetTextureStageState( int stage, D3DTEXTURESTAGESTATETYPE state, DWORD val )
{
	if( IsDeactivated() )
		return;

	RECORD_TEXTURE_STAGE_STATE( stage, state, val ); 

	HRESULT	hr = m_pD3DDevice->SetTextureStageState( stage, state, val );
	Assert( !FAILED(hr) );
}

inline void CShaderAPIDX8::SetRenderState( D3DRENDERSTATETYPE state, DWORD val )
{
	if( IsDeactivated() )
		return;

	Assert( state >= 0 && state < MAX_NUM_RENDERSTATES );
	if( m_DynamicState.m_RenderState[state] != val )
	{
		RECORD_RENDER_STATE( state, val ); 
		HRESULT	hr = m_pD3DDevice->SetRenderState( state, val );
		Assert( !FAILED(hr) );
		m_DynamicState.m_RenderState[state] = val;
	}
}

inline void CShaderAPIDX8::SetRenderStateForce( D3DRENDERSTATETYPE state, DWORD val )
{
	if( IsDeactivated() )
		return;

	Assert( state >= 0 && state < MAX_NUM_RENDERSTATES );
	RECORD_RENDER_STATE( state, val ); 
	HRESULT	hr = m_pD3DDevice->SetRenderState( state, val );
	Assert( !FAILED(hr) );
	m_DynamicState.m_RenderState[state] = val;
}

void CShaderAPIDX8::ResetDXRenderState( void )
{
	float zero = 0.0f;
	float one = 1.0f;
	DWORD dZero = *((DWORD*)(&zero));
	DWORD dOne = *((DWORD*)(&one));
	
	// Set default values for all dx render states.
    SetRenderStateForce( D3DRS_FILLMODE, D3DFILL_SOLID );
    SetRenderStateForce( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
    SetRenderStateForce( D3DRS_LASTPIXEL, TRUE );
    SetRenderStateForce( D3DRS_CULLMODE, D3DCULL_CCW );
    SetRenderStateForce( D3DRS_DITHERENABLE, FALSE );
    SetRenderStateForce( D3DRS_FOGENABLE, FALSE );
    SetRenderStateForce( D3DRS_SPECULARENABLE, FALSE );
    SetRenderStateForce( D3DRS_FOGCOLOR, 0 );
    SetRenderStateForce( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    SetRenderStateForce( D3DRS_FOGSTART, dZero );
    SetRenderStateForce( D3DRS_FOGEND, dOne );
    SetRenderStateForce( D3DRS_FOGDENSITY, dZero );
    SetRenderStateForce( D3DRS_RANGEFOGENABLE, FALSE );
    SetRenderStateForce( D3DRS_STENCILENABLE, FALSE);
    SetRenderStateForce( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
    SetRenderStateForce( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );
    SetRenderStateForce( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
    SetRenderStateForce( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
    SetRenderStateForce( D3DRS_STENCILREF, 0 );
    SetRenderStateForce( D3DRS_STENCILMASK, 0xFFFFFFFF );
    SetRenderStateForce( D3DRS_STENCILWRITEMASK, 0xFFFFFFFF );
    SetRenderStateForce( D3DRS_TEXTUREFACTOR, 0xFFFFFFFF );
    SetRenderStateForce( D3DRS_WRAP0, 0 );
    SetRenderStateForce( D3DRS_WRAP1, 0 );
    SetRenderStateForce( D3DRS_WRAP2, 0 );
    SetRenderStateForce( D3DRS_WRAP3, 0 );
    SetRenderStateForce( D3DRS_WRAP4, 0 );
    SetRenderStateForce( D3DRS_WRAP5, 0 );
    SetRenderStateForce( D3DRS_WRAP6, 0 );
    SetRenderStateForce( D3DRS_WRAP7, 0 );
    SetRenderStateForce( D3DRS_CLIPPING, TRUE );
    SetRenderStateForce( D3DRS_LIGHTING, TRUE );
    SetRenderStateForce( D3DRS_AMBIENT, 0 );
    SetRenderStateForce( D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    SetRenderStateForce( D3DRS_COLORVERTEX, TRUE );
    SetRenderStateForce( D3DRS_LOCALVIEWER, TRUE );
    SetRenderStateForce( D3DRS_NORMALIZENORMALS, FALSE );
    SetRenderStateForce( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );
    SetRenderStateForce( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2 );
    SetRenderStateForce( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL );
    SetRenderStateForce( D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );
    SetRenderStateForce( D3DRS_VERTEXBLEND, D3DVBF_DISABLE );
    SetRenderStateForce( D3DRS_CLIPPLANEENABLE, 0 );
    SetRenderStateForce( D3DRS_POINTSIZE, dOne );
    SetRenderStateForce( D3DRS_POINTSIZE_MIN, dOne );
    SetRenderStateForce( D3DRS_POINTSPRITEENABLE, FALSE );
    SetRenderStateForce( D3DRS_POINTSCALEENABLE, FALSE );
    SetRenderStateForce( D3DRS_POINTSCALE_A, dOne );
    SetRenderStateForce( D3DRS_POINTSCALE_B, dZero );
    SetRenderStateForce( D3DRS_POINTSCALE_C, dZero );
    SetRenderStateForce( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
    SetRenderStateForce( D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF );
    SetRenderStateForce( D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE );
    SetRenderStateForce( D3DRS_DEBUGMONITORTOKEN, D3DDMT_ENABLE );
	float sixtyFour = 64.0f;
    SetRenderStateForce( D3DRS_POINTSIZE_MAX, *((DWORD*)(&sixtyFour)));
    SetRenderStateForce( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
    SetRenderStateForce( D3DRS_TWEENFACTOR, dZero );
    SetRenderStateForce( D3DRS_BLENDOP, D3DBLENDOP_ADD );
    SetRenderStateForce( D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC );
    SetRenderStateForce( D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR );
    SetRenderStateForce( D3DRS_SCISSORTESTENABLE, FALSE);
    SetRenderStateForce( D3DRS_SLOPESCALEDEPTHBIAS, dZero );
    SetRenderStateForce( D3DRS_ANTIALIASEDLINEENABLE, FALSE );
    SetRenderStateForce( D3DRS_MINTESSELLATIONLEVEL, dOne );
    SetRenderStateForce( D3DRS_MAXTESSELLATIONLEVEL, dOne );
    SetRenderStateForce( D3DRS_ADAPTIVETESS_X, dZero );
    SetRenderStateForce( D3DRS_ADAPTIVETESS_Y, dZero );
    SetRenderStateForce( D3DRS_ADAPTIVETESS_Z, dOne );
    SetRenderStateForce( D3DRS_ADAPTIVETESS_W, dZero );
    SetRenderStateForce( D3DRS_ENABLEADAPTIVETESSELLATION, FALSE );
    SetRenderStateForce( D3DRS_TWOSIDEDSTENCILMODE, FALSE );
    SetRenderStateForce( D3DRS_CCW_STENCILFAIL, 0x00000001);
    SetRenderStateForce( D3DRS_CCW_STENCILZFAIL, 0x00000001 );
    SetRenderStateForce( D3DRS_CCW_STENCILPASS, 0x00000001 );
    SetRenderStateForce( D3DRS_CCW_STENCILFUNC, 0x00000008 );
    SetRenderStateForce( D3DRS_COLORWRITEENABLE1, 0x0000000f );
    SetRenderStateForce( D3DRS_COLORWRITEENABLE2, 0x0000000f );
    SetRenderStateForce( D3DRS_COLORWRITEENABLE3, 0x0000000f);
    SetRenderStateForce( D3DRS_BLENDFACTOR, 0xffffffff );
    SetRenderStateForce( D3DRS_SRGBWRITEENABLE, 0);
    SetRenderStateForce( D3DRS_DEPTHBIAS, dZero );
    SetRenderStateForce( D3DRS_WRAP8, 0 );
    SetRenderStateForce( D3DRS_WRAP9, 0 );
    SetRenderStateForce( D3DRS_WRAP10, 0 );
    SetRenderStateForce( D3DRS_WRAP11, 0 );
    SetRenderStateForce( D3DRS_WRAP12, 0 );
    SetRenderStateForce( D3DRS_WRAP13, 0 );
    SetRenderStateForce( D3DRS_WRAP14, 0 );
    SetRenderStateForce( D3DRS_WRAP15, 0 );
    SetRenderStateForce( D3DRS_SEPARATEALPHABLENDENABLE, FALSE );
    SetRenderStateForce( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );
    SetRenderStateForce( D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO );
    SetRenderStateForce( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );
}


//-----------------------------------------------------------------------------
// Reset render state (to its initial value)
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ResetRenderState( )
{ 
	ResetDXRenderState();

	// We're not currently rendering anything
	m_nCurrentSnapshot = -1;

	// This is state that isn't part of the snapshot per-se, because we
	// don't need it when it's actually time to render. This just helps us
	// to construct the shadow state.
	m_DynamicState.m_ClearColor = D3DCOLOR_XRGB(0,0,0);

	InitVertexAndPixelShaders();

	m_CachedAmbientLightCube = STATE_CHANGED;

	// Set the constant color
	m_DynamicState.m_ConstantColor = 0xFFFFFFFF;
	Color4ub( 255, 255, 255, 255 );

	// Set the point size.
	m_DynamicState.m_PointSize = -1.0f;
	PointSize( 1.0 );

	// Ambient light color
	m_DynamicState.m_Ambient = 0;
	SetRenderState( D3DRS_AMBIENT, m_DynamicState.m_Ambient );

	// Fog
	m_DynamicState.m_FogColor = 0xFFFFFFFF;
	m_DynamicState.m_FogEnable = false;
	m_DynamicState.m_FogHeightEnabled = false;
	m_DynamicState.m_FogMode = D3DFOG_NONE;
	m_DynamicState.m_FogStart = -1;
	m_DynamicState.m_FogEnd = -1;
	SetRenderState( D3DRS_FOGCOLOR, m_DynamicState.m_FogColor );
	SetRenderState( D3DRS_FOGENABLE, m_DynamicState.m_FogEnable );
	SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	SetRenderState( D3DRS_FOGVERTEXMODE, D3DFOG_NONE );
	SetRenderState( D3DRS_RANGEFOGENABLE, false );
	FogStart( 0 );
	FogEnd( 0 );

	// Set the cull mode
	m_DynamicState.m_CullMode = D3DCULL_CCW;

	// No shade mode yet
	m_DynamicState.m_ShadeMode = (D3DSHADEMODE)-1;
	ShadeMode( SHADER_SMOOTH );

	// Skinning...
	m_DynamicState.m_NumBones = 0;
	m_DynamicState.m_VertexBlend = (D3DVERTEXBLENDFLAGS)-1;
	SetRenderState( D3DRS_VERTEXBLEND, D3DVBF_DISABLE );
	SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );

	// No normal normalization
	m_DynamicState.m_NormalizeNormals = false;
	SetRenderState( D3DRS_NORMALIZENORMALS, m_DynamicState.m_NormalizeNormals );

	bool bAntialiasing = ( m_PresentParameters.MultiSampleType != D3DMULTISAMPLE_NONE );
	SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, bAntialiasing );
	
	m_DynamicState.m_VertexShaderOverbright = 0.0f;

	int i;
	for ( i = 0; i < m_Caps.m_NumTextureUnits; ++i)
	{
		TextureStage(i).m_BoundTexture = -1;						  
		TextureStage(i).m_UTexWrap = D3DTADDRESS_WRAP;
		TextureStage(i).m_VTexWrap = D3DTADDRESS_WRAP;
		TextureStage(i).m_MagFilter = D3DTEXF_POINT;
		TextureStage(i).m_MinFilter = D3DTEXF_POINT;
		TextureStage(i).m_MipFilter = D3DTEXF_NONE;
		TextureStage(i).m_nAnisotropyLevel = m_Caps.m_nMaxAnisotropy;
		TextureStage(i).m_TextureTransformFlags = D3DTTFF_DISABLE;
		TextureStage(i).m_TextureEnable = false;
		TextureStage(i).m_BumpEnvMat00 = 1.0f;
		TextureStage(i).m_BumpEnvMat01 = 0.0f;
		TextureStage(i).m_BumpEnvMat10 = 0.0f;
		TextureStage(i).m_BumpEnvMat11 = 1.0f;
		TextureStage(i).m_bSRGBReadEnabled = false;

		// Just some initial state...
		RECORD_COMMAND( DX8_SET_TEXTURE, 3 );
		RECORD_INT( i );
		RECORD_INT( -1 );
		RECORD_INT( -1 );

		HRESULT hr = m_pD3DDevice->SetTexture( i, 0 );
		Assert( !FAILED(hr) );

		SetSamplerState( i, D3DSAMP_ADDRESSU, TextureStage(i).m_UTexWrap );
		SetSamplerState( i, D3DSAMP_ADDRESSV, TextureStage(i).m_VTexWrap ); 
		SetSamplerState( i, D3DSAMP_MINFILTER, TextureStage(i).m_MinFilter );
		SetSamplerState( i, D3DSAMP_MAGFILTER, TextureStage(i).m_MagFilter );
		SetSamplerState( i, D3DSAMP_MIPFILTER, TextureStage(i).m_MipFilter );
		SetSamplerState( i, D3DSAMP_MAXANISOTROPY, TextureStage(i).m_nAnisotropyLevel );
		SetSamplerState( i, D3DSAMP_SRGBTEXTURE, TextureStage(i).m_bSRGBReadEnabled );
		SetTextureStageState( i, D3DTSS_TEXTURETRANSFORMFLAGS, TextureStage(i).m_TextureTransformFlags );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT00, *( ( LPDWORD ) (&TextureStage(i).m_BumpEnvMat00) ) );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT01, *( ( LPDWORD ) (&TextureStage(i).m_BumpEnvMat01) ) );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT10, *( ( LPDWORD ) (&TextureStage(i).m_BumpEnvMat10) ) );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT11, *( ( LPDWORD ) (&TextureStage(i).m_BumpEnvMat11) ) );
	}

	m_DynamicState.m_NumLights = 0;
	for ( i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		m_DynamicState.m_LightEnable[i] = false;
		m_DynamicState.m_LightChanged[i] = STATE_CHANGED;
		m_DynamicState.m_LightEnableChanged[i] = STATE_CHANGED;
	}

	for ( i = 0; i < NUM_MATRIX_MODES; ++i)
	{
		// By setting this to *not* be identity, we force an update...
		m_DynamicState.m_TransformType[i] = TRANSFORM_IS_GENERAL;
		m_DynamicState.m_TransformChanged[i] = STATE_CHANGED;
	}

	// set the board state to match the default state
	m_TransitionTable.UseDefaultState();

	// Set the default render state
	ShaderUtil()->SetDefaultState();

	// Constant for all time
	SetRenderState( D3DRS_CLIPPING, TRUE );
	SetRenderState( D3DRS_LOCALVIEWER, TRUE );
	SetRenderState( D3DRS_POINTSCALEENABLE, FALSE );
	SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );
	SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL );
	SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL );
#ifdef GARY_WORK_IN_PROGRESS
	SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR2 );
#else
	SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );
#endif
	SetRenderState( D3DRS_COLORVERTEX, TRUE ); // This defaults to true anyways. . . 

	// When the ambient cube is enabled and we're doing lighting in hardware,
	// we'll be multiplying stage 0 by 2 so we can get a range of ambient cube
	// values which range from -1 to 1. When this happens, we're gonna need to
	// reduce the contribution of normal lights by a half so it all works out
	// correctly once that factor of 2 is applied.
	m_DynamicState.m_MaterialOverbrightVal = -1.0f;
	SetMaterialProperty( 0.5f );

#if 0
	float fBias = -1.0f;
	SetTextureStageState( 0, D3DTSS_MIPMAPLODBIAS, *( ( LPDWORD ) (&fBias) ) );
	SetTextureStageState( 1, D3DTSS_MIPMAPLODBIAS, *( ( LPDWORD ) (&fBias) ) );
	SetTextureStageState( 2, D3DTSS_MIPMAPLODBIAS, *( ( LPDWORD ) (&fBias) ) );
	SetTextureStageState( 3, D3DTSS_MIPMAPLODBIAS, *( ( LPDWORD ) (&fBias) ) );
#endif

	// Set the modelview matrix to identity too
	for ( i = 0; i < NUM_MODEL_TRANSFORMS; ++i )
	{
		MatrixMode( MATERIAL_MODEL_MATRIX(i) );
		LoadIdentity( );
	}
	MatrixMode( MATERIAL_VIEW );
	LoadIdentity( );
	MatrixMode( MATERIAL_PROJECTION );
	LoadIdentity( );

	m_DynamicState.m_Viewport.X = m_DynamicState.m_Viewport.Y = 
		m_DynamicState.m_Viewport.Width = m_DynamicState.m_Viewport.Height = 0xFFFFFFFF;
	m_DynamicState.m_Viewport.MinZ = m_DynamicState.m_Viewport.MaxZ = 0.0;

	SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );
	float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float one[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Make sure that our state is dirry.
	m_DynamicState.m_UserClipPlaneEnabled = 0;
	m_DynamicState.m_UserClipPlaneChanged = 0;
	for( i = 0; i < MaxUserClipPlanes(); i++ )
	{
		// Make sure that our state is dirty.
		m_DynamicState.m_UserClipPlaneWorld[i][0] = -1.0f;
		m_DynamicState.m_UserClipPlaneProj[i][0] = -9999.0f;
		m_DynamicState.m_UserClipPlaneEnabled |= ( 1 << i );
		SetClipPlane( i, zero );
		EnableClipPlane( i, false );
		Assert( m_DynamicState.m_UserClipPlaneEnabled == 0 );
	}
	m_DynamicState.m_FastClipEnabled = false;

	// Viewport defaults to the window size
	RECT windowRect;
    GetClientRect( m_HWnd, &windowRect );
	Viewport( windowRect.left, windowRect.top, 
				windowRect.right - windowRect.left,
				windowRect.bottom - windowRect.top );
	DepthRange( 0, 1 );

	// No render mesh
	m_pRenderMesh = 0;

	// Reset cached vertex decl
	m_DynamicState.m_pVertexDecl = NULL;
	
	// Reset the render target to be the normal backbuffer
	AcquireRenderTargets();
	SetRenderTarget( );
}


//-----------------------------------------------------------------------------
// Sets the default render state
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetDefaultState()
{
	ShaderUtil()->SetDefaultState();
}



//-----------------------------------------------------------------------------
//
// Methods related to vertex format
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sets the vertex
//-----------------------------------------------------------------------------
inline void CShaderAPIDX8::SetVertexDecl( VertexFormat_t vertexFormat, bool bHasColorMesh )
{
	IDirect3DVertexDeclaration9 *pDecl = FindOrCreateVertexDecl( vertexFormat, bHasColorMesh );
	Assert( pDecl );
	if( ( pDecl != m_DynamicState.m_pVertexDecl ) && pDecl )
	{
		RECORD_COMMAND( DX8_SET_VERTEX_DECLARATION, 1 );
		RECORD_INT( ( int )pDecl );
		HRESULT hr = m_pD3DDevice->SetVertexDeclaration( pDecl );
		Assert( hr == D3D_OK );
		m_DynamicState.m_pVertexDecl = pDecl;
	}
}



//-----------------------------------------------------------------------------
//
// Methods related to vertex buffers
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Creates/destroys Mesh
//-----------------------------------------------------------------------------
IMesh* CShaderAPIDX8::CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh )
{
	return MeshMgr()->CreateStaticMesh( pMaterial, bForceTempMesh );
}

IMesh* CShaderAPIDX8::CreateStaticMesh( VertexFormat_t vertexFormat, bool bSoftwareVertexShader )
{
	return MeshMgr()->CreateStaticMesh( vertexFormat, bSoftwareVertexShader );
}

void CShaderAPIDX8::DestroyStaticMesh( IMesh* pMesh )
{
	MeshMgr()->DestroyStaticMesh( pMesh );
}


//-----------------------------------------------------------------------------
// Gets the dynamic mesh
//-----------------------------------------------------------------------------
IMesh* CShaderAPIDX8::GetDynamicMesh( IMaterial* pMaterial, bool buffered,
								IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	return MeshMgr()->GetDynamicMesh( pMaterial, buffered, pVertexOverride, pIndexOverride );
}


//-----------------------------------------------------------------------------
// Draws the mesh
//-----------------------------------------------------------------------------
void CShaderAPIDX8::DrawMesh( IMeshDX8* pMesh )
{
	if ( ShaderUtil()->GetConfig().m_bSuppressRendering )
		return;

	m_pRenderMesh = pMesh;
	SetVertexDecl( m_pRenderMesh->GetVertexFormat(), m_pRenderMesh->HasColorMesh() );
	CommitStateChanges();
	Assert( m_pRenderMesh && m_pMaterial );
	m_pMaterial->DrawMesh( m_pRenderMesh );
	m_pRenderMesh = NULL;
}


//-----------------------------------------------------------------------------
// Discards the vertex buffers
//-----------------------------------------------------------------------------
void CShaderAPIDX8::DiscardVertexBuffers()
{
	MeshMgr()->DiscardVertexBuffers();
}


void CShaderAPIDX8::ForceHardwareSync( void )
{
	VPROF( "CShaderAPIDX8::ForceHardwareSync" );

	// need to flush the dynamic buffer	and make sure the entire image is there
	FlushBufferedPrimitives();

	RECORD_COMMAND( DX8_HARDWARE_SYNC, 0 );

	// How do you query dx9 for how many frames behind the hardware is or, alternatively, how do you tell the hardware to never be more than N frames behind?
	// 1) The old QueryPendingFrameCount design was removed.  It was
	// a simple transaction with the driver through the 
	// GetDriverState, trivial for the drivers to lie.  We came up 
	// with a much better scheme for tracking pending frames where 
	// the driver can not lie without a possible performance loss:  
	// use the asynchronous query system with D3DQUERYTYPE_EVENT and 
	// data size 0.  When GetData returns S_OK for the query, you 
	// know that frame has finished.
	
	if( m_DeviceSupportsCreateQuery && m_pFrameSyncQueryObject )
	{
		BOOL bDummy;
		HRESULT hr;
		
		// FIXME: Could install a callback into the materialsystem to do something while waiting for
		// the frame to finish (update sound, etc.)
		do
		{
			hr = m_pFrameSyncQueryObject->GetData( &bDummy, sizeof( bDummy ), D3DGETDATA_FLUSH );
		} while( hr == S_FALSE );

		
		// FIXME: Need to check for hr==D3DERR_DEVICELOST here.
		Assert( hr != D3DERR_DEVICELOST );
		Assert( hr == S_OK );
		m_pFrameSyncQueryObject->Issue( D3DISSUE_END );
	} 
	else if( m_DeviceSupportsCreateQuery == 0 )	// (need to ensure device was created with a lockable back buffer)
	{
		
		MEASURE_TIMED_STAT_DAMMIT(MATERIAL_SYSTEM_STATS_FLUSH_HARDWARE);
		HRESULT hr;
		IDirect3DSurface* pSurfaceBits = 0;
		
		// Get the back buffer
		IDirect3DSurface* pBackBuffer;
		hr = m_pD3DDevice->GetRenderTarget( 0, &pBackBuffer );
		if (FAILED(hr))
		{
			Warning("GetRenderTarget() failed");
			return;
		}

		// Find about its size and format
		D3DSURFACE_DESC desc;
		hr = pBackBuffer->GetDesc( &desc );
		if (FAILED(hr))
		{
			Warning("GetDesc() failed");
			goto CleanUp;
		}
		D3DLOCKED_RECT lockedRect;
		RECT sourceRect;
		sourceRect.bottom = 4;
		sourceRect.top = 0;
		sourceRect.left = 0;
		sourceRect.right = 4;
		hr = pBackBuffer->LockRect(&lockedRect,&sourceRect,D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			Warning("LockRect() failed");
			goto CleanUp;
		}
		// Utterly lame, but prevents the optimizer from removing the read:
		volatile unsigned long a;
		memcpy((void *)&a,(unsigned char*)lockedRect.pBits,sizeof(a));

		hr = pBackBuffer->UnlockRect();
		if (FAILED(hr))
			Warning("UnlockRect() failed");

CleanUp:
		pBackBuffer->Release();
		
	}
}

//-----------------------------------------------------------------------------
// Use this to begin and end the frame
//-----------------------------------------------------------------------------

void CShaderAPIDX8::BeginFrame()
{
	++m_CurrentFrame;
}

void CShaderAPIDX8::EndFrame()
{
	MEMCHECK;
}


//-----------------------------------------------------------------------------
// Releases/reloads resources when other apps want some memory
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ReleaseResources()
{
	FreeFrameSyncObjects();
	ReleaseAmbientCubeTexture();
	MeshMgr()->ReleaseBuffers();
	ShaderUtil()->ReleaseShaderObjects();
	ReleaseRenderTargets();

#ifdef _DEBUG
	if ( MeshMgr()->BufferCount() != 0 )
	{

		for( int i = 0; i < MeshMgr()->BufferCount(); i++ )
		{
		}
	}
#endif
	
	// All meshes cleaned up?
	Assert( MeshMgr()->BufferCount() == 0 );


#ifdef _DEBUG
	// Helps to find the unreleased textures.
	if (TextureCount() > 0)
	{
		for (int i = 0; i < GetTextureCount(); ++i)
		{
			if (GetTexture( i ).m_NumCopies == 1)
			{
				if (GetTexture( i ).GetTexture() )
					Warning( "Didn't correctly clean up texture %d\n", GetTexture( i ).m_BindId ); 
			}
			else
			{
				for (int k = GetTexture( i ).m_NumCopies; --k >= 0; )
				{
					if ( GetTexture( i ).GetTexture( k ) != 0 )
					{
						Warning( "Didn't correctly clean up texture %d\n", GetTexture( i ).m_BindId ); 
						break;
					}
				}
			}
		}
	}
#endif

	Assert( TextureCount() == 0 );

}

void CShaderAPIDX8::ReacquireResources()
{
	AcquireRenderTargets();
	CreateAmbientCubeTexture();
	MeshMgr()->RestoreBuffers();
	ShaderUtil()->RestoreShaderObjects();
	AllocFrameSyncObjects();
}


//-----------------------------------------------------------------------------
// Occurs when another application is initializing
//-----------------------------------------------------------------------------

void CShaderAPIDX8::OtherAppInitializing( bool initializing )
{
	Assert( m_OtherAppInitializing != initializing );

	if (!IsDeactivated())
	{
		RECORD_COMMAND( DX8_END_SCENE, 0 );
		m_pD3DDevice->EndScene();
	}

	// NOTE: OtherApp is set in this way because we need to know we're
	// active as we release and restore everything
	if (initializing)
	{
		CheckDeviceLost();
		if (!IsDeactivated())
			ReleaseResources();
		m_OtherAppInitializing = true;
	}
	else
	{
		m_OtherAppInitializing = false;
		CheckDeviceLost();
		if (!IsDeactivated())
			ReacquireResources();
	}

	if (!IsDeactivated())
	{
		RECORD_COMMAND( DX8_BEGIN_SCENE, 0 );
		m_pD3DDevice->BeginScene();
	}
}


//-----------------------------------------------------------------------------
// Check for device lost
//-----------------------------------------------------------------------------

void CShaderAPIDX8::CheckDeviceLost()
{
	// Check here to see if we lost the device. If so, don't bother to start
	// the scene...
	RECORD_COMMAND( DX8_TEST_COOPERATIVE_LEVEL, 0 );
	HRESULT hr = m_pD3DDevice->TestCooperativeLevel();
	switch(hr)
	{
	case D3DERR_DEVICELOST:
		// We're lost baby lost
//		Warning( "CheckDeviceLost: display lost!\n" );
		if (!IsDeactivated())
		{
			ReleaseResources();
		}
		m_DisplayLost = true;

		break;

	case D3DERR_DEVICENOTRESET:
		if (!IsDeactivated())
		{
			ReleaseResources();
		}

		m_DisplayLost = true;

		RECORD_COMMAND( DX8_RESET, 1 );
		RECORD_STRUCT( &m_PresentParameters, sizeof(m_PresentParameters) );

		hr = m_pD3DDevice->Reset( &m_PresentParameters );
		if (!FAILED(hr))
		{
			// The display is no longer lost
			// Gotta do this before resetting the render state
			m_DisplayLost = false;

			// Reset render state
			ResetRenderState();

			// Have to mark the display not lost here
			// or mesh restore breaks down
//			Warning( "CheckDeviceLost: display not lost!\n" );

			// Still could be deactivated if this occurs while the other app is initializing
			if (!IsDeactivated())
			{
				ReacquireResources();
			}
		}
		else
		{
//			Warning( "CheckDeviceLost: reset failed\n" );
		}
		break;
	case D3D_OK:
//		Warning( "CheckDeviceLost: OK\n" );
		break;
	default:
		Warning( "CheckDeviceLost: Unknown result: %d\n", ( int )hr );
		break;
	}
}

//-----------------------------------------------------------------------------
// Compute fill rate
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ComputeFillRate()
{
	static unsigned char* pBuf = 0;

	int width, height;
	GetWindowSize( width, height );
	// Snapshot; look at total # pixels drawn...
	if (!pBuf)
	{
		pBuf = (unsigned char*)malloc( 
			ShaderUtil()->GetMemRequired( width, height, IMAGE_FORMAT_RGB888, false ) + 4 );
	}
	ReadPixels( 0, 0, width, height, pBuf, IMAGE_FORMAT_RGB888 );
	int mask = 0xFF;
	int count = 0;
	unsigned char* pRead = pBuf;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			int val = *(int*)pRead;
			count += (val & mask);
			pRead += 3;
		}
	}

	MaterialSystemStats()->IncrementCountedStat(MATERIAL_SYSTEM_STATS_FILL_RATE, count );
}

//-----------------------------------------------------------------------------
// Page Flipc
//-----------------------------------------------------------------------------

void CShaderAPIDX8::SwapBuffers( )
{
	// need to flush the dynamic buffer
	FlushBufferedPrimitives();

	if (!IsDeactivated())
	{
		RECORD_COMMAND( DX8_END_SCENE, 0 );
		m_pD3DDevice->EndScene();
	}

	if ((m_IsResizing) || (m_ViewHWnd != m_HWnd))
	{
		RECT destRect;
		GetClientRect( m_ViewHWnd, &destRect );

		RECT srcRect;
		srcRect.left = m_DynamicState.m_Viewport.X;
		srcRect.right = m_DynamicState.m_Viewport.X + m_DynamicState.m_Viewport.Width;
		srcRect.top = m_DynamicState.m_Viewport.Y;
		srcRect.bottom = m_DynamicState.m_Viewport.Y + m_DynamicState.m_Viewport.Height;

		RECORD_COMMAND( DX8_PRESENT, 2 );
		RECORD_STRUCT( &srcRect, sizeof(srcRect) ); 
		RECORD_STRUCT( &destRect, sizeof(destRect) ); 

		m_pD3DDevice->Present( &srcRect, &destRect, m_ViewHWnd, 0 );
    }
	else
	{
		RECORD_COMMAND( DX8_PRESENT, 0 );

		m_pD3DDevice->Present( 0, 0, 0, 0 );
	}

	DiscardVertexBuffers();

	CheckDeviceLost();

#ifdef RECORD_KEYFRAMES
	static int frame = 0;
	++frame;
	if (frame == KEYFRAME_INTERVAL)
	{
		RECORD_COMMAND( DX8_KEYFRAME, 0 );

		ResetRenderState();
		frame = 0;
	}
#endif

	if( !IsDeactivated() )
	{
		bool clearDepth = SupportsHardwareLighting() != 0;

		if (ShaderUtil()->GetConfig().bMeasureFillRate || 
			ShaderUtil()->GetConfig().bVisualizeFillRate )
		{
			ClearBuffers( true, clearDepth, -1, -1 );
		}
		
		RECORD_COMMAND( DX8_BEGIN_SCENE, 0 );
		m_pD3DDevice->BeginScene();
	}
}


//-----------------------------------------------------------------------------
// Use this to get the mesh builder that allows us to modify vertex data
//-----------------------------------------------------------------------------

CMeshBuilder* CShaderAPIDX8::GetVertexModifyBuilder()
{
	return &m_ModifyBuilder;
}


//-----------------------------------------------------------------------------
// returns the current time in seconds....
//-----------------------------------------------------------------------------
double CShaderAPIDX8::CurrentTime() const
{
	// FIXME: Return game time instead of real time!
	// Or eliminate this altogether and put it into a material var
	// (this is used by vertex modifiers in shader code at the moment)
	return Plat_FloatTime();
}


//-----------------------------------------------------------------------------
// Methods called by the transition table that use dynamic state...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ApplyZBias( ShadowState_t const& shaderState )
{
	MaterialSystem_Config_t &config = ShaderUtil()->GetConfig();
    float a = config.m_SlopeScaleDepthBias_Decal;
    float b = config.m_SlopeScaleDepthBias_Normal;
    float c = config.m_DepthBias_Decal;
    float d = config.m_DepthBias_Normal;

	// GR - hack for R200
	bool bPS14Only = m_Caps.m_SupportsPixelShaders_1_4 && !m_Caps.m_SupportsPixelShaders_2_0;
	if( ( m_nVendorId == 0x1002 ) && bPS14Only )
	{
		c /= 8.888889f;
		d /= 8.888889f;
	}

    if( m_Caps.m_ZBiasSupported && m_Caps.m_SlopeScaledDepthBiasSupported ) 
    { 
		SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, shaderState.m_ZBias ? *((DWORD*) (&a) ) : *((DWORD*) (&b) ) ); 
		SetRenderState( D3DRS_DEPTHBIAS, shaderState.m_ZBias ? *((DWORD*) (&c) ) : *((DWORD*) (&d) ) ); 
    } 
	else if (m_Caps.m_ZBiasSupported)
	{
		SetRenderState( D3DRS_DEPTHBIAS, shaderState.m_ZBias ? 1 : 0 );
	}
	else
	{
		m_DynamicState.m_TransformChanged[MATERIAL_PROJECTION] |= 
			STATE_CHANGED_VERTEX_SHADER | STATE_CHANGED_FIXED_FUNCTION;
	}
}


void CShaderAPIDX8::ApplyTextureEnable( ShadowState_t const& state, int stage )
{
	if (state.m_TextureStage[stage].m_TextureEnable)
	{
		TextureStage(stage).m_TextureEnable = true;
		// Don't do this here!!  It ends up giving us extra texture sets.
		// We'll Assert in debug mode if you enable a texture stage
		// but don't bind a texture.
		// see CShaderAPIDX8::RenderPass() for this check.
		// NOTE: We aren't doing this optimization quite yet.  There are situations
		// where you want a texture stage enabled for its texture coordinates, but
		// you don't actually bind a texture (texmvspec for example.)
		SetTextureState( stage, TextureStage(stage).m_BoundTexture, true );
	}
	else
	{
		TextureStage(stage).m_TextureEnable = false;
		SetTextureState(stage, -1);
	}
}

void CShaderAPIDX8::ApplyCullEnable( bool bEnable )
{ 
	SetRenderState( D3DRS_CULLMODE, bEnable ? m_DynamicState.m_CullMode : D3DCULL_NONE );
}

//-----------------------------------------------------------------------------
// Used to clear the transition table when we know it's become invalid.
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ClearSnapshots()
{
	m_TransitionTable.Reset();
	InitRenderState();
}


static void KillTranslation( D3DXMATRIX& mat )
{
	mat[3] = 0.0f;
	mat[7] = 0.0f;
	mat[11] = 0.0f;
	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = 0.0f;
	mat[15] = 1.0f;
}

static void PrintMatrix( const char *name, const D3DXMATRIX& mat )
{
	int row, col;
	char buf[128];

	OutputDebugString( name );
	OutputDebugString( "\n" );
	for( row = 0; row < 4; row++ )
	{
		OutputDebugString( "    " );
		for( col = 0; col < 4; col++ )
		{
			sprintf( buf, "%f ", ( float )mat( row, col ) );
			OutputDebugString( buf );
		}
		OutputDebugString( "\n" );
	}
	OutputDebugString( "\n" );
}


//-----------------------------------------------------------------------------
// Gets the vertex format for a particular snapshot id
//-----------------------------------------------------------------------------
VertexFormat_t CShaderAPIDX8::ComputeVertexUsage( int num, StateSnapshot_t* pIds ) const
{
	if (num == 0)
		return 0;

	// We don't have to all sorts of crazy stuff if there's only one snapshot
	if (num == 1)
		return m_TransitionTable.GetSnapshot(pIds[0]).m_VertexUsage;

	Assert( pIds );

	// Aggregating vertex formats is a little tricky;
	// For example, what do we do when two passes want user data? 
	// Can we assume they are the same? For now, I'm going to
	// just print a warning in debug.

	int userDataSize = 0;
	int numBones = 0;
	int numTexCoords = 0;
	int texCoordSize[4] = { 0, 0, 0, 0 };
	int flags = 0;

	for (int i = num; --i >= 0; )
	{
		VertexFormat_t fmt = m_TransitionTable.GetSnapshot(pIds[i]).m_VertexUsage;
		flags |= VertexFlags(fmt);

		// Too sneaky for my own good; probably shouldn't be 
		// putting this flag here...
		if (fmt & VERTEX_PREPROCESS_DATA)
			flags |= VERTEX_PREPROCESS_DATA;

		int newNumBones = NumBoneWeights(fmt);
		if ((numBones != newNumBones) && (newNumBones != 0))
		{
			if (numBones != 0)
			{
				Warning("Encountered a material with two passes that use different numbers of bones!\n");
			}
			numBones = newNumBones;
		}

		int newUserSize = UserDataSize(fmt);
		if ((userDataSize != newUserSize) && (newUserSize != 0))
		{
			if (userDataSize != 0)
			{
				Warning("Encountered a material with two passes that use different user data sizes!\n");
			}
			userDataSize = newUserSize;
		}

		if (numTexCoords < NumTextureCoordinates(fmt) )
			numTexCoords = NumTextureCoordinates(fmt);

		for (int j = 0; j < numTexCoords; ++j)
		{
			int newSize = TexCoordSize( (TextureStage_t)j, fmt);
			if ((texCoordSize[j] != newSize) && (newSize != 0))
			{
				if (texCoordSize[j] != 0) 
				{
					Warning("Encountered a material with two passes that use different texture coord sizes!\n");
				}
				texCoordSize[j] = newSize;
			}
		}
	}

	return MeshMgr()->ComputeVertexFormat( flags, numTexCoords, 
		texCoordSize, numBones, userDataSize );
}

VertexFormat_t CShaderAPIDX8::ComputeVertexFormat( int num, StateSnapshot_t* pIds ) const
{
	VertexFormat_t fmt = ComputeVertexUsage( num, pIds );
	return MeshMgr()->ComputeStandardFormat( fmt );
}


//-----------------------------------------------------------------------------
// Gets the current buffered state... (debug only)
//-----------------------------------------------------------------------------
void CShaderAPIDX8::GetBufferedState( BufferedState_t& state )
{
	memcpy( &state.m_Transform[0], &GetTransform(MATERIAL_MODEL), sizeof(D3DXMATRIX) ); 
	memcpy( &state.m_Transform[1], &GetTransform(MATERIAL_VIEW), sizeof(D3DXMATRIX) ); 
	memcpy( &state.m_Transform[2], &GetTransform(MATERIAL_PROJECTION), sizeof(D3DXMATRIX) ); 
	memcpy( &state.m_Viewport, &m_DynamicState.m_Viewport, sizeof(state.m_Viewport) );
	state.m_PixelShader = ShaderManager()->GetCurrentPixelShader();
	state.m_VertexShader = ShaderManager()->GetCurrentVertexShader();
	for (int i = 0; i < GetNumTextureUnits(); ++i)
	{
		state.m_BoundTexture[i] = m_DynamicState.m_TextureStage[i].m_BoundTexture;
	}
}


//-----------------------------------------------------------------------------
// constant color methods
//-----------------------------------------------------------------------------

void CShaderAPIDX8::Color3f( float r, float g, float b )
{
	unsigned int color = D3DCOLOR_ARGB( 255, (int)(r * 255), 
		(int)(g * 255), (int)(b * 255) );
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color4f( float r, float g, float b, float a )
{
	unsigned int color = D3DCOLOR_ARGB( (int)(a * 255), (int)(r * 255), 
		(int)(g * 255), (int)(b * 255) );
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color3fv( float const *c )
{
	Assert( c );
	unsigned int color = D3DCOLOR_ARGB( 255, (int)(c[0] * 255), 
		(int)(c[1] * 255), (int)(c[2] * 255) );
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color4fv( float const *c )
{
	Assert( c );
	unsigned int color = D3DCOLOR_ARGB( (int)(c[3] * 255), (int)(c[0] * 255), 
		(int)(c[1] * 255), (int)(c[2] * 255) );
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	unsigned int color = D3DCOLOR_ARGB( 255, r, g, b ); 
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color3ubv( unsigned char const* pColor )
{
	Assert( pColor );
	unsigned int color = D3DCOLOR_ARGB( 255, pColor[0], pColor[1], pColor[2] ); 
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	unsigned int color = D3DCOLOR_ARGB( a, r, g, b );
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::Color4ubv( unsigned char const* pColor )
{
	Assert( pColor );
	unsigned int color = D3DCOLOR_ARGB( pColor[3], pColor[0], pColor[1], pColor[2] ); 
	if (color != m_DynamicState.m_ConstantColor)
	{
		m_DynamicState.m_ConstantColor = color;
		SetRenderState( D3DRS_TEXTUREFACTOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// The shade mode
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ShadeMode( ShaderShadeMode_t mode )
{
	D3DSHADEMODE shadeMode = (mode == SHADER_FLAT) ? D3DSHADE_FLAT : D3DSHADE_GOURAUD;
	if (m_DynamicState.m_ShadeMode != shadeMode)
	{
		m_DynamicState.m_ShadeMode = shadeMode;
		SetRenderState( D3DRS_SHADEMODE, shadeMode );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Point size...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::PointSize( float size )
{
	if (size != m_DynamicState.m_PointSize)
	{
		FlushBufferedPrimitives();
		SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&size));
		m_DynamicState.m_PointSize = size;
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Cull mode..
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CullMode( MaterialCullMode_t cullMode )
{
	D3DCULL newCullMode;
	switch( cullMode )
	{
	case MATERIAL_CULLMODE_CCW:
		// Culls backfacing polys (normal)
		newCullMode = D3DCULL_CCW;
		break;

	case MATERIAL_CULLMODE_CW:
		// Culls frontfacing polys
		newCullMode = D3DCULL_CW;
		break;

	default:
		Warning( "CullMode: invalid cullMode\n" );
		return;
	}

	if (m_DynamicState.m_CullMode != newCullMode)
	{
		FlushBufferedPrimitives();
		m_DynamicState.m_CullMode = newCullMode;
		ApplyCullEnable( m_TransitionTable.CurrentShadowState().m_CullEnable );
	}
}

void CShaderAPIDX8::ForceDepthFuncEquals( bool bEnable )
{
	m_TransitionTable.ForceDepthFuncEquals( bEnable );
}

void CShaderAPIDX8::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
	m_TransitionTable.OverrideDepthEnable( bEnable, bDepthEnable );
}

void CShaderAPIDX8::UpdateFastClipUserClipPlane( void )
{

	float plane[4];
	switch( m_DynamicState.m_HeightClipMode )
	{
	case MATERIAL_HEIGHTCLIPMODE_DISABLE:
		EnableFastClip( false );
		break;
	case MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT:
		plane[0] = 0.0f;
		plane[1] = 0.0f;
		plane[2] = 1.0f;
		plane[3] = m_DynamicState.m_HeightClipZ;
		EnableFastClip( true );
		SetFastClipPlane(plane);
		break;
	case MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT:
		plane[0] = 0.0f;
		plane[1] = 0.0f;
		plane[2] = -1.0f;
		plane[3] = -m_DynamicState.m_HeightClipZ;
		EnableFastClip( true );
		SetFastClipPlane(plane);
		break;
	}
}

void CShaderAPIDX8::SetHeightClipZ( float z )
{
	if( z != m_DynamicState.m_HeightClipZ )
	{
		FlushBufferedPrimitives();
		m_DynamicState.m_HeightClipZ = z;
		UpdateVertexShaderFogParams();
		UpdateFastClipUserClipPlane();
		m_DynamicState.m_TransformChanged[MATERIAL_PROJECTION] |= 
			STATE_CHANGED_VERTEX_SHADER | STATE_CHANGED_FIXED_FUNCTION;
	}
}

void CShaderAPIDX8::SetHeightClipMode( MaterialHeightClipMode_t heightClipMode )
{
	if( heightClipMode != m_DynamicState.m_HeightClipMode )
	{
		FlushBufferedPrimitives();
		m_DynamicState.m_HeightClipMode = heightClipMode;
		UpdateVertexShaderFogParams();
		UpdateFastClipUserClipPlane();
	}
}

void CShaderAPIDX8::SetClipPlane( int index, const float *pPlane )
{
	Assert( index < MaxUserClipPlanes() && index >= 0 );

	// NOTE: The plane here is specified in *world space*
	// NOTE: This is done because they assume Ax+By+Cz+Dw = 0 (where w = 1 in real space)
	// while we use Ax+By+Cz=D
	D3DXPLANE plane;
	plane.a = pPlane[0];
	plane.b = pPlane[1];
	plane.c = pPlane[2];
	plane.d = -pPlane[3];

	if ( plane != m_DynamicState.m_UserClipPlaneWorld[index] )
	{
		FlushBufferedPrimitives();

		m_DynamicState.m_UserClipPlaneChanged |= ( 1 << index );
		m_DynamicState.m_UserClipPlaneWorld[index] = plane;
	}
}

void CShaderAPIDX8::EnableClipPlane( int index, bool bEnable )
{
	Assert( index < MaxUserClipPlanes() && index >= 0 );
	if( ( m_DynamicState.m_UserClipPlaneEnabled & ( 1 << index ) ? true : false ) !=
		bEnable )
	{
		FlushBufferedPrimitives();
		if( bEnable )
		{
			m_DynamicState.m_UserClipPlaneEnabled |= ( 1 << index );
		}
		else
		{
			m_DynamicState.m_UserClipPlaneEnabled &= ~( 1 << index );
		}
		SetRenderState( D3DRS_CLIPPLANEENABLE, m_DynamicState.m_UserClipPlaneEnabled );
	}
}

void CShaderAPIDX8::SetFastClipPlane(const float *pPlane)
{
	D3DXPLANE plane;
	plane.a = pPlane[0];
	plane.b = pPlane[1];
	plane.c = pPlane[2];
	plane.d = -pPlane[3];
	if (plane != m_DynamicState.m_FastClipPlane)
	{
		m_DynamicState.m_FastClipPlane = plane;

		// Compute worldToProjection - need inv. transpose for transforming plane.
		D3DXMATRIX worldToProjectionInvTrans = GetTransform(MATERIAL_VIEW) * GetTransform(MATERIAL_PROJECTION);
		D3DXMatrixInverse(&worldToProjectionInvTrans, NULL, &worldToProjectionInvTrans);
		D3DXMatrixTranspose(&worldToProjectionInvTrans, &worldToProjectionInvTrans); 
		
		D3DXPlaneNormalize(&plane,&plane);
		D3DXVECTOR4 clipPlane(plane.a, plane.b, plane.c, plane.d);

		// transform clip plane into projection space
		D3DXVec4Transform(&clipPlane, &clipPlane, &worldToProjectionInvTrans);

		D3DXMatrixIdentity(&m_CachedFastClipProjectionMatrix);

		// put projection space clip plane in Z column
		m_CachedFastClipProjectionMatrix(0, 2) = clipPlane.x;
		m_CachedFastClipProjectionMatrix(1, 2) = clipPlane.y;
		m_CachedFastClipProjectionMatrix(2, 2) = clipPlane.z;
		m_CachedFastClipProjectionMatrix(3, 2) = clipPlane.w;

		m_CachedFastClipProjectionMatrix = GetTransform(MATERIAL_PROJECTION) * m_CachedFastClipProjectionMatrix;

		// if polyoffset workaround, then update the cached polyoffset matrix (with clip) too:
		if( ( m_TransitionTable.CurrentSnapshot() != -1 ) && 
			!( m_Caps.m_ZBiasSupported && m_Caps.m_SlopeScaledDepthBiasSupported ) &&
			m_TransitionTable.CurrentShadowState().m_ZBias )
		{
			float offsetVal = 
				-1.0f * (m_DynamicState.m_Viewport.MaxZ - m_DynamicState.m_Viewport.MinZ) /
				16384.0f;

			D3DXMATRIX offset;
			D3DXMatrixTranslation( &offset, 0.0f, 0.0f, offsetVal );		
			D3DXMatrixMultiply( &m_CachedPolyOffsetProjectionMatrixWithClip, &m_CachedFastClipProjectionMatrix, &offset );
		}
	}
}

void CShaderAPIDX8::EnableFastClip(bool bEnable)
{
	m_DynamicState.m_FastClipEnabled = bEnable;
}

//-----------------------------------------------------------------------------
// Vertex blending
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetVertexBlendState( int numBones )
{
	if (numBones < 0)
		numBones = m_DynamicState.m_NumBones;

	// For fixed-function, the number of weights is actually one less than
	// the number of bones
	if (numBones > 0)
		--numBones;

	bool normalizeNormals = true;
	D3DVERTEXBLENDFLAGS vertexBlend;
	switch(numBones)
	{
	case 0:
		vertexBlend = D3DVBF_DISABLE;
		normalizeNormals = false;
		break;

	case 1:
		vertexBlend = D3DVBF_1WEIGHTS;
		break;

	case 2:
		vertexBlend = D3DVBF_2WEIGHTS;
		break;

	case 3:
		vertexBlend = D3DVBF_3WEIGHTS;
		break;

	default:
		vertexBlend = D3DVBF_DISABLE;
		Assert(0);
		break;
	}

	if (m_DynamicState.m_VertexBlend != vertexBlend)
	{
		m_DynamicState.m_VertexBlend  = vertexBlend;
		SetRenderState( D3DRS_VERTEXBLEND, vertexBlend );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}

	// Activate normalize normals when skinning is on
	if (m_DynamicState.m_NormalizeNormals != normalizeNormals)
	{
		m_DynamicState.m_NormalizeNormals  = normalizeNormals;
		SetRenderState( D3DRS_NORMALIZENORMALS, normalizeNormals );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Vertex blending
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetNumBoneWeights( int numBones )
{
	if (m_DynamicState.m_NumBones != numBones)
	{
		FlushBufferedPrimitives();
		m_DynamicState.m_NumBones = numBones;
		SetVertexBlendState( m_TransitionTable.CurrentShadowState().m_VertexBlendEnable ? -1 : 0 );
	}
}


//-----------------------------------------------------------------------------
// Fog methods...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SceneFogMode( MaterialFogMode_t fogMode )
{
	if( m_SceneFogMode != fogMode )
	{
		FlushBufferedPrimitives();
		m_SceneFogMode = fogMode;
	}
}

MaterialFogMode_t CShaderAPIDX8::GetSceneFogMode( )
{
	return m_SceneFogMode;
}


//-----------------------------------------------------------------------------
// Fog methods...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::FogMode( MaterialFogMode_t fogMode )
{
	// NOTE: CurrentSnapshot can be -1 when we're initializing renderstate
	bool useVertexAndPixelShaders = false;
	if ( m_nCurrentSnapshot != - 1 )
	{
		useVertexAndPixelShaders = UsesVertexAndPixelShaders( m_nCurrentSnapshot );
	}

	D3DFOGMODE dxFogMode;
	bool fogEnable;
	if( useVertexAndPixelShaders )
	{
		switch( fogMode )
		{
		default:
			Assert( 0 );
			// fall through
		case MATERIAL_FOG_NONE:
			fogEnable = false;
			m_DynamicState.m_FogHeightEnabled = false;
			break;
		case MATERIAL_FOG_LINEAR:
			fogEnable = true;
			m_DynamicState.m_FogHeightEnabled = false;
			break;
		case MATERIAL_FOG_LINEAR_BELOW_FOG_Z:
			fogEnable = true;
			m_DynamicState.m_FogHeightEnabled = true;
			break;
		}

		// vertex and pixel shaders always need D3DFOG_NONE!
		dxFogMode = D3DFOG_NONE;
	}
	else
	{
		switch( fogMode )
		{
		case MATERIAL_FOG_LINEAR_BELOW_FOG_Z:
			// Sorry, no height fog in fixed function. . .
//			Assert( 0 ); // garymcthack!!  this needs to be here!
			// fall through. . . . 
		case MATERIAL_FOG_LINEAR:
			dxFogMode = D3DFOG_LINEAR;
			fogEnable = true;
			break;
		default:
			Assert( 0 );
			// fall through
		case MATERIAL_FOG_NONE:
			dxFogMode = D3DFOG_NONE;
			fogEnable = false;
			break;
		}
	}

#if 0
	// HACK - do this to disable fog always
	fogEnable = false;
	m_DynamicState.m_FogHeightEnabled = false;
#endif

	// These two are always set to this, so don't bother setting them.
	// We are always using vertex fog.
//	SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
//	SetRenderState( D3DRS_RANGEFOGENABLE, false );

	// Set fog mode if it's different than before.
	if( m_DynamicState.m_FogMode != dxFogMode )
	{
		SetRenderState( D3DRS_FOGVERTEXMODE, dxFogMode );
		m_DynamicState.m_FogMode = dxFogMode;
	}

	// Set fog enable if it's different than before.
	if( fogEnable != m_DynamicState.m_FogEnable )
	{
		SetRenderState( D3DRS_FOGENABLE, fogEnable );
		m_DynamicState.m_FogEnable = fogEnable;
	}

	// Update vertex and pixel shader constants related to fog.
	if( useVertexAndPixelShaders )
	{
		UpdateVertexShaderFogParams();
	}
}


void CShaderAPIDX8::FogStart( float fStart )
{
	if (fStart != m_DynamicState.m_FogStart)
	{
		// need to flush the dynamic buffer
		FlushBufferedPrimitives();

		SetRenderState(D3DRS_FOGSTART, *((DWORD*) (&fStart)));
		m_VertexShaderFogParams[0] = fStart;
		UpdateVertexShaderFogParams();
		m_DynamicState.m_FogStart = fStart;
	}
}

void CShaderAPIDX8::FogEnd( float fEnd )
{
	if (fEnd != m_DynamicState.m_FogEnd)
	{
		// need to flush the dynamic buffer
		FlushBufferedPrimitives();

		SetRenderState(D3DRS_FOGEND, *((DWORD*) (&fEnd)));
		m_VertexShaderFogParams[1] = fEnd;
		UpdateVertexShaderFogParams();
		m_DynamicState.m_FogEnd = fEnd;
	}
}

void CShaderAPIDX8::SetFogZ( float fogZ )
{
	if (fogZ != m_DynamicState.m_FogZ)
	{
		// need to flush the dynamic buffer
		FlushBufferedPrimitives();
		m_DynamicState.m_FogZ = fogZ;
		UpdateVertexShaderFogParams();
	}
}

float CShaderAPIDX8::GetFogStart( void ) const
{
	return m_DynamicState.m_FogStart;
}

float CShaderAPIDX8::GetFogEnd( void ) const
{
	return m_DynamicState.m_FogEnd;
}

void CShaderAPIDX8::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	// FIXME: Should we check here for having the same fog color?
	FlushBufferedPrimitives();
	m_SceneFogColor[0] = r;
	m_SceneFogColor[1] = g;
	m_SceneFogColor[2] = b;
}

void CShaderAPIDX8::GetSceneFogColor( unsigned char *rgb )
{
	rgb[0] = m_SceneFogColor[0];
	rgb[1] = m_SceneFogColor[1];
	rgb[2] = m_SceneFogColor[2];
}

void CShaderAPIDX8::FogColor3f( float r, float g, float b )
{
	unsigned int color = D3DCOLOR_ARGB( 255, (int)(r * 255), 
		(int)(g * 255), (int)(b * 255) );
	if (color != m_DynamicState.m_FogColor)
	{
		m_DynamicState.m_FogColor = color;
		SetRenderState( D3DRS_FOGCOLOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::FogColor3fv( float const* rgb )
{
	Assert(rgb);
	unsigned int color = D3DCOLOR_ARGB( 255, (int)(rgb[0] * 255), 
		(int)(rgb[1] * 255), (int)(rgb[2] * 255) );
	if (color != m_DynamicState.m_FogColor)
	{
		m_DynamicState.m_FogColor = color;
		SetRenderState( D3DRS_FOGCOLOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	unsigned int color = D3DCOLOR_ARGB( 255, r, g, b ); 
	if (color != m_DynamicState.m_FogColor)
	{
		m_DynamicState.m_FogColor = color;
		SetRenderState( D3DRS_FOGCOLOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::FogColor3ubv( unsigned char const* rgb )
{
	Assert(rgb);
	unsigned int color = D3DCOLOR_ARGB( 255, rgb[0], rgb[1], rgb[2] ); 
	if (color != m_DynamicState.m_FogColor)
	{
		m_DynamicState.m_FogColor = color;
		SetRenderState( D3DRS_FOGCOLOR, color );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Some methods chaining vertex + pixel shaders through to the shader manager
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetVertexShaderIndex( int vshIndex )
{
	ShaderManager()->SetVertexShaderIndex( vshIndex );
}

void CShaderAPIDX8::SetPixelShaderIndex( int pshIndex )
{
//	Assert( pshIndex == 0 );
	ShaderManager()->SetPixelShaderIndex( pshIndex );
}


//-----------------------------------------------------------------------------
// Adds/removes precompiled shader dictionaries
//-----------------------------------------------------------------------------
ShaderDLL_t CShaderAPIDX8::AddShaderDLL( )
{
	return ShaderManager()->AddShaderDLL( );
}

void CShaderAPIDX8::RemoveShaderDLL( ShaderDLL_t hShaderDLL )
{
	ShaderManager()->RemoveShaderDLL( hShaderDLL );
}

void CShaderAPIDX8::AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict )
{
	ShaderManager()->AddShaderDictionary( hShaderDLL, pDict );
}

void CShaderAPIDX8::SetShaderDLL( ShaderDLL_t hShaderDLL )
{
	ShaderManager()->SetShaderDLL( hShaderDLL );
}

void CShaderAPIDX8::SyncToken( const char *pToken )
{
	RECORD_COMMAND( DX8_SYNC_TOKEN, 1 );
	RECORD_STRING( pToken );
}


//-----------------------------------------------------------------------------
// Deals with vertex buffers
//-----------------------------------------------------------------------------
void CShaderAPIDX8::DestroyVertexBuffers()
{
	MeshMgr()->DestroyVertexBuffers();
}


//-----------------------------------------------------------------------------
// Sets the constant register for vertex and pixel shaders
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetVertexShaderConstant( int var, float const* pVec, int numVecs, bool bForce )
{
	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_VERTEX_SHADER_CONSTANT_TIME);

	Assert( pVec );
	Assert( var + numVecs <= NumVertexShaderConstants() );

	if( !bForce && memcmp( pVec, &m_DynamicState.m_pVertexShaderConstant[var], numVecs * 4 * sizeof( float ) ) == 0 )
	{
		return;
	}

	RECORD_COMMAND( DX8_SET_VERTEX_SHADER_CONSTANT, 3 );
	RECORD_INT( var );
	RECORD_INT( numVecs );
	RECORD_STRUCT( pVec, numVecs * 4 * sizeof(float) );

	HRESULT hr = m_pD3DDevice->SetVertexShaderConstantF( var, pVec, numVecs );
	Assert( !FAILED(hr) );
	g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

	memcpy( &m_DynamicState.m_pVertexShaderConstant[var], pVec, numVecs * 4 * sizeof(float) );
}

void CShaderAPIDX8::SetPixelShaderConstant( int var, float const* pVec, int numVecs, bool bForce )
{
	Assert( pVec );
	Assert( var + numVecs <= NumPixelShaderConstants() );

	if( !bForce )
	{
		DWORD* pSrc = (DWORD*)pVec;
		DWORD* pDst = (DWORD*)&m_DynamicState.m_pPixelShaderConstant[var];
		int count = numVecs * 4;

		for(int i=0; i<count; i++) 
		{
			if( pSrc[i] != pDst[i] )
				break;
		}
		
		// If they are the same, get outta here:
		if( i == count )
			return;

		//if( memcmp( pVec, &m_DynamicState.m_pPixelShaderConstant[var], numVecs * 4 * sizeof( float ) ) == 0 )
		//	return;
	}

	RECORD_COMMAND( DX8_SET_PIXEL_SHADER_CONSTANT, 3 );
	RECORD_INT( var );
	RECORD_INT( numVecs );
	RECORD_STRUCT( pVec, numVecs * 4 * sizeof(float) );

	HRESULT hr = m_pD3DDevice->SetPixelShaderConstantF( var, pVec, numVecs );
	Assert( !FAILED(hr) );
	g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

	memcpy( &m_DynamicState.m_pPixelShaderConstant[var], pVec, numVecs * 4 * sizeof(float) );
}

//-----------------------------------------------------------------------------
//
// Methods dealing with texture stage state
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Gets the texture associated with a texture state...
//-----------------------------------------------------------------------------

inline IDirect3DBaseTexture* CShaderAPIDX8::GetD3DTexture( int idx )
{
	if (idx == -1)
		return 0;

	Texture_t& tex = GetTexture( idx );
	if (tex.m_NumCopies == 1)
		return tex.GetTexture();
	else
		return tex.GetTexture( tex.m_CurrentCopy );
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------

inline int CShaderAPIDX8::GetModifyTextureIndex() const
{
	return m_ModifyTextureIdx;
}

inline IDirect3DBaseTexture* CShaderAPIDX8::GetModifyTexture()
{
	return GetD3DTexture(m_ModifyTextureIdx);
}

IDirect3DBaseTexture** CShaderAPIDX8::GetModifyTextureReference()
{
	if (m_ModifyTextureIdx == -1)
		return 0;
	
	Texture_t& tex = GetTexture( m_ModifyTextureIdx );
	return tex.m_NumCopies == 1 ? &tex.GetTexture() : &tex.GetTexture( tex.m_CurrentCopy );
}

inline int CShaderAPIDX8::GetBoundTextureBindId( int textureStage ) const
{
	if (TextureStage(textureStage).m_BoundTexture != -1)
		return m_Textures[ TextureStage(textureStage).m_BoundTexture].m_BindId;
	return -1;
}


//-----------------------------------------------------------------------------
// Sets state on the board related to the texture state
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetTextureState( int stage, int textureIdx, bool force )
{
	// Disabling texturing
	if (textureIdx == -1)
	{
		RECORD_COMMAND( DX8_SET_TEXTURE, 3 );
		RECORD_INT( stage );
		RECORD_INT( -1 );
		RECORD_INT( -1 );

		m_pD3DDevice->SetTexture( stage, 0 );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_TEXTURE_STATE, 1 );
		return;
	}

	// Get the dynamic texture info
	TextureStageState_t& tsState = TextureStage(stage);

	// Set the texture state, but only if it changes
	int prevBindIdx = tsState.m_BoundTexture;
	if ((prevBindIdx == textureIdx) && (!force))
		return;

	tsState.m_BoundTexture = textureIdx;

	// Don't set this if we're disabled
	if (!tsState.m_TextureEnable)
		return;

	RECORD_COMMAND( DX8_SET_TEXTURE, 3 );
	RECORD_INT( stage );
	RECORD_INT( GetTexture( textureIdx ).m_BindId );
	RECORD_INT( GetTexture( textureIdx) .m_CurrentCopy );

	HRESULT hr = m_pD3DDevice->SetTexture( stage, GetD3DTexture(textureIdx) );
	Assert( !FAILED(hr) );

	g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_TEXTURE_STATE, 1 );
	g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_TEXELS, 
		GetTexture( textureIdx ).m_SizeTexels );
	g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_BYTES, 
		GetTexture( textureIdx ).m_SizeBytes );

	if (GetTexture( textureIdx ).m_LastBoundFrame != m_CurrentFrame)
	{
		if (GetTexture( textureIdx ).m_LastBoundFrame != m_CurrentFrame - 1)
		{
			g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_NEW_THIS_FRAME, 
				GetTexture( textureIdx ).m_SizeBytes );
		}

		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXELS, 
			GetTexture( textureIdx ).m_SizeTexels );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_UNIQUE_BYTES, 
			GetTexture( textureIdx ).m_SizeBytes );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXTURES, 1 );
		GetTexture( textureIdx ).m_LastBoundFrame = m_CurrentFrame;
	}

	if (tsState.m_UTexWrap != GetTexture( textureIdx ).m_UTexWrap)
	{
		tsState.m_UTexWrap = GetTexture( textureIdx ).m_UTexWrap;
		SetSamplerState( stage, D3DSAMP_ADDRESSU, tsState.m_UTexWrap );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
	if (tsState.m_VTexWrap != GetTexture( textureIdx ).m_VTexWrap)
	{
		tsState.m_VTexWrap = GetTexture( textureIdx ).m_VTexWrap;
		SetSamplerState( stage, D3DSAMP_ADDRESSV, tsState.m_VTexWrap );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}

	bool noFilter = ShaderUtil()->GetConfig().bFilterTextures == 0;

	D3DTEXTUREFILTERTYPE minFilter = noFilter ? D3DTEXF_NONE : GetTexture( textureIdx ).m_MinFilter;
	if (tsState.m_MinFilter != minFilter)
	{
		tsState.m_MinFilter = minFilter;
		SetSamplerState( stage, D3DSAMP_MINFILTER, tsState.m_MinFilter );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
	D3DTEXTUREFILTERTYPE magFilter = noFilter ? D3DTEXF_NONE : GetTexture( textureIdx ).m_MagFilter;
	if (tsState.m_MagFilter != magFilter)
	{
		tsState.m_MagFilter = magFilter;
		SetSamplerState( stage, D3DSAMP_MAGFILTER, tsState.m_MagFilter );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}

	bool noMipFilter = ShaderUtil()->GetConfig().bMipMapTextures == 0;

	D3DTEXTUREFILTERTYPE mipFilter = noMipFilter ? D3DTEXF_NONE : GetTexture( textureIdx ).m_MipFilter;
	if (tsState.m_MipFilter != mipFilter)
	{
		tsState.m_MipFilter = mipFilter;
		SetSamplerState( stage, D3DSAMP_MIPFILTER, tsState.m_MipFilter );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::BindTexture( TextureStage_t stage, unsigned int textureNum )
{
	int idx;
	if (textureNum == -1)
	{
		SetTextureState( stage, -1 );
	}
	else
	{
		if ( FindTexture( textureNum, idx ) )
		{
			SetTextureState( stage, idx );
		}
	}
}

//-----------------------------------------------------------------------------
// Texture allocation/deallocation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Finds a texture
//-----------------------------------------------------------------------------

bool CShaderAPIDX8::FindTexture( int id, int& idx )
{
	// FIXME: SLOW! Need to make this system hand out Ids, not be given them
	// Use binary search...
	int start = 0, end = GetTextureCount() - 1;
	int mid;
	while (start <= end)
	{
		mid = (start + end) >> 1;
		if (GetTexture( mid ).m_BindId == id)
		{
			idx = mid;
			return true;
		}

		if (GetTexture( mid ).m_BindId < id )
			start = mid + 1;
		else
			end = mid - 1;
	}

	// Where we should insert
	idx = start;
	return false;
}


//-----------------------------------------------------------------------------
// Computes stats info for a texture
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ComputeStatsInfo( unsigned int textureIdx, bool isCubeMap )
{
	GetTexture( textureIdx ).m_SizeBytes = 0;
	GetTexture( textureIdx ).m_SizeTexels = 0;
	GetTexture( textureIdx ).m_LastBoundFrame = -1;

	IDirect3DBaseTexture* pD3DTex = GetD3DTexture( textureIdx );
	if (isCubeMap)
	{
		IDirect3DCubeTexture* pTex = static_cast<IDirect3DCubeTexture*>(pD3DTex);
		if( !pTex )
		{
			Assert( 0 );
			return;
		}
		int numLevels = pTex->GetLevelCount();
		for (int i = 0; i < numLevels; ++i)
		{
			D3DSURFACE_DESC desc;
			HRESULT hr = pTex->GetLevelDesc( i, &desc );
			Assert( !FAILED(hr) );
// Why does D3DSURFACE_DESC no longer have a "Size" member?  Is there a helper for getting this value?
//			Assert( 0 ); // fixme
			GetTexture( textureIdx ).m_SizeBytes = 0;
			GetTexture( textureIdx ).m_SizeTexels += 6 * desc.Width * desc.Height;
		}
	}
	else
	{
		IDirect3DTexture* pTex = static_cast<IDirect3DTexture*>(pD3DTex);
		if( !pTex )
		{
			Assert( 0 );
			return;
		}
		int numLevels = pTex->GetLevelCount();
		for (int i = 0; i < numLevels; ++i)
		{
			D3DSURFACE_DESC desc;
			HRESULT hr = pTex->GetLevelDesc( i, &desc );
			Assert( !FAILED(hr) );
// Why does D3DSURFACE_DESC no longer have a "Size" member?  Is there a helper for getting this value?
//			Assert( 0 ); // fixme
			GetTexture( textureIdx ).m_SizeBytes = 0;
			GetTexture( textureIdx ).m_SizeTexels += desc.Width * desc.Height;
		}
	}
}

static D3DFORMAT ComputeFormat( IDirect3DBaseTexture* pTexture, bool isCubeMap )
{
	Assert( pTexture );
	D3DSURFACE_DESC desc;
	if (isCubeMap)
	{
		IDirect3DCubeTexture* pTex = static_cast<IDirect3DCubeTexture*>(pTexture);
		HRESULT hr = pTex->GetLevelDesc( 0, &desc );
		Assert( !FAILED(hr) );
	}
	else
	{
		IDirect3DTexture* pTex = static_cast<IDirect3DTexture*>(pTexture);
		HRESULT hr = pTex->GetLevelDesc( 0, &desc );
		Assert( !FAILED(hr) );
	}
	return desc.Format;
}

//-----------------------------------------------------------------------------
// This is only used for statistics gathering..
//-----------------------------------------------------------------------------

void CShaderAPIDX8::SetLightmapTextureIdRange( int firstId, int lastId )
{
	m_FirstLightmapId = firstId;
	m_LastLightmapId = lastId;
}

void CShaderAPIDX8::CreateDepthTexture( unsigned int textureNum, ImageFormat renderTargetFormat, int width, int height )
{
	// This will change all the bind indices; we need to save this info off
	int temp[NUM_TEXTURE_STAGES];
	StoreBindIndices( temp );

	// Check to see if this is a texture we already know about...
	int i;
	bool found = FindTexture( textureNum, i );
	if ( found )
	{
		// Ok, we're done with the old version....
		DeleteTexture( textureNum );
	}
	else
	{
		// Didn't find it, make a new one
		i = m_Textures.InsertBefore( i );
	}
	
#ifdef _DEBUG
	Texture_t *pTexture = &GetTexture( i );
#endif

	GetTexture( i ).m_Flags = 0;
	GetTexture( i ).m_Flags |= Texture_t::IS_DEPTH_STENCIL;
	GetTexture( i ).m_NumCopies = 1;

	D3DFORMAT renderFormat = ImageFormatToD3DFormat(FindNearestRenderTargetFormat(renderTargetFormat));
	D3DFORMAT format = FindNearestSupportedDepthFormat( D3DFMT_D24S8, renderFormat );
	D3DMULTISAMPLE_TYPE multisampleType = D3DMULTISAMPLE_NONE;

	RECORD_COMMAND( DX8_CREATE_DEPTH_TEXTURE, 5 );
	RECORD_INT( textureNum );
	RECORD_INT( width );
	RECORD_INT( height );
	RECORD_INT( format );
	RECORD_INT( multisampleType );
	
	HRESULT hr = D3DDevice()->CreateDepthStencilSurface(
		width, height, format, multisampleType, 0, TRUE, &GetTexture( i ).GetDepthStencilSurface(), NULL );

    if ( FAILED(hr) )
	{
		switch( hr )
		{
		case D3DERR_INVALIDCALL:
			Warning( "ShaderAPIDX8::CreateD3DTexture: D3DERR_INVALIDCALL\n" );
			break;
		case D3DERR_OUTOFVIDEOMEMORY:
			Warning( "ShaderAPIDX8::CreateD3DTexture: D3DERR_OUTOFVIDEOMEMORY\n" );
			break;
		default:
			break;
		}
		Assert( 0 );
	}

	GetTexture( i ).m_BindId = textureNum;

	// Restore the bind indices
	RestoreBindIndices( temp );
}


//-----------------------------------------------------------------------------
// Creates a lovely texture
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CreateTexture( unsigned int textureNum, int width, int height, 
	ImageFormat dstImageFormat, int numMipLevels, int numCopies, int creationFlags )
{
	bool isCubeMap = (creationFlags & TEXTURE_CREATE_CUBEMAP) != 0;
	bool isRenderTarget = (creationFlags & TEXTURE_CREATE_RENDERTARGET) != 0;
	bool managed = (creationFlags & TEXTURE_CREATE_MANAGED) != 0;
	bool isDepthBuffer = (creationFlags & TEXTURE_CREATE_DEPTHBUFFER) != 0;
	bool isDynamic = (creationFlags & TEXTURE_CREATE_DYNAMIC) != 0;

	// Can't be both managed + dynamic. Dynamic is an optimization, but 
	// if it's not managed, then we gotta do special client-specific stuff
	// So, managed wins out!
	if ( managed )
		isDynamic = false;

	// This will change all the bind indices; we need to save this info off
	int temp[NUM_TEXTURE_STAGES];
	StoreBindIndices( temp );

	// Check to see if this is a texture we already know about...
	int i;
	bool found = FindTexture( textureNum, i );
	if ( found )
	{
		// Ok, we're done with the old version....
		DeleteTexture( textureNum );
	}
	else
	{
		// Didn't find it, make a new one
		i = m_Textures.InsertBefore( i );
	}

#ifdef _DEBUG
	Texture_t *pTexture = &GetTexture( i );
#endif
	
	GetTexture( i ).m_Flags = 0;

	RECORD_COMMAND( DX8_CREATE_TEXTURE, 11 );
	RECORD_INT( textureNum );
	RECORD_INT( width );
	RECORD_INT( height );
	RECORD_INT( ImageFormatToD3DFormat( FindNearestSupportedFormat(dstImageFormat)) );
	RECORD_INT( numMipLevels );
	RECORD_INT( isCubeMap );
	RECORD_INT( numCopies <= 1 ? 1 : numCopies );
	RECORD_INT( isRenderTarget ? 1 : 0 );
	RECORD_INT( managed );
	RECORD_INT( isDepthBuffer ? 1 : 0 );
	RECORD_INT( isDynamic ? 1 : 0 );

	// Set the initial texture state
	if (numCopies <= 1)
	{
		GetTexture( i ).m_NumCopies = 1;
		GetTexture( i ).GetTexture() = 
			CreateD3DTexture( width, height, dstImageFormat, numMipLevels, creationFlags );
	}
	else
	{
		GetTexture( i ).m_NumCopies = numCopies;
		GetTexture( i ).GetTextureArray() = new IDirect3DBaseTexture* [numCopies];
		for (int k = 0; k < numCopies; ++k)
		{
			GetTexture( i ).GetTexture( k ) = 
				CreateD3DTexture( width, height, dstImageFormat, numMipLevels, creationFlags );
		}
	}
	GetTexture( i ).m_CurrentCopy = 0;

	IDirect3DBaseTexture* pD3DTex = GetD3DTexture( i );

	GetTexture( i ).m_BindId = textureNum;
	GetTexture( i ).m_UTexWrap = D3DTADDRESS_CLAMP;
	GetTexture( i ).m_VTexWrap = D3DTADDRESS_CLAMP;
	GetTexture( i ).m_MagFilter = D3DTEXF_LINEAR;
	if( isRenderTarget )
	{
		GetTexture( i ).m_NumLevels = 1;
		GetTexture( i ).m_MipFilter = D3DTEXF_NONE;
		GetTexture( i ).m_MinFilter = D3DTEXF_LINEAR;
	}
	else
	{
		GetTexture( i ).m_NumLevels = pD3DTex ? pD3DTex->GetLevelCount() : 1;
		GetTexture( i ).m_MipFilter = (GetTexture( i ).m_NumLevels != 1) ? 
			D3DTEXF_LINEAR : D3DTEXF_NONE;
		GetTexture( i ).m_MinFilter = D3DTEXF_LINEAR;
	}
	GetTexture( i ).m_SwitchNeeded = false;
	GetTexture( i ).m_MipFilter = (GetTexture( i ).m_NumLevels != 1) ? 
		D3DTEXF_LINEAR : D3DTEXF_NONE;
	ComputeStatsInfo( i, isCubeMap );


	// Keep track of total allocation size
	if ((GetTexture( i ).m_BindId < m_FirstLightmapId) ||
		(GetTexture( i ).m_BindId > m_LastLightmapId))
	{
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_ALLOCATED, 
			GetTexture( i ).m_SizeBytes * GetTexture( i ).m_NumCopies, true );
	}
	else
	{
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_LIGHTMAP_BYTES_ALLOCATED, 
			GetTexture( i ).m_SizeBytes * GetTexture( i ).m_NumCopies, true );
	}

	// Restore the bind indices
	RestoreBindIndices( temp );
}


//-----------------------------------------------------------------------------
// Deletes a texture...
//-----------------------------------------------------------------------------

void CShaderAPIDX8::DeleteD3DTexture( int idx )
{
	int numDeallocated = 0;

	if( GetTexture( idx ).m_Flags & Texture_t::IS_DEPTH_STENCIL  )
	{
		// garymcthack - need to make sure that playback knows how to deal with these.
		RECORD_COMMAND( DX8_DESTROY_DEPTH_TEXTURE, 1 );
		RECORD_INT( GetTexture( idx ).m_BindId );

		GetTexture( idx ).GetDepthStencilSurface()->Release();
		GetTexture( idx ).GetDepthStencilSurface() = 0;
		numDeallocated = 1;
	}
	else if (GetTexture( idx ).m_NumCopies == 1)
	{
		if (GetTexture( idx ).GetTexture())
		{
			RECORD_COMMAND( DX8_DESTROY_TEXTURE, 1 );
			RECORD_INT( GetTexture( idx ).m_BindId );

			DestroyD3DTexture( GetTexture( idx ).GetTexture() );
			GetTexture( idx ).GetTexture() = 0;
			numDeallocated = 1;
		}
	}
	else
	{
		if (GetTexture( idx ).GetTextureArray())
		{
			RECORD_COMMAND( DX8_DESTROY_TEXTURE, 1 );
			RECORD_INT( GetTexture( idx ).m_BindId );

			// Multiple copy texture
			for (int j = 0; j < GetTexture( idx ).m_NumCopies; ++j)
			{
				if (GetTexture( idx ).GetTexture( j ))
				{
					DestroyD3DTexture( GetTexture( idx ).GetTexture( j ) );
					GetTexture( idx ).GetTexture( j ) = 0;
					++numDeallocated;
				}
			}

			delete[] GetTexture( idx ).GetTextureArray();
			GetTexture( idx ).GetTextureArray() = 0;
		}
	}

	// Keep track of total allocation size
	if ((GetTexture( idx ).m_BindId < m_FirstLightmapId) ||
		(GetTexture( idx ).m_BindId > m_LastLightmapId))
	{
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_ALLOCATED, 
			-GetTexture( idx ).m_SizeBytes * numDeallocated, true );
	}
	else
	{
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_LIGHTMAP_BYTES_ALLOCATED, 
			-GetTexture( idx ).m_SizeBytes * numDeallocated, true );
	}

	GetTexture( idx ).m_NumCopies = 0;
}


//-----------------------------------------------------------------------------
// Unbinds a texture from all texture stages
//-----------------------------------------------------------------------------
void CShaderAPIDX8::UnbindTexture( int textureIdx )
{
	// Make sure no texture units are currently bound to it...
	for (int unit = 0; unit < GetNumTextureUnits(); ++unit )
	{
		if (textureIdx == TextureStage(unit).m_BoundTexture)
		{
			// Gotta set this here because -1 means don't actually
			// set bound texture (it's used for disabling texturemapping)
			TextureStage(unit).m_BoundTexture = -1;
			SetTextureState( unit, -1 );
		}
	}
}


//-----------------------------------------------------------------------------
// Saves/restores all bind indices...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::StoreBindIndices( int* pBindId )
{
	for ( int j = 0; j < GetNumTextureUnits(); ++j)
	{
		pBindId[j] = GetBoundTextureBindId( j );
	}
}

void CShaderAPIDX8::RestoreBindIndices( int* pBindId )
{
	for ( int j = 0; j < GetNumTextureUnits(); ++j)
	{
		if (pBindId[j] == -1)
		{
			TextureStage(j).m_BoundTexture = -1;
		}
		else
		{
			bool found;
			found = FindTexture( pBindId[j], TextureStage(j).m_BoundTexture );
			Assert( found );
		}
	}
}

//-----------------------------------------------------------------------------
// Deletes a texture...
//-----------------------------------------------------------------------------

void CShaderAPIDX8::DeleteTexture( unsigned int textureNum )
{
	int i;
	bool found = FindTexture( textureNum, i );
	if (found)
	{
		// Delete it baby
		DeleteD3DTexture( i );

		// Unbind it!
		UnbindTexture( i );
		
		// Now remove the texture from the list
		// This will change all the bind indices; we need to save this info off
		int temp[NUM_TEXTURE_STAGES];
		StoreBindIndices( temp );

		m_Textures.Remove( i );

		// Restore the bind indices
		RestoreBindIndices( temp );
	}
}

//-----------------------------------------------------------------------------
// Releases all textures
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ReleaseAllTextures()
{
	for ( int i = GetTextureCount(); --i >= 0; )
	{
		// Delete it baby
		DeleteD3DTexture( i );
	}

	// Make sure all texture units are pointing to nothing
	for (int unit = 0; unit < GetNumTextureUnits(); ++unit )
	{
		TextureStage(unit).m_BoundTexture = -1;
		SetTextureState(unit, -1);
	}
}

void CShaderAPIDX8::DeleteAllTextures()
{
	ReleaseAllTextures();
	m_Textures.Purge();
}

bool CShaderAPIDX8::IsTexture( unsigned int textureNum )
{
	int i;
	bool found = FindTexture( textureNum, i );

	if( !found )
	{
		return false;
	}

	if( GetTexture( i ).m_Flags & Texture_t::IS_DEPTH_STENCIL )
	{
		return GetTexture( i ).GetDepthStencilSurface() != 0;
	}
	else if( (GetTexture( i ).m_NumCopies == 1 && GetTexture( i ).GetTexture() != 0) ||
		(GetTexture( i ).m_NumCopies > 1 && GetTexture( i ).GetTexture( 0 ) != 0) )
	{
		return true;
	}
	else
	{
		return false;
	}
}



//-----------------------------------------------------------------------------
// Gets the surface associated with a texture (refcount of surface is increased)
//-----------------------------------------------------------------------------
IDirect3DSurface* CShaderAPIDX8::GetTextureSurface( unsigned int tex )
{
	IDirect3DSurface* pSurface;

	// We'll be modifying this sucka
	int renderTargetID;
	bool found = FindTexture( tex, renderTargetID );
	Assert( found );
	if( !found )
		return NULL;

	IDirect3DBaseTexture* pD3DTex = GetD3DTexture( renderTargetID );
	IDirect3DTexture* pTex = static_cast<IDirect3DTexture*>( pD3DTex );
	if( !pTex )
	{
		Assert( 0 );
		return NULL;
	}
	
	HRESULT hr = pTex->GetSurfaceLevel( 0, &pSurface );
	Assert( hr == D3D_OK );

	return pSurface;
}

//-----------------------------------------------------------------------------
// Gets the surface associated with a texture (refcount of surface is increased)
//-----------------------------------------------------------------------------
IDirect3DSurface* CShaderAPIDX8::GetDepthTextureSurface( unsigned int tex )
{
	int renderTargetID;
	bool found = FindTexture( tex, renderTargetID );
	Assert( found );
	if( !found )
		return NULL;

	return GetTexture( renderTargetID ).GetDepthStencilSurface();
}



//-----------------------------------------------------------------------------
// Changes the render target
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetRenderTarget( unsigned int colorTexture, unsigned int depthTexture )
{
	IDirect3DSurface* pColorSurface;
	IDirect3DSurface* pZSurface;

	RECORD_COMMAND( DX8_SET_RENDER_TARGET, 2 );
	RECORD_INT( colorTexture );
	RECORD_INT( depthTexture );

	// GR - need to flush batched geometry
	FlushBufferedPrimitives();

	// NOTE!!!!  If this code changes, also change Dx8SetRenderTarget in playback.cpp
	bool usingTextureTarget = false;
	if( colorTexture == SHADER_RENDERTARGET_BACKBUFFER )
	{
		pColorSurface = m_pBackBufferSurface;

		// This is just to make the code a little simpler...
		// (simplifies the release logic)
		pColorSurface->AddRef();
	}
	else
	{
		pColorSurface = GetTextureSurface( colorTexture );
		if (!pColorSurface)
			return;

		usingTextureTarget = true;
	}

	if ( depthTexture == SHADER_RENDERTARGET_DEPTHBUFFER)
	{
		pZSurface = m_pZBufferSurface;

		// This is just to make the code a little simpler...
		// (simplifies the release logic)
		pZSurface->AddRef();
	}
	else
	{
		pZSurface = GetDepthTextureSurface( depthTexture );
		pZSurface->AddRef();
		if (!pZSurface)
		{
			// Refcount of color surface was increased above
			pColorSurface->Release();
			return;
		}
		usingTextureTarget = true;
	}

#ifdef _DEBUG
	D3DSURFACE_DESC zSurfaceDesc, colorSurfaceDesc;
	pZSurface->GetDesc( &zSurfaceDesc );
	pColorSurface->GetDesc( &colorSurfaceDesc );
	Assert( colorSurfaceDesc.Width <= zSurfaceDesc.Width );
	Assert( colorSurfaceDesc.Height <= zSurfaceDesc.Height );
#endif

	m_UsingTextureRenderTarget = usingTextureTarget;

	// NOTE: The documentation says that SetRenderTarget increases the refcount
	// but it doesn't appear to in practice. If this somehow changes (perhaps
	// in a device-specific manner, we're in trouble).
	m_pD3DDevice->SetRenderTarget( 0, pColorSurface );
	m_pD3DDevice->SetDepthStencilSurface( pZSurface );
	int ref = pZSurface->Release();
	Assert( ref != 0 );
	ref = pColorSurface->Release();
	Assert( ref != 0 );

	if (m_UsingTextureRenderTarget)
	{
		D3DSURFACE_DESC  desc;
		HRESULT hr = pColorSurface->GetDesc( &desc );
		Assert( !FAILED(hr) );
		m_ViewportMaxWidth = desc.Width;
		m_ViewportMaxHeight = desc.Height;
	}
}


//-----------------------------------------------------------------------------
// Returns the nearest supported format
//-----------------------------------------------------------------------------
ImageFormat CShaderAPIDX8::GetNearestSupportedFormat( ImageFormat fmt ) const
{
	return FindNearestSupportedFormat(fmt);
}

ImageFormat CShaderAPIDX8::GetNearestRenderTargetFormat( ImageFormat fmt ) const
{
	return FindNearestRenderTargetFormat(fmt);
}


//-----------------------------------------------------------------------------
// Indicates we're modifying a texture
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ModifyTexture( unsigned int textureNum )
{
	// Can't do this if we're locked!
	Assert( m_ModifyTextureLockedLevel < 0 );

	// We'll be modifying this sucka
	bool found = FindTexture( textureNum, m_ModifyTextureIdx );
	Assert ( found );

	// If we're got a multi-copy texture, we need to up the current copy count
	Texture_t& tex = GetTexture( m_ModifyTextureIdx );
	if (tex.m_NumCopies > 1)
	{
		// Each time we modify a texture, we'll want to switch texture
		// as soon as a TexImage2D call is made...
		tex.m_SwitchNeeded = true;
	}
}


//-----------------------------------------------------------------------------
// Advances the current copy of a texture...
//-----------------------------------------------------------------------------
void CShaderAPIDX8::AdvanceCurrentCopy( int textureIdx )
{
	// May need to switch textures....
	Texture_t& tex = GetTexture( textureIdx );
	if (tex.m_NumCopies > 1)
	{
		if (++tex.m_CurrentCopy >= tex.m_NumCopies)
			tex.m_CurrentCopy = 0;

		// When the current copy changes, we need to make sure this texture
		// isn't bound to any stages any more; thereby guaranteeing the new
		// copy will be re-bound.
		UnbindTexture( textureIdx );
	}
}


//-----------------------------------------------------------------------------
// Locks, unlocks the current texture
//-----------------------------------------------------------------------------

bool CShaderAPIDX8::TexLock( int level, int cubeFaceID, int xOffset, int yOffset, 
								int width, int height, CPixelWriter& writer )
{
	Assert( m_ModifyTextureLockedLevel < 0 );

	if ( (GetModifyTextureIndex() < 0) || (GetModifyTextureIndex() > GetTextureCount()) )
		return false;

	// Blow off mip levels if we don't support mipmapping
	if (!SupportsMipmapping() && (level > 0))
		return false;

	// Do the dirty deed (see TextureDX8.cpp)
	// This test here just makes sure we don't try to download mipmap levels
	// if we weren't able to create them in the first place
	Texture_t& tex = GetTexture( GetModifyTextureIndex() );
	if (tex.m_NumLevels <= level)
		return false;

	// May need to switch textures....
	if (tex.m_SwitchNeeded)
	{
		AdvanceCurrentCopy( GetModifyTextureIndex() );
		tex.m_SwitchNeeded = false;
	}

	bool ok = LockTexture( tex.m_BindId, tex.m_CurrentCopy, *GetModifyTextureReference(),
		level, (D3DCUBEMAP_FACES)cubeFaceID, xOffset, yOffset, width, height, writer );
	if (ok)
	{
		m_ModifyTextureLockedLevel = level;
		m_ModifyTextureLockedFace = cubeFaceID;
	}
	return ok;
}

void CShaderAPIDX8::TexUnlock( )
{
	if( m_ModifyTextureLockedLevel >= 0 )
	{
		Texture_t& tex = GetTexture( GetModifyTextureIndex() );
		UnlockTexture( tex.m_BindId, tex.m_CurrentCopy, *GetModifyTextureReference(),
			m_ModifyTextureLockedLevel, (D3DCUBEMAP_FACES)m_ModifyTextureLockedFace );

		m_ModifyTextureLockedLevel = -1;
	}
}

//-----------------------------------------------------------------------------
// Texture image upload
//-----------------------------------------------------------------------------

void CShaderAPIDX8::TexImage2D( int level, int cubeFaceID, ImageFormat dstFormat, int width, 
					int height, ImageFormat srcFormat, void *pSrcData )
{
	Assert( pSrcData );
	if ( (GetModifyTextureIndex() < 0) || (GetModifyTextureIndex() > GetTextureCount()) )
		return;

	Assert( (width <= m_Caps.m_MaxTextureWidth) && (height <= m_Caps.m_MaxTextureHeight) );

	// Blow off mip levels if we don't support mipmapping
	if (!SupportsMipmapping() && (level > 0))
		return;

	// Do the dirty deed (see TextureDX8.cpp)
	// This test here just makes sure we don't try to download mipmap levels
	// if we weren't able to create them in the first place
	Texture_t& tex = GetTexture( GetModifyTextureIndex() );
	if (tex.m_NumLevels > level)
	{
		// May need to switch textures....
		if (tex.m_SwitchNeeded)
		{
			AdvanceCurrentCopy( GetModifyTextureIndex() );
			tex.m_SwitchNeeded = false;
		}

		LoadTexture( tex.m_BindId, tex.m_CurrentCopy, *GetModifyTextureReference(),
			level, (D3DCUBEMAP_FACES)cubeFaceID, dstFormat, width, height,
			srcFormat, pSrcData );
	}
}

//-----------------------------------------------------------------------------
// Upload to a sub-piece of a texture
//-----------------------------------------------------------------------------

void CShaderAPIDX8::TexSubImage2D( int level, int cubeFaceID, int xOffset, int yOffset, 
	int width, int height, ImageFormat srcFormat, int srcStride, void *pSrcData )
{
	Assert( pSrcData );
	if ( (GetModifyTextureIndex() < 0) || (GetModifyTextureIndex() > GetTextureCount()) )
		return;

	// Blow off mip levels if we don't support mipmapping
	if (!SupportsMipmapping() && (level > 0))
		return;

	// NOTE: This can only be done with procedural textures if this method is
	// being used to download the entire texture, cause last frame's partial update
	// may be in a completely different texture! Sadly, I don't have all of the
	// information I need, but I can at least check a couple things....
#ifdef _DEBUG
	if (GetTexture( GetModifyTextureIndex() ).m_NumCopies > 1)
	{
		Assert( (xOffset == 0) && (yOffset == 0) );
	}
#endif

	// Do the dirty deed (see TextureDX8.cpp)
	// This test here just makes sure we don't try to download mipmap levels
	// if we weren't able to create them in the first place (happens for cubemaps
	// on Radeon, for example).
	Texture_t& tex = GetTexture( GetModifyTextureIndex() );
	if (tex.m_NumLevels > level)
	{
		// May need to switch textures....
		if (tex.m_SwitchNeeded)
		{
			AdvanceCurrentCopy( GetModifyTextureIndex() );
			tex.m_SwitchNeeded = false;
		}

		LoadSubTexture( tex.m_BindId, tex.m_CurrentCopy, GetModifyTexture(), level,
			(D3DCUBEMAP_FACES)cubeFaceID, xOffset, yOffset, width, height, srcFormat, srcStride, pSrcData );
	}
}


//-----------------------------------------------------------------------------
// Is the texture resident?
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::IsTextureResident( unsigned int textureNum )
{
	return true;
}


//-----------------------------------------------------------------------------
// Level of anisotropic filtering
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetAnisotropicLevel( int nAnisotropyLevel )
{
	if ( nAnisotropyLevel > m_Caps.m_nMaxAnisotropy )
	{
		nAnisotropyLevel = m_Caps.m_nMaxAnisotropy;
	}

	for ( int i = 0; i < m_Caps.m_NumTextureUnits; ++i)
	{
		TextureStage(i).m_nAnisotropyLevel = nAnisotropyLevel;
		SetSamplerState( i, D3DSAMP_MAXANISOTROPY, TextureStage(i).m_nAnisotropyLevel );
	}
}

//-----------------------------------------------------------------------------
// Level of anisotropic filtering
//-----------------------------------------------------------------------------
void CShaderAPIDX8::EnableSRGBRead( TextureStage_t stage, bool bEnable )
{
	if( TextureStage( stage ).m_bSRGBReadEnabled != bEnable )
	{
		TextureStage( stage ).m_bSRGBReadEnabled = bEnable;
		SetSamplerState( stage, D3DSAMP_SRGBTEXTURE, bEnable );
	}
}

//-----------------------------------------------------------------------------
// Sets the priority
//-----------------------------------------------------------------------------
void CShaderAPIDX8::TexSetPriority( int priority )
{
	// A hint to the cacher...
	int modifyTextureIndex = GetModifyTextureIndex();
	if (modifyTextureIndex == -1)
		return;

	Texture_t& tex = GetTexture( modifyTextureIndex );
	if (tex.m_NumCopies > 1)
	{
		for (int i = 0; i < tex.m_NumCopies; ++i)
			tex.GetTexture( i )->SetPriority( priority );
	}
	else
	{
		tex.GetTexture()->SetPriority( priority );
	}
}


//-----------------------------------------------------------------------------
// Texturemapping state
//-----------------------------------------------------------------------------
void CShaderAPIDX8::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
	int modifyTextureIndex = GetModifyTextureIndex();
	if (modifyTextureIndex == -1)
		return;

	D3DTEXTUREADDRESS address;
	switch( wrapMode )
	{
	case SHADER_TEXWRAPMODE_CLAMP:
		address = D3DTADDRESS_CLAMP;
		break;
	case SHADER_TEXWRAPMODE_REPEAT:
		address = D3DTADDRESS_WRAP;
		break; 
	default:
		address = D3DTADDRESS_CLAMP;
		Warning( "CShaderAPIDX8::TexWrap: unknown wrapMode\n" );
		break;
	}

	switch( coord )
	{
	case SHADER_TEXCOORD_S:
		GetTexture( modifyTextureIndex ).m_UTexWrap = address;
		break;
	case SHADER_TEXCOORD_T:
		GetTexture( modifyTextureIndex ).m_VTexWrap = address;
		break;
	default:
		Warning( "CShaderAPIDX8::TexWrap: unknown coord\n" );
		break;
	}
}

void CShaderAPIDX8::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{
	int modifyTextureIndex = GetModifyTextureIndex();
	if (modifyTextureIndex == -1)
		return;

	switch( texFilterMode )
	{
	case SHADER_TEXFILTERMODE_NEAREST:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_POINT;
		GetTexture( modifyTextureIndex ).m_MipFilter = D3DTEXF_NONE;
		break;
	case SHADER_TEXFILTERMODE_LINEAR:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_LINEAR;
		GetTexture( modifyTextureIndex ).m_MipFilter = D3DTEXF_NONE;
		break;
	case SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_POINT;
		GetTexture( modifyTextureIndex ).m_MipFilter = 
			GetTexture( modifyTextureIndex ).m_NumLevels != 1 ? D3DTEXF_POINT : D3DTEXF_NONE;
		break;
	case SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_LINEAR;
		GetTexture( modifyTextureIndex ).m_MipFilter = 
			GetTexture( modifyTextureIndex ).m_NumLevels != 1 ? D3DTEXF_POINT : D3DTEXF_NONE;
		break;
	case SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_POINT;
		GetTexture( modifyTextureIndex ).m_MipFilter = 
			GetTexture( modifyTextureIndex ).m_NumLevels != 1 ? D3DTEXF_LINEAR : D3DTEXF_NONE;
		break;
	case SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_LINEAR;
		GetTexture( modifyTextureIndex ).m_MipFilter = 
			GetTexture( modifyTextureIndex ).m_NumLevels != 1 ? D3DTEXF_LINEAR : D3DTEXF_NONE;
		break;
	case SHADER_TEXFILTERMODE_ANISOTROPIC:
		GetTexture( modifyTextureIndex ).m_MinFilter = D3DTEXF_ANISOTROPIC;
		GetTexture( modifyTextureIndex ).m_MipFilter = 
			GetTexture( modifyTextureIndex ).m_NumLevels != 1 ? D3DTEXF_LINEAR : D3DTEXF_NONE;
		break;
	default:
		Warning( "CShaderAPIDX8::TexMinFilter: Unknown texFilterMode\n" );
		break;
	}
}

void CShaderAPIDX8::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{
	int modifyTextureIndex = GetModifyTextureIndex();
	if (modifyTextureIndex == -1)
		return;

	switch( texFilterMode )
	{
	case SHADER_TEXFILTERMODE_NEAREST:
		GetTexture( modifyTextureIndex ).m_MagFilter = D3DTEXF_POINT;
		break;
	case SHADER_TEXFILTERMODE_LINEAR:
		GetTexture( modifyTextureIndex ).m_MagFilter = D3DTEXF_LINEAR;
		break;
	case SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST:
		Warning( "CShaderAPIDX8::TexMagFilter: SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST is invalid\n" );
		break;
	case SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST:
		Warning( "CShaderAPIDX8::TexMagFilter: SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST is invalid\n" );
		break;
	case SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR:
		Warning( "CShaderAPIDX8::TexMagFilter: SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR is invalid\n" );
		break;
	case SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR:
		Warning( "CShaderAPIDX8::TexMagFilter: SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR is invalid\n" );
		break;
	case SHADER_TEXFILTERMODE_ANISOTROPIC:
		GetTexture( modifyTextureIndex ).m_MagFilter = D3DTEXF_ANISOTROPIC;
		break;
	default:
		Warning( "CShaderAPIDX8::TexMAGFilter: Unknown texFilterMode\n" );
		break;
	}
}


//-----------------------------------------------------------------------------
// Gets the matrix stack from the matrix mode
//-----------------------------------------------------------------------------

int CShaderAPIDX8::GetMatrixStack( MaterialMatrixMode_t mode ) const
{
	Assert( mode >= 0 && mode < NUM_MATRIX_MODES );
	return mode;
}


//-----------------------------------------------------------------------------
// Generate the spheremapping coordinates 
//-----------------------------------------------------------------------------

void CShaderAPIDX8::GenerateSpheremapCoordinates( CMeshBuilder& meshBuilder, int stage )
{
	meshBuilder.Reset();

	D3DXVECTOR3 reflection, point, normal;
	D3DXMATRIX* pModel = &GetTransform(MATERIAL_MODEL);
	D3DXMATRIX* pView = &GetTransform(MATERIAL_VIEW);
	D3DXMATRIX mModelView = *pModel * *pView;
	for ( int i = 0; i < meshBuilder.NumVertices(); ++i )
	{
		// Put point and normal into camera space
		D3DXVec3TransformCoord( &point, 
			(D3DXVECTOR3*)meshBuilder.Position(), &mModelView ); 
		D3DXVec3Normalize( &point, &point );

		D3DXVec3TransformNormal( &normal, 
			(D3DXVECTOR3*)meshBuilder.Normal(), &mModelView ); 

		// Compute reflection vector
		float ndotp = D3DXVec3Dot( &point, &normal );
		D3DXVec3Scale( &reflection, &normal, - 2 * ndotp );
		D3DXVec3Add( &reflection, &point, &reflection );
		
		// See openGL 1.1 red book 2nd ed, pg 371 for this computation
		reflection[2] += 1.0F;
		float length = D3DXVec3Length( &reflection );

		meshBuilder.TexCoord2f( stage, 
			0.5f * (reflection[0] / length + 1.0f),
			-0.5f * (reflection[1] / length + 1.0f) );
		meshBuilder.AdvanceVertex();
	}
}


//-----------------------------------------------------------------------------
// Modulate the vertex color 
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ModulateVertexColor( CMeshBuilder& meshBuilder )
{
	meshBuilder.Reset();

	float factor[4];
	unsigned char* pColor = (unsigned char*)&m_DynamicState.m_ConstantColor;
	factor[0] = pColor[0] / 255.0f;
	factor[1] = pColor[1] / 255.0f;
	factor[2] = pColor[2] / 255.0f;
	factor[3] = pColor[3] / 255.0f;

	for ( int i = 0; i < meshBuilder.NumVertices(); ++i )
	{
		unsigned int color = meshBuilder.Color();
		unsigned char* pVertexColor = (unsigned char*)&color;

		pVertexColor[0] = (unsigned char)((float)pVertexColor[0] * factor[0]);
		pVertexColor[1] = (unsigned char)((float)pVertexColor[1] * factor[1]);
		pVertexColor[2] = (unsigned char)((float)pVertexColor[2] * factor[2]);
		pVertexColor[3] = (unsigned char)((float)pVertexColor[3] * factor[3]);
		meshBuilder.Color4ubv( pVertexColor );

		meshBuilder.AdvanceVertex();
	}
}


//-----------------------------------------------------------------------------
// Are we spheremapping?
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::IsSpheremapRenderStateActive( int stage ) const
{
	return m_TransitionTable.CurrentShadowState().m_TextureStage[stage].m_GenerateSphericalCoords;
}


//-----------------------------------------------------------------------------
// Returns true if we're modulating constant color into the vertex color
//-----------------------------------------------------------------------------
bool CShaderAPIDX8::IsModulatingVertexColor() const
{
	return m_TransitionTable.CurrentShadowState().m_ModulateConstantColor;
}


//-----------------------------------------------------------------------------
// Returns the current cull mode
//-----------------------------------------------------------------------------
D3DCULL CShaderAPIDX8::GetCullMode() const
{
	return m_TransitionTable.CurrentShadowState().m_CullEnable ? m_DynamicState.m_CullMode : D3DCULL_NONE;
}


//-----------------------------------------------------------------------------
// Material property (used to deal with overbright for lights)
//-----------------------------------------------------------------------------

void CShaderAPIDX8::SetMaterialProperty( float color )
{
	// Overbrighting on stage 0 affects color...
	if (m_DynamicState.m_MaterialOverbrightVal != color)
	{
		D3DMATERIAL mat;
		mat.Diffuse.r = mat.Diffuse.g = mat.Diffuse.b = mat.Diffuse.a = color;
		mat.Ambient.r = mat.Ambient.g = mat.Ambient.b = mat.Ambient.a = color;
		mat.Specular.r = mat.Specular.g = mat.Specular.b = mat.Specular.a = color;
		mat.Emissive.r = mat.Emissive.g = mat.Emissive.b = mat.Emissive.a = 0.0f;
		mat.Power = 8.0f;

		RECORD_COMMAND( DX8_SET_MATERIAL, 1 );
		RECORD_STRUCT( &mat, sizeof(mat) );

		m_pD3DDevice->SetMaterial( &mat );

		m_DynamicState.m_MaterialOverbrightVal = color;

		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

	}
}

//-----------------------------------------------------------------------------
// lighting related methods
//-----------------------------------------------------------------------------

void CShaderAPIDX8::SetAmbientLight( float r, float g, float b )
{
	unsigned int ambient = D3DCOLOR_ARGB( 255, (int)(r * 255), 
		(int)(g * 255), (int)(b * 255) );
	if (ambient != m_DynamicState.m_Ambient)
	{
		m_DynamicState.m_Ambient = ambient;
		SetRenderState( D3DRS_AMBIENT, ambient );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//#define NO_LOCAL_LIGHTS
void CShaderAPIDX8::SetLight( int lightNum, 
#ifdef NO_LOCAL_LIGHTS
							 LightDesc_t& desc_ 
#else
							 LightDesc_t& desc 
#endif
							 )
{
#ifdef NO_LOCAL_LIGHTS
	LightDesc_t desc = desc_;
	desc.m_Type = MATERIAL_LIGHT_DISABLE;
#endif
	Assert( lightNum < MAX_NUM_LIGHTS && lightNum >= 0 );
	
	m_DynamicState.m_LightDescs[lightNum] = desc;
	
	FlushBufferedPrimitives();

	if (desc.m_Type == MATERIAL_LIGHT_DISABLE)
	{
		if (m_DynamicState.m_LightEnable[lightNum])
		{
			m_DynamicState.m_LightEnableChanged[lightNum] = STATE_CHANGED;
			m_DynamicState.m_LightEnable[lightNum] = false;
		}
		return;
	}

	if (!m_DynamicState.m_LightEnable[lightNum])
	{
		m_DynamicState.m_LightEnableChanged[lightNum] = STATE_CHANGED;
		m_DynamicState.m_LightEnable[lightNum] = true;
	}

	D3DLIGHT light;
	switch( desc.m_Type )
	{
	case MATERIAL_LIGHT_POINT:
		light.Type = D3DLIGHT_POINT;
		light.Range = desc.m_Range;
		break;

	case MATERIAL_LIGHT_DIRECTIONAL:
		light.Type = D3DLIGHT_DIRECTIONAL;
		light.Range = 1e12;	// This is supposed to be ignored
		break;

	case MATERIAL_LIGHT_SPOT:
		light.Type = D3DLIGHT_SPOT;
		light.Range = desc.m_Range;
		break;

	default:
		m_DynamicState.m_LightEnable[lightNum] = false;
		return;
	}

	// This is a D3D limitation
	Assert( (light.Range >= 0) && (light.Range <= sqrt(FLT_MAX)) );

	memcpy( &light.Diffuse, &desc.m_Color[0], 3*sizeof(float) );
	memcpy( &light.Specular, &desc.m_Color[0], 3*sizeof(float) );
	light.Diffuse.a = 1.0f;
	light.Specular.a = 1.0f;
	light.Ambient.a = light.Ambient.b = light.Ambient.g = light.Ambient.r = 0;
	memcpy( &light.Position, &desc.m_Position[0], 3 * sizeof(float) );
	memcpy( &light.Direction, &desc.m_Direction[0], 3 * sizeof(float) );
	light.Falloff = desc.m_Falloff;
	light.Attenuation0 = desc.m_Attenuation0;
	light.Attenuation1 = desc.m_Attenuation1;
	light.Attenuation2 = desc.m_Attenuation2;

	// normalize light color...
	light.Theta = desc.m_Theta;
	light.Phi = desc.m_Phi;
	if (light.Phi > M_PI)
		light.Phi = M_PI;

	// This piece of crap line of code is because if theta gets too close to phi,
	// we get no light at all.
	if (light.Theta - light.Phi > -1e-3)
		light.Theta = light.Phi - 1e-3;

	m_DynamicState.m_LightChanged[lightNum] = STATE_CHANGED;
	memcpy( &m_DynamicState.m_Lights[lightNum], &light, sizeof(light) );
}

int CShaderAPIDX8::GetMaxLights( void ) const
{
	return MAX_NUM_LIGHTS;
}

const LightDesc_t& CShaderAPIDX8::GetLight( int lightNum ) const
{
	Assert( lightNum < MAX_NUM_LIGHTS && lightNum >= 0 );
	return m_DynamicState.m_LightDescs[lightNum];
}

//-----------------------------------------------------------------------------
// Ambient cube 
//-----------------------------------------------------------------------------

//#define NO_AMBIENT_CUBE 1
void CShaderAPIDX8::SetAmbientLightCube( Vector4D cube[6] )
{
/*
	int i;
	for( i = 0; i < 6; i++ )
	{
		ColorClamp( cube[i].AsVector3D() );
//		if( i == 0 )
//		{
//			Warning( "%d: %f %f %f\n", i, cube[i][0], cube[i][1], cube[i][2] );
//		}
	}
*/
	if (memcmp(&m_DynamicState.m_AmbientLightCube[0][0], cube, 6 * sizeof(Vector4D)))
	{
		memcpy( &m_DynamicState.m_AmbientLightCube[0][0], cube, 6 * sizeof(Vector4D) );

#ifdef NO_AMBIENT_CUBE
		memset( &m_DynamicState.m_AmbientLightCube[0][0], 0, 6 * sizeof(Vector4D) );
#endif

//#define DEBUG_AMBIENT_CUBE

#ifdef DEBUG_AMBIENT_CUBE
		m_DynamicState.m_AmbientLightCube[0][0] = 1.0f;
		m_DynamicState.m_AmbientLightCube[0][1] = 0.0f;
		m_DynamicState.m_AmbientLightCube[0][2] = 0.0f;

		m_DynamicState.m_AmbientLightCube[1][0] = 0.0f;
		m_DynamicState.m_AmbientLightCube[1][1] = 1.0f;
		m_DynamicState.m_AmbientLightCube[1][2] = 0.0f;

		m_DynamicState.m_AmbientLightCube[2][0] = 0.0f;
		m_DynamicState.m_AmbientLightCube[2][1] = 0.0f;
		m_DynamicState.m_AmbientLightCube[2][2] = 1.0f;

		m_DynamicState.m_AmbientLightCube[3][0] = 1.0f;
		m_DynamicState.m_AmbientLightCube[3][1] = 0.0f;
		m_DynamicState.m_AmbientLightCube[3][2] = 1.0f;

		m_DynamicState.m_AmbientLightCube[4][0] = 1.0f;
		m_DynamicState.m_AmbientLightCube[4][1] = 1.0f;
		m_DynamicState.m_AmbientLightCube[4][2] = 0.0f;

		m_DynamicState.m_AmbientLightCube[5][0] = 0.0f;
		m_DynamicState.m_AmbientLightCube[5][1] = 1.0f;
		m_DynamicState.m_AmbientLightCube[5][2] = 1.0f;
#endif

		m_CachedAmbientLightCube = STATE_CHANGED;
	}
}

void CShaderAPIDX8::SetVertexShaderStateAmbientLightCube()
{
	if (m_CachedAmbientLightCube & STATE_CHANGED_VERTEX_SHADER)
	{
		SetVertexShaderConstant( VERTEX_SHADER_AMBIENT_LIGHT, 
			m_DynamicState.m_AmbientLightCube[0].Base(), 6 );
		m_CachedAmbientLightCube &= ~STATE_CHANGED_VERTEX_SHADER;
	}
}

void CShaderAPIDX8::BindAmbientLightCubeToStage0( )
{
	// See if we've gotta recompute the ambient light cube...
	if (m_CachedAmbientLightCube & STATE_CHANGED_FIXED_FUNCTION)
	{
		RecomputeAmbientLightCube();
		m_CachedAmbientLightCube &= ~STATE_CHANGED_FIXED_FUNCTION;
	}

	BindTexture( SHADER_TEXTURE_STAGE0, TEXTURE_AMBIENT_CUBE );
}

void CShaderAPIDX8::CopyRenderTargetToTexture( int texID )
{
	VPROF( "CShaderAPIDX8::CopyRenderTargetToTexture" );

	int idx;
	// hack!
	if( !FindTexture( texID, idx ) )
	{
		Assert( 0 );
		return;
	}

	IDirect3DSurface* pRenderTargetSurface;
	HRESULT hr = m_pD3DDevice->GetRenderTarget( 0, &pRenderTargetSurface );
	if (FAILED(hr))
	{
		Assert( 0 );
		return;
	}

	// Don't flush here!!  If you have to flush here, then there is a driver bug.
	//	FlushHardware( );

	Texture_t *pTexture = &GetTexture( idx );
	Assert( pTexture );
	IDirect3DTexture *pD3DTexture = ( IDirect3DTexture * )pTexture->GetTexture();;
	Assert( pD3DTexture );

	IDirect3DSurface *pDstSurf;
	hr = pD3DTexture->GetSurfaceLevel( 0, &pDstSurf );
	Assert( !FAILED( hr ) );
	if( FAILED( hr ) )
	{
		return;
	}

	bool tryblit = true;

	if ( tryblit )
	{
		RECORD_COMMAND( DX8_COPY_FRAMEBUFFER_TO_TEXTURE, 1 );
		RECORD_INT( texID );
		
		hr = m_pD3DDevice->StretchRect( pRenderTargetSurface, NULL, pDstSurf, NULL, D3DTEXF_NONE );
		Assert( !FAILED( hr ) );
	}

	pDstSurf->Release();
	pRenderTargetSurface->Release();
}

static float ComputeFixedUpComponent( int val, int numTexels )
{
	// FIXME: Maybe this would look better using equal subdivision sizes?

	// This is necessary because cubemaps aren't interpolated across
	// cubemap boundaries
	if (val == 0)
		return -1.0f;
	if (val == numTexels - 1)
		return 1.0f;

	// Interpolate
	return 2.0f * (float)val / (float)(numTexels-1) - 1.0f;

	// Choose the center of the texels in the standard case
	return 2.0f * (0.5f + (float)val) / (float)numTexels - 1.0f;
}


struct AmbientCubeComp_t
{
	int m_Sample1, m_Sample2, m_Sample3;
	float m_Scale1, m_Scale2, m_Scale3;
};

typedef AmbientCubeComp_t AmbientCubeFactors_t[6][AMBIENT_CUBE_TEXTURE_SIZE][AMBIENT_CUBE_TEXTURE_SIZE];

static void PrecomputeAmbientCubeScaleFactors( AmbientCubeFactors_t& ambientCube )
{
	D3DXVECTOR3 dir, unitDir;
	int normalComp[6] =	{  0,  0,  1,  1,  2,  2 };
	int normalDir[6]  = {  1, -1,  1, -1,  1, -1 };

	// NOTE: Bug in DX8 docs; direction of x is wrong for the -z face 
	int uComp[6] =		{  2,  2,  0,  0,  0,  0 };
	int uDir[6] =		{ -1,  1,  1,  1,  1, -1 };

	int vComp[6] =		{  1,  1,  2,  2,  1,  1 };
	int vDir[6] =		{ -1, -1,  1, -1, -1, -1 };

	for (int i = 0; i < 6; ++i)
	{
		dir[normalComp[i]] = (normalDir[i] < 0) ? -1.0f : 1.0f;

		for (int j = 0; j < AMBIENT_CUBE_TEXTURE_SIZE; ++j)
		{
			dir[vComp[i]] = ComputeFixedUpComponent(j, AMBIENT_CUBE_TEXTURE_SIZE );
			if (vDir[i] < 0)
				dir[vComp[i]] *= -1.0f;

			for (int k = 0; k < AMBIENT_CUBE_TEXTURE_SIZE; ++k)
			{
				dir[uComp[i]] = ComputeFixedUpComponent(k, AMBIENT_CUBE_TEXTURE_SIZE );
				if (uDir[i] < 0)
					dir[uComp[i]] *= -1.0f;

				D3DXVec3Normalize( &unitDir, &dir );

				// Interpolate color using box ambient lighting function
				ambientCube[i][j][k].m_Sample1 = (unitDir[0] > 0.0f) ? 0 : 1;
				ambientCube[i][j][k].m_Sample2 = (unitDir[1] > 0.0f) ? 2 : 3;
				ambientCube[i][j][k].m_Sample3 = (unitDir[2] > 0.0f) ? 4 : 5;

				ambientCube[i][j][k].m_Scale1 = unitDir[0]*unitDir[0];
				ambientCube[i][j][k].m_Scale2 = unitDir[1]*unitDir[1];
				ambientCube[i][j][k].m_Scale3 = unitDir[2]*unitDir[2];
			}
		}
	}
}

inline static void ClampColor( D3DXVECTOR3 const& rgb, int& r, int& g, int& b )
{
	if (rgb[0] >= 1.0f) 
		r = 255;
	else if (rgb[0] <= 0.0f)
		r = 0;
	else
		r = 255.0 * rgb[0];

	if (rgb[1] >= 1.0f) 
		g = 255;
	else if (rgb[1] <= 0.0f)
		g = 0;
	else
		g = 255.0 * rgb[1];

	if (rgb[2] >= 1.0f) 
		b = 255;
	else if (rgb[2] <= 0.0f)
		b = 0;
	else
		b = 255.0 * rgb[2];
}

void CShaderAPIDX8::RecomputeAmbientLightCube( )
{
	static AmbientCubeFactors_t ambientCube;
	static bool initialized = false;

	if (!initialized)
	{
		PrecomputeAmbientCubeScaleFactors( ambientCube );
		initialized = true;
	}

	// Need to do this here since we're bypassing the normal TexImage2D
	// for optimization reasons
	ModifyTexture( TEXTURE_AMBIENT_CUBE ); 
	AdvanceCurrentCopy( GetModifyTextureIndex() );

	Texture_t& tex = GetTexture( GetModifyTextureIndex() );

	IDirect3DCubeTexture* pTex = (IDirect3DCubeTexture*)GetModifyTexture();

	D3DSURFACE_DESC desc;
	HRESULT hr = pTex->GetLevelDesc( 0, &desc );
	ImageFormat fmt = D3DFormatToImageFormat(desc.Format);

	D3DXVECTOR3 rgb;
	CPixelWriter writer;

	int r, g, b;
	for (int i = 0; i < 6; ++i)
	{
		D3DLOCKED_RECT lockedRect;
		IDirect3DSurface* pTextureLevel;

		RECORD_COMMAND( DX8_LOCK_TEXTURE, 5 );
		RECORD_INT( tex.m_BindId );
		RECORD_INT( tex.m_CurrentCopy );
		RECORD_INT( 0 );
		RECORD_INT( i );
		RECORD_INT( D3DLOCK_NOSYSLOCK );

		HRESULT hr = pTex->GetCubeMapSurface( (D3DCUBEMAP_FACES)i, 0, &pTextureLevel );
		hr = pTextureLevel->LockRect( &lockedRect, 0, D3DLOCK_NOSYSLOCK );
		writer.SetPixelMemory( fmt, lockedRect.pBits, lockedRect.Pitch );

		for (int j = 0; j < AMBIENT_CUBE_TEXTURE_SIZE; ++j)
		{
			for (int k = 0; k < AMBIENT_CUBE_TEXTURE_SIZE; ++k)
			{
				Vector4D& c1 = m_DynamicState.m_AmbientLightCube[ambientCube[i][j][k].m_Sample1];
				Vector4D& c2 = m_DynamicState.m_AmbientLightCube[ambientCube[i][j][k].m_Sample2];
				Vector4D& c3 = m_DynamicState.m_AmbientLightCube[ambientCube[i][j][k].m_Sample3];
				
				// Interpolate color using box ambient lighting function
				// also add bias..
				rgb[0] = c1[0] * ambientCube[i][j][k].m_Scale1 + c2[0] * ambientCube[i][j][k].m_Scale2 +
						c3[0] * ambientCube[i][j][k].m_Scale3;
				rgb[1] = c1[1] * ambientCube[i][j][k].m_Scale1 + c2[1] * ambientCube[i][j][k].m_Scale2 +
						c3[1] * ambientCube[i][j][k].m_Scale3;
				rgb[2] = c1[2] * ambientCube[i][j][k].m_Scale1 + c2[2] * ambientCube[i][j][k].m_Scale2 +
						c3[2] * ambientCube[i][j][k].m_Scale3;

				// scale and bias the color to move -1 to 1 -> 0 to 1
				rgb[0] = 0.5f * rgb[0] + 0.5f;
				rgb[1] = 0.5f * rgb[1] + 0.5f;
				rgb[2] = 0.5f * rgb[2] + 0.5f;

				/*
				pDst[0] = 255.0 * (dir[0] + 1) * 0.5f;
				pDst[1] = pDst[2] = 0.0f;
				*/

				// clamp
				ClampColor( rgb, r, g, b );

//				r = g = b = 127;

				/*
				if (i == 0)
				{
					r = 255; g = 0; b = 0;
				} else if (i == 1)
				{
					r = 0; g = 255; b = 0;
				} else if (i == 2)
				{
					r = 0; g = 0; b = 255;
				} else if (i == 3)
				{
					r = 255; g = 255; b = 0;
				} else if (i == 4)
				{
					r = 255; g = 0; b = 255;
				} else if (i == 5)
				{
					r = 0; g = 255; b = 255;
				}
				*/

				writer.WritePixel( r, g, b );
			}
		}

		RECORD_COMMAND( DX8_UNLOCK_TEXTURE, 4 );
		RECORD_INT( tex.m_BindId );
		RECORD_INT( tex.m_CurrentCopy );
		RECORD_INT( 0 );
		RECORD_INT( i );

		hr = pTextureLevel->UnlockRect();
		pTextureLevel->Release();
	}
}

//-----------------------------------------------------------------------------
// modifies the vertex data when necessary
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ModifyVertexData( )
{
	// We have to modulate the vertex color by the constant color sometimes
	if (IsModulatingVertexColor())
		ModulateVertexColor( m_ModifyBuilder );

	// We have to generate spherical coordinates ourselves 
	for (int i = 0; i < GetNumTextureUnits(); ++i)
	{
		if (IsSpheremapRenderStateActive(i))
		{
			// texgenned coordinates always go in tex coord 2
			GenerateSpheremapCoordinates( m_ModifyBuilder, 2 );
			break;
		}
	}
}


static const char *TextureArgToString( int arg )
{
	static char buf[128];
	switch( arg & D3DTA_SELECTMASK )
	{
	case D3DTA_DIFFUSE:
		strcpy( buf, "D3DTA_DIFFUSE" );
		break;
	case D3DTA_CURRENT:
		strcpy( buf, "D3DTA_CURRENT" );
		break;
	case D3DTA_TEXTURE:
		strcpy( buf, "D3DTA_TEXTURE" );
		break;
	case D3DTA_TFACTOR:
		strcpy( buf, "D3DTA_TFACTOR" );
		break;
	case D3DTA_SPECULAR:
		strcpy( buf, "D3DTA_SPECULAR" );
		break;
	case D3DTA_TEMP:
		strcpy( buf, "D3DTA_TEMP" );
		break;
	default:
		strcpy( buf, "<ERROR>" );
		break;
	}

	if( arg & D3DTA_COMPLEMENT )
	{
		strcat( buf, "|D3DTA_COMPLEMENT" );
	}
	if( arg & D3DTA_ALPHAREPLICATE )
	{
		strcat( buf, "|D3DTA_ALPHAREPLICATE" );
	}
	return buf;
}

static const char *TextureOpToString( D3DTEXTUREOP op )
{
	switch( op )
	{
	case D3DTOP_DISABLE:
		return "D3DTOP_DISABLE";
    case D3DTOP_SELECTARG1:
		return "D3DTOP_SELECTARG1";
    case D3DTOP_SELECTARG2:
		return "D3DTOP_SELECTARG2";
    case D3DTOP_MODULATE:
		return "D3DTOP_MODULATE";
    case D3DTOP_MODULATE2X:
		return "D3DTOP_MODULATE2X";
    case D3DTOP_MODULATE4X:
		return "D3DTOP_MODULATE4X";
    case D3DTOP_ADD:
		return "D3DTOP_ADD";
    case D3DTOP_ADDSIGNED:
		return "D3DTOP_ADDSIGNED";
    case D3DTOP_ADDSIGNED2X:
		return "D3DTOP_ADDSIGNED2X";
    case D3DTOP_SUBTRACT:
		return "D3DTOP_SUBTRACT";
    case D3DTOP_ADDSMOOTH:
		return "D3DTOP_ADDSMOOTH";
    case D3DTOP_BLENDDIFFUSEALPHA:
		return "D3DTOP_BLENDDIFFUSEALPHA";
    case D3DTOP_BLENDTEXTUREALPHA:
		return "D3DTOP_BLENDTEXTUREALPHA";
    case D3DTOP_BLENDFACTORALPHA:
		return "D3DTOP_BLENDFACTORALPHA";
    case D3DTOP_BLENDTEXTUREALPHAPM:
		return "D3DTOP_BLENDTEXTUREALPHAPM";
    case D3DTOP_BLENDCURRENTALPHA:
		return "D3DTOP_BLENDCURRENTALPHA";
    case D3DTOP_PREMODULATE:
		return "D3DTOP_PREMODULATE";
    case D3DTOP_MODULATEALPHA_ADDCOLOR:
		return "D3DTOP_MODULATEALPHA_ADDCOLOR";
    case D3DTOP_MODULATECOLOR_ADDALPHA:
		return "D3DTOP_MODULATECOLOR_ADDALPHA";
    case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
		return "D3DTOP_MODULATEINVALPHA_ADDCOLOR";
    case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
		return "D3DTOP_MODULATEINVCOLOR_ADDALPHA";
    case D3DTOP_BUMPENVMAP:
		return "D3DTOP_BUMPENVMAP";
    case D3DTOP_BUMPENVMAPLUMINANCE:
		return "D3DTOP_BUMPENVMAPLUMINANCE";
    case D3DTOP_DOTPRODUCT3:
		return "D3DTOP_DOTPRODUCT3";
    case D3DTOP_MULTIPLYADD:
		return "D3DTOP_MULTIPLYADD";
    case D3DTOP_LERP:
		return "D3DTOP_LERP";
	default:
		return "<ERROR>";
	}
}

static const char *BlendModeToString( int blendMode )
{
	switch( blendMode )
	{
	case D3DBLEND_ZERO:
		return "D3DBLEND_ZERO";
    case D3DBLEND_ONE:
		return "D3DBLEND_ONE";
    case D3DBLEND_SRCCOLOR:
		return "D3DBLEND_SRCCOLOR";
    case D3DBLEND_INVSRCCOLOR:
		return "D3DBLEND_INVSRCCOLOR";
    case D3DBLEND_SRCALPHA:
		return "D3DBLEND_SRCALPHA";
    case D3DBLEND_INVSRCALPHA:
		return "D3DBLEND_INVSRCALPHA";
    case D3DBLEND_DESTALPHA:
		return "D3DBLEND_DESTALPHA";
    case D3DBLEND_INVDESTALPHA:
		return "D3DBLEND_INVDESTALPHA";
    case D3DBLEND_DESTCOLOR:
		return "D3DBLEND_DESTCOLOR";
    case D3DBLEND_INVDESTCOLOR:
		return "D3DBLEND_INVDESTCOLOR";
    case D3DBLEND_SRCALPHASAT:
		return "D3DBLEND_SRCALPHASAT";
    case D3DBLEND_BOTHSRCALPHA:
		return "D3DBLEND_BOTHSRCALPHA";
    case D3DBLEND_BOTHINVSRCALPHA:
		return "D3DBLEND_BOTHINVSRCALPHA";
	default:
		return "<ERROR>";
	}
}


//-----------------------------------------------------------------------------
// Spew Board State
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SpewBoardState()
{
#ifdef DEBUG_BOARD_STATE
	if (0)
	{
		static ID3DXFont* pFont = 0;
		if (!pFont)
		{
			HFONT hFont = CreateFont( 0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
				ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_MODERN, 0 );
			Assert( hFont != 0 );

			HRESULT hr = D3DXCreateFont( m_pD3DDevice, hFont, &pFont );
		}

		static char buf[1024];
		static RECT r = { 0, 0, 640, 480 };

		if (m_DynamicState.m_VertexBlend == 0)
			return;
		
#if 1
		D3DXMATRIX* m = &GetTransform(MATERIAL_MODEL);
		D3DXMATRIX* m2 = &GetTransform(MATERIAL_MODEL + 1);
		sprintf(buf,"FVF %x\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n",
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n",
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n",
			ShaderManager()->GetCurrentVertexShader(),
			m->m[0][0],	m->m[0][1],	m->m[0][2],	m->m[0][3],	
			m->m[1][0],	m->m[1][1],	m->m[1][2],	m->m[1][3],	
			m->m[2][0],	m->m[2][1],	m->m[2][2],	m->m[2][3],	
			m->m[3][0], m->m[3][1], m->m[3][2], m->m[3][3],
			m2->m[0][0], m2->m[0][1], m2->m[0][2], m2->m[0][3],	
			m2->m[1][0], m2->m[1][1], m2->m[1][2], m2->m[1][3],	
			m2->m[2][0], m2->m[2][1], m2->m[2][2], m2->m[2][3],	
			m2->m[3][0], m2->m[3][1], m2->m[3][2], m2->m[3][3]
			 );
#else
		Vector4D *pVec1 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_MODELVIEW];
		Vector4D *pVec2 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_MODELVIEWPROJ];
		Vector4D *pVec3 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_VIEWPROJ];
		Vector4D *pVec4 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_MODEL];

		sprintf(buf,"\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n"

			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n"

			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n"

			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
			"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n",

			pVec1[0][0], pVec1[0][1], pVec1[0][2], pVec1[0][3],	
			pVec1[1][0], pVec1[1][1], pVec1[1][2], pVec1[1][3],	
			pVec1[2][0], pVec1[2][1], pVec1[2][2], pVec1[2][3],	
			pVec1[3][0], pVec1[3][1], pVec1[3][2], pVec1[3][3],

			pVec2[0][0], pVec2[0][1], pVec2[0][2], pVec2[0][3],	
			pVec2[1][0], pVec2[1][1], pVec2[1][2], pVec2[1][3],	
			pVec2[2][0], pVec2[2][1], pVec2[2][2], pVec2[2][3],	
			pVec2[3][0], pVec2[3][1], pVec2[3][2], pVec2[3][3],

			pVec3[0][0], pVec3[0][1], pVec3[0][2], pVec3[0][3],	
			pVec3[1][0], pVec3[1][1], pVec3[1][2], pVec3[1][3],	
			pVec3[2][0], pVec3[2][1], pVec3[2][2], pVec3[2][3],	
			pVec3[3][0], pVec3[3][1], pVec3[3][2], pVec3[3][3],

			pVec4[0][0], pVec4[0][1], pVec4[0][2], pVec4[0][3],	
			pVec4[1][0], pVec4[1][1], pVec4[1][2], pVec4[1][3],	
			pVec4[2][0], pVec4[2][1], pVec4[2][2], pVec4[2][3],	
			0, 0, 0, 1
			);
#endif
		pFont->Begin();
		pFont->DrawText( buf, -1, &r, DT_LEFT | DT_TOP,
			D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
		pFont->End();

		return;
	}

#if 0
	Vector4D *pVec1 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_MODELVIEW];
	Vector4D *pVec2 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_MODELVIEWPROJ];
	Vector4D *pVec3 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_VIEWPROJ];
	Vector4D *pVec4 = &m_DynamicState.m_pVertexShaderConstant[VERTEX_SHADER_MODEL];

	static char buf2[1024];
	sprintf(buf2,"\n"
		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n"

		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n"

		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n\n"

		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n"
		"[%6.3f %6.3f %6.3f %6.3f]\n[%6.3f %6.3f %6.3f %6.3f]\n",

		pVec1[0][0], pVec1[0][1], pVec1[0][2], pVec1[0][3],	
		pVec1[1][0], pVec1[1][1], pVec1[1][2], pVec1[1][3],	
		pVec1[2][0], pVec1[2][1], pVec1[2][2], pVec1[2][3],	
		pVec1[3][0], pVec1[3][1], pVec1[3][2], pVec1[3][3],

		pVec2[0][0], pVec2[0][1], pVec2[0][2], pVec2[0][3],	
		pVec2[1][0], pVec2[1][1], pVec2[1][2], pVec2[1][3],	
		pVec2[2][0], pVec2[2][1], pVec2[2][2], pVec2[2][3],	
		pVec2[3][0], pVec2[3][1], pVec2[3][2], pVec2[3][3],

		pVec3[0][0], pVec3[0][1], pVec3[0][2], pVec3[0][3],	
		pVec3[1][0], pVec3[1][1], pVec3[1][2], pVec3[1][3],	
		pVec3[2][0], pVec3[2][1], pVec3[2][2], pVec3[2][3],	
		pVec3[3][0], pVec3[3][1], pVec3[3][2], pVec3[3][3],

		pVec4[0][0], pVec4[0][1], pVec4[0][2], pVec4[0][3],	
		pVec4[1][0], pVec4[1][1], pVec4[1][2], pVec4[1][3],	
		pVec4[2][0], pVec4[2][1], pVec4[2][2], pVec4[2][3],	
		0, 0, 0, 1.0f
		);
	OutputDebugString(buf2);
	return;
#endif

	char buf[256];
	sprintf(buf, "\nSnapshot id %d : \n", m_TransitionTable.CurrentSnapshot() );
	OutputDebugString(buf);

	ShadowState_t &boardState = m_TransitionTable.BoardState();

	sprintf(buf,"Depth States: ZFunc %d, ZWrite %d, ZEnable %d, ZBias %d\n",
		boardState.m_ZFunc, boardState.m_ZWriteEnable, 
		boardState.m_ZEnable, boardState.m_ZBias );
	OutputDebugString(buf);
	sprintf(buf,"Cull Enable %d Cull Mode %d Color Write %d Fill %d Const Color Mod %d\n",
		boardState.m_CullEnable, m_DynamicState.m_CullMode, boardState.m_ColorWriteEnable, 
		boardState.m_FillMode, boardState.m_ModulateConstantColor );
	OutputDebugString(buf);
	sprintf(buf,"Blend States: Blend Enable %d Test Enable %d Func %d SrcBlend %d (%s) DstBlend %d (%s)\n",
		boardState.m_AlphaBlendEnable, boardState.m_AlphaTestEnable, 
		boardState.m_AlphaFunc, boardState.m_SrcBlend, BlendModeToString( boardState.m_SrcBlend ),
		boardState.m_DestBlend, BlendModeToString( boardState.m_DestBlend ) );
	OutputDebugString(buf);
	int len = sprintf(buf,"Alpha Ref %d, Lighting: %d, Ambient Color %x, LightsEnabled ",
		boardState.m_AlphaRef, boardState.m_Lighting, m_DynamicState.m_Ambient);

	int i;
	for ( i = 0; i < m_Caps.m_MaxNumLights; ++i)
	{
		len += sprintf(buf+len,"%d ", m_DynamicState.m_LightEnable[i] );
	}
	sprintf(buf+len,"\n");
	OutputDebugString(buf);
	sprintf(buf,"Fixed Function: %d, VertexBlend %d\n",
		boardState.m_UsingFixedFunction, m_DynamicState.m_VertexBlend );
	OutputDebugString(buf);
	
	sprintf(buf,"Pass Vertex Usage: %x Pixel Shader %d Vertex Shader %x\n",
		boardState.m_VertexUsage, ShaderManager()->GetCurrentPixelShader(), 
		ShaderManager()->GetCurrentVertexShader() );
	OutputDebugString(buf);

	D3DXMATRIX* m = &GetTransform(MATERIAL_MODEL);
	sprintf(buf,"WorldMat [%4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f]\n",
		m->m[0][0],	m->m[0][1],	m->m[0][2],	m->m[0][3],	
		m->m[1][0],	m->m[1][1],	m->m[1][2],	m->m[1][3],	
		m->m[2][0],	m->m[2][1],	m->m[2][2],	m->m[2][3],	
		m->m[3][0], m->m[3][1], m->m[3][2], m->m[3][3] );
	OutputDebugString(buf);

	m = &GetTransform(MATERIAL_MODEL + 1);
	sprintf(buf,"WorldMat2 [%4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f]\n",
		m->m[0][0],	m->m[0][1],	m->m[0][2],	m->m[0][3],	
		m->m[1][0],	m->m[1][1],	m->m[1][2],	m->m[1][3],	
		m->m[2][0],	m->m[2][1],	m->m[2][2],	m->m[2][3],	
		m->m[3][0], m->m[3][1], m->m[3][2], m->m[3][3] );
	OutputDebugString(buf);

	m = &GetTransform(MATERIAL_VIEW);
	sprintf(buf,"ViewMat [%4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f]\n",
		m->m[0][0],	m->m[0][1],	m->m[0][2],	
		m->m[1][0],	m->m[1][1],	m->m[1][2],
		m->m[2][0],	m->m[2][1],	m->m[2][2],
		m->m[3][0], m->m[3][1], m->m[3][2] );
	OutputDebugString(buf);

	m = &GetTransform(MATERIAL_PROJECTION);
	sprintf(buf,"ProjMat [%4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f]\n",
		m->m[0][0],	m->m[0][1],	m->m[0][2],	
		m->m[1][0],	m->m[1][1],	m->m[1][2],
		m->m[2][0],	m->m[2][1],	m->m[2][2],
		m->m[3][0], m->m[3][1], m->m[3][2] );
	OutputDebugString(buf);

	for (i = 0; i < GetNumTextureUnits(); ++i)
	{
		m = &GetTransform(MATERIAL_TEXTURE0 + i);
		sprintf(buf,"TexMat%d [%4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f]\n",
			i, m->m[0][0],	m->m[0][1],	m->m[0][2],	
			m->m[1][0],	m->m[1][1],	m->m[1][2],
			m->m[2][0],	m->m[2][1],	m->m[2][2],
			m->m[3][0], m->m[3][1], m->m[3][2] );
		OutputDebugString(buf);
	}

	sprintf(buf,"Viewport (%d %d) [%d %d] %4.3f %4.3f\n",
		m_DynamicState.m_Viewport.X, m_DynamicState.m_Viewport.Y, 
		m_DynamicState.m_Viewport.Width, m_DynamicState.m_Viewport.Height,
		m_DynamicState.m_Viewport.MinZ, m_DynamicState.m_Viewport.MaxZ);
	OutputDebugString(buf);

	for (i = 0; i < NUM_TEXTURE_STAGES; ++i)
	{
		sprintf(buf,"Stage %d :\n", i);
		OutputDebugString(buf);
		sprintf(buf,"   Color Op: %d (%s) Color Arg1: %d (%s)",
			boardState.m_TextureStage[i].m_ColorOp, 
			TextureOpToString( boardState.m_TextureStage[i].m_ColorOp ),
			boardState.m_TextureStage[i].m_ColorArg1, 
			TextureArgToString( boardState.m_TextureStage[i].m_ColorArg1 ) );
		OutputDebugString(buf);
		sprintf( buf, " Color Arg2: %d (%s)\n",
			boardState.m_TextureStage[i].m_ColorArg2,
			TextureArgToString( boardState.m_TextureStage[i].m_ColorArg2 ) );
		OutputDebugString(buf);
		sprintf(buf,"   Alpha Op: %d (%s) Alpha Arg1: %d (%s)",
			boardState.m_TextureStage[i].m_AlphaOp, 
			TextureOpToString( boardState.m_TextureStage[i].m_AlphaOp ),
			boardState.m_TextureStage[i].m_AlphaArg1, 
			TextureArgToString( boardState.m_TextureStage[i].m_AlphaArg1 ) );
		OutputDebugString(buf);
		sprintf(buf," Alpha Arg2: %d (%s)\n",
			boardState.m_TextureStage[i].m_AlphaArg2,
			TextureArgToString( boardState.m_TextureStage[i].m_AlphaArg2 ) );
		OutputDebugString(buf);
		sprintf(buf,"   Spherical Coords: %d TexCoordIndex: %X\n",
			boardState.m_TextureStage[i].m_GenerateSphericalCoords,
			boardState.m_TextureStage[i].m_TexCoordIndex);
		OutputDebugString(buf);
		sprintf(buf,"	Texture Enabled: %d Bound Texture: %d UWrap: %d VWrap: %d\n",
			TextureStage(i).m_TextureEnable, GetBoundTextureBindId(i),
			TextureStage(i).m_UTexWrap, TextureStage(i).m_VTexWrap );
		OutputDebugString(buf);
		sprintf(buf,"	Mag Filter: %d Min Filter: %d Mip Filter: %d Transform: %d\n",
			TextureStage(i).m_MagFilter, TextureStage(i).m_MinFilter,
			TextureStage(i).m_MipFilter, TextureStage(i).m_TextureTransformFlags );
		OutputDebugString(buf);
	}
#endif
}


//-----------------------------------------------------------------------------
// Begin a render pass
//-----------------------------------------------------------------------------
void CShaderAPIDX8::BeginPass( StateSnapshot_t snapshot )
{
	// FIXME: This is only used for shaders that modify the vertex data
	// Do we need that feature?

	if (IsDeactivated())
		return;

	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_BEGIN_PASS );

	m_nCurrentSnapshot = snapshot;
	m_pRenderMesh->BeginPass( );
}


//-----------------------------------------------------------------------------
// Render da polygon!
//-----------------------------------------------------------------------------
void CShaderAPIDX8::RenderPass( )
{
	if (IsDeactivated())
		return;

	Assert( m_nCurrentSnapshot != -1 );

	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_RENDER_PASS_TIME);
  
	Assert (m_pRenderMesh);
	m_TransitionTable.UseSnapshot( m_nCurrentSnapshot );
	CommitPerPassStateChanges( m_nCurrentSnapshot );

	// Make sure that we bound a texture for every stage that is enabled
#ifdef _DEBUG
	// NOTE: not enabled yet. . see comment in CShaderAPIDX8::ApplyTextureEnable
	int stage;
	for( stage = 0; stage < GetNumTextureStages(); stage++ )
	{
		if( TextureStage(stage).m_TextureEnable )
		{
//			Assert( TextureStage(stage).m_BoundTexture != -1 );
		}
	}
#endif
	
#ifdef DEBUG_BOARD_STATE
	// Spew out render state...
	if (m_pMaterial->PerformDebugTrace())

	{
		SpewBoardState();
	}
#endif

	m_pRenderMesh->RenderPass();
	m_nCurrentSnapshot = -1;
}


//-----------------------------------------------------------------------------
// Matrix mode
//-----------------------------------------------------------------------------
void CShaderAPIDX8::MatrixMode( MaterialMatrixMode_t matrixMode )
{
	// NOTE!!!!!!
	// The only time that m_MatrixMode is used is for texture matrices.  Do not use
	// it for anything else unless you change this code!
	if( matrixMode >= MATERIAL_TEXTURE0 && matrixMode <= MATERIAL_TEXTURE7 )
	{
		m_MatrixMode = ( D3DTRANSFORMSTATETYPE )( matrixMode - MATERIAL_TEXTURE0 + D3DTS_TEXTURE0 );
	}
	else
	{
		m_MatrixMode = (D3DTRANSFORMSTATETYPE)-1;
	}

	m_CurrStack = GetMatrixStack( matrixMode );
}
				 	    
//-----------------------------------------------------------------------------
// the current camera position in world space.
//-----------------------------------------------------------------------------

void CShaderAPIDX8::GetWorldSpaceCameraPosition( float* pPos ) const
{
	memcpy( pPos, m_WorldSpaceCameraPositon.Base(), sizeof( float[3] ) );
}

void CShaderAPIDX8::CacheWorldSpaceCameraPosition()
{
	D3DXMATRIX& view = GetTransform(MATERIAL_VIEW);
	m_WorldSpaceCameraPositon[0] = 
		-( view( 3, 0 ) * view( 0, 0 ) + 
		   view( 3, 1 ) * view( 0, 1 ) + 
		   view( 3, 2 ) * view( 0, 2 ) );
	m_WorldSpaceCameraPositon[1] = 
		-( view( 3, 0 ) * view( 1, 0 ) + 
		   view( 3, 1 ) * view( 1, 1 ) + 
		   view( 3, 2 ) * view( 1, 2 ) );
	m_WorldSpaceCameraPositon[2] = 
		-( view( 3, 0 ) * view( 2, 0 ) + 
		   view( 3, 1 ) * view( 2, 1 ) + 
		   view( 3, 2 ) * view( 2, 2 ) );
	m_WorldSpaceCameraPositon[3] = 1.0f;
}

void CShaderAPIDX8::CachePolyOffsetProjectionMatrix()
{
	if( !( m_Caps.m_ZBiasSupported && m_Caps.m_SlopeScaledDepthBiasSupported ) )
	{
		float offsetVal = 
			-1.0f * (m_DynamicState.m_Viewport.MaxZ - m_DynamicState.m_Viewport.MinZ) /
			16384.0f;

		D3DXMATRIX offset;
		D3DXMatrixTranslation( &offset, 0.0f, 0.0f, offsetVal );
		if (!m_DynamicState.m_FastClipEnabled)
		{
			D3DXMatrixMultiply( &m_CachedPolyOffsetProjectionMatrix, &GetTransform(MATERIAL_PROJECTION), &offset );
		}
		else { // fast clip enabled, so concatenate both matrices (fast clip matrix already has original projection)
			D3DXMatrixMultiply( &m_CachedPolyOffsetProjectionMatrixWithClip, &m_CachedFastClipProjectionMatrix, &offset );
		}
	}
}




//-----------------------------------------------------------------------------
// Performs a flush on the matrix state	if necessary
//-----------------------------------------------------------------------------

bool CShaderAPIDX8::MatrixIsChanging( TransformType_t type )
{
	if( IsDeactivated() )	
	{
		return false;
	}

	// early out if the transform is already one of our standard types
	if ((type != TRANSFORM_IS_GENERAL) && (type == m_DynamicState.m_TransformType[m_CurrStack]))
		return false;

	// Only flush state if we're changing something other than a texture transform
	int textureMatrix = m_CurrStack - MATERIAL_TEXTURE0;
	if (( textureMatrix < 0 ) || (textureMatrix >= NUM_TEXTURE_TRANSFORMS))
		FlushBufferedPrimitives();

	return true;
}

void CShaderAPIDX8::SetTextureTransformDimension( int textureMatrix, int dimension, bool projected )
{
	D3DTEXTURETRANSFORMFLAGS textureTransformFlags = ( D3DTEXTURETRANSFORMFLAGS )dimension;
	if( projected )
	{
		Assert( sizeof( int ) == sizeof( D3DTEXTURETRANSFORMFLAGS ) );
		( *( int * )&textureTransformFlags ) |= D3DTTFF_PROJECTED;
	}

	if (TextureStage(textureMatrix).m_TextureTransformFlags != textureTransformFlags )
	{
		SetTextureStageState( textureMatrix, D3DTSS_TEXTURETRANSFORMFLAGS, textureTransformFlags );
		TextureStage(textureMatrix).m_TextureTransformFlags = textureTransformFlags;
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::DisableTextureTransform( int textureMatrix )
{
	if (TextureStage(textureMatrix).m_TextureTransformFlags != D3DTTFF_DISABLE )
	{
		SetTextureStageState( textureMatrix, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
		TextureStage(textureMatrix).m_TextureTransformFlags = D3DTTFF_DISABLE;
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}

void CShaderAPIDX8::SetBumpEnvMatrix( int textureStage, float m00, float m01, float m10, float m11 )
{	
	TextureStageState_t &textureStageState = TextureStage( textureStage );
	
	if( textureStageState.m_BumpEnvMat00 != m00 ||
		textureStageState.m_BumpEnvMat01 != m01 ||
		textureStageState.m_BumpEnvMat10 != m10 ||
		textureStageState.m_BumpEnvMat11 != m11 )
	{
		SetTextureStageState( textureStage, D3DTSS_BUMPENVMAT00, *( ( LPDWORD ) (&m00) ) );
		SetTextureStageState( textureStage, D3DTSS_BUMPENVMAT01, *( ( LPDWORD ) (&m01) ) );
		SetTextureStageState( textureStage, D3DTSS_BUMPENVMAT10, *( ( LPDWORD ) (&m10) ) );
		SetTextureStageState( textureStage, D3DTSS_BUMPENVMAT11, *( ( LPDWORD ) (&m11) ) );
		textureStageState.m_BumpEnvMat00 = m00;
		textureStageState.m_BumpEnvMat01 = m01;
		textureStageState.m_BumpEnvMat10 = m10;
		textureStageState.m_BumpEnvMat11 = m11;
	}
}

//-----------------------------------------------------------------------------
// Sets the actual matrix state
//-----------------------------------------------------------------------------
void CShaderAPIDX8::UpdateMatrixTransform( TransformType_t type )
{
	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_UPDATE_MATRIX_TRANSFORM );

	int textureMatrix = m_CurrStack - MATERIAL_TEXTURE0;
	if (( textureMatrix >= 0 ) && (textureMatrix < NUM_TEXTURE_TRANSFORMS))
	{
		// NOTE: Flush shouldn't happen here because we
		// expect that texture transforms will be set within the shader

		// FIXME: We only want to use D3DTTFF_COUNT3 for cubemaps
		// D3DTFF_COUNT2 is used for non-cubemaps. Of course, if there's
		// no performance penalty for COUNT3, we should just use that.
		D3DTEXTURETRANSFORMFLAGS transformFlags;
		transformFlags = (type == TRANSFORM_IS_IDENTITY) ? D3DTTFF_DISABLE : D3DTTFF_COUNT3;

		if (TextureStage(textureMatrix).m_TextureTransformFlags != transformFlags )
		{
			SetTextureStageState( textureMatrix, D3DTSS_TEXTURETRANSFORMFLAGS, transformFlags );
			TextureStage(textureMatrix).m_TextureTransformFlags = transformFlags;
			g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
		}
	}

	m_DynamicState.m_TransformType[m_CurrStack] = type;
	m_DynamicState.m_TransformChanged[m_CurrStack] = STATE_CHANGED;

#ifdef _DEBUG
	// Store off the board state
	D3DXMATRIX *pSrc = &GetTransform(m_CurrStack);
	D3DXMATRIX *pDst = &m_DynamicState.m_Transform[m_CurrStack];
//	Assert( *pSrc != *pDst );
	memcpy( pDst, pSrc, sizeof(D3DXMATRIX) );
#endif

	if ( m_CurrStack == MATERIAL_VIEW )
	{
		CacheWorldSpaceCameraPosition();
	}

	if( m_CurrStack == MATERIAL_PROJECTION )
	{
		CachePolyOffsetProjectionMatrix();
	}

	// Any time the view or projection matrix changes, the user clip planes need recomputing....
	if ( (m_CurrStack == MATERIAL_VIEW) || (m_CurrStack == MATERIAL_PROJECTION) )
	{
		m_DynamicState.m_UserClipPlaneChanged |= ( 1 << MAXUSERCLIPPLANES ) - 1;
	}

	// Set the state if it's a texture transform
	if ( (m_CurrStack >= MATERIAL_TEXTURE0) && (m_CurrStack <= MATERIAL_TEXTURE7) )
	{
		RECORD_COMMAND( DX8_SET_TRANSFORM, 2 );
		RECORD_INT( m_MatrixMode );
		RECORD_STRUCT( &GetTransform(m_CurrStack), sizeof(D3DXMATRIX) );

		m_pD3DDevice->SetTransform( m_MatrixMode, &GetTransform(m_CurrStack) );

		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Matrix stack operations
//-----------------------------------------------------------------------------

void CShaderAPIDX8::PushMatrix()
{
	// NOTE: No matrix transform update needed here.
	m_pMatrixStack[m_CurrStack]->Push();
}

void CShaderAPIDX8::PopMatrix()
{
	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->Pop();
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::LoadIdentity( )
{
	if (MatrixIsChanging(TRANSFORM_IS_IDENTITY))
	{
		m_pMatrixStack[m_CurrStack]->LoadIdentity( );
		UpdateMatrixTransform( TRANSFORM_IS_IDENTITY );
	}
}

void CShaderAPIDX8::LoadCameraToWorld( )
{
	if (MatrixIsChanging(TRANSFORM_IS_CAMERA_TO_WORLD))
	{
		// could just use the transpose instead, if we know there's no scale
		float det;
		D3DXMATRIX inv;
		D3DXMatrixInverse( &inv, &det, &GetTransform(MATERIAL_VIEW) );

		// Kill translation
		inv.m[3][0] = inv.m[3][1] = inv.m[3][2] = 0.0f;

		m_pMatrixStack[m_CurrStack]->LoadMatrix( &inv );
		UpdateMatrixTransform( TRANSFORM_IS_CAMERA_TO_WORLD );
	}
}

void CShaderAPIDX8::LoadMatrix( float *m )
{
	// Check for identity...
	if ( (fabs(m[0] - 1.0f) < 1e-3) && (fabs(m[5] - 1.0f) < 1e-3) && (fabs(m[10] - 1.0f) < 1e-3) && (fabs(m[15] - 1.0f) < 1e-3) &&
		 (fabs(m[1]) < 1e-3)  && (fabs(m[2]) < 1e-3)  && (fabs(m[3]) < 1e-3) &&
		 (fabs(m[4]) < 1e-3)  && (fabs(m[6]) < 1e-3)  && (fabs(m[7]) < 1e-3) &&
		 (fabs(m[8]) < 1e-3)  && (fabs(m[9]) < 1e-3)  && (fabs(m[11]) < 1e-3) &&
		 (fabs(m[12]) < 1e-3) && (fabs(m[13]) < 1e-3) && (fabs(m[14]) < 1e-3) )
	{
		LoadIdentity();
		return;
	}

	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->LoadMatrix( (D3DXMATRIX*)m );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::MultMatrix( float *m )
{
	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->MultMatrix( (D3DXMATRIX*)m );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::MultMatrixLocal( float *m )
{
	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->MultMatrixLocal( (D3DXMATRIX*)m );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::Rotate( float angle, float x, float y, float z )
{
	if (MatrixIsChanging())
	{
		D3DXVECTOR3 axis( x, y, z );
		m_pMatrixStack[m_CurrStack]->RotateAxisLocal( &axis, M_PI * angle / 180.0f );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::Translate( float x, float y, float z )
{
	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->TranslateLocal( x, y, z );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::Scale( float x, float y, float z )
{
	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->ScaleLocal( x, y, z );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::ScaleXY( float x, float y )
{
	if (MatrixIsChanging())
	{
		m_pMatrixStack[m_CurrStack]->ScaleLocal( x, y, 1.0f );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
	if (MatrixIsChanging())
	{
		D3DXMATRIX matrix;
		D3DXMatrixOrthoOffCenterRH( &matrix, left, right, top, bottom, zNear, zFar );
		m_pMatrixStack[m_CurrStack]->MultMatrixLocal(&matrix);
		Assert( m_CurrStack == MATERIAL_PROJECTION );
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
	if (MatrixIsChanging())
	{
		float width = 2 * zNear * tan( fovx * M_PI / 360.0 );
		float height = width / aspect;
		Assert( m_CurrStack == MATERIAL_PROJECTION );
		D3DXMATRIX rh;
		D3DXMatrixPerspectiveRH( &rh, width, height, zNear, zFar );
		m_pMatrixStack[m_CurrStack]->MultMatrixLocal(&rh);
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::PickMatrix( int x, int y, int width, int height )
{
	if (MatrixIsChanging())
	{
		Assert( m_CurrStack == MATERIAL_PROJECTION );

		// This is going to create a matrix to append to the standard projection.
		// Projection space goes from -1 to 1 in x and y. This matrix we append
		// will transform the pick region to -1 to 1 in projection space

		int vx, vy, vwidth, vheight;
		GetViewport( vx, vy, vwidth, vheight );
		
		// Compute the location of the pick region in projection space...
		float px = 2.0 * (float)(x - vx) / (float)vwidth - 1;
		float py = 2.0 * (float)(y - vy)/ (float)vheight - 1;
		float pw = 2.0 * (float)width / (float)vwidth;
		float ph = 2.0 * (float)height / (float)vheight;

		// we need to translate (px, py) to the origin
		// and scale so (pw,ph) -> (2, 2)
		D3DXMATRIX matrix;
		D3DXMatrixIdentity( &matrix );
		matrix.m[0][0] = 2.0 / pw;
		matrix.m[1][1] = 2.0 / ph;
		matrix.m[3][0] = -2.0 * px / pw;
		matrix.m[3][1] = -2.0 * py / ph;

		m_pMatrixStack[m_CurrStack]->MultMatrixLocal(&matrix);
		UpdateMatrixTransform();
	}
}

void CShaderAPIDX8::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
	memcpy( dst, GetTransform(matrixMode), sizeof(D3DXMATRIX) );
}


//-----------------------------------------------------------------------------
// Did a transform change?
//-----------------------------------------------------------------------------
inline bool CShaderAPIDX8::VertexShaderTransformChanged( int i )
{
	return (m_DynamicState.m_TransformChanged[i] & STATE_CHANGED_VERTEX_SHADER) != 0;
}

inline bool CShaderAPIDX8::FixedFunctionTransformChanged( int i )
{
	return (m_DynamicState.m_TransformChanged[i] & STATE_CHANGED_FIXED_FUNCTION) != 0;
}


const D3DXMATRIX &CShaderAPIDX8::GetProjectionMatrix( void )
{
	if ( m_DynamicState.m_FastClipEnabled)
	{
		if( ( m_TransitionTable.CurrentSnapshot() != -1 ) && 
			!( m_Caps.m_ZBiasSupported && m_Caps.m_SlopeScaledDepthBiasSupported ) &&
			m_TransitionTable.CurrentShadowState().m_ZBias )
			return m_CachedPolyOffsetProjectionMatrixWithClip;
		else
			return m_CachedFastClipProjectionMatrix;
	}
	else 
	if( ( m_TransitionTable.CurrentSnapshot() != -1 ) && 
		!( m_Caps.m_ZBiasSupported && m_Caps.m_SlopeScaledDepthBiasSupported ) &&
		m_TransitionTable.CurrentShadowState().m_ZBias )
	{
		return m_CachedPolyOffsetProjectionMatrix;
	}
	else
	{
		return GetTransform( MATERIAL_PROJECTION );
	}	
}

//-----------------------------------------------------------------------------
// Set view transforms
//-----------------------------------------------------------------------------
void CShaderAPIDX8::SetVertexShaderViewProj()
{
	if (m_Caps.m_SupportsPixelShaders)
	{
		D3DXMATRIX transpose = GetTransform(MATERIAL_VIEW) *
								GetProjectionMatrix();
		D3DXMatrixTranspose( &transpose, &transpose );
		SetVertexShaderConstant( VERTEX_SHADER_VIEWPROJ, transpose, 4 );
	}
}

void CShaderAPIDX8::SetVertexShaderModelViewProjAndModelView( void )
{
	if (m_Caps.m_SupportsPixelShaders)
	{
		D3DXMATRIX modelView, transpose;
		D3DXMatrixMultiply( &modelView, 
			&GetTransform(MATERIAL_MODEL), &GetTransform(MATERIAL_VIEW) );

		D3DXMatrixTranspose( &transpose, &modelView );
		SetVertexShaderConstant( VERTEX_SHADER_MODELVIEW, transpose, 4 );

		D3DXMatrixMultiply( &transpose, &modelView, &GetProjectionMatrix() );
		
		D3DXMatrixTranspose( &transpose, &transpose );
		SetVertexShaderConstant( VERTEX_SHADER_MODELVIEWPROJ, transpose, 4 );
	}
}

void CShaderAPIDX8::UpdateVertexShaderMatrix( int iMatrix )
{
	int matrix = MATERIAL_MODEL + iMatrix;
	if (VertexShaderTransformChanged(matrix))
	{
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_VERTEX_TRANSFORM, 3 );

		int vertexShaderConstant = VERTEX_SHADER_MODEL + iMatrix * 3;

		// Put the transform into the vertex shader constants...
		D3DXMATRIX transpose;
		D3DXMatrixTranspose( &transpose, &GetTransform(matrix) );
		SetVertexShaderConstant( vertexShaderConstant, transpose, 3 );

		// clear the change flag
		m_DynamicState.m_TransformChanged[matrix] &= ~STATE_CHANGED_VERTEX_SHADER;
	}
}

void CShaderAPIDX8::UpdateAllVertexShaderMatrices( void )
{
	// NUM_MODEL_TRANSFORMS 4x3 matrices with 4 floats of overflow since we cast to a 4x4 D3DXMATRIX
	float results[NUM_MODEL_TRANSFORMS*12+4];
	int numMats = MaxVertexShaderBlendMatrices();
	g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_VERTEX_TRANSFORM, numMats );
	for( int i = 0; i < numMats; ++i )
	{
		// Put the transform into the vertex shader constants...
		D3DXMatrixTranspose( ( D3DXMATRIX * )&results[i*12], &GetTransform( MATERIAL_MODEL + i ) );

		// clear the change flag  FIXME do we need to do this anymore?
		m_DynamicState.m_TransformChanged[MATERIAL_MODEL+i] &= ~STATE_CHANGED_VERTEX_SHADER;
	}
	SetVertexShaderConstant( VERTEX_SHADER_MODEL, results, numMats * 3, true );
}

void CShaderAPIDX8::SetVertexShaderStateSkinningMatrices()
{
#if 1
	UpdateAllVertexShaderMatrices();
#else
	for (int i = 1; i < MaxVertexShaderBlendMatrices(); ++i)
	{
		UpdateVertexShaderMatrix( i );
	}
#endif
}

//-----------------------------------------------------------------------------
// Commits vertex shader transforms that can change on a per pass basis
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitPerPassVertexShaderTransforms()
{
	Assert( m_Caps.m_SupportsPixelShaders );

	bool projChanged = VertexShaderTransformChanged(MATERIAL_PROJECTION);

	if( projChanged )
	{
		SetVertexShaderViewProj();
		SetVertexShaderModelViewProjAndModelView();
	}

	// Clear change flags
	m_DynamicState.m_TransformChanged[MATERIAL_PROJECTION] &= ~STATE_CHANGED_VERTEX_SHADER;
}

//-----------------------------------------------------------------------------
// Commits vertex shader transforms
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitVertexShaderTransforms()
{
	Assert( m_Caps.m_SupportsPixelShaders );

	bool viewChanged = VertexShaderTransformChanged(MATERIAL_VIEW);
	bool projChanged = VertexShaderTransformChanged(MATERIAL_PROJECTION);
	bool modelChanged = VertexShaderTransformChanged(MATERIAL_MODEL);

	if (viewChanged)
	{
		UpdateVertexShaderFogParams();
	}

	if( viewChanged || projChanged )
	{
		SetVertexShaderViewProj();
	}

	if( viewChanged || modelChanged || projChanged )
	{
		SetVertexShaderModelViewProjAndModelView();
	}

	if( modelChanged )
	{
		UpdateVertexShaderMatrix( 0 );
	}

	// Clear change flags
	m_DynamicState.m_TransformChanged[MATERIAL_MODEL] &= ~STATE_CHANGED_VERTEX_SHADER;
	m_DynamicState.m_TransformChanged[MATERIAL_VIEW] &= ~STATE_CHANGED_VERTEX_SHADER;
	m_DynamicState.m_TransformChanged[MATERIAL_PROJECTION] &= ~STATE_CHANGED_VERTEX_SHADER;
}


void CShaderAPIDX8::UpdateFixedFunctionMatrix( int iMatrix )
{
	int matrix = MATERIAL_MODEL + iMatrix;
	if (FixedFunctionTransformChanged( matrix ))
	{
		RECORD_COMMAND( DX8_SET_TRANSFORM, 2 );
		RECORD_INT( D3DTS_WORLDMATRIX(iMatrix) );
		RECORD_STRUCT( &GetTransform(matrix), sizeof(D3DXMATRIX) );

		m_pD3DDevice->SetTransform( D3DTS_WORLDMATRIX(iMatrix), &GetTransform(matrix) );

		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

		// clear the change flag
		m_DynamicState.m_TransformChanged[matrix] &= ~STATE_CHANGED_FIXED_FUNCTION;
	}
}


void CShaderAPIDX8::SetFixedFunctionStateSkinningMatrices()
{
	for( int i=1; i < MaxBlendMatrices(); i++ )
		UpdateFixedFunctionMatrix( i );
}

//-----------------------------------------------------------------------------
// Commits transforms for the fixed function pipeline that can happen on a per pass basis
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitPerPassFixedFunctionTransforms()
{
	// Update projection
	if (FixedFunctionTransformChanged( MATERIAL_PROJECTION ))
	{
		D3DTRANSFORMSTATETYPE matrix = D3DTS_PROJECTION;
		RECORD_COMMAND( DX8_SET_TRANSFORM, 2 );
		RECORD_INT( matrix );
		RECORD_STRUCT( &GetProjectionMatrix(), sizeof(D3DXMATRIX) );

		m_pD3DDevice->SetTransform( matrix, &GetProjectionMatrix() );
		g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

		// clear the change flag
		m_DynamicState.m_TransformChanged[MATERIAL_PROJECTION] &= ~STATE_CHANGED_FIXED_FUNCTION;
	}
}


//-----------------------------------------------------------------------------
// Commits transforms for the fixed function pipeline
//-----------------------------------------------------------------------------

void CShaderAPIDX8::CommitFixedFunctionTransforms()
{
	// Update view + projection
	int i;
	for ( i = MATERIAL_VIEW; i <= MATERIAL_PROJECTION; ++i)
	{
		if (FixedFunctionTransformChanged( i ))
		{
			D3DTRANSFORMSTATETYPE matrix = (i == MATERIAL_VIEW) ? D3DTS_VIEW : D3DTS_PROJECTION;
			RECORD_COMMAND( DX8_SET_TRANSFORM, 2 );
			RECORD_INT( matrix );
			RECORD_STRUCT( &GetTransform(i), sizeof(D3DXMATRIX) );

			if( i == MATERIAL_PROJECTION )
			{
				m_pD3DDevice->SetTransform( matrix, &GetProjectionMatrix() );
			}
			else
			{
				m_pD3DDevice->SetTransform( matrix, &GetTransform(i) );
			}

			g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

			// clear the change flag
			m_DynamicState.m_TransformChanged[i] &= ~STATE_CHANGED_FIXED_FUNCTION;
		}
	}

	UpdateFixedFunctionMatrix( 0 );
}


void CShaderAPIDX8::SetSkinningMatrices()
{
	Assert( m_pMaterial );

	if( m_DynamicState.m_NumBones == 0 )
	{
		return;
	}
	
	if( UsesVertexShader(m_pMaterial->GetVertexFormat()) )
	{
		SetVertexShaderStateSkinningMatrices();
	}
	else
	{
		SetFixedFunctionStateSkinningMatrices();
	}
}



//-----------------------------------------------------------------------------
// Commits vertex shader lighting
//-----------------------------------------------------------------------------

inline bool CShaderAPIDX8::VertexShaderLightingChanged( int i )
{
	return (m_DynamicState.m_LightChanged[i] & STATE_CHANGED_VERTEX_SHADER) != 0;
}

inline bool CShaderAPIDX8::VertexShaderLightingEnableChanged( int i )
{
	return (m_DynamicState.m_LightEnableChanged[i] & STATE_CHANGED_VERTEX_SHADER) != 0;
}

inline bool CShaderAPIDX8::FixedFunctionLightingChanged( int i )
{
	return (m_DynamicState.m_LightChanged[i] & STATE_CHANGED_FIXED_FUNCTION) != 0;
}

inline bool CShaderAPIDX8::FixedFunctionLightingEnableChanged( int i )
{
	return (m_DynamicState.m_LightEnableChanged[i] & STATE_CHANGED_FIXED_FUNCTION) != 0;
}



//-----------------------------------------------------------------------------
// Computes the light type
//-----------------------------------------------------------------------------

VertexShaderLightTypes_t CShaderAPIDX8::ComputeLightType( int i ) const
{
	if (!m_DynamicState.m_LightEnable[i])
		return LIGHT_NONE;

	switch( m_DynamicState.m_Lights[i].Type )
	{
	case D3DLIGHT_POINT:
		return LIGHT_POINT;

	case D3DLIGHT_DIRECTIONAL:
		return LIGHT_DIRECTIONAL;

	case D3DLIGHT_SPOT:
		return LIGHT_SPOT;
	}

	Assert(0);
	return LIGHT_NONE;
}


//-----------------------------------------------------------------------------
// Sort the lights by type
//-----------------------------------------------------------------------------

void CShaderAPIDX8::SortLights( int* index )
{
	m_DynamicState.m_NumLights = 0;

	for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		VertexShaderLightTypes_t type = ComputeLightType(i);
		int j = i;
		if (type != LIGHT_NONE)
		{
			while ( --j >= 0 )
			{
				if (m_DynamicState.m_LightType[j] <= type)
					break;

				// shift...
				m_DynamicState.m_LightType[j+1] = m_DynamicState.m_LightType[j];
				index[j+1] = index[j];
			}
			++j;

			m_DynamicState.m_LightType[j] = type;
			index[j] = i;

			++m_DynamicState.m_NumLights;
		}

		// Since we're iterating over all the lights anyways, let's reset
		// their change flags since we no longer need them
		m_DynamicState.m_LightEnableChanged[i] &= ~STATE_CHANGED_VERTEX_SHADER;
		m_DynamicState.m_LightChanged[i] &= ~STATE_CHANGED_VERTEX_SHADER;
	}
}


//-----------------------------------------------------------------------------
// Vertex Shader lighting
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitVertexShaderLighting()
{
	// If nothing changed, then don't bother. Otherwise, reload...
	int i;
	for ( i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		if (VertexShaderLightingChanged(i) || VertexShaderLightingEnableChanged(i))
			break;
	}

	// Yeah baby
	if (i == MAX_NUM_LIGHTS)
		return;

	// First, gotta sort the lights by their type
	int lightIndex[MAX_NUM_LIGHTS];
	SortLights( lightIndex );

	// Set the lighting state
	for ( i = 0; i < m_DynamicState.m_NumLights; ++i )
	{
		D3DLIGHT& light = m_DynamicState.m_Lights[lightIndex[i]];

		Vector4D lightState[5];

/*
		// Calculate the length of the light color vector.  We are going to 
		// normalize it in some shaders to be sent down to pixel shaders and
		// we need to know the scale value to do some math in the vertex shader.
		Vector tmpLightColor( light.Diffuse.r, light.Diffuse.g, light.Diffuse.b );
		float lightMag = VectorLength( tmpLightColor );
		
		lightState[0].Init( light.Diffuse.r, light.Diffuse.g, light.Diffuse.b,
			lightMag );
*/
		// The first one is the light color
		lightState[0].Init( light.Diffuse.r, light.Diffuse.g, light.Diffuse.b,
			light.Diffuse.a );

		// The next constant holds the light direction
		lightState[1].Init( light.Direction.x, light.Direction.y, light.Direction.z, 0.0f );

		// The next constant holds the light position
		lightState[2].Init( light.Position.x, light.Position.y, light.Position.z, 1.0f );

		// The next constant holds exponent, stopdot, stopdot2, 1 / (stopdot - stopdot2)
		if (light.Type == D3DLIGHT_SPOT)
		{
			float stopdot = cos( light.Theta * 0.5f );
			float stopdot2 = cos( light.Phi * 0.5f );
			float oodot = (stopdot > stopdot2) ? 1.0f / (stopdot - stopdot2) : 0.0f;
			lightState[3].Init( light.Falloff, stopdot, stopdot2, oodot );
		}
		else
		{
			lightState[3].Init( 0, 1, 1, 1 );
		}

		// The last constant holds attenuation0, attenuation1, attenuation2
		lightState[4].Init( light.Attenuation0, light.Attenuation1, light.Attenuation2, 0.0f );

		// Set the state
		SetVertexShaderConstant( VERTEX_SHADER_LIGHTS + i * 5, lightState[0].Base(), 5 );
	}
}


//-----------------------------------------------------------------------------
// Fixed function lighting
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitFixedFunctionLighting()
{	
	// Commit each light
	for (int i = 0; i < MaxNumLights(); ++i)
	{
		// Change light enable
		if (FixedFunctionLightingEnableChanged(i))
		{
			RECORD_COMMAND( DX8_LIGHT_ENABLE, 2 );
			RECORD_INT( i );
			RECORD_INT( m_DynamicState.m_LightEnable[i] );

			HRESULT hr = m_pD3DDevice->LightEnable( i, m_DynamicState.m_LightEnable[i] );
			Assert( !FAILED(hr) );
			g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

			// Clear change flag
			m_DynamicState.m_LightEnableChanged[i] &= ~STATE_CHANGED_FIXED_FUNCTION;
		}

		// Change lighting state...
		if (m_DynamicState.m_LightEnable[i])
		{
			if (FixedFunctionLightingChanged(i))
			{
				// Store off the "correct" falloff...
				D3DLIGHT& light = m_DynamicState.m_Lights[i];

				float falloff = light.Falloff;

				RECORD_COMMAND( DX8_SET_LIGHT, 2 );
				RECORD_INT( i );
				RECORD_STRUCT( &light, sizeof(light) );

				HRESULT hr = m_pD3DDevice->SetLight( i, &light );
				Assert( !FAILED(hr) );

				g_Stats.IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

				// Clear change flag
				m_DynamicState.m_LightChanged[i] &= ~STATE_CHANGED_FIXED_FUNCTION;

				// restore the correct falloff
				light.Falloff = falloff;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Commits user clip planes
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitUserClipPlanes()
{
	// We need to transform the clip planes, specified in world space,
	// to be in projection space.. To transform the plane, we must transform
	// the intercept and then transform the normal.
	for (int i = 0; i < MaxUserClipPlanes(); ++i)
	{
		// Don't bother with the plane if it's not enabled
		if ( (m_DynamicState.m_UserClipPlaneEnabled & (1 << i)) == 0 )
			continue;

		// Don't bother if it didn't change...
		if ( (m_DynamicState.m_UserClipPlaneChanged & (1 << i)) == 0 )
			continue;

		m_DynamicState.m_UserClipPlaneChanged &= ~(1 << i);

		D3DXMATRIX worldToProjectionInvTrans = GetTransform(MATERIAL_VIEW) * GetTransform(MATERIAL_PROJECTION);
		D3DXMatrixInverse(&worldToProjectionInvTrans, NULL, &worldToProjectionInvTrans);
		D3DXMatrixTranspose(&worldToProjectionInvTrans, &worldToProjectionInvTrans);

		D3DXPLANE clipPlaneProj;
		D3DXPlaneTransform( &clipPlaneProj, &m_DynamicState.m_UserClipPlaneWorld[i], &worldToProjectionInvTrans );

		if ( clipPlaneProj != m_DynamicState.m_UserClipPlaneProj[i] )
		{
			RECORD_COMMAND( DX8_SET_CLIP_PLANE, 5 );
			RECORD_INT( i );
			RECORD_FLOAT( clipPlaneProj.a );
			RECORD_FLOAT( clipPlaneProj.b );
			RECORD_FLOAT( clipPlaneProj.c );
			RECORD_FLOAT( clipPlaneProj.d );

			m_pD3DDevice->SetClipPlane( i, (float*)clipPlaneProj );
			m_DynamicState.m_UserClipPlaneProj[i] = clipPlaneProj;
		}
	}
}


void CShaderAPIDX8::CommitPerPassStateChanges( StateSnapshot_t id )
{
	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_COMMIT_STATE_TIME);

	if ( UsesVertexAndPixelShaders(id) )
	{
		CommitPerPassVertexShaderTransforms();
	}
	else
	{
		CommitPerPassFixedFunctionTransforms();
	}
}


//-----------------------------------------------------------------------------
// Commits transforms and lighting
//-----------------------------------------------------------------------------
void CShaderAPIDX8::CommitStateChanges()
{
	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_COMMIT_STATE_TIME);

	if ( UsesVertexShader(m_pMaterial->GetVertexFormat()) )
	{
		CommitVertexShaderTransforms();

		if (m_pMaterial->IsVertexLit())
		{
			CommitVertexShaderLighting();
		}
	}
	else
	{
		CommitFixedFunctionTransforms();

		if (m_pMaterial->IsVertexLit())
		{
			CommitFixedFunctionLighting();
		}
	}

	if ( m_DynamicState.m_UserClipPlaneEnabled )
	{
		CommitUserClipPlanes();
	}
}


//-----------------------------------------------------------------------------
// Viewport-related methods
//-----------------------------------------------------------------------------
void CShaderAPIDX8::Viewport( int x, int y, int width, int height )
{
	if (IsDeactivated())
		return;

	// Clamp the viewport to the current render target...
	if (!m_UsingTextureRenderTarget)
	{
		if (m_IsResizing)
		{
			RECT viewRect;
			GetClientRect( m_ViewHWnd, &viewRect );
			m_nWindowWidth = viewRect.right - viewRect.left;
			m_nWindowHeight = viewRect.bottom - viewRect.top;
		}

		if (width > m_nWindowWidth )
			width = m_nWindowWidth;
		if (height > m_nWindowHeight )
			height = m_nWindowHeight;
	}
	else
	{
		if (width > m_ViewportMaxWidth)
			width = m_ViewportMaxWidth;
		if (height > m_ViewportMaxHeight)
			height = m_ViewportMaxHeight;
	}
	// FIXME! : since SetRenderTarget messed up the viewport, don't check the cache value.
	/*
	if (((unsigned int)x != m_DynamicState.m_Viewport.X) || 
		((unsigned int)y != m_DynamicState.m_Viewport.Y) ||
		((unsigned int)width != m_DynamicState.m_Viewport.Width) ||
		((unsigned int)height != m_DynamicState.m_Viewport.Height))
*/
	{
		// State changed... need to flush the dynamic buffer
		FlushBufferedPrimitives();

		m_DynamicState.m_Viewport.X = x;
		m_DynamicState.m_Viewport.Y = y;
		m_DynamicState.m_Viewport.Width = width;
		m_DynamicState.m_Viewport.Height = height;

		// Don't actually set the state to 0 width; this can happen at init
		if ((m_DynamicState.m_Viewport.Width != 0) && 
			(m_DynamicState.m_Viewport.Height != 0))
		{
			RECORD_COMMAND( DX8_SET_VIEWPORT, 1 );
			RECORD_STRUCT( &m_DynamicState.m_Viewport, sizeof( m_DynamicState.m_Viewport ) );

			HRESULT hr = m_pD3DDevice->SetViewport( &m_DynamicState.m_Viewport );
			Assert( hr == D3D_OK );
		}
	}
}

void CShaderAPIDX8::DepthRange( float zNear, float zFar )
{
	if (IsDeactivated())
		return;

	if ((zNear != m_DynamicState.m_Viewport.MinZ) || 
		(zFar != m_DynamicState.m_Viewport.MaxZ))
	{
		// State changed... need to flush the dynamic buffer
		FlushBufferedPrimitives();

		m_DynamicState.m_Viewport.MinZ = zNear;
		m_DynamicState.m_Viewport.MaxZ = zFar;

		// Don't actually set the state to 0 width; this can happen at init
		if ((m_DynamicState.m_Viewport.Width != 0) && 
			(m_DynamicState.m_Viewport.Height != 0))
		{
			RECORD_COMMAND( DX8_SET_VIEWPORT, 1 );
			RECORD_STRUCT( &m_DynamicState.m_Viewport, sizeof( m_DynamicState.m_Viewport ) );

			HRESULT hr = m_pD3DDevice->SetViewport( &m_DynamicState.m_Viewport );
			Assert( hr == D3D_OK );
		}
	}
}

//-----------------------------------------------------------------------------
// Gets the current viewport size
//-----------------------------------------------------------------------------

void CShaderAPIDX8::GetViewport( int& x, int& y, int& width, int& height ) const
{
	x = m_DynamicState.m_Viewport.X;
	y = m_DynamicState.m_Viewport.Y;
	width = m_DynamicState.m_Viewport.Width;
	height = m_DynamicState.m_Viewport.Height;
}

//-----------------------------------------------------------------------------
// Flushes buffered primitives
//-----------------------------------------------------------------------------

void CShaderAPIDX8::FlushBufferedPrimitives( )
{
	// This shouldn't happen in the inner rendering loop!
	Assert( m_pRenderMesh == 0 );

	// NOTE: We've gotta store off the matrix mode because
	// it'll get reset by the default state application caused by the flush
	int tempStack = m_CurrStack;
	D3DTRANSFORMSTATETYPE tempMatrixMode = m_MatrixMode;

	MeshMgr()->Flush();

	m_CurrStack = tempStack;
	m_MatrixMode = tempMatrixMode;
}


//-----------------------------------------------------------------------------
// Flush the hardware
//-----------------------------------------------------------------------------

void CShaderAPIDX8::FlushHardware( )
{
	FlushBufferedPrimitives();

	RECORD_COMMAND( DX8_END_SCENE, 0 );
	HRESULT hr = m_pD3DDevice->EndScene();

	DiscardVertexBuffers();

	RECORD_COMMAND( DX8_BEGIN_SCENE, 0 );
	hr = m_pD3DDevice->BeginScene();
	
	ForceHardwareSync();
}


//-----------------------------------------------------------------------------
// Buffer clear	color
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	float a = (r * 0.30f + g * 0.59f + b * 0.11f) / MAX_HDR_OVERBRIGHT;

	// GR - need to force alpha to black for HDR
	m_DynamicState.m_ClearColor = D3DCOLOR_ARGB((unsigned char)a,r,g,b);
}

void CShaderAPIDX8::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_DynamicState.m_ClearColor = D3DCOLOR_ARGB(a,r,g,b);
}


//-----------------------------------------------------------------------------
// Buffer clear
//-----------------------------------------------------------------------------
void CShaderAPIDX8::ClearBuffers( bool bClearColor, bool bClearDepth, int renderTargetWidth, int renderTargetHeight )
{
	if ( ShaderUtil()->GetConfig().m_bSuppressRendering )
		return;

	if( IsDeactivated() )
	{
		return;
	}
	
	// State changed... need to flush the dynamic buffer
	FlushBufferedPrimitives();

	float depth = ShaderUtil()->GetConfig().bReverseDepth ? 0.0f : 1.0f;
	DWORD mask = 0;

	if( bClearColor )
		mask |= D3DCLEAR_TARGET;

	if( bClearDepth )
		mask |= D3DCLEAR_ZBUFFER;

	// Only clear the current view... right!??!
	D3DRECT clear;
	clear.x1 = m_DynamicState.m_Viewport.X;
	clear.y1 = m_DynamicState.m_Viewport.Y;
	clear.x2 = clear.x1 + m_DynamicState.m_Viewport.Width;
	clear.y2 = clear.y1 + m_DynamicState.m_Viewport.Height;

	RECORD_COMMAND( DX8_CLEAR, 6 );
	RECORD_INT( 1 );
	RECORD_STRUCT( &clear, sizeof(clear) );
	RECORD_INT( mask );
	RECORD_INT( m_DynamicState.m_ClearColor );
	RECORD_FLOAT( depth );
	RECORD_INT( 0 );

	HRESULT hr;
	if( ( renderTargetWidth == -1 && renderTargetHeight == -1 ) ||
		( renderTargetWidth == ( int )m_DynamicState.m_Viewport.Width &&
		  renderTargetHeight == ( int )m_DynamicState.m_Viewport.Height ) )
	{
		hr = m_pD3DDevice->Clear( 
			0, NULL, mask, m_DynamicState.m_ClearColor, depth, 0L );	
	}
	else
	{
		hr = m_pD3DDevice->Clear( 
			1, &clear, mask, m_DynamicState.m_ClearColor, depth, 0L );	
	}
	Assert( !FAILED(hr) );
}


//-----------------------------------------------------------------------------
// Returns a copy of the front buffer
//-----------------------------------------------------------------------------

IDirect3DSurface* CShaderAPIDX8::GetFrontBufferImage( ImageFormat& format )
{
	// need to flush the dynamic buffer	and make sure the entire image is there
	FlushBufferedPrimitives();

	HRESULT hr;
	IDirect3DSurface *pFullScreenSurfaceBits = 0;
	hr = m_pD3DDevice->CreateOffscreenPlainSurface( m_Mode.m_Width, m_Mode.m_Height, 
		D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &pFullScreenSurfaceBits, NULL );
	if (FAILED(hr))
		return 0;

	hr = m_pD3DDevice->GetFrontBufferData( 0, pFullScreenSurfaceBits );
	if (FAILED(hr))
		return 0;

	int windowWidth, windowHeight;
	GetWindowSize( windowWidth, windowHeight );
	
	IDirect3DSurface *pSurfaceBits = 0;
	hr = m_pD3DDevice->CreateOffscreenPlainSurface( windowWidth, windowHeight,
		D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &pSurfaceBits, NULL );
	Assert( hr == D3D_OK );
	
	POINT pnt;
	pnt.x = pnt.y = 0;
	BOOL result = ClientToScreen( m_HWnd, &pnt );
	Assert( result );

	RECT srcRect;
	srcRect.left = pnt.x;
	srcRect.top = pnt.y;
	srcRect.right = pnt.x + windowWidth;
	srcRect.bottom = pnt.y + windowHeight;

	POINT dstPnt;
	dstPnt.x = dstPnt.y = 0;
	
	D3DLOCKED_RECT lockedSrcRect;
	hr = pFullScreenSurfaceBits->LockRect( &lockedSrcRect, &srcRect, D3DLOCK_READONLY );
	Assert( hr == D3D_OK );
	
	D3DLOCKED_RECT lockedDstRect;
	hr = pSurfaceBits->LockRect( &lockedDstRect, NULL, 0 );
	Assert( hr == D3D_OK );

	int i;
	for( i = 0; i < windowHeight; i++ )
	{
		memcpy( ( unsigned char * )lockedDstRect.pBits + ( i * lockedDstRect.Pitch ), 
			    ( unsigned char * )lockedSrcRect.pBits + ( i * lockedSrcRect.Pitch ),
				windowWidth * 4 ); // hack . .  what if this is a different format?
	}
	hr = pSurfaceBits->UnlockRect();
	Assert( hr == D3D_OK );
	hr = pFullScreenSurfaceBits->UnlockRect();
	Assert( hr == D3D_OK );

	pFullScreenSurfaceBits->Release();

	format = D3DFormatToImageFormat( D3DFMT_A8R8G8B8 );
	return pSurfaceBits;
}


//-----------------------------------------------------------------------------
// Returns a copy of the back buffer
//-----------------------------------------------------------------------------
IDirect3DSurface* CShaderAPIDX8::GetBackBufferImage( ImageFormat& format )
{
/*
	// We'll go ahead and assert since we really shouldn't be using this with dx9
	// This won't work in dx9 if fsaa is enabled. . use GetFrontBufferImage instead!
	Assert( 0 );
	return NULL;
*/
	
	// need to flush the dynamic buffer	and make sure the entire image is there
	FlushBufferedPrimitives();

	HRESULT hr;
	IDirect3DSurface *pSurfaceBits = 0;
	IDirect3DSurface *pTmpSurface = NULL;

	// Get the back buffer
	IDirect3DSurface* pBackBuffer;
	hr = m_pD3DDevice->GetRenderTarget( 0, &pBackBuffer );
	if (FAILED(hr))
		return 0;

	// Find about its size and format
	D3DSURFACE_DESC desc;
	hr = pBackBuffer->GetDesc( &desc );
	if (FAILED(hr))
		goto CleanUp;

	// Can't use GetRenderTargetData directly when multisample is enabled.
	if ( m_PresentParameters.MultiSampleType != D3DMULTISAMPLE_NONE )
	{
		hr = m_pD3DDevice->CreateRenderTarget( desc.Width, desc.Height, desc.Format,
			D3DMULTISAMPLE_NONE, 0, TRUE, &pTmpSurface, NULL );
		if ( FAILED(hr) )
			goto CleanUp;

		hr = m_pD3DDevice->StretchRect( pBackBuffer, NULL, pTmpSurface, NULL, D3DTEXF_NONE );
		if ( FAILED(hr) )
			goto CleanUp;
	}

	// Create a buffer the same size and format
	hr = m_pD3DDevice->CreateOffscreenPlainSurface( desc.Width, desc.Height, 
		desc.Format, D3DPOOL_SYSTEMMEM, &pSurfaceBits, NULL );
	if (FAILED(hr))
		goto CleanUp;

	// Blit from the back buffer to our scratch buffer
	hr = m_pD3DDevice->GetRenderTargetData( pTmpSurface ? pTmpSurface : pBackBuffer, pSurfaceBits );
	if (FAILED(hr))
		goto CleanUp2;

	format = D3DFormatToImageFormat(desc.Format);
	if ( pTmpSurface )
	{
		pTmpSurface->Release();
	}
	pBackBuffer->Release();
	return pSurfaceBits;

CleanUp2:
	pSurfaceBits->Release();

CleanUp:
	if ( pTmpSurface )
	{
		pTmpSurface->Release();
	}

	pBackBuffer->Release();
	return 0;
}


//-----------------------------------------------------------------------------
// Flips the image vertically
//-----------------------------------------------------------------------------

void CShaderAPIDX8::FlipVertical( int width, int height, ImageFormat format, unsigned char *pData )
{
	int rowBytes = ShaderUtil()->GetMemRequired( width, height, format, false );
	rowBytes /= height;

	unsigned char* pTemp = (unsigned char*)_alloca( rowBytes );
	for (int i = height / 2; --i >= 0; )
	{
		int swapRow = height - i - 1;
		memcpy( pTemp, &pData[rowBytes*i], rowBytes );
		memcpy( &pData[rowBytes*i], &pData[rowBytes*swapRow], rowBytes );
		memcpy( &pData[rowBytes*swapRow], pTemp, rowBytes );
	}
}

//-----------------------------------------------------------------------------
// Reads from the current read buffer
//-----------------------------------------------------------------------------

void CShaderAPIDX8::ReadPixels( int x, int y, int width, int height, unsigned char *pData, ImageFormat dstFormat )
{
	IDirect3DSurface* pSurfaceBits = 0;
	ImageFormat format = IMAGE_FORMAT_RGB888;

//	pSurfaceBits = GetFrontBufferImage( format );
	pSurfaceBits = GetBackBufferImage( format );

	// Error? Exit.
	if (!pSurfaceBits)
		return;

	// Copy out the bits...
	D3DLOCKED_RECT lockedRect;
	RECT rect;
	rect.left = x; rect.right = x + width; 
	rect.top = y; rect.bottom = y + height;

	HRESULT hr;
	hr = pSurfaceBits->LockRect( &lockedRect, &rect, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK );
	if (!FAILED(hr))
	{
		unsigned char *pImage = (unsigned char *)lockedRect.pBits;
		ShaderUtil()->ConvertImageFormat( (unsigned char *)pImage, format,
							pData, dstFormat, width, height, lockedRect.Pitch, 0 );

		hr = pSurfaceBits->UnlockRect( );

		// Vertically flip the image, cause DX8 gives it to us upside down
//		FlipVertical( width, height, IMAGE_FORMAT_RGB888, pData );
	}

	// Release the temporary surface
	if (pSurfaceBits)
		pSurfaceBits->Release();
}


//-----------------------------------------------------------------------------
// Binds a particular material to render with
//-----------------------------------------------------------------------------

void CShaderAPIDX8::Bind( IMaterial* pMaterial )
{
	IMaterialInternal* pMatInt = static_cast<IMaterialInternal*>(pMaterial);
	if (m_pMaterial != pMatInt)
	{
		FlushBufferedPrimitives();
		RECORD_DEBUG_STRING( ( char * )pMaterial->GetName() );
		m_pMaterial = pMatInt;
	}
}

// Get the currently bound material
IMaterialInternal* CShaderAPIDX8::GetBoundMaterial()
{
	return m_pMaterial;
}


//-----------------------------------------------------------------------------
// lightmap stuff
//-----------------------------------------------------------------------------
void CShaderAPIDX8::BindBumpLightmap( TextureStage_t stage )
{
	ShaderUtil()->BindBumpLightmap( stage );
}

void CShaderAPIDX8::BindLightmap( TextureStage_t stage )
{
	ShaderUtil()->BindLightmap( stage );
}

void CShaderAPIDX8::BindLightmapAlpha( TextureStage_t stage )
{
	ShaderUtil()->BindLightmapAlpha( stage );
}

void CShaderAPIDX8::BindWhite( TextureStage_t stage )
{
	ShaderUtil()->BindWhite( stage );
}

void CShaderAPIDX8::BindBlack( TextureStage_t stage )
{
	ShaderUtil()->BindBlack( stage );
}

void CShaderAPIDX8::BindGrey( TextureStage_t stage )
{
	ShaderUtil()->BindGrey( stage );
}

void CShaderAPIDX8::BindFlatNormalMap( TextureStage_t stage )
{
	ShaderUtil()->BindFlatNormalMap( stage );
}

void CShaderAPIDX8::BindSyncTexture( TextureStage_t stage, int texture )
{
	ShaderUtil()->BindSyncTexture( stage, texture );
}

void CShaderAPIDX8::BindFBTexture( TextureStage_t stage )
{
	ShaderUtil()->BindFBTexture( stage );
}

void CShaderAPIDX8::BindNormalizationCubeMap( TextureStage_t stage )
{
	ShaderUtil()->BindNormalizationCubeMap( stage );
}


//-----------------------------------------------------------------------------
// Gets the lightmap dimensions
//-----------------------------------------------------------------------------
void CShaderAPIDX8::GetLightmapDimensions( int *w, int *h )
{
	ShaderUtil()->GetLightmapDimensions( w, h );
}



//-----------------------------------------------------------------------------
// Selection mode methods
//-----------------------------------------------------------------------------

int CShaderAPIDX8::SelectionMode( bool selectionMode )
{
	int numHits = m_NumHits;
	if (m_InSelectionMode)
		WriteHitRecord();
	m_InSelectionMode = selectionMode;
	m_pCurrSelectionRecord = m_pSelectionBuffer;
	m_NumHits = 0;
	return numHits;
}

bool CShaderAPIDX8::IsInSelectionMode() const
{
	return m_InSelectionMode;
}

void CShaderAPIDX8::SelectionBuffer( unsigned int* pBuffer, int size )
{
	Assert( !m_InSelectionMode );
	Assert( pBuffer && size );
	m_pSelectionBufferEnd = pBuffer + size;
	m_pSelectionBuffer = pBuffer;
	m_pCurrSelectionRecord = pBuffer;
}

void CShaderAPIDX8::ClearSelectionNames( )
{
	if (m_InSelectionMode)
		WriteHitRecord();
	m_SelectionNames.Clear();
}

void CShaderAPIDX8::LoadSelectionName( int name )
{
	if (m_InSelectionMode)
	{
		WriteHitRecord();
		Assert( m_SelectionNames.Count() > 0 );
		m_SelectionNames.Top() = name;
	}
}

void CShaderAPIDX8::PushSelectionName( int name )
{
	if (m_InSelectionMode)
	{
		WriteHitRecord();
		m_SelectionNames.Push(name);
	}
}

void CShaderAPIDX8::PopSelectionName()
{
	if (m_InSelectionMode)
	{
		WriteHitRecord();
		m_SelectionNames.Pop();
	}
}


void CShaderAPIDX8::WriteHitRecord( )
{
	FlushBufferedPrimitives();

	if (m_SelectionNames.Count() && (m_SelectionMinZ != FLT_MAX))
	{
		Assert( m_pCurrSelectionRecord + m_SelectionNames.Count() + 3 <	m_pSelectionBufferEnd );
		*m_pCurrSelectionRecord++ = m_SelectionNames.Count();
	    *m_pCurrSelectionRecord++ = (int)((double)m_SelectionMinZ * (double)0xFFFFFFFF);
	    *m_pCurrSelectionRecord++ = (int)((double)m_SelectionMaxZ * (double)0xFFFFFFFF);
		for (int i = 0; i < m_SelectionNames.Count(); ++i)
		{
			*m_pCurrSelectionRecord++ = m_SelectionNames[i];
		}

		++m_NumHits;
	}

	m_SelectionMinZ = FLT_MAX;
	m_SelectionMaxZ = FLT_MIN;
}

// We hit somefin in selection mode
void CShaderAPIDX8::RegisterSelectionHit( float minz, float maxz )
{
	if (minz < 0)
		minz = 0;
	if (maxz > 1)
		maxz = 1;
	if (m_SelectionMinZ > minz)
		m_SelectionMinZ = minz;
	if (m_SelectionMaxZ < maxz)
		m_SelectionMaxZ = maxz;
}

//
// Debugging text output
//
void CShaderAPIDX8::DrawDebugText( int desiredLeft, int desiredTop, 
								   MaterialRect_t *pActualRect,
								   const char *pText )
{
#if 0 // FIXME: This is all broken
/*
	OutputDebugString( pText );
	OutputDebugString( "\n" );
*/
	static ID3DXFont* pFont = 0;
    // Stateblocks for setting and restoring render states
    static DWORD   m_dwSavedStateBlock;
    static DWORD   m_dwDrawTextStateBlock;
	if (!pFont)
	{
		HFONT hFont = CreateFont( 12, 5, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_MODERN, 0 );
		Assert( hFont != 0 );

		HRESULT hr = D3DXCreateFont( m_pD3DDevice, hFont, &pFont );

		// Create the state blocks for rendering text
		for( UINT which=0; which<2; which++ )
		{
			m_pD3DDevice->BeginStateBlock();
			
			m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
			
			m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,   D3DBLEND_SRCALPHA );
			m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND,  D3DBLEND_INVSRCALPHA );
			m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE );
			m_pD3DDevice->SetRenderState( D3DRS_ALPHAREF,         0x08 );
			m_pD3DDevice->SetRenderState( D3DRS_ALPHAFUNC,  D3DCMP_GREATEREQUAL );
			m_pD3DDevice->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
			m_pD3DDevice->SetRenderState( D3DRS_CULLMODE,   D3DCULL_CCW );
			m_pD3DDevice->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
			m_pD3DDevice->SetRenderState( D3DRS_CLIPPING,         TRUE );
			m_pD3DDevice->SetRenderState( D3DRS_CLIPPLANEENABLE,  FALSE );
			m_pD3DDevice->SetRenderState( D3DRS_VERTEXBLEND,      FALSE );
			m_pD3DDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
			m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE,        FALSE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
			m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
			m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
			m_pD3DDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
			m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
			m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
			
			if( which==0 )
				m_pD3DDevice->EndStateBlock( &m_dwSavedStateBlock );
			else
				m_pD3DDevice->EndStateBlock( &m_dwDrawTextStateBlock );
		}
	}

    // Setup renderstate
    m_pD3DDevice->CaptureStateBlock( m_dwSavedStateBlock );
    m_pD3DDevice->ApplyStateBlock( m_dwDrawTextStateBlock );
	if( HardwareConfig()->SupportsVertexAndPixelShaders() )
	{
		SetVertexShader( NULL );
	    SetPixelShader( NULL );
	}

	static char buf[1024];
	RECT r = { desiredLeft, desiredTop, 0, 0 };

	pFont->Begin();
	pFont->DrawText( pText, -1, &r, DT_CALCRECT /*| DT_LEFT | DT_TOP */,
		D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
	if( pActualRect )
	{
		pActualRect->top = r.top;
		pActualRect->bottom = r.bottom;
		pActualRect->left = r.left;
		pActualRect->right = r.right;
	}
	pFont->DrawText( pText, -1, &r, 0 /*DT_LEFT | DT_TOP */,
		D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
	pFont->End();
    // Restore the modified renderstates
    m_pD3DDevice->ApplyStateBlock( m_dwSavedStateBlock );
#endif
}

int CShaderAPIDX8::GetCurrentNumBones( void ) const
{
	return m_DynamicState.m_NumBones;
}

int CShaderAPIDX8::GetCurrentLightCombo( void ) const
{
	// hack . . do this a cheaper way.
	bool bUseAmbientCube;
	if( m_DynamicState.m_AmbientLightCube[0][0] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[0][1] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[0][2] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[1][0] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[1][1] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[1][2] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[2][0] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[2][1] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[2][2] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[3][0] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[3][1] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[3][2] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[4][0] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[4][1] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[4][2] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[5][0] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[5][1] == 0.0f && 
		m_DynamicState.m_AmbientLightCube[5][2] == 0.0f )
	{
		bUseAmbientCube = false;
	}
	else
	{
		bUseAmbientCube = true;
	}

	Assert( m_pRenderMesh );
	return ComputeLightIndex( m_DynamicState.m_NumLights, m_DynamicState.m_LightType, 
							  bUseAmbientCube, m_pRenderMesh->HasColorMesh() );
}

int CShaderAPIDX8::GetCurrentFogType( void ) const
{
	return m_DynamicState.m_FogMode;
}

void CShaderAPIDX8::RecordString( const char *pStr )
{
	RECORD_STRING( pStr );
}

void CShaderAPIDX8::EvictManagedResources( void )
{
	m_pD3DDevice->EvictManagedResources();
}
