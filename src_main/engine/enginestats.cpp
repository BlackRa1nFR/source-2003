//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "enginestats.h"
#include "basetypes.h"
#include "clientstats.h"
#include "limits.h"
#include "sysexternal.h"
#include "gl_matsysiface.h"
#include "filesystem_engine.h"
#include "demo.h"
#include "tier0/vprof.h"

extern ConVar r_speeds;


const char *CEngineStats::s_CountedStatsName[] =
{
	"ENGINE_STATS_NUM_PRIMITIVES",
	"ENGINE_STATS_NUM_DRAW_CALLS",
	"ENGINE_STATS_MODEL_NUM_BONE_CHANGES",
	"ENGINE_STATS_LIGHTCACHE_MISSES",
	"ENGINE_STATS_LIGHTCACHE_STATICPROP_LIGHTSTYLE_MISSES",
	"ENGINE_STATS_LIGHTCACHE_QUERIES",
	"ENGINE_STATS_BRUSH_TRIANGLE_COUNT",
	"ENGINE_STATS_DECAL_TRIANGLES",
	"ENGINE_STATS_TRANSLUCENT_WORLD_TRIANGLES",
	"ENGINE_STATS_CLOCK_SAMPLE_COUNT",
	"ENGINE_STATS_SHADOWS_MOVED",
	"ENGINE_STATS_SHADOWS_CLIPPED",
	"ENGINE_STATS_NUM_WORLD_MATERIALS_RENDERED",
	"ENGINE_STATS_NUM_MRM_TRIS_RENDERED",
	"ENGINE_STATS_NUM_WORLD_TRIS_RENDERED",
	"ENGINE_STATS_NUM_DISP_TRIANGLES",
	"ENGINE_STATS_NUM_BOX_TRACES",
	"ENGINE_STATS_NUM_BOX_VISIBLE_TESTS",
	"ENGINE_STATS_NUM_DISP_SURFACES",
	"ENGINE_STATS_NUM_TRACE_LINES",

	"ENGINE_STATS_ACTUAL_MODEL_TRIANGLES",
	"ENGINE_STATS_EFFECTIVE_MODEL_TRIANGLES",

	"ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES",
	"ENGINE_STATS_EFFECTIVE_SHADOW_RENDER_TO_TEXTURE_TRIANGLES",

	"ENGINE_STATS_SHADOW_ACTUAL_TRIANGLES",
	"ENGINE_STATS_SHADOW_EFFECTIVE_TRIANGLES",

	"ENGINE_STATS_LIGHTMAP_LUXEL_UPDATES",

	"ENGINE_STATS_LOWEND_BRUSH_TRIANGLE_COUNT",
	"ENGINE_STATS_NUM_LOWEND_WORLD_TRIS_RENDERED",
};

