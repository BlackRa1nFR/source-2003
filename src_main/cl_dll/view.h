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

#if !defined( VIEW_H )
#define VIEW_H
#ifdef _WIN32
#pragma once
#endif

class VMatrix;
class Vector;
class QAngle;
class VPlane;


// near and far Z it uses to render the world.
#define VIEW_NEARZ	8
//#define VIEW_FARZ	28400


//-----------------------------------------------------------------------------
// There's a difference between the 'current view' and the 'main view'
// The 'main view' is where the player is sitting. Current view is just
// what's currently being rendered, which, owing to monitors or water,
// could be just about anywhere.
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin();
const QAngle &MainViewAngles();
const VMatrix &MainWorldToViewMatrix();
const Vector &MainViewForward();
const Vector &MainViewRight();
const Vector &MainViewUp();

const Vector &CurrentViewOrigin();
const QAngle &CurrentViewAngles();
const VMatrix &CurrentWorldToViewMatrix();
const Vector &CurrentViewForward();
const Vector &CurrentViewRight();
const Vector &CurrentViewUp();

void AllowCurrentViewAccess( bool allow );

// Returns true of the sphere is outside the frustum defined by pPlanes.
// (planes point inwards).
bool R_CullSphere( const VPlane *pPlanes, int nPlanes, const Vector *pCenter, float radius );

#endif // VIEW_H