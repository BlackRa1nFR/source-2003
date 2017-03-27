//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		AI Utility classes for building the initial AI Networks
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_initutils.h"
#include "ai_networkmanager.h"

// to help eliminate node clutter by level designers, this is used to cap how many other nodes
// any given node is allowed to 'see' in the first stage of graph creation "LinkVisibleNodes()".

#include "ai_network.h"

LINK_ENTITY_TO_CLASS( info_hint,			CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node,			CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_hint,		CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_air,		CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_air_hint,	CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_climb,		CNodeEnt );
LINK_ENTITY_TO_CLASS( aitesthull, CAI_TestHull );

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_TestHull*	CAI_TestHull::pTestHull			= NULL;

//-----------------------------------------------------------------------------
// Purpose: Make sure we have a "player.mdl" hull to test with
//-----------------------------------------------------------------------------
void CAI_TestHull::Precache()
{
	BaseClass::Precache();
	engine->PrecacheModel( "models/player.mdl" );
}

//=========================================================
// CAI_TestHull::Spawn
//=========================================================
void CAI_TestHull::Spawn(void)
{
	Precache();

	SetModel( "models/player.mdl" );

	// Set an initial hull size (this will change later)
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_STEP );
	m_iHealth			= 50;

	bInUse				= false;

	// Make this invisible
	m_fEffects |= EF_NODRAW;
}

//-----------------------------------------------------------------------------
// Purpose: Get the test hull (create if none)
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_TestHull* CAI_TestHull::GetTestHull(void)
{
	if (!CAI_TestHull::pTestHull)
	{
		CAI_TestHull::pTestHull = CREATE_ENTITY( CAI_TestHull, "aitesthull" );
		CAI_TestHull::pTestHull->Spawn();
		CAI_TestHull::pTestHull->AddFlag( FL_NPC );
	}

	if (CAI_TestHull::pTestHull->bInUse == true)
	{
		Msg("WARNING: TestHull used and never returned!\n");
	}


	CAI_TestHull::pTestHull->RemoveSolidFlags( FSOLID_NOT_SOLID );
	CAI_TestHull::pTestHull->bInUse = true;
	CAI_TestHull::pTestHull->Relink();

	return CAI_TestHull::pTestHull;
}

//-----------------------------------------------------------------------------
// Purpose: Get the test hull (create if none)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_TestHull::ReturnTestHull(void)
{
	CAI_TestHull::pTestHull->bInUse = false;
	CAI_TestHull::pTestHull->AddSolidFlags( FSOLID_NOT_SOLID );
	UTIL_SetSize(CAI_TestHull::pTestHull, vec3_origin, vec3_origin);
	CAI_TestHull::pTestHull->Relink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &startPos - 
//			&endPos - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_TestHull::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 1024.0f;
	const float MAX_JUMP_DISTANCE	= 1024.0f;
	const float MAX_JUMP_DROP		= 1024.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DISTANCE, MAX_JUMP_DROP );
}
//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_TestHull::~CAI_TestHull(void)
{
	CAI_TestHull::pTestHull = NULL;
}

//###########################################################
//	> CNodeEnt
//
// nodes start out as ents in the world. As they are spawned,
// the node info is recorded then the ents are discarded.
//###########################################################

//----------------------------------------------------
//  Static vars
//----------------------------------------------------
int CNodeEnt::m_nNodeCount = 0;

// -------------
// Data table
// -------------
BEGIN_DATADESC( CNodeEnt )

	DEFINE_KEYFIELD( CNodeEnt, m_eHintType,			FIELD_SHORT,	"hinttype" ),
	DEFINE_KEYFIELD( CNodeEnt, m_nWCNodeID,			FIELD_INTEGER,	"nodeid" ),
	DEFINE_KEYFIELD( CNodeEnt, m_strGroup,			FIELD_STRING,	"Group" ),

END_DATADESC()


