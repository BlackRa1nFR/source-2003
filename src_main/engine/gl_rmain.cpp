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

#include "glquake.h"
#include "gl_model_private.h"
#include "r_local.h"
#include "gl_cvars.h"
#include "sound.h"
#include "debug_leafvis.h"
#include "cdll_int.h"
#include "EngineStats.h"
#include "gl_matsysiface.h"
#include "gl_rsurf.h"
#include "gl_lightmap.h"
#include "materialsystem/IMesh.h"
#include "ivrenderview.h"
#include "studio.h"
#include "l_studio.h"
#include "r_areaportal.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "gl_rmain.h"
#include "cdll_engine_int.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "istudiorender.h"
#include "staticpropmgr.h"
#include "tier0/vprof.h"
#include "IOcclusionSystem.h"



extern colorVec R_LightPoint (Vector& p);

CEngineStats g_EngineStats;

extern "C"
{

// For VectorTransform in math.s
float asmvpn[3];
float asmvright[3];
float asmvup[3];

}

//
// view origin
//
extern Vector vup, vpn, vright, r_origin;

int	d_lightstyleframe[256];

void ProjectPointOnPlane( Vector& dst, const Vector& p, const Vector& normal )
{
	float d;
	Vector n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( Vector& dst, const Vector& src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	Vector tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

/*
====================
CalcFov
====================
*/
float CalcFov( float fov_x, float width, float height )
{
	if (fov_x < 1 || fov_x > 179)
		fov_x = 90;	// error, set to 90

	float x = width / tan( DEG2RAD(fov_x) * 0.5);

	float half_fov_y = atan( height / x );

	return RAD2DEG(half_fov_y) * 2;
}

void R_DrawScreenRect( float left, float top, float right, float bottom )
{
	float mat[16], proj[16];
	materialSystemInterface->GetMatrix( MATERIAL_VIEW, mat );
	materialSystemInterface->MatrixMode( MATERIAL_VIEW );
	materialSystemInterface->LoadIdentity();
	
	materialSystemInterface->GetMatrix( MATERIAL_PROJECTION, proj );
	materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );
	materialSystemInterface->LoadIdentity();
	
	
	IMaterial *pMaterial = materialSystemInterface->FindMaterial( "debug\\debugportals", NULL );
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true, NULL, NULL, pMaterial );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_LINE_LOOP, 4 );

		Vector v1( left, bottom, 0.5 );
		Vector v2( left, top, 0.5 );
		Vector v3( right, top, 0.5 );
		Vector v4( right, bottom, 0.5 );

		builder.Position3fv( v1.Base() ); 		builder.AdvanceVertex();  
		builder.Position3fv( v2.Base() ); 		builder.AdvanceVertex();  
		builder.Position3fv( v3.Base() ); 		builder.AdvanceVertex();  
		builder.Position3fv( v4.Base() ); 		builder.AdvanceVertex();  

	builder.End( false, true );

	materialSystemInterface->MatrixMode( MATERIAL_VIEW );
	materialSystemInterface->LoadIdentity();
	materialSystemInterface->MultMatrix( mat );

	materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );
	materialSystemInterface->LoadIdentity();
	materialSystemInterface->MultMatrix( proj );
}


void R_DrawPortals()
{
	// Draw the portals.
	if( !r_DrawPortals.GetInt() )
		return;

	IMaterial *pMaterial = materialSystemInterface->FindMaterial( "debug\\debugportals", NULL );
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true, NULL, NULL, pMaterial );

	brushdata_t *pBrush = &host_state.worldmodel->brush;
	for( int i=0; i < pBrush->m_nAreaPortals; i++ )
	{
		dareaportal_t *pAreaPortal = &pBrush->m_pAreaPortals[i];

		if( !R_IsAreaVisible( pAreaPortal->otherarea ) )
			continue;

		CMeshBuilder builder;
		builder.Begin( pMesh, MATERIAL_LINES, pAreaPortal->m_nClipPortalVerts );

		for( int j=0; j < pAreaPortal->m_nClipPortalVerts; j++ )
		{
			unsigned short iVert;

			iVert = pAreaPortal->m_FirstClipPortalVert + j;
			builder.Position3f( VectorExpand( pBrush->m_pClipPortalVerts[iVert] ) );
			builder.Color4f( 0, 0, 0, 1 );
			builder.AdvanceVertex();

			iVert = pAreaPortal->m_FirstClipPortalVert + (j+1) % pAreaPortal->m_nClipPortalVerts;
			builder.Position3f( VectorExpand( pBrush->m_pClipPortalVerts[iVert] ) );
			builder.Color4f( 0, 0, 0, 1 );
			builder.AdvanceVertex();
		}

		builder.End( false, true );
	}

	// Draw the clip rectangles.
	for( i=0; i < g_PortalRects.Size(); i++ )
	{
		CPortalRect *pRect = &g_PortalRects[i];
		R_DrawScreenRect( pRect->left, pRect->top, pRect->right, pRect->bottom );
	}
	g_PortalRects.Purge();
}


