//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LABELED_SLIDER_H
#define LABELED_SLIDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

enum MouseCode;
class TextImage;
class IBorder;

//-----------------------------------------------------------------------------
// Labeled horizontal slider
//-----------------------------------------------------------------------------
class CLabeledSlider : public Panel
{
public:
	CLabeledSlider(Panel *parent, const char *panelName );

	// interface
	virtual void SetValue(int value); 
	virtual int  GetValue();
    virtual void SetRange(int min,int max);	 // set to max and min range of rows to display
	virtual void GetRange(int& min,int& max);
	virtual void SetSize(int wide,int tall);
	virtual void GetNobPos(int& min, int& max);	// get current CLabeledSlider position
	virtual void SetButtonOffset(int buttonOffset);
	virtual void OnCursorMoved(int x, int y);
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void SetTickCaptions( char const *left, char const *right );
	virtual void SetNumTicks( int ticks );


protected:
	virtual void Paint();
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	virtual void RecomputeNobPosFromValue();
	virtual void RecomputeValueFromNobPos();
	virtual void SendSliderMovedMessage();
	virtual void GetTrackRect( int& x, int& y, int& w, int& h );
	void DrawNob();
	void DrawTicks();
	void DrawTickLabels();
	
	bool _dragging;
	int _nobPos[2];
	int _nobDragStartPos[2];
	int _dragStartPos[2];
	int _range[2];
	int _value;		// the position of the CLabeledSlider, in coordinates as specified by SetRange/SetRangeWindow
	int _buttonOffset;
	IBorder *_sliderBorder;
	IBorder *_insetBorder;

	char		m_szTickCaptions[ 2 ][ 32 ];

	int			m_nNumTicks;

};

}

#endif // LABELED_SLIDER_H





