//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================
#include "glquake.h"
#include "vid.h"
#include "vgui_int.h"
#include <vgui_controls/Panel.h>
#include "EngineStats.h"
#include "gl_matsysiface.h"
#include "vgui_BasePanel.h"
#include "clientstats.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include "FileSystem.h"
#include "FileSystem_Engine.h"
#include <vgui/IScheme.h>
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar s_MaxHiEndModelTris( "perf_maxhiendmodeltris", "75000" );
static ConVar s_ModelScalabilityFactor( "pref_modelscalabilityfactor", "6" );
static ConVar s_MaxWorldTris( "perf_maxworldtris", "4000" );
static ConVar s_MaxMiscTris( "perf_maxmisctris", "1500" );
// This is the factor that we can assume that we'll scale terrain by.
// ie. if we have n triangles on the high end, we'll have n/s_DispToWorldTriRatio
// on the low end.
static ConVar s_DispToWorldTriRatio( "perf_disptoworldtriratio", "4.0" );
static ConVar s_PerfWarningLevel( "perf_warninglevel", "0.8" );
static ConVar s_MaxEffectiveTrisPerFrame( "perf_maxeffectivetrispreframe", "200000" );

ConVar r_Speeds_Background_Pad( "r_Speeds_Background_Pad", "5", 0, "Border for r_speeds background" );
ConVar r_Speeds_Background_Alpha( "r_Speeds_Background_Alpha", "200", 0, "Alpha for r_speeds background" );

//-----------------------------------------------------------------------------
// Purpose: Displays rendering statistics
//-----------------------------------------------------------------------------
class CDisplayStatsPanel : public CBasePanel, public IClientStatsTextDisplay
{
public:
					CDisplayStatsPanel( vgui::Panel *parent );
	virtual			~CDisplayStatsPanel( void );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	Paint();

	virtual bool	ShouldDraw( void );
	
	void SetDrawPos( int xPos, int yPos)
	{
		m_x = xPos;
		m_y = yPos;
	}
	
	void SetDrawColor( unsigned char r, unsigned char g, unsigned char b )
	{
		m_drawColor.r = r;
		m_drawColor.g = g;
		m_drawColor.b = b;
	}

	void SetDrawColorFromStatValues( float limit, float value )
	{
		GetWarningColor( limit,  value, m_drawColor.r, m_drawColor.g, m_drawColor.b );
	}
	
	void Newline( int n = 1 )
	{
		m_y += n * m_yinc;
	}

	// Inherited from IClientStatsTextDisplay
	void DrawStatsText( const char *fmt, ... );
		
private:

	void GetWarningColor( float limit, float value,
		unsigned char& r, unsigned char& g, unsigned char& b );

	color24 	m_drawColor;
	int 		m_x, m_y;
	int 		m_yinc;
	
	vgui::HFont m_hFont;

};

//-----------------------------------------------------------------------------
// Purpose: Instances the stats panel
// Input  : *parent - 
//-----------------------------------------------------------------------------
CDisplayStatsPanel::CDisplayStatsPanel( vgui::Panel *parent )
: CBasePanel( parent, "CDisplayStatsPanel" )
{
	m_drawColor.r = 255;
	m_drawColor.r = 255;
	m_drawColor.r = 255;
	m_y = 0;
	m_yinc = 1;
	m_x = 0;

	m_hFont = vgui::INVALID_FONT;

	SetSize( vid.width, vid.height );
	SetPos( 0, 0 );
	SetVisible( true );
	SetCursor( null );

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDisplayStatsPanel::~CDisplayStatsPanel( void )
{
}

void CDisplayStatsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// If you change this font, be sure to mark it with
	// $use_in_fillrate_mode in its .vmt file
	m_hFont = pScheme->GetFont( "DefaultFixed" );
	m_yinc = vgui::surface()->GetFontTall( m_hFont );
	Assert( m_hFont );
}

//-----------------------------------------------------------------------------
// Purpose: Only draw if r_speeds is active
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDisplayStatsPanel::ShouldDraw( void )
{
	return true;
}