//==================================================================================

class CRender : public IRender
{
public:
	void FrameBegin( void );
	void FrameEnd( void );

	void ViewSetupVis( bool novis, int numorigins, const Vector origin[] );

	void ViewSetup3D( const CViewSetup *pView, Frustum frustumPlanes );
	void ViewEnd( void );

	void ViewDrawFade( byte *color, IMaterial* pMaterial );

	void BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf );
	void DrawWorldLists( unsigned long flags );

	void DrawSceneBegin( void );
	void DrawSceneEnd( void );

	// utility functions
	void SetProjectionMatrix( float fov, float zNear, float zFar, bool forceAspectRatio1To1 );
	void SetProjectionMatrixOrtho(float left, float top, float right, float bottom, float zNear, float zFar);

	void SetViewMatrix( const Vector& origin, const QAngle& angles );
	void ExtractMatrices( void );
	void ExtractFrustumPlanes( Frustum frustumPlanes );
	void OrthoExtractFrustumPlanes( Frustum frustumPlanes );

	void SetViewport( int x, int y, int w, int h );
	// UNDONE: these are temporary functions that will end up on the other
	// side of this interface
	const Vector& UnreflectedViewOrigin( ) { return m_view.m_vUnreflectedOrigin; }
	const Vector &ViewOrigin( ) { return m_view.origin; }
	const QAngle &ViewAngles( ) { return m_view.angles; }
	CViewSetup const &ViewGetCurrent( void ) { return m_view; }
	const VMatrix &ViewMatrix( ) { return m_matrixView; }
	const VMatrix &WorldToScreenMatrix( void ) { return m_matrixWorldToScreen; }

	float	GetFramerate( void ) { return m_framerate; }
	virtual float	GetZNear( void ) { return m_zNear; }
	virtual float	GetZFar( void ) { return m_zFar; }

	// Query current fov and view model fov
	float	GetFov( void ) { return m_view.fov; };
	float	GetFovY( void ) { return m_yFOV; };
	float	GetFovViewmodel( void ) { return m_view.fovViewmodel; };

	// Compute the number of pixels in screen space wide a particular sphere is 
	float	GetScreenSize( const Vector& origin, float radius );

	virtual bool	ClipTransform( Vector const& point, Vector* pClip );
	virtual bool	ScreenTransform( Vector const& point, Vector* pScreen );

private:
	CViewSetup		m_view;

	// Y field of view, calculated from X FOV and screen aspect ratio.
	float			m_yFOV;

	// timing
	double		m_frameStartTime;
	float		m_framerate;

	float		m_zNear;
	float		m_zFar;
	
	// matrices
	VMatrix		m_matrixView;
	VMatrix		m_matrixProjection;
	VMatrix		m_matrixWorldToScreen;
};


void CRender::FrameBegin( void )
{
	if ( !host_state.worldmodel )
		return;

// don't allow cheats in multiplayer
#if !defined( _DEBUG )
	if (cl.maxclients > 1)
	{
//		mat_fullbright.SetValue( 0 );
//		mat_drawflat.SetValue( 0 );
		mat_reversedepth.SetValue( 0 );
		mat_luxels.SetValue( 0 );
		mat_normals.SetValue( 0 );
	}
#endif

	// This has to be before R_AnimateLight because it uses it to
	// set the frame number of changed lightstyles
	r_framecount++;
	R_AnimateLight ();
	R_PushDlights();

	if (!r_norefresh.GetInt())
	{
		m_frameStartTime = Sys_FloatTime ();
	}
	UpdateStudioRenderConfig();
	materialSystemInterface->BeginFrame();
	g_pStudioRender->BeginFrame();
}

void CRender::FrameEnd( void )
{
	m_framerate = cl.getframetime();
	if ( m_framerate > 0 )
	{
		m_framerate = 1 / m_framerate;
	}
	g_pStudioRender->EndFrame();
	materialSystemInterface->EndFrame();
}