const char *CEngineStats::s_TimedStatsName[] =
{
	"ENGINE_STATS_PLAYER_THINK",
	"ENGINE_STATS_ENTITY_THINK",
	"ENGINE_STATS_DYNAMIC_LIGHTMAP_BUILD",
	"ENGINE_STATS_DECAL_RENDER",

	"ENGINE_STATS_BRUSH_TRIANGLE_TIME",
	"ENGINE_STATS_SUBTREE_TRACE",
	"ENGINE_STATS_SUBTREE_BUILD",
	"ENGINE_STATS_SUBTREE_TRACE_WORLD",
	"ENGINE_STATS_SUBTREE_TRACE_LINKS",
	"ENGINE_STATS_SUBTREE_TRACE_SETUP",

	"ENGINE_STATS_TEST1",
	"ENGINE_STATS_TEST2",
	"ENGINE_STATS_TEST3",
	"ENGINE_STATS_TEST4",
	"ENGINE_STATS_TEST5",
	"ENGINE_STATS_TEST6",
	"ENGINE_STATS_TEST7",
	"ENGINE_STATS_TEST8",
	"ENGINE_STATS_TEST9",
	"ENGINE_STATS_TEST10",
	"ENGINE_STATS_TEST11",
	"ENGINE_STATS_TEST12",
	"ENGINE_STATS_TEST13",
	"ENGINE_STATS_TEST14",
	"ENGINE_STATS_TEST15",
	"ENGINE_STATS_TEST16",

	"ENGINE_STATS_SHADOW_RENDER_TIME",
	"ENGINE_STATS_SHADOW_TRIANGLE_TIME",

	"ENGINE_STATS_SETUP_BONES_TIME",

	"ENGINE_STATS_MRM_RENDER_TIME",
	"ENGINE_STATS_WORLD_RENDER_TIME",
	"ENGINE_STATS_BOX_VISIBLE_TEST_TIME",
	"ENGINE_STATS_DISP_RENDER_TIME",
	"ENGINE_STATS_UPDATE_MRM_LIGHTING_TIME",
	"ENGINE_STATS_TRACE_LINE_TIME",
	"ENGINE_STATS_FRAME_TIME",
	"ENGINE_STATS_BOX_TRACE_TIME",
	"ENGINE_STATS_FIND_ENVCUBEMAP_TIME",
	"ENGINE_STATS_FPS",
	"ENGINE_STATS_FPS_VARIABILITY",
};


//-----------------------------------------------------------------------------
// itty bitty interface for stat time
//-----------------------------------------------------------------------------

class CStatTime : public IClientStatsTime
{
public:
	float GetTime()
	{
		return Sys_FloatTime();
	}
};

CStatTime	g_StatTime;


CEngineStats::CEngineStats() : m_InFrame( false ), 
	m_LogCountedStats( NULL ), m_LogTimedStats( NULL ), m_LogNumFrames( 0 ),
	m_LogCurFrame( 0 )
{
	m_LogFileHandle = 0;
	COMPILE_TIME_ASSERT( ENGINE_STATS_NUM_COUNTED_STATS == sizeof( s_CountedStatsName ) / sizeof( s_CountedStatsName[0] ) );
	COMPILE_TIME_ASSERT( ENGINE_STATS_NUM_TIMED_STATS == sizeof( s_TimedStatsName ) / sizeof( s_TimedStatsName[0] ) );

	LogAllTimedStats( true );
	LogAllCountedStats( true );
	LogClientStats( true );

	m_bInRun = false;
}

// Allows us to hook in client stats too
void CEngineStats::InstallClientStats( IClientStats* pClientStats )
{
	Assert( !UsingLogFile() );
	m_ClientStats.AddToTail(pClientStats);

	// Hook in the time interface
	pClientStats->Init( &g_StatTime );
}

void CEngineStats::RemoveClientStats( IClientStats* pClientStats )
{
	Assert( !UsingLogFile() );
	int i = m_ClientStats.Find( pClientStats );
	if (i >= 0)
		m_ClientStats.Remove(i);
}

// Log filter
void CEngineStats::LogClientStats( bool bEnable )
{
	m_bShouldLogClientStats = bEnable;
}

void CEngineStats::LogTimedStat( EngineTimedStatId_t stat, bool bEnable )
{
	m_bShouldLogTimedStat[stat] = bEnable;
}

void CEngineStats::LogAllTimedStats( bool bEnable )
{						   
	for ( int i = 0; i < ENGINE_STATS_NUM_TIMED_STATS; ++i )
	{
		m_bShouldLogTimedStat[i] = bEnable;
	}
}

void CEngineStats::LogCountedStat( EngineCountedStatId_t stat, bool bEnable )
{
	m_bShouldLogCountedStat[stat] = bEnable;
}

void CEngineStats::LogAllCountedStats( bool bEnable )
{
	for ( int i = 0; i < ENGINE_STATS_NUM_COUNTED_STATS; ++i )
	{
		m_bShouldLogCountedStat[i] = bEnable;
	}
}

