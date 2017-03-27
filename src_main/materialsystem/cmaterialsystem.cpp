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
// Implementation of the main material system interface
//=============================================================================
	   
#ifdef _WIN32
#include "windows.h"
#endif
#include "imaterialinternal.h"
#include "materialsystem/imaterialvar.h"
#include "crtmemdebug.h"
#include "colorspace.h"
#include "texturemanager.h"
#include "string.h"
#include "materialsystem_global.h"
#include "shaderapi.h"
#include "ishaderutil.h"
#include "shadersystem.h"
#include "imagepacker.h"
#include "imageloader.h"
#include "tgawriter.h"
#include "tgaloader.h"
#include "IHardwareConfigInternal.h"
#include "materialsystem/imesh.h"
#include "utlsymbol.h"
#include "utlrbtree.h"
#include <malloc.h>
#include "filesystem.h"
#include "pixelwriter.h"
#include "itextureinternal.h"
#include "tier0/fasttimer.h"
#include "vmatrix.h"
#include "vstdlib/strtools.h"
#include "tier0/vprof.h"
#include "vstdlib/strtools.h"
#include "icvar.h"
#include "stdshaders/common_hlsl_cpp_consts.h" // hack hack hack!
#include "vstdlib/ICommandLine.h"

// NOTE: This must be the last file included!!!
#include "tier0/memdbgon.h"


//#define WRITE_LIGHTMAP_TGA 1
//#define READ_LIGHTMAP_TGA 1
//#define PERF_TESTING 1

//-----------------------------------------------------------------------------
// Implementational structures
//-----------------------------------------------------------------------------

#define MATERIAL_MAX_TREE_DEPTH 256
#define FIRST_LIGHTMAP_TEXID 20000
// GR - offset for sprate lightmap alpha textures
#define FIRST_LIGHTMAP_ALPHA_TEXID ( FIRST_LIGHTMAP_TEXID + 10000 )
// NOTE:  These must match the values in StandardLightmap_t in IMaterialSystem.h
#define FULLBRIGHT_LIGHTMAP_TEXID			( FIRST_LIGHTMAP_TEXID - 1 )
#define BLACK_TEXID							( FIRST_LIGHTMAP_TEXID - 2 )
#define FLAT_NORMAL_TEXTURE					( FIRST_LIGHTMAP_TEXID - 3 )
#define FULLBRIGHT_BUMPED_LIGHTMAP_TEXID	( FIRST_LIGHTMAP_TEXID - 4 )
#define FRAMESYNC_TEXID1					( FIRST_LIGHTMAP_TEXID - 5 )
#define FRAMESYNC_TEXID2					( FIRST_LIGHTMAP_TEXID - 6 )
#define GREY_TEXID							( FIRST_LIGHTMAP_TEXID - 7 )

//-----------------------------------------------------------------------------
// The material system implementation
//-----------------------------------------------------------------------------

// fixme: clean this up
class CMaterialSystem : public IMaterialSystemInternal, public IShaderUtil
{
public:
	CMaterialSystem();
	virtual ~CMaterialSystem();

	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();
	virtual void Shutdown();
	virtual IMaterialSystemHardwareConfig *GetHardwareConfig( const char *pVersion, int *returnCode );

	CreateInterfaceFn Init( char const* pShaderDLL,
							IMaterialProxyFactory* pMaterialProxyFactory,
							CreateInterfaceFn fileSystemFactory,
							CreateInterfaceFn cvarFactory
							);
//	void Shutdown( );

	// Call this to set an explicit shader version to use 
	// Must be called before Init().
	virtual void SetShaderAPI( char const *pShaderAPIDLL );

	// Must be called before Init(), if you're going to call it at all...
	virtual void SetAdapter( int nAdapter, int nFlags );

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

	// Sets the mode...
	bool SetMode( void* hwnd, MaterialVideoMode_t const& mode, int flags, int nSuperSamples = 0 );

	// Creates/ destroys a child window
	bool AddView( void* hwnd );
	void RemoveView( void* hwnd );

	// Sets the view
	void SetView( void* hwnd );

	void Get3DDriverInfo( Material3DDriverInfo_t& info ) const;

	// Installs a function to be called when we need to release vertex buffers
	void AddReleaseFunc( MaterialBufferReleaseFunc_t func );
	void RemoveReleaseFunc( MaterialBufferReleaseFunc_t func );

	// Installs a function to be called when we need to restore vertex buffers
	void AddRestoreFunc( MaterialBufferRestoreFunc_t func );
	void RemoveRestoreFunc( MaterialBufferRestoreFunc_t func );

	// Called by the shader API when it's just about to lose video memory
	void ReleaseShaderObjects();
	void RestoreShaderObjects();

	// Release temporary HW memory...
	void ReleaseTempTextureMemory();

	// Creates a texture for use as a render target
	ITexture* CreateRenderTargetTexture( int w, int h, ImageFormat format, bool depth );

	// Set the current texture that is a copy of the framebuffer.
	void SetFrameBufferCopyTexture( ITexture *pTexture );
	ITexture *GetFrameBufferCopyTexture( void );

	// Creates a procedural texture
	ITexture *CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags );

	// Sets the default state in the currently selected shader
	void SetDefaultState( );
	void SetDefaultShadowState( );

	// Gets the maximum lightmap page size...
	int GetMaxLightmapPageWidth() const;
	int GetMaxLightmapPageHeight() const;

	IMesh*	CreateStaticMesh( MaterialVertexFormat_t vertexFormat, bool bSoftwareVertexShader );
	IMesh*  CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh );
	void	DestroyStaticMesh( IMesh* pMesh );
	IMesh*	GetDynamicMesh( bool buffered, IMesh* pVertexOverride = 0, IMesh* pIndexOverride = 0, IMaterial *pAutoBind = 0 );

	IMaterial* GetCurrentMaterial() { return m_pCurrentMaterial; }

	bool				UpdateConfig( MaterialSystem_Config_t *config, bool forceUpdate );
	MaterialHandle_t	FirstMaterial() const;
	MaterialHandle_t	NextMaterial( MaterialHandle_t h ) const;
	MaterialHandle_t	InvalidMaterial() const;
	IMaterial*			GetMaterial( MaterialHandle_t h ) const;
	IMaterialInternal*  GetMaterialInternal( MaterialHandle_t idx ) const;

	IMaterial*			FindMaterial( char const *materialName, bool *found, bool complain = true );
	ITexture *			FindTexture( char const* pTextureName, bool *pFound, bool complain = true );
	void				BindLocalCubemap( ITexture *pTexture );
	ITexture *			GetLocalCubemap( void );
	void				SetRenderTarget( ITexture *pTexture );
	ITexture *			GetRenderTarget( void );
	ImageFormat			GetBackBufferFormat() const;
	void				GetBackBufferDimensions( int &width, int &height ) const;
	void				GetRenderTargetDimensions( int &width, int &height) const;
	int					GetNumMaterials( void ) const;
	IMaterialInternal*	AddMaterial( char const* pName );
	void				AddMaterialToMaterialList( IMaterialInternal *pMaterial );
	void				RemoveMaterialFromMaterialList( IMaterialInternal *pMaterial );
	void				RemoveMaterial( IMaterialInternal* pMaterial );
	void				RemoveAllMaterials();

	// Allows us to override the depth buffer setting of a material
	virtual void	OverrideDepthEnable( bool bEnable, bool bDepthEnable );

	// FIXME: This is a hack required for NVidia, can they fix in drivers?
	virtual void	DrawScreenSpaceQuad( IMaterial* pMaterial );

	void		UncacheAllMaterials( void );
	void		UncacheUnusedMaterials( void );
	void		CacheUsedMaterials( void );
	void		ReloadTextures( void );
	void		ReloadMaterials( const char *pSubString = NULL );
	
	// Allocate lightmap textures in D3D
	void		AllocateLightmapTexture( int lightmap );

	void		BeginLightmapAllocation( void );
	int 		AllocateLightmap( int width, int height, 
		                                  int offsetIntoLightmapPage[2],
										  IMaterial *pMaterial );
	int			AllocateWhiteLightmap( IMaterial *pMaterial );
	void		GetLightmapPageSize( int lightmapPageID, int *width, int *height ) const;
	void		EndLightmapAllocation( void );
	void		UpdateLightmap( int lightmapPageID, int lightmapSize[2],
										int offsetIntoLightmapPage[2], 
										float *pFloatImage, float *pFloatImageBump1,
										float *pFloatImageBump2, float *pFloatImageBump3 );
	void		FlushLightmaps( void );
	void		CleanupLightmaps();

	int			GetNumSortIDs( void );
	void		GetSortInfo( MaterialSystem_SortInfo_t *sortInfoArray );

	void		BeginFrame( );
	void		EndFrame( );
	void		Bind( IMaterial *material, void *proxyData = NULL );
	void		BindLightmapPage( int lightmapPageID );
	void		SetForceBindWhiteLightmaps( bool bForce );

	void		SetLight( int lightNum, LightDesc_t& desc );
	void		SetAmbientLight( float r, float g, float b );
	void		SetAmbientLightCube( Vector4D cube[6] );

	void		CopyRenderTargetToTexture( ITexture *pTexture );

	void		DepthRange( float zNear, float zFar );
	void		ClearBuffers( bool bClearColor, bool bClearDepth );
	void		ClearColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void		ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	virtual void	SetInStubMode( bool bInStubMode );
	virtual bool	IsInStubMode();

	// read to a unsigned char rgb image.
	void		ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat );

	void		Flush( bool flushHardware = false );

	// matrix api
	virtual void	MatrixMode( MaterialMatrixMode_t matrixMode );
	virtual void	PushMatrix( void );
	virtual void	PopMatrix( void );
	virtual void	LoadMatrix( float * );
	virtual void	LoadMatrix( VMatrix const& matrix );
	virtual void	MultMatrix( float * );
	virtual void	MultMatrixLocal( float * );
	virtual void	GetMatrix( MaterialMatrixMode_t matrixMode, float *dst );
	virtual void	GetMatrix( MaterialMatrixMode_t matrixMode, VMatrix *pMatrix );
	virtual void	LoadIdentity( void );
	virtual void	Ortho( double left, double top, double right, double bottom, double zNear, double zFar );
	virtual void	PerspectiveX( double fovx, double aspect, double zNear, double zFar );
	virtual void	PickMatrix( int x, int y, int width, int height );
	virtual void	Rotate( float angle, float x, float y, float z );
	virtual void	Translate( float x, float y, float z );
	virtual void	Scale( float x, float y, float z );
	// end matrix api

	// Gets/sets viewport
	void Viewport( int x, int y, int width, int height );
	void GetViewport( int& x, int& y, int& width, int& height ) const;

	// Selection mode methods
	int  SelectionMode( bool selectionMode );
	void SelectionBuffer( unsigned int* pBuffer, int size );
	void ClearSelectionNames( );
	void LoadSelectionName( int name );
	void PushSelectionName( int name );
	void PopSelectionName();

	int					GetNumShaders( void ) const;
	const char *		GetShaderName( int shaderID ) const;
	int					GetNumShaderParams( int shaderID ) const;
	const char *		GetShaderParamName( int shaderID, int paramID ) const;
	const char *		GetShaderParamHelp( int shaderID, int paramID ) const;
	ShaderParamType_t	GetShaderParamType( int shaderID, int paramID ) const;
	const char *		GetShaderParamDefault( int shaderID, int paramID ) const;

	// Point size
	void PointSize( float size );

	// Sets the cull mode
	void CullMode( MaterialCullMode_t cullMode );

//	void RenderZOnlyWithHeightClip( bool bEnable );
//	bool RenderZOnlyWithHeightClipEnabled( void );
	void SetHeightClipMode( MaterialHeightClipMode_t mode );
	MaterialHeightClipMode_t GetHeightClipMode( void );
	void SetHeightClipZ( float z );

	// Sets the number of bones for skinning
	void SetNumBoneWeights( int numBones );

	// Fog-related methods
	void FogMode( MaterialFogMode_t fogMode );
	void FogStart( float fStart );
	void FogEnd( float fEnd );
	void SetFogZ( float fogZ );
	void FogColor3f( float r, float g, float b );
	void FogColor3fv( float const* rgb );
	void FogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void FogColor3ubv( unsigned char const* rgb );

	virtual MaterialFogMode_t GetFogMode( void );

	// Allows us to convert image formats
	bool ConvertImageFormat( unsigned char *src, enum ImageFormat srcImageFormat,
									 unsigned char *dst, enum ImageFormat dstImageFormat, 
									 int width, int height, int srcStride = 0, int dstStride = 0 );

	// Figures out the amount of memory needed by a bitmap
	int GetMemRequired( int width, int height, ImageFormat format, bool mipmap );

	IMaterialProxyFactory* GetMaterialProxyFactory() { return m_pMaterialProxyFactory; }

	void DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose );
	void DebugPrintUsedTextures( void );

	void ToggleSuppressMaterial( char const* pMaterialName );
	void ToggleDebugMaterial( char const* pMaterialName );

	void SwapBuffers( void );

	// Use this to spew information about the 3D layer 
	void SpewDriverInfo() const;

	// Inherited from IMaterialSystemInternal
	int GetLightmapPage( void );

	// Inherited from IShaderUtil
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

	// Create new materials	(currently only used by the editor!)
	IMaterial *CreateMaterial();

	// Method to allow clients access to the MaterialSystem_Config
	MaterialSystem_Config_t& GetConfig();

	// Gets image format info
	ImageFormatInfo_t const& ImageFormatInfo( ImageFormat fmt) const;
	
	//
	// Debugging text output
	//
	void DrawDebugText( int desiredLeft, int desiredTop, 
						MaterialRect_t *actualRect,	const char *fmt, ... );

private:
	// Stores a dictionary of materials, searched by name
	struct MaterialLookup_t
	{
		IMaterialInternal* m_pMaterial;
		CUtlSymbol m_Name;
	};

	// Stores a dictionary of missing materials to cut down on redundant warning messages
	// TODO:  1) Could add a counter
	//        2) Could dump to file/console at exit for exact list of missing materials
	struct MissingMaterial_t
	{
		CUtlSymbol			m_Name;
	};

	// Information about each lightmap
	enum
	{
		LIGHTMAP_PAGE_DIRTY = 0x1,
	};

	struct LightmapPageInfo_t
	{
		unsigned short m_Width;
		unsigned short m_Height;
		unsigned char *m_pImageData;
		// GR - for HDR we keep blend alpha separately
		unsigned char *m_pAlphaData;
		bool m_bSeparateAlpha;
		int	m_Flags;
	};

private:
	// Creates the debug materials
	void		CreateDebugMaterials();
	void		CleanUpDebugMaterials();
	void		DebugWriteLightmapImages( void );

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
	void ForceDepthFuncEquals( bool bEnable );

	static bool MaterialLessFunc( MaterialLookup_t const& src1, 
								  MaterialLookup_t const& src2 );

	static bool MissingMaterialLessFunc( MissingMaterial_t const& src1, 
								  MissingMaterial_t const& src2 );


	void		SetHardwareGamma( float gamma );
	void		SaveHardwareGammaRamp( void );
	void		RestoreHardwareGammaRamp( void );
	void		EnableLightmapFiltering( bool enabled );

	void		EnumerateMaterials( void );
	void		ResetMaterialLightmapPageInfo( void );

	// Two methods of building lightmap bits from floating point src data
	void UpdateBumpedLightmapBits( int lightmap, 
		float* pFloatImage,	float *pFloatImageBump1, float *pFloatImageBump2, 
		float *pFloatImageBump3, int pLightmapSize[2], int pOffsetIntoLightmapPage[2] );

	void UpdateLightmapBits( int lightmap, float* pFloatImage, 
		int pLightmapSize[2], int pOffsetIntoLightmapPage[2] );

	// Versions for the dynamic lightmaps (driver optimization)
	void UpdateBumpedLightmapBitsDynamic( int lightmap, 
		float* pFloatImage,	float *pFloatImageBump1, float *pFloatImageBump2,
		float *pFloatImageBump3, int pLightmapSize[2], int pOffsetIntoLightmapPage[2] );

	void UpdateLightmapBitsDynamic( int lightmap, float* pFloatImage,
							int pLightmapSize[2], int pOffsetIntoLightmapPage[2] );

	// Methods related to lightmaps
	int GetLightmapWidth( int lightmap ) const;
	int GetLightmapHeight( int lightmap ) const;

	// What are the lightmap dimensions?
	virtual void GetLightmapDimensions( int *w, int *h );

	// For computing sort info
	void ComputeSortInfo( MaterialSystem_SortInfo_t* pInfo, int& sortId, bool alpha );
	void ComputeWhiteLightmappedSortInfo( MaterialSystem_SortInfo_t* pInfo, int& sortId, bool alpha );

	// Initializes lightmap bits
	void InitLightmapBits( int lightmap );
	void InitLightmapBitsDynamic( int lightmap );

	// Updates the dirty lightmaps in memory
	void CommitDirtyLightmaps( );

	// Adds the lightmap page to the dirty list
	void AddLightmapToDirtyList( int lightmapPageID );

	// Allocates standard lightmaps
	void AllocateStandardTextures( );
	void ReleaseStandardTextures();

	// Releases/restores lightmap pages
	void ReleaseLightmapPages();
	void RestoreLightmapPages();

	// Recomputes a state snapshot
	void RecomputeAllStateSnapshots();

	// Used to dynamically load and unload the shader api
	CreateInterfaceFn	CreateShaderAPI( char const* pShaderDLL );
	void				DestroyShaderAPI();
	
	void UpdateHeightClipUserClipPlane( void );