void CRender::ViewSetupVis( bool novis, int numorigins, const Vector origin[] )
{
	Map_VisSetup( host_state.worldmodel, numorigins, origin, novis );
}

static float g_ViewportScale = 1.0f;
static float g_ViewportOOScale = 1.0f;
static void ScaleChangeCallback_f( ConVar *var, char const *pOldString );

static ConVar mat_viewportscale( "mat_viewportscale", "1.0", 0, "debugging viewport scale", false, 0.0f, true, 1.0f, ScaleChangeCallback_f );

static int g_nFrameBuffersToClear = 0;

static void ScaleChangeCallback_f( ConVar *var, char const *pOldString )
{
	g_ViewportScale = mat_viewportscale.GetFloat();
	g_ViewportOOScale = 1.0f / g_ViewportScale;
	g_nFrameBuffersToClear = 3;
}

void CRender::ViewSetup3D( const CViewSetup *pView, Frustum frustumPlanes )
{
	VPROF("CRender::ViewSetup3D");
	m_view = *pView;
	m_yFOV = CalcFov( m_view.fov, ( float )m_view.width, ( float )m_view.height );

	if( g_nFrameBuffersToClear > 0 )
	{
		SetViewport( m_view.x, m_view.y, m_view.width, m_view.height );
		materialSystemInterface->ClearBuffers( true, m_view.clearDepth );
		g_nFrameBuffersToClear--;
	}

	SetViewport( m_view.x, m_view.y + m_view.height * ( 1.0f - g_ViewportScale ), 
		m_view.width * g_ViewportScale, m_view.height * g_ViewportScale );
	if( m_view.clearColor || m_view.clearDepth )
	{
		materialSystemInterface->ClearBuffers( m_view.clearColor, m_view.clearDepth );
	}
	materialSystemInterface->DepthRange( 0, 1 );

	// build the transformation matrix for the given view angles
	VectorCopy( m_view.origin, r_origin );

	AngleVectors( m_view.angles, &vpn, &vright, &vup );

//	vup = -vup;

	// Copy for VectorTransform
	VectorCopy( &vpn.x, asmvpn );
	VectorCopy( &vright.x, asmvright );
	VectorCopy( &vup.x, asmvup );

	if ( pView->m_bOrtho )
	{
		SetProjectionMatrixOrtho(pView->m_OrthoLeft, pView->m_OrthoTop, pView->m_OrthoRight, pView->m_OrthoBottom, pView->zNear, pView->zFar);
	}
	else
	{
		SetProjectionMatrix( m_view.fov, m_view.zNear, m_view.zFar, m_view.m_bForceAspectRatio1To1 );
	}

	SetViewMatrix( m_view.origin, m_view.angles );

	ExtractMatrices();

	if ( pView->m_bOrtho )
	{
		OrthoExtractFrustumPlanes( frustumPlanes );
	}
	else
	{
		ExtractFrustumPlanes(frustumPlanes);
	}

	OcclusionSystem()->SetView( m_view.origin, m_view.fov, m_matrixView, m_matrixProjection, frustumPlanes[ FRUSTUM_NEARZ ] );

	R_SceneBegin();

	// debug, build leaf volume
	LeafVisBuild( Vector(r_origin) );
}


//-----------------------------------------------------------------------------
// Compute the number of pixels in screen space wide a particular sphere is 
//-----------------------------------------------------------------------------
float CRender::GetScreenSize( const Vector& vecOrigin, float flRadius )
{
	// This is sort of faked, but it's faster that way
	// FIXME: Also, there's a much faster way to do this with similar triangles
	// but I want to make sure it exactly matches the current matrices, so
	// for now, I do it this conservative way
	Vector4D testPoint1, testPoint2;
	VectorMA( vecOrigin, flRadius, vup, testPoint1.AsVector3D() );
	VectorMA( vecOrigin, -flRadius, vup, testPoint2.AsVector3D() );
	testPoint1.w = testPoint2.w = 1.0f;

	Vector4D clipPos1, clipPos2;
	Vector4DMultiply( m_matrixWorldToScreen, testPoint1, clipPos1 );
	Vector4DMultiply( m_matrixWorldToScreen, testPoint2, clipPos2 );
	if (clipPos1.w >= 0.001f)
		clipPos1.y /= clipPos1.w;
	else
		clipPos1.y *= 1000;
	if (clipPos2.w >= 0.001f)
		clipPos2.y /= clipPos2.w;
	else
		clipPos2.y *= 1000;
	return m_view.height * fabs( clipPos2.y - clipPos1.y );
}