void CEngineStats::BeginRun( void )
{
	m_bInRun = true;
	m_totalNumFrames = 0;

	// Displacement triangles
	int i;
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->BeginRun();
	}

	int j;
	for (j = 0; j < ENGINE_STATS_NUM_TIMED_STATS; ++j)
	{
		m_StatGroup.m_TotalStatTime[j] = 0.0;
	}

	for (j = 0; j < ENGINE_STATS_NUM_COUNTED_STATS; ++j)
	{
		m_StatGroup.m_TotalStat[j] = 0;
		m_StatGroup.m_MinStatPerFrame[j] = INT_MAX;
		m_StatGroup.m_MaxStatPerFrame[j] = INT_MIN;
	}

	// frame timing data
	m_runStartTime = Sys_FloatTime();
}


void CEngineStats::EndRun( void )
{
	m_runEndTime = Sys_FloatTime();

	for ( int i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->EndRun();
	}
	m_bInRun = false;
}

void CEngineStats::BeginFrame( void )
{
	m_bPaused = false;
	m_InFrame = false;

	if ( demo->IsPlayingBack_TimeDemo() && !m_bInRun )
		return;

	if ( r_speeds.GetInt() != 0 || UsingLogFile() )
	{
		int i;
		for ( i = 0; i < m_ClientStats.Size(); ++i )
		{
			m_ClientStats[i]->BeginFrame();
		}

		int j;
		for (j = 0; j < ENGINE_STATS_NUM_TIMED_STATS; ++j)
		{
			m_StatGroup.m_StatFrameTime[j] = 0.0;
		}

		for (j = 0; j < ENGINE_STATS_NUM_COUNTED_STATS; ++j)
		{
			m_StatGroup.m_NumStatInFrame[j] = 0;
		}

		m_InFrame = true;
//		BeginTimedStat( ENGINE_STATS_FRAME_TIME );
	}
}

#define CHECK_COUNTED_STAT_PTR( p )	\
Assert( (p) >= m_LogCountedStats && (p) < m_LogCountedStats + m_LogNumFrames * m_LogTotalCountedStats )


#define CHECK_TIMED_STAT_PTR( p )	\
Assert( (p) >= m_LogTimedStats && (p) < m_LogTimedStats + m_LogNumFrames * m_LogTotalTimedStats )

static void PrintSpace( int space )
{
	int i;
	for( i = 0; i < space; i++ )
	{
		Warning( "." );
	}
}

#ifdef VPROF_ENABLED
static void PrintProfileNode( CVProfNode *pNode, int depth )
{
	PrintSpace( depth );
	Warning( "%s calls: %d time: %lfms\n", 
		pNode->GetName(), 
		pNode->GetTotalCalls(), 
		pNode->GetTotalTime() );
	if( pNode->GetChild() )
	{
		PrintProfileNode( pNode->GetChild(), depth + 1 );
	}

	if( pNode->GetSibling() )
	{
		PrintProfileNode( pNode->GetSibling(), depth );
	}
}
#endif


void CEngineStats::ComputeFrameTimeStats( void )
{
	m_StatGroup.m_StatFrameTime[ENGINE_STATS_FRAME_TIME] = m_flFrameTime / 1000.0f;
	m_StatGroup.m_StatFrameTime[ENGINE_STATS_FPS_VARIABILITY] = m_flFPSVariability / 1000.0f;
	m_StatGroup.m_StatFrameTime[ENGINE_STATS_FPS] = (m_flFrameTime != 0.0f) ? ( 1.0f / (1000.0f * m_flFrameTime) ) : 0.0f;
}