//-----------------------------------------------------------------------------
// Allows client dll to draw stats
//-----------------------------------------------------------------------------
void CDisplayStatsPanel::DrawStatsText( const char *fmt, ... )
{
	if( r_speedsquiet.GetInt() && 
		( ( m_drawColor.r == m_drawColor.g && m_drawColor.g == m_drawColor.b ) || 
		  ( m_drawColor.r == 0 && m_drawColor.g == 255 && m_drawColor.b == 0 ) ) )
	{
		// it's greyscale or green, get outta here....
		return;
	}
	
	while ( *fmt == '\n' )
	{
		Newline();
		fmt++;
	}
	
	va_list argptr;
	va_start( argptr, fmt );
	DrawColoredText( m_hFont, m_x, m_y, m_drawColor.r, m_drawColor.g, m_drawColor.b, 255, const_cast<char *>(fmt), argptr );
	va_end( argptr );
	
	int iTest = strlen( fmt ) - 1;
	while ( iTest >= 0 )
	{
		if ( fmt[iTest--] != '\n' )
			break;
		Newline();
	}
}
		
void CDisplayStatsPanel::GetWarningColor( float limit, float value,
	unsigned char& r, unsigned char& g, unsigned char& b )
{
	if( value > limit )
	{
		r = 255;
		g = 0;
		b = 0;
	}
	else if( value > limit * s_PerfWarningLevel.GetFloat() )
	{
		r = 255;
		g = 255;
		b = 0;
	}
	else
	{
		r = 0;
		g = 255;
		b = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw current frame stats
//-----------------------------------------------------------------------------

void CDisplayStatsPanel::Paint() 
{
	VPROF( "CDisplayStatsPanel::Paint" );

	if ( !r_speeds.GetInt() )
		return;

	materialSystemInterface->Flush();
	g_EngineStats.PauseStats( true );

	// Draw a background so it's readable.
	if ( !r_speedsquiet.GetInt() )
	{
		int alpha = r_Speeds_Background_Alpha.GetInt();
		if ( alpha )
		{
			vgui::surface()->DrawSetColor( 0, 0, 0, alpha );
			int pad = r_Speeds_Background_Pad.GetInt();
			vgui::surface()->DrawFilledRect( pad, pad, vid.width-pad, vid.height-pad );
		}
	}

	SetDrawPos( 10, 50 );

	SetDrawColor( 255, 255, 255 );

	if( r_speeds.GetInt() == 1 )
	{
		DrawStatsText( "FRAME BUDGETS:\n" );
		DrawStatsText( "    displacement surface triangles/frame: effective: %7d lowend: %7d (actual triangles: %7d)\n", 
			-1,
			( int )( g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) * 
				( 1.0f / s_DispToWorldTriRatio.GetFloat() ) ),
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) );
		DrawStatsText( "    brush triangles/frame:                effective: %7d lowend: %7d (actual triangles: %7d)\n", 
			-1, 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_LOWEND_WORLD_TRIS_RENDERED ),
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED ) );
		DrawStatsText( "    dynamic brush triangles/frame:        effective: %7d lowend: %7d (actual triangles: %7d)\n", 
			 -1,
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_LOWEND_BRUSH_TRIANGLE_COUNT ),
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_BRUSH_TRIANGLE_COUNT ) );
		DrawStatsText( "                                                                   + -------\n" );
		int totalLowendTris = 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_LOWEND_WORLD_TRIS_RENDERED ) +
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_LOWEND_BRUSH_TRIANGLE_COUNT ) +
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) * 
			( 1.0f / s_DispToWorldTriRatio.GetFloat() );
		SetDrawColorFromStatValues( s_MaxWorldTris.GetFloat(), totalLowendTris );
		DrawStatsText( "                                                   total lowend/max: %d/%d\n", totalLowendTris, s_MaxWorldTris.GetInt() );
		SetDrawColor( 255, 255, 255 );
		DrawStatsText( "\n" );
		
		DrawStatsText( "    decal triangles/frame:                effective: %7d                 (actual triangles: %7d)\n",
			-1,
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_DECAL_TRIANGLES ) );
		DrawStatsText( "    model triangles/frame:                effective: %7d                 (actual triangles: %7d)\n",
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_EFFECTIVE_MODEL_TRIANGLES ),
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_ACTUAL_MODEL_TRIANGLES ) );
		DrawStatsText( "    shadow render-to-texture tris/frame:  effective: %7d                 (actual triangles: %7d)\n",
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_EFFECTIVE_SHADOW_RENDER_TO_TEXTURE_TRIANGLES ),
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES ) );
		DrawStatsText( "    shadow triangles/frame:               effective: %7d                 (actual triangles: %7d)\n",
			-1, //( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_SHADOW_EFFECTIVE_TRIANGLES ),
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_SHADOW_ACTUAL_TRIANGLES ) );
		DrawStatsText( "    other triangles/frame:                effective: %7d                 (actual triangles: %7d)\n",
			-1, -1 );
		DrawStatsText( "        (particles, ropes, beams, etc.)\n" );
		DrawStatsText( "  + -------------------------------------------------------------------\n" );

		int totalEffectiveTrisPerFrame = 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_EFFECTIVE_MODEL_TRIANGLES ) +
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_EFFECTIVE_SHADOW_RENDER_TO_TEXTURE_TRIANGLES );
		
		int totalActualTrisPerFrame = 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) + 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED ) + 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_BRUSH_TRIANGLE_COUNT ) + 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_DECAL_TRIANGLES ) + 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_ACTUAL_MODEL_TRIANGLES ) + 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES ) + 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_SHADOW_ACTUAL_TRIANGLES );
		
		SetDrawColorFromStatValues( s_MaxEffectiveTrisPerFrame.GetFloat(), totalEffectiveTrisPerFrame );
		DrawStatsText( "    total triangles/frame:                effective/max: %7d/%7d     (actual triangles: %7d)\n", totalEffectiveTrisPerFrame, s_MaxEffectiveTrisPerFrame.GetInt(), totalActualTrisPerFrame );
		SetDrawColor( 255, 255, 255 );
		DrawStatsText( "\n" );
		DrawStatsText( "    luxel updates/frame:                  actual/max: %5d/%d\n",
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_LIGHTMAP_LUXEL_UPDATES ), 
			-1 );
	}
	
	if( r_speeds.GetInt() >= 2 )
	{
		DrawStatsText( "fps: %.1f\n", ( float )( 1.0 / g_EngineStats.TimedStatInFrame( ENGINE_STATS_FRAME_TIME ) ) );
		DrawStatsText( "frametime: %.1f ms\n\n", ( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_FRAME_TIME ) ) );
		DrawStatsText( "world material changes: %d\n", ( int )g_EngineStats.CountedStatInFrame(ENGINE_STATS_NUM_WORLD_MATERIALS_RENDERED) );

