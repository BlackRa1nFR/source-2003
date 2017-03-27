//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This is the public interface sported by all client-side entities
// Methods that belong in IClientEntityInternal are ones that we need all
// client entities to implement but which we don't want to expose to the server. 
//
// $Revision:$
// $NoKeywords: $

//=============================================================================
#ifndef ICLIENTVEHICLE_H
#define ICLIENTVEHICLE_H

#ifdef _WIN32
#pragma once
#endif

#include "IVehicle.h"

class C_BasePlayer;
class Vector;
class QAngle;
class C_BaseEntity;


//-----------------------------------------------------------------------------
// Purpose: All client vehicles must implement this interface.
//-----------------------------------------------------------------------------
class IClientVehicle : public IVehicle
{
public:
	// When a player is in a vehicle, here's where the camera will be
	virtual void GetVehicleFOV( float &flFOV ) = 0;

	// Allows the vehicle to restrict view angles, blend, etc.
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) = 0;

	// Hud redraw...
	virtual void DrawHudElements() = 0;

	// Is this predicted?
	virtual bool IsPredicted() const = 0;

	// Get the entity associated with the vehicle.
	virtual C_BaseEntity *GetVehicleEnt() = 0;

	// Allows the vehicle to change the near clip plane
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const = 0;
};


#endif // ICLIENTVEHICLE_H
