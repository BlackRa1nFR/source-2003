//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ENGINESTATS_H
#define ENGINESTATS_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "sysexternal.h"
#include "filesystem.h" // FileHandle_t define

class IClientStats;
struct IClientStatsTextDisplay;

enum
{
	ES_TRACER_SETUP = 0,
	ES_TRACER_CULL,
	ES_TRACER_RENDER,
	ES_TRACER_PHYSICS,

	ES_NUM_TRACER
};

enum EngineCountedStatId_t
{
	ENGINE_STATS_NUM_PRIMITIVES = 0,
	ENGINE_STATS_NUM_DRAW_CALLS,
	ENGINE_STATS_MODEL_NUM_BONE_CHANGES,
	ENGINE_STATS_LIGHTCACHE_MISSES,
	ENGINE_STATS_LIGHTCACHE_STATICPROP_LIGHTSTYLE_MISSES,
	ENGINE_STATS_LIGHTCACHE_QUERIES,
	ENGINE_STATS_BRUSH_TRIANGLE_COUNT,
	ENGINE_STATS_DECAL_TRIANGLES,
	ENGINE_STATS_TRANSLUCENT_WORLD_TRIANGLES,
	ENGINE_STATS_CLOCK_SAMPLE_COUNT,
	ENGINE_STATS_SHADOWS_MOVED,
	ENGINE_STATS_SHADOWS_CLIPPED,
	ENGINE_STATS_NUM_WORLD_MATERIALS_RENDERED,
	ENGINE_STATS_NUM_MRM_TRIS_RENDERED,
	ENGINE_STATS_NUM_WORLD_TRIS_RENDERED,
	ENGINE_STATS_NUM_DISP_TRIANGLES,
	ENGINE_STATS_NUM_BOX_TRACES,
	ENGINE_STATS_NUM_BOX_VISIBLE_TESTS,
	ENGINE_STATS_NUM_DISP_SURFACES,
	ENGINE_STATS_NUM_TRACE_LINES,

	ENGINE_STATS_ACTUAL_MODEL_TRIANGLES,
	ENGINE_STATS_EFFECTIVE_MODEL_TRIANGLES,

	ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES,
	ENGINE_STATS_EFFECTIVE_SHADOW_RENDER_TO_TEXTURE_TRIANGLES,

	ENGINE_STATS_SHADOW_ACTUAL_TRIANGLES,
	ENGINE_STATS_SHADOW_EFFECTIVE_TRIANGLES,

	// FIXME: Should differentiate between bumped and unbumped since the perf characteristics
	// are completely different?
	ENGINE_STATS_LIGHTMAP_LUXEL_UPDATES,

	ENGINE_STATS_LOWEND_BRUSH_TRIANGLE_COUNT,
	ENGINE_STATS_NUM_LOWEND_WORLD_TRIS_RENDERED,
	ENGINE_STATS_NUM_COUNTED_STATS
};

enum EngineTimedStatId_t
{
	ENGINE_STATS_PLAYER_THINK = 0,
	ENGINE_STATS_ENTITY_THINK,
	ENGINE_STATS_DYNAMIC_LIGHTMAP_BUILD,
	ENGINE_STATS_DECAL_RENDER,

	ENGINE_STATS_BRUSH_TRIANGLE_TIME,
	ENGINE_STATS_SUBTREE_TRACE,
	ENGINE_STATS_SUBTREE_BUILD,
	ENGINE_STATS_SUBTREE_TRACE_WORLD,
	ENGINE_STATS_SUBTREE_TRACE_LINKS,
	ENGINE_STATS_SUBTREE_TRACE_SETUP,

	ENGINE_STATS_TEST1,
	ENGINE_STATS_TEST2,
	ENGINE_STATS_TEST3,
	ENGINE_STATS_TEST4,
	ENGINE_STATS_TEST5,
	ENGINE_STATS_TEST6,
	ENGINE_STATS_TEST7,
	ENGINE_STATS_TEST8,
	ENGINE_STATS_TEST9,
	ENGINE_STATS_TEST10,
	ENGINE_STATS_TEST11,
	ENGINE_STATS_TEST12,
	ENGINE_STATS_TEST13,
	ENGINE_STATS_TEST14,
	ENGINE_STATS_TEST15,
	ENGINE_STATS_TEST16,

