//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "glquake.h"
#include "client.h"
#include "gl_model_private.h"
#include "gl_rsurf.h"
#include "gl_rmain.h"
#include "debug_leafvis.h"
#include "con_nprint.h"
#include "tier0/fasttimer.h"
#include "r_areaportal.h"


ConVar r_ClipAreaPortals( "r_ClipAreaPortals", "1" );
ConVar r_DrawPortals( "r_DrawPortals", "0" );
extern ConVar r_speeds; // hook into r_speeds.

CUtlVector<CPortalRect> g_PortalRects;
extern bool				g_bViewerInSolidSpace = false;



// ------------------------------------------------------------------------------------ //
// Classes.
// ------------------------------------------------------------------------------------ //

#define MAX_PORTAL_VERTS	32


class CAreaCullInfo
{
public:
	Frustum_t		m_Frustum;
	CPortalRect		m_Rect;
	unsigned short	m_GlobalCounter; // Used to tell if it's been touched yet this frame.
};



// ------------------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------------------ //

// Visible areas from the client DLL + occluded areas using area portals.
unsigned char g_RenderAreaBits[32];

// Used to prevent it from coming back into portals while flowing through them.
static unsigned char g_AreaStack[32];

// Frustums for each area for the current frame. Used to cull out leaves.
static CAreaCullInfo g_AreaCullInfo[MAX_MAP_AREAS];

// List of areas marked visible this frame.
static unsigned short g_VisibleAreas[MAX_MAP_AREAS];
static int g_nVisibleAreas;

// Tied to CAreaCullInfo::m_GlobalCounter.
static unsigned short g_GlobalCounter = 1;


// ------------------------------------------------------------------------------------ //
// Functions.
// ------------------------------------------------------------------------------------ //

static inline void R_SetBit( unsigned char *pBits, int bit )
{
	pBits[bit>>3] |= (1 << (bit&7));
}

static inline void R_ClearBit( unsigned char *pBits, int bit )
{
	pBits[bit>>3] &= ~(1 << (bit&7));
}

static inline unsigned char R_TestBit( unsigned char *pBits, int bit )
{
	return pBits[bit>>3] & (1 << (bit&7));
}


// Transforms and clips the portal's verts to the view frustum. Returns false
// if the verts lie outside the frustum.
static inline bool GetPortalScreenExtents( dareaportal_t *pPortal, 
	Vector *outVerts, int &nOutVerts, CPortalRect &portalRect )
{
	portalRect.left = portalRect.bottom = 0;
	portalRect.right = portalRect.top   = 0;

	brushdata_t *pBrushData = &host_state.worldmodel->brush;
	
	// Setup the initial vert list.
	Vector vTemp[MAX_PORTAL_VERTS];
	Vector *lists[2] = {outVerts, vTemp};
	
	int nStartVerts = min( pPortal->m_nClipPortalVerts, MAX_PORTAL_VERTS );
	int i;
	for( i=0; i < nStartVerts; i++ )
	{
		outVerts[i] = pBrushData->m_pClipPortalVerts[pPortal->m_FirstClipPortalVert+i];
	}

	int iCurList = 0;
	for( int iPlane=0; iPlane < 4; iPlane++ )
	{
		const cplane_t *pPlane = g_Frustum.GetPlane(iPlane);

		Vector *pIn = lists[iCurList];
		Vector *pOut = lists[!iCurList];

		nOutVerts = 0;
		int iPrev = nStartVerts - 1;
		float flPrevDot = pPlane->normal.Dot( pIn[iPrev] ) - pPlane->dist;
		for( int iCur=0; iCur < nStartVerts; iCur++ )
		{
			float flCurDot = pPlane->normal.Dot( pIn[iCur] ) - pPlane->dist;

			if( (flCurDot > 0) != (flPrevDot > 0) )
			{
				if( nOutVerts < MAX_PORTAL_VERTS )
				{
					// Add the vert at the intersection.
					float t = flPrevDot / (flPrevDot - flCurDot);
					VectorLerp( pIn[iPrev], pIn[iCur], t, pOut[nOutVerts] );

					++nOutVerts;
				}
			}

			// Add this vert?
			if( flCurDot > 0 )
			{
				if( nOutVerts < MAX_PORTAL_VERTS )
				{
					pOut[nOutVerts] = pIn[iCur];
					++nOutVerts;
				}
			}

			flPrevDot = flCurDot;
			iPrev = iCur;
		}

		if( nOutVerts == 0 )
		{
			// If they're all behind, then this portal is clipped out.
			return false;
		}

		nStartVerts = nOutVerts;
		iCurList = !iCurList;
	}

	portalRect.left = portalRect.bottom = 1e24;
	portalRect.right = portalRect.top   = -1e24;

	// Project all the verts and figure out the screen extents.
	Vector screenPos;
	Assert( iCurList == 0 );
	for( i=0; i < nStartVerts; i++ )
	{
		Vector &point = outVerts[i];

		g_EngineRenderer->ClipTransform( point, &screenPos );
		point = screenPos;

		portalRect.left   = min( point.x, portalRect.left );
		portalRect.bottom = min( point.y, portalRect.bottom );
		portalRect.top    = max( point.y, portalRect.top );
		portalRect.right  = max( point.x, portalRect.right );
	}

	return true;
}