private:
	CUtlRBTree< MaterialLookup_t, MaterialHandle_t > m_MaterialDict;
	CUtlRBTree< MissingMaterial_t, int > m_MissingList;

	// stuff that is exported to the launcher
	IMaterialInternal *					m_pCurrentMaterial;
	IMaterialInternal *					m_currentWhiteLightmapMaterial;

	LightmapPageInfo_t *				m_pLightmapPages;
	int									m_NumLightmapPages;
	CUtlVector<CImagePacker>			m_ImagePackers;
	int									m_numSortIDs;
	CUtlVector<int>						m_DirtyLightmaps;

	// Have we allocated the standard lightmaps?
	bool								m_StandardLightmapsAllocated;

	IMaterialProxyFactory*				m_pMaterialProxyFactory;
		
	// Used to dynamically load the shader DLL
	CSysModule *m_ShaderHInst;

	// The lightmap page
	int m_lightmapPageID;

	// Callback methods for releasing + restoring video memory
	CUtlVector< MaterialBufferReleaseFunc_t > m_ReleaseFunc;
	CUtlVector< MaterialBufferRestoreFunc_t > m_RestoreFunc;

	// Shared materials used for debugging....
	static IMaterialInternal* s_pDrawFlatMaterial;
	static IMaterialInternal* s_pErrorMaterial;
	
	ITexture *m_pLocalCubemapTexture;
	
	ITexture *m_pRenderTargetTexture;

	ITexture *m_pCurrentFrameBufferCopyTexture;

	bool m_bInStubMode;
	bool m_bForceBindWhiteLightmaps;
	MaterialHeightClipMode_t m_HeightClipMode;
	float m_HeightClipZ;
	char *m_pShaderDLL;

	int m_nAdapter;
	int m_nAdapterFlags;

	CreateInterfaceFn m_ShaderAPIFactory;

	// GR
	bool KeepLightmapAlphaSeparate();

public:
	// GR - named RT
	ITexture* CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat format, bool depth, bool bClampTexCoords, bool bAutoMipMap );

	virtual void	SyncToken( const char *pToken );
};


//-----------------------------------------------------------------------------
// Singleton instance exposed to the engine
//-----------------------------------------------------------------------------
CMaterialSystem g_MaterialSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CMaterialSystem, IMaterialSystem, 
						MATERIAL_SYSTEM_INTERFACE_VERSION, g_MaterialSystem );


//-----------------------------------------------------------------------------
// Material system config
//-----------------------------------------------------------------------------
MaterialSystem_Config_t g_config;

// Expose this to the external shader DLLs
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( MaterialSystem_Config_t, MaterialSystem_Config_t, MATERIALSYSTEM_CONFIG_VERSION, g_config );


//-----------------------------------------------------------------------------
// Necessary to allow the shader DLLs to get ahold of IMaterialSystemHardwareConfig
//-----------------------------------------------------------------------------
IHardwareConfigInternal* g_pHWConfig = 0;
static void *GetHardwareConfig()
{
	return (IMaterialSystemHardwareConfig*)g_pHWConfig;
}
EXPOSE_INTERFACE_FN( GetHardwareConfig, IMaterialSystemHardwareConfig, MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Necessary to allow the shader DLLs to get ahold of ICvar
//-----------------------------------------------------------------------------
static ICvar *s_pCVar;
static void *GetICVar()
{
	return s_pCVar;
}
EXPOSE_INTERFACE_FN( GetICVar, ICVar, VENGINE_CVAR_INTERFACE_VERSION );


static void FixSlashes( char *str )
{
	while( *str )
	{
		if( *str == '\\' )
		{
			*str = '/';
		}
		str++;
	}
}

//-----------------------------------------------------------------------------
// Accessor to get at the material system
//-----------------------------------------------------------------------------

IMaterialSystemInternal* MaterialSystem()
{
	return &g_MaterialSystem;
}

IShaderUtil* ShaderUtil()
{
	return &g_MaterialSystem;
}


//-----------------------------------------------------------------------------
// Shared material types used for debugging....
//-----------------------------------------------------------------------------
IMaterialInternal* CMaterialSystem::s_pDrawFlatMaterial = 0;
IMaterialInternal* CMaterialSystem::s_pErrorMaterial = 0;


//-----------------------------------------------------------------------------
// Creates the debugging materials
//-----------------------------------------------------------------------------
void CMaterialSystem::CreateDebugMaterials()
{
	if (!s_pDrawFlatMaterial)
	{
		s_pErrorMaterial = IMaterialInternal::Create( "___error.vmt", false );
		s_pErrorMaterial->SetShader( "UnlitGeneric" );
		s_pErrorMaterial->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );
		IMaterialVar *pVar = IMaterialVar::Create( s_pErrorMaterial, "$decalscale", 0.05f );
		s_pErrorMaterial->AddMaterialVar( pVar );
		s_pErrorMaterial->GetShaderParams()[BASETEXTURE]->SetTextureValue( TextureManager()->ErrorTexture() );
		s_pErrorMaterial->IncrementReferenceCount();
		AddMaterialToMaterialList( s_pErrorMaterial );

		s_pDrawFlatMaterial = IMaterialInternal::Create( "___flat.vmt", false );
		s_pDrawFlatMaterial->SetShader( "UnlitGeneric" );
		s_pDrawFlatMaterial->SetMaterialVarFlag( MATERIAL_VAR_VERTEXCOLOR, true );
		s_pDrawFlatMaterial->SetMaterialVarFlag( MATERIAL_VAR_FLAT, true );
		s_pDrawFlatMaterial->IncrementReferenceCount();
		AddMaterialToMaterialList( s_pDrawFlatMaterial );

		s_pErrorMaterial->Refresh();
		s_pDrawFlatMaterial->Refresh();

		ShaderSystem()->CreateDebugMaterials();
	}
}


//-----------------------------------------------------------------------------
// Creates the debugging materials
//-----------------------------------------------------------------------------
void CMaterialSystem::CleanUpDebugMaterials()
{
	if (s_pDrawFlatMaterial)
	{
		s_pErrorMaterial->DecrementReferenceCount();
		s_pDrawFlatMaterial->DecrementReferenceCount();

		RemoveMaterial( s_pErrorMaterial );
		RemoveMaterial( s_pDrawFlatMaterial );

		s_pErrorMaterial = NULL;
		s_pDrawFlatMaterial = NULL;

		ShaderSystem()->CleanUpDebugMaterials();
	}
}


//-----------------------------------------------------------------------------
// sort function
//-----------------------------------------------------------------------------
bool CMaterialSystem::MaterialLessFunc( MaterialLookup_t const& src1, 
							  MaterialLookup_t const& src2 )
{
	return src1.m_Name < src2.m_Name;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CMaterialSystem::CMaterialSystem() : 
    m_MaterialDict( 0, 256, MaterialLessFunc ),
	m_MissingList( 0, 32, MissingMaterialLessFunc )
{
	// set default values for members
	m_pRenderTargetTexture = NULL;
	m_pCurrentFrameBufferCopyTexture = NULL;
	m_pCurrentMaterial = NULL;
	m_currentWhiteLightmapMaterial = NULL;
	m_pLightmapPages = NULL;
	m_NumLightmapPages = 0;
	m_numSortIDs = 0;
	g_FrameNum = 0;
	m_ShaderHInst = 0;
	m_pMaterialProxyFactory = NULL;
	m_StandardLightmapsAllocated = false;
	m_bForceBindWhiteLightmaps = false;
	m_HeightClipMode = MATERIAL_HEIGHTCLIPMODE_DISABLE;
	m_HeightClipZ = 0.0f;
	m_bInStubMode = false;
	m_pShaderDLL = NULL;
	m_nAdapter = 0;
	m_nAdapterFlags = 0;
}

CMaterialSystem::~CMaterialSystem()
{
	if (m_pShaderDLL)
	{
		delete[] m_pShaderDLL;
	}
}


//-----------------------------------------------------------------------------
// Creates/destroys the shader implementation for the selected API
//-----------------------------------------------------------------------------
CreateInterfaceFn CMaterialSystem::CreateShaderAPI( char const* pShaderDLL )
{
	if ( !pShaderDLL )
		return 0;

	// Clean up the old shader
	DestroyShaderAPI();

	// Load the new shader
	m_ShaderHInst = g_pFileSystem->LoadModule(pShaderDLL);

	// Error loading the shader
	if (!m_ShaderHInst)
		return 0;

	// Get our class factory methods...
	return Sys_GetFactory( pShaderDLL );
}

void CMaterialSystem::DestroyShaderAPI()
{
	if (m_ShaderHInst)
	{
		// NOTE: By unloading the library, this will destroy m_pShaderAPI
		g_pFileSystem->UnloadModule( m_ShaderHInst );
		g_pShaderAPI = 0;
		g_pHWConfig = 0;
		g_pShaderShadow = 0;
		m_ShaderHInst = 0;
	}
}


//-----------------------------------------------------------------------------
// Connect/disconnect
//-----------------------------------------------------------------------------
bool CMaterialSystem::Connect( CreateInterfaceFn factory )
{
	if ( !factory )
		return false;

	g_pFileSystem = (IFileSystem *)factory( BASEFILESYSTEM_INTERFACE_VERSION, NULL );
	if( !g_pFileSystem )
	{
		Warning( "The material system requires the filesystem to run!\n" );
		return false;
	}

	TGALoader::SetFileSystem( factory );
	TGAWriter::SetFileSystem( factory );
	return true;	
}

void CMaterialSystem::Disconnect()
{
	g_pFileSystem = NULL;
}


//-----------------------------------------------------------------------------
// Method to get at different interfaces supported by the material system
//-----------------------------------------------------------------------------
void *CMaterialSystem::QueryInterface( const char *pInterfaceName )
{
	// Returns various interfaces supported by the shader API dll
	if (m_ShaderAPIFactory)
		return m_ShaderAPIFactory( pInterfaceName, NULL );
	return NULL;
}


//-----------------------------------------------------------------------------
// Sets which shader we should be using
//-----------------------------------------------------------------------------
void CMaterialSystem::SetShaderAPI( char const *pShaderAPIDLL )
{
	Assert( pShaderAPIDLL );
	int len = Q_strlen( pShaderAPIDLL ) + 1;
	m_pShaderDLL = new char[len];
	memcpy( m_pShaderDLL, pShaderAPIDLL, len );
}
	

//-----------------------------------------------------------------------------
// Must be called before Init(), if you're going to call it at all...
//-----------------------------------------------------------------------------
void CMaterialSystem::SetAdapter( int nAdapter, int nAdapterFlags )
{
	m_nAdapter = nAdapter;
	m_nAdapterFlags = nAdapterFlags;
}

	
//-----------------------------------------------------------------------------
// Initialization + shutdown of the material system
//-----------------------------------------------------------------------------
InitReturnVal_t CMaterialSystem::Init()
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2, true, false, false, true );	// NJS: Why no SSE?

	CreateInterfaceFn clientFactory = CreateShaderAPI( m_pShaderDLL ? m_pShaderDLL : "shaderapidx9" );
	if (!clientFactory)
	{
		DestroyShaderAPI();
		return INIT_FAILED;
	}

	m_ShaderAPIFactory = clientFactory;

	// Get at the interfaces exported by the shader DLL
	g_pShaderAPI = (IShaderAPI*)clientFactory( SHADERAPI_INTERFACE_VERSION, 0 );
	g_pHWConfig = (IHardwareConfigInternal*)clientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, 0 );
	g_pShaderShadow = (IShaderShadow*)clientFactory( SHADERSHADOW_INTERFACE_VERSION, 0 );
	if ((!g_pShaderAPI) || (!g_pHWConfig) || (!g_pShaderShadow))
	{
		DestroyShaderAPI();
		return INIT_FAILED;
	}

	// Hook it in...
	if (!g_pShaderAPI->Init( this, g_pFileSystem, m_nAdapter, m_nAdapterFlags ))
	{
		DestroyShaderAPI();
		return INIT_FAILED;
	}

	// For statistics...
	g_pShaderAPI->SetLightmapTextureIdRange( FIRST_LIGHTMAP_TEXID, FIRST_LIGHTMAP_TEXID + 255 );

	// Texture manager...
	TextureManager()->Init();

	// Shader system!
	ShaderSystem()->Init();

	// Create some lovely textures
	m_pLocalCubemapTexture = TextureManager()->ErrorTexture();

	// Set up debug materials...
	CreateDebugMaterials();

	// Set up a default material system config
	MaterialSystem_Config_t config;
	memset( &config, 0, sizeof(MaterialSystem_Config_t) );
	config.texGamma = 2.2f;
	config.screenGamma = 2.2f;
	config.overbright = 2.0f;
	config.bFilterLightmaps = true;
	config.bFilterTextures = true;
	config.bMipMapTextures = true;
	config.bBumpmap = true;
	config.bShowSpecular = true;
	config.bShowDiffuse = true;
	config.bCompressedTextures = true; // (UpdateConfig gives us the actual value for this).
	config.bBufferPrimitives = true;
	config.maxFrameLatency = 1;
	UpdateConfig( &config, false );

	return INIT_OK;
}


//-----------------------------------------------------------------------------
// Initializes and shuts down the shader API
//-----------------------------------------------------------------------------
CreateInterfaceFn CMaterialSystem::Init( char const* pShaderAPIDLL,
										IMaterialProxyFactory *pMaterialProxyFactory,
										CreateInterfaceFn fileSystemFactory,
										CreateInterfaceFn cvarFactory )
{
	// Hook cvars?
	if ( cvarFactory )
	{
		s_pCVar = (ICvar*)cvarFactory( VENGINE_CVAR_INTERFACE_VERSION, NULL );
	}
	
	if (!Connect(fileSystemFactory))
		return 0;

	if (pShaderAPIDLL)
		SetShaderAPI(pShaderAPIDLL);

	if (Init() != INIT_OK)
		return NULL;

	// save the proxy factory
	m_pMaterialProxyFactory = pMaterialProxyFactory;

	return m_ShaderAPIFactory;
}

void CMaterialSystem::Shutdown( )
{
	// Clean up the debug materials
	CleanUpDebugMaterials();

	// Clean up all materials..
	RemoveAllMaterials();

	// Clean up all lightmaps
	CleanupLightmaps();

	// Clean up standard textures
	ReleaseStandardTextures();

	// Shader system!
	ShaderSystem()->Shutdown();

	// Texture manager...
	TextureManager()->Shutdown();

	if (g_pShaderAPI)
	{
		g_pShaderAPI->Shutdown();

		// Unload the DLL
		DestroyShaderAPI();
	}

	// FIXME: Could dump list here...
	m_MissingList.RemoveAll();
}

IMaterialSystemHardwareConfig *CMaterialSystem::GetHardwareConfig( const char *pVersion, int *returnCode )
{
	return ( IMaterialSystemHardwareConfig * )m_ShaderAPIFactory( pVersion, returnCode );
}


//-----------------------------------------------------------------------------
// Methods related to mode setting...
//-----------------------------------------------------------------------------

// Gets the number of adapters...
int	 CMaterialSystem::GetDisplayAdapterCount() const
{
	return g_pShaderAPI->GetDisplayAdapterCount( );
}

// Returns info about each adapter
void CMaterialSystem::GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const
{
	g_pShaderAPI->GetDisplayAdapterInfo( adapter, info );
}

// Returns info about each adapter
void CMaterialSystem::Get3DDriverInfo( Material3DDriverInfo_t& info ) const
{
	g_pShaderAPI->Get3DDriverInfo( info );
}

// Returns the number of modes
int	 CMaterialSystem::GetModeCount( int adapter ) const
{
	return g_pShaderAPI->GetModeCount( adapter );
}

// Returns mode information..
void CMaterialSystem::GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const
{
	g_pShaderAPI->GetModeInfo( adapter, mode, info );
}

// Returns the mode info for the current display device
void CMaterialSystem::GetDisplayMode( MaterialVideoMode_t& info ) const
{
	g_pShaderAPI->GetDisplayMode( info );
}

// Sets the mode...
bool CMaterialSystem::SetMode( void* hwnd, MaterialVideoMode_t const& mode, int flags, int nSuperSamples )
{
	bool bPreviouslyUsingGraphics = g_pShaderAPI->IsUsingGraphics();
	bool bOk = g_pShaderAPI->SetMode( hwnd, mode, flags, nSuperSamples );
	if (!bOk)
		return false;

	// FIXME: There's gotta be a better way of doing this?
	// After the first mode set, make sure to download any textures created
	// before the first mode set. After the first mode set, all textures
	// will be reloaded via the reaquireresources call. Same goes for procedural materials
	if (!bPreviouslyUsingGraphics)
	{
		TextureManager()->ReloadTextures();

		// FIXME: This is a little tricky. We need to create the debug materials
		// *debug* set mode so tools will work. But because the device hasn't been
		// created at that point, it's selected the highest end shader no matter what.
		// Therefore, we need to re-create them here for the shader fallback to be
		// reselected based on the device info.
		CleanUpDebugMaterials();
		CreateDebugMaterials();
	}

	return true;
}

