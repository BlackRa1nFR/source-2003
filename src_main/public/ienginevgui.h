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
#if !defined( IENGINEVGUI_H )
#define IENGINEVGUI_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

// Forward declarations.
namespace vgui
{
	class Panel;
};

enum VGUIPANEL
{
	PANEL_ROOT = 0,
	PANEL_GAMEUIDLL,
	PANEL_CLIENTDLL,
	PANEL_TOOLS,
	PANEL_INGAMESCREENS
};

class IEngineVGui
{
public:
	virtual					~IEngineVGui( void ) { }

	virtual vgui::VPANEL	GetPanel( VGUIPANEL type ) = 0;
};

#define VENGINE_VGUI_VERSION	"VEngineVGui001"

extern IEngineVGui *enginevgui;

#endif // IENGINEVGUI_H