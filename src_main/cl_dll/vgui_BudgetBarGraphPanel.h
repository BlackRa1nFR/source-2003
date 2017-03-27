//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef VGUI_BUDGETBARGRAPHPANEL_H
#define VGUI_BUDGETBARGRAPHPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

class CBudgetBarGraphPanel : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	CBudgetBarGraphPanel( vgui::Panel *pParent, const char *pPanelName );
	~CBudgetBarGraphPanel();
	void SetRange( float fMin, float fMax );
	virtual void Paint( void );
private:
	void DrawInstantaneous();
	void DrawPeaks();
	void DrawAverages();
	void DrawTimeLines( void );
	void DrawBarAtIndex( int id, float percent );
	void DrawTickAtIndex( int id, float percent, int red, int green, int blue, int alpha );
	float m_fRangeMin;
	float m_fRangeMax;
};

#endif // VGUI_BUDGETBARGRAPHPANEL_H