// Creates/ destroys a child window
bool CMaterialSystem::AddView( void* hwnd )
{
	return g_pShaderAPI->AddView( hwnd );
}

void CMaterialSystem::RemoveView( void* hwnd )
{
	g_pShaderAPI->RemoveView( hwnd );
}

// Activates a view
void CMaterialSystem::SetView( void* hwnd )
{
	g_pShaderAPI->SetView( hwnd );
}


//-----------------------------------------------------------------------------
// Installs a function to be called when we need to release vertex buffers
//-----------------------------------------------------------------------------

void CMaterialSystem::AddReleaseFunc( MaterialBufferReleaseFunc_t func )
{
	// Shouldn't have two copies in our list
	Assert( m_ReleaseFunc.Find( func ) == -1 );
	m_ReleaseFunc.AddToTail( func );
}

void CMaterialSystem::RemoveReleaseFunc( MaterialBufferReleaseFunc_t func )
{
	int i = m_ReleaseFunc.Find( func );
	if( i != -1 )
		m_ReleaseFunc.Remove(i);
}

//-----------------------------------------------------------------------------
// Installs a function to be called when we need to restore vertex buffers
//-----------------------------------------------------------------------------

void CMaterialSystem::AddRestoreFunc( MaterialBufferRestoreFunc_t func )
{
	// Shouldn't have two copies in our list
	Assert( m_RestoreFunc.Find( func ) == -1 );
	m_RestoreFunc.AddToTail( func );
}

void CMaterialSystem::RemoveRestoreFunc( MaterialBufferRestoreFunc_t func )
{
	int i = m_RestoreFunc.Find( func );
	Assert( i != -1 );
	m_RestoreFunc.Remove(i);
}

//-----------------------------------------------------------------------------
// Called by the shader API when it's just about to lose video memory
//-----------------------------------------------------------------------------

void CMaterialSystem::ReleaseShaderObjects()
{
	TextureManager()->ReleaseTextures();
	ReleaseStandardTextures();
	ReleaseLightmapPages();
	for (int i = 0; i < m_ReleaseFunc.Count(); ++i)
	{
		m_ReleaseFunc[i]();
	}
}

void CMaterialSystem::RestoreShaderObjects()
{
	TextureManager()->RestoreTextures();
	AllocateStandardTextures();
	RestoreLightmapPages();
	for (int i = 0; i < m_RestoreFunc.Count(); ++i)
	{
		m_RestoreFunc[i]();
	}
}


//-----------------------------------------------------------------------------
// Use this to create static vertex and index buffers 
//-----------------------------------------------------------------------------
IMesh* CMaterialSystem::CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh )
{
	Assert( pMaterial );
	return g_pShaderAPI->CreateStaticMesh( pMaterial, bForceTempMesh );
}

IMesh* CMaterialSystem::CreateStaticMesh( MaterialVertexFormat_t vertexFormat, bool bSoftwareVertexShader )
{
	return g_pShaderAPI->CreateStaticMesh( ShaderSystem()->GetVertexFormat(vertexFormat), bSoftwareVertexShader );
}

void CMaterialSystem::DestroyStaticMesh( IMesh* pMesh )
{
	g_pShaderAPI->DestroyStaticMesh( pMesh );
}

IMesh* CMaterialSystem::GetDynamicMesh( 
	bool buffered, 
	IMesh* pVertexOverride, 
	IMesh* pIndexOverride,
	IMaterial *pAutoBind )
{
	VPROF_ASSERT_ACCOUNTED( "CMaterialSystem::GetDynamicMesh" );
	if( pAutoBind )
		Bind( pAutoBind, NULL );

	return g_pShaderAPI->GetDynamicMesh( m_pCurrentMaterial, buffered,
		pVertexOverride, pIndexOverride);
}


//-----------------------------------------------------------------------------
// Use this to spew information about the 3D layer 
//-----------------------------------------------------------------------------
void CMaterialSystem::SpewDriverInfo() const
{
	Warning( "ShaderAPI: %s\n", m_pShaderDLL );
	g_pShaderAPI->SpewDriverInfo();
}


//-----------------------------------------------------------------------------
// Sets the default state in the currently selected shader
//-----------------------------------------------------------------------------
void CMaterialSystem::SetDefaultState( )
{
	int numTextureUnits = HardwareConfig()->GetNumTextureUnits();
	int i;
	for( i = 0; i < numTextureUnits; i++ )
	{
		g_pShaderAPI->DisableTextureTransform( i );
		g_pShaderAPI->MatrixMode( (MaterialMatrixMode_t)(MATERIAL_TEXTURE0 + i) );
		g_pShaderAPI->LoadIdentity( );
		g_pShaderAPI->EnableSRGBRead( ( TextureStage_t )i, false );
	}
	g_pShaderAPI->MatrixMode( MATERIAL_MODEL );

	g_pShaderAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	g_pShaderAPI->ShadeMode( SHADER_SMOOTH );
	g_pShaderAPI->PointSize( 1.0 );
	g_pShaderAPI->FogMode( MATERIAL_FOG_NONE );
	g_pShaderAPI->SetVertexShaderIndex( );
	g_pShaderAPI->SetPixelShaderIndex( );
}

void CMaterialSystem::SetDefaultShadowState( )
{
	int i;
	int numTextureUnits = HardwareConfig()->GetNumTextureUnits();
	g_pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );
	g_pShaderShadow->EnableDepthWrites( true );
	g_pShaderShadow->EnableDepthTest( true );
	g_pShaderShadow->EnableColorWrites( true );
	g_pShaderShadow->EnableAlphaWrites( false );
	g_pShaderShadow->EnableAlphaTest( false );
	g_pShaderShadow->EnableLighting( false );
	g_pShaderShadow->EnableAmbientLightCubeOnStage0( false );
	g_pShaderShadow->EnableConstantColor( false );
	g_pShaderShadow->EnableBlending( false );
	g_pShaderShadow->BlendFunc( SHADER_BLEND_ZERO, SHADER_BLEND_ZERO );
	// GR - separate alpha
	g_pShaderShadow->EnableBlendingSeparateAlpha( false );
	g_pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ZERO, SHADER_BLEND_ZERO );

	g_pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, 0.7f );
	g_pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT_AND_BACK, SHADER_POLYMODE_FILL );
	g_pShaderShadow->EnableCulling( true );
	g_pShaderShadow->EnablePolyOffset( false );
	g_pShaderShadow->EnableVertexDataPreprocess( false );
	g_pShaderShadow->EnableVertexBlend( false );
	g_pShaderShadow->EnableSpecular( false );
	g_pShaderShadow->EnableSRGBWrite( false );
	g_pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
	g_pShaderShadow->VertexShaderOverbrightValue( 1.0f );
	g_pShaderShadow->EnableCustomPixelPipe( false );
	g_pShaderShadow->CustomTextureStages( 0 );
	g_pShaderShadow->EnableAlphaPipe( false );
	g_pShaderShadow->EnableConstantAlpha( false );
	g_pShaderShadow->EnableVertexAlpha( false );
	g_pShaderShadow->SetVertexShader( NULL );
	g_pShaderShadow->SetPixelShader( NULL );
	for( i = 0; i < numTextureUnits; i++ )
	{
		g_pShaderShadow->EnableTexture( (TextureStage_t)i, false );
		g_pShaderShadow->EnableTexGen( (TextureStage_t)i, false );
		g_pShaderShadow->OverbrightValue( (TextureStage_t)i, 1.0f );
		g_pShaderShadow->EnableTextureAlpha( (TextureStage_t)i, false );
		g_pShaderShadow->CustomTextureOperation( (TextureStage_t)i, SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_DISABLE, SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );
		g_pShaderShadow->CustomTextureOperation( (TextureStage_t)i, SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_DISABLE, SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );
	}
}

//-----------------------------------------------------------------------------
// Color converting blitter
//-----------------------------------------------------------------------------

bool CMaterialSystem::ConvertImageFormat( unsigned char *src, enum ImageFormat srcImageFormat,
						unsigned char *dst, enum ImageFormat dstImageFormat, 
						int width, int height, int srcStride, int dstStride )
{
	return ImageLoader::ConvertImageFormat( src, srcImageFormat, 
		dst, dstImageFormat, width, height, srcStride, dstStride );
}

// Figures out the amount of memory needed by a bitmap
int CMaterialSystem::GetMemRequired( int width, int height, ImageFormat format, bool mipmap )
{
	return ImageLoader::GetMemRequired( width, height, format, mipmap );
}


//-----------------------------------------------------------------------------
// Method to allow clients access to the MaterialSystem_Config
//-----------------------------------------------------------------------------
MaterialSystem_Config_t& CMaterialSystem::GetConfig()
{
	return g_config;
}


//-----------------------------------------------------------------------------
// Gets image format info
//-----------------------------------------------------------------------------
ImageFormatInfo_t const& CMaterialSystem::ImageFormatInfo( ImageFormat fmt) const
{
	return ImageLoader::ImageFormatInfo(fmt);
}


//-----------------------------------------------------------------------------
// This is called when the config changes
//-----------------------------------------------------------------------------
bool CMaterialSystem::UpdateConfig( MaterialSystem_Config_t *config, bool forceUpdate )
{
	bool bRedownloadLightmaps = false;
	bool bRedownloadTextures = false;
    bool recomputeSnapshots = false;
	bool dxSupportLevelChanged = false;
	bool bReloadMaterials = false;
	bool bResetAnisotropy = false;

#if 0
	if( memcmp( &g_config, config, sizeof( g_config ) ) != 0 )
	{
		Warning( "MaterialSystem_Interface_t::UpdateConfig\n" );
		Warning( "screenGamma: %f\n", config->screenGamma );
		Warning( "texGamma: %f\n\n", config->texGamma );
		Warning( "overbright: %f\n", config->overbright );
		Warning( "bAllowCheats: %s\n", config->bAllowCheats ? "true" : "false" );
		Warning( "bLinearFrameBuffer: %s\n", config->bLinearFrameBuffer ? "true" : "false" );
		Warning( "polyOffset: %f\n", config->polyOffset );
		Warning( "skipMipLevels: %f\n", config->skipMipLevels );
		Warning( "lightScale: %f\n", config->lightScale );
		Warning( "bFilterLightmaps: %s\n", config->bFilterLightmaps ? "true" : "false" );
		Warning( "bBumpmap: %s\n", config->bBumpmap ? "true" : "false" );
		Warning( "bLightingOnly: %s\n", config->bLightingOnly ? "true" : "false" );
		Warning( "bCompressedTextures: %s\n", config->bCompressedTextures ? "true" : "false" );
	}
#endif
	if( g_pShaderAPI->IsUsingGraphics() )
	{
		// set the default state since we might be changing the number of
		// texture units, etc. (ie. we don't want to leave unit 2 in overbright mode
		// if it isn't going to be reset upon each SetDefaultState because there is
		// effectively only one texture unit.)
		SetDefaultState();
		
		if( !HardwareConfig()->SupportsVertexAndPixelShaders() )
		{
			config->m_bForceTrilinear = false;
		}
		
				
		// Don't use compressed textures for the moment if we don't support them
		if (HardwareConfig() && !HardwareConfig()->SupportsCompressedTextures())
		{
			config->bCompressedTextures = false;
		}
		if (forceUpdate)
		{
			EnableLightmapFiltering( config->bFilterLightmaps );
			recomputeSnapshots = true;
			bRedownloadLightmaps = true;
			bRedownloadTextures = true;
			bResetAnisotropy = true;
		}

		// toggle bump mapping
		if( config->bBumpmap != g_config.bBumpmap )
		{
			recomputeSnapshots = true;
			bReloadMaterials = true;
			bResetAnisotropy = true;
		}
		
		// toggle reverse depth
		if (config->bReverseDepth != g_config.bReverseDepth)
		{
			recomputeSnapshots = true;
			bResetAnisotropy = true;
		}

		// toggle no transparency
		if (config->bNoTransparency != g_config.bNoTransparency)
		{
			recomputeSnapshots = true;
			bResetAnisotropy = true;
		}

		// toggle dx emulation level
		if (config->dxSupportLevel != g_config.dxSupportLevel)
		{
			dxSupportLevelChanged = true;
			bResetAnisotropy = true;
		}

		// toggle lightmap filtering
		if( config->bFilterLightmaps != g_config.bFilterLightmaps )
		{
			EnableLightmapFiltering( config->bFilterLightmaps );
			recomputeSnapshots = true;
			bResetAnisotropy = true;
		}
		
		// toggle software lighting
		if ( config->bSoftwareLighting != g_config.bSoftwareLighting )
		{
			bReloadMaterials = true;
		}

		// toggle overbright. . causes lightmap redownload
		if( (HardwareConfig()->HasTextureEnvCombine() &&
			(config->overbright != g_config.overbright)) )
		{
			bRedownloadLightmaps = true;
			recomputeSnapshots = true;
			bResetAnisotropy = true;
		}
				
		// generic things that cause us to redownload lightmaps
		if( config->screenGamma != g_config.screenGamma ||
			config->texGamma != g_config.texGamma ||
			config->bAllowCheats != g_config.bAllowCheats ||
			config->bLinearFrameBuffer != g_config.bLinearFrameBuffer )
		{
			bRedownloadLightmaps = true;
		}

		// generic things that cause us to redownload textures
		if( config->screenGamma != g_config.screenGamma ||
			config->texGamma != g_config.texGamma ||
			config->bAllowCheats != g_config.bAllowCheats ||
			config->bLinearFrameBuffer != g_config.bLinearFrameBuffer ||
			config->skipMipLevels != g_config.skipMipLevels  ||
			config->bShowMipLevels != g_config.bShowMipLevels  ||
			((config->bCompressedTextures != g_config.bCompressedTextures) && HardwareConfig()->SupportsCompressedTextures())||
			config->bShowLowResImage != g_config.bShowLowResImage ||
			config->m_bForceBilinear != g_config.m_bForceBilinear ||
			config->m_bForceTrilinear != g_config.m_bForceTrilinear ||
			config->m_nForceAnisotropicLevel != g_config.m_nForceAnisotropicLevel 
			)
		{
			bRedownloadTextures = true;
			recomputeSnapshots = true;
			bResetAnisotropy = true;
		}		
	
		if ( config->m_nForceAnisotropicLevel != g_config.m_nForceAnisotropicLevel )
		{
			bResetAnisotropy = true;
		}

		g_config = *config;

		if (dxSupportLevelChanged)
		{
			// All snapshots have basically become invalid;
			g_pShaderAPI->ClearSnapshots();

			// Treat it as a task switch, and uncache all materials too
			ReloadMaterials();
		}

		if( bRedownloadTextures || bRedownloadLightmaps )
		{
			ColorSpace::SetGamma( g_config.screenGamma, g_config.texGamma, 
				g_config.overbright, g_config.bAllowCheats, g_config.bLinearFrameBuffer );
			if( config->bLinearFrameBuffer )
			{
				SetHardwareGamma( config->screenGamma );
			}
			else
			{
				SetHardwareGamma( 1.0f );
			}
		}

		if( bRedownloadTextures )
		{
			TextureManager()->ReloadTextures();
		}

		if ( bReloadMaterials )
		{
			ReloadMaterials();
		}

		// Recompute all state snapshots
		if (recomputeSnapshots && HardwareConfig()->SupportsStateSnapshotting())
		{
			RecomputeAllStateSnapshots();
		}

		if ( bResetAnisotropy )
		{
			g_pShaderAPI->SetAnisotropicLevel( g_config.m_nForceAnisotropicLevel );
		}
	}
	else
	{
		g_config = *config;
		ColorSpace::SetGamma( g_config.screenGamma, g_config.texGamma, 
			g_config.overbright, g_config.bAllowCheats, g_config.bLinearFrameBuffer );
	}

	return bRedownloadLightmaps;
}


//-----------------------------------------------------------------------------
// Adds/removes the material to the list of all materials
//-----------------------------------------------------------------------------
void CMaterialSystem::AddMaterialToMaterialList( IMaterialInternal *pMaterial )
{
	// FIXME: Is there a better way of handling procedurally-created materials?
	MaterialLookup_t lookup;
	lookup.m_pMaterial = pMaterial;
	lookup.m_Name = pMaterial->GetName();
	m_MaterialDict.Insert( lookup );
}

void CMaterialSystem::RemoveMaterialFromMaterialList( IMaterialInternal *pMaterial )
{
	// FIXME: Is there a better way of handling procedurally-created materials?
	MaterialLookup_t lookup;
	lookup.m_pMaterial = pMaterial;
	lookup.m_Name = pMaterial->GetName();
	m_MaterialDict.Remove( lookup );
}


//-----------------------------------------------------------------------------
// Adds, removes materials
//-----------------------------------------------------------------------------
IMaterialInternal* CMaterialSystem::AddMaterial( char const* pName )
{
	IMaterialInternal* pMaterial = IMaterialInternal::Create( pName );
	Assert( pMaterial );
	AddMaterialToMaterialList( pMaterial );
	return pMaterial;
}

void CMaterialSystem::RemoveMaterial( IMaterialInternal* pMaterial )
{
	RemoveMaterialFromMaterialList( pMaterial );
	IMaterialInternal::Destroy( pMaterial );
}