void CEngineStats::EndFrame( void )
{
	if (!m_InFrame)
		return;

	if ( demo->IsPlayingBack_TimeDemo() && !m_bInRun )
		return;

#ifdef VPROF_ENABLED
	//CVProfNode *pProfileNode =  g_ProfileManager.GetRoot();
	//PrintProfileNode( pProfileNode, 0 );
#endif

	if( r_speeds.GetInt() != 0 || m_LogFileHandle )
	{	
		double frameTrisPerSecond;
		
		ComputeFrameTimeStats();
		++m_totalNumFrames;
		m_InFrame = false;

		// Compute a couple other values....
		int i;
		int j;
		for (j = 0; j < ENGINE_STATS_NUM_TIMED_STATS; ++j)
		{
			m_StatGroup.m_TotalStatTime[j] += m_StatGroup.m_StatFrameTime[j];
		}

		for (j = 0; j < ENGINE_STATS_NUM_COUNTED_STATS; ++j)
		{
			m_StatGroup.m_TotalStat[j] += ( int64 )m_StatGroup.m_NumStatInFrame[j];
			if( m_StatGroup.m_NumStatInFrame[j] < m_StatGroup.m_MinStatPerFrame[j] )
			{
				m_StatGroup.m_MinStatPerFrame[j] = m_StatGroup.m_NumStatInFrame[j];
			}
			if( m_StatGroup.m_NumStatInFrame[j] > m_StatGroup.m_MaxStatPerFrame[j] )
			{
				m_StatGroup.m_MaxStatPerFrame[j] = m_StatGroup.m_NumStatInFrame[j];
			}
		}

		frameTrisPerSecond = GetCurrentFrameTrisPerSecond();
		if( frameTrisPerSecond < m_minTrisPerSecond )
		{
			m_minTrisPerSecond = frameTrisPerSecond;
		}
		if( frameTrisPerSecond > m_maxTrisPerSecond )
		{
			m_maxTrisPerSecond = frameTrisPerSecond;
		}
		
		for ( i = 0; i < m_ClientStats.Size(); ++i )
		{
			m_ClientStats[i]->EndFrame();
		}
	}
	// deal with logging if enabled
	if( m_LogFileHandle && m_LogNumFrames != 0 )
	{
#ifndef SWDS
		if( !demo->IsTimeDemoTimerStarted() )
		{
			return;
		}
#endif
		// FIXME!  We aren't getting the actual first frame of the time demo with respect
		// to how many frames the dem file says that it has.  Just skip the last few.
		//		Assert( m_LogCurFrame < m_LogNumFrames );
		if( m_LogCurFrame >= m_LogNumFrames )
		{
			return;
		}

		int totalCountedStats = GetTotalCountedStats();
		int totalTimedStats = GetTotalTimedStats();
		int i;
		int *pCountedStats = &m_LogCountedStats[totalCountedStats*m_LogCurFrame];
		CHECK_COUNTED_STAT_PTR( pCountedStats );
		for( i = 0; i < ENGINE_STATS_NUM_COUNTED_STATS; i++ )
		{
			CHECK_COUNTED_STAT_PTR( pCountedStats );
			*pCountedStats = CountedStatInFrame( ( EngineCountedStatId_t )i );
			pCountedStats++;
		}
		for ( i = 0; i < m_ClientStats.Size(); ++i )
		{
			int j;
			IClientStats *pClient = m_ClientStats[i];
			int saveStatGroup = pClient->GetCurrentStatGroup();
			for( j = 0; j < pClient->GetNumStatGroups(); j++ )
			{
				int k;
				pClient->SetStatGroup( j );
				for( k = 0; k < pClient->GetNumCountedStats(); k++ )
				{
					CHECK_COUNTED_STAT_PTR( pCountedStats );
					*pCountedStats = pClient->CountedStatInFrame( k );
					pCountedStats++;
				}
			}
			pClient->SetStatGroup( saveStatGroup );
		}

		double *pTimedStats = &m_LogTimedStats[totalTimedStats*m_LogCurFrame];
		CHECK_TIMED_STAT_PTR( pTimedStats );
		for( i = 0; i < ENGINE_STATS_NUM_TIMED_STATS; i++ )
		{
			CHECK_TIMED_STAT_PTR( pTimedStats );
			*pTimedStats = TimedStatInFrame( ( EngineTimedStatId_t )i );
			Assert( *pTimedStats >= 0.0 );
			pTimedStats++;
		}
		for ( i = 0; i < m_ClientStats.Size(); ++i )
		{
			int j;
			IClientStats *pClient = m_ClientStats[i];
			int saveStatGroup = pClient->GetCurrentStatGroup();
			for( j = 0; j < pClient->GetNumStatGroups(); j++ )
			{
				int k;
				pClient->SetStatGroup( j );
				for( k = 0; k < pClient->GetNumTimesStats(); k++ )
				{
					CHECK_TIMED_STAT_PTR( pTimedStats );
					*pTimedStats = pClient->TimedStatInFrame( k );
					Assert( *pTimedStats >= 0.0 );
					pTimedStats++;
				}
			}
			pClient->SetStatGroup( saveStatGroup );
		}
		m_LogCurFrame++;		
		Assert( pCountedStats == &m_LogCountedStats[totalCountedStats*m_LogCurFrame] );
		Assert( pTimedStats == &m_LogTimedStats[totalTimedStats*m_LogCurFrame] );
	}
}


