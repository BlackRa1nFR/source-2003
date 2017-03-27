//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Header file for player-thrown grenades.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef GRENADE_BASE_H
#define GRENADE_BASE_H
#pragma once

#include "basegrenade_shared.h"

class CSprite;

#define GRENADE_TIMER		5		// Try 5 seconds instead of 3?

//-----------------------------------------------------------------------------
// Purpose: Base Thrown-Grenade class
//-----------------------------------------------------------------------------
class CThrownGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CThrownGrenade, CBaseGrenade );

	void	Spawn( void );
	void	Thrown( Vector vecOrigin, Vector vecVelocity, float flExplodeTime );
};



#endif // GRENADE_BASE_H
