//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shadersystem.h"
#include <stdlib.h>
#include "materialsystem_global.h"
#include "filesystem.h"
#include "utldict.h"
#include "shaderlib/ShaderDLL.h"
#include "texturemanager.h"
#include "itextureinternal.h"
#include "IHardwareConfigInternal.h"
#include "utlstack.h"
#include "utlbuffer.h"
#include "vmatrix.h"
#include "imaterialinternal.h"
#include "vstdlib/strtools.h"
#include "vstdlib/ICommandLine.h"

// NOTE: This must be the last file included!
#include "tier0/memdbgon.h"



//-----------------------------------------------------------------------------
// Implementation of the shader system
//-----------------------------------------------------------------------------
class CShaderSystem : public IShaderSystemInternal
{
public:
	CShaderSystem();

	// Methods of IShaderSystem
	virtual void		BindTexture( TextureStage_t stage, ITexture *pTexture, int nFrame );
	virtual void		TakeSnapshot( );
	virtual void		DrawSnapshot( );
	virtual bool		IsUsingGraphics() const;

	// Methods of IShaderSystemInternal
	virtual void		Init();
	virtual void		Shutdown();
	virtual bool		LoadShaderDLL( const char *pFullPath );
	virtual void		UnloadShaderDLL( const char *pFullPath );
	virtual IShader*	FindShader( char const* pShaderName );
	virtual void		CreateDebugMaterials();
	virtual void		CleanUpDebugMaterials();
	virtual char const* ShaderStateString( int i );

	virtual void		InitShaderParameters( IShader *pShader, IMaterialVar **params, const char *pMaterialName );
	virtual void		InitShaderInstance( IShader *pShader, IMaterialVar **params, const char *pMaterialName );
	virtual bool		InitRenderState( IShader *pShader, int numParams, IMaterialVar **params, ShaderRenderState_t* pRenderState, char const* pMaterialName );
	virtual void		CleanupRenderState( ShaderRenderState_t* pRenderState );
	virtual void		DrawElements( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pShaderState );

	// Methods of IShaderInit
	virtual void		LoadTexture( IMaterialVar *pTextureVar ); 
	virtual void		LoadBumpMap( IMaterialVar *pTextureVar );
	virtual void		LoadCubeMap( IMaterialVar **ppParams, IMaterialVar *pTextureVar );

	// Used to prevent re-entrant rendering from warning messages
	void BufferSpew( const char *pMsg );

private:
	struct ShaderDLLInfo_t
	{
		char *m_pFileName;
		CSysModule *m_hInstance;
		IShaderDLLInternal *m_pShaderDLL;
		ShaderDLL_t m_hShaderDLL;
		CUtlDict< IShader *, unsigned short >	m_ShaderDict; 
	};

private:
	// Load up the shader DLLs...
	void LoadAllShaderDLLs();

	// Unload all the shader DLLs...
	void UnloadAllShaderDLLs();

	// Sets up the shader dictionary.
	void SetupShaderDictionary( int nShaderDLLIndex );

	// Cleans up the shader dictionary.
	void CleanupShaderDictionary( int nShaderDLLIndex );

	// Finds an already loaded shader DLL
	int FindShaderDLL( const char *pFullPath );

	// Unloads a particular shader DLL
	void UnloadShaderDLL( int nShaderDLLIndex );

	// Sets up the current ShaderState_t for rendering
	void PrepForShaderDraw( IShader *pShader, IMaterialVar** ppParams, 
		ShaderRenderState_t* pRenderState, int modulation );
	void DoneWithShaderDraw();

	// Initializes state snapshots
	void InitStateSnapshots( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pRenderState );

	// Compute snapshots for all combinations of alpha + color modulation
	void InitRenderStateFlags( ShaderRenderState_t* pRenderState, int numParams, IMaterialVar **params );

	// Computes flags from a particular snapshot
	void ComputeRenderStateFlagsFromSnapshot( ShaderRenderState_t* pRenderState );

	// Computes vertex format + usage from a particular snapshot
	bool ComputeVertexFormatFromSnapshot( IMaterialVar **params, ShaderRenderState_t* pRenderState );

	// Used to prevent re-entrant rendering from warning messages
	void PrintBufferedSpew( void );

	// returns various vertex formats
	VertexFormat_t GetVertexFormat( MaterialVertexFormat_t vfmt );

	// Gets at the current snapshot
	StateSnapshot_t CurrentStateSnapshot();

	// Draws using a particular material..
	void DrawUsingMaterial( IMaterialInternal *pMaterial );

	// Copies material vars
	void CopyMaterialVarToDebugShader( IMaterialInternal *pDebugMaterial, IShader *pShader, IMaterialVar **ppParams, const char *pVarName );

	// Debugging draw methods...
	void DrawMeasureFillRate( ShaderRenderState_t* pRenderState, int mod );
	void DrawNormalMap( IShader *pShader, IMaterialVar **ppParams );
	void DrawUnlitOnly( IMaterialVar **params, bool usesVertexShader );
	void DrawVertexLitOnly( bool usesVertexShader );
	void DrawLightmappedOnly( bool usesVertexShader );
	void DrawBumpedLightmapOnly( IShader *pShader, IMaterialVar **ppParams );
	void DrawBumpedModelLightingOnly( IShader *pShader, IMaterialVar **ppParams );
	void DrawElementsLightingOnly( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pRenderState );

private:
	// List of all DLLs containing shaders
	CUtlVector< ShaderDLLInfo_t > m_ShaderDLLs;

	// Used to prevent re-entrant rendering from warning messages
	SpewOutputFunc_t m_SaveSpewOutput;
	CUtlBuffer m_StoredSpew;

	// Render state we're drawing with
	ShaderRenderState_t* m_pRenderState;
	unsigned short m_hShaderDLL;
	unsigned char m_nModulation;
	unsigned char m_nRenderPass;

	// Debugging materials
	// If you add to this, add to the list of debug shader names (s_pDebugShaderName) below
	enum
	{
		MATERIAL_FILL_RATE = 0,
		MATERIAL_FILL_RATE_DX6,
		MATERIAL_DEBUG_NORMALMAP,
		MATERIAL_DEBUG_NORMALMAP_MODEL,
		MATERIAL_DEBUG_LIGHTMAP_VS,
		MATERIAL_DEBUG_LIGHTMAP_FF,
		MATERIAL_DEBUG_BUMPED_LIGHTMAP,
		MATERIAL_DEBUG_UNLIT_FF,
		MATERIAL_DEBUG_UNLIT_FF_MODEL,
		MATERIAL_DEBUG_UNLIT_VS,
		MATERIAL_DEBUG_UNLIT_VS_MODEL,
		MATERIAL_DEBUG_VERTEXLIT_FF,
		MATERIAL_DEBUG_VERTEXLIT_VS,
		MATERIAL_DEBUG_BUMPED_VERTEXLIT,
		MATERIAL_DEBUG_BUMPED_VERTEXLIT_SELF_ILLUM,

		MATERIAL_DEBUG_COUNT,
	};

	IMaterialInternal* m_pDebugMaterials[MATERIAL_DEBUG_COUNT];
	static const char *s_pDebugShaderName[MATERIAL_DEBUG_COUNT];
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CShaderSystem s_ShaderSystem;
IShaderSystemInternal *g_pShaderSystem = &s_ShaderSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderSystem, IShaderSystem, 
						SHADERSYSTEM_INTERFACE_VERSION, s_ShaderSystem );


