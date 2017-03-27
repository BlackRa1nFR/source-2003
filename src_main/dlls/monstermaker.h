//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef MONSTERMAKER_H
#define MONSTERMAKER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"


//-----------------------------------------------------------------------------
// Spawnflags
//-----------------------------------------------------------------------------
#define	SF_NPCMAKER_START_ON	1	// start active ( if has targetname )
#define SF_NPCMAKER_NPCCLIP		8	// Children are blocked by NPCclip
#define SF_NPCMAKER_FADE		16	// Children's corpses fade
#define SF_NPCMAKER_INF_CHILD	32	// Infinite number of children
#define	SF_NPCMAKER_NO_DROP		64	// Do not adjust for the ground's position when checking for spawn

class CBaseNPCMaker : public CBaseEntity
{
public:
	DECLARE_CLASS( CBaseNPCMaker, CBaseEntity );

	void Spawn( void );
	void MakerThink( void );
	bool CanMakeNPC( void );

	virtual void DeathNotice( CBaseEntity *pChild );// NPC maker children use this to tell the NPC maker that they have died.
	virtual void MakeNPC( void ) = 0;

	virtual	void ChildPreSpawn( CAI_BaseNPC *pChild ) {}
	virtual	void ChildPostSpawn( CAI_BaseNPC *pChild ) {}

	CBaseNPCMaker(void) {}

	// Input handlers
	void InputSpawnNPC( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputSetMaxChildren( inputdata_t &inputdata );

	// State changers
	void Toggle( void );
	void Enable( void );
	void Disable( void );

	bool IsDepleted();

	DECLARE_DATADESC();
	
	int			m_nMaxNumNPCs;			// max number of NPCs this ent can create
	float		m_flSpawnFrequency;		// delay (in secs) between spawns 

	COutputEvent m_OnSpawnNPC;
	COutputEvent m_OnAllSpawned;
	COutputEvent m_OnAllSpawnedDead;
	
	int		m_nLiveChildren;	// how many NPCs made by this NPC maker that are currently alive
	int		m_nMaxLiveChildren;	// max number of NPCs that this maker may have out at one time.

	float	m_flGround; // z coord of the ground under me, used to make sure no NPCs are under the maker when it drops a new child
	bool	m_bDisabled;
};


class CNPCMaker : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CNPCMaker, CBaseNPCMaker );

	CNPCMaker( void );

	void Precache( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_iszNPCClassname;			// classname of the NPC(s) that will be created.
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_spawnEquipment;
	string_t m_RelationshipString;		// Used to load up relationship keyvalues
	string_t m_ChildTargetName;
};

class CTemplateNPCMaker : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CTemplateNPCMaker, CBaseNPCMaker );

	CTemplateNPCMaker( void ) {}

	void Activate( void );

	virtual void MakeNPC( void );
	void MakeNPCInRadius( void );
	void MakeNPCInLine( void );

	bool PlaceNPCInRadius( CAI_BaseNPC *pNPC );
	bool PlaceNPCInLine( CAI_BaseNPC *pNPC );

	// Inputs
	void InputSpawnInRadius( inputdata_t &inputdata ) { MakeNPCInRadius(); }
	void InputSpawnInLine( inputdata_t &inputdata ) { MakeNPCInLine(); }

	
	float	m_flRadius;

	DECLARE_DATADESC();
	
	string_t m_iszTemplateName;		// The name of the NPC that will be used as the template.
	string_t m_iszTemplateData;		// The keyvalue data blob from the template NPC that will be used to spawn new ones.
};

#endif // MONSTERMAKER_H
