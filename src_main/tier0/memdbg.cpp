//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Memory allocation!
//
// $NoKeywords: $
//=============================================================================


#ifdef _DEBUG

#include <malloc.h>
#include <string.h>
#include "tier0/Dbg.h"
#include "tier0/memalloc.h"
#include "crtdbg.h"
#include <map>
#include <limits.h>


//-----------------------------------------------------------------------------
// NOTE! This should never be called directly from leaf code
// Just use new,delete,malloc,free etc. They will call into this eventually
//-----------------------------------------------------------------------------
class CDbgMemAlloc : public IMemAlloc
{
public:
	CDbgMemAlloc();
	virtual ~CDbgMemAlloc();

	// Release versions
	virtual void *Alloc( size_t nSize );
	virtual void *Realloc( void *pMem, size_t nSize );
	virtual void  Free( void *pMem );
    virtual void *Expand( void *pMem, size_t nSize );

	// Debug versions
    virtual void *Alloc( size_t nSize, const char *pFileName, int nLine );
    virtual void *Realloc( void *pMem, size_t nSize, const char *pFileName, int nLine );
    virtual void  Free( void *pMem, const char *pFileName, int nLine );
    virtual void *Expand( void *pMem, size_t nSize, const char *pFileName, int nLine );

	// Returns size of a particular allocation
	virtual size_t GetSize( void *pMem );

    // Force file + line information for an allocation
    virtual void PushAllocDbgInfo( const char *pFileName, int nLine );
    virtual void PopAllocDbgInfo();

	virtual long CrtSetBreakAlloc( long lNewBreakAlloc );
	virtual	int CrtSetReportMode( int nReportType, int nReportMode );
	virtual int CrtIsValidHeapPointer( const void *pMem );
	virtual int CrtCheckMemory( void );
	virtual int CrtSetDbgFlag( int nNewFlag );
	virtual void CrtMemCheckpoint( _CrtMemState *pState );

	// FIXME: Remove when we have our own allocator
	virtual void* CrtSetReportFile( int nRptType, void* hFile );
	virtual void* CrtSetReportHook( void* pfnNewHook );
	virtual int CrtDbgReport( int nRptType, const char * szFile,
			int nLine, const char * szModule, const char * szFormat );

	virtual int heapchk();

	enum
	{
		BYTE_COUNT_16 = 0,
		BYTE_COUNT_32,
		BYTE_COUNT_128,
		BYTE_COUNT_1024,
		BYTE_COUNT_GREATER,

		NUM_BYTE_COUNT_BUCKETS
	};

private:
	enum
	{
		DBG_INFO_STACK_DEPTH = 32
	};

	struct DbgInfoStack_t
	{
		const char *m_pFileName;
		int m_nLine;
	};

	struct MemInfo_t
	{
		MemInfo_t()
		{
			memset( this, 0, sizeof(*this) );
		}

		// Size in bytes
		int m_nCurrentSize;
		int m_nPeakSize;
		int m_nTotalSize;
		int m_nOverheadSize;
		int m_nPeakOverheadSize;

		// Count in terms of # of allocations
		int m_nCurrentCount;
		int m_nPeakCount;
		int m_nTotalCount;

		// Count in terms of # of allocations of a particular size
		int m_pCount[NUM_BYTE_COUNT_BUCKETS];

		// Time spent allocating + deallocating	(microseconds)
		__int64 m_nTime;
	};

	struct MemInfoKey_t
	{
		MemInfoKey_t( const char *pFileName, int line ) : m_pFileName(pFileName), m_nLine(line) {}
		bool operator<( const MemInfoKey_t &key ) const
		{
			int iret = stricmp( m_pFileName, key.m_pFileName );
			if ( iret < 0 )
				return true;

			if ( iret > 0 )
				return false;

			return m_nLine < key.m_nLine;
		}

		const char *m_pFileName;
		int			m_nLine;
	};

	// NOTE: This exactly mirrors the dbg header in the MSDEV crt
	// eventually when we write our own allocator, we can kill this
	struct CrtDbgMemHeader_t
	{
		unsigned char m_Reserved[8];
		const char *m_pFileName;
		int			m_nLineNumber;
		unsigned char m_Reserved2[16];
	};

	// NOTE: Deliberately using STL here because the UTL stuff
	// is a client of this library; want to avoid circular dependency

	// Maps file name to info
	typedef std::map< MemInfoKey_t, MemInfo_t > StatMap_t;
	typedef StatMap_t::iterator StatMapIter_t;
	typedef StatMap_t::value_type StatMapEntry_t;

