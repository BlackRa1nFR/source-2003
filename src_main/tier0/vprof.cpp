//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Real-Time Hierarchical Profiling
//
// $NoKeywords: $
//=============================================================================

#ifdef _WIN32
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#endif

#pragma warning(push, 1)
#pragma warning(disable:4786)
#pragma warning(disable:4530)
#include <map>
#include <vector>
#include <algorithm>
#pragma warning(pop)

#include "tier0/vprof.h"

// NOTE: Explicitly and intentionally using STL in here to not generate any
// cyclical dependencies between the low-level debug library and the higher
// level data structures (toml 01-27-03)
using namespace std;

#ifdef VPROF_ENABLED

//-----------------------------------------------------------------------------

CVProfile g_VProfCurrentProfile;

#ifdef _WIN32

//-----------------------------------------------------------------------------

class CMemoryRegion
{
public:
	CMemoryRegion( unsigned maxSize, unsigned commitSize = 0 );
	~CMemoryRegion();

	bool Init();
	void Term();
	
	void *Alloc( unsigned bytes );
	void FreeAll();
	
	void Access( void **ppRegion, unsigned *pBytes );

private:
	unsigned char *m_pNextAlloc;
	unsigned char *m_pCommitLimit;
	
	unsigned char *m_pBase;
	unsigned	   m_commitSize;
	unsigned	   m_size;
#ifdef _DEBUG
	unsigned char *m_pAllocLimit;
#endif
};

//-------------------------------------

CMemoryRegion::CMemoryRegion( unsigned maxSize, unsigned commitSize )
 : 	m_pBase( NULL ),
 	m_commitSize( commitSize ),
 	m_size( maxSize )
{
	Init();
}
	
//-------------------------------------

CMemoryRegion::~CMemoryRegion()
{
	Term();
}

//-------------------------------------

bool CMemoryRegion::Init()
{
	Assert( !m_pBase );
	
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	
	if ( m_commitSize == 0 )
	{
		m_commitSize = sysInfo.dwPageSize;
	}
	else if ( m_commitSize % sysInfo.dwPageSize != 0 )
	{
		m_commitSize = m_commitSize + ( sysInfo.dwPageSize  - ( m_commitSize % sysInfo.dwPageSize ) );
	}

	if ( m_size % m_commitSize != 0 )
		m_size = m_size + ( m_commitSize - ( m_size % m_commitSize ) );
	
	Assert( m_size % sysInfo.dwPageSize == 0 && m_commitSize % sysInfo.dwPageSize == 0 && m_commitSize <= m_size );

	m_pBase = (unsigned char *)VirtualAlloc( NULL, m_size, MEM_RESERVE, PAGE_NOACCESS );
	Assert( m_pBase );
	m_pCommitLimit = m_pNextAlloc = m_pBase;

#ifdef _DEBUG
	m_pAllocLimit = m_pBase + m_size;
#endif

	return ( m_pBase != NULL );
}

//-------------------------------------

void *CMemoryRegion::Alloc( unsigned bytes )
{
	Assert( m_pBase );
	Assert( bytes <= m_commitSize );
	
	void *pResult = m_pNextAlloc;
	m_pNextAlloc += bytes;
	
	if ( m_pNextAlloc > m_pCommitLimit )
	{
		Assert( m_pCommitLimit + m_commitSize < m_pAllocLimit );
		if ( !VirtualAlloc( m_pCommitLimit, m_commitSize, MEM_COMMIT, PAGE_READWRITE ) )
		{
			Assert( 0 );
			return NULL;
		}
		m_pCommitLimit += m_commitSize;
	}
	
	return pResult;
}

//-------------------------------------

void CMemoryRegion::FreeAll()
{
	if ( m_pBase && m_pCommitLimit - m_pBase > 0 )
	{
		VirtualFree( m_pBase, m_pCommitLimit - m_pBase, MEM_DECOMMIT );
		m_pCommitLimit = m_pNextAlloc = m_pBase;
	}
}

