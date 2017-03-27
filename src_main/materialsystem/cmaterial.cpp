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
// Implementation of a material
//=============================================================================

 
#include "imaterialinternal.h"
#include "tgaloader.h"
#include "colorspace.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include <string.h>
#include "materialsystem_global.h"
#include "shaderapi.h"
#include "materialsystem/imaterialproxy.h"							   
#include "shadersystem.h"
#include "materialsystem/imaterialproxyfactory.h"
#include "IHardwareConfigInternal.h"
#include "utlsymbol.h"
#include <malloc.h>
#include "filesystem.h"
#include <KeyValues.h>
#include "mempool.h"
#include "ishaderutil.h"
#include "vtf/vtf.h"
#include "vstdlib/strtools.h"
#include <ctype.h>
#include "utlbuffer.h"
#include "vmatrix.h"
#include "texturemanager.h"
#include "itextureinternal.h"
#include "mempool.h"


//-----------------------------------------------------------------------------
// Material implementation
//-----------------------------------------------------------------------------
class CMaterial : public IMaterialInternal
{
public:
	// Members of the IMaterial interface
	char const *GetName() const;
	PreviewImageRetVal_t GetPreviewImageProperties( int *width, int *height, 
				 				ImageFormat *imageFormat, bool* isTranslucent ) const;
	PreviewImageRetVal_t GetPreviewImage( unsigned char *data, int width, int height,
								ImageFormat imageFormat ) const;
	int			GetMappingWidth( );
	int			GetMappingHeight( );
	int			GetNumAnimationFrames( );
	void		IncrementReferenceCount( );
	void		DecrementReferenceCount( );
	int 		GetEnumerationID( ) const;
	void		GetLowResColorSample( float s, float t, float *color ) const;
	
	IMaterialVar *		FindVar( char const *varName, bool *found, bool complain = true );

	bool				UsesEnvCubemap( void );
	bool				NeedsSoftwareSkinning( void );
	virtual bool		NeedsSoftwareLighting( void );
	bool				NeedsTangentSpace( void );
	bool				NeedsFrameBufferTexture( void );

	// GR - Is lightmap alpha needed?
	bool	NeedsLightmapBlendAlpha( void );
	
	void	AlphaModulate( float alpha );
	void	ColorModulate( float r, float g, float b );

	void	SetMaterialVarFlag( MaterialVarFlags_t flag, bool on );
	bool	GetMaterialVarFlag( MaterialVarFlags_t flag ) const;

	bool	IsTranslucent();
	bool	IsAlphaTested();
	bool	IsVertexLit();

	void	GetReflectivity( Vector& reflect );
	bool	GetPropertyFlag( MaterialPropertyTypes_t type );

	// Is the material visible from both sides?
	bool	IsTwoSided();

	int		GetNumPasses( void );
	int		GetTextureMemoryBytes( void );

public:
	// stuff that is visible only from within the material system

	// constructor, destructor
				CMaterial( char const* materialName, bool bCreatedFromFile );
	virtual		~CMaterial();

	void		DrawMesh( IMesh* pMesh );
	int			GetReferenceCount( ) const;
	void		Uncache( bool bForceUncacheVars = false );
	void		Precache();
	bool		PrecacheVars();
	void		SetMinLightmapPageID( int pageID );
	void		SetMaxLightmapPageID( int pageID );
	int			GetMinLightmapPageID( ) const;
	int			GetMaxLightmapPageID( ) const;
	void		SetNeedsWhiteLightmap( bool val );
	bool		GetNeedsWhiteLightmap( ) const;
	bool		IsPrecached( ) const;
	bool		IsPrecachedVars( ) const;
	IShader *	GetShader() const;
	void		SetEnumerationID( int id );
	void		CallBindProxy( void *proxyData );
	bool		HasProxy( void ) const;

	// Sets the shader associated with the material
	void SetShader( const char *pShaderName );

	// returns various vertex formats
	VertexFormat_t GetVertexFormat( MaterialVertexFormat_t vfmt ) const;

	bool WillPreprocessData( void ) const;

	// Can we override this material in debug?
	bool NoDebugOverride() const;

	// Gets the vertex format
	VertexFormat_t GetVertexFormat() const;
	
	// diffuse bump lightmap?
	bool IsUsingDiffuseBumpedLighting() const;

	// lightmap?
	bool IsUsingLightmap() const;

	// Gets the vertex usage flags
	int	GetVertexUsage() const;

	// Debugs this material
	bool PerformDebugTrace() const;

	// Are we suppressed?
	bool IsSuppressed() const;

	// Do we use fog?
	bool UseFog( void ) const;
	
	// Should we draw?
	void ToggleSuppression();
	void ToggleDebugTrace();
	
	// Refresh material based on current var values
	void Refresh();

	// This computes the state snapshots for this material
	void RecomputeStateSnapshots();

	// Get the software vertex shader from this material's shader if it is necessary.
	const SoftwareVertexShader_t GetSoftwareVertexShader() const;

	// returns true if there is a software vertex shader.  Software vertex shaders are
	// different from "WillPreprocessData" in that all the work is done on the verts
	// before calling the shader instead of between passes.  Software vertex shaders are
	// preferred when possible.
	bool UsesSoftwareVertexShader( void ) const;

	// Gets at the shader parameters
	virtual int ShaderParamCount() const;
	virtual IMaterialVar **GetShaderParams( void );

	virtual void AddMaterialVar( IMaterialVar *pMaterialVar );

private:
	// Initializes, cleans up the shader params
	void CleanUpShaderParams();

	// Sets up an error shader when we run into problems.
	void SetupErrorShader();

	// Parses a .VMT file and generates a key value list
	bool LoadVMTFile( KeyValues& vmtKeyValues );
	
	// Prints material flags.
	void PrintMaterialFlags( int flags, int flagsDefined );

	// Parses material flags
	bool ParseMaterialFlag( KeyValues* pParseValue, IMaterialVar* pFlagVar,
		IMaterialVar* pFlagDefinedVar, bool parsingOverrides, int& flagMask, int& overrideMask );

	// Computes the material vars for the shader
	int ParseMaterialVars( IShader* pShader, KeyValues& keyValues, 
		KeyValues* pOverride, bool modelDefault, IMaterialVar** ppVars );

	// Figures out the preview image for worldcraft
	char const*	GetPreviewImageName( );
	char const* GetPreviewImageFileName( void ) const;

	// Hooks up the shader, returns keyvalues of fallback that was used
	KeyValues* InitializeShader( KeyValues& keyValues );

	// Finds the flag associated with a particular flag name
	int FindMaterialVarFlag( char const* pFlagName ) const;

	// Initializes, cleans up the state snapshots
	bool InitializeStateSnapshots();
	void CleanUpStateSnapshots();

	// Initializes, cleans up the material proxy
	void InitializeMaterialProxy( KeyValues* pFallbackKeyValues );
	void CleanUpMaterialProxy();

	// Grabs the texture width and height from the var list for faster access
	void PrecacheMappingDimensions( );

	// Gets the renderstate
	virtual ShaderRenderState_t *GetRenderState();

	// Do we have a valid renderstate?
	bool IsValidRenderState() const;
	bool IsManuallyCreated() const;

	// Get the material var flags
	int	GetMaterialVarFlags() const;
	void SetMaterialVarFlags( int flags, bool on );
	int	GetMaterialVarFlags2() const;
	void SetMaterialVarFlags2( int flags, bool on );

	// Returns a dummy material variable
	IMaterialVar*	GetDummyVariable();

	IMaterialVar*	GetShaderParam( int id );

	void			FindRepresentativeTexture( void );

	// Fixed-size allocator
	DECLARE_FIXEDSIZE_ALLOCATOR( CMaterial );

private:
	enum
	{
		MATERIAL_NEEDS_WHITE_LIGHTMAP = 0x1,
		MATERIAL_IS_PRECACHED = 0x2,
		MATERIAL_VARS_IS_PRECACHED = 0x4,
		MATERIAL_VALID_RENDERSTATE = 0x8,
		MATERIAL_IS_MANUALLY_CREATED = 0x10,
	};

	int m_iEnumerationID;
	
	int m_minLightmapPageID;
	int m_maxLightmapPageID;

	unsigned short m_MappingWidth;
	unsigned short m_MappingHeight;
	
	IShader *m_pShader;
	CUtlSymbol m_Name;
	unsigned short	m_RefCount;
	unsigned char	m_VarCount;
	unsigned char	m_ProxyCount;
	unsigned char	m_Flags;
	IMaterialVar**		m_pShaderParams;
	IMaterialProxy**	m_ppProxies;
	ShaderRenderState_t m_ShaderRenderState;

