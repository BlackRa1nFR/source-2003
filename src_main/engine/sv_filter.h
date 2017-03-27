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
#if !defined( SV_FILTER_H )
#define SV_FILTER_H
#ifdef _WIN32
#pragma once
#endif

void Filter_Init( void );
void Filter_Shutdown( void );

qboolean Filter_ShouldDiscardID( unsigned int userid );
qboolean Filter_ShouldDiscard( netadr_t *adr );
void Filter_SendBan( netadr_t *adr );

#endif // SV_FILTER_H