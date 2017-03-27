//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "ai_baseactor.h"
#include "npc_talker.h"
#include "ai_behavior_follow.h"

class CNPC_Alyx : public CAI_PlayerAlly
{
public:
	DECLARE_CLASS( CNPC_Alyx, CAI_PlayerAlly );

	bool	CreateBehaviors();
	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );
	int		SelectSchedule ( void );

	CAI_FollowBehavior		m_FollowBehavior;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};