	// this is the texture that is used for it's palette entries.
	// UNDONE: should this also be used for reflectance?
	// would take care of 90% of the situations so that we don't have
	// to write special code for each shader to get reflectance.
	ITextureInternal *m_representativeTexture;

#ifdef _DEBUG
	// Makes it easier to see what's going on
	char*	m_pDebugName;
#endif
};


// NOTE: This must be the last file included
// Has to exist *after* fixed size allocator declaration
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// class factory methods
//-----------------------------------------------------------------------------
DEFINE_FIXEDSIZE_ALLOCATOR( CMaterial, 256, true );

IMaterialInternal* IMaterialInternal::Create( char const* pMaterialName, bool bCreatedFromFile )
{
	return new CMaterial( pMaterialName, bCreatedFromFile );
}

void IMaterialInternal::Destroy( IMaterialInternal* pMaterial )
{
	if (pMaterial)
	{
		CMaterial* pMatImp = static_cast<CMaterial*>(pMaterial);
		delete pMatImp;
	}
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CMaterial::CMaterial( char const* materialName, bool bCreatedFromFile )
{
	int len = strlen(materialName);
	char* pTemp = (char*)_alloca( len + 1 );
	strcpy( pTemp, materialName );
	Q_strlower( pTemp );

	// Strip off the extension
	pTemp[ len - 4 ] = '\0';

	// Convert it to a symbol
	m_Name = pTemp;

#ifdef _DEBUG
	m_pDebugName = new char[strlen(pTemp) + 1];
	strcpy( m_pDebugName, pTemp );
#endif

	m_Flags = 0;
	m_pShader = NULL;
	m_pShaderParams = NULL;
	m_RefCount = 0;
	m_representativeTexture = NULL;
	m_ppProxies = NULL;
	m_ProxyCount = 0;
	m_VarCount = 0;
	m_MappingWidth = m_MappingHeight = 0;
	m_iEnumerationID = 0;
	m_minLightmapPageID = m_maxLightmapPageID = 0;

	if (!bCreatedFromFile)
	{
		m_Flags |= MATERIAL_IS_MANUALLY_CREATED; 
	}

	// Initialize the renderstate to something indicating nothing should be drawn
	m_ShaderRenderState.m_Flags = 0;
	m_ShaderRenderState.m_VertexFormat = m_ShaderRenderState.m_VertexUsage = 0;
	for ( int i = 0; i < 4; ++i )
	{
		m_ShaderRenderState.m_Snapshots[i].m_nPassCount = 0;
	}
}

CMaterial::~CMaterial()
{
	Uncache( true );
	if( m_RefCount != 0 )
	{
		Warning( "Reference Count for Material %s (%d) != 0\n", GetName(), m_RefCount );
	}

#ifdef _DEBUG
	if (m_pDebugName)
		delete[] m_pDebugName;
#endif
}


//-----------------------------------------------------------------------------
// Gets the renderstate
//-----------------------------------------------------------------------------
ShaderRenderState_t *CMaterial::GetRenderState()
{
	Precache();
	return &m_ShaderRenderState;
}


//-----------------------------------------------------------------------------
// Returns a dummy material variable
//-----------------------------------------------------------------------------
IMaterialVar* CMaterial::GetDummyVariable()
{
	static IMaterialVar* pDummyVar = 0;
	if (!pDummyVar)
		pDummyVar = IMaterialVar::Create( 0, "$dummyVar", 0 );

	return pDummyVar;
}


//-----------------------------------------------------------------------------
// Are vars precached?
//-----------------------------------------------------------------------------
bool CMaterial::IsPrecachedVars( ) const
{
	return (m_Flags & MATERIAL_VARS_IS_PRECACHED) != 0;
}


//-----------------------------------------------------------------------------
// Are we precached?
//-----------------------------------------------------------------------------
bool CMaterial::IsPrecached( ) const
{
	return (m_Flags & MATERIAL_IS_PRECACHED) != 0;
}


//-----------------------------------------------------------------------------
// Cleans up shader parameters
//-----------------------------------------------------------------------------
void CMaterial::CleanUpShaderParams()
{
	if( m_pShaderParams )
	{
		for (int i = 0; i < m_VarCount; ++i)
		{
			IMaterialVar::Destroy( m_pShaderParams[i] );
		}

		free( m_pShaderParams );
		m_pShaderParams = 0;
	}
	m_VarCount = 0;
}


//-----------------------------------------------------------------------------
// Initializes the material proxy
//-----------------------------------------------------------------------------
void CMaterial::InitializeMaterialProxy( KeyValues* pFallbackKeyValues )
{
	IMaterialProxyFactory *pMaterialProxyFactory;
	pMaterialProxyFactory = MaterialSystem()->GetMaterialProxyFactory();	
	if( !pMaterialProxyFactory )
		return;

	// See if we've got a proxy section; obey fallbacks
	KeyValues* pProxySection = pFallbackKeyValues->FindKey("Proxies");
	if (!pProxySection)
		return;

	// Iterate through the section + create all of the proxies
	int proxyCount = 0;
	IMaterialProxy* ppProxies[256];
	KeyValues* pProxyKey = pProxySection->GetFirstSubKey();
	for ( ; pProxyKey; pProxyKey = pProxyKey->GetNextKey() )
	{
		// Each of the proxies should themselves be databases
		IMaterialProxy* pProxy = pMaterialProxyFactory->CreateProxy( pProxyKey->GetName() );
		if (!pProxy)
		{
			Warning( "Error: Material \"%s\" : proxy \"%s\" not found!\n", GetName(), pProxyKey->GetName() );
			continue;
		}

		if (!pProxy->Init( this, pProxyKey ))
		{
			pMaterialProxyFactory->DeleteProxy( pProxy );
			Warning( "Error: Material \"%s\" : proxy \"%s\" unable to initialize!\n", GetName(), pProxyKey->GetName() );
		}
		else
		{
			ppProxies[proxyCount] = pProxy;
			++proxyCount;
		}
	}

	// Allocate space for the number of proxies we successfully made...
	m_ProxyCount = proxyCount;
	if (proxyCount)
	{
		m_ppProxies = (IMaterialProxy**)malloc( proxyCount * sizeof(IMaterialProxy*) );
		memcpy( m_ppProxies, ppProxies, proxyCount * sizeof(IMaterialProxy*) );
	}
	else
	{
		m_ppProxies = 0;
	}
}


//-----------------------------------------------------------------------------
// Cleans up the material proxy
//-----------------------------------------------------------------------------
void CMaterial::CleanUpMaterialProxy()
{
	IMaterialProxyFactory *pMaterialProxyFactory;
	pMaterialProxyFactory = MaterialSystem()->GetMaterialProxyFactory();	
	if( !pMaterialProxyFactory )
		return;

	// Clean up material proxies
	for ( int i = m_ProxyCount; --i >= 0; )
	{
		pMaterialProxyFactory->DeleteProxy( m_ppProxies[i] );
	}

	m_ProxyCount = 0;
	if (m_ppProxies)
	{
		free(m_ppProxies);
		m_ppProxies = 0;
	}
}


//-----------------------------------------------------------------------------
// Finds the index of the material var associated with a var
//-----------------------------------------------------------------------------
static int FindMaterialVar( IShader* pShader, char const* pVarName )
{
	for (int i = pShader->GetNumParams(); --i >= 0; )
	{
		if (!stricmp( pShader->GetParamName(i), pVarName ))
			return i;
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Parser utilities
//-----------------------------------------------------------------------------
static inline bool IsWhitespace( char c )
{
	return c == ' ' || c == '\t';
}

static inline bool IsEndline( char c )
{
	return c == '\n' || c == '\0';
}

static inline bool IsVector( char const* v )
{
	while (IsWhitespace(*v))
	{
		++v;
		if (IsEndline(*v))
			return false;
	}
	return *v == '[' || *v == '{';
}


//-----------------------------------------------------------------------------
// Creates a vector material var
//-----------------------------------------------------------------------------
static IMaterialVar* CreateVectorMaterialVarFromKeyValue( IMaterial* pMaterial, KeyValues* pKeyValue )
{
	float vecVal[4];
	char const* pScan = pKeyValue->GetString();
	bool divideBy255 = false;

	// skip whitespace
	while( IsWhitespace(*pScan) )
	{
		++pScan;
	}

	if( *pScan == '{' )
	{
		divideBy255 = true;
	}
	else
	{
		Assert( *pScan == '[' );
	}
	
	// skip the '['
	++pScan;
	int i;
	for( i = 0; i < 4; i++ )
	{
		// skip whitespace
		while( IsWhitespace(*pScan) )
		{
			++pScan;
		}

		if( IsEndline(*pScan) || *pScan == ']' || *pScan == '}' )
		{
			if (*pScan != ']' && *pScan != '}')
			{
				Warning( "Warning in .VMT file (%s): no ']' or '}' found in vector key \"%s\".\n"
					"Did you forget to surround the vector with \"s?\n", pMaterial->GetName(), pKeyValue->GetName() );
			}

			// allow for vec2's, etc.
			vecVal[i] = 0.0f;
			break;
		}

		char* pEnd;

		vecVal[i] = strtod( pScan, &pEnd );
		if (pScan == pEnd)
		{
			Warning( "Error in .VMT file: error parsing vector element \"%s\" in \"%s\"\n", pKeyValue->GetName(), pMaterial->GetName() );
			return 0;
		}

		pScan = pEnd;
	}

	if( divideBy255 )
	{
		vecVal[0] *= ( 1.0f / 255.0f );
		vecVal[1] *= ( 1.0f / 255.0f );
		vecVal[2] *= ( 1.0f / 255.0f );
		vecVal[3] *= ( 1.0f / 255.0f );
	}
	
	// Create the variable!
	return IMaterialVar::Create( pMaterial, pKeyValue->GetName(), vecVal, i );
}


//-----------------------------------------------------------------------------
// Creates a vector material var
//-----------------------------------------------------------------------------
static IMaterialVar* CreateMatrixMaterialVarFromKeyValue( IMaterial* pMaterial, KeyValues* pKeyValue )
{
	char const* pScan = pKeyValue->GetString();

	// Matrices can be specified one of two ways:
	// [ # # # #  # # # #  # # # #  # # # # ]
	// or
	// center # # scale # # rotate # translate # #

	VMatrix mat;
	int count = sscanf( pScan, " [ %f %f %f %f  %f %f %f %f  %f %f %f %f  %f %f %f %f ]",
		&mat.m[0][0], &mat.m[0][1], &mat.m[0][2], &mat.m[0][3],
		&mat.m[1][0], &mat.m[1][1], &mat.m[1][2], &mat.m[1][3],
		&mat.m[2][0], &mat.m[2][1], &mat.m[2][2], &mat.m[2][3],
		&mat.m[3][0], &mat.m[3][1], &mat.m[3][2], &mat.m[3][3] );
	if (count == 16)
	{
		return IMaterialVar::Create( pMaterial, pKeyValue->GetName(), mat );
	}

	Vector2D scale, center;
	float angle;
	Vector2D translation;
	count = sscanf( pScan, " center %f %f scale %f %f rotate %f translate %f %f",
		&center.x, &center.y, &scale.x, &scale.y, &angle, &translation.x, &translation.y );
	if (count != 7)
		return NULL;

	VMatrix temp;
	MatrixBuildTranslation( mat, -center.x, -center.y, 0.0f );
	MatrixBuildScale( temp, scale.x, scale.y, 1.0f );
	MatrixMultiply( temp, mat, mat );
	MatrixBuildRotateZ( temp, angle );
	MatrixMultiply( temp, mat, mat );
	MatrixBuildTranslation( temp, center.x + translation.x, center.y + translation.y, 0.0f );
	MatrixMultiply( temp, mat, mat );

	// Create the variable!
	return IMaterialVar::Create( pMaterial, pKeyValue->GetName(), mat );
}


//-----------------------------------------------------------------------------
// Creates a material var from a key value
//-----------------------------------------------------------------------------

static IMaterialVar* CreateMaterialVarFromKeyValue( IMaterial* pMaterial, KeyValues* pKeyValue )
{
	switch( pKeyValue->GetDataType() )
	{
	case KeyValues::TYPE_INT:
		return IMaterialVar::Create( pMaterial, pKeyValue->GetName(), pKeyValue->GetInt() );

	case KeyValues::TYPE_FLOAT:
		return IMaterialVar::Create( pMaterial, pKeyValue->GetName(), pKeyValue->GetFloat() );

	case KeyValues::TYPE_STRING:
		{
			char const* pString = pKeyValue->GetString();
			if (!pString || !pString[0])
				return 0;

			// Look for matrices
			IMaterialVar *pMatrixVar = CreateMatrixMaterialVarFromKeyValue( pMaterial, pKeyValue );
			if (pMatrixVar)
				return pMatrixVar;

			// Look for vectors
			if (!IsVector(pString))
				return IMaterialVar::Create( pMaterial, pKeyValue->GetName(), pString );

			// Parse the string as a vector...
			return CreateVectorMaterialVarFromKeyValue( pMaterial, pKeyValue );
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Reads out common flags, prevents them from becoming material vars
//-----------------------------------------------------------------------------
int CMaterial::FindMaterialVarFlag( char const* pFlagName ) const
{
	for( int i = 0; *ShaderSystem()->ShaderStateString(i); ++i )
	{
		if (!stricmp( pFlagName, ShaderSystem()->ShaderStateString(i) ))
			return (1 << i);
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Print material flags
//-----------------------------------------------------------------------------
void CMaterial::PrintMaterialFlags( int flags, int flagsDefined )
{
	int i;
	for( i = 0; *ShaderSystem()->ShaderStateString(i); i++ )
	{
		if( flags & ( 1<<i ) )
		{
			Warning( "%s|", ShaderSystem()->ShaderStateString(i) );
		}
	}
	Warning( "\n" );
}


//-----------------------------------------------------------------------------
// Parses material flags
//-----------------------------------------------------------------------------
bool CMaterial::ParseMaterialFlag( KeyValues* pParseValue, IMaterialVar* pFlagVar,
	IMaterialVar* pFlagDefinedVar, bool parsingOverrides, int& flagMask, int& overrideMask )
{
	// See if the var is a flag...
	int flagbit = FindMaterialVarFlag( pParseValue->GetName() );
	if (!flagbit)
		return false;

	// Allow for flag override
	int testMask = parsingOverrides ? overrideMask : flagMask;
	if (testMask & flagbit)
	{
		Warning("Error! Flag \"%s\" is multiply defined in material \"%s\"!\n", pParseValue->GetName(), GetName() );
		return true;
	}

	// Make sure overrides win
	if (overrideMask & flagbit)
		return true;

	if (parsingOverrides)
		overrideMask |= flagbit;
	else
		flagMask |= flagbit;

	// If so, then set the flag bit
	if (pParseValue->GetInt())
		pFlagVar->SetIntValue( pFlagVar->GetIntValue() | flagbit );
	else
		pFlagVar->SetIntValue( pFlagVar->GetIntValue() & (~flagbit) );

	// Mark the flag as being defined
	pFlagDefinedVar->SetIntValue( pFlagDefinedVar->GetIntValue() | flagbit );

/*
	if( stristr( m_pDebugName, "glasswindow064a" ) )
	{
		Warning( "flags\n" );
		PrintMaterialFlags( pFlagVar->GetIntValue(), pFlagDefinedVar->GetIntValue() );
	}
*/
	
	return true;
}


//-----------------------------------------------------------------------------
// Computes the material vars for the shader
//-----------------------------------------------------------------------------
int CMaterial::ParseMaterialVars( IShader* pShader, KeyValues& keyValues, 
			KeyValues* pOverrideKeyValues, bool modelDefault, IMaterialVar** ppVars )
{
	IMaterialVar* pNewVar;
	bool pOverride[256];
	int overrideMask = 0;
	int flagMask = 0;
	int varIdx;
	memset( ppVars, 0, 256 * sizeof(IMaterialVar*) );
	memset( pOverride, 0, 256 * sizeof(bool) );

	// Create the flag var...
	// Set model mode if we fell back from a model mode shader
	int modelFlag = modelDefault ? MATERIAL_VAR_MODEL : 0;
	ppVars[FLAGS] = IMaterialVar::Create( this, "$flags", modelFlag );
	ppVars[FLAGS_DEFINED] = IMaterialVar::Create( this, "$flags_defined", modelFlag );
	ppVars[FLAGS2] = IMaterialVar::Create( this, "$flags2", modelFlag );
	ppVars[FLAGS_DEFINED2] = IMaterialVar::Create( this, "$flags_defined2", modelFlag );

	int numParams = pShader->GetNumParams();
	int varCount = numParams;

	bool parsingOverrides = (pOverrideKeyValues != 0);
	KeyValues* pVar = pOverrideKeyValues ? pOverrideKeyValues->GetFirstSubKey() : keyValues.GetFirstSubKey();
	while( pVar )
	{
		// Variables preceeded by a '%' are used by the tools only
		if ((pVar->GetName()[0] == '%') && (g_pShaderAPI->IsUsingGraphics()) && (!g_config.bEditMode) )
			goto nextVar;

		// See if the var is a flag...
		if (ParseMaterialFlag( pVar, ppVars[FLAGS], ppVars[FLAGS_DEFINED], parsingOverrides, flagMask, overrideMask ))
			goto nextVar;
		if (ParseMaterialFlag( pVar, ppVars[FLAGS2], ppVars[FLAGS_DEFINED2], parsingOverrides, flagMask, overrideMask ))
			goto nextVar;

		// See if the var is one of the shader params
		varIdx = FindMaterialVar( pShader, pVar->GetName() );

		// Check for multiply defined or overridden
		if ((varIdx >= 0) && (ppVars[varIdx]))
		{
			if (ppVars[varIdx])
			{
				if ( !pOverride[varIdx] || parsingOverrides )
				{
					Warning("Error! Variable \"%s\" is multiply defined in material \"%s\"!\n", pVar->GetName(), GetName() );
				}
				goto nextVar;
			}
			else
			{
				int i;
				for ( i = numParams; i < varCount; ++i)
				{
					Assert( ppVars[i] );
					if (!stricmp( ppVars[i]->GetName(), pVar->GetName() ))
						break;
				}
				if (i != varCount)
				{
					if ( !pOverride[i] || parsingOverrides )
					{
						Warning("Error! Variable \"%s\" is multiply defined in material \"%s\"!\n", pVar->GetName(), GetName() );
					}
					goto nextVar;
				}
			}
		}

		// Create a material var for this dudely dude; could be zero...
		pNewVar = CreateMaterialVarFromKeyValue( this, pVar );
		if (!pNewVar) 
			goto nextVar;

		if (varIdx < 0)
		{
			varIdx = varCount++;
		}
		ppVars[varIdx] = pNewVar;
		if (parsingOverrides)
			pOverride[varIdx] = true;

nextVar:
		pVar = pVar->GetNextKey();
		if (!pVar && parsingOverrides)
		{
			pVar = keyValues.GetFirstSubKey();
			parsingOverrides = false;
		}
	}

	// Create undefined vars for all the actual material vars
	for (int i = 0; i < numParams; ++i)
	{
		if (!ppVars[i])
			ppVars[i] = IMaterialVar::Create( this, pShader->GetParamName(i) );
	}

	return varCount;
}


//-----------------------------------------------------------------------------
// Hooks up the shader
//-----------------------------------------------------------------------------
KeyValues* CMaterial::InitializeShader( KeyValues& keyValues )
{
	KeyValues* pCurrentFallback = &keyValues;
	KeyValues* pFallbackSection = 0;

	// I'm not quite sure how this can happen, but we'll see... 
	char const* pShaderName = pCurrentFallback->GetName();
	if (!pShaderName)
	{
		Warning("Shader not specified in material %s\nUsing wireframe instead...\n", GetName());
		pShaderName = "Wireframe";
	}

	IShader* pShader;
	IMaterialVar*	ppVars[256];
	int varCount = 0;
	bool modelDefault = false;

	// Keep going until there's no more fallbacks...
	while( true )
	{
		// Find the shader for this material. Note that this may not be
		// the actual shader we use due to fallbacks...
		pShader = ShaderSystem()->FindShader( pShaderName );
		if ( !pShader )
		{
			Warning( "Error: Material \"%s\" uses unknown shader \"%s\"\n", GetName(), pShaderName );
			pShaderName = "wireframe";
			pShader = ShaderSystem()->FindShader( pShaderName );
			Assert( pShader );
		}

		// Here we must set up all flags + material vars that the shader needs
		// because it may look at them when choosing shader fallback.
		varCount = ParseMaterialVars( pShader, keyValues, pFallbackSection, modelDefault, ppVars );

		// Make sure we set default values before the fallback is looked for
		ShaderSystem()->InitShaderParameters( pShader, ppVars, GetName() );

		// Now that the material vars are parsed, see if there's a fallback
		// But only if we're not in the tools
/*
		if (!g_pShaderAPI->IsUsingGraphics())
			break;
*/

		// Check for a fallback; if not, we're done
		pShaderName = pShader->GetFallbackShader( ppVars );
		if (!pShaderName)
			break;

		// Remember the model flag if we're on dx7 or higher...
		if (HardwareConfig()->SupportsVertexAndPixelShaders() ||
			HardwareConfig()->SupportsHardwareLighting())
		{
			modelDefault = ( ppVars[FLAGS]->GetIntValue() & MATERIAL_VAR_MODEL ) != 0;
		}

		// Try to get the section associated with the fallback shader
		// Then chain it to the base data so it can override the 
		// values if it wants to
		pFallbackSection = keyValues.FindKey( pShaderName );
		if (pFallbackSection)
		{
			pFallbackSection->ChainKeyValue( &keyValues );
			pCurrentFallback = pFallbackSection;
		}

		// Now, blow away all of the material vars + try again...
		for (int i = 0; i < varCount; ++i)
		{
			Assert( ppVars[i] );
			IMaterialVar::Destroy( ppVars[i] );
		}
	}

	// Store off the shader
	m_pShader = pShader;

	// Store off the material vars + flags
	m_VarCount = varCount;
	m_pShaderParams = (IMaterialVar**)malloc( varCount * sizeof(IMaterialVar*) );
	memcpy( m_pShaderParams, ppVars, varCount * sizeof(IMaterialVar*) );

#ifdef _DEBUG
	for (int i = 0; i < varCount; ++i)
	{
		Assert( ppVars[i] );
	}
#endif

	return pCurrentFallback;
}

//-----------------------------------------------------------------------------
// Gets the texturemap size
//-----------------------------------------------------------------------------

void CMaterial::PrecacheMappingDimensions( )
{
	// Cache mapping width and mapping height
	if (!m_representativeTexture)
	{
#ifdef PARANOID
		Warning( "No representative texture on material: \"%s\"\n", GetName() );
#endif
		m_MappingWidth = 64;
		m_MappingHeight = 64;
	}
	else
	{
		m_MappingWidth = m_representativeTexture->GetMappingWidth();
		m_MappingHeight = m_representativeTexture->GetMappingHeight();
	}
}


//-----------------------------------------------------------------------------
// Initialize the state snapshot
//-----------------------------------------------------------------------------
bool CMaterial::InitializeStateSnapshots()
{
	if (IsPrecached())
	{
		// Default state
		CleanUpStateSnapshots();

		if (!ShaderSystem()->InitRenderState( m_pShader, m_VarCount, m_pShaderParams, &m_ShaderRenderState, GetName() ))
			return false;

		m_Flags |= MATERIAL_VALID_RENDERSTATE; 
	}

	return true;
}

void CMaterial::CleanUpStateSnapshots()
{
	if (IsValidRenderState())
	{
		ShaderSystem()->CleanupRenderState(&m_ShaderRenderState);
		m_Flags &= ~MATERIAL_VALID_RENDERSTATE;
	}
}


//-----------------------------------------------------------------------------
// This sets up a debugging/error shader...
//-----------------------------------------------------------------------------
void CMaterial::SetupErrorShader()
{
	// Preserve the model flags
	Assert( m_pShaderParams[FLAGS] );
	int flags = (m_pShaderParams[FLAGS]->GetIntValue() & MATERIAL_VAR_MODEL);

	CleanUpShaderParams();
	CleanUpMaterialProxy();

	// We had a failure; replace it with a valid shader...
	m_pShader = ShaderSystem()->FindShader( "wireframe" );
	Assert( m_pShader );

	// Create undefined vars for all the actual material vars
	m_VarCount = m_pShader->GetNumParams();
	m_pShaderParams = (IMaterialVar**)malloc( m_VarCount * sizeof(IMaterialVar*) );

	for (int i = 0; i < m_VarCount; ++i)
	{
		m_pShaderParams[i] = IMaterialVar::Create( this, m_pShader->GetParamName(i) );
	}

	// Store the model flags
	SetMaterialVarFlags( flags, true );

	// Set the default values
	ShaderSystem()->InitShaderParameters( m_pShader, m_pShaderParams, "Error" );

	// Invokes the SHADER_INIT block in the various shaders,
	ShaderSystem()->InitShaderInstance( m_pShader, m_pShaderParams, "Error" );

#ifdef _DEBUG
	bool ok = 
#endif
		InitializeStateSnapshots();
	Assert(ok);
}


//-----------------------------------------------------------------------------
// This computes the state snapshots for this material
//-----------------------------------------------------------------------------
void CMaterial::RecomputeStateSnapshots()
{
	bool ok = InitializeStateSnapshots();

	// compute the state snapshots
	if (!ok)
	{
		SetupErrorShader();
	}
}


//-----------------------------------------------------------------------------
// Get the software vertex shader from this material's shader if it is necessary.
//-----------------------------------------------------------------------------
const SoftwareVertexShader_t CMaterial::GetSoftwareVertexShader() const
{
	return GetShader()->GetSoftwareVertexShader();
}

bool CMaterial::UsesSoftwareVertexShader( void ) const
{
	return GetShader()->GetSoftwareVertexShader() ? true : false;
}


//-----------------------------------------------------------------------------
// Are we valid
//-----------------------------------------------------------------------------
inline bool CMaterial::IsValidRenderState() const
{
	return (m_Flags & MATERIAL_VALID_RENDERSTATE) != 0;
}


//-----------------------------------------------------------------------------
// Gets/sets material var flags
//-----------------------------------------------------------------------------
inline int CMaterial::GetMaterialVarFlags() const
{
	return m_pShaderParams[FLAGS]->GetIntValue();
}

inline void CMaterial::SetMaterialVarFlags( int flags, bool on )
{
	if (on)
		m_pShaderParams[FLAGS]->SetIntValue( GetMaterialVarFlags() | flags );
	else
		m_pShaderParams[FLAGS]->SetIntValue( GetMaterialVarFlags() & (~flags) );

	// Mark it as being defined...
	m_pShaderParams[FLAGS_DEFINED]->SetIntValue( 
		m_pShaderParams[FLAGS_DEFINED]->GetIntValue() | flags );
}

inline int CMaterial::GetMaterialVarFlags2() const
{
	return m_pShaderParams[FLAGS2]->GetIntValue();
}

inline void CMaterial::SetMaterialVarFlags2( int flags, bool on )
{
	if (on)
		m_pShaderParams[FLAGS2]->SetIntValue( GetMaterialVarFlags2() | flags );
	else
		m_pShaderParams[FLAGS2]->SetIntValue( GetMaterialVarFlags2() & (~flags) );

	// Mark it as being defined...
	m_pShaderParams[FLAGS_DEFINED2]->SetIntValue( 
		m_pShaderParams[FLAGS_DEFINED2]->GetIntValue() | flags );
}


//-----------------------------------------------------------------------------
// Gets the vertex format
//-----------------------------------------------------------------------------
VertexFormat_t CMaterial::GetVertexFormat() const
{
	Assert( IsValidRenderState() );
	return m_ShaderRenderState.m_VertexFormat;
}

int CMaterial::GetVertexUsage() const
{
	Assert( IsValidRenderState() );
	return m_ShaderRenderState.m_VertexUsage;
}

bool CMaterial::WillPreprocessData( void ) const
{
	if ( GetVertexUsage() & VERTEX_PREPROCESS_DATA )
		return true;
	return false;
}

bool CMaterial::PerformDebugTrace() const
{
	return IsValidRenderState() && ((GetMaterialVarFlags() & MATERIAL_VAR_DEBUG ) != 0);
}


//-----------------------------------------------------------------------------
// Are we suppressed?
//-----------------------------------------------------------------------------
bool  CMaterial::IsSuppressed() const
{
	if ( !IsValidRenderState() )
		return true;

	return ((GetMaterialVarFlags() & MATERIAL_VAR_NO_DRAW) != 0);
}

void CMaterial::ToggleSuppression()
{
	if (IsValidRenderState())
	{
		if ((GetMaterialVarFlags() & MATERIAL_VAR_NO_DEBUG_OVERRIDE) != 0)
			return;

		SetMaterialVarFlags( MATERIAL_VAR_NO_DRAW, 
			(GetMaterialVarFlags() & MATERIAL_VAR_NO_DRAW) == 0 );
	}
}

void CMaterial::ToggleDebugTrace()
{
	if (IsValidRenderState())
	{
		SetMaterialVarFlags( MATERIAL_VAR_DEBUG, 
			(GetMaterialVarFlags() & MATERIAL_VAR_DEBUG) == 0 );
	}
}

//-----------------------------------------------------------------------------
// Can we override this material in debug?
//-----------------------------------------------------------------------------

bool CMaterial::NoDebugOverride() const
{
	return IsValidRenderState() && (GetMaterialVarFlags() & MATERIAL_VAR_NO_DEBUG_OVERRIDE) != 0;
}


//-----------------------------------------------------------------------------
// Material Var flags
//-----------------------------------------------------------------------------
void CMaterial::SetMaterialVarFlag( MaterialVarFlags_t flag, bool on )
{
	bool oldOn = (GetMaterialVarFlags( ) & flag) != 0;
	if (oldOn != on)
	{
		SetMaterialVarFlags( flag, on );

		// This is going to be called from client code; recompute snapshots!
		RecomputeStateSnapshots();
	}
}

bool CMaterial::GetMaterialVarFlag( MaterialVarFlags_t flag ) const
{
	return (GetMaterialVarFlags() & flag) != 0;
}


//-----------------------------------------------------------------------------
// Do we use the env_cubemap entity to get cubemaps from the level?
//-----------------------------------------------------------------------------
bool CMaterial::UsesEnvCubemap( void )
{
	Precache();
	Assert( m_pShader );
	if( !m_pShader )
	{
		return false;
	}
	Assert( m_pShaderParams );
	return IsFlag2Set( m_pShaderParams, MATERIAL_VAR2_USES_ENV_CUBEMAP );
}


//-----------------------------------------------------------------------------
// Do we need a tangent space at the vertex level?
//-----------------------------------------------------------------------------
bool CMaterial::NeedsTangentSpace( void )
{
	Precache();
	Assert( m_pShader );
	if( !m_pShader )
	{
		return false;
	}
	Assert( m_pShaderParams );
	return IsFlag2Set( m_pShaderParams, MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
}

bool CMaterial::NeedsFrameBufferTexture( void )
{
	Precache();
	Assert( m_pShader );
	if( !m_pShader )
	{
		return false;
	}
	Assert( m_pShaderParams );
	return m_pShader->NeedsFrameBufferTexture( m_pShaderParams );
}

// GR - Is lightmap alpha needed?
bool CMaterial::NeedsLightmapBlendAlpha( void )
{
	Precache();
	return (GetMaterialVarFlags2() & MATERIAL_VAR2_BLEND_WITH_LIGHTMAP_ALPHA ) != 0;
}

//-----------------------------------------------------------------------------
// Do we need software skinning?
//-----------------------------------------------------------------------------
bool CMaterial::NeedsSoftwareSkinning( void )
{
	Precache();
	Assert( m_pShader );
	if( !m_pShader )
	{
		return false;
	}
	Assert( m_pShaderParams );
	return IsFlagSet( m_pShaderParams, MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING );
}


//-----------------------------------------------------------------------------
// Do we need software lighting?
//-----------------------------------------------------------------------------
bool CMaterial::NeedsSoftwareLighting( void )
{
	Precache();
	Assert( m_pShader );
	if( !m_pShader )
	{
		return false;
	}
	Assert( m_pShaderParams );
	return IsFlag2Set( m_pShaderParams, MATERIAL_VAR2_NEEDS_SOFTWARE_LIGHTING );
}


//-----------------------------------------------------------------------------
// Alpha/color modulation
//-----------------------------------------------------------------------------
void CMaterial::AlphaModulate( float alpha )
{
	Precache();
	m_pShaderParams[ALPHA]->SetFloatValue(alpha);
}

void CMaterial::ColorModulate( float r, float g, float b )
{
	Precache();
	m_pShaderParams[COLOR]->SetVecValue( r, g, b );
}


//-----------------------------------------------------------------------------
// Do we use fog?
//-----------------------------------------------------------------------------
bool CMaterial::UseFog() const
{
	Assert( m_VarCount > 0 );
	return IsValidRenderState() && ((GetMaterialVarFlags() & MATERIAL_VAR_NOFOG) == 0);
}


//-----------------------------------------------------------------------------
// diffuse bump?
//-----------------------------------------------------------------------------
bool CMaterial::IsUsingDiffuseBumpedLighting() const
{
	return (GetMaterialVarFlags2() & MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP ) != 0;
}


//-----------------------------------------------------------------------------
// lightmap?
//-----------------------------------------------------------------------------
bool CMaterial::IsUsingLightmap() const
{
	return (GetMaterialVarFlags2() & MATERIAL_VAR2_LIGHTING_LIGHTMAP ) != 0;
}

bool CMaterial::IsManuallyCreated() const
{
	return (m_Flags & MATERIAL_IS_MANUALLY_CREATED) != 0;
}


//-----------------------------------------------------------------------------
// Loads the material vars
//-----------------------------------------------------------------------------
bool CMaterial::PrecacheVars()
{
	// Don't bother if we're already precached
	if( IsPrecachedVars() )
		return true;

	// load data from the vmt file
	KeyValues * vmtKeyValues = new KeyValues("vmt");

	if( !LoadVMTFile( *vmtKeyValues ) )
	{
		Warning( "CMaterial::PrecacheVars: error loading vmt file for %s\n", GetName() );
		vmtKeyValues->deleteThis();
		return false;
	}

	// Needed to prevent re-entrancy
	m_Flags |= MATERIAL_VARS_IS_PRECACHED;

	// Create shader and the material vars...
	KeyValues* pFallbackKeyValues = InitializeShader( *vmtKeyValues );
	if (!pFallbackKeyValues)
	{
		vmtKeyValues->deleteThis();
		return false;
	}

	// Gotta initialize the proxies too, using the fallback proxies
	InitializeMaterialProxy(pFallbackKeyValues);

	vmtKeyValues->deleteThis();

	return true;
}


//-----------------------------------------------------------------------------
// Loads the material info from the VMT file
//-----------------------------------------------------------------------------
void CMaterial::Precache()
{
	// Don't bother if we're already precached
	if( IsPrecached() )
		return;

	// load data from the vmt file
	if( !PrecacheVars() )
		return;

	m_Flags |= MATERIAL_IS_PRECACHED;

	// Invokes the SHADER_INIT block in the various shaders,
	ShaderSystem()->InitShaderInstance( m_pShader, m_pShaderParams, GetName() );

	// compute the state snapshots
	RecomputeStateSnapshots();

	FindRepresentativeTexture();

	// Reads in the texture width and height from the material var
	PrecacheMappingDimensions();
}


//-----------------------------------------------------------------------------
// Unloads the material data from memory
//-----------------------------------------------------------------------------
void CMaterial::Uncache( bool bForceUncacheVars )
{
	// Don't bother if we're not cached
	if( IsPrecached() )
	{		
		// Clean up the state snapshots
		CleanUpStateSnapshots();

		m_Flags &= ~MATERIAL_IS_PRECACHED;
	}

	if( IsPrecachedVars() && (bForceUncacheVars || !IsManuallyCreated()) )
	{
		// Clean up the shader + params
		CleanUpShaderParams();
		m_pShader = 0;
		
		// Clean up the material proxy
		CleanUpMaterialProxy();
	
		m_Flags &= ~MATERIAL_VARS_IS_PRECACHED;
	}
}


//-----------------------------------------------------------------------------
// Meant to be used with materials created using CreateMaterial
// It updates the materials to reflect the current values stored in the material vars
//-----------------------------------------------------------------------------
void CMaterial::Refresh()
{
	if ( g_pShaderAPI->IsUsingGraphics() )
	{
		Uncache();
		Precache();
	}
}


//-----------------------------------------------------------------------------
// Gets the material name
//-----------------------------------------------------------------------------
char const* CMaterial::GetName() const
{
	return m_Name.String();
}


//-----------------------------------------------------------------------------
// Material dimensions
//-----------------------------------------------------------------------------
int	CMaterial::GetMappingWidth( )
{
	Precache();
	return m_MappingWidth;
}

int	CMaterial::GetMappingHeight( )
{
	Precache();
	return m_MappingHeight;
}


//-----------------------------------------------------------------------------
// Animated material info
//-----------------------------------------------------------------------------

int CMaterial::GetNumAnimationFrames( )
{
	Precache();
	if( m_representativeTexture )
	{
		return m_representativeTexture->GetNumAnimationFrames();
	}
	else
	{
		Warning( "CMaterial::GetNumAnimationFrames:\nno representative texture for material %s\n", GetName() );
		return 1;
	}
}


//-----------------------------------------------------------------------------
// Reference count
//-----------------------------------------------------------------------------
void CMaterial::IncrementReferenceCount( )
{
	++m_RefCount;
}

void CMaterial::DecrementReferenceCount( )
{
	--m_RefCount;
}

int CMaterial::GetReferenceCount( )	const
{
	return m_RefCount;
}


//-----------------------------------------------------------------------------
// Sets the shader associated with the material
//-----------------------------------------------------------------------------
void CMaterial::SetShader( const char *pShaderName )
{
	Assert( pShaderName );

	int i;
	IShader* pShader;
	IMaterialVar* ppVars[256];
	int iVarCount = 0;

	// Clean up existing state
	Uncache();

	// Keep going until there's no more fallbacks...
	while( true )
	{
		// Find the shader for this material. Note that this may not be
		// the actual shader we use due to fallbacks...
		pShader = ShaderSystem()->FindShader( pShaderName );
		if (!pShader)
		{
			// Couldn't find the shader we wanted to use; it's not defined...
			Warning( "SetShader: Couldn't find shader %s for material %s!\n", pShaderName, GetName() );
			pShaderName = "wireframe";
			pShader = ShaderSystem()->FindShader( pShaderName );
			Assert( pShader );
		}

		// Create undefined vars for all the actual material vars
		iVarCount = pShader->GetNumParams();
		for (i = 0; i < iVarCount; ++i)
		{
			ppVars[i] = IMaterialVar::Create( this, pShader->GetParamName(i) );
		}

		// Make sure we set default values before the fallback is looked for
		ShaderSystem()->InitShaderParameters( pShader, ppVars, pShaderName );

		// Now that the material vars are parsed, see if there's a fallback
		// But only if we're not in the tools
		if (!g_pShaderAPI->IsUsingGraphics())
			break;

		// Check for a fallback; if not, we're done
		pShaderName = pShader->GetFallbackShader( ppVars );
		if (!pShaderName)
			break;

		// Now, blow away all of the material vars + try again...
		for (i = 0; i < iVarCount; ++i)
		{
			Assert( ppVars[i] );
			IMaterialVar::Destroy( ppVars[i] );
		}
	}

	// Store off the shader
	m_pShader = pShader;

	// Store off the material vars + flags
	m_VarCount = iVarCount;
	m_pShaderParams = (IMaterialVar**)malloc( iVarCount * sizeof(IMaterialVar*) );
	memcpy( m_pShaderParams, ppVars, iVarCount * sizeof(IMaterialVar*) );

	// Invokes the SHADER_INIT block in the various shaders,
	ShaderSystem()->InitShaderInstance( m_pShader, m_pShaderParams, GetName() );

	// Precache our initial state...
	// NOTE: What happens here for textures???

	// Pretend that we precached our material vars; we certainly don't have any!
	m_Flags |= MATERIAL_VARS_IS_PRECACHED;

	// NOTE: The caller has to call 'Refresh' for the shader to be ready...
}



//-----------------------------------------------------------------------------
// Enumeration ID
//-----------------------------------------------------------------------------
int CMaterial::GetEnumerationID( ) const
{
	return m_iEnumerationID;
}
	
void CMaterial::SetEnumerationID( int id )
{
	m_iEnumerationID = id;
}


//-----------------------------------------------------------------------------
// Preview image
//-----------------------------------------------------------------------------
char const* CMaterial::GetPreviewImageName( void )
{
	PrecacheVars();

	bool found;
	IMaterialVar *pRepresentativeTextureVar;
	
	FindVar( "%noToolTexture", &found, false );
	if (found)
		return NULL;

	pRepresentativeTextureVar = FindVar( "%toolTexture", &found, false );
	if( found )
	{
		if (pRepresentativeTextureVar->GetType() == MATERIAL_VAR_TYPE_STRING )
			return pRepresentativeTextureVar->GetStringValue();
		if (pRepresentativeTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE )
			return pRepresentativeTextureVar->GetTextureValue()->GetName();
	}
	pRepresentativeTextureVar = FindVar( "$baseTexture", &found, false );
	if( found )
	{
		if (pRepresentativeTextureVar->GetType() == MATERIAL_VAR_TYPE_STRING )
			return pRepresentativeTextureVar->GetStringValue();
		if (pRepresentativeTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE )
			return pRepresentativeTextureVar->GetTextureValue()->GetName();
	}
	return GetName();
}

char const* CMaterial::GetPreviewImageFileName( void ) const
{
	char const* pName = const_cast<CMaterial*>(this)->GetPreviewImageName();
	if( !pName )
		return NULL;

	static char vtfFilename[MATERIAL_MAX_PATH];
	if( strlen( pName ) >= MATERIAL_MAX_PATH - 5 )
	{
		Warning( "MATERIAL_MAX_PATH to short for %s.vtf\n", pName );
		return NULL;
	}
	strcpy( vtfFilename, "materials/" );
	strcat( vtfFilename, pName );
	strcat( vtfFilename, ".vtf" );

	return vtfFilename;
}

PreviewImageRetVal_t CMaterial::GetPreviewImageProperties( int *width, int *height, 
				 		ImageFormat *imageFormat, bool* isTranslucent ) const
{	
	char const* pFileName = GetPreviewImageFileName();
	if ( !pFileName )
	{
		*width = *height = 0;
		*imageFormat = IMAGE_FORMAT_RGBA8888;
		*isTranslucent = false;
		return MATERIAL_NO_PREVIEW_IMAGE;
	}

	int nHeaderSize = VTFFileHeaderSize();
	unsigned char *pMem = (unsigned char *)stackalloc( nHeaderSize );
	CUtlBuffer buf( pMem, nHeaderSize );

	FileHandle_t fileHandle;
	fileHandle = g_pFileSystem->Open( pFileName, "rb" );
	if( !fileHandle )
	{
		Warning( "\"%s\": cached version doesn't exist\n", pFileName );
		return MATERIAL_PREVIEW_IMAGE_BAD;
	}
	
	// read the header only.. it's faster!!
	g_pFileSystem->Read( buf.Base(), nHeaderSize, fileHandle );
	g_pFileSystem->Close( fileHandle );

	IVTFTexture *pVTFTexture = CreateVTFTexture();
	if (!pVTFTexture->Unserialize( buf, true ))
	{
		Warning( "Error reading material \"%s\"\n", pFileName );
		DestroyVTFTexture( pVTFTexture );
		return MATERIAL_PREVIEW_IMAGE_BAD;
	}

	*width = pVTFTexture->Width();
	*height = pVTFTexture->Height();
	*imageFormat = pVTFTexture->Format();
	*isTranslucent = (pVTFTexture->Flags() & (TEXTUREFLAGS_ONEBITALPHA | TEXTUREFLAGS_EIGHTBITALPHA)) != 0;
	DestroyVTFTexture( pVTFTexture );
	return MATERIAL_PREVIEW_IMAGE_OK;
}

PreviewImageRetVal_t CMaterial::GetPreviewImage( unsigned char *pData, int width, int height,
					             ImageFormat imageFormat ) const
{
	CUtlBuffer buf;
	int nHeaderSize;
	int nImageOffset, nImageSize;

	char const* pFileName = GetPreviewImageFileName();
	if ( !pFileName )
		return MATERIAL_NO_PREVIEW_IMAGE;

	IVTFTexture *pVTFTexture = CreateVTFTexture();
	FileHandle_t fileHandle = g_pFileSystem->Open( pFileName, "rb" );
	if( !fileHandle )
	{
		Warning( "\"%s\": cached version doesn't exist\n", pFileName );
		goto fail;
	}
		
	nHeaderSize = VTFFileHeaderSize();
	buf.EnsureCapacity( nHeaderSize );
	
	// read the header first.. it's faster!!
	g_pFileSystem->Read( buf.Base(), nHeaderSize, fileHandle );
		
	// Unserialize the header
	if (!pVTFTexture->Unserialize( buf, true ))
	{
		Warning( "Error reading material \"%s\"\n", pFileName );
		goto fail;
	}
		
	// FIXME: Make sure the preview image size requested is the same
	// size as mip level 0 of the texture
	Assert( (width == pVTFTexture->Width()) && (height == pVTFTexture->Height()) );
		
	// Determine where in the file to start reading (frame 0, face 0, mip 0)
	pVTFTexture->ImageFileInfo( 0, 0, 0, &nImageOffset, &nImageSize );
		
	// Prep the utlbuffer for reading
	buf.EnsureCapacity( nImageSize );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
		
	// Read in the bits at the specified location
	g_pFileSystem->Seek( fileHandle, nImageOffset, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Read( buf.Base(), nImageSize, fileHandle );
	g_pFileSystem->Close( fileHandle );
		
	// Convert from the format read in to the requested format
	ImageLoader::ConvertImageFormat( (unsigned char*)buf.Base(), pVTFTexture->Format(), 
		pData, imageFormat, width, height );

	DestroyVTFTexture( pVTFTexture );
	return MATERIAL_PREVIEW_IMAGE_OK;

fail:
	if( fileHandle )
	{
		g_pFileSystem->Close( fileHandle );
	}
	int nSize = ImageLoader::GetMemRequired( width, height, imageFormat, false );
	memset( pData, 0xff, nSize );
	DestroyVTFTexture( pVTFTexture );
	return MATERIAL_PREVIEW_IMAGE_BAD;
}


//-----------------------------------------------------------------------------
// Material variables
//-----------------------------------------------------------------------------
IMaterialVar *CMaterial::FindVar( char const *pVarName, bool *pFound, bool complain )
{
	PrecacheVars();

	// FIXME: Could look for flags here too...

	MaterialVarSym_t sym = IMaterialVar::GetSymbol(pVarName);
	for (int i = m_VarCount; --i >= 0; )
	{
		if (m_pShaderParams[i]->GetNameAsSymbol() == sym)
		{
			if( pFound )
				*pFound = true;					  
			return m_pShaderParams[i];
		}
	}

	if( pFound )
		*pFound = false;

	if( complain )
	{
		Warning( "No such variable \"%s\" for material \"%s\"\n", pVarName, GetName() );
	}
	return GetDummyVariable();
}


//-----------------------------------------------------------------------------
// Lovely material properties
//-----------------------------------------------------------------------------
void CMaterial::GetReflectivity( Vector& reflect )
{
	Precache();

	if( m_representativeTexture )
	{
		m_representativeTexture->GetReflectivity( reflect );
	}
	else
	{
		VectorClear( reflect );
	}
}

bool CMaterial::GetPropertyFlag( MaterialPropertyTypes_t type )
{
	Precache();

	if (!IsValidRenderState())
		return false;

	switch( type )
	{
	case MATERIAL_PROPERTY_NEEDS_LIGHTMAP:
		return IsUsingLightmap();

	case MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS:
		return IsUsingDiffuseBumpedLighting();
	}

	return false;
}


//-----------------------------------------------------------------------------
// Is the material visible from both sides?
//-----------------------------------------------------------------------------
bool CMaterial::IsTwoSided()
{
	PrecacheVars();
	return GetMaterialVarFlag(MATERIAL_VAR_NOCULL);
}


//-----------------------------------------------------------------------------
// Are we translucent?
//-----------------------------------------------------------------------------
bool CMaterial::IsTranslucent()
{
	Precache();
	if (m_pShader && IsValidRenderState())
	{
		// I have to check for alpha modulation here because it isn't
		// factored into the shader's notion of whether or not it's transparent
		return ::IsTranslucent(&m_ShaderRenderState) || 
				(m_pShaderParams[ALPHA]->GetFloatValue() < 1.0f) ||
				GetMaterialVarFlag( MATERIAL_VAR_TRANSLUCENT );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Are we alphatested?
//-----------------------------------------------------------------------------
bool CMaterial::IsAlphaTested()
{
	Precache();
	if (m_pShader && IsValidRenderState())
	{
		return ::IsAlphaTested(&m_ShaderRenderState) || 
				GetMaterialVarFlag( MATERIAL_VAR_ALPHATEST );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Are we vertex lit?
//-----------------------------------------------------------------------------
bool CMaterial::IsVertexLit()
{
	Precache();
	if (IsValidRenderState())
	{
		return ( GetMaterialVarFlags2() & MATERIAL_VAR2_LIGHTING_VERTEX_LIT ) != 0;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Proxies
//-----------------------------------------------------------------------------
void CMaterial::CallBindProxy( void *proxyData )
{
	switch (g_config.proxiesTestMode)
	{
	case 0:
		{
			// Make sure we call the proxies in the order in which they show up
			// in the .vmt file

		//	CMaterialSystemStats::BeginBindProxy();
			for( int i = 0; i < m_ProxyCount; ++i )
			{
				m_ppProxies[i]->OnBind( proxyData );
			}
		//	CMaterialSystemStats::EndBindProxy();
		}
		break;

	case 2:
		// alpha mod all....
		{
			float value = ( sin( 2.0f * M_PI * Plat_FloatTime() / 10.0f ) * 0.5f ) + 0.5f;
			m_pShaderParams[ALPHA]->SetFloatValue( value );
		}
		break;

	case 3:
		// color mod all...
		{
			float value = ( sin( 2.0f * M_PI * Plat_FloatTime() / 10.0f ) * 0.5f ) + 0.5f;
			m_pShaderParams[COLOR]->SetVecValue( value, 1.0f, 1.0f );
		}
		break;
	}
}

bool CMaterial::HasProxy( )	const
{
	return (m_ProxyCount > 0);
}


//-----------------------------------------------------------------------------
// Main draw method
//-----------------------------------------------------------------------------

#ifdef _WIN32
#pragma warning (disable: 4189)
#endif

void CMaterial::DrawMesh( IMesh* pMesh )
{
	if( m_pShader )
	{
#ifdef _DEBUG
		if (GetMaterialVarFlags() & MATERIAL_VAR_DEBUG)
		{
			// Putcher breakpoint here to catch the rendering of a material
			// marked for debugging ($debug = 1 in a .vmt file) dynamic state version
			int x = 0;
		}
#endif

		if ((GetMaterialVarFlags() & MATERIAL_VAR_NO_DRAW) == 0)
		{
			ShaderSystem()->DrawElements( m_pShader, m_pShaderParams, &m_ShaderRenderState );
		}
	}
	else
	{
		Warning( "CMaterial::DrawElements: No bound shader\n" );
	}
}

#ifdef _WIN32
#pragma warning (default: 4189)
#endif

IShader *CMaterial::GetShader( ) const
{
	return m_pShader;
}

IMaterialVar *CMaterial::GetShaderParam( int id )
{
	return m_pShaderParams[id];
}


//-----------------------------------------------------------------------------
// Adds a material variable to the material
//-----------------------------------------------------------------------------
void CMaterial::AddMaterialVar( IMaterialVar *pMaterialVar )
{
	++m_VarCount;
	m_pShaderParams = (IMaterialVar**)realloc( m_pShaderParams, m_VarCount * sizeof( IMaterialVar*) );
	m_pShaderParams[m_VarCount-1] = pMaterialVar;
}


void CMaterial::FindRepresentativeTexture( void )
{
	Precache();
	
	// First try to find the base texture...
	bool found;
	IMaterialVar *textureVar = FindVar( "$baseTexture", &found, false );
	if( !found || textureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
	{
		// Try the env map mask if the base texture doesn't work...
		// this is needed for specular decals
		textureVar = FindVar( "$envmapmask", &found, false );
		if( !found || textureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
		{
			// Try the bumpmap
			textureVar = FindVar( "$bumpmap", &found, false );
			if( !found || textureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
			{
				textureVar = FindVar( "$dudvmap", &found, false );
				if( !found || textureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
				{
					textureVar = FindVar( "$normalmap", &found, false );
					if( !found || textureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
					{
						//				Warning( "Can't find representative texture for material \"%s\"\n",	GetName() );
						m_representativeTexture = TextureManager()->ErrorTexture();
						return;
					}
				}
			}
		}
	}

	m_representativeTexture = static_cast<ITextureInternal *>( textureVar->GetTextureValue() );
	if (m_representativeTexture)
	{
		m_representativeTexture->Precache();
	}
	else
	{
		m_representativeTexture = TextureManager()->ErrorTexture();
		Assert( m_representativeTexture );
	}
}


void CMaterial::GetLowResColorSample( float s, float t, float *color ) const
{
	if( !m_representativeTexture )
	{
		return;
	}
	m_representativeTexture->GetLowResColorSample( s, t, color);
}


//-----------------------------------------------------------------------------
// Lightmap-related methods
//-----------------------------------------------------------------------------

void CMaterial::SetMinLightmapPageID( int pageID )
{
	m_minLightmapPageID = pageID;
}

void CMaterial::SetMaxLightmapPageID( int pageID )
{
	m_maxLightmapPageID = pageID;
}

int CMaterial::GetMinLightmapPageID( ) const
{
	return m_minLightmapPageID;
}

int	CMaterial::GetMaxLightmapPageID( ) const
{
	return m_maxLightmapPageID;
}

void CMaterial::SetNeedsWhiteLightmap( bool val )
{
	if (val)
		m_Flags |= MATERIAL_NEEDS_WHITE_LIGHTMAP;
	else
		m_Flags &= ~MATERIAL_NEEDS_WHITE_LIGHTMAP;
}

bool CMaterial::GetNeedsWhiteLightmap( ) const
{
	return (m_Flags & MATERIAL_NEEDS_WHITE_LIGHTMAP) != 0;
}


//-----------------------------------------------------------------------------
// Return the shader params
//-----------------------------------------------------------------------------
IMaterialVar **CMaterial::GetShaderParams( void )
{
	return m_pShaderParams;
}

int CMaterial::ShaderParamCount() const
{
	return m_VarCount;
}


//-----------------------------------------------------------------------------
// VMT parser
//-----------------------------------------------------------------------------
static void InsertKeyValues( KeyValues& dst, KeyValues& src )
{
	KeyValues *pSrcVar = src.GetFirstSubKey();
	while( pSrcVar )
	{
		switch( pSrcVar->GetDataType() )
		{
		case KeyValues::TYPE_STRING:
			dst.SetString( pSrcVar->GetName(), pSrcVar->GetString() );
			break;
		case KeyValues::TYPE_INT:
			dst.SetInt( pSrcVar->GetName(), pSrcVar->GetInt() );
			break;
		case KeyValues::TYPE_FLOAT:
			dst.SetFloat( pSrcVar->GetName(), pSrcVar->GetFloat() );
			break;
		case KeyValues::TYPE_PTR:
			dst.SetPtr( pSrcVar->GetName(), pSrcVar->GetPtr() );
			break;
		}
		pSrcVar = pSrcVar->GetNextKey();
	}
}

static void WriteKeyValuesToFile( const char *pFileName, KeyValues& keyValues )
{
	keyValues.SaveToFile( g_pFileSystem, pFileName );
}

static void ExpandPatchFile( KeyValues& keyValues )
{
	int count = 0;
	while( count < 10 && stricmp( keyValues.GetName(), "patch" ) == 0 )
	{
//		WriteKeyValuesToFile( "patch.txt", keyValues );
		const char *pIncludeFileName = keyValues.GetString( "include" );
		if( pIncludeFileName )
		{
			KeyValues * includeKeyValues = new KeyValues( "vmt" );
			char *pFileName = ( char * )_alloca( strlen( pIncludeFileName ) + 
												 strlen( "materials/.vmt" ) + 1 );
			sprintf( pFileName, "%s", pIncludeFileName );
			bool success = includeKeyValues->LoadFromFile( g_pFileSystem, pFileName );
			if( success )
			{
//				WriteKeyValuesToFile( "include.txt", includeKeyValues );
				KeyValues *pInsertSection = keyValues.FindKey( "insert" );
//				WriteKeyValuesToFile( "insertsection.txt", *pInsertSection );
				if( pInsertSection )
				{
//					WriteKeyValuesToFile( "before.txt", includeKeyValues );
					InsertKeyValues( *includeKeyValues, *pInsertSection );
//					WriteKeyValuesToFile( "after.txt", includeKeyValues );
					keyValues = *includeKeyValues;
				}
				includeKeyValues->deleteThis();
				// Could add other commands here, like "delete", "rename", etc.
			}
			else
			{
				includeKeyValues->deleteThis();
				return;
			}
		}
		else
		{
			return;
		}
		count++;
	}
	if( count >= 10 )
	{
		Warning( "Infinite recursion in patch file?\n" );
	}
}

bool CMaterial::LoadVMTFile( KeyValues& vmtKeyValues )
{
	char pFileName[256];
	sprintf( pFileName, "materials/%s.vmt", GetName() );
	if (!vmtKeyValues.LoadFromFile( g_pFileSystem, pFileName))
	{
		Warning( "CMaterial::LoadVMTFile: can't open \"%s\"\n", pFileName );
		return false;
	}
	ExpandPatchFile( vmtKeyValues );

	return true;
}

int CMaterial::GetNumPasses( void )
{
	Precache();
//	int mod = m_ShaderRenderState.m_Modulation;
	int mod = 0;
	return m_ShaderRenderState.m_Snapshots[mod].m_nPassCount;
}

int CMaterial::GetTextureMemoryBytes( void )
{
	Precache();
	int bytes = 0;
	int i;
	for( i = 0; i < m_VarCount; i++ )
	{
		IMaterialVar *pVar = m_pShaderParams[i];
		if( pVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE )
		{
			ITexture *pTexture = pVar->GetTextureValue();
			if( pTexture && pTexture != ( ITexture * )0xffffffff )
			{
				bytes += pTexture->GetApproximateVidMemBytes();
			}
		}
	}
	return bytes;
}
