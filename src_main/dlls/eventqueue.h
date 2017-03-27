//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: A global class that holds a prioritized queue of entity I/O events.
//			Events can be posted with a nonzero delay, which determines how long
//			they are held before being dispatched to their recipients.
//
//			The queue is serviced once per server frame.
//
//=============================================================================

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H
#ifdef _WIN32
#pragma once
#endif

#include "mempool.h"

class CEventQueue
{
public:
	// pushes an event into the queue, targeting a string name (m_iName), or directly by a pointer
	void AddEvent( const char *target, const char *action, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID = 0 );
	void AddEvent( CBaseEntity *target, const char *action, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID = 0 );
	void AddEvent( CBaseEntity *target, const char *action, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID = 0 );

	void CancelEvents( CBaseEntity *pCaller );

	// services the queue, firing off any events who's time hath come
	void ServiceEvents( void );

	// debugging
	void ValidateQueue( void );

	// serialization
	int Save( ISave &save );
	int Restore( IRestore &restore );

	CEventQueue();
	~CEventQueue();

	void Init( void );
	void Clear( void ); // resets the list

private:
	struct PrioritizedEvent_t
	{
		float m_flFireTime;
		string_t m_iTarget;
		string_t m_iTargetInput;
		EHANDLE m_pActivator;
		EHANDLE m_pCaller;
		int m_iOutputID;
		EHANDLE m_pEntTarget;  // a pointer to the entity to target; overrides m_iTarget

		variant_t m_VariantValue;	// variable-type parameter

		PrioritizedEvent_t *m_pNext;
		PrioritizedEvent_t *m_pPrev;

		static typedescription_t m_SaveData[];

		DECLARE_FIXEDSIZE_ALLOCATOR( PrioritizedEvent_t );
	};

	void AddEvent( PrioritizedEvent_t *event );
	void RemoveEvent( PrioritizedEvent_t *pe );

	static typedescription_t m_SaveData[];
	PrioritizedEvent_t m_Events;
	int m_iListCount;
};

extern CEventQueue g_EventQueue;


#endif // EVENTQUEUE_H

