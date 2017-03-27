//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "isaverestore.h"
#include "ai_behavior.h"
#include "scripted.h"

//-----------------------------------------------------------------------------
// CAI_BehaviorBase
//-----------------------------------------------------------------------------

BEGIN_DATADESC_NO_BASE( CAI_BehaviorBase )

END_DATADESC()

//-------------------------------------

CAI_ClassScheduleIdSpace *CAI_BehaviorBase::GetClassScheduleIdSpace()
{
	return GetOuter()->GetClassScheduleIdSpace();
}

//-------------------------------------

void CAI_BehaviorBase::GatherConditions()
{
	Assert( m_pBackBridge != NULL );
	
	m_pBackBridge->BackBridge_GatherConditions();
}

//-------------------------------------

void CAI_BehaviorBase::PrescheduleThink()
{
	return;
}

//-------------------------------------

void CAI_BehaviorBase::OnScheduleChange()
{
	return;
}

//-------------------------------------

int CAI_BehaviorBase::SelectSchedule()
{
	m_fOverrode = false; 
	return SCHED_NONE;
}

//-------------------------------------

int CAI_BehaviorBase::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	m_fOverrode = false; 
	return SCHED_NONE;
}

//-------------------------------------

void CAI_BehaviorBase::StartTask( const Task_t *pTask )
{
	m_fOverrode = false;
}

//-------------------------------------

void CAI_BehaviorBase::RunTask( const Task_t *pTask )
{
	m_fOverrode = false;
}

//-------------------------------------

int CAI_BehaviorBase::TranslateSchedule( int scheduleType )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_TranslateSchedule( scheduleType );
}

//-------------------------------------

CAI_Schedule *CAI_BehaviorBase::GetSchedule(int schedule)
{
	if (!GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		Warning("ERROR: %s missing schedule!\n", GetSchedulingErrorName());
		return g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
	}
	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}

	if ( schedule == -1 )
		return NULL;

	return g_AI_SchedulesManager.GetScheduleFromID( schedule );
}

//-------------------------------------

const char *CAI_BehaviorBase::GetSchedulingErrorName()
{
	return "CAI_Behavior";
}

//-------------------------------------

Activity CAI_BehaviorBase::NPC_TranslateActivity( Activity activity )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_NPC_TranslateActivity( activity );
}

//-------------------------------------

bool CAI_BehaviorBase::IsCurTaskContinuousMove()
{
	m_fOverrode = false;
	return false;
}

//-------------------------------------

bool CAI_BehaviorBase::FValidateHintType( CAI_Hint *pHint )
{
	m_fOverrode = false;
	return false;
}

//-------------------------------------

bool CAI_BehaviorBase::IsValidEnemy( CBaseEntity *pEnemy )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsValidEnemy( pEnemy );
}

//-------------------------------------

bool CAI_BehaviorBase::IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsValidCover( vLocation, pHint );
}

//-------------------------------------

bool CAI_BehaviorBase::IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsValidShootPosition( vLocation, pHint );
}

//-------------------------------------

bool CAI_BehaviorBase::ShouldAlwaysThink()
{
	m_fOverrode = false;
	return false;
}

//-------------------------------------

bool CAI_BehaviorBase::NotifyChangeBehaviorStatus( bool fCanFinishSchedule )
{
	bool fInterrupt = GetOuter()->OnBehaviorChangeStatus( this, fCanFinishSchedule );
	
	if ( !GetOuter()->IsInterruptable())
		return false;
		
	if ( fInterrupt )
	{
		if ( GetOuter()->m_hCine )
			GetOuter()->m_hCine->CancelScript();

		//!!!HACKHACK
		// this is dirty, but it forces NPC to pick a new schedule next time through.
		GetOuter()->ClearSchedule();
	}

	return fInterrupt;
}

//-------------------------------------

int	CAI_BehaviorBase::Save( ISave &save )				
{ 
	return save.WriteAll( this, GetDataDescMap() );	
}

//-------------------------------------

int	CAI_BehaviorBase::Restore( IRestore &restore )
{ 
	return restore.ReadAll( this, GetDataDescMap() );	
}

//-------------------------------------

#define BEHAVIOR_SAVE_BLOCKNAME "AI_Behaviors"
#define BEHAVIOR_SAVE_VERSION	2

void CAI_BehaviorBase::SaveBehaviors(ISave &save, CAI_BehaviorBase *pCurrentBehavior, CAI_BehaviorBase **ppBehavior, int nBehaviors )		
{ 
	save.StartBlock( BEHAVIOR_SAVE_BLOCKNAME );
	short temp = BEHAVIOR_SAVE_VERSION;
	save.WriteShort( &temp );
	temp = (short)nBehaviors;
	save.WriteShort( &temp );

	for ( int i = 0; i < nBehaviors; i++ )
	{
		if ( strcmp( ppBehavior[i]->GetDataDescMap()->dataClassName, CAI_BehaviorBase::m_DataMap.dataClassName ) != 0 )
		{
			save.StartBlock();
			save.WriteString( ppBehavior[i]->GetDataDescMap()->dataClassName );
			bool bIsCurrent = ( pCurrentBehavior == ppBehavior[i] );
			save.WriteBool( &bIsCurrent );
			ppBehavior[i]->Save( save );
			save.EndBlock();
		}
	}

	save.EndBlock();
}

//-------------------------------------

int CAI_BehaviorBase::RestoreBehaviors(IRestore &restore, CAI_BehaviorBase **ppBehavior, int nBehaviors )	
{ 
	int iCurrent = -1;
	char szBlockName[SIZE_BLOCK_NAME_BUF];
	restore.StartBlock( szBlockName );
	if ( strcmp( szBlockName, BEHAVIOR_SAVE_BLOCKNAME ) == 0 )
	{
		short version;
		restore.ReadShort( &version );
		if ( version == BEHAVIOR_SAVE_VERSION )
		{
			short nToRestore;
			char szClassNameCurrent[256];
			restore.ReadShort( &nToRestore );
			for ( int i = 0; i < nToRestore; i++ )
			{
				restore.StartBlock();
				restore.ReadString( szClassNameCurrent, sizeof( szClassNameCurrent ), 0 );
				bool bIsCurrent;
				restore.ReadBool( &bIsCurrent );

				for ( int j = 0; j < nBehaviors; j++ )
				{
					if ( strcmp( ppBehavior[j]->GetDataDescMap()->dataClassName, szClassNameCurrent ) == 0 )
					{
						if ( bIsCurrent )
							iCurrent = j;
						ppBehavior[j]->Restore( restore );
					}
				}

				restore.EndBlock();

			}
		}
	}
	restore.EndBlock();
	return iCurrent; 
}


//-----------------------------------------------------------------------------
