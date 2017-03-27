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
// main material system interface
//=============================================================================

#ifndef IMATERIALSYSTEM_H
#define IMATERIALSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "vector.h"
#include "vector4d.h"
#include "appframework/IAppSystem.h"
#include "imageloader.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class IMesh;
struct MaterialSystem_Config_t;
class VMatrix;


//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------

// NOTE NOTE NOTE!!!!  If you up this, grep for "NEW_INTERFACE" to see if there is anything
// waiting to be enabled during an interface revision.
#define MATERIAL_SYSTEM_INTERFACE_VERSION "VMaterialSystem060"

enum ShaderParamType_t 
{ 
	SHADER_PARAM_TYPE_TEXTURE, 
	SHADER_PARAM_TYPE_INTEGER,
	SHADER_PARAM_TYPE_COLOR,
	SHADER_PARAM_TYPE_VEC2,
	SHADER_PARAM_TYPE_VEC3,
	SHADER_PARAM_TYPE_VEC4,
	SHADER_PARAM_TYPE_ENVMAP,	// obsolete
	SHADER_PARAM_TYPE_FLOAT,
	SHADER_PARAM_TYPE_BOOL,
	SHADER_PARAM_TYPE_FOURCC,
	SHADER_PARAM_TYPE_MATRIX,
	SHADER_PARAM_TYPE_MATERIAL,
};

enum MaterialMatrixMode_t
{
	MATERIAL_VIEW = 0,
	MATERIAL_PROJECTION,

	// Texture matrices
	MATERIAL_TEXTURE0,
	MATERIAL_TEXTURE1,
	MATERIAL_TEXTURE2,
	MATERIAL_TEXTURE3,
	MATERIAL_TEXTURE4,
	MATERIAL_TEXTURE5,
	MATERIAL_TEXTURE6,
	MATERIAL_TEXTURE7,

	MATERIAL_MODEL,

	// FIXME: How do I specify the actual number of matrix modes?
	NUM_MODEL_TRANSFORMS = 16,
	MATERIAL_MODEL_MAX = MATERIAL_MODEL + NUM_MODEL_TRANSFORMS, 

	// Total number of matrices
	NUM_MATRIX_MODES = MATERIAL_MODEL_MAX,

	// Number of texture transforms
	NUM_TEXTURE_TRANSFORMS = MATERIAL_TEXTURE7 - MATERIAL_TEXTURE0 + 1
};

#define MATERIAL_MODEL_MATRIX( _n ) (MaterialMatrixMode_t)(MATERIAL_MODEL + (_n))

enum MaterialPrimitiveType_t 
{ 
	MATERIAL_POINTS			= 0x0,
	MATERIAL_LINES,
	MATERIAL_TRIANGLES,
	MATERIAL_TRIANGLE_STRIP,
	MATERIAL_LINE_STRIP,
	MATERIAL_LINE_LOOP,	// a single line loop
	MATERIAL_POLYGON,	// this is a *single* polygon
	MATERIAL_QUADS,

	// This is used for static meshes that contain multiple types of
	// primitive types.	When calling draw, you'll need to specify
	// a primitive type.
	MATERIAL_HETEROGENOUS
};

enum MaterialPropertyTypes_t
{
	MATERIAL_PROPERTY_NEEDS_LIGHTMAP = 0,					// bool
	MATERIAL_PROPERTY_OPACITY,								// int (enum MaterialPropertyOpacityTypes_t)
	MATERIAL_PROPERTY_REFLECTIVITY,							// vec3_t
	MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS				// bool
};

// acceptable property values for MATERIAL_PROPERTY_OPACITY
enum MaterialPropertyOpacityTypes_t
{
	MATERIAL_ALPHATEST = 0,
	MATERIAL_OPAQUE,
	MATERIAL_TRANSLUCENT
};

enum MaterialBufferTypes_t
{
	MATERIAL_FRONT = 0,
	MATERIAL_BACK
};

enum MaterialCullMode_t
{
	MATERIAL_CULLMODE_CCW,	// this culls polygons with counterclockwise winding
	MATERIAL_CULLMODE_CW	// this culls polygons with clockwise winding
};

