//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "utlrbtree.h"
#include "saverestore_utlvector.h"
#include "ai_goalentity.h"

//-----------------------------------------------------------------------------
//
// CAI_GoalEntity implementation
//

BEGIN_DATADESC( CAI_GoalEntity )

	DEFINE_KEYFIELD(	CAI_GoalEntity, m_iszActor,				FIELD_STRING, 	"Actor"					),
	DEFINE_KEYFIELD(	CAI_GoalEntity, m_iszGoal,				FIELD_STRING, 	"Goal"					),
	DEFINE_KEYFIELD(	CAI_GoalEntity, m_fStartActive,			FIELD_BOOLEAN,  "StartActive"			),
	DEFINE_KEYFIELD(	CAI_GoalEntity, m_iszConceptModifiers,	FIELD_STRING, 	"BaseConceptModifiers"	),
	DEFINE_KEYFIELD(	CAI_GoalEntity, m_SearchType,			FIELD_INTEGER, 	"SearchType"			),
	DEFINE_UTLVECTOR(	CAI_GoalEntity, m_actors, 				FIELD_EHANDLE 							),
	DEFINE_FIELD(		CAI_GoalEntity, m_hGoalEntity, 			FIELD_EHANDLE 							),
	DEFINE_FIELD(		CAI_GoalEntity, m_flags, 				FIELD_INTEGER 							),

	DEFINE_THINKFUNC( CAI_GoalEntity, DelayedRefresh ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_GoalEntity, FIELD_VOID, "Activate", 		InputActivate ),
	DEFINE_INPUTFUNC( CAI_GoalEntity, FIELD_VOID, "UpdateActors",	InputUpdateActors ),
	DEFINE_INPUTFUNC( CAI_GoalEntity, FIELD_VOID, "Deactivate",		InputDeactivate ),

END_DATADESC()


//-------------------------------------

void CAI_GoalEntity::Spawn()
{
	SetThink( &CAI_GoalEntity::DelayedRefresh );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-------------------------------------

void CAI_GoalEntity::DelayedRefresh()
{
	inputdata_t ignored;
	if ( m_fStartActive )
	{
		Assert( !(m_flags & ACTIVE) );
		InputActivate( ignored );
		m_fStartActive = false;
	}
	else
		InputUpdateActors( ignored );
	
	SetThink( NULL );
}

//-------------------------------------

void CAI_GoalEntity::PruneActors()
{
	for ( int i = m_actors.Count() - 1; i >= 0; i-- )
	{
		if ( m_actors[i] == NULL || m_actors[i]->IsMarkedForDeletion() || m_actors[i]->GetState() == NPC_STATE_DEAD )
			m_actors.FastRemove( i );
	}
}

//-------------------------------------

void CAI_GoalEntity::ResolveNames()
{
	m_actors.SetCount( 0 );
	
	CBaseEntity *pEntity = NULL;
	for (;;)
	{
		switch ( m_SearchType )
		{
			case ST_ENTNAME:
			{
				pEntity = gEntList.FindEntityByName( pEntity, m_iszActor, NULL );
				break;
			}
			
			case ST_CLASSNAME:
			{
				pEntity = gEntList.FindEntityByClassname( pEntity, STRING( m_iszActor ) );
				break;
			}
		}
		
		if ( !pEntity )
			break;
			
		CAI_BaseNPC *pActor = pEntity->MyNPCPointer();
		
		if ( pActor  && pActor->GetState() != NPC_STATE_DEAD )
		{
			AIHANDLE temp;
			temp = pActor;
			m_actors.AddToTail( temp );
		}
	}
		
	m_hGoalEntity = gEntList.FindEntityByName( NULL, m_iszGoal, NULL );
}

//-------------------------------------

void CAI_GoalEntity::InputActivate( inputdata_t &inputdata )
{
	if ( !( m_flags & ACTIVE ) )
	{
		gEntList.AddListenerEntity( this );
		
		UpdateActors();
		m_flags |= ACTIVE;
		
		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			EnableGoal( m_actors[i] );
		}
	}
}

//-------------------------------------

void CAI_GoalEntity::InputUpdateActors( inputdata_t &inputdata )
{
	int i;
	CUtlRBTree<CAI_BaseNPC *> prevActors;
	CUtlRBTree<CAI_BaseNPC *>::IndexType_t index;

	SetDefLessFunc( prevActors );
	
	PruneActors();
	
	for ( i = 0; i < m_actors.Count(); i++ )
	{
		prevActors.Insert( m_actors[i] );
	}
	
	ResolveNames();
	
	for ( i = 0; i < m_actors.Count(); i++ )
	{
		index = prevActors.Find( m_actors[i] );
		if ( index == prevActors.InvalidIndex() )
		{
			if ( m_flags & ACTIVE )
				EnableGoal( m_actors[i] );
		}
		else
			prevActors.Remove( m_actors[i] );
	}
	
	for ( index = prevActors.FirstInorder(); index != prevActors.InvalidIndex(); index = prevActors.NextInorder( index ) )
	{
		if ( m_flags & ACTIVE )
			DisableGoal( prevActors[ index ] );
	}
}

//-------------------------------------

void CAI_GoalEntity::InputDeactivate( inputdata_t &inputdata ) 	
{
	if ( m_flags & ACTIVE )
	{
		UpdateActors();
		m_flags &= ~ACTIVE;
		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			DisableGoal( m_actors[i] );
		}
		
		gEntList.RemoveListenerEntity( this );
	}
}

//-------------------------------------

void CAI_GoalEntity::UpdateOnRemove()
{
	if ( m_flags & ACTIVE )
	{
		inputdata_t inputdata;
		InputDeactivate( inputdata );
	}
	BaseClass::UpdateOnRemove();
}

//-------------------------------------

void CAI_GoalEntity::OnEntityCreated( CBaseEntity *pEntity )
{
	Assert( m_flags & ACTIVE );
	
	if ( pEntity->MyNPCPointer() )
	{
		SetThink( &CAI_GoalEntity::DelayedRefresh );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	
}

//-------------------------------------

void CAI_GoalEntity::OnEntityDeleted( CBaseEntity *pEntity )
{
	Assert( pEntity != this );
}

//-----------------------------------------------------------------------------
