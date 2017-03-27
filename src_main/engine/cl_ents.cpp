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
// cl_ents.cpp -- entity management

#include "quakedef.h"
#include "client_class.h"
#include "cdll_int.h"
#include "r_local.h"
#include "icliententitylist.h"
#include "cl_ents.h"
#include "measure_section.h"
#include "demo.h"
#include "cdll_engine_int.h"
#include "icliententity.h"
#include "networkstringtablecontainerclient.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Entity baseline lists.
PackedEntity *g_EntityBaselines = NULL;
int g_nEntityBaselines = 0;

// Allocator for packed entities structures.
PackedDataAllocator g_ClientPackedDataAllocator;


//-----------------------------------------------------------------------------
// Entity baseline accessors.
//-----------------------------------------------------------------------------
bool AllocateEntityBaselines(int nBaselines)
{
	FreeEntityBaselines();

	if ( g_EntityBaselines = new PackedEntity[ nBaselines ] )
	{
		g_nEntityBaselines = nBaselines;

		for ( int i = 0 ; i < g_nEntityBaselines; i++ )
		{
			PackedEntity *p = &g_EntityBaselines[ i ];
			p->SetNumBits( 0 );
			p->m_nEntityIndex = 0;
			p->m_pRecvTable = NULL;
			p->m_pSendTable = NULL;
			p->m_ReferenceCount = 0;
		}
		return true;
	}
	else
	{
		return false;
	}
}

void FreeEntityBaselines()
{
	delete [] g_EntityBaselines;
	g_EntityBaselines = NULL;
	g_nEntityBaselines = 0;
}

PackedEntity* GetStaticBaseline(int index)
{
	if ( index < 0 || index >= g_nEntityBaselines )
	{
		return NULL;
	}

	return &g_EntityBaselines[index];
}

bool GetDynamicBaseline( int iClass, void const **pData, int *pDatalen )
{
	ErrorIfNot( 
		iClass >= 0 && iClass < cl.m_nServerClasses, 
		("GetDynamicBaseline: invalid class index '%d'", iClass) );

	// We lazily update these because if you connect to a server that's already got some dynamic baselines,
	// you'll get the baselines BEFORE you get the class descriptions.
	C_ServerClassInfo *pInfo = &cl.m_pServerClasses[iClass];
	if ( pInfo->m_InstanceBaselineIndex == INVALID_STRING_INDEX )
	{
		// The key is the class index string.
		char str[64];
		Q_snprintf( str, sizeof( str ), "%d", iClass );

		pInfo->m_InstanceBaselineIndex = networkStringTableContainerClient->FindStringIndex( cl.GetInstanceBaselineTable(), str );
		
		ErrorIfNot( 
			pInfo->m_InstanceBaselineIndex != INVALID_STRING_INDEX,
			("GetDynamicBaseline: FindStringIndex(%s) failed.", str)
			);
	}

	*pData = networkStringTableContainerClient->GetStringUserData( 
		cl.GetInstanceBaselineTable(),
		pInfo->m_InstanceBaselineIndex,
		pDatalen );

	return *pData != NULL;
}

bool AllocPackedData( int size, DataHandle &data )
{
	return g_ClientPackedDataAllocator.Alloc( size, data );
}



//-----------------------------------------------------------------------------
// Purpose: Determines the fraction between the last two messages that the objects
//   should be put at.
//-----------------------------------------------------------------------------
float CL_LerpPoint (void)
{
	return g_ClientGlobalVariables.interpolation_amount;
}

//-----------------------------------------------------------------------------
// Purpose: Build list of visible objects for rendering this frame
//-----------------------------------------------------------------------------
void CL_UpdateFrameLerp( void )
{
	// Not in server yet, no entities to redraw
	if ( cls.state != ca_active )      
	{
		return;
	}

	// We haven't received our first valid update from the server.  Leave visible entities empty for now.
	if ( !cl.validsequence )
	{
		return;
	}

	if ( cl.frames[ cl.parsecountmod ].invalid )
		return;

	// Compute last interpolation amount
	cl.commands[ ( cls.netchan.outgoing_sequence - 1 ) & CL_UPDATE_MASK ].frame_lerp = CL_LerpPoint();
}