	ENGINE_STATS_SHADOW_RENDER_TIME,
	ENGINE_STATS_SHADOW_TRIANGLE_TIME,

	ENGINE_STATS_SETUP_BONES_TIME,

	ENGINE_STATS_MRM_RENDER_TIME,
	ENGINE_STATS_WORLD_RENDER_TIME,
	ENGINE_STATS_BOX_VISIBLE_TEST_TIME,
	ENGINE_STATS_DISP_RENDER_TIME,
	ENGINE_STATS_UPDATE_MRM_LIGHTING_TIME,
	ENGINE_STATS_TRACE_LINE_TIME,
	ENGINE_STATS_FRAME_TIME,
	ENGINE_STATS_BOX_TRACE_TIME,
	ENGINE_STATS_FIND_ENVCUBEMAP_TIME,
	ENGINE_STATS_FPS, // this is calculated at EndFrame!
	ENGINE_STATS_FPS_VARIABILITY,

	ENGINE_STATS_NUM_TIMED_STATS,
};

class CEngineStats
{
public:
	CEngineStats();

	// Allows us to hook in client stats too
	void InstallClientStats( IClientStats* pClientStats );
	void RemoveClientStats( IClientStats* pClientStats );
	
	// Tells the clients to draw its stats at a particular location
	void DrawClientStats( IClientStatsTextDisplay* pDisplay, int category );

	//
	// stats input
	//

	void BeginRun( void );

	// Advances the next frame for the stats...
	void NextFrame(); 

	void BeginFrame( void );

	// Timed stat gathering
	void BeginTimedStat( EngineTimedStatId_t stat );
	void EndTimedStat( EngineTimedStatId_t stat );

	// Adds to a timed stat...
	void AddToTimedStat( EngineTimedStatId_t stat, float time );

	// Slams a timed stat
	void SetTimedStat( EngineTimedStatId_t stat, float time );

	// Counted stat gathering
	void IncrementCountedStat( EngineCountedStatId_t stat, int increment );

	// returns counted stats...
	int CountedStatInFrame( EngineCountedStatId_t stat ) const;
	int64 TotalCountedStat( EngineCountedStatId_t stat ) const;
	int MinCountedStat( EngineCountedStatId_t stat ) const;
	int MaxCountedStat( EngineCountedStatId_t stat ) const;

	// returns timed stats
	double TimedStatInFrame( EngineTimedStatId_t stat ) const;
	double TotalTimedStat( EngineTimedStatId_t stat ) const;

	void BeginDrawWorld( void );
	void EndDrawWorld( void );

	void EndFrame( void );
	void EndRun( void );

	void PauseStats( bool bPaused );

	//
	// stats output
	// call these outside of a BeginFrame/EndFrame pair
	//

	double GetCurrentFrameTrisPerSecond( void );
	double GetAverageTrisPerSecond();
	double GetMinTrisPerSecond();
	double GetMaxTrisPerSecond();

	double GetRunTime( void );
	
//	double	GetHUDRedrawTime()					{return m_HUDRedrawTime;}
//	void 	SetHUDRedrawTime(double redrawTime)	{m_HUDRedrawTime = redrawTime;}

	void SetFrameTime( float flFrameTime ) { m_flFrameTime = flFrameTime; }
	void SetFPSVariability( float flFPSVariability ) { m_flFPSVariability = flFPSVariability; }

	bool OpenLogFile( const char *pFileName, int numFrames );
	void CloseLogFile();
	bool UsingLogFile( void );

	// Log filter
	void LogClientStats( bool bEnable );
	void LogTimedStat( EngineTimedStatId_t stat, bool bEnable );
	void LogAllTimedStats( bool bEnable );
	void LogCountedStat( EngineCountedStatId_t stat, bool bEnable );
	void LogAllCountedStats( bool bEnable );

	int FrameCount() const { return m_totalNumFrames; }

private:
	// Get the total number of counted stats, including clients
	int GetTotalCountedStats( void ) const;

	// Get the total number of counted stats, including clients
	int GetTotalTimedStats( void ) const;
	
	void PrintLogHeader( void );
	
	bool CountedStatHasNonZeroValues( int statOffset );
	bool TimedStatHasNonZeroValues( int statOffset );

	void ComputeFrameTimeStats( void );

	// Allows the client to draw its own stats
	CUtlVector<IClientStats*> m_ClientStats;