//-----------------------------------------------------------------------------
// Compute the scene coordinates of a point in 3D
//-----------------------------------------------------------------------------
bool CRender::ClipTransform( Vector const& point, Vector* pClip )
{
// UNDONE: Clean this up some, handle off-screen vertices
	float w;
	const VMatrix &worldToScreen = g_EngineRenderer->WorldToScreenMatrix();

	pClip->x = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	pClip->y = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
//	z		 = worldToScreen[2][0] * point[0] + worldToScreen[2][1] * point[1] + worldToScreen[2][2] * point[2] + worldToScreen[2][3];
	w		 = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];

	// Just so we have something valid here
	pClip->z = 0.0f;

	bool behind;
	if( w < 0.001f )
	{
		behind = true;
		pClip->x *= 100000;
		pClip->y *= 100000;
	}
	else
	{
		behind = false;
		float invw = 1.0f / w;
		pClip->x *= invw;
		pClip->y *= invw;
	}

	return behind;
}

//-----------------------------------------------------------------------------
// Purpose: Given a point, return the screen position in pixels
//-----------------------------------------------------------------------------
bool CRender::ScreenTransform( Vector const& point, Vector* pScreen )
{
	bool retval = ClipTransform( point, pScreen );

	pScreen->x = 0.5f * ( pScreen->x + 1.0f ) * m_view.width + m_view.x;
	pScreen->y = 0.5f * ( pScreen->y + 1.0f ) * m_view.height + m_view.y;

	return retval;
}

void CRender::ViewDrawFade( byte *color, IMaterial* pFadeMaterial )
{		
	if ( !color || !color[3] )
		return;

	if( !pFadeMaterial )
		return;

	materialSystemInterface->Bind( pFadeMaterial );
	pFadeMaterial->AlphaModulate( color[3] * ( 1.0f / 255.0f ) );
	pFadeMaterial->ColorModulate( color[0] * ( 1.0f / 255.0f ),
		color[1] * ( 1.0f / 255.0f ),
		color[2] * ( 1.0f / 255.0f ) );
	pFadeMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, true );

	int nTexWidth, nTexHeight;
	nTexWidth = pFadeMaterial->GetMappingWidth();
	nTexHeight = pFadeMaterial->GetMappingHeight();
	float flUOffset = 0.5f / nTexWidth;
	float flVOffset = 0.5f / nTexHeight;

	int width, height;
	materialSystemInterface->GetRenderTargetDimensions( width, height );
	materialSystemInterface->Viewport( 0, 0, width, height );

	materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );

	materialSystemInterface->PushMatrix();
	materialSystemInterface->LoadIdentity();

	materialSystemInterface->Scale( 1, -1, 1 );
	materialSystemInterface->Ortho( 0, 0, width, height, -99999, 99999 );

	materialSystemInterface->MatrixMode( MATERIAL_MODEL );
	materialSystemInterface->PushMatrix();
	materialSystemInterface->LoadIdentity();	

	materialSystemInterface->MatrixMode( MATERIAL_VIEW );
	materialSystemInterface->PushMatrix();
	materialSystemInterface->LoadIdentity();	

	IMesh* pMesh = materialSystemInterface->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( m_view.x, m_view.y, 0.0f );
	meshBuilder.TexCoord2f( 0, flUOffset, flVOffset );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_view.x + m_view.width, m_view.y, 0.0f );
	meshBuilder.TexCoord2f( 0, 1.0f - flUOffset, flVOffset );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_view.x + m_view.width, m_view.y + m_view.height, 0.0f );
	meshBuilder.TexCoord2f( 0, 1.0f - flUOffset, 1.0f - flVOffset );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_view.x, m_view.y + m_view.height, 0.0f );
	meshBuilder.TexCoord2f( 0, flUOffset, 1.0f - flVOffset );
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();

	materialSystemInterface->MatrixMode( MATERIAL_MODEL );
    materialSystemInterface->PopMatrix();
	materialSystemInterface->MatrixMode( MATERIAL_VIEW );
    materialSystemInterface->PopMatrix();
	materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );
    materialSystemInterface->PopMatrix();
}

