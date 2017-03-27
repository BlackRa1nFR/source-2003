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
#if !defined( IVMODEMANAGER_H )
#define IVMODEMANAGER_H
#ifdef _WIN32
#pragma once
#endif

class IVModeManager
{
public:
	virtual void	Init( void ) = 0;
	// HL2 will ignore, TF2 will change modes.
	virtual void	SwitchMode( bool commander, bool force ) = 0;
	virtual void	LevelInit( const char *newmap ) = 0;
	virtual void	LevelShutdown( void ) = 0;
};

extern IVModeManager *modemanager;

#endif // IVMODEMANAGER_H