//-----------------------------------------------------------------------------
// Advances the next frame for the stats...
//-----------------------------------------------------------------------------
void CEngineStats::NextFrame() 
{
	if ( r_speeds.GetInt() || UsingLogFile() )
	{
		EndFrame();
		BeginFrame();
	}
}


//-----------------------------------------------------------------------------
// Pause those stats!
//-----------------------------------------------------------------------------
void CEngineStats::PauseStats( bool bPaused )
{
	if (bPaused)
	{
		if (m_InFrame)
		{
			m_bPaused = true;
			m_InFrame = false;
		}
	}
	else	// !bPaused
	{
		if (m_bPaused)
		{
			m_InFrame = true;
			m_bPaused = false;
		}
	}
}


//-----------------------------------------------------------------------------
// retuns counted stats...
//-----------------------------------------------------------------------------
int CEngineStats::CountedStatInFrame( EngineCountedStatId_t stat ) const
{
	return m_StatGroup.m_NumStatInFrame[stat];
}

int64 CEngineStats::TotalCountedStat( EngineCountedStatId_t stat ) const
{
	return m_StatGroup.m_TotalStat[stat];
}

int CEngineStats::MinCountedStat( EngineCountedStatId_t stat ) const
{
	return m_StatGroup.m_MinStatPerFrame[stat];
}

int CEngineStats::MaxCountedStat( EngineCountedStatId_t stat ) const
{
	return m_StatGroup.m_MaxStatPerFrame[stat];
}



//-----------------------------------------------------------------------------
// returns timed stats
//-----------------------------------------------------------------------------

double CEngineStats::TimedStatInFrame( EngineTimedStatId_t stat ) const
{
	return m_StatGroup.m_StatFrameTime[stat];
}

double CEngineStats::TotalTimedStat( EngineTimedStatId_t stat ) const
{
	return m_StatGroup.m_TotalStatTime[stat];
}


void CEngineStats::DrawClientStats( IClientStatsTextDisplay* pDisplay, int category )
{
	for ( int i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->DisplayStats( pDisplay, category );
	}
}

void CEngineStats::BeginDrawWorld( void )
{
	if ( m_InFrame )
	{
		materialSystemInterface->Flush();
		materialSystemStatsInterface->SetStatGroup( MATERIAL_SYSTEM_STATS_WORLD );
		BeginTimedStat( ENGINE_STATS_WORLD_RENDER_TIME );
	}
}

void CEngineStats::EndDrawWorld( void )
{
	VPROF("CEngineStats::EndDrawWorld");
	if ( m_InFrame )
	{
		materialSystemInterface->Flush();
		materialSystemStatsInterface->SetStatGroup( MATERIAL_SYSTEM_STATS_OTHER );
		EndTimedStat( ENGINE_STATS_WORLD_RENDER_TIME );
	}
}

