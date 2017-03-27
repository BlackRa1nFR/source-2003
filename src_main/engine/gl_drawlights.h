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
// $NoKeywords: $
//=============================================================================

#ifndef GL_DRAWLIGHTS_H
#define GL_DRAWLIGHTS_H

#ifdef _WIN32
#pragma once
#endif

// Should we draw light sprites over visible lights?
bool ActivateLightSprites( bool bActive );

// Draws sprites over all visible lights
void DrawLightSprites( void );

// Draws lighting debugging information
void DrawLightDebuggingInfo( void );

#endif // GL_DRAWLIGHTS_H
