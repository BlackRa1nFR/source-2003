//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "beam_shared.h"
#include "npc_sscanner_beam.h"
#include "ndebugoverlay.h"
#include "AI_BaseNPC.h"

#define	SHIELD_BEAM_BRIGHTNESS		155.0

//#define SHIELD_DEBUG
#ifdef SHIELD_DEBUG
int nodecount = 0;
int beamcount = 0;
#endif

//#################################################################################
//	>> CShieldBeamNode
//#################################################################################
LINK_ENTITY_TO_CLASS( shieldbeam_node, CShieldBeamNode );

BEGIN_DATADESC( CShieldBeamNode )

	DEFINE_FIELD( CShieldBeamNode, m_pNextNode,			FIELD_CLASSPTR	),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CShieldBeamNode::Spawn(void)
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetGravity(0);
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
	UTIL_SetSize( this, vec3_origin, vec3_origin );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CShieldBeam::CountNodes(void)
{
	int nNumNodes = 0;
	CShieldBeamNode*	pNode		= m_pBeamNode;

	while (pNode)
	{
		nNumNodes++;
		pNode = pNode->m_pNextNode;
	}
	return nNumNodes;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CShieldBeamNode* CShieldBeam::GetNewNode( void )
{

	CShieldBeamNode* pNewNode = (CShieldBeamNode *)CreateEntityByName("shieldbeam_node");	

	if (!pNewNode)
	{
		Msg("WARNING: Failed to create CShieldBeamNode!\n");
		return NULL;
	}

#ifdef SHIELD_DEBUG
		nodecount++;
		Msg("%i[%i] (%i)\n",nodecount,CountNodes(),beamcount);
#endif

	pNewNode->Spawn();

	Vector vStartPos;
	QAngle vStartAng;

	CAI_BaseNPC *pNPC = m_hHead->MyNPCPointer();
	if (pNPC)
	{
		pNPC->GetAttachment(m_nHeadAttachment,vStartPos, vStartAng);
	}
	else
	{
		vStartPos = m_hHead->GetAbsOrigin();
	}
	UTIL_SetOrigin( pNewNode, vStartPos );
	
	Vector vHeadVel;
	m_hHead->GetVelocity( &vHeadVel, NULL );
	pNewNode->SetAbsVelocity( vHeadVel );
	UTIL_Relink( pNewNode );
	pNewNode->m_pNextNode = NULL;

	return pNewNode;
}

//#################################################################################
//	>> CShieldBeam
//#################################################################################

BEGIN_DATADESC( CShieldBeam )

	DEFINE_FIELD( CShieldBeam, m_hHead,				FIELD_EHANDLE ),
	DEFINE_FIELD( CShieldBeam, m_nHeadAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( CShieldBeam, m_hTail,				FIELD_EHANDLE ),
	DEFINE_FIELD( CShieldBeam, m_vTailOffset,		FIELD_VECTOR ),

	DEFINE_FIELD( CShieldBeam, m_fSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( CShieldBeam, m_pBeamNode,			FIELD_CLASSPTR ),
	DEFINE_FIELD( CShieldBeam, m_fNextNodeTime,		FIELD_FLOAT ),
	DEFINE_FIELD( CShieldBeam, m_bKillBeam,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CShieldBeam, m_bReachedTail,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CShieldBeam, m_fBrightness,		FIELD_FLOAT ),
	DEFINE_FIELD( CShieldBeam, m_fNoise,			FIELD_FLOAT ),
	DEFINE_FIELD( CShieldBeam, m_vInitialVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( CShieldBeam, m_pBeam,				FIELD_CLASSPTR ),

	// Function Pointers
	DEFINE_FUNCTION( CShieldBeam, ShieldBeamThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( shield_beam, CShieldBeam );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  : Return true if sprite has reached it's destination
//------------------------------------------------------------------------------
bool CShieldBeam::UpdateShieldNode( CShieldBeamNode* pNode, Vector &vTargetPos, float fInterval )
{
	if (m_bReachedTail && !pNode->m_pNextNode)
	{
		pNode->SetLocalOrigin( m_hTail->GetLocalOrigin() + m_vTailOffset );
		return true;
	}
	
	// Averate of tail position and target position
	Vector fTargetDir		= vTargetPos - pNode->GetLocalOrigin();
	float fDistance			= fTargetDir.Length();

	if (fDistance < 5)
	{
		pNode->SetLocalOrigin( m_hTail->GetLocalOrigin() + m_vTailOffset );
		return true;
	}
	fTargetDir				= fTargetDir * (1/fDistance);

	Vector vVelocity		= (pNode->GetAbsVelocity() * 0.6) + (fTargetDir * m_fSpeed * 0.4);

	if (vVelocity.Length() > 0)
	{
		float fTravelDist = ( vVelocity * fInterval).Length();

		if ( fTravelDist > fDistance)
		{
			pNode->SetAbsVelocity( vec3_origin );
			pNode->SetLocalOrigin( vTargetPos );
			return true;
		}
		else
		{
			pNode->SetAbsVelocity( vVelocity );
			return false;
		}
	}
	else
	{
		pNode->SetAbsVelocity( vec3_origin );
		return true;
	}
}

//------------------------------------------------------------------------------
// Purpose : How much time has passed since last think
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CShieldBeam::ThinkInterval( void )
{
	float fInterval = gpGlobals->curtime - m_flAnimTime;
	if (fInterval <= 0.001)
	{
		m_flAnimTime = gpGlobals->curtime;
		fInterval = 0.0;
	}
	else if (!(float)m_flAnimTime)
	{
		fInterval = 0.0;
	}
	m_flAnimTime = gpGlobals->curtime;

	return fInterval;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CShieldBeam::UpdateBeam( void )
{
	// ------------------------------------
	//	Send new node positions to the beam
	// ------------------------------------
	if (m_pBeam && m_pBeamNode)
	{
		Vector			 vEndPos;

		CBaseEntity* entArray[MAX_BEAM_ENTS];
		int attArray[MAX_BEAM_ENTS];
		int counter = 0;

		if (!m_bKillBeam)
		{
			entArray[counter] = m_hHead;
			attArray[counter] = m_nHeadAttachment;
			counter++;
		}

		CShieldBeamNode* pNode = m_pBeamNode;
		while (pNode && counter < MAX_BEAM_ENTS-1)
		{
			entArray[counter] = pNode;
			attArray[counter] = 0;
			counter++;
			pNode		= pNode->m_pNextNode;
		}

		if (counter >= 2)
		{
			m_pBeam->SplineInit(counter, entArray, attArray);
		}
		
		// ----------------------
		//  Set brightness
		// ----------------------
		int nBeamBrightness = m_fBrightness * SHIELD_BEAM_BRIGHTNESS;
		m_pBeam->SetBrightness(nBeamBrightness);
		m_pBeam->SetNoise(m_fNoise);

#ifdef SHIELD_DEBUG2
		for (int i=0;i<counter;i++)
		{
			edict_t* pEntity = INDEXENT( entArray[i] );
			NDebugOverlay::Cross3D(pEntity->origin,Vector(-5,-5,-5),Vector(5,5,5),255,255,255,false,0.0);
		}
#endif
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CShieldBeam::ShieldBeamThink( void )
{
	float fInterval	= ThinkInterval( );
	
	// -----------------------------------------------------
	//  If head or tail entity disappears, kill me
	// -----------------------------------------------------
	if (m_hTail == NULL || m_hHead == NULL)
	{
		m_bKillBeam = true;
	}

	
	// -----------------------------------------------------
	//  Adjust brightness
	// -----------------------------------------------------
	if (m_bKillBeam)
	{
		m_fBrightness -= 0.1;
		if (m_fBrightness < 0)
		{
			m_fBrightness = 0.0;
		}
	}
	else
	{
		m_fBrightness += 0.05;
		if (m_fBrightness > 1)
		{
			m_fBrightness = 1.0;
		}
	}
	

	// -----------------------------------------------------
	//  If I'm the only node and beam is being
	//  killed remove the final node and beam
	// -----------------------------------------------------
	if ( m_bKillBeam && 
		 m_pBeamNode &&
		!m_pBeamNode->m_pNextNode)
	{
		UTIL_Remove(m_pBeamNode);
		m_pBeamNode = NULL;

		UTIL_Remove(m_pBeam);

#ifdef SHIELD_DEBUG
		beamcount--;
		nodecount--;
		Msg("%i[%i] (%i)\n",nodecount,CountNodes(),beamcount);
#endif

		return;
	}

	// -----------------------------------------------
	//  Update position of all beam nodes
	// -----------------------------------------------
	CShieldBeamNode* pNode = m_pBeamNode;
	while (pNode)
	{
		// -----------------------------------------------------
		//  If I'm the last node move towards the tail entity
		//  if I still have one, or just stand still
		// -----------------------------------------------------
		if (!pNode->m_pNextNode)
		{
			Vector vTargetPos;
			if (m_hTail != NULL)
			{
				vTargetPos = m_hTail->GetLocalOrigin() + m_vTailOffset;
			}
			else
			{
				vTargetPos = pNode->GetLocalOrigin();
			}

			if (UpdateShieldNode(pNode,vTargetPos,fInterval))
			{
				m_bReachedTail = true;
			}
		}
		// --------------------------------------------------
		//  Otherwise move toward the next node and remove
		//  next node if I reach it
		// --------------------------------------------------
		else
		{
			Vector vTargetPos	= pNode->m_pNextNode->GetLocalOrigin();

			// Average with tail if it still exists
			if (m_hTail != NULL)
			{
				vTargetPos		= 0.5 * (vTargetPos + (m_hTail->GetLocalOrigin() + m_vTailOffset));
			}

			if (UpdateShieldNode(pNode,vTargetPos,fInterval))
			{
				// ------------------------------------------
				//  If there is a segment after me
				// ------------------------------------------
				if (pNode->m_pNextNode)
				{
					CShieldBeamNode* pNextSegment = pNode->m_pNextNode->m_pNextNode;
					CShieldBeamNode* pLastSegment = pNode->m_pNextNode;

					// --------------------------------------
					// Update my pointer to the next segment
					// --------------------------------------
					pNode->m_pNextNode = pNextSegment;

					// ------------------------------------------------
					//  Attach the start of the next beam to my sprite
					// ------------------------------------------------
					if (!pNextSegment)
					{
						m_bReachedTail = true;
					}

					// ------------------------
					//  Remove the old segment
					// ------------------------
					UTIL_Remove(pLastSegment);

#ifdef SHIELD_DEBUG
		nodecount--;
		Msg("%i[%i] (%i)\n",nodecount,CountNodes(),beamcount);
#endif
				}
				else
				{
					pNode->m_pNextNode = NULL;
				}
			}
		}

		pNode = pNode->m_pNextNode;
	}

	// -----------------------------------------------------
	// Check if its time to create another chase sprite
	// unless I'm killing the beam
	// -----------------------------------------------------
	if (!m_bKillBeam	&& 
		m_hHead != NULL &&
		gpGlobals->curtime > m_fNextNodeTime)
	{
		m_fNextNodeTime = gpGlobals->curtime + 0.2;

		// -----------------------------------------------------
		// Create new segment and point it at the first segment
		// -----------------------------------------------------
		CShieldBeamNode* pNewNode		= GetNewNode();

		if (pNewNode)
		{
			pNewNode->m_pNextNode	= m_pBeamNode;

			// ------------------------------------------------
			// Detach first beam from source entity and attach
			// to beam sprite of new segment
			// ------------------------------------------------
			if (m_pBeamNode)
			{
				pNewNode->SetAbsVelocity( m_pBeamNode->GetAbsVelocity() );
			}
			else
			{
				pNewNode->SetAbsVelocity( m_vInitialVelocity );
			}
			// -----------------------------------------------
			//  Point to the new segment
			// -----------------------------------------------
			m_pBeamNode = pNewNode;
		}
	}
	UpdateBeam();

	SetNextThink( gpGlobals->curtime );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CShieldBeam::SetNoise( float fNoise )
{
	if (fNoise > 50)
	{
		m_fNoise = 50;
	}
	else
	{
		m_fNoise = fNoise;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CShieldBeam::ShieldBeamStop( void )
{
	// Check if I'm already killing the beam
	if (m_bKillBeam)
	{
		return;
	}

	m_bKillBeam = true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CShieldBeam::IsShieldBeamOn( void )
{
	if (m_pBeamNode)
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CShieldBeam::ReachedTail( void )
{
	return m_bReachedTail;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CShieldBeam::ShieldBeamStart(  CBaseEntity* pHeadEntity, CBaseEntity*	pTailEntity,  const Vector &vInitialVelocity, float speed )
{
	// If beam is still running reject start
	if (m_pBeamNode)
	{
		return;
	}

	m_hHead				= pHeadEntity;
	m_hTail				= pTailEntity;
	m_fSpeed			= speed;
	m_fBrightness		= 0;
	m_vInitialVelocity	= vInitialVelocity;

	// -------------
	// Create beam 
	// -------------
	m_pBeam = CBeam::BeamCreate( "sprites/physbeam.vmt", 1 );//<<TEMP>>temp art
	m_pBeam->SetColor( 255, 255, 255 );
	m_pBeam->SetBrightness( 255 );
	m_pBeam->SetNoise( 0 );
	m_pBeam->SetWidth(1);
	m_pBeam->SetEndWidth(20);
	m_pBeam->SetHaloScale(13);

#ifdef SHIELD_DEBUG
	beamcount++;
	Msg("%i[%i] (%i)\n",nodecount,CountNodes(),beamcount);
#endif

	// ------------------------
	//  Throw out a first node
	// ------------------------
	m_pBeamNode	= GetNewNode();
	if (m_pBeamNode)
	{
		m_pBeamNode->SetAbsVelocity( m_vInitialVelocity );
	}
	else
	{
		m_bKillBeam = true;
		UTIL_Remove(m_pBeam);

#ifdef SHIELD_DEBUG
		beamcount--;
		Msg("%i[%i] (%i)\n",nodecount,CountNodes(),beamcount);
#endif
	}

	SetThink( ShieldBeamThink );
	SetNextThink( gpGlobals->curtime );
	m_bKillBeam						= false;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CShieldBeam *CShieldBeam::ShieldBeamCreate(CBaseEntity* pHeadEntity, int nAttachment)
{
	CShieldBeam *pShieldBeam					= CREATE_ENTITY( CShieldBeam, "shield_beam" );
	pShieldBeam->m_pBeamNode					= NULL;
	pShieldBeam->m_hHead						= pHeadEntity;
	pShieldBeam->m_nHeadAttachment				= nAttachment;
	pShieldBeam->SetNextThink( gpGlobals->curtime );
	pShieldBeam->SetThink(ShieldBeamThink);
	pShieldBeam->m_bKillBeam					= false;
	
	return pShieldBeam;
}
