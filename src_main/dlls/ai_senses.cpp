//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "ai_senses.h"

#include "soundent.h"
#include "team.h"
#include "ai_basenpc.h"

// Use this to disable caching and other optimizations in senses
//#define AI_SENSES_HOMOGENOUS_TREATMENT 1
//#define DEBUG_SENSES 1

#ifdef DEBUG_SENSES
#define AI_PROFILE_SENSES(tag) AI_PROFILE_SCOPE(tag)
#else
#define AI_PROFILE_SENSES(tag) ((void)0)
#endif

//-----------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)

struct AISightIterVal_t
{
	short array;
	short iNext;
};

#pragma pack(pop)

//=============================================================================
//
// CAI_Senses
//
//=============================================================================

BEGIN_SIMPLE_DATADESC( CAI_Senses )

	DEFINE_FIELD( CAI_Senses,		m_LookDist,			FIELD_FLOAT	),
	//								m_LastLookDist
	//								m_TimeLastLook
	//								m_iAudibleList
	//								m_SeenHighPriority
	//								m_SeenNPCs
	//								m_SeenMisc
	//								m_SeenArrays
	//								m_HighPriorityTimer
	//								m_NPCsTimer
	//								m_MiscTimer

END_DATADESC()

//-----------------------------------------------------------------------------

