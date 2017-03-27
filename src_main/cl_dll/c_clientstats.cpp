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
#include "cbase.h"
#include "c_clientstats.h"
#include "particlemgr.h"
					  
const char *C_ClientStats::s_ClientCountedStatsNames[] =
{
	"CS_TRACER_NUMTRIANGLES",
	"CS_PARTICLE_NUMTRIANGLES",
	"CS_NUM_COPYFB",
	"CS_WATER_VISIBLE",
	"CS_SHADOWS_RENDERED_TO_TEXTURE",
	"CS_DRAW_NETGRAPH_COUNT",
};

const char *C_ClientStats::s_ClientTimedStatsNames[] =
{
	"CS_TRACER_RENDER",
	"CS_TRACER_TEST",
	"CS_PRECIPITATION_TIME",
	"CS_PRECIPITATION_SETUP",
	"CS_PRECIPITATION_PHYSICS",
	"CS_ADD_RENDERABLE",
	"CS_ADD_DETAILOBJECTS",
	"CS_LEAF_INSERT",
	"CS_LEAF_REMOVE",
	"CS_TEST_1",
	"CS_TEST_2",
	"CS_TEST_3",
	"CS_TEST_4",
	"CS_TEST_5",
	"CS_TRANSLUCENT_RENDER_TIME",
	"CS_BUILDWORLDLISTS_TIME",
	"CS_GETVISIBLEFOGVOLUME_TIME",
	"CS_BUILDRENDERABLELIST_TIME",
	"CS_OPAQUE_WORLD_RENDER_TIME",
	"CS_OPAQUE_RENDER_TIME",
	"CS_COPYFB_TIME",
	"CS_GET_VISIBLE_FOG_VOLUME_TIME",
	"CS_BONE_SETUP_TIME",
	"CS_DRAW_STATIC_PROP_TIME",
	"CS_DRAW_STUDIO_MODEL_TIME",
	"CS_DRAW_BRUSH_MODEL_TIME",
	"CS_DRAW_NETGRAPH_TIME",
};

C_ClientStats::C_ClientStats()
{
	COMPILE_TIME_ASSERT( CS_NUM_COUNTED_STATS == sizeof( s_ClientCountedStatsNames ) / sizeof( s_ClientCountedStatsNames[0] ) );
	COMPILE_TIME_ASSERT( CS_NUM_TIMED_STATS == sizeof(  s_ClientTimedStatsNames ) / sizeof( s_ClientTimedStatsNames[0] ) );
}

C_ClientStats g_ClientStats;

// Expose it to the game...
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( C_ClientStats, IClientStats, 
							INTERFACEVERSION_CLIENTSTATS, g_ClientStats );

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

C_ClientStats& ClientStats()
{
	return g_ClientStats;
}


//-----------------------------------------------------------------------------
// This method is called when it's time to draw the stats
//-----------------------------------------------------------------------------

