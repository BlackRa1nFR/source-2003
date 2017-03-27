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
// Code for dealing with vertex shaders
//=============================================================================

#include <windows.h>
#include "VertexShaderDx8.h"
#include "UtlSymbol.h"
#include "UtlVector.h"
#include "UtlDict.h"
#include "locald3dtypes.h"
#include "ShaderAPIDX8_Global.h"
#include "recording.h"
#include "CMaterialSystemStats.h"
#include "tier0/vprof.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "shaderapidx8.h"
#include "materialsystem/ishader.h"
#include "utllinkedlist.h"
#include "materialsystem/ishadersystem.h"


// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//#define NO_AMBIENT_CUBE
#define MAX_BONES 3
#define MAX_LIGHTS 2


//-----------------------------------------------------------------------------
// Inserts the lighting block into the code
//-----------------------------------------------------------------------------

// If you change the number of lighting combinations, change this enum
enum
{
	LIGHTING_COMBINATION_COUNT = 22
};

// NOTE: These should match g_lightType* in vsh_prep.pl!
static int g_LightCombinations[][MAX_LIGHTS+2] = 
{
	// static		ambient				local1				local2

	// This is a special case for no lighting at all.
	{ LIGHT_NONE,	LIGHT_NONE,			LIGHT_NONE,			LIGHT_NONE },			

	// This is a special case so that we don't have to do the ambient cube
	// when we only have static lighting
	{ LIGHT_STATIC,	LIGHT_NONE,			LIGHT_NONE,			LIGHT_NONE },			

	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_NONE,			LIGHT_NONE },			
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_NONE },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_POINT,		LIGHT_NONE },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_DIRECTIONAL,	LIGHT_NONE },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_SPOT },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_POINT, },			
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_DIRECTIONAL, },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_POINT,		LIGHT_POINT, },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_POINT,		LIGHT_DIRECTIONAL, },
	{ LIGHT_NONE,	LIGHT_AMBIENTCUBE,	LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL, },

	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_NONE,			LIGHT_NONE },			
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_NONE },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_POINT,		LIGHT_NONE },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_DIRECTIONAL,	LIGHT_NONE },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_SPOT },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_POINT, },			
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_SPOT,			LIGHT_DIRECTIONAL, },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_POINT,		LIGHT_POINT, },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_POINT,		LIGHT_DIRECTIONAL, },
	{ LIGHT_STATIC,	LIGHT_AMBIENTCUBE,	LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL, },
	
/*
	{ LIGHT_NONE,			LIGHT_NONE },			// 0
	{ LIGHT_SPOT,			LIGHT_NONE },
	{ LIGHT_POINT,			LIGHT_NONE },
	{ LIGHT_DIRECTIONAL,	LIGHT_NONE },
	{ LIGHT_SPOT,			LIGHT_SPOT },

	{ LIGHT_SPOT,			LIGHT_POINT, },			// 5
	{ LIGHT_SPOT,			LIGHT_DIRECTIONAL, },
	{ LIGHT_POINT,			LIGHT_POINT, },
	{ LIGHT_POINT,			LIGHT_DIRECTIONAL, },
	{ LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL, },

	{ LIGHT_STATIC,			LIGHT_NONE },			// 10
*/

/*
	{ LIGHT_SPOT,			LIGHT_SPOT,			LIGHT_SPOT },			// 10
	{ LIGHT_SPOT,			LIGHT_SPOT,			LIGHT_POINT },
	{ LIGHT_SPOT,			LIGHT_SPOT,			LIGHT_DIRECTIONAL },
	{ LIGHT_SPOT,			LIGHT_POINT,		LIGHT_POINT },
	{ LIGHT_SPOT,			LIGHT_POINT,		LIGHT_DIRECTIONAL },

	{ LIGHT_SPOT,			LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL },	// 15
	{ LIGHT_POINT,			LIGHT_POINT,		LIGHT_POINT },
	{ LIGHT_POINT,			LIGHT_POINT,		LIGHT_DIRECTIONAL },
	{ LIGHT_POINT,			LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL },
	{ LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL,	LIGHT_DIRECTIONAL },
*/
};

static const char *GetLightTypeName( VertexShaderLightTypes_t type )
{
	static const char *s_VertexShaderLightTypeNames[] = 
	{
		"LIGHT_NONE",
		"LIGHT_SPOT",
		"LIGHT_POINT",
		"LIGHT_DIRECTIONAL",
		"LIGHT_STATIC",
		"LIGHT_AMBIENTCUBE",
	};
	return s_VertexShaderLightTypeNames[type+1];
}


