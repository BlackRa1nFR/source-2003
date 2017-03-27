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
// The dx8 dynamic vertex buffer (snagged from nvidia, modified by brian)
//=============================================================================

#ifndef DYNAMICVB_H
#define DYNAMICVB_H

#ifdef _WIN32
#pragma once
#endif

#include "locald3dtypes.h"
#include "Recording.h"
#include "CMaterialSystemStats.h"
#include "ShaderAPIDX8_Global.h"
#include "tier0/dbg.h"

/////////////////////////////
// D. Sim Dietrich Jr.
// sim.dietrich@nvidia.com
//////////////////////

class CVertexBuffer
{
public:
	CVertexBuffer( const LPDIRECT3DDEVICE pD3D, DWORD theFVF, int vertexSize,
					int theVertexCount, bool dynamic = false );
	~CVertexBuffer();
	
	LPDIRECT3DVERTEXBUFFER GetInterface() const { return m_pVB; };
	
	// Use at beginning of frame to force a flush of VB contents on first draw
	void FlushAtFrameStart() { m_bFlush = true; }
	
	// lock, unlock
	unsigned char* Lock( int numVerts, int& baseVertexIndex );	
	unsigned char* Modify( int firstVertex, int numVerts );	
	void Unlock( int numVerts );
	
	// Vertex size
	int VertexSize() const { return m_VertexSize; }

	// Vertex count
	int VertexCount() const { return m_VertexCount; }

	static int BufferCount()
	{
#ifdef _DEBUG
		return s_BufferCount;
#else
		return 0;
#endif
	}

	// UID
	unsigned int UID() const 
	{ 
#ifdef RECORDING
		return m_UID; 
#else
		return 0;
#endif
	}

	// Do we have enough room without discarding?
	bool HasEnoughRoom( int numVertices ) const;

private :
	enum LOCK_FLAGS
	{
		LOCKFLAGS_FLUSH  = D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD,
		LOCKFLAGS_APPEND = D3DLOCK_NOSYSLOCK | D3DLOCK_NOOVERWRITE
	};

	LPDIRECT3DVERTEXBUFFER m_pVB;
	
	unsigned short	m_VertexCount;
	unsigned short	m_Position;
	unsigned char	m_VertexSize;
	bool	m_bDynamic;
	bool	m_bLocked;
	bool	m_bFlush;


#ifdef _DEBUG
	static int s_BufferCount;
#endif

#ifdef RECORDING
	unsigned int	m_UID;
#endif
};


#ifdef _DEBUG
int CVertexBuffer::s_BufferCount = 0;
#endif

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CVertexBuffer::CVertexBuffer( 
							 const LPDIRECT3DDEVICE pD3D, 
							 DWORD theFVF, 
			int vertexSize, int vertexCount, bool dynamic ) :
	m_pVB(0), m_Position(0), m_VertexSize(vertexSize), m_bFlush(true),
	m_bLocked(false), m_VertexCount(vertexCount), m_bDynamic(dynamic)
{
#ifdef RECORDING
	// assign a UID
	static unsigned int uid = 0;
	m_UID = uid++;
#endif

#ifdef _DEBUG
	++s_BufferCount;
#endif

	D3DVERTEXBUFFER_DESC desc;
	memset( &desc, 0x00, sizeof( desc ) );
	desc.Format = D3DFMT_VERTEXDATA;
	desc.Size = vertexCount * m_VertexSize;
	desc.Type = D3DRTYPE_VERTEXBUFFER;
	desc.Pool = D3DPOOL_DEFAULT; //m_bDynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
	desc.FVF = theFVF;
	
	desc.Usage = D3DUSAGE_WRITEONLY;
	if (m_bDynamic)
		desc.Usage |= D3DUSAGE_DYNAMIC;
	
	RECORD_COMMAND( DX8_CREATE_VERTEX_BUFFER, 6 );
	RECORD_INT( m_UID );
	RECORD_INT( m_VertexCount * m_VertexSize );
	RECORD_INT( desc.Usage );
	RECORD_INT( desc.FVF );
	RECORD_INT( desc.Pool );
	RECORD_INT( m_bDynamic );

	HRESULT hr = pD3D->CreateVertexBuffer( m_VertexCount * m_VertexSize,
					desc.Usage,	desc.FVF, desc.Pool, &m_pVB, NULL );
	if( hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY )
	{
		// Don't have the memory for this.  Try flushing all managed resources
		// out of vid mem and try again.
		// FIXME: need to record this
		pD3D->EvictManagedResources();
		pD3D->CreateVertexBuffer( m_VertexCount * m_VertexSize,
		         				  desc.Usage,	desc.FVF, desc.Pool, &m_pVB, NULL );
	}

#ifdef _DEBUG
	if( hr != D3D_OK )
	{
		switch( hr )
		{
		case D3DERR_INVALIDCALL:
			Assert( !"D3DERR_INVALIDCALL" );
			break;
		case D3DERR_OUTOFVIDEOMEMORY:
			Assert( !"D3DERR_OUTOFVIDEOMEMORY" );
			break;
		case E_OUTOFMEMORY:
			Assert( !"E_OUTOFMEMORY" );
			break;
		default:
			Assert( 0 );
			break;
		}
	}
#endif // _DEBUG
	Assert( m_pVB );

	MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED, 
		m_VertexCount * m_VertexSize, true );
}