bool CAI_Senses::CanHearSound( CSound *pSound )
{
	if ( pSound->m_hOwner == GetOuter() )
		return false;

	if( GetOuter()->GetState() == NPC_STATE_SCRIPT && pSound->IsSoundType( SOUND_DANGER ) )
	{
		// For now, don't hear danger in scripted sequences. This probably isn't a
		// good long term solution, but it makes the Bank Exterior work better.
		return false;
	}

	// @TODO (toml 10-18-02): what about player sounds and notarget?

	if( ( pSound->GetSoundOrigin() - GetOuter()->EarPosition() ).Length() <= pSound->Volume() * GetOuter()->HearingSensitivity() )
	{
		return GetOuter()->QueryHearSound( pSound );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Listen - npcs dig through the active sound list for
// any sounds that may interest them. (smells, too!)

void CAI_Senses::Listen( void )
{
	m_iAudibleList = SOUNDLIST_EMPTY; 

	int iSoundMask = GetOuter()->GetSoundInterests();
	
	if ( iSoundMask != SOUND_NONE )
	{
		int	iSound = CSoundEnt::ActiveList();
		
		while ( iSound != SOUNDLIST_EMPTY )
		{
			CSound *pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );

			if ( pCurrentSound	&& (iSoundMask & pCurrentSound->SoundType()) && CanHearSound( pCurrentSound ) )
			{
	 			// the npc cares about this sound, and it's close enough to hear.
				pCurrentSound->m_iNextAudible = m_iAudibleList;
				m_iAudibleList = iSound;
			}
			iSound = pCurrentSound->NextSound();
		}
	}
	
	GetOuter()->OnListened();
}

//-----------------------------------------------------------------------------

bool CAI_Senses::ShouldSeeEntity( CBaseEntity *pSightEnt )
{
	if ( pSightEnt == GetOuter() || !pSightEnt->IsAlive() )
		return false;

	if ( pSightEnt->IsPlayer() && ( pSightEnt->GetFlags() & FL_NOTARGET ) )
		return false;

	// don't notice anyone waiting to be seen by the player
	if ( pSightEnt->m_spawnflags & SF_NPC_WAIT_TILL_SEEN )
		return false;

	if ( !pSightEnt->CanBeSeen() )
		return false;
	
	if ( !GetOuter()->QuerySeeEntity( pSightEnt ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Senses::CanSeeEntity( CBaseEntity *pSightEnt )
{
	return ( GetOuter()->FInViewCone( pSightEnt ) && 
			 GetOuter()->FVisible( pSightEnt ) );
}

//-----------------------------------------------------------------------------

bool CAI_Senses::DidSeeEntity( CBaseEntity *pSightEnt ) const
{
	AISightIter_t iter;
	CBaseEntity *pTestEnt;

	pTestEnt = GetFirstSeenEntity( &iter );

	while( pTestEnt )
	{
		if ( pSightEnt == pTestEnt )
			return true;
		pTestEnt = GetNextSeenEntity( &iter );
	}
	return false;
}

//-----------------------------------------------------------------------------

void CAI_Senses::NoteSeenEntity( CBaseEntity *pSightEnt )
{
	pSightEnt->m_pLink = GetOuter()->m_pLink;
	GetOuter()->m_pLink = pSightEnt;
}
		
//-----------------------------------------------------------------------------

bool CAI_Senses::WaitingUntilSeen( CBaseEntity *pSightEnt )
{
	if ( GetOuter()->m_spawnflags & SF_NPC_WAIT_TILL_SEEN )
	{
		if ( pSightEnt->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pSightEnt );
			Vector zero =  Vector(0,0,0);
			// don't link this client in the list if the npc is wait till seen and the player isn't facing the npc
			if ( pPlayer
				// && pPlayer->FVisible( GetOuter() ) 
				&& FBoxVisible( pSightEnt, static_cast<CBaseEntity*>(GetOuter()), zero )
				&& pPlayer->FInViewCone( GetOuter() ) )
			{
				// player sees us, become normal now.
				GetOuter()->m_spawnflags &= ~SF_NPC_WAIT_TILL_SEEN;
				return false;
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Senses::SeeEntity( CBaseEntity *pSightEnt )
{
	GetOuter()->OnSeeEntity( pSightEnt );

	// insert at the head of my sight list
	NoteSeenEntity( pSightEnt );

	return true;
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_Senses::GetFirstSeenEntity( AISightIter_t *pIter ) const
{ 
	COMPILE_TIME_ASSERT( sizeof( AISightIter_t ) == sizeof( AISightIterVal_t ) );
	
	AISightIterVal_t *pIterVal = (AISightIterVal_t *)pIter;
	
	for ( int i = 0; i < ARRAYSIZE( m_SeenArrays ); i++ )
	{
		if ( m_SeenArrays[i]->Count() != 0 )
		{
			pIterVal->array = i;
			pIterVal->iNext = 1;
			return (*m_SeenArrays[i])[0];
		}
	}
	
	(*pIter) = (AISightIter_t)(-1); 
	return NULL;
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_Senses::GetNextSeenEntity( AISightIter_t *pIter ) const	
{ 
	if ( ((int)*pIter) != -1 )
	{
		AISightIterVal_t *pIterVal = (AISightIterVal_t *)pIter;
		
		for ( int i = pIterVal->array;  i < ARRAYSIZE( m_SeenArrays ); i++ )
		{
			for ( int j = pIterVal->iNext; j < m_SeenArrays[i]->Count(); j++ )
			{
				if ( (*m_SeenArrays[i])[j].Get() != NULL )
				{
					pIterVal->array = i;
					pIterVal->iNext = j+1;
					return (*m_SeenArrays[i])[j];
				}
			}
			pIterVal->iNext = 0;
		}
		(*pIter) = (AISightIter_t)(-1); 
	}
	return NULL;
}

//-----------------------------------------------------------------------------

void CAI_Senses::BeginGather()
{
	// clear my sight list
	GetOuter()->m_pLink = NULL;
}

//-----------------------------------------------------------------------------

void CAI_Senses::EndGather( int nSeen, CUtlVector<EHANDLE> *pResult )
{
	pResult->SetCount( nSeen );
	if ( nSeen )
	{
		CBaseEntity *pCurrent = GetOuter()->m_pLink;
		for (int i = 0; i < nSeen; i++ )
		{
			Assert( pCurrent );
			(*pResult)[i].Set( pCurrent );
			pCurrent = pCurrent->m_pLink;
		}
		GetOuter()->m_pLink = NULL;
	}
}

//-----------------------------------------------------------------------------
// Look - Base class npc function to find enemies or 
// food by sight. iDistance is distance ( in units ) that the 
// npc can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink 
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//

void CAI_Senses::Look( int iDistance )
{
	if ( m_TimeLastLook != gpGlobals->curtime || m_LastLookDist != iDistance )
	{
		//-----------------------------
		
		LookForHighPriorityEntities( iDistance );
		LookForNPCs( iDistance);
		LookForObjects( iDistance );
		
		//-----------------------------
		
		m_LastLookDist = iDistance;
		m_TimeLastLook = gpGlobals->curtime;
	}
	
	GetOuter()->OnLooked( iDistance );
}

//-----------------------------------------------------------------------------

bool CAI_Senses::Look( CBaseEntity *pSightEnt )
{
	if ( WaitingUntilSeen( pSightEnt ) )
		return false;
	
	if ( ShouldSeeEntity( pSightEnt ) && CanSeeEntity( pSightEnt ) )
	{
		return SeeEntity( pSightEnt );
	}
	return false;
}

//-----------------------------------------------------------------------------

int CAI_Senses::LookForHighPriorityEntities( int iDistance )
{
#ifndef AI_SENSES_HOMOGENOUS_TREATMENT
	int nSeen = 0;
	if ( m_HighPriorityTimer.Expired() )
	{
		AI_PROFILE_SENSES(CAI_Senses_LookForHighPriorityEntities);
		m_HighPriorityTimer.Reset();
		
		BeginGather();
	
		float distSq = ( iDistance * iDistance );
		const Vector &origin = GetLocalOrigin();
		
		// Players
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( origin.DistToSqr(pPlayer->GetAbsOrigin()) < distSq && Look( pPlayer ) )
				{
					nSeen++;
				}
			}
		}
	
#if OTHER_IMPORTANT_ENTITIES_NOT_BAKED
		// @TODO (toml 10-18-02): for now, not doing this so dont have to deal with
		// enemy or target changing, requiring a fixup of cached data
		
		// Enemy
		if ( GetOuter()->GetEnemy() && !GetOuter()->GetEnemy()->IsPlayer() )
		{
			if ( origin.DistToSqr(GetOuter()->GetEnemy()->GetAbsOrigin()) < distSq && Look( GetOuter()->GetEnemy() ) )
				nSeen++;
		}
		
		// Target
		if ( GetOuter()->GetTarget() && !GetOuter()->GetTarget()->IsPlayer() )
		{
			if ( origin.DistToSqr(GetOuter()->GetTarget()->GetAbsOrigin()) < distSq && Look( GetOuter()->GetTarget() ) )
				nSeen++;
		}
#endif
		EndGather( nSeen, &m_SeenHighPriority );
    }
    else
    {
    	nSeen = m_SeenHighPriority.Count();
    }
	
	return nSeen;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------

int CAI_Senses::LookForNPCs( int iDistance )
{
#ifndef AI_SENSES_HOMOGENOUS_TREATMENT
	int nSeen = 0;
	if ( m_NPCsTimer.Expired() )
	{
		AI_PROFILE_SENSES(CAI_Senses_LookForNPCs);
		m_NPCsTimer.Reset();

		BeginGather();

		float distSq = ( iDistance * iDistance );
		const Vector &origin = GetLocalOrigin();
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		
		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
#if OTHER_IMPORTANT_ENTITIES_NOT_BAKED
			if ( ppAIs[i] != GetOuter()->GetTarget() && ppAIs[i] != GetOuter()->GetEnemy() )
#endif
			{
				if ( origin.DistToSqr(ppAIs[i]->GetAbsOrigin()) < distSq && Look( ppAIs[i] ) )
				{
					nSeen++;
				}
			}
		}

		EndGather( nSeen, &m_SeenNPCs );
	}
    else
    {
    	nSeen = m_SeenNPCs.Count();
    }
	
	return nSeen;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------

int CAI_Senses::LookForObjects( int iDistance )
{
	
#ifndef AI_SENSES_HOMOGENOUS_TREATMENT
	const int BOX_QUERY_MASK = FL_OBJECT;
#else
	const int BOX_QUERY_MASK = ( FL_OBJECT | FL_CLIENT | FL_NPC );
	m_MiscTimer.Force();
#endif

	int	nSeen = 0;
	if ( m_MiscTimer.Expired() )
	{
		AI_PROFILE_SENSES(CAI_Senses_LookForObjects);
		m_MiscTimer.Reset();
		
		BeginGather();
		
		CBaseEntity*	pList[100];
		Vector			delta( iDistance, iDistance, iDistance );

		// Find only npcs/clients in box, NOT limited to PVS
		// HACKHACK there's some hacky stuff here. FL_OBJECT exists so we can find entities 
		// in the search that are not necesssarily NPCs. If we were to flag 
		// these objects as NPCs to get them to show up in this 
		// search, those objects would then touch triggers and lots of other stuff that we don't want. (sjb)
		int count = UTIL_EntitiesInBox( pList, 100, GetOuter()->GetAbsOrigin() - delta, GetOuter()->GetAbsOrigin() + delta, BOX_QUERY_MASK );
		for ( int i = 0; i < count; i++ )
		{
#ifndef AI_SENSES_HOMOGENOUS_TREATMENT
			Assert( !pList[i]->IsPlayer() );
			Assert( !pList[i]->MyNPCPointer() );
#endif
			
#if OTHER_IMPORTANT_ENTITIES_NOT_BAKED
			if ( pList[i] != GetOuter()->GetTarget() && pList[i] != GetOuter()->GetEnemy() )
#endif
			{
				if ( Look( pList[i] ) )
				{
					nSeen++;
				}
			}
		}
		
		EndGather( nSeen, &m_SeenMisc );
	}
    else
    {
    	nSeen = m_SeenMisc.Count();
    }

	return nSeen;
}

//-----------------------------------------------------------------------------

CSound* CAI_Senses::GetFirstHeardSound( AISoundIter_t *pIter )
{
	int iFirst = GetAudibleList(); 

	if ( iFirst == SOUNDLIST_EMPTY )
	{
		*pIter = NULL;
		return NULL;
	}
	
	*pIter = (AISoundIter_t)iFirst;
	return CSoundEnt::SoundPointerForIndex( iFirst );
}

//-----------------------------------------------------------------------------

CSound* CAI_Senses::GetNextHeardSound( AISoundIter_t *pIter )
{
	if ( !*pIter )
		return NULL;

	int iCurrent = (int)*pIter;
	
	Assert( iCurrent != SOUNDLIST_EMPTY );
	if ( iCurrent == SOUNDLIST_EMPTY )
	{
		*pIter = NULL;
		return NULL;
	}
	
	iCurrent = CSoundEnt::SoundPointerForIndex( iCurrent )->m_iNextAudible;
	if ( iCurrent == SOUNDLIST_EMPTY )
	{
		*pIter = NULL;
		return NULL;
	}
	
	*pIter = (AISoundIter_t)iCurrent;
	return CSoundEnt::SoundPointerForIndex( iCurrent );
}

//-----------------------------------------------------------------------------

CSound *CAI_Senses::GetClosestSound( bool fScent )
{
	float flBestDist = MAX_COORD_RANGE*MAX_COORD_RANGE;// so first nearby sound will become best so far.
	float flDist;
	
	AISoundIter_t iter;
	
	CSound *pResult = NULL;
	CSound *pCurrent = GetFirstHeardSound( &iter );

	Vector earPosition = GetOuter()->EarPosition();
	
	while ( pCurrent )
	{
		if ( ( !fScent && pCurrent->FIsSound() ) || 
			 ( fScent && pCurrent->FIsScent() ) )
		{
			flDist = ( pCurrent->GetSoundOrigin() - earPosition ).LengthSqr();

			if ( flDist < flBestDist )
			{
				pResult = pCurrent;
				flBestDist = flDist;
			}
		}
		
		pCurrent = GetNextHeardSound( &iter );
	}
	
	return pResult;
}

//-----------------------------------------------------------------------------

void CAI_Senses::PerformSensing( void )
{
	AI_PROFILE_SCOPE	(CAI_BaseNPC_PerformSensing);
		
	// -----------------
	//  Look	
	// -----------------
	Look( m_LookDist );
	
	// ------------------
	//  Listen
	// ------------------
	Listen();
}

//=============================================================================