	// Heap reporting method
	typedef void (*HeapReportFunc_t)( char const *pFormat, ... );

private:
	// Returns the actual debug info
	void GetActualDbgInfo( const char *&pFileName, int &nLine );

	// Finds the file in our map
	MemInfo_t &FindOrCreateEntry( const char *pFileName, int line );

	// Updates stats
	void RegisterAllocation( MemInfo_t &info, int nSize, int nTime );
	void RegisterDeallocation( MemInfo_t &info, int nSize, int nTime );

	// Gets the allocation file name
	const char *GetAllocatonFileName( void *pMem );
	int GetAllocatonLineNumber( void *pMem );

	// FIXME: specify a spew output func for dumping stats
	// Stat output
	void DumpMemInfo( const char *pAllocationName, int line, const MemInfo_t &info );
	void DumpFileStats();
	void DumpStats();

private:
	DbgInfoStack_t	m_DbgInfoStack[DBG_INFO_STACK_DEPTH];
	int m_nDbgInfoStackDepth;
	StatMap_t m_StatMap;
	MemInfo_t m_GlobalInfo;
	CFastTimer m_Timer;

	HeapReportFunc_t m_OutputFunc;

	static int s_pCountSizes[NUM_BYTE_COUNT_BUCKETS];
	static const char *s_pCountHeader[NUM_BYTE_COUNT_BUCKETS];
};


//-----------------------------------------------------------------------------
// Singleton...
//-----------------------------------------------------------------------------
static CDbgMemAlloc s_DbgMemAlloc;

#ifndef TIER0_VALIDATE_HEAP
IMemAlloc *g_pMemAlloc = &s_DbgMemAlloc;
#else
IMemAlloc *g_pActualAlloc = &s_DbgMemAlloc;
#endif


//-----------------------------------------------------------------------------
// Byte count buckets
//-----------------------------------------------------------------------------
int CDbgMemAlloc::s_pCountSizes[CDbgMemAlloc::NUM_BYTE_COUNT_BUCKETS] = 
{
	16, 32, 128, 1024, INT_MAX
};

const char *CDbgMemAlloc::s_pCountHeader[CDbgMemAlloc::NUM_BYTE_COUNT_BUCKETS] = 
{
	"<=16 byte allocations", 
	"17-32 byte allocations",
	"33-128 byte allocations", 
	"129-1024 byte allocations",
	">1024 byte allocations"
};


//-----------------------------------------------------------------------------
// Standard output
//-----------------------------------------------------------------------------
static FILE* s_DbgFile;