enum MaterialVertexFormat_t
{
	MATERIAL_VERTEX_FORMAT_MODEL,
	MATERIAL_VERTEX_FORMAT_COLOR,
};

enum MaterialFogMode_t
{
	MATERIAL_FOG_NONE,
	MATERIAL_FOG_LINEAR,
	MATERIAL_FOG_LINEAR_BELOW_FOG_Z,
};

enum MaterialHeightClipMode_t
{
	MATERIAL_HEIGHTCLIPMODE_DISABLE,
	MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT,
	MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT
};

//-----------------------------------------------------------------------------
// Light structure
//-----------------------------------------------------------------------------

enum LightType_t
{
	MATERIAL_LIGHT_DISABLE = 0,
	MATERIAL_LIGHT_POINT,
	MATERIAL_LIGHT_DIRECTIONAL,
	MATERIAL_LIGHT_SPOT,
};

enum LightType_OptimizationFlags_t
{
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 = 1,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 = 2,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 = 4,
};

struct LightDesc_t 
{
    LightType_t		m_Type;
	Vector			m_Color;
    Vector	m_Position;
    Vector  m_Direction;
    float   m_Range;
    float   m_Falloff;
    float   m_Attenuation0;
    float   m_Attenuation1;
    float   m_Attenuation2;
    float   m_Theta;
    float   m_Phi;
	// These aren't used by DX8. . used for software lighting.
	float	m_ThetaDot;
	float	m_PhiDot;
	unsigned int	m_Flags;
};


//-----------------------------------------------------------------------------
// Standard lightmaps
//-----------------------------------------------------------------------------
enum StandardLightmap_t
{
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE = -1,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_BLACK = -2,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_FLAT_NORMAL = -3,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE_BUMP = -4
};


struct MaterialSystem_SortInfo_t
{
	IMaterial *material;
	int lightmapPageID;
};


//-----------------------------------------------------------------------------
// Information about each adapter
//-----------------------------------------------------------------------------
enum
{
	MATERIAL_ADAPTER_NAME_LENGTH = 512
};

struct MaterialAdapterInfo_t
{
	char m_pDriverName[MATERIAL_ADAPTER_NAME_LENGTH];
	char m_pDriverDescription[MATERIAL_ADAPTER_NAME_LENGTH];
};

struct Material3DDriverInfo_t
{
	char m_pDriverName[MATERIAL_ADAPTER_NAME_LENGTH];
	char m_pDriverDescription[MATERIAL_ADAPTER_NAME_LENGTH];
	char m_pDriverVersion[MATERIAL_ADAPTER_NAME_LENGTH];
	unsigned int m_VendorID;
	unsigned int m_DeviceID;
	unsigned int m_SubSysID;
	unsigned int m_Revision;
	unsigned int m_WHQLLevel;

};

//-----------------------------------------------------------------------------
// Video mode info..
//-----------------------------------------------------------------------------

struct MaterialVideoMode_t
{
	int m_Width;			// if width and height are 0 and you select 
	int m_Height;			// windowed mode, it'll use the window size
	ImageFormat m_Format;	// use ImageFormats (ignored for windowed mode)
	int m_RefreshRate;		// 0 == default (ignored for windowed mode)
};


//-----------------------------------------------------------------------------
// Flags to be used with the Init call
//-----------------------------------------------------------------------------
enum MaterialInitFlags_t
{
	MATERIAL_INIT_REFERENCE_RASTERIZER	= 0x4,
};


//-----------------------------------------------------------------------------
// Flags to be used when setting video mode..
//-----------------------------------------------------------------------------
enum MaterialVideoModeFlags_t
{
	MATERIAL_VIDEO_MODE_WINDOWED				= 0x1,
	MATERIAL_VIDEO_MODE_RESIZING				= 0x2,
	MATERIAL_VIDEO_MODE_SOFTWARE_TL				= 0x8,
	MATERIAL_VIDEO_MODE_NO_WAIT_FOR_VSYNC		= 0x10,