// Fill in the intersection between the two rectangles.
inline bool GetRectIntersection( CPortalRect const *pRect1, CPortalRect const *pRect2, CPortalRect *pOut )
{
	pOut->left  = max( pRect1->left, pRect2->left );
	pOut->right = min( pRect1->right, pRect2->right );
	if( pOut->left >= pOut->right )
		return false;

	pOut->bottom = max( pRect1->bottom, pRect2->bottom );
	pOut->top    = min( pRect1->top, pRect2->top );
	if( pOut->bottom >= pOut->top )
		return false;

	return true;
}

static void R_FlowThroughArea( int area, CPortalRect const *pClipRect )
{
#ifndef SWDS
	// Update this area's frustum.
	if( g_AreaCullInfo[area].m_GlobalCounter != g_GlobalCounter )
	{
		g_VisibleAreas[g_nVisibleAreas] = area;
		++g_nVisibleAreas;

		g_AreaCullInfo[area].m_GlobalCounter = g_GlobalCounter;
		g_AreaCullInfo[area].m_Rect = *pClipRect;
	}
	else
	{
		// Expand the areaportal's rectangle to include the new cliprect.
		CPortalRect *pFrustumRect = &g_AreaCullInfo[area].m_Rect;
		pFrustumRect->left   = min( pFrustumRect->left, pClipRect->left );
		pFrustumRect->bottom = min( pFrustumRect->bottom, pClipRect->bottom );
		pFrustumRect->top    = max( pFrustumRect->top, pClipRect->top );
		pFrustumRect->right  = max( pFrustumRect->right, pClipRect->right );
	}
	
	// Mark this area as visible.
	R_SetBit( g_RenderAreaBits, area );

	// Set that we're in this area on the stack.
	R_SetBit( g_AreaStack, area );

	brushdata_t *pBrushData = &host_state.worldmodel->brush;

	Assert( area < host_state.worldmodel->brush.m_nAreas );
	darea_t *pArea = &host_state.worldmodel->brush.m_pAreas[area];

	// Check all areas that connect to this area.
	for( int iAreaPortal=0; iAreaPortal < pArea->numareaportals; iAreaPortal++ )
	{
		Assert( pArea->firstareaportal + iAreaPortal < pBrushData->m_nAreaPortals );
		dareaportal_t *pAreaPortal = &pBrushData->m_pAreaPortals[ pArea->firstareaportal + iAreaPortal ];

		// Don't flow back into a portal on the stack.
		if( R_TestBit( g_AreaStack, pAreaPortal->otherarea ) )
			continue;

		// Make sure the viewer is on the right side of the portal to see through it.
		cplane_t *pPlane = &pBrushData->planes[ pAreaPortal->planenum ];
		float flDist = pPlane->normal.Dot( g_EngineRenderer->ViewOrigin() ) - pPlane->dist;
		if( flDist < 0.0f )
			continue;

		// If the client doesn't want this area visible, don't try to flow into it.
		if( !R_TestBit( cl.pAreaBits, pAreaPortal->otherarea ) )
			continue;

		// Find out if we can see any of the portals that lead into this area.
		Vector outVerts[MAX_PORTAL_VERTS];
		int nOutVerts;
		CPortalRect portalRect;
		if( GetPortalScreenExtents( pAreaPortal, outVerts, nOutVerts, portalRect ) )
		{
			CPortalRect intersection;
			if( GetRectIntersection( &portalRect, pClipRect, &intersection ) )
			{
#ifdef USE_CONVARS
				if( r_DrawPortals.GetInt() )
				{
					g_PortalRects.AddToTail( intersection );
				}
#endif

				// Ok, we can see into this area.
				R_FlowThroughArea( pAreaPortal->otherarea, &intersection );
			}
		}
	}
	
	// Mark that we're leaving this area.
	R_ClearBit( g_AreaStack, area );
#endif
}


