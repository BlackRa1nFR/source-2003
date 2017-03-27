//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "entitylist.h"
#include "ai_basenpc.h"
#include "npc_citizen17.h"

#define MAX_ALLIES	10

class CAI_AllyManager : public CBaseEntity
{
	DECLARE_CLASS( CAI_AllyManager, CBaseEntity );

public:
	void Spawn();

	void CountAllies( int *pTotal, int *pMedics );

private:
	int			m_iMaxAllies;
	int			m_iMaxMedics;

	int			m_iAlliesLast;
	int 		m_iMedicsLast;
public:
	void WatchCounts();

	// Input functions
	void InputSetMaxAllies( inputdata_t &inputdata );
	void InputSetMaxMedics( inputdata_t &inputdata );
	void InputReplenish( inputdata_t &inputdata );

	// Outputs
	COutputEvent	m_SpawnAlly[ MAX_ALLIES ];
	COutputEvent	m_SpawnMedicAlly;
	COutputEvent	m_OnZeroAllies;
	COutputEvent	m_OnZeroMedicAllies;
	

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( ai_ally_manager, CAI_AllyManager );

BEGIN_DATADESC( CAI_AllyManager )
	DEFINE_KEYFIELD( CAI_AllyManager, m_iMaxAllies,	FIELD_INTEGER, "maxallies" ),
	DEFINE_KEYFIELD( CAI_AllyManager, m_iMaxMedics,	FIELD_INTEGER, "maxmedics" ),
	DEFINE_FIELD( CAI_AllyManager, m_iAlliesLast,	FIELD_INTEGER ),
	DEFINE_FIELD( CAI_AllyManager, m_iMedicsLast, 	FIELD_INTEGER ),

	DEFINE_THINKFUNC( CAI_AllyManager, WatchCounts ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_AllyManager, FIELD_INTEGER, "SetMaxAllies", InputSetMaxAllies ),
	DEFINE_INPUTFUNC( CAI_AllyManager, FIELD_INTEGER, "SetMaxMedics", InputSetMaxMedics ),
	DEFINE_INPUTFUNC( CAI_AllyManager, FIELD_VOID, "Replenish", InputReplenish ),

	// Outputs
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 0 ], "SpawnAlly0" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 1 ], "SpawnAlly1" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 2 ], "SpawnAlly2" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 3 ], "SpawnAlly3" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 4 ], "SpawnAlly4" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 5 ], "SpawnAlly5" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 6 ], "SpawnAlly6" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 7 ], "SpawnAlly7" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 8 ], "SpawnAlly8" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnAlly[ 9 ], "SpawnAlly9" ),

	DEFINE_OUTPUT( CAI_AllyManager, m_SpawnMedicAlly, "SpawnMedicAlly" ),

	DEFINE_OUTPUT( CAI_AllyManager, m_OnZeroAllies, "OnZeroAllies" ),
	DEFINE_OUTPUT( CAI_AllyManager, m_OnZeroMedicAllies, "OnZeroMedicAllies" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AllyManager::Spawn()
{
	SetThink( WatchCounts );
	SetNextThink( gpGlobals->curtime + 1.0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AllyManager::WatchCounts()
{
	// Count the number of allies with the player right now.
	int iCurrentAllies;
	int iCurrentMedics;
	
	CountAllies( &iCurrentAllies, &iCurrentMedics );

	if ( !iCurrentAllies && m_iAlliesLast )
		m_OnZeroAllies.FireOutput( this, this, 0 );
		
	if ( !iCurrentMedics && m_iMedicsLast )
		m_OnZeroMedicAllies.FireOutput( this, this, 0 );
	
	m_iAlliesLast = iCurrentAllies;
	m_iMedicsLast = iCurrentMedics;

	SetNextThink( gpGlobals->curtime + 1.0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AllyManager::CountAllies( int *pTotal, int *pMedics )
{
	(*pTotal) = (*pMedics) = 0;
			
	CAI_BaseNPC **	ppAIs 	= g_AI_Manager.AccessAIs();
	int 			nAIs 	= g_AI_Manager.NumAIs();

	for ( int i = 0; i < nAIs; i++ )
	{
		if ( ppAIs[i]->IsAlive() && ppAIs[i]->IsPlayerAlly() )
		{
			(*pTotal)++;
			if( FClassnameIs( ppAIs[i], "npc_citizen" ) && ppAIs[i]->HasSpawnFlags( SF_CITIZEN_MEDIC ) )
				(*pMedics)++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_AllyManager::InputSetMaxAllies( inputdata_t &inputdata )
{
	m_iMaxAllies = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AllyManager::InputSetMaxMedics( inputdata_t &inputdata )
{
	m_iMaxMedics = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_AllyManager::InputReplenish( inputdata_t &inputdata )
{
	// Count the number of allies with the player right now.
	int iCurrentAllies;
	int iCurrentMedics;
	
	CountAllies( &iCurrentAllies, &iCurrentMedics );

	// TOTAL number of allies to be replaced.
	int iReplaceAllies = m_iMaxAllies - iCurrentAllies;

	// The number of total allies that should be medics.
	int iReplaceMedics = m_iMaxMedics - iCurrentMedics;

	if( iReplaceMedics > iReplaceAllies )
	{
		// Clamp medics.
		iReplaceMedics = iReplaceAllies;
	}

// Medics.
	if( m_iMaxMedics > 0 )
	{

		if( iReplaceMedics > MAX_ALLIES )
		{
			// This error is fatal now. (sjb)
			Msg("**ERROR! ai_allymanager - ReplaceMedics > MAX_ALLIES\n" );
			return;
		}

		int i;
		for( i = 0 ; i < iReplaceMedics ; i++ )
		{
			m_SpawnMedicAlly.FireOutput( this, this, 0 );

			// Don't forget to count this guy against the number of
			// allies to be replenished.
			iReplaceAllies--;
		}
	}

// Allies
	if( iReplaceAllies < 1 ) 
	{
		return;
	}

	if( iReplaceAllies > MAX_ALLIES )
	{
		Msg("**ERROR! ai_allymanager - ReplaceAllies > MAX_ALLIES\n" );
		iReplaceAllies = MAX_ALLIES;
	}

	int i;
	for( i = 0 ; i < iReplaceAllies ; i++ )
	{
		m_SpawnAlly[ i ].FireOutput( this, this, 0 );
	}
}