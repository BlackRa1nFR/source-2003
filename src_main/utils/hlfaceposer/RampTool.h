//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef RAMPTOOL_H
#define RAMPTOOL_H
#ifdef _WIN32
#pragma once
#endif

#include <mx/mx.h>
#include "studio.h"
#include "UtlVector.h"
#include "faceposertoolwindow.h"

class CChoreoEvent;
class CChoreoWidgetDrawHelper;
class CChoreoView;
struct CExpressionSample;

#define IDC_REDO_RT					1000
#define IDC_UNDO_RT					1001

#define IDC_RT_DELETE				1002
#define IDC_RT_DESELECT				1003
#define IDC_RT_SELECTALL			1004

#define IDC_RT_CHANGESCALE			1005
#define IDC_RAMPHSCROLL			1006

class RampTool : public mxWindow, public IFacePoserToolWindow
{
public:
	// Construction
						RampTool( mxWindow *parent );
						~RampTool( void );

	virtual void		Think( float dt );
	void				ScrubThink( float dt, bool scrubbing );
	virtual bool		IsScrubbing( void ) const;
	virtual bool		IsProcessing( void );

	virtual int			handleEvent( mxEvent *event );
	virtual void		redraw( void );
	virtual bool		PaintBackground();

	void				SetEvent( CChoreoEvent *event );

	void				GetScrubHandleRect( RECT& rcHandle, float scrub, bool clipped = false );

	void				DrawScrubHandle( CChoreoWidgetDrawHelper& drawHelper, RECT& rcHandle, float scrub, bool reference );
	void				DrawTimeLine( CChoreoWidgetDrawHelper& drawHelper, RECT& rc, float left, float right );
	void				DrawEventEnd( CChoreoWidgetDrawHelper& drawHelper );

	void				SetMouseOverPos( int x, int y );
	void				GetMouseOverPos( int &x, int& y );
	void				GetMouseOverPosRect( RECT& rcPos );
	void				DrawMouseOverPos( CChoreoWidgetDrawHelper& drawHelper, RECT& rcPos );
	void				DrawMouseOverPos();

	void				DrawScrubHandles();

	CChoreoEvent		*GetSafeEvent( void );

	bool				IsMouseOverScrubHandle( mxEvent *event );
	void				ForceScrubPosition( float newtime );
	void				ForceScrubPositionFromSceneTime( float scenetime );

	void				SetScrubTime( float t );
	void				SetScrubTargetTime( float t );

	void				DrawSamplesSimple( CChoreoWidgetDrawHelper& drawHelper, CChoreoEvent *e, bool clearbackground, COLORREF sampleColor, RECT &rcSamples );
private:

	void				GetSampleTrayRect( RECT& rc );
	void				DrawSamples( CChoreoWidgetDrawHelper& drawHelper, RECT &rcSamples );

	void				SelectPoints( void );
	CExpressionSample	*GetSampleUnderMouse( mxEvent *event );
	void				DeselectAll();
	void				SelectAll();
	void				Delete( void );

	int					CountSelectedSamples( void );
	void				MoveSelectedSamples( float dfdx, float dfdy );

	void				StartDragging( int dragtype, int startx, int starty, HCURSOR cursor );
	void				AddFocusRect( RECT& rc );
	void				OnMouseMove( mxEvent *event );
	void				DrawFocusRect( void );
	void				ShowContextMenu( mxEvent *event, bool include_track_menus );
	void				GetWorkspaceLeftRight( int& left, int& right );
	void				SetClickedPos( int x, int y );
	float				GetTimeForClickedPos( void );
	
	void				ApplyBounds( int& mx, int& my );
	void				CalcBounds( int movetype );
	void				OnUndo( void );
	void				OnRedo( void );

	//CEventAbsoluteTag	*IsMouseOverTag( int mx, int my );
	void				OnRevert( void );

	void				DrawTimingTags( CChoreoWidgetDrawHelper& drawHelper, RECT& rc );
	void				DrawRelativeTagsForEvent( CChoreoWidgetDrawHelper& drawHelper, RECT& rc, CChoreoEvent *rampevent, CChoreoEvent *event, float starttime, float endtime );
	void				DrawAbsoluteTagsForEvent( CChoreoWidgetDrawHelper& drawHelper, RECT &rc, CChoreoEvent *rampevent, CChoreoEvent *event, float starttime, float endtime );


	// Readjust slider
	void				MoveTimeSliderToPos( int x );
	void				OnChangeScale();
	int					ComputeHPixelsNeeded( void );
	void				SetTimeZoomScale( int scale );
	float				GetTimeZoomScale( void );
	float				GetPixelsPerSecond( void );
	void				InvalidateLayout( void );
	void				RepositionHSlider( void );
	void				GetStartAndEndTime( float& st, float& ed );
	float				GetTimeValueForMouse( int mx, bool clip = false );
	int					GetPixelForTimeValue( float time, bool *clipped = NULL );

	float				m_flScrub;
	float				m_flScrubTarget;

	enum
	{
		DRAGTYPE_NONE = 0,
		DRAGTYPE_SCRUBBER,
		DRAGTYPE_MOVEPOINTS_VALUE,
		DRAGTYPE_MOVEPOINTS_TIME,
		DRAGTYPE_SELECTION,
	};

	char				m_szEvent[ 128 ];

	int					m_nMousePos[ 2 ];

	bool				m_bUseBounds;
	int					m_nMinX;
	int					m_nMaxX;

	HCURSOR				m_hPrevCursor;
	int					m_nDragType;

	int					m_nStartX;
	int					m_nStartY;
	int					m_nLastX;
	int					m_nLastY;

	int					m_nClickedX;
	int					m_nClickedY;

	struct CFocusRect
	{
		RECT	m_rcOrig;
		RECT	m_rcFocus;
	};
	CUtlVector < CFocusRect >	m_FocusRects;
	CChoreoEvent				*m_pLastEvent;

	bool				m_bSuppressLayout;
	// 100 == normal scale
	int					m_nTimeZoom;
	// Height/width of scroll bars
	int					m_nScrollbarHeight;
	int					m_nLeftOffset;
	mxScrollbar			*m_pHorzScrollBar;
	int					m_nLastHPixelsNeeded;
	// How many pixels per second we are showing in the UI
	float				m_flPixelsPerSecond;
	// Do we need to move controls?
	bool				m_bLayoutIsValid;
	float				m_flLastDuration;
	bool				m_bInSetEvent;
	float				m_flScrubberTimeOffset;
};

extern RampTool	*g_pRampTool;

#endif // RAMPTOOL_H