//-----------------------------------------------------------------------------
// Computes the lighting type from the light types...
//-----------------------------------------------------------------------------
int ComputeLightIndex( int numLights, const VertexShaderLightTypes_t *pLightType, 
					  bool bUseAmbientCube, bool bHasColorMesh )
{
	COMPILE_TIME_ASSERT( LIGHTING_COMBINATION_COUNT == 
		sizeof( g_LightCombinations ) / sizeof( g_LightCombinations[0] ) );
	
	Assert( numLights <= MAX_LIGHTS );

	if( numLights == 0 && !bUseAmbientCube )
	{
		if( bHasColorMesh )
		{
			// special case for static lighting only
			return 1;
		}
		else
		{
			// special case for no lighting at all.
			return 0;
		}
	}
	
	int i;
	// hack - skip the first two for now since we don't know if the ambient cube is needed or not.
	for( i = 2; i < LIGHTING_COMBINATION_COUNT; ++i )
	{
		int j;
		for( j = 0; j < numLights; ++j )
		{
			if( pLightType[j] != g_LightCombinations[i][j+2] )
				break;
		}
		if( j == numLights )
		{
			while( j < MAX_LIGHTS )
			{
				if (g_LightCombinations[i][j+2] != LIGHT_NONE)
					break;
				++j;
			}
			if( j == MAX_LIGHTS )
			{
				if( bHasColorMesh )
				{
					return i + 10;
				}
				else
				{
					return i;
				}
			}
		}
	}

	// should never get here!
	Assert(0);
	return 0;
}


