//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CHOREOCHANNELWIDGET_H
#define CHOREOCHANNELWIDGET_H
#ifdef _WIN32
#pragma once
#endif

#include "choreowidget.h"
#include "utlvector.h"

class CChoreoEventWidget;
class CChoreoActorWidget;
class CChoreoChannel;
class CChoreoChannelWidget;

//-----------------------------------------------------------------------------
// Purpose: The channel container
//-----------------------------------------------------------------------------
class CChoreoChannelWidget : public CChoreoWidget
{
public:
	typedef CChoreoWidget		BaseClass;

	enum
	{
		FULLMENU = 0,
		NEWEVENTMENU
	};

	// Construction
								CChoreoChannelWidget( CChoreoActorWidget *parent );
	virtual						~CChoreoChannelWidget( void );

	virtual void				Create( void );
	virtual void				Layout( RECT& rc );

	virtual	int					GetItemHeight( void );

	virtual void				redraw(CChoreoWidgetDrawHelper& drawHelper);

	// Accessors
	CChoreoChannel				*GetChannel( void );
	void						SetChannel( CChoreoChannel *channel );

	// Manipulate child events
	void						AddEvent( CChoreoEventWidget *event );
	void						RemoveEvent( CChoreoEventWidget *event );

	void						MoveEventToTail( CChoreoEventWidget *event );

	CChoreoEventWidget			*GetEvent( int num );
	int							GetNumEvents( void );

	// Determine time for click position
	float						GetTimeForMousePosition( int mx );

private:

	int							GetVerticalStackingCount( bool dolayout, RECT* rc );
	void						LayoutEventInRow( CChoreoEventWidget *event, int row, RECT& rc );

	// The actor to whom we belong
	CChoreoActorWidget			*m_pParent;

	// The underlying scene object
	CChoreoChannel				*m_pChannel;

	// Children
	CUtlVector < CChoreoEventWidget * >	m_Events;
};

#endif // CHOREOCHANNELWIDGET_H
