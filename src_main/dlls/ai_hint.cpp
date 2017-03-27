//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Hint node utilities and functions
//
// $NoKeywords: $
//=============================================================================

// @TODO (toml 03-04-03): there is far too much duplicate code in here

#include "cbase.h"
#include "ai_hint.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_basenpc.h"
#include "ai_networkmanager.h"
#include "ndebugoverlay.h"
#include "animation.h"
#include "vstdlib/strtools.h"

//==================================================
// CHintCriteria
//==================================================

CHintCriteria::CHintCriteria( void )
{
	m_iHintType		= HINT_NONE; 
	m_strGroup		= NULL_STRING;
	m_iFlags		= 0;
}

//---------------------------------------------------------------------------`--
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHintCriteria::~CHintCriteria( void )
{
	m_zoneInclude.Purge();
	m_zoneExclude.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the hint type for this search criteria
// Input  : nHintType - the hint type for this search criteria
//-----------------------------------------------------------------------------
void CHintCriteria::SetHintType( int nHintType )
{
	m_iHintType = nHintType;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bitmask - 
//-----------------------------------------------------------------------------
void CHintCriteria::SetFlag( int bitmask )
{
	m_iFlags |= bitmask;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : group - 
//-----------------------------------------------------------------------------
void CHintCriteria::SetGroup( string_t group )
{
	m_strGroup = group;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a zone to a zone list
// Input  : list - the list of zones to add the new zone to
//			&position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddZone( zoneList_t &list, const Vector &position, float radius )
{
	int id = list.AddToTail();
	list[id].position	= position;
	list[id].radius		= radius;
}

//-----------------------------------------------------------------------------
// Purpose: Adds an include zone to the search criteria
// Input  : &position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddIncludePosition( const Vector &position, float radius )
{
	AddZone( m_zoneInclude, position, radius );
}

//-----------------------------------------------------------------------------
// Purpose: Adds an exclude zone to the search criteria
// Input  : &position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddExcludePosition( const Vector &position, float radius )
{
	AddZone( m_zoneExclude, position, radius );
}

//-----------------------------------------------------------------------------
// Purpose: Test to see if this position falls within any of the zones in the list
// Input  : *zone - list of zones to test against
//			&testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CHintCriteria::InZone( zoneList_t &zone, const Vector &testPosition )
{
	int	numZones = zone.Count();

	//Iterate through all zones in the list
	for ( int i = 0; i < numZones; i++ )
	{
		if ( ((zone[i].position) - testPosition).LengthSqr() < (zone[i].radius*zone[i].radius) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if a point within our include list
// Input  : &testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHintCriteria::InIncludedZone( const Vector &testPosition )
{
	return InZone( m_zoneInclude, testPosition );
}

//-----------------------------------------------------------------------------
// Purpose: Determine if a point within our exclude list
// Input  : &testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHintCriteria::InExcludedZone( const Vector &testPosition )
{
	return InZone( m_zoneExclude, testPosition );
}

//##################################################################
// > CAI_Hint
//##################################################################
LINK_ENTITY_TO_CLASS( ai_hint, CAI_Hint );

BEGIN_DATADESC( CAI_Hint )

	//							m_pNextHint
	DEFINE_KEYFIELD( CAI_Hint,	m_iDisabled,		FIELD_INTEGER,	"StartHintDisabled" ),
	DEFINE_KEYFIELD( CAI_Hint,	m_nHintType,		FIELD_INTEGER,	"HintType" ),
//	DEFINE_KEYFIELD( CAI_Hint,  m_flRadius,			FIELD_FLOAT,	"Radius"),		// UNDONE
	DEFINE_KEYFIELD( CAI_Hint,	m_strGroup,			FIELD_STRING,	"Group" ),

	DEFINE_FIELD(	 CAI_Hint,	m_hHintOwner,		FIELD_EHANDLE),
	DEFINE_FIELD(	 CAI_Hint,  m_flNextUseTime,	FIELD_TIME),
	DEFINE_FIELD(	 CAI_Hint,  m_nNodeID,			FIELD_INTEGER),

	// Inputs
	DEFINE_INPUTFUNC( CAI_Hint,	FIELD_VOID,		"EnableHint",		InputEnableHint ),
	DEFINE_INPUTFUNC( CAI_Hint,	FIELD_VOID,		"DisableHint",		InputDisableHint ),

	// Outputs
	DEFINE_OUTPUT( CAI_Hint,	m_OnNPCArrival,	"OnNPCArrival" ),

END_DATADESC( );

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_Hint*	CAI_Hint::m_pAllHints		= NULL;
CAI_Hint*	CAI_Hint::m_pLastFoundHint	= NULL;


//------------------------------------------------------------------------------
// Purpose : 
//------------------------------------------------------------------------------
void CAI_Hint::InputEnableHint( inputdata_t &inputdata )
{
	m_iDisabled		= false;
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAI_Hint::InputDisableHint( inputdata_t &inputdata )
{
	m_iDisabled		= true;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_Hint::Spawn( void )
{
	SetSolid( SOLID_NONE );
	Relink();
}

//------------------------------------------------------------------------------
// Purpose :  If connected to a node returns node position, otherwise
//			  returns local hint position
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_Hint::GetPosition(CBaseCombatCharacter *pBCC, Vector *vPosition)
{
	if ( m_nNodeID != NO_NODE )
	{
		*vPosition = g_pBigAINet->GetNodePosition( pBCC, m_nNodeID );
	}
	else
	{
		*vPosition = GetAbsOrigin();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hull - 
//			*vPosition - 
//-----------------------------------------------------------------------------
void CAI_Hint::GetPosition( Hull_t hull, Vector *vPosition )
{
	if ( m_nNodeID != NO_NODE )
	{
		*vPosition = g_pBigAINet->GetNodePosition( hull, m_nNodeID );
	}
	else
	{
		*vPosition = GetAbsOrigin();
	}
}

//------------------------------------------------------------------------------
// Purpose :  If connected to a node returns node direction, otherwise
//			  returns local hint direction
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_Hint::GetDirection( )
{
	return UTIL_YawToVector( Yaw() );
}

//------------------------------------------------------------------------------
// Purpose :  If connected to a node returns node yaw, otherwise
//			  returns local hint yaw
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CAI_Hint::Yaw(void)
{
	if (m_nNodeID != NO_NODE)
	{
		return g_pBigAINet->GetNodeYaw(m_nNodeID );
	}
	else
	{
		return GetLocalAngles().y;
	}
}



//------------------------------------------------------------------------------
// Purpose :  Returns if this is something that's interesting to look at
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_Hint::IsViewable(void)
{
	if (m_iDisabled)
	{
		return false;
	}

	switch( m_nHintType )
	{
	case HINT_WORLD_VISUALLY_INTERESTING:
		return true;
	}
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Locks the node for use by an AI for hints
// Output : Returns true if the node was available for locking, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Hint::Lock( CBaseEntity* pNPC )
{
	if ( m_hHintOwner != pNPC && m_hHintOwner != NULL )
		return false;
	m_hHintOwner = pNPC;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the node, making it available for hint use by other AIs.
//			after the given delay time
//-----------------------------------------------------------------------------
void CAI_Hint::Unlock( float delay )
{
	m_hHintOwner	= NULL;
	m_flNextUseTime = gpGlobals->curtime + delay;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true is hint node is open for use
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Hint::IsLockedBy(  CBaseEntity *pNPC )
{
	return (m_hHintOwner == pNPC);
};

//-----------------------------------------------------------------------------
// Purpose: Returns true is hint node is open for use
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Hint::IsLocked( void )
{
	if (m_iDisabled)
	{
		return true;
	}

	if (gpGlobals->curtime < m_flNextUseTime)
	{
		return true;
	}
	
	if (m_hHintOwner != NULL)
	{
		return true;
	}
	return false;
};

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_Hint::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"%i", m_nHintType);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
		Q_snprintf(tempstr,sizeof(tempstr),"delay %f", max( 0.0f, m_flNextUseTime - gpGlobals->curtime ) ) ;
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a random hint within the requested radious of the npc
//  Builds a list of all suitable hints and chooses randomly from amongst them.
// Input  : *pNPC - 
//			nHintType - 
//			nFlags - 
//			flMaxDist - 
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_Hint::FindHintRandom( CAI_BaseNPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom )
{
	// -------------------------------------------
	//  If we have no hints, bail
	// -------------------------------------------
	if (!CAI_Hint::m_pAllHints)
	{
		return NULL;
	}

	float flDistSquared = flMaxDist*flMaxDist;

	CUtlVector < CAI_Hint * > hintList;
	
	if ( pMaxDistFrom == NULL )
		pMaxDistFrom = &pNPC->GetAbsOrigin();

	// -----------------------------------------------------------
	//  Now loop till we find a valid hint or return to the start
	// -----------------------------------------------------------
	CAI_Hint *pTestHint;
	for ( pTestHint = CAI_Hint::m_pAllHints; pTestHint; pTestHint = pTestHint->m_pNextHint )
	{
		if (!pTestHint->IsLocked())
		{
			//If we're specifying the hint type, validate it
			if ( ( nHintType == HINT_NONE ) || ( pTestHint->m_nHintType == nHintType ) )
			{
				// Make sure hint is allowed distance away
				if ((pTestHint->GetAbsOrigin() - *pMaxDistFrom).LengthSqr() < flDistSquared )
				{
					// Take it if the NPC likes it and the NPC has an animation to match the hint's activity.
					if (pNPC->FValidateHintType(pTestHint) )
					{
						// Check for visibility if requested
						if (nFlags & bits_HINT_NODE_VISIBLE)
						{
							trace_t tr;
							AI_TraceLine ( pNPC->EyePosition(), pTestHint->GetAbsOrigin() + pNPC->GetViewOffset(), 
								MASK_NPCSOLID_BRUSHONLY, pNPC, COLLISION_GROUP_NONE, &tr );

							if ( tr.fraction == 1.0 )
							{
								hintList.AddToTail( pTestHint );
							}
						}
						else 
						{
							hintList.AddToTail( pTestHint );
						}
					}
				}
			}
		}
	}

	if ( hintList.Size() <= 0 )
	{
		// start at the top of the list for the next search
		m_pLastFoundHint = NULL;
		return NULL;
	}

	// Pick one randomly
	int index = random->RandomInt( 0, hintList.Size() - 1 );
	m_pLastFoundHint = hintList[ index ];
	return m_pLastFoundHint;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pHintCriteria - 
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_Hint::FindHint( CAI_BaseNPC *pNPC, const Vector &position, CHintCriteria *pHintCriteria )
{
	// -------------------------------------------
	//  If we have no hints, bail
	// -------------------------------------------
	if (!CAI_Hint::m_pAllHints)
		return NULL;

	// -------------------------------------------
	// Start with hint after the last one used
	// -------------------------------------------
	CAI_Hint *pTestHint = NULL;
	if (m_pLastFoundHint)
	{
		pTestHint = m_pLastFoundHint->m_pNextHint;
	}
	if (pTestHint == NULL)
	{
		pTestHint = CAI_Hint::m_pAllHints;
	}

	// -----------------------------------------------------------
	//  Now loop till we find a valid hint or return to the start
	// -----------------------------------------------------------
	unsigned long	bestDistance	= (unsigned long) -1;
	unsigned long	distance		= (unsigned long) -1;
	CAI_Hint*		pBestHint		= NULL;
	do
	{
		//Cannot be locked
		if ( pTestHint->IsLocked() )
		{
			goto NextHint;
		}

		//See if we're trying to filter the nodes
		if ( pHintCriteria->GetHintType() != HINT_ANY && pTestHint->HintType() != pHintCriteria->GetHintType() )
		{
			goto NextHint;
		}

		//See if we're filtering by group name
		if ( ( ( pHintCriteria->GetGroup() != NULL_STRING ) && ( pTestHint->GetGroup() != NULL_STRING ) ) && ( strcmp( STRING(pTestHint->GetGroup()), STRING(pHintCriteria->GetGroup()) ) ) )
		{
			goto NextHint;
		}

		//If we're watching for include zones, test it
		if ( ( pHintCriteria->HasIncludeZones() ) && ( pHintCriteria->InIncludedZone( pTestHint->GetAbsOrigin() ) == false ) )
		{
			goto NextHint;
		}
		
		//If we're watching for exclude zones, test it
		if ( ( pHintCriteria->HasExcludeZones() ) && ( pHintCriteria->InExcludedZone( pTestHint->GetAbsOrigin() ) ) )
		{
			goto NextHint;
		}

		// See if the class handles this hint type
		if ( ( pNPC != NULL ) && ( pNPC->FValidateHintType( pTestHint ) == false ) )
		{
			goto NextHint;
		}

		//See if we're requesting a visible node
		if ( pHintCriteria->HasFlag( bits_HINT_NODE_VISIBLE ) )
		{
			if ( pNPC == NULL )
			{
				//NOTENOTE: If you're hitting this, you've asked for a visible node without specifing an NPC!
				AssertMsg( 0, "Hint node attempted to find visible node without specifying NPC!\n" );
			}
			else
			{
				trace_t tr;
				Vector vHintPos;
				pTestHint->GetPosition(pNPC,&vHintPos);
				AI_TraceLine ( pNPC->EyePosition(), vHintPos + pNPC->GetViewOffset(), MASK_NPCSOLID_BRUSHONLY, pNPC, COLLISION_GROUP_NONE, &tr );

				if ( tr.fraction != 1.0f )
				{
					goto NextHint;
				}
			}
		}

		//See if this is our next, closest node
		if ( pHintCriteria->HasFlag( bits_HINT_NODE_NEAREST ) )
		{
			//Calculate our distance
			distance = (pTestHint->GetAbsOrigin() - position).Length();

			//Must be closer than the current best
			if ( distance < bestDistance )
			{
				pBestHint	 = pTestHint;
				bestDistance = distance;
			}
		}
		else //If we're not looking for the nearest, we're done
		{
			m_pLastFoundHint = pTestHint; 
			return pTestHint;		
		}


NextHint:

		// Get the next hint
		pTestHint = pTestHint->m_pNextHint;

		// ------------------------------------------------------------------
		// If I've reached the end, go to the start if I began in the middle 
		// ------------------------------------------------------------------
		if (!pTestHint && m_pLastFoundHint)
		{
			pTestHint = CAI_Hint::m_pAllHints;
		}
	} while (pTestHint != m_pLastFoundHint);

	m_pLastFoundHint = pBestHint; 
	return pBestHint;
}

//-----------------------------------------------------------------------------
// Purpose: Searches for a hint node that this NPC cares about. If one is
//			claims that hint node for this NPC so that no other NPCs
//			try to use it.
//
// Input  : nFlags - Search criterea. Can currently be one or more of the following:
//				bits_HINT_NODE_VISIBLE - searches for visible hint nodes.
//				bits_HINT_NODE_RANDOM - calls through the FindHintRandom and builds list of all matching
//				nodes and picks randomly from among them.  Note:  Depending on number of hint nodes, this
//				could be slower, so use with care.
//
// Output : Returns pointer to hint node if available hint node was found that matches the
//			given criterea that this NPC also cares about. Otherwise, returns NULL
//-----------------------------------------------------------------------------
CAI_Hint* CAI_Hint::FindHint( CAI_BaseNPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom )
{
	assert( pNPC != NULL );
	if ( pNPC == NULL )
		return NULL;

	// If asking for a random node, use random logic instead
	if ( nFlags & bits_HINT_NODE_RANDOM )
		return FindHintRandom( pNPC, nHintType, nFlags, flMaxDist );

	CHintCriteria	hintCriteria;

	hintCriteria.SetHintType( nHintType );
	hintCriteria.SetFlag( nFlags );

	Vector	vecPosition = ( pMaxDistFrom != NULL ) ? (*pMaxDistFrom) : pNPC->GetAbsOrigin();

	hintCriteria.AddIncludePosition( vecPosition, flMaxDist );

	return FindHint( pNPC, vecPosition, &hintCriteria );
}

//-----------------------------------------------------------------------------
// Purpose: Position only search
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_Hint::FindHint( const Vector &position, CHintCriteria *pHintCriteria )
{
	return FindHint( NULL, position, pHintCriteria );
}

//-----------------------------------------------------------------------------
// Purpose: NPC only search
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_Hint::FindHint( CAI_BaseNPC *pNPC, CHintCriteria *pHintCriteria )
{
	assert( pNPC != NULL );
	if ( pNPC == NULL )
		return NULL;

	return FindHint( pNPC, pNPC->GetAbsOrigin(), pHintCriteria );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Hint::CAI_Hint(void)
{
	m_flNextUseTime	= 0;

	// ---------------------------------
	//  Add to linked list of hints
	// ---------------------------------
	m_pNextHint = CAI_Hint::m_pAllHints;
	CAI_Hint::m_pAllHints = this;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_Hint* CAI_Hint::CreateHint( string_t nName, const Vector &pos, Hint_e nHintType, int nNodeID, string_t strGroup )
{
	// Reset last found hint if new node is added
	m_pLastFoundHint = 0;

	CAI_Hint *pHint = (CAI_Hint*)CreateEntityByName("ai_hint");
	if(pHint)
	{	
		pHint->SetName( nName );
		pHint->m_nNodeID	= nNodeID;
		pHint->m_nHintType	= nHintType;
		pHint->SetLocalOrigin( pos );
		pHint->m_strGroup	= strGroup;
		pHint->Spawn();
		return pHint;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Hint::~CAI_Hint(void)
{
	// --------------------------------------
	//  Remove from linked list of hints
	// --------------------------------------
	CAI_Hint *pHint = CAI_Hint::m_pAllHints;
	if (pHint == this)
	{
		CAI_Hint::m_pAllHints = pHint->m_pNextHint;
	}
	else
	{
		while (pHint)
		{
			if (pHint->m_pNextHint == this)
			{
				pHint->m_pNextHint = pHint->m_pNextHint->m_pNextHint;
				break;
			}
			pHint = pHint->m_pNextHint;
		}
	}

	if ( m_pLastFoundHint == this )
		m_pLastFoundHint = NULL;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_Hint::DrawHintOverlays(float flDrawDuration)
{

	CAI_Hint *pHint = CAI_Hint::m_pAllHints;
	while (pHint)
	{
		int		r		= 0;
		int		g		= 0;
		int		b		= 255;
		Vector	vHintPos;

		if (pHint->m_nNodeID != NO_NODE)
		{
			vHintPos = g_pBigAINet->GetNode(pHint->m_nNodeID)->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
		}
		else
		{
			vHintPos = pHint->GetAbsOrigin();
		}
		// If node is currently locked
		if (pHint->m_iDisabled)
		{
			r = 100;
			g = 100;
			b = 100;
		}
		else if (pHint->m_hHintOwner != NULL)
		{
			r = 255;
			g = 0;
			b = 0;

			CBaseEntity* pOwner = pHint->User();
			if (pOwner)
			{
				char owner[255];
				Q_strncpy(owner,pOwner->GetDebugName(),sizeof(owner));
				Vector loc = vHintPos;
				loc.x+=6;
				loc.y+=6;
				loc.z+=6;
				NDebugOverlay::Text( loc, owner, true, flDrawDuration );
			}
		}
		else if (pHint->IsLocked())
		{
			r = 200;
			g = 150;
			b = 10;
		}

		NDebugOverlay::Box(vHintPos, Vector(-3,-3,-3), Vector(3,3,3), r,g,b,0,flDrawDuration);

		// Draw line in facing direction
		Vector offsetDir	= 12.0 * Vector(cos(DEG2RAD(pHint->Yaw())),sin(DEG2RAD(pHint->Yaw())),0);
		NDebugOverlay::Line(vHintPos, vHintPos+offsetDir, r,g,b,false,flDrawDuration);

		// Get the next hint
		pHint = pHint->m_pNextHint;	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sometimes FValidateHint, etc. will want to examine the underlying node to 
//  see if it's truly suitable ( e.g., in the same air/ground network of nodes? )
// Output : C_AINode *
//-----------------------------------------------------------------------------
CAI_Node *CAI_Hint::GetNode( void )
{
	if ( m_nNodeID != NO_NODE )
	{
		return g_pBigAINet->GetNode( m_nNodeID );
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *token - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_Hint::GetFlags( const char *token )
{
	int len = strlen( token );
	if ( len <= 0 )
	{
		return bits_HINT_NODE_NONE;
	}

	char *lowercase = (char *)_alloca( len + 1 );
	Q_strncpy( lowercase, token, len+1 );
	strlwr( lowercase );

	if ( strstr( "none", lowercase ) )
	{
		return bits_HINT_NODE_NONE;
	}

	int bits = 0;

	if ( strstr( "visible", lowercase ) )
	{
		bits |= bits_HINT_NODE_VISIBLE;
	}

	if ( strstr( "nearest", lowercase ) )
	{
		bits |= bits_HINT_NODE_NEAREST;
	}

	if ( strstr( "random", lowercase ) )
	{
		bits |= bits_HINT_NODE_RANDOM;
	}

	// Can't be nearest and random, defer to nearest
	if ( ( bits & bits_HINT_NODE_NEAREST ) &&
		 ( bits & bits_HINT_NODE_RANDOM ) )
	{
		// Remove random
		bits &= ~bits_HINT_NODE_RANDOM;

		Msg( "HINTFLAGS:%s, inconsistent, the nearest node is never a random hint node, treating as nearest request!\n",
			token );
	}

	return bits;
}
