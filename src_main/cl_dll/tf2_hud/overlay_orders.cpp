//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:
//
// $Workfile: $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "vgui_entityimagepanel.h"
#include "CommanderOverlay.h"



class COrderStatusPanel : public CEntityImagePanel
{
	DECLARE_CLASS( COrderStatusPanel, CEntityImagePanel );
public:
	COrderStatusPanel( vgui::Panel *parent, const char *panelName )
		: BaseClass( parent, "COrderStatusPanel" )
	{
	}
};


DECLARE_OVERLAY_FACTORY( COrderStatusPanel, "personal_order" );
