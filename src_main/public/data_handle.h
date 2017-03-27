#if !defined( DATA_HANDLE_H )
#define DATA_HANDLE_H
#ifdef _WIN32
#pragma once
#endif

#include <stdio.h>

class PackedDataAllocator;

class DataBlock
{
public:
#ifdef _DEBUG
	unsigned long	m_Size;
#endif
	int			m_nReferences;
	char		m_Data[1];
};

class DataHandle
{
public:
	DataHandle();
	~DataHandle();
	
	DataHandle&	operator=(const DataHandle &other);
	
	// Use these to access the data.
	void*		Lock();
	void		Unlock();
	
public:
	DataBlock			*m_pBlock;
	PackedDataAllocator	*m_pAllocator;
};


// --------------------------------------------------------------------------------------------------- //
// Inlines.
// --------------------------------------------------------------------------------------------------- //

inline DataHandle::DataHandle()
{
	m_pBlock = NULL; m_pAllocator = NULL;
}

inline DataHandle::~DataHandle()
{
	m_pAllocator->Free(*this);
}

#endif // DATA_HANDLE_H