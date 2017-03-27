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

#undef PROTECTED_THINGS_ENABLE
#include "vertexdecl.h" // this includes <windows.h> inside the dx headers
#define PROTECTED_THINGS_ENABLE
#include "ShaderAPIDX8_Global.h"
#include "tier0/dbg.h"
#include "UtlRBTree.h"
#include "recording.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// This is where we'll always pass in the vertex data to the shaders
//-----------------------------------------------------------------------------
enum VertexShaderDataLocation_t
{
	VSD_POSITION = 0,
	VSD_BONE_WEIGHTS = 1,
	VSD_BONE_INDICES = 2,
	VSD_NORMAL = 3,
	VSD_COLOR = 5,
	VSD_SPECULAR = 6,

	VSD_TEXTURE = 7,
	VSD_TEXTURE0 = VSD_TEXTURE,
	VSD_TEXTURE1,
	VSD_TEXTURE2,
	VSD_TEXTURE3,

	VSD_TANGENT_S = 11,
	VSD_TANGENT_T = 12,
	VSD_USERDATA = 14,
};


//-----------------------------------------------------------------------------
// Computes the DX8 vertex specification
//-----------------------------------------------------------------------------
static const char *DeclTypeToString( BYTE type )
{
	switch( type )
	{
	case D3DDECLTYPE_FLOAT1:
		return "D3DDECLTYPE_FLOAT1";
	case D3DDECLTYPE_FLOAT2:
		return "D3DDECLTYPE_FLOAT2";
	case D3DDECLTYPE_FLOAT3:
		return "D3DDECLTYPE_FLOAT3";
	case D3DDECLTYPE_FLOAT4:
		return "D3DDECLTYPE_FLOAT4";
	case D3DDECLTYPE_D3DCOLOR:
		return "D3DDECLTYPE_D3DCOLOR";
	case D3DDECLTYPE_UBYTE4:
		return "D3DDECLTYPE_UBYTE4";
	case D3DDECLTYPE_SHORT2:
		return "D3DDECLTYPE_SHORT2";
	case D3DDECLTYPE_SHORT4:
		return "D3DDECLTYPE_SHORT4";
	case D3DDECLTYPE_UBYTE4N:
		return "D3DDECLTYPE_UBYTE4N";
	case D3DDECLTYPE_SHORT2N:
		return "D3DDECLTYPE_SHORT2N";
	case D3DDECLTYPE_SHORT4N:
		return "D3DDECLTYPE_SHORT4N";
	case D3DDECLTYPE_USHORT2N:
		return "D3DDECLTYPE_USHORT2N";
	case D3DDECLTYPE_USHORT4N:
		return "D3DDECLTYPE_USHORT4N";
	case D3DDECLTYPE_UDEC3:
		return "D3DDECLTYPE_UDEC3";
	case D3DDECLTYPE_DEC3N:
		return "D3DDECLTYPE_DEC3N";
	case D3DDECLTYPE_FLOAT16_2:
		return "D3DDECLTYPE_FLOAT16_2";
	case D3DDECLTYPE_FLOAT16_4:
		return "D3DDECLTYPE_FLOAT16_4";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

static const char *DeclMethodToString( BYTE method )
{
	switch( method )
	{
	case D3DDECLMETHOD_DEFAULT:
		return "D3DDECLMETHOD_DEFAULT";
	case D3DDECLMETHOD_PARTIALU:
		return "D3DDECLMETHOD_PARTIALU";
	case D3DDECLMETHOD_PARTIALV:
		return "D3DDECLMETHOD_PARTIALV";
	case D3DDECLMETHOD_CROSSUV:
		return "D3DDECLMETHOD_CROSSUV";
	case D3DDECLMETHOD_UV:
		return "D3DDECLMETHOD_UV";
	case D3DDECLMETHOD_LOOKUP:
		return "D3DDECLMETHOD_LOOKUP";
	case D3DDECLMETHOD_LOOKUPPRESAMPLED:
		return "D3DDECLMETHOD_LOOKUPPRESAMPLED";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

static const char *DeclUsageToString( BYTE usage )
{
	switch( usage )
	{
	case D3DDECLUSAGE_POSITION:
		return "D3DDECLUSAGE_POSITION";
	case D3DDECLUSAGE_BLENDWEIGHT:
		return "D3DDECLUSAGE_BLENDWEIGHT";
	case D3DDECLUSAGE_BLENDINDICES:
		return "D3DDECLUSAGE_BLENDINDICES";
	case D3DDECLUSAGE_NORMAL:
		return "D3DDECLUSAGE_NORMAL";
	case D3DDECLUSAGE_PSIZE:
		return "D3DDECLUSAGE_PSIZE";
	case D3DDECLUSAGE_COLOR:
		return "D3DDECLUSAGE_COLOR";
	case D3DDECLUSAGE_TEXCOORD:
		return "D3DDECLUSAGE_TEXCOORD";
	case D3DDECLUSAGE_TANGENT:
		return "D3DDECLUSAGE_TANGENT";
	case D3DDECLUSAGE_BINORMAL:
		return "D3DDECLUSAGE_BINORMAL";
	case D3DDECLUSAGE_TESSFACTOR:
		return "D3DDECLUSAGE_TESSFACTOR";
//	case D3DDECLUSAGE_POSITIONTL:
//		return "D3DDECLUSAGE_POSITIONTL";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

void PrintVertexDeclaration( const D3DVERTEXELEMENT9 *pDecl )
{
	int i;
	static D3DVERTEXELEMENT9 declEnd = D3DDECL_END();
	for( i = 0; ; i++ )
	{
		if( memcmp( &pDecl[i], &declEnd, sizeof( declEnd ) ) == 0 )
		{
			Warning( "D3DDECL_END\n" );
			break;
		}
		Warning( "%d: Stream: %d, Offset: %d, Type: %s, Method: %s, Usage: %s, UsageIndex: %d\n",
			i, ( int )pDecl[i].Stream, ( int )pDecl[i].Offset,
			DeclTypeToString( pDecl[i].Type ),
			DeclMethodToString( pDecl[i].Method ),
			DeclUsageToString( pDecl[i].Usage ),
			( int )pDecl[i].UsageIndex );
	}
}


//-----------------------------------------------------------------------------
// Converts format to a vertex decl
//-----------------------------------------------------------------------------
void ComputeVertexSpec( VertexFormat_t fmt, D3DVERTEXELEMENT9 *pDecl, bool bStaticLit )
{
	static DWORD sizeLookup[] =
	{
		D3DDECLTYPE_FLOAT1,
		D3DDECLTYPE_FLOAT2,
		D3DDECLTYPE_FLOAT3,
		D3DDECLTYPE_FLOAT4
	};
	int i = 0;
	int offset = 0;

	if (fmt & VERTEX_POSITION)
	{
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_FLOAT3;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_POSITION;
		pDecl[i].UsageIndex = 0;
		offset += sizeof( float ) * 3;
		++i;
	}

	int numBones = NumBoneWeights(fmt);
	if (numBones > 0)
	{
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = sizeLookup[numBones-1];
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_BLENDWEIGHT;
		pDecl[i].UsageIndex = 0;
		offset += sizeof( float ) * numBones;
		++i;
	}

	if (fmt & VERTEX_BONE_INDEX)
	{
		// this isn't FVF!!!!!
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_D3DCOLOR;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_BLENDINDICES;
		pDecl[i].UsageIndex = 0;
		offset += 4;
		++i;
	}

	if (fmt & VERTEX_NORMAL)
	{
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_FLOAT3;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_NORMAL;
		pDecl[i].UsageIndex = 0;
		offset += sizeof( float ) * 3;
		++i;
	}

	if (fmt & VERTEX_COLOR)
	{
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_D3DCOLOR;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_COLOR;
		pDecl[i].UsageIndex = 0;
		offset += 4;
		++i;
	}

	if (fmt & VERTEX_SPECULAR)
	{
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_D3DCOLOR;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_COLOR;
		pDecl[i].UsageIndex = 1;
		offset += 4;
		++i;
	}

	int numTexCoords = NumTextureCoordinates(fmt);
	for (int j = 0; j < numTexCoords; ++j )
	{
		int coordSize = TexCoordSize( j, fmt );
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = sizeLookup[coordSize-1];
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_TEXCOORD;
		pDecl[i].UsageIndex = j;
		offset += sizeof( float ) * coordSize;
		++i;
	}

	if (fmt & VERTEX_TANGENT_S)
	{
		// this isn't FVF!!!!!
//		pDecl[i] = D3DVSD_REG( VSD_TANGENT_S, D3DVSDT_FLOAT3 );
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_FLOAT3;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_TANGENT;
		pDecl[i].UsageIndex = 0;
		offset += sizeof( float ) * 3;
		++i;
	}

	if (fmt & VERTEX_TANGENT_T)
	{
		// this isn't FVF!!!!!
//		pDecl[i] = D3DVSD_REG( VSD_TANGENT_T, D3DVSDT_FLOAT3 );
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = D3DDECLTYPE_FLOAT3;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage =   D3DDECLUSAGE_BINORMAL;
		pDecl[i].UsageIndex = 0;
		offset += sizeof( float ) * 3;
		++i;
	}

	int userDataSize = UserDataSize(fmt);
	if (userDataSize > 0)
	{
		// this isn't FVF!!!!!
//		pDecl[i] = D3DVSD_REG( VSD_USERDATA, sizeLookup[userDataSize-1] );
		pDecl[i].Stream = 0;
		pDecl[i].Offset = offset;
		pDecl[i].Type = sizeLookup[userDataSize-1];
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_TANGENT;
		pDecl[i].UsageIndex = 0;
		offset += sizeof( float ) * userDataSize;
		++i;
	}

	if( bStaticLit )
	{
		// this isn't FVF!!!!!
		// force stream 1 to have specular color in it, which is used for baked static lighting
		pDecl[i].Stream = 1;
		pDecl[i].Offset = 0;
		pDecl[i].Type = D3DDECLTYPE_D3DCOLOR;
		pDecl[i].Method = D3DDECLMETHOD_DEFAULT;
		pDecl[i].Usage = D3DDECLUSAGE_COLOR;
		pDecl[i].UsageIndex = 1;
		++i;
	}

	static D3DVERTEXELEMENT9 declEnd = D3DDECL_END();
	pDecl[i] = declEnd;

//	PrintVertexDeclaration( pDecl );
}


//-----------------------------------------------------------------------------
// Gets the declspec associated with a vertex format
//-----------------------------------------------------------------------------
struct VertexDeclLookup_t
{
	VertexFormat_t m_VertexFormat;
	bool m_bStaticLit;
	IDirect3DVertexDeclaration9 *m_pDecl;

	bool operator==( const VertexDeclLookup_t &src ) const
	{
		return ( m_VertexFormat == src.m_VertexFormat ) && ( m_bStaticLit == src.m_bStaticLit );
	}
};


//-----------------------------------------------------------------------------
// Dictionary of vertex decls
// FIXME: stick this in the class?
// FIXME: Does anything cause this to get flushed?
//-----------------------------------------------------------------------------
static bool VertexDeclLessFunc( const VertexDeclLookup_t &src1, const VertexDeclLookup_t &src2 )
{
	if( src1.m_bStaticLit == src2.m_bStaticLit )
		return src1.m_VertexFormat < src2.m_VertexFormat;

	// consider static lit to be less than non-static lit
	return ( src1.m_bStaticLit && !src2.m_bStaticLit );
}

static CUtlRBTree<VertexDeclLookup_t, int> s_VertexDeclDict( 0, 256, VertexDeclLessFunc );


//-----------------------------------------------------------------------------
// Gets the declspec associated with a vertex format
//-----------------------------------------------------------------------------
IDirect3DVertexDeclaration9 *FindOrCreateVertexDecl( VertexFormat_t fmt, bool bStaticLit )
{
	VertexDeclLookup_t lookup;
	lookup.m_VertexFormat = fmt;
	lookup.m_bStaticLit = bStaticLit;
	int i = s_VertexDeclDict.Find( lookup );
	if( i != s_VertexDeclDict.InvalidIndex() )
	{
		return s_VertexDeclDict[i].m_pDecl;
	}

	D3DVERTEXELEMENT9 decl[32];
	ComputeVertexSpec( fmt, decl, bStaticLit );

#ifdef _DEBUG
	HRESULT hr = 
#endif
		D3DDevice()->CreateVertexDeclaration( decl, &lookup.m_pDecl );

	// NOTE: can't record until we have m_pDecl!
	RECORD_COMMAND( DX8_CREATE_VERTEX_DECLARATION, 2 );
	RECORD_INT( ( int )lookup.m_pDecl );
	RECORD_STRUCT( decl, sizeof( decl ) );
	COMPILE_TIME_ASSERT( sizeof( decl ) == sizeof( D3DVERTEXELEMENT9 ) * 32 );

	Assert( hr == D3D_OK );
	s_VertexDeclDict.Insert( lookup );
	return lookup.m_pDecl;
}


//-----------------------------------------------------------------------------
// Clears out all declspecs
//-----------------------------------------------------------------------------
void ReleaseAllVertexDecl( )
{
	int i = s_VertexDeclDict.FirstInorder();
	while ( i != s_VertexDeclDict.InvalidIndex() )
	{
		s_VertexDeclDict[i].m_pDecl->Release();

		i = s_VertexDeclDict.NextInorder(i);
	}
}
