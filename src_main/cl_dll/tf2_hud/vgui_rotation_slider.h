//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_ROTATION_SLIDER_H
#define VGUI_ROTATION_SLIDER_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Slider.h>
#include "c_baseobject.h"


//-----------------------------------------------------------------------------
//
// Slider for object control
//
//-----------------------------------------------------------------------------
class CRotationSlider : public vgui::Slider
{
	typedef vgui::Slider BaseClass;
	DECLARE_PANELMAP();

public:
	CRotationSlider( vgui::Panel *pParent, const char *pName );
	void SetControlledObject( C_BaseObject *pObject );

	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	void OnSliderMoved( int position );

private:
	CHandle<C_BaseObject>	m_hObject;
	float m_flInitialYaw;
	float m_flYaw;
};


#endif // VGUI_ROTATION_SLIDER_H