	// Use this for multiple views. When setting the mode,
	// the parent window won't actually get drawn. You
	// need to add views which will get drawn.
	MATERIAL_VIDEO_MODE_PARENT_WINDOW			= 0x20,

	MATERIAL_VIDEO_MODE_ANTIALIAS				= 0x40,
	MATERIAL_VIDEO_MODE_PRELOAD_SHADERS			= 0x80,
	MATERIAL_VIDEO_MODE_QUIT_AFTER_PRELOAD		= 0x100,
};


//-----------------------------------------------------------------------------
// MaterialRect_t struct - used for DrawDebugText
//-----------------------------------------------------------------------------
struct MaterialRect_t
{
    long left;
    long top;
    long right;
    long bottom;
};

//-----------------------------------------------------------------------------
// A function to be called when we need to release all vertex buffers
//-----------------------------------------------------------------------------

typedef void (*MaterialBufferReleaseFunc_t)( );
typedef void (*MaterialBufferRestoreFunc_t)( );

typedef int VertexBufferHandle_t;
typedef unsigned short MaterialHandle_t;

class IMaterialProxyFactory;
class ITexture;
class IMaterialSystemHardwareConfig;

class IMaterialSystem : public IAppSystem
{
public:
	// Call this to set an explicit shader version to use 
	// Must be called before Init().
	virtual void				SetShaderAPI( char const *pShaderAPIDLL ) = 0;

	// Must be called before Init(), if you're going to call it at all...
	virtual void				SetAdapter( int nAdapter, int nFlags ) = 0;

	// Call this to initialize the material system
	// returns a method to create interfaces in the shader dll
	virtual CreateInterfaceFn	Init( char const* pShaderAPIDLL, 
									  IMaterialProxyFactory *pMaterialProxyFactory,
									  CreateInterfaceFn fileSystemFactory,
									  CreateInterfaceFn cvarFactory=NULL ) = 0;

//	virtual void				Shutdown( ) = 0;

	virtual IMaterialSystemHardwareConfig *GetHardwareConfig( const char *pVersion, int *returnCode ) = 0;

	// Gets the number of adapters...
	virtual int					GetDisplayAdapterCount() const = 0;

	// Returns info about each adapter
	virtual void				GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const = 0;

	// Returns the number of modes
	virtual int					GetModeCount( int adapter ) const = 0;

	// Returns mode information..
	virtual void				GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const = 0;

	// Returns the mode info for the current display device
	virtual void				GetDisplayMode( MaterialVideoMode_t& mode ) const = 0;

	// Sets the mode...
	virtual bool				SetMode( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSuperSamples = 0 ) = 0;

	// Creates/ destroys a child window
	virtual bool				AddView( void* hwnd ) = 0;
	virtual void				RemoveView( void* hwnd ) = 0;

	// Sets the view
	virtual void				SetView( void* hwnd ) = 0;

	virtual void				Get3DDriverInfo( Material3DDriverInfo_t& info ) const = 0;
	
	// return true if lightmaps need to be redownloaded
	// Call this before rendering each frame with the current config
	// for the material system.
	// Will do whatever is necessary to get the material system into the correct state
	// upon configuration change. .doesn't much else otherwise.
	virtual bool				UpdateConfig( MaterialSystem_Config_t *config, bool forceUpdate ) = 0;

	// This is the interface for knowing what materials are available
	// is to use the following functions to get a list of materials.  The
	// material names will have the full path to the material, and that is the 
	// only way that the directory structure of the materials will be seen through this
	// interface.
	// NOTE:  This is mostly for worldcraft to get a list of materials to put
	// in the "texture" browser.in Worldcraft
	virtual MaterialHandle_t	FirstMaterial() const = 0;

	// returns InvalidMaterial if there isn't another material.
	// WARNING: you must call GetNextMaterial until it returns NULL, 
	// otherwise there will be a memory leak.
	virtual MaterialHandle_t	NextMaterial( MaterialHandle_t h ) const = 0;

	// This is the invalid material
	virtual MaterialHandle_t	InvalidMaterial() const = 0;

