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
#if !defined( IVRENDERVIEW_H )
#define IVRENDERVIEW_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "vplane.h"
#include "interface.h"
#include "materialsystem/imaterialsystem.h"
//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CViewSetup;
class CEngineSprite;
class IClientEntity;
class IMaterial;
struct model_t;
class IClientRenderable;

//-----------------------------------------------------------------------------
// Frustum plane indices.
// WARNING: there is code that depends on these values
//-----------------------------------------------------------------------------

enum
{
	FRUSTUM_RIGHT		= 0,
	FRUSTUM_LEFT		= 1,
	FRUSTUM_TOP			= 2,
	FRUSTUM_BOTTOM		= 3,
	FRUSTUM_NEARZ		= 4,
	FRUSTUM_FARZ		= 5,
	FRUSTUM_NUMPLANES	= 6
};

//-----------------------------------------------------------------------------
// Flags used by DrawWorldLists
//-----------------------------------------------------------------------------

enum
{
	DRAWWORLDLISTS_DRAW_STRICTLYABOVEWATER		= 0x1,
	DRAWWORLDLISTS_DRAW_STRICTLYUNDERWATER		= 0x2,
	DRAWWORLDLISTS_DRAW_INTERSECTSWATER			= 0x4,
	DRAWWORLDLISTS_DRAW_WATERSURFACE			= 0x8,
	DRAWWORLDLISTS_DRAW_SKYBOX					= 0x10,
};

enum
{
	MAT_SORT_GROUP_STRICTLY_ABOVEWATER = 0,
	MAT_SORT_GROUP_STRICTLY_UNDERWATER,
	MAT_SORT_GROUP_INTERSECTS_WATER_SURFACE,
	MAT_SORT_GROUP_WATERSURFACE,

	MAX_MAT_SORT_GROUPS
};

typedef VPlane Frustum[FRUSTUM_NUMPLANES];

struct WorldListInfo_t
{
	int		m_ViewFogVolume;
	int		m_LeafCount;
	int*	m_pLeafList;
	int*	m_pLeafFogVolume;
};


//-----------------------------------------------------------------------------
// Vertex format for brush models
//-----------------------------------------------------------------------------

struct BrushVertex_t
{
	Vector		m_Pos;
	Vector		m_Normal;
	Vector		m_TangentS;
	Vector		m_TangentT;
	Vector2D	m_TexCoord;
	Vector2D	m_LightmapCoord;
};


//-----------------------------------------------------------------------------
// interface for asking about the Brush surfaces from the client DLL
//-----------------------------------------------------------------------------

class IBrushSurface
{
public:
	// Computes texture coordinates + lightmap coordinates given a world position
	virtual void ComputeTextureCoordinate( Vector const& worldPos, Vector2D& texCoord ) = 0;
	virtual void ComputeLightmapCoordinate( Vector const& worldPos, Vector2D& lightmapCoord ) = 0;

	// Gets the vertex data for this surface
	virtual int  GetVertexCount() const = 0;
	virtual void GetVertexData( BrushVertex_t* pVerts ) = 0;

	// Gets at the material properties for this surface
	virtual IMaterial* GetMaterial() = 0;
};


//-----------------------------------------------------------------------------
// interface for installing a new renderer for brush surfaces
//-----------------------------------------------------------------------------

class IBrushRenderer
{
public:
	// Draws the surface; returns true if decals should be rendered on this surface
	virtual bool RenderBrushModelSurface( IClientEntity* pBaseEntity, IBrushSurface* pBrushSurface ) = 0;
};


#define MAX_VIS_LEAVES	32
//-----------------------------------------------------------------------------
// Purpose: Interface to client .dll to set up a rendering pass over world
//  The client .dll can call Render multiple times to overlay one or more world
//  views on top of one another
//-----------------------------------------------------------------------------
class IVRenderView
{
public:
	// Draw normal brush model.
	// If pMaterialOverride is non-null, then all the faces of the bmodel will
	// set this material rather than their regular material.
	virtual void			DrawBrushModel( 
		IClientEntity *baseentity, 
		model_t *model, 
		const Vector& origin, 
		const QAngle& angles, 
		bool sort ) = 0;
	
	// Draw brush model that has no origin/angles change ( uses identity transform )
	// FIXME, Material proxy IClientEntity *baseentity is unused right now, use DrawBrushModel for brushes with
	//  proxies for now.
	virtual void			DrawIdentityBrushModel( model_t *model ) = 0;