#if 0
		DrawStatsText( "tris/sec: %.1f\n", ( float )g_EngineStats.GetCurrentFrameTrisPerSecond() );
#endif

		SetDrawColorFromStatValues( s_MaxHiEndModelTris.GetFloat(), g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_MRM_TRIS_RENDERED ) );
		DrawStatsText( "hiend num model tris (actual): %d/%d (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_MRM_TRIS_RENDERED ),
			( int )s_MaxHiEndModelTris.GetInt(),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_MRM_RENDER_TIME ) ) );
		int effectiveLowEndModelTris = 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_MRM_TRIS_RENDERED ) / 
			s_ModelScalabilityFactor.GetFloat();
		DrawStatsText( "lowend num model tris (approx): %d\n\n", 
			effectiveLowEndModelTris );
	}
	
	if (r_speeds.GetInt() != 8 && r_speeds.GetInt() > 1 )
	{
		
		int effectiveWorldTriangles;

		effectiveWorldTriangles = 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_BRUSH_TRIANGLE_COUNT) +
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED ) +
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_TRANSLUCENT_WORLD_TRIANGLES) +
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ) * 
			( 1.0f / s_DispToWorldTriRatio.GetFloat() );
		
		SetDrawColorFromStatValues( s_MaxWorldTris.GetFloat(), effectiveWorldTriangles );
		
		DrawStatsText( "effective lowend world triangles: %d/%d\n",
			effectiveWorldTriangles, s_MaxWorldTris.GetInt() );
		DrawStatsText( "    num bmodel tris: %d (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame(ENGINE_STATS_BRUSH_TRIANGLE_COUNT),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_BRUSH_TRIANGLE_TIME) ) );
		DrawStatsText( "    num world tris: %d (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED ),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_WORLD_RENDER_TIME ) ) );
		DrawStatsText( "    num translucent world tris: %d\n", 
			( int )g_EngineStats.CountedStatInFrame(ENGINE_STATS_TRANSLUCENT_WORLD_TRIANGLES) );
		DrawStatsText( "    num displacement tris: %d (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_DISP_RENDER_TIME ) ) );
		DrawStatsText( "    num render-to-texture shadow tris: %d (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame(ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SHADOW_TRIANGLE_TIME) ) );
		DrawStatsText( "    num shadow tris (world): %d (%.1f ms)\n\n", 
			( int )g_EngineStats.CountedStatInFrame(ENGINE_STATS_SHADOW_ACTUAL_TRIANGLES),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SHADOW_RENDER_TIME) ) );

		// fixme: need to take scalibility if any into account
		// fixme - need to count particles
		// fixme - need to count tracers. . those are in the client now, right?
		int numMiscTriangles = 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_DECAL_TRIANGLES);

		SetDrawColorFromStatValues( s_MaxMiscTris.GetFloat(), numMiscTriangles );
		DrawStatsText( "num misc tris: %d/%d\n", numMiscTriangles, s_MaxMiscTris.GetInt() );
		
		DrawStatsText( "    num decal tris: %d (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame(ENGINE_STATS_DECAL_TRIANGLES),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_DECAL_RENDER) ) );
		//DrawStatsText( "    num particle tris: %d (?? ms)\n", g_nParticlesDrawn ); 
		DrawStatsText( "update mrm lighting time: %.1f ms / %d cache misses / %d queries\n", 
			( float )( 1000.0f * g_EngineStats.TimedStatInFrame( ENGINE_STATS_UPDATE_MRM_LIGHTING_TIME ) ), 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_LIGHTCACHE_MISSES),
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_LIGHTCACHE_QUERIES) );
		DrawStatsText( "static prop lightstyle cache misses: %d\n", 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_LIGHTCACHE_STATICPROP_LIGHTSTYLE_MISSES) );
		DrawStatsText( "update dynamic lightmaps time: %.1f ms\n\n", 
			( float )( 1000.0f * g_EngineStats.TimedStatInFrame(ENGINE_STATS_DYNAMIC_LIGHTMAP_BUILD) ) );

		DrawStatsText( "Player think time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_PLAYER_THINK) ) );

		DrawStatsText( "Entity think time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_ENTITY_THINK) ) );

		DrawStatsText( "client renderable bone setup time: %.1f ms\n",	
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SETUP_BONES_TIME) ) );

		DrawStatsText( "time to find env_cubemaps: %.1f ms\n\n",	
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_FIND_ENVCUBEMAP_TIME) ) );
	}

	if( r_speeds.GetInt() == 2 )
	{
		DrawStatsText( "num box visible tests %d: (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_BOX_VISIBLE_TESTS ),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_BOX_VISIBLE_TEST_TIME ) ) );
		DrawStatsText( "num box traces %d: (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_BOX_TRACES ),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_BOX_TRACE_TIME ) ) );
		DrawStatsText( "num trace lines %d: (%.1f ms)\n", 
			( int )g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_TRACE_LINES ),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_TRACE_LINE_TIME ) ) );

		// displacement data
		DrawStatsText( "Num Disp Surfs Per Frame: %d\n", g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_SURFACES ) );
		DrawStatsText( "Num Disp Tris Per Frame: %d (%0.0f ms)\n", 
			g_EngineStats.CountedStatInFrame( ENGINE_STATS_NUM_DISP_TRIANGLES ),
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame( ENGINE_STATS_DISP_RENDER_TIME ) ) );
	}