	// Returns a particular material
	virtual IMaterial*			GetMaterial( MaterialHandle_t h ) const = 0;

	// Find a material by name.
	// The name of a material is a full path to 
	// the vmt file starting from "hl2/materials" (or equivalent) without
	// a file extension.
	// eg. "dev/dev_bumptest" refers to somethign similar to:
	// "d:/hl2/hl2/materials/dev/dev_bumptest.vmt"
	virtual IMaterial *			FindMaterial( char const* pMaterialName, bool *pFound, bool complain = true ) = 0;

	virtual ITexture *			FindTexture( char const* pTextureName, bool *pFound, bool complain = true ) = 0;
	virtual void				BindLocalCubemap( ITexture *pTexture ) = 0;
	
	// pass in an ITexture (that is build with "rendertarget" "1") or
	// pass in NULL for the regular backbuffer.
	virtual void				SetRenderTarget( ITexture *pTexture ) = 0;
	virtual ITexture *			GetRenderTarget( void ) = 0;
	
	virtual void				GetRenderTargetDimensions( int &width, int &height) const = 0;
	virtual void				GetBackBufferDimensions( int &width, int &height) const = 0;
	
	// Get the total number of materials in the system.  These aren't just the used
	// materials, but the complete collection.
	virtual int					GetNumMaterials( ) const = 0;

	// Remove any materials from memory that aren't in use as determined
	// by the IMaterial's reference count.
	virtual void				UncacheUnusedMaterials( ) = 0;

	// uncache all materials. .  good for forcing reload of materials.
	virtual void				UncacheAllMaterials( ) = 0;

	// Load any materials into memory that are to be used as determined
	// by the IMaterial's reference count.
	virtual void				CacheUsedMaterials( ) = 0;
	
	// Force all textures to be reloaded from disk.
	virtual void				ReloadTextures( ) = 0;
	
	//
	// lightmap allocation stuff
	//

	// To allocate lightmaps, sort the whole world by material twice.
	// The first time through, call AllocateLightmap for every surface.
	// that has a lightmap.
	// The second time through, call AllocateWhiteLightmap for every 
	// surface that expects to use shaders that expect lightmaps.
	virtual void				BeginLightmapAllocation( ) = 0;
	// returns the sorting id for this surface
	virtual int 				AllocateLightmap( int width, int height, 
		                                          int offsetIntoLightmapPage[2],
												  IMaterial *pMaterial ) = 0;
	// returns the sorting id for this surface
	virtual int					AllocateWhiteLightmap( IMaterial *pMaterial ) = 0;
	virtual void				EndLightmapAllocation( ) = 0;

	// lightmaps are in linear color space
	// lightmapPageID is returned by GetLightmapPageIDForSortID
	// lightmapSize and offsetIntoLightmapPage are returned by AllocateLightmap.
	// You should never call UpdateLightmap for a lightmap allocated through
	// AllocateWhiteLightmap.
	virtual void				UpdateLightmap( int lightmapPageID, int lightmapSize[2],
												int offsetIntoLightmapPage[2], 
												float *pFloatImage, float *pFloatImageBump1,
												float *pFloatImageBump2, float *pFloatImageBump3 ) = 0;
	// Force the lightmaps updated with UpdateLightmap to be sent to the hardware.
	virtual void				FlushLightmaps( ) = 0;

	// fixme: could just be an array of ints for lightmapPageIDs since the material
	// for a surface is already known.
	virtual int					GetNumSortIDs( ) = 0;
//	virtual int					GetLightmapPageIDForSortID( int sortID ) = 0;
	virtual void				GetSortInfo( MaterialSystem_SortInfo_t *sortInfoArray ) = 0;

	virtual void				BeginFrame( ) = 0;
	virtual void				EndFrame( ) = 0;
	
	// Bind a material is current for rendering.
	virtual void				Bind( IMaterial *material, void *proxyData = 0 ) = 0;
	// Bind a lightmap page current for rendering.  You only have to 
	// do this for materials that require lightmaps.
	virtual void				BindLightmapPage( int lightmapPageID ) = 0;

