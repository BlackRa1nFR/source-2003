//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Dr. Mossman, stalwart heroine, doing what is right in the face of 
//			near certain doom, all while fighting off the clumsy advances of her
//			misogynistic colleges.
//=============================================================================


//-----------------------------------------------------------------------------
// Generic NPC - purely for scripted sequence work.
//-----------------------------------------------------------------------------
#include	"cbase.h"
#include	"NPCevent.h"
#include	"AI_BaseNPC.h"
#include	"ai_hull.h"
#include "ai_baseactor.h"

//-----------------------------------------------------------------------------
// NPC's Anim Events Go Here
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Mossman : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Mossman, CAI_BaseHumanoid );

	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );
};

LINK_ENTITY_TO_CLASS( npc_mossman, CNPC_Mossman );

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Mossman :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Mossman :: HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_Mossman :: GetSoundInterests ( void )
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Mossman :: Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/mossman.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );

	NPCInit();

	Relink();
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Mossman :: Precache()
{
	engine->PrecacheModel( "models/mossman.mdl" );
	
	BaseClass::Precache();
}	

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------