void CMaterialSystem::RemoveAllMaterials()
{
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		IMaterialInternal::Destroy( m_MaterialDict[i].m_pMaterial );
	}
	m_MaterialDict.RemoveAll();
}


//-----------------------------------------------------------------------------
// Creates a material instance
//-----------------------------------------------------------------------------

/*
IMaterial* CMaterialSystem::CreateRenderTargetMaterial( int w, int h )
{
	// Create a unique name for each material
	static int id = 0;
	char buf[128];
	sprintf( buf, "__RenderTarget%d", id );
	IMaterialInternal* pMaterial = AddMaterial( buf );
	++id;

	// create a base texture with the appropriate size and hook it in!
	// FIXME: Could try batching textures here and setting base texture offset

}
*/


ITexture* CMaterialSystem::CreateRenderTargetTexture( int w, int h, ImageFormat format, bool depth )
{
	ITextureInternal* pTex = TextureManager()->CreateRenderTargetTexture( w, h, format, depth );
	return pTex;
}

// GR - named RT
ITexture* CMaterialSystem::CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat format, bool depth, bool bClampTexCoords, bool bAutoMipMap )
{
	ITextureInternal* pTex = TextureManager()->CreateNamedRenderTargetTexture( pRTName, w, h, format, depth, bClampTexCoords, bAutoMipMap );
	return pTex;
}

void CMaterialSystem::SyncToken( const char *pToken )
{
	if ( g_pShaderAPI )
		g_pShaderAPI->SyncToken( pToken );
}

void CMaterialSystem::SetFrameBufferCopyTexture( ITexture *pTexture )
{
	if( m_pCurrentFrameBufferCopyTexture != pTexture )
	{
		g_pShaderAPI->FlushBufferedPrimitives();
	}
	// FIXME: Do I need to increment/decrement ref counts here, or assume that the app is going to do it?
	m_pCurrentFrameBufferCopyTexture = pTexture;
}

ITexture *CMaterialSystem::GetFrameBufferCopyTexture( void )
{
	return m_pCurrentFrameBufferCopyTexture;
}

//-----------------------------------------------------------------------------
// Creates a procedural texture
//-----------------------------------------------------------------------------
ITexture *CMaterialSystem::CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags )
{
	ITextureInternal* pTex = TextureManager()->CreateProceduralTexture( pTextureName, w, h, fmt, nFlags );
	return pTex;
}

	
//-----------------------------------------------------------------------------
// Material iteration methods
//-----------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::FirstMaterial() const
{
	return m_MaterialDict.FirstInorder();
}

MaterialHandle_t CMaterialSystem::NextMaterial( MaterialHandle_t h ) const
{
	return m_MaterialDict.NextInorder(h);
}

int CMaterialSystem::GetNumMaterials( )	const
{
	return m_MaterialDict.Count();
}

//-----------------------------------------------------------------------------
// Invalid index handle....
//-----------------------------------------------------------------------------

MaterialHandle_t CMaterialSystem::InvalidMaterial() const
{
	return m_MaterialDict.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Handle to material
//-----------------------------------------------------------------------------

IMaterial* CMaterialSystem::GetMaterial( MaterialHandle_t idx ) const
{
	return m_MaterialDict[idx].m_pMaterial;
}

IMaterialInternal* CMaterialSystem::GetMaterialInternal( MaterialHandle_t idx ) const
{
	return m_MaterialDict[idx].m_pMaterial;
}


//-----------------------------------------------------------------------------
// Create new materials	(currently only used by the editor!)
//-----------------------------------------------------------------------------
IMaterial *CMaterialSystem::CreateMaterial()
{
	// For not, just create a material with no default settings
	IMaterialInternal* pMaterial = IMaterialInternal::Create( "", false );
	pMaterial->SetShader("Wireframe");
	pMaterial->IncrementReferenceCount();
	return pMaterial;
}


//-----------------------------------------------------------------------------
// sort function for missing materials
//-----------------------------------------------------------------------------
bool CMaterialSystem::MissingMaterialLessFunc( MissingMaterial_t const& src1, 
		MissingMaterial_t const& src2 )
{
	return src1.m_Name < src2.m_Name;
}


//-----------------------------------------------------------------------------
// Search by name
//-----------------------------------------------------------------------------
IMaterial* CMaterialSystem::FindMaterial( char const *pMaterialName, bool *pFound, bool complain )
{
	// We need lower-case symbols for this to work
	size_t len = strlen(pMaterialName) + 1;
	char *pTemp = (char*)_alloca( len );
	strcpy( pTemp, pMaterialName );
	Q_strlower( pTemp );
	FixSlashes( pTemp );
	Assert( len == strlen( pTemp ) + 1 );

	MaterialLookup_t lookup;
	lookup.m_Name = pTemp;

	if( pFound )
	{
		*pFound = false;
	}
	
	MaterialHandle_t h = m_MaterialDict.Find( lookup );
	if( h == m_MaterialDict.InvalidIndex() )
	{
		// It hasn't been seen yet, so let's check to see if it's in the filesystem.
		int len = strlen( "materials/" ) + strlen( pTemp ) + strlen( ".vmt" ) + 1;
		char *vmtName;
		vmtName = ( char * )_alloca( len );
		strcpy( vmtName, "materials/" );
		strcat( vmtName, pTemp );
		if ( !Q_stristr( pMaterialName, ".vmt" ) )
		{
			strcat( vmtName, ".vmt" );
		}
		Q_strlower( vmtName );
		FixSlashes( vmtName );
		Assert( len >= (int)strlen( vmtName ) + 1 );
		
		if( g_pFileSystem->GetFileTime( vmtName ) )
		{
			if( pFound )
			{
				*pFound = true;
			}
#if 0
			Warning( "========\n" );
			for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
			{
				Warning( "material: \"%s\"\n", m_MaterialDict[i].m_pMaterial->GetName() );
			}
#endif
			char *matNameWithExtension;
			int len = strlen( pTemp ) + strlen( ".vmt" ) + 1;
			matNameWithExtension = ( char * )_alloca( len );
			strcpy( matNameWithExtension, pTemp );
			strcat( matNameWithExtension, ".vmt" );
			return AddMaterial( matNameWithExtension );
		}
		else
		{
			if( complain )
			{
				Assert( pTemp );
				// convert to lowercase
				char *name = (char*)_alloca( strlen(pTemp) + 1 );
				strcpy( name, pTemp );
				Q_strlower( name );

				MissingMaterial_t missing;
				missing.m_Name = name;
				if ( m_MissingList.Find( missing ) == m_MissingList.InvalidIndex() )
				{
					m_MissingList.Insert( missing );
					Warning( "material \"%s\" not found\n", name );
				}
			}

			return s_pErrorMaterial;
		}
	}
	else
	{
		if( pFound )
		{
			*pFound = true;
		}
	}
	return m_MaterialDict[h].m_pMaterial;
}

ITexture *CMaterialSystem::FindTexture( char const* pTextureName, bool *pFound, bool complain )
{
	ITextureInternal *pTexture = TextureManager()->FindOrLoadTexture( pTextureName, false );
	Assert( pTexture );
	if (pTexture->IsError())
	{
		if( complain )
		{
			Warning( "Couldn't find texture %s\n", pTextureName );
		}
		if( pFound )
		{
			*pFound = false;
		}
	}
	else
	{
		if( pFound )
		{
			*pFound = true;
		}
	}
	return pTexture;
}

void CMaterialSystem::BindLocalCubemap( ITexture *pTexture )
{
	if( pTexture )
	{
		if( m_pLocalCubemapTexture != pTexture )
		{
			g_pShaderAPI->FlushBufferedPrimitives();
		}
		m_pLocalCubemapTexture = pTexture;
	}
	else
	{
		if( m_pLocalCubemapTexture != TextureManager()->ErrorTexture() )
		{
			g_pShaderAPI->FlushBufferedPrimitives();
		}
		m_pLocalCubemapTexture = TextureManager()->ErrorTexture();
	}
}

ITexture *CMaterialSystem::GetLocalCubemap( void )
{
	return m_pLocalCubemapTexture;
}

void CMaterialSystem::SetRenderTarget( ITexture *pTexture )
{
	if( pTexture != m_pRenderTargetTexture )
	{
		// Make sure the texture is a render target...
		bool reset = true;
		ITextureInternal *pTexInt = static_cast<ITextureInternal*>(pTexture);
		if (pTexInt)
		{
			reset = !pTexInt->SetRenderTarget();
			if (reset)
				pTexture = NULL;
		}

		if (reset)
			g_pShaderAPI->SetRenderTarget();
		m_pRenderTargetTexture = pTexture;
	}
}

ITexture *CMaterialSystem::GetRenderTarget( void )
{
	return m_pRenderTargetTexture;
}

void CMaterialSystem::GetRenderTargetDimensions( int &width, int &height ) const
{
	if( m_pRenderTargetTexture )
	{
		width = m_pRenderTargetTexture->GetActualWidth();
		height = m_pRenderTargetTexture->GetActualHeight(); // GR - bug!!!
	}
	else
	{
		g_pShaderAPI->GetWindowSize( width, height );
	}
}

//-----------------------------------------------------------------------------
// Assign enumeration IDs to all materials
//-----------------------------------------------------------------------------
void CMaterialSystem::EnumerateMaterials( void )
{
	int id = 0;
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		GetMaterialInternal(i)->SetEnumerationID( id );
		++id;
	}
}


//-----------------------------------------------------------------------------
// Recomputes state snapshots for all materials
//-----------------------------------------------------------------------------
void CMaterialSystem::RecomputeAllStateSnapshots()
{
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		GetMaterialInternal(i)->RecomputeStateSnapshots();
	}
	g_pShaderAPI->ResetRenderState();
}


//-----------------------------------------------------------------------------
// Uncache all materials
//-----------------------------------------------------------------------------
void CMaterialSystem::UncacheAllMaterials( )
{
	Flush( true );

	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		Assert( GetMaterialInternal(i)->GetReferenceCount() >= 0 );
		GetMaterialInternal(i)->Uncache();
	}
	TextureManager()->RemoveUnusedTextures();
}


//-----------------------------------------------------------------------------
// Uncache unused materials
//-----------------------------------------------------------------------------
void CMaterialSystem::UncacheUnusedMaterials( )
{
	Flush( true );

	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		Assert( GetMaterialInternal(i)->GetReferenceCount() >= 0 );
		if( GetMaterialInternal(i)->GetReferenceCount() <= 0 )
		{
			GetMaterialInternal(i)->Uncache();
		}
	}
	TextureManager()->RemoveUnusedTextures();
}


//-----------------------------------------------------------------------------
// Release temporary HW memory...
//-----------------------------------------------------------------------------
void CMaterialSystem::ReleaseTempTextureMemory()
{
	g_pShaderAPI->DestroyVertexBuffers();
}


//-----------------------------------------------------------------------------
// Cache used materials
//-----------------------------------------------------------------------------
void CMaterialSystem::CacheUsedMaterials( )
{
	g_pShaderAPI->EvictManagedResources();
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		Assert( GetMaterialInternal(i)->GetReferenceCount() >= 0 );
		if( GetMaterialInternal(i)->GetReferenceCount() > 0 )
		{
			GetMaterialInternal(i)->Precache();
		}
	}
}

//-----------------------------------------------------------------------------
// Reloads textures + materials
//-----------------------------------------------------------------------------

void CMaterialSystem::ReloadTextures( void )
{
	TextureManager()->ReloadTextures();
}

void CMaterialSystem::ReloadMaterials( const char *pSubString )
{
	bool bVertexFormatChanged = false;
	if( pSubString == NULL )
	{
		bVertexFormatChanged = true;
		UncacheAllMaterials();
		CacheUsedMaterials();
	}
	else
	{
		for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
		{
			if( GetMaterialInternal(i)->GetReferenceCount() <= 0 )
			{
				continue;
			}
			VertexFormat_t oldVertexFormat = GetMaterialInternal(i)->GetVertexFormat();
			if( Q_stristr( GetMaterialInternal(i)->GetName(), pSubString ) )
			{
				GetMaterialInternal(i)->Uncache();
				GetMaterialInternal(i)->Precache();
				if( GetMaterialInternal(i)->GetVertexFormat() != oldVertexFormat )
				{
					bVertexFormatChanged = true;
				}
			}
		}
	}

	if( bVertexFormatChanged )
	{
		// Reloading materials could cause a vertex format change, so
		// we need to release and restore
		ReleaseShaderObjects();
		RestoreShaderObjects();
	}
}

//-----------------------------------------------------------------------------
// Gets the maximum lightmap page size...
//-----------------------------------------------------------------------------
int CMaterialSystem::GetMaxLightmapPageWidth() const
{
	// FIXME: It's unclear which we want here.
	// It doesn't drastically increase primitives per DrawIndexedPrimitive
	// call at the moment to increase it, so let's not for now.
	
	// If we're using dynamic textures though, we want bigger that's for sure.
	// The tradeoff here is how much memory we waste if we don't fill the lightmap

	// We need to go to 512x256 textures because that's the only way bumped
	// lighting on displacements can work given the 128x128 allowance..
	int nWidth = 512;
	if ( nWidth > HardwareConfig()->MaxTextureWidth() )
		nWidth = HardwareConfig()->MaxTextureWidth();

	return nWidth;
}

int CMaterialSystem::GetMaxLightmapPageHeight() const
{
	// FIXME: It's unclear which we want here.
	// It doesn't drastically increase primitives per DrawIndexedPrimitive
	// call at the moment to increase it, so let's not for now.

	// If we're using dynamic textures though, we want bigger that's for sure.
	// The tradeoff here is how much memory we waste if we don't fill the lightmap
	int nHeight = 256;
	if ( HardwareConfig()->PreferDynamicTextures() )
	{
		nHeight = 512;
	}

	if ( nHeight > HardwareConfig()->MaxTextureHeight() )
		nHeight = HardwareConfig()->MaxTextureHeight();

	return nHeight;
}


//-----------------------------------------------------------------------------
// Returns the lightmap page size
//-----------------------------------------------------------------------------
void CMaterialSystem::GetLightmapPageSize( int lightmapPageID, int *pWidth, int *pHeight ) const
{
	if ((lightmapPageID < 0) || (lightmapPageID >= m_NumLightmapPages ))
	{
		*pWidth = *pHeight = 1;
	}
	else
	{
		*pWidth = m_pLightmapPages[lightmapPageID].m_Width;
		*pHeight = m_pLightmapPages[lightmapPageID].m_Height;
	}
}

inline int CMaterialSystem::GetLightmapWidth( int lightmap ) const
{
	// Negative lightmap values have height and width 1
	Assert( lightmap < m_NumLightmapPages );
	return (lightmap >= 0) ? m_pLightmapPages[lightmap].m_Width : 1;
}

inline int CMaterialSystem::GetLightmapHeight( int lightmap ) const
{
	// Negative lightmap values have height and width 1
	Assert( lightmap < m_NumLightmapPages );
	return (lightmap >= 0) ? m_pLightmapPages[lightmap].m_Height : 1;
}


//-----------------------------------------------------------------------------
// What are the lightmap dimensions?
//-----------------------------------------------------------------------------
void CMaterialSystem::GetLightmapDimensions( int *w, int *h )
{
	*w = GetLightmapWidth( GetLightmapPage() );
	*h = GetLightmapHeight( GetLightmapPage() );
}


//-----------------------------------------------------------------------------
// Clean up lightmap pages.
//-----------------------------------------------------------------------------
void CMaterialSystem::CleanupLightmaps( )
{
	// delete old lightmap pages
	if( m_pLightmapPages )
	{
		int i;
		for( i = 0; i < m_NumLightmapPages; i++ )
		{
			g_pShaderAPI->DeleteTexture( FIRST_LIGHTMAP_TEXID + i );

			if( m_pLightmapPages[i].m_pImageData )
			{
				delete [] m_pLightmapPages[i].m_pImageData;
				m_pLightmapPages[i].m_pImageData = 0;
			}

			// GR - delete alpha data
			g_pShaderAPI->DeleteTexture( FIRST_LIGHTMAP_ALPHA_TEXID + i );

			if( m_pLightmapPages[i].m_pAlphaData )
			{
				delete [] m_pLightmapPages[i].m_pAlphaData;
				m_pLightmapPages[i].m_pAlphaData = 0;
			}
		}
		delete [] m_pLightmapPages;
		m_pLightmapPages = 0;
	}

	m_NumLightmapPages = 0;
}

//-----------------------------------------------------------------------------
// Resets the lightmap page info for each material
//-----------------------------------------------------------------------------
void CMaterialSystem::ResetMaterialLightmapPageInfo( void )
{
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		IMaterialInternal *pMaterial = GetMaterialInternal(i);
		pMaterial->SetMinLightmapPageID( 9999 );
		pMaterial->SetMaxLightmapPageID( -9999 );
		pMaterial->SetNeedsWhiteLightmap( false );
	}
}