	// Used for WorldCraft lighting preview mode.
	virtual void				SetForceBindWhiteLightmaps( bool bForce ) = 0;
	
	// inputs are between 0 and 1
	virtual void				DepthRange( float zNear, float zFar ) = 0;

	virtual void				ClearBuffers( bool bClearColor, bool bClearDepth ) = 0;

	// read to a unsigned char rgb image.
	virtual void				ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat ) = 0;

	// Sets lighting
	virtual void				SetAmbientLight( float r, float g, float b ) = 0;
	virtual void				SetLight( int lightNum, LightDesc_t& desc ) = 0;

	// The faces of the cube are specified in the same order as cubemap textures
	virtual void				SetAmbientLightCube( Vector4D cube[6] ) = 0;
	
	// Blit the backbuffer to the framebuffer texture
	virtual void				CopyRenderTargetToTexture( ITexture *pTexture ) = 0;
	
	// Set the current texture that is a copy of the framebuffer.
	virtual void				SetFrameBufferCopyTexture( ITexture *pTexture ) = 0;
	virtual ITexture		   *GetFrameBufferCopyTexture( void ) = 0;
	
	// Get the image format of the back buffer. . useful when creating render targets, etc.
	virtual ImageFormat			GetBackBufferFormat() const = 0;
	
	// do we need this?
	virtual void				Flush( bool flushHardware = false ) = 0;

	//
	// end vertex array api
	//

	//
	// Debugging tools
	//
	virtual void				DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose ) = 0;
	virtual void				DebugPrintUsedTextures( void ) = 0;
	virtual void				ToggleSuppressMaterial( char const* pMaterialName ) = 0;
	virtual void				ToggleDebugMaterial( char const* pMaterialName ) = 0;

	//
	// Debugging text output
	//
	virtual void				DrawDebugText( int desiredLeft, int desiredTop, 
											   MaterialRect_t *pActualRect,									   
											   const char *fmt, ... ) = 0;

	// matrix api
	virtual void				MatrixMode( MaterialMatrixMode_t matrixMode ) = 0;
	virtual void				PushMatrix( void ) = 0;
	virtual void				PopMatrix( void ) = 0;
	virtual void				LoadMatrix( float * ) = 0;
	virtual void				MultMatrix( float * ) = 0;
	virtual void				MultMatrixLocal( float * ) = 0;
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, float *dst ) = 0;
	virtual void				LoadIdentity( void ) = 0;
	virtual void				Ortho( double left, double top, double right, double bottom, double zNear, double zFar ) = 0;
	virtual void				PerspectiveX( double fovx, double aspect, double zNear, double zFar ) = 0;
	virtual void				PickMatrix( int x, int y, int width, int height ) = 0;
	virtual void				Rotate( float angle, float x, float y, float z ) = 0;
	virtual void				Translate( float x, float y, float z ) = 0;
	virtual void				Scale( float x, float y, float z ) = 0;
	// end matrix api

	// Sets/gets the viewport
	virtual void				Viewport( int x, int y, int width, int height ) = 0;
	virtual void				GetViewport( int& x, int& y, int& width, int& height ) const = 0;

	// Point size
	virtual void				PointSize( float size ) = 0;

	// The cull mode
	virtual void				CullMode( MaterialCullMode_t cullMode ) = 0;

	// end matrix api

	// This could easily be extended to a general user clip plane
	virtual void				SetHeightClipMode( enum MaterialHeightClipMode_t ) = 0;
	// garymcthack : fog z is always used for heightclipz for now.
	virtual void				SetHeightClipZ( float z ) = 0;
	
	// Fog methods...
	virtual void				FogMode( MaterialFogMode_t fogMode ) = 0;
	virtual void				FogStart( float fStart ) = 0;
	virtual void				FogEnd( float fEnd ) = 0;
	virtual void				SetFogZ( float fogZ ) = 0;
	virtual MaterialFogMode_t	GetFogMode( void ) = 0;

	virtual void				FogColor3f( float r, float g, float b ) = 0;
	virtual void				FogColor3fv( float const* rgb ) = 0;
	virtual void				FogColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void				FogColor3ubv( unsigned char const* rgb ) = 0;

	// Sets the number of bones for skinning
	virtual void				SetNumBoneWeights( int numBones ) = 0;

	virtual IMaterialProxyFactory *GetMaterialProxyFactory() = 0;
	
	// Read the page size of an existing lightmap by sort id (returned from AllocateLightmap())
	virtual void				GetLightmapPageSize( int lightmap, int *width, int *height ) const = 0;
	
	/// FIXME: This stuff needs to be cleaned up and abstracted.
	// Stuff that gets exported to the launcher through the engine
	virtual void				SwapBuffers( ) = 0;

	// Use this to spew information about the 3D layer 
	virtual void				SpewDriverInfo() const = 0;

	// Creates/destroys Mesh
	virtual IMesh* CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh = false ) = 0;
	virtual IMesh* CreateStaticMesh( MaterialVertexFormat_t fmt, bool bSoftwareVertexShader ) = 0;
	virtual void DestroyStaticMesh( IMesh* mesh ) = 0;

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
		IMaterial *pAutoBind = 0 ) = 0;
		
	// Selection mode methods
	virtual int  SelectionMode( bool selectionMode ) = 0;
	virtual void SelectionBuffer( unsigned int* pBuffer, int size ) = 0;
	virtual void ClearSelectionNames( ) = 0;
	virtual void LoadSelectionName( int name ) = 0;
	virtual void PushSelectionName( int name ) = 0;
	virtual void PopSelectionName() = 0;
	
	// Installs a function to be called when we need to release vertex buffers + textures
	virtual void AddReleaseFunc( MaterialBufferReleaseFunc_t func ) = 0;
	virtual void RemoveReleaseFunc( MaterialBufferReleaseFunc_t func ) = 0;

	// Installs a function to be called when we need to restore vertex buffers
	virtual void AddRestoreFunc( MaterialBufferRestoreFunc_t func ) = 0;
	virtual void RemoveRestoreFunc( MaterialBufferRestoreFunc_t func ) = 0;

	// Reloads materials
	virtual void	ReloadMaterials( const char *pSubString = NULL ) = 0;

	virtual void	ResetMaterialLightmapPageInfo() = 0;

	// Methods that use VMatrix
	virtual void		LoadMatrix( VMatrix const& matrix ) = 0;

	// Sets the Clear Color for ClearBuffer....
	virtual void		ClearColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void		ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a ) = 0;

	// Force it to ignore Draw calls.
	virtual void		SetInStubMode( bool bInStubMode ) = 0;
	virtual void		GetMatrix( MaterialMatrixMode_t matrixMode, VMatrix *pMatrix ) = 0;

	// Create new materials
	virtual IMaterial	*CreateMaterial() = 0;

	// Creates a render target
	// If depth == true, a depth buffer is also allocated. If not, then
	// the screen's depth buffer is used.
	virtual ITexture*	CreateRenderTargetTexture( int w, int h, ImageFormat format, bool depth = false ) = 0;

	// Creates a procedural texture
	virtual ITexture *CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags ) = 0;

	// Allows us to override the depth buffer setting of a material
	virtual void	OverrideDepthEnable( bool bEnable, bool bDepthEnable ) = 0;

	// FIXME: This is a hack required for NVidia, can they fix in drivers?
	virtual void	DrawScreenSpaceQuad( IMaterial* pMaterial ) = 0;

	// Release temporary HW memory...
	virtual void	ReleaseTempTextureMemory() = 0;

	// GR - named RT
	virtual ITexture*	CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat format, bool depth = false, bool bClampTexCoords = true, bool bAutoMipMap = false ) = 0;

	// For debugging and building recording files. This will stuff a token into the recording file,
	// then someone doing a playback can watch for the token.
	virtual void	SyncToken( const char *pToken ) = 0;
};

extern IMaterialSystem *materials;

#endif // IMATERIALSYSTEM_H
