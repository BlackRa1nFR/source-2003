//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TIMELINEITEM_H
#define TIMELINEITEM_H
#ifdef _WIN32
#pragma once
#endif

#include <mx/mx.h>
#include "UtlVector.h"
#include "ExpressionSample.h"

class CExpression;
class ExpressionTool;
class CFlexAnimationTrack;
class CChoreoWidgetDrawHelper;
class CChoreoView;

class TimelineItem /*: public mxWindow*/
{
public:
	// Construction
						TimelineItem( mxWindow *workspace );
						~TimelineItem( void );

	virtual int			handleEvent( mxEvent *event );
	virtual void		Draw( CChoreoWidgetDrawHelper& drawHelper );
	void				DrawEventEnd( CChoreoWidgetDrawHelper& drawHelper );
	void				DrawSelf( void );

	void				SetExpressionInfo( CFlexAnimationTrack *track, int flexnum );

	void				Clear( bool preserveundo );

	void				SetCollapsed( bool state );
	bool				IsCollapsed( void ) const;

	void				SetActive( bool state );
	bool				IsActive( void );

	int					GetHeight( void );
	void				ResetHeight();

	// If samples > 0
	bool				IsValid( void );

	void				SetEditType( int type );
	int					GetEditType( void );

	void				SetBounds( const RECT& rect );

	void				GetBounds( RECT& rect );

	void				SetVisible( bool vis );
	bool				GetVisible( void ) const;

	void				CountSelected( void );
	CFlexAnimationTrack	*GetSafeTrack( void );
	int					GetNumSelected( void );

	void				Copy( void );
	void				Paste( void );

	void				SelectAll( void );
	void				DeselectAll( void );
	void				Delete( void );

	void				GetLastMouse( int& mx, int& my )
	{
		mx = m_nLastX;
		my = m_nLastY;
	}

	void				SnapAll();
	void				SnapSelected();
	void				DeletePoints( float start, float end );

	float				GetTimeForMouse( int mx, bool clip = false );
	int					GetMouseForTime( float t, bool *clipped = NULL );

private:
	enum
	{
		DRAGTYPE_NONE = 0,
		DRAGTYPE_MOVEPOINTS_VALUE,
		DRAGTYPE_MOVEPOINTS_TIME,
		DRAGTYPE_SELECTION,
		DRAGTYPE_GROW,
	};

	void				PreDataChanged( char *undodescription );
	void				PostDataChanged( char *redodescription );

	bool				CanHaveGrowHandle();
	void				DrawGrowHandle( CChoreoWidgetDrawHelper& helper, RECT& handleRect );
	void				GetGrowHandleRect( RECT& rc );
	bool				IsMouseOverGrowHandle( int x, int y);

	CExpressionSample	*GetSampleUnderMouse( mxEvent *event );

	void				MouseDrag( int x, int y, int modifiers, bool snap = false );

	void				DrawGrowRect();

	void				DrawFocusRect( void );
	void				SelectPoints( void );

	void				OnDoubleClicked( void );

	int					m_nDragging;
	int					m_nLastX;
	int					m_nLastY;

	int					m_nStartX;
	int					m_nStartY;

	void				AddSample( CExpressionSample const& sample );

	void				DrawRelativeTags( CChoreoWidgetDrawHelper& drawHelper );

	int					m_nNumSelected;

	int					m_nFlexNum;

	bool				m_bCollapsed;

	char				m_szTrackName[ 128 ];

	int					m_nEditType;

	int					m_bUndoSetup;
	RECT				m_rcBounds;
	bool				m_bVisible;

	mxWindow			*m_pWorkspace;
	double				m_flLastClickTime;

	int					m_nCurrentHeight;
};

#endif // TIMELINEITEM_H
