//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The base interface 
//
// The implementation of the client stats; exists as a singleton accessed
// by the clientstats global variable
// $NoKeywords: $
//=============================================================================

#if !defined( C_CLIENTSTATS_H )
#define C_CLIENTSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "clientstats.h"
#include "tier0/dbg.h"

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

enum ClientTimedStatId_t
{
	CS_TRACER_RENDER = 0,
	CS_TRACER_TEST,
	CS_PRECIPITATION_TIME,
	CS_PRECIPITATION_SETUP,
	CS_PRECIPITATION_PHYSICS,
	CS_ADD_RENDERABLE,
	CS_ADD_DETAILOBJECTS,
	CS_LEAF_INSERT,
	CS_LEAF_REMOVE,
	CS_TEST_1,
	CS_TEST_2,
	CS_TEST_3,
	CS_TEST_4,
	CS_TEST_5,
	CS_TRANSLUCENT_RENDER_TIME,
	CS_BUILDWORLDLISTS_TIME,
	CS_GETVISIBLEFOGVOLUME_TIME,
	CS_BUILDRENDERABLELIST_TIME,
	CS_OPAQUE_WORLD_RENDER_TIME,
	CS_OPAQUE_RENDER_TIME,
	CS_COPYFB_TIME,
	CS_GET_VISIBLE_FOG_VOLUME_TIME,
	CS_BONE_SETUP_TIME,
	CS_DRAW_STATIC_PROP_TIME,
	CS_DRAW_STUDIO_MODEL_TIME,
	CS_DRAW_BRUSH_MODEL_TIME,
	CS_DRAW_NETGRAPH_TIME,

// NOTE!!!!  If you add to this, make sure and update s_ClientTimedStatsNames in c_clientstats.cpp
	CS_NUM_TIMED_STATS
};

enum ClientCountedStatId_t
{
	CS_TRACER_NUMTRIANGLES = 0,
	CS_PARTICLE_NUMTRIANGLES,
	CS_NUM_COPYFB,
	CS_WATER_VISIBLE,
	CS_SHADOWS_RENDERED_TO_TEXTURE,
	CS_DRAW_NETGRAPH_COUNT,

// NOTE!!!!  If you add to this, make sure and update s_ClientTimedStatsNames in c_clientstats.cpp
	CS_NUM_COUNTED_STATS
};

class C_ClientStats : public CBaseClientStats< CS_NUM_TIMED_STATS, CS_NUM_COUNTED_STATS>
{
public:
	C_ClientStats();
	// This method is called when it's time to draw the stats
	// The category is set at the console (r_speeds argument),
	// Use the method pDisplay->DrawStatsText to draw the stats text
	void DisplayStats( IClientStatsTextDisplay* pDisplay, int category );

	const char *GetCountedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < CS_NUM_COUNTED_STATS );
		return s_ClientCountedStatsNames[statID];
	}
	const char *GetTimedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < CS_NUM_TIMED_STATS );
		return s_ClientTimedStatsNames[statID];
	}
private:
	static const char *s_ClientTimedStatsNames[];
	static const char *s_ClientCountedStatsNames[];
};



//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

C_ClientStats& ClientStats();


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
	C_MeasureTimedStat( ClientTimedStatId_t stat ) : m_Stat(stat)
	{
		ClientStats().BeginTimedStat(m_Stat);
	}

	~C_MeasureTimedStat()
	{
		ClientStats().EndTimedStat(m_Stat);
	}

private:
	ClientTimedStatId_t m_Stat;
};


#define MEASURE_TIMED_STAT( _id )	C_MeasureTimedStat _stat ## _id ( _id )

#endif // C_CLIENTSTATS_H
