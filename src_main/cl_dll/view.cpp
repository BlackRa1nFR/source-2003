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
#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "iviewrender_beams.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "iclientmode.h"
#include "prediction.h"
#include "viewrender.h"
#include "c_te_legacytempents.h"
#include "mat_stub.h"
#include "vgui_int.h"
#include "tier0/vprof.h"
#include "IClientVehicle.h"
#include "engine/IEngineTrace.h"
#include "vmatrix.h"
#include "rendertexture.h"
#include "c_world.h"
#include <KeyValues.h>
#include "igameevents.h"

#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>

#if defined( HL2_CLIENT_DLL )
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "c_point_camera.h"
#endif
			    
extern ConVar default_fov;

static Vector g_vecRenderOrigin(0,0,0);
static QAngle g_vecRenderAngles(0,0,0);
static Vector g_vecVForward(0,0,0), g_vecVRight(0,0,0), g_vecVUp(0,0,0);
static VMatrix g_matCamInverse;

extern ConVar cl_forwardspeed;

static CViewRender g_ViewRender;
IViewRender *view = ( IViewRender * )&g_ViewRender;

static ConVar cl_rollspeed( "cl_rollspeed", "200" );
static ConVar cl_rollangle( "cl_rollangle", "2" );

static ConVar v_kicktime( "v_kicktime", "0.5" );
static ConVar v_kickroll( "v_kickroll", "0.6" );
static ConVar v_kickpitch( "v_kickpitch", "0.6" );

static ConVar v_centermove( "v_centermove", "0.15");
static ConVar v_centerspeed( "v_centerspeed","500" );

static ConVar v_split( "v_split", "0", 0, "Split the view into to panes ( 1 == vert, -1 == vert with inverted lower, other == nxn split )" );
static ConVar v_rearview( "v_rearview", "0", 0, "Rearview mirror mode" );

// 54 degrees approximates a 35mm camera - we determined that this makes the viewmodels
// and motions look the most natural.
static ConVar v_viewmodel_fov( "viewmodel_fov", "54", 0 );

static ConVar r_viewcubemap( "r_viewcubemap", "0" );
static ConVar r_viewcubemapsize( "r_viewcubemapsize", "96", FCVAR_ARCHIVE );
static ConVar cl_leveloverview( "cl_leveloverview", "0" );

static ConVar r_mapextents( "r_mapextents", "16384", 0, 
						   "Set the max dimension for the map.  This determines the far clipping plane" );

// UNDONE: Delete this or move to the material system?
ConVar	gl_clear( "gl_clear","0");

static ConVar r_farz( "r_farz", "-1", 0, "Override the far clipping plane. -1 means to use the value in env_fog_controller." );

CON_COMMAND( setoverview, "set map overview parameters" )
{	
	if ( engine->Cmd_Argc() == 5 )
	{
		float x = atof( engine->Cmd_Argv( 1 ) );
		float y = atof( engine->Cmd_Argv( 2 ) );
		float s = atof( engine->Cmd_Argv( 3 ) );
		float z = atof( engine->Cmd_Argv( 4 ) );

		g_ViewRender.SetOverviewParameters( x, y, s, z );
	}
	else
		Msg("setoverview params are: x y scale hight\n");

}

//-----------------------------------------------------------------------------
// Accessors to return the main view (where the player's looking)
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin()
{
	return g_vecRenderOrigin;
}

const QAngle &MainViewAngles()
{
	return g_vecRenderAngles;
}

const Vector &MainViewForward()
{
	return g_vecVForward;
}

const Vector &MainViewRight()
{
	return g_vecVRight;
}

const Vector &MainViewUp()
{
	return g_vecVUp;
}

const VMatrix &MainWorldToViewMatrix()
{
	return g_matCamInverse;
}

//-----------------------------------------------------------------------------
// Compute the world->camera transform
//-----------------------------------------------------------------------------
void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, 
	Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse )
{
	// Compute view bases
	AngleVectors( vecAngles, pVecForward, pVecRight, pVecUp );

	for (int i = 0; i < 3; ++i)
	{
		(*pMatCamInverse)[0][i] = (*pVecRight)[i];	
		(*pMatCamInverse)[1][i] = (*pVecUp)[i];	
		(*pMatCamInverse)[2][i] = -(*pVecForward)[i];	
		(*pMatCamInverse)[3][i] = 0.0F;
	}
	(*pMatCamInverse)[0][3] = -DotProduct( *pVecRight, vecOrigin );
	(*pMatCamInverse)[1][3] = -DotProduct( *pVecUp, vecOrigin );
	(*pMatCamInverse)[2][3] =  DotProduct( *pVecForward, vecOrigin );
	(*pMatCamInverse)[3][3] = 1.0F;
}


bool R_CullSphere(
	VPlane const *pPlanes,
	int nPlanes,
	Vector const *pCenter,
	float radius)
{
	for(int i=0; i < nPlanes; i++)
		if(pPlanes[i].DistTo(*pCenter) < -radius)
			return true;
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void StartPitchDrift( void )
{
	view->StartPitchDrift();
}

static ConCommand centerview( "centerview", StartPitchDrift );

//-----------------------------------------------------------------------------
// Purpose: Initializes all view systems
//-----------------------------------------------------------------------------
void CViewRender::Init( void )
{
	m_pDrawEntities		= cvar->FindVar( "r_drawentities" );
	m_pDrawBrushModels	= cvar->FindVar( "r_drawbrushmodels" );

	memset( &m_PitchDrift, 0, sizeof( m_PitchDrift ) );

	beams->InitBeams();
	tempents->Init();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor" );
	m_ModulateSingleColor.Init( "engine/modulatesinglecolor" );

	//FIXME:  
	QAngle angles;
	engine->GetViewAngles( angles );
	AngleVectors( angles, &m_vecLastFacing );
}


//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelInit( void )
{
	beams->ClearBeams();
	tempents->Clear();
	m_FrameNumber = 0;
	m_BuildWorldListsNumber = 0;
	
	// set default bounderies for overview
	
	m_OverviewSettings[0] = 0.0f;
	m_OverviewSettings[1] = 0.0f;
	m_OverviewSettings[2] = 1.0f;
	m_OverviewSettings[3] = 5000.0f;
}


//-----------------------------------------------------------------------------
// Purpose: Called at shutdown
//-----------------------------------------------------------------------------
void CViewRender::Shutdown( void )
{
	m_TranslucentSingleColor.Shutdown( );
	m_ModulateSingleColor.Shutdown( );
	beams->ShutdownBeams();
	tempents->Shutdown();
}


//-----------------------------------------------------------------------------
// Returns the frame number
//-----------------------------------------------------------------------------

int CViewRender::FrameNumber( void ) const
{
	return m_FrameNumber;
}

//-----------------------------------------------------------------------------
// Returns the worldlists build number
//-----------------------------------------------------------------------------

int CViewRender::BuildWorldListsNumber( void ) const
{
	return m_BuildWorldListsNumber;
}

//-----------------------------------------------------------------------------
// Purpose: Make sure view origin is pretty close so we don't look from inside a wall
//-----------------------------------------------------------------------------
void CViewRender::BoundOffsets( void )
{
	int	horiz_gap		= 14;
	int	vert_gap_down	= 22;
	int vert_gap_up		= 30;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	Vector playerOrigin = player->GetAbsOrigin();

	// Absolutely bound refresh reletive to entity clipping hull
	//  so the view can never be inside a solid wall
	if ( m_View.origin[0] < ( playerOrigin[0] - horiz_gap ) )
	{
		m_View.origin[0] = playerOrigin[0] - horiz_gap;
	}
	else if ( m_View.origin[0] > ( playerOrigin[0] + horiz_gap ))
	{
		m_View.origin[0] = playerOrigin[0] + horiz_gap;
	}

	if ( m_View.origin[1] < ( playerOrigin[1] - horiz_gap ) )
	{
		m_View.origin[1] = playerOrigin[1] - horiz_gap;
	}
	else if ( m_View.origin[1] > ( playerOrigin[1] + horiz_gap ) )
	{
		m_View.origin[1] = playerOrigin[1] + horiz_gap;
	}

	if ( m_View.origin[2] < ( playerOrigin[2] - vert_gap_down ) )
	{
		m_View.origin[2] = playerOrigin[2] - vert_gap_down;
	}
	else if ( m_View.origin[2] > ( playerOrigin[2] + vert_gap_up ) )
	{
		m_View.origin[2] = playerOrigin[2] + vert_gap_up;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start moving pitch toward ideal
//-----------------------------------------------------------------------------
void CViewRender::StartPitchDrift (void)
{
	if ( m_PitchDrift.laststop == gpGlobals->curtime )
	{
		// Something else is blocking the drift.
		return;		
	}

	if ( m_PitchDrift.nodrift || !m_PitchDrift.pitchvel )
	{
		m_PitchDrift.pitchvel	= v_centerspeed.GetFloat();
		m_PitchDrift.nodrift	= false;
		m_PitchDrift.driftmove	= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::StopPitchDrift (void)
{
	m_PitchDrift.laststop	= gpGlobals->curtime;
	m_PitchDrift.nodrift	= true;
	m_PitchDrift.pitchvel	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the client pitch angle towards cl.idealpitch sent by the server.
// If the user is adjusting pitch manually, either with lookup/lookdown,
//   mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
//-----------------------------------------------------------------------------
void CViewRender::DriftPitch (void)
{
	float		delta, move;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	if ( input->IsNoClipping() || ( player->GetGroundEntity() == NULL ) || render->IsPlayingDemo() )
	{
		m_PitchDrift.driftmove = 0;
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Don't count small mouse motion
	if ( m_PitchDrift.nodrift )
	{
		if ( fabs( input->GetLastForwardMove() ) < cl_forwardspeed.GetFloat() )
		{
			m_PitchDrift.driftmove = 0;
		}
		else
		{
			m_PitchDrift.driftmove += gpGlobals->frametime;
		}
	
		if ( m_PitchDrift.driftmove > v_centermove.GetFloat() )
		{
			StartPitchDrift ();
		}
		return;
	}
	
	// How far off are we
	delta = prediction->GetIdealPitch() - player->GetAbsAngles()[ PITCH ];
	if ( !delta )
	{
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Determine movement amount
	move = gpGlobals->frametime * m_PitchDrift.pitchvel;
	// Accelerate
	m_PitchDrift.pitchvel += gpGlobals->frametime * v_centerspeed.GetFloat();
	
	// Move predicted pitch appropriately
	if (delta > 0)
	{
		if ( move > delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() + QAngle( move, 0, 0 ) );
	}
	else if ( delta < 0 )
	{
		if ( move > -delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = -delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() - QAngle( move, 0, 0 ) );
	}
}

// This is called by cdll_client_int to setup view model origins. This has to be done before
// simulation so entities can access attachment points on view models during simulation.
void CViewRender::OnRenderStart()
{
	SetUpView();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetViewSetup( void ) const
{
	return &m_View;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//-----------------------------------------------------------------------------
void CViewRender::AddVisOrigin( const Vector& origin )
{
	m_bOverrideVisOrigin = true;
	m_rgVisOrigins[ m_nNumVisOrigins++ ] = origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DisableVis( void )
{
	m_bForceNoVis = true;
}

static Vector s_TestOrigin;
static QAngle s_TestAngles;

//-----------------------------------------------------------------------------
// Sets up the view parameters
//-----------------------------------------------------------------------------
void CViewRender::SetUpView()
{
	// Initialize view structure with default values
	float farZ;
	if ( r_farz.GetFloat() < 1 )
	{
		// Use the far Z from the map's parameters.
		farZ = r_mapextents.GetFloat() * 1.73205080757f;
		
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer )
		{
			CPlayerLocalData *local = &pPlayer->m_Local;

			if ( local->m_fog.farz > 0 )
				farZ = local->m_fog.farz;
		}
	}
	else
	{
		farZ = r_farz.GetFloat();
	}

	m_View.zFar				= farZ;
	m_View.zFarViewmodel	= farZ;
	// UNDONE: Make this farther out? 
	//  closest point of approach seems to be view center to top of crouched box
	m_View.zNear			= VIEW_NEARZ;
	m_View.zNearViewmodel	= 1;

	m_View.fov				= render->GetFieldOfView();

	//Find the offset our current FOV is from the default value
	float flFOVOffset		= default_fov.GetFloat() - m_View.fov;

	//Adjust the viewmodel's FOV to move with any FOV offsets on the viewer's end
	m_View.fovViewmodel		= v_viewmodel_fov.GetFloat() - flFOVOffset;
	
	m_View.context			= 0;
	m_View.clearColor		= (gl_clear.GetInt() != 0) ? true : m_View.clearColor;
	m_View.clearDepth		= true;

	m_View.m_bOrtho = false;

	// Enable access to all model bones except view models.
	C_BaseAnimating::AllowBoneAccess( true, false );
	// Enable spatial partition access to edicts
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	// FIXME: Are there multiple views? If so, then what?
	// FIXME: What happens when there's no player?
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer)
	{
		IClientVehicle *pVehicle = pPlayer->GetVehicle();
		if (!pVehicle)
		{

			if ( !pPlayer->IsObserver() )
			{
				pPlayer->CalcPlayerView( m_View.origin, m_View.angles, m_View.fov );
			}
			else
			{
				pPlayer->CalcObserverView( m_View.origin, m_View.angles, m_View.fov );
			}
		}
		else
		{
			pPlayer->CalcVehicleView( pVehicle, m_View.origin, m_View.angles, m_View.zNear, m_View.zFar, m_View.fov );
		}
	}

	// If we are looking through another entities eyes, then override the angles/origin for m_View
	int viewentity = render->GetViewEntity();
	if ( viewentity > engine->GetMaxClients() )
	{
		C_BaseEntity *ve = cl_entitylist->GetEnt( viewentity );
		if ( ve )
		{
			VectorCopy( ve->GetAbsOrigin(), m_View.origin );
			VectorCopy( ve->GetAbsAngles(), m_View.angles );
		}
	}
	
	// FIXME:  Should this if go away and we move this code to CalcView?
	if ( !render->IsPaused() )
	{
		g_pClientMode->OverrideView( &m_View );
	}

	// Disable spatical partition access
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
	// Enable access to all model bones
	C_BaseAnimating::AllowBoneAccess( true, true );

	// Remember the origin, not reflected on a water plane.
	m_View.m_vUnreflectedOrigin = m_View.origin;
	
	g_vecRenderOrigin = m_View.origin;
	g_vecRenderAngles = m_View.angles;

	// Compute the world->main camera transform
	ComputeCameraVariables( m_View.origin, m_View.angles, 
		&g_vecVForward, &g_vecVRight, &g_vecVUp, &g_matCamInverse );

	// set up the hearing origin...
	engine->SetHearingOrigin( m_View.origin, m_View.angles );

	s_TestOrigin = m_View.origin;
	s_TestAngles = m_View.angles;
}


void CViewRender::DrawHighEndMonitors( CViewSetup cameraView )
{
#ifdef HL2_CLIENT_DLL
	// FIXME: this should check for the ability to do a render target maybe instead.
	// FIXME: shouldn't have to truck through all of the visible entities for this!!!!
	ITexture *pRenderTarget = GetCameraTexture();

	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();
	
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PushMatrix();
	
	// Save off the viewport...
	int x, y, w, h;
	materials->GetViewport( x, y, w, h );
	
	ITexture *pSaveRenderTarget = materials->GetRenderTarget();
	
	materials->SetRenderTarget( pRenderTarget );
	
	int width, height;
	materials->GetRenderTargetDimensions( width, height );
	materials->Viewport( 0, 0, width, height );
	
	// FIXME: shouldn't have to truck through all of the visible entities for this!!!!
	// FIXME: stick this is its own function.
	int cameraNum = 0;
	for( ClientEntityHandle_t entity = ClientEntityList().FirstHandle();
	     entity != ClientEntityList().InvalidHandle(); 		
	     entity = ClientEntityList().NextHandle( entity ) )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( entity );
		C_PointCamera *pCameraEnt = dynamic_cast<C_PointCamera *>( pEnt );
		if( !pCameraEnt || !pCameraEnt->IsActive() )
		{
			continue;
		}
		cameraView.width = width;
		cameraView.height = height;
		
		// need to render the camera viewport.
		cameraView.x = 0;
		
		// OpenGL has 0,0 in lower left
		cameraView.y = 0;
		
		
		cameraView.clearColor = false;
		cameraView.clearDepth = true;
		
		cameraView.origin = pCameraEnt->GetAbsOrigin();
		//			AddVisOrigin( cameraView.origin );
		cameraView.angles = pCameraEnt->GetAbsAngles();
		cameraView.fov = pCameraEnt->GetFOV();
		cameraView.m_bOrtho = false;
		cameraView.m_bForceAspectRatio1To1 = true;
		ViewDrawScene( true, cameraView );
		//			RenderView( cameraView, false );
		cameraNum++;
	}
	materials->SetRenderTarget( pSaveRenderTarget );
	materials->Viewport( x, y, w, h );
	
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();
	
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PopMatrix();
#endif // HL2_CLIENT_DLL
}

void CViewRender::DrawLowEndMonitors( CViewSetup cameraView, const vrect_t *rect )
{
#ifdef HL2_CLIENT_DLL
	// FIXME: this should check for the ability to do a render target maybe instead.
	if( !g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() )
	{
		// FIXME: shouldn't have to truck through all of the visible entities for this!!!!
		// FIXME: stick this is its own function.
		int cameraNum = 0;
		for( ClientEntityHandle_t entity = ClientEntityList().FirstHandle();
			 entity != ClientEntityList().InvalidHandle(); 		
			 entity = ClientEntityList().NextHandle( entity ) )
		{
			C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( entity );
			C_PointCamera *pCameraEnt = dynamic_cast<C_PointCamera *>( pEnt );
			if( !pCameraEnt || !pCameraEnt->IsActive() )
			{
				continue;
			}
			cameraView.width = 256.0f;
			cameraView.height = 256.0f * ( 3.0f / 4.0f );

			// need to render the camera viewport.
			cameraView.x = rect->width - (1+cameraNum) * cameraView.width;
			
			// OpenGL has 0,0 in lower left
			cameraView.y = rect->y;


			cameraView.clearColor = false;
			cameraView.clearDepth = true;
			
			cameraView.origin = pCameraEnt->GetAbsOrigin();
//			AddVisOrigin( cameraView.origin );
			cameraView.angles = pCameraEnt->GetAbsAngles();
			cameraView.fov = pCameraEnt->GetFOV();
			cameraView.m_bOrtho = false;
			// Render world and all entities, particles, etc.
			// Start view, clear frame/z buffer if necessary
			SetupVis( cameraView );
			ViewDrawScene( true, cameraView );
//			RenderView( cameraView, false );
			cameraNum++;
		}
	}
#endif // HL2_CLIENT_DLL
}

ConVar r_anamorphic( "r_anamorphic", "0" );

static float ScaleFOVByWidthRatio( float fovDegrees, float ratio )
{
	float halfAngleRadians = fovDegrees * ( 0.5f * M_PI / 180.0f );
	float t = tan( halfAngleRadians );
	t *= ratio;
	float retDegrees = ( 180.0f / M_PI ) * atan( t );
	return retDegrees * 2.0f;
}

void CViewRender::SetOverviewParameters( float x, float y, float scale, float height )
{
	m_OverviewSettings[0] = x;
	m_OverviewSettings[1] = y;
	m_OverviewSettings[2] = scale;
	m_OverviewSettings[3] = height;
}

//-----------------------------------------------------------------------------
// Purpose: Sets view parameters for level overview mode
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::SetUpOverView()
{
	m_View.m_bOrtho = true;

	float aspect = (float)m_View.width/(float)m_View.height;

	int size_y = 1024.0f * m_OverviewSettings[2];
	int size_x = size_y * aspect;	// standard screen aspect 
	
	m_View.m_OrthoLeft   = 0;
	m_View.m_OrthoTop    = -size_y;
	m_View.m_OrthoRight  = size_x;
	m_View.m_OrthoBottom = 0;

	m_View.origin.x = m_OverviewSettings[0];
	m_View.origin.y = m_OverviewSettings[1];
	m_View.origin.z = m_OverviewSettings[3];
	m_View.angles = QAngle( 90, 90, 0 );
//		DisableVis();

	m_View.clearColor = true;
	
	materials->ClearColor4ub( 0, 255, 0, 0 );

	// render->DrawTopView( true );
}

//-----------------------------------------------------------------------------
// Purpose: Render current view into specified rectangle
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::Render( vrect_t *rect )
{
	Assert(s_TestOrigin == m_View.origin);
	Assert(s_TestAngles == m_View.angles);

	VPROF( "CViewRender::Render" );

	// Stub out the material system if necessary.
	CMatStubHandler matStub;
	bool drawViewModel;

	engine->EngineStats_BeginFrame();
	
	// Assume normal vis
	m_bForceNoVis			= false;
	m_bOverrideVisOrigin	= false;
	m_nNumVisOrigins		= 0;

	// hook into the r_anamorphic convar from the engine.
	switch( r_anamorphic.GetInt() )
	{
	case 1:
		m_View.fov = ScaleFOVByWidthRatio( m_View.fov, ( 16.0f / 9.0f ) / ( 4.0f / 3.0f ) );
		m_View.fovViewmodel = ScaleFOVByWidthRatio( m_View.fovViewmodel, ( 16.0f / 9.0f ) / ( 4.0f / 3.0f ) );
		break;
	case 2:
		m_View.fov = ScaleFOVByWidthRatio( m_View.fov, ( 16.0f / 10.0f ) / ( 4.0f / 3.0f ) );
		m_View.fovViewmodel = ScaleFOVByWidthRatio( m_View.fovViewmodel, ( 16.0f / 10.0f ) / ( 4.0f / 3.0f ) );
		break;
	default:
		break;
	}
	
	// Invoke pre-render methods
	// IGameSystem::PreRenderAllSystems();

	// Let the client mode hook stuff.
	g_pClientMode->PreRender(&m_View);

	vrect_t vr = *rect;
	
	g_pClientMode->AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

	m_View.x				= vr.x;
	m_View.y				= vr.y;
	m_View.width			= vr.width;
	m_View.height			= vr.height;

	// Determine if we should draw view model ( client mode override )
	drawViewModel = g_pClientMode->ShouldDrawViewModel();

	if ( cl_leveloverview.GetInt() )
	{
		SetUpOverView();		
		drawViewModel = false;
	}

	// Apply any player specific overrides
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		// Override view model if necessary
		if ( !pPlayer->m_Local.m_bDrawViewmodel )
		{
			drawViewModel = false;
		}
	}

#ifdef HL2_CLIENT_DLL
	if( g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() )
	{
		DrawHighEndMonitors( m_View );	
	}
#endif // HL2_CLIENT_DLL

	RenderView( m_View, drawViewModel );

#ifdef HL2_CLIENT_DLL
	if( !g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() )
	{
		DrawLowEndMonitors( m_View, rect );
	}
#endif // HL2_CLIENT_DLL

	g_pClientMode->PostRender();

	engine->EngineStats_EndFrame();
	// Stop stubbing the material system so we can see r_speeds.	
	matStub.End();

	// paint the vgui screen
	VGui_PreRender();

	CViewSetup view2d;
	view2d.x				= rect->x;
	view2d.y				= rect->y;
	view2d.width			= rect->width;
	view2d.height			= rect->height;
	view2d.clearColor		= false; //IsMatStubEnabled(); // clear the screen before vgui drawing if mat_stub is on
	view2d.clearDepth		= false;
	render->ViewSetup2D( &view2d );

	// The crosshair needs to get at the current setup stuff
	AllowCurrentViewAccess( true );
	{
		render->VguiPaint();
	}

	if ( cl_leveloverview.GetInt() )
	{
		// draw a 1024x1024 pixel box
		vgui::surface()->DrawSetColor( 255, 0, 0, 255 );
		vgui::surface()->DrawLine( 1024, 0, 1024, 1024 );
		vgui::surface()->DrawLine( 0, 1024, 1024, 1024 );
	}

	AllowCurrentViewAccess( false );
}

