/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "cbase.h"
#include "studio.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "ai_activity.h"
#include "animation.h"
#include "bone_setup.h"
#include "util.h"
#include "scriptevent.h"
#include "npcevent.h"

#if !defined( CLIENT_DLL )
#include "enginecallback.h"
#endif

#pragma warning( disable : 4244 )
#define iabs(i) (( (i) >= 0 ) ? (i) : -(i) )

int ExtractBbox( studiohdr_t *pstudiohdr, int sequence, Vector& mins, Vector& maxs )
{
	if (! pstudiohdr)
		return 0;

	mstudioseqdesc_t	*pseqdesc = pstudiohdr->pSeqdesc( sequence );
	
	mins[0] = pseqdesc->bbmin[0];
	mins[1] = pseqdesc->bbmin[1];
	mins[2] = pseqdesc->bbmin[2];

	maxs[0] = pseqdesc->bbmax[0];
	maxs[1] = pseqdesc->bbmax[1];
	maxs[2] = pseqdesc->bbmax[2];

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pstudiohdr - 
//			iSequence - 
//
// Output : mstudioseqdesc_t
//-----------------------------------------------------------------------------
mstudioseqdesc_t *GetSequenceDesc( studiohdr_t *pstudiohdr )
{
	mstudioseqdesc_t	*pseqdesc;

	if( !pstudiohdr )
	{
		return NULL;
	}

	pseqdesc = pstudiohdr->pSeqdesc( 0 );

	return pseqdesc;
}

//=========================================================
// IndexModelSequences - set activity indexes for all model
// sequences that have activities.
//=========================================================
void IndexModelSequences( studiohdr_t *pstudiohdr )
{
	int i;
	int iActivityIndex;
	const char *pszActivityName;

	if (! pstudiohdr)
		return;

	mstudioseqdesc_t *pseqdesc;

	pseqdesc = pstudiohdr->pSeqdesc( 0 );
	
	for ( i = 0 ; i < pstudiohdr->numseq ; i++ )
	{
		// look up the activity number, but only for sequences that are assigned activities.
		pszActivityName = GetSequenceActivityName( pstudiohdr, i );
		if ( pszActivityName[0] != '\0' )
		{
			iActivityIndex = ActivityList_IndexForName( pszActivityName );
			
			if ( iActivityIndex == -1 )
			{
				// Allow this now.  Animators can create custom activities that are referenced only on the client or by scripts, etc.
				//Warning( "***\nModel %s tried to reference unregistered activity: %s \n***\n", pstudiohdr->name, pszActivityName );
				//Assert(0);
				pseqdesc[i].activity = ActivityList_RegisterPrivateActivity( pszActivityName );
			}
			else
			{
				pseqdesc[i].activity = iActivityIndex;
			}
		}
	}

	pstudiohdr->sequencesindexed = true;
}

//-----------------------------------------------------------------------------
// Purpose: Ensures that activity / index relationship is recalculated
// Input  :
// Output :
//-----------------------------------------------------------------------------
void ResetActivityIndexes( studiohdr_t *pstudiohdr )
{	
	if (! pstudiohdr)
		return;

	pstudiohdr->sequencesindexed = false;
}

void VerifySequenceIndex( studiohdr_t *pstudiohdr )
{
	if (! pstudiohdr)
		return;

	if( pstudiohdr->sequencesindexed == false )
	{
		// this model's sequences have not yet been indexed by activity
		IndexModelSequences( pstudiohdr );
	}
}

int SelectWeightedSequence( studiohdr_t *pstudiohdr, int activity, int curSequence )
{
	if (! pstudiohdr)
		return 0;

	VerifySequenceIndex( pstudiohdr );

	mstudioseqdesc_t	*pseqdesc = pstudiohdr->pSeqdesc( 0 );

	int weighttotal = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			if ( curSequence == i && pseqdesc[i].actweight < 0 )
			{
				seq = i;
				break;
			}
			weighttotal += iabs(pseqdesc[i].actweight);
			if (!weighttotal || random->RandomInt(0,weighttotal-1) < iabs(pseqdesc[i].actweight))
				seq = i;
		}
	}

	return seq;
}


