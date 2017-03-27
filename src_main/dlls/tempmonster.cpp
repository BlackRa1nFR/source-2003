/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// NPC template
//=========================================================
#include "cbase.h"
#if 0

//=========================================================
// NPC's Anim Events Go Here
//=========================================================

class CMyNPC : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CMyNPC, CAI_BaseNPC );

	void Spawn( void );
	void Precache( void );
	void MaxYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( animevent_t *pEvent );
};
LINK_ENTITY_TO_CLASS( my_NPC, CMyNPC );

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
int	CMyNPC::Classify ( void )
{
	return	CLASS_MY_NPC;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CMyNPC::MaxYawSpeed ( void )
{
	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		return 90;
	}
}

//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMyNPC::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CAI_BaseNPC::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMyNPC::Spawn()
{
	Precache( );

	engine.SetModel(edict(), "models/mymodel.mdl");
	UTIL_SetSize( this, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );

	SetSolid( SOLID_SLIDEBOX );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= 8;
	m_vecViewOffset		= Vector ( 0, 0, 0 );// position of the eyes relative to NPC's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState		= NPCSTATE_NONE;

	NPCInit();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CMyNPC::Precache()
{
	engine.PrecacheModel("models/mymodel.mdl");
}	

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================
#endif
