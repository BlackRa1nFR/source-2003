//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_IMAGEHEALTHPANEL_H
#define VGUI_IMAGEHEALTHPANEL_H

#include "vgui_EntityPanel.h"
#include "vgui_EntityImagePanel.h"
#include "vgui_HealthBar.h"
#include "vgui_BitmapPanel.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class C_BaseEntity;
class KeyValues;
class BitmapImage;

//-----------------------------------------------------------------------------
// This is a base class for a panel which always is rendered on top of an entity
//-----------------------------------------------------------------------------

class CEntityImageHealthPanel : public CEntityPanel
{
public:
	DECLARE_CLASS( CEntityImageHealthPanel, CEntityPanel );

	// constructor
	CEntityImageHealthPanel( vgui::Panel *parent, const char *panelName );
	~CEntityImageHealthPanel();

	// initialization
	bool Init( KeyValues* pInitData, C_BaseEntity* pEntity );

	// called when we're ticked...
	virtual void OnTick();
	virtual bool ShouldDraw( void );
	virtual void ComputeAndSetSize( void );

private:
	CHealthBarPanel			*m_CommanderHealthBar;
	CHealthBarPanel			*m_NormalHealthBar;
	CHealthBarPanel			*m_ResourceLevelBar;
	CEntityTeamImagePanel	*m_pImagePanel;
};

#endif //  VGUI_IMAGEHEALTHPANEL_H