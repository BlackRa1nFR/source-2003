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
#if !defined( SCREEN_H )
#define SCREEN_H
#ifdef _WIN32
#pragma once
#endif

void SCR_Init (void);
void SCR_Shutdown( void );

void SCR_UpdateScreen (void);
void SCR_CenterPrint (char *str);
void SCR_CenterStringOff( void );
void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);

extern	qboolean	scr_disabled_for_loading;
extern	qboolean	scr_skipupdate;

#endif // SCREEN_H