//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "gl_model_private.h"
#include "mathlib.h"
#include "zone.h"
#include "gl_matsysiface.h"
#include "mempool.h"
#include "disp_leaflink.h"


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void AddDispSurfsToLeafs_r( IDispInfo *pDispInfo, mnode_t *pNode )
{
	assert( pDispInfo );

	while( 1 )
	{
		//
		// leaf
		//
		if( pNode->contents >= 0 )
		{
			// get the leaf
			mleaf_t *pLeaf = ( mleaf_t* )pNode;
			
			CDispLeafLink *pLink = static_cast< CDispLeafLink* >( g_DispLeafMPool.Alloc( sizeof(CDispLeafLink) ) );
			pLink->Add( pDispInfo, pDispInfo->GetLeafLinkHead(), pLeaf, pLeaf->m_pDisplacements );
			return;
		}

		//
		// get displacement bounding box position relative to the node plane
		//
		cplane_t *pPlane = pNode->plane;

		Vector mins, maxs;
		pDispInfo->GetBoundingBox( mins, maxs );
        int sideResult = BOX_ON_PLANE_SIDE( mins, maxs, pPlane );

        // front side
        if( sideResult == 1 )
        {
			pNode = pNode->children[0];
        }
        // back side
        else if( sideResult == 2 )
        {
			pNode = pNode->children[1];
        }
        //split
        else
        {
			AddDispSurfsToLeafs_r( pDispInfo, pNode->children[0] );
			pNode = pNode->children[1];
        }
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void AddDispSurfsToLeafs( model_t *pWorld )
{
	int i;

	//
	// first determine how many displacement surfaces there will be per leaf
	//
	for( i = 0; i < pWorld->brush.numDispInfos; i++ )
	{
		IDispInfo *pDisp = DispInfo_IndexArray( pWorld->brush.hDispInfos, i );
		if( pDisp->GetParent() == NULL )
			continue;

		AddDispSurfsToLeafs_r( pDisp, &pWorld->brush.nodes[0] );
	}
}


void ReAddDispSurfToLeafs( model_t *pWorld, IDispInfo *pDispInfo )
{
	// Remove it from leaves.
	CDispLeafLink *pHead;
	while( pHead = pDispInfo->GetLeafLinkHead() )
	{
		mleaf_t *pLeaf = static_cast<mleaf_t*>( pHead->m_pLeaf );
		pHead->Remove( pDispInfo->GetLeafLinkHead(), pLeaf->m_pDisplacements );
		g_DispLeafMPool.Free( pHead );
	}

	// Add it back.
	AddDispSurfsToLeafs_r( pDispInfo, &pWorld->brush.nodes[0] );
}

