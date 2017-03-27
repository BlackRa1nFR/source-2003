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

#include "glquake.h"
#include "gl_model_private.h"
#include "dispchain.h"
#include "gl_rsurf.h"
#include "gl_cvars.h"
#include "enginestats.h"

//=============================================================================

ConVar r_lod_noupdate( "r_lod_noupdate", "0" );
extern ConVar r_DrawDisp;

//=============================================================================



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDispChain::AddTo( IDispInfo *pDispInfo )
{
#if 0
	//
	// only add a surface once
	//
	// NOTE: search is linear for now, look into this during opt. phase
	//
	for( IDispInfo *pCmpDispInfo = m_pHead; pCmpDispInfo; pCmpDispInfo = pCmpDispInfo->GetNextInChain() )
	{
		if( pCmpDispInfo == pDispInfo )
			return;
	}
#endif

	//
	// add unique surface to the chain
	//
	if( m_bForRayCasts )
	{
		pDispInfo->SetNextInRayCastChain( m_pHead );
	}
	else
	{
		pDispInfo->SetNextInRenderChain( m_pHead );
	}
	m_pHead = pDispInfo;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDispChain::RenderWireframeInLightmapPage( void )
{
#ifdef USE_CONVARS
	if( !r_DrawDisp.GetInt() )
		return;
#endif

    //
    // render wireframe in lightmap page for the given displacement
    //
	for( IDispInfo *pDispInfo = m_pHead; pDispInfo; pDispInfo = pDispInfo->GetNextInRenderChain() )
	{
		pDispInfo->RenderWireframeInLightmapPage();

		g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_DISP_SURFACES, 1 );	
	}
}
