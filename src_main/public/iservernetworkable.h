//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISERVERNETWORKABLE_H
#define ISERVERNETWORKABLE_H
#ifdef _WIN32
#pragma once
#endif


#include "ihandleentity.h"
#include "basetypes.h"


enum EntityChange_t
{
	ENTITY_CHANGE_AUTODETECT = 0,
	ENTITY_CHANGE_NONE
};


class ServerClass;
class SendTable;
struct edict_t;
class CBaseEntity;
class CSerialEntity;
class CBaseNetworkable;


class CCheckTransmitInfo
{
public:
	// Entities use these in their CheckTransmit function to tell the engine whether or not the 
	// entity should be transmitted.
	int		WillTransmit( int iEdict ) const;
	void	SetTransmit( int iEdict );
	void	ClearTransmit( int iEdict );

public:

	edict_t	*m_pClientEnt;
	byte	*m_pPVS;
	int		m_iArea;
	byte	*m_pEdictBits;
};

inline int CCheckTransmitInfo::WillTransmit( int iEdict ) const
{
	return m_pEdictBits[iEdict >> 3] & (1 << (iEdict & 7));
}

inline void CCheckTransmitInfo::SetTransmit( int iEdict )
{ 
	m_pEdictBits[iEdict >> 3] |= (1 << (iEdict & 7)); 
}

inline void CCheckTransmitInfo::ClearTransmit( int iEdict )
{ 
	m_pEdictBits[iEdict >> 3] &= ~(1 << (iEdict & 7)); 
}


// IServerNetworkable is the interface the engine uses for all networkable data.
class IServerNetworkable : public IHandleEntity
{
public:
	virtual					~IServerNetworkable() {}


// These functions are handled automatically by the server_class macros and CBaseNetworkable.
public:

	// Tell the engine which class this object is.
	virtual ServerClass*	GetServerClass() = 0;

	// Return a combo of the EFL_ flags.
	virtual int				GetEFlags() const = 0;
	virtual void			SetEFlags( int iEFlags ) = 0;

	virtual edict_t			*GetEdict() const = 0;

public:

	// This is called once per frame per client per visible area on each entity. The entity should
	// call pInfo->SetShouldTransmit if it wants to be sent to the specified client.
	//
	// This is also where an entity can force other entities to be transmitted if it refers to them
	// with ehandles.
	virtual void			CheckTransmit( CCheckTransmitInfo *pInfo ) = 0;

	// Asks whether a particular entity has changed network state this frame.
	virtual EntityChange_t	DetectNetworkStateChanges() = 0;

	// Called by the engine to indicate that its processed the state changes
	virtual void			ResetNetworkStateChanges() = 0;

	// In place of a generic QueryInterface.
	virtual CBaseNetworkable* GetBaseNetworkable() = 0;
	virtual CBaseEntity*	GetBaseEntity() = 0; // Only used by game code.
};


#endif // ISERVERNETWORKABLE_H
