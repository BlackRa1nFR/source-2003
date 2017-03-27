//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASENETWORKABLE_H
#define BASENETWORKABLE_H
#ifdef _WIN32
#pragma once
#endif


#include "iservernetworkable.h"
#include "server_class.h"
#include "edict.h"
#include "netstatemgr.h"


//
// Lightweight base class for networkable data on the server.
//
class CBaseNetworkable : public IServerNetworkable
{
public:

	DECLARE_CLASS_NOBASE( CBaseNetworkable );
	DECLARE_SERVERCLASS();


public:

							CBaseNetworkable();
	virtual					~CBaseNetworkable();

	// This must be called when a CBaseNetworkable is created in order
	// to link it to the engine (by creating an edict).
	void					AttachEdict();
	
	// Called by LINK_ENTITY_TO_CLASS. Attaches an edict.
	void					PostConstructor( const char *pClassName );

	int						entindex() const;

	void NetworkStateChanged();


// IHandleEntity overrides.
public:

	virtual void SetRefEHandle( const CBaseHandle &handle );
	virtual const CBaseHandle& GetRefEHandle() const;


// IServerNetworkable implementation.
public:

	virtual EntityChange_t	DetectNetworkStateChanges();
	virtual void			ResetNetworkStateChanges();

	virtual int				GetEFlags() const;
	virtual void			SetEFlags( int iEFlags );

	virtual edict_t			*GetEdict() const;

	virtual void			CheckTransmit( CCheckTransmitInfo *pInfo );

	virtual CBaseNetworkable* GetBaseNetworkable();
	virtual CBaseEntity*	GetBaseEntity();



// Overrideables.
protected:

	virtual bool ShouldTransmit( const edict_t *recipient, 
									  const void *pvs, int clientArea );

	// Derived classes can implement this and pass the SetTransmit call to dependents.
	virtual void			SetTransmit( CCheckTransmitInfo *pInfo );


private:

	// Detaches the edict.. should only be called by CBaseNetworkable's destructor.
	void					DetachEdict();


private:

	CBaseEdict				*m_pPev;

	CBaseHandle m_RefEHandle;

	int						m_iEFlags;	// Combination of EFL_ flags.


protected:

	// Manual mode, timed, etc...
	CNetStateMgr			m_NetStateMgr;
};


#endif // BASENETWORKABLE_H
