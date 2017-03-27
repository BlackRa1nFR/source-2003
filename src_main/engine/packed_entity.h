//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#if !defined( PACKED_ENTITY_H )
#define PACKED_ENTITY_H
#ifdef _WIN32
#pragma once
#endif


#include "basetypes.h"
#include "mempool.h"
#include "utlvector.h"
#include "tier0/dbg.h"


// This is the maximum amount of data a PackedEntity can have. Having a limit allows us
// to use static arrays sometimes instead of allocating memory all over the place.
#define MAX_PACKEDENTITY_DATA	1024

// This is the maximum number of properties that can be delta'd. Must be evenly divisible by 8.
#define MAX_PACKEDENTITY_PROPS	1024


class CSendProxyRecipients;
class SendTable;
class RecvTable;
class PackedDataAllocator;
class IChangeFrameList;


class DataBlock
{
public:
	unsigned short	m_Size;
	short			m_nReferences;
	char		m_Data[1];
};


// -------------------------------------------------------------------------------------------------- //
// DataHandle
// -------------------------------------------------------------------------------------------------- //

class DataHandle
{
friend class PackedDataAllocator;

public:
				DataHandle();
	virtual		~DataHandle();

	DataHandle&	operator=(const DataHandle &other);

	// Use these to access the data.
	void*		Lock();
	void		Unlock();

	// Free the data.
	void		Free();

private:
	DataBlock			*m_pBlock;
	PackedDataAllocator	*m_pAllocator;
};


// -------------------------------------------------------------------------------------------------- //
//
// PackedDataAllocator
//
// This allocates chunks of data for use by the networking data tables.
// Eventually, it will just have a big block of memory to deal with and will 
// unfragment the block as it goes.
// -------------------------------------------------------------------------------------------------- //

class PackedDataAllocator
{
public:
	PackedDataAllocator();
	
	bool			Alloc(unsigned long size, DataHandle &theHandle);
	void			Free(DataHandle &theHandle);

	// Current number of bytes allocated (in debug mode).
	unsigned long	m_nBytesAllocated;
	unsigned long	m_nBlocksAllocated;

private:
	CMemoryPool	m_64BytePool;
	CMemoryPool	m_128BytePool;
	CMemoryPool	m_1024BytePool;
};


// Replaces entity_state_t.
// This is what we send to clients.

class PackedEntity
{
public:

				PackedEntity();
				~PackedEntity();
	
	void		SetNumBits( int nBits );
	int			GetNumBits() const;
	int			GetNumBytes() const;

	// Access the data in the entity.
	void*		LockData();
	void		UnlockData();
	void		FreeData();

	// Copy the data into the PackedEntity's data and make sure the # bytes allocated is
	// an integer multiple of 4.
	bool		AllocAndCopyPadded( const void *pData, unsigned long size, PackedDataAllocator *pAllocator );

	// These are like Get/Set, except SnagChangeFrameList clears out the
	// PackedEntity's pointer since the usage model in sv_main is to keep
	// the same CChangeFrameList in the most recent PackedEntity for the
	// lifetime of an edict.
	//
	// When the PackedEntity is deleted, it deletes its current CChangeFrameList if it exists.
	void				SetChangeFrameList( IChangeFrameList *pList );
	IChangeFrameList*	SnagChangeFrameList();

	// If this PackedEntity has a ChangeFrameList, then this calls through. If not, this returns -1.
	int					GetPropsChangedAfterTick( int iTick, int *iOutProps, int nMaxOutProps );
	
	// Access the recipients array.
	const CSendProxyRecipients*	GetRecipients() const;
	int							GetNumRecipients() const;

	void				SetRecipients( const CUtlMemory<CSendProxyRecipients> &recipients );
	bool				CompareRecipients( const CUtlMemory<CSendProxyRecipients> &recipients );

public:
	
	SendTable	*m_pSendTable;	// Valid on the server.
	RecvTable	*m_pRecvTable;	// Valid on the client.
	
	int			m_nEntityIndex;	// Entity index.
	int			m_ReferenceCount;	// reference count;


private:

	CUtlVector<CSendProxyRecipients>	m_Recipients;

	DataHandle			m_Data;					// Packed data.
	int					m_nBits;				// Number of bits used to encode.
	IChangeFrameList	*m_pChangeFrameList;	// Only the most current 
};


inline void PackedEntity::SetNumBits( int nBits )
{
	Assert( !( nBits & 31 ) );
	m_nBits = nBits;
}

inline int PackedEntity::GetNumBits() const
{
	Assert( !( m_nBits & 31 ) ); 
	return m_nBits; 
}

inline int PackedEntity::GetNumBytes() const
{
	return GetNumBits() >> 3; 
}

inline void* PackedEntity::LockData()
{
	return m_Data.Lock();
}

inline void PackedEntity::UnlockData()
{
	m_Data.Unlock();
}

inline void PackedEntity::FreeData()
{
	m_Data.Free();
}

inline void PackedEntity::SetChangeFrameList( IChangeFrameList *pList )
{
	Assert( !m_pChangeFrameList );
	m_pChangeFrameList = pList;
}

inline IChangeFrameList* PackedEntity::SnagChangeFrameList()
{
	IChangeFrameList *pRet = m_pChangeFrameList;
	m_pChangeFrameList = 0;
	return pRet;
}

#endif // PACKED_ENTITY_H