void CRender::ExtractFrustumPlanes( Frustum frustumPlanes )
{
	int		i;

	float orgOffset = DotProduct(r_origin, vpn);
	// Setup the near and far planes.
	frustumPlanes[FRUSTUM_FARZ].m_Normal = -vpn;
	frustumPlanes[FRUSTUM_FARZ].m_Dist = -m_view.zFar - orgOffset;

	frustumPlanes[FRUSTUM_NEARZ].m_Normal = vpn;
	frustumPlanes[FRUSTUM_NEARZ].m_Dist = m_view.zNear + orgOffset;

	float fovx, fovy;

	fovx = m_view.fov;
	fovy = m_yFOV;

	fovx *= 0.5f;
	fovy *= 0.5f;

	float tanx = tan(DEG2RAD(fovx));
	float tany = tan(DEG2RAD(fovy));

	// OPTIMIZE: Normalizing these planes is not necessary for culling
	frustumPlanes[FRUSTUM_LEFT].m_Normal = (tanx * vpn) + vright;
	VectorNormalize(frustumPlanes[FRUSTUM_LEFT].m_Normal);
	
	frustumPlanes[FRUSTUM_RIGHT].m_Normal = (tanx * vpn) - vright;
	VectorNormalize(frustumPlanes[FRUSTUM_RIGHT].m_Normal);
	
	frustumPlanes[FRUSTUM_BOTTOM].m_Normal = (tany * vpn) + vup;
	VectorNormalize(frustumPlanes[FRUSTUM_BOTTOM].m_Normal);

	frustumPlanes[FRUSTUM_TOP].m_Normal = (tany * vpn) - vup;
	VectorNormalize(frustumPlanes[FRUSTUM_TOP].m_Normal);

	for (i=0; i < 4; i++)
	{
		frustumPlanes[i].m_Dist = frustumPlanes[i].m_Normal.Dot(r_origin);
	}

	// Copy out to the planes that the engine renderer uses.
	for(i=0; i < FRUSTUM_NUMPLANES; i++)
	{
		g_Frustum.SetPlane( i, PLANE_ANYZ, frustumPlanes[i].m_Normal, frustumPlanes[i].m_Dist );
	}
}

void CRender::OrthoExtractFrustumPlanes( Frustum frustumPlanes )
{
	// Setup the near and far planes.
	float orgOffset = DotProduct(r_origin, vpn);
	frustumPlanes[FRUSTUM_FARZ].m_Normal = -vpn;
	frustumPlanes[FRUSTUM_FARZ].m_Dist = -m_view.zFar - orgOffset;

	frustumPlanes[FRUSTUM_NEARZ].m_Normal = vpn;
	frustumPlanes[FRUSTUM_NEARZ].m_Dist = m_view.zNear + orgOffset;

	// Left and right planes...
	orgOffset = DotProduct(r_origin, vright);
	frustumPlanes[FRUSTUM_LEFT].m_Normal = vright;
	frustumPlanes[FRUSTUM_LEFT].m_Dist = m_view.m_OrthoLeft + orgOffset;

	frustumPlanes[FRUSTUM_RIGHT].m_Normal = -vright;
	frustumPlanes[FRUSTUM_RIGHT].m_Dist = -m_view.m_OrthoRight - orgOffset;

	// Top and buttom planes...
	orgOffset = DotProduct(r_origin, vup);
	frustumPlanes[FRUSTUM_TOP].m_Normal = vup;
	frustumPlanes[FRUSTUM_TOP].m_Dist = m_view.m_OrthoTop + orgOffset;

	frustumPlanes[FRUSTUM_BOTTOM].m_Normal = -vup;
	frustumPlanes[FRUSTUM_BOTTOM].m_Dist = -m_view.m_OrthoBottom - orgOffset;

	// Copy out to the planes that the engine renderer uses.
	for(int i=0; i < FRUSTUM_NUMPLANES; i++)
	{
		/*
		if (fabs(frustumPlanes[i].m_Normal.x) - 1.0f > -1e-3)
			frustum[i].type = PLANE_X;
		else if (fabs(frustumPlanes[i].m_Normal.y) - 1.0f > -1e-3)
			frustum[i].type = PLANE_Y;
		else if (fabs(frustumPlanes[i].m_Normal.z) - 1.0f > -1e-3)
			frustum[i].type = PLANE_Z;
		else
		*/
		g_Frustum.SetPlane( i, PLANE_ANYZ, frustumPlanes[i].m_Normal, frustumPlanes[i].m_Dist );
	}
}

