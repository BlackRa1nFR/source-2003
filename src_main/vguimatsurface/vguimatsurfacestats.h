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

#if !defined( VGUIMATSURFACESTATS_H )
#define VGUIMATSURFACESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "clientstats.h"
#include "ConVar.h"

extern ConVar const *g_pCVr_speeds;
extern int g_nCVr_speeds;


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

enum VGuiMatSurfaceTimedStatId_t
{
	VGUIMATSURFACE_STATS_PAINTTRAVERSE_TIME = 0,
	VGUIMATSURFACE_STATS_DRAWSETTEXTURE_TIME,
	VGUIMATSURFACE_STATS_DRAWFILLEDRECT_TIME,
	VGUIMATSURFACE_STATS_DRAWTEXTUREDRECT_TIME,
	VGUIMATSURFACE_STATS_PUSHMAKECURRENT_TIME,
	VGUIMATSURFACE_STATS_POPMAKECURRENT_TIME,
	VGUIMATSURFACE_STATS_DRAWPRINTCHAR_TIME,
	VGUIMATSURFACE_STATS_DRAWUNICODECHAR_TIME,
	VGUIMATSURFACE_STATS_DRAWPRINTTEXT_TIME,
	VGUIMATSURFACE_STATS_DRAWQUAD_TIME,
	VGUIMATSURFACE_STATS_DRAWQUADARRAY_TIME,

	VGUIMATSURFACE_STATS_TEST1,
	VGUIMATSURFACE_STATS_TEST2,
	VGUIMATSURFACE_STATS_TEST3,
	VGUIMATSURFACE_STATS_TEST4,
	VGUIMATSURFACE_STATS_TEST5,
	VGUIMATSURFACE_STATS_TEST6,

// NOTE!!!!  If you add to this, make sure and update the names in vguimatsurfacestats.cpp
	VGUIMATSURFACE_NUM_TIMED_STATS
};

enum VGuiMatSurfaceCountedStatId_t
{
	VGUIMATSURFACE_STATS_COUNTED_STAT1 = 0,
// NOTE!!!!  If you add to this, make sure and update the names in vguimatsurfacestats.cpp

	VGUIMATSURFACE_NUM_COUNTED_STATS
};

class CVGuiMatSurfaceStats : public CBaseClientStats< VGUIMATSURFACE_NUM_TIMED_STATS, VGUIMATSURFACE_NUM_COUNTED_STATS>
{
public:
	CVGuiMatSurfaceStats::CVGuiMatSurfaceStats();

	// This method is called when it's time to draw the stats
	// The category is set at the console (r_speeds argument),
	// Use the method pDisplay->DrawStatsText to draw the stats text
	void DisplayStats( IClientStatsTextDisplay* pDisplay, int category );
	const char *GetCountedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < VGUIMATSURFACE_NUM_COUNTED_STATS );
		return s_VGuiMatSurfaceCountedStatsNames[statID];
	}
	const char *GetTimedStatName( int statID ) const
	{
		Assert( statID >= 0 && statID < VGUIMATSURFACE_NUM_TIMED_STATS );
		return s_VGuiMatSurfaceTimedStatsNames[statID];
	}
private:
	static const char *s_VGuiMatSurfaceTimedStatsNames[];
	static const char *s_VGuiMatSurfaceCountedStatsNames[];
};



//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
extern CVGuiMatSurfaceStats g_VGuiMatSurfaceStats;

FORCEINLINE CVGuiMatSurfaceStats& VGuiMatSurfaceStats()
{
	return g_VGuiMatSurfaceStats;
}


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
	C_MeasureTimedStat( VGuiMatSurfaceTimedStatId_t stat ) : m_Stat(stat)
	{
		if( g_pCVr_speeds && g_pCVr_speeds->GetInt() != 0 )
		{
			VGuiMatSurfaceStats().BeginTimedStat(m_Stat);
		}
	}

	~C_MeasureTimedStat()
	{
		if( g_pCVr_speeds && g_pCVr_speeds->GetInt() != 0 )
		{
			VGuiMatSurfaceStats().EndTimedStat(m_Stat);
		}
	}

private:
	VGuiMatSurfaceTimedStatId_t m_Stat;
};


#define MEASURE_TIMED_STAT( _id )	C_MeasureTimedStat _stat ## _id ( _id )

#endif // VGUIMATSURFACESTATS_H