//-------------------------------------

void CMemoryRegion::Term()
{
	FreeAll();
	if ( m_pBase )
	{
		VirtualFree( m_pBase, 0, MEM_RELEASE );
		m_pBase = NULL;
	}
}

//-------------------------------------

void CMemoryRegion::Access( void **ppRegion, unsigned *pBytes )
{
	*ppRegion = m_pBase;
	*pBytes = ( m_pNextAlloc - m_pBase);
}

//-----------------------------------------------------------------------------

static CMemoryRegion g_NodeMemRegion( 0x400000, 0x10000 );

//-------------------------------------

void *CVProfNode::operator new( size_t bytes )
{
	return g_NodeMemRegion.Alloc( bytes );
}

//-------------------------------------

void CVProfNode::FreeAll()
{
	g_NodeMemRegion.FreeAll();
}


//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------

void *CVProfNode::operator new( size_t bytes )
{
	return ::new char[bytes];
}

//-------------------------------------

void CVProfNode::operator delete( void *p )
{
	::delete [] p;
}
#endif

//-------------------------------------

CVProfNode *CVProfNode::GetSubNode( const char *pszName, int detailLevel, const char *pBudgetGroupName )
{
	// Try to find this sub node
	CVProfNode * child = m_pChild;
	while ( child ) 
	{
		if ( child->m_pszName == pszName ) 
		{
			return child;
		}
		child = child->m_pSibling;
	}

	// We didn't find it, so add it
	CVProfNode * node = new CVProfNode( pszName, detailLevel, this, pBudgetGroupName );
	node->m_pSibling = m_pChild;
	m_pChild = node;
	return node;
}

//-------------------------------------

void CVProfNode::Pause()
{
	if ( m_nRecursions > 0 ) 
	{
		m_Timer.End();
		m_CurFrameTime += m_Timer.GetDuration();
	}
	if ( m_pChild ) 
	{
		m_pChild->Pause();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->Pause();
	}
}

//-------------------------------------

void CVProfNode::Resume()
{
	if ( m_nRecursions > 0 ) 
	{
		m_Timer.Start();
	}
	if ( m_pChild ) 
	{
		m_pChild->Resume();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->Resume();
	}
}

//-------------------------------------

void CVProfNode::Reset()
{
	m_nPrevFrameCalls = 0;
	m_PrevFrameTime.Init();

	m_nCurFrameCalls = 0;
	m_CurFrameTime.Init();
	
	m_nTotalCalls = 0;
	m_TotalTime.Init();
	
	m_PeakTime.Init();

	if ( m_pChild ) 
	{
		m_pChild->Reset();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->Reset();
	}
}


//-------------------------------------

void CVProfNode::MarkFrame()
{
	m_nPrevFrameCalls = m_nCurFrameCalls;
	m_PrevFrameTime = m_CurFrameTime;

	m_nTotalCalls += m_nCurFrameCalls;
	m_TotalTime += m_CurFrameTime;
	
	if ( m_PeakTime.IsLessThan( m_CurFrameTime ) )
		m_PeakTime = m_CurFrameTime;
	
	m_CurFrameTime.Init();
	m_nCurFrameCalls = 0;

	if ( m_pChild ) 
	{
		m_pChild->MarkFrame();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->MarkFrame();
	}
}

//-------------------------------------

void CVProfNode::ResetPeak()
{
	m_PeakTime.Init();

	if ( m_pChild ) 
	{
		m_pChild->ResetPeak();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->ResetPeak();
	}
}

//-----------------------------------------------------------------------------

struct TimeSums_t
{
	const char *pszProfileScope;
	unsigned	calls;
	double 		time;
	double 		timeLessChildren;
	double		peak;
};

static bool TimeCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	return ( lhs.time > rhs.time );
}

static bool TimeLessChildrenCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	return ( lhs.timeLessChildren > rhs.timeLessChildren );
}

static bool PeakCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	return ( lhs.peak > rhs.peak );
}

