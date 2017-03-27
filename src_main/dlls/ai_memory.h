//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose:		An NPC's memory of potential enemies 
//
//=============================================================================

#include "mempool.h"
#include "utlmap.h"

#ifndef AI_MEMORY_H
#define AI_MEMORY_H
#pragma once

class CAI_Network;

DECLARE_POINTER_HANDLE(AIEnemiesIter_t);

//-----------------------------------------------------------------------------
// AI_EnemyInfo_t
//
// Purpose: Stores relevant tactical information about an enemy
//
//-----------------------------------------------------------------------------
struct AI_EnemyInfo_t
{
	AI_EnemyInfo_t();
	
	EHANDLE			hEnemy;				// Pointer to the enemy

	Vector			vLastKnownLocation;
	float			flLastTimeSeen;		// Last time enemy was seen
	bool			bDangerMemory;		// Memory of danger position w/o Enemy pointer
	bool			bEludedMe;			// True if enemy not at last known location 

	DECLARE_SIMPLE_DATADESC();
	DECLARE_FIXEDSIZE_ALLOCATOR(AI_EnemyInfo_t);
};

//-----------------------------------------------------------------------------
// CAI_Enemies
//
// Purpose: Stores a set of AI_EnemyInfo_t's
//
//-----------------------------------------------------------------------------
class CAI_Enemies
{
public:
	CAI_Enemies(void);
	~CAI_Enemies();
	
	AI_EnemyInfo_t *GetFirst( AIEnemiesIter_t *pIter );
	AI_EnemyInfo_t *GetNext( AIEnemiesIter_t *pIter );
	AI_EnemyInfo_t *Find( CBaseEntity *pEntity, bool bTryDangerMemory = false );

	void			RefreshMemories(void);
	bool			UpdateMemory( CAI_Network* pAINet, CBaseEntity *enemy, const Vector &vPosition, bool firstHand = true );

	bool			HasMemory( CBaseEntity *enemy );
	void			ClearMemory( CBaseEntity *enemy );

	const Vector &	LastKnownPosition( CBaseEntity *pEnemy );
	float			LastTimeSeen( CBaseEntity *pEnemy );
	
	void			MarkAsEluded( CBaseEntity *enemy );						// Don't know where he is (whole squad)
	bool			HasEludedMe( CBaseEntity *pEnemy );
	
	void			SetFreeKnowledgeDuration( float flDuration ) { m_flFreeKnowledgeDuration = flDuration; }

	DECLARE_SIMPLE_DATADESC();

	typedef CUtlMap<CBaseEntity *, AI_EnemyInfo_t*, unsigned char> CMemMap;

private:
	CMemMap m_Map;
	float	m_flFreeKnowledgeDuration;
};

//-----------------------------------------------------------------------------

#endif // AI_MEMORY_H