void CRender::ExtractMatrices( void )
{
	materialSystemInterface->GetMatrix( MATERIAL_PROJECTION, &m_matrixProjection );
	materialSystemInterface->GetMatrix( MATERIAL_VIEW, &m_matrixView );
	MatrixMultiply( m_matrixProjection, m_matrixView, m_matrixWorldToScreen );
}

void CRender::SetViewMatrix( const Vector& origin, const QAngle& angles )
{
	materialSystemInterface->MatrixMode( MATERIAL_VIEW );
	materialSystemInterface->LoadIdentity();

	// convert from a right handed system to a left handed system
	// since dx for wants it that way.  
//	materialSystemInterface->Scale( 1.0f, 1.0f, -1.0f );

	materialSystemInterface->Rotate( -90,  1, 0, 0 );	    // put Z going up
	materialSystemInterface->Rotate( 90,  0, 0, 1 );	    // put Z going up
	materialSystemInterface->Rotate( -angles[2],  1, 0, 0 );
	materialSystemInterface->Rotate( -angles[0],  0, 1, 0 );
	materialSystemInterface->Rotate( -angles[1],  0, 0, 1 );
	materialSystemInterface->Translate( -origin[0],  -origin[1],  -origin[2] );
}

void CRender::SetViewport( int x, int y, int w, int h )
{
	int		x2, y2;

	int  windowWidth, windowHeight;

	materialSystemInterface->GetRenderTargetDimensions( windowWidth, windowHeight );

	x2 = (x + w);
	y2 = (windowHeight - (y + h));
	y = (windowHeight - y);

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < windowWidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < windowHeight)
		y++;

	w = x2 - x;
	h = y - y2;

	materialSystemInterface->Viewport( x, y2, w, h );
}

ConVar r_anamorphic( "r_anamorphic", "0", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : viewmodel - true if we are rendering the view model
//			fov - field of view
//-----------------------------------------------------------------------------
void CRender::SetProjectionMatrix( float fov, float zNear, float zFar, bool forceAspectRatio1To1 )
{
	float	screenaspect;

	m_zNear = zNear;
	m_zFar = zFar;	// cache this for queries
	//
	// set up viewpoint
	//
	materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );
	materialSystemInterface->LoadIdentity();

	if( forceAspectRatio1To1 )
	{
		screenaspect = 1.0f;
	}
	else
	{
		screenaspect = ( float )m_view.width / ( float )m_view.height;
#if 0
		if( screenaspect == ( 4.0f / 3.0f ) )
		{
			// This is a square pixel, so leave it alone.
		} 
		else if( screenaspect == ( 5.0f / 4.0f ) )
		{
			// Doh! Non-square pixel
			screenaspect = ( 4.0f / 3.0f );
		}
		else if( screenaspect == ( 16.0f / 9.0f ) )
		{
			// This is a square pixel, so leave it alone.
		}
		else
		{
			Assert( 0 );
		}
#endif
		// garymcthack!!!  This is bogus. . . need to find a way to figure out the physical
		// aspect ratio of the screen pixel.
		switch( r_anamorphic.GetInt() )
		{
		case 1:
			screenaspect = ( 16.0f / 9.0f );
			break;
		case 2:
			screenaspect = ( 16.0f / 10.0f );
			break;
		default:
			screenaspect = ( 4.0f / 3.0f );
			break;
		}
	}
	materialSystemInterface->PerspectiveX( fov,  screenaspect, zNear, zFar );
}

void CRender::SetProjectionMatrixOrtho(float left, float top, float right, float bottom, float zNear, float zFar)
{
	m_zNear = zNear;
	m_zFar = zFar;	// cache this for queries
	materialSystemInterface->MatrixMode( MATERIAL_PROJECTION );
	materialSystemInterface->LoadIdentity();
	materialSystemInterface->Ortho(left, top, right, bottom, zNear, zFar);
}

// hack
extern float g_lightmapScale;
	
