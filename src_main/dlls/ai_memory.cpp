//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose:		An NPC's memory of potential enemies 
//
//=============================================================================

#include "cbase.h"
#include "isaverestore.h"
#include "ai_debug.h"
#include "ai_memory.h"
#include "ai_basenpc.h"

#define	EMEMORY_POOL_SIZE		  64
#define AI_FREE_KNOWLEDGE_DURATION 1.75

//-----------------------------------------------------------------------------
// AI_EnemyInfo_t
//
//-----------------------------------------------------------------------------

DEFINE_FIXEDSIZE_ALLOCATOR( AI_EnemyInfo_t, EMEMORY_POOL_SIZE, CMemoryPool::GROW_FAST );

//-----------------------------------------------------------------------------

AI_EnemyInfo_t::AI_EnemyInfo_t(void) 
{
	hEnemy				= NULL;
	vLastKnownLocation	= vec3_origin;
}

 
//-----------------------------------------------------------------------------
// CAI_EnemiesListSaveRestoreOps
//
// Purpose: Handles save and load for enemy memories
//
//-----------------------------------------------------------------------------

class CAI_EnemiesListSaveRestoreOps : public CDefSaveRestoreOps
{
public:
	CAI_EnemiesListSaveRestoreOps()
	{
	}

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		CAI_Enemies::CMemMap *pMemMap = (CAI_Enemies::CMemMap *)fieldInfo.pField;
		
		int nMemories = pMemMap->Count();
		pSave->WriteInt( &nMemories );
		
		for ( CAI_Enemies::CMemMap::IndexType_t i = pMemMap->FirstInorder(); i != pMemMap->InvalidIndex(); i = pMemMap->NextInorder( i ) )
		{
			pSave->WriteAll( (*pMemMap)[i] );
		}
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		CAI_Enemies::CMemMap *pMemMap = (CAI_Enemies::CMemMap *)fieldInfo.pField;
		Assert( pMemMap->Count() == 0 );
		
		int nMemories = pRestore->ReadInt();
		
		while ( nMemories-- )
		{
			AI_EnemyInfo_t *pAddMemory = new AI_EnemyInfo_t;
			
			pRestore->ReadAll( pAddMemory );
			
			if ( pAddMemory->hEnemy != NULL )
			{
				pMemMap->Insert( pAddMemory->hEnemy, pAddMemory );
			}
			else
				delete pAddMemory;
		}
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CAI_Enemies::CMemMap *pMemMap = (CAI_Enemies::CMemMap *)fieldInfo.pField;
		
		for ( CAI_Enemies::CMemMap::IndexType_t i = pMemMap->FirstInorder(); i != pMemMap->InvalidIndex(); i = pMemMap->NextInorder( i ) )
		{
			delete (*pMemMap)[i];
		}
		
		pMemMap->RemoveAll();
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CAI_Enemies::CMemMap *pMemMap = (CAI_Enemies::CMemMap *)fieldInfo.pField;
		return ( pMemMap->Count() == 0 );
	}
	
} g_AI_MemoryListSaveRestoreOps;

