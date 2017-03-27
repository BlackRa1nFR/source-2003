//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================

#include "locald3dtypes.h"
#include "CMaterialSystemStats.h"								    
#include "limits.h"														    
#include "materialsystem/IMaterialSystem.h"
#include "ShaderAPIDX8_Global.h"
#include "ShaderAPIDX8.h"
#include "IShaderUtil.h"
#include "materialsystem/MaterialSystem_Config.h"

const char *CMaterialSystemStatsDX8::s_TimedStatsNames[] =
{
	"MATERIAL_SYSTEM_STATS_VERTEX_COPY_TIME0",
	"MATERIAL_SYSTEM_STATS_RENDER_PASS_TIME",
	"MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME",
	"MATERIAL_SYSTEM_STATS_AMBIENT_CUBE_TIME",
	"MATERIAL_SYSTEM_STATS_BUFFER_LOCK_TIME",
	"MATERIAL_SYSTEM_STATS_BUFFER_UNLOCK_TIME",
	"MATERIAL_SYSTEM_STATS_SWAP_TIME",
	"MATERIAL_SYSTEM_STATS_SET_RENDER_STATE",
	"MATERIAL_SYSTEM_STATS_DRAW_INDEXED_PRIMITIVE",
	"MATERIAL_SYSTEM_STATS_SET_TRANSFORM",
	"MATERIAL_SYSTEM_STATS_TRANSFORM_COPY",
	"MATERIAL_SYSTEM_STATS_UPDATE_FLUSH",
	"MATERIAL_SYSTEM_STATS_TFF",
	"MATERIAL_SYSTEM_STATS_AMBIENT_LOCK",
	"MATERIAL_SYSTEM_STATS_COMMIT_STATE_TIME",
	"MATERIAL_SYSTEM_STATS_COMMIT_TRANSFORMS_TIME",
	"MATERIAL_SYSTEM_STATS_COMMIT_LIGHTING_TIME",
	"MATERIAL_SYSTEM_STATS_VERTEX_SHADER_CONSTANT_TIME",
	"MATERIAL_SYSTEM_STATS_CLEAR_TIME",
	"MATERIAL_SYSTEM_STATS_SET_VERTEX_SHADER",
	"MATERIAL_SYSTEM_STATS_SET_PIXEL_SHADER",
	"MATERIAL_SYSTEM_STATS_DRAW_MESH",
	"MATERIAL_SYSTEM_STATS_SET_TEXTURE",
	"MATERIAL_SYSTEM_STATS_BEGIN_PASS",
	"MATERIAL_SYSTEM_STATS_UPDATE_MATRIX_TRANSFORM",
	"MATERIAL_SYSTEM_STATS_FLUSH",
	"MATERIAL_SYSTEM_STATS_FOG_MODE",
	"MATERIAL_SYSTEM_STATS_SHADEMODE",
	"MATERIAL_SYSTEM_STATS_COLOR4F",
	"MATERIAL_SYSTEM_STATS_AMBIENT_VERTEX",
	"MATERIAL_SYSTEM_STATS_COMMIT_VIEW_TIME",
	"MATERIAL_SYSTEM_STATS_COMMIT_VIEWMODEL_TIME",
	"MATERIAL_SYSTEM_STATS_TEST1",
	"MATERIAL_SYSTEM_STATS_TEST2",
	"MATERIAL_SYSTEM_STATS_TEST3",
	"MATERIAL_SYSTEM_STATS_TEST4",
	"MATERIAL_SYSTEM_STATS_TEST5",
	"MATERIAL_SYSTEM_STATS_FLUSH_HARDWARE",
	"MATERIAL_SYSTEM_STATS_READ_PIXEL_SHADER_TIME",
	"MATERIAL_SYSTEM_STATS_ASSEMBLE_PIXEL_SHADER_TIME",
	"MATERIAL_SYSTEM_STATS_CREATE_PIXEL_SHADER_TIME",
	"MATERIAL_SYSTEM_STATS_CREATE_VERTEX_SHADER_TIME",
};

