//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vbsp.h"

// Overlay list.
CUtlVector<mapoverlay_t> g_aMapOverlays;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_GetFromEntity( entity_t *pMapEnt )
{
	// Allocate the new overlay.
	int iOverlay = g_aMapOverlays.AddToTail();
	mapoverlay_t *pMapOverlay = &g_aMapOverlays[iOverlay];

	// Get the overlay data.
	pMapOverlay->nId = g_aMapOverlays.Count() - 1;

	pMapOverlay->flU[0] = FloatForKey( pMapEnt, "StartU" );
	pMapOverlay->flU[1] = FloatForKey( pMapEnt, "EndU" );
	pMapOverlay->flV[0] = FloatForKey( pMapEnt, "StartV" );
	pMapOverlay->flV[1] = FloatForKey( pMapEnt, "EndV" );

	GetVectorForKey( pMapEnt, "origin", pMapOverlay->vecOrigin );

	GetVectorForKey( pMapEnt, "uv0", pMapOverlay->vecUVPoints[0] );
	GetVectorForKey( pMapEnt, "uv1", pMapOverlay->vecUVPoints[1] );
	GetVectorForKey( pMapEnt, "uv2", pMapOverlay->vecUVPoints[2] );
	GetVectorForKey( pMapEnt, "uv3", pMapOverlay->vecUVPoints[3] );

	const char *pMaterialName = ValueForKey( pMapEnt, "material" );
	Assert( strlen( pMaterialName ) < OVERLAY_MAP_STRLEN );
	if ( strlen( pMaterialName ) >= OVERLAY_MAP_STRLEN )
	{
		Error( "Overlay Material Name > OVERLAY_MAP_STRLEN" );
		return;
	}
	strcpy( pMapOverlay->szMaterialName, pMaterialName );	

	// Convert the sidelist to side id(s).
	const char *pSideList = ValueForKey( pMapEnt, "sides" );
	char *pTmpList = ( char* )_alloca( strlen( pSideList ) + 1 );
	strcpy( pTmpList, pSideList );
	const char *pScan = strtok( pTmpList, " " );
	if ( !pScan )
		return;

	do
	{
		int nSideId;
		if ( sscanf( pScan, "%d", &nSideId ) == 1 )
		{
			pMapOverlay->aSideList.AddToTail( nSideId );
		}
	} while ( ( pScan = strtok( NULL, " " ) ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
side_t *GetSide( int nSideId )
{
	for( int iSide = 0; iSide < nummapbrushsides; ++iSide )
	{
		if ( brushsides[iSide].id == nSideId )
			return &brushsides[iSide];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void GetSideNormal( mapoverlay_t *pMapOverlay, side_t *pSide )
{
	plane_t *pPlane = &mapplanes[pSide->planenum];
	VectorCopy( pPlane->normal, pMapOverlay->vecBasisNormal );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_UpdateSideLists( void )
{
	int nMapOverlayCount = g_aMapOverlays.Count();
	for( int iMapOverlay = 0; iMapOverlay < nMapOverlayCount; ++iMapOverlay )
	{
		mapoverlay_t *pMapOverlay = &g_aMapOverlays.Element( iMapOverlay );
		if ( pMapOverlay )
		{
			int nSideCount = pMapOverlay->aSideList.Count();
			for( int iSide = 0; iSide < nSideCount; ++iSide )
			{
				side_t *pSide = GetSide( pMapOverlay->aSideList[iSide] );
				if ( pSide )
				{
					if ( iSide == 0 )
					{
						GetSideNormal( pMapOverlay, pSide );
					}

					pSide->aOverlayIds.AddToTail( pMapOverlay->nId );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_AddFaceToLists( int iFace, side_t *pSide )
{
	int nOverlayIdCount = pSide->aOverlayIds.Count();
	for( int iOverlayId = 0; iOverlayId < nOverlayIdCount; ++iOverlayId )
	{
		mapoverlay_t *pMapOverlay = &g_aMapOverlays.Element( pSide->aOverlayIds[iOverlayId] );
		if ( pMapOverlay )
		{
			pMapOverlay->aFaceList.AddToTail( iFace );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_EmitOverlayFace( mapoverlay_t *pMapOverlay )
{
	Assert( g_nOverlayCount < MAX_MAP_OVERLAYS );
	if ( g_nOverlayCount >= MAX_MAP_OVERLAYS )
	{
		Error ( "g_nOverlayCount >= MAX_MAP_OVERLAYS" );
		return;
	}

	doverlay_t *pOverlay = &g_Overlays[g_nOverlayCount];
	g_nOverlayCount++;

	// Conver the map overlay into a .bsp overlay (doverlay_t).
	if ( pOverlay )
	{
		pOverlay->nId = pMapOverlay->nId;

		pOverlay->flU[0] = pMapOverlay->flU[0];
		pOverlay->flU[1] = pMapOverlay->flU[1];
		pOverlay->flV[0] = pMapOverlay->flV[0];
		pOverlay->flV[1] = pMapOverlay->flV[1];

		VectorCopy( pMapOverlay->vecOrigin, pOverlay->vecOrigin );

		VectorCopy( pMapOverlay->vecUVPoints[0], pOverlay->vecUVPoints[0] );
		VectorCopy( pMapOverlay->vecUVPoints[1], pOverlay->vecUVPoints[1] );
		VectorCopy( pMapOverlay->vecUVPoints[2], pOverlay->vecUVPoints[2] );
		VectorCopy( pMapOverlay->vecUVPoints[3], pOverlay->vecUVPoints[3] );

		VectorCopy( pMapOverlay->vecBasisNormal, pOverlay->vecBasisNormal );

		// Texinfo.
		texinfo_t texInfo;
		texInfo.flags = 0;
		texInfo.texdata = FindTexData( pMapOverlay->szMaterialName );
		for( int iVec = 0; iVec < 2; ++iVec )
		{
			for( int iAxis = 0; iAxis < 3; ++iAxis )
			{
				texInfo.lightmapVecsLuxelsPerWorldUnits[iVec][iAxis] = 0.0f;
				texInfo.textureVecsTexelsPerWorldUnits[iVec][iAxis] = 0.0f;
			}

			texInfo.lightmapVecsLuxelsPerWorldUnits[iVec][3] = -99999.0f;
			texInfo.textureVecsTexelsPerWorldUnits[iVec][3] = -99999.0f;			
		}
		pOverlay->nTexInfo = FindOrCreateTexInfo( texInfo );

		// Face List
		int nFaceCount = pMapOverlay->aFaceList.Count();
		Assert( nFaceCount < OVERLAY_BSP_FACE_COUNT );
		if ( nFaceCount >= OVERLAY_BSP_FACE_COUNT )
		{
			Error( "Face List Count >= OVERLAY_BSP_FACE_COUNT" );
			return;
		}

		pOverlay->nFaceCount = nFaceCount;
		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			pOverlay->aFaces[iFace] = pMapOverlay->aFaceList.Element( iFace );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_EmitOverlayFaces( void )
{
	int nMapOverlayCount = g_aMapOverlays.Count();
	for( int iMapOverlay = 0; iMapOverlay < nMapOverlayCount; ++iMapOverlay )
	{
		Overlay_EmitOverlayFace( &g_aMapOverlays.Element( iMapOverlay ) );
	}
}