//-----------------------------------------------------------------------------
// Debugging shader names
//-----------------------------------------------------------------------------
const char *CShaderSystem::s_pDebugShaderName[MATERIAL_DEBUG_COUNT]	=
{
	"FillRate",
	"FillRate_Dx6",
	"DebugNormalMap",
	"DebugNormalMap",
	"DebugLightmap",
	"DebugLightmap",
	"DebugBumpedLightmap",
	"DebugUnlit",
	"DebugUnlit",
	"DebugUnlit",
	"DebugUnlit",
	"DebugVertexLit",
	"DebugVertexLit",
	"DebugBumpedVertexLit",
	"DebugBumpedVertexLit",
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CShaderSystem::CShaderSystem() : m_StoredSpew( 0, 512, true )
{
}


//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
void CShaderSystem::Init()
{
	m_SaveSpewOutput = NULL;
	
	int i;
	for ( i = 0; i < MATERIAL_DEBUG_COUNT; ++i )
	{
		m_pDebugMaterials[i] = NULL;
	}

	LoadAllShaderDLLs();
}

void CShaderSystem::Shutdown()
{
	UnloadAllShaderDLLs();
}


//-----------------------------------------------------------------------------
// Load up the shader DLLs...
//-----------------------------------------------------------------------------
void CShaderSystem::LoadAllShaderDLLs( )
{
	UnloadAllShaderDLLs();

	GetShaderDLLInternal()->Connect( Sys_GetFactoryThis() );

	// Loads local shaders
	int i = m_ShaderDLLs.AddToHead();
	m_ShaderDLLs[i].m_pFileName = new char[ 1 ];
	m_ShaderDLLs[i].m_pFileName[0] = 0;
	m_ShaderDLLs[i].m_hInstance = NULL;
	m_ShaderDLLs[i].m_pShaderDLL = GetShaderDLLInternal();

	// Add the shaders to the dictionary of shaders...
	SetupShaderDictionary( i );

	// Always load the debug shader DLL
	LoadShaderDLL( "stdshader_dbg.dll" );

	// Load up standard shader DLLs...
	int dxSupportLevel = HardwareConfig()->GetDXSupportLevel();
	Assert( dxSupportLevel >= 60 );
	dxSupportLevel /= 10;

	char buf[32];
	for ( i = 6; i <= dxSupportLevel; ++i )
	{
		Q_snprintf( buf, 32, "stdshader_dx%d.dll", i );
		LoadShaderDLL( buf );
	}

	const char *pShaderName = NULL;
	pShaderName = CommandLine()->ParmValue( "-shader" );
	if( !pShaderName )
	{
		pShaderName = HardwareConfig()->GetHWSpecificShaderDLLName();
	}
	if ( pShaderName )
	{
		LoadShaderDLL( pShaderName );
	}

#ifdef _DEBUG
	// For fast-iteration debugging
	if ( CommandLine()->FindParm( "-testshaders" ) )
	{
		LoadShaderDLL( "shader_test.dll" );
	}
#endif
}


//-----------------------------------------------------------------------------
// Unload all the shader DLLs...
//-----------------------------------------------------------------------------
void CShaderSystem::UnloadAllShaderDLLs()
{
	if ( m_ShaderDLLs.Count() == 0 )
		return;

	for ( int i = m_ShaderDLLs.Count(); --i >= 0; )
	{
		UnloadShaderDLL(i);
		delete[] m_ShaderDLLs[i].m_pFileName;
	}

	m_ShaderDLLs.RemoveAll();

	GetShaderDLLInternal()->Disconnect( );
}


//-----------------------------------------------------------------------------
// Methods related to reading in shader DLLs
//-----------------------------------------------------------------------------
bool CShaderSystem::LoadShaderDLL( const char *pFullPath )
{
	if ( !pFullPath && !pFullPath[0] )
		return true;

	// Load the new shader
	CSysModule *hInstance = g_pFileSystem->LoadModule(pFullPath);
	if ( !hInstance )
		return false;

	// Get at the shader DLL interface
	CreateInterfaceFn factory = Sys_GetFactory( hInstance );
	if (!factory)
	{
		g_pFileSystem->UnloadModule( hInstance );
		return false;
	}

	IShaderDLLInternal *pShaderDLL = (IShaderDLLInternal*)factory( SHADER_DLL_INTERFACE_VERSION, NULL );
	if ( !pShaderDLL )
	{
		g_pFileSystem->UnloadModule( hInstance );
		return false;
	}

	// Allow the DLL to try to connect to interfaces it needs
	if ( !pShaderDLL->Connect( Sys_GetFactoryThis() ) )
	{
		g_pFileSystem->UnloadModule( hInstance );
		return false;
	}

	// FIXME: We need to do some sort of shader validation here for anticheat.

	// Now replace any existing shader
	int nShaderDLLIndex = FindShaderDLL( pFullPath );
	if ( nShaderDLLIndex >= 0 )
	{
		UnloadShaderDLL( nShaderDLLIndex );
	}
	else
	{
		nShaderDLLIndex = m_ShaderDLLs.AddToTail();
		int nLen = Q_strlen(pFullPath) + 1;
		m_ShaderDLLs[nShaderDLLIndex].m_pFileName = new char[ nLen ];
		Q_strncpy( m_ShaderDLLs[nShaderDLLIndex].m_pFileName, pFullPath, nLen );
	}

	// Ok, the shader DLL's good!
	m_ShaderDLLs[nShaderDLLIndex].m_hInstance = hInstance;
	m_ShaderDLLs[nShaderDLLIndex].m_pShaderDLL = pShaderDLL;
	
	// Add the shaders to the dictionary of shaders...
	SetupShaderDictionary( nShaderDLLIndex );
	
	// FIXME: Fix up existing materials that were using shaders that have
	// been reloaded?

	return true;
}


//-----------------------------------------------------------------------------
// Finds an already loaded shader DLL
//-----------------------------------------------------------------------------
int CShaderSystem::FindShaderDLL( const char *pFullPath )
{
	for ( int i = m_ShaderDLLs.Count(); --i >= 0; )
	{
		if ( !Q_stricmp( pFullPath, m_ShaderDLLs[i].m_pFileName ) )
			return i;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Unloads a particular shader DLL
//-----------------------------------------------------------------------------
void CShaderSystem::UnloadShaderDLL( int nShaderDLLIndex )
{
	if ( nShaderDLLIndex < 0 )
		return;

	// FIXME: Do some sort of fixup of materials to determine which
	// materials are referencing shaders in this DLL?
	CleanupShaderDictionary( nShaderDLLIndex );
	m_ShaderDLLs[nShaderDLLIndex].m_pShaderDLL->Disconnect();
	if ( m_ShaderDLLs[nShaderDLLIndex].m_hInstance )
	{
		g_pFileSystem->UnloadModule( m_ShaderDLLs[nShaderDLLIndex].m_hInstance );
	}
}


//-----------------------------------------------------------------------------
// Unloads a particular shader DLL
//-----------------------------------------------------------------------------
void CShaderSystem::UnloadShaderDLL( const char *pFullPath )
{
	int nShaderDLLIndex = FindShaderDLL( pFullPath );
	if ( nShaderDLLIndex >= 0 )
	{
		UnloadShaderDLL( nShaderDLLIndex );
		delete[] m_ShaderDLLs[nShaderDLLIndex].m_pFileName;
		m_ShaderDLLs.Remove( nShaderDLLIndex ); 
	}
}


//-----------------------------------------------------------------------------
// returns strings associated with the shader state flags...
//-----------------------------------------------------------------------------
char const* CShaderSystem::ShaderStateString( int i )
{
	// Make sure these match the bits in imaterial.h
	static char* s_pShaderStateString[] =
	{
		"$debug",
		"$no_fullbright",
		"$no_draw",
		"$use_in_fillrate_mode",

		"$vertexcolor",
		"$vertexalpha",
		"$selfillum",
		"$additive",
		"$alphatest",
		"$multipass",
		"$znearer",
		"$model",
		"$flat",
		"$nocull",
		"$nofog",
		"$ignorez",
		"$decal",
		"$envmapsphere",
		"$noalphamod",
		"$envmapcameraspace",
		"$nooverbright",
		"$basealphaenvmapmask",
		"$translucent",
		"$normalmapalphaenvmapmask",
		"$softwareskin",
		"$opaquetexture",
		"$envmapmode",

		""			// last one must be null
	};

	return s_pShaderStateString[i];
}


//-----------------------------------------------------------------------------
// Sets up the shader dictionary.
//-----------------------------------------------------------------------------
void CShaderSystem::SetupShaderDictionary( int nShaderDLLIndex )
{
	// We could have put the shader dictionary into each shader DLL
	// I'm not sure if that makes this system any less secure than it already is
	int i;
	ShaderDLLInfo_t &info = m_ShaderDLLs[nShaderDLLIndex];
	info.m_hShaderDLL = g_pShaderAPI->AddShaderDLL();
	int nCount = info.m_pShaderDLL->ShaderCount();
	for ( i = 0; i < nCount; ++i )
	{
		IShader *pShader = info.m_pShaderDLL->GetShader( i );
		pShader->SetShaderDLLHandle( info.m_hShaderDLL );
		info.m_ShaderDict.Insert( pShader->GetName(), pShader );
	}

	// Install precompiled shaders into the shader DLL
	nCount = info.m_pShaderDLL->ShaderDictionaryCount();
	for ( i = 0; i < nCount; ++i )
	{
		IPrecompiledShaderDictionary *pDict = info.m_pShaderDLL->GetShaderDictionary( i );
		g_pShaderAPI->AddShaderDictionary( nShaderDLLIndex, pDict );
	}
}


//-----------------------------------------------------------------------------
// Cleans up the shader dictionary.
//-----------------------------------------------------------------------------
void CShaderSystem::CleanupShaderDictionary( int nShaderDLLIndex )
{
	// Remove precompiled shaders from the shader DLL
	ShaderDLLInfo_t &info = m_ShaderDLLs[nShaderDLLIndex];
	g_pShaderAPI->RemoveShaderDLL( info.m_hShaderDLL );
}


//-----------------------------------------------------------------------------
// Finds a shader in the shader dictionary
//-----------------------------------------------------------------------------
IShader* CShaderSystem::FindShader( char const* pShaderName )
{
	// FIXME: What kind of search order should we use here?
	// I'm currently assuming last added, first searched.
	for (int i = m_ShaderDLLs.Count(); --i >= 0; )
	{
		ShaderDLLInfo_t &info = m_ShaderDLLs[i];
		unsigned short idx = info.m_ShaderDict.Find( pShaderName );
		if ( idx != info.m_ShaderDict.InvalidIndex() )
		{
			return info.m_ShaderDict[idx];
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
//
// Methods of IShaderInit lie below
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Gets at the render pass info for this pass...
//-----------------------------------------------------------------------------
inline StateSnapshot_t CShaderSystem::CurrentStateSnapshot()
{
	Assert( m_pRenderState );
	Assert( m_nRenderPass < MAX_RENDER_PASSES );
	Assert( m_nRenderPass < m_pRenderState->m_Snapshots[m_nModulation].m_nPassCount );
	return m_pRenderState->m_Snapshots[m_nModulation].m_Snapshot[m_nRenderPass];
}


//-----------------------------------------------------------------------------
// Create debugging materials
//-----------------------------------------------------------------------------
void CShaderSystem::CreateDebugMaterials()
{
	if (m_pDebugMaterials[0])
		return;

	int i;
	for ( i = 0; i < MATERIAL_DEBUG_COUNT; ++i )
	{
		char shaderName[64];
		sprintf( shaderName, "___%s_%d.vmt", s_pDebugShaderName[i], i );
		m_pDebugMaterials[i] = IMaterialInternal::Create( shaderName, false );
		m_pDebugMaterials[i]->SetShader( s_pDebugShaderName[i] );
		m_pDebugMaterials[i]->IncrementReferenceCount();
		MaterialSystem()->AddMaterialToMaterialList( m_pDebugMaterials[i] );
	}

	bool bFound;
	IMaterialVar *pMaterialVar;
	
	m_pDebugMaterials[MATERIAL_DEBUG_NORMALMAP_MODEL]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );
	
	pMaterialVar = m_pDebugMaterials[MATERIAL_DEBUG_LIGHTMAP_FF]->FindVar( "$fixedfunction", &bFound );
	pMaterialVar->SetIntValue( 1 );
	
	pMaterialVar = m_pDebugMaterials[MATERIAL_DEBUG_UNLIT_FF]->FindVar( "$fixedfunction", &bFound );
	pMaterialVar->SetIntValue( 1 );

	pMaterialVar = m_pDebugMaterials[MATERIAL_DEBUG_UNLIT_FF_MODEL]->FindVar( "$fixedfunction", &bFound );
	pMaterialVar->SetIntValue( 1 );
	m_pDebugMaterials[MATERIAL_DEBUG_UNLIT_FF_MODEL]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );

	m_pDebugMaterials[MATERIAL_DEBUG_UNLIT_VS_MODEL]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );

	pMaterialVar = m_pDebugMaterials[MATERIAL_DEBUG_VERTEXLIT_FF]->FindVar( "$fixedfunction", &bFound );
	pMaterialVar->SetIntValue( 1 );
	m_pDebugMaterials[MATERIAL_DEBUG_VERTEXLIT_FF]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );

	m_pDebugMaterials[MATERIAL_DEBUG_VERTEXLIT_VS]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );

	m_pDebugMaterials[MATERIAL_DEBUG_BUMPED_VERTEXLIT]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );

	pMaterialVar = m_pDebugMaterials[MATERIAL_DEBUG_BUMPED_VERTEXLIT_SELF_ILLUM]->FindVar( "$hasselfillum", &bFound );
	pMaterialVar->SetIntValue( 1 );
	m_pDebugMaterials[MATERIAL_DEBUG_BUMPED_VERTEXLIT_SELF_ILLUM]->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );

	for ( i = 0; i < MATERIAL_DEBUG_COUNT; ++i )
	{
		m_pDebugMaterials[i]->Refresh();
	}
}


//-----------------------------------------------------------------------------
// Cleans up the debugging materials
//-----------------------------------------------------------------------------
void CShaderSystem::CleanUpDebugMaterials()
{
	if (m_pDebugMaterials[0])
	{
		for ( int i = 0; i < MATERIAL_DEBUG_COUNT; ++i )
		{
			m_pDebugMaterials[i]->DecrementReferenceCount();
			MaterialSystem()->RemoveMaterial( m_pDebugMaterials[i] );
			m_pDebugMaterials[i] = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Deal with buffering of spew while doing shader draw so that we don't get 
// recursive spew during precache due to fonts not being loaded, etc.
//-----------------------------------------------------------------------------
void CShaderSystem::BufferSpew( const char *pMsg )
{
	if ( m_StoredSpew.TellPut() > 0 )
	{
		// Write over the trailing '0'
		m_StoredSpew.SeekPut( CUtlBuffer::SEEK_CURRENT, -1 );
	}

	m_StoredSpew.PutString( pMsg );
	m_StoredSpew.PutChar( 0 );
}

void CShaderSystem::PrintBufferedSpew( void )
{
	if( m_StoredSpew.TellPut() > 0 )
	{
		Warning( (char*)m_StoredSpew.PeekGet() );
		m_StoredSpew.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
	}
}

static SpewRetval_t MySpewOutputFunc( SpewType_t spewType, char const *pMsg )
{
	s_ShaderSystem.BufferSpew( pMsg );

	switch( spewType )
	{
	case SPEW_MESSAGE:
	case SPEW_WARNING:
	case SPEW_LOG:
		return SPEW_CONTINUE;

	case SPEW_ASSERT:
	case SPEW_ERROR:
	default:
		return SPEW_DEBUGGER;
	}
}


//-----------------------------------------------------------------------------
// Deals with shader draw
//-----------------------------------------------------------------------------
void CShaderSystem::PrepForShaderDraw( IShader *pShader,
	IMaterialVar** ppParams, ShaderRenderState_t* pRenderState, int nModulation )
{
	Assert( !m_pRenderState );
	Assert( !m_SaveSpewOutput );
	m_SaveSpewOutput = GetSpewOutputFunc();
	SpewOutputFunc( MySpewOutputFunc );

	m_pRenderState = pRenderState;
	m_nModulation = nModulation;
	m_nRenderPass = 0;
	m_hShaderDLL = pShader->GetShaderDLLHandle();
	g_pShaderAPI->SetShaderDLL( m_hShaderDLL );
}

void CShaderSystem::DoneWithShaderDraw()
{
	SpewOutputFunc( m_SaveSpewOutput );
	PrintBufferedSpew();
	m_SaveSpewOutput = NULL;
	m_pRenderState = NULL;
	g_pShaderAPI->SetShaderDLL( -1 );
}


//-----------------------------------------------------------------------------
// Call the SHADER_PARAM_INIT block of the shaders
//-----------------------------------------------------------------------------
void CShaderSystem::InitShaderParameters( IShader *pShader, IMaterialVar **params, const char *pMaterialName )
{
	// Let the derived class do its thing
	PrepForShaderDraw( pShader, params, 0, 0 );
	pShader->InitShaderParams( params, pMaterialName );
	DoneWithShaderDraw();

	// Set up color + alpha defaults
	if (!params[COLOR]->IsDefined())
	{
		params[COLOR]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if (!params[ALPHA]->IsDefined())
	{
		params[ALPHA]->SetFloatValue( 1.0f );
	}

	// Initialize all shader params based on their type...
	int i;
	for ( i = pShader->GetNumParams(); --i >= 0; )
	{
		// Don't initialize parameters that are already set up
		if (params[i]->IsDefined())
			continue;

		int type = pShader->GetParamType( i );
		switch( type )
		{
		case SHADER_PARAM_TYPE_TEXTURE:
			// Do nothing; we'll be loading in a string later
			break;
		case SHADER_PARAM_TYPE_MATERIAL:
			params[i]->SetMaterialValue( NULL );
			break;
		case SHADER_PARAM_TYPE_BOOL:
		case SHADER_PARAM_TYPE_INTEGER:
			params[i]->SetIntValue( 0 );
			break;
		case SHADER_PARAM_TYPE_COLOR:
			params[i]->SetVecValue( 1.0f, 1.0f, 1.0f );
			break;
		case SHADER_PARAM_TYPE_VEC2:
			params[i]->SetVecValue( 0.0f, 0.0f );
			break;
		case SHADER_PARAM_TYPE_VEC3:
			params[i]->SetVecValue( 0.0f, 0.0f, 0.0f );
			break;
		case SHADER_PARAM_TYPE_VEC4:
			params[i]->SetVecValue( 0.0f, 0.0f, 0.0f, 0.0f );
			break;
		case SHADER_PARAM_TYPE_FLOAT:
			params[i]->SetFloatValue( 0 );
			break;
		case SHADER_PARAM_TYPE_FOURCC:
			params[i]->SetFourCCValue( 0, 0 );
			break;
		case SHADER_PARAM_TYPE_MATRIX:
			{
				VMatrix identity;
				MatrixSetIdentity( identity );
				params[i]->SetMatrixValue( identity );
			}
			break;

		default:
			Assert(0);
		}
	}
}


//-----------------------------------------------------------------------------
// Call the SHADER_INIT block of the shaders
//-----------------------------------------------------------------------------
void CShaderSystem::InitShaderInstance( IShader *pShader, IMaterialVar **params, const char *pMaterialName )
{
	// Let the derived class do its thing
	PrepForShaderDraw( pShader, params, 0, 0 );
	pShader->InitShaderInstance( params, ShaderSystem(), pMaterialName );
	DoneWithShaderDraw();
}


//-----------------------------------------------------------------------------
// Compute snapshots for all combinations of alpha + color modulation
//-----------------------------------------------------------------------------
void CShaderSystem::InitRenderStateFlags( ShaderRenderState_t* pRenderState, int numParams, IMaterialVar **params )
{
	// Compute vertex format and flags
	pRenderState->m_Flags = 0;

	// Make sure the shader don't force these flags. . they are automatically computed.
	Assert( !( pRenderState->m_Flags & SHADER_OPACITY_TRANSLUCENT ) );
	Assert( !( pRenderState->m_Flags & SHADER_OPACITY_ALPHATEST ) );
	Assert( !( pRenderState->m_Flags & SHADER_OPACITY_OPAQUE ) );

	// If we are in release mode, just go ahead and clear in case the above is screwed up.
	pRenderState->m_Flags &= ~SHADER_OPACITY_MASK;

/*
	// HACK: Also kind of gross; turn off bump lightmapping for low-end
	if (g_config.bUseGraphics && !HardwareConfig()->SupportsVertexAndPixelShaders())
	{
		pRenderState->m_Flags &= ~SHADER_NEEDS_BUMPED_LIGHTMAPS;
	}
*/
/*
	// HACK: more grossness!!!  turn off bump lightmapping if we don't have a bumpmap
	// Shaders should specify SHADER_NEEDS_BUMPED_LIGHTMAPS if they might need a bumpmap,
	// and this'll take care of getting rid of it if it isn't there.
	if( pRenderState->m_Flags & SHADER_NEEDS_BUMPED_LIGHTMAPS )
	{
		pRenderState->m_Flags &= ~SHADER_NEEDS_BUMPED_LIGHTMAPS;
		for( int i = 0; i < numParams; i++ )
		{
			if( stricmp( params[i]->GetName(), "$bumpmap" ) == 0 )
			{
				if( params[i]->IsDefined() )
				{
					const char *blah = params[i]->GetStringValue();
					pRenderState->m_Flags |= SHADER_NEEDS_BUMPED_LIGHTMAPS;
					break;
				}
			}
		}
	}
*/
}


//-----------------------------------------------------------------------------
// Computes flags from a particular snapshot
//-----------------------------------------------------------------------------
void CShaderSystem::ComputeRenderStateFlagsFromSnapshot( ShaderRenderState_t* pRenderState )
{
	// When computing the flags, use the snapshot that has no alpha or color
	// modulation. When asking for translucency, we'll have to check for
	// alpha modulation in addition to checking the TRANSLUCENT flag.

	// I have to do it this way because I'm really wanting to treat alpha
	// modulation as a dynamic state, even though it's being used to compute
	// shadow state. I still want to use it to compute shadow state though
	// because it's somewhat complicated code that I'd rather precache.

	StateSnapshot_t snapshot = pRenderState->m_Snapshots[0].m_Snapshot[0];

	// Automatically compute if the snapshot is transparent or not
	if ( g_pShaderAPI->IsTranslucent( snapshot ) )
	{
		pRenderState->m_Flags |= SHADER_OPACITY_TRANSLUCENT;
	}
	else
	{
		if ( g_pShaderAPI->IsAlphaTested( snapshot ) )
		{
			pRenderState->m_Flags |= SHADER_OPACITY_ALPHATEST;
		}
		else
		{
			pRenderState->m_Flags |= SHADER_OPACITY_OPAQUE;
		}
	}

#ifdef _DEBUG
	if( pRenderState->m_Flags & SHADER_OPACITY_TRANSLUCENT )
	{
		Assert( !( pRenderState->m_Flags & SHADER_OPACITY_ALPHATEST ) );
		Assert( !( pRenderState->m_Flags & SHADER_OPACITY_OPAQUE ) );
	}
	if( pRenderState->m_Flags & SHADER_OPACITY_ALPHATEST )
	{
		Assert( !( pRenderState->m_Flags & SHADER_OPACITY_TRANSLUCENT ) );
		Assert( !( pRenderState->m_Flags & SHADER_OPACITY_OPAQUE ) );
	}
	if( pRenderState->m_Flags & SHADER_OPACITY_OPAQUE )
	{
		Assert( !( pRenderState->m_Flags & SHADER_OPACITY_ALPHATEST ) );
		Assert( !( pRenderState->m_Flags & SHADER_OPACITY_TRANSLUCENT ) );
	}
#endif

	if ( g_pShaderAPI->UsesAmbientCube( snapshot ) )
		pRenderState->m_Flags |= SHADER_USES_AMBIENT_CUBE_STAGE0;
}


//-----------------------------------------------------------------------------
// Initializes state snapshots
//-----------------------------------------------------------------------------
#ifdef _DEBUG
#pragma warning (disable:4189)
#endif

void CShaderSystem::InitStateSnapshots( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pRenderState )
{
#ifdef _DEBUG
	if ( IsFlagSet( params, MATERIAL_VAR_DEBUG ) )
	{
		// Putcher breakpoint here to catch the rendering of a material
		// marked for debugging ($debug = 1 in a .vmt file) shadow state version
		int x = 0;
	}
#endif

	// Store off the current alpha + color modulations
	float alpha;
	float color[3];
	params[COLOR]->GetVecValue( color, 3 );
	alpha = params[ALPHA]->GetFloatValue( );

	float white[3] = { 1, 1, 1 };
	float grey[3] = { .5, .5, .5 };

	for (int i = 0; i < 4; ++i)
	{
		// Set modulation to force particular code paths
		if (i & SHADER_USING_COLOR_MODULATION)
		{
			params[COLOR]->SetVecValue( grey, 3 );
		}
		else
		{
			params[COLOR]->SetVecValue( white, 3 );
		}

		if (i & SHADER_USING_ALPHA_MODULATION)
		{
			params[ALPHA]->SetFloatValue( grey[0] );
		}
		else
		{
			params[ALPHA]->SetFloatValue( white[0] );
		}

		PrepForShaderDraw( pShader, params, pRenderState, i );

		// Now snapshot how we're going to draw
		pRenderState->m_Snapshots[i].m_nPassCount = 0;
		pShader->DrawElements( params, i, g_pShaderShadow, 0 );

		DoneWithShaderDraw();
	}

	// Restore alpha + color modulation
	params[COLOR]->SetVecValue( color, 3 );
	params[ALPHA]->SetFloatValue( alpha );
}

#ifdef _DEBUG
#pragma warning (default:4189)
#endif


//-----------------------------------------------------------------------------
// returns various vertex formats
//-----------------------------------------------------------------------------
VertexFormat_t CShaderSystem::GetVertexFormat( MaterialVertexFormat_t vfmt )
{
	VertexFormat_t fmt;
	switch (vfmt)
	{
	case MATERIAL_VERTEX_FORMAT_MODEL:
		// These are the formats used by all models.
		// We basically guarantee pos, color, normal, 1 2D texcoord, 3 bones
		// and tangent space specification and that's it.

		if (!HardwareConfig()->SupportsVertexAndPixelShaders())
		{
			// 48 bytes...
			// only specify 2 bone weights 'cause the third is implied
			fmt = VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL | 
				VERTEX_NUM_TEXCOORDS(1) | VERTEX_TEXCOORD_SIZE(0, 2) | 
				VERTEX_BONEWEIGHT(2) | VERTEX_BONE_INDEX;
		}
		else
		{
			// 64 bytes
			// The third bone weight is calculated in the vertex shader.
			// VERTEX_USERDATA_SIZE(4) is for tangentS plus a -1,1 sign value.
			fmt = VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL | VERTEX_USERDATA_SIZE(4) |
				VERTEX_NUM_TEXCOORDS(1) | VERTEX_TEXCOORD_SIZE(0, 2) | 
				VERTEX_BONEWEIGHT(2) | VERTEX_BONE_INDEX;
		}
		break;
	case MATERIAL_VERTEX_FORMAT_COLOR:
		// This is the format for the color stream for static props
		fmt = VERTEX_SPECULAR;
		break;
	default:
		fmt = VERTEX_POSITION;
		Assert(0);
		break;
	}
	return fmt;
}


//-----------------------------------------------------------------------------
// Displays the vertex format
//-----------------------------------------------------------------------------
static void OutputVertexFormat( VertexFormat_t format )
{
	if( format & VERTEX_POSITION )
	{
		Warning( "VERTEX_POSITION|" );
	}
	if( format & VERTEX_NORMAL )
	{
		Warning( "VERTEX_NORMAL|" );
	}
	if( format & VERTEX_COLOR )
	{
		Warning( "VERTEX_COLOR|" );
	}
	if( format & VERTEX_SPECULAR )
	{
		Warning( "VERTEX_SPECULAR|" );
	}
	if( format & VERTEX_TANGENT_S )
	{
		Warning( "VERTEX_TANGENT_S|" );
	}
	if( format & VERTEX_TANGENT_T )
	{
		Warning( "VERTEX_TANGENT_T|" );
	}
	if( format & VERTEX_BONE_INDEX )
	{
		Warning( "VERTEX_BONE_INDEX|" );
	}
	if( format & VERTEX_FORMAT_VERTEX_SHADER )
	{
		Warning( "VERTEX_FORMAT_VERTEX_SHADER|" );
	}
}


//-----------------------------------------------------------------------------
// Checks if it's a valid format
//-----------------------------------------------------------------------------
static bool IsValidVertexFormat( VertexFormat_t format, VertexFormat_t usage )
{
	int flags = VertexFlags( usage ) & ~VertexFlags(format);
	if (flags != 0)
	{
		Warning( "Format needs: " );
		OutputVertexFormat( format );
		Warning( "\nFormat has: " );
		OutputVertexFormat( usage );
		Warning( "\nFormat missing: " );
		OutputVertexFormat( flags );
		Warning( "\n" );
		return false;
	}

	if (NumBoneWeights(usage) > NumBoneWeights(format))
	{
		// If we want 3 bones, the third one is implied by the first two, so it's
		// OK to only have two bones in the real format.
		if( !( NumBoneWeights( usage ) == 3 && NumBoneWeights( format ) == 2 ) )
		{
			return false;
		}
	}

	if (UserDataSize(usage) > UserDataSize(format))
		return false;

	int numTexCoord = NumTextureCoordinates(usage);
	if (numTexCoord > NumTextureCoordinates(format))
		return false;

	for (int i = 0; i < numTexCoord; ++i)
	{
		if (TexCoordSize(i, usage) != TexCoordSize(i, format))
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Computes vertex format + usage from a particular snapshot
//-----------------------------------------------------------------------------
bool CShaderSystem::ComputeVertexFormatFromSnapshot( IMaterialVar **params, ShaderRenderState_t* pRenderState )
{
	// When computing the usage, use the snapshot that has no alpha or color
	// modulation. We need the usage + format to be the same for all
	// combinations of alpha + color modulation, though, or we are asking for
	// trouble.

	int numSnapshots = pRenderState->m_Snapshots[0].m_nPassCount;
	StateSnapshot_t* pSnapshots = (StateSnapshot_t*)stackalloc( 
		numSnapshots * sizeof(StateSnapshot_t) ); 
	for (int i = 0; i < numSnapshots; ++i)
	{
		pSnapshots[i] = pRenderState->m_Snapshots[0].m_Snapshot[i];
	}

	pRenderState->m_VertexUsage = g_pShaderAPI->ComputeVertexUsage( 
		numSnapshots, pSnapshots );

#ifdef _DEBUG
	// Make sure all modulation combinations match vertex usage
	for (int mod = 1; mod < 4; ++mod)
	{
		int numSnapshotsTest = pRenderState->m_Snapshots[mod].m_nPassCount;
		StateSnapshot_t* pSnapshotsTest = (StateSnapshot_t*)_alloca( 
			numSnapshotsTest * sizeof(StateSnapshot_t) );

		for (int i = 0; i < numSnapshotsTest; ++i)
		{
			pSnapshotsTest[i] = pRenderState->m_Snapshots[mod].m_Snapshot[i];
		}

		int usageTest = g_pShaderAPI->ComputeVertexUsage( 
			numSnapshotsTest, pSnapshotsTest );
		Assert( (usageTest & 0x7FFFFFFF) == (pRenderState->m_VertexUsage & 0x7FFFFFFF) );
	}
#endif

	// If we're not rendering models, just pick a vertex format
	// that has good alignment and contains all fields in vertex usage.
	if ( (params[FLAGS]->GetIntValue() & MATERIAL_VAR_MODEL) == 0 )
	{
		pRenderState->m_VertexFormat = g_pShaderAPI->ComputeVertexFormat( 
			numSnapshots, pSnapshots );
	}
	else
	{
		pRenderState->m_VertexFormat = GetVertexFormat( MATERIAL_VERTEX_FORMAT_MODEL );

		// Makes fixed-function shaders work with DX8 parts
		// assuming skinning rules aren't violated by the data
		if (pRenderState->m_VertexUsage & VERTEX_FORMAT_VERTEX_SHADER)
			pRenderState->m_VertexFormat |= VERTEX_FORMAT_VERTEX_SHADER;

		if (!IsValidVertexFormat(pRenderState->m_VertexFormat, pRenderState->m_VertexUsage ))
		{
			// Bad format! something's screwy...
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// go through each param and make sure it is the right type, load textures, 
// compute state snapshots and vertex types, etc.
//-----------------------------------------------------------------------------
bool CShaderSystem::InitRenderState( IShader *pShader, int numParams, IMaterialVar **params, ShaderRenderState_t* pRenderState, char const* pMaterialName )
{
	Assert( !m_pRenderState );

	// Initialize render state flags
	InitRenderStateFlags( pRenderState, numParams, params );

	// Compute state snapshots for each combination of alpha + color
	InitStateSnapshots( pShader, params, pRenderState );

	// Compute other infomation for the render state based on snapshots
	if (pRenderState->m_Snapshots[0].m_nPassCount == 0)
	{
		Warning( "Material \"%s\":\n   No render states in shader \"%s\"\n", pMaterialName, pShader->GetName() );
		return false;
	}

	// Set a couple additional flags based on the render state
	ComputeRenderStateFlagsFromSnapshot( pRenderState );

	// Compute the vertex format + usage from the snapshot
	if ( !ComputeVertexFormatFromSnapshot( params, pRenderState ) )
	{
		// warn.. return a null render state...
		Warning("Material \"%s\":\n   Shader \"%s\" can't be used with models!\n", pMaterialName, pShader->GetName() );
		CleanupRenderState( pRenderState );
		return false;
	}
	return true;
}

// When you're done with the shader, be sure to call this to clean up
void CShaderSystem::CleanupRenderState( ShaderRenderState_t* pRenderState )
{
	if (pRenderState)
	{
#ifdef _DEBUG
		// Make sure we don't use these again...
		memset( pRenderState, 0xdd, sizeof(ShaderRenderState_t) );
#endif

		// Indicate no passes for any of the snapshot lists
		for ( int i = 0; i < 4; ++i )
		{
			pRenderState->m_Snapshots[i].m_nPassCount = 0;
		}
	}
}


//-----------------------------------------------------------------------------
// Does the rendering!
//-----------------------------------------------------------------------------
void CShaderSystem::DrawElements( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pRenderState )
{
	// Compute modulation...
	int mod = pShader->ComputeModulationFlags( params );

	// No snapshots? do nothing.
	if (pRenderState->m_Snapshots[mod].m_nPassCount == 0)
		return;

	// If we're rendering a model, gotta have skinning matrices
	int materialVarFlags = params[FLAGS]->GetIntValue();
	if (materialVarFlags & MATERIAL_VAR_MODEL)
	{
		g_pShaderAPI->SetSkinningMatrices( );
	}

	if ( (g_config.bMeasureFillRate || g_config.bVisualizeFillRate)
		&& ((materialVarFlags & MATERIAL_VAR_USE_IN_FILLRATE_MODE) == 0) )
	{
		DrawMeasureFillRate( pRenderState, mod );
	}
	else if( g_config.bShowNormalMap && IsFlag2Set( params, MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP ) )
	{
		DrawNormalMap( pShader, params );
	}
	else if (!g_config.bLightingOnly ||
			 ((materialVarFlags & MATERIAL_VAR_NO_DEBUG_OVERRIDE) != 0) )
	{
		g_pShaderAPI->SetDefaultState();

		// If we're rendering flat, turn on flat mode...
		if (materialVarFlags & MATERIAL_VAR_FLAT)
		{
			g_pShaderAPI->ShadeMode( SHADER_FLAT );
		}
	
		PrepForShaderDraw( pShader, params, pRenderState, mod );
		g_pShaderAPI->BeginPass( CurrentStateSnapshot() );
		pShader->DrawElements( params, mod, 0, g_pShaderAPI );
		DoneWithShaderDraw();
	}
	else if (g_config.bLightingOnly)
	{
		DrawElementsLightingOnly( pShader, params, pRenderState );
	}
	else
	{
		Assert( 0 );
	}

	MaterialSystem()->ForceDepthFuncEquals( false );
}


//-----------------------------------------------------------------------------
// Are we using graphics?
//-----------------------------------------------------------------------------
bool CShaderSystem::IsUsingGraphics() const
{
	return g_pShaderAPI->IsUsingGraphics();
}


//-----------------------------------------------------------------------------
// Takes a snapshot
//-----------------------------------------------------------------------------
void CShaderSystem::TakeSnapshot( )
{
	Assert( m_pRenderState );
	RenderPassList_t& snapshotList = m_pRenderState->m_Snapshots[m_nModulation];

	// Take a snapshot...
	snapshotList.m_Snapshot[snapshotList.m_nPassCount] = g_pShaderAPI->TakeSnapshot();
	++snapshotList.m_nPassCount;
}


//-----------------------------------------------------------------------------
// Draws a snapshot
//-----------------------------------------------------------------------------
void CShaderSystem::DrawSnapshot( )
{
	Assert( m_pRenderState );
	RenderPassList_t& snapshotList = m_pRenderState->m_Snapshots[m_nModulation];

	int nPassCount = snapshotList.m_nPassCount;
	Assert( m_nRenderPass < nPassCount );

	g_pShaderAPI->RenderPass( );

	if (++m_nRenderPass < nPassCount)
	{
		g_pShaderAPI->BeginPass( CurrentStateSnapshot() );
	}
}



//-----------------------------------------------------------------------------
//
// Debugging material methods below
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Draws a using a particular material..
//-----------------------------------------------------------------------------
void CShaderSystem::DrawUsingMaterial( IMaterialInternal *pMaterial )
{
	ShaderRenderState_t *pRenderState = pMaterial->GetRenderState();
	g_pShaderAPI->SetDefaultState( );

	IShader *pShader = pMaterial->GetShader();
	int nMod = pShader->ComputeModulationFlags( pMaterial->GetShaderParams() );
	PrepForShaderDraw( pShader, pMaterial->GetShaderParams(), pRenderState, nMod );
	g_pShaderAPI->BeginPass( pRenderState->m_Snapshots[nMod].m_Snapshot[0] );
	pShader->DrawElements( pMaterial->GetShaderParams(), nMod, 0, g_pShaderAPI );
	DoneWithShaderDraw( );
}


//-----------------------------------------------------------------------------
// Copies material vars
//-----------------------------------------------------------------------------
void CShaderSystem::CopyMaterialVarToDebugShader( IMaterialInternal *pDebugMaterial, IShader *pShader, IMaterialVar **ppParams, const char *pVarName )
{
	bool bFound;
	IMaterialVar *pMaterialVar = pDebugMaterial->FindVar( pVarName, &bFound );
	Assert( bFound );

	for( int i = pShader->GetNumParams(); --i >= 0; )
	{
		if( !Q_stricmp( ppParams[i]->GetName( ), pVarName ) )
		{
			pMaterialVar->CopyFrom( ppParams[i] );
			return;
		}
	}

	pMaterialVar->SetUndefined();
}


//-----------------------------------------------------------------------------
// Draws the puppy in fill rate mode...
//-----------------------------------------------------------------------------
void CShaderSystem::DrawMeasureFillRate( ShaderRenderState_t* pRenderState, int mod )
{
	int nPassCount = pRenderState->m_Snapshots[mod].m_nPassCount;

	bool bUsesVertexShader = (VertexFlags(pRenderState->m_VertexFormat) & VERTEX_FORMAT_VERTEX_SHADER) != 0;
	IMaterialInternal *pMaterial = m_pDebugMaterials[ bUsesVertexShader ? MATERIAL_FILL_RATE : MATERIAL_FILL_RATE_DX6 ];

	bool bFound;
	IMaterialVar *pMaterialVar = pMaterial->FindVar( "$passcount", &bFound );
	pMaterialVar->SetIntValue( nPassCount );
	DrawUsingMaterial( pMaterial );
}


//-----------------------------------------------------------------------------
// Draws normalmaps
//-----------------------------------------------------------------------------
void CShaderSystem::DrawNormalMap( IShader *pShader, IMaterialVar **ppParams )
{
	IMaterialInternal *pDebugMaterial = m_pDebugMaterials[ 
		IsFlagSet( ppParams, MATERIAL_VAR_MODEL ) ? MATERIAL_DEBUG_NORMALMAP_MODEL : MATERIAL_DEBUG_NORMALMAP ];
	
	if( !g_config.m_bFastNoBump )
	{
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumpmap" );
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumpframe" );
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumptransform" );
	}
	else
	{
		bool bFound;
		IMaterialVar *pMaterialVar = pDebugMaterial->FindVar( "$bumpmap", &bFound );
		Assert( bFound );

		pMaterialVar->SetUndefined();
	}

	DrawUsingMaterial( pDebugMaterial );
}


//-----------------------------------------------------------------------------
// Draws vertex lit only.
//-----------------------------------------------------------------------------
void CShaderSystem::DrawVertexLitOnly( bool usesVertexShader )
{
	int nShaderIndex;
	if ( g_config.bSoftwareLighting || (!usesVertexShader) )
	{
		nShaderIndex = MATERIAL_DEBUG_VERTEXLIT_FF;
	}
	else
	{
		nShaderIndex = MATERIAL_DEBUG_VERTEXLIT_VS;
	}
	DrawUsingMaterial( m_pDebugMaterials[nShaderIndex] );
}


//-----------------------------------------------------------------------------
// Draws bumped vertex lit only.
//-----------------------------------------------------------------------------
void CShaderSystem::DrawBumpedModelLightingOnly( IShader *pShader, IMaterialVar **ppParams )
{
	bool bHasSelfIllum = IsFlagSet( ppParams, MATERIAL_VAR_SELFILLUM );
	int nShaderIndex = bHasSelfIllum ? MATERIAL_DEBUG_BUMPED_VERTEXLIT_SELF_ILLUM : MATERIAL_DEBUG_BUMPED_VERTEXLIT; 
	
	IMaterialInternal *pDebugMaterial = m_pDebugMaterials[ nShaderIndex ];	
	if( !g_config.m_bFastNoBump )
	{
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumpmap" );
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumpframe" );
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumptransform" );
	}
	else
	{
		bool bFound;
		IMaterialVar *pMaterialVar = pDebugMaterial->FindVar( "$bumpmap", &bFound );
		Assert( bFound );

		pMaterialVar->SetUndefined();
	}

	CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$selfillumtint" );

	bool bFound;
	IMaterialVar *pMaterialVar = pDebugMaterial->FindVar( "$hasselfillum", &bFound );
	Assert( bFound );
	pMaterialVar->SetIntValue( bHasSelfIllum );

	DrawUsingMaterial( pDebugMaterial );
}


//-----------------------------------------------------------------------------
// Draws lightmapped only.
//-----------------------------------------------------------------------------
void CShaderSystem::DrawLightmappedOnly( bool usesVertexShader )
{
	IMaterialInternal *pDebugMaterial = m_pDebugMaterials[ usesVertexShader ? MATERIAL_DEBUG_LIGHTMAP_VS : MATERIAL_DEBUG_LIGHTMAP_FF ];
	DrawUsingMaterial( pDebugMaterial );
}


//-----------------------------------------------------------------------------
// Draws bumped lightmapped only.
//-----------------------------------------------------------------------------
void CShaderSystem::DrawBumpedLightmapOnly( IShader *pShader, IMaterialVar **ppParams )
{
	if (!HardwareConfig()->SupportsVertexAndPixelShaders())
	{
		Assert( 0 );
	}

	IMaterialInternal *pDebugMaterial = m_pDebugMaterials[ MATERIAL_DEBUG_BUMPED_LIGHTMAP ];
	if( !g_config.m_bFastNoBump )
	{
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumpmap" );
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumpframe" );
		CopyMaterialVarToDebugShader( pDebugMaterial, pShader, ppParams, "$bumptransform" );
	}
	else
	{
		bool bFound;
		IMaterialVar *pMaterialVar = pDebugMaterial->FindVar( "$bumpmap", &bFound );
		Assert( bFound );

		pMaterialVar->SetUndefined();
	}

	DrawUsingMaterial( pDebugMaterial );
}


//-----------------------------------------------------------------------------
// Draws unlit only.for models
//-----------------------------------------------------------------------------
void CShaderSystem::DrawUnlitOnly( IMaterialVar **params, bool usesVertexShader )
{
	int nShaderIndex;
	if ( usesVertexShader )
	{
		nShaderIndex = IsFlagSet( params, MATERIAL_VAR_MODEL ) ? 
			MATERIAL_DEBUG_UNLIT_VS_MODEL : MATERIAL_DEBUG_UNLIT_VS;
	}
	else
	{
		nShaderIndex = IsFlagSet( params, MATERIAL_VAR_MODEL ) ? 
			MATERIAL_DEBUG_UNLIT_FF_MODEL : MATERIAL_DEBUG_UNLIT_FF;
	}
	DrawUsingMaterial( m_pDebugMaterials[nShaderIndex] );
}


//-----------------------------------------------------------------------------
// Draws the puppy with lighting only...
//-----------------------------------------------------------------------------
void CShaderSystem::DrawElementsLightingOnly( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pRenderState )
{
	int vertexFormat = pRenderState->m_VertexFormat;
	bool bUsesVertexShader = (VertexFlags(vertexFormat) & VERTEX_FORMAT_VERTEX_SHADER) != 0;

	// Deal with lighting only
	if( g_config.bBumpmap && IsFlag2Set( params, MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP ) )
	{
		DrawBumpedLightmapOnly( pShader, params );
	}
	else if( IsFlag2Set( params, MATERIAL_VAR2_LIGHTING_LIGHTMAP ) )
	{
		DrawLightmappedOnly( bUsesVertexShader );
	}
	else if( g_config.bBumpmap && IsFlag2Set( params, MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL ) )
	{
		DrawBumpedModelLightingOnly( pShader, params );
	}
	else if( IsFlag2Set( params, MATERIAL_VAR2_LIGHTING_VERTEX_LIT ) )
	{
		DrawVertexLitOnly( bUsesVertexShader );
	}
	else
	{
		DrawUnlitOnly( params, bUsesVertexShader );
	}
}


//-----------------------------------------------------------------------------
//
// Methods of IShaderSystem lie below
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Binds a texture
//-----------------------------------------------------------------------------
void CShaderSystem::BindTexture( TextureStage_t stage, ITexture *pTexture, int nFrame )
{
	if ( pTexture != ( ITexture * )-1 )
	{
		// Bind away baby
		if( pTexture )
		{
			static_cast<ITextureInternal*>(pTexture)->Bind( stage, nFrame );
		}
	}
	else
	{
		static_cast<ITextureInternal*>(MaterialSystem()->GetLocalCubemap())->Bind( stage, nFrame );
	}
}


//-----------------------------------------------------------------------------
//
// Methods of IShaderInit lie below
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Loads a texture
//-----------------------------------------------------------------------------
void CShaderSystem::LoadTexture( IMaterialVar *pTextureVar )
{
	if (pTextureVar->GetType() != MATERIAL_VAR_TYPE_STRING)
	{
		// This here will cause 'UNDEFINED' material vars
		if (pTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE)
		{
			pTextureVar->SetTextureValue( TextureManager()->ErrorTexture() );
		}
		return;
	}

	// In this case, we have to convert the string into a texture value
	const char *pName = pTextureVar->GetStringValue();

	ITextureInternal *pTexture;
	pTexture = TextureManager()->FindOrLoadTexture( pName );

	if( !pTexture )
	{
		if( !g_pShaderAPI->IsUsingGraphics() && ( stricmp( pName, "env_cubemap" ) != 0 ) )
		{
			Warning( "Shader_t::LoadTexture: texture \"%s.vtf\" doesn't exist\n", pName );
		}
		pTexture = TextureManager()->ErrorTexture();
	}

	pTextureVar->SetTextureValue( pTexture );
}


//-----------------------------------------------------------------------------
// Loads a bumpmap
//-----------------------------------------------------------------------------
void CShaderSystem::LoadBumpMap( IMaterialVar *pTextureVar )
{
	Assert( pTextureVar );

	if (pTextureVar->GetType() != MATERIAL_VAR_TYPE_STRING)
	{
		// This here will cause 'UNDEFINED' material vars
		if (pTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE)
		{
			pTextureVar->SetTextureValue( TextureManager()->ErrorTexture() );
		}
		return;
	}

	// Convert a string to the actual texture
	ITextureInternal *pTexture;
	pTexture = TextureManager()->FindOrLoadTexture( pTextureVar->GetStringValue(), true );

	// FIXME: Make a bumpmap error texture
	if (!pTexture)
	{
		pTexture = TextureManager()->ErrorTexture();
	}

	pTextureVar->SetTextureValue( pTexture );
}


//-----------------------------------------------------------------------------
// Loads a cubemap
//-----------------------------------------------------------------------------
void CShaderSystem::LoadCubeMap( IMaterialVar **ppParams, IMaterialVar *pTextureVar )
{
	if( !HardwareConfig()->SupportsCubeMaps() )
		return;
	
	if (pTextureVar->GetType() != MATERIAL_VAR_TYPE_STRING)
	{
		// This here will cause 'UNDEFINED' material vars
		if (pTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE)
		{
			pTextureVar->SetTextureValue( TextureManager()->ErrorTexture() );
		}
		return;
	}

	if( stricmp( pTextureVar->GetStringValue(), "env_cubemap" ) == 0 )
	{
		// garymcthack 
		// don't have to load anything here. . just set the texture value to something
		// special that says to use the cubemap entity.
		pTextureVar->SetTextureValue( ( ITexture * )-1 );
		SetFlags2( ppParams, MATERIAL_VAR2_USES_ENV_CUBEMAP );
	}
	else
	{
		ITextureInternal *pTexture;
		pTexture = TextureManager()->FindOrLoadTexture( pTextureVar->GetStringValue(), false );

		// FIXME: Make a cubemap error texture
		if (!pTexture)
		{
			pTexture = TextureManager()->ErrorTexture();
		}

		pTextureVar->SetTextureValue( pTexture );
	}
}