const char *CMaterialSystemStatsDX8::s_CountedStatsNames[] =
{
	"MATERIAL_SYSTEM_STATS_TEXTURE_STATE",
	"MATERIAL_SYSTEM_STATS_DYNAMIC_STATE",
	"MATERIAL_SYSTEM_STATS_SHADOW_STATE",
	"MATERIAL_SYSTEM_STATS_NUM_TEXELS",
	"MATERIAL_SYSTEM_STATS_NUM_BYTES",
	"MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXELS",
	"MATERIAL_SYSTEM_STATS_NUM_UNIQUE_BYTES",
	"MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_DOWNLOADED",
	"MATERIAL_SYSTEM_STATS_TEXTURE_TEXELS_DOWNLOADED",
	"MATERIAL_SYSTEM_STATS_NUM_INDEX_PRIMITIVE_CALLS",
	"MATERIAL_SYSTEM_STATS_NUM_PRIMITIVES",
	"MATERIAL_SYSTEM_STATS_TEXTURE_UPLOADS",
	"MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXTURES",
	"MATERIAL_SYSTEM_STATS_FILL_RATE",
	"MATERIAL_SYSTEM_STATS_NUM_BUFFER_LOCK",
	"MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_ALLOCATED",
	"MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED",
	"MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_NEW_THIS_FRAME",
	"MATERIAL_SYSTEM_STATS_LIGHTMAP_BYTES_ALLOCATED",
	"MATERIAL_SYSTEM_STATS_VERTEX_TRANSFORM",
	"MATERIAL_SYSTEM_STATS_NUM_PIXEL_SHADER_CREATES",
	"MATERIAL_SYSTEM_STATS_NUM_VERTEX_SHADER_CREATES",
};

