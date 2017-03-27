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

#ifndef VGUI_BITMAPBUTTON_H
#define VGUI_BITMAPBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Button.h>
#include "VGui_BitmapImage.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class KeyValues;

//-----------------------------------------------------------------------------
// A button that renders images instead of standard vgui stuff...
//-----------------------------------------------------------------------------
class CBitmapButton : public vgui::Button
{
	typedef vgui::Button BaseClass;

public:
	enum ButtonImageType_t
	{
		BUTTON_ENABLED = 0,
		BUTTON_ENABLED_MOUSE_OVER,
		BUTTON_PRESSED,
		BUTTON_DISABLED,

		BUTTON_STATE_COUNT
	};

	// constructor
	CBitmapButton( vgui::Panel *pParent, const char *pName, const char *pText );
	~CBitmapButton();

	// initialization
	bool Init( KeyValues* pInitData );

	void SetImage( ButtonImageType_t type, const char *pMaterialName, color32 color );
	bool IsImageLoaded( ButtonImageType_t type ) const;

	// initialization from build-mode dialog style .res files
	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void Paint( void );
	virtual void PaintBackground( void ) {}

private:

	BitmapImage	m_pImage[BUTTON_STATE_COUNT];
	bool m_bImageLoaded[BUTTON_STATE_COUNT];
};


#endif //  VGUI_BITMAPBUTTON_H