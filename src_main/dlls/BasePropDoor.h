//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef BASEPROPDOOR_H
#define BASEPROPDOOR_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"
#include "locksounds.h"
#include "entityoutput.h"


struct opendata_t
{
	Vector vecStandPos;		// Where the NPC should stand.
	Vector vecFaceDir;		// What direction the NPC should face.
	Activity eActivity;		// What activity the NPC should play.
};


class CBasePropDoor : public CDynamicProp
{
public:

	typedef CDynamicProp BaseClass;

	void Spawn();
	void Precache();
	void Activate();
	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

	void HandleAnimEvent( animevent_t *pEvent );

	// Interface for NPCs to query door operability.
	virtual bool NPCOpenDoor(CAI_BaseNPC *pNPC);
	virtual void GetNPCOpenData(CAI_BaseNPC *pNPC, opendata_t &opendata) = 0;
	virtual float GetOpenInterval(void) = 0;

protected:

	enum DoorState_t
	{
		DOOR_STATE_CLOSED = 0,
		DOOR_STATE_OPENING,
		DOOR_STATE_OPEN,
		DOOR_STATE_CLOSING,
	};

	inline bool IsDoorOpen() { return m_eDoorState == DOOR_STATE_OPEN; }
	inline bool IsDoorOpening() { return m_eDoorState == DOOR_STATE_OPENING; }
	inline bool IsDoorClosed() { return m_eDoorState == DOOR_STATE_CLOSED; }
	inline bool IsDoorClosing() { return m_eDoorState == DOOR_STATE_CLOSING; }

	inline bool WillAutoReturn() { return m_flAutoReturnDelay != -1; }

	void DoorOpen();
	void DoorClose();
	void DoorOpened();
	void DoorClosed();

	bool DoorActivate();

	void Lock();
	void Unlock();

	virtual void BeginOpening() = 0;
	virtual void BeginClosing() = 0;

	virtual void SetDoorOpen() = 0;
	virtual void SetDoorClosed() = 0;

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void StartBlocked(CBaseEntity *pOther);
	void Blocked(CBaseEntity *pOther);
	void EndBlocked(void);

	void UpdateAreaPortals(bool bOpen);

	// Input handlers
	void InputClose(inputdata_t &inputdata);
	void InputLock(inputdata_t &inputdata);
	void InputOpen(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);
	void InputUnlock(inputdata_t &inputdata);

	int m_nHardwareType;
	DoorState_t m_eDoorState;	// Holds whether the door is open, closed, opening, or closing.
	locksound_t m_ls;			// The sounds the door plays when being locked, unlocked, etc.
	bool m_bLocked;				// True if the door is locked.
	float m_flAutoReturnDelay;	// How many seconds to wait before automatically closing, -1 never closes automatically.
	EHANDLE m_hActivator;		

	string_t m_SoundMoving;
	string_t m_SoundOpen;
	string_t m_SoundClose;
	
	// dvs: FIXME: can we remove m_flSpeed from CBaseEntity?
	//float m_flSpeed;			// Rotation speed when opening or closing in degrees per second.

	CHandle<CAI_BaseNPC> m_hOpeningNPC;		// The NPC that is opening us, so we can notify them.

	DECLARE_DATADESC();
private:

	static void RegisterPrivateActivities();

	// Outputs
	COutputEvent m_OnBlockedClosing;		// Triggered when the door becomes blocked while closing.
	COutputEvent m_OnBlockedOpening;		// Triggered when the door becomes blocked while opening.
	COutputEvent m_OnUnblockedClosing;		// Triggered when the door becomes unblocked while closing.
	COutputEvent m_OnUnblockedOpening;		// Triggered when the door becomes unblocked while opening.
	COutputEvent m_OnFullyClosed;			// Triggered when the door reaches the fully closed position.
	COutputEvent m_OnFullyOpen;				// Triggered when the door reaches the fully open position.
	COutputEvent m_OnClose;					// Triggered when the door is told to close.
	COutputEvent m_OnOpen;					// Triggered when the door is told to open.

};


#endif // BASEPROPDOOR_H
