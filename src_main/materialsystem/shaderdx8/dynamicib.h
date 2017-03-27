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
// The dx8 dynamic index buffer (snagged from nvidia, modified by brian)
//=============================================================================

#ifndef DYNAMICIB_H
#define DYNAMICIB_H

#ifdef _WIN32
#pragma once
#endif

#include "locald3dtypes.h"
#include "Recording.h"
#include "CMaterialSystemStats.h"
#include "ShaderAPIDX8_Global.h"
#include "IShaderUtil.h"

/////////////////////////////
// D. Sim Dietrich Jr.
// sim.dietrich@nvidia.com
//////////////////////

#ifdef _WIN32
#pragma warning (disable:4189)
#endif

#include "locald3dtypes.h"
#include "tier0/memdbgon.h"

class CIndexBuffer
{
public:
	CIndexBuffer( const LPDIRECT3DDEVICE pD3D, int count, bool dynamic = false );
	~CIndexBuffer();
	
	LPDIRECT3DINDEXBUFFER GetInterface() const { return m_pIB; }
	
	// Use at beginning of frame to force a flush of VB contents on first draw
	void FlushAtFrameStart() { m_bFlush = true; }
	
	// lock, unlock
	unsigned short* Lock( int numIndices, int& startIndex, int startPosition = -1 );	
	void Unlock( int numIndices );

	// Index position
	int IndexPosition() const { return m_Position; }
	
	// Index size
	int IndexSize() const { return sizeof(unsigned short); }

	// Index count
	int IndexCount() const { return m_IndexCount; }

	// Do we have enough room without discarding?
	bool HasEnoughRoom( int numIndices ) const;

#ifdef CHECK_INDICES
	void UpdateShadowIndices( unsigned short *pData )
	{
		Assert( m_LockedStartIndex + m_LockedNumIndices <= m_NumIndices );
		memcpy( m_pShadowIndices + m_LockedStartIndex, pData, m_LockedNumIndices * IndexSize() );
	}

	unsigned short GetShadowIndex( int i )
	{
		Assert( i >= 0 && i < (int)m_NumIndices );
		return m_pShadowIndices[i];
	}
#endif

	// UID
	unsigned int UID() const 
	{ 
#ifdef RECORDING
		return m_UID; 
#else
		return 0;
#endif
	}

	static int BufferCount()
	{
#ifdef _DEBUG
		return s_BufferCount;
#else
		return 0;
#endif
	}

private :
	enum LOCK_FLAGS
	{
		LOCKFLAGS_FLUSH  = D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD,
		LOCKFLAGS_APPEND = D3DLOCK_NOSYSLOCK | D3DLOCK_NOOVERWRITE
	};

	LPDIRECT3DINDEXBUFFER m_pIB;
	
	unsigned short	m_IndexCount;
	unsigned short	m_Position;

	bool	m_bLocked;
	bool	m_bFlush;
	bool	m_bDynamic;

#ifdef _DEBUG
	static int s_BufferCount;
#endif

#ifdef RECORDING
	unsigned int	m_UID;
#endif

protected:
#ifdef CHECK_INDICES
	unsigned short *m_pShadowIndices;
	unsigned int m_NumIndices;
	unsigned int m_LockedStartIndex;
	unsigned int m_LockedNumIndices;
#endif
};

 
#ifdef _DEBUG
int CIndexBuffer::s_BufferCount = 0;
#endif

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CIndexBuffer::CIndexBuffer( const LPDIRECT3DDEVICE pD3D, int count, bool dynamic ) :
	m_pIB(0), m_Position(0), m_bFlush(true), m_bLocked(false), 
	m_IndexCount(count), m_bDynamic(dynamic)
{
#ifdef CHECK_INDICES
	m_pShadowIndices = NULL;
#endif

#ifdef RECORDING
	// assign a UID
	static unsigned int uid = 0;
	m_UID = uid++;
#endif

#ifdef _DEBUG
	++s_BufferCount;
#endif

	D3DINDEXBUFFER_DESC desc;
	memset( &desc, 0x00, sizeof( desc ) );
	desc.Format = D3DFMT_INDEX16;
	desc.Size = sizeof(unsigned short) * count;
	desc.Type = D3DRTYPE_INDEXBUFFER;
	desc.Pool = D3DPOOL_DEFAULT;

	desc.Usage = D3DUSAGE_WRITEONLY;
	if (m_bDynamic)
		desc.Usage |= D3DUSAGE_DYNAMIC;

	RECORD_COMMAND( DX8_CREATE_INDEX_BUFFER, 6 );
	RECORD_INT( m_UID );
	RECORD_INT( count * IndexSize() );
	RECORD_INT( desc.Usage );
	RECORD_INT( desc.Format );
	RECORD_INT( desc.Pool );
	RECORD_INT( m_bDynamic );

#ifdef CHECK_INDICES
	Assert( desc.Format == D3DFMT_INDEX16 );
	m_pShadowIndices = new unsigned short[count];
	m_NumIndices = count;
#endif

	HRESULT hr = pD3D->CreateIndexBuffer( count * IndexSize(),
		desc.Usage, desc.Format, desc.Pool, &m_pIB, NULL
		);

	if ( m_pIB )
	{
		D3DINDEXBUFFER_DESC aDesc;
		m_pIB->GetDesc( &aDesc );
		Assert( memcmp( &aDesc, &desc, sizeof( desc ) ) == 0 );
	}
	Assert( ( hr == D3D_OK ) && ( m_pIB ) );

	MaterialSystemStats()->IncrementCountedStat( 
		MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED, m_IndexCount * IndexSize(), true );
}

