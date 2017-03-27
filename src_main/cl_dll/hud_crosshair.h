//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HUD_CROSSHAIR_H
#define HUD_CROSSHAIR_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

namespace vgui
{
	class IScheme;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCrosshair : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudCrosshair, vgui::Panel );
public:
	CHudCrosshair( const char *pElementName );

	void				SetCrosshairAngle( const QAngle& angle );
	void				SetCrosshair( CHudTexture *texture, Color& clr );
	void				DrawCrosshair( void );
	bool				HasCrosshair( void )		{ return ( m_pCrosshair != NULL ); }

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );


private:
	// Crosshair sprite and colors
	CHudTexture			*m_pCrosshair;
	Color				m_clrCrosshair;
	QAngle				m_vecCrossHairOffsetAngle;

	QAngle				m_curViewAngles;
	Vector				m_curViewOrigin;
};

#endif // HUD_CROSSHAIR_H