//-----------------------------------------------------------------------------
// This is called before any lightmap allocations take place
//-----------------------------------------------------------------------------
void CMaterialSystem::BeginLightmapAllocation( )
{	
	// delete old lightmap pages
	CleanupLightmaps();

	m_ImagePackers.RemoveAll();
	int i = m_ImagePackers.AddToTail();
	m_ImagePackers[i].Reset( 0, GetMaxLightmapPageWidth(), GetMaxLightmapPageHeight() );

	m_pCurrentMaterial = 0;
	m_currentWhiteLightmapMaterial = 0;
	m_numSortIDs = 0;

	// need to set the min and max sorting id number for each material to 
	// a default value that basically means that it hasn't been used yet.
	ResetMaterialLightmapPageInfo();

	EnumerateMaterials();
}


//-----------------------------------------------------------------------------
// Allocates space in the lightmaps; must be called after BeginLightmapAllocation
//-----------------------------------------------------------------------------
int CMaterialSystem::AllocateLightmap( int width, int height, 
		                               int offsetIntoLightmapPage[2],
									   IMaterial *iMaterial )
{
	IMaterialInternal *pMaterial = static_cast<IMaterialInternal *>( iMaterial );
	if( !pMaterial )
	{
		Warning( "Programming error: CMaterialSystem::AllocateLightmap: NULL material\n" );
		return m_numSortIDs;
	}
	
	// material change
	int i;
	int nPackCount = m_ImagePackers.Count();
	if( m_pCurrentMaterial != pMaterial )
	{
		// If this happens, then we need to close out all image packers other than
		// the last one so as to produce as few sort IDs as possible
		for ( i = nPackCount - 1; --i >= 0; )
		{
			// NOTE: We *must* use the order preserving one here so the remaining one
			// is the last lightmap
			m_ImagePackers.Remove( i );
		}

		// If it's not the first material, increment the sort id
		if (m_pCurrentMaterial)
		{
			m_ImagePackers[0].IncrementSortId( );
			++m_numSortIDs;
		}

		m_pCurrentMaterial = pMaterial;

		// This assertion guarantees we don't see the same material twice in this loop.
		Assert( pMaterial->GetMinLightmapPageID( ) > pMaterial->GetMaxLightmapPageID() );

		// NOTE: We may not use this lightmap page, but we might
		// we won't know for sure until the next material is passed in.
		// So, for now, we're going to forcibly add the current lightmap
		// page to this material so the sort IDs work out correctly.
		m_pCurrentMaterial->SetMinLightmapPageID( m_NumLightmapPages );
		m_pCurrentMaterial->SetMaxLightmapPageID( m_NumLightmapPages );
	}

	// Try to add it to any of the current images...
	bool bAdded = false;
	for ( i = 0; i < nPackCount; ++i )
	{
		bAdded = m_ImagePackers[i].AddBlock( width, height, &offsetIntoLightmapPage[0],
									 &offsetIntoLightmapPage[1] );
		if (bAdded)
			break;
	}

	if( !bAdded )
	{
		++m_numSortIDs;
		i = m_ImagePackers.AddToTail();
		m_ImagePackers[i].Reset( m_numSortIDs, GetMaxLightmapPageWidth(), GetMaxLightmapPageHeight() );
		++m_NumLightmapPages;
		if( !m_ImagePackers[i].AddBlock( width, height, &offsetIntoLightmapPage[0],
			&offsetIntoLightmapPage[1] ) )
		{
			Error( "MaterialSystem_Interface_t::AllocateLightmap: lightmap (%dx%d) too big to fit in page (%dx%d)\n", 
				width, height, GetMaxLightmapPageWidth(), GetMaxLightmapPageHeight() );
		}

		// Add this lightmap to the material...
		m_pCurrentMaterial->SetMaxLightmapPageID( m_NumLightmapPages );
	}

	return m_ImagePackers[i].GetSortId();
}



void CMaterialSystem::EndLightmapAllocation( )
{
	// count the last page that we were on.if it wasn't 
	// and count the last sortID that we were on
	m_NumLightmapPages++; 
	m_numSortIDs++;

	// Compute the dimensions of the last lightmap 
	int lastLightmapPageWidth, lastLightmapPageHeight;
	int nLastIdx = m_ImagePackers.Count();
	m_ImagePackers[nLastIdx - 1].GetMinimumDimensions( &lastLightmapPageWidth, &lastLightmapPageHeight );

	m_pLightmapPages = new LightmapPageInfo_t[m_NumLightmapPages];
	Assert( m_pLightmapPages );

	bool bUseDynamicTextures = HardwareConfig()->PreferDynamicTextures();
	bool bAllocateLightmapBits = bUseDynamicTextures;

#ifdef WRITE_LIGHTMAP_TGA
	bAllocateLightmapBits = true;
#endif

	int i;

	// GR - figure out if alpha has to be separate
	bool bKeepLightmapAlphaSeparate = KeepLightmapAlphaSeparate();

	// GR - build a list of lightmaps that need alpha for blending (based on material requirements)
	bool *lightmapAlphaFlag = NULL;
	lightmapAlphaFlag = new bool[m_NumLightmapPages];

	for( i = 0; i < m_NumLightmapPages; i++ )
		lightmapAlphaFlag[i] = false;

	if( bKeepLightmapAlphaSeparate )
	{
		for (MaterialHandle_t im = FirstMaterial(); im != InvalidMaterial(); im = NextMaterial(im) )
		{
			IMaterialInternal* mat = GetMaterialInternal(im);
			if( ( mat->GetReferenceCount() > 0 ) && mat->NeedsLightmapBlendAlpha() )
			{
				for( int j = mat->GetMinLightmapPageID(); j <= mat->GetMaxLightmapPageID(); j++ )
					lightmapAlphaFlag[j] = true;
			}
		}
	}

	for( i = 0; i < m_NumLightmapPages; i++ )
	{
		bool bSeparateAlpha = bKeepLightmapAlphaSeparate && lightmapAlphaFlag[i];

		// Compute lightmap dimensions
		bool lastLightmap = ( i == (m_NumLightmapPages-1));
		m_pLightmapPages[i].m_Width = (unsigned short)(lastLightmap ? lastLightmapPageWidth : GetMaxLightmapPageWidth());
		m_pLightmapPages[i].m_Height = (unsigned short)(lastLightmap ? lastLightmapPageHeight : GetMaxLightmapPageHeight());
		m_pLightmapPages[i].m_Flags = 0;

		// GR
		m_pLightmapPages[i].m_bSeparateAlpha = bSeparateAlpha;

		// Allocate bits
		m_pLightmapPages[i].m_pImageData = NULL;
		m_pLightmapPages[i].m_pAlphaData = NULL;
		if (bAllocateLightmapBits)
		{
			m_pLightmapPages[i].m_pImageData = new unsigned char[GetLightmapWidth(i) * GetLightmapHeight(i) * 4];
			Assert( m_pLightmapPages[i].m_pImageData );

			// GR - if necessary allocate space for alpha
			if( bSeparateAlpha )
			{
				m_pLightmapPages[i].m_pAlphaData = new unsigned char[GetLightmapWidth(i) * GetLightmapHeight(i)];
				Assert( m_pLightmapPages[i].m_pAlphaData );
			}

		}

		AllocateLightmapTexture( i );
	}

	// GR - destroy list
	delete [] lightmapAlphaFlag;

	AllocateStandardTextures();
}


//-----------------------------------------------------------------------------
// Allocate lightmap textures
//-----------------------------------------------------------------------------
void CMaterialSystem::AllocateLightmapTexture( int lightmap )
{
	bool bUseDynamicTextures = HardwareConfig()->PreferDynamicTextures();
	bool bAllocAlphaTexture = m_pLightmapPages[lightmap].m_bSeparateAlpha;

	int flags = 0;
	if( bUseDynamicTextures )
	{
		flags |= TEXTURE_CREATE_DYNAMIC;
	}
	else
	{
		flags |= TEXTURE_CREATE_MANAGED;
	}
	
	// don't mipmap lightmaps
	if( HardwareConfig()->GetDXSupportLevel() < 70 )
	{
		g_pShaderAPI->CreateTexture( lightmap + FIRST_LIGHTMAP_TEXID,
			GetLightmapWidth(lightmap), GetLightmapHeight(lightmap), IMAGE_FORMAT_BGRX5551, 
			1, 1, flags );
	}
	else
	{
		g_pShaderAPI->CreateTexture( lightmap + FIRST_LIGHTMAP_TEXID,
			GetLightmapWidth(lightmap), GetLightmapHeight(lightmap), IMAGE_FORMAT_RGBA8888, 
			1, 1, flags );
		if( bAllocAlphaTexture )
		{
			// GR - TODO: we actually need to check if IMAGE_FORMAT_A8 is supported
			g_pShaderAPI->CreateTexture( lightmap + FIRST_LIGHTMAP_ALPHA_TEXID,
				GetLightmapWidth(lightmap), GetLightmapHeight(lightmap), IMAGE_FORMAT_A8, 
				1, 1, flags );
		}
	}

	// Load up the texture data
	g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_TEXID );
	g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexSetPriority( 1 );

	if( bAllocAlphaTexture )
	{
		// Load up the alpha texture data
		g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_ALPHA_TEXID );
		g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
		g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
		g_pShaderAPI->TexSetPriority( 1 );
	}

	// Blat out the lightmap bits
	InitLightmapBits( lightmap );
}


int	CMaterialSystem::AllocateWhiteLightmap( IMaterial *iMaterial )
{
	IMaterialInternal *pMaterial = static_cast<IMaterialInternal *>( iMaterial );
	if( !pMaterial )
	{
		Warning( "Programming error: CMaterialSystem::AllocateWhiteLightmap: NULL material\n" );
		return m_numSortIDs;
	}

	if( !m_currentWhiteLightmapMaterial || ( m_currentWhiteLightmapMaterial != pMaterial ) )
	{
		if( !m_pCurrentMaterial && !m_currentWhiteLightmapMaterial )
		{
			// don't increment if this is the very first material (ie. no lightmaps
			// allocated with AllocateLightmap
			// Assert( 0 );
		}
		else
		{
			// material change
			m_numSortIDs++;
#if 0
			char buf[128];
			sprintf( buf, "AllocateWhiteLightmap: m_numSortIDs = %d %s\n", m_numSortIDs, pMaterial->GetName() );
			OutputDebugString( buf );
#endif
		}
//		Warning( "%d material: \"%s\" lightmapPageID: -1\n", m_numSortIDs, pMaterial->GetName() );
		m_currentWhiteLightmapMaterial = pMaterial;
		pMaterial->SetNeedsWhiteLightmap( true );
	}

	return m_numSortIDs;
}


//-----------------------------------------------------------------------------
// Allocates the standard lightmaps used by the material system
//-----------------------------------------------------------------------------

void CMaterialSystem::AllocateStandardTextures( )
{
	if (m_StandardLightmapsAllocated)
		return;

	m_StandardLightmapsAllocated = true;

	// allocate a white, single texel texture for the fullbright 
	// lightmap at lightmapPage = -1
	// note: make sure and redo this when changing gamma, etc.
	unsigned char texel[3];

	// don't mipmap lightmaps
	g_pShaderAPI->CreateTexture( FULLBRIGHT_LIGHTMAP_TEXID, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_MANAGED );
	g_pShaderAPI->ModifyTexture( FULLBRIGHT_LIGHTMAP_TEXID );
	g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
	float tmpVect[3] = { 1.0f, 1.0f, 1.0f };
	ColorSpace::LinearToLightmap( texel, tmpVect );
	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

	// allocate a black single texel texture
	// lightmap at lightmapPage = -2
	g_pShaderAPI->CreateTexture( BLACK_TEXID, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_MANAGED );
	g_pShaderAPI->ModifyTexture( BLACK_TEXID );
	g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
	texel[0] = texel[1] = texel[2] = 0;
	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

	// allocate a black single texel texture
	// lightmap at lightmapPage = -2
	g_pShaderAPI->CreateTexture( GREY_TEXID, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_MANAGED );
	g_pShaderAPI->ModifyTexture( GREY_TEXID );
	g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
	texel[0] = texel[1] = texel[2] = 128;
	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

	// allocate a single texel flat normal texture at lightmapPage = -3
	g_pShaderAPI->CreateTexture( FLAT_NORMAL_TEXTURE, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_MANAGED );
	g_pShaderAPI->ModifyTexture( FLAT_NORMAL_TEXTURE );
	g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
	texel[0] = texel[1] = texel[2] = 0;
	texel[0] = 127;
	texel[1] = 127;
	texel[2] = 255;
	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

	// allocate a single texel fullbright 1 lightmap for use with bump textures at lightmapPage = -4
	g_pShaderAPI->CreateTexture( FULLBRIGHT_BUMPED_LIGHTMAP_TEXID, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_MANAGED );
	g_pShaderAPI->ModifyTexture( FULLBRIGHT_BUMPED_LIGHTMAP_TEXID );
	g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
	g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
	float linearColor[3] = { 1.0f, 1.0f, 1.0f };
	unsigned char dummy[3];
	ColorSpace::LinearToBumpedLightmap( linearColor, linearColor, linearColor, linearColor,
				dummy, texel, dummy, dummy );
	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

	// allocate two single texel textures for frame sync'ing.
//	g_pShaderAPI->CreateTexture( FRAMESYNC_TEXID1, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_DYNAMIC );
//	g_pShaderAPI->ModifyTexture( FRAMESYNC_TEXID1 );
//	texel[0] = texel[1] = texel[2] = 0;
//	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

//	g_pShaderAPI->CreateTexture( FRAMESYNC_TEXID2, 1, 1, IMAGE_FORMAT_RGB888, 1, 1, TEXTURE_CREATE_DYNAMIC );
//	g_pShaderAPI->ModifyTexture( FRAMESYNC_TEXID2 );
//	texel[0] = texel[1] = texel[2] = 0;
//	g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGB888, 1, 1, IMAGE_FORMAT_RGB888, texel );

}

void CMaterialSystem::ReleaseStandardTextures( )
{
	if (m_StandardLightmapsAllocated)
	{
		g_pShaderAPI->DeleteTexture( FULLBRIGHT_LIGHTMAP_TEXID );
		g_pShaderAPI->DeleteTexture( BLACK_TEXID );
		g_pShaderAPI->DeleteTexture( GREY_TEXID );
		g_pShaderAPI->DeleteTexture( FLAT_NORMAL_TEXTURE );
		g_pShaderAPI->DeleteTexture( FULLBRIGHT_BUMPED_LIGHTMAP_TEXID );
//		g_pShaderAPI->DeleteTexture( FRAMESYNC_TEXID1 );
//		g_pShaderAPI->DeleteTexture( FRAMESYNC_TEXID2 );


		m_StandardLightmapsAllocated = false;
	}
}

