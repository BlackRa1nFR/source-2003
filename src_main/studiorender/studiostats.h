//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The base interface 
//
// $Id: $
//
// The implementation of the client stats; exists as a singleton accessed
// by the clientstats global variable
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#if !defined( STUDIOSTATS_H )
#define STUDIOSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "clientstats.h"
#include "cstudiorender.h"

//-----------------------------------------------------------------------------
// IClientStats defines two different kinds of statistics: timed and counted.
//
// To add a timed statistic, simply add an enumeration to TimedStatId_t, and
// use either ClientStats().BeginTimedStat() / ClientStats().EndTimedStat()
// or the MEASURE_TIMED_STAT macro to measure time for that statistic.
//
// To add a counted statistic, add an enumeration to CountedStatId_t
// and use ClientStats().IncrementCountedStat() to increase or decrease the 
// count for that statistic.
//
// To display the statistic, you'll need to modify the implementation of
// C_ClientStats::DisplayStats().
//
// The system automagically deals with keeping track of the min and max counts
// in a single frame, along with the total number of counts in the entire run.
// It also keeps track of the total stat time in the entire run.
//-----------------------------------------------------------------------------

enum StudioTimedStatId_t
{
	STUDIO_STATS_HW_SKIN_TIME = 0,
	STUDIO_STATS_STATIC_MESH_TIME,
	STUDIO_STATS_SET_BONE_WEIGHTS,
	STUDIO_STATS_SET_BONE_MATRICES,
	STUDIO_STATS_DRAW_MESH,
	STUDIO_STATS_SAVE_MATRIX,
	STUDIO_STATS_EYE_TIME,
	STUDIO_STATS_RENDER_FINAL,

	STUDIO_STATS_TEST1,
	STUDIO_STATS_TEST2,
	STUDIO_STATS_TEST3,
	STUDIO_STATS_TEST4,
	STUDIO_STATS_TEST5,

// NOTE!!!!  If you add to this, make sure and update the names in studiostats.cpp
	STUDIO_NUM_TIMED_STATS
};

enum StudioCountedStatId_t
{
	STUDIO_STATS_MODEL_NUM_BONE_CHANGES = 0,
// NOTE!!!!  If you add to this, make sure and update the names in studiostats.cpp

	STUDIO_NUM_COUNTED_STATS
};

class CStudioStats : public CBaseClientStats< STUDIO_NUM_TIMED_STATS, STUDIO_NUM_COUNTED_STATS>
{
public:
	CStudioStats::CStudioStats();

	// This method is called when it's time to draw the stats
	// The category is set at the console (r_speeds argument),
	// Use the method pDisplay->DrawStatsText to draw the stats text
	void DisplayStats( IClientStatsTextDisplay* pDisplay, int category );
	const char *GetCountedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < STUDIO_NUM_COUNTED_STATS );
		return s_StudioCountedStatsNames[statID];
	}
	const char *GetTimedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < STUDIO_NUM_TIMED_STATS );
		return s_StudioTimedStatsNames[statID];
	}
private:
	static const char *s_StudioTimedStatsNames[];
	static const char *s_StudioCountedStatsNames[];
};



//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

CStudioStats& StudioStats();


//-----------------------------------------------------------------------------
// MEASURE_TIMED_STAT Macro used to time. Usage:
//
// void Foo()
// {
//		MEASURE_TIMED_STAT( CS_STAT_IDENTIFIER );
//		.. do stuff ...
// }
//
// The macro starts measuring on the line you define it and stops measuring
// when you leave the scope in which the macro was placed. You cannot measure
// the same stat twice within the same scope.
//-----------------------------------------------------------------------------

class C_MeasureTimedStat
{
public:
	// constructor, destructor
	C_MeasureTimedStat( StudioTimedStatId_t stat ) : m_Stat(stat)
	{
		if( g_StudioRender.GetConfig().r_speeds != 0 )
		{
			StudioStats().BeginTimedStat(m_Stat);
		}
	}

	~C_MeasureTimedStat()
	{
		if( g_StudioRender.GetConfig().r_speeds != 0 )
		{
			StudioStats().EndTimedStat(m_Stat);
		}
	}

private:
	StudioTimedStatId_t m_Stat;
};


#define MEASURE_TIMED_STAT( _id )	C_MeasureTimedStat _stat ## _id ( _id )

#endif // STUDIOSTATS_H