double CEngineStats::GetCurrentFrameTrisPerSecond( void )
{
	long double totalTrisInFrame = 
		( long double )( CountedStatInFrame( ENGINE_STATS_NUM_MRM_TRIS_RENDERED ) +
						 CountedStatInFrame(ENGINE_STATS_BRUSH_TRIANGLE_COUNT) + 
			 		     CountedStatInFrame( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED ) +
						 CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) + 
						 CountedStatInFrame( ENGINE_STATS_DECAL_TRIANGLES ) );
	return ( double )( totalTrisInFrame / ( TimedStatInFrame( ENGINE_STATS_FRAME_TIME ) ) );
}

double CEngineStats::GetAverageTrisPerSecond()
{
	long double totalNumTrisInRun = 
		( long double )( TotalCountedStat( ENGINE_STATS_NUM_MRM_TRIS_RENDERED ) + 
						 TotalCountedStat(ENGINE_STATS_BRUSH_TRIANGLE_COUNT) + 
			             TotalCountedStat( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED ) + 
						 CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) +
						 TotalCountedStat(ENGINE_STATS_DECAL_TRIANGLES) );
	return ( double )( totalNumTrisInRun / ( long double )( m_runEndTime - m_runStartTime ) );
}

double CEngineStats::GetMinTrisPerSecond()
{
	return m_minTrisPerSecond;
}

double CEngineStats::GetMaxTrisPerSecond()
{
	return m_maxTrisPerSecond;
}

double CEngineStats::GetRunTime( void )
{
	return m_runEndTime - m_runStartTime;
}

bool CEngineStats::OpenLogFile( const char *pFileName, int numFrames )
{
	m_LogNumFrames = numFrames;
	// Deal with the ouput file.
	Assert( m_LogFileHandle == 0 );
	m_LogFileHandle = g_pFileSystem->Open( pFileName, "w" );
	if( !m_LogFileHandle )
	{
		Warning("Unable to open log file %s\n", pFileName );
		return false;
	}

	m_LogTotalCountedStats = GetTotalCountedStats();
	m_LogTotalTimedStats = GetTotalTimedStats();
	// Allocate memory to store the stats.
	m_LogCountedStats = new int[m_LogNumFrames * GetTotalCountedStats()];
	m_LogTimedStats = new double[m_LogNumFrames * GetTotalTimedStats()];
	return true;
}

bool CEngineStats::CountedStatHasNonZeroValues( int statOffset )
{
	int frameID;
	for( frameID = 0; frameID < m_LogNumFrames; frameID++ )
	{
		int *pCountedStat = &m_LogCountedStats[frameID * m_LogTotalCountedStats + statOffset];
		CHECK_COUNTED_STAT_PTR( pCountedStat );
		if( *pCountedStat != 0 )
		{
			return true;
		}
	}
	return false;
}

bool CEngineStats::TimedStatHasNonZeroValues( int statOffset )
{
	int frameID;
	for( frameID = 0; frameID < m_LogNumFrames; frameID++ )
	{
		double *pTimedStat = &m_LogTimedStats[frameID * m_LogTotalTimedStats + statOffset];
		CHECK_TIMED_STAT_PTR( pTimedStat );
		if( *pTimedStat != 0.0 )
		{
			return true;
		}
	}
	return false;
}