void CMaterialSystem::DebugWriteLightmapImages( void )
{
#ifdef WRITE_LIGHTMAP_TGA
	static bool firstTime = true;
	if( !firstTime )
		return;

	firstTime = false;

	{
		int i;
		for( i = 0; i < m_NumLightmapPages; i++ )
		{
			char filename[256];
			sprintf( filename, "lightmap%d.tga", i );
			TGAWriter::Write( m_pLightmapPages[i].m_pImageData, filename, 
				GetLightmapWidth(i), GetLightmapHeight(i), IMAGE_FORMAT_RGB888, 
				IMAGE_FORMAT_RGB888 );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Releases/restores lightmap pages
//-----------------------------------------------------------------------------
void CMaterialSystem::ReleaseLightmapPages()
{
	for( int i = 0; i < m_NumLightmapPages; i++ )
	{
		g_pShaderAPI->DeleteTexture( i + FIRST_LIGHTMAP_TEXID );
	}
}

void CMaterialSystem::RestoreLightmapPages()
{
	for( int i = 0; i < m_NumLightmapPages; i++ )
	{
		AllocateLightmapTexture( i );
	}
}


//-----------------------------------------------------------------------------
// FIXME: This is a hack required for NVidia, can they fix in drivers?
//-----------------------------------------------------------------------------
void CMaterialSystem::DrawScreenSpaceQuad( IMaterial* pMaterial )
{
	// This is required because the texture coordinates for NVidia reading
	// out of non-power-of-two textures is borked
	int w, h;

	GetRenderTargetDimensions( w, h );

	float s0, t0;
	float s1, t1;

	float flOffsetS = 0.5f / w;
	float flOffsetT = 0.5f / h;
	s0 = flOffsetS;
	t0 = flOffsetT;
	s1 = 1.0f - flOffsetS;
	t1 = 1.0f - flOffsetT;

	s0 += flOffsetS;
	t0 += flOffsetT;
	s1 += flOffsetS;
	t1 += flOffsetT;
	
	Bind( pMaterial );
	IMesh* pMesh = GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( -1.0f, -1.0f, 0.0f );
	meshBuilder.TexCoord2f( 0, s0, t1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( -1.0f, 1, 0.0f );
	meshBuilder.TexCoord2f( 0, s0, t0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( 1, 1, 0.0f );
	meshBuilder.TexCoord2f( 0, s1, t0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( 1, -1.0f, 0.0f );
	meshBuilder.TexCoord2f( 0, s1, t1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// This initializes the lightmap bits
//-----------------------------------------------------------------------------
void CMaterialSystem::InitLightmapBitsDynamic( int lightmap )
{
	int width = GetLightmapWidth(lightmap);
	int height = GetLightmapHeight(lightmap);

	// in debug mode, make em green checkerboard
	for( int j = 0; j < height; ++j )
	{
		int texel = 4 * ( width * j );
		unsigned long *pDst = (unsigned long *)(&m_pLightmapPages[lightmap].m_pImageData[texel]);
		for( int k = 0; k < width; ++k )
		{
#ifndef _DEBUG
			*pDst++ = 0xFFFFFFFF;
#else // _DEBUG
			if( ( j + k ) & 1 )
			{
				*pDst++ = 0x00FF00FF;
			}
			else
			{
				*pDst++ = 0x000000FF;
			}
#endif // _DEBUG
		}
	}

	// GR - init alpha for DX 9
	if( m_pLightmapPages[lightmap].m_pAlphaData )
	{
		for( int j = 0; j < height; ++j )
		{
			unsigned char *pDst = (unsigned char *)(&m_pLightmapPages[lightmap].m_pAlphaData[width * j]);
			for( int k = 0; k < width; ++k )
			{
				*pDst++ = 0x00;
			}
		}
	}

	AddLightmapToDirtyList( lightmap );
}

void CMaterialSystem::InitLightmapBits( int lightmap )
{
	if ( HardwareConfig()->PreferDynamicTextures() )
	{
		InitLightmapBitsDynamic( lightmap );
		return;
	}

	int width = GetLightmapWidth(lightmap);
	int height = GetLightmapHeight(lightmap);

	CPixelWriter writer;
	g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_TEXID );
	if (!g_pShaderAPI->TexLock( 0, 0, 0, 0, width, height, writer ))
		return;

	// Debug mode, make em green checkerboard
	for( int j = 0; j < height; ++j )
	{
		writer.Seek( 0, j );
		for( int k = 0; k < width; ++k )
		{
#ifndef _DEBUG
			writer.WritePixel( 255, 255, 255 );
#else // _DEBUG

			if( ( j + k ) & 1 )
			{
				writer.WritePixel( 0, 255, 0 );
			}
			else
			{
				writer.WritePixel( 0, 0, 0 );
			}

#endif // _DEBUG
		}
	}

#ifdef WRITE_LIGHTMAP_TGA
	InitLightmapBitsDynamic( lightmap );
#endif

#ifdef READ_LIGHTMAP_TGA
	char filename[256];
	static unsigned char pImage[MAX_MAX_LIGHTMAP_WIDTH*MAX_MAX_LIGHTMAP_WIDTH*3];
	sprintf( filename, "lightmap%d.tga", lightmap );
	int tgaWidth, tgaHeight;
	float gamma;
	ImageFormat imageFormat;
	if( !TGALoader::GetInfo( filename, &tgaWidth, &tgaHeight, &imageFormat, &gamma ) )
	{
		Assert( 0 );
	}
	Assert( tgaWidth == width );
	Assert( tgaHeight == height );
	TGALoader::Load( pImage, filename, width, height, imageFormat, gamma, false );
	int tgaTexelSize = ImageLoader::GetMemRequired( 1, 1, imageFormat, false );
	for( j = 0; j < height; ++j )
	{
		writer.Seek( 0, j );
		for( int k = 0; k < width; ++k )
		{
			int texel = tgaTexelSize * ( width * j + k );
			// fucking hack . . . need to really look at the image format.
			writer.WritePixel( pImage[texel+2], pImage[texel+1], pImage[texel] );
		}
	}
#endif
	
	g_pShaderAPI->TexUnlock();

	// GR - init alpha for DX 9
	if( m_pLightmapPages[lightmap].m_bSeparateAlpha )
	{
		g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_ALPHA_TEXID );
		if (!g_pShaderAPI->TexLock( 0, 0, 0, 0, width, height, writer ))
			return;

		for( int j = 0; j < height; ++j )
		{
			writer.Seek( 0, j );
			for( int k = 0; k < width; ++k )
			{
				writer.WritePixel( 0, 0, 0, 0 );
			}
		}
		g_pShaderAPI->TexUnlock();
	}
}


//-----------------------------------------------------------------------------
// Updates the lightmap	bits based on a bumped lightmap
//-----------------------------------------------------------------------------
void CMaterialSystem::UpdateBumpedLightmapBitsDynamic( int lightmap, 
	float* pFloatImage,	float *pFloatImageBump1, float *pFloatImageBump2,
	float *pFloatImageBump3, int pLightmapSize[2], int pOffsetIntoLightmapPage[2] )
{
	VPROF( "CMaterialSystem::UpdateBumpedLightmapBits" );

#ifdef READ_LIGHTMAP_TGA
	return;
#endif

	bool bLumInAlpha = false;
	if( g_pHWConfig->SupportsHDR() )
	{
		bLumInAlpha = true;
	}

	unsigned char* pLightmapBits = m_pLightmapPages[lightmap].m_pImageData;

	// NOTE: Change how the lock is taking place if you ever change how bumped
	// lightmaps are put into the page. Right now, we assume that they're all
	// added to the right of the original lightmap.
	const int nLightmapSize0 = pLightmapSize[0];
	const int nLightmapSize1 = pLightmapSize[1];

	int nLightmapWidth = GetLightmapWidth(lightmap);
	unsigned char *pDst0, *pDst1, *pDst2, *pDst3;
	unsigned char color[4][3];
	for( int t = 0; t < nLightmapSize1; t++ )
	{
		int srcTexelOffset = (sizeof(Vector4D)/sizeof(float)) * ( 0 + t * nLightmapSize0 );

		int dstTexelOffset = 4 * ( pOffsetIntoLightmapPage[0] + ( t + pOffsetIntoLightmapPage[1] ) * nLightmapWidth );
		pDst0 = &pLightmapBits[dstTexelOffset];
		pDst1 = &pLightmapBits[dstTexelOffset + nLightmapSize0 * 4];
		pDst2 = &pLightmapBits[dstTexelOffset + nLightmapSize0 * 8];
		pDst3 = &pLightmapBits[dstTexelOffset + nLightmapSize0 * 12];

		for( int s = 0; s < nLightmapSize0; s++, srcTexelOffset += (sizeof(Vector4D)/sizeof(float)))
		{
			ColorSpace::LinearToBumpedLightmap( &pFloatImage[srcTexelOffset],
				&pFloatImageBump1[srcTexelOffset], &pFloatImageBump2[srcTexelOffset],
				&pFloatImageBump3[srcTexelOffset],
				color[0], color[1], color[2], color[3] );
		
			unsigned char alpha =  RoundFloatToByte( pFloatImage[srcTexelOffset+3] * 255.0f );
			
			// Compute luminance
			float lumF = pFloatImage[srcTexelOffset] * 0.30f + pFloatImage[srcTexelOffset+1] * 0.59f + pFloatImage[srcTexelOffset+2] * 0.11f;
			unsigned char lum =  RoundFloatToByte( LinearToVertexLight( lumF ) / MAX_HDR_OVERBRIGHT * 255.0f );

			if( bLumInAlpha )
			{
				// not implemented for HDR
				Assert( 0 );
				*pDst0++ = color[0][0]; *pDst0++ = color[0][1]; *pDst0++ = color[0][2]; *pDst0++ = lum;
			}
			else
			{
				*pDst0++ = color[0][0]; *pDst0++ = color[0][1]; *pDst0++ = color[0][2]; *pDst0++ = alpha;
			}

			*pDst1++ = color[1][0]; *pDst1++ = color[1][1]; *pDst1++ = color[1][2]; *pDst1++ = alpha;
			*pDst2++ = color[2][0]; *pDst2++ = color[2][1]; *pDst2++ = color[2][2]; *pDst2++ = alpha;
			*pDst3++ = color[3][0]; *pDst3++ = color[3][1]; *pDst3++ = color[3][2]; *pDst3++ = alpha;
		}
	}
}


static void HDRToTexture( const float *pSrc, unsigned char *pDst )
{
	float fMax = max( max( pSrc[0], pSrc[1] ), pSrc[2] );
	if( fMax < 1.0f )
	{
		fMax = 1.0f;
	}
	// We want fMax / MAX_HDR_OVERBRIGHT * 255.0f to be an integer.
	int tmpInt = ( int )( fMax / MAX_HDR_OVERBRIGHT * 255.0f );
	if( tmpInt == 0 ) tmpInt = 1;
	fMax = MAX_HDR_OVERBRIGHT / 255.0f * tmpInt;
	float scale = 1.0f / fMax;
	pDst[0] = min( pow( pSrc[0] * scale, 1.0f / 2.2f ) * 255.0f, 255.0f );
	pDst[1] = min( pow( pSrc[1] * scale, 1.0f / 2.2f ) * 255.0f, 255.0f );
	pDst[2] = min( pow( pSrc[2] * scale, 1.0f / 2.2f ) * 255.0f, 255.0f );
	pDst[3] = min( fMax / MAX_HDR_OVERBRIGHT * 255.0f, 255.0f );
}

//-----------------------------------------------------------------------------
// Updates the lightmap	bits based on a bumped lightmap
//-----------------------------------------------------------------------------
void CMaterialSystem::UpdateBumpedLightmapBits( int lightmap, 
	float* pFloatImage,	float *pFloatImageBump1, float *pFloatImageBump2,
	float *pFloatImageBump3, int pLightmapSize[2], int pOffsetIntoLightmapPage[2] )
{
	VPROF( "CMaterialSystem::UpdateBumpedLightmapBits" );
#ifdef READ_LIGHTMAP_TGA
	return;
#endif

	bool bLumInAlpha = false;
	if( g_pHWConfig->SupportsHDR() )
	{
		bLumInAlpha = true;
	}

	// NOTE: Change how the lock is taking place if you ever change how bumped
	// lightmaps are put into the page. Right now, we assume that they're all
	// added to the right of the original lightmap.

	CPixelWriter writer;
	g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_TEXID );
	if (!g_pShaderAPI->TexLock( 0, 0, pOffsetIntoLightmapPage[0], pOffsetIntoLightmapPage[1],
		pLightmapSize[0] * 4, pLightmapSize[1], writer ))
	{
		return;
	}

	const int nLightmapSize0 = pLightmapSize[0];
	const int nLightmap0WriterSizeBytes = nLightmapSize0 * writer.GetPixelSize();
	const int nRewindToNextPixel = -( ( nLightmap0WriterSizeBytes * 3 ) - writer.GetPixelSize() );

	for( int t = 0; t < pLightmapSize[1]; t++ )
	{
		int srcTexelOffset = ( sizeof( Vector4D ) / sizeof( float ) ) * ( 0 + t * nLightmapSize0 );
		writer.Seek( 0, t );	

		for( int s = 0; 
		     s < nLightmapSize0; 
			 s++, writer.SkipBytes(nRewindToNextPixel),srcTexelOffset += (sizeof(Vector4D)/sizeof(float)))
		{
			if( bLumInAlpha )
			{
				unsigned char color[4][4];
	
				Vector vBumpColor1( pFloatImageBump1[srcTexelOffset+0], 
						            pFloatImageBump1[srcTexelOffset+1], 
						            pFloatImageBump1[srcTexelOffset+2] );
				Vector vBumpColor2( pFloatImageBump2[srcTexelOffset+0], 
						            pFloatImageBump2[srcTexelOffset+1], 
						            pFloatImageBump2[srcTexelOffset+2] );
				Vector vBumpColor3( pFloatImageBump3[srcTexelOffset+0], 
						            pFloatImageBump3[srcTexelOffset+1], 
						            pFloatImageBump3[srcTexelOffset+2] );
				Vector vUnbumpedColor( pFloatImage[srcTexelOffset+0], 
						               pFloatImage[srcTexelOffset+1], 
						               pFloatImage[srcTexelOffset+2] );
				Vector average = ( 1.0f / 3.0f ) * ( vBumpColor1 + vBumpColor2 + vBumpColor3 );
				Vector scale( 0.0f, 0.0f, 0.0f );
				int i;
				for( i = 0; i < 3; i++ )
				{
					if( average[i] > 0.0f )
					{
						scale[i] = vUnbumpedColor[i] / average[i];
					}
					else
					{
						scale[i] = 0.0f;
					}
				}
				vBumpColor1 *= scale;
				vBumpColor2 *= scale;
				vBumpColor3 *= scale;
				HDRToTexture( &vUnbumpedColor[0], color[0] );
				HDRToTexture( &vBumpColor1[0], color[1] );
				HDRToTexture( &vBumpColor2[0], color[2] );
				HDRToTexture( &vBumpColor3[0], color[3] );

				writer.WritePixelNoAdvance( color[0][0], color[0][1], color[0][2], color[0][3] );

				writer.SkipBytes( nLightmap0WriterSizeBytes );
				writer.WritePixelNoAdvance( color[1][0], color[1][1], color[1][2], color[1][3] );

				writer.SkipBytes( nLightmap0WriterSizeBytes );
				writer.WritePixelNoAdvance( color[2][0], color[2][1], color[2][2], color[2][3] );

				writer.SkipBytes( nLightmap0WriterSizeBytes );
				writer.WritePixelNoAdvance( color[3][0], color[3][1], color[3][2], color[3][3] );
			}
			else
			{
				unsigned char color[4][3];

				ColorSpace::LinearToBumpedLightmap( &pFloatImage[srcTexelOffset],
					&pFloatImageBump1[srcTexelOffset], &pFloatImageBump2[srcTexelOffset],
					&pFloatImageBump3[srcTexelOffset],
					color[0], color[1], color[2], color[3] );
			
				unsigned char alpha =  RoundFloatToByte( pFloatImage[srcTexelOffset+3] * 255.0f );
				// Compute luminance
				writer.WritePixelNoAdvance( color[0][0], color[0][1], color[0][2], alpha );

				writer.SkipBytes( nLightmap0WriterSizeBytes );
				writer.WritePixelNoAdvance( color[1][0], color[1][1], color[1][2], alpha );

				writer.SkipBytes( nLightmap0WriterSizeBytes );
				writer.WritePixelNoAdvance( color[2][0], color[2][1], color[2][2], alpha );

				writer.SkipBytes( nLightmap0WriterSizeBytes );
				writer.WritePixelNoAdvance( color[3][0], color[3][1], color[3][2], alpha );
			}
		}
	}

	g_pShaderAPI->TexUnlock();

#ifdef WRITE_LIGHTMAP_TGA
	UpdateBumpedLightmapBits( lightmap, pFloatImage, pFloatImageBump1, pFloatImageBump2,
		pFloatImageBump3, pLightmapSize, pOffsetIntoLightmapPage )
#endif
}

			    
//-----------------------------------------------------------------------------
// In the case of dynamic lightmaps, we have to update the backing buffer...
//-----------------------------------------------------------------------------
void CMaterialSystem::UpdateLightmapBitsDynamic( int lightmap, float* pFloatImage,
						int pLightmapSize[2], int pOffsetIntoLightmapPage[2] )
{
	unsigned char* pLightmapBits = m_pLightmapPages[lightmap].m_pImageData;
	int dstTexel = pOffsetIntoLightmapPage[0] + 
		GetLightmapWidth(lightmap) * pOffsetIntoLightmapPage[1];
	unsigned char* pDst = pLightmapBits + 4 * dstTexel;
	int dstRowInc = 4 * ( GetLightmapWidth(lightmap) - pLightmapSize[0] );

	unsigned char color[4];
	float* pSrc;

	if( !g_pHWConfig->SupportsHDR() )
	{
		// DX 8 lightmap processing
		pSrc = pFloatImage;
		for( int t = 0; t < pLightmapSize[1]; ++t )
		{
			for( int s = 0; s < pLightmapSize[0]; ++s, pSrc += sizeof(Vector4D)/sizeof(float) )
			{
				ColorSpace::LinearToLightmap( color, pSrc );
				color[3] =  RoundFloatToByte( pSrc[3] * 255.0f );
				*pDst++ = color[0];
				*pDst++ = color[1];
				*pDst++ = color[2];
				*pDst++ = color[3];
			}
			pDst += dstRowInc;
		}
	}
	else
	{
		// not implemented for HDR
		Assert( 0 );
		// DX 9 lightmap support with HDR capability
		pSrc = pFloatImage;
		for( int t = 0; t < pLightmapSize[1]; ++t )
		{
			for( int s = 0; s < pLightmapSize[0]; ++s, pSrc += sizeof(Vector4D)/sizeof(float) )
			{
				ColorSpace::LinearToLightmap( color, pSrc );
				// Compute luminance
				float lum = pSrc[0] * 0.30f + pSrc[1] * 0.59f + pSrc[2] * 0.11f;
				color[3] =  RoundFloatToByte( LinearToVertexLight( lum ) / MAX_HDR_OVERBRIGHT * 255.0f );
				*pDst++ = color[0];
				*pDst++ = color[1];
				*pDst++ = color[2];
				*pDst++ = color[3];
			}
			pDst += dstRowInc;
		}

		// Process alpha for DX 9
		unsigned char* pAlphaBits = m_pLightmapPages[lightmap].m_pAlphaData;
		if( pAlphaBits )
		{
			pSrc = &pFloatImage[3];
			pDst = pAlphaBits + dstTexel;
			dstRowInc = GetLightmapWidth(lightmap) - pLightmapSize[0];
			for( int t = 0; t < pLightmapSize[1]; ++t )
			{
				for( int s = 0; s < pLightmapSize[0]; ++s, pSrc += sizeof(Vector4D)/sizeof(float) )
				{
					unsigned char alpha = RoundFloatToByte( *pSrc * 255.0f );
					*pDst++ = alpha;
				}
				pDst += dstRowInc;
			}

		}
	}
}

//-----------------------------------------------------------------------------
// Updates the lightmap	bits based on a non-bumped lightmap
//-----------------------------------------------------------------------------
void CMaterialSystem::UpdateLightmapBits( int lightmap, 
	float* pFloatImage,	int pLightmapSize[2], int pOffsetIntoLightmapPage[2] )
{
	VPROF( "CMaterialSystem::UpdateLightmapBits" );

#ifdef READ_LIGHTMAP_TGA
	return;
#endif

	CPixelWriter writer;
	unsigned char color[4];
	float* pSrc;

	// Process lightmap
	g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_TEXID );
	if (!g_pShaderAPI->TexLock( 0, 0, pOffsetIntoLightmapPage[0], pOffsetIntoLightmapPage[1],
		pLightmapSize[0], pLightmapSize[1], writer ))
	{
		return;
	}

	if( !g_pHWConfig->SupportsHDR() )
	{
		// DX 8 lightmap processing
		pSrc = pFloatImage;
		for( int t = 0; t < pLightmapSize[1]; ++t )
		{
			writer.Seek( 0, t );
			for( int s = 0; s < pLightmapSize[0]; ++s, pSrc += (sizeof(Vector4D)/sizeof(*pSrc)) )
			{
				ColorSpace::LinearToLightmap( color, pSrc );
				color[3] =  RoundFloatToByte( pSrc[3] * 255.0f );
				writer.WritePixel( color[0], color[1], color[2], color[3] );
			}
		}
	}
	else
	{
		// DX 9 lightmap support with HDR capability
		pSrc = pFloatImage;
		for( int t = 0; t < pLightmapSize[1]; ++t )
		{
			writer.Seek( 0, t );
			for( int s = 0; s < pLightmapSize[0]; ++s, pSrc += (sizeof(Vector4D)/sizeof(*pSrc)) )
			{
				HDRToTexture( pSrc, color );
				writer.WritePixel( color[0], color[1], color[2], color[3] );
			}
		}
	}

	g_pShaderAPI->TexUnlock();

	// Process alpha for DX 9
	if( ( g_pHWConfig->GetDXSupportLevel() >= 90 ) && m_pLightmapPages[lightmap].m_bSeparateAlpha )
	{
		g_pShaderAPI->ModifyTexture( lightmap + FIRST_LIGHTMAP_ALPHA_TEXID );
		if (!g_pShaderAPI->TexLock( 0, 0, pOffsetIntoLightmapPage[0], pOffsetIntoLightmapPage[1],
			pLightmapSize[0], pLightmapSize[1], writer ))
		{
			return;
		}

		pSrc = &pFloatImage[3];
		for( int t = 0; t < pLightmapSize[1]; ++t )
		{
			writer.Seek( 0, t );
			for( int s = 0; s < pLightmapSize[0]; ++s, pSrc += (sizeof(Vector4D)/sizeof(*pSrc)) )
			{
				unsigned char alpha =  RoundFloatToByte( *pSrc * 255.0f );
				writer.WritePixel( 0, 0, 0, alpha );
			}
		}
	
		g_pShaderAPI->TexUnlock();
	}

#ifdef WRITE_LIGHTMAP_TGA
	UpdateLightmapBitsDynamic( lightmap, pFloatImage, pLightmapSize, pOffsetIntoLightmapPage );
#endif
}


//-----------------------------------------------------------------------------
// Updates the lightmap
//-----------------------------------------------------------------------------
void CMaterialSystem::UpdateLightmap( int lightmapPageID, int lightmapSize[2],
									  int offsetIntoLightmapPage[2], 
									  float *pFloatImage, float *pFloatImageBump1,
									  float *pFloatImageBump2, float *pFloatImageBump3 )
{
	VPROF( "CMaterialSystem::UpdateLightmap" );
	bool hasBump = false;
	if( pFloatImageBump1 && pFloatImageBump2 && pFloatImageBump3 )
	{
		hasBump = true;
	}

	if( lightmapPageID >= m_NumLightmapPages || lightmapPageID < 0 )
	{
		Error( "MaterialSystem_Interface_t::UpdateLightmap lightmapPageID=%d out of range\n", lightmapPageID );
		return;
	}

	if ( HardwareConfig()->PreferDynamicTextures() )
	{
		if( hasBump )
		{
			UpdateBumpedLightmapBitsDynamic( lightmapPageID, pFloatImage, 
				pFloatImageBump1, pFloatImageBump2, pFloatImageBump3,
  	    		lightmapSize, offsetIntoLightmapPage );
		}
		else
		{
			UpdateLightmapBitsDynamic( lightmapPageID, pFloatImage, 
									lightmapSize, offsetIntoLightmapPage );
		}

		// Add the lightmap to the list of dirty lightmaps
		AddLightmapToDirtyList( lightmapPageID );
		return;
	}

	if( hasBump )
	{
		UpdateBumpedLightmapBits( lightmapPageID, pFloatImage, 
			pFloatImageBump1, pFloatImageBump2, pFloatImageBump3,
  	    	lightmapSize, offsetIntoLightmapPage );
	}
	else
	{
		UpdateLightmapBits( lightmapPageID, pFloatImage, 
								lightmapSize, offsetIntoLightmapPage );
	}
}


//-----------------------------------------------------------------------------
// Updates the dirty lightmaps in memory
//-----------------------------------------------------------------------------
void CMaterialSystem::CommitDirtyLightmaps( )
{
	int nDirtyCount = m_DirtyLightmaps.Count();
	for ( int i = 0; i < nDirtyCount; ++i )
	{
		int lightmapPageID = m_DirtyLightmaps[i];
		m_pLightmapPages[ lightmapPageID ].m_Flags &= ~LIGHTMAP_PAGE_DIRTY;

		int nWidth = m_pLightmapPages[ lightmapPageID ].m_Width;
		int nHeight = m_pLightmapPages[ lightmapPageID ].m_Height;

		g_pShaderAPI->ModifyTexture( lightmapPageID + FIRST_LIGHTMAP_TEXID );
		g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_RGBA8888, nWidth, nHeight, IMAGE_FORMAT_RGBA8888, m_pLightmapPages[ lightmapPageID ].m_pImageData );

		// GR - update alpha page
		if ( m_pLightmapPages[ lightmapPageID ].m_pAlphaData )
		{
			g_pShaderAPI->ModifyTexture( lightmapPageID + FIRST_LIGHTMAP_ALPHA_TEXID );
			g_pShaderAPI->TexImage2D( 0, 0, IMAGE_FORMAT_A8, nWidth, nHeight, IMAGE_FORMAT_A8, m_pLightmapPages[ lightmapPageID ].m_pAlphaData );
		}
	}

	m_DirtyLightmaps.RemoveAll();
}


//-----------------------------------------------------------------------------
// Adds the lightmap page to the dirty list
//-----------------------------------------------------------------------------
void CMaterialSystem::AddLightmapToDirtyList( int lightmapPageID )
{
	if ( (m_pLightmapPages[ lightmapPageID ].m_Flags & LIGHTMAP_PAGE_DIRTY) == 0 )
	{
		m_DirtyLightmaps.AddToTail( lightmapPageID );
		m_pLightmapPages[ lightmapPageID ].m_Flags |= LIGHTMAP_PAGE_DIRTY;
	}
}



//-----------------------------------------------------------------------------
// Uploads all lightmaps marked as dirty
//-----------------------------------------------------------------------------
void CMaterialSystem::FlushLightmaps( void )
{
	DebugWriteLightmapImages();
	CommitDirtyLightmaps();
}


int	CMaterialSystem::GetNumSortIDs( void )
{
	return m_numSortIDs;
}

void CMaterialSystem::ComputeSortInfo( MaterialSystem_SortInfo_t* pInfo, int& sortId, bool alpha )
{
	int lightmapPageID;

	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		IMaterialInternal* pMaterial = GetMaterialInternal(i);

		if( pMaterial->GetMinLightmapPageID() > pMaterial->GetMaxLightmapPageID() )
		{
			continue;
		}
		
		//	const IMaterialVar *pTransVar = pMaterial->GetMaterialProperty( MATERIAL_PROPERTY_OPACITY );
		//	if( ( !alpha && ( pTransVar->GetIntValue() == MATERIAL_TRANSLUCENT ) ) ||
		//		( alpha && !( pTransVar->GetIntValue() == MATERIAL_TRANSLUCENT ) ) )
		//	{
		//		return true;
		//	}

	
//		Warning( "sort stuff: %s %s\n", material->GetName(), bAlpha ? "alpha" : "not alpha" );
		
		// fill in the lightmapped materials
		for( lightmapPageID = pMaterial->GetMinLightmapPageID(); 
			 lightmapPageID <= pMaterial->GetMaxLightmapPageID(); ++lightmapPageID )
		{
			pInfo[sortId].material = pMaterial;
			pInfo[sortId].lightmapPageID = lightmapPageID;

#if 0
			char buf[128];
			sprintf( buf, "ComputeSortInfo: %s lightmapPageID: %d sortID: %d\n", pMaterial->GetName(), lightmapPageID, sortId );
			OutputDebugString( buf );
#endif

			++sortId;

		}
	}
}

void CMaterialSystem::ComputeWhiteLightmappedSortInfo( MaterialSystem_SortInfo_t* pInfo, int& sortId, bool alpha )
{
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		IMaterialInternal* pMaterial = GetMaterialInternal(i);

		// fill in the lightmapped materials that are actually used by this level
		if( pMaterial->GetNeedsWhiteLightmap() && 
			( pMaterial->GetReferenceCount() > 0 ) )
		{
			// const IMaterialVar *pTransVar = pMaterial->GetMaterialProperty( MATERIAL_PROPERTY_OPACITY );
			//		if( ( !alpha && ( pTransVar->GetIntValue() == MATERIAL_TRANSLUCENT ) ) ||
			//			( alpha && !( pTransVar->GetIntValue() == MATERIAL_TRANSLUCENT ) ) )
			//		{
			//			return true;
			//		}

			pInfo[sortId].material = pMaterial;
			pInfo[sortId].lightmapPageID = -1; // hack : need to name this

#if 0
			char buf[128];
			sprintf( buf, "ComputeWhiteLightmappedSortInfo: %s lightmapPageID: %d sortID: %d\n", pMaterial->GetName(), -1, sortId );
			OutputDebugString( buf );
#endif

			sortId++;

		}
	}
}

void CMaterialSystem::GetSortInfo( MaterialSystem_SortInfo_t *pSortInfoArray )
{
	// sort non-alpha blended materials first
	int sortId = 0;
	ComputeSortInfo( pSortInfoArray, sortId, false );
	ComputeWhiteLightmappedSortInfo( pSortInfoArray, sortId, false );

	// . . and then alpha blended materials
//	ComputeSortInfo ( true );
//	ComputeWhiteLightmappedSortInfo ( true );

	// BUGBUG: This breaks when you have unlit textures in one map, but not
	// in the next!?!?!
	Assert( m_numSortIDs == sortId );
}

void CMaterialSystem::BeginFrame( void )
{
	VPROF( "CMaterialSystem::BeginFrame" );
	if( g_config.m_bForceHardwareSync )
	{
		g_pShaderAPI->ForceHardwareSync();
	}

	g_pShaderAPI->BeginFrame();
}

void CMaterialSystem::EndFrame( void )
{
	VPROF( "CMaterialSystem::EndFrame" );
	g_pShaderAPI->EndFrame();
}

void CMaterialSystem::Bind( IMaterial *iMaterial, void *proxyData )
{
	IMaterialInternal *material = static_cast<IMaterialInternal *>( iMaterial );

	if( !material )
	{
		Warning( "Programming error: CMaterialSystem::Bind: NULL material\n" );
		material = static_cast<IMaterialInternal *>( s_pErrorMaterial );
	}

	if (g_config.bDrawFlat && !material->NoDebugOverride())
	{
		material = static_cast<IMaterialInternal *>( s_pDrawFlatMaterial );
	}
	
	if( m_pCurrentMaterial != material )
	{
		if( !material->IsPrecached() )
		{
			Warning( "Binding uncached material \"%s\", artificially incrementing refcount\n", material->GetName() );
			material->IncrementReferenceCount();
			material->Precache();
		}
		m_pCurrentMaterial = material;
	}
	
	// We've always gotta call the bind proxy
	m_pCurrentMaterial->CallBindProxy( proxyData );
	g_pShaderAPI->Bind( m_pCurrentMaterial );
}

void CMaterialSystem::SetLight( int lightNum, LightDesc_t& desc )
{
	g_pShaderAPI->SetLight( lightNum, desc );
}

void CMaterialSystem::SetAmbientLight( float r, float g, float b )
{
	g_pShaderAPI->SetAmbientLight( r, g, b );
}

void CMaterialSystem::SetAmbientLightCube( Vector4D cube[6] )
{
	g_pShaderAPI->SetAmbientLightCube( cube );
}

void CMaterialSystem::CopyRenderTargetToTexture( ITexture *pTexture )
{
	if( !pTexture )
	{
		Assert( 0 );
		return;
	}
	if( HardwareConfig()->SupportsVertexAndPixelShaders() && 
		HardwareConfig()->SupportsNonPow2Textures() &&
		!g_config.bEditMode )
	{
		ITextureInternal *pTextureInternal = ( ITextureInternal * )pTexture;
		pTextureInternal->CopyFrameBufferToMe();
	}
}

void CMaterialSystem::DepthRange( float zNear, float zFar )
{
	g_pShaderAPI->DepthRange( zNear, zFar );
}

void CMaterialSystem::ClearBuffers( bool bClearColor, bool bClearDepth )
{
	int width, height;
	GetRenderTargetDimensions( width, height );
	g_pShaderAPI->ClearBuffers( bClearColor, bClearDepth, width, height );
}

void CMaterialSystem::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	g_pShaderAPI->ClearColor3ub( r, g, b );
}

