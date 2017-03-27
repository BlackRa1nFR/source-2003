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
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "gl_model_private.h"
#include "console.h"
#include "vid.h"
#include "cdll_engine_int.h"
#include "gl_cvars.h"
#include "ivrenderview.h"
#include "gl_matsysiface.h"
#include "gl_drawlights.h"
#include "gl_rsurf.h"
#include "r_local.h"
#include "debugoverlay.h"
#include "EngineStats.h"
#include "vgui_int.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "demo.h"
#include "IStudioRender.h"
#include "materialsystem/imesh.h"
#include "tier0/vprof.h"
#include "host.h"
#include "view.h"
#include "enginebugreporter.h"


class IClientEntity;

float r_blend;
float r_colormod[3] = { 1, 1, 1 };

extern IStudioRender *g_pStudioRender;
extern vrect_t	scr_vrect;
extern float	scr_fov_value;

colorVec R_LightPoint (Vector& p);
void R_DrawLightmaps( void );
void R_DrawIdentityBrushModel( model_t *model );

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

extern ConVar r_avglightmap;

/*
=================
V_CheckGamma

FIXME:  Define this as a change function to the ConVar's below rather than polling it
 every frame.  Note, still need to make sure it gets called very first time through frame loop.
=================
*/
qboolean V_CheckGamma (void)
{
	static float oldgammavalue, oldtexgamma, oldbrightness, ambientr, ambientg, ambientb;
	static int oldoverbright = -1;
	extern void GL_RebuildLightmaps( void );
	
	// Refresh all lightmaps if r_avglightmap changes
	static int lastLightmap = -1;
	if (r_avglightmap.GetInt() != lastLightmap)
	{
		lastLightmap = r_avglightmap.GetInt();
		GL_RebuildLightmaps();
	}

	int overbright = oldoverbright;
	bool obchecked = false;
	if ( ( host_tickcount & 0x3f ) == 0 )
	{
		obchecked = true;

		if ( g_pMaterialSystemHardwareConfig->SupportsOverbright())	
		{
			overbright = mat_overbright.GetInt();
		}
	}

	if ((v_gamma.GetFloat() == oldgammavalue) && 
		(v_texgamma.GetFloat() == oldtexgamma) &&
		(v_brightness.GetFloat() == oldbrightness) && 
		(overbright == oldoverbright) )
		return false;

	// clamp values to prevent cheating in multiplayer
	if ( cl.maxclients > 1 )
	{
		if ( v_brightness.GetFloat() > 2.0 )
		{
			v_brightness.SetValue( 2.0f );
		}

		if ( v_gamma.GetFloat() < 1.8f )
		{
			v_gamma.SetValue( 1.8f );
		}
	}
	
	if( !g_pMaterialSystemHardwareConfig || !g_pMaterialSystemHardwareConfig->SupportsOverbright() )
	{
		if( v_brightness.GetFloat() != 1.0f )
		{
			v_gamma.SetValue( 1.0f );
		}
	}
	extern void InitMathlib( void );
	InitMathlib();

	oldgammavalue = v_gamma.GetFloat();
	oldtexgamma = v_texgamma.GetFloat();
	oldbrightness = v_brightness.GetFloat();
	if ( obchecked )
	{
		oldoverbright = overbright;
	}
	GL_RebuildLightmaps();
	// FIXME: should reload all textures, invalidate models, etc.
	
	vid.recalc_refdef = 1;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the view renderer
// Output : void V_Init
//-----------------------------------------------------------------------------
void V_Init( void )
{
	vid.recalc_refdef = 1;
	BuildGammaTable( 2.2f, 2.2f, 0.0f, 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void V_Shutdown( void )
{
	// TODO, cleanup
}

//-----------------------------------------------------------------------------
// Purpose: Render the world
//-----------------------------------------------------------------------------
void V_RenderView( void )
{
	VPROF( "V_RenderView" );

	g_EngineRenderer->FrameBegin();

	bool can_render_world = host_state.worldmodel && ( cls.signon == SIGNONS ) && ( cls.state == ca_active );

	// Because we now do a lot of downloading before spawning map, don't render anything world related 
	//  until we are an active client.
	if ( !can_render_world )
	{
		// Need to clear the screen in this case, cause we're not drawing
		// the loading screen.
		materialSystemInterface->ClearBuffers( true, true );
		VGui_Paint();
		g_EngineRenderer->FrameEnd();
		return;	
	}

	// We can get into situations where some other material system app
	// is trying to start up; in those cases, we shouldn't render...
	if (!g_LostVideoMemory)
	{
		g_ClientDLL->View_Render( &scr_vrect );
	}

	g_EngineRenderer->FrameEnd();
}

void Linefile_Draw( void );

//-----------------------------------------------------------------------------
// Purpose: Expose rendering interface to client .dll
//-----------------------------------------------------------------------------
class CVRenderView : public IVRenderView
{
public:
	void TouchLight( dlight_t *light )
	{
		int i;
		
		i = light - cl_dlights;
		if (i >= 0 && i < MAX_DLIGHTS)
		{
			r_dlightchanged |= (1 << i);
		}
	}

	void DrawBrushModel( 
		IClientEntity *baseentity, 
		model_t *model, 
		const Vector& origin, 
		const QAngle& angles, 
		bool sort )
	{
		R_DrawBrushModel( baseentity, model, origin, angles, sort );
	}

	// Draw brush model shadow
	void DrawBrushModelShadow( IClientRenderable *pRenderable )
	{
		R_DrawBrushModelShadow( pRenderable );
	}

	void DrawIdentityBrushModel( model_t *model )
	{
		R_DrawIdentityBrushModel( model );
	}

	void Draw3DDebugOverlays( void )
	{
		// Fixme, find a better home
		demo->DrawDebuggingInfo();
		CDebugOverlay::Draw3DOverlays();
	}

	void BeginDrawMRM( void )
	{
		g_EngineStats.BeginTimedStat( ENGINE_STATS_MRM_RENDER_TIME );
	}

	void EndDrawMRM( void )
	{
		g_EngineStats.EndTimedStat( ENGINE_STATS_MRM_RENDER_TIME );
	}

	void SetBlend( float blend )
	{
		r_blend = blend;
	}

	float GetBlend( void )
	{
		return r_blend;
	}

	void SetColorModulation( float const* blend )
	{
		VectorCopy( blend, r_colormod );
	}

	void GetColorModulation( float* blend )
	{
		VectorCopy( r_colormod, blend );
	}

	void SceneBegin( void )
	{
		g_EngineRenderer->DrawSceneBegin();
	}

	void SceneEnd( void )
	{
		g_EngineRenderer->DrawSceneEnd();
	}
	 
	void GetVisibleFogVolume( int *visibleFogVolume, int *visibleFogVolumeLeaf, bool *eyeInFogVolume, float *distanceToWater, const Vector& eyePoint )
	{
		R_GetVisibleFogVolume( visibleFogVolume, visibleFogVolumeLeaf, eyeInFogVolume, distanceToWater, eyePoint );
	}
	
	void BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf )
	{	
		g_EngineRenderer->BuildWorldLists( pInfo, updateLightmaps, iForceViewLeaf );
	}

	void DrawWorldLists( unsigned long flags )
	{	
		g_EngineRenderer->DrawWorldLists( flags );
	}

	// Optimization for top view
	void DrawTopView( bool enable )
	{
		R_DrawTopView( enable );
	}

	void TopViewBounds( Vector2D const& mins, Vector2D const& maxs )
	{
		R_TopViewBounds( mins, maxs );
	}

	void DrawLights( void )
	{
		DrawLightSprites();

#ifdef USE_CONVARS
		DrawLightDebuggingInfo();
#endif
	}

	void DrawMaskEntities( void )
	{
		// UNDONE: Don't do this with masked brush models, they should probably be in a separate list
		// R_DrawMaskEntities()
	}

	void DrawTranslucentSurfaces( int sortIndex, unsigned long flags )
	{
		Shader_DrawTranslucentSurfaces( sortIndex, flags );
	}

	bool LeafContainsTranslucentSurfaces( int sortIndex, unsigned long flags )
	{
		return Shader_LeafContainsTranslucentSurfaces( sortIndex, flags );
	}

	void DrawLineFile( void )
	{
		Linefile_Draw();
	}

	void DrawLightmaps( void )
	{
		R_DrawLightmaps();
	}

	void ViewSetup3D( const CViewSetup *view, Frustum frustumPlanes )
	{
		g_EngineRenderer->ViewSetup3D( view, frustumPlanes );
	}
	
	void ViewSetupVis( bool novis, int numorigins, const Vector origin[] )
	{
		g_EngineRenderer->ViewSetupVis( novis, numorigins, origin );
	}

	void ViewSetup2D( const CViewSetup *view )
	{
		materialSystemInterface->Viewport( view->x, view->y, view->width, view->height );
		
		if ( view->clearColor || view->clearDepth )
		{
			materialSystemInterface->ClearBuffers( view->clearColor, view->clearDepth );
		}

		materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );
		materialSystemInterface->LoadIdentity();
		materialSystemInterface->Scale( 1, -1, 1 );
		materialSystemInterface->Ortho( 0, 0, view->width, view->height, -99999, 99999 );

		materialSystemInterface->MatrixMode( MATERIAL_VIEW );
		materialSystemInterface->LoadIdentity();

		materialSystemInterface->MatrixMode( MATERIAL_MODEL );
		materialSystemInterface->LoadIdentity();
	}

	void VguiPaint( void )
	{
		::VGui_Paint();
	}

	void ViewDrawFade( byte *color, IMaterial* pFadeMaterial )
	{
		VPROF_BUDGET( "ViewDrawFade", VPROF_BUDGETGROUP_WORLD_RENDERING );
		g_EngineRenderer->ViewDrawFade( color, pFadeMaterial );
	}

	void SetProjectionMatrix( float fov, float zNear, float zFar )
	{
		g_EngineRenderer->SetProjectionMatrix( fov, zNear, zFar );
	}

	colorVec GetLightAtPoint( Vector& pos )
	{
		return R_LightPoint( pos );
	}

	bool IsPlayingDemo( void )
	{
		return demo->IsPlayingBack();
	}

	bool IsRecordingDemo( void )
	{
		return demo->IsRecording();
	}
	
	bool IsPlayingTimeDemo( void )
	{
		return demo->IsPlayingBack_TimeDemo();
	}

	bool IsPaused( void )
	{
		return (cl.paused || bugreporter->ShouldPause()) ? true : false;
	}

	int GetViewEntity( void )
	{
		return cl.viewentity;
	}

	float GetFieldOfView( void )
	{
		extern float src_demo_override_fov;

		if ( demo->IsPlayingBack() &&
			src_demo_override_fov != 0.0f )
		{
			return src_demo_override_fov;
		}
		return scr_fov_value;
	}

	unsigned char **GetAreaBits( void )
	{
		return &cl.pAreaBits;
	}

	float GetInterpolationTime( void )
	{
		return cl_interp.GetFloat();
	}

	// World fog for world rendering
	void SetFogVolumeState( int fogVolume, bool useHeightFog )
	{
		R_SetFogVolumeState(fogVolume, useHeightFog );
	}

	IMaterial *GetFogVolumeMaterial( int fogVolume )
	{
		return R_GetFogVolumeMaterial( fogVolume );
	}

	virtual float GetWaterHeight( int fogVolume )
	{
		return R_GetWaterHeight( fogVolume );
	}

	virtual void InstallBrushSurfaceRenderer( IBrushRenderer* pBrushRenderer )
	{
		R_InstallBrushRenderOverride( pBrushRenderer );
	}
};

EXPOSE_SINGLE_INTERFACE( CVRenderView, IVRenderView, VENGINE_RENDERVIEW_INTERFACE_VERSION );