//-----------------------------------------------------------------------------
// CAI_Enemies
//
// Purpose: Stores a set of AI_EnemyInfo_t's
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_Enemies )

	DEFINE_CUSTOM_FIELD( CAI_Enemies, m_Map, &g_AI_MemoryListSaveRestoreOps ),
  	DEFINE_FIELD( CAI_Enemies,	m_flFreeKnowledgeDuration,		FIELD_FLOAT ),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( AI_EnemyInfo_t )
	DEFINE_FIELD( AI_EnemyInfo_t, vLastKnownLocation, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( AI_EnemyInfo_t, hEnemy, 			FIELD_EHANDLE ),
	DEFINE_FIELD( AI_EnemyInfo_t, flLastTimeSeen, 	FIELD_TIME ),
	DEFINE_FIELD( AI_EnemyInfo_t, bDangerMemory, 		FIELD_BOOLEAN ),
	DEFINE_FIELD( AI_EnemyInfo_t, bEludedMe, 			FIELD_BOOLEAN ),
	// NOT SAVED nextEMemory
END_DATADESC()

//-----------------------------------------------------------------------------

CAI_Enemies::CAI_Enemies(void)
{
	m_flFreeKnowledgeDuration = AI_FREE_KNOWLEDGE_DURATION;
	SetDefLessFunc( m_Map );
}


//-----------------------------------------------------------------------------

CAI_Enemies::~CAI_Enemies()
{
	for ( CMemMap::IndexType_t i = m_Map.FirstInorder(); i != m_Map.InvalidIndex(); i = m_Map.NextInorder( i ) )
	{
		delete m_Map[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Purges any dead enemies from memory
//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CAI_Enemies::GetFirst( AIEnemiesIter_t *pIter )
{
	CMemMap::IndexType_t i = m_Map.FirstInorder();
	*pIter = (AIEnemiesIter_t)(unsigned)i;

	return ( i != m_Map.InvalidIndex() ) ? m_Map[i] :  NULL;
}

//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CAI_Enemies::GetNext( AIEnemiesIter_t *pIter )
{
	CMemMap::IndexType_t i = (CMemMap::IndexType_t)((unsigned)(*pIter));

	if ( i == m_Map.InvalidIndex() )
		return NULL;

	i = m_Map.NextInorder( i );
	*pIter = (AIEnemiesIter_t)(unsigned)i;
	if ( i == m_Map.InvalidIndex() )
		return NULL;

	return m_Map[i];
}
	
//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CAI_Enemies::Find( CBaseEntity *pEntity, bool bTryDangerMemory )
{
	CMemMap::IndexType_t i = m_Map.Find( pEntity );
	if ( i == m_Map.InvalidIndex() )
	{
		if ( !bTryDangerMemory || ( i = m_Map.Find( NULL ) ) == m_Map.InvalidIndex() )
			return NULL;
		Assert(m_Map[i]->bDangerMemory == true);
	}
	return m_Map[i];
}

//-----------------------------------------------------------------------------

const float ENEMY_DISCARD_TIME = 60;

void CAI_Enemies::RefreshMemories(void)
{
	AI_PROFILE_SCOPE(CAI_Enemies_RefreshMemories);

	// -------------------
	// Check each record
	// -------------------
	
	CMemMap::IndexType_t i = m_Map.FirstInorder();
	while ( i != m_Map.InvalidIndex() )
	{	
		AI_EnemyInfo_t *pMemory = m_Map[i];
		CBaseEntity *pEnemy = pMemory->hEnemy;
		
		CMemMap::IndexType_t iNext = m_Map.NextInorder( i ); // save so can remove
		if ( !pEnemy || 
			 ( pEnemy->MyNPCPointer() != NULL && pEnemy->MyNPCPointer()->GetState() == NPC_STATE_DEAD ) ||
			 gpGlobals->curtime > pMemory->flLastTimeSeen + ENEMY_DISCARD_TIME )
		{
			delete pMemory;
			m_Map.RemoveAt(i);
		}
		else
		{
			if ( gpGlobals->curtime <= pMemory->flLastTimeSeen + m_flFreeKnowledgeDuration )
				pMemory->vLastKnownLocation = pMemory->hEnemy->GetAbsOrigin();
		}
		i = iNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Updates information about our enemies
// Output : Returns true if new enemy, false if already know of enemy
//-----------------------------------------------------------------------------
bool CAI_Enemies::UpdateMemory(CAI_Network* pAINet, CBaseEntity *pEnemy, const Vector &vPosition, bool firstHand )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	// -------------------------------------------
	//  Otherwise just update my own
	// -------------------------------------------
	// Update enemy information
	if ( pMemory )
	{
		if ( firstHand )
			pMemory->flLastTimeSeen = gpGlobals->curtime;
		pMemory->bEludedMe = false;

		// Only update if the enemy has moved
		if ((pMemory->vLastKnownLocation - vPosition).Length()>12.0)
		{
			pMemory->vLastKnownLocation = vPosition;

		}
		return false;
	}

	// If not on my list of enemies add it
	AI_EnemyInfo_t *pAddMemory = new AI_EnemyInfo_t;
	pAddMemory->vLastKnownLocation = vPosition;

	pAddMemory->flLastTimeSeen = gpGlobals->curtime;
	pAddMemory->bEludedMe = false;

	// I'm either remembering a postion of an enmey of just a danger position
	pAddMemory->hEnemy = pEnemy;
	pAddMemory->bDangerMemory = ( pEnemy == NULL );

	// add to the list
	m_Map.Insert( pEnemy, pAddMemory );

	return true;
}

//------------------------------------------------------------------------------
// Purpose : Returns true if this enemy is part of my memory
//------------------------------------------------------------------------------
bool CAI_Enemies::HasMemory( CBaseEntity *pEnemy )
{
	return ( Find( pEnemy ) != NULL );
}

//-----------------------------------------------------------------------------
// Purpose:	Clear information about our enemy
//-----------------------------------------------------------------------------
void CAI_Enemies::ClearMemory(CBaseEntity *pEnemy)
{
	CMemMap::IndexType_t i = m_Map.Find( pEnemy );
	if ( i != m_Map.InvalidIndex() )
	{
		delete m_Map[i];
		m_Map.RemoveAt( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notes that the given enemy has eluded me
//-----------------------------------------------------------------------------
void CAI_Enemies::MarkAsEluded( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
	{
		pMemory->bEludedMe = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns last known posiiton of given enemy
//-----------------------------------------------------------------------------
const Vector &CAI_Enemies::LastKnownPosition( CBaseEntity *pEnemy )
{
	static Vector defPos;
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		return pMemory->vLastKnownLocation;

	DevWarning( 2,"Asking LastKnownPosition for enemy that's not in my memory!!\n");
	return defPos;
}

//-----------------------------------------------------------------------------
// Purpose: Sets position to the last known position of an enemy.  If enemy
//			was not found returns last memory of danger position if it exists
// Output : Returns false is no position is known
//-----------------------------------------------------------------------------
float CAI_Enemies::LastTimeSeen( CBaseEntity *pEnemy)
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		return pMemory->flLastTimeSeen;

	DevWarning( 2,"Asking LastTimeSeen for enemy that's not in my memory!!\n");
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Sets position to the last known position of an enemy.  If enemy
//			was not found returns last memory of danger position if it exists
// Output : Returns false is no position is known
//-----------------------------------------------------------------------------
bool CAI_Enemies::HasEludedMe( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
		return pMemory->bEludedMe;
	return false;
}

//-----------------------------------------------------------------------------
