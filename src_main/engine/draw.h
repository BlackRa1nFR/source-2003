//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:	Some little utility drawing methods
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#ifndef DRAW_H
#define DRAW_H

#ifdef _WIN32
#pragma once
#endif

void Draw_Init (void);
void Draw_Shutdown ( void );

void	Draw_WireframeBox( const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b );
void	Draw_AlphaBox( const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b , int a);
void	Draw_Line( const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest );
void	Draw_Triangle( const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest );

#endif			// DRAW_H