static void IncrementGlobalCounter()
{
	if( g_GlobalCounter == 0xFFFF )
	{
		for( int i=0; i < MAX_MAP_AREAS; i++ )
			g_AreaCullInfo[i].m_GlobalCounter = 0;
	
		g_GlobalCounter = 1;
	}
	else
	{
		g_GlobalCounter++;
	}
}

ConVar r_snapportal( "r_snapportal", "-1" );
extern void CSGFrustum( Frustum_t &frustum );

static void R_SetupVisibleAreaFrustums()
{
#ifndef SWDS
	CViewSetup const &viewSetup = g_EngineRenderer->ViewGetCurrent();
	
	CPortalRect viewWindow;
	if( viewSetup.m_bOrtho )
	{
		viewWindow.right	= viewSetup.m_OrthoRight;
		viewWindow.left		= viewSetup.m_OrthoLeft;
		viewWindow.top		= viewSetup.m_OrthoTop;
		viewWindow.bottom	= viewSetup.m_OrthoBottom;
	}
	else
	{
		// Assuming a view plane distance of 1, figure out the boundaries of a window
		// the view would project into given the FOV.
		float xFOV = g_EngineRenderer->GetFov() * 0.5f;
		float yFOV = g_EngineRenderer->GetFovY() * 0.5f;

		viewWindow.right	= tan( DEG2RAD( xFOV ) );
		viewWindow.left		= -viewWindow.right;
		viewWindow.top		= tan( DEG2RAD( yFOV ) );
		viewWindow.bottom	= -viewWindow.top;
	}

	// Now scale the portals as specified in the normalized view frustum (-1,-1,1,1)
	// into our view window and generate planes out of that.
	for( int i=0; i < g_nVisibleAreas; i++ )
	{
		CAreaCullInfo *pInfo = &g_AreaCullInfo[ g_VisibleAreas[i] ];

		CPortalRect portalWindow;
		portalWindow.left    = RemapVal( pInfo->m_Rect.left,   -1, 1, viewWindow.left,   viewWindow.right );
		portalWindow.right   = RemapVal( pInfo->m_Rect.right,  -1, 1, viewWindow.left,   viewWindow.right );
		portalWindow.top     = RemapVal( pInfo->m_Rect.top,    -1, 1, viewWindow.bottom, viewWindow.top );
		portalWindow.bottom  = RemapVal( pInfo->m_Rect.bottom, -1, 1, viewWindow.bottom, viewWindow.top );
		
		if( viewSetup.m_bOrtho )
		{
			// Left and right planes...
			float orgOffset = DotProduct(r_origin, vright);
			pInfo->m_Frustum.SetPlane( FRUSTUM_LEFT, PLANE_ANYZ, vright, portalWindow.left + orgOffset );
			pInfo->m_Frustum.SetPlane( FRUSTUM_RIGHT, PLANE_ANYZ, -vright, -portalWindow.right - orgOffset );

			// Top and buttom planes...
			orgOffset = DotProduct(r_origin, vup);
			pInfo->m_Frustum.SetPlane( FRUSTUM_TOP, PLANE_ANYZ, vup, portalWindow.top + orgOffset );
			pInfo->m_Frustum.SetPlane( FRUSTUM_BOTTOM, PLANE_ANYZ, -vup, -portalWindow.bottom - orgOffset );
		}
		else
		{
			Vector normal;
			const cplane_t *pTest;

			// rigth side
			normal = portalWindow.right * vpn - vright;
			VectorNormalize(normal); // OPTIMIZE: This is unnecessary for culling
			pTest = pInfo->m_Frustum.GetPlane( FRUSTUM_RIGHT );
			pInfo->m_Frustum.SetPlane( FRUSTUM_RIGHT, PLANE_ANYZ, normal, DotProduct(normal,r_origin) );

			// left side
			normal = vright - portalWindow.left * vpn;
			VectorNormalize(normal); // OPTIMIZE: This is unnecessary for culling
			pTest = pInfo->m_Frustum.GetPlane( FRUSTUM_LEFT );
			pInfo->m_Frustum.SetPlane( FRUSTUM_LEFT, PLANE_ANYZ, normal, DotProduct(normal,r_origin) );

			// top
			normal = portalWindow.top * vpn - vup;
			VectorNormalize(normal); // OPTIMIZE: This is unnecessary for culling
			pTest = pInfo->m_Frustum.GetPlane( FRUSTUM_TOP );
			pInfo->m_Frustum.SetPlane( FRUSTUM_TOP, PLANE_ANYZ, normal, DotProduct(normal,r_origin) );

			// bottom
			normal = vup - portalWindow.bottom * vpn;
			VectorNormalize(normal); // OPTIMIZE: This is unnecessary for culling
			pTest = pInfo->m_Frustum.GetPlane( FRUSTUM_BOTTOM );
			pInfo->m_Frustum.SetPlane( FRUSTUM_BOTTOM, PLANE_ANYZ, normal, DotProduct(normal,r_origin) );
		}

		// DEBUG: Code to visualize the areaportal frustums in 3D
		// Useful for debugging
		extern void CSGFrustum( Frustum_t &frustum );
		if ( r_snapportal.GetInt() >= 0 )
		{
			if ( g_VisibleAreas[i] == r_snapportal.GetInt() )
			{
				pInfo->m_Frustum.SetPlane( FRUSTUM_NEARZ, PLANE_ANYZ, vpn, DotProduct(vpn, r_origin) );
				pInfo->m_Frustum.SetPlane( FRUSTUM_FARZ, PLANE_ANYZ, -vpn, DotProduct(-vpn, r_origin + vpn*500) );
				r_snapportal.SetValue(-1);
				CSGFrustum( pInfo->m_Frustum );
			}
		}
	}
#endif
}




