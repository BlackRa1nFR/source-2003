//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#if !defined( ICLIENTENTITYLIST_H )
#define ICLIENTENTITYLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IClientEntity;
class ClientClass;
class IClientNetworkable;
class CBaseHandle;
class IClientUnknown;


//-----------------------------------------------------------------------------
// Purpose: Exposes IClientEntity's to engine
//-----------------------------------------------------------------------------
class IClientEntityList
{
public:
	// Get IClientNetworkable interface for specified entity
	virtual IClientNetworkable*	GetClientNetworkable( int entnum ) = 0;
	virtual IClientNetworkable*	GetClientNetworkableFromHandle( CBaseHandle hEnt ) = 0;
	virtual IClientUnknown*		GetClientUnknownFromHandle( CBaseHandle hEnt ) = 0;

	// NOTE: This function is only a convenience wrapper.
	// It returns GetClientNetworkable( entnum )->GetIClientEntity().
	virtual IClientEntity*		GetClientEntity( int entnum ) = 0;
	virtual IClientEntity*		GetClientEntityFromHandle( CBaseHandle hEnt ) = 0;

	// Returns number of entities currently in use
	virtual int					NumberOfEntities( void ) = 0;

	// Returns highest index actually used
	virtual int					GetHighestEntityIndex( void ) = 0;

	// Sizes entity list to specified size
	virtual void				SetMaxEntities( int maxents ) = 0;
	virtual int					GetMaxEntities( ) = 0;
};

extern IClientEntityList *entitylist;

#define VCLIENTENTITYLIST_INTERFACE_VERSION	"VClientEntityList002"

#endif // ICLIENTENTITYLIST_H