//=========================================================
//=========================================================
void CNodeEnt::Spawn( void )
{
	// ---------------------------------------------------------------------------------
	//  If just a hint node (not used for navigation) just create a hint and bail
	// ---------------------------------------------------------------------------------
	if (FClassnameIs( this, "info_hint" ))
	{
		if (m_eHintType)
		{
			CAI_Hint::CreateHint(GetEntityName(),GetAbsOrigin(),(Hint_e)m_eHintType,NO_NODE,m_strGroup);
		}
		else
		{
			Msg("WARNING: Hint node with no hint type!\n");
		}
		UTIL_RemoveImmediate( this );
		return;
	}
	
	// ---------------------------------------------------------------------------------
	//  First check if this node has a hint.  If so create a hint entity
	// ---------------------------------------------------------------------------------
	CAI_Hint *pHint = NULL;
	if (m_eHintType || m_strGroup != NULL_STRING)
	{
		pHint = CAI_Hint::CreateHint(GetEntityName(),GetAbsOrigin(),(Hint_e)m_eHintType,m_nNodeCount,m_strGroup);
	}


	// ---------------------------------------------------------------------------------
	//  If we loaded from disk, we can discard all these node ents as soon as they spawn
	//  unless we are in WC edited mode
	// ---------------------------------------------------------------------------------
	if ( g_pAINetworkManager->NetworksLoaded() && !engine->IsInEditMode())
	{
		// If hint exists for this node, set it
		if (pHint)
		{
			CAI_Node *pNode = g_pBigAINet->GetNode(m_nNodeCount);
			if (pNode)
				pNode->SetHint( pHint );
			else
			{
				Msg("AI node graph corrupt\n");
			}
		}
		m_nNodeCount++;
		UTIL_RemoveImmediate( this );
		return;
	}	
	else
	{
		m_nNodeCount++;
	}

	// ---------------------------------------------------------------------------------
	//	Add a new node to the network
	// ---------------------------------------------------------------------------------
	// For now just using one big AI network
	CAI_Node *new_node = g_pBigAINet->AddNode( GetAbsOrigin(), GetAbsAngles().y );
	new_node->SetHint( pHint );

	// -------------------------------------------------------------------------
	//  Update table of how each WC id relates to each engine ID	
	// -------------------------------------------------------------------------
	if (g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable)
	{
		g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[new_node->GetId()]	= m_nWCNodeID;
	}
	// Keep track of largest index used by WC
	if (g_pAINetworkManager->GetEditOps()->m_nNextWCIndex <= m_nWCNodeID)
	{
		g_pAINetworkManager->GetEditOps()->m_nNextWCIndex = m_nWCNodeID+1;
	}

	// -------------------------------------------------------------------------
	// If in WC edit mode:
	// 	Remember the original positions of the nodes before
	//	they drop so we can send the undropped positions to wc.
	// -------------------------------------------------------------------------
	if (engine->IsInEditMode())
	{
		if (g_pAINetworkManager->GetEditOps()->m_pWCPosition)
		{
			g_pAINetworkManager->GetEditOps()->m_pWCPosition[new_node->GetId()]		= new_node->GetOrigin();
		}
	}
	
	if (FClassnameIs( this, "info_node_air" ) || FClassnameIs( this, "info_node_air_hint" ))
	{
		new_node->SetType( NODE_AIR );
	}
	else if (FClassnameIs( this, "info_node_climb" ))
	{
		new_node->SetType( NODE_CLIMB );
	}
	else
	{
		new_node->SetType( NODE_GROUND );
	}

	// If changed as part of WC editing process note that network must be rebuilt
	if (m_debugOverlays & OVERLAY_WC_CHANGE_ENTITY)
	{
		g_pAINetworkManager->GetEditOps()->SetRebuildFlags();
		new_node->m_eNodeInfo			|= bits_NODE_WC_CHANGED;

		// Initialize the new nodes position.  The graph may not be rebuild
		// right away but the node should at least be positioned correctly
		g_AINetworkBuilder.InitNodePosition( g_pBigAINet, new_node );
	}

	UTIL_RemoveImmediate( this );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CNodeEnt::CNodeEnt(void) {

	m_debugOverlays = 0;
}