// Intersection of AABB and a frustum. The frustum may contain 0-32 planes
// (active planes are defined by inClipMask). Returns boolean value indicating
// whether AABB intersects the view frustum or not.
// If AABB intersects the frustum, an output clip mask is returned as well
// (indicating which planes are crossed by the AABB). This information
// can be used to optimize testing of child nodes or objects inside the
// nodes (pass value as 'inClipMask').
inline bool R_CullNodeInternal( mnode_t *pNode, int &nClipMask, const Frustum_t& frustum )
{
	int nOutClipMask = nClipMask & FRUSTUM_CLIP_IN_AREA;	// init outclip mask

	float flCenterDotNormal, flHalfDiagDotAbsNormal;
	if (nClipMask & FRUSTUM_CLIP_RIGHT)
	{
		const cplane_t *pPlane = frustum.GetPlane(FRUSTUM_RIGHT);
		flCenterDotNormal = DotProduct( pNode->m_vecCenter, pPlane->normal ) - pPlane->dist;
		flHalfDiagDotAbsNormal = DotProduct( pNode->m_vecHalfDiagonal, frustum.GetAbsNormal(FRUSTUM_RIGHT) );
		if (flCenterDotNormal + flHalfDiagDotAbsNormal < 0.0f)
			return true;
		if (flCenterDotNormal - flHalfDiagDotAbsNormal < 0.0f)
			nOutClipMask |= FRUSTUM_CLIP_RIGHT;	
	}

	if (nClipMask & FRUSTUM_CLIP_LEFT)
	{
		const cplane_t *pPlane = frustum.GetPlane(FRUSTUM_LEFT);
		flCenterDotNormal = DotProduct( pNode->m_vecCenter, pPlane->normal ) - pPlane->dist;
		flHalfDiagDotAbsNormal = DotProduct( pNode->m_vecHalfDiagonal, frustum.GetAbsNormal(FRUSTUM_LEFT) );
		if (flCenterDotNormal + flHalfDiagDotAbsNormal < 0.0f)
			return true;
		if (flCenterDotNormal - flHalfDiagDotAbsNormal < 0.0f)
			nOutClipMask |= FRUSTUM_CLIP_LEFT;	
	}

	if (nClipMask & FRUSTUM_CLIP_TOP)
	{
		const cplane_t *pPlane = frustum.GetPlane(FRUSTUM_TOP);
		flCenterDotNormal = DotProduct( pNode->m_vecCenter, pPlane->normal ) - pPlane->dist;
		flHalfDiagDotAbsNormal = DotProduct( pNode->m_vecHalfDiagonal, frustum.GetAbsNormal(FRUSTUM_TOP) );
		if (flCenterDotNormal + flHalfDiagDotAbsNormal < 0.0f)
			return true;
		if (flCenterDotNormal - flHalfDiagDotAbsNormal < 0.0f)
			nOutClipMask |= FRUSTUM_CLIP_TOP;	
	}

	if (nClipMask & FRUSTUM_CLIP_BOTTOM)
	{
		const cplane_t *pPlane = frustum.GetPlane(FRUSTUM_BOTTOM);
		flCenterDotNormal = DotProduct( pNode->m_vecCenter, pPlane->normal ) - pPlane->dist;
		flHalfDiagDotAbsNormal = DotProduct( pNode->m_vecHalfDiagonal, frustum.GetAbsNormal(FRUSTUM_BOTTOM) );
		if (flCenterDotNormal + flHalfDiagDotAbsNormal < 0.0f)
			return true;
		if (flCenterDotNormal - flHalfDiagDotAbsNormal < 0.0f)
			nOutClipMask |= FRUSTUM_CLIP_BOTTOM;	
	}

	nClipMask = nOutClipMask;
	return false;						// AABB intersects frustum
}