const char *CMaterialSystemStatsDX8::s_StatGroupNames[] =
{
	"MATERIAL_SYSTEM_STATS_OTHER",
	"MATERIAL_SYSTEM_STATS_WORLD",
	"MATERIAL_SYSTEM_STATS_STATIC_PROPS",
	"MATERIAL_SYSTEM_STATS_TOTAL",
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CMaterialSystemStatsDX8::CMaterialSystemStatsDX8() : 
	m_CurrentStatGroup(MATERIAL_SYSTEM_STATS_OTHER), m_InFrame( false )
{
	Assert( MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS == sizeof( s_TimedStatsNames ) / sizeof( s_TimedStatsNames[0] ) );
	Assert( MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS == sizeof( s_CountedStatsNames ) / sizeof( s_CountedStatsNames[0] ) );
	Assert( MATERIAL_SYSTEM_STATS_NUM_GROUPS == sizeof( s_StatGroupNames ) / sizeof( s_StatGroupNames[0] ) );
}
	

CMaterialSystemStatsDX8::~CMaterialSystemStatsDX8()
{
}

//-----------------------------------------------------------------------------
// Reset
//-----------------------------------------------------------------------------

void CMaterialSystemStatsDX8::BeginRun()
{
	int i;
	for( i = 0; i < MATERIAL_SYSTEM_STATS_NUM_GROUPS; i++ )
	{
		int j;
		for (j = 0; j < MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS; ++j)
			m_StatGroup[i].m_TotalStatTime[j] = 0.0;

		for (j = 0; j < MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS; ++j)
		{
			m_StatGroup[i].m_TotalStat[j] = 0;
			m_StatGroup[i].m_MinStatPerFrame[j] = INT_MAX;
			m_StatGroup[i].m_MaxStatPerFrame[j] = INT_MIN;
		}
	}
}

void CMaterialSystemStatsDX8::EndRun()
{
}

//-----------------------------------------------------------------------------
// Begin, end frame
//-----------------------------------------------------------------------------

void CMaterialSystemStatsDX8::BeginFrame( )
{
	// bah... performance overhead for doing stats. ick
	ShaderAPI()->FlushBufferedPrimitives();

	m_InFrame = true;

	int i;
	for( i = 0; i < MATERIAL_SYSTEM_STATS_NUM_GROUPS; i++ )
	{
		int j;
		for (j = 0; j < MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS; ++j)
		{
			m_StatGroup[i].m_StatFrameTime[j] = 0.0;
		}
		for (j = 0; j < MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS; ++j)
		{
			m_StatGroup[i].m_NumStatInFrame[j] = 0;
		}
	}
}

void CMaterialSystemStatsDX8::EndFrame( )
{
	// bah... performance overhead for doing stats. ick
	ShaderAPI()->FlushBufferedPrimitives();

	// Measure the fill rate....
	if (ShaderUtil()->GetConfig().bMeasureFillRate)
	{
		ShaderAPI()->ComputeFillRate();
	}

	m_InFrame = false;

	int i;
	for( i = 0; i < MATERIAL_SYSTEM_STATS_NUM_GROUPS; i++ )
	{
		int j;
		for (j = 0; j < MATERIAL_SYSTEM_STATS_NUM_TIMED_STATS; ++j)
		{
			m_StatGroup[i].m_TotalStatTime[j] += m_StatGroup[i].m_StatFrameTime[j];
		}

		for (j = 0; j < MATERIAL_SYSTEM_STATS_NUM_COUNTED_STATS; ++j)
		{
			if( m_StatGroup[i].m_NumStatInFrame[j] < m_StatGroup[i].m_MinStatPerFrame[j] )
			{
				m_StatGroup[i].m_MinStatPerFrame[j] = m_StatGroup[i].m_NumStatInFrame[j];
			}
			if( m_StatGroup[i].m_NumStatInFrame[j] > m_StatGroup[i].m_MaxStatPerFrame[j] )
			{
				m_StatGroup[i].m_MaxStatPerFrame[j] = m_StatGroup[i].m_NumStatInFrame[j];
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------

void CMaterialSystemStatsDX8::SetStatGroup( int group )
{
	// bah... performance overhead for doing stats. ick
	// but maybe it doesn't matter, since we're switching from world to non-world
	// this likely implies a material switch anyways and a flush.
	ShaderAPI()->FlushBufferedPrimitives();
	m_CurrentStatGroup = ( MaterialSystemStatsGroup_t )group;
}

int CMaterialSystemStatsDX8::GetCurrentStatGroup( void ) const
{
	return m_CurrentStatGroup;
}

const char *CMaterialSystemStatsDX8::GetStatGroupName( int groupID ) const
{
	Assert( groupID >= 0 && groupID < MATERIAL_SYSTEM_STATS_NUM_GROUPS );
	return s_StatGroupNames[groupID];
}

int CMaterialSystemStatsDX8::GetNumStatGroups( void ) const
{
	return MATERIAL_SYSTEM_STATS_NUM_GROUPS;
}

//-----------------------------------------------------------------------------
// retuns counted stats...
//-----------------------------------------------------------------------------

int CMaterialSystemStatsDX8::CountedStatInFrame( int /*MaterialSystemCountedStatId_t*/ stat ) const
{
	return m_StatGroup[m_CurrentStatGroup].m_NumStatInFrame[stat];
}

__int64 CMaterialSystemStatsDX8::TotalCountedStat( int /*MaterialSystemCountedStatId_t*/ stat ) const
{
	return m_StatGroup[m_CurrentStatGroup].m_TotalStat[stat];
}

int CMaterialSystemStatsDX8::MinCountedStat( int /*MaterialSystemCountedStatId_t*/ stat ) const
{
	return m_StatGroup[m_CurrentStatGroup].m_MinStatPerFrame[stat];
}

int CMaterialSystemStatsDX8::MaxCountedStat( int /*MaterialSystemCountedStatId_t*/ stat ) const
{
	return m_StatGroup[m_CurrentStatGroup].m_MaxStatPerFrame[stat];
}


//-----------------------------------------------------------------------------
// returns timed stats
//-----------------------------------------------------------------------------

double CMaterialSystemStatsDX8::TimedStatInFrame( int /*MaterialSystemTimedStatId_t*/ stat ) const
{
	return m_StatGroup[m_CurrentStatGroup].m_StatFrameTime[stat];
}

double CMaterialSystemStatsDX8::TotalTimedStat( int /*MaterialSystemTimedStatId_t*/ stat ) const
{
	return m_StatGroup[m_CurrentStatGroup].m_TotalStatTime[stat];
}


//-----------------------------------------------------------------------------
// This method is called when it's time to draw the stats
//-----------------------------------------------------------------------------

void CMaterialSystemStatsDX8::DisplayStats( IClientStatsTextDisplay* pDisplay, int category)
{
	float oneOverOneMeg =  1.0 / ( 1024.0 * 1024.0 );

	// fixme: doesn't take procedural textures into account?
	unsigned char r, g, b;
	r = 255;
	g = 255;
	b = 255;
		
	pDisplay->SetDrawColor( r, g, b );

	if( category == 1 )
	{
		SetStatGroup( MATERIAL_SYSTEM_STATS_TOTAL );
		float totalMegatexels = oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXELS);
		float totalMegabytes =  oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_BYTES);
//		int totalNumTextures = CountedStatInFrame( MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXTURES );
		float megabytesNewThisFrame = oneOverOneMeg * CountedStatInFrame( MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_NEW_THIS_FRAME );
		pDisplay->DrawStatsText( "\n" );
		float maxMegabytes = ( float )ShaderUtil()->GetConfig().m_MaxTextureMemory;

		pDisplay->SetDrawColorFromStatValues( (float)maxMegabytes, totalMegabytes );
		pDisplay->DrawStatsText( "    total textures/frame:                 actual/max: %.1f/%.1fMB (%.1f MTexels)\n", 
			totalMegabytes, maxMegabytes, totalMegatexels );

		pDisplay->SetDrawColorFromStatValues( 
			(float)( float )ShaderUtil()->GetConfig().m_NewTextureMemoryPerFrame, 
			megabytesNewThisFrame );
		pDisplay->DrawStatsText( "    total new textures/frame:             actual/max: %.1f/%.1fMB\n", 
			megabytesNewThisFrame, ( float )ShaderUtil()->GetConfig().m_NewTextureMemoryPerFrame );

		pDisplay->SetDrawColor( r, g, b );
		pDisplay->DrawStatsText( "\n" );
		// HACK HACK: This really shouldn't be in here.
		pDisplay->DrawStatsText( "    depth complexity:\n" );
		pDisplay->DrawStatsText( "\n" );
		pDisplay->DrawStatsText( "    physics interactions:\n" );
		pDisplay->DrawStatsText( "\n" );
		pDisplay->DrawStatsText( "GLOBAL BUDGETS:\n" );

		float allocatedMegabytes =  oneOverOneMeg * TotalCountedStat(MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_ALLOCATED);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxTextureMemory, allocatedMegabytes );
		pDisplay->DrawStatsText( "    total texture memory:                 actual/max: %.1f/%.1fMB\n",
			allocatedMegabytes,
			( float )ShaderUtil()->GetConfig().m_MaxTextureMemory );

		float lightmapMegabytes =  oneOverOneMeg * TotalCountedStat(MATERIAL_SYSTEM_STATS_LIGHTMAP_BYTES_ALLOCATED);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxLightmapMemory, lightmapMegabytes );
		pDisplay->DrawStatsText( "    total lightmap memory:                actual/max: %.1f/%.1fMB\n", 
			lightmapMegabytes,
			( float )ShaderUtil()->GetConfig().m_MaxLightmapMemory );

		float allocatedModels =  oneOverOneMeg * TotalCountedStat(MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxModelMemory, allocatedModels );
		pDisplay->DrawStatsText( "    total model memory:                   actual/max: %.1f/%.1fMB\n",
			( float )allocatedModels,
			( float )ShaderUtil()->GetConfig().m_MaxModelMemory );
	}
	
	if( category != 8 && category > 1 )
	{
		SetStatGroup( MATERIAL_SYSTEM_STATS_WORLD );
		float worldMegatexels = oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXELS);
		float worldMegabytes =  oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_BYTES);
		SetStatGroup( MATERIAL_SYSTEM_STATS_OTHER );
		float otherMegatexels = oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXELS);
		float otherMegabytes =  oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_BYTES);
		//	float lightmapMegatexels = oneOverOneMeg * GetUniqueLightmapTexelsInFrame();
		
		SetStatGroup( MATERIAL_SYSTEM_STATS_WORLD );
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxWorldMegabytes, worldMegabytes );
		pDisplay->DrawStatsText( "%0.1f/%d megabytes/frame world textures (%0.1f megatexels)\n", 
			worldMegabytes /* + lightmapMegatexels*/,
			ShaderUtil()->GetConfig().m_MaxWorldMegabytes, worldMegatexels );
		
		//	pDisplay->DrawStatsText( m_pFont, x, y, r, g, b, a, "    %0.1f megatexels/frame lightmaps\n", 
		//		lightmapMegatexels );
		
		SetStatGroup( MATERIAL_SYSTEM_STATS_OTHER );
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxOtherMegabytes, otherMegabytes );
		pDisplay->DrawStatsText( "%0.1f/%d megabytes/frame other textures (%0.1f megatexels)\n", 
			otherMegabytes,
			ShaderUtil()->GetConfig().m_MaxOtherMegabytes, otherMegatexels );
		
		SetStatGroup( MATERIAL_SYSTEM_STATS_TOTAL );
		
		float allocatedModels =  oneOverOneMeg * TotalCountedStat(MATERIAL_SYSTEM_STATS_MODEL_BYTES_ALLOCATED);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxModelMemory, allocatedModels );
		pDisplay->DrawStatsText( "%0.1f/%d total model memory allocated (megabytes)\n", 
			allocatedModels,
			ShaderUtil()->GetConfig().m_MaxModelMemory );
		
		float allocatedMegabytes =  oneOverOneMeg * TotalCountedStat(MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_ALLOCATED);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxTextureMemory, allocatedMegabytes );
		pDisplay->DrawStatsText( "%0.1f/%d total texture memory allocated (megabytes)\n", 
			allocatedMegabytes,
			ShaderUtil()->GetConfig().m_MaxTextureMemory );
		
		float lightmapMegabytes =  oneOverOneMeg * TotalCountedStat(MATERIAL_SYSTEM_STATS_LIGHTMAP_BYTES_ALLOCATED);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_MaxLightmapMemory, lightmapMegabytes );
		pDisplay->DrawStatsText( "%0.1f/%d total lightmap memory allocated (megabytes)\n", 
			lightmapMegabytes,
			ShaderUtil()->GetConfig().m_MaxLightmapMemory );
		
		float newMegabytes =  oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_NEW_THIS_FRAME);
		pDisplay->SetDrawColorFromStatValues( (float)ShaderUtil()->GetConfig().m_NewTextureMemoryPerFrame, newMegabytes );
		pDisplay->DrawStatsText( "%0.1f/%d new textures this frame (megabytes)\n", 
			newMegabytes,
			ShaderUtil()->GetConfig().m_NewTextureMemoryPerFrame );
		
		int screenWidth, screenHeight;
		ShaderAPI()->GetWindowSize( screenWidth, screenHeight );
		int numScreenPixels = screenWidth * screenHeight;
		float depthComplexity = (float)CountedStatInFrame(MATERIAL_SYSTEM_STATS_FILL_RATE) / 
			(float)numScreenPixels;
		
		pDisplay->SetDrawColorFromStatValues( ShaderUtil()->GetConfig().m_MaxDepthComplexity, depthComplexity );
		pDisplay->DrawStatsText( "%.2f depth complexity (%d pixels rendered)\n", 
			depthComplexity,
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_FILL_RATE) );
		
		pDisplay->DrawStatsText( "%d pixel shader creates/frame\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_PIXEL_SHADER_CREATES) );

		pDisplay->DrawStatsText( "%d vertex shader creates/frame\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_VERTEX_SHADER_CREATES) );
	}

	if( category == 3 )
	{
		pDisplay->DrawStatsText( "%0.3f megabytes/frame cache misses\n", 
			oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_DOWNLOADED) );
		pDisplay->DrawStatsText( "%0.3f megatexels/frame cache misses\n", 
			oneOverOneMeg * CountedStatInFrame(MATERIAL_SYSTEM_STATS_TEXTURE_TEXELS_DOWNLOADED) );
		pDisplay->DrawStatsText( "%d cache misses/frame\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_TEXTURE_UPLOADS) );
		pDisplay->DrawStatsText( "%d texture binds/frame (%d unique)\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_TEXTURE_STATE),
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_UNIQUE_TEXTURES) );
	}
	else if( category > 1 )
	{
		pDisplay->DrawStatsText( "%d cache misses/frame\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_TEXTURE_UPLOADS) );
	}
	pDisplay->DrawStatsText( "\n" );
	
	if( category == 3 )
	{
		pDisplay->SetDrawColor( 255, 0, 0 );
		/*
		pDisplay->DrawStatsText( "%.2f ambient lock\n", 
			1000.0f * TimedStatInFrame(MATERIAL_SYSTEM_STATS_AMBIENT_LOCK) );
		pDisplay->DrawStatsText( "%.2f set render state\n", 
			1000.0f * TimedStatInFrame(MATERIAL_SYSTEM_STATS_SET_RENDER_STATE) );
		pDisplay->DrawStatsText( "%.2f flush\n", 
			1000.0f * TimedStatInFrame(MATERIAL_SYSTEM_STATS_UPDATE_FLUSH) );
		pDisplay->DrawStatsText( "%.2f copy\n", 
			1000.0f * TimedStatInFrame(MATERIAL_SYSTEM_STATS_TRANSFORM_COPY) );
		pDisplay->DrawStatsText( "%.2f set transform\n", 
			1000.0f * TimedStatInFrame(MATERIAL_SYSTEM_STATS_SET_TRANSFORM) );
		*/
		pDisplay->DrawStatsText( "%.2f draw indexed primitive\n", 
			1000.0f * TimedStatInFrame(MATERIAL_SYSTEM_STATS_DRAW_INDEXED_PRIMITIVE) );
			
		pDisplay->SetDrawColor( r, g, b );
			
		pDisplay->DrawStatsText( "%d shadow state changes\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_SHADOW_STATE) );
		pDisplay->DrawStatsText( "%d dynamic state changes\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_DYNAMIC_STATE) );
		pDisplay->DrawStatsText( "%d primitives drawn %0.3f ave primitive buffer size\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_PRIMITIVES),
			(float)CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_PRIMITIVES) /
			(float)CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_INDEX_PRIMITIVE_CALLS)
			);
		pDisplay->DrawStatsText( "%.2f render pass time (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_RENDER_PASS_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "%.2f vertex copy time (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_VERTEX_COPY_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "%.2f mesh build (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "%.2f ambient cube build (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_AMBIENT_CUBE_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "%d # of vertex + index buffer locks\n", 
			CountedStatInFrame(MATERIAL_SYSTEM_STATS_NUM_BUFFER_LOCK) );
		pDisplay->DrawStatsText( "%.2f vertex + index buffer lock (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_BUFFER_LOCK_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "%.2f vertex + index buffer unlock (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_BUFFER_UNLOCK_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "\n" );
	}

	if( category == 8 )
	{
		r = g = b = 128;
		pDisplay->DrawStatsText( "%.2f ForceHardwareSync (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_FLUSH_HARDWARE) * 1000.0f );
		pDisplay->DrawStatsText( "%.2f render state (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_SET_RENDER_STATE) * 1000.0f );
		pDisplay->DrawStatsText( "%.2f draw mesh time (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_DRAW_MESH) * 1000.0f );
		pDisplay->DrawStatsText( "  %.2f commit time (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_COMMIT_STATE_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f commit view time (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_COMMIT_VIEW_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f commit viewmodel time (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_COMMIT_VIEWMODEL_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f commit transform (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_COMMIT_TRANSFORMS_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f vertex shader constants (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_VERTEX_SHADER_CONSTANT_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "  %.2f begin pass (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_BEGIN_PASS) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f set vertex shader (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_SET_VERTEX_SHADER) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f set pixel shader (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_SET_PIXEL_SHADER) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f update matrix (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_UPDATE_MATRIX_TRANSFORM) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f test 1 (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_TEST1) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f update matrix2 (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_TEST2) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f update matrix3 (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_TEST3) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f update matrix4 (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_TEST4) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f update matrix5 (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_TEST5) * 1000.0f );
		pDisplay->DrawStatsText( "    %.2f fog (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_FOG_MODE) * 1000.0f );
		pDisplay->DrawStatsText( "  %.2f render pass (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_RENDER_PASS_TIME) * 1000.0f );
		pDisplay->DrawStatsText( "     %.2f draw indexed (ms)\n", 
			TimedStatInFrame(MATERIAL_SYSTEM_STATS_DRAW_INDEXED_PRIMITIVE) * 1000.0f );
	}
}


