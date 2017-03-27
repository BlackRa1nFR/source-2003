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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( IMAPOVERVIEW_H )
#define IMAPOVERVIEW_H
#ifdef _WIN32
#pragma once
#endif

// #include "interface.h"

// #define INTERFACEVERSION_HLTVPANEL	"HLTVPANEL001"

//-----------------------------------------------------------------------------
// Purpose: interface for map overview panel
//-----------------------------------------------------------------------------

#include <vector.h>
#include <vector2d.h>

class IMapOverview // : public IBaseInterface
{
public:
	virtual	~IMapOverview( void ) {};

	virtual	void SetVisible(bool state) = 0;	// set map panel visible
	virtual void SetBounds(int x, int y, int wide, int tall) = 0; // set pos & size
	virtual void SetZoom( float zoom ) = 0; // set zoom
	virtual void SetAngle( float angle) = 0; // set map orientation
	virtual void SetCenter( Vector2D &mappos) = 0; // set map pos in center of panel
	virtual Vector2D WorldToMap( Vector &worldpos ) = 0; // convert 3d world to 2d map pos

	virtual bool IsVisible() = 0;	// true if MapOverview is visible
	virtual void GetBounds(int& x, int& y, int& wide, int& tall) = 0; // get current pos & size

	// deatils properties
	virtual	void ShowPlayerNames(bool state) = 0;	// show player names under icons
	virtual	void ShowTracers(bool state) = 0;	// show shooting traces as lines
	virtual	void ShowExplosions(bool state) = 0;	// show, smoke, flash & HE grenades
	virtual	void ShowHealth(bool state) = 0;		// show player health under icon
	virtual	void ShowHurts(bool state) = 0;	// show player icon flashing if player is hurt
	virtual	void ShowTrails(float seconds) = 0; // show player trails for n seconds
};

#endif // IMAPOVERVIEW_H