//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
// Implements local hooks into named renderable textures.
// See matrendertexture.cpp in material system for list of available RT's
//
//=============================================================================

#include "materialsystem/imesh.h"
#include "materialsystem/ITexture.h"

//=============================================================================
// 
//=============================================================================
static ITexture *s_pPowerOfTwoFrameBufferTexture = NULL;

static void PowerOfTwoFrameBufferTextureRestoreFunc( void )
{
	s_pPowerOfTwoFrameBufferTexture = NULL;
}

ITexture *GetPowerOfTwoFrameBufferTexture( void )
{
	if( !s_pPowerOfTwoFrameBufferTexture )
	{
		bool bFound;
		s_pPowerOfTwoFrameBufferTexture = materials->FindTexture( "_rt_PowerOfTwoFB", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( PowerOfTwoFrameBufferTextureRestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pPowerOfTwoFrameBufferTexture;
}

//=============================================================================
// 
//=============================================================================
static ITexture *s_pCameraTexture = NULL;

static void CameraTextureRestoreFunc( void )
{
	s_pCameraTexture = NULL;
}

ITexture *GetCameraTexture( void )
{
	if( !s_pCameraTexture )
	{
		bool bFound;
		s_pCameraTexture = materials->FindTexture( "_rt_Camera", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( CameraTextureRestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pCameraTexture;
}

//=============================================================================
// 
//=============================================================================
static ITexture *s_pFullFrameFrameBufferTexture = NULL;

static void FullFrameFrameBufferTextureRestoreFunc( void )
{
	if (s_pFullFrameFrameBufferTexture)
	{
		s_pFullFrameFrameBufferTexture = NULL;
	}
}

ITexture *GetFullFrameFrameBufferTexture( void )
{
	if( !s_pFullFrameFrameBufferTexture )
	{
		bool bFound;
		s_pFullFrameFrameBufferTexture = materials->FindTexture( "_rt_FullFrameFB", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( FullFrameFrameBufferTextureRestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pFullFrameFrameBufferTexture;
}

//=============================================================================
// Water reflection
//=============================================================================
static ITexture *s_pWaterReflectionTexture = NULL;

static void WaterReflectionTextureRestoreFunc( void )
{
	s_pWaterReflectionTexture = NULL;
}

ITexture *GetWaterReflectionTexture( void )
{
	if( !s_pWaterReflectionTexture )
	{
		bool bFound;
		s_pWaterReflectionTexture = materials->FindTexture( "_rt_WaterReflection", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( WaterReflectionTextureRestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pWaterReflectionTexture;
}

//=============================================================================
// Water refraction
//=============================================================================
static ITexture *s_pWaterRefractionTexture = NULL;

static void WaterRefractionTextureRestoreFunc( void )
{
	s_pWaterRefractionTexture = NULL;
}

ITexture *GetWaterRefractionTexture( void )
{
	if( !s_pWaterRefractionTexture )
	{
		bool bFound;
		s_pWaterRefractionTexture = materials->FindTexture( "_rt_WaterRefraction", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( WaterRefractionTextureRestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pWaterRefractionTexture;
}


//=============================================================================
// 
//=============================================================================
static ITexture *s_pSmallBufferHDR0 = NULL;

static void SmallBufferHDR0RestoreFunc( void )
{
	s_pSmallBufferHDR0 = NULL;
}

ITexture *GetSmallBufferHDR0( void )
{
	if( !s_pSmallBufferHDR0 )
	{
		bool bFound;
		s_pSmallBufferHDR0 = materials->FindTexture( "_rt_SmallHDR0", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( SmallBufferHDR0RestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pSmallBufferHDR0;
}

//=============================================================================
// 
//=============================================================================
static ITexture *s_pSmallBufferHDR1 = NULL;

static void SmallBufferHDR1RestoreFunc( void )
{
	s_pSmallBufferHDR0 = NULL;
}

ITexture *GetSmallBufferHDR1( void )
{
	if( !s_pSmallBufferHDR1 )
	{
		bool bFound;
		s_pSmallBufferHDR1 = materials->FindTexture( "_rt_SmallHDR1", &bFound );
		Assert( bFound );

		static bool bAddedRestoreFunc = false;
		if( !bAddedRestoreFunc )
		{
			materials->AddRestoreFunc( SmallBufferHDR1RestoreFunc );
			bAddedRestoreFunc = true;
		}
	}
	
	return s_pSmallBufferHDR1;
}