	// How many frames worth of data have we logged?
	int m_totalNumFrames;

	// run timing data
	double m_runStartTime;
	double m_runEndTime;

	// triangle rate data
	double m_minTrisPerSecond;
	double m_maxTrisPerSecond;
	
	struct StatGroupInfo_t
	{
		// Counted statistics
		int m_NumStatInFrame[ENGINE_STATS_NUM_COUNTED_STATS];
		int64 m_TotalStat[ENGINE_STATS_NUM_COUNTED_STATS];
		int m_MinStatPerFrame[ENGINE_STATS_NUM_COUNTED_STATS];
		int m_MaxStatPerFrame[ENGINE_STATS_NUM_COUNTED_STATS];

		// Timed statistics
		double m_StatFrameTime[ENGINE_STATS_NUM_TIMED_STATS];
		double m_StatStartTime[ENGINE_STATS_NUM_TIMED_STATS];
		double m_TotalStatTime[ENGINE_STATS_NUM_TIMED_STATS];
	};

	StatGroupInfo_t m_StatGroup;
	bool m_InFrame;

	FileHandle_t m_LogFileHandle;
	int m_LogNumFrames;
	int m_LogCurFrame;
	int *m_LogCountedStats;
	double *m_LogTimedStats;
	int m_LogTotalCountedStats;
	int m_LogTotalTimedStats;
	bool m_bPaused;
	bool m_bInRun;

	bool m_bShouldLogClientStats;
	bool m_bShouldLogCountedStat[ENGINE_STATS_NUM_COUNTED_STATS];
	bool m_bShouldLogTimedStat[ENGINE_STATS_NUM_TIMED_STATS];

	float m_flFrameTime;
	float m_flFPSVariability;

	static const char *s_CountedStatsName[];
	static const char *s_TimedStatsName[];
};


//-----------------------------------------------------------------------------
// Inlined stat gathering methods
//-----------------------------------------------------------------------------
inline void CEngineStats::BeginTimedStat( EngineTimedStatId_t stat )
{
	if (m_InFrame)
	{
		m_StatGroup.m_StatStartTime[stat] = 
			Sys_FloatTime();
	}
}

inline void CEngineStats::EndTimedStat( EngineTimedStatId_t stat )
{
	if (m_InFrame)
	{
		float dt = (float)Sys_FloatTime() - (float)(m_StatGroup.m_StatStartTime[stat]);
		m_StatGroup.m_StatFrameTime[stat] += dt; 
	}
}

// Adds to a timed stat...
inline void CEngineStats::AddToTimedStat( EngineTimedStatId_t stat, float dt )
{
	if (m_InFrame)
	{
		m_StatGroup.m_StatFrameTime[stat] += dt; 
	}
}

// Slams a timed stat
inline void CEngineStats::SetTimedStat( EngineTimedStatId_t stat, float time )
{
	m_StatGroup.m_StatFrameTime[stat] = time; 
}

inline void CEngineStats::IncrementCountedStat( EngineCountedStatId_t stat, int inc )
{
	if (m_InFrame)
	{
		m_StatGroup.m_NumStatInFrame[stat] += inc;
	}
}


extern CEngineStats g_EngineStats;


//-----------------------------------------------------------------------------
// MEASURE_TIMED_STAT Macro used to time. Usage:
//
// void Foo()
// {
//		MEASURE_TIMED_STAT( ENGINE_STATS_VERTEX_COPY_TIME );
//		.. do stuff ...
// }
//
// The macro starts measuring on the line you define it and stops measuring
// when you leave the scope in which the macro was placed. You cannot measure
// the same stat twice within the same scope.
//-----------------------------------------------------------------------------

extern ConVar r_speeds;

class CMeasureTimedStat
{
public:
	// constructor, destructor
	CMeasureTimedStat( EngineTimedStatId_t stat ) : m_Stat(stat)
	{
		if( r_speeds.GetBool() )
		{
			g_EngineStats.BeginTimedStat(m_Stat);
		}
	}

	~CMeasureTimedStat()
	{
		if( r_speeds.GetBool() )
		{
			g_EngineStats.EndTimedStat(m_Stat);
		}
	}

private:
	EngineTimedStatId_t m_Stat;
};


#define MEASURE_TIMED_STAT( _id )	CMeasureTimedStat _stat ## _id ( _id )

#endif // ENGINESTATS_H