#if 0
	if (r_speeds.GetInt() == 3)
	{
		DrawStatsText( "total # primitives (non-degenerate) : %d", 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_NUM_PRIMITIVES) );
		DrawStatsText( "draw dynamic : %.2f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_DRAW_DYNAMIC_MESH) );
		DrawStatsText( "  sw light : %.2f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_PROCESS_SOFTWARE) );
		DrawStatsText( "  flex : %.2f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_PROCESS_FLEX) );
		DrawStatsText( "  dynamic : %.2f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_GET_DYNAMIC) );
		DrawStatsText( "software process : %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_SOFTWARE_PROCESS) );
		DrawStatsText( "model rotate: %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_ROTATE) );
		DrawStatsText( "main: %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_DRAW_MESH) );
		DrawStatsText( "static: %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_DRAW_STATIC_MESH) );
		DrawStatsText( "static light: %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_DRAW_STATIC_SWLIGHT) );
		DrawStatsText( "hwskin: %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_DRAW_HWSKIN) );
		DrawStatsText( "load matrix: %.1f\n", 
			1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_MODEL_LOAD_MATRIX) );
	}
#endif

	if( r_speeds.GetInt() == 5 )
	{
		int i;
		for( i = ENGINE_STATS_TEST1; i <= ENGINE_STATS_TEST16; i++ )
		{
			if( g_EngineStats.TimedStatInFrame((EngineTimedStatId_t)i) != 0.0f )
			{
				DrawStatsText( "Engine test %d: %.1f ms\n", 
					( int )( i - ENGINE_STATS_TEST1 + 1 ),
					( float )( 1000.0 * g_EngineStats.TimedStatInFrame((EngineTimedStatId_t)i) ) );
			}
		}
	}

	if( r_speeds.GetInt() == 7 )
	{
		DrawStatsText( "Shadows moved: %d ms\n", 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_SHADOWS_MOVED) );
		DrawStatsText( "Shadows clipped: %d ms\n", 
			g_EngineStats.CountedStatInFrame(ENGINE_STATS_SHADOWS_CLIPPED) );
		DrawStatsText( "Shadow render time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SHADOW_RENDER_TIME) ) );
		DrawStatsText( "Subtree build time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SUBTREE_BUILD) ) );
		DrawStatsText( "Subtree trace time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SUBTREE_TRACE) ) );
		DrawStatsText( "Subtree trace world time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SUBTREE_TRACE_WORLD) ) );
		DrawStatsText( "Subtree trace link time: %.1f ms\n", 
			( float )( 1000.0 * g_EngineStats.TimedStatInFrame(ENGINE_STATS_SUBTREE_TRACE_LINKS) ) );

	}

	if( r_speeds.GetInt() == 8 )
	{
		FileSystemStatistics const* pStats = g_pFileSystem->GetFilesystemStatistics();
		DrawStatsText( "Filesystem Reads: %u (%0.1fk)\n",pStats->nReads,pStats->nBytesRead / 1024.f );
		DrawStatsText( "Filesystem Writes: %u (%0.1fk)\n",pStats->nWrites,pStats->nBytesWritten / 1024.f );
	}

	// Allow the client to draw its own stats, if necessary.
	g_EngineStats.DrawClientStats( this, r_speeds.GetInt() );

	materialSystemStatsInterface->SetStatGroup( MATERIAL_SYSTEM_STATS_OTHER );

	g_EngineStats.PauseStats( false );

	// Make sure we're at the front
//	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: Create the display stats panel
//-----------------------------------------------------------------------------
void SCR_CreateDisplayStatsPanel( vgui::Panel *parent )
{
	static CDisplayStatsPanel *displayStatsPanel = new CDisplayStatsPanel( parent );
}
