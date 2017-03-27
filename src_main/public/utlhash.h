//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Serialization/unserialization buffer
//=============================================================================

#ifndef UTLHASH_H
#define UTLHASH_H
#pragma once

#include <assert.h>
#include "utlmemory.h"
#include "utlvector.h"

typedef unsigned int UtlHashHandle_t;

template<class Data>
class CUtlHash
{
public:

	// compare and key functions - implemented by the 
	typedef bool (*CompareFunc_t)( Data const&, Data const& );
	typedef unsigned int (*KeyFunc_t)( Data const& );
	
	// constructor/deconstructor
	CUtlHash( int bucketCount = 0, int growCount = 0, int initCount = 0,
		      CompareFunc_t compareFunc = 0, KeyFunc_t keyFunc = 0 );
	~CUtlHash();

	// invalid handle
	static UtlHashHandle_t InvalidHandle( void )  { return ( UtlHashHandle_t )~0; }
	bool IsValidHandle( UtlHashHandle_t handle ) const;

	// size
	int Count( void );

	// memory
	void Purge( void );

	// insertion methods
	UtlHashHandle_t Insert( Data const &src );
	UtlHashHandle_t AllocEntryFromKey( Data const &src );

	// removal methods
	void Remove( UtlHashHandle_t handle );
	void RemoveAll( void );

	// retrieval methods
	UtlHashHandle_t Find( Data const &src );

	Data &Element( UtlHashHandle_t handle );
	Data const &Element( UtlHashHandle_t handle ) const;
	Data &operator[]( UtlHashHandle_t handle );
	Data const &operator[]( UtlHashHandle_t handle ) const;

	// debugging!!
	void Log( const char *filename );

protected:

	int GetBucketIndex( UtlHashHandle_t handle );
	int GetKeyDataIndex( UtlHashHandle_t handle );
	UtlHashHandle_t BuildHandle( int ndxBucket, int ndxKeyData );

	bool IsPowerOfTwo( int value );

protected:

	// handle upper 16 bits = bucket index (bucket heads)
	// handle lower 16 bits = key index (bucket list)
	typedef CUtlVector<Data> HashBucketList_t;
	CUtlVector<HashBucketList_t>	m_Buckets;
	
	CompareFunc_t					m_CompareFunc;			// function used to handle unique compares on data
	KeyFunc_t						m_KeyFunc;				// function used to generate the key value

