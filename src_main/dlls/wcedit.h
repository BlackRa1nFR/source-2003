//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Namespace for functions having to do with WC Edit mode
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef WCEDIT_H
#define WCEDIT_H
#pragma once

class CBaseEntity;

//=============================================================================
//	>> NWCEdit
//=============================================================================
namespace NWCEdit
{
	Vector	AirNodePlacementPosition( void );
	bool	IsWCVersionValid(void);
	void	CreateAINode(   CBasePlayer *pPlayer );
	void	DestroyAINode(  CBasePlayer *pPlayer );
	void	CreateAILink(	CBasePlayer *pPlayer );
	void	DestroyAILink(  CBasePlayer *pPlayer );
	void	UndoDestroyAINode(void);
};

#endif // WCEDIT_H