static bool AverageTimeCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	double avgLhs = ( lhs.calls ) ? lhs.time / (double)lhs.calls : 0.0;
	double avgRhs = ( rhs.calls ) ? rhs.time / (double)rhs.calls : 0.0;
	return ( avgLhs > avgRhs );
}

static bool AverageTimeLessChildrenCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	double avgLhs = ( lhs.calls ) ? lhs.timeLessChildren / (double)lhs.calls : 0.0;
	double avgRhs = ( rhs.calls ) ? rhs.timeLessChildren / (double)rhs.calls : 0.0;
	return ( avgLhs > avgRhs );

}
static bool PeakOverAverageCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	double avgLhs = ( lhs.calls ) ? lhs.timeLessChildren / (double)lhs.calls : 0.0;
	double avgRhs = ( rhs.calls ) ? rhs.timeLessChildren / (double)rhs.calls : 0.0;
	
	double lhsPoA = ( avgLhs != 0 ) ? lhs.peak / avgLhs : 0.0;
	double rhsPoA = ( avgRhs != 0 ) ? rhs.peak / avgRhs : 0.0;
	
	return ( lhsPoA > rhsPoA );
}

map<CVProfNode *, double> 	g_TimesLessChildren;
map<const char *, unsigned> g_TimeSumsMap;
vector<TimeSums_t> 			g_TimeSums;

//-------------------------------------

void CVProfile::SumTimes( CVProfNode *pNode, int budgetGroupID )
{
	if ( !pNode )
		return; // this generally only happens on a failed FindNode()

	if ( GetRoot() != pNode )
	{
		if ( budgetGroupID == -1 || pNode->GetBudgetGroupID() == budgetGroupID )
		{
			double timeLessChildren = pNode->GetTotalTimeLessChildren();
			
			g_TimesLessChildren.insert( make_pair( pNode, timeLessChildren ) );
			
			map<const char *, unsigned>::iterator iter;
			iter = g_TimeSumsMap.find( pNode->GetName() ); // intenionally using address of string rather than string compare (toml 01-27-03)
			if ( iter == g_TimeSumsMap.end() )
			{
				TimeSums_t timeSums = { pNode->GetName(), pNode->GetTotalCalls(), pNode->GetTotalTime(), timeLessChildren, pNode->GetPeakTime() };
				g_TimeSumsMap.insert( make_pair( pNode->GetName(), g_TimeSums.size() ) );
				g_TimeSums.push_back( timeSums );
			}
			else
			{
				TimeSums_t &timeSums = g_TimeSums[iter->second];
				timeSums.calls += pNode->GetTotalCalls();
				timeSums.time += pNode->GetTotalTime();
				timeSums.timeLessChildren += timeLessChildren;
				if ( pNode->GetPeakTime() > timeSums.peak )
					timeSums.peak = pNode->GetPeakTime();
			}
		}
			
		if( pNode->GetSibling() )
		{
			SumTimes( pNode->GetSibling(), budgetGroupID );
		}
	}
	
	if( pNode->GetChild() )
	{
		SumTimes( pNode->GetChild(), budgetGroupID );
	}
		
}

//-------------------------------------

CVProfNode *CVProfile::FindNode( CVProfNode *pStartNode, const char *pszNode )
{
	if ( strcmp( pStartNode->GetName(), pszNode ) != 0 )
	{
		CVProfNode *pFoundNode = NULL;
		if ( pStartNode->GetSibling() )
		{
			pFoundNode = FindNode( pStartNode->GetSibling(), pszNode );
		}

		if ( !pFoundNode && pStartNode->GetChild() )
		{
			pFoundNode = FindNode( pStartNode->GetChild(), pszNode );
		}

		return pFoundNode;
	}
	return pStartNode;
}

//-------------------------------------

