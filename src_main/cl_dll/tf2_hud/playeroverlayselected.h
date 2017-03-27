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
#if !defined( PLAYEROLVERLAYSELECTED_H )
#define PLAYEROLVERLAYSELECTED_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_BasePanel.h"

class BitmapImage;
class KeyValues;
class CHudPlayerOverlay;
class CHudPlayerOverlaySelected : public CBasePanel
{
public:
	DECLARE_CLASS( CHudPlayerOverlaySelected, CBasePanel );

	CHudPlayerOverlaySelected( CHudPlayerOverlay *baseOverlay );
	virtual ~CHudPlayerOverlaySelected( void );

	bool Init( KeyValues* pInitData );

	void SetImages( BitmapImage *pImage[4] );

	virtual void Paint( void );

	virtual void OnCursorEntered();
	virtual void OnCursorExited();


private:
	BitmapImage *m_pImages[4];

	Color m_fgColor;
	Color m_bgColor;
	CHudPlayerOverlay *m_pBaseOverlay;
};

#endif // PLAYEROLVERLAYSELECTED_H