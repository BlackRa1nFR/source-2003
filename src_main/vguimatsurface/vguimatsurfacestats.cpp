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

#include "vguimatsurfacestats.h"


const char *CVGuiMatSurfaceStats::s_VGuiMatSurfaceCountedStatsNames[] =
{
	"VGUIMATSURFACE_STATS_COUNTED_STAT1",
};

const char *CVGuiMatSurfaceStats::s_VGuiMatSurfaceTimedStatsNames[] =
{
	"VGUIMATSURFACE_STATS_PAINTTRAVERSE_TIME",
	"VGUIMATSURFACE_STATS_DRAWSETTEXTURE_TIME",
	"VGUIMATSURFACE_STATS_DRAWFILLEDRECT_TIME",
	"VGUIMATSURFACE_STATS_DRAWTEXTUREDRECT_TIME",
	"VGUIMATSURFACE_STATS_PUSHMAKECURRENT_TIME",
	"VGUIMATSURFACE_STATS_POPMAKECURRENT_TIME",
	"VGUIMATSURFACE_STATS_DRAWPRINTCHAR_TIME",
	"VGUIMATSURFACE_STATS_DRAWUNICODECHAR_TIME",
	"VGUIMATSURFACE_STATS_DRAWPRINTTEXT_TIME",
	"VGUIMATSURFACE_STATS_DRAWQUAD_TIME",
	"VGUIMATSURFACE_STATS_DRAWQUADARRAY_TIME",
	"VGUIMATSURFACE_STATS_TEST1",
	"VGUIMATSURFACE_STATS_TEST2",
	"VGUIMATSURFACE_STATS_TEST3",
	"VGUIMATSURFACE_STATS_TEST4",
	"VGUIMATSURFACE_STATS_TEST5",
	"VGUIMATSURFACE_STATS_TEST6",
};

CVGuiMatSurfaceStats g_VGuiMatSurfaceStats;

// Expose it to the engine
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CVGuiMatSurfaceStats, IClientStats, 
							INTERFACEVERSION_CLIENTSTATS, g_VGuiMatSurfaceStats );

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

CVGuiMatSurfaceStats::CVGuiMatSurfaceStats()
{
	Assert( VGUIMATSURFACE_NUM_COUNTED_STATS == sizeof( s_VGuiMatSurfaceCountedStatsNames ) / sizeof( s_VGuiMatSurfaceCountedStatsNames[0] ) );
	Assert( VGUIMATSURFACE_NUM_TIMED_STATS == sizeof( s_VGuiMatSurfaceTimedStatsNames ) / sizeof( s_VGuiMatSurfaceTimedStatsNames[0] ) );
}



//-----------------------------------------------------------------------------
// This method is called when it's time to draw the stats
//-----------------------------------------------------------------------------

void CVGuiMatSurfaceStats::DisplayStats( IClientStatsTextDisplay* pDisplay, int category )
{
	// tracer data
	if ( category == 6 )
	{
		unsigned char r, g, b;
		r = b = 50; g = 200;

		pDisplay->SetDrawColor( r, g, b );

//		pDisplay->DrawStatsText( "num bone changes : %d\n", 
//			m_NumStatInFrame[VGUIMATSURFACE_STATS_MODEL_NUM_BONE_CHANGES] );
//		pDisplay->DrawStatsText( "DrawQuad: %.1f\n", 
//			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWQUAD_TIME] );
		pDisplay->DrawStatsText( "DrawQuadArray: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWQUADARRAY_TIME] );
		pDisplay->DrawStatsText( "DrawFilledRect: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWFILLEDRECT_TIME] );
		pDisplay->DrawStatsText( "DrawTexturedRect: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWTEXTUREDRECT_TIME] );

		/*
		pDisplay->DrawStatsText( "PushMakeCurrent: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_PUSHMAKECURRENT_TIME] );
		pDisplay->DrawStatsText( "PopMakeCurrent: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_POPMAKECURRENT_TIME] );
		pDisplay->DrawStatsText( "DrawPrintChar: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWPRINTCHAR_TIME] );
		pDisplay->DrawStatsText( "DrawUnicodeChar: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWUNICODECHAR_TIME] );
		pDisplay->DrawStatsText( "DrawPrintText: %.1f\n", 
			1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_DRAWPRINTTEXT_TIME] );
		*/

		if ( category == 7 )
		{
			pDisplay->DrawStatsText( "Test1: %.1f\n", 
				1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_TEST1] );
			pDisplay->DrawStatsText( "Test2: %.1f\n", 
				1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_TEST2] );
			pDisplay->DrawStatsText( "Test3: %.1f\n", 
				1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_TEST3] );
			pDisplay->DrawStatsText( "Test4: %.1f\n", 
				1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_TEST4] );
			pDisplay->DrawStatsText( "Test5: %.1f\n", 
				1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_TEST5] );
			pDisplay->DrawStatsText( "Test6: %.1f\n", 
				1000.0f * m_StatFrameTime[VGUIMATSURFACE_STATS_TEST6] );
		}
	}
}