int SelectHeaviestSequence( studiohdr_t *pstudiohdr, int activity )
{
	if ( !pstudiohdr )
		return 0;

	VerifySequenceIndex( pstudiohdr );

	mstudioseqdesc_t	*pseqdesc = pstudiohdr->pSeqdesc( 0 );

	int weight = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			if ( iabs(pseqdesc[i].actweight) > weight )
			{
				weight = iabs(pseqdesc[i].actweight);
				seq = i;
			}
		}
	}

	return seq;
}

void GetEyePosition ( studiohdr_t *pstudiohdr, Vector &vecEyePosition )
{
	if ( !pstudiohdr )
	{
		Warning( "GetEyePosition() Can't get pstudiohdr ptr!\n" );
		return;
	}

	vecEyePosition = pstudiohdr->eyeposition;
}


//-----------------------------------------------------------------------------
// Purpose: Looks up an activity by name.
// Input  : label - Name of the activity to look up, ie "ACT_IDLE"
// Output : Activity index or ACT_INVALID if not found.
//-----------------------------------------------------------------------------
int LookupActivity( studiohdr_t *pstudiohdr, const char *label )
{
	if ( !pstudiohdr )
	{
		return 0;
	}

	mstudioseqdesc_t *pseqdesc = pstudiohdr->pSeqdesc( 0 );

	for ( int i = 0; i < pstudiohdr->numseq; i++ )
	{
		if ( stricmp( pseqdesc[i].pszActivityName(), label ) == 0 )
		{
			return pseqdesc[i].activity;
		}
	}

	return ACT_INVALID;
}


//-----------------------------------------------------------------------------
// Purpose: Looks up a sequence by sequence name first, then by activity name.
// Input  : label - The sequence name or activity name to look up.
// Output : Returns the sequence index of the matching sequence, or ACT_INVALID.
//-----------------------------------------------------------------------------
int LookupSequence( studiohdr_t *pstudiohdr, const char *label )
{
	if (! pstudiohdr)
		return 0;

	mstudioseqdesc_t	*pseqdesc;

	pseqdesc = pstudiohdr->pSeqdesc( 0 );

	//
	// Look up by sequence name.
	//
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (stricmp( pseqdesc[i].pszLabel(), label ) == 0)
			return i;
	}

	//
	// Not found, look up by activity name.
	//
	int nActivity = LookupActivity( pstudiohdr, label );
	if (nActivity != ACT_INVALID )
	{
		return SelectWeightedSequence( pstudiohdr, nActivity );
	}

	return ACT_INVALID;
}

void GetSequenceLinearMotion( studiohdr_t *pstudiohdr, int iSequence, const float poseParameter[], Vector *pVec )
{
	if (! pstudiohdr)
	{
		Msg( "Bad pstudiohdr in GetSequenceLinearMotion()!\n" );
		return;
	}

	if( iSequence < 0 || iSequence >= pstudiohdr->numseq )
	{
		// Don't spam on bogus model
		if ( pstudiohdr->numseq > 0 )
		{
			Msg( "Bad sequence (%i out of %i max) in GetSequenceLinearMotion() for model '%s'!\n", iSequence, pstudiohdr->numseq, pstudiohdr->name );
		}
		return;
	}

	QAngle vecAngles;
	Studio_SeqMovement( pstudiohdr, iSequence, 0, 1.0, poseParameter, (*pVec), vecAngles );
}


const char *GetSequenceName( studiohdr_t *pstudiohdr, int iSequence )
{
	mstudioseqdesc_t	*pseqdesc;

	if( !pstudiohdr || iSequence < 0 || iSequence >= pstudiohdr->numseq )
	{
		if ( pstudiohdr )
		{
			Msg( "Bad sequence in GetSequenceName() for model '%s'!\n", pstudiohdr->name );
		}
		return "Unknown";
	}

	pseqdesc = pstudiohdr->pSeqdesc( iSequence );
	return pseqdesc->pszLabel();
}

const char *GetSequenceActivityName( studiohdr_t *pstudiohdr, int iSequence )
{
	mstudioseqdesc_t	*pseqdesc;

	if( !pstudiohdr || iSequence < 0 || iSequence >= pstudiohdr->numseq )
	{
		if ( pstudiohdr )
		{
			Msg( "Bad sequence in GetSequenceActivityName() for model '%s'!\n", pstudiohdr->name );
		}
		return "Unknown";
	}

	pseqdesc = pstudiohdr->pSeqdesc( iSequence );
	return pseqdesc->pszActivityName( );
}

