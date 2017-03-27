//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include <mx/mxPopupMenu.h>
#include "hlfaceposer.h"
#include "choreochannelwidget.h"
#include "choreoeventwidget.h"
#include "choreoactorwidget.h"
#include "choreochannel.h"
#include "choreowidgetdrawhelper.h"
#include "choreoview.h"
#include "choreoevent.h"
#include "choreoviewcolors.h"
#include "utlrbtree.h"
#include "utllinkedlist.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CChoreoChannelWidget::CChoreoChannelWidget( CChoreoActorWidget *parent )
: CChoreoWidget( parent )
{
	m_pChannel = NULL;
	m_pParent = parent;
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoChannelWidget::~CChoreoChannelWidget( void )
{
	for ( int i = 0 ; i < m_Events.Size(); i++ )
	{
		CChoreoEventWidget *e = m_Events[ i ];
		delete e;
	}
	m_Events.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Create child windows
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::Create( void )
{
	Assert( m_pChannel );

	// Create objects for children
	for ( int i = 0; i < m_pChannel->GetNumEvents(); i++ )
	{
		CChoreoEvent *e = m_pChannel->GetEvent( i );
		Assert( e );
		if ( !e )
		{
			continue;
		}

		CChoreoEventWidget *eventWidget = new CChoreoEventWidget( this );
		eventWidget->SetEvent( e );
		eventWidget->Create();
		
		AddEvent( eventWidget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoChannelWidget::GetTimeForMousePosition( int mx )
{
	int dx = mx - m_pView->GetLabelWidth();
	float windowfrac = ( float ) dx / ( float ) ( w() -  m_pView->GetLabelWidth() );
	float time = m_pView->GetStartTime() + windowfrac * ( m_pView->GetEndTime() - m_pView->GetStartTime() );
	return time;
}

bool EventStartTimeLessFunc( CChoreoEventWidget * const &p1, CChoreoEventWidget * const  &p2 )
{
	CChoreoEventWidget *w1;
	CChoreoEventWidget *w2;

	w1 = const_cast< CChoreoEventWidget * >( p1 );
	w2 = const_cast< CChoreoEventWidget * >( p2 );

	CChoreoEvent *e1;
	CChoreoEvent *e2;

	e1 = w1->GetEvent();
	e2 = w2->GetEvent();

	return e1->GetStartTime() < e2->GetStartTime();
}

void CChoreoChannelWidget::LayoutEventInRow( CChoreoEventWidget *event, int row, RECT& rc )
{
	int itemHeight = BaseClass::GetItemHeight();

	RECT rcEvent;
	rcEvent.left = m_pView->GetPixelForTimeValue( event->GetEvent()->GetStartTime() );
	if ( event->GetEvent()->HasEndTime() )
	{
		rcEvent.right = m_pView->GetPixelForTimeValue( event->GetEvent()->GetEndTime() );
	}
	else
	{
		rcEvent.right = rcEvent.left + 8;
	}
	rcEvent.top = rc.top + ( row ) * itemHeight + 2;
	rcEvent.bottom = rc.top + ( row + 1 ) * itemHeight - 2;
	event->Layout( rcEvent );
}

static bool EventCollidesWithRows( CUtlLinkedList< CChoreoEventWidget *, int >& list, CChoreoEventWidget *event )
{
	float st = event->GetEvent()->GetStartTime();
	float ed = event->GetEvent()->HasEndTime() ? event->GetEvent()->GetEndTime() : event->GetEvent()->GetStartTime();

	for ( int i = list.Head(); i != list.InvalidIndex(); i = list.Next( i ) )
	{
		CChoreoEvent *test = list[ i ]->GetEvent();

		float teststart = test->GetStartTime();
		float testend = test->HasEndTime() ? test->GetEndTime() : test->GetStartTime();

		// See if spans overlap
		if ( teststart >= ed )
			continue;
		if ( testend <= st )
			continue;

		return true;
	}

	return false;
}

int CChoreoChannelWidget::GetVerticalStackingCount( bool layout, RECT *rc )
{
	int stackCount = 1;

	CUtlRBTree< CChoreoEventWidget * >  sorted( 0, 0, EventStartTimeLessFunc );

	CUtlVector< CUtlLinkedList< CChoreoEventWidget *, int > >	rows;

	int i;
	// Sort items
	int c = m_Events.Size();
	for ( i = 0; i < c; i++ )
	{
		sorted.Insert( m_Events[ i ] );
	}

	for ( i = sorted.FirstInorder(); i != sorted.InvalidIndex(); i = sorted.NextInorder( i ) )
	{
		CChoreoEventWidget *event = sorted[ i ];
		Assert( event );
		CChoreoEvent *e = event->GetEvent();

		float endtime = e->HasEndTime() ? e->GetEndTime() : e->GetStartTime();
		float starttime = e->GetStartTime();

		if ( !rows.Count() )
		{
			rows.AddToTail();

			CUtlLinkedList< CChoreoEventWidget *, int >& list = rows[ 0 ];
			list.AddToHead( event );

			if ( layout )
			{
				LayoutEventInRow( event, 0, *rc );
			}
			continue;
		}

		// Does it come totally after what's in rows[0]?
		int rowCount = rows.Count();
		bool addrow = true;

		for ( int j = 0; j < rowCount; j++ )
		{
			CUtlLinkedList< CChoreoEventWidget *, int >& list = rows[ j ];

			if ( !EventCollidesWithRows( list, event ) )
			{
				// Update row event list
				list.AddToHead( event );
				addrow = false;
				if ( layout )
				{
					LayoutEventInRow( event, j, *rc );
				}
				break;
			}
		}

		if ( addrow )
		{
			// Add a new row
			int idx = rows.AddToTail();
			CUtlLinkedList< CChoreoEventWidget *, int >& list = rows[ idx ];
			list.AddToHead( event );
			if ( layout )
			{
				LayoutEventInRow( event, rows.Count() - 1, *rc );
			}
		}
	}

	return max( 1, rows.Count() );
}

int	CChoreoChannelWidget::GetItemHeight( void )
{
	int itemHeight = BaseClass::GetItemHeight();
	int stackCount = GetVerticalStackingCount( false, NULL );
	return stackCount * itemHeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rc - 
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::Layout( RECT& rc )
{
	setBounds( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top );

	GetVerticalStackingCount( true, &rc );

	/*
	// Create objects for children
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		CChoreoEventWidget *event = m_Events[ i ];
		Assert( event );
		if ( !event )
		{
			continue;
		}

		RECT rcEvent;
		rcEvent.left = m_pView->GetPixelForTimeValue( event->GetEvent()->GetStartTime() );
		if ( event->GetEvent()->HasEndTime() )
		{
			rcEvent.right = m_pView->GetPixelForTimeValue( event->GetEvent()->GetEndTime() );
		}
		else
		{
			rcEvent.right = rcEvent.left + 8;
		}
		rcEvent.top = rc.top + 2;
		rcEvent.bottom = rc.bottom - 2;
		event->Layout( rcEvent );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::redraw( CChoreoWidgetDrawHelper& drawHelper )
{
	if ( !getVisible() )
		return;

	CChoreoChannel *channel = GetChannel();
	if ( !channel )
		return;

	RECT rcText;
	rcText = getBounds();

	rcText.right = m_pView->GetLabelWidth();

	if ( !channel->GetActive() )
	{
		RECT rcBg = rcText;
		InflateRect( &rcBg, -5, -5 );

		drawHelper.DrawFilledRect( RGB( 210, 210, 210 ), rcBg );
	}

	RECT rcName = rcText;

	rcName.left += 20;
	char n[ 512 ];
	strcpy( n, channel->GetName() );

	drawHelper.DrawColoredText( "Arial", 
		m_pView->GetFontSize() + 2, 
		FW_HEAVY, 
		channel->GetActive() ? COLOR_CHOREO_CHANNELNAME : COLOR_CHOREO_ACTORNAME_INACTIVE,
		rcName, n );

	if ( !channel->GetActive() )
	{
		strcpy( n, "(inactive)" );

		RECT rcInactive = rcName;
		int len = drawHelper.CalcTextWidth( "Arial", m_pView->GetFontSize(), 500, n );
		rcInactive.left = rcInactive.right - len;
		//rcInactive.top += 3;
		//rcInactive.bottom = rcInactive.top + m_pView->GetFontSize() - 2;

		drawHelper.DrawColoredText( "Arial", m_pView->GetFontSize() - 2, 500,
			COLOR_CHOREO_ACTORNAME_INACTIVE, rcInactive, n );
	}

	rcName.left -= 20;

	RECT rcEventArea = getBounds();
	rcEventArea.left = m_pView->GetLabelWidth() + 1;
	rcEventArea.top -= 20;

	drawHelper.StartClipping( rcEventArea );

	for ( int j =  GetNumEvents()-1; j >= 0; j-- )
	{
		CChoreoEventWidget *event = GetEvent( j );
		if ( event )
		{
			event->redraw( drawHelper );
		}
	}

	drawHelper.StopClipping();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoChannelWidget::GetChannel( void )
{
	return m_pChannel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::SetChannel( CChoreoChannel *channel )
{
	m_pChannel = channel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::AddEvent( CChoreoEventWidget *event )
{
	m_Events.AddToTail( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::RemoveEvent( CChoreoEventWidget *event )
{
	m_Events.FindAndRemove( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : num - 
// Output : CChoreoEventWidget
//-----------------------------------------------------------------------------
CChoreoEventWidget *CChoreoChannelWidget::GetEvent( int num )
{
	return m_Events[ num ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoChannelWidget::GetNumEvents( void )
{
	return m_Events.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannelWidget::MoveEventToTail( CChoreoEventWidget *event )
{
	for ( int i = 0; i < GetNumEvents(); i++ )
	{
		CChoreoEventWidget *ew = GetEvent( i );
		if ( ew == event )
		{
			m_Events.Remove( i );
			m_Events.AddToTail( ew );
			break;
		}
	}
}
