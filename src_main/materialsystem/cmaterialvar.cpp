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
// All of the material variables loaded from a .VMT file
//=============================================================================

#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/itexture.h"
#include <string.h>
#include "materialsystem_global.h"
#include <stdlib.h>
#include "shaderapi.h"
#include "imaterialinternal.h"
#include "utlsymbol.h"
#include "mempool.h"
#include "itextureinternal.h"
#include "tier0/dbg.h"
#include "vmatrix.h"
#include "vstdlib/strtools.h"
#include "texturemanager.h"

#define MATERIALVAR_CHAR_BUF_SIZE 512

#pragma pack (1)

struct MaterialVarMatrix_t
{
	VMatrix m_Matrix;

	// Fixed-size allocator
	DECLARE_FIXEDSIZE_ALLOCATOR( MaterialVarMatrix_t );
};

class CMaterialVar : public IMaterialVar
{
public:
	// stuff from IMaterialVar
	virtual char const *		GetName( void ) const;
	virtual MaterialVarSym_t	GetNameAsSymbol() const;
	virtual void				SetFloatValue( float val );
	virtual float				GetFloatValue( void );
	virtual void				SetIntValue( int val );
	virtual int					GetIntValue( void ) const;
	virtual void				SetStringValue( char const *val );
	virtual char const *		GetStringValue( void ) const;
	virtual void				SetMatrixValue( VMatrix const& matrix );
	virtual VMatrix const&		GetMatrixValue( );
	virtual void				SetVecValue( const float* pVal, int numComps );
	virtual void				SetVecValue( float x, float y );
	virtual void				SetVecValue( float x, float y, float z );
	virtual void				SetVecValue( float x, float y, float z, float w );
	virtual void				GetVecValue( float *val, int numComps ) const;
	virtual float const*		GetVecValue() const;
	virtual void				SetFourCCValue( FourCC type, void *pData );
	virtual void				GetFourCCValue( FourCC *type, void **ppData );
	virtual int					VectorSize() const;

	// revisit: is this a good interface for textures?

	virtual ITexture *			GetTextureValue( void );
	virtual void				SetTextureValue( ITexture * );
	virtual IMaterial *			GetMaterialValue( void );
	virtual void				SetMaterialValue( IMaterial * );

	virtual MaterialVarType_t	GetType() const;

	virtual 					operator ITexture *() { return GetTextureValue(); }
	virtual bool				IsDefined() const;
	virtual void				SetUndefined();

	virtual void				CopyFrom( IMaterialVar *pMaterialVar );

	// stuff that is only visible inside of the material system
	CMaterialVar( IMaterial* pMaterial, char const *key, VMatrix const& matrix );
	CMaterialVar( IMaterial* pMaterial, char const *key, char const *val );
	CMaterialVar( IMaterial* pMaterial, char const *key, float* pVal, int numcomps );
	CMaterialVar( IMaterial* pMaterial, char const *key, float val );
	CMaterialVar( IMaterial* pMaterial, char const *key, int val );
	CMaterialVar( IMaterial* pMaterial, char const *key );
	virtual ~CMaterialVar();

private:
	// Cleans up material var data
	void CleanUpData();

	// class data
	static char s_CharBuf[MATERIALVAR_CHAR_BUF_SIZE];
	static ITextureInternal *m_dummyTexture;

	// Fixed-size allocator
	DECLARE_FIXEDSIZE_ALLOCATOR( CMaterialVar );

	// Owning material....
	IMaterialInternal* m_pMaterial;

	// member data
	CUtlSymbol m_Name;
	unsigned char m_NumComps;
	unsigned char m_Type;

	// Only using one of these at a time...
	// FIXME: Remove Vector4D and replace with index into a
	// fixed-size Vector4D allocator
	union
	{
		char* m_pStringVal;
		float m_floatVal;
		ITextureInternal* m_pTexture;
		IMaterialInternal* m_pMaterialValue;
		MaterialVarMatrix_t* m_pMatrix;
		float m_VecVal[4];
		int m_intVal;
		struct 
		{
			FourCC	m_FourCC;
			void	*m_pFourCCData;
		} m_FourCC;
	};
};

// Has to exist *after* fixed size allocator declaration
#include "tier0/memdbgon.h"

typedef CMaterialVar *CMaterialVarPtr;

