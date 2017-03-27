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

#include <stdio.h>

#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/LabeledSlider.h>
#include <vgui_controls/TextImage.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Create a slider bar with ticks underneath it
//-----------------------------------------------------------------------------
CLabeledSlider::CLabeledSlider(Panel *parent, const char *panelName ) : Panel(parent, panelName)
{
	_dragging=false;
	_value=0;
	_range[0]=0;
	_range[1]=0;
	_buttonOffset=0;
	_sliderBorder=NULL;
	_insetBorder =NULL;
	m_nNumTicks = 10;
	m_szTickCaptions[ 0 ][ 0 ] = 0;
	m_szTickCaptions[ 1 ][ 0 ] = 0;

	RecomputeNobPosFromValue();
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the slider bar.
//   Warning less than 30 pixels tall and everything probably won't fit.
//-----------------------------------------------------------------------------
void CLabeledSlider::SetSize(int wide,int tall)
{
	Panel::SetSize(wide,tall);

	RecomputeNobPosFromValue();
}

//-----------------------------------------------------------------------------
// Purpose: Set the value of the slider to one of the ticks.
//-----------------------------------------------------------------------------
void CLabeledSlider::SetValue(int value)
{
	int oldValue=_value;

	if(value<_range[0])
	{
		value=_range[0];
	}

	if(value>_range[1])
	{
		value=_range[1];	
	}

	_value=value;
	
	RecomputeNobPosFromValue();

	if (_value != oldValue)
	{
		SendSliderMovedMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of the slider
//-----------------------------------------------------------------------------
int CLabeledSlider::GetValue()
{
	return _value;
}

//-----------------------------------------------------------------------------
// Purpose: Layout the slider before drawing it on screen.
//-----------------------------------------------------------------------------
void CLabeledSlider::PerformLayout()
{
	RecomputeNobPosFromValue();
}

//-----------------------------------------------------------------------------
// Purpose: Move the nob on the slider in response to changing its value.
//-----------------------------------------------------------------------------
void CLabeledSlider::RecomputeNobPosFromValue()
{
	int wide,tall;
	GetPaintSize(wide,tall);

	tall -= 20;

	float fwide=(float)wide;
	float ftall=(float)tall;
	float frange=(float)(_range[1] -_range[0]);
	float fvalue=(float)(_value -_range[0]);
	float fper=fvalue / (frange);

	float fnobsize = 8;

	float freepixels = fwide - fnobsize;
	float leftpixel = fnobsize / 2.0f;
	float firstpixel = leftpixel + freepixels * fper;

	_nobPos[0]=(int)( firstpixel );
	_nobPos[1]=(int)( firstpixel + fnobsize );

	if(_nobPos[1]>wide)
	{
		_nobPos[0]=wide-((int)fnobsize);
		_nobPos[1]=wide;
	}
	
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sync the slider's value up with the nob's position.
//-----------------------------------------------------------------------------
void CLabeledSlider::RecomputeValueFromNobPos()
{
	int wide, tall;
	GetPaintSize(wide, tall);

	float fwide = (float)wide;
	float ftall = (float)tall;
	float frange = (float)( _range[1] - _range[0] );
	float fvalue = (float)( _value - _range[0] );
	float fnob = (float)_nobPos[0];

	float fnobsize = 8;

	float freepixels = fwide - fnobsize;
	float leftpixel = fnobsize / 2.0f;

	// Map into reduced range
	fvalue = ( fnob - leftpixel ) / freepixels;

	// Scale by true range
	fvalue *= frange;

	// Take care of rounding issues.
	_value = (int)( fvalue + _range[0] + 0.5);
}

//-----------------------------------------------------------------------------
// Purpose: Send a message to interested parties when the slider moves
//-----------------------------------------------------------------------------
void CLabeledSlider::SendSliderMovedMessage()
{	
	// send a changed message
	PostActionSignal(new KeyValues("SliderMoved", "position", _value));

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void CLabeledSlider::ApplySchemeSettings(IScheme *pScheme)
{
	Panel::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("Slider/SliderFgColor", pScheme));
	// this line is useful for debugging
	//SetBgColor(GetSchemeColor("Slider/SliderBgColor"));

	_sliderBorder = pScheme->GetBorder("ButtonBorder");
	_insetBorder = pScheme->GetBorder("ButtonDepressedBorder" );
}

//-----------------------------------------------------------------------------
// Purpose: Get the rectangle to draw the slider track in.
//-----------------------------------------------------------------------------
void CLabeledSlider::GetTrackRect( int& x, int& y, int& w, int& h )
{
	int wide, tall;
	GetPaintSize( wide, tall );

	x = 2;
	y = 8;
	w = wide - 4;
	h = 4;
}

//-----------------------------------------------------------------------------
// Purpose: Draw everything on screen
//-----------------------------------------------------------------------------
void CLabeledSlider::Paint()
{
	DrawTicks();

	DrawTickLabels();

	// Draw nob last so it draws over ticks.
	DrawNob();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the ticks below the slider.
//-----------------------------------------------------------------------------
void CLabeledSlider::DrawTicks()
{
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );

	// Figure out how to draw the ticks
	GetPaintSize( wide, tall );

	float fwide  = (float)wide;
	float fnobsize = 8;
	float freepixels = fwide - fnobsize;

	float leftpixel = fnobsize / 2.0f;

	float pixelspertick = freepixels / ( m_nNumTicks );

	y =  y + (int)fnobsize;
	int tickHeight = 5;

	surface()->DrawSetColor( Color( 127, 140, 127, 255 ) );

	for ( int i = 0; i <= m_nNumTicks; i++ )
	{
		int xpos = (int)( leftpixel + i * pixelspertick );

		surface()->DrawFilledRect( xpos, y, xpos + 1, y + tickHeight );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw Tick labels under the ticks.
//-----------------------------------------------------------------------------
void CLabeledSlider::DrawTickLabels()
{
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );

	// Figure out how to draw the ticks
	GetPaintSize( wide, tall );

	float fwide  = (float)wide;
	float fnobsize = 8;
	float freepixels = fwide - fnobsize;

	float leftpixel = fnobsize / 2.0f;

	float pixelspertick = freepixels / ( m_nNumTicks );

	y =  y + (int)fnobsize + 4;

	// Draw Start and end range values
	surface()->DrawSetTextColor( Color( 127, 140, 127, 255 ) );
	
	surface()->DrawSetTextPos( 0, y);

	IScheme *pScheme = scheme()->GetIScheme( scheme()->GetDefaultScheme() );
	if ( m_szTickCaptions[ 0 ][ 0 ] )
	{
		surface()->DrawSetTextFont(pScheme->GetFont("DefaultVerySmall"));

		surface()->DrawPrintText( (const unsigned short *)m_szTickCaptions[ 0 ], strlen( m_szTickCaptions[ 0 ] ) );
	}

	if ( m_szTickCaptions[ 1 ][ 0 ] )
	{
		surface()->DrawSetTextFont(pScheme->GetFont("DefaultVerySmall"));

		// was 5 *  now is 2 *
		surface()->DrawSetTextPos( (int)( m_nNumTicks * pixelspertick - 2 * strlen( m_szTickCaptions[ 1 ] ) ), y);

		surface()->DrawPrintText((const unsigned short *) m_szTickCaptions[ 1 ], strlen( m_szTickCaptions[ 1 ] ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw the nob part of the slider.
//-----------------------------------------------------------------------------
void CLabeledSlider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );
	Color col = GetFgColor();
	surface()->DrawSetColor(col);

	int nobheight = 16;

	surface()->DrawFilledRect(
		_nobPos[0], 
		y + tall / 2 - nobheight / 2, 
		_nobPos[1], 
		y + tall / 2 + nobheight / 2);
	// border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0], 
			y + tall / 2 - nobheight / 2, 
			_nobPos[1], 
			y + tall / 2 + nobheight / 2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the text labels of the Start and end ticks.
//-----------------------------------------------------------------------------
void CLabeledSlider::SetTickCaptions( char const *left, char const *right )
{
	strcpy( m_szTickCaptions[ 0 ], left );
	strcpy( m_szTickCaptions[ 1 ], right );
}

//-----------------------------------------------------------------------------
// Purpose: Draw the slider track
//-----------------------------------------------------------------------------
void CLabeledSlider::PaintBackground()
{
	Panel::PaintBackground();
	
	int x, y;
	int wide,tall;

	GetTrackRect( x, y, wide, tall );

	surface()->DrawSetColor( Color(31,31,31,255) );

	surface()->DrawFilledRect( x, y, x + wide, y + tall );
	if (_insetBorder)
	{
		_insetBorder->Paint( x, y, x + wide, y + tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the slider.
//-----------------------------------------------------------------------------
void CLabeledSlider::SetRange(int min,int max)
{
	if(max<min)
	{
		max=min;
	}

	if(min>max)
	{
		min=max;
	}

	_range[0]=min;
	_range[1]=max;
}

//-----------------------------------------------------------------------------
// Purpose: Get the max and min values of the slider
//-----------------------------------------------------------------------------
void CLabeledSlider::GetRange(int& min,int& max)
{
	min=_range[0];
	max=_range[1];
}

//-----------------------------------------------------------------------------
// Purpose: Respond when the cursor is moved in our window if we are clicking
// and dragging.
//-----------------------------------------------------------------------------
void CLabeledSlider::OnCursorMoved(int x,int y)
{
	if(!_dragging)
	{
		return;
	}

	input()->GetCursorPos(x,y);
	ScreenToLocal(x,y);

	int wide,tall;
	GetPaintSize(wide,tall);

	_nobPos[0]=_nobDragStartPos[0]+(x-_dragStartPos[0]);
	_nobPos[1]=_nobDragStartPos[1]+(x-_dragStartPos[0]);
	if(_nobPos[1]>wide)
	{
		_nobPos[0]=wide-(_nobPos[1]-_nobPos[0]);
		_nobPos[1]=wide;
	}
		
	if(_nobPos[0]<0)
	{
		_nobPos[1]=_nobPos[1]-_nobPos[0];
		_nobPos[0]=0;
	}

	RecomputeValueFromNobPos();
	Repaint();
	SendSliderMovedMessage();
}

//-----------------------------------------------------------------------------
// Purpose: Respond to mouse presses. Trigger Record staring positon.
//-----------------------------------------------------------------------------
void CLabeledSlider::OnMousePressed(MouseCode code)
{
	int x,y;
	input()->GetCursorPos(x,y);
	ScreenToLocal(x,y);

	if((x>=_nobPos[0])&&(x<_nobPos[1]))
	{
		_dragging=true;
		input()->SetMouseCapture(GetVPanel());
		_nobDragStartPos[0]=_nobPos[0];
		_nobDragStartPos[1]=_nobPos[1];
		_dragStartPos[0]=x;
		_dragStartPos[1]=y;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Just handle double presses like mouse presses
//-----------------------------------------------------------------------------
void CLabeledSlider::OnMouseDoublePressed(MouseCode code)
{
	OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: Stop dragging when the mouse is released.
//-----------------------------------------------------------------------------
void CLabeledSlider::OnMouseReleased(MouseCode code)
{
	_dragging=false;
	input()->SetMouseCapture(null);
}

//-----------------------------------------------------------------------------
// Purpose: Get the nob's position (the ends of each side of the nob)
//-----------------------------------------------------------------------------
void CLabeledSlider::GetNobPos(int& min, int& max)
{
	min=_nobPos[0];
	max=_nobPos[1];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLabeledSlider::SetButtonOffset(int buttonOffset)
{
	_buttonOffset=buttonOffset;
}


//-----------------------------------------------------------------------------
// Purpose: Set the number of ticks that appear under the slider.
//-----------------------------------------------------------------------------
void CLabeledSlider::SetNumTicks( int ticks )
{
	m_nNumTicks = ticks;
}