CVertexBuffer::~CVertexBuffer()
{
#ifdef _DEBUG
	--s_BufferCount;
#endif

	Unlock(0);
	if ( m_pVB )
	{
		MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED,
			-m_VertexCount * m_VertexSize, true );

		RECORD_COMMAND( DX8_DESTROY_VERTEX_BUFFER, 1 );
		RECORD_INT( m_UID );

#ifdef _DEBUG
		int ref =
#endif
			m_pVB->Release();
		Assert( ref == 0 );
	}
}

//-----------------------------------------------------------------------------
// Do we have enough room without discarding?
//-----------------------------------------------------------------------------

inline bool CVertexBuffer::HasEnoughRoom( int numVertices ) const
{
	return ( numVertices + m_Position ) <= m_VertexCount;
}

//-----------------------------------------------------------------------------
// lock, unlock
//-----------------------------------------------------------------------------
	
unsigned char* CVertexBuffer::Lock( int numVerts, int& baseVertexIndex )
{
	unsigned char* pLockedData = 0;
	baseVertexIndex = 0;

	// Ensure there is enough space in the VB for this data
	if ( numVerts > m_VertexCount ) 
	{ 
		Assert( 0 );
		return 0; 
	}
	
	if ( m_pVB )
	{
		DWORD dwFlags;
		if (m_bDynamic)
		{
			dwFlags = LOCKFLAGS_APPEND;
		
			// If either user forced us to flush,
			// or there is not enough space for the vertex data,
			// then flush the buffer contents
			if ( m_bFlush || !HasEnoughRoom(numVerts) )
			{
				m_bFlush = false;
				m_Position = 0;
				dwFlags = LOCKFLAGS_FLUSH;
			}
		}
		else
		{
			dwFlags = D3DLOCK_NOSYSLOCK;
		}

		RECORD_COMMAND( DX8_LOCK_VERTEX_BUFFER, 4 );
		RECORD_INT( m_UID );
		RECORD_INT( m_Position * m_VertexSize );
		RECORD_INT( numVerts * m_VertexSize );
		RECORD_INT( dwFlags );

		HRESULT hr = m_pVB->Lock( m_Position * m_VertexSize, 
			numVerts * m_VertexSize, 
			reinterpret_cast< void** >( &pLockedData ), 
			dwFlags );
		
		Assert( hr == D3D_OK );
		if ( hr == D3D_OK )
		{
			Assert( pLockedData != 0 );
			m_bLocked = true;
			baseVertexIndex = m_Position;
		}
	}
	
	return pLockedData;
}

unsigned char* CVertexBuffer::Modify( int firstVertex, int numVerts )
{
	// D3D still returns a pointer when you call lock with 0 verts, so just in
	// case it's actually doing something, don't even try to lock the buffer with 0 verts.
	if ( numVerts == 0 )
		return NULL;

	Assert( m_pVB && !m_bDynamic );
	unsigned char* pLockedData = 0;
	if ( firstVertex + numVerts > m_VertexCount ) 
	{ 
		Assert( false ); 
		return 0; 
	}

	RECORD_COMMAND( DX8_LOCK_VERTEX_BUFFER, 4 );
	RECORD_INT( m_UID );
	RECORD_INT( firstVertex * m_VertexSize );
	RECORD_INT( numVerts * m_VertexSize );
	RECORD_INT( D3DLOCK_NOSYSLOCK );

	HRESULT hr = m_pVB->Lock( firstVertex * m_VertexSize, 
		numVerts * m_VertexSize, 
		reinterpret_cast< void** >( &pLockedData ), 
        D3DLOCK_NOSYSLOCK );
// mmw: for forcing all dynamic...        LOCKFLAGS_FLUSH );
	
	Assert( hr == D3D_OK );
	if ( hr == D3D_OK )
	{
		Assert( pLockedData != 0 );
		m_bLocked = true;
	}

	return pLockedData;
}
	
void CVertexBuffer::Unlock( int numVerts )
{
	if ( ( m_bLocked ) && ( m_pVB ) )
	{
		m_Position += numVerts;

		RECORD_COMMAND( DX8_UNLOCK_VERTEX_BUFFER, 1 );
		RECORD_INT( m_UID );

#ifdef _DEBUG
		HRESULT hr = 
#endif
			m_pVB->Unlock();				
		Assert( hr == D3D_OK );
		m_bLocked = false;
	}
}
	

#endif  // DYNAMICVB_H