//-----------------------------------------------------------------------------
// Gets the vertex shader index given a number of bones, lights...
// FIXME: Let's try to remove this and make the shaders set the index
//-----------------------------------------------------------------------------
static int ComputeVertexShader( int nBoneCount, int nLightCombo, int nFogType, int fShaderFlags ) 
{
	Assert( nBoneCount <= MAX_BONES );

	// uses some combination of skinning, lighting, and fogging
	int nLightComboCount = LIGHTING_COMBINATION_COUNT;
	if ( ( fShaderFlags & SHADER_USES_LIGHTING ) == 0 )
	{
		nLightComboCount = 1;
		nLightCombo = 0;
	}

	bool bUsesHeightFog = ( nFogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
	int nFogComboCount = 1;
	int nFogCombo = 0;
	if ( fShaderFlags & SHADER_USES_HEIGHT_FOG )
	{
		nFogComboCount = 2;
		nFogCombo = bUsesHeightFog ? 1 : 0;
	}

#ifdef _DEBUG
	if ( bUsesHeightFog && !( fShaderFlags & SHADER_USES_HEIGHT_FOG ) )
	{
		// Doh!  Trying to use height fog with a vertex shader that doesn't support it!
		Assert( 0 );
	}
#endif

	int nSkinningComboCount = 1;
	if ( fShaderFlags & SHADER_USES_SKINNING )
	{
		nSkinningComboCount = MAX_BONES + 1; // + 1 since we have a combo for 0 bones.
	}

	int offset = ( ( ( nBoneCount * nLightComboCount ) + nLightCombo ) * nFogComboCount ) + nFogCombo;
	return offset;
}


//-----------------------------------------------------------------------------
// Shader dictionary
//-----------------------------------------------------------------------------
class CShaderDictionary
{
public:
	// Static methods
	static void CreateErrorPS();
	static void DestroyErrorPS();

	// Adds precompiled shader dictionaries
	void AddShaderDictionary( IPrecompiledShaderDictionary *pDict, bool bPreloadShaders );

	// Creates vertex + pixel shaders
	VertexShader_t CreateVertexShader( const char *pVertexShaderFile, int nStaticVshIndex = 0 );
	PixelShader_t CreatePixelShader( const char *pPixelShaderFile, int nStaticPshIndex = 0 );

	// Destroys vertex shaders
	void DestroyVertexShader( VertexShader_t shader );
	void DestroyAllVertexShaders();

	// Destroys pixel shaders
	void DestroyPixelShader( PixelShader_t shader );
	void DestroyAllPixelShaders();

	// Gets the hardware vertex shader
	HardwareVertexShader_t GetHardwareVertexShader( VertexShader_t shader, int vshIndex );
	HardwarePixelShader_t GetHardwarePixelShader( PixelShader_t shader, int pshIndex );

private:
	// Information associated with each VertexShader_t + PixelShader_t
	struct VertexShaderInfo_t
	{
		HardwareVertexShader_t *m_HardwareShaders;
		unsigned short	m_nRefCount;
		unsigned short	m_nShaderCount;
		unsigned char	m_Flags;
	};
	
	struct PixelShaderInfo_t
	{
		// GR - for binding shader list with pre-compiled DX shaders
		// for later shader creation
		const PrecompiledShader_t *m_pPrecompiledDXShader;
		HardwarePixelShader_t *m_HardwareShaders;
		unsigned short	m_nRefCount;
		unsigned short	m_nShaderCount;
	};

private:
	// Finds a precompiled shader
	const PrecompiledShader_t *LookupPrecompiledShader( PrecompiledShaderType_t type, const char *pShaderName );

	// Name-based lookup of vertex shaders
	int  FindVertexShader( char const* pFileName ) const;

	// Initializes vertex shaders
	bool InitializeVertexShaders( char const* pFileName, VertexShaderInfo_t& info );

	// The low-level dx call to set the vertex shader state
	void SetVertexShaderState( HardwareVertexShader_t shader );


	// Name-based lookup of pixel shaders
	int  FindPixelShader( char const* pFileName ) const;

	// Initializes pixel shaders
	bool InitializePixelShader( const char *pFileName, PixelShaderInfo_t &info );

	// The low-level dx call to set the pixel shader state
	void SetPixelShaderState( HardwarePixelShader_t shader );

private:
	// Precompiled shader dictionary
	CUtlDict< const PrecompiledShader_t*, unsigned short > m_PrecompiledShaders[PRECOMPILED_SHADER_TYPE_COUNT];

	// Used to lookup vertex shaders
	CUtlDict< VertexShaderInfo_t, unsigned short > m_VertexShaders;
	CUtlDict< PixelShaderInfo_t, unsigned short > m_PixelShaders;

	// GR - hack for illegal materials
	static HardwarePixelShader_t s_pIllegalMaterialPS;
};


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
HardwarePixelShader_t CShaderDictionary::s_pIllegalMaterialPS = INVALID_HARDWARE_PIXEL_SHADER;


//-----------------------------------------------------------------------------
// Static methods
//-----------------------------------------------------------------------------
void CShaderDictionary::CreateErrorPS()
{
	// GR - illegal material hack
	if( HardwareConfig()->SupportsVertexAndPixelShaders() )
	{
		// GR - hack for illegal materials
		const DWORD psIllegalMaterial[] =
		{
			0xffff0101, 0x00000051, 0xa00f0000, 0x00000000, 0x3f800000, 0x00000000, 
			0x3f800000, 0x00000001, 0x800f0000, 0xa0e40000, 0x0000ffff
		};

		// create default shader
		D3DDevice()->CreatePixelShader( psIllegalMaterial, &s_pIllegalMaterialPS );
	}
}

void CShaderDictionary::DestroyErrorPS()
{
	// GR - invalid material hack
	// destroy internal shader
	if( s_pIllegalMaterialPS != INVALID_HARDWARE_PIXEL_SHADER )
	{
		s_pIllegalMaterialPS->Release();
		s_pIllegalMaterialPS = INVALID_HARDWARE_PIXEL_SHADER;
	}
}


//-----------------------------------------------------------------------------
// Adds precompiled shader dictionaries
//-----------------------------------------------------------------------------
void CShaderDictionary::AddShaderDictionary( IPrecompiledShaderDictionary *pDict, bool bPreloadShaders )
{
	int nCount = pDict->ShaderCount();
	
	for ( int i = 0; i < nCount; ++i )
	{
		const PrecompiledShader_t *pShader = pDict->GetShader(i);
		m_PrecompiledShaders[pDict->GetType()].Insert( pShader->m_pName, pShader );
	}

	// Create these up front in dx9 since we don't care about the vertex format.
	if ( bPreloadShaders )
	{
		VPROF( "PreloadVertexAndPixelShaders" );

		// Preload shaders
		switch ( pDict->GetType() )
		{
		case PRECOMPILED_VERTEX_SHADER:
			{
				for ( int i = 0; i < nCount; ++i )
				{
					CreateVertexShader( pDict->GetShader(i)->m_pName );
				}
			}
			break;

		case PRECOMPILED_PIXEL_SHADER:
			{
				for ( int i = 0; i < nCount; ++i )
				{
					CreatePixelShader( pDict->GetShader(i)->m_pName );
				}
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
//
// Methods related to vertex shaders
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// The lovely low-level dx call to create a vertex shader
//-----------------------------------------------------------------------------
static HardwareVertexShader_t CreateVertexShader( const PrecompiledShaderByteCode_t& byteCode )
{
	MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_VERTEX_SHADER_CREATES, 1 );
	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_CREATE_VERTEX_SHADER_TIME );

	// Compute the vertex specification
	HardwareVertexShader_t shader;
	HRESULT hr = D3DDevice()->CreateVertexShader( (DWORD*)byteCode.m_pRawData, &shader );

	// NOTE: This isn't recorded before the CreateVertexShader because
	// we don't know the value of shader until after the CreateVertexShader.
	RECORD_COMMAND( DX8_CREATE_VERTEX_SHADER, 3 );
	RECORD_INT( ( int )shader ); // hack hack hack
	RECORD_INT( byteCode.m_nSizeInBytes );
	RECORD_STRUCT( byteCode.m_pRawData, byteCode.m_nSizeInBytes );

	if ( FAILED( hr ) )
	{
		Assert(0);
		shader = INVALID_HARDWARE_VERTEX_SHADER;
	}

	return shader;
}


//-----------------------------------------------------------------------------
// Finds a precompiled shader
//-----------------------------------------------------------------------------
const PrecompiledShader_t *CShaderDictionary::LookupPrecompiledShader( PrecompiledShaderType_t type, const char *pShaderName )
{
	unsigned short i = m_PrecompiledShaders[type].Find( pShaderName );
	if ( i != m_PrecompiledShaders[type].InvalidIndex() )
	{
		return m_PrecompiledShaders[type][i];
	}

	Warning( "shader \"%s\" not found!\n", pShaderName );
	// Whoops! Using a bogus shader
	// FIXME: Should we return an error shader here?
	Assert( 0 );
	return NULL;
}


//-----------------------------------------------------------------------------
// Find duplicate vertex shaders...
//-----------------------------------------------------------------------------
int CShaderDictionary::FindVertexShader( char const* pFileName ) const
{
	return m_VertexShaders.Find( pFileName );
}


//-----------------------------------------------------------------------------
// Initializes vertex shaders
//-----------------------------------------------------------------------------
bool CShaderDictionary::InitializeVertexShaders( const char *pFileName, VertexShaderInfo_t &info )
{
	// Increase the reference count by one so we don't deallocate
	// until the end of the level
	info.m_nRefCount = 1;
	info.m_nShaderCount = 0;
	info.m_HardwareShaders = NULL;

	const PrecompiledShader_t *pShader = LookupPrecompiledShader( PRECOMPILED_VERTEX_SHADER, pFileName );
	if ( !pShader )
		return false;

	info.m_HardwareShaders = new HardwareVertexShader_t[pShader->m_nShaderCount];
	info.m_nShaderCount = pShader->m_nShaderCount;
	info.m_Flags = pShader->m_nFlags;

	for( int j = 0; j < pShader->m_nShaderCount; j++ )
	{
		info.m_HardwareShaders[j] = ::CreateVertexShader( pShader->m_pByteCode[j] );
		if ( info.m_HardwareShaders[j] == INVALID_HARDWARE_VERTEX_SHADER )
		{
			Assert( 0 );
			delete[] info.m_HardwareShaders;
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Creates and destroys vertex shaders
//-----------------------------------------------------------------------------
VertexShader_t CShaderDictionary::CreateVertexShader( char const* pFileName, int nStaticVshIndex )
{
	if ( !pFileName )
		return INVALID_VERTEX_SHADER;

	// Find duplicate vertex shaders...
	int i = FindVertexShader( pFileName );
	if (i == m_VertexShaders.InvalidIndex())
	{
		i = m_VertexShaders.Insert( pFileName );
		if ( !InitializeVertexShaders( pFileName, m_VertexShaders[i] ))
		{
			m_VertexShaders.RemoveAt( i );
			return INVALID_VERTEX_SHADER;
		}
	}

	++m_VertexShaders[i].m_nRefCount;
	return (VertexShader_t)i;
}


//-----------------------------------------------------------------------------
// Destroys a vertex shader.
//-----------------------------------------------------------------------------
void CShaderDictionary::DestroyVertexShader( VertexShader_t shader )
{
	// Can't delete this one...
	if ( (shader == INVALID_VERTEX_SHADER) || ((int)shader >= m_VertexShaders.Count()))
		return;

	if (--m_VertexShaders[shader].m_nRefCount <= 0)
	{
		for (int i = m_VertexShaders[shader].m_nShaderCount; --i >= 0; )
		{
			HardwareVertexShader_t vertexShader = m_VertexShaders[shader].m_HardwareShaders[i];
			if (vertexShader == INVALID_HARDWARE_VERTEX_SHADER)
				continue;

			RECORD_COMMAND( DX8_DESTROY_VERTEX_SHADER, 1 );
			RECORD_INT( ( int )vertexShader );

			vertexShader->Release();
		}

		if ( m_VertexShaders[shader].m_HardwareShaders )
		{
			delete[] m_VertexShaders[shader].m_HardwareShaders;
		}

		m_VertexShaders.RemoveAt( shader );
	}
}


//-----------------------------------------------------------------------------
// Destroys all vertex shaders.
//-----------------------------------------------------------------------------
void CShaderDictionary::DestroyAllVertexShaders( )
{
	for (int i = m_VertexShaders.First(); i != m_VertexShaders.InvalidIndex(); i = m_VertexShaders.Next(i) )
	{
		for (int j = m_VertexShaders[i].m_nShaderCount; --j >= 0; )
		{
			HardwareVertexShader_t vertexShader = m_VertexShaders[i].m_HardwareShaders[j];
			if (vertexShader != INVALID_HARDWARE_VERTEX_SHADER)
			{
				RECORD_COMMAND( DX8_DESTROY_VERTEX_SHADER, 1 );
				RECORD_INT( ( int )vertexShader );
				vertexShader->Release();
			}
		}

		if ( m_VertexShaders[i].m_HardwareShaders )
		{
			delete[] m_VertexShaders[i].m_HardwareShaders;
		}
	}
	m_VertexShaders.RemoveAll();
}


//-----------------------------------------------------------------------------
// Gets the hardware vertex shader
//-----------------------------------------------------------------------------
HardwareVertexShader_t CShaderDictionary::GetHardwareVertexShader( VertexShader_t shader, int vshIndex )
{
	if ( vshIndex == -1 )
	{
		int nLightCombo = ShaderAPI()->GetCurrentLightCombo();
		int nBoneCount = ShaderAPI()->GetCurrentNumBones();
		int nFogType = ShaderAPI()->GetCurrentFogType();

		// use the old method of figuring out the vertex shader index using
		// numbones, light combo, and fog mode.
		vshIndex = ComputeVertexShader( nBoneCount,	nLightCombo, nFogType, m_VertexShaders[shader].m_Flags ); 
	}

	Assert( vshIndex >= 0 && vshIndex < m_VertexShaders[shader].m_nShaderCount );
	return m_VertexShaders[shader].m_HardwareShaders[vshIndex];
}


//-----------------------------------------------------------------------------
//
// Methods related to pixel shaders
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Find duplicate pixel shaders...
//-----------------------------------------------------------------------------
int CShaderDictionary::FindPixelShader( char const* pFileName ) const
{
	// Find the shader...
	return m_PixelShaders.Find( pFileName );
}

static void PatchPixelShaderForAtiMsaaHack(DWORD *pShader, DWORD dwTexCoordMask) 
{ 
	bool bIsSampler, bIsTexCoord; 
	
	// Should be able to patch only ps2.0 
	if (*pShader != 0xFFFF0200) 
		return; 
	
	pShader++; 
	
	while (pShader) 
	{ 
		switch (*pShader & D3DSI_OPCODE_MASK) 
		{ 
		case D3DSIO_COMMENT: 
			// Process comment 
			pShader = pShader + (*pShader >> 16) + 1; 
			break; 
			
		case D3DSIO_END: 
			// End of shader 
			return; 
			
		case D3DSIO_DCL: 
			bIsSampler = (*(pShader + 1) & D3DSP_TEXTURETYPE_MASK) != D3DSTT_UNKNOWN; 
			bIsTexCoord = (((*(pShader + 2) & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) + 
				((*(pShader + 2) & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2)) == D3DSPR_TEXTURE; 
			
			if (!bIsSampler && bIsTexCoord) 
			{ 
				DWORD dwTexCoord = *(pShader + 2) & D3DSP_REGNUM_MASK; 
				DWORD mask = 0x01; 
				for (DWORD i = 0; i < 16; i++) 
				{ 
					if (((dwTexCoordMask & mask) == mask) && (dwTexCoord == i)) 
					{ 
						// If found -- patch and get out 
						*(pShader + 2) |= D3DSPDM_PARTIALPRECISION; 
						break; 
					} 
					mask <<= 1; 
				} 
			} 
			// Intentionally fall through... 
			
		default: 
			// Skip instruction 
			pShader = pShader + ((*pShader & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT) + 1; 
		} 
	} 
} 

//-----------------------------------------------------------------------------
// The lovely low-level dx call to create a pixel shader
//-----------------------------------------------------------------------------
static HardwarePixelShader_t CreatePixelShader( PrecompiledShaderByteCode_t& byteCode, 
											    unsigned int nCentroidMask )
{
	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_CREATE_PIXEL_SHADER_TIME );

	if( nCentroidMask && HardwareConfig()->NeedsATICentroidHack() )
	{
		PatchPixelShaderForAtiMsaaHack( ( DWORD * )byteCode.m_pRawData, nCentroidMask );
	}
	
	HardwarePixelShader_t shader;
	HRESULT hr = D3DDevice()->CreatePixelShader( (DWORD*)byteCode.m_pRawData, &shader );

	// NOTE: We have to do this after creating the pixel shader since we don't know
	// lookup.m_PixelShader yet!!!!!!!
	RECORD_COMMAND( DX8_CREATE_PIXEL_SHADER, 3 );
	RECORD_INT( ( int )shader );  // hack hack hack
	RECORD_INT( byteCode.m_nSizeInBytes );
	RECORD_STRUCT( byteCode.m_pRawData, byteCode.m_nSizeInBytes );
	
	if ( FAILED( hr ) )
	{
		Assert(0);
		shader = INVALID_HARDWARE_PIXEL_SHADER;
	}

	return shader;
}


//-----------------------------------------------------------------------------
// Initializes pixel shaders
//-----------------------------------------------------------------------------
bool CShaderDictionary::InitializePixelShader( const char *pFileName, PixelShaderInfo_t &info )
{
	// FIXME: do we need to do this for pixel shaders?  We do this for vertex shaders.
	// Increase the reference count by one so we don't deallocate
	// until the end of the level
	info.m_nRefCount = 1;
	info.m_nShaderCount = 0;
	info.m_HardwareShaders = NULL;
	info.m_pPrecompiledDXShader = NULL;

	const PrecompiledShader_t *pShader = LookupPrecompiledShader( PRECOMPILED_PIXEL_SHADER, pFileName );
	if ( !pShader )
		return false;

	// Cache off the precompiled shaders + create them on-demand
	info.m_pPrecompiledDXShader = pShader;
	info.m_nShaderCount = pShader->m_nShaderCount;
	info.m_HardwareShaders = new HardwarePixelShader_t[ pShader->m_nShaderCount ];

	// Initialize these to 0 to force them to be cached later
	for( int j = 0; j < pShader->m_nShaderCount; j++ )
	{
		info.m_HardwareShaders[j] = INVALID_HARDWARE_PIXEL_SHADER;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Creates and destroys pixel shaders
//-----------------------------------------------------------------------------
PixelShader_t CShaderDictionary::CreatePixelShader( char const* pFileName, int nStaticPshIndex )
{
	// FIXME: NOTE: We could also implement this by returning a unique PixelShader_t
	// for each pFileName/nStaticPshIndex combination. I didn't do this because
	// even though it's cleaner from the API standpoint because it would mean a
	// much larger m_PixelShader dictionary + resulting slower search times.

	if ( !pFileName )
		return INVALID_PIXEL_SHADER;

	// Find duplicate pixel shaders...
	int i = FindPixelShader( pFileName );
	if (i == m_PixelShaders.InvalidIndex())
	{	
		i = m_PixelShaders.Insert( pFileName );
		if ( !InitializePixelShader( pFileName, m_PixelShaders[i] ) )
		{
			m_PixelShaders.RemoveAt( i );
			return INVALID_PIXEL_SHADER;
		}
	}

	PixelShaderInfo_t &info	= m_PixelShaders[i];

	// FIXME: At the moment, pixel shaders don't know about dynamic psh indices
	// which means they only need to precache one thing here. When we introduce
	// dynamic pixel shader indices, we'll need to precache *all* dynamic pshs
	// associated with the static psh here.

	// Make sure the psh index is valid.
	if ( nStaticPshIndex >= info.m_pPrecompiledDXShader->m_nShaderCount )
		return INVALID_PIXEL_SHADER;

	++info.m_nRefCount;

	int nDynamicPshIndex;
	for( nDynamicPshIndex = 0; nDynamicPshIndex < info.m_pPrecompiledDXShader->m_nDynamicCombos; nDynamicPshIndex++ )
	{
		HardwarePixelShader_t &shader = info.m_HardwareShaders[nStaticPshIndex+nDynamicPshIndex];
		if ( shader == INVALID_HARDWARE_PIXEL_SHADER )
		{
			// Precache the pixel shader
			PrecompiledShaderByteCode_t &byteCode = info.m_pPrecompiledDXShader->m_pByteCode[nStaticPshIndex+nDynamicPshIndex];

			// Make sure the data is valid
			if ( !( byteCode.m_nSizeInBytes == 4 && 
					( ( unsigned int * )byteCode.m_pRawData )[0] == 0x00000000 ) )
			{
				shader = ::CreatePixelShader( byteCode, info.m_pPrecompiledDXShader->m_nCentroidMask );
			}

			// Check again
			if ( shader == INVALID_HARDWARE_PIXEL_SHADER )
			{
				// Set shader to ugly green for invalid shader...
				shader = s_pIllegalMaterialPS;
				Assert( 0 );
			}
		}
	}

	return (PixelShader_t)i;
}


//-----------------------------------------------------------------------------
// Destroys a pixel shader
//-----------------------------------------------------------------------------
void CShaderDictionary::DestroyPixelShader( PixelShader_t shader )
{
	// Can't delete this one...
	if ( (shader == INVALID_PIXEL_SHADER) || ((int)shader >= m_PixelShaders.Count()) )
		return;

	if (--m_PixelShaders[shader].m_nRefCount <= 0)
	{
		for (int i = m_PixelShaders[shader].m_nShaderCount; --i >= 0; )
		{
			HardwarePixelShader_t pixelShader = m_PixelShaders[shader].m_HardwareShaders[i];
			if (pixelShader == INVALID_HARDWARE_PIXEL_SHADER)
				continue;

			// GR - invalid material hack
			// don't delete shared shader used for hack
			if ( pixelShader != s_pIllegalMaterialPS )
			{
				RECORD_COMMAND( DX8_DESTROY_PIXEL_SHADER, 1 );
				RECORD_INT( ( int )pixelShader );
				
				pixelShader->Release();
			}
		}

		if ( m_PixelShaders[shader].m_HardwareShaders )
		{
			delete[] m_PixelShaders[shader].m_HardwareShaders;
		}

		m_PixelShaders.RemoveAt( shader );
	}
}


//-----------------------------------------------------------------------------
// Destroys all pixel shaders
//-----------------------------------------------------------------------------
void CShaderDictionary::DestroyAllPixelShaders( )
{
	for (int i = m_PixelShaders.First(); i != m_PixelShaders.InvalidIndex(); i = m_PixelShaders.Next(i) )
	{
		for (int j = m_PixelShaders[i].m_nShaderCount; --j >= 0; )
		{
			HardwarePixelShader_t pixelShader = m_PixelShaders[i].m_HardwareShaders[j];
			if ( (pixelShader == INVALID_HARDWARE_PIXEL_SHADER) || ( pixelShader == s_pIllegalMaterialPS ))
				continue;

			RECORD_COMMAND( DX8_DESTROY_PIXEL_SHADER, 1 );
			RECORD_INT( ( int )pixelShader );
			pixelShader->Release();
		}

		if ( m_PixelShaders[i].m_HardwareShaders )
		{
			delete[] m_PixelShaders[i].m_HardwareShaders;
		}
	}
	m_PixelShaders.RemoveAll();
}


//-----------------------------------------------------------------------------
// Gets the hardware pixel shader
//-----------------------------------------------------------------------------
HardwarePixelShader_t CShaderDictionary::GetHardwarePixelShader( PixelShader_t shader, int pshIndex )
{
	return m_PixelShaders[shader].m_HardwareShaders[pshIndex];	
}


//-----------------------------------------------------------------------------
// Vertex + pixel shader manager
//-----------------------------------------------------------------------------
class CShaderManager : public IShaderManager
{
public:
	CShaderManager();
	~CShaderManager();

	// Methods of IShaderManager
	virtual void Init( bool bPreloadShaders );
	virtual void Shutdown();
	virtual VertexShader_t CreateVertexShader( const char *pVertexShaderFile, int nStaticVshIndex = 0 );
	virtual PixelShader_t CreatePixelShader( const char *pPixelShaderFile, int nStaticPshIndex = 0 );
	virtual void SetVertexShaderIndex( int vshIndex = -1 );
	virtual void SetPixelShaderIndex( int pshIndex = 0 );
	virtual void SetVertexShader( VertexShader_t shader, int nStaticVshIndex = 0 );
	virtual void SetPixelShader( PixelShader_t shader, int nStaticPshIndex = 0 );
	virtual void* GetCurrentVertexShader();
	virtual void* GetCurrentPixelShader();
	virtual void ResetShaderState();
	virtual ShaderDLL_t AddShaderDLL( );
	virtual void RemoveShaderDLL( ShaderDLL_t hShaderDLL );
	virtual void AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict );
	virtual void SetShaderDLL( ShaderDLL_t hShaderDLL );

private:
	// The low-level dx call to set the vertex shader state
	void SetVertexShaderState( HardwareVertexShader_t shader );

	// The low-level dx call to set the pixel shader state
	void SetPixelShaderState( HardwarePixelShader_t shader );

private:
	// Shaders defined in external DLLs
	CUtlLinkedList< CShaderDictionary > m_ShaderDLLs;
	ShaderDLL_t m_hCurrentShaderDLL;

	// The current vertex + pixel shader
	HardwareVertexShader_t	m_HardwareVertexShader;
	HardwarePixelShader_t	m_HardwarePixelShader;

	// The current vertex + pixel shader index
	int m_nVertexShaderIndex;
	int m_nPixelShaderIndex;

	// Should we preload shaders?
	bool m_bPreloadShaders;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
static CShaderManager s_ShaderManager;
IShaderManager *g_pShaderManager = &s_ShaderManager;


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CShaderManager::CShaderManager()
{
	m_hCurrentShaderDLL = m_ShaderDLLs.InvalidIndex();
}


CShaderManager::~CShaderManager()
{
}


//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
void CShaderManager::Init( bool bPreloadShaders )
{
	m_bPreloadShaders = bPreloadShaders;
	CShaderDictionary::CreateErrorPS();
}

void CShaderManager::Shutdown()
{
	for ( unsigned short i = m_ShaderDLLs.Head(); i != m_ShaderDLLs.InvalidIndex(); i = m_ShaderDLLs.Next(i) )
	{
		RemoveShaderDLL( i );
	}
	CShaderDictionary::DestroyErrorPS();
}


//-----------------------------------------------------------------------------
// Adds/removes shader DLLs
//-----------------------------------------------------------------------------
ShaderDLL_t CShaderManager::AddShaderDLL( )
{
	return m_ShaderDLLs.AddToTail();
}

void CShaderManager::RemoveShaderDLL( ShaderDLL_t hShaderDLL )
{
	// Destroy all vertex shaders
	m_ShaderDLLs[hShaderDLL].DestroyAllVertexShaders( );

	// Destroy all pixel shaders
	m_ShaderDLLs[hShaderDLL].DestroyAllPixelShaders( );

	m_ShaderDLLs.Remove( hShaderDLL );
}


//-----------------------------------------------------------------------------
// Adds precompiled shader dictionaries
//-----------------------------------------------------------------------------
void CShaderManager::AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict )
{
	Assert( m_ShaderDLLs.IsValidIndex( hShaderDLL ) );
	m_ShaderDLLs[hShaderDLL].AddShaderDictionary( pDict, m_bPreloadShaders );
}


//-----------------------------------------------------------------------------
// Sets the shader DLL index
//-----------------------------------------------------------------------------
void CShaderManager::SetShaderDLL( ShaderDLL_t hShaderDLL )
{
	m_hCurrentShaderDLL = (hShaderDLL >= 0) ? hShaderDLL : m_ShaderDLLs.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Creates and destroys vertex shaders
//-----------------------------------------------------------------------------
VertexShader_t CShaderManager::CreateVertexShader( char const* pFileName, int nStaticVshIndex )
{
	if ( !pFileName || (m_hCurrentShaderDLL == m_ShaderDLLs.InvalidIndex()) )
		return INVALID_VERTEX_SHADER;

	VertexShader_t shader = m_ShaderDLLs[m_hCurrentShaderDLL].CreateVertexShader( pFileName, nStaticVshIndex );
	if ( shader != INVALID_VERTEX_SHADER )
	{
		shader |= (m_hCurrentShaderDLL << 16);
	}
	return shader;
}


//-----------------------------------------------------------------------------
// Creates and destroys pixel shaders
//-----------------------------------------------------------------------------
PixelShader_t CShaderManager::CreatePixelShader( char const* pFileName, int nStaticPshIndex )
{
	// FIXME: NOTE: We could also implement this by returning a unique PixelShader_t
	// for each pFileName/nStaticPshIndex combination. I didn't do this because
	// even though it's cleaner from the API standpoint because it would mean a
	// much larger m_PixelShader dictionary + resulting slower search times.

	if ( !pFileName || (m_hCurrentShaderDLL == m_ShaderDLLs.InvalidIndex()) )
		return INVALID_PIXEL_SHADER;

	PixelShader_t shader = m_ShaderDLLs[m_hCurrentShaderDLL].CreatePixelShader( pFileName, nStaticPshIndex );
	if ( shader != INVALID_PIXEL_SHADER )
	{
		shader |= (m_hCurrentShaderDLL << 16);
	}
	return shader;
}


//-----------------------------------------------------------------------------
//
// Methods related to setting vertex + pixel shader state
//
//-----------------------------------------------------------------------------
void CShaderManager::SetVertexShaderIndex( int vshIndex )
{
	m_nVertexShaderIndex = vshIndex;
}

void CShaderManager::SetPixelShaderIndex( int pshIndex )
{
	m_nPixelShaderIndex = pshIndex;
}

void* CShaderManager::GetCurrentVertexShader()
{
	return m_HardwareVertexShader;
}

void* CShaderManager::GetCurrentPixelShader()
{
	return m_HardwarePixelShader;
}


//-----------------------------------------------------------------------------
// The low-level dx call to set the vertex shader state
//-----------------------------------------------------------------------------
void CShaderManager::SetVertexShaderState( HardwareVertexShader_t shader )
{
	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_SET_VERTEX_SHADER);
	
	if ( m_HardwareVertexShader != shader )
	{
		RECORD_COMMAND( DX8_SET_VERTEX_SHADER, 1 );
		RECORD_INT( ( int )shader ); // hack hack hack

#ifdef _DEBUG
		HRESULT hr = 
#endif
				D3DDevice()->SetVertexShader( shader );
		Assert( hr == D3D_OK );

		m_HardwareVertexShader = shader;
		MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Sets a particular vertex shader as the current shader
//-----------------------------------------------------------------------------
void CShaderManager::SetVertexShader( VertexShader_t shader, int nStaticVshIndex )
{
	// Determine which vertex shader to use...
	if( shader == INVALID_VERTEX_SHADER )
	{
		SetVertexShaderState( 0 );
		return;
	}

	int hShaderDLL = shader >> 16;
	shader &= 0xFFFF;

	int vshIndex = (m_nVertexShaderIndex >= 0) ? m_nVertexShaderIndex + nStaticVshIndex : -1;
	HardwareVertexShader_t dxshader = m_ShaderDLLs[hShaderDLL].GetHardwareVertexShader( shader, vshIndex );
	SetVertexShaderState( dxshader );
}


//-----------------------------------------------------------------------------
// The low-level dx call to set the pixel shader state
//-----------------------------------------------------------------------------
void CShaderManager::SetPixelShaderState( HardwarePixelShader_t shader )
{
	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_SET_PIXEL_SHADER);

	if ( m_HardwarePixelShader != shader )
	{
		RECORD_COMMAND( DX8_SET_PIXEL_SHADER, 1 );
		RECORD_INT( ( int )shader );
		
#ifdef _DEBUG
		HRESULT hr = 
#endif
				D3DDevice()->SetPixelShader( shader );
		Assert( !FAILED(hr) );

		m_HardwarePixelShader = shader;
		MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );
	}
}


//-----------------------------------------------------------------------------
// Sets a particular pixel shader as the current shader
//-----------------------------------------------------------------------------
void CShaderManager::SetPixelShader( PixelShader_t shader, int nStaticPshIndex )
{
	if( shader == INVALID_PIXEL_SHADER )
	{
		SetPixelShaderState( 0 );
		return;
	}

	int hShaderDLL = shader >> 16;
	shader &= 0xFFFF;

	int pshIndex = m_nPixelShaderIndex + nStaticPshIndex;
	Assert( pshIndex >= 0 );
	HardwarePixelShader_t hardwarePixelShader =	m_ShaderDLLs[hShaderDLL].GetHardwarePixelShader( shader, pshIndex );
	Assert( hardwarePixelShader != INVALID_HARDWARE_PIXEL_SHADER );
	SetPixelShaderState( hardwarePixelShader );
}


//-----------------------------------------------------------------------------
// Resets the shader state
//-----------------------------------------------------------------------------
void CShaderManager::ResetShaderState()
{
	// This will force the calls to SetVertexShader + SetPixelShader to actually set the state
	m_HardwareVertexShader = (HardwareVertexShader_t)-1;
	m_HardwarePixelShader = (HardwarePixelShader_t)-1;

	SetVertexShader( INVALID_VERTEX_SHADER );
	SetPixelShader( INVALID_PIXEL_SHADER );
}