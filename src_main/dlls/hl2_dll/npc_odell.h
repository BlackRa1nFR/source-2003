//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef	NPC_ODELL_H
#define	NPC_ODELL_H

#include "ai_baseactor.h"
#include "ai_behavior.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"

class CNPC_Odell;


//-----------------------------------------------------------------------------

#if 0
#define CONCEPT_LEAD_LOST_FOLLOWER		"OD_LOSTYOU"
#define CONCEPT_LEAD_FOUND_FOLLOWER		"OD_FIND"
#define CONCEPT_LEAD_SEEKING_FOLLOWER	"OD_SEEK"
#endif

//-----------------------------------------------------------------------------

class CNPC_OdellExpresser : public CAI_Expresser
{
public:
	CNPC_OdellExpresser()
	{
	}
	
	virtual bool Speak( AIConcept_t concept, const char *pszModifiers = NULL );
	
private:
};

//-----------------------------------------------------------------------------
// CNPC_Odell
//
//-----------------------------------------------------------------------------


class CNPC_Odell : public CAI_BaseActor
{
	DECLARE_CLASS( CNPC_Odell, CAI_BaseActor );

public:
	CNPC_Odell();

	bool CreateBehaviors();
	void Spawn( void );
	void Precache( void );

	Class_T		Classify( void );
	float		MaxYawSpeed( void );
	int			ObjectCaps( void ) { return UsableNPCObjectCaps( BaseClass::ObjectCaps() ); }
	int			TranslateSchedule( int scheduleType );
	int			SelectSchedule ( void );
	bool 		OnBehaviorChangeStatus(  CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );

	void        HandleAnimEvent( animevent_t *pEvent );

	bool		IsLeading( void ) { return ( GetRunningBehavior() == &m_LeadBehavior && m_LeadBehavior.HasGoal() ); }
	bool		IsFollowing( void ) { return ( GetRunningBehavior() == &m_FollowBehavior && m_FollowBehavior.GetFollowTarget() ); }

private:

	// --------------------------------
	//
	// Talking
	//
	virtual CAI_Expresser *CreateExpresser() { return new CNPC_OdellExpresser; }

	// --------------------------------
	
	DEFINE_CUSTOM_AI;

	// --------------------------------
	//
	// Lead
	//
	CAI_LeadBehavior m_LeadBehavior;

	// --------------------------------
	//
	// Follow
	//
	CAI_FollowBehavior m_FollowBehavior;
	

};

//-----------------------------------------------------------------------------

#endif	//NPC_ODELL_H