//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		A link that can be turned on and off.  Unlike normal links
//				dyanimc links must be entities so they can receive messages.
//				They update the state of the actual links.  Allows us to save
//				a lot of memory by not making all links into entities
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "collisionutils.h"
#include "ai_dynamiclink.h"
#include "ai_node.h"
#include "ai_link.h"
#include "ai_network.h"
#include "ai_networkmanager.h"
#include "saverestore_utlvector.h"
#include "editor_sendcommand.h"
#include "bitstring.h"

//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(info_node_link_controller, CAI_DynamicLinkController);

BEGIN_DATADESC( CAI_DynamicLinkController )

	DEFINE_KEYFIELD( CAI_DynamicLinkController, m_nLinkState, FIELD_INTEGER, "initialstate" ),
	DEFINE_KEYFIELD( CAI_DynamicLinkController, m_strAllowUse, FIELD_STRING, "AllowUse" ),
	DEFINE_UTLVECTOR( CAI_DynamicLinkController, m_ControlledLinks, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( CAI_DynamicLinkController, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CAI_DynamicLinkController, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()


void CAI_DynamicLinkController::GenerateLinksFromVolume()
{
	int nNodes = g_pBigAINet->NumNodes();
	CAI_Node **ppNodes = g_pBigAINet->AccessNodes();

	const float MinDistCareSq = Square(MAX_NODE_LINK_DIST * 0.5);

	const Vector &origin = GetAbsOrigin();
	const Vector &vAbsMins = GetAbsMins();
	const Vector &vAbsMaxs = GetAbsMaxs();

	for ( int i = 0; i < nNodes; i++ )
	{
		CAI_Node *pNode = ppNodes[i];
		const Vector &nodeOrigin = pNode->GetOrigin();
		if ( origin.DistToSqr(nodeOrigin) < MinDistCareSq )
		{
			int nLinks = pNode->NumLinks();
			for ( int j = 0; j < nLinks; j++ )
			{
				CAI_Link *pLink = pNode->GetLinkByIndex( j );
				int iLinkDest = pLink->DestNodeID( i );
				if ( iLinkDest > i )
				{
					const Vector &originOther = ppNodes[iLinkDest]->GetOrigin();
					if ( origin.DistToSqr(originOther) < MinDistCareSq )
					{
						if ( IsBoxIntersectingRay( vAbsMins, vAbsMaxs, nodeOrigin, originOther - nodeOrigin ) && 
							 IsBoxIntersectingRay( vAbsMins, vAbsMaxs, originOther, nodeOrigin - originOther ) )
						{
							CAI_DynamicLink *pLink = (CAI_DynamicLink *)CreateEntityByName( "info_node_link" );
							pLink->m_nSrcID = i;
							pLink->m_nDestID = iLinkDest;
							pLink->m_nLinkState = m_nLinkState;
							pLink->m_strAllowUse = m_strAllowUse;
							pLink->m_bFixedUpIds = true;

							pLink->Spawn();
							m_ControlledLinks.AddToTail( pLink );
						}
					}
				}
			}
		}
	}
}

void CAI_DynamicLinkController::InputTurnOn( inputdata_t &inputdata )
{
	for ( int i = 0; i < m_ControlledLinks.Count(); i++ )
	{
		if ( m_ControlledLinks[i] == NULL )
		{
			m_ControlledLinks.FastRemove(i);
			if ( i >= m_ControlledLinks.Count() )
				break;
		}
		m_ControlledLinks[i]->InputTurnOn( inputdata );
	}
}

void CAI_DynamicLinkController::InputTurnOff( inputdata_t &inputdata )
{
	for ( int i = 0; i < m_ControlledLinks.Count(); i++ )
	{
		if ( m_ControlledLinks[i] == NULL )
		{
			m_ControlledLinks.FastRemove(i);
			if ( i >= m_ControlledLinks.Count() )
				break;
		}
		m_ControlledLinks[i]->InputTurnOff( inputdata );
	}
}

//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(info_node_link, CAI_DynamicLink);

BEGIN_DATADESC( CAI_DynamicLink )

	//								m_pNextDynamicLink
	DEFINE_KEYFIELD( CAI_DynamicLink, m_nLinkState, FIELD_INTEGER, "initialstate" ),
	DEFINE_KEYFIELD( CAI_DynamicLink, m_nSrcID,		FIELD_INTEGER, "startnode" ),
	DEFINE_KEYFIELD( CAI_DynamicLink, m_nDestID,	FIELD_INTEGER, "endnode" ),
	DEFINE_KEYFIELD( CAI_DynamicLink, m_strAllowUse, FIELD_STRING, "AllowUse" ),
	DEFINE_FIELD(	 CAI_DynamicLink, m_bFixedUpIds, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( CAI_DynamicLink, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CAI_DynamicLink, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_DynamicLink *CAI_DynamicLink::m_pAllDynamicLinks = NULL;


//------------------------------------------------------------------------------

void CAI_DynamicLink::GenerateControllerLinks()
{
	CAI_DynamicLinkController *pController = NULL;
	while ( ( pController = gEntList.NextEntByClass( pController ) ) != NULL )
	{
		pController->GenerateLinksFromVolume();
	}
	
}

//------------------------------------------------------------------------------
// Purpose : Initializes src and dest IDs for all dynamic links
//			 	
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::InitDynamicLinks(void)
{
	if (!g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable)
	{
		Msg("ERROR: Trying initialize links with no WC ID table!\n");
		return;
	}
	
	GenerateControllerLinks();

	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		// -------------------------------------------------------------
		//  First convert this links WC IDs to engine IDs
		// -------------------------------------------------------------
		if ( !pDynamicLink->m_bFixedUpIds )
		{
			int	nSrcID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( pDynamicLink->m_nSrcID );
			if (nSrcID == -1)
			{
				Msg( "ERROR: Dynamic link WC node %d not found\n", pDynamicLink->m_nSrcID );
				nSrcID = NO_NODE;
			}

			int	nDestID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( pDynamicLink->m_nDestID );
			if (nDestID == -1)
			{
				Msg( "ERROR: Dynamic link WC node %d not found\n", pDynamicLink->m_nDestID );
				nDestID = NO_NODE;
			}
			
			pDynamicLink->m_nSrcID  = nSrcID;
			pDynamicLink->m_nDestID  = nDestID;
		}

		// Now set the link's state
		pDynamicLink->SetLinkState();

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
}


//------------------------------------------------------------------------------
// Purpose : Goes through each dynamic link and updates the state of all
//			 AINetwork links	
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::ResetDynamicLinks(void)
{
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		// Now set the link's state
		pDynamicLink->SetLinkState();

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
}


//------------------------------------------------------------------------------
// Purpose : Goes through each dynamic link and checks to make sure that
//			 there is still a corresponding node link, if not removes it
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::PurgeDynamicLinks(void)
{
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		if (!pDynamicLink->IsLinkValid())
		{
			// Didn't find the link, so remove it
#ifdef _WIN32
			int nWCSrcID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pDynamicLink->m_nSrcID];
			int nWCDstID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pDynamicLink->m_nDestID];
			int	status	 = Editor_DeleteNodeLink(nWCSrcID, nWCDstID, false);
			if (status == Editor_BadCommand)
			{
				Msg( "Worldcraft failed in PurgeDynamicLinks...\n" );
			}
#endif
			// Safe to remove it here as this happens only after I leave this function
			UTIL_Remove(pDynamicLink);
		}

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
}

//------------------------------------------------------------------------------
// Purpose : Returns false if the dynamic link doesn't have a corresponding
//			 node link
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLink::IsLinkValid( void )
{
	CAI_Node *pNode = g_pBigAINet->GetNode(m_nSrcID);

	return ( pNode->GetLink( m_nDestID ) != NULL );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAI_DynamicLink::InputTurnOn( inputdata_t &inputdata )
{
	if (m_nLinkState == LINK_OFF)
	{
		m_nLinkState = LINK_ON;
		CAI_DynamicLink::SetLinkState();
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAI_DynamicLink::InputTurnOff( inputdata_t &inputdata )
{
	if (m_nLinkState == LINK_ON)
	{
		m_nLinkState = LINK_OFF;
		CAI_DynamicLink::SetLinkState();
	}
}

//------------------------------------------------------------------------------
// Purpose : Updates network link state if dynamic link state has changed
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::SetLinkState(void)
{
	if (m_nSrcID == NO_NODE || m_nDestID == NO_NODE)
	{
		Msg("ERROR: Dynamic link pointing to invalid node ID!!\n");
		return;
	}

	// ------------------------------------------------------------------
	// Now update the node links...
	//  Nodes share links so we only have to find the node from the src 
	//  For now just using one big AINetwork so find src node on that network
	// ------------------------------------------------------------------
	CAI_Node*	pSrcNode = g_pBigAINet->GetNode(m_nSrcID);
	int			numLinks = pSrcNode->NumLinks();
	bool		bLinkFormed = false;
	for (int i=0;i<numLinks;i++)
	{
		CAI_Link* pLink = pSrcNode->GetLinkByIndex(i);

		if (((pLink->m_iSrcID  == m_nSrcID )&&
			 (pLink->m_iDestID == m_nDestID))   ||

			((pLink->m_iSrcID  == m_nDestID)&&
			 (pLink->m_iDestID == m_nSrcID ))   )
		{
			pLink->m_pDynamicLink = this;
			if (m_nLinkState == LINK_OFF)
			{
				pLink->m_LinkInfo |=  bits_LINK_OFF;
			}
			else
			{
				pLink->m_LinkInfo &= ~bits_LINK_OFF;
			}
			bLinkFormed = true;
			break;
		}
	}

	if (!bLinkFormed)
	{
		Msg("Error: info_node_link unable to form between nodes %d and %d\n", m_nSrcID, m_nDestID );
	}
}

//------------------------------------------------------------------------------
// Purpose : Given two node ID's return the related dynamic link if any or NULL
//			 	
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_DynamicLink* CAI_DynamicLink::GetDynamicLink(int nSrcID, int nDstID)
{
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		if ((nSrcID == pDynamicLink->m_nSrcID  && nDstID == pDynamicLink->m_nDestID) ||
			(nSrcID == pDynamicLink->m_nDestID && nDstID == pDynamicLink->m_nSrcID ) ) 
		{
			return pDynamicLink;
		}

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_DynamicLink::CAI_DynamicLink(void)
{
	m_bFixedUpIds		= false;
	m_nSrcID			= NO_NODE;
	m_nDestID			= NO_NODE;
	m_nLinkState		= LINK_OFF;

	// -------------------------------------
	//  Add to linked list of dynamic links
	// -------------------------------------
	m_pNextDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;
	CAI_DynamicLink::m_pAllDynamicLinks = this;
};


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_DynamicLink::~CAI_DynamicLink(void) {

	// ----------------------------------------------
	//  Remove from linked list of all dynamic links
	// ----------------------------------------------
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;
	if (pDynamicLink == this)
	{
		m_pAllDynamicLinks = pDynamicLink->m_pNextDynamicLink;
	}
	else
	{
		while (pDynamicLink)
		{
			if (pDynamicLink->m_pNextDynamicLink == this)
			{
				pDynamicLink->m_pNextDynamicLink = pDynamicLink->m_pNextDynamicLink->m_pNextDynamicLink;
				break;
			}
			pDynamicLink = pDynamicLink->m_pNextDynamicLink;
		}
	}
}

