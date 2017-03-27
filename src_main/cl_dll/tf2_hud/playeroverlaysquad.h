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
#if !defined( PLAYEROVERLAYSQUAD_H )
#define PLAYEROVERLAYSQUAD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Label.h>

class KeyValues;
class CHudPlayerOverlay;
class CHudPlayerOverlaySquad : public vgui::Label
{
public:
	typedef vgui::Label BaseClass;

	CHudPlayerOverlaySquad( CHudPlayerOverlay *baseOverlay, const char *squadname );
	virtual ~CHudPlayerOverlaySquad( void );

	bool Init( KeyValues* pInitData );

	void SetSquad( const char *squadname );
	
	virtual void	Paint();
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	SetReflectMouse( bool reflect );
	// If reflect mouse is true, then pass these up to parent
	virtual void	OnCursorMoved(int x,int y);
	virtual void	OnMousePressed(vgui::MouseCode code);
	virtual void	OnMouseDoublePressed(vgui::MouseCode code);
	virtual void	OnMouseReleased(vgui::MouseCode code);
	virtual void	OnMouseWheeled(int delta);

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

private:

	char	m_szSquad[ 64 ];
	Color	m_fgColor;
	Color	m_bgColor;
	CHudPlayerOverlay *m_pBaseOverlay;

	bool			m_bReflectMouse;
};

#endif // PLAYEROVERLAYSQUAD_H