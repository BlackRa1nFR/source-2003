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
#if !defined( HUD_HANDLERS_H )
#define HUD_HANDLERS_H
#ifdef _WIN32
#pragma once
#endif

// message handling
int		hudClientCmd( char *szCmdString );
int		hudServerCmd( char *szCmdString, bool bReliable );
// info handling
void	hudGetPlayerInfo( int ent_num, hud_player_info_t *pinfo );

#endif // HUD_HANDLERS_H