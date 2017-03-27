#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "clientdata.h"

// -------------------------------------------------------------------------------------------------- //
// ClientDataAllocator.
// -------------------------------------------------------------------------------------------------- //

ClientDataAllocator::ClientDataAllocator()
{
	m_nBytesAllocated = 0;
	m_nBlocksAllocated = 0;
}


bool ClientDataAllocator::Alloc(unsigned long size, ClientDataHandle &theHandle)
{
	int extra;

	
	Free(theHandle);
	
	#ifdef _DEBUG
		extra = 8;
	#else
		extra = 0;
	#endif

	theHandle.m_pBlock = (ClientDataBlock*)malloc(sizeof(ClientDataBlock) + size - 1 + extra);

	if(theHandle.m_pBlock)
	{
		// Leave markers for corruption checks.
		#ifdef _DEBUG
			char *ptr = (char*)theHandle.m_pBlock + sizeof(ClientDataBlock) + size - 1 + 4;
			*((long*)ptr) = 0xFCFED2;
			*((long*)theHandle.m_pBlock->m_Data) = 0xFCFED23;

			theHandle.m_pBlock->m_Size = size;
			m_nBytesAllocated += size;
			m_nBlocksAllocated++;
		#endif

		theHandle.m_pBlock->m_nReferences = 1;
		theHandle.m_pAllocator = this;
		return true;
	}
	else
	{
		return false;
	}
}

void ClientDataAllocator::Free(ClientDataHandle &theHandle)
{
	if(theHandle.m_pBlock)
	{
		theHandle.m_pBlock->m_nReferences--;
		if(theHandle.m_pBlock->m_nReferences == 0)
		{
			#ifdef _DEBUG
				assert(m_nBlocksAllocated > 0);
				m_nBytesAllocated -= theHandle.m_pBlock->m_Size;
				m_nBlocksAllocated--;
				theHandle.m_pBlock->m_Size = 0;
			#endif

			free(theHandle.m_pBlock);
		}
		
		theHandle.m_pBlock = NULL;
		theHandle.m_pAllocator = NULL;
	}
}


// -------------------------------------------------------------------------------------------------- //
// DataHandle.
// -------------------------------------------------------------------------------------------------- //

void* ClientDataHandle::Lock()
{
	if(m_pBlock)
	{
		// Check for corruption.
		#ifdef _DEBUG
			char *ptr = (char*)m_pBlock + sizeof(ClientDataBlock) + m_pBlock->m_Size - 1 + 4;
			assert(*((long*)ptr) == 0xFCFED2);
			assert(*((long*)m_pBlock->m_Data) == 0xFCFED23);
			return &m_pBlock->m_Data[4];
		#else
			return m_pBlock->m_Data;
		#endif
	}
	else
	{
		return NULL;
	}
}


void ClientDataHandle::Unlock()
{
	if(m_pBlock)
	{
		// Check for corruption.
		#ifdef _DEBUG
			char *ptr = (char*)m_pBlock + sizeof(ClientDataBlock) + m_pBlock->m_Size - 1 + 4;
			assert(*((long*)ptr) == 0xFCFED2);
			assert(*((long*)m_pBlock->m_Data) == 0xFCFED23);
		#endif
	}
}


ClientDataHandle& ClientDataHandle::operator=(const ClientDataHandle &other)
{
	ClientDataBlock *pBlock;


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

