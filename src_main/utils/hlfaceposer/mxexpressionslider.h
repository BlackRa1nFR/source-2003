//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MXEXPRESSIONSLIDER_H
#define MXEXPRESSIONSLIDER_H
#ifdef _WIN32
#pragma once
#endif

#include <mx/mxWindow.h>
#include <mx/mxCheckBox.h>

#define IDC_INFLUENCE 1000

class mxExpressionSlider : public mxWindow
{
public:
	enum
	{
		MAGNITUDE_BAR = 0,
		BALANCE_BAR = 1,

		NUMBARS = 2
	};

	mxExpressionSlider (mxWindow *parent, int x, int y, int w, int h, int id );
	~mxExpressionSlider( void );
	
	void SetTitleWidth( int width );
	void SetDrawTitle( bool drawTitle );

	void SetMode( bool paired );

	void setValue ( int barnum, float value);
	void setRange ( int barnum, float min, float max, int ticks = 100);
	void setInfluence ( float value );

	// ACCESSORS
	float getRawValue( int barnum ) const;

	float getValue( int barnum ) const;
	float getMinValue( int barnum ) const;
	float getMaxValue( int barnum ) const;
	float getInfluence( ) const;

	virtual void redraw();
	virtual bool PaintBackground( void );

	virtual int			handleEvent (mxEvent *event);

private:
	void	BoundValue( void );

	void	GetSliderRect( RECT& rc );
	void	GetBarRect( RECT &rc );
	void	GetThumbRect( int barnum, RECT &rc );

	void	DrawBar( HDC& dc );
	void	DrawThumb( int barnum, HDC& dc );
	void	DrawTitle( HDC &dc );

	void	MoveThumb( int barnum, int xpos, bool finish );

	int		m_nTitleWidth;
	bool	m_bDrawTitle;

	float	m_flMin[ NUMBARS ], m_flMax[ NUMBARS ];
	int		m_nTicks[ NUMBARS ];
	float	m_flCurrent[ NUMBARS ];

	float	m_flSetting[ NUMBARS ];

	bool	m_bDraggingThumb;
	int		m_nCurrentBar;

	bool	m_bPaired;

	mxCheckBox	*m_pInfluence;
};


#endif // MXEXPRESSIONSLIDER_H