void CEngineStats::PrintLogHeader( void )
{
	// print the headers for the columns.
	g_pFileSystem->FPrintf( m_LogFileHandle, "framenum," );
	int i;
	int statOffset = 0;
	for( i = 0; i < ENGINE_STATS_NUM_COUNTED_STATS; i++ )
	{
		if ( m_bShouldLogCountedStat[i] && CountedStatHasNonZeroValues( statOffset ) )
		{
			g_pFileSystem->FPrintf( m_LogFileHandle, "%s,", s_CountedStatsName[i] );
		}
		statOffset++;
	}
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		int j;
		IClientStats *pClient = m_ClientStats[i];
		int saveStatGroup = pClient->GetCurrentStatGroup();
		for( j = 0; j < pClient->GetNumStatGroups(); j++ )
		{
			int k;
			pClient->SetStatGroup( j );
			const char *pGroupName = pClient->GetStatGroupName( j );
			for( k = 0; k < pClient->GetNumCountedStats(); k++ )
			{
				if( m_bShouldLogClientStats && CountedStatHasNonZeroValues( statOffset ) )
				{
					if( pGroupName != '\0' )
					{
						g_pFileSystem->FPrintf( m_LogFileHandle, "%s::", pGroupName  );
					}
					g_pFileSystem->FPrintf( m_LogFileHandle, "%s,", pClient->GetCountedStatName( k )  );
				}
				statOffset++;
			}
		}
		pClient->SetStatGroup( saveStatGroup );
	}
	Assert( statOffset == m_LogTotalCountedStats );
	statOffset = 0;
	for( i = 0; i < ENGINE_STATS_NUM_TIMED_STATS; i++ )
	{
		if( m_bShouldLogTimedStat[i] && TimedStatHasNonZeroValues( statOffset ) )
		{
			g_pFileSystem->FPrintf( m_LogFileHandle, "%s,", s_TimedStatsName[i] );
		}
		statOffset++;
	}
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		int j;
		IClientStats *pClient = m_ClientStats[i];
		int saveStatGroup = pClient->GetCurrentStatGroup();
		for( j = 0; j < pClient->GetNumStatGroups(); j++ )
		{
			int k;
			pClient->SetStatGroup( j );
			const char *pGroupName = pClient->GetStatGroupName( j );
			for( k = 0; k < pClient->GetNumTimesStats(); k++ )
			{
				if( m_bShouldLogClientStats && TimedStatHasNonZeroValues( statOffset ) )
				{
					if( pGroupName != '\0' )
					{
						g_pFileSystem->FPrintf( m_LogFileHandle, "%s::", pGroupName  );
					}
					g_pFileSystem->FPrintf( m_LogFileHandle, "%s,", pClient->GetTimedStatName( k )  );
				}
				statOffset++;
			}
		}
		pClient->SetStatGroup( saveStatGroup );
	}
	Assert( statOffset == m_LogTotalTimedStats );
	g_pFileSystem->FPrintf( m_LogFileHandle, "\n" );
}