int GetSequenceFlags( studiohdr_t *pstudiohdr, int sequence )
{
	if ( !pstudiohdr || 
		sequence < 0 || 
		sequence >= pstudiohdr->numseq )
	{
		return 0;
	}

	mstudioseqdesc_t *pseqdesc = pstudiohdr->pSeqdesc( sequence );

	return pseqdesc->flags;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
//			sequence - 
//			type - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool HasAnimationEventOfType( studiohdr_t *pstudiohdr, int sequence, int type )
{
	if ( !pstudiohdr || sequence >= pstudiohdr->numseq )
		return false;

	mstudioseqdesc_t *pseqdesc = pstudiohdr->pSeqdesc( sequence );
	if ( !pseqdesc )
		return false;

	mstudioevent_t *pevent = pseqdesc->pEvent( 0 );
	if ( !pevent )
		return false;

	if (pseqdesc->numevents == 0 )
		return false;

	int index;
	for ( index = 0; index < pseqdesc->numevents; index++ )
	{
		if ( pevent[ index ].event == type )
		{
			return true;
		}
	}

	return false;
}

int GetAnimationEvent( studiohdr_t *pstudiohdr, int sequence, animevent_t *pNPCEvent, float flStart, float flEnd, int index )
{
	if ( !pstudiohdr || sequence >= pstudiohdr->numseq || !pNPCEvent )
		return 0;

	mstudioseqdesc_t *pseqdesc = pstudiohdr->pSeqdesc( sequence );
	mstudioevent_t *pevent = pseqdesc->pEvent( 0 );

	if (pseqdesc->numevents == 0 || index > pseqdesc->numevents )
		return 0;

	// Msg( "flStart %f flEnd %f (%d) %s\n", flStart, flEnd, pseqdesc->numevents, pseqdesc->label );

	for (; index < pseqdesc->numevents; index++)
	{
		// Don't send client-side events to the server AI
		if ( pevent[index].event >= EVENT_CLIENT )
			continue;

		bool bOverlapEvent = false;

		if (pevent[index].cycle >= flStart && pevent[index].cycle < flEnd)
		{
			bOverlapEvent = true;
		}
		// FIXME: doesn't work with animations being played in reverse
		else if ((pseqdesc->flags & STUDIO_LOOPING) && flEnd < flStart)
		{
			if (pevent[index].cycle >= flStart || pevent[index].cycle < flEnd)
			{
				bOverlapEvent = true;
			}
		}

		if (bOverlapEvent)
		{
			pNPCEvent->pSource = NULL;
			pNPCEvent->cycle = pevent[index].cycle;
			pNPCEvent->eventtime = gpGlobals->curtime;
			pNPCEvent->event = pevent[index].event;
			pNPCEvent->options = pevent[index].options;
			return index + 1;
		}
	}
	return 0;
}



int FindTransitionSequence( studiohdr_t *pstudiohdr, int iCurrentSequence, int iGoalSequence, int *piDir )
{
	if ( !pstudiohdr )
		return iGoalSequence;

	if ( ( iCurrentSequence < 0 ) || ( iCurrentSequence >= pstudiohdr->numseq ) )
		return iGoalSequence;

	mstudioseqdesc_t *pseqdesc = pstudiohdr->pSeqdesc( 0 );

	// bail if we're going to or from a node 0
	if (pseqdesc[iCurrentSequence].entrynode == 0 || pseqdesc[iGoalSequence].entrynode == 0)
	{
		*piDir = 1;
		return iGoalSequence;
	}

	int	iEndNode;

	// Msg( "from %d to %d: ", pEndNode->iEndNode, pGoalNode->iStartNode );

	// check to see if we should be going forward or backward through the graph
	if (*piDir > 0)
	{
		iEndNode = pseqdesc[iCurrentSequence].exitnode;
	}
	else
	{
		iEndNode = pseqdesc[iCurrentSequence].entrynode;
	}

	// if both sequences are on the same node, just go there
	if (iEndNode == pseqdesc[iGoalSequence].entrynode)
	{
		*piDir = 1;
		return iGoalSequence;
	}

	byte *pTransition = pstudiohdr->pTransition( 0 );

	int iInternNode = pTransition[(iEndNode-1)*pstudiohdr->numtransitions + (pseqdesc[iGoalSequence].entrynode-1)];

	// if there is no transitionial node, just go to the goal sequence
	if (iInternNode == 0)
		return iGoalSequence;

	int i;

	// look for someone going from the entry node to next node it should hit
	// this may be the goal sequences node or an intermediate node
	for (i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].entrynode == iEndNode && pseqdesc[i].exitnode == iInternNode)
		{
			*piDir = 1;
			return i;
		}
		if (pseqdesc[i].nodeflags)
		{
			if (pseqdesc[i].exitnode == iEndNode && pseqdesc[i].entrynode == iInternNode)
			{
				*piDir = -1;
				return i;
			}
		}
	}

	// this means that two parts of the node graph are not connected.
	Msg( "error in transition graph" );
	// Go ahead and jump to the goal sequence
	return iGoalSequence;
}