void CMaterialSystem::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	g_pShaderAPI->ClearColor4ub( r, g, b, a );
}

void CMaterialSystem::SetInStubMode( bool bInStubMode )
{
	m_bInStubMode = bInStubMode;
}

bool CMaterialSystem::IsInStubMode()
{
	return m_bInStubMode;
}

void CMaterialSystem::ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat )
{
	g_pShaderAPI->ReadPixels( x, y, width, height, data, dstFormat );
}

void CMaterialSystem::Flush( bool flushHardware )
{
	VPROF( "CMaterialSystem::Flush" );
	g_pShaderAPI->FlushBufferedPrimitives();

	if( flushHardware )
	{
		g_pShaderAPI->FlushBufferedPrimitives();
	}
}

void CMaterialSystem::DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose )
{
	for (MaterialHandle_t i = FirstMaterial(); i != InvalidMaterial(); i = NextMaterial(i) )
	{
		IMaterialInternal *pMaterial = GetMaterialInternal(i);

		if( pMaterial->GetReferenceCount() < 0 )
		{
			Warning( "DebugShowUsedMaterialsCallback: refcount for material \"%s\" (%d) < 0\n",
				pMaterial->GetName(), pMaterial->GetReferenceCount() );
		}

		if( pMaterial->GetReferenceCount() > 0 )
		{
			if( pSearchSubString )
			{
				if( !Q_stristr( pMaterial->GetName(), pSearchSubString ) &&
					!Q_stristr( pMaterial->GetShader()->GetName(), pSearchSubString ) )
				{
					continue;
				}
			}

			Warning( "material %s (shader %s) refcount: %d.\n", pMaterial->GetName(), 
				pMaterial->GetShader() ? pMaterial->GetShader()->GetName() : "unknown\n", pMaterial->GetReferenceCount() );

			if( !bVerbose )
			{
				continue;
			}
			if( pMaterial->IsPrecached() )
			{
				int i;
				if( pMaterial->GetShader() )
				{
					for( i = 0; i < pMaterial->GetShader()->GetNumParams(); i++ )
					{
						IMaterialVar *var;
						var = pMaterial->GetShaderParams()[i];
						
						if( var )
						{
							switch( var->GetType() )
							{
							case MATERIAL_VAR_TYPE_TEXTURE:
								{
									ITextureInternal *texture = static_cast<ITextureInternal *>( var->GetTextureValue() );
									if( !texture )
									{
										Warning( "Programming error: CMaterialSystem::DebugPrintUsedMaterialsCallback: NULL texture\n" );
										continue;
									}
									
									// garymcthack - check for env_cubemap
									if( texture == ( ITextureInternal * )-1 )
									{
										Warning( "    \"%s\" \"env_cubemap\"\n", var->GetName() );
									}
									else
									{
										Warning( "    \"%s\" \"%s\"\n", 
											var->GetName(),
											texture->GetName() );
										Warning( "        %dx%d refCount: %d numframes: %d\n", texture->GetActualWidth(), texture->GetActualHeight(), 
											texture->GetReferenceCount(), texture->GetNumAnimationFrames() );
									}
								}
								break;
							case MATERIAL_VAR_TYPE_UNDEFINED:
								break;
							default:
								Warning( "    \"%s\" \"%s\"\n", var->GetName(), var->GetStringValue() );
								break;
							}
						}
					}
				}
			}
		}
	}
}

void CMaterialSystem::DebugPrintUsedTextures( void )
{
	TextureManager()->DebugPrintUsedTextures();
}

