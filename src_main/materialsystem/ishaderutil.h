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
// A semi-secret interface provided by the materialsystem to implementations
// of IShaderAPI.
//=============================================================================

#ifndef ISHADERUTIL_H
#define ISHADERUTIL_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class ITexture;
struct MaterialSystem_Config_t;
struct ImageFormatInfo_t;
enum TextureStage_t;


class IShaderUtil
{
public:
	// Method to allow clients access to the MaterialSystem_Config
	virtual MaterialSystem_Config_t& GetConfig() = 0;

	// Allows us to convert image formats
	virtual bool ConvertImageFormat( unsigned char *src, enum ImageFormat srcImageFormat,
									 unsigned char *dst, enum ImageFormat dstImageFormat, 
									 int width, int height, int srcStride = 0, int dstStride = 0 ) = 0;

	// Figures out the amount of memory needed by a bitmap
	virtual int GetMemRequired( int width, int height, ImageFormat format, bool mipmap ) = 0;

	// Gets image format info
	virtual ImageFormatInfo_t const& ImageFormatInfo( ImageFormat fmt) const = 0;

	// Allows us to set the default shadow state
	virtual void SetDefaultShadowState() = 0;

	// Allows us to set the default shader state
	virtual void SetDefaultState( ) = 0;

    // lightmap stuff
	virtual void BindLightmap( TextureStage_t stage ) = 0;
	// GR - bind separate lightmap alpha
	virtual void BindLightmapAlpha( TextureStage_t stage ) = 0;
	virtual void BindBumpLightmap( TextureStage_t stage ) = 0;
	virtual void BindWhite( TextureStage_t stage ) = 0;
	virtual void BindBlack( TextureStage_t stage ) = 0;
	virtual void BindGrey( TextureStage_t stage ) = 0;
	virtual void BindSyncTexture( TextureStage_t stage, int texture ) = 0;
	virtual void BindFBTexture( TextureStage_t stage ) = 0;

	// What are the lightmap dimensions?
	virtual void GetLightmapDimensions( int *w, int *h ) = 0;

	// system flat normal map
	virtual void BindFlatNormalMap( TextureStage_t stage ) = 0;
	virtual void BindNormalizationCubeMap( TextureStage_t stage ) = 0;

	// These methods are called when the shader must eject + restore HW memory
	virtual void ReleaseShaderObjects() = 0;
	virtual void RestoreShaderObjects() = 0;

	// Used to prevent meshes from drawing.
	virtual bool IsInStubMode() = 0;
};

#endif // ISHADERUTIL_H