bool R_CullNode( mnode_t *pNode, int& nClipMask )
{
	// Now cull to this area's frustum.
	Frustum_t *pAreaFrustum = &g_Frustum;

	if (( !g_bViewerInSolidSpace ) && ( pNode->area > 0 ))
	{
		// First make sure its whole area is even visible.
		if( !R_IsAreaVisible( pNode->area ) )
			return true;

		// This ConVar access causes a huge perf hit in some cases.
		// If you want to put it back in please cache it's value.
//#ifdef USE_CONVARS
//		if( r_ClipAreaPortals.GetInt() )
//#else
		if( 1 )
//#endif
		{
			if ((nClipMask & FRUSTUM_CLIP_IN_AREA) == 0)
			{
				// In this case, we've never hit this area before and
				// we need to test all planes again because we just changed the frustum
				nClipMask = FRUSTUM_CLIP_IN_AREA | FRUSTUM_CLIP_ALL;
			}

			pAreaFrustum = &g_AreaCullInfo[pNode->area].m_Frustum;
		}
	}

	return R_CullNodeInternal( pNode, nClipMask, *pAreaFrustum );
}


void R_SetupAreaBits( int iForceViewLeaf )
{
	IncrementGlobalCounter();

	g_bViewerInSolidSpace = false;

#ifdef USE_CONVARS
	if( r_ClipAreaPortals.GetInt() == 2 )
	{
		// If set to 2, just leave the area clipping how it was (for debugging).
	}
	else if( r_ClipAreaPortals.GetInt() == 1 )
#else
	if( 1 )
#endif
	{
		CFastTimer flowTimer;
		flowTimer.Start();

		// Clear the visible area bits.
		memset( g_RenderAreaBits, 0, sizeof( g_RenderAreaBits ) );
		memset( g_AreaStack, 0, sizeof( g_AreaStack ) );

		// Our initial clip rect is the whole screen.
		CPortalRect rect;
		rect.left = rect.bottom = -1;
		rect.top = rect.right = 1;

		// Flow through areas starting at the one we're in.
		int leaf;
		if ( iForceViewLeaf == -1 )
			leaf = ComputeLeaf( g_EngineRenderer->ViewOrigin() );
		else
			leaf = iForceViewLeaf;

		if( host_state.worldmodel->brush.leafs[leaf].contents & CONTENTS_SOLID )
		{
			// Draw everything if we're in solid space.
			g_bViewerInSolidSpace = true;
		}
		else
		{
			int area = host_state.worldmodel->brush.leafs[leaf].area;

			g_nVisibleAreas = 0;		
			R_FlowThroughArea( area, &rect );
			flowTimer.End();

			// Initialize the frustums for all the areas so we can cull leaves and nodes.
			CFastTimer frustumTimer;
			frustumTimer.Start();
				
				R_SetupVisibleAreaFrustums();

			frustumTimer.End();

/*
			if( r_speeds.GetInt() )
			{
				Con_NPrintf( 0, "AreaPortals: flow: %.2fms frustum: %.2fms", 
					flowTimer.GetDuration().GetMillisecondsF(),
					frustumTimer.GetDuration().GetMillisecondsF() );
			}
*/
		}
	}
	else
	{
		// They don't want to clip, just copy the areabits from doors and such.
		memcpy( g_RenderAreaBits, cl.pAreaBits, sizeof( g_RenderAreaBits ) );
	}
}


const Frustum_t* GetAreaFrustum( int area )
{
	return &g_AreaCullInfo[area].m_Frustum;
}

