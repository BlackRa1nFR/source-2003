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

#ifndef VGUI_BITMAPPANEL_H
#define VGUI_BITMAPPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class KeyValues;
class BitmapImage;

//-----------------------------------------------------------------------------
// This is a base class for a panel which always is rendered on top of an entity
//-----------------------------------------------------------------------------
class CBitmapPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	// constructor
	CBitmapPanel( );
	CBitmapPanel( vgui::Panel *pParent, const char *pName );
	~CBitmapPanel();

	// initialization
	bool Init( KeyValues* pInitData );

	// initialization from build-mode dialog style .res files
	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void Paint( void );
	virtual void PaintBackground( void ) {}

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	// Setup for panels that aren't created by the commander overlay factory (i.e. aren't parsed from a keyvalues file)
	virtual void SetImage( BitmapImage *pImage );

	const char *GetMouseOverText( void );

private:
	enum
	{
		MAX_ENTITY_MOUSEOVER = 256
	};
	// The bitmap to render
	BitmapImage *m_pImage;
	int m_r, m_g, m_b, m_a;
	bool m_bOwnsImage;

	char			m_szMouseOverText[ MAX_ENTITY_MOUSEOVER ];

};

#endif //  VGUI_BITMAPPANEL_H