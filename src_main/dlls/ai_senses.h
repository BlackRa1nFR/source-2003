//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_SENSES_H
#define AI_SENSES_H

#include "utlvector.h"
#include "simtimer.h"
#include "ai_component.h"

#if defined( _WIN32 )
#pragma once
#endif

class CBaseEntity;
class CSound;

//-------------------------------------

DECLARE_POINTER_HANDLE( AISightIter_t );
DECLARE_POINTER_HANDLE( AISoundIter_t );

//-----------------------------------------------------------------------------
// class CAI_ScriptConditions
//
// Purpose: 
//-----------------------------------------------------------------------------

class CAI_Senses : public CAI_Component
{
public:
	CAI_Senses()
	 : 	m_LookDist(2048),
		m_LastLookDist(-1),
		m_TimeLastLook(-1),
		m_iAudibleList(0),
		m_HighPriorityTimer(0.15),	// every other thinks (5Hz)
		m_NPCsTimer(0.25),			// every third think (~3Hz)
		m_MiscTimer(0.45)			// every fifth think (2Hz)
	{
		m_SeenArrays[0] = &m_SeenHighPriority;
		m_SeenArrays[1] = &m_SeenNPCs;
		m_SeenArrays[2] = &m_SeenMisc;
	}
	
	float			GetDistLook() const				{ return m_LookDist; }
	void			SetDistLook( float flDistLook ) { m_LookDist = flDistLook; }

	void			PerformSensing();

	void			Listen( void );
	void			Look( int iDistance );// basic sight function for npcs

	bool			ShouldSeeEntity( CBaseEntity *pEntity ); // logical query
	bool			CanSeeEntity( CBaseEntity *pSightEnt ); // more expensive cone & raycast test
	
	bool			DidSeeEntity( CBaseEntity *pSightEnt ) const; //  a less expensive query that looks at cached results from recent conditionsa gathering

	CBaseEntity *	GetFirstSeenEntity( AISightIter_t *pIter ) const;
	CBaseEntity *	GetNextSeenEntity( AISightIter_t *pIter ) const;

	CSound *		GetFirstHeardSound( AISoundIter_t *pIter );
	CSound *		GetNextHeardSound( AISoundIter_t *pIter );
	CSound *		GetClosestSound( bool fScent = false );

	//---------------------------------

	DECLARE_SIMPLE_DATADESC();

private:
	int				GetAudibleList() const { return m_iAudibleList; }
	bool 			CanHearSound( CSound *pSound );

	bool			WaitingUntilSeen( CBaseEntity *pSightEnt );

	void			BeginGather();
	void 			NoteSeenEntity( CBaseEntity *pSightEnt );
	void			EndGather( int nSeen, CUtlVector<EHANDLE> *pResult );
	
	bool 			Look( CBaseEntity *pSightEnt );
	int 			LookForHighPriorityEntities( int iDistance );
	int 			LookForNPCs( int iDistance );
	int 			LookForObjects( int iDistance );
	
	bool			SeeEntity( CBaseEntity *pEntity );
	
	float			m_LookDist;				// distance npc sees (Default 2048)
	float			m_LastLookDist;
	float			m_TimeLastLook;
	
	int				m_iAudibleList;				// first index of a linked list of sounds that the npc can hear.
	
	CUtlVector<EHANDLE> m_SeenHighPriority;
	CUtlVector<EHANDLE> m_SeenNPCs;
	CUtlVector<EHANDLE> m_SeenMisc;
	
	CUtlVector<EHANDLE> *m_SeenArrays[3];
	
	CSimTimer		m_HighPriorityTimer;
	CSimTimer		m_NPCsTimer;
	CSimTimer		m_MiscTimer;
};

//-----------------------------------------------------------------------------

#endif // AI_SENSES_H