static void DefaultHeapReportFunc( char const *pFormat, ... )
{
	va_list args;
	va_start( args, pFormat );
	vfprintf( s_DbgFile, pFormat, args );
	va_end( args );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CDbgMemAlloc::CDbgMemAlloc()
{
	m_OutputFunc = DefaultHeapReportFunc;
	m_nDbgInfoStackDepth = -1;
}

CDbgMemAlloc::~CDbgMemAlloc()
{
	StatMapIter_t iter = m_StatMap.begin();
	while(iter != m_StatMap.end())
	{
		if (iter->first.m_pFileName)
		{
			char *pFileName = (char*)iter->first.m_pFileName;
			delete [] pFileName;
		}
		iter++;
	}
}


//-----------------------------------------------------------------------------
// Release versions
//-----------------------------------------------------------------------------
static char const *pUnknown = "unknown";

void *CDbgMemAlloc::Alloc( size_t nSize )
{
	/*
	// NOTE: Uncomment this to find unknown allocations
	const char *pFileName = pUnknown;
	int nLine;
	GetActualDbgInfo( pFileName, nLine );
	if (pFileName == pUnknown)
	{
		int x = 3;
	}
	*/

	// FIXME: Should these gather stats?
	return Alloc( nSize, pUnknown, 0 );
//	return malloc( nSize );
}

void *CDbgMemAlloc::Realloc( void *pMem, size_t nSize )
{
	// FIXME: Should these gather stats?
	return Realloc( pMem, nSize, pUnknown, 0 );
//	return realloc( pMem, nSize );
}

void CDbgMemAlloc::Free( void *pMem )
{
	// FIXME: Should these gather stats?
	Free( pMem, pUnknown, 0 );
//	free( pMem );
}

void *CDbgMemAlloc::Expand( void *pMem, size_t nSize )
{
	// FIXME: Should these gather stats?
	return Expand( pMem, nSize, pUnknown, 0 );
//	return _expand( pMem, nSize );
}


//-----------------------------------------------------------------------------
// Force file + line information for an allocation
//-----------------------------------------------------------------------------
void CDbgMemAlloc::PushAllocDbgInfo( const char *pFileName, int nLine )
{
	++m_nDbgInfoStackDepth;
	Assert( m_nDbgInfoStackDepth < DBG_INFO_STACK_DEPTH );
	m_DbgInfoStack[m_nDbgInfoStackDepth].m_pFileName = pFileName;
	m_DbgInfoStack[m_nDbgInfoStackDepth].m_nLine = nLine;
}

void CDbgMemAlloc::PopAllocDbgInfo()
{
	--m_nDbgInfoStackDepth;
	Assert( m_nDbgInfoStackDepth >= -1 );
}


//-----------------------------------------------------------------------------
// Returns the actual debug info
//-----------------------------------------------------------------------------
void CDbgMemAlloc::GetActualDbgInfo( const char *&pFileName, int &nLine )
{
	if (m_nDbgInfoStackDepth >= 0)
	{
		pFileName = m_DbgInfoStack[m_nDbgInfoStackDepth].m_pFileName;
		nLine = m_DbgInfoStack[m_nDbgInfoStackDepth].m_nLine;
	}
}


//-----------------------------------------------------------------------------
// Finds the file in our map
//-----------------------------------------------------------------------------
CDbgMemAlloc::MemInfo_t &CDbgMemAlloc::FindOrCreateEntry( const char *pFileName, int line )
{
	// Oh how I love crazy STL. retval.first == the StatMapIter_t in the std::pair
	// retval.first->second == the MemInfo_t that's part of the StatMapIter_t 
	std::pair<StatMapIter_t, bool> retval;
	retval = m_StatMap.insert( StatMapEntry_t( MemInfoKey_t( pFileName, line ), MemInfo_t() ) );

	// If we created it for the first time, actually *allocate* the filename memory
	// This is necessary for shutdown conditions: the file name is stored
	// in some piece of memory in a DLL; if that DLL becomes unloaded,
	// we'll have a pointer to crap memory
	if (retval.second)
	{
		int nLen = strlen(pFileName) + 1;
		char **ppStatsFileName = (char**)(&retval.first->first.m_pFileName);
		*ppStatsFileName = new char[nLen];
		memcpy( *ppStatsFileName, pFileName, nLen );
	}

	return retval.first->second;
}


//-----------------------------------------------------------------------------
// Updates stats
//-----------------------------------------------------------------------------
void CDbgMemAlloc::RegisterAllocation( MemInfo_t &info, int nSize, int nTime )
{
	++info.m_nCurrentCount;
	++info.m_nTotalCount;
	if (info.m_nCurrentCount > info.m_nPeakCount)
	{
		info.m_nPeakCount = info.m_nCurrentCount;
	}

	info.m_nCurrentSize += nSize;
	info.m_nTotalSize += nSize;
	if (info.m_nCurrentSize > info.m_nPeakSize)
	{
		info.m_nPeakSize = info.m_nCurrentSize;
	}

	for (int i = 0; i < NUM_BYTE_COUNT_BUCKETS; ++i)
	{
		if (nSize <= s_pCountSizes[i])
		{
			++info.m_pCount[i];
			break;
		}
	}

	Assert( info.m_nPeakCount >= info.m_nCurrentCount );
	Assert( info.m_nPeakSize >= info.m_nCurrentSize );

	// Based on looking at the standard allocator, there's 12 bytes of
	// overhead per allocation + the allocation size is rounded to the
	// next highest 16 bytes
	int nActualSize = ((nSize + 0xF) & 0xFFFFFFF0);
	info.m_nOverheadSize += 12 + (nActualSize - nSize);
	if (info.m_nOverheadSize > info.m_nPeakOverheadSize)
	{
		info.m_nPeakOverheadSize = info.m_nOverheadSize;
	}

	info.m_nTime += nTime;
}

void CDbgMemAlloc::RegisterDeallocation( MemInfo_t &info, int nSize, int nTime )
{
	--info.m_nCurrentCount;
	info.m_nCurrentSize -= nSize;

	Assert( info.m_nPeakCount >= info.m_nCurrentCount );
	Assert( info.m_nPeakSize >= info.m_nCurrentSize );
	Assert( info.m_nCurrentCount >= 0 );
	Assert( info.m_nCurrentSize >= 0 );

	// Based on looking at the standard allocator, there's 12 bytes of
	// overhead per allocation + the allocation size is rounded to the
	// next highest 16 bytes
	int nActualSize = ((nSize + 0xF) & 0xFFFFFFF0);
	info.m_nOverheadSize -= 12 + (nActualSize - nSize);

	info.m_nTime += nTime;
}


//-----------------------------------------------------------------------------
// Gets the allocation file name
//-----------------------------------------------------------------------------
const char *CDbgMemAlloc::GetAllocatonFileName( void *pMem )
{
	if (!pMem)
		return "";

	CrtDbgMemHeader_t *pHeader = (CrtDbgMemHeader_t*)pMem;
	--pHeader;
	return pHeader->m_pFileName;
}

//-----------------------------------------------------------------------------
// Gets the allocation file name
//-----------------------------------------------------------------------------
int CDbgMemAlloc::GetAllocatonLineNumber( void *pMem )
{
	if (!pMem)
		return 0;

	CrtDbgMemHeader_t *pHeader = (CrtDbgMemHeader_t*)pMem;
	--pHeader;
	return pHeader->m_nLineNumber;
}

//-----------------------------------------------------------------------------
// Debug versions of the main allocation methods
//-----------------------------------------------------------------------------
void *CDbgMemAlloc::Alloc( size_t nSize, const char *pFileName, int nLine )
{
	GetActualDbgInfo( pFileName, nLine );

	m_Timer.Start();
	void *pMem = _malloc_dbg( nSize, _NORMAL_BLOCK, pFileName, nLine );
	m_Timer.End();

	unsigned long nTime = m_Timer.GetDuration().GetMicroseconds();

	RegisterAllocation( m_GlobalInfo, nSize, nTime );
	RegisterAllocation( FindOrCreateEntry( pFileName, nLine ), nSize, nTime );

	return pMem;
}

void *CDbgMemAlloc::Realloc( void *pMem, size_t nSize, const char *pFileName, int nLine )
{
	GetActualDbgInfo( pFileName, nLine );

	int nOldSize = GetSize( pMem );
	const char *pOldFileName = GetAllocatonFileName( pMem );
	int oldLine = GetAllocatonLineNumber( pMem );

	if ( pMem != 0 )
	{
		RegisterDeallocation( FindOrCreateEntry( pOldFileName, oldLine ), nOldSize, 0 );
		RegisterDeallocation( m_GlobalInfo, nOldSize, 0 );
	}

	m_Timer.Start();
	pMem = _realloc_dbg( pMem, nSize, _NORMAL_BLOCK, pFileName, nLine );
	m_Timer.End();
	
	unsigned long nTime = m_Timer.GetDuration().GetMicroseconds();

	RegisterAllocation( FindOrCreateEntry( pFileName, nLine ), nSize, nTime );
	RegisterAllocation( m_GlobalInfo, nSize, nTime );

	return pMem;
}

void  CDbgMemAlloc::Free( void *pMem, const char *pFileName, int nLine )
{
	if (!pMem)
		return;

	int nOldSize = GetSize( pMem );
	const char *pOldFileName = GetAllocatonFileName( pMem );
	int oldLine = GetAllocatonLineNumber( pMem );

	m_Timer.Start();
	_free_dbg( pMem, _NORMAL_BLOCK );
 	m_Timer.End();

	unsigned long nTime = m_Timer.GetDuration().GetMicroseconds();

	RegisterDeallocation( m_GlobalInfo, nOldSize, nTime );
	RegisterDeallocation( FindOrCreateEntry( pOldFileName, oldLine ), nOldSize, nTime );
}

void *CDbgMemAlloc::Expand( void *pMem, size_t nSize, const char *pFileName, int nLine )
{
	GetActualDbgInfo( pFileName, nLine );

	int nOldSize = GetSize( pMem );
	const char *pOldFileName = GetAllocatonFileName( pMem );
	int oldLine = GetAllocatonLineNumber( pMem );

	RegisterDeallocation( FindOrCreateEntry( pOldFileName, oldLine ), nOldSize, 0 );
	RegisterDeallocation( m_GlobalInfo, nOldSize, 0 );

	m_Timer.Start();
	pMem = _expand_dbg( pMem, nSize, _NORMAL_BLOCK, pFileName, nLine );
	m_Timer.End();

	unsigned long nTime = m_Timer.GetDuration().GetMicroseconds();

	RegisterAllocation( FindOrCreateEntry( pFileName, nLine ), nSize, nTime );
	RegisterAllocation( m_GlobalInfo, nSize, nTime );
	return pMem;
}


//-----------------------------------------------------------------------------
// Returns size of a particular allocation
//-----------------------------------------------------------------------------
size_t CDbgMemAlloc::GetSize( void *pMem )
{
	if (!pMem)
		return 0;

	return _msize_dbg( pMem, _NORMAL_BLOCK );
}


//-----------------------------------------------------------------------------
// FIXME: Remove when we make our own heap! Crt stuff we're currently using
//-----------------------------------------------------------------------------
long CDbgMemAlloc::CrtSetBreakAlloc( long lNewBreakAlloc )
{
	return _CrtSetBreakAlloc( lNewBreakAlloc );
}

int CDbgMemAlloc::CrtSetReportMode( int nReportType, int nReportMode )
{
	return _CrtSetReportMode( nReportType, nReportMode );
}

int CDbgMemAlloc::CrtIsValidHeapPointer( const void *pMem )
{
	return _CrtIsValidHeapPointer( pMem );
}

int CDbgMemAlloc::CrtCheckMemory( void )
{
	return CrtCheckMemory( );
}

int CDbgMemAlloc::CrtSetDbgFlag( int nNewFlag )
{
	return _CrtSetDbgFlag( nNewFlag );
}

void CDbgMemAlloc::CrtMemCheckpoint( _CrtMemState *pState )
{
	_CrtMemCheckpoint( pState );
}

// FIXME: Remove when we have our own allocator
void* CDbgMemAlloc::CrtSetReportFile( int nRptType, void* hFile )
{
	return (void*)_CrtSetReportFile( nRptType, (_HFILE)hFile );
}

void* CDbgMemAlloc::CrtSetReportHook( void* pfnNewHook )
{
	return (void*)_CrtSetReportHook( (_CRT_REPORT_HOOK)pfnNewHook );
}

int CDbgMemAlloc::CrtDbgReport( int nRptType, const char * szFile,
		int nLine, const char * szModule, const char * pMsg )
{
	return _CrtDbgReport( nRptType, szFile, nLine, szModule, pMsg );
}

int CDbgMemAlloc::heapchk()
{
	return _heapchk();
}

//-----------------------------------------------------------------------------
// Stat output
//-----------------------------------------------------------------------------
void CDbgMemAlloc::DumpMemInfo( const char *pAllocationName, int line, const MemInfo_t &info )
{
	m_OutputFunc("%s, line %i\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%d\t%d\t%d\t%d",
		pAllocationName,
		line,
		info.m_nCurrentSize / 1024.0f,
		info.m_nPeakSize / 1024.0f,
		info.m_nTotalSize / 1024.0f,
		info.m_nOverheadSize / 1024.0f,
		info.m_nPeakOverheadSize / 1024.0f,
		(int)(info.m_nTime / 1000),
		info.m_nCurrentCount,
		info.m_nPeakCount,
		info.m_nTotalCount
		);

	for (int i = 0; i < NUM_BYTE_COUNT_BUCKETS; ++i)
	{
		m_OutputFunc( "\t%d", info.m_pCount[i] );
	}

	m_OutputFunc("\n");
}


//-----------------------------------------------------------------------------
// Stat output
//-----------------------------------------------------------------------------
void CDbgMemAlloc::DumpFileStats()
{
	StatMapIter_t iter = m_StatMap.begin();
	while(iter != m_StatMap.end())
	{
		DumpMemInfo( iter->first.m_pFileName, iter->first.m_nLine, iter->second );
		iter++;
	}
}


//-----------------------------------------------------------------------------
// Stat output
//-----------------------------------------------------------------------------
void CDbgMemAlloc::DumpStats()
{
	if (m_OutputFunc == DefaultHeapReportFunc)
	{
		static int s_FileCount = 0;
		char pFileName[64];
		sprintf( pFileName, "memstats%d.txt", s_FileCount );
		++s_FileCount;

		s_DbgFile = fopen(pFileName, "wt");
		if (!s_DbgFile)
			return;
	}

	m_OutputFunc("Allocation type\tCurrent Size(k)\tPeak Size(k)\tTotal Allocations(k)\tOverhead Size(k)\tPeak Overhead Size(k)\tTime(ms)\tCurrent Count\tPeak Count\tTotal Count");

	for (int i = 0; i < NUM_BYTE_COUNT_BUCKETS; ++i)
	{
		m_OutputFunc( "\t%s", s_pCountHeader[i] );
	}

	m_OutputFunc("\n");

	DumpMemInfo( "Totals", 0, m_GlobalInfo );
	DumpFileStats();

	if (m_OutputFunc == DefaultHeapReportFunc)
	{
		fclose(s_DbgFile);
	}
}



#endif // _DEBUG