DEFINE_FIXEDSIZE_ALLOCATOR( CMaterialVar, 1024, true );
DEFINE_FIXEDSIZE_ALLOCATOR( MaterialVarMatrix_t, 32, true );

// Stores symbols for the material vars
static CUtlSymbolTable s_MaterialVarSymbols;


//-----------------------------------------------------------------------------
// class factory methods
//-----------------------------------------------------------------------------
IMaterialVar* IMaterialVar::Create( IMaterial* pMaterial, char const* pKey, VMatrix const& matrix )
{
	return new CMaterialVar( pMaterial, pKey, matrix );
}

IMaterialVar* IMaterialVar::Create( IMaterial* pMaterial, char const* pKey, char const* pVal )
{
	return new CMaterialVar( pMaterial, pKey, pVal );
}

IMaterialVar* IMaterialVar::Create( IMaterial* pMaterial, char const* pKey, float* pVal, int numComps )
{
	return new CMaterialVar( pMaterial, pKey, pVal, numComps );
}

IMaterialVar* IMaterialVar::Create( IMaterial* pMaterial, char const* pKey, float val )
{
	return new CMaterialVar( pMaterial, pKey, val );
}

IMaterialVar* IMaterialVar::Create( IMaterial* pMaterial, char const* pKey, int val )
{
	return new CMaterialVar( pMaterial, pKey, val );
}

IMaterialVar* IMaterialVar::Create( IMaterial* pMaterial, char const* pKey )
{
	return new CMaterialVar( pMaterial, pKey );
}

void IMaterialVar::Destroy( IMaterialVar* pVar )
{
	if (pVar)
	{
		CMaterialVar* pVarImp = static_cast<CMaterialVar*>(pVar);
		delete pVarImp;
	}
}

MaterialVarSym_t IMaterialVar::GetSymbol( char const* pName )
{
	if (!pName)
		return UTL_INVAL_SYMBOL;

	char temp[1024];
	strcpy( temp, pName );
	Q_strlower( temp );
	return s_MaterialVarSymbols.AddString( temp );
}


//-----------------------------------------------------------------------------
// class globals
//-----------------------------------------------------------------------------
char CMaterialVar::s_CharBuf[MATERIALVAR_CHAR_BUF_SIZE];


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
CMaterialVar::CMaterialVar( IMaterial* pMaterial, char const *pKey, VMatrix const& matrix )
{
	Assert( pKey );

	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);
	m_Name = GetSymbol( pKey );
	Assert( m_Name != UTL_INVAL_SYMBOL );
	m_Type = MATERIAL_VAR_TYPE_MATRIX;
	m_pMatrix = new MaterialVarMatrix_t;
	Assert( m_pMatrix );
	MatrixCopy( matrix, m_pMatrix->m_Matrix );
}

CMaterialVar::CMaterialVar( IMaterial* pMaterial, char const *pKey, char const *pVal )
{
	Assert( pVal && pKey );

	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);
	m_Name = GetSymbol( pKey );
	Assert( m_Name != UTL_INVAL_SYMBOL );
	m_pStringVal = new char[strlen( pVal ) + 1];
	strcpy( m_pStringVal, pVal );
	m_Type = MATERIAL_VAR_TYPE_STRING;
}

CMaterialVar::CMaterialVar( IMaterial* pMaterial, char const *pKey, float* pVal, int numComps )
{
	Assert( pVal && pKey && (numComps <= 4) );

	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);;
	m_Name = GetSymbol( pKey );
	Assert( m_Name != UTL_INVAL_SYMBOL );
	m_Type = MATERIAL_VAR_TYPE_VECTOR;
	memcpy( &m_VecVal, pVal, numComps * sizeof(float) );
	for (int i = numComps; i < 4; ++i)
		m_VecVal[i] = 0.0f;

	m_NumComps = numComps;
}

CMaterialVar::CMaterialVar( IMaterial* pMaterial, char const *pKey, float val )
{
	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);
	m_Name = GetSymbol( pKey );
	Assert( m_Name != UTL_INVAL_SYMBOL );
	m_Type = MATERIAL_VAR_TYPE_FLOAT;
	m_floatVal = val;
}

CMaterialVar::CMaterialVar( IMaterial* pMaterial, char const *pKey, int val )
{
	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);
	m_Name = GetSymbol( pKey );
	Assert( m_Name != UTL_INVAL_SYMBOL );
	m_Type = MATERIAL_VAR_TYPE_INT;
	m_intVal = val;
}