	// Mark this dynamic light as having changed this frame ( so light maps affected will be recomputed )
	virtual void			TouchLight( struct dlight_s *light ) = 0;
	// Draw 3D Overlays
	virtual void			Draw3DDebugOverlays( void ) = 0;
	// Wraps MRM drawing
	virtual void			BeginDrawMRM( void ) = 0;
	virtual void			EndDrawMRM( void ) = 0;
	// Sets global blending fraction
	virtual void			SetBlend( float blend ) = 0;
	virtual float			GetBlend( void ) = 0;

	// Sets global color modulation
	virtual void			SetColorModulation( float const* blend ) = 0;
	virtual void			GetColorModulation( float* blend ) = 0;

	// Wrap entire scene drawing
	virtual void			SceneBegin( void ) = 0;
	virtual void			SceneEnd( void ) = 0;

	virtual void			GetVisibleFogVolume( int *visibleFogVolume, int *visibleFogVolumeLeaf, bool *eyeInFogVolume, float *pDistanceToWater, const Vector& eyePoint ) = 0;

	// Wraps world drawing
	// If iForceViewLeaf is not -1, then it uses the specified leaf as your starting area for setting up area portal culling.
	// This is used by water since your reflected view origin is often in solid space, but we still want to treat it as though
	// the first portal we're looking out of is a water portal, so our view effectively originates under the water.
	virtual void			BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf ) = 0;
	virtual void			DrawWorldLists( unsigned long flags ) = 0;

	// Optimization for top view
	virtual void			DrawTopView( bool enable ) = 0;
	virtual void			TopViewBounds( Vector2D const& mins, Vector2D const& maxs ) = 0;

	// Draw lights
	virtual void			DrawLights( void ) = 0;
	// FIXME:  This function is a stub, doesn't do anything in the engine right now
	virtual void			DrawMaskEntities( void ) = 0;
	// Draw surfaces with alpha
	virtual void			DrawTranslucentSurfaces( int sortIndex, unsigned long flags ) = 0;
	// Draw Particles ( just draws the linefine for debugging map leaks )
	virtual void			DrawLineFile( void ) = 0;
	// Draw lightmaps
	virtual void			DrawLightmaps( void ) = 0;
	// Wraps view render sequence, sets up a view
	virtual void			ViewSetupVis( bool novis, int numorigins, const Vector origin[] ) = 0;

	virtual void			ViewSetup3D( const CViewSetup *view, Frustum frustumPlanes ) = 0;
	virtual void			ViewSetup2D( const CViewSetup *view ) = 0;
	virtual	void			VguiPaint( void ) = 0;
	// Sets up view fade parameters
	virtual void			ViewDrawFade( byte *color, IMaterial *pMaterial ) = 0;
	// Sets up the projection matrix for the specified field of view
	virtual void			SetProjectionMatrix( float fov, float zNear, float zFar ) = 0;
	// Determine lighting at specified position
	virtual colorVec		GetLightAtPoint( Vector& pos ) = 0;
	// Determine whether the client is playing back or recording a demo
	virtual bool			IsPlayingDemo( void ) = 0;
	virtual bool			IsRecordingDemo( void ) = 0;
	virtual bool			IsPlayingTimeDemo( void ) = 0;
	// Is the game paused?
	virtual bool			IsPaused( void ) = 0;
	// Whose eyes are we looking through?
	virtual int				GetViewEntity( void ) = 0;
	// Get engine field of view setting
	virtual float			GetFieldOfView( void ) = 0;
	// 1 == ducking, 0 == not
	virtual unsigned char	**GetAreaBits( void ) = 0;
	// Determine the value of cl_interp
	virtual float			GetInterpolationTime( void ) = 0;
	// Set up fog for a particular leaf
	virtual void			SetFogVolumeState( int fogVolume, bool useHeightFog ) = 0;
	virtual IMaterial *		GetFogVolumeMaterial( int fogVolume ) = 0;
	virtual float			GetWaterHeight( int fogVolume ) = 0;

	// Installs a brush surface draw override method, null means use normal renderer
	virtual void			InstallBrushSurfaceRenderer( IBrushRenderer* pBrushRenderer ) = 0;
	// Draw brush model shadow
	virtual void			DrawBrushModelShadow( IClientRenderable *pRenderable ) = 0;

	// Does the leaf contain translucent surfaces?
	virtual	bool			LeafContainsTranslucentSurfaces( int sortIndex, unsigned long flags ) = 0;
};

// change this when the new version is incompatable with the old
#define VENGINE_RENDERVIEW_INTERFACE_VERSION	"VEngineRenderView008"

extern IVRenderView *render;

#endif // IVRENDERVIEW_H