void C_ClientStats::DisplayStats( IClientStatsTextDisplay* pDisplay,	int category )
{
	unsigned char r, g, b;
	r = b = 50; g = 200;

	pDisplay->SetDrawColor( r, g, b );

	if (category == 10)
	{
//		pDisplay->DrawStatsText( "GetVisibleFogVolume: %.1f ms\n", 					1000.0f * m_StatFrameTime[CS_GETVISIBLEFOGVOLUME_TIME] );
		pDisplay->DrawStatsText( "BuildWorldLists: %.1f ms\n", 						1000.0f * m_StatFrameTime[CS_BUILDWORLDLISTS_TIME] );
		pDisplay->DrawStatsText( "Build Render List: %.1f ms\n", 					1000.0f * m_StatFrameTime[CS_BUILDRENDERABLELIST_TIME] );
		pDisplay->DrawStatsText( "Get visible fog volume: %.1f ms\n",				1000.0f * m_StatFrameTime[CS_GET_VISIBLE_FOG_VOLUME_TIME] );
		pDisplay->DrawStatsText( "Opaque World Render time: %.1f\n", 				1000.0f * m_StatFrameTime[CS_OPAQUE_WORLD_RENDER_TIME] );
		pDisplay->DrawStatsText( "Opaque Entity Render time: %.1f ms\n", 			1000.0f * m_StatFrameTime[CS_OPAQUE_RENDER_TIME] );
		pDisplay->DrawStatsText( "    bone setup time: %.1f ms\n", 					1000.0f * m_StatFrameTime[CS_BONE_SETUP_TIME] );
		pDisplay->DrawStatsText( "Static Prop Render time: %.1f ms\n", 				1000.0f * m_StatFrameTime[CS_DRAW_STATIC_PROP_TIME] );
		pDisplay->DrawStatsText( "Studio Model Render time: %.1f ms\n", 			1000.0f * m_StatFrameTime[CS_DRAW_STUDIO_MODEL_TIME] );
		pDisplay->DrawStatsText( "Brush Model Render time: %.1f ms\n", 				1000.0f * m_StatFrameTime[CS_DRAW_BRUSH_MODEL_TIME] );
		pDisplay->DrawStatsText( "Translucent World + Entity Render time: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_TRANSLUCENT_RENDER_TIME] );
		pDisplay->DrawStatsText( "Copy frame to texture time: %.1f ms count: %d\n",	1000.0f * m_StatFrameTime[CS_COPYFB_TIME], ( int )m_NumStatInFrame[CS_NUM_COPYFB] );
		
		pDisplay->DrawStatsText( "nParticles: %d, (%.3f) ms\n", g_nParticlesDrawn, g_ParticleTimer.GetMillisecondsF() );
		if( m_NumStatInFrame[CS_WATER_VISIBLE] )
		{
			pDisplay->SetDrawColor( 255, 0, 0 );
			pDisplay->DrawStatsText( "water visible: %d\n", ( int )m_NumStatInFrame[CS_WATER_VISIBLE] );
			pDisplay->SetDrawColor( r, g, b );
		}
		else
		{
			pDisplay->DrawStatsText( "water visible: %d\n", ( int )m_NumStatInFrame[CS_WATER_VISIBLE] );
		}
	}

	if ( category == 5 )
	{
		pDisplay->DrawStatsText( "CS Test 1: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_TEST_1] );
		pDisplay->DrawStatsText( "CS Test 2: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_TEST_2] );
		pDisplay->DrawStatsText( "CS Test 3: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_TEST_3] );
		pDisplay->DrawStatsText( "CS Test 4: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_TEST_4] );
		pDisplay->DrawStatsText( "CS Test 5: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_TEST_5] );
		pDisplay->DrawStatsText( "Shadows Rendered: %d\n", m_NumStatInFrame[CS_SHADOWS_RENDERED_TO_TEXTURE] );
	}

	// vgui
	if ( category == 6 )
	{
		pDisplay->DrawStatsText( "Netgraph: %.1f\n", 1000.0f * m_StatFrameTime[CS_DRAW_NETGRAPH_TIME] );
		pDisplay->DrawStatsText( "Netgraph Count: %d\n", m_NumStatInFrame[CS_DRAW_NETGRAPH_COUNT] );
	}

	if ( category == 7 )
	{
		pDisplay->DrawStatsText( "add renderable time: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_ADD_RENDERABLE] );
		pDisplay->DrawStatsText( "Add detail time: %.1f\n", 1000.0f * m_StatFrameTime[CS_ADD_DETAILOBJECTS] );
		pDisplay->DrawStatsText( "leaf insert time: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_LEAF_INSERT] );
		pDisplay->DrawStatsText( "leaf remove time: %.1f ms\n", 1000.0f * m_StatFrameTime[CS_LEAF_REMOVE] );
	}

	// tracer data
	if ( category == 9 )
	{
		pDisplay->DrawStatsText( "num tracer tris: %d (%.1f ms)\n", m_NumStatInFrame[CS_TRACER_NUMTRIANGLES],	1000.0f * m_StatFrameTime[CS_TRACER_RENDER] );
		pDisplay->DrawStatsText( "Precipitation total time: %.1f\n", 1000.0f * m_StatFrameTime[CS_PRECIPITATION_TIME] );
		pDisplay->DrawStatsText( "Precipitation physics time: %.1f\n", 1000.0f * m_StatFrameTime[CS_PRECIPITATION_PHYSICS] );
		pDisplay->DrawStatsText( "Precipitation setup time: %.1f\n", 1000.0f * m_StatFrameTime[CS_PRECIPITATION_SETUP] );
		pDisplay->DrawStatsText( "Tracer test time: %.1f\n", 1000.0f * m_StatFrameTime[CS_TRACER_TEST] );
	}
}

