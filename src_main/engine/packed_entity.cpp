#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "packed_entity.h"
#include "basetypes.h"
#include "changeframelist.h"
#include "dt_send.h"


// -------------------------------------------------------------------------------------------------- //
// PackedDataAllocator.
// -------------------------------------------------------------------------------------------------- //

PackedDataAllocator::PackedDataAllocator() : m_64BytePool( 64, 256, CMemoryPool::GROW_SLOW ),
	m_128BytePool( 128, 128, CMemoryPool::GROW_SLOW ), m_1024BytePool( 1024, 4, CMemoryPool::GROW_SLOW )

{
	m_nBytesAllocated = 0;
	m_nBlocksAllocated = 0;
}


bool PackedDataAllocator::Alloc(unsigned long size, DataHandle &theHandle)
{
	Free(theHandle);

	int totalSize = sizeof(DataBlock) + size - 1;
	if (totalSize <= 64)
	{
		theHandle.m_pBlock = (DataBlock*)m_64BytePool.Alloc();
	}
	else if (totalSize <= 128)
	{
		theHandle.m_pBlock = (DataBlock*)m_128BytePool.Alloc();
	}
	else  if (totalSize <= 1024)
	{
		theHandle.m_pBlock = (DataBlock*)m_1024BytePool.Alloc();
	}
	else
	{
		theHandle.m_pBlock = (DataBlock*)malloc(totalSize);
	}

	if(theHandle.m_pBlock)
	{
		theHandle.m_pBlock->m_nReferences = 1;
		theHandle.m_pBlock->m_Size = totalSize;
		theHandle.m_pAllocator = this;
		return true;
	}
	else
	{
		return false;
	}
}

void PackedDataAllocator::Free(DataHandle &theHandle)
{
	if(theHandle.m_pBlock)
	{
		theHandle.m_pBlock->m_nReferences--;
		if(theHandle.m_pBlock->m_nReferences == 0)
		{
			if (theHandle.m_pBlock->m_Size <= 64)
			{
				m_64BytePool.Free( theHandle.m_pBlock );
			}
			else if (theHandle.m_pBlock->m_Size <= 128)
			{
				m_128BytePool.Free( theHandle.m_pBlock );
			}
			else if (theHandle.m_pBlock->m_Size <= 1024)
			{
				m_1024BytePool.Free( theHandle.m_pBlock );
			}
			else
			{
				free(theHandle.m_pBlock);
			}
		}
		
		theHandle.m_pBlock = NULL;
		theHandle.m_pAllocator = NULL;
	}
}


// -------------------------------------------------------------------------------------------------- //
// PackedEntity.
// -------------------------------------------------------------------------------------------------- //

PackedEntity::PackedEntity()
{
	m_pChangeFrameList = 0;
}


PackedEntity::~PackedEntity()
{
	if ( m_pChangeFrameList )
		m_pChangeFrameList->Release();
}


bool PackedEntity::AllocAndCopyPadded( const void *pData, unsigned long size, PackedDataAllocator *pAllocator )
{
	m_Data.Free();
	
	unsigned long nBytes = PAD_NUMBER( size, 4 );
	if ( pAllocator->Alloc( nBytes, m_Data ) )
	{
		void *pDest = m_Data.Lock();
		if ( pDest )
		{
			memcpy( pDest, pData, size );
			m_Data.Unlock();
			SetNumBits( nBytes * 8 );
			return true;
		}
	}

	return false;
}


int PackedEntity::GetPropsChangedAfterTick( int iTick, int *iOutProps, int nMaxOutProps )
{
	if ( m_pChangeFrameList )
		return m_pChangeFrameList->GetPropsChangedAfterTick( iTick, iOutProps, nMaxOutProps );
	else
		return -1;
}


const CSendProxyRecipients*	PackedEntity::GetRecipients() const
{
	return m_Recipients.Base();
}


int PackedEntity::GetNumRecipients() const
{
	return m_Recipients.Count();
}


void PackedEntity::SetRecipients( const CUtlMemory<CSendProxyRecipients> &recipients )
{
	m_Recipients.CopyArray( recipients.Base(), recipients.Count() );
}


bool PackedEntity::CompareRecipients( const CUtlMemory<CSendProxyRecipients> &recipients )
{
	if ( recipients.Count() != m_Recipients.Count() )
		return false;
	
	return memcmp( recipients.Base(), m_Recipients.Base(), sizeof( CSendProxyRecipients ) * m_Recipients.Count() ) == 0;
}	


// -------------------------------------------------------------------------------------------------- //
// DataHandle.
// -------------------------------------------------------------------------------------------------- //

DataHandle::DataHandle()
{
	m_pBlock = NULL; 
	m_pAllocator = NULL;
}


DataHandle::~DataHandle()
{
	m_pAllocator->Free(*this);
}


void* DataHandle::Lock()
{
	if(m_pBlock)
	{
		// Check for corruption.
		return m_pBlock->m_Data;
	}
	else
	{
		return NULL;
	}
}


void DataHandle::Unlock()
{
}


void DataHandle::Free()
{
	m_pAllocator->Free( *this );
}


DataHandle& DataHandle::operator=(const DataHandle &other)
{
	DataBlock *pBlock;


	assert(this != &other);

	// (Increment references first just in case, for some reason it calls = on ourself).
	pBlock = other.m_pBlock;
	if(pBlock)
		pBlock->m_nReferences++;

	m_pAllocator->Free(*this);
	m_pBlock = pBlock;

	m_pAllocator = other.m_pAllocator;
	return *this;
}