//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#ifndef CMATERIALSYSTEMSTATS_H
#define CMATERIALSYSTEMSTATS_H
#pragma once

#include "materialsystem/IMaterialSystemStats.h"
#include "ShaderAPIDX8_Global.h"

class CMaterialSystemStatsDX8 : public IMaterialSystemStats
{
public:
	// constructor, destructor
	CMaterialSystemStatsDX8();
	virtual ~CMaterialSystemStatsDX8();

	// This is called at startup to tell the stats about time
	void Init( IClientStatsTime* pTime ) { m_pTime = pTime; }

	// Begins/ends a particular run
	void BeginRun();
	void EndRun();

	// Call this at the beginning and end of every frame
	void BeginFrame();
	void EndFrame();

	// This method is called when it's time to draw the stats
	// The category is set at the console (r_speeds argument),
	// Use the method pDisplay->DrawStatsText to draw the stats text
	void DisplayStats( IClientStatsTextDisplay* pDisplay, int category ); 

	// These are used to delimit world rendering so that we can get separate
	// stats on world rendering and everything else.
	void SetStatGroup( int groupID );

	// Timed stat gathering
	void BeginTimedStat( MaterialSystemTimedStatId_t stat );
	void EndTimedStat( MaterialSystemTimedStatId_t stat );

	// Counted stat gathering
	void IncrementCountedStat( MaterialSystemCountedStatId_t stat, int increment, bool alwaysAdd = false );

	// retuns counted stats...
	int CountedStatInFrame( int /*MaterialSystemCountedStatId_t*/ stat ) const;
	__int64 TotalCountedStat( int /*MaterialSystemCountedStatId_t*/ stat ) const;
	int MinCountedStat( int /*MaterialSystemCountedStatId_t*/ stat ) const;
	int MaxCountedStat( int /*MaterialSystemCountedStatId_t*/ stat ) const;

	// returns timed stats
	double TimedStatInFrame( int /*MaterialSystemTimedStatId_t*/ stat ) const;
	double TotalTimedStat( int /*MaterialSystemTimedStatId_t*/ stat ) const;

	int GetNumStatGroups( void ) const;
	const char *GetStatGroupName( int groupID ) const;
	int GetCurrentStatGroup( void ) const;
	
	const char *GetCountedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS );
		return s_CountedStatsNames[statID];
	}
	const char *GetTimedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS );
		return s_TimedStatsNames[statID];
	}

private:
	struct StatGroupInfo_t
	{
		// Counted statistics
		int m_NumStatInFrame[MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS];
		__int64 m_TotalStat[MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS];
		int m_MinStatPerFrame[MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS];
		int m_MaxStatPerFrame[MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS];

		// Timed statistics
		double m_StatFrameTime[MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS];
		double m_StatStartTime[MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS];
		double m_TotalStatTime[MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS];
	};

	IClientStatsTime* m_pTime;
	StatGroupInfo_t m_StatGroup[MATERIAL_SYSTEM_STATS_NUM_GROUPS];
	MaterialSystemStatsGroup_t m_CurrentStatGroup;
	bool m_InFrame;

	static const char *s_TimedStatsNames[];
	static const char *s_CountedStatsNames[];
	static const char *s_StatGroupNames[];
};


//-----------------------------------------------------------------------------
// Inlined stat gathering methods
//-----------------------------------------------------------------------------

inline void CMaterialSystemStatsDX8::BeginTimedStat( MaterialSystemTimedStatId_t stat )
{
	if (m_InFrame)
	{
		m_StatGroup[m_CurrentStatGroup].m_StatStartTime[stat] = 
			m_pTime->GetTime();
	}
}

inline void CMaterialSystemStatsDX8::EndTimedStat( MaterialSystemTimedStatId_t stat )
{
	if (m_InFrame)
	{
		float dt = m_pTime->GetTime() - (float)m_StatGroup[m_CurrentStatGroup].m_StatStartTime[stat];
		m_StatGroup[m_CurrentStatGroup].m_StatFrameTime[stat] += dt; 
		if (m_CurrentStatGroup != MATERIAL_SYSTEM_STATS_TOTAL)
			m_StatGroup[MATERIAL_SYSTEM_STATS_TOTAL].m_StatFrameTime[stat] += dt;
	}
}

inline void CMaterialSystemStatsDX8::IncrementCountedStat( MaterialSystemCountedStatId_t stat, int inc, bool alwaysAdd  )
{
	if (m_InFrame || alwaysAdd)
	{
		m_StatGroup[m_CurrentStatGroup].m_NumStatInFrame[stat] += inc;
		if (m_CurrentStatGroup != MATERIAL_SYSTEM_STATS_TOTAL)
			m_StatGroup[MATERIAL_SYSTEM_STATS_TOTAL].m_NumStatInFrame[stat] += inc;
	}

	m_StatGroup[m_CurrentStatGroup].m_TotalStat[stat] += inc;
	if (m_CurrentStatGroup != MATERIAL_SYSTEM_STATS_TOTAL)
		m_StatGroup[MATERIAL_SYSTEM_STATS_TOTAL].m_TotalStat[stat] += inc;
}



//-----------------------------------------------------------------------------
// MEASURE_TIMED_STAT Macro used to time. Usage:
//
// void Foo()
// {
//		MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_VERTEX_COPY_TIME );
//		.. do stuff ...
// }
//
// The macro starts measuring on the line you define it and stops measuring
// when you leave the scope in which the macro was placed. You cannot measure
// the same stat twice within the same scope.
//-----------------------------------------------------------------------------

class CMeasureTimedStat
{
public:
	// constructor, destructor
	CMeasureTimedStat( MaterialSystemTimedStatId_t stat ) : m_Stat(stat)
	{
		MaterialSystemStats()->BeginTimedStat(m_Stat);
	}

	~CMeasureTimedStat()
	{
		MaterialSystemStats()->EndTimedStat(m_Stat);
	}

private:
	MaterialSystemTimedStatId_t m_Stat;
};

#ifdef IHVTEST
//#define MEASURE_STATS 1
#endif

//#define MEASURE_STATS 1

#ifdef MEASURE_STATS
#define MEASURE_TIMED_STAT( _id )	CMeasureTimedStat _stat ## _id ( _id )
#else
#define MEASURE_TIMED_STAT( _id ) 0
#endif

#define MEASURE_TIMED_STAT_DAMMIT( _id )	CMeasureTimedStat _stat ## _id ( _id )

#endif // CMATERIALSYSTEMSTATS_H
