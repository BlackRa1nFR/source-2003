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

#ifndef STUDIOSTATS_H
#include "studiostats.h"
#endif
					  

const char *CStudioStats::s_StudioCountedStatsNames[] =
{
	"STUDIO_STATS_MODEL_NUM_BONE_CHANGES",
};

const char *CStudioStats::s_StudioTimedStatsNames[] =
{
	"STUDIO_STATS_HW_SKIN_TIME",
	"STUDIO_STATS_STATIC_MESH_TIME",
	"STUDIO_STATS_SET_BONE_WEIGHTS",
	"STUDIO_STATS_SET_BONE_MATRICES",
	"STUDIO_STATS_DRAW_MESH",
	"STUDIO_STATS_SAVE_MATRIX",
	"STUDIO_STATS_EYE_TIME",
	"STUDIO_STATS_RENDER_FINAL",

	"STUDIO_STATS_TEST1",
	"STUDIO_STATS_TEST2",
	"STUDIO_STATS_TEST3",
	"STUDIO_STATS_TEST4",
	"STUDIO_STATS_TEST5",
};

CStudioStats g_StudioStats;

// Expose it to the engine
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CStudioStats, IClientStats, 
							INTERFACEVERSION_CLIENTSTATS, g_StudioStats );

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

CStudioStats::CStudioStats()
{
	COMPILE_TIME_ASSERT( STUDIO_NUM_COUNTED_STATS == sizeof( s_StudioCountedStatsNames ) / sizeof( s_StudioCountedStatsNames[0] ) );
	COMPILE_TIME_ASSERT( STUDIO_NUM_TIMED_STATS == sizeof( s_StudioTimedStatsNames ) / sizeof( s_StudioTimedStatsNames[0] ) );
}

CStudioStats& StudioStats()
{
	return g_StudioStats;
}


//-----------------------------------------------------------------------------
// This method is called when it's time to draw the stats
//-----------------------------------------------------------------------------

void CStudioStats::DisplayStats( IClientStatsTextDisplay* pDisplay, int category )
{
	// tracer data
	if ( category == 6 )
	{
		unsigned char r, g, b;
		r = b = 50; g = 200;

		pDisplay->SetDrawColor( r, g, b );

		pDisplay->DrawStatsText( "num bone changes : %d\n", 
			m_NumStatInFrame[STUDIO_STATS_MODEL_NUM_BONE_CHANGES] );
		pDisplay->DrawStatsText( "Render final time: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_RENDER_FINAL] );
		pDisplay->DrawStatsText( "Draw Mesh time: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_DRAW_MESH] );
		pDisplay->DrawStatsText( "  HW Skin time: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_HW_SKIN_TIME] );
		pDisplay->DrawStatsText( "  Eye time: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_EYE_TIME] );

		pDisplay->DrawStatsText( "Studio Test1: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_TEST1] );
		pDisplay->DrawStatsText( "Studio Test2: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_TEST2] );
		pDisplay->DrawStatsText( "Studio Test3: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_TEST3] );
		pDisplay->DrawStatsText( "Studio Test4: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_TEST4] );
		pDisplay->DrawStatsText( "Studio Test5: %.1f\n", 
			1000.0f * m_StatFrameTime[STUDIO_STATS_TEST5] );
	}
}