	bool							m_bPowerOfTwo;			// if the bucket value is a power of two, 
	unsigned int					m_ModMask;				// use the mod mask to "mod"	
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline bool CUtlHash<Data>::IsPowerOfTwo( int value )
{
	int ndx;
	int tmp = 1;
	for( ndx = 0; ndx < 32; ndx++ )
	{
		if( tmp == value )
			break;

		tmp = ( tmp << 1 );
	}

	if( ndx != 32 )
	{
		m_ModMask = 0;
		for( int shift = 0; shift < ndx; shift++ )
		{
			m_ModMask = ( m_ModMask << 1 );
			m_ModMask |= 0x00000001;
		}
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
CUtlHash<Data>::CUtlHash( int bucketCount, int growCount, int initCount,
						  CompareFunc_t compareFunc, KeyFunc_t keyFunc ) :
	m_CompareFunc( compareFunc ),
	m_KeyFunc( keyFunc )
{
	m_Buckets.SetSize( bucketCount );
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		m_Buckets[ndxBucket].SetSize( initCount );
		m_Buckets[ndxBucket].SetGrowSize( growCount );
	}

	// check to see if the bucket count is a power of 2 and set up
	// optimizations appropriately
	m_bPowerOfTwo = IsPowerOfTwo( bucketCount );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
CUtlHash<Data>::~CUtlHash()
{
	Purge();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline bool CUtlHash<Data>::IsValidHandle( UtlHashHandle_t handle ) const
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	if( ( ndxBucket >= 0 ) && ( ndxBucket < 65536 ) )
	{
		if( ( ndxKeyData >= 0 ) &&  ( ndxKeyData < 65536 ) )
			return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline int CUtlHash<Data>::Count( void )
{
	int count = 0;

	int bucketCount = m_Buckets.Count();
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		count += m_Buckets[ndxBucket].Count();
	}

	return count;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline int CUtlHash<Data>::GetBucketIndex( UtlHashHandle_t handle )
{
	return ( ( ( handle >> 16 ) & 0x0000ffff ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline int CUtlHash<Data>::GetKeyDataIndex( UtlHashHandle_t handle )
{
	return ( handle & 0x0000ffff );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline UtlHashHandle_t CUtlHash<Data>::BuildHandle( int ndxBucket, int ndxKeyData )
{
	assert( ( ndxBucket >= 0 ) && ( ndxBucket < 65536 ) );
	assert( ( ndxKeyData >= 0 ) && ( ndxKeyData < 65536 ) );

	UtlHashHandle_t handle = ndxKeyData;
	handle |= ( ndxBucket << 16 );

	return handle;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline void CUtlHash<Data>::Purge( void )
{
	int bucketCount = m_Buckets.Count();
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		m_Buckets[ndxBucket].Purge();
	}

	m_Buckets.Purge();

	m_CompareFunc = 0;
	m_KeyFunc = 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline UtlHashHandle_t CUtlHash<Data>::Insert( Data const &src )
{
	// check to see if that "key" already exists in the "bucket"
	// as "keys" are unique
	UtlHashHandle_t handle = Find( src );
	if( handle != InvalidHandle() )
		return handle;

	// generate the data "key"
	unsigned int key = m_KeyFunc( src );

	// hash the "key" - get the correct hash table "bucket"
	int bucketCount = m_Buckets.Count();
	unsigned int ndxBucket;
	if( m_bPowerOfTwo )
	{
		ndxBucket = ( key & m_ModMask );
	}
	else
	{
		ndxBucket = key % bucketCount;
	}

	// add the key/data pair to the "bucket"
	int	ndx = m_Buckets[ndxBucket].AddToTail( src );

	return ( BuildHandle( ndxBucket, ndx ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline UtlHashHandle_t CUtlHash<Data>::AllocEntryFromKey( Data const &src )
{
	// check to see if that "key" already exists in the "bucket"
	// as "keys" are unique
	UtlHashHandle_t handle = Find( src );
	if( handle != InvalidHandle() )
		return handle;

	// generate the data "key"
	unsigned int key = m_KeyFunc( src );

	// hash the "key" - get the correct hash table "bucket"
	int bucketCount = m_Buckets.Count();
	unsigned int ndxBucket;
	if( m_bPowerOfTwo )
	{
		ndxBucket = ( key & m_ModMask );
	}
	else
	{
		ndxBucket = key % bucketCount;
	}

	// add the key/data pair to the "bucket"
	int	ndx = m_Buckets[ndxBucket].AddToTail();

	return ( BuildHandle( ndxBucket, ndx ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline void CUtlHash<Data>::Remove( UtlHashHandle_t handle )
{
	assert( IsValidHandle( handle ) );

	// check to see if the bucket exists
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	if( m_Buckets[ndxBucket].Element( ndxKeyData ) != InvalidHande() )
	{
		m_Buckets[ndxBucket].Remove( ndxKeyData );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline void CUtlHash<Data>::RemoveAll( void )
{
	int bucketCount = m_Buckets.Count();
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		m_Buckets[ndxBucket].RemoveAll();
	}

	m_Buckets.RemoveAll();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline UtlHashHandle_t CUtlHash<Data>::Find( Data const &src )
{
	// sanity check(s)
	assert( m_KeyFunc );
	assert( m_CompareFunc );

	// generate the data "key"
	unsigned int key = m_KeyFunc( src );

	// hash the "key" - get the correct hash table "bucket"
	int bucketCount = m_Buckets.Count();
	unsigned int ndxBucket;
	if( m_bPowerOfTwo )
	{
		ndxBucket = ( key & m_ModMask );
	}
	else
	{
		ndxBucket = key % bucketCount;
	}

	int ndxKeyData;
	int keyDataCount = m_Buckets[ndxBucket].Count();
	for( ndxKeyData = 0; ndxKeyData < keyDataCount; ndxKeyData++ )
	{
		if( m_CompareFunc( m_Buckets[ndxBucket].Element( ndxKeyData ), src ) )
			break;
	}
	
	if( ndxKeyData == keyDataCount )
		return ( InvalidHandle() );

	return ( BuildHandle( ndxBucket, ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline Data &CUtlHash<Data>::Element( UtlHashHandle_t handle )
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline Data const &CUtlHash<Data>::Element( UtlHashHandle_t handle ) const
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline Data &CUtlHash<Data>::operator[]( UtlHashHandle_t handle )
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline Data const &CUtlHash<Data>::operator[]( UtlHashHandle_t handle ) const
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data>
inline void CUtlHash<Data>::Log( const char *filename )
{
	FILE *pDebugFp;
	pDebugFp = fopen( filename, "w" );
	if( !pDebugFp )
		return;

	int maxBucketSize = 0;
	int numBucketsEmpty = 0;

	int bucketCount = m_Buckets.Count();
	fprintf( pDebugFp, "\n%d Buckets\n", bucketCount ); 

	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		int count = m_Buckets[ndxBucket].Count();
		
		if( count > maxBucketSize ) { maxBucketSize = count; }
		if( count == 0 )
			numBucketsEmpty++;

		fprintf( pDebugFp, "Bucket %d: %d\n", ndxBucket, count );
	}

	fprintf( pDebugFp, "\nBucketHeads Used: %d\n", bucketCount - numBucketsEmpty );
	fprintf( pDebugFp, "Max Bucket Size: %d\n", maxBucketSize );

	fclose( pDebugFp );
}

#endif // UTLHASH_H