CMaterialVar::CMaterialVar( IMaterial* pMaterial, char const *pKey )
{
	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);
	m_Name = GetSymbol( pKey );
	Assert( m_Name != UTL_INVAL_SYMBOL );
	m_Type = MATERIAL_VAR_TYPE_UNDEFINED;
}


//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
CMaterialVar::~CMaterialVar()
{
	CleanUpData();
}


//-----------------------------------------------------------------------------
// Cleans up material var allocated data if necessary
//-----------------------------------------------------------------------------
void CMaterialVar::CleanUpData()
{
	switch (m_Type)
	{
	case MATERIAL_VAR_TYPE_STRING:
		if (m_pStringVal)
		{
			delete [] m_pStringVal;
		}
		break;

	case MATERIAL_VAR_TYPE_TEXTURE:
		// garymcthack
		if( m_pTexture != ( ITextureInternal * )-1 )
		{
			m_pTexture->DecrementReferenceCount();
		}
		break;

	case MATERIAL_VAR_TYPE_MATERIAL:
		if( m_pMaterialValue != NULL )
		{
			m_pMaterialValue->DecrementReferenceCount();
		}
		break;

	case MATERIAL_VAR_TYPE_MATRIX:
		if (m_pMatrix)
		{
			delete m_pMatrix;
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// name	+ type
//-----------------------------------------------------------------------------
MaterialVarSym_t CMaterialVar::GetNameAsSymbol() const
{
	return m_Name;
}

char const *CMaterialVar::GetName( ) const
{
	if( !m_Name.IsValid() )
	{
		Warning( "m_pName is NULL for CMaterialVar\n" );
		return "";
	}
	return s_MaterialVarSymbols.String( m_Name );
}

MaterialVarType_t CMaterialVar::GetType( void ) const
{
	return (MaterialVarType_t)m_Type;
}


//-----------------------------------------------------------------------------
// float
//-----------------------------------------------------------------------------
void CMaterialVar::SetFloatValue( float val )
{
	// Suppress all this if we're not actually changing anything
	if ((m_Type == MATERIAL_VAR_TYPE_FLOAT) && (m_floatVal == val))
		return;

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == ( IMaterialInternal* )MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_floatVal = val;
	m_Type = MATERIAL_VAR_TYPE_FLOAT;
}

float CMaterialVar::GetFloatValue( void )
{
	switch( m_Type )
	{
	case MATERIAL_VAR_TYPE_FLOAT:
		return m_floatVal;

	case MATERIAL_VAR_TYPE_STRING:
		return ( float )atof( m_pStringVal );

	case MATERIAL_VAR_TYPE_INT:
		return ( float )m_intVal;

	case MATERIAL_VAR_TYPE_VECTOR:
		return m_VecVal[0];

	case MATERIAL_VAR_TYPE_MATRIX:
	case MATERIAL_VAR_TYPE_TEXTURE:
	case MATERIAL_VAR_TYPE_MATERIAL:
	case MATERIAL_VAR_TYPE_UNDEFINED:
		return 0.0f;

	default:
		Warning( "CMaterialVar::GetFloatValue: Unknown material var type\n" );
		break;
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
// int
//-----------------------------------------------------------------------------

void CMaterialVar::SetIntValue( int val )
{
	// Suppress all this if we're not actually changing anything
	if ((m_Type == MATERIAL_VAR_TYPE_INT) && (m_intVal == val))
		return;

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == ( IMaterialInternal* )MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_intVal = val;
	m_Type = MATERIAL_VAR_TYPE_INT;
}

int CMaterialVar::GetIntValue( void ) const
{
	switch( m_Type )
	{
	case MATERIAL_VAR_TYPE_INT:
		return m_intVal;

	case MATERIAL_VAR_TYPE_FLOAT:
		return ( int )m_floatVal;

	case MATERIAL_VAR_TYPE_STRING:
		return ( int )atoi( m_pStringVal );

	case MATERIAL_VAR_TYPE_VECTOR:
		return ( int )m_VecVal[0];

	case MATERIAL_VAR_TYPE_MATRIX:
	case MATERIAL_VAR_TYPE_TEXTURE:
	case MATERIAL_VAR_TYPE_MATERIAL:
	case MATERIAL_VAR_TYPE_UNDEFINED:
		return 0;

	default:
		Warning( "CMaterialVar::GetIntValue: Unknown material var type\n" );
		break;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// string
//-----------------------------------------------------------------------------

char const *CMaterialVar::GetStringValue( void ) const
{
	switch( m_Type )
	{
	case MATERIAL_VAR_TYPE_STRING:
		return m_pStringVal;

	case MATERIAL_VAR_TYPE_INT:
		Q_snprintf( s_CharBuf, MATERIALVAR_CHAR_BUF_SIZE, "%d", m_intVal );
		return s_CharBuf;

	case MATERIAL_VAR_TYPE_FLOAT:
		Q_snprintf( s_CharBuf, MATERIALVAR_CHAR_BUF_SIZE, "%f", m_floatVal );
		return s_CharBuf;

	case MATERIAL_VAR_TYPE_VECTOR:
		{
			s_CharBuf[0] = '[';
			s_CharBuf[1] = ' ';
			int len = 2;
			for (int i = 0; i < m_NumComps; ++i)
			{
				if (len < MATERIALVAR_CHAR_BUF_SIZE)
				{
					Q_snprintf( s_CharBuf + len, MATERIALVAR_CHAR_BUF_SIZE - len, "%f ", m_VecVal[i] );
					len += strlen( s_CharBuf + len );
				}
			}
			if (len < MATERIALVAR_CHAR_BUF_SIZE - 1)
			{
				s_CharBuf[len] = ']';
				s_CharBuf[len+1] = '\0';
			}
			else
			{
				s_CharBuf[MATERIALVAR_CHAR_BUF_SIZE-1] = '\0';
			}
			return s_CharBuf;
		}

	case MATERIAL_VAR_TYPE_MATRIX:
 		{
			s_CharBuf[0] = '[';
			s_CharBuf[1] = ' ';
			int len = 2;
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					if (len < MATERIALVAR_CHAR_BUF_SIZE)
						len += Q_snprintf( s_CharBuf + len, MATERIALVAR_CHAR_BUF_SIZE - len, "%.3f ", m_pMatrix->m_Matrix[j][i] );
				}
			}
			if (len < MATERIALVAR_CHAR_BUF_SIZE - 1)
			{
				s_CharBuf[len] = ']';
				s_CharBuf[len+1] = '\0';
			}
			else
			{
				s_CharBuf[MATERIALVAR_CHAR_BUF_SIZE-1] = '\0';
			}
			return s_CharBuf;
		}

	case MATERIAL_VAR_TYPE_TEXTURE:
		Q_snprintf( s_CharBuf, MATERIALVAR_CHAR_BUF_SIZE, "%s", m_pTexture->GetName() );
		return s_CharBuf;

	case MATERIAL_VAR_TYPE_MATERIAL:
		Q_snprintf( s_CharBuf, MATERIALVAR_CHAR_BUF_SIZE, "%s", m_pMaterialValue->GetName() );
		return s_CharBuf;

	case MATERIAL_VAR_TYPE_UNDEFINED:
		return "<UNDEFINED>";

	default:
		Warning( "CMaterialVar::GetStringValue: Unknown material var type\n" );
		return "";
	}
}

void CMaterialVar::SetStringValue( char const *val )
{
	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == ( IMaterialInternal* )MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_pStringVal = new char[strlen( val ) + 1];
	strcpy( m_pStringVal, val );
	m_Type = MATERIAL_VAR_TYPE_STRING;
}

void CMaterialVar::SetFourCCValue( FourCC type, void *pData )
{
	// Suppress all this if we're not actually changing anything
	if ((m_Type == MATERIAL_VAR_TYPE_FOURCC) && m_FourCC.m_FourCC == type && m_FourCC.m_pFourCCData == pData )
		return;

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == ( IMaterialInternal* )MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();

	m_FourCC.m_FourCC = type;
	m_FourCC.m_pFourCCData = pData;
	m_Type = MATERIAL_VAR_TYPE_FOURCC;
}

void CMaterialVar::GetFourCCValue( FourCC *type, void **ppData )
{
	if( m_Type == MATERIAL_VAR_TYPE_FOURCC )
	{
		*type = m_FourCC.m_FourCC;
		*ppData = m_FourCC.m_pFourCCData;
	}
	else
	{
		*type = FOURCC_UNKNOWN;
		*ppData = 0;

		Warning( "CMaterialVar::GetVecValue: trying to get a vec value for %s which is of type %d\n",
			GetName(), ( int )m_Type );
	}
}


//-----------------------------------------------------------------------------
// texture
//-----------------------------------------------------------------------------
ITexture *CMaterialVar::GetTextureValue( void )
{
	ITexture *retVal = NULL;
	
	if( m_pMaterial )
	{
		m_pMaterial->Precache();
	}
	
	if( m_Type == MATERIAL_VAR_TYPE_TEXTURE )
	{
		retVal = static_cast<ITexture *>( m_pTexture );
		if( !retVal )
		{
			Warning( "Invalid texture value in CMaterialVar::GetTextureValue\n" );
		}
	}
	else
	{
		Warning( "Requesting texture value from var \"%s\" which is "
				  "not a texture value (material: %s)\n", GetName(),
					m_pMaterial ? m_pMaterial->GetName() : "NULL material" );
	}

	if( !retVal )
	{
		retVal = TextureManager()->ErrorTexture();
	}
	return retVal;
}

void CMaterialVar::SetTextureValue( ITexture *texture )
{
	ITextureInternal* pTexImp = static_cast<ITextureInternal *>( texture );

	// Suppress all this if we're not actually changing anything
	if ((m_Type == MATERIAL_VAR_TYPE_TEXTURE) && (m_pTexture == pTexImp))
		return;

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	// garymcthack
	if( pTexImp != ( ITextureInternal * )-1 )
	{
		pTexImp->IncrementReferenceCount();
	}

	CleanUpData();
	m_pTexture = pTexImp;
	m_Type = MATERIAL_VAR_TYPE_TEXTURE;
}


//-----------------------------------------------------------------------------
// material
//-----------------------------------------------------------------------------
IMaterial *CMaterialVar::GetMaterialValue( void )
{
	IMaterial *retVal = NULL;
	
	if( m_pMaterial )
	{
		m_pMaterial->Precache();
	}
	
	if( m_Type == MATERIAL_VAR_TYPE_MATERIAL )
	{
		retVal = static_cast<IMaterial *>( m_pMaterialValue );
	}
	else
	{
		Warning( "Requesting material value from var \"%s\" which is "
				  "not a material value (material: %s)\n", GetName(),
					m_pMaterial ? m_pMaterial->GetName() : "NULL material" );
	}
	return retVal;
}

void CMaterialVar::SetMaterialValue( IMaterial *pMaterial )
{
	IMaterialInternal* pMaterialImp = static_cast<IMaterialInternal *>( pMaterial );

	// Suppress all this if we're not actually changing anything
	if ((m_Type == MATERIAL_VAR_TYPE_MATERIAL) && (m_pMaterialValue == pMaterialImp))
		return;

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
	{
		g_pShaderAPI->FlushBufferedPrimitives();
	}

	if( pMaterialImp != NULL )
	{
		pMaterialImp->IncrementReferenceCount();
	}

	CleanUpData();
	m_pMaterialValue = pMaterialImp;
	m_Type = MATERIAL_VAR_TYPE_MATERIAL;
}


//-----------------------------------------------------------------------------
// Vector
//-----------------------------------------------------------------------------
void CMaterialVar::GetVecValue( float *pVal, int numComps ) const
{
	Assert( numComps <= 4 );

	switch( m_Type )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			for ( int i = 0; i < numComps; ++i )
			{
				pVal[i] = m_VecVal[i];
			}
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		{
			for ( int i = 0; i < numComps; ++i )
			{
				pVal[i] = m_intVal;
			}
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			for ( int i = 0; i < numComps; ++i )
			{
				pVal[i] = m_floatVal;
			}
		}
		break;

	case MATERIAL_VAR_TYPE_MATRIX:
	case MATERIAL_VAR_TYPE_UNDEFINED:
		{
			for ( int i = 0; i < numComps; ++i )
			{
				pVal[i] = 0.0f;
			}
		}
		break;

	default:
		Warning( "CMaterialVar::GetVecValue: trying to get a vec value for %s which is of type %d\n",
			GetName(), ( int )m_Type );
		break;
	}
}

float const* CMaterialVar::GetVecValue( ) const
{
	static Vector4D zero(0.0f, 0.0f, 0.0f, 0.0f);

	if( m_Type == MATERIAL_VAR_TYPE_VECTOR )
	{
		return m_VecVal;
	}
	else
	{
		return zero.Base();
	}
}

int	CMaterialVar::VectorSize() const
{
	return m_NumComps;
}

void CMaterialVar::SetVecValue( const float* pVal, int numComps )
{
	Assert( numComps <= 4 );

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_NumComps = numComps;
	memcpy( &m_VecVal, pVal, numComps * sizeof(float) );
	for (int i = numComps; i < 4; ++i )
		m_VecVal[i] = 0.0f;
	m_Type = MATERIAL_VAR_TYPE_VECTOR;
}

void CMaterialVar::SetVecValue( float x, float y )
{
	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_NumComps = 2;
	m_VecVal[0] = x;
	m_VecVal[1] = y;
	m_VecVal[2] = 0.0f;
	m_VecVal[3] = 0.0f;
	m_Type = MATERIAL_VAR_TYPE_VECTOR;
}

void CMaterialVar::SetVecValue( float x, float y, float z )
{
	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_NumComps = 3;
	m_VecVal[0] = x;
	m_VecVal[1] = y;
	m_VecVal[2] = z;
	m_VecVal[3] = 0.0f;
	m_Type = MATERIAL_VAR_TYPE_VECTOR;
}

void CMaterialVar::SetVecValue( float x, float y, float z, float w )
{
	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_NumComps = 4;
	m_VecVal[0] = x;
	m_VecVal[1] = y;
	m_VecVal[2] = z;
	m_VecVal[3] = w;
	m_Type = MATERIAL_VAR_TYPE_VECTOR;
}


//-----------------------------------------------------------------------------
// Matrix 
//-----------------------------------------------------------------------------
VMatrix const& CMaterialVar::GetMatrixValue( )
{
	if (m_Type == MATERIAL_VAR_TYPE_MATRIX)
		return m_pMatrix->m_Matrix;

	static VMatrix identity( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
	return identity;
}

void CMaterialVar::SetMatrixValue( VMatrix const& matrix )
{
	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_pMatrix = new MaterialVarMatrix_t;
	MatrixCopy( matrix, m_pMatrix->m_Matrix );
	m_Type = MATERIAL_VAR_TYPE_MATRIX;
}



//-----------------------------------------------------------------------------
// Undefined 
//-----------------------------------------------------------------------------
bool CMaterialVar::IsDefined() const
{
	return m_Type != MATERIAL_VAR_TYPE_UNDEFINED;
}

void CMaterialVar::SetUndefined()
{
	if (m_Type == MATERIAL_VAR_TYPE_UNDEFINED)
		return;

	// Gotta flush if we've changed state and this is the current material
	if (m_pMaterial && (m_pMaterial == MaterialSystem()->GetCurrentMaterial()))
		g_pShaderAPI->FlushBufferedPrimitives();

	CleanUpData();
	m_Type = MATERIAL_VAR_TYPE_UNDEFINED;
}


//-----------------------------------------------------------------------------
// Copy from another material var 
//-----------------------------------------------------------------------------
void CMaterialVar::CopyFrom( IMaterialVar *pMaterialVar )
{
	switch( pMaterialVar->GetType() )
	{
	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatValue( pMaterialVar->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_STRING:
		SetStringValue( pMaterialVar->GetStringValue() );
		break;

	case MATERIAL_VAR_TYPE_VECTOR:
		SetVecValue( pMaterialVar->GetVecValue(), pMaterialVar->VectorSize() );
		break;

	case MATERIAL_VAR_TYPE_TEXTURE:
		SetTextureValue( pMaterialVar->GetTextureValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		SetIntValue( pMaterialVar->GetIntValue() );
		break;

	case MATERIAL_VAR_TYPE_FOURCC:
		{
			FourCC fourCC;
			void *pData;
			pMaterialVar->GetFourCCValue( &fourCC, &pData );
			SetFourCCValue( fourCC, pData );
		}
		break;

	case MATERIAL_VAR_TYPE_UNDEFINED:
		SetUndefined();
		break;

	case MATERIAL_VAR_TYPE_MATRIX:
		SetMatrixValue( pMaterialVar->GetMatrixValue() );
		break;

	case MATERIAL_VAR_TYPE_MATERIAL:
		SetMaterialValue( pMaterialVar->GetMaterialValue() );
		break;

	default:
		Assert(0);
	}
}