void CVProfile::SumTimes( const char *pszStartNode, int budgetGroupID )
{
	if ( GetRoot()->GetChild() )
	{
		CVProfNode *pStartNode;
		if ( pszStartNode == NULL )
			pStartNode = GetRoot();
		else
		{
			// This only works for nodes that show up only once in tree
			pStartNode = FindNode( GetRoot(), pszStartNode );
		}
		SumTimes( pStartNode, budgetGroupID );
	}

}

//-------------------------------------

void CVProfile::DumpNodes( CVProfNode *pNode, int indent )
{
	if ( !pNode )
		return; // this generally only happens on a failed FindNode()

	bool fIsRoot = ( pNode == GetRoot() );

	if ( !fIsRoot )
	{
		for ( int i = 0; i < indent; i++ )
			Msg( "  " );

		map<CVProfNode *, double>::iterator iterTimeLessChildren = g_TimesLessChildren.find( pNode );
			
		Msg("%s: Sum[%dc, func+child %.3f, func %.3f] Avg[ func+child %.3f, func %.3f], Peak[%.3f]\n", 
					 pNode->GetName(), 
					 pNode->GetTotalCalls(), pNode->GetTotalTime(), iterTimeLessChildren->second,
					 ( pNode->GetTotalCalls() > 0 ) ? pNode->GetTotalTime() / (double)pNode->GetTotalCalls() : 0, 
					 ( pNode->GetTotalCalls() > 0 ) ? iterTimeLessChildren->second / (double)pNode->GetTotalCalls() : 0, 
					 pNode->GetPeakTime()  );
		
	}

	if( pNode->GetChild() )
	{
		DumpNodes( pNode->GetChild(), indent + 1 );
	}
	
	if( !fIsRoot && pNode->GetSibling() )
	{
		DumpNodes( pNode->GetSibling(), indent );
	}
}

//-------------------------------------

static void DumpSorted( const char *pszHeading, double totalTime, bool (*pfnSort)( const TimeSums_t &, const TimeSums_t & ) )
{
	unsigned i;
	vector<TimeSums_t> sortedSums;
	sortedSums = g_TimeSums;
	sort( sortedSums.begin(), sortedSums.end(), pfnSort );
	
	Msg( "%s\n", pszHeading);
    Msg( "  Scope                                                      Calls  Time+Child    Pct        Time    Pct   Avg+Child         Avg        Peak\n");
    Msg( "  ---------------------------------------------------- ----------- ----------- ------ ----------- ------ ----------- ----------- -----------\n");
    for ( i = 0; i < sortedSums.size(); i++ )
    {
		double avg = ( sortedSums[i].calls ) ? sortedSums[i].time / (double)sortedSums[i].calls : 0.0;
		double avgLessChildren = ( sortedSums[i].calls ) ? sortedSums[i].timeLessChildren / (double)sortedSums[i].calls : 0.0;
		
        Msg( "  %52s%12d%12.3f%6.2f%%%12.3f%6.2f%%%12.3f%12.3f%12.3f\n", 
             sortedSums[i].pszProfileScope,
             sortedSums[i].calls,
			 sortedSums[i].time,
			 ( sortedSums[i].time / totalTime ) * 100.0,
			 sortedSums[i].timeLessChildren,
			 ( sortedSums[i].timeLessChildren / totalTime ) * 100.0,
			 avg,
			 avgLessChildren,
			 sortedSums[i].peak );
	}
}

//-------------------------------------

void CVProfile::OutputReport( VProfReportType_t type, const char *pszStartNode, int budgetGroupID )
{
	Msg( "******** BEGIN VPROF REPORT ********\n");
	Msg( "  (note: this report exceeds the output capacity of MSVC debug window. Use console window or console log.) \n");
	
	if ( NumFramesSampled() == 0 || GetTotalTimeSampled() == 0)
		Msg( "No samples\n" );
	else
	{
		if ( type == VPRT_SUMMARY || type == VPRT_FULL )
		{
			Msg( "-- Summary --\n" );
			Msg( "%d frames sampled for %.2f seconds\n", NumFramesSampled(), GetTotalTimeSampled() / 1000.0 );
			Msg( "Average %.2f fps, %.2f ms per frame\n", 1000.0 / ( GetTotalTimeSampled() / NumFramesSampled() ), GetTotalTimeSampled() / NumFramesSampled() );
			Msg( "Peak %.2f ms frame\n", GetPeakFrameTime() );
			
			double timeAccountedFor = 100.0 - ( m_Root.GetTotalTimeLessChildren() / m_Root.GetTotalTime() );
			Msg( "%.0f pct of time accounted for\n", min( 100.0, timeAccountedFor ) );
			Msg( "\n" );
		}

		if ( pszStartNode == NULL )
		{
			pszStartNode = GetRoot()->GetName();
		}

		SumTimes( pszStartNode, budgetGroupID );
		
		// Dump the hierarchy
		if ( type == VPRT_HIERARCHY || type == VPRT_FULL )
		{
			Msg( "-- Hierarchical Call Graph --\n");
			DumpNodes( (pszStartNode == NULL ) ? GetRoot() : FindNode( GetRoot(), pszStartNode ), 0 );
			Msg( "\n" );
		}
		
		if ( type == VPRT_LIST_BY_TIME || type == VPRT_FULL )
		{
			DumpSorted( "-- Profile scopes sorted by time (including children) --", GetTotalTimeSampled(), TimeCompare );
			Msg( "\n" );
		}
		if ( type == VPRT_LIST_BY_TIME_LESS_CHILDREN || type == VPRT_FULL )
		{
			DumpSorted( "-- Profile scopes sorted by time (without children) --", GetTotalTimeSampled(), TimeLessChildrenCompare );
			Msg( "\n" );
		}
		if ( type == VPRT_LIST_BY_AVG_TIME || type == VPRT_FULL )
		{
			DumpSorted( "-- Profile scopes sorted by average time (including children) --", GetTotalTimeSampled(), AverageTimeCompare );
			Msg( "\n" );
		}
		if ( type == VPRT_LIST_BY_AVG_TIME_LESS_CHILDREN || type == VPRT_FULL )
		{
			DumpSorted( "-- Profile scopes sorted by average time (without children) --", GetTotalTimeSampled(), AverageTimeLessChildrenCompare );
			Msg( "\n" );
		}
		if ( type == VPRT_LIST_BY_PEAK_TIME || type == VPRT_FULL )
		{
			DumpSorted( "-- Profile scopes sorted by peak --", GetTotalTimeSampled(), PeakCompare );
			Msg( "\n" );
		}
		if ( type == VPRT_LIST_BY_PEAK_OVER_AVERAGE || type == VPRT_FULL )
		{
			DumpSorted( "-- Profile scopes sorted by peak over average (including children) --", GetTotalTimeSampled(), PeakOverAverageCompare );
			Msg( "\n" );
		}
		
		// TODO: Functions by time less children
		// TODO: Functions by time averages
		// TODO: Functions by peak
		// TODO: Peak deviation from average
		g_TimesLessChildren.clear();
		g_TimeSumsMap.clear();
		g_TimeSums.clear();
	}
	Msg( "******** END VPROF REPORT ********\n");

}

//=============================================================================

CVProfile::CVProfile() 
 :	m_Root( "Root", 0, NULL, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED ), 
 	m_pCurNode( &m_Root ), 
 	m_nFrames( 0 ),
 	m_enabled( 0 ),
 	m_pausedEnabledDepth( 0 ),
	m_fAtRoot( true )
{
	// Go ahead and allocate 32 slots for budget group names
	m_pBudgetGroupNames = ( char ** )malloc( sizeof( char * ) * 32 );
	m_nBudgetGroupNames = 0;
	m_nBudgetGroupNamesAllocated = 32;
		
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_UNACCOUNTED );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_WORLD_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DISPLACEMENT_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_GAME );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_NPCS );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_NPC_ANIM );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PHYSICS );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_STATICPROP_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_MODEL_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_BRUSHMODEL_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_SHADOW_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_ROPES );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DLIGHT_RENDERING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_NETWORKING );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_ANIMATION );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_SOUND );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_VGUI );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_FILESYSTEM );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PREDICTION );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_INTERPOLATION );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_SWAP_BUFFERS );
}

CVProfile::~CVProfile()
{
#ifdef _WIN32
	CVProfNode::FreeAll();
#endif
	int i;
	for( i = 0; i < m_nBudgetGroupNames; i++ )
	{
		free( m_pBudgetGroupNames[i] );
	}
	free( m_pBudgetGroupNames );
}

const char *CVProfile::GetBudgetGroupName( int budgetGroupID )
{
//	COMPILE_TIME_ASSERT( sizeof( g_pBudgetGroupNames ) / sizeof( g_pBudgetGroupNames[0] ) == VPROF_NUM_BUDGETGROUPS );
	Assert( budgetGroupID >= 0 && budgetGroupID < m_nBudgetGroupNames );
	return m_pBudgetGroupNames[budgetGroupID];
}

#define COLORMIN 160
#define COLORMAX 255

static int g_ColorLookup[4] = 
{
	COLORMIN, 
	COLORMAX,
	COLORMIN+(COLORMAX-COLORMIN)/3,
	COLORMIN+((COLORMAX-COLORMIN)*2)/3, 
};

#define GET_BIT( val, bitnum ) ( ( val >> bitnum ) & 0x1 )

void CVProfile::GetBudgetGroupColor( int budgetGroupID, int &r, int &g, int &b, int &a )
{
	budgetGroupID = budgetGroupID % ( 1 << 6 );

	int index;
	index = GET_BIT( budgetGroupID, 0 ) | ( GET_BIT( budgetGroupID, 5 ) << 1 );
	r = g_ColorLookup[index];
	index = GET_BIT( budgetGroupID, 1 ) | ( GET_BIT( budgetGroupID, 4 ) << 1 );
	g = g_ColorLookup[index];
	index = GET_BIT( budgetGroupID, 2 ) | ( GET_BIT( budgetGroupID, 3 ) << 1 );
	b = g_ColorLookup[index];
	a = 255;
}

// return -1 if it doesn't exist.
int CVProfile::FindBudgetGroupName( const char *pBudgetGroupName )
{
	int i;
	for( i = 0; i < m_nBudgetGroupNames; i++ )
	{
		if( stricmp( pBudgetGroupName, m_pBudgetGroupNames[i] ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

int CVProfile::AddBudgetGroupName( const char *pBudgetGroupName )
{
	char *pNewString = ( char * )malloc( strlen( pBudgetGroupName ) + 1 );
	strcpy( pNewString, pBudgetGroupName );
	if( m_nBudgetGroupNames + 1 > m_nBudgetGroupNamesAllocated )
	{
		m_nBudgetGroupNamesAllocated *= 2;
		m_pBudgetGroupNames = ( char ** )realloc( m_pBudgetGroupNames, sizeof( char * ) * m_nBudgetGroupNamesAllocated );
	}

	m_pBudgetGroupNames[m_nBudgetGroupNames] = pNewString;
	m_nBudgetGroupNames++;
	if( m_pNumBudgetGroupsChangedCallBack )
	{
		(*m_pNumBudgetGroupsChangedCallBack)();
	}
	return m_nBudgetGroupNames - 1;
}

int CVProfile::BudgetGroupNameToBudgetGroupID( const char *pBudgetGroupName )
{
	int budgetGroupID = FindBudgetGroupName( pBudgetGroupName );
	if( budgetGroupID == -1 )
	{
		budgetGroupID = AddBudgetGroupName( pBudgetGroupName );
	}
	return budgetGroupID;
}

int CVProfile::GetNumBudgetGroups( void )
{
	return m_nBudgetGroupNames;
}

void CVProfile::RegisterNumBudgetGroupsChangedCallBack( void (*pCallBack)(void) )
{
	m_pNumBudgetGroupsChangedCallBack = pCallBack;
}

#endif	

