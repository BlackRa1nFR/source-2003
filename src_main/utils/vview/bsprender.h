//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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

#ifndef BSPRENDER_H
#define BSPRENDER_H
#pragma once

class Vector;

extern int	BspLoad( const char *pFileName );
extern void BspRender( Vector& cameraPosition );
extern void BspFree( void );
extern void BspToggleVis( void );
extern void BspToggleTexture( void );
extern void BspTogglePortals( void );
extern void BspToggleLight( void );
extern void BspToggleBmodels( void );

#endif // BSPRENDER_H