// r_lightmapcolorscale is the goal color scale
bool UpdateLightmapColorScale( void )
{
	static float prevLightmapColorScale = 1.0f;
	static float prevlightmapScale = 0.0f;
	float lightmapScale;
	bool rebuild = false;
	
	//return false; // hack
	
	if( prevLightmapColorScale != r_lightmapcolorscale.GetFloat() )
	{
		// transition in 1 second
		if( prevLightmapColorScale < r_lightmapcolorscale.GetFloat() )
		{
			prevLightmapColorScale += 0.25 * host_frametime;
			if( prevLightmapColorScale > r_lightmapcolorscale.GetFloat() )
			{
				prevLightmapColorScale = r_lightmapcolorscale.GetFloat();
			}
		}
		else
		{
			prevLightmapColorScale -= 0.25 * host_frametime;
			if( prevLightmapColorScale < r_lightmapcolorscale.GetFloat() )
			{
				prevLightmapColorScale = r_lightmapcolorscale.GetFloat();
			}
		}

		// figure out what power of two we should be at.
		// garymct hack. . this is amazingly stupid

		if( prevLightmapColorScale <= 1.0f )
		{
			lightmapScale = 1.0f;
		}
		else if( prevLightmapColorScale <= 2.0f )
		{
			lightmapScale = 2.0f;
		}
		else if( prevLightmapColorScale <= 4.0f )
		{
			lightmapScale = 4.0f;
		}
		else if( prevLightmapColorScale <= 8.0f )
		{
			lightmapScale = 8.0f;
		}
		else if( prevLightmapColorScale <= 16.0f )
		{
			lightmapScale = 16.0f;
		}
		else
		{
			assert(false);
			lightmapScale = 1;
		}

		if( lightmapScale != prevlightmapScale )
		{
			prevlightmapScale = lightmapScale;
			rebuild = true;
		}

		g_lightmapScale = lightmapScale;

		g_materialSystemConfig.lightScale = prevLightmapColorScale / lightmapScale;
		Con_Printf( "prevLightmapColorScale: %f\n", prevLightmapColorScale );
	}

	return rebuild;
}

void DrawLightmapPage( int lightmapPageID )
{
	// assumes that we are already in ortho mode.
	int lightmapPageWidth, lightmapPageHeight;

	IMesh* pMesh = materialSystemInterface->GetDynamicMesh( true, NULL, NULL, g_materialDebugLightmap );
//	materialSystemInterface->Bind( g_materialWireframe );
//	IMesh* pMesh = materialSystemInterface->GetDynamicMesh( g_materialWireframe );

	materialSystemInterface->GetLightmapPageSize( lightmapPageID, &lightmapPageWidth, &lightmapPageHeight );
	materialSystemInterface->BindLightmapPage( lightmapPageID );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	// texcoord 1 is lightmaptexcoord for fixed function.
	meshBuilder.TexCoord2f( 1, 0.0f, 0.0f );
	meshBuilder.Position3f( 0.0f, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 1, 1.0f, 0.0f );
	meshBuilder.Position3f( lightmapPageWidth, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 1, 1.0f, 1.0f );
	meshBuilder.Position3f( lightmapPageWidth, lightmapPageHeight, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 1, 0.0f, 1.0f );
	meshBuilder.Position3f( 0.0f, lightmapPageHeight, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//hack
extern void DebugDrawLightmapAtCrossHair();

void R_DrawLightmaps( void )
{
#ifdef USE_CONVARS
	if ( mat_showlightmappage.GetInt() != -1 )
	{
		DrawLightmapPage( mat_showlightmappage.GetInt() );
		Shader_DrawLightmapPageChains();
	}
#endif
}

void CRender::DrawSceneBegin( void )
{
	UpdateStudioRenderConfig();
	UpdateMaterialSystemConfig();
	if( MaterialConfigLightingChanged() || UpdateLightmapColorScale() )
	{
		ClearMaterialConfigLightingChanged();
		Con_Printf( "Redownloading all lightmaps\n" );
		float overbright = 1.0f;
		if (g_pMaterialSystemHardwareConfig->SupportsOverbright())
		{
			overbright = mat_overbright.GetFloat();
		}
		BuildGammaTable( 2.2f, 2.2f, 0.0f, overbright );
		R_RedownloadAllLightmaps( vec3_origin );
		materialSystemInterface->FlushLightmaps();
		StaticPropMgr()->RecomputeStaticLighting();
	}
}

void CRender::DrawSceneEnd( void )
{
	R_SceneEnd();
	LeafVisDraw();
}

void CRender::BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf )
{
	R_BuildWorldLists( pInfo, updateLightmaps, iForceViewLeaf );
}

void CRender::DrawWorldLists( unsigned long flags )
{
	R_DrawWorldLists( flags );
}

static CRender gRender;

IRender *g_EngineRenderer = &gRender;