void SetBodygroup( studiohdr_t *pstudiohdr, int& body, int iGroup, int iValue )
{
	if (! pstudiohdr)
		return;

	if (iGroup >= pstudiohdr->numbodyparts)
		return;

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );

	if (iValue >= pbodypart->nummodels)
		return;

	int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;

	body = (body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
}


int GetBodygroup( studiohdr_t *pstudiohdr, int body, int iGroup )
{
	if (! pstudiohdr)
		return 0;

	if (iGroup >= pstudiohdr->numbodyparts)
		return 0;

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );

	if (pbodypart->nummodels <= 1)
		return 0;

	int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;

	return iCurrent;
}

const char *GetBodygroupName( studiohdr_t *pstudiohdr, int iGroup )
{
	if ( !pstudiohdr)
		return "";

	if (iGroup >= pstudiohdr->numbodyparts)
		return "";

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
	return pbodypart->pszName();
}

int FindBodygroupByName( studiohdr_t *pstudiohdr, const char *name )
{
	if ( !pstudiohdr )
		return -1;

	int group;
	for ( group = 0; group < pstudiohdr->numbodyparts; group++ )
	{
		mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( group );
		if ( !Q_strcasecmp( name, pbodypart->pszName() ) )
		{
			return group;
		}
	}

	return -1;
}

int GetBodygroupCount( studiohdr_t *pstudiohdr, int iGroup )
{
	if ( !pstudiohdr )
		return 0;

	if (iGroup >= pstudiohdr->numbodyparts)
		return 0;

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
	return pbodypart->nummodels;
}

int GetNumBodyGroups( studiohdr_t *pstudiohdr )
{
	if ( !pstudiohdr )
		return 0;

	return pstudiohdr->numbodyparts;
}

int GetSequenceActivity( studiohdr_t *pstudiohdr, int sequence )
{
	if (! pstudiohdr)
		return 0;

	return pstudiohdr->pSeqdesc( sequence )->activity;
}


void GetAttachmentLocalSpace( studiohdr_t *pstudiohdr, int attachIndex, matrix3x4_t &pLocalToWorld )
{
	if ( attachIndex >= 0 )
	{
		mstudioattachment_t *pAttachment = pstudiohdr->pAttachment(attachIndex);
		MatrixCopy( pAttachment->local, pLocalToWorld );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
//			*name - 
// Output : int
//-----------------------------------------------------------------------------
int FindHitboxSetByName( studiohdr_t *pstudiohdr, const char *name )
{
	if ( !pstudiohdr )
		return -1;

	for ( int i = 0; i < pstudiohdr->numhitboxsets; i++ )
	{
		mstudiohitboxset_t *set = pstudiohdr->pHitboxSet( i );
		if ( !set )
			continue;

		if ( !stricmp( set->pszName(), name ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
//			setnumber - 
// Output : char const
//-----------------------------------------------------------------------------
const char *GetHitboxSetName( studiohdr_t *pstudiohdr, int setnumber )
{
	if ( !pstudiohdr )
		return "";

	mstudiohitboxset_t *set = pstudiohdr->pHitboxSet( setnumber );
	if ( !set )
		return "";

	return set->pszName();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
// Output : int
//-----------------------------------------------------------------------------
int GetHitboxSetCount( studiohdr_t *pstudiohdr )
{
	if ( !pstudiohdr )
		return 0;

	return pstudiohdr->numhitboxsets;
}
