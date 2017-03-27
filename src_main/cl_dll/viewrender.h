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
#if !defined( VIEWRENDER_H )
#define VIEWRENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class ConVar;
struct WorldListInfo_t;
class CRenderList;
class IClientVehicle;

//-----------------------------------------------------------------------------
// Purpose: Stored pitch drifting variables
//-----------------------------------------------------------------------------
class CPitchDrift
{
public:
	float		pitchvel;
	bool		nodrift;
	float		driftmove;
	double		laststop;
};


//-----------------------------------------------------------------------------
// Purpose: Implements the interview to view rendering for the client .dll
//-----------------------------------------------------------------------------
class CViewRender : public IViewRender
{
public:
					CViewRender();
	virtual			~CViewRender( void ) {}

// Implementation of IViewRender interface
public:

	virtual void	Init( void );
	virtual void	Shutdown( void );

	// Render functions
	virtual void	OnRenderStart();
	virtual	void	Render( vrect_t *rect );
	virtual void	RenderView( const CViewSetup &view, bool drawViewmodel );

	virtual void	StartPitchDrift( void );
	virtual void	StopPitchDrift( void );

	// Called once per level change
	void			LevelInit( void );
	// Add entity to transparent entity queue
	void			AddVisibleEntity( C_BaseEntity *pEnt );

	virtual VPlane*	GetFrustum();

	bool			ShouldDrawBrushModels( void );
	bool			ShouldDrawEntities( void );

	const CViewSetup *GetViewSetup( void ) const;
	
	void			AddVisOrigin( const Vector& origin );
	void			DisableVis( void );

	int				FrameNumber() const;
	int				BuildWorldListsNumber() const;

	// Sets up the view model position relative to the local player
	void			MoveViewModels( );

	// Gets the abs origin + angles of the view models
	void			GetViewModelPosition( int nIndex, Vector *pPos, QAngle *pAngle );

	void			SetCheapWaterStartDistance( float flCheapWaterStartDistance );
	void			SetCheapWaterEndDistance( float flCheapWaterEndDistance );

	void			GetWaterLODParams( float &flCheapWaterStartDistance, float &flCheapWaterEndDistance );

	void			DriftPitch (void);

public:	// public non-interface functions
	void			SetOverviewParameters( float x, float y, float scale, float height );

private:
	// Flags for drawing
	enum DrawFlags_t
	{
		DF_RENDER_REFRACTION	= 0x1,
		DF_RENDER_REFLECTION	= 0x2,

		DF_CLIP_Z				= 0x4,
		DF_CLIP_BELOW			= 0x8,

		DF_RENDER_UNDERWATER	= 0x10,
		DF_RENDER_ABOVEWATER	= 0x20,
		DF_RENDER_WATER			= 0x40,

		DF_CLEARDEPTH			= 0x100,
		DF_WATERHEIGHT			= 0x200,
		DF_BUILDWORLDLISTS		= 0x400,
		DF_DRAWSKYBOX			= 0x800,

		DF_FUDGE_UP				= 0x1000,

		DF_DRAW_ENTITITES		= 0x2000,
		DF_CLEARCOLOR			= 0x4000,
	};

	// Draw setup
	void			BoundOffsets( void );

