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
// $NoKeywords: $
//=============================================================================
#ifndef VGUI_HEALTHBAR_H
#define VGUI_HEALTHBAR_H

#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Panel.h>

class KeyValues;

class CHealthBarPanel : public vgui::Panel
{
public:
	CHealthBarPanel( vgui::Panel *pParent = NULL );
	virtual ~CHealthBarPanel( void );

	// Setup
	bool Init( KeyValues* pInitData );
	void SetGoodColor( int r, int g, int b, int a );
	void SetBadColor( int r, int g, int b, int a );
	void SetVertical( bool bVertical );

	// Health is expected to go from 0 to 1
	void SetHealth( float health );

	virtual void Paint( void );
	virtual void PaintBackground( void ) {}

private:
	float	m_Health;
	Color m_Ok;
	Color m_Bad;
	bool	m_bVertical;	// True if this bar should be vertical
};

#endif // VGUI_HEALTHBAR_H