void CEngineStats::CloseLogFile( void )
{
	Assert( m_LogFileHandle );
	if( m_LogFileHandle )
	{
		PrintLogHeader();
		Assert( m_LogTotalCountedStats == GetTotalCountedStats() );
		Assert( m_LogTotalTimedStats == GetTotalTimedStats() );

		bool *pCountedStatHasNonZeroValues = new bool[m_LogTotalCountedStats];
		bool *pTimedStatHasNonZeroValues = new bool[m_LogTotalTimedStats];
		
		int i;
		for( i = 0; i < m_LogTotalCountedStats; i++ )
		{
			pCountedStatHasNonZeroValues[i] = CountedStatHasNonZeroValues( i );
		}
		for( i = 0; i < m_LogTotalTimedStats; i++ )
		{
			pTimedStatHasNonZeroValues[i] = TimedStatHasNonZeroValues( i );
		}

		int frame;
		int *pCountedStats = m_LogCountedStats;
		double *pTimedStats = m_LogTimedStats;
		CHECK_COUNTED_STAT_PTR( pCountedStats );
		CHECK_TIMED_STAT_PTR( pTimedStats );

		int nFrameCount = min( m_LogNumFrames, m_totalNumFrames );
		for( frame = 0; frame < nFrameCount; frame++ )
		{
			int statOffset = 0;
			g_pFileSystem->FPrintf( m_LogFileHandle, "%d,", frame );
			int i;
			CHECK_COUNTED_STAT_PTR( pCountedStats );
			for( i = 0; i < ENGINE_STATS_NUM_COUNTED_STATS; i++ )
			{
				CHECK_COUNTED_STAT_PTR( pCountedStats );
				if( m_bShouldLogCountedStat[i] && pCountedStatHasNonZeroValues[statOffset] )
				{
					g_pFileSystem->FPrintf( m_LogFileHandle, "%d,", *pCountedStats );
				}
				pCountedStats++;
				statOffset++;
			}
			for ( i = 0; i < m_ClientStats.Size(); ++i )
			{
				int j;
				IClientStats *pClient = m_ClientStats[i];
				int saveStatGroup = pClient->GetCurrentStatGroup();
				for( j = 0; j < pClient->GetNumStatGroups(); j++ )
				{
					int k;
					pClient->SetStatGroup( j );
					for( k = 0; k < pClient->GetNumCountedStats(); k++ )
					{
						CHECK_COUNTED_STAT_PTR( pCountedStats );
						if( m_bShouldLogClientStats && pCountedStatHasNonZeroValues[statOffset] )
						{
							g_pFileSystem->FPrintf( m_LogFileHandle, "%d,", *pCountedStats );
						}
						pCountedStats++;
						statOffset++;
					}
				}
				pClient->SetStatGroup( saveStatGroup );
			}
			Assert( statOffset == m_LogTotalCountedStats );

			statOffset = 0;
			CHECK_TIMED_STAT_PTR( pTimedStats );
			for( i = 0; i < ENGINE_STATS_NUM_TIMED_STATS; i++ )
			{
				CHECK_TIMED_STAT_PTR( pTimedStats );
				Assert( *pTimedStats >= 0.0 );
				if( m_bShouldLogTimedStat[i] && pTimedStatHasNonZeroValues[statOffset] )
				{
					g_pFileSystem->FPrintf( m_LogFileHandle, "%lf,", 1000.0 * *pTimedStats );
				}
				pTimedStats++;
				statOffset++;
			}
			for ( i = 0; i < m_ClientStats.Size(); ++i )
			{
				int j;
				IClientStats *pClient = m_ClientStats[i];
				int saveStatGroup = pClient->GetCurrentStatGroup();
				for( j = 0; j < pClient->GetNumStatGroups(); j++ )
				{
					int k;
					pClient->SetStatGroup( j );
					for( k = 0; k < pClient->GetNumTimesStats(); k++ )
					{
						CHECK_TIMED_STAT_PTR( pTimedStats );
						Assert( *pTimedStats >= 0.0 );
						if( m_bShouldLogClientStats && pTimedStatHasNonZeroValues[statOffset] )
						{
							g_pFileSystem->FPrintf( m_LogFileHandle, "%lf,", 1000.0 * *pTimedStats );
						}
						pTimedStats++;
						statOffset++;
					}
				}
				pClient->SetStatGroup( saveStatGroup );
			}
			Assert( statOffset == m_LogTotalTimedStats );
			g_pFileSystem->FPrintf( m_LogFileHandle, "\n" );
		}

		g_pFileSystem->Close( m_LogFileHandle );
		m_LogFileHandle = 0;
		delete [] pCountedStatHasNonZeroValues;
		delete [] pTimedStatHasNonZeroValues;
	}
	m_LogNumFrames = 0;
	m_LogCurFrame = 0;
	delete [] m_LogCountedStats;
	delete [] m_LogTimedStats;
	m_LogCountedStats = NULL;
	m_LogTimedStats = NULL;
}

bool CEngineStats::UsingLogFile( void )
{
	if( m_LogFileHandle )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Get the total number of counted stats, including clients
int CEngineStats::GetTotalCountedStats( void ) const
{
	int total = ENGINE_STATS_NUM_COUNTED_STATS;
	int i;
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		total += m_ClientStats[i]->GetNumCountedStats() * m_ClientStats[i]->GetNumStatGroups();
	}
	return total;
}

// Get the total number of counted stats, including clients
int CEngineStats::GetTotalTimedStats( void ) const
{
	int total = ENGINE_STATS_NUM_TIMED_STATS;
	int i;
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		total += m_ClientStats[i]->GetNumTimesStats() * m_ClientStats[i]->GetNumStatGroups();
	}
	return total;
}

