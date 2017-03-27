//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CLIENT_THINKLIST_H
#define CLIENT_THINKLIST_H
#ifdef _WIN32
#pragma once
#endif


#include "IGameSystem.h"
#include "utllinkedlist.h"
#include "cliententitylist.h"
#include "iclientthinkable.h"
#include "utlrbtree.h"


#define CLIENT_THINK_ALWAYS	-1293
#define CLIENT_THINK_NEVER	-1


#define INVALID_THINK_HANDLE ClientThinkList()->GetInvalidThinkHandle()


class CClientThinkList : public IGameSystem
{
public:

							CClientThinkList();
	virtual					~CClientThinkList();
	
	// Set the next time at which you want to think. You can also use
	// one of the CLIENT_THINK_ defines.
	void					SetNextClientThink( ClientEntityHandle_t hEnt, float nextTime );
	
	// Remove an entity from the think list.
	void					RemoveThinkable( ClientEntityHandle_t hEnt );

	// Use to initialize your think handles in IClientThinkables.
	ClientThinkHandle_t		GetInvalidThinkHandle();

	// This is called after network updating and before rendering.
	void					PerformThinkFunctions();

	// Call this to destroy a thinkable object - deletes the object post think.
	void					AddToDeleteList( ClientEntityHandle_t hEnt );	
	void					RemoveFromDeleteList( ClientEntityHandle_t hEnt );

// IClientSystem implementation.
public:

	virtual bool Init();
	virtual void Shutdown();
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();
	virtual void PreRender();
	virtual void Update( float frametime );
	virtual void OnSave() {}
	virtual void OnRestore() {}

private:

	class CThinkEntry
	{
	public:
		ClientEntityHandle_t	m_hEnt;
		float					m_flNextClientThink;
		float					m_flLastClientThink;
	};

	CUtlLinkedList<CThinkEntry, unsigned short>	m_ThinkEntries;

	CUtlVector<ClientEntityHandle_t>	m_aDeleteList;

// Internal stuff.
private:

	CThinkEntry*	GetThinkEntry( ClientThinkHandle_t hThink );
	void			CleanUpDeleteList();
};


// -------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------- //

inline ClientThinkHandle_t CClientThinkList::GetInvalidThinkHandle()
{
	return (ClientThinkHandle_t)m_ThinkEntries.InvalidIndex();
}


inline CClientThinkList::CThinkEntry* CClientThinkList::GetThinkEntry( ClientThinkHandle_t hThink )
{
	return &m_ThinkEntries[ (unsigned long)hThink ];
}


inline CClientThinkList* ClientThinkList()
{
	extern CClientThinkList g_ClientThinkList;
	return &g_ClientThinkList;
}


#endif // CLIENT_THINKLIST_H
