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

#include "shaderlib/ShaderDLL.h"
#include "materialsystem/IShader.h"
#include "utlvector.h"
#include "tier0/dbg.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/materialsystem_config.h"
#include "materialsystem/IShaderSystem.h"
#include "materialsystem/ishaderapi.h"
#include "shaderlib_cvar.h"


//-----------------------------------------------------------------------------
// The standard implementation of IPrecompiledShaderDictionary
//-----------------------------------------------------------------------------
class CPrecompiledShaderDictionary : public IPrecompiledShaderDictionary
{
public:
	// Gets at the precompiled shaders defined in this DLL
	virtual int ShaderCount() const
	{
		return m_ShaderList.Count();
	}

	virtual const PrecompiledShader_t *GetShader( int nShader ) const
	{
		if ( ( nShader < 0 ) || ( nShader >= ShaderCount() ) )
			return NULL;

		return m_ShaderList[nShader];
	}

	virtual	PrecompiledShaderType_t GetType() const
	{
		return m_nType;
	}

public:
	void InsertPrecompiledShader( const PrecompiledShader_t *pVertexShader )
	{
		m_ShaderList.AddToTail( pVertexShader );
	}

	void SetType( PrecompiledShaderType_t type )
	{
		m_nType = type;
	}

private:
	PrecompiledShaderType_t m_nType;
	CUtlVector< const PrecompiledShader_t * >	m_ShaderList;
};


//-----------------------------------------------------------------------------
// The standard implementation of CShaderDLL
//-----------------------------------------------------------------------------
class CShaderDLL : public IShaderDLLInternal, public IShaderDLL
{
public:
	CShaderDLL();

	// methods of IShaderDLL
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual int ShaderCount() const;
	virtual IShader *GetShader( int nShader );
	virtual int ShaderDictionaryCount() const;
	virtual IPrecompiledShaderDictionary *GetShaderDictionary( int nDictIndex );

	// methods of IShaderDLLInternal
	virtual void InsertShader( IShader *pShader );
	virtual void InsertPrecompiledShader( PrecompiledShaderType_t type, const PrecompiledShader_t *pVertexShader );

private:
	CUtlVector< IShader * >	m_ShaderList;
	CPrecompiledShaderDictionary m_PrecompiledShaderList[PRECOMPILED_SHADER_TYPE_COUNT];
};


//-----------------------------------------------------------------------------
// Global interfaces/structures
//-----------------------------------------------------------------------------
IMaterialSystemHardwareConfig* g_pHardwareConfig;
const MaterialSystem_Config_t *g_pConfig;


//-----------------------------------------------------------------------------
// Interfaces/structures local to shaderlib
//-----------------------------------------------------------------------------
IShaderSystem* g_pSLShaderSystem;


// Pattern necessary because shaders register themselves in global constructors
static CShaderDLL *s_pShaderDLL;


//-----------------------------------------------------------------------------
// Global accessor
//-----------------------------------------------------------------------------
IShaderDLL *GetShaderDLL()
{
	// Pattern necessary because shaders register themselves in global constructors
	if ( !s_pShaderDLL )
	{
		s_pShaderDLL = new CShaderDLL;
	}

	return s_pShaderDLL;
}

IShaderDLLInternal *GetShaderDLLInternal()
{
	// Pattern necessary because shaders register themselves in global constructors
	if ( !s_pShaderDLL )
	{
		s_pShaderDLL = new CShaderDLL;
	}

	return static_cast<IShaderDLLInternal*>( s_pShaderDLL );
}

//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
EXPOSE_INTERFACE_FN( (InstantiateInterfaceFn)GetShaderDLLInternal, IShaderDLLInternal, SHADER_DLL_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
CShaderDLL::CShaderDLL()
{
	for ( int i = 0; i < PRECOMPILED_SHADER_TYPE_COUNT; ++i )
	{
		m_PrecompiledShaderList[i].SetType( (PrecompiledShaderType_t)i );
	}
}


//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
bool CShaderDLL::Connect( CreateInterfaceFn factory )
{
	g_pHardwareConfig =  (IMaterialSystemHardwareConfig*)factory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, NULL );
	g_pConfig = (const MaterialSystem_Config_t*)factory( MATERIALSYSTEM_CONFIG_VERSION, NULL );
	g_pSLShaderSystem =  (IShaderSystem*)factory( SHADERSYSTEM_INTERFACE_VERSION, NULL );
  	InitShaderLibCVars( factory );

	return ( g_pConfig != NULL ) && (g_pHardwareConfig != NULL) && ( g_pSLShaderSystem != NULL );
}

void CShaderDLL::Disconnect()
{
	g_pHardwareConfig = NULL;
	g_pConfig = NULL;
	g_pSLShaderSystem = NULL;
}


//-----------------------------------------------------------------------------
// Iterates over all shaders
//-----------------------------------------------------------------------------
int CShaderDLL::ShaderCount() const
{
	return m_ShaderList.Count();
}

IShader *CShaderDLL::GetShader( int nShader ) 
{
	if ( ( nShader < 0 ) || ( nShader >= m_ShaderList.Count() ) )
		return NULL;

	return m_ShaderList[nShader];
}


//-----------------------------------------------------------------------------
// Precompiled shaders
//-----------------------------------------------------------------------------
int CShaderDLL::ShaderDictionaryCount() const
{
	return PRECOMPILED_SHADER_TYPE_COUNT;
}

IPrecompiledShaderDictionary *CShaderDLL::GetShaderDictionary( int nDictIndex )
{
	if ( nDictIndex >= PRECOMPILED_SHADER_TYPE_COUNT )
		return NULL;

	return &m_PrecompiledShaderList[nDictIndex];
}


//-----------------------------------------------------------------------------
// Adds to the shader lists
//-----------------------------------------------------------------------------
void CShaderDLL::InsertShader( IShader *pShader )
{
	Assert( pShader );
	m_ShaderList.AddToTail( pShader );
}

void CShaderDLL::InsertPrecompiledShader( PrecompiledShaderType_t type, const PrecompiledShader_t *pShader )
{
	if ( type < PRECOMPILED_SHADER_TYPE_COUNT )
	{
		Assert( pShader );
		m_PrecompiledShaderList[type].InsertPrecompiledShader( pShader );
	}
}
