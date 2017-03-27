// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "glquake.h"
#include "keys.h"
#include "draw.h"
#include "console.h"
#include "vid.h"
#include "screen.h"
#include "sound.h"
#include "sbar.h"
#include "debugoverlay.h"
#include "ivguicenterprint.h"
#include "cdll_int.h"
#include "gl_matsysiface.h"
#include "cdll_engine_int.h"
#include "demo.h"


// In other C files.
qboolean V_CheckGamma( void );

extern ConVar host_speeds;

float		scr_fov_value = 90;	// 10 - 170
float		src_demo_override_fov = 0.0f;

// Download slider
ConVar      scr_connectmsg( "scr_connectmsg", "0" );
ConVar      scr_connectmsg1( "scr_connectmsg1", "0" );
ConVar      scr_connectmsg2( "scr_connectmsg2", "0" );
ConVar      scr_downloading( "scr_downloading", "-1" );

qboolean	scr_initialized;		// ready to draw

viddef_t	vid;				// global video state

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
bool		scr_drawloading;
static float scr_disabled_time;

//=============================================================================

// UNDONE: Move to client DLL
/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
void SCR_CalcRefdef (void)
{
	vid.recalc_refdef = 0;

	scr_fov_value = max( 2.0f, scr_fov_value );
	scr_fov_value = min( 170.0f, scr_fov_value );

	int width, height;
	materialSystemInterface->GetRenderTargetDimensions( width, height );
	scr_vrect.width		= width;
	scr_vrect.height	= height;
	scr_vrect.x			= 0;
	scr_vrect.y			= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SCR_Init (void)
{
	scr_initialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SCR_Shutdown( void )
{
	scr_initialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected && cls.state != ca_active)
		return;

	// redraw with no console and the loading plaque
	Con_ClearNotify ();

	SCR_CenterStringOff();

	scr_drawloading = true;
	SCR_UpdateScreen ();
	SCR_UpdateScreen ();

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	scr_drawloading = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *str - 
// Output : void SCR_CenterPrint
//-----------------------------------------------------------------------------
void SCR_CenterPrint (char *str)
{
	if ( !centerprint )
		return;

	centerprint->ColorPrint( 255, 255, 255, 0, str );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SCR_CenterStringOff( void )
{
	if ( !centerprint )
		return;

	centerprint->Clear();
}

//-----------------------------------------------------------------------------
// Purpose: This is called every frame, and can also be called explicitly to flush
//  text to the screen.
//-----------------------------------------------------------------------------
void SCR_UpdateScreen (void)
{
	// Always force the Gamma Table to be rebuilt. Otherwise,
	// we'll load textures with an all white gamma lookup table.
	V_CheckGamma();

	if (scr_skipupdate)
		return;

	if ( scr_disabled_for_loading )
	{
		if ( realtime - scr_disabled_time <= 60 )
			return;

		scr_disabled_for_loading = false;
		Con_Printf( "Connecting may have failed.\n" );
	}

	// No screen refresh on dedicated servers
	if ( cls.state == ca_dedicated )
		return;				

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	// Let demo system overwrite view origin/angles during playback
	demo->GetInterpolatedViewpoint();

	// Simulation meant to occur before any views are rendered
	ClientDLL_FrameStageNotify( FRAME_RENDER_START );
	
	{

		Shader_BeginRendering ();
		
		//
		// determine size of refresh window
		//
		if ( vid.recalc_refdef )
		{
			SCR_CalcRefdef ();
		}
		
		// Draw world, etc.
		V_RenderView();

		CL_TakeSnapshotAndSwap();

	}

	
	ClientDLL_FrameStageNotify( FRAME_RENDER_END );
}

