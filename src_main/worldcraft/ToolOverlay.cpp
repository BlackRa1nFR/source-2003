//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdafx.h>
#include "MapWorld.h"
#include "GlobalFunctions.h"
#include "MainFrm.h"
#include "ToolOverlay.h"
#include "MapDoc.h"
#include "History.h"
#include "CollisionUtils.h"
#include "cmodel.h"
#include "MapView3D.h"
#include "MapView2D.h"
#include "MapSolid.h"
#include "Camera.h"
#include "ObjectProperties.h"  // FIXME: For ObjectProperties::RefreshData

#define OVERLAY_TOOL_SNAP_DISTANCE	35.0f

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CToolOverlay::CToolOverlay()
{
	m_bEmpty = true;
	m_bDragging = false;
	m_pDoc = NULL;
	m_pActiveOverlay = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CToolOverlay::~CToolOverlay()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::OnActivate( ToolID_t eOldTool )
{
	m_bEmpty = true;
	m_bDragging = false;
	m_pDoc = CMapDoc::GetActiveMapDoc();
	m_pActiveOverlay = NULL;
}
    
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::OnDeactivate( ToolID_t eOldTool )
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::OnLMouseUp3D( CMapView3D *pView, UINT nFlags, CPoint point )
{
	// Reset dragging (we may or may not have been dragging!)
	m_bDragging = false;

	ReClip();
	HandlesReset();

	// Update the entity properties dialog.
	GetMainWnd()->pObjectProperties->RefreshData();

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::OnLMouseDown3D( CMapView3D *pView, UINT nFlags, CPoint point )
{
	//
	// Handle the overlay "handle" selection.
	//
	if ( HandlesHitTest( point ) )
	{
		m_bDragging = true;
		SetupHandleDragUndo();
		return true;
	}

	//
	// Handle the overlay creation and placement (if we hit a solid).
	//
	ULONG		ulFace;
	CMapClass	*pObject;
	if ( ( pObject = pView->NearestObjectAt( point, ulFace ) ) != NULL )
	{
		CMapSolid *pSolid = dynamic_cast<CMapSolid*>( pObject );
		if ( pSolid )
			return CreateOverlay( pSolid, ulFace, pView, point );
	}

	// Handle adding and removing overlay entities from the selection list.
	OverlaySelection( pView, nFlags, point );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::AddInitialFaceToSideList( CMapEntity *pEntity, CMapFace *pFace )
{
	// Valid face?
	if ( !pFace )
		return;

	POSITION pos;
	CMapClass *pChild = pEntity->GetFirstChild( pos );
	while( pChild )
	{
		CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pChild );
		if ( pOverlay )
		{
			pOverlay->AddInitialFaceToSideList( pFace );
		}

		pChild = pEntity->GetNextChild( pos );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::CreateOverlay( CMapSolid *pSolid, ULONG iFace, CMapView3D *pView, CPoint point )
{
	//
	// Build a ray to trace against the face that they clicked on to
	// find the point of intersection.
	//			
	Vector vecStart, vecEnd;
	pView->BuildRay( point, vecStart, vecEnd );
	
	Vector vecHitPos, vecHitNormal;
	CMapFace *pFace = pSolid->GetFace( iFace );
	if( pFace->TraceLine( vecHitPos, vecHitNormal, vecStart, vecEnd ) )
	{
		//
		// Create and initialize the "entity" --> "overlay."
		//
		CMapEntity *pEntity = new CMapEntity;
		pEntity->SetKeyValue( "material", GetDefaultTextureName() );
		pEntity->SetPlaceholder( TRUE );
		pEntity->SetOrigin( vecHitPos );
		pEntity->SetClass( "info_overlay" );				
		pEntity->CalcBounds( TRUE );
		
		// Add the entity to the world.
		m_pDoc->AddObjectToWorld( pEntity );
		
		// Setup "history."
		GetHistory()->MarkUndoPosition( NULL, "Create Overlay" );
		GetHistory()->KeepNew( pEntity );
		
		UpdateBox ub;
		CMapObjectList ObjectList;
		ObjectList.AddTail( pEntity );
		ub.Objects = &ObjectList;
		
		Vector vecMins, vecMaxs;
		pEntity->GetRender2DBox( vecMins, vecMaxs );
		ub.Box.SetBounds( vecMins, vecMaxs );

		// Get the overlay and add the face to the sidelist.
		AddInitialFaceToSideList( pEntity, pFace );

		// Add to selection list.
		m_pDoc->SelectObject( pEntity, CMapDoc::scSelect | CMapDoc::scUpdateDisplay );
		m_bEmpty = false;

		// Set modified and update views.
		m_pDoc->UpdateAllViews( NULL, MAPVIEW_UPDATE_OBJECTS, &ub );
		m_pDoc->SetModifiedFlag();
		
		// Set modified and update the property box.
		GetMainWnd()->pObjectProperties->LoadData( m_pDoc->Selection_GetList(), false );

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::OverlaySelection( CMapView3D *pView, UINT nFlags, CPoint point )
{
	CMapObjectList aSelectionList;
	m_pDoc->ClearHitList();

	//
	// Find out how many (and what) map objects are under the point clicked on.
	//
	RenderHitInfo_t Objects[512];
	int nHits = pView->ObjectsAt( point.x, point.y, 1, 1, Objects, sizeof( Objects ) / sizeof( Objects[0] ) );
	if ( nHits != 0 )
	{
		//
		// We now have an array of pointers to CMapAtoms. Any that can be upcast to CMapClass objects?
		//
		for ( int iHit = 0; iHit < nHits; ++iHit )
		{
			CMapClass *pMapClass = dynamic_cast<CMapClass*>( Objects[iHit].pObject );
			if ( pMapClass )
			{
				aSelectionList.AddTail( pMapClass );
			}
		}
	}

	// Did we hit anything?
	if ( !aSelectionList.GetCount() )
	{
		m_pDoc->SelectFace( NULL, 0, CMapDoc::scClear );
		m_pDoc->SelectObject( NULL, CMapDoc::scClear | CMapDoc::scUpdateDisplay );
		SetEmpty();
		return;
	}

	//
	// Find overlays!
	//
	bool bUpdateViews = false;
	POSITION position = aSelectionList.GetHeadPosition();
	SelectMode_t eSelectMode = m_pDoc->Selection_GetMode();
	
	while ( position )
	{
		CMapClass *pObject = aSelectionList.GetNext( position );
		CMapClass *pHitObject = pObject->PrepareSelection( eSelectMode );
		if ( pHitObject )
		{
			if ( pHitObject->IsMapClass( MAPCLASS_TYPE( CMapEntity ) ) ) 
			{
				POSITION pos;
				CMapClass *pEntityChild = pHitObject->GetFirstChild( pos );
				while( pEntityChild )
				{
					CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pEntityChild );
					if ( pOverlay )
					{
						m_pDoc->AddHit( pHitObject );
						m_bEmpty = false;

						UINT cmd = CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay;
						if (nFlags & MK_CONTROL)
						{
							cmd = CMapDoc::scToggle;
						}
						m_pDoc->SelectObject( pHitObject, cmd );
						bUpdateViews = true;
					}

					pEntityChild = pHitObject->GetNextChild( pos );
				}
			}
		}
	}

	//
	// Update the views.
	//
	if ( bUpdateViews )
	{
		m_pDoc->UpdateAllViews( NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_OBJECTS );		
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::OnMouseMove3D( CMapView3D *pView, UINT nFlags, CPoint point )
{
	if ( m_bDragging )
	{
		bool bShift = ( ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0 );

		// Build the ray and drag the overlay handle to the impact point.
		const CCamera *pCamera = pView->GetCamera();
		if ( pCamera )
		{
			Vector vecStart, vecEnd;
			pView->BuildRay( point, vecStart, vecEnd );
			OnDrag( vecStart, vecEnd, bShift );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::OnContextMenu2D( CMapView2D *pView, CPoint point )
{
	static CMenu menu, menuOverlay;
	static bool bInit = false;

	if ( !bInit )
	{
		// Create the menu.
		menu.LoadMenu( IDR_POPUPS );
		menuOverlay.Attach( ::GetSubMenu( menu.m_hMenu,  6 ) );
		bInit = true;
	}

	pView->ScreenToClient( &point );

	CRect rect;
	pView->GetClientRect( &rect );
	if ( !rect.PtInRect( point ) )
		return false;

	CPoint ptScreen( point );
	pView->ClientToScreen( &ptScreen );
	CPoint ptMapScreen( point );
	ptMapScreen.x += pView->GetScrollPos( SB_HORZ );
	ptMapScreen.y += pView->GetScrollPos( SB_VERT );

	if (!IsEmpty())
	{
		if ( HitTest( ptMapScreen, FALSE ) != -1 )
		{
			menuOverlay.TrackPopupMenu( TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptScreen.x, ptScreen.y, pView );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			BOOL - 
// Output : int
//-----------------------------------------------------------------------------
int CToolOverlay::HitTest( CPoint pt, BOOL )
{
	return Box3D::HitTest( pt, FALSE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL CToolOverlay::UpdateTranslation( CPoint pt, UINT nFlags, CSize& )
{
//	if( m_bBoxSelecting )
//		return Box3D::UpdateTranslation( pt, nFlags );

	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::HandlesReset( void )
{
	// Go through selection list and rest overlay handles.
	int nSelectionCount = m_pDoc->Selection_GetCount();
	for( int iSelection = 0; iSelection < nSelectionCount; ++iSelection )
	{
		CMapClass *pMapClass = m_pDoc->Selection_GetObject( iSelection );
		if ( pMapClass && pMapClass->IsMapClass( MAPCLASS_TYPE( CMapEntity ) ) )
		{	
			POSITION childPos;
			CMapClass *pChild = pMapClass->GetFirstChild( childPos );
			while ( pChild )
			{
				CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pChild );
				if ( pOverlay )
				{
					pOverlay->HandlesReset();
					break;
				}
				
				pChild = pMapClass->GetNextChild( childPos );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::HandlesHitTest( CPoint const &pt )
{
	// Reset the hit overlay.
	m_pActiveOverlay = NULL;

	// Go through selection list and test all overlay's handles and set the
	// "hit" overlay current.
	int nSelectionCount = m_pDoc->Selection_GetCount();
	for( int iSelection = 0; iSelection < nSelectionCount; ++iSelection )
	{
		CMapClass *pMapClass = m_pDoc->Selection_GetObject( iSelection );
		if ( pMapClass && pMapClass->IsMapClass( MAPCLASS_TYPE( CMapEntity ) ) )
		{
			POSITION childPos;
			CMapClass *pChild = pMapClass->GetFirstChild( childPos );
			while ( pChild )
			{
				CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pChild );
				if ( pOverlay )
				{
					if ( pOverlay->HandlesHitTest( pt ) )
					{
						m_pActiveOverlay = pOverlay;
						break;
					}
				}
				
				pChild = pMapClass->GetNextChild( childPos );
			}
		}
	}

	if ( !m_pActiveOverlay )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::HandleSnap( CMapOverlay *pOverlay, Vector &vecHandlePt )
{
	Vector vecTmp;
	for ( int i = 0; i < OVERLAY_HANDLES_COUNT; i++ )
	{
		pOverlay->GetHandlePos( i, vecTmp );
		vecTmp -= vecHandlePt;
		float flDist = vecTmp.Length();

		if ( flDist < OVERLAY_TOOL_SNAP_DISTANCE )
		{
			// Snap!
			pOverlay->GetHandlePos( i, vecHandlePt );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CToolOverlay::HandleInBBox( CMapOverlay *pOverlay, Vector const &vecHandlePt )
{
	Vector vecMin, vecMax;
	pOverlay->GetCullBox( vecMin, vecMax );

	for ( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		vecMin[iAxis] -= OVERLAY_TOOL_SNAP_DISTANCE;
		vecMax[iAxis] += OVERLAY_TOOL_SNAP_DISTANCE;

		if( ( vecHandlePt[iAxis] < vecMin[iAxis] ) || ( vecHandlePt[iAxis] > vecMax[iAxis] ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::SnapHandle( Vector &vecHandlePt )
{
	CMapWorld *pWorld = GetActiveWorld();
	if ( !pWorld )
		return;

	EnumChildrenPos_t pos;
	CMapClass *pChild = pWorld->GetFirstDescendent( pos );
	while ( pChild )
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity*>( pChild );
		if ( pEntity )
		{
			POSITION entPos;
			CMapClass *pEntityChild = pEntity->GetFirstChild( entPos );
			while ( pEntityChild )
			{
				CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pEntityChild );
				if ( pOverlay )
				{
					// Intersection test and attempt to snap
					if ( HandleInBBox( pOverlay, vecHandlePt ) )
					{
						if ( HandleSnap( pOverlay, vecHandlePt ) )
							return;
					}
				}
				
				pEntityChild = pEntity->GetNextChild( entPos );
			}
		}

		pChild = pWorld->GetNextDescendent( pos );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::OnDrag( Vector const &vecRayStart, Vector const &vecRayEnd, bool bShift )
{
	// Get the current overlay.
	CMapOverlay *pOverlay = m_pActiveOverlay;
	if ( !pOverlay )
		return;

	// Get a list of faces and test for "impact."
	Vector vecImpact( 0.0f, 0.0f, 0.0f );
	Vector vecImpactNormal( 0.0f, 0.0f, 0.0f );
	CMapFace *pFace = NULL;
		
	int nFaceCount = pOverlay->GetFaceCount();
	for ( int iFace = 0; iFace < nFaceCount; iFace++ )
	{
		// Get the current face.
		pFace = pOverlay->GetFace( iFace );
		if ( pFace )
		{
			if ( pFace->TraceLineInside( vecImpact, vecImpactNormal, vecRayStart, vecRayEnd ) )
				break;
		}
	}

	// Test for impact (face index = count mean no impact).
	if ( iFace == nFaceCount )
		return;

	if ( bShift )
	{
		SnapHandle( vecImpact );
	}
	
	// Pass the new handle position to the overlay.
	pOverlay->HandlesDragTo( vecImpact, pFace );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::ReClip( void )
{
	// Get the current overlay.
	CMapOverlay *pOverlay = m_pActiveOverlay;
	if ( pOverlay )
	{
		pOverlay->DoClip();
		pOverlay->PostUpdate( Notify_Changed );

		Vector vecMins, vecMaxs;
		UpdateBox ub;
		pOverlay->GetRender2DBox( vecMins, vecMaxs );
		ub.Box.SetBounds( vecMins, vecMaxs );	
		m_pDoc->UpdateAllViews( NULL, MAPVIEW_UPDATE_OBJECTS, &ub );	
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::ReClipAndReCenterEntity( void )
{
	// Get the current overlay.
	CMapOverlay *pOverlay = m_pActiveOverlay;
	if ( pOverlay )
	{
		pOverlay->DoClip();
		pOverlay->PostUpdate( Notify_Changed );

		// Update the properties box.
		UpdatePropertiesBox();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::SetupHandleDragUndo( void )
{
	// Get the current overlay.
	CMapOverlay *pOverlay = m_pActiveOverlay;
	if ( pOverlay )
	{
		CMapEntity *pEntity = ( CMapEntity* )pOverlay->GetParent();
		if ( pEntity )
		{
			// Setup for drag undo.
			GetHistory()->MarkUndoPosition( m_pDoc->Selection_GetList(), "Drag Overlay Handle" );
			GetHistory()->Keep( ( CMapClass* )pEntity );				
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CToolOverlay::UpdatePropertiesBox( void )
{
	GetMainWnd()->pObjectProperties->LoadData( m_pDoc->Selection_GetList(), false );
}


