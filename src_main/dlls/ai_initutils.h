//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:		AI Utility classes for building the initial AI Networks
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_INITUTILS_H
#define AI_INITUTILS_H
#ifdef _WIN32
#pragma once
#endif


#include "ai_basenpc.h"

//###########################################################
//  >> CNodeEnt
//
// This is the entity that is loaded in from worldcraft.
// It is only used to build the network and is deleted
// immediately
//###########################################################
class CNodeEnt : public CLogicalPointEntity
{
	DECLARE_CLASS( CNodeEnt, CLogicalPointEntity );

public:
	static int			m_nNodeCount;
	short				m_eHintType;
	int					m_nWCNodeID;			// Node ID assigned by worldcraft (not same as engine!)
	string_t			m_strGroup;				// Group name for hint nodes

	void				Spawn( void );

	DECLARE_DATADESC();

	CNodeEnt(void);
};


//###########################################################
// >> CAI_TestHull 
//
// a modelless clip hull that verifies reachable nodes by 
// walking from every node to each of it's connections//
//###########################################################
class CAI_TestHull : public CAI_BaseNPC
{
	DECLARE_CLASS( CAI_TestHull, CAI_BaseNPC );
private:
	static CAI_TestHull*	pTestHull;								// Hull for testing connectivity

public:
	static CAI_TestHull*	GetTestHull(void);						// Get the test hull
	static void				ReturnTestHull(void);					// Return the test hull

	bool					bInUse;
	virtual void			Precache();
	void					Spawn(void);
	virtual int				ObjectCaps( void ) { return BaseClass :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual bool			IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;

	~CAI_TestHull(void);
};


#endif // AI_INITUTILS_H