	float			CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);

	void			SetupRenderList( const CViewSetup *pView, WorldListInfo_t& info, CRenderList &renderList );

	// General draw methods
	void			ViewDrawScene( bool drawSkybox, const CViewSetup &view, bool bSetupViewModel = false, bool bDrawViewModel = false );

	bool			Draw3dSkyboxworld( const CViewSetup &view );
	
	// If iForceViewLeaf is not -1, then it uses the specified leaf as your starting area for setting up area portal culling.
	// This is used by water since your reflected view origin is often in solid space, but we still want to treat it as though
	// the first portal we're looking out of is a water portal, so our view effectively originates under the water.
	void			BuildWorldRenderLists( 
		const CViewSetup *pView, 
		WorldListInfo_t& info, 
		CRenderList &renderList,
		bool updateLightmaps,
		int iForceViewLeaf );

	void			DrawWorld( WorldListInfo_t& info, CRenderList &renderList, int flags ); 

	void			DrawHighEndMonitors( CViewSetup cameraView );
	void			DrawLowEndMonitors( CViewSetup cameraView, const vrect_t *rect );
	void			SetupVis( const CViewSetup& view );

	// Drawing primitives
	bool			ShouldDrawViewModel( bool drawViewmodel );
	void			DrawViewModels( const CViewSetup &view, bool drawViewmodel );

	// Fog setup
	void			EnableWorldFog( void );
	void			Enable3dSkyboxFog( void );
	void			DisableFog( void );
	
	// Draws all opaque/translucent renderables in leaves that were rendered
	void			DrawOpaqueRenderables( WorldListInfo_t& info, CRenderList &renderList );
	void			DrawTranslucentRenderables( WorldListInfo_t& info, CRenderList &renderList );

	// Sets up the view parameters
	void			SetUpView();
	// Sets up the view parameters of map overview mode (cl_leveloverview)
	void			SetUpOverView();

	// Purpose: Renders world and all entities, etc.
	void			DrawWorldAndEntities( bool drawSkybox, const CViewSetup &view );

	// Draws all the debugging info
	void			Draw3DDebuggingInfo( const CViewSetup &view );
	void			Draw2DDebuggingInfo( const CViewSetup &view );

	// Screen-space effects (uses the contents of the frame buffer)
	void			SetScreenSpaceEffectMaterial( IMaterial *pMaterial );
	void			PerformScreenSpaceEffects();

	// Overlays
	void			SetScreenOverlayMaterial( IMaterial *pMaterial );
	IMaterial		*GetScreenOverlayMaterial( );
	void			PerformScreenOverlay();

	// Water-related methods
	void			WaterDrawWorldAndEntities( bool drawSkybox, const CViewSetup &view );
	void			DrawOpaqueRenderablesInWater( WorldListInfo_t& info, CRenderList &renderList, bool drawUnderWater, bool drawAboveWater );
	void			DrawTranslucentRenderablesInWater( WorldListInfo_t& info, CRenderList &renderList, bool drawUnderWater, bool drawAboveWater );
	
	void			WaterDrawHelper( 
		const CViewSetup &view, 
		WorldListInfo_t &info, 
		CRenderList &renderList, 
		float waterHeight, 
		int flags, 
		int iForceViewLeaf = -1 );
	
	void			SetRenderTargetAndView( CViewSetup &view, float waterHeight, int flags );	// see DrawFlags_t
	void			ViewDrawScene_EyeAboveWater( bool drawSkybox, const CViewSetup &view, int visibleFogVolume, int visibleFogVolumeLeaf, bool bReflect, bool bRefract, bool bReflectEntities );
	void			ViewDrawScene_EyeUnderWater( bool drawSkybox, const CViewSetup &view, int visibleFogVolume, bool bReflect, bool bRefract );
	void			ViewDrawScene_NoWater( bool drawSkybox, const CViewSetup &view, bool bCheapWater );

	// Renders all translucent world + detail objects in a particular set of leaves
	void DrawTranslucentWorldInLeaves( int iCurLeaf, int iFinalLeaf, int *pLeafList, int drawFlags, int &nDetailLeafCount, int* pDetailLeafList );

	// A version which renders detail objects in a big batch for perf reasons
	void DrawTranslucentRenderablesV2( WorldListInfo_t& info, CRenderList &renderList );
	void DrawTranslucentRenderablesInWaterV2( WorldListInfo_t& info, CRenderList &renderList, 
											 bool drawUnderWater, bool drawAboveWater );

private:
	enum
	{
		ANGLESHISTORY_SIZE	= 8,
		ANGLESHISTORY_MASK	= 7,
	};

	// This stores all of the view setup parameters that the engine needs to know about
	CViewSetup		m_View;
	
	// VIS Overrides
	// Set to true to turn off client side vis ( !!!! rendering will be slow since everything will draw )
	bool			m_bForceNoVis;		

	// Set to true if you want to use multiple origins for doing client side map vis culling
	// NOTE:  In generaly, you won't want to do this, and by default the 3d origin of the camera, as above,
	//  will be used as the origin for vis, too.
	bool			m_bOverrideVisOrigin; 
	// Number of origins to use from m_rgVisOrigins
	int				m_nNumVisOrigins;
	// Array of origins
	Vector			m_rgVisOrigins[ MAX_VIS_LEAVES ];

	Frustum			m_Frustum;

	// Pitch drifting data
	CPitchDrift		m_PitchDrift;

	// For tracking angles history.
	QAngle			m_AnglesHistory[ANGLESHISTORY_SIZE];
	int				m_AnglesHistoryCounter;

	// The frame number
	int				m_FrameNumber;
	int				m_BuildWorldListsNumber;

	// Some cvars needed by this system
	const ConVar	*m_pDrawEntities;
	const ConVar	*m_pDrawBrushModels;

	// Some materials used...
	CMaterialReference	m_TranslucentSingleColor;
	CMaterialReference	m_ModulateSingleColor;
	CMaterialReference	m_ScreenOverlayMaterial;

	// The render target coming into ViewDrawScene so that we can restore in as appropriate
	ITexture		*m_pViewDrawSceneRenderTarget;

	CMaterialReference m_ScreenSpaceEffectMaterial;

	Vector			m_vecLastFacing;
	float			m_flCheapWaterStartDistance;
	float			m_flCheapWaterEndDistance;

	float				m_OverviewSettings[4];	// x,x,sacle,height
};

#endif // VIEWRENDER_H
