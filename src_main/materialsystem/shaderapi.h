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
// The main shader API interface
//=============================================================================

#ifndef SHADERAPI_H
#define SHADERAPI_H

#ifdef _WIN32
#pragma once
#endif

#include "vector4d.h"
#include "materialsystem/ishaderapi.h"
#include "materialsystem/imaterial.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IShaderUtil;
class IBaseFileSystem;
class CPixelWriter;
class CMeshBuilder;
struct MaterialAdapterInfo_t;
struct Material3DDriverInfo_t;
struct MaterialVideoMode_t;
class IMesh;
enum MaterialCullMode_t;
struct MaterialRect_t;
class IPrecompiledShaderDictionary;


//-----------------------------------------------------------------------------
// This must match the definition in playback.cpp!
//-----------------------------------------------------------------------------
enum ShaderRenderTarget_t
{
	SHADER_RENDERTARGET_BACKBUFFER = 0xffffffff,
	SHADER_RENDERTARGET_DEPTHBUFFER = 0xffffffff,
};


//-----------------------------------------------------------------------------
// The state snapshot handle
//-----------------------------------------------------------------------------
typedef short StateSnapshot_t;


//-----------------------------------------------------------------------------
// The state snapshot handle
//-----------------------------------------------------------------------------
typedef int ShaderDLL_t;


//-----------------------------------------------------------------------------
// This is naughty of me, but I'm sneaking a flag into the vertex format
//-----------------------------------------------------------------------------
enum
{
	VERTEX_PREPROCESS_DATA = 0x80000000
};


//-----------------------------------------------------------------------------
// Texture creation
//-----------------------------------------------------------------------------
enum CreateTextureFlags_t
{
	TEXTURE_CREATE_CUBEMAP		= 0x1,
	TEXTURE_CREATE_RENDERTARGET = 0x2,
	TEXTURE_CREATE_MANAGED		= 0x4,
	TEXTURE_CREATE_DEPTHBUFFER	= 0x8,
	TEXTURE_CREATE_DYNAMIC		= 0x10,
	TEXTURE_CREATE_AUTOMIPMAP   = 0x20,
};


//-----------------------------------------------------------------------------
// This is what the material system gets to see.
//-----------------------------------------------------------------------------
class IShaderAPI : public IShaderDynamicAPI
{
public:
	// Initialization, shutdown
	virtual bool Init( IShaderUtil* pShaderUtil, IBaseFileSystem* pFileSystem, int nAdapter, int nFlags ) = 0;
	virtual void Shutdown( ) = 0;

	// Gets the number of adapters...
	virtual int	 GetDisplayAdapterCount() const = 0;

	// Returns info about each adapter
	virtual void GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const = 0;

	// Returns the number of modes
	virtual int	 GetModeCount( int adapter ) const = 0;

	// Returns mode information..
	virtual void GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const = 0;

	// Returns the mode info for the current display device
	virtual void GetDisplayMode( MaterialVideoMode_t& info ) const = 0;

	// Sets the mode...
	virtual bool SetMode( void* hwnd, const MaterialVideoMode_t &mode, int flags, int nSuperSamples = 0 ) = 0;

	virtual void GetWindowSize( int &width, int &height ) const = 0;

	// Get info about the 3d driver.
	virtual void Get3DDriverInfo( Material3DDriverInfo_t &info ) const = 0;
	
	// Creates/ destroys a child window
	virtual bool AddView( void* hwnd ) = 0;
	virtual void RemoveView( void* hwnd ) = 0;

	// Activates a view
	virtual void SetView( void* hwnd ) = 0;

	// Returns the snapshot id for the shader state
	virtual StateSnapshot_t	 TakeSnapshot( ) = 0;

	virtual void TexMinFilter( ShaderTexFilterMode_t texFilterMode ) = 0;
	virtual void TexMagFilter( ShaderTexFilterMode_t texFilterMode ) = 0;
	virtual void TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode ) = 0;

	virtual void CopyRenderTargetToTexture( int texID ) = 0;
	
	// Binds a particular material to render with
	virtual void Bind( IMaterial* pMaterial ) = 0;

	// Flushes any primitives that are buffered
	virtual void FlushBufferedPrimitives() = 0;

	// Creates/destroys Mesh
	virtual IMesh* CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh ) = 0;
	virtual IMesh* CreateStaticMesh( VertexFormat_t vertexFormat, bool bSoftwareVertexShader ) = 0;
	virtual void DestroyStaticMesh( IMesh* mesh ) = 0;

	// Gets the dynamic mesh; note that you've got to render the mesh
	// before calling this function a second time. Clients should *not*
	// call DestroyStaticMesh on the mesh returned by this call.
	virtual IMesh* GetDynamicMesh( IMaterial* pMaterial, bool buffered = true,
		IMesh* pVertexOverride = 0, IMesh* pIndexOverride = 0) = 0;

	// Page flip
	virtual void SwapBuffers( ) = 0;

	// Methods to ask about particular state snapshots
	virtual bool IsTranslucent( StateSnapshot_t id ) const = 0;
	virtual bool IsAlphaTested( StateSnapshot_t id ) const = 0;
	virtual bool UsesVertexAndPixelShaders( StateSnapshot_t id ) const = 0;

	// returns true if the state uses an ambient cube
	virtual bool UsesAmbientCube( StateSnapshot_t id ) const = 0;

	// Gets the vertex format for a set of snapshot ids
	virtual VertexFormat_t ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const = 0;

	// What fields in the vertex do we actually use?
	virtual VertexFormat_t ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const = 0;

	// Begins a rendering pass
	virtual void BeginPass( StateSnapshot_t snapshot ) = 0;

	// Renders a single pass of a material
	virtual void RenderPass( ) = 0;

	// Set the number of bone weights
	virtual void SetNumBoneWeights( int numBones ) = 0;

	// Sets the lights
	virtual void SetLight( int lightNum, LightDesc_t& desc ) = 0;
	virtual void SetAmbientLight( float r, float g, float b ) = 0;
	virtual void SetAmbientLightCube( Vector4D cube[6] ) = 0;

	// The shade mode
	virtual void ShadeMode( ShaderShadeMode_t mode ) = 0;

	// The cull mode
	virtual void CullMode( MaterialCullMode_t cullMode ) = 0;

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
	virtual void ForceDepthFuncEquals( bool bEnable ) = 0;

	// Forces Z buffering to be on or off
	virtual void OverrideDepthEnable( bool bEnable, bool bDepthEnable ) = 0;

	virtual void SetHeightClipZ( float z ) = 0;
	virtual void SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode ) = 0;

	virtual void SetClipPlane( int index, const float *pPlane ) = 0;
	virtual void EnableClipPlane( int index, bool bEnable ) = 0;
	
	// Put all the model matrices into vertex shader constants.
	virtual void SetSkinningMatrices() = 0;

	// Viewport methods
	virtual void Viewport( int x, int y, int width, int height ) = 0;
	virtual void GetViewport( int& x, int& y, int& width, int& height ) const = 0;

	// Point size...
	virtual void PointSize( float size ) = 0;
	
	// Returns the nearest supported format
	virtual ImageFormat GetNearestSupportedFormat( ImageFormat fmt ) const = 0;
	virtual ImageFormat GetNearestRenderTargetFormat( ImageFormat fmt ) const = 0;

	// This is only used for statistics gathering..
	virtual void SetLightmapTextureIdRange( int firstId, int lastId ) = 0;

	// Texture management methods
	virtual void CreateTexture( unsigned int textureNum, int width, int height,
		ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags = 0 ) = 0;
	virtual void DeleteTexture( unsigned int textureNum ) = 0;
	virtual void CreateDepthTexture( unsigned int textureNum, ImageFormat renderTargetFormat, int width, int height ) = 0;
	virtual bool IsTexture( unsigned int textureNum ) = 0;
	virtual bool IsTextureResident( unsigned int textureNum ) = 0;

	// Indicates we're going to be modifying this texture
	// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
	// all use the texture specified by this function.
	virtual void ModifyTexture( unsigned int textureNum ) = 0;

	virtual void TexImage2D( int level, int cubeFaceID, ImageFormat dstFormat, int width, int height, 
							 ImageFormat srcFormat, void *imageData ) = 0;
	virtual void TexSubImage2D( int level, int cubeFaceID, int xOffset, int yOffset, int width, int height,
							 ImageFormat srcFormat, int srcStride, void *imageData ) = 0;
	
	// An alternate (and faster) way of writing image data
	// (locks the current Modify Texture). Use the pixel writer to write the data
	// after Lock is called
	// Doesn't work for compressed textures 
	virtual bool TexLock( int level, int cubeFaceID, int xOffset, int yOffset, 
		int width, int height, CPixelWriter& writer ) = 0;
	virtual void TexUnlock( ) = 0;

	// These are bound to the texture
	virtual void TexSetPriority( int priority ) = 0;

	// Sets the texture state
	virtual void BindTexture( TextureStage_t stage, unsigned int textureNum ) = 0;

	// Set the render target to a texID.
	// Set to SHADER_RENDERTARGET_BACKBUFFER if you want to use the regular framebuffer.
	// Set to SHADER_RENDERTARGET_DEPTHBUFFER if you want to use the regular z buffer.
	virtual void SetRenderTarget( unsigned int colorTexture = SHADER_RENDERTARGET_BACKBUFFER, 
		unsigned int depthTexture = SHADER_RENDERTARGET_DEPTHBUFFER ) = 0;
	
	virtual ImageFormat GetBackBufferFormat() const = 0;
	virtual void GetBackBufferDimensions( int& width, int& height ) const = 0;
	
	// stuff that isn't to be used from within a shader
	virtual void DepthRange( float zNear, float zFar ) = 0;
	virtual void ClearBuffers( bool bClearColor, bool bClearDepth, int renderTargetWidth, int renderTargetHeight ) = 0;
	virtual void ClearColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a ) = 0;
	virtual void ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat ) = 0;

	virtual void FlushHardware() = 0;

	// Use this to spew information about the 3D layer 
	virtual void SpewDriverInfo() const = 0;

	// Use this to begin and end the frame
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	// Selection mode methods
	virtual int  SelectionMode( bool selectionMode ) = 0;
	virtual void SelectionBuffer( unsigned int* pBuffer, int size ) = 0;
	virtual void ClearSelectionNames( ) = 0;
	virtual void LoadSelectionName( int name ) = 0;
	virtual void PushSelectionName( int name ) = 0;
	virtual void PopSelectionName() = 0;

	// Force the hardware to finish whatever it's doing
	virtual void ForceHardwareSync() = 0;

	// Used to clear the transition table when we know it's become invalid.
	virtual void ClearSnapshots() = 0;

	// Returns true if the material system should try to use 16 bit textures.
	virtual bool Prefer16BitTextures( ) const = 0;

	// Debugging text output
	virtual void DrawDebugText( int desiredLeft, int desiredTop, 
		                        MaterialRect_t *pActualRect, const char *pText ) = 0;
	virtual void FogStart( float fStart ) = 0;
	virtual void FogEnd( float fEnd ) = 0;
	virtual void SetFogZ( float fogZ ) = 0;	
	virtual float GetFogStart( void ) const = 0;
	virtual float GetFogEnd( void ) const = 0;

	// Can we download textures?
	virtual bool CanDownloadTextures() const = 0;

	// Are we using graphics?
	virtual bool IsUsingGraphics() const = 0;

	virtual void ResetRenderState() = 0;

	virtual void DestroyVertexBuffers() = 0;

	virtual void EvictManagedResources() = 0;

	// Scene fog state.
	virtual void SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void SceneFogMode( MaterialFogMode_t fogMode ) = 0;

	// Adds/removes precompiled shader dictionaries
	virtual ShaderDLL_t AddShaderDLL( ) = 0;
	virtual void RemoveShaderDLL( ShaderDLL_t hShaderDLL ) = 0;
	virtual void AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict ) = 0;
	virtual void SetShaderDLL( ShaderDLL_t hShaderDLL ) = 0;

	// Level of anisotropic filtering
	virtual void SetAnisotropicLevel( int nAnisotropyLevel ) = 0;

	// For debugging and building recording files. This will stuff a token into the recording file,
	// then someone doing a playback can watch for the token.
	virtual void	SyncToken( const char *pToken ) = 0;
};



#endif // SHADERAPI_H