void CMaterialSystem::ToggleSuppressMaterial( char const* pMaterialName )
{
	/*
	// This version suppresses all but the material
	IMaterial *pMaterial = GetFirstMaterial();
	while (pMaterial)
	{
		if (stricmp(pMaterial->GetName(), pMaterialName))
		{
			IMaterialInternal* pMatInt = static_cast<IMaterialInternal*>(pMaterial);
			pMatInt->ToggleSuppression();
		}
		pMaterial = GetNextMaterial();
	}
	*/

	bool found;
	IMaterial* pMaterial = FindMaterial( pMaterialName, &found );
	if (found)
	{
		IMaterialInternal* pMatInt = static_cast<IMaterialInternal*>(pMaterial);
		pMatInt->ToggleSuppression();
	}
}

void CMaterialSystem::ToggleDebugMaterial( char const* pMaterialName )
{
	bool found;
	IMaterial* pMaterial = FindMaterial( pMaterialName, &found, false );
	if (found)
	{
		IMaterialInternal* pMatInt = static_cast<IMaterialInternal*>(pMaterial);
		pMatInt->ToggleDebugTrace();
	}
	else
	{
		Warning("Unknown material %s\n", pMaterialName );
	}
}

#define MAXPRINTMSG 1024
void CMaterialSystem::DrawDebugText( int desiredLeft, int desiredTop, 
									 MaterialRect_t *actualRect,
									 const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	// garymcthack - is there a way to force vsprintf to not overflow msg?
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );
	Assert( strlen( msg ) < MAXPRINTMSG - 1 );

	g_pShaderAPI->DrawDebugText( desiredLeft, desiredTop, actualRect, msg );
}

void CMaterialSystem::MatrixMode( MaterialMatrixMode_t matrixMode )
{
	g_pShaderAPI->MatrixMode( matrixMode );
}

void CMaterialSystem::PushMatrix()
{
	g_pShaderAPI->PushMatrix();
}

void CMaterialSystem::PopMatrix()
{
	g_pShaderAPI->PopMatrix();
}

void CMaterialSystem::LoadMatrix( float *m )
{
	g_pShaderAPI->LoadMatrix( m );
}

void CMaterialSystem::LoadMatrix( VMatrix const& matrix )
{
	// Bah. Gotta transpose the matrix for the shader system
	VMatrix transposeMatrix;
	MatrixTranspose( matrix, transposeMatrix );
	g_pShaderAPI->LoadMatrix( (float*)transposeMatrix.m );
}

void CMaterialSystem::MultMatrix( float *m )
{
	g_pShaderAPI->MultMatrix( m );
}

void CMaterialSystem::MultMatrixLocal( float *m )
{
	g_pShaderAPI->MultMatrixLocal( m );
}

void CMaterialSystem::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
	g_pShaderAPI->GetMatrix( matrixMode, dst );
}

void CMaterialSystem::GetMatrix( MaterialMatrixMode_t matrixMode, VMatrix *pMatrix )
{
	Assert( pMatrix );

	// Bah. Gotta transpose the matrix for the shader system
	VMatrix transposeMatrix;
	g_pShaderAPI->GetMatrix( matrixMode, (float*)transposeMatrix.m );
	MatrixTranspose( transposeMatrix, *pMatrix );
}

void CMaterialSystem::LoadIdentity( void )
{
	g_pShaderAPI->LoadIdentity();
}

void CMaterialSystem::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
	g_pShaderAPI->Ortho( left, top, right, bottom, zNear, zFar );
}

void CMaterialSystem::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
	g_pShaderAPI->PerspectiveX( fovx, aspect, zNear, zFar );
}

void CMaterialSystem::PickMatrix( int x, int y, int width, int height )
{
	g_pShaderAPI->PickMatrix( x, y, width, height );
}

void CMaterialSystem::Rotate( float angle, float x, float y, float z )
{
	g_pShaderAPI->Rotate( angle, x, y, z );
}

void CMaterialSystem::Translate( float x, float y, float z )
{
	g_pShaderAPI->Translate( x, y, z );
}

void CMaterialSystem::Scale( float x, float y, float z )
{
	g_pShaderAPI->Scale( x, y, z );
}

void CMaterialSystem::Viewport( int x, int y, int width, int height )
{
	g_pShaderAPI->Viewport( x, y, width, height );
}

void CMaterialSystem::GetViewport( int& x, int& y, int& width, int& height ) const
{
	g_pShaderAPI->GetViewport( x, y, width, height );
}

void CMaterialSystem::PointSize( float size )
{
	g_pShaderAPI->PointSize( size );
}

// The cull mode
void CMaterialSystem::CullMode( MaterialCullMode_t cullMode )
{
	g_pShaderAPI->CullMode( cullMode );
}

void CMaterialSystem::ForceDepthFuncEquals( bool bEnable )
{
	g_pShaderAPI->ForceDepthFuncEquals( bEnable );
}

// Allows us to override the depth buffer setting of a material
void CMaterialSystem::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
	g_pShaderAPI->OverrideDepthEnable( bEnable, bDepthEnable );
}

/*
void CMaterialSystem::RenderZOnlyWithHeightClip( bool bEnable )
{
	if( m_bRenderDepthOnlyWithHeightClip != bEnable )
	{
		g_pShaderAPI->FlushBufferedPrimitives();
		m_bRenderDepthOnlyWithHeightClip = bEnable;
	}
}
*/

void CMaterialSystem::UpdateHeightClipUserClipPlane( void )
{
	float plane[4];
	switch( m_HeightClipMode )
	{
	case MATERIAL_HEIGHTCLIPMODE_DISABLE:
		g_pShaderAPI->EnableClipPlane( 0, false );
		break;
	case MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT:
		plane[0] = 0.0f;
		plane[1] = 0.0f;
		plane[2] = 1.0f;
		plane[3] = m_HeightClipZ;
		g_pShaderAPI->SetClipPlane( 0, plane );
		g_pShaderAPI->EnableClipPlane( 0, true );
		break;
	case MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT:
		plane[0] = 0.0f;
		plane[1] = 0.0f;
		plane[2] = -1.0f;
		plane[3] = -m_HeightClipZ;
		g_pShaderAPI->SetClipPlane( 0, plane );
		g_pShaderAPI->EnableClipPlane( 0, true );
		break;
	}
}

void CMaterialSystem::SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode )
{
	if( m_HeightClipMode != heightClipMode  )
	{
		m_HeightClipMode = heightClipMode;
		if ( HardwareConfig()->MaxUserClipPlanes() >= 1 && !HardwareConfig()->UseFastClipping())
		{
			UpdateHeightClipUserClipPlane();			
		}
		else
		{
			g_pShaderAPI->SetHeightClipMode( heightClipMode );
		}
	}
}

enum MaterialHeightClipMode_t CMaterialSystem::GetHeightClipMode( void )
{
	return m_HeightClipMode;
}

void CMaterialSystem::SetHeightClipZ( float z )
{
	m_HeightClipZ = z;
	// FIXME!  : Need to move user clip plane support back to pre-dx9 cards (all of the pixel shaders
	// have texkill in them. . blich.)
	if ( HardwareConfig()->MaxUserClipPlanes() >= 1 && !HardwareConfig()->UseFastClipping() )
	{
		UpdateHeightClipUserClipPlane();			
	}
	else
	{
		g_pShaderAPI->SetHeightClipZ( z );
	}
}

// Sets the number of bones for skinning
void CMaterialSystem::SetNumBoneWeights( int numBones )
{
	g_pShaderAPI->SetNumBoneWeights( numBones );
}

void CMaterialSystem::SwapBuffers( void )
{
	VPROF_BUDGET( "CMaterialSystem::SwapBuffers", VPROF_BUDGETGROUP_SWAP_BUFFERS );
	g_pShaderAPI->SwapBuffers();
	g_FrameNum++;
}

void CMaterialSystem::SetHardwareGamma( float gamma )
{
	return;
	
#if 0
	GammaRamp_t gammaRamp;
	
	int i;
	for( i = 0; i < 256; i++ )
	{
		float f;
		f = 255.0f * pow( i / 255.0f, 1.0f / gamma );
		if( f < 0.0f )
		{
			f = 0.0f;
		}
		if( f > 255.0f )
		{
			f = 255.0f;
		}
		int low, high;
		low = ( int )f;
		high = ( int )( ( f - ( int )f ) * 256.0f );

		gammaRamp.red[i].high = high;
		gammaRamp.red[i].low = low;
		gammaRamp.grn[i].high = high;
		gammaRamp.grn[i].low = low;
		gammaRamp.blu[i].high = high;
		gammaRamp.blu[i].low = low;
	}
	
	if( !SetDeviceGammaRamp( m_mainDC, ( void * )&gammaRamp ) ) 
	{
		Error( "Can't SetDeviceGammaRamp" );
	}
#endif
}

void CMaterialSystem::SaveHardwareGammaRamp( void )
{
	return;
#if 0
	if( !GetDeviceGammaRamp( m_mainDC, ( void * )&m_savedGammaRamp ) ) 
	{
		Error( "Can't GetDeviceGammaRamp" );
	}
#endif
}

void CMaterialSystem::RestoreHardwareGammaRamp( void )
{
	return;
#if 0
	if( !SetDeviceGammaRamp( m_mainDC, ( void * )&m_savedGammaRamp ) ) 
	{
		Error( "Can't SetDeviceGammaRamp" );
	}
#endif
}

void CMaterialSystem::EnableLightmapFiltering( bool enabled )
{
	int i;
	for( i = 0; i < m_NumLightmapPages; i++ )
	{
		g_pShaderAPI->ModifyTexture( i + FIRST_LIGHTMAP_TEXID );
		if( enabled )
		{
			g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
			g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
		}
		else
		{
			g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_NEAREST );
			g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_NEAREST );
		}
	}
}

//-----------------------------------------------------------------------------
// Lightmap stuff
//-----------------------------------------------------------------------------

void CMaterialSystem::BindLightmapPage( int lightmapPageID )
{
	if( m_lightmapPageID == lightmapPageID )
		return;

	// We gotta make sure there's no buffered primitives 'cause this'll
	// change the render state.
	g_pShaderAPI->FlushBufferedPrimitives();

	m_lightmapPageID = lightmapPageID;
}

void CMaterialSystem::SetForceBindWhiteLightmaps( bool bForce )
{
	m_bForceBindWhiteLightmaps = bForce;
}

int CMaterialSystem::GetLightmapPage( void )
{
	return m_lightmapPageID;
}

void CMaterialSystem::BindLightmap( TextureStage_t stage )
{
	if (m_NumLightmapPages != 0 && !m_bForceBindWhiteLightmaps)
		g_pShaderAPI->BindTexture( stage, m_lightmapPageID + FIRST_LIGHTMAP_TEXID );
	else
		BindWhite(stage);
}

void CMaterialSystem::BindLightmapAlpha( TextureStage_t stage )
{
	if (m_NumLightmapPages != 0 && !m_bForceBindWhiteLightmaps && m_pLightmapPages[m_lightmapPageID].m_bSeparateAlpha)
		g_pShaderAPI->BindTexture( stage, m_lightmapPageID + FIRST_LIGHTMAP_ALPHA_TEXID );
	else
		BindWhite(stage);
}

void CMaterialSystem::BindBumpLightmap( TextureStage_t stage )
{
	if (m_NumLightmapPages != 0)
	{
		g_pShaderAPI->BindTexture( stage,	m_lightmapPageID + FIRST_LIGHTMAP_TEXID );
		g_pShaderAPI->BindTexture( (TextureStage_t)(stage+1), m_lightmapPageID + FIRST_LIGHTMAP_TEXID );
		g_pShaderAPI->BindTexture( (TextureStage_t)(stage+2), m_lightmapPageID + FIRST_LIGHTMAP_TEXID );
	}
	else
	{
		BindWhite(stage);
		BindWhite((TextureStage_t)(stage+1));
		BindWhite((TextureStage_t)(stage+2));
	}
}

void CMaterialSystem::BindWhite( TextureStage_t stage )
{
	// FIXME: Surely, there's a better place for this...
	// in here for WC
	AllocateStandardTextures();
	g_pShaderAPI->BindTexture( stage, FULLBRIGHT_LIGHTMAP_TEXID );
}

void CMaterialSystem::BindBlack( TextureStage_t stage )
{
	// FIXME: Surely, there's a better place for this...
	// in here for WC
	AllocateStandardTextures();
	g_pShaderAPI->BindTexture( stage, BLACK_TEXID );
}

void CMaterialSystem::BindGrey( TextureStage_t stage )
{
	AllocateStandardTextures();
	g_pShaderAPI->BindTexture( stage, GREY_TEXID );
}

void CMaterialSystem::BindFlatNormalMap( TextureStage_t stage )
{
	// FIXME: Surely, there's a better place for this...
	// in here for WC
	AllocateStandardTextures();
	g_pShaderAPI->BindTexture( stage, FLAT_NORMAL_TEXTURE );
}

void CMaterialSystem::BindSyncTexture( TextureStage_t stage, int texture )
{
	// FIXME: Surely, there's a better place for this...
	// in here for WC
	AllocateStandardTextures();
	g_pShaderAPI->BindTexture( stage, texture );
}

void CMaterialSystem::BindFBTexture( TextureStage_t stage )
{
//	Assert( m_pCurrentFrameBufferCopyTexture );  // FIXME: Need to make Hammer bind something for this.
	if( !m_pCurrentFrameBufferCopyTexture )
	{
		return;
	}

	if( m_pCurrentFrameBufferCopyTexture )
	{
		( ( ITextureInternal * )m_pCurrentFrameBufferCopyTexture )->Bind( stage );
	}
}

void CMaterialSystem::BindNormalizationCubeMap( TextureStage_t stage )
{
	// FIXME: Surely, there's a better place for this...
	// in here for WC
	if (HardwareConfig()->SupportsCubeMaps())
	{
		TextureManager()->NormalizationCubemap()->Bind( stage );
	}
}


//-----------------------------------------------------------------------------
// Selection mode methods
//-----------------------------------------------------------------------------

int CMaterialSystem::SelectionMode( bool selectionMode )
{
	return g_pShaderAPI->SelectionMode( selectionMode );
}

void CMaterialSystem::SelectionBuffer( unsigned int* pBuffer, int size )
{
	g_pShaderAPI->SelectionBuffer( pBuffer, size );
}

void CMaterialSystem::ClearSelectionNames( )
{
	g_pShaderAPI->ClearSelectionNames( );
}

void CMaterialSystem::LoadSelectionName( int name )
{
	g_pShaderAPI->LoadSelectionName( name );
}

void CMaterialSystem::PushSelectionName( int name )
{
	g_pShaderAPI->PushSelectionName( name );
}

void CMaterialSystem::PopSelectionName()
{
	g_pShaderAPI->PopSelectionName( );
}


//-----------------------------------------------------------------------------
// Fog methods
//-----------------------------------------------------------------------------
void CMaterialSystem::FogMode( MaterialFogMode_t fogMode )
{
	g_pShaderAPI->SceneFogMode( fogMode );
}

void CMaterialSystem::FogStart( float fStart )
{
	g_pShaderAPI->FogStart( fStart );
}

void CMaterialSystem::FogEnd( float fEnd )
{
	g_pShaderAPI->FogEnd( fEnd );
}

void CMaterialSystem::SetFogZ( float fogZ )
{
	g_pShaderAPI->SetFogZ( fogZ );
}

void CMaterialSystem::FogColor3f( float r, float g, float b )
{
	unsigned char fogColor[3];
	fogColor[0] = clamp( (int)(r * 255.0f), 0, 255 );
	fogColor[1] = clamp( (int)(g * 255.0f), 0, 255 );
	fogColor[2] = clamp( (int)(b * 255.0f), 0, 255 );
	g_pShaderAPI->SceneFogColor3ub( fogColor[0], fogColor[1], fogColor[2] );
}

void CMaterialSystem::FogColor3fv( float const* rgb )
{
	unsigned char fogColor[3];
	fogColor[0] = clamp( (int)(rgb[0] * 255.0f), 0, 255 );
	fogColor[1] = clamp( (int)(rgb[1] * 255.0f), 0, 255 );
	fogColor[2] = clamp( (int)(rgb[2] * 255.0f), 0, 255 );
	g_pShaderAPI->SceneFogColor3ub( fogColor[0], fogColor[1], fogColor[2] );
}

void CMaterialSystem::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	g_pShaderAPI->SceneFogColor3ub( r, g, b );
}

void CMaterialSystem::FogColor3ubv( unsigned char const* rgb )
{
	g_pShaderAPI->SceneFogColor3ub( rgb[0], rgb[1], rgb[2] );
}

MaterialFogMode_t CMaterialSystem::GetFogMode( void )
{
	return g_pShaderAPI->GetSceneFogMode();
}

ImageFormat	CMaterialSystem::GetBackBufferFormat() const
{
	return g_pShaderAPI->GetBackBufferFormat();
}

void CMaterialSystem::GetBackBufferDimensions( int &width, int &height ) const
{
	g_pShaderAPI->GetBackBufferDimensions( width, height );
}

bool CMaterialSystem::KeepLightmapAlphaSeparate()
{
	return ( g_pHWConfig->GetDXSupportLevel() >= 90 && g_pHWConfig->SupportsHDR() );
}