CIndexBuffer::~CIndexBuffer()
{
#ifdef _DEBUG
	--s_BufferCount;
#endif

	Unlock(0);
	if ( m_pIB )
	{
		MaterialSystemStats()->IncrementCountedStat(
			MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED, -m_IndexCount * IndexSize(), true );

		RECORD_COMMAND( DX8_DESTROY_INDEX_BUFFER, 1 );
		RECORD_INT( m_UID );

#ifdef CHECK_INDICES
		delete [] m_pShadowIndices;
		m_pShadowIndices = NULL;
#endif

		int ref = m_pIB->Release();
		Assert( ref == 0 );
	}
}
	

//-----------------------------------------------------------------------------
// Do we have enough room without discarding?
//-----------------------------------------------------------------------------

inline bool CIndexBuffer::HasEnoughRoom( int numIndices ) const
{
	return ( numIndices + m_Position ) <= m_IndexCount;
}

//-----------------------------------------------------------------------------
// lock, unlock
//-----------------------------------------------------------------------------
	
unsigned short* CIndexBuffer::Lock( int numIndices, int& startIndex, int startPosition )
{
	unsigned short* pLockedData = 0;
	
	// Ensure there is enough space in the VB for this data
	if ( numIndices > m_IndexCount ) 
	{ 
		Error( "too many indices for index buffer. . tell a programmer (%d>%d)\n", ( int )numIndices, ( int )m_IndexCount );
		Assert( false ); 
		return 0; 
	}
	
	if ( m_pIB )
	{
		DWORD dwFlags;
		
		if (m_bDynamic)
		{
			Assert( startPosition < 0 );
			dwFlags = LOCKFLAGS_APPEND;
		
			// If either user forced us to flush,
			// or there is not enough space for the vertex data,
			// then flush the buffer contents
			if ( m_bFlush || !HasEnoughRoom(numIndices) )
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


		int position = m_Position;
		if( startPosition >= 0 )
		{
			position = startPosition;
		}
		
		RECORD_COMMAND( DX8_LOCK_INDEX_BUFFER, 4 );
		RECORD_INT( m_UID );
		RECORD_INT( position * IndexSize() );
		RECORD_INT( numIndices * IndexSize() );
		RECORD_INT( dwFlags );

#ifdef CHECK_INDICES
		m_LockedStartIndex = position;
		m_LockedNumIndices = numIndices;
#endif
		
		HRESULT hr = m_pIB->Lock( position * IndexSize(), 
			numIndices * IndexSize(), 
			reinterpret_cast< void** >( &pLockedData ), 
			dwFlags );
		
		Assert( hr == D3D_OK );
		if ( hr == D3D_OK )
		{
			Assert( pLockedData != 0 );
			startIndex = position;
			m_bLocked = true;
		}
	}
	
	return pLockedData;
}
	
void CIndexBuffer::Unlock( int numIndices )
{
	if ( ( m_bLocked ) && ( m_pIB ) )
	{
		m_Position += numIndices;

		RECORD_COMMAND( DX8_UNLOCK_INDEX_BUFFER, 1 );
		RECORD_INT( m_UID );

#ifdef CHECK_INDICES
		m_LockedStartIndex = 0;
		m_LockedNumIndices = 0;
#endif

		HRESULT hr = m_pIB->Unlock();				
		Assert( hr == D3D_OK );
		m_bLocked = false;
	}
}

#ifdef _WIN32
#pragma warning (default:4189)
#endif

#include "tier0/memdbgoff.h"

#endif  // DYNAMICIB_H

