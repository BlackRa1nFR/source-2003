//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include "hlfaceposer.h"
#include "choreoeventwidget.h"
#include "choreochannelwidget.h"
#include "choreowidgetdrawhelper.h"
#include "choreoview.h"
#include "choreoevent.h"
#include "choreochannel.h"
#include "choreoscene.h"
#include "choreoviewcolors.h"
#include "ifaceposersound.h"
#include "snd_audio_source.h"
#include "RampTool.h"

// Static members
mxbitmapdata_t CChoreoEventWidget::m_Bitmaps[ FP_NUM_BITMAPS ];
mxbitmapdata_t CChoreoEventWidget::m_ResumeConditionBitmap;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CChoreoEventWidget::CChoreoEventWidget( CChoreoWidget *parent )
 : CChoreoWidget( parent )
{
	m_pEvent			= NULL;
	m_pParent			= parent;
	m_pWaveFile			= NULL;
	m_nDurationRightEdge= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEventWidget::~CChoreoEventWidget( void )
{
	delete m_pWaveFile;
	m_pWaveFile = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::Create( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEventWidget::GetLabelText( void )
{
	static char label[ 256 ];
	if ( GetEvent()->GetType() == CChoreoEvent::EXPRESSION )
	{
		sprintf( label, "%s : %s", GetEvent()->GetParameters(), GetEvent()->GetParameters2() );
	}
	else
	{
		strcpy( label, GetEvent()->GetParameters() );
	}

	return label;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CChoreoEventWidget::GetDurationRightEdge( void )
{
	return m_nDurationRightEdge;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rc - 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::Layout( RECT& rc )
{
	int requestedW = rc.right - rc.left;

	m_nDurationRightEdge = requestedW;

	setBounds( rc.left, rc.top, requestedW, rc.bottom - rc.top );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			&rcWAV - 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::DrawRelativeTags( CChoreoWidgetDrawHelper& drawHelper, RECT &rcWAV, float length, CChoreoEvent *event )
{
	for ( int i = 0; i < event->GetNumRelativeTags(); i++ )
	{
		CEventRelativeTag *tag = event->GetRelativeTag( i );
		if ( !tag )
			continue;

		// 
		int left = rcWAV.left + (int)( tag->GetPercentage() * ( float )( rcWAV.right - rcWAV.left ) + 0.5f );

		RECT rcMark;
		rcMark = rcWAV;
		rcMark.top -= 2;
		rcMark.bottom = rcMark.top + 6;
		rcMark.left = left - 3;
		rcMark.right = left + 3;

		drawHelper.DrawTriangleMarker( rcMark, RGB( 0, 100, 250 ) );

		RECT rcText;
		rcText = rcMark;
		rcText.top -= 12;
		
		int len = drawHelper.CalcTextWidth( "Arial", 9, FW_NORMAL, tag->GetName() );
		rcText.left = left - len / 2;
		rcText.right = rcText.left + len + 2;

		rcText.bottom = rcText.top + 10;

		drawHelper.DrawColoredText( "Arial", 9, FW_NORMAL, RGB( 0, 100, 200 ), rcText, tag->GetName() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			&rcWAV - 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::DrawAbsoluteTags( CChoreoWidgetDrawHelper& drawHelper, RECT &rcWAV, float length, CChoreoEvent *event )
{
	for ( int i = 0; i < event->GetNumAbsoluteTags( CChoreoEvent::PLAYBACK ); i++ )
	{
		CEventAbsoluteTag *tag = event->GetAbsoluteTag( CChoreoEvent::PLAYBACK, i );
		if ( !tag )
			continue;

		// 
		int left = rcWAV.left + (int)( tag->GetTime() * ( float )( rcWAV.right - rcWAV.left ) + 0.5f );

		RECT rcMark;
		rcMark = rcWAV;
		rcMark.top -= 2;
		rcMark.bottom = rcMark.top + 6;
		rcMark.left = left - 3;
		rcMark.right = left + 3;

		drawHelper.DrawTriangleMarker( rcMark, RGB( 0, 100, 250 ) );

		RECT rcText;
		rcText = rcMark;
		rcText.top -= 12;
		
		int len = drawHelper.CalcTextWidth( "Arial", 9, FW_NORMAL, tag->GetName() );
		rcText.left = left - len / 2;
		rcText.right = rcText.left + len + 2;

		rcText.bottom = rcText.top + 10;

		drawHelper.DrawColoredText( "Arial", 9, FW_NORMAL, RGB( 0, 100, 200 ), rcText, tag->GetName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			rcBounds - 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::redrawStatus( CChoreoWidgetDrawHelper& drawHelper, RECT& rcClient )
{
	if ( !getVisible() )
		return;

	CChoreoEvent *event = GetEvent();
	if ( !event )
		return;

	int deflateborder = 1;
	int fontsize = 9;

	HDC dc = drawHelper.GrabDC();

	// Now draw the label
	RECT rcEventLabel;
	rcEventLabel = rcClient;

	InflateRect( &rcEventLabel, 0, -deflateborder );

	// rcEventLabel.top += 2;
	rcEventLabel.left += 2;
	//rcEventLabel.top = rcEventLabel.bottom - 2 * ( fontsize + 2 ) - 1;
	//rcEventLabel.bottom = rcEventLabel.top + fontsize + 2;

	int leftAdd = 16;

	if ( CChoreoEventWidget::GetImage( event->GetType() ) )
	{
		mxbitmapdata_t *image = CChoreoEventWidget::GetImage( event->GetType() );
		if ( image )
		{
			RECT rcFixed = rcEventLabel;
			drawHelper.OffsetSubRect( rcFixed );
			DrawBitmapToDC( dc, rcFixed.left, rcFixed.top, leftAdd, leftAdd,
				*image );	
		}
	}

	if ( event->IsResumeCondition() )
	{
		RECT rc = rcEventLabel;
		OffsetRect( &rc, 16, 0 );
		rc.right = rc.left + 16;

		RECT rcFixed = rc;
		drawHelper.OffsetSubRect( rcFixed );
		DrawBitmapToDC( dc, rcFixed.left, rcFixed.top, 
			rcFixed.right - rcFixed.left, rcFixed.bottom - rcFixed.top,
			*CChoreoEventWidget::GetPauseImage() );
	}

	// Draw Type Name:
	//rcEventLabel.top -= 4;

	rcEventLabel.left = rcClient.left + 32;
	rcEventLabel.bottom = rcEventLabel.top + fontsize + 2;
	// OffsetRect( &rcEventLabel, 0, 2 );

	int len = drawHelper.CalcTextWidth( "Arial", fontsize, FW_NORMAL, "%s event \"%s\"", event->NameForType( event->GetType() ), event->GetName() );
	drawHelper.DrawColoredText( "Arial", fontsize, FW_NORMAL, COLOR_INFO_TEXT, rcEventLabel, "%s event \"%s\"", event->NameForType( event->GetType() ), event->GetName() );

	OffsetRect( &rcEventLabel, 0, fontsize + 2 );

	drawHelper.DrawColoredText( "Arial", fontsize, FW_NORMAL, COLOR_INFO_TEXT, rcEventLabel, "parameters \"%s\"", GetLabelText() );

}

//-----------------------------------------------------------------------------
// Purpose: FIXME:  This should either be embedded or we should draw the caption
//  here
//-----------------------------------------------------------------------------
void CChoreoEventWidget::redraw( CChoreoWidgetDrawHelper& drawHelper )
{
	if ( !getVisible() )
		return;

	CChoreoEvent *event = GetEvent();
	if ( !event )
		return;

	int deflateborder = 1;
	int fontsize = 9;

	HDC dc = drawHelper.GrabDC();
	RECT rcClient = getBounds();

	RECT rcDC;
	drawHelper.GetClientRect( rcDC );

	RECT dummy;
	if ( !IntersectRect( &dummy, &rcDC, &rcClient ) )
		return;

	bool ramponly = m_pView->IsRampOnly();

	if ( IsSelected() && !ramponly )
	{
		InflateRect( &rcClient, 3, 1 );
		//rcClient.bottom -= 1;
		rcClient.right += 1;

		RECT rcFrame = rcClient;
		RECT rcBorder = rcClient;

		rcFrame.bottom = rcFrame.top + 17;
		rcBorder.bottom = rcFrame.top + 17;

		COLORREF clrSelection = RGB( 0, 63, 63 );
		COLORREF clrBorder = RGB( 100, 200, 255 );

		HBRUSH brBorder = CreateSolidBrush( clrBorder );
		HBRUSH brSelected = CreateHatchBrush( HS_FDIAGONAL, clrSelection );
		for ( int i = 0; i < 2; i++ )
		{
			FrameRect( dc, &rcFrame, brSelected );
			InflateRect( &rcFrame, -1, -1 );
		}
		FrameRect( dc, &rcBorder, brBorder );
		FrameRect( dc, &rcFrame, brBorder );

		DeleteObject( brSelected );
		DeleteObject( brBorder );
		rcClient.right -= 1;
		//rcClient.bottom += 1;
		InflateRect( &rcClient, -3, -1 );
	}	

	RECT rcEvent;
	rcEvent = rcClient;

	InflateRect( &rcEvent, 0, -deflateborder );

	rcEvent.bottom = rcEvent.top + 10;

	if ( event->GetType() == CChoreoEvent::SPEAK && m_pWaveFile && !event->HasEndTime() )
	{
		event->SetEndTime( event->GetStartTime() + m_pWaveFile->GetRunningLength() );
		rcEvent.right = ( int )( m_pWaveFile->GetRunningLength() * m_pView->GetPixelsPerSecond() );  
	}

	if ( event->HasEndTime() )
	{
		rcEvent.right = rcEvent.left + m_nDurationRightEdge;

		RECT rcEventLine = rcEvent;
		OffsetRect( &rcEventLine, 0, 1 );

		if ( event->GetType() == CChoreoEvent::SPEAK )
		{
			if ( m_pWaveFile )
			{
				HBRUSH brEvent = CreateSolidBrush( COLOR_CHOREO_EVENT );
				HBRUSH brBackground = CreateSolidBrush( COLOR_CHOREO_DARKBACKGROUND );

				if ( !ramponly )
				{
					FillRect( dc, &rcEventLine, brBackground );
				}

				// Only draw wav form here if selected
				if ( IsSelected() )
				{
					sound->RenderWavToDC( dc, rcEventLine, IsSelected() ? COLOR_CHOREO_EVENT_SELECTED : COLOR_CHOREO_EVENT, 0.0, m_pWaveFile->GetRunningLength(), m_pWaveFile );
				}

				//FrameRect( dc, &rcEventLine, brEvent );
				drawHelper.DrawColoredLine( COLOR_CHOREO_EVENT, PS_SOLID, 3,
					rcEventLine.left, rcEventLine.top, rcEventLine.left, rcEventLine.bottom );
				drawHelper.DrawColoredLine( COLOR_CHOREO_EVENT, PS_SOLID, 3,
					rcEventLine.right, rcEventLine.top, rcEventLine.right, rcEventLine.bottom );
				
				DeleteObject( brBackground );
				DeleteObject( brEvent );

				//rcEventLine.top -= 3;
				DrawRelativeTags( drawHelper, rcEventLine, m_pWaveFile->GetRunningLength(), event );
			}
		}
		else
		{
			COLORREF clrEvent = IsSelected() ? COLOR_CHOREO_EVENT_SELECTED : COLOR_CHOREO_EVENT;
			if ( event->GetType() == CChoreoEvent::SUBSCENE )
			{
				clrEvent = RGB( 200, 180, 200 );
			}

			HBRUSH brEvent = CreateSolidBrush( clrEvent );
		
			if ( !ramponly )
			{
				FillRect( dc, &rcEventLine, brEvent );
			}
		
			DeleteObject( brEvent );

			if ( ramponly && IsSelected() )
			{
				drawHelper.DrawOutlinedRect( RGB( 150, 180, 250 ), PS_SOLID, 1,
					rcEventLine );
			}
			else
			{
				drawHelper.DrawColoredLine( RGB( 127, 127, 127 ), PS_SOLID, 1, rcEventLine.left, rcEventLine.bottom,
					rcEventLine.left, rcEventLine.top );
				drawHelper.DrawColoredLine( RGB( 127, 127, 127 ), PS_SOLID, 1, rcEventLine.left, rcEventLine.top,
					rcEventLine.right, rcEventLine.top );
				drawHelper.DrawColoredLine( RGB( 31, 31, 31 ), PS_SOLID, 1, rcEventLine.right, rcEventLine.top,
					rcEventLine.right, rcEventLine.bottom );
				drawHelper.DrawColoredLine( RGB( 0, 0, 0 ), PS_SOLID, 1, rcEventLine.right, rcEventLine.bottom,
					rcEventLine.left, rcEventLine.bottom );
			}

			g_pRampTool->DrawSamplesSimple( drawHelper, event, false, RGB( 63, 63, 63 ), rcEventLine );

			DrawRelativeTags( drawHelper, rcEventLine, event->GetDuration(), event );
			DrawAbsoluteTags( drawHelper, rcEventLine, event->GetDuration(), event );

		}
	}
	else
	{
		RECT rcEventLine = rcEvent;
		OffsetRect( &rcEventLine, 0, 1 );

		drawHelper.DrawColoredLine( COLOR_CHOREO_EVENT, PS_SOLID, 3,
			rcEventLine.left - 1, rcEventLine.top, rcEventLine.left - 1, rcEventLine.bottom );
	}

	if ( event->IsUsingRelativeTag() )
	{
		RECT rcTagName;
		rcTagName = rcClient;

		int length = drawHelper.CalcTextWidth( "Arial", 9, FW_NORMAL, event->GetRelativeTagName() );

		rcTagName.right = rcTagName.left;
		rcTagName.left = rcTagName.right - length - 4;
		rcTagName.top += 3;
		rcTagName.bottom = rcTagName.top + 10;
		
		drawHelper.DrawColoredText( "Arial", 9, FW_NORMAL, RGB( 0, 100, 200 ), rcTagName, event->GetRelativeTagName() );

		drawHelper.DrawFilledRect( RGB( 0, 100, 250 ), rcTagName.right-1, rcTagName.top-2,
			rcTagName.right+2, rcTagName.bottom + 2 );

	}

	// Now draw the label
	RECT rcEventLabel;
	rcEventLabel = rcClient;

	InflateRect( &rcEventLabel, 0, -deflateborder );

	rcEventLabel.top += 15; // rcEventLabel.bottom - 2 * ( fontsize + 2 ) - 1;
	rcEventLabel.bottom = rcEventLabel.top + fontsize + 2;
	rcEventLabel.left += 1;

	//rcEventLabel.left -= 8;

	int leftAdd = 16;

	if ( CChoreoEventWidget::GetImage( event->GetType() ) )
	{
		mxbitmapdata_t *image = CChoreoEventWidget::GetImage( event->GetType() );
		if ( image )
		{
			DrawBitmapToDC( dc, rcEventLabel.left, rcEventLabel.top, leftAdd, leftAdd,
				*image );	
		}
	}

	if ( event->IsResumeCondition() )
	{
		RECT rc = rcEventLabel;
		OffsetRect( &rc, leftAdd, 0 );
		rc.right = rc.left + leftAdd;

		DrawBitmapToDC( dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			*CChoreoEventWidget::GetPauseImage() );
	}

	//rcEventLabel.left += 8;

	OffsetRect( &rcEventLabel, 18, 1 );
	
	int len = drawHelper.CalcTextWidth( "Arial", fontsize, FW_NORMAL, event->GetName() );

	rcEventLabel.right = rcEventLabel.left + len + 2;
	drawHelper.DrawColoredText( "Arial", fontsize, FW_NORMAL, RGB( 0, 0, 120 ), 
		rcEventLabel, event->GetName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoEventWidget::GetEvent( void )
{
	return m_pEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::SetEvent( CChoreoEvent *event )
{
	sound->StopAll();

	delete m_pWaveFile;
	m_pWaveFile = NULL;

	m_pEvent = event;

	if ( event->GetType() == CChoreoEvent::SPEAK )
	{
		m_pWaveFile = sound->LoadSound( va( "sound/%s", FacePoser_TranslateSoundName( event->GetParameters() ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::LoadImages( void )
{
	for ( int i = 0; i < FP_NUM_BITMAPS; i++ )
	{
		m_Bitmaps[ i ].valid = false;
	}

	m_ResumeConditionBitmap.valid = false;

	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_expression.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::EXPRESSION ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_lookat.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::LOOKAT ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_moveto.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::MOVETO ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_speak.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::SPEAK ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_gesture.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::GESTURE ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_face.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::FACE ] );

	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_firetrigger.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::FIRETRIGGER ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_sequence.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::SEQUENCE ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_flexanimation.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::FLEXANIMATION ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_subscene.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::SUBSCENE ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_loop.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::LOOP ] );

	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/pause.bmp", GetGameDirectory() ), m_ResumeConditionBitmap );

	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_interrupt.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::INTERRUPT ] );
	LoadBitmapFromFile( va( "%s/gfx/hlfaceposer/ev_stoppoint.bmp", GetGameDirectory() ), m_Bitmaps[ CChoreoEvent::STOPPOINT ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEventWidget::DestroyImages( void )
{
	for ( int i = 0; i < FP_NUM_BITMAPS; i++ )
	{
		if ( m_Bitmaps[ i ].valid )
		{
			m_Bitmaps[ i ].valid = false;
			DeleteObject( m_Bitmaps[ i ].image );
			m_Bitmaps[ i ].image = NULL;
		}
	}

	if ( m_ResumeConditionBitmap.valid )
	{
		m_ResumeConditionBitmap.valid = false;
		DeleteObject( m_ResumeConditionBitmap.image );
		m_ResumeConditionBitmap.image = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : mxbitmapdata_t
//-----------------------------------------------------------------------------
mxbitmapdata_t *CChoreoEventWidget::GetImage( int type )
{
	return &m_Bitmaps[ type ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : mxbitmapdata_t
//-----------------------------------------------------------------------------
mxbitmapdata_t *CChoreoEventWidget::GetPauseImage( void )
{
	return &m_ResumeConditionBitmap;
}