//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include <mx/mxWindow.h>
#include "mdlviewer.h"
#include "hlfaceposer.h"
#include "StudioModel.h"
#include "expressions.h"
#include "expclass.h"
#include "ChoreoView.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "choreoscene.h"
#include "choreowidget.h"
#include "choreoactorwidget.h"
#include "choreochannelwidget.h"
#include "choreoglobaleventwidget.h"
#include "choreowidgetdrawhelper.h"
#include "choreoeventwidget.h"
#include "viewerSettings.h"
#include "cmdlib.h"
#include "choreoviewcolors.h"
#include "ActorProperties.h"
#include "ChannelProperties.h"
#include "EventProperties.h"
#include "GlobalEventProperties.h"
#include "ifaceposersound.h"
#include "snd_wave_source.h"
#include "ifaceposerworkspace.h"
#include "PhonemeEditor.h"
#include "scriplib.h"
#include "cmdlib.h"
#include "iscenetokenprocessor.h"
#include "InputProperties.h"
#include "FileSystem.h"
#include "ExpressionTool.h"
#include "ControlPanel.h"
#include "faceposer_models.h"
#include "choiceproperties.h"
#include "MatSysWin.h"
#include "vstdlib/strtools.h"
#include "GestureTool.h"
#include "npcevent.h"
#include "RampTool.h"
#include "SceneRampTool.h"
#include "KeyValues.h"


// 10x magnification
#define MAX_TIME_ZOOM 1000

#define PHONEME_FILTER 0.08f
#define PHONEME_DELAY  0.0f

#define SCRUBBER_HEIGHT	15

extern double realtime;

//-----------------------------------------------------------------------------
// Purpose: Helper to scene module for parsing the .vcd file
//-----------------------------------------------------------------------------
class CSceneTokenProcessor : public ISceneTokenProcessor
{
public:
	const char	*CurrentToken( void );
	bool		GetToken( bool crossline );
	bool		TokenAvailable( void );
	void		Error( const char *fmt, ... );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const
//-----------------------------------------------------------------------------
const char	*CSceneTokenProcessor::CurrentToken( void )
{
	return token;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : crossline - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::GetToken( bool crossline )
{
	return ::GetToken( crossline ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::TokenAvailable( void )
{
	return ::TokenAvailable() ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CSceneTokenProcessor::Error( const char *fmt, ... )
{
	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, string );
	::Error( string );
}

static CSceneTokenProcessor g_TokenProcessor;
ISceneTokenProcessor *tokenprocessor = &g_TokenProcessor;

// Try to keep shifted times at same absolute time
static void RescaleGestureTimes( CChoreoEvent *event, float newstart, float newend )
{
	if ( !event || event->GetType() != CChoreoEvent::GESTURE )
		return;

	// Did it actually change
	if ( newstart == event->GetStartTime() &&
		 newend == event->GetEndTime() )
	{
		 return;
	}

	float newduration = newend - newstart;

	float dt = 0.0f;
	//If the end is moving, leave tags stay where they are (dt == 0.0f)
	if ( newstart != event->GetStartTime() )
	{
		// Otherwise, if the new start is later, then tags need to be shifted backwards
		dt -= ( newstart - event->GetStartTime() );
	}

	int count = event->GetNumAbsoluteTags( CChoreoEvent::SHIFTED );
	int i;

	for ( i = 0; i < count; i++ )
	{
		CEventAbsoluteTag *tag = event->GetAbsoluteTag( CChoreoEvent::SHIFTED, i );
		float tagtime = tag->GetTime() * event->GetDuration();

		tagtime += dt;

		float frac = tagtime / newduration;
		frac = clamp( frac, 0.0f, 1.0f );

		tag->SetTime( frac );
	}

	float expansion =  newduration / event->GetDuration();

	count = event->GetNumAbsoluteTags( CChoreoEvent::PLAYBACK );
	for ( i = 0; i < count; i++ )
	{
		CEventAbsoluteTag *tag = event->GetAbsoluteTag( CChoreoEvent::PLAYBACK, i );
		float tagtime = tag->GetTime() * event->GetDuration();

		tagtime += dt;

		tagtime = clamp( tagtime / newduration, 0.0f, 1.0f );

		tag->SetTime( tagtime );
	}
}

// Try to keep shifted times at same absolute time
static void RescaleExpressionTimes( CChoreoEvent *event, float newstart, float newend )
{
	if ( !event || event->GetType() != CChoreoEvent::FLEXANIMATION )
		return;

	// Did it actually change
	if ( newstart == event->GetStartTime() &&
		 newend == event->GetEndTime() )
	{
		 return;
	}

	float newduration = newend - newstart;

	float dt = 0.0f;
	//If the end is moving, leave tags stay where they are (dt == 0.0f)
	if ( newstart != event->GetStartTime() )
	{
		// Otherwise, if the new start is later, then tags need to be shifted backwards
		dt -= ( newstart - event->GetStartTime() );
	}

	int count = event->GetNumFlexAnimationTracks();
	int i;

	for ( i = 0; i < count; i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		for ( int type = 0; type < 2; type++ )
		{
			int sampleCount = track->GetNumSamples( type );
			for ( int sample = sampleCount - 1; sample >= 0 ; sample-- )
			{
				CExpressionSample *s = track->GetSample( sample, type );
				if ( !s )
					continue;

				s->time += dt;

				if ( s->time > newduration || s->time < 0.0f )
				{
					track->RemoveSample( sample, type );
				}
			}
		}
	}
}

static void RescaleRamp( CChoreoEvent *event, float newduration )
{
	float oldduration = event->GetDuration();

	if ( fabs( oldduration - newduration ) < 0.000001f )
		return;

	if ( newduration <= 0.0f )
		return;

	float midpointtime = oldduration * 0.5f;
	float newmidpointtime = newduration * 0.5f;

	int count = event->GetRampCount();
	int i;

	for ( i = 0; i < count; i++ )
	{
		CExpressionSample *sample = event->GetRamp( i );
		if ( !sample )
			continue;

		float t = sample->time;
		if ( t < midpointtime )
			continue;

		float timefromend = oldduration - t;

		// There's room to just shift it
		if ( timefromend <= newmidpointtime )
		{
			t = newduration - timefromend;
		}
		else
		{
			// No room, rescale them instead
			float frac = ( t - midpointtime ) / midpointtime;
			t = newmidpointtime + frac * newmidpointtime;
		}

		sample->time = t;
	}
}

bool DoesAnyActorHaveAssociatedModelLoaded( CChoreoScene *scene )
{
	if ( !scene )
		return false;

	int c = scene->GetNumActors();
	int i;
	for ( i = 0; i < c; i++ )
	{
		CChoreoActor *a = scene->GetActor( i );
		if ( !a )
			continue;

		char const *modelname = a->GetFacePoserModelName();
		if ( !modelname )
			continue;

		if ( !modelname[ 0 ] )
			continue;

		int idx = models->FindModelByFilename( modelname );
		if ( idx >= 0 )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *a - 
// Output : StudioModel
//-----------------------------------------------------------------------------
StudioModel *FindAssociatedModel( CChoreoScene *scene, CChoreoActor *a )
{
	if ( !a || !scene )
		return NULL;

	Assert( models->GetActiveStudioModel() );

	StudioModel *model = NULL;
	if ( a->GetFacePoserModelName()[ 0 ] )
	{
		int idx = models->FindModelByFilename( a->GetFacePoserModelName() );
		if ( idx >= 0 )
		{
			model = models->GetStudioModel( idx );
			return model;
		}
	}

	// Does any actor have an associated model which is loaded
	if ( DoesAnyActorHaveAssociatedModelLoaded( scene ) )
	{
		// Then return NULL here so we don't override with the default an actor who has a valid model going
		return model;
	}

	// Couldn't find it and nobody else has a loaded associated model, so just use the default model
	if ( !model )
	{
		model = models->GetActiveStudioModel();
	}
	return model;
}


CChoreoView		*g_pChoreoView = 0;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//			x - 
//			y - 
//			w - 
//			h - 
//			id - 
//-----------------------------------------------------------------------------
CChoreoView::CChoreoView( mxWindow *parent, int x, int y, int w, int h, int id )
: IFacePoserToolWindow( "CChoreoView", "Choreography" ), mxWindow( parent, x, y, w, h )
{
	m_bRampOnly = false;

	m_bSuppressLayout = true;
	m_bHasLookTarget = false;

	SetAutoProcess( true );

	m_flLastMouseClickTime = -1.0f;
	m_bProcessSequences = true;

	m_flPlaybackRate	= 1.0f;

	m_pScene = NULL;

	m_flScrub			= 0.0f;
	m_flScrubTarget		= 0.0f;

	m_bCanDraw = false;

	m_bRedoPending = false;
	m_nUndoLevel = 0;

	CChoreoEventWidget::LoadImages();

	CChoreoWidget::m_pView = this;

	setId( id );

	m_flLastSpeedScale = 0.0f;
	m_bResetSpeedScale = false;

	m_nTopOffset = 0;
	m_nLeftOffset = 0;
	m_nLastHPixelsNeeded = -1;
	m_nLastVPixelsNeeded = -1;


	m_nStartRow = 45;
	m_nLabelWidth = 140;
	m_nRowHeight = 35;

	m_bSimulating = false;
	m_bPaused = false;

	m_bForward = true;

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;
	m_flFrameTime = 0.0f;

	m_bAutomated		= false;
	m_nAutomatedAction	= SCENE_ACTION_UNKNOWN;
	m_flAutomationDelay = 0.0f;
	m_flAutomationTime = 0.0f;

	m_nTimeZoom = 100;

	m_pVertScrollBar = new mxScrollbar( this, 0, 0, 18, 100, IDC_CHOREOVSCROLL, mxScrollbar::Vertical );
	m_pHorzScrollBar = new mxScrollbar( this, 0, 0, 18, 100, IDC_CHOREOHSCROLL, mxScrollbar::Horizontal );

	m_bLayoutIsValid = false;
	m_flPixelsPerSecond = 150.0f;

	m_btnPlay = new mxBitmapButton( this, 2, 4, 16, 16, IDC_PLAYSCENE, "gfx/hlfaceposer/play.bmp" );
	m_btnPause = new mxBitmapButton( this, 18, 4, 16, 16, IDC_PAUSESCENE, "gfx/hlfaceposer/pause.bmp"  );
	m_btnStop = new mxBitmapButton( this, 34, 4, 16, 16, IDC_STOPSCENE, "gfx/hlfaceposer/stop.bmp"  );

	m_pPlaybackRate				= new mxSlider( this, 0, 0, 16, 16, IDC_CHOREO_PLAYBACKRATE );
	m_pPlaybackRate->setRange( 0.0, 2.0, 40 );
	m_pPlaybackRate->setValue( m_flPlaybackRate );

	ShowButtons( false );

	m_nFontSize = 12;

	for ( int i = 0; i < MAX_ACTORS; i++ )
	{
		m_ActorExpanded[ i ].expanded = true;
	}

	SetChoreoFile( "" );

	//SetFocus( (HWND)getHandle() );
	if ( workspacefiles->GetNumStoredFiles( IWorkspaceFiles::CHOREODATA ) >= 1 )
	{
		LoadSceneFromFile( workspacefiles->GetStoredFile( IWorkspaceFiles::CHOREODATA, 0 ) );
	}

	ClearABPoints();

	m_pClickedActor = NULL;
	m_pClickedChannel = NULL;
	m_pClickedEvent = NULL;
	m_pClickedGlobalEvent = NULL;
	m_nClickedX = 0;
	m_nClickedY = 0;
	m_nSelectedEvents = 0;
	m_nClickedTag = -1;

	// Mouse dragging
	m_bDragging			= false;
	m_xStart			= 0;
	m_yStart			= 0;
	m_nDragType			= DRAGTYPE_NONE;
	m_hPrevCursor		= 0;


	m_nScrollbarHeight	= 12;
	m_nInfoHeight		= 30;

	ClearStatusArea();

	SetDirty( false );

	m_bCanDraw = true;
	m_bSuppressLayout = false;
	m_flScrubberTimeOffset = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : closing - 
//-----------------------------------------------------------------------------
bool CChoreoView::Close( void )
{
	if ( m_pScene && m_bDirty )
	{
		int retval = mxMessageBox( NULL, va( "Save changes to class '%s'?", GetChoreoFile() ), g_appTitle, MX_MB_YESNOCANCEL );
		if ( retval == 2 )
		{
			return false;
		}
		if ( retval == 0 )
		{
			Save();
		}
	}

	if ( m_pScene )
	{
		UnloadScene();
	}
	return true;
}

bool CChoreoView::CanClose()
{
	if ( m_pScene )
	{
		workspacefiles->StartStoringFiles( IWorkspaceFiles::CHOREODATA );
		workspacefiles->StoreFile( IWorkspaceFiles::CHOREODATA, GetChoreoFile() );
		workspacefiles->FinishStoringFiles( IWorkspaceFiles::CHOREODATA );
	}

	if ( m_pScene && m_bDirty && !Close() )
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called just before window is destroyed
//-----------------------------------------------------------------------------
void CChoreoView::OnDelete()
{
	if ( m_pScene )
	{
		UnloadScene();
	}

	CChoreoWidget::m_pView = NULL;

	CChoreoEventWidget::DestroyImages();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoView::~CChoreoView()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::ReportSceneClearToTools( void )
{
	g_pPhonemeEditor->ClearEvent();
	g_pExpressionTool->LayoutItems( true );
	g_pExpressionTool->redraw();
	g_pGestureTool->redraw();
	g_pRampTool->redraw();
	g_pSceneRampTool->redraw();
}

//-----------------------------------------------------------------------------
// Purpose: Find a time that's less than input on the granularity:
// e.g., 3.01 granularity 0.05 will be 3.00, 3.05 will be 3.05
// Input  : input - 
//			granularity - 
// Output : float
//-----------------------------------------------------------------------------
float SnapTime( float input, float granularity )
{
	float base = (float)(int)input;
	float multiplier = (float)(int)( 1.0f / granularity );
	float fracpart = input - (int)input;

	fracpart *= multiplier;

	fracpart = (float)(int)fracpart;
	fracpart *= granularity;

	return base + fracpart;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			rc - 
//			left - 
//			right - 
//-----------------------------------------------------------------------------
void CChoreoView::DrawTimeLine( CChoreoWidgetDrawHelper& drawHelper, RECT& rc, float left, float right )
{
	RECT rcLabel;
	float granularity = 0.5f / GetTimeZoomScale();

	drawHelper.DrawColoredLine( COLOR_CHOREO_TIMELINE, PS_SOLID, 1, rc.left, GetStartRow() - 1, rc.right, GetStartRow() - 1 );

	float f = SnapTime( left, granularity );
	while ( f < right )
	{
		float frac = ( f - left ) / ( right - left );
		if ( frac >= 0.0f && frac <= 1.0f )
		{
			rcLabel.left = GetLabelWidth() + (int)( frac * ( rc.right - GetLabelWidth() ) );
			rcLabel.bottom = GetStartRow() - 1;
			rcLabel.top = rcLabel.bottom - 10;

			if ( f != left )
			{
				drawHelper.DrawColoredLine( RGB( 220, 220, 240 ), PS_DOT,  1, 
					rcLabel.left, GetStartRow(), rcLabel.left, h2() );
			}

			char sz[ 32 ];
			sprintf( sz, "%.2f", f );

			int textWidth = drawHelper.CalcTextWidth( "Arial", 9, FW_NORMAL, sz );

			rcLabel.right = rcLabel.left + textWidth;

			OffsetRect( &rcLabel, -textWidth / 2, 0 );

			drawHelper.DrawColoredText( "Arial", 9, FW_NORMAL, COLOR_CHOREO_TEXT, rcLabel, sz );

		}
		f += granularity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::PaintBackground( void )
{
	redraw();
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//-----------------------------------------------------------------------------
void CChoreoView::DrawSceneABTicks( CChoreoWidgetDrawHelper& drawHelper )
{
	RECT rcThumb;

	float scenestart = m_rgABPoints[ 0 ].active ? m_rgABPoints[ 0 ].time : 0.0f;
	float sceneend = m_rgABPoints[ 1 ].active ? m_rgABPoints[ 1 ].time : 0.0f;

	if ( scenestart )
	{
		int markerstart = GetPixelForTimeValue( scenestart );

		rcThumb.left = markerstart - 4;
		rcThumb.right = markerstart + 4;
		rcThumb.top = 2 + GetCaptionHeight() + SCRUBBER_HEIGHT;
		rcThumb.bottom = rcThumb.top + 8;

		drawHelper.DrawTriangleMarker( rcThumb, COLOR_CHOREO_TICKAB );
	}

	if ( sceneend )
	{
		int markerend = GetPixelForTimeValue( sceneend );

		rcThumb.left = markerend - 4;
		rcThumb.right = markerend + 4;
		rcThumb.top = 2 + GetCaptionHeight() + SCRUBBER_HEIGHT;
		rcThumb.bottom = rcThumb.top + 8;

		drawHelper.DrawTriangleMarker( rcThumb, COLOR_CHOREO_TICKAB );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			rc - 
//-----------------------------------------------------------------------------
void CChoreoView::DrawRelativeTagLines( CChoreoWidgetDrawHelper& drawHelper, RECT& rc )
{
	if ( !m_pScene )
		return;

	RECT rcClip;
	GetClientRect( (HWND)getHandle(), &rcClip );
	rcClip.top = GetStartRow();
	rcClip.bottom -= ( m_nInfoHeight + m_nScrollbarHeight );
	rcClip.right -= m_nScrollbarHeight;

	drawHelper.StartClipping( rcClip );

	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *c = a->GetChannel( j );
			if ( !c )
				continue;

			for ( int k = 0; k < c->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *e = c->GetEvent( k );
				if ( !e )
					continue;

				CChoreoEvent *event = e->GetEvent();
				if ( !event )
					continue;

				if ( !event->IsUsingRelativeTag() )
					continue;

				// Using it, find the tag and figure out the time for it
				CEventRelativeTag *tag = m_pScene->FindTagByName( 
					event->GetRelativeWavName(),
					event->GetRelativeTagName() );

				if ( !tag )
					continue;

				// Found it, draw a vertical line
				//
				float tagtime = tag->GetStartTime();
				
				// Convert to pixel value
				bool clipped = false;
				int pixel = GetPixelForTimeValue( tagtime, &clipped );
				if ( clipped )
					continue;

				drawHelper.DrawColoredLine( RGB( 180, 180, 220 ), PS_SOLID, 1, 
					pixel, rcClip.top, pixel, rcClip.bottom );
			}
		}
	}

	drawHelper.StopClipping();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the background markings (actor names, etc)
// Input  : drawHelper - 
//			rc - 
//-----------------------------------------------------------------------------
void CChoreoView::DrawBackground( CChoreoWidgetDrawHelper& drawHelper, RECT& rc )
{
	RECT rcClip;
	GetClientRect( (HWND)getHandle(), &rcClip );
	rcClip.top = GetStartRow();
	rcClip.bottom -= ( m_nInfoHeight + m_nScrollbarHeight );
	rcClip.right -= m_nScrollbarHeight;

	int i;

	for ( i = 0; i < m_SceneGlobalEvents.Size(); i++ )
	{
		CChoreoGlobalEventWidget *event = m_SceneGlobalEvents[ i ];
		if ( event )
		{
			event->redraw( drawHelper );
		}
	}

	drawHelper.StartClipping( rcClip );

	for ( i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actorW = m_SceneActors[ i ];
		if ( !actorW )
			continue;

		actorW->redraw( drawHelper );
	}

	drawHelper.StopClipping();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::redraw() 
{
	if ( !ToolCanDraw() )
		return;

	if ( m_bSuppressLayout )
		return;

	LayoutScene();

	CChoreoWidgetDrawHelper drawHelper( this, COLOR_CHOREO_BACKGROUND );
	HandleToolRedraw( drawHelper );

	if ( !m_bCanDraw )
		return;

	RECT rc;
	rc.left = 0;
	rc.top = GetCaptionHeight();
	rc.right = drawHelper.GetWidth();
	rc.bottom = drawHelper.GetHeight();

	RECT rcInfo;
	rcInfo.left = rc.left;
	rcInfo.right = rc.right - m_nScrollbarHeight;
	rcInfo.bottom = rc.bottom - m_nScrollbarHeight;
	rcInfo.top = rcInfo.bottom - m_nInfoHeight;

	drawHelper.StartClipping( rcInfo );

	RedrawStatusArea( drawHelper, rcInfo );

	drawHelper.StopClipping();

	RECT rcClip = rc;
	rcClip.bottom -= ( m_nInfoHeight + m_nScrollbarHeight );

	drawHelper.StartClipping( rcClip );

	if ( !m_pScene )
	{
		char sz[ 256 ];
		sprintf( sz, "No choreography scene file (.vcd) loaded" );

		int pointsize = 18;
		int textlen = drawHelper.CalcTextWidth( "Arial", pointsize, FW_NORMAL, sz );

		RECT rcText;
		rcText.top = ( rc.bottom - rc.top ) / 2 - pointsize / 2;
		rcText.bottom = rcText.top + pointsize + 10;
		rcText.left = rc.right / 2 - textlen / 2;
		rcText.right = rcText.left + textlen;

		drawHelper.DrawColoredText( "Arial", pointsize, FW_NORMAL, COLOR_CHOREO_LIGHTTEXT, rcText, sz );

		drawHelper.StopClipping();
		return;
	}

	DrawTimeLine( drawHelper, rc, m_flStartTime, m_flEndTime );

	bool clipped = false;
	int finishx = GetPixelForTimeValue( m_pScene->FindStopTime(), &clipped );
	if ( !clipped )
	{
		drawHelper.DrawColoredLine( COLOR_CHOREO_ENDTIME, PS_DOT, 1, finishx, rc.top + GetStartRow(), finishx, rc.bottom );
	}

	DrawRelativeTagLines( drawHelper, rc );
	DrawBackground( drawHelper, rc );

	DrawSceneABTicks( drawHelper );

	drawHelper.StopClipping();

	if ( m_UndoStack.Size() > 0 )
	{
		int length = drawHelper.CalcTextWidth( "Arial", 9, FW_NORMAL, 
			"undo %i/%i", m_nUndoLevel, m_UndoStack.Size() );
		RECT rcText = rc;
		rcText.top = rc.top + 48;
		rcText.bottom = rcText.top + 10;
		rcText.left = GetLabelWidth() - length - 20;
		rcText.right = rcText.left + length;

		drawHelper.DrawColoredText( "Arial", 9, FW_NORMAL, RGB( 100, 180, 100 ), rcText,
			"undo %i/%i", m_nUndoLevel, m_UndoStack.Size() );
	}

	DrawScrubHandle( drawHelper );

	char sz[ 48 ];
	sprintf( sz, "Speed: %.2fx", m_flPlaybackRate );

	int fontsize = 9;

	int length = drawHelper.CalcTextWidth( "Arial", fontsize, FW_NORMAL, sz);
	
	RECT rcText = rc;
	rcText.top = rc.top + 25;
	rcText.bottom = rcText.top + 10;
	rcText.left = GetLabelWidth() + 20;
	rcText.right = rcText.left + length;

	drawHelper.DrawColoredText( "Arial", fontsize, FW_NORMAL, 
		RGB( 50, 50, 50 ), rcText, sz );

	sprintf( sz, "Zoom: %.2fx", (float)m_nTimeZoom / 100.0f );

	length = drawHelper.CalcTextWidth( "Arial", fontsize, FW_NORMAL, sz);

	rcText = rc;
	rcText.left = 5;
	rcText.top = rc.top + 48;
	rcText.bottom = rcText.top + 10;
	rcText.right = rcText.left + length;

	drawHelper.DrawColoredText( "Arial", fontsize, FW_NORMAL, 
		RGB( 50, 50, 50 ), rcText, sz );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : current - 
//			number - 
// Output : int
//-----------------------------------------------------------------------------
void CChoreoView::GetUndoLevels( int& current, int& number )
{
	current = m_nUndoLevel;
	number	= m_UndoStack.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			*clipped - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetPixelForTimeValue( float time, bool *clipped /*=NULL*/ )
{
	if ( clipped )
	{
		*clipped = false;
	}

	float frac = ( time - m_flStartTime ) / ( m_flEndTime - m_flStartTime );
	if ( frac < 0.0 || frac > 1.0 )
	{
		if ( clipped )
		{
			*clipped = true;
		}
	}

	int pixel = GetLabelWidth() + (int)( frac * ( w2() - GetLabelWidth() ) );
	return pixel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			clip - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoView::GetTimeValueForMouse( int mx, bool clip /*=false*/)
{
	RECT rc = m_rcTimeLine;
	rc.left = GetLabelWidth();

	if ( clip )
	{
		if ( mx < rc.left )
		{
			return m_flStartTime;
		}
		if ( mx > rc.right )
		{
			return m_flEndTime;
		}
	}

	float frac = (float)( mx - rc.left )  / (float)( rc.right - rc.left );

	return m_flStartTime + frac * ( m_flEndTime - m_flStartTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CChoreoView::SetStartTime( float time )
{
	m_flStartTime = time;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoView::GetStartTime( void )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoView::GetEndTime( void )
{
	return m_flEndTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoView::GetPixelsPerSecond( void )
{
	return m_flPixelsPerSecond * GetTimeZoomScale();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoView::GetTimeZoomScale( void )
{
	return ( float )m_nTimeZoom / 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scale - 
//-----------------------------------------------------------------------------
void CChoreoView::SetTimeZoomScale( int scale )
{
	m_nTimeZoom = scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			origmx - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoView::GetTimeDeltaForMouseDelta( int mx, int origmx )
{
	float t1, t2;

	t2 = GetTimeValueForMouse( mx );
	t1 = GetTimeValueForMouse( origmx );

	return t2 - t1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//-----------------------------------------------------------------------------
void CChoreoView::PlaceABPoint( int mx )
{
	m_rgABPoints[ ( m_nCurrentABPoint) & 0x01 ].time = GetTimeValueForMouse( mx );
	m_rgABPoints[ ( m_nCurrentABPoint) & 0x01 ].active = true;
	m_nCurrentABPoint++;

	if ( m_rgABPoints[ 0 ].active && m_rgABPoints	[ 1 ].active && 
		 m_rgABPoints[ 0 ].time > m_rgABPoints[ 1 ].time )
	{
		float temp = m_rgABPoints[ 0 ].time;
		m_rgABPoints[ 0 ].time = m_rgABPoints[ 1 ].time;
		m_rgABPoints[ 1 ].time = temp;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::ClearABPoints( void )
{
	memset( m_rgABPoints, 0, sizeof( m_rgABPoints ) );
	m_nCurrentABPoint = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::IsMouseOverTimeline( int mx, int my )
{
	POINT pt;
	pt.x = mx;
	pt.y = my;

	RECT rcCheck = m_rcTimeLine;
	rcCheck.bottom -= 8;

	if ( PtInRect( &rcCheck, pt ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::ShowContextMenu( int mx, int my )
{
	CChoreoActorWidget		*a = NULL;
	CChoreoChannelWidget	*c = NULL;
	CChoreoEventWidget		*e = NULL;
	CChoreoGlobalEventWidget *ge = NULL;
	int						ct = -1;

	GetObjectsUnderMouse( mx, my, &a, &c, &e, &ge, &ct );
	
	m_pClickedActor			= a;
	m_pClickedChannel		= c;
	m_pClickedEvent			= e;
	m_pClickedGlobalEvent	= ge;
	m_nClickedX				= mx;
	m_nClickedY				= my;
	m_nClickedTag			= ct;

	// Construct main
	mxPopupMenu *pop = new mxPopupMenu();

	if ( a && c )
	{
		if (!e)
		{
			pop->add( "Expression...", IDC_ADDEVENT_EXPRESSION );
			pop->add( "WAV File...", IDC_ADDEVENT_SPEAK );
			pop->add( "Gesture...", IDC_ADDEVENT_GESTURE );
			pop->add( "Look at actor...", IDC_ADDEVENT_LOOKAT );
			pop->add( "Move to actor...", IDC_ADDEVENT_MOVETO );
			pop->add( "Face actor...", IDC_ADDEVENT_FACE );
			pop->add( "Fire Trigger...", IDC_ADDEVENT_FIRETRIGGER );
			pop->add( "Sequence...", IDC_ADDEVENT_SEQUENCE );
			pop->add( "Flex animation...", IDC_ADDEVENT_FLEXANIMATION );
			pop->add( "Sub-scene...", IDC_ADDEVENT_SUBSCENE );
			pop->add( "Interrupt...", IDC_ADDEVENT_INTERRUPT );

			if ( e && e->GetEvent()->HasEndTime() )
			{
				pop->add( "Timing Tag...", IDC_ADDTIMINGTAG );
			}

			pop->addSeparator();
		}
		else
		{
			pop->add( va( "Edit Event '%s'...", e->GetEvent()->GetName() ), IDC_EDITEVENT );
			switch ( e->GetEvent()->GetType() )
			{
			default:
				break;
			case CChoreoEvent::FLEXANIMATION:
				{
					pop->add( va( "Edit Event '%s' in expression tool", e->GetEvent()->GetName() ), IDC_EXPRESSIONTOOL );
				}
				break;
			case CChoreoEvent::GESTURE:
				{
					pop->add( va( "Edit Event '%s' in gesture tool", e->GetEvent()->GetName() ), IDC_GESTURETOOL );
				}
				break;
			}
			pop->addSeparator();
		}
	}

	// Construct "New..."
	mxPopupMenu *newMenu = new mxPopupMenu();
	{
		newMenu->add( "Actor...", IDC_ADDACTOR );
		if ( a )
		{
			newMenu->add( "Channel...", IDC_ADDCHANNEL );
		}
		newMenu->add( "Section Pause...", IDC_ADDEVENT_PAUSE );
		newMenu->add( "Loop...", IDC_ADDEVENT_LOOP );
		newMenu->add( "Stop point...", IDC_ADDEVENT_STOPPOINT );
	}
	pop->addMenu( "New", newMenu );

	// Now construct "Edit..."
	if ( a || c || e || ge )
	{
		mxPopupMenu	*editMenu = new mxPopupMenu();
		{
			if ( a )
			{
				editMenu->add( va( "Actor '%s'...", a->GetActor()->GetName() ), IDC_EDITACTOR );
			}
			if ( c )
			{
				editMenu->add( va( "Channel '%s'...", c->GetChannel()->GetName() ), IDC_EDITCHANNEL );
			}
			if ( ge )
			{
				switch ( ge->GetEvent()->GetType() )
				{
				default:
					break;
				case CChoreoEvent::SECTION:
					{
						editMenu->add( va( "Section Pause '%s'...", ge->GetEvent()->GetName() ), IDC_EDITGLOBALEVENT );
					}
					break;
				case CChoreoEvent::LOOP:
					{
						editMenu->add( va( "Loop Point '%s'...", ge->GetEvent()->GetName() ), IDC_EDITGLOBALEVENT );
					}
					break;
				case CChoreoEvent::STOPPOINT:
					{
						editMenu->add( va( "Stop Point '%s'...", ge->GetEvent()->GetName() ), IDC_EDITGLOBALEVENT );
					}
					break;
				}
			}
		}

		pop->addMenu( "Edit", editMenu );

	}

	// Move up/down
	if ( a || c )
	{
		mxPopupMenu	*moveUpMenu = new mxPopupMenu();
		mxPopupMenu	*moveDownMenu = new mxPopupMenu();

		if ( a )
		{
			moveUpMenu->add( va( "Move '%s' up", a->GetActor()->GetName() ), IDC_MOVEACTORUP );
			moveDownMenu->add( va( "Move '%s' down", a->GetActor()->GetName() ), IDC_MOVEACTORDOWN );
		}
		if ( c )
		{
			moveUpMenu->add( va( "Move '%s' up", c->GetChannel()->GetName() ), IDC_MOVECHANNELUP );
			moveDownMenu->add( va( "Move '%s' down", c->GetChannel()->GetName() ), IDC_MOVECHANNELDOWN );
		}

		pop->addMenu( "Move Up", moveUpMenu );
		pop->addMenu( "Move Down", moveDownMenu );
	}

	// Delete
	if ( a || c || e || ge || (ct != -1) )
	{
		mxPopupMenu *deleteMenu = new mxPopupMenu();
		if ( a )
		{
			deleteMenu->add( va( "Actor '%s'", a->GetActor()->GetName() ), IDC_DELETEACTOR );
		}
		if ( c )
		{
			deleteMenu->add( va( "Channel '%s'", c->GetChannel()->GetName() ), IDC_DELETECHANNEL );
		}
		if ( e )
		{
			deleteMenu->add( va( "Event '%s'", e->GetEvent()->GetName() ), IDC_DELETEEVENT );
		}
		if ( ge )
		{
			switch ( ge->GetEvent()->GetType() )
			{
			default:
				break;
			case CChoreoEvent::SECTION:
				{
					deleteMenu->add( va( "Section Pause '%s'...", ge->GetEvent()->GetName() ), IDC_DELETEGLOBALEVENT );
				}
				break;
			case CChoreoEvent::LOOP:
				{
					deleteMenu->add( va( "Loop Point '%s'...", ge->GetEvent()->GetName() ), IDC_DELETEGLOBALEVENT );
				}
				break;
			case CChoreoEvent::STOPPOINT:
				{
					deleteMenu->add( va( "Stop Point '%s'...", ge->GetEvent()->GetName() ), IDC_DELETEGLOBALEVENT );
				}
				break;
			}
		}
		if ( e && ct != -1 )
		{
			CEventRelativeTag *tag = e->GetEvent()->GetRelativeTag( ct );
			if ( tag )
			{
				deleteMenu->add( va( "Relative Tag '%s'...", tag->GetName() ), IDC_DELETERELATIVETAG );
			}
		}
		pop->addMenu( "Delete", deleteMenu );
	}

	// Select
	{
		mxPopupMenu *selectMenu = new mxPopupMenu();
		selectMenu->add( "Select All", IDC_SELECTALL );
		selectMenu->add( "Deselect All", IDC_DESELECTALL );
		pop->addMenu( "Select/Deselect", selectMenu );
	}

	// Quick delete for events
	if ( e )
	{
		pop->addSeparator();

		switch ( e->GetEvent()->GetType() )
		{
		default:
			break;
		case CChoreoEvent::FLEXANIMATION:
			{
				pop->add( va( "Edit event '%s' in expression tool", e->GetEvent()->GetName() ), IDC_EXPRESSIONTOOL );
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				pop->add( va( "Edit event '%s' in gesture tool", e->GetEvent()->GetName() ), IDC_GESTURETOOL );
			}
			break;
		}

		pop->add( va( "Move event '%s' to back", e->GetEvent()->GetName() ), IDC_MOVETOBACK );
		if ( CountSelectedEvents() > 1 )
		{
			pop->add( va( "Delete events" ), IDC_DELETEEVENT );
		}
		else
		{
			pop->add( va( "Delete event '%s'", e->GetEvent()->GetName() ), IDC_DELETEEVENT );
		}
	}

	// Copy/paste
	if ( CanPaste() || e )
	{
		pop->addSeparator();

		if ( CountSelectedEvents() > 1 )
		{
			pop->add( va( "Copy events to clipboard" ), IDC_COPYEVENTS );
		}
		else if ( e )
		{
			pop->add( va( "Copy event '%s' to clipboard", e->GetEvent()->GetName() ), IDC_COPYEVENTS );
		}

		if ( CanPaste() )
		{
			pop->add( va( "Paste events" ), IDC_PASTEEVENTS );
		}
	}

	// Export / import
	pop->addSeparator();
	if ( e )
	{
		if ( CountSelectedEvents() > 1 )
		{
			pop->add( va( "Export events..." ), IDC_EXPORTEVENTS );
		}
		else if ( e )
		{
			pop->add( va( "Export event '%s'...", e->GetEvent()->GetName() ), IDC_EXPORTEVENTS );
		}
	}
	pop->add( va( "Import events..." ), IDC_IMPORTEVENTS );
	pop->addSeparator();
	pop->add( va( "Change scale..." ), IDC_CV_CHANGESCALE );
	pop->add( va( "Check sequences" ), IDC_CV_CHECKSEQLENGTHS );
	pop->add( va( "Process sequences" ), IDC_CV_PROCESSSEQUENCES );
	pop->add( va( m_bRampOnly ? "Ramp normal" : "Ramp only" ), IDC_CV_TOGGLERAMPONLY );
	pop->setChecked( IDC_CV_PROCESSSEQUENCES, m_bProcessSequences );

	// Undo/redo
	if ( m_nUndoLevel != 0 ||
		m_nUndoLevel != m_UndoStack.Size() )
	{

		pop->addSeparator();

		if ( m_nUndoLevel != 0 )
		{
			pop->add( va( "Undo %s", GetUndoDescription() ), IDC_CVUNDO );
		}
		if ( m_nUndoLevel != m_UndoStack.Size() )
		{
			pop->add( va( "Redo %s", GetRedoDescription() ), IDC_CVREDO );
		}
	}

	if ( m_pScene )
	{
		// Associate map file
		pop->addSeparator();
		pop->add( va( "Associate .bsp (%s)", m_pScene->GetMapname() ), IDC_ASSOCIATEBSP );
		if ( a )
		{
			if ( a->GetActor() && a->GetActor()->GetFacePoserModelName()[0] )
			{
				pop->add( va( "Change .mdl for %s", a->GetActor()->GetName() ), IDC_ASSOCIATEMODEL );
			}
			else
			{
				pop->add( va( "Associate .mdl with %s", a->GetActor()->GetName() ), IDC_ASSOCIATEMODEL );
			}
		}
	}

	pop->popup( this, mx, my );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::AssociateModel( void )
{
	if ( !m_pScene )
		return;

	CChoreoActorWidget *actor = m_pClickedActor;
	if ( !actor )
		return;

	CChoreoActor *a = actor->GetActor();
	if ( !a )
		return;

	CChoiceParams params;
	strcpy( params.m_szDialogTitle, "Associate Model" );

	params.m_bPositionDialog = false;
	params.m_nLeft = 0;
	params.m_nTop = 0;
	strcpy( params.m_szPrompt, "Choose model:" );

	params.m_Choices.RemoveAll();

	params.m_nSelected = -1;
	int oldsel = -1;

	int c = models->Count();
	ChoiceText text;
	for ( int i = 0; i < c; i++ )
	{
		char const *modelname = models->GetModelName( i );

		strcpy( text.choice, modelname );

		if ( !stricmp( a->GetName(), modelname ) )
		{
			params.m_nSelected = i;
			oldsel = -1;
		}

		params.m_Choices.AddToTail( text );
	}

	// Add an extra entry which is "No association"
	strcpy( text.choice, "No Associated Model" );
	params.m_Choices.AddToTail( text );

	if ( !ChoiceProperties( &params ) )
		return;
	
	if ( params.m_nSelected == oldsel )
		return;

	// Chose something new...
	if ( params.m_nSelected >= 0 &&
		 params.m_nSelected < params.m_Choices.Count() )
	{
		AssociateModelToActor( a, params.m_nSelected );
	}
	else
	{
		// Chose "No association"
		AssociateModelToActor( a, -1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			modelindex - 
//-----------------------------------------------------------------------------
void CChoreoView::AssociateModelToActor( CChoreoActor *actor, int modelindex )
{
	Assert( actor );

	SetDirty( true );

	PushUndo( "Associate model" );

	// Chose something new...
	if ( modelindex >= 0 &&
		 modelindex < models->Count() )
	{
		actor->SetFacePoserModelName( models->GetModelFileName( modelindex ) );
	}
	else
	{
		// Chose "No Associated Model"
		actor->SetFacePoserModelName( "" );
	}

	PushRedo( "Associate model" );
}

void CChoreoView::AssociateBSP( void )
{
	if ( !m_pScene )
		return;

	// Show file io
	const char *fullpath = mxGetOpenFileName( 
		0, 
		FacePoser_MakeWindowsSlashes( va( "%s/maps/", GetGameDirectory() ) ), 
		"*.bsp" );
	if ( !fullpath || !fullpath[ 0 ] )
		return;

	// Strip game directory and slash
	char mapname[ 512 ];
	filesystem->FullPathToRelativePath( fullpath, mapname );

	m_pScene->SetMapname( mapname );
	redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::DrawFocusRect( void )
{
	HDC dc = GetDC( NULL );

	for ( int i = 0; i < m_FocusRects.Size(); i++ )
	{
		RECT rc = m_FocusRects[ i ].m_rcFocus;

		::DrawFocusRect( dc, &rc );
	}

	ReleaseDC( NULL, dc );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : events - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetSelectedEvents( CUtlVector< CChoreoEvent * >& events )
{
	events.RemoveAll();

	int c = 0;

	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		for ( int j = 0; j < actor->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *channel = actor->GetChannel( j );
			if ( !channel )
				continue;

			for ( int k = 0; k < channel->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *event = channel->GetEvent( k );
				if ( !event )
					continue;

				if ( event->IsSelected() )
				{
					events.AddToTail( event->GetEvent() );
					c++;
				}
			}
		}
	}

	return c;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::CountSelectedEvents( void )
{
	int c = 0;

	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		for ( int j = 0; j < actor->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *channel = actor->GetChannel( j );
			if ( !channel )
				continue;

			for ( int k = 0; k < channel->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *event = channel->GetEvent( k );
				if ( !event )
					continue;

				if ( event->IsSelected() )
					c++;

			}
		}
	}

	return c;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::ComputeEventDragType( int mx, int my )
{
	// Iterate the events and see who's closest
	CChoreoEventWidget *ew = GetEventUnderCursorPos( mx, my );
	if ( !ew )
		return DRAGTYPE_NONE;

	int tagnum = GetTagUnderCursorPos( ew, mx, my );
	if ( tagnum != -1 && CountSelectedEvents() <= 1 )
	{
		return DRAGTYPE_EVENTTAG_MOVE;
	}

	CChoreoEvent *event = ew->GetEvent();
	if ( event )
	{
		if ( event->IsFixedLength() || !event->HasEndTime() )
		{
			return DRAGTYPE_EVENT_MOVE;
		}
	}

	if ( CountSelectedEvents() > 1 )
	{
		return DRAGTYPE_EVENT_MOVE;
	}

	int tolerance = DRAG_EVENT_EDGE_TOLERANCE;
	// Deal with small windows by lowering tolerance
	if ( ew->w() < 4 * tolerance )
	{
		tolerance = 2;
	}

	RECT bounds = ew->getBounds();
	mx -= bounds.left;
	my -= bounds.top;

	if ( mx <= tolerance )
	{
		return DRAGTYPE_EVENT_STARTTIME;
	}

	if ( event )
	{
		if ( event->HasEndTime() )
		{
			int rightside = ew->GetDurationRightEdge() ? ew->GetDurationRightEdge() : ew->w();

			if ( mx >= rightside - tolerance )
			{
				if ( mx > rightside + tolerance )
				{
					return DRAGTYPE_NONE;
				}
				else
				{
					return DRAGTYPE_EVENT_ENDTIME;
				}
			}
		}
	}

	return DRAGTYPE_EVENT_MOVE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::StartDraggingGlobalEvent( int mx, int my )
{
	m_nDragType = DRAGTYPE_GLOBALEVENT_MOVE;

	m_FocusRects.Purge();

	RECT rcFocus;
	rcFocus.left = m_pClickedGlobalEvent->x() + m_pClickedGlobalEvent->w() / 2;
	rcFocus.top = 0;
	rcFocus.right = rcFocus.left + 2;
	rcFocus.bottom = h2();

	POINT offset;
	offset.x = 0;
	offset.y = 0;
	ClientToScreen( (HWND)getHandle(), &offset );
	OffsetRect( &rcFocus, offset.x, offset.y );

	CFocusRect fr;
	fr.m_rcFocus = rcFocus;
	fr.m_rcOrig = rcFocus;

	m_FocusRects.AddToTail( fr );

	m_xStart = mx;
	m_yStart = my;
	m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEALL ) );

	DrawFocusRect();

	m_bDragging = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::StartDraggingEvent( int mx, int my )
{
	m_nDragType = ComputeEventDragType( mx, my );
	if ( m_nDragType == DRAGTYPE_NONE )
		return;

	m_FocusRects.Purge();

	// Go through all selected events
	RECT rcFocus;
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		for ( int j = 0; j < actor->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *channel = actor->GetChannel( j );
			if ( !channel )
				continue;

			for ( int k = 0; k < channel->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *event = channel->GetEvent( k );
				if ( !event )
					continue;

				if ( !event->IsSelected() )
					continue;

				if ( event == m_pClickedEvent && m_nClickedTag != -1 )
				{
					int leftEdge = 0;
					CEventRelativeTag *tag = event->GetEvent()->GetRelativeTag( m_nClickedTag );
					if ( tag )
					{
						// Determine left edcge
						RECT bounds;
						bounds = event->getBounds();
						if ( bounds.right - bounds.left > 0 )
						{
							leftEdge = (int)( tag->GetPercentage() * (float)( bounds.right - bounds.left ) + 0.5f );
						}
					}

					rcFocus.left = event->x() + leftEdge - 4;
					rcFocus.top = event->y() - 4;
					rcFocus.right = rcFocus.left + 8;
					rcFocus.bottom = rcFocus.top + 8;
				}
				else
				{
					rcFocus.left = event->x();
					rcFocus.top = event->y();
					if ( event->GetDurationRightEdge() )
					{
						rcFocus.right = event->x() + event->GetDurationRightEdge();
					}
					else
					{
						rcFocus.right = rcFocus.left + event->w();
					}
					rcFocus.bottom = rcFocus.top + event->h();
				}

				POINT offset;
				offset.x = 0;
				offset.y = 0;
				ClientToScreen( (HWND)getHandle(), &offset );
				OffsetRect( &rcFocus, offset.x, offset.y );

				CFocusRect fr;
				fr.m_rcFocus = rcFocus;
				fr.m_rcOrig = rcFocus;

				// Relative tag events don't move
				m_FocusRects.AddToTail( fr );
			}
		}
	}
	

	m_xStart = mx;
	m_yStart = my;
	m_hPrevCursor = NULL;
	switch ( m_nDragType )
	{
	default:
		break;
	case DRAGTYPE_EVENTTAG_MOVE:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );
		break;
	case DRAGTYPE_EVENT_MOVE:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEALL ) );
		break;
	case DRAGTYPE_EVENT_STARTTIME:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );
		break;
	case DRAGTYPE_EVENT_ENDTIME:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );
		break;
	}

	DrawFocusRect();

	m_bDragging = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::MouseStartDrag( mxEvent *event, int mx, int my )
{
	bool isrightbutton = event->buttons & mxEvent::MouseRightButton ? true : false;

	if ( m_bDragging )
	{
		return;
	}

	GetObjectsUnderMouse( mx, my, &m_pClickedActor, &m_pClickedChannel, &m_pClickedEvent, &m_pClickedGlobalEvent, &m_nClickedTag );

	if ( m_pClickedEvent )
	{
		CChoreoEvent *e = m_pClickedEvent->GetEvent();
		Assert( e );

		if ( !( event->modifiers & ( mxEvent::KeyCtrl | mxEvent::KeyShift ) ) )
		{
			if ( !m_pClickedEvent->IsSelected() )
			{
				DeselectAll();
			}
			TraverseWidgets( Select, m_pClickedEvent );
		}
		else
		{
			m_pClickedEvent->SetSelected( !m_pClickedEvent->IsSelected() );
		}

		switch ( m_pClickedEvent->GetEvent()->GetType() )
		{
		default:
			break;
		case CChoreoEvent::FLEXANIMATION:
			{
				g_pExpressionTool->SetEvent( e );
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				g_pGestureTool->SetEvent( e );
			}
			break;
		}

		if ( e->HasEndTime() )
		{
			g_pRampTool->SetEvent( e );
		}

		redraw();
		StartDraggingEvent( mx, my );
	}
	else if ( m_pClickedGlobalEvent )
	{
		StartDraggingGlobalEvent( mx, my );
	}
	else if ( IsMouseOverScrubArea( event ) )
	{
		if ( IsMouseOverScrubHandle( event ) )
		{
			m_nDragType = DRAGTYPE_SCRUBBER;

			m_bDragging = true;
			
			float t = GetTimeValueForMouse( (short)event->x );
			m_flScrubberTimeOffset = m_flScrub - t;
			float maxoffset = 0.5f * (float)SCRUBBER_HANDLE_WIDTH / GetPixelsPerSecond();
			m_flScrubberTimeOffset = clamp( m_flScrubberTimeOffset, -maxoffset, maxoffset );
			t += m_flScrubberTimeOffset;

			ClampTimeToSelectionInterval( t );

			SetScrubTime( t );
			SetScrubTargetTime( t );

			redraw();

			RECT rcScrub;
			GetScrubHandleRect( rcScrub, true );

			m_FocusRects.Purge();

			// Go through all selected events
			RECT rcFocus;

			rcFocus.top = GetStartRow();
			rcFocus.bottom = h2() - m_nScrollbarHeight - m_nInfoHeight;
			rcFocus.left = ( rcScrub.left + rcScrub.right ) / 2;
			rcFocus.right = rcFocus.left;

			POINT pt;
			pt.x = pt.y = 0;
			ClientToScreen( (HWND)getHandle(), &pt );

			OffsetRect( &rcFocus, pt.x, pt.y );

			CFocusRect fr;
			fr.m_rcFocus = rcFocus;
			fr.m_rcOrig = rcFocus;

			m_FocusRects.AddToTail( fr );

			m_xStart = mx;
			m_yStart = my;

			m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );

			DrawFocusRect();
		}
		else
		{
			float t = GetTimeValueForMouse( (short)event->x );

			ClampTimeToSelectionInterval( t );

			SetScrubTargetTime( t );

			// Unpause the scene
			m_bPaused = false;
			redraw();
		}
	}
	else
	{
		if ( !( event->modifiers & ( mxEvent::KeyCtrl | mxEvent::KeyShift ) ) )
		{
			DeselectAll();

			if ( !isrightbutton )
			{
				if ( realtime - m_flLastMouseClickTime < 0.3f )
				{
					OnDoubleClicked();
					m_flLastMouseClickTime = -1.0f;
				}
				else
				{
					m_flLastMouseClickTime = realtime;
				}
			}

			redraw();
		}
	}


}

void CChoreoView::OnDoubleClicked()
{
	if ( m_pClickedChannel )
	{
		SetDirty( true );
		PushUndo( "Enable/disable Channel" );

		m_pClickedChannel->GetChannel()->SetActive( !m_pClickedChannel->GetChannel()->GetActive() );

		PushRedo( "Enable/disable Channel" );
		return;
	}

	if ( m_pClickedActor )
	{
		SetDirty( true );
		PushUndo( "Enable/disable Actor" );

		m_pClickedActor->GetActor()->SetActive( !m_pClickedActor->GetActor()->GetActive() );

		PushRedo( "Enable/disable Actor" );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::MouseContinueDrag( mxEvent *event, int mx, int my )
{
	if ( !m_bDragging )
		return;

	DrawFocusRect();

	for ( int i = 0; i < m_FocusRects.Size(); i++ )
	{
		CFocusRect *f = &m_FocusRects[ i ];
		f->m_rcFocus = f->m_rcOrig;

		switch ( m_nDragType )
		{
		default:
		case DRAGTYPE_SCRUBBER:
			{
				float t = GetTimeValueForMouse( mx );
				t += m_flScrubberTimeOffset;

				ClampTimeToSelectionInterval( t );

				float dt = t - m_flScrub;

				SetScrubTargetTime( t );

				m_bSimulating = true;
				ScrubThink( dt, true, this );

				SetScrubTime( t );

				OffsetRect( &f->m_rcFocus, ( mx - m_xStart ), 0 );
			}
			break;
		case DRAGTYPE_EVENT_MOVE:
		case DRAGTYPE_EVENTTAG_MOVE:
			{
				bool shiftdown = ( event->modifiers & mxEvent::KeyShift ) ? true : false;
				int dy = my - m_yStart;

				// Only allow jumping channels if shift is down
				if ( !shiftdown )
				{
					dy = 0;
				}
				if ( abs( dy ) < m_pClickedEvent->GetItemHeight() )
				{
					dy = 0;
				}
				if ( m_nSelectedEvents > 1 )
				{
					dy = 0;
				}
				if ( m_nDragType == DRAGTYPE_EVENTTAG_MOVE )
				{
					dy = 0;
				}
				int dx = mx - m_xStart;
				if ( m_pClickedEvent->GetEvent()->IsUsingRelativeTag() )
				{
					dx = 0;
				}
				OffsetRect( &f->m_rcFocus, dx, dy );
			}
			break;
		case DRAGTYPE_GLOBALEVENT_MOVE:
			{
				OffsetRect( &f->m_rcFocus, ( mx - m_xStart ), 0 );
			}
			break;
		case DRAGTYPE_EVENT_STARTTIME:
			f->m_rcFocus.left += ( mx - m_xStart );
			break;
		case DRAGTYPE_EVENT_ENDTIME:
			f->m_rcFocus.right += ( mx - m_xStart );
			break;
		}
	}

	DrawFocusRect();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::MouseMove( int mx, int my )
{
	if ( m_bDragging )
		return;

	int dragtype = ComputeEventDragType( mx, my );
	if ( dragtype == DRAGTYPE_NONE )
	{
		CChoreoGlobalEventWidget *ge = NULL;
		GetObjectsUnderMouse( mx, my, NULL, NULL, NULL, &ge, NULL );
		if ( ge )
		{
			dragtype = DRAGTYPE_GLOBALEVENT_MOVE;
		}
	}

	if ( m_hPrevCursor )
	{
		SetCursor( m_hPrevCursor );
		m_hPrevCursor = NULL;
	}
	switch ( dragtype )
	{
	default:
		break;
	case DRAGTYPE_EVENTTAG_MOVE:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );
		break;
	case DRAGTYPE_EVENT_MOVE:
	case DRAGTYPE_GLOBALEVENT_MOVE:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEALL ) );
		break;
	case DRAGTYPE_EVENT_STARTTIME:
	case DRAGTYPE_EVENT_ENDTIME:
		m_hPrevCursor = SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::FinishDraggingGlobalEvent( int mx, int my )
{
	DrawFocusRect();

	m_FocusRects.Purge();

	m_bDragging = false;

	if ( !m_pClickedGlobalEvent )
		return;

	CChoreoEvent *e = m_pClickedGlobalEvent->GetEvent();
	if ( !e )
		return;

	// Figure out true dt
	float dt = GetTimeDeltaForMouseDelta( mx, m_xStart );
	if ( !dt )
		return;

	SetDirty( true );

	char undotext[ 256 ];
	undotext[0]=0;
	switch( e->GetType() )
	{
	default:
		Assert( 0 );
		break;
	case CChoreoEvent::SECTION:
		{
			Q_strcpy( undotext, "Move section pause" );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			Q_strcpy( undotext, "Move loop event" );
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			Q_strcpy( undotext, "Move stop event" );
		}
		break;
	}

	PushUndo( undotext );

	e->OffsetTime( dt );
	e->SnapTimes();

	PushRedo( undotext );

	InvalidateLayout();

	m_nDragType = DRAGTYPE_NONE;

	if ( m_hPrevCursor )
	{
		SetCursor( m_hPrevCursor );
		m_hPrevCursor = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *e - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::CheckGestureLength( CChoreoEvent *e, bool checkonly )
{
	Assert( e );
	if ( !e )
		return false;

	if ( e->GetType() != CChoreoEvent::GESTURE )
	{
		Con_Printf( "CheckGestureLength:  called on non-GESTURE event %s\n", e->GetName() );
		return false;
	}

	StudioModel *model = FindAssociatedModel( e->GetScene(), e->GetActor()  );
	if ( !model )
		return false;

	if ( !model->getStudioHeader() )
		return false;

	int iSequence = model->LookupSequence( e->GetParameters() );
	if ( iSequence < 0 )
		return false;

	bool bret = false;

	bool looping = model->GetSequenceLoops( iSequence );
	if ( looping )
	{
		Con_Printf( "gesture %s of model %s is marked as STUDIO_LOOPING!\n",
			e->GetParameters(), model->getStudioHeader()->name );
	}

	float seqduration = model->GetDuration( iSequence );
	float curduration;
	e->GetGestureSequenceDuration( curduration );
	if ( seqduration != 0.0f &&
		seqduration != curduration )
	{
		bret = true;
		if ( !checkonly )
		{
			e->SetGestureSequenceDuration( seqduration );
		}
	}

	return bret;
}




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *e - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::DefaultGestureLength( CChoreoEvent *e, bool checkonly )
{
	Assert( e );
	if ( !e )
		return false;

	if ( e->GetType() != CChoreoEvent::GESTURE )
	{
		Con_Printf( "DefaultGestureLength:  called on non-GESTURE event %s\n", e->GetName() );
		return false;
	}

	StudioModel *model = FindAssociatedModel( e->GetScene(), e->GetActor()  );
	if ( !model )
		return false;

	if ( !model->getStudioHeader() )
		return false;

	int iSequence = model->LookupSequence( e->GetParameters() );
	if ( iSequence < 0 )
		return false;

	bool bret = false;

	float seqduration = model->GetDuration( iSequence );
	if ( seqduration != 0.0f )
	{
		bret = true;
		if ( !checkonly )
		{
			e->SetEndTime( e->GetStartTime() + seqduration );
		}
	}

	return bret;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *e - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::AutoaddGestureEvents( CChoreoEvent *e, bool checkonly )
{
	Assert( e );
	if ( !e )
		return false;

	if ( e->GetType() != CChoreoEvent::GESTURE )
	{
		Con_Printf( "CheckGestureLength:  called on non-GESTURE event %s\n", e->GetName() );
		return false;
	}

	StudioModel *model = FindAssociatedModel( e->GetScene(), e->GetActor() );
	if ( !model )
		return false;

	if ( !model->getStudioHeader() )
		return false;

	int iSequence = model->LookupSequence( e->GetParameters() );
	if ( iSequence < 0 )
		return false;

	bool bret = false;

	mstudioseqdesc_t *pSeqDesc = model->GetSeqDesc( iSequence );

	for (int i = 0; i < pSeqDesc->numevents; i++)
	{
		mstudioevent_t *pSeqEvent = pSeqDesc->pEvent( i );
		if (pSeqEvent->event == EVENT_FACEPOSER_TAG)
		{
			if ( !checkonly )
			{
				e->AddAbsoluteTag( CChoreoEvent::SHIFTED, pSeqEvent->options, pSeqEvent->cycle );
				e->AddAbsoluteTag( CChoreoEvent::PLAYBACK, pSeqEvent->options, pSeqEvent->cycle );
			}
			bret = true;
		}
	}

	return bret;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *e - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------

bool CChoreoView::AutoaddSequenceKeys( CChoreoEvent *e )
{
	StudioModel *model = FindAssociatedModel( e->GetScene(), e->GetActor() );
	if ( !model )
		return false;

	if ( !model->getStudioHeader() )
		return false;

	int iSequence = model->LookupSequence( e->GetParameters() );
	if ( iSequence < 0 )
		return false;

	bool bret = false;

	KeyValues *seqKeyValues = new KeyValues("");
	if ( seqKeyValues->LoadFromBuffer( model->GetFileName( ), model->GetKeyValueText( iSequence ) ) )
	{
		// Do we have a build point section?
		KeyValues *pkvAllFaceposer = seqKeyValues->FindKey("faceposer");
		if ( pkvAllFaceposer )
		{
			// Start grabbing the sounds and slotting them in
			KeyValues *pkvFaceposer;
			for ( pkvFaceposer = pkvAllFaceposer->GetFirstSubKey(); pkvFaceposer; pkvFaceposer = pkvFaceposer->GetNextKey() )
			{
				if (!stricmp( pkvFaceposer->GetName(), "event_ramp" ))
				{
					KeyValues *pkvTags;
					for ( pkvTags = pkvFaceposer->GetFirstSubKey(); pkvTags; pkvTags = pkvTags->GetNextKey() )
					{
						float time  = atof( pkvTags->GetName() );
						float weight  = pkvTags->GetFloat();
						// e->AddRamp( time, weight );
					}
				}
				else if (!stricmp( pkvFaceposer->GetName(), "length" ))
				{
					float seqduration  = pkvFaceposer->GetFloat();
					// e->SetEndTime( e->GetStartTime() + seqduration );
				} else if (!stricmp( pkvFaceposer->GetName(), "tags" ))
				{
					KeyValues *pkvTags;
					for ( pkvTags = pkvFaceposer->GetFirstSubKey(); pkvTags; pkvTags = pkvTags->GetNextKey() )
					{
						float percentage  = pkvTags->GetInt() / model->GetMaxFrame();
						e->AddAbsoluteTag( CChoreoEvent::SHIFTED, pkvTags->GetName(), percentage );
						e->AddAbsoluteTag( CChoreoEvent::PLAYBACK, pkvTags->GetName(), percentage );
					}
				}
			}
		}

		seqKeyValues->deleteThis();
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *e - 
//-----------------------------------------------------------------------------
bool CChoreoView::CheckSequenceLength( CChoreoEvent *e, bool checkonly )
{
	Assert( e );
	if ( !e )
		return false;

	if ( e->GetType() != CChoreoEvent::SEQUENCE )
	{
		Con_Printf( "CheckSequenceLength:  called on non-SEQUENCE event %s\n", e->GetName() );
		return false;
	}

	StudioModel *model = FindAssociatedModel( e->GetScene(), e->GetActor() );
	if ( !model )
		return false;

	if ( !model->getStudioHeader() )
		return false;

	int iSequence = model->LookupSequence( e->GetParameters() );
	if ( iSequence < 0 )
		return false;

	bool bret = false;

	bool looping = model->GetSequenceLoops( iSequence );

	float seqduration = model->GetDuration( iSequence );

	if ( looping )
	{
		if ( e->IsFixedLength() )
		{
			if ( checkonly )
				return true;

			Con_Printf( "CheckSequenceLength:  %s is looping, removing fixed length flag\n",
				e->GetName() );
			bret = true;
		}
		e->SetFixedLength( false );

		if ( !e->HasEndTime() )
		{
			if ( checkonly )
				return true;

			Con_Printf( "CheckSequenceLength:  %s is looping, setting default end time\n",
				e->GetName() );

			e->SetEndTime( e->GetStartTime() + seqduration );

			bret = true;
		}
	}
	else
	{
		if ( !e->IsFixedLength() )
		{
			if ( checkonly )
				return true;

			Con_Printf( "CheckSequenceLength:  %s is fixed length, removing looping flag\n",
				e->GetName() );
			bret = true;
		}
		e->SetFixedLength( true );

		if ( e->HasEndTime() )
		{
			float dt = e->GetDuration();
			if ( fabs( dt - seqduration ) > 0.01f )
			{
				if ( checkonly )
					return true;

				Con_Printf( "CheckSequenceLength:  %s has wrong duration, changing length from %f to %f seconds\n",
					e->GetName(), dt, seqduration );
				bret = true;
			}
		}
		else
		{
			if ( checkonly )
				return true;

			Con_Printf( "CheckSequenceLength:  %s has wrong duration, changing length to %f seconds\n",
				e->GetName(), seqduration );
			bret = true;
		}

		if ( !checkonly )
		{
			e->SetEndTime( e->GetStartTime() + seqduration );
		}
	}

	return bret;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::FinishDraggingEvent( mxEvent *event, int mx, int my )
{
	DrawFocusRect();

	m_FocusRects.Purge();

	m_bDragging = false;

	float dt = GetTimeDeltaForMouseDelta( mx, m_xStart );
	if ( !dt )
	{
		if ( m_pScene && m_pClickedEvent && m_pClickedEvent->GetEvent()->GetType() == CChoreoEvent::SPEAK )
		{
			// Show phone wav in wav viewer
			SetCurrentWaveFile( va( "sound/%s", FacePoser_TranslateSoundName( m_pClickedEvent->GetEvent()->GetParameters() ) ), m_pClickedEvent->GetEvent() );
		}
		return;
	}

	SetDirty( true );

	char *desc = "";

	switch ( m_nDragType )
	{
		default:
		case DRAGTYPE_EVENT_MOVE:
			desc = "Event Move";
			break;
		case DRAGTYPE_EVENT_STARTTIME:
			desc = "Change Start Time";
			break;
		case DRAGTYPE_EVENT_ENDTIME:
			desc = "Change End Time";
			break;
		case DRAGTYPE_EVENTTAG_MOVE:
			desc = "Move Event Tag";
			break;
	}
	PushUndo( desc );

	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		for ( int j = 0; j < actor->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *channel = actor->GetChannel( j );
			if ( !channel )
				continue;

			for ( int k = 0; k < channel->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *event = channel->GetEvent( k );
				if ( !event )
					continue;

				if ( !event->IsSelected() )
					continue;

				// Figure out true dt
				CChoreoEvent *e = event->GetEvent();
				if ( e )
				{
					switch ( m_nDragType )
					{
					default:
					case DRAGTYPE_EVENT_MOVE:
						e->OffsetTime( dt );
						e->SnapTimes();
						break;
					case DRAGTYPE_EVENT_STARTTIME:
						{
							float newduration = e->GetDuration() - dt;
							RescaleRamp( e, newduration );
							switch ( e->GetType() )
							{
							default:
								break;
							case CChoreoEvent::GESTURE:
								{
									RescaleGestureTimes( e, e->GetStartTime() + dt, e->GetEndTime() );
								}
								break;
							case CChoreoEvent::FLEXANIMATION:
								{
									RescaleExpressionTimes( e, e->GetStartTime() + dt, e->GetEndTime() );
								}
								break;
							}
							e->OffsetStartTime( dt );
							e->SnapTimes();
							e->ResortRamp();
						}
						break;
					case DRAGTYPE_EVENT_ENDTIME:
						{
							float newduration = e->GetDuration() + dt;
							RescaleRamp( e, newduration );
							switch ( e->GetType() )
							{
							default:
								break;
							case CChoreoEvent::GESTURE:
								{
									RescaleGestureTimes( e, e->GetStartTime(), e->GetEndTime() + dt );
								}
								break;
							case CChoreoEvent::FLEXANIMATION:
								{
									RescaleExpressionTimes( e, e->GetStartTime(), e->GetEndTime() + dt );
								}
								break;
							}
							e->OffsetEndTime( dt );
							e->SnapTimes();
							e->ResortRamp();
						}
						break;
					case DRAGTYPE_EVENTTAG_MOVE:
						{
							// Get current x position
							if ( m_nClickedTag != -1 )
							{
								CEventRelativeTag *tag = e->GetRelativeTag( m_nClickedTag );
								if ( tag )
								{
									float dx = mx - m_xStart;
									// Determine left edcge
									RECT bounds;
									bounds = event->getBounds();
									if ( bounds.right - bounds.left > 0 )
									{
										int left = bounds.left + (int)( tag->GetPercentage() * (float)( bounds.right - bounds.left ) + 0.5f );

										left += dx;

										if ( left < bounds.left )
										{
											left = bounds.left;
										}
										else if ( left >= bounds.right )
										{
											left = bounds.right - 1;
										}

										// Now convert back to a percentage
										float frac = (float)( left - bounds.left ) / (float)( bounds.right - bounds.left );

										tag->SetPercentage( frac );
									}
								}
							}
						}
						break;
					}
				}

				switch ( e->GetType() )
				{
				default:
					break;
				case CChoreoEvent::SPEAK:
					{
						// Try and load wav to get length
						CAudioSource *wave = sound->LoadSound( va( "sound/%s", FacePoser_TranslateSoundName( e->GetParameters() ) ) );
						if ( wave )
						{
							e->SetEndTime( e->GetStartTime() + wave->GetRunningLength() );
							delete wave;
						}
					}
					break;
				case CChoreoEvent::SEQUENCE:
					{
						CheckSequenceLength( e, false );
					}
					break;
				case CChoreoEvent::GESTURE:
					{
						CheckGestureLength( e, false );
					}
					break;
				}
			}
		}
	}

	m_nDragType = DRAGTYPE_NONE;

	if ( m_hPrevCursor )
	{
		SetCursor( m_hPrevCursor );
		m_hPrevCursor = 0;
	}

	CChoreoEvent *e = m_pClickedEvent->GetEvent();
	// See if event is moving to a new owner
	CChoreoChannelWidget *chOrig, *chNew;

	int dy = my - m_yStart;
	bool shiftdown = ( event->modifiers & mxEvent::KeyShift ) ? true : false;
	if ( !shiftdown )
	{
		dy = 0;
	}

	if ( abs( dy ) < m_pClickedEvent->GetItemHeight() )
	{
		my = m_yStart;
	}

	chNew = GetChannelUnderCursorPos( mx, my );
	
	InvalidateLayout();

	mx = m_xStart;
	my = m_yStart;

	chOrig = m_pClickedChannel;

	if ( chOrig && chNew && chOrig != chNew )
	{
		// Swap underlying objects
		CChoreoChannel *pOrigChannel, *pNewChannel;

		pOrigChannel = chOrig->GetChannel();
		pNewChannel = chNew->GetChannel();

		Assert( pOrigChannel && pNewChannel );

		// Remove event and move to new object
		DeleteSceneWidgets();

		pOrigChannel->RemoveEvent( e );
		pNewChannel->AddEvent( e );

		e->SetChannel( pNewChannel );

		CreateSceneWidgets();
	}
	else
	{
		if ( e && e->GetType() == CChoreoEvent::SPEAK )
		{
			// Show phone wav in wav viewer
			SetCurrentWaveFile( va( "sound/%s", FacePoser_TranslateSoundName( e->GetParameters() ) ), e );
		}
	}

	PushRedo( desc );
	InvalidateLayout();

	if ( e )
	{
		switch ( e->GetType() )
		{
		default:
			break;
		case CChoreoEvent::FLEXANIMATION:
			{
				g_pExpressionTool->SetEvent( e );
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				g_pGestureTool->SetEvent( e );
			}
			break;
		}

		if ( e->HasEndTime() )
		{
			g_pRampTool->SetEvent( e );
		}
	}
	g_pExpressionTool->LayoutItems( true );
	g_pExpressionTool->redraw();
	g_pGestureTool->redraw();
	g_pRampTool->redraw();
	g_pSceneRampTool->redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::MouseFinishDrag( mxEvent *event, int mx, int my )
{
	if ( !m_bDragging )
		return;

	switch ( m_nDragType )
	{
	case DRAGTYPE_SCRUBBER:
		{
			DrawFocusRect();
	
			m_FocusRects.Purge();

			float t = GetTimeValueForMouse( mx );
			t += m_flScrubberTimeOffset;
			m_flScrubberTimeOffset = 0.0f;

			ClampTimeToSelectionInterval( t );

			SetScrubTime( t );
			SetScrubTargetTime( t );

			m_bDragging = false;
			m_nDragType = DRAGTYPE_NONE;

			redraw();
		}
		break;
	case DRAGTYPE_EVENT_MOVE:
	case DRAGTYPE_EVENT_STARTTIME:
	case DRAGTYPE_EVENT_ENDTIME:
	case DRAGTYPE_EVENTTAG_MOVE:
		FinishDraggingEvent( event, mx, my );
		break;
	case DRAGTYPE_GLOBALEVENT_MOVE:
		FinishDraggingGlobalEvent( mx, my );
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::handleEvent( mxEvent *event )
{
	int iret = 0;

	if ( HandleToolEvent( event ) )
	{
		return iret;
	}

	switch ( event->event )
	{
	case mxEvent::Size:
		{
			m_nLastHPixelsNeeded = 0;
			m_nLastVPixelsNeeded = 0;
			InvalidateLayout();
			PositionControls();
			iret = 1;
		}
		break;
	case mxEvent::MouseDown:
		{
			if ( !m_bDragging )
			{
				if ( event->buttons & mxEvent::MouseRightButton )
				{
					if ( IsMouseOverTimeline( (short)event->x, (short)event->y ) )
					{
						PlaceABPoint( (short)event->x );
						redraw();
					}
					else if ( IsMouseOverScrubArea( event ) )
					{
						float t = GetTimeValueForMouse( (short)event->x );
						
						ClampTimeToSelectionInterval( t );

						SetScrubTime( t );
						SetScrubTargetTime( t );

						sound->Flush();

						// Unpause the scene
						m_bPaused = false;

						redraw();
					}
					else
					{
						// Show right click menu
						ShowContextMenu( (short)event->x, (short)event->y );
					}
				}
				else
				{
					if ( IsMouseOverTimeline( (short)event->x, (short)event->y ) )
					{
						ClearABPoints();
						redraw();
					}
					else
					{
						// Handle mouse dragging here
						MouseStartDrag( event, (short)event->x, (short)event->y );
					}
				}
			}
			iret = 1;
		}
		break;
	case mxEvent::MouseDrag:
		{
			MouseContinueDrag( event, (short)event->x, (short)event->y );
			iret = 1;
		}
		break;
	case mxEvent::MouseUp:
		{
			MouseFinishDrag( event, (short)event->x, (short)event->y );
			iret = 1;
		}
		break;
	case mxEvent::MouseMove:
		{
			MouseMove( (short)event->x, (short)event->y );
			UpdateStatusArea( (short)event->x, (short)event->y );
			iret = 1;
		}
		break;
	case mxEvent::KeyDown:
		{
			switch ( event->key )
			{
			case VK_ESCAPE:
				DeselectAll();
				iret = 1;
				break;
			case 'C':
				CopyEvents();
				iret = 1;
				break;
			case 'V':
				PasteEvents();
				redraw();
				iret = 1;
				break;
			default:
				break;
			}
		}
		break;
	case mxEvent::Action:
		{
			iret = 1;
			switch ( event->action )
			{
			default:
				iret = 0;
				break;
			case IDC_CV_TOGGLERAMPONLY:
				{
					m_bRampOnly = !m_bRampOnly;
					redraw();
				}
				break;
			case IDC_CV_PROCESSSEQUENCES:
				{
					m_bProcessSequences = !m_bProcessSequences;
				}
				break;
			case IDC_CV_CHECKSEQLENGTHS:
				{
					OnCheckSequenceLengths();
				}
				break;
			case IDC_CV_CHANGESCALE:
				{
					OnChangeScale();
				}
				break;
			case IDC_CHOREO_PLAYBACKRATE:
				{
					m_flPlaybackRate = m_pPlaybackRate->getValue();
					redraw();
				}
				break;
			case IDC_COPYEVENTS:
				CopyEvents();
				break;
			case IDC_PASTEEVENTS:
				PasteEvents();
				redraw();
				break;
			case IDC_IMPORTEVENTS:
				ImportEvents();
				redraw();
				break;
			case IDC_EXPORTEVENTS:
				ExportEvents();
				redraw();
				break;
			case IDC_EXPRESSIONTOOL:
				OnExpressionTool();
				break;
			case IDC_GESTURETOOL:
				OnGestureTool();
				break;
			case IDC_ASSOCIATEBSP:
				AssociateBSP();
				break;
			case IDC_ASSOCIATEMODEL:
				AssociateModel();
				break;
			case IDC_CVUNDO:
				Undo();
				break;
			case IDC_CVREDO:
				Redo();
				break;
			case IDC_SELECTALL:
				SelectAll();
				break;
			case IDC_DESELECTALL:
				DeselectAll();
				break;
			case IDC_PLAYSCENE:	
				Con_Printf( "Commencing playback\n" );
				PlayScene( true );
				break;
			case IDC_PAUSESCENE:
				Con_Printf( "Pausing playback\n" );
				PauseScene();
				break;
			case IDC_STOPSCENE:
				Con_Printf( "Canceling playback\n" );
				SetScrubTargetTime( m_flScrub );
				FinishSimulation();
				sound->Flush();
				break;
			case IDC_CHOREOVSCROLL:
				{
					int offset = 0;
					bool processed = true;

					switch ( event->modifiers )
					{
					case SB_THUMBTRACK:
						offset = event->height;
						break;
					case SB_PAGEUP:
						offset = m_pVertScrollBar->getValue();
						offset -= 20;
						offset = max( offset, m_pVertScrollBar->getMinValue() );
						break;
					case SB_PAGEDOWN:
						offset = m_pVertScrollBar->getValue();
						offset += 20;
						offset = min( offset, m_pVertScrollBar->getMaxValue() );
						break;
					case SB_LINEDOWN:
						offset = m_pVertScrollBar->getValue();
						offset += 10;
						offset = min( offset, m_pVertScrollBar->getMaxValue() );
						break;
					case SB_LINEUP:
						offset = m_pVertScrollBar->getValue();
						offset -= 10;
						offset = max( offset, m_pVertScrollBar->getMinValue() );
						break;
					default:
						processed = false;
						break;
					}
		
					if ( processed )
					{
						m_pVertScrollBar->setValue( offset );
						InvalidateRect( (HWND)m_pVertScrollBar->getHandle(), NULL, TRUE );
						m_nTopOffset = offset;
						InvalidateLayout();
					}
				}
				break;
			case IDC_CHOREOHSCROLL:
				{
					int offset = 0;
					bool processed = true;

					switch ( event->modifiers )
					{
					case SB_THUMBTRACK:
						offset = event->height;
						break;
					case SB_PAGEUP:
						offset = m_pHorzScrollBar->getValue();
						offset -= 20;
						offset = max( offset, m_pHorzScrollBar->getMinValue() );
						break;
					case SB_PAGEDOWN:
						offset = m_pHorzScrollBar->getValue();
						offset += 20;
						offset = min( offset, m_pHorzScrollBar->getMaxValue() );
						break;
					case SB_LINEUP:
						offset = m_pHorzScrollBar->getValue();
						offset -= 10;
						offset = max( offset, m_pHorzScrollBar->getMinValue() );
						break;
					case SB_LINEDOWN:
						offset = m_pHorzScrollBar->getValue();
						offset += 10;
						offset = min( offset, m_pHorzScrollBar->getMaxValue() );
						break;
					default:
						processed = false;
						break;
					}

					if ( processed )
					{
						MoveTimeSliderToPos( offset );
					}
				}
				break;
			case IDC_ADDACTOR:
				{
					NewActor();
				}
				break;
			case IDC_EDITACTOR:
				{
					CChoreoActorWidget *actor = m_pClickedActor;
					if ( actor )
					{
						EditActor( actor->GetActor() );
					}			
				}
				break;
			case IDC_DELETEACTOR:
				{
					CChoreoActorWidget *actor = m_pClickedActor;
					if ( actor )
					{
						DeleteActor( actor->GetActor() );
					}
				}
				break;
			case IDC_MOVEACTORUP:
				{
					CChoreoActorWidget *actor = m_pClickedActor;
					if ( actor )				
					{
						MoveActorUp( actor->GetActor()  );
					}
				}
				break;
			case IDC_MOVEACTORDOWN:
				{
					CChoreoActorWidget *actor = m_pClickedActor;
					if ( actor )				
					{
						MoveActorDown( actor->GetActor() );
					}
				}
				break;
			case IDC_CHANNELOPEN:
				{
					CActorBitmapButton *btn = static_cast< CActorBitmapButton * >( event->widget );
					if ( btn )
					{
						CChoreoActorWidget *a = btn->GetActor();
						if ( a )
						{
							a->ShowChannels( true );
						}
					}
				}
				break;
			case IDC_CHANNELCLOSE:
				{
					CActorBitmapButton *btn = static_cast< CActorBitmapButton * >( event->widget );
					if ( btn )
					{
						CChoreoActorWidget *a = btn->GetActor();
						if ( a )
						{
							a->ShowChannels( false );
						}
					}
				}
				break;
			case IDC_ADDEVENT_INTERRUPT:
				{
					AddEvent( CChoreoEvent::INTERRUPT );
				}
				break;
			case IDC_ADDEVENT_EXPRESSION:
				{
					AddEvent( CChoreoEvent::EXPRESSION );
				}
				break;
			case IDC_ADDEVENT_FLEXANIMATION:
				{
					AddEvent( CChoreoEvent::FLEXANIMATION );
				}
				break;
			case IDC_ADDEVENT_GESTURE:
				{
					AddEvent( CChoreoEvent::GESTURE );
				}
				break;
			case IDC_ADDEVENT_LOOKAT:
				{
					AddEvent( CChoreoEvent::LOOKAT );
				}
				break;
			case IDC_ADDEVENT_MOVETO:
				{
					AddEvent( CChoreoEvent::MOVETO );
				}
				break;
			case IDC_ADDEVENT_FACE:
				{
					AddEvent( CChoreoEvent::FACE );
				}
				break;
			case IDC_ADDEVENT_SPEAK:
				{
					AddEvent( CChoreoEvent::SPEAK );
				}
				break;
			case IDC_ADDEVENT_FIRETRIGGER:
				{
					AddEvent( CChoreoEvent::FIRETRIGGER );
				}
				break;
			case IDC_ADDEVENT_SUBSCENE:
				{
					AddEvent( CChoreoEvent::SUBSCENE );
				}
				break;
			case IDC_ADDEVENT_SEQUENCE:
				{
					AddEvent( CChoreoEvent::SEQUENCE );
				}
				break;
			case IDC_EDITEVENT:
				{
					CChoreoEventWidget *event = m_pClickedEvent;
					if ( event )
					{
						EditEvent( event->GetEvent() );
						redraw();
					}
				}
				break;
			case IDC_DELETEEVENT:
				{
					DeleteSelectedEvents();
				}
				break;
			case IDC_MOVETOBACK:
				{
					CChoreoEventWidget *event = m_pClickedEvent;
					if ( event )
					{
						MoveEventToBack( event->GetEvent() );
					}
				}
				break;
			case IDC_DELETERELATIVETAG:
				{
					CChoreoEventWidget *event = m_pClickedEvent;
					if ( event && m_nClickedTag >= 0 )
					{
						DeleteEventRelativeTag( event->GetEvent(), m_nClickedTag );
					}
				}
				break;
			case IDC_ADDTIMINGTAG:
				{
					AddEventRelativeTag();
				}
				break;
			case IDC_ADDEVENT_PAUSE:
				{
					AddGlobalEvent( CChoreoEvent::SECTION );
				}
				break;
			case IDC_ADDEVENT_LOOP:
				{
					AddGlobalEvent( CChoreoEvent::LOOP );
				}
				break;
			case IDC_ADDEVENT_STOPPOINT:
				{
					AddGlobalEvent( CChoreoEvent::STOPPOINT );
				}
				break;
			case IDC_EDITGLOBALEVENT:
				{
					CChoreoGlobalEventWidget *event = m_pClickedGlobalEvent;
					if ( event )
					{
						EditGlobalEvent( event->GetEvent() );
						redraw();
					}
				}
				break;
			case IDC_DELETEGLOBALEVENT:
				{
					CChoreoGlobalEventWidget *event = m_pClickedGlobalEvent;
					if ( event )
					{
						DeleteGlobalEvent( event->GetEvent() );
					}
				}
				break;
			case IDC_ADDCHANNEL:
				{
					NewChannel();
				}
				break;
			case IDC_EDITCHANNEL:
				{
					CChoreoChannelWidget *channel = m_pClickedChannel;
					if ( channel )
					{
						EditChannel( channel->GetChannel() );
					}
				}
				break;
			case IDC_DELETECHANNEL:
				{
					CChoreoChannelWidget *channel = m_pClickedChannel;
					if ( channel )
					{
						DeleteChannel( channel->GetChannel() );
					}
				}
				break;
			case IDC_MOVECHANNELUP:
				{
					CChoreoChannelWidget *channel = m_pClickedChannel;
					if ( channel )
					{
						MoveChannelUp( channel->GetChannel() );
					}
				}
				break;
			case IDC_MOVECHANNELDOWN:
				{
					CChoreoChannelWidget *channel = m_pClickedChannel;
					if ( channel )
					{
						MoveChannelDown( channel->GetChannel() );
					}
				}
				break;
			}

			if ( iret == 1 )
			{
				SetActiveTool( this );
			}
		}
		break;
	}
	return iret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::PlayScene( bool forward )
{
	m_bForward = forward;
	if ( !m_pScene )
		return;

	// Make sure phonemes are loaded
	FacePoser_EnsurePhonemesLoaded();

	// Unpause
	if ( m_bSimulating && m_bPaused )
	{
		m_bPaused = false;
		return;
	}

	m_bSimulating = true;
	m_bPaused = false;

//	float soundlatency =  max( sound->GetAmountofTimeAhead(), 0.0f );
//	soundlatency = min( 0.5f, soundlatency );

	float soundlatency = 0.0f;

	float sceneendtime = m_pScene->FindStopTime();

	m_pScene->SetSoundFileStartupLatency( soundlatency );

	if ( m_rgABPoints[ 0 ].active ||
		 m_rgABPoints[ 1 ].active  )
	{
		if ( m_rgABPoints[ 0 ].active &&
			 m_rgABPoints[ 1 ].active  )
		{
			float st = m_rgABPoints[ 0 ].time;
			float ed = m_rgABPoints[ 1 ].time;

			m_pScene->ResetSimulation( m_bForward, st, ed );

			SetScrubTime( m_bForward ? st : ed );
			SetScrubTargetTime( m_bForward ? ed : st );
		}
		else
		{
			float startonly = m_rgABPoints[ 0 ].active ? m_rgABPoints[ 0 ].time : m_rgABPoints[ 1 ].time;

			m_pScene->ResetSimulation( m_bForward, startonly );

			SetScrubTime( m_bForward ? startonly : sceneendtime );
			SetScrubTargetTime( m_bForward ? sceneendtime : startonly );
		}
	}
	else
	{
		// NO start end/loop
		m_pScene->ResetSimulation( m_bForward );

		SetScrubTime( m_bForward ? 0 : sceneendtime );
		SetScrubTargetTime( m_bForward ? sceneendtime : 0 );
	}

	expressions->NewMarkovIndices();

	if ( g_viewerSettings.speedScale == 0.0f )
	{
		m_flLastSpeedScale = g_viewerSettings.speedScale;
		m_bResetSpeedScale = true;

		g_viewerSettings.speedScale = 1.0f;

		Con_Printf( "Resetting speed scale to 1.0\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
//-----------------------------------------------------------------------------
void CChoreoView::MoveTimeSliderToPos( int x )
{
	m_nLeftOffset = x;
	m_pHorzScrollBar->setValue( m_nLeftOffset );
	InvalidateRect( (HWND)m_pHorzScrollBar->getHandle(), NULL, TRUE );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::PauseScene( void )
{
	if ( !m_bSimulating )
		return;

	m_bPaused = true;
	sound->StopAll();
}

//-----------------------------------------------------------------------------
// Purpose: Apply expression to actor's face
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CChoreoView::ProcessExpression( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::EXPRESSION );

	StudioModel *model = FindAssociatedModel( scene, event->GetActor() );
	if ( !model )
		return;

	studiohdr_t *hdr = model->getStudioHeader ();
	if ( !hdr )
	{
		return;
	}

	CExpClass *p = expressions->FindClass( event->GetParameters() );
	if ( !p )
	{
		return;
	}

	CExpression *exp = p->FindExpression( event->GetParameters2() );
	if ( !exp )
	{
		return;
	}

	// Resolve markov chains
	exp = exp->GetExpression();

	// Allow per model override to apply
	if ( FacePoser_GetOverridesShowing() )
	{
		if ( exp->HasOverride() && exp->GetOverride() )
		{
			exp = exp->GetOverride();
		}
	}

	CChoreoActor *a = event->GetActor();
	if ( !a )
		return;

	CChoreoActorWidget *actor = NULL;

	int i;
	for ( i = 0; i < m_SceneActors.Size(); i++ )
	{
		actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		if ( actor->GetActor() == a )
			break;
	}

	if ( !actor || i >= m_SceneActors.Size() )
		return;

	float *settings = exp->GetSettings();
	Assert( settings );
	float *weights = exp->GetWeights();
	Assert( weights );
	float *current = actor->GetSettings();
	Assert( current );

	float flIntensity = event->GetIntensity( scene->GetTime() );

	// Just add in to target values for correct actor
	for ( i = 0; i < hdr->numflexcontrollers; i++ )
	{
		int j = hdr->pFlexcontroller( i )->link;

		// FIXME: the events need to be processed in a specific order....
		// current[ i ] = current[i] * (1.0f - weights[i]) + settings[ i ] * flIntensity * weights[i];
		current[ i ] += settings[ i ] * flIntensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hdr - 
//			*event - 
//-----------------------------------------------------------------------------
void SetupFlexControllerTracks( studiohdr_t *hdr, CChoreoEvent *event )
{
	Assert( hdr );
	Assert( event );

	if ( !hdr )
		return;

	if ( !event )
		return;

	// Already done
	if ( event->GetTrackLookupSet() )
		return;

	for ( int i = 0; i < hdr->numflexcontrollers; i++ )
	{
		int j = hdr->pFlexcontroller( i )->link;

		char const *name = hdr->pFlexcontroller( i )->pszName();
		if ( !name )
			continue;

		bool combo = false;
		// Look up or create all necessary tracks
		if ( strncmp( "right_", name, 6 ) == 0 )
		{
			combo = true;
			name = &name[6];
		}

		CFlexAnimationTrack *track = event->FindTrack( name );
		if ( !track )
		{
			track = event->AddTrack( name );
			Assert( track );
		}

		track->SetFlexControllerIndex( i, j, 0 );
		if ( combo )
		{
			track->SetFlexControllerIndex( i + 1, hdr->pFlexcontroller( i + 1 )->link, 1 );
			track->SetComboType( true );
		}
		
		// set range
		if (hdr->pFlexcontroller( i )->max == 1.0f)
		{
			track->SetMin( hdr->pFlexcontroller( i )->min );
			track->SetMax( hdr->pFlexcontroller( i )->max );
		}
		else
		{
			// invert ranges for wide ranged, makes sense considering flexcontroller names...
			track->SetMin( hdr->pFlexcontroller( i )->max );
			track->SetMax( hdr->pFlexcontroller( i )->min );
		}

		// skip next flex since we've already assigned it
		if ( combo )
		{
			i++;
		}
	}

	event->SetTrackLookupSet( true );
}

//-----------------------------------------------------------------------------
// Purpose: Apply flexanimation to actor's face
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CChoreoView::ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::FLEXANIMATION );

	StudioModel *model = FindAssociatedModel( scene, event->GetActor()  );
	if ( !model )
		return;

	studiohdr_t *hdr = model->getStudioHeader ();
	if ( !hdr )
	{
		return;
	}

	CChoreoActor *a = event->GetActor();

	CChoreoActorWidget *actor = NULL;

	int i;
	for ( i = 0; i < m_SceneActors.Size(); i++ )
	{
		actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		if ( !stricmp( actor->GetActor()->GetName(), a->GetName() ) )
			break;
	}

	if ( !actor || i >= m_SceneActors.Size() )
		return;

	float *current = actor->GetSettings();
	Assert( current );

	if ( !event->GetTrackLookupSet() )
	{
		SetupFlexControllerTracks( hdr, event );
	}

	float weight = event->GetIntensity( scene->GetTime() );

	// Iterate animation tracks
	for ( i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				int controller = track->GetFlexControllerIndex( side );
				if ( controller != -1 )
				{
					// Get spline intensity for controller
					float flIntensity = track->GetIntensity( scene->GetTime(), side );
					flIntensity = current[ controller ] * (1 - weight) + flIntensity * weight;
					current[ controller ] = flIntensity;
				}
			}
		}
		else
		{
			int controller = track->GetFlexControllerIndex( 0 );
			if ( controller != -1 )
			{
				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scene->GetTime(), 0 );
				flIntensity = current[ controller ] * (1 - weight) + flIntensity * weight;
				current[ controller ] = flIntensity;
			}
		}
	}
}

#include "mapentities.h"

//-----------------------------------------------------------------------------
// Purpose: Apply lookat target
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CChoreoView::ProcessLookat( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::LOOKAT );

	if ( !event->GetActor() )
		return;

	CChoreoActor *a = event->GetActor();

	Assert( a );

	if (!stricmp( event->GetParameters(), a->GetName() ))
	{
		g_viewerSettings.flHeadTurn = 0.0f;
		g_viewerSettings.vecHeadTarget.Init();
		g_viewerSettings.vecEyeTarget.Init();
		m_bHasLookTarget = false;
	}
	else if ( !stricmp( event->GetParameters(), "player" ) || 
		!stricmp( event->GetParameters(), "!player" ) )
	{
		g_viewerSettings.flHeadTurn = event->GetIntensity( scene->GetTime() );
		Vector vecTarget = g_viewerSettings.trans;
		vecTarget.z = 0;

		g_viewerSettings.vecHeadTarget = vecTarget;
		g_viewerSettings.vecEyeTarget= vecTarget;

		AddHeadTarget( g_viewerSettings.flHeadTurn, vecTarget, vecTarget );

		m_bHasLookTarget = true;
	}
	else
	{
		mapentities->CheckUpdateMap( scene->GetMapname() );

		Vector orgActor;
		Vector orgTarget;
		QAngle anglesActor;
		QAngle anglesDummy;

		if ( event->GetPitch() != 0 ||
			 event->GetYaw() != 0 )
		{
			g_viewerSettings.flHeadTurn = event->GetIntensity( scene->GetTime() );

			QAngle angles( -(float)event->GetPitch(),
				(float)event->GetYaw(),
				0 );

			matrix3x4_t matrix;

			AngleMatrix( g_viewerSettings.rot, matrix );

			QAngle angles2 = angles;
			angles2.x *= 0.6f;
			angles2.y *= 0.8f;

			Vector vecForward, vecForward2;
			AngleVectors( angles, &vecForward );
			AngleVectors( angles2, &vecForward2 );

			VectorNormalize( vecForward );
			VectorNormalize( vecForward2 );

			Vector eyeTarget, headTarget;

			VectorRotate( vecForward, matrix, eyeTarget );
			VectorRotate( vecForward2, matrix, headTarget );

			VectorScale( eyeTarget, 150, eyeTarget );
			g_viewerSettings.vecEyeTarget = eyeTarget;

			VectorScale( headTarget, 150, headTarget );
			g_viewerSettings.vecHeadTarget = headTarget;

			AddHeadTarget( g_viewerSettings.flHeadTurn, eyeTarget, headTarget );

			m_bHasLookTarget = true;
		}
		else
		{
			if ( mapentities->LookupOrigin( a->GetName(), orgActor, anglesActor ) )
			{
				if ( mapentities->LookupOrigin( event->GetParameters(), orgTarget, anglesDummy ) )
				{
					Vector delta = orgTarget - orgActor;
					
					matrix3x4_t matrix;
					Vector lookTarget;

					// Rotate around actor's placed forward direction since we look straight down x in faceposer/hlmv
					AngleMatrix( anglesActor, matrix );
					VectorIRotate( delta, matrix, lookTarget );
					
					g_viewerSettings.flHeadTurn = event->GetIntensity( scene->GetTime() );

					g_viewerSettings.vecHeadTarget = lookTarget;
					g_viewerSettings.vecEyeTarget = lookTarget;

					AddHeadTarget( g_viewerSettings.flHeadTurn, lookTarget, lookTarget );

					m_bHasLookTarget = true;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoView::ProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::LOOP );

	// Don't loop when dragging scrubber!
	if ( IsScrubbing() )
		return;

	float t = scene->GetTime();
	float backtime = (float)atof( event->GetParameters() );

	bool process = true;
	int counter = event->GetLoopCount();
	if ( counter != -1 )
	{
		int remaining = event->GetNumLoopsRemaining();
		if ( remaining <= 0 )
		{
			process = false;
		}
		else
		{
			event->SetNumLoopsRemaining( --remaining );
		}
	}

	if ( !process )
		return;

	scene->LoopToTime( backtime );
	SetScrubTime( backtime ); 
}

//-----------------------------------------------------------------------------
// Purpose: Apply lookat target
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CChoreoView::ProcessGesture( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::GESTURE );

	StudioModel *model = FindAssociatedModel( scene, event->GetActor()  );
	if ( !model )
		return;

	if ( !event->GetActor() )
		return;

	CChoreoActor *a = event->GetActor();

	Assert( a );

	int iSequence = model->LookupSequence( event->GetParameters() );

	// Get spline intensity for controller
	float eventlocaltime = scene->GetTime() - event->GetStartTime();

	float referencetime = event->GetShiftedTimeFromReferenceTime( eventlocaltime / event->GetDuration() ) * event->GetDuration();

	float resampledtime = event->GetStartTime() + referencetime;

	float cycle = event->GetCompletion( resampledtime );

	int iLayer = model->GetNewAnimationLayer( scene->GetChannelIndex( event->GetChannel() ) );
	model->SetOverlaySequence( iLayer, iSequence, event->GetIntensity( scene->GetTime() ) );
	model->SetOverlayRate( iLayer, cycle, 0.0 );
}



//-----------------------------------------------------------------------------
// Purpose: Apply lookat target
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::ProcessSequence( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::SEQUENCE );

	if ( !m_bProcessSequences )
	{
		return;
	}

	StudioModel *model = FindAssociatedModel( scene, event->GetActor()  );
	if ( !model )
		return;

	if ( !event->GetActor() )
		return;

	CChoreoActor *a = event->GetActor();

	Assert( a );

	int iSequence = model->LookupSequence( event->GetParameters() );
	float flFrameRate;
	float flGroundSpeed;
	model->GetSequenceInfo( iSequence, &flFrameRate, &flGroundSpeed );

	float dt = scene->GetTime() - event->GetStartTime();
	float cycle = flFrameRate * dt;
	cycle = cycle - (int)(cycle);

	int iLayer = model->GetNewAnimationLayer( scene->GetChannelIndex( event->GetChannel() ) );
	model->SetOverlaySequence( iLayer, iSequence, event->GetIntensity( scene->GetTime() ) );
	model->SetOverlayRate( iLayer, cycle, 0.0 );
}



//-----------------------------------------------------------------------------
// Purpose: Process a pause event
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::ProcessPause( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::SECTION );

	// Don't pause if scrubbing
	bool scrubbing = ( m_nDragType == DRAGTYPE_SCRUBBER ) ? true : false;
	if ( scrubbing )
		return;

	PauseScene();

	m_bAutomated		= false;
	m_nAutomatedAction	= SCENE_ACTION_UNKNOWN;
	m_flAutomationDelay = 0.0f;
	m_flAutomationTime = 0.0f;

	// Check for auto resume/cancel
	ParseFromMemory( (char *)event->GetParameters(), strlen( event->GetParameters() ) );
	if ( g_TokenProcessor.TokenAvailable() )
	{
		g_TokenProcessor.GetToken( false );
		if ( !stricmp( g_TokenProcessor.CurrentToken(), "automate" ) )
		{
			if ( g_TokenProcessor.TokenAvailable() )
			{
				g_TokenProcessor.GetToken( false );
				if ( !stricmp( g_TokenProcessor.CurrentToken(), "Cancel" ) )
				{
					m_nAutomatedAction = SCENE_ACTION_CANCEL;
				}
				else if ( !stricmp( g_TokenProcessor.CurrentToken(), "Resume" ) )
				{
					m_nAutomatedAction = SCENE_ACTION_RESUME;
				}

				if ( g_TokenProcessor.TokenAvailable() && 
					m_nAutomatedAction != SCENE_ACTION_UNKNOWN )
				{
					g_TokenProcessor.GetToken( false );
					m_flAutomationDelay = (float)atof( g_TokenProcessor.CurrentToken() );

					if ( m_flAutomationDelay > 0.0f )
					{
						// Success
						m_bAutomated = true;
						m_flAutomationTime = 0.0f;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Main event processor
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CChoreoView::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event)
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
	{
		return;
	}

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
	{
		return;
	}

	switch( event->GetType() )
	{
	case CChoreoEvent::EXPRESSION:
		ProcessExpression( scene, event );
		break;
	case CChoreoEvent::FLEXANIMATION:
		ProcessFlexAnimation( scene, event );
		break;
	case CChoreoEvent::LOOKAT:
		ProcessLookat( scene, event );
		break;
	case CChoreoEvent::GESTURE:
		ProcessGesture( scene, event );
		break;
	case CChoreoEvent::SEQUENCE:
		ProcessSequence( scene, event );
		break;
	case CChoreoEvent::SUBSCENE:
		ProcessSubscene( scene, event );
		break;
	case CChoreoEvent::SPEAK:
		ProcessSpeak( scene, event );
		break;
	case CChoreoEvent::STOPPOINT:
		// Nothing
		break;
	case CChoreoEvent::INTERRUPT:
		ProcessInterrupt( scene, event );
		break;
	default:
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Main event completion checker
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event)
		return true;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
	{
		return true;
	}

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
	{
		return true;
	}

	switch( event->GetType() )
	{
	case CChoreoEvent::EXPRESSION:
		break;
	case CChoreoEvent::FLEXANIMATION:
		break;
	case CChoreoEvent::LOOKAT:
		break;
	case CChoreoEvent::GESTURE:
		break;
	case CChoreoEvent::SEQUENCE:
		break;
	case CChoreoEvent::SUBSCENE:
		break;
	case CChoreoEvent::SPEAK:
		break;
	case CChoreoEvent::MOVETO:
		break;
	case CChoreoEvent::INTERRUPT:
		break;
	default:
		break;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::PauseThink( void )
{
	// FIXME:  Game code would check for conditions being met

	if ( !m_bAutomated )
		return;

	m_flAutomationTime += fabs( m_flFrameTime );

	RECT rcPauseRect;
	rcPauseRect.left = 0;
	rcPauseRect.right = w2();
	rcPauseRect.top = GetCaptionHeight() + SCRUBBER_HEIGHT;
	rcPauseRect.bottom = rcPauseRect.top + 10;

	CChoreoWidgetDrawHelper drawHelper( this, 
		rcPauseRect,
		COLOR_CHOREO_BACKGROUND );

	DrawSceneABTicks( drawHelper );

	if ( m_flAutomationDelay > 0.0f &&
		m_flAutomationTime < m_flAutomationDelay )
	{
		char sz[ 256 ];
		sprintf( sz, "Pause %.2f/%.2f", m_flAutomationTime, m_flAutomationDelay );
	
		int textlen = drawHelper.CalcTextWidth( "Arial", 9, FW_NORMAL, sz );

		RECT rcText;
		GetScrubHandleRect( rcText, true );

		rcText.left = ( rcText.left + rcText.right ) / 2;
		rcText.left -= ( textlen * 0.5f );
		rcText.right = rcText.left + textlen + 1;

		rcText.top = rcPauseRect.top;
		rcText.bottom = rcPauseRect.bottom;

		drawHelper.DrawColoredText( "Arial", 9, FW_NORMAL, COLOR_CHOREO_PLAYBACKTICKTEXT, rcText, sz );

		return;
	}

	// Time to act
	m_bAutomated = false;

	switch ( m_nAutomatedAction )
	{
	case SCENE_ACTION_RESUME:
		m_bPaused = false;
		sound->StopAll();
		break;
	case SCENE_ACTION_CANCEL:
		FinishSimulation();
		break;
	default:
		break;
	}

	m_nAutomatedAction = SCENE_ACTION_UNKNOWN;
	m_flAutomationTime = 0.0f;
	m_flAutomationDelay = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Conclude simulation
//-----------------------------------------------------------------------------
void CChoreoView::FinishSimulation( void )
{
	if ( !m_bSimulating )
		return;

//	m_pScene->ResetSimulation();

	expressions->NewMarkovIndices();

	m_bSimulating = false;
	m_bPaused = false;

	sound->StopAll();

	if ( m_bResetSpeedScale )
	{
		m_bResetSpeedScale = false;
		g_viewerSettings.speedScale = m_flLastSpeedScale;
		m_flLastSpeedScale = 0.0f;

		Con_Printf( "Resetting speed scale to %f\n", m_flLastSpeedScale );
	}

	models->ClearOverlaysSequences();

	// redraw();
}

void CChoreoView::SceneThink( float time )
{
	if ( !m_pScene )
		return;

	if ( m_bSimulating )
	{
		if ( m_bPaused )
		{
			PauseThink();
		}
		else
		{
			m_pScene->SetSoundFileStartupLatency( 0.0f );

			models->CheckResetFlexes();

			ResetTargetSettings();

			models->ClearOverlaysSequences();

			ResetHeadTarget();

			// Tell scene to go
			m_pScene->Think( time );

			UpdateHeadTarget();

			// Move flexes toward their targets
			UpdateCurrentSettings();
		}
	}
	else
	{
		FinishSimulation();
	}

	if ( !ShouldProcessSpeak() )
	{
		bool autoprocess = ShouldAutoProcess();
		bool anyscrub = IsAnyToolScrubbing() ;
		bool anyprocessing = IsAnyToolProcessing();

		//Con_Printf( "autoprocess %i anyscrub %i anyprocessing %i\n",
		//	autoprocess ? 1 : 0,
		//	anyscrub ? 1 : 0,
		//	anyprocessing ? 1 : 0 );

		if ( !anyscrub &&
			 !anyprocessing &&
			 autoprocess
		 )
		{
			sound->StopAll();

			if ( !m_bHasLookTarget )
			{
				g_viewerSettings.flHeadTurn = 0.0f;
				g_viewerSettings.vecHeadTarget.Init();
				g_viewerSettings.vecEyeTarget.Init();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::LayoutScene( void )
{
	if ( !m_pScene )
		return;

	if ( m_bLayoutIsValid )
		return;

	m_pScene->ReconcileTags();

	RECT rc;
	GetClientRect( (HWND)getHandle(), &rc );

	RECT rcClient = rc;
	rcClient.top += GetStartRow();
	OffsetRect( &rcClient, 0, -m_nTopOffset );

	m_flStartTime = m_nLeftOffset / GetPixelsPerSecond();

	m_flEndTime = m_flStartTime + (float)( rcClient.right - GetLabelWidth() ) / GetPixelsPerSecond();

	m_rcTimeLine = rcClient;
	m_rcTimeLine.top = GetCaptionHeight() + SCRUBBER_HEIGHT;
	m_rcTimeLine.bottom = rcClient.top - 1;

	int currentRow = rcClient.top + 2;
	int itemHeight;

	// Draw actors
	int i;
	for ( i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		Assert( a );
		if ( !a )
		{
			continue;
		}

		// Figure out rectangle
		itemHeight = a->GetItemHeight();

		RECT rcActor = rcClient;
		rcActor.top = currentRow;
		rcActor.bottom = currentRow + itemHeight;

		a->Layout( rcActor );

		currentRow += itemHeight;
	}

	// Draw section tabs
	for ( i = 0; i < m_SceneGlobalEvents.Size(); i++ )
	{
		CChoreoGlobalEventWidget *e = m_SceneGlobalEvents[ i ];
		if ( !e )
			continue;

		RECT rcEvent;
		rcEvent = m_rcTimeLine;

		float frac = ( e->GetEvent()->GetStartTime() - m_flStartTime ) / ( m_flEndTime - m_flStartTime );
			
		rcEvent.left = GetLabelWidth() + rcEvent.left + (int)( frac * ( m_rcTimeLine.right - m_rcTimeLine.left - GetLabelWidth() ) );
		rcEvent.left -= 4;
		rcEvent.right = rcEvent.left + 8;
		rcEvent.bottom += 0;
		rcEvent.top = rcEvent.bottom - 8;

		if ( rcEvent.left + 10 < GetLabelWidth() )
		{
			e->setVisible( false );
		}
		else
		{
			e->setVisible( true );
		}

	//	OffsetRect( &rcEvent, GetLabelWidth(), 0 );

		e->Layout( rcEvent );
	}

	m_bLayoutIsValid = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::DeleteSceneWidgets( void )
{
	bool oldcandraw = m_bCanDraw;

	m_bCanDraw = false;

	int i;
	CChoreoWidget *w;

	ClearStatusArea();

	for( i = 0 ; i < m_SceneActors.Size(); i++ )
	{
		w = m_SceneActors[ i ];
		m_ActorExpanded[ i ].expanded = ((CChoreoActorWidget *)w)->GetShowChannels();
		delete w;
	}

	m_SceneActors.RemoveAll();

	for( i = 0 ; i < m_SceneGlobalEvents.Size(); i++ )
	{
		w = m_SceneGlobalEvents[ i ];
		delete w;
	}

	m_SceneGlobalEvents.RemoveAll();

	m_bCanDraw = oldcandraw;

	// Make sure nobody is still pointing at us
	m_pClickedActor = NULL;
	m_pClickedChannel = NULL;
	m_pClickedEvent = NULL;
	m_pClickedGlobalEvent = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::InvalidateLayout( void )
{
	if ( m_bSuppressLayout )
		return;

	if ( ComputeHPixelsNeeded() != m_nLastHPixelsNeeded )
	{
		RepositionHSlider();
	}

	if ( ComputeVPixelsNeeded() != m_nLastVPixelsNeeded )
	{
		RepositionVSlider();
	}

	m_bLayoutIsValid = false;
	redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::CreateSceneWidgets( void )
{
	DeleteSceneWidgets();

	m_bSuppressLayout = true;

	int i;
	for ( i = 0; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor *a = m_pScene->GetActor( i );
		Assert( a );
		if ( !a )
			continue;

		CChoreoActorWidget *actorWidget = new CChoreoActorWidget( NULL );
		Assert( actorWidget );

		actorWidget->SetActor( a );
		actorWidget->Create();

		m_SceneActors.AddToTail( actorWidget );

		actorWidget->ShowChannels( m_ActorExpanded[ i ].expanded );
	}

	// Find global events
	for ( i = 0; i < m_pScene->GetNumEvents(); i++ )
	{
		CChoreoEvent *e = m_pScene->GetEvent( i );
		if ( !e || e->GetActor() )
			continue;

		CChoreoGlobalEventWidget *eventWidget = new CChoreoGlobalEventWidget( NULL );
		Assert( eventWidget );

		eventWidget->SetEvent( e );
		eventWidget->Create();

		m_SceneGlobalEvents.AddToTail( eventWidget );
	}

	m_bSuppressLayout = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetLabelWidth( void )
{
	return m_nLabelWidth;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetStartRow( void )
{
	return m_nStartRow + GetCaptionHeight() + SCRUBBER_HEIGHT;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetRowHeight( void )
{
	return m_nRowHeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetFontSize( void )
{
	return m_nFontSize;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::ComputeVPixelsNeeded( void )
{
	int pixels = 0;
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		pixels += actor->GetItemHeight() + 2;
	}

	pixels += GetStartRow() + 15;

//	pixels += m_nInfoHeight;

	//pixels += 30;
	return pixels;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::ComputeHPixelsNeeded( void )
{
	if ( !m_pScene )
	{
		return 0;
	}

	int pixels = 0;
	float maxtime = m_pScene->FindStopTime();
	if ( maxtime < 5.0 )
	{
		maxtime = 5.0f;
	}
	pixels = (int)( ( maxtime + 5.0 ) * GetPixelsPerSecond() );

	return pixels;

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::RepositionVSlider( void )
{
	int pixelsneeded = ComputeVPixelsNeeded();

	if ( pixelsneeded <= ( h2() - GetStartRow() ))
	{
		m_pVertScrollBar->setVisible( false );
		m_nTopOffset = 0;
	}
	else
	{
		m_pVertScrollBar->setVisible( true );
	}

	m_pVertScrollBar->setBounds( w2() - m_nScrollbarHeight, GetStartRow(), m_nScrollbarHeight, h2() - m_nScrollbarHeight - GetStartRow() );

	//int visiblepixels = h2() - m_nScrollbarHeight - GetStartRow();
	//m_nTopOffset = min( pixelsneeded - visiblepixels, m_nTopOffset );
	m_nTopOffset = max( 0, m_nTopOffset );
	m_nTopOffset = min( pixelsneeded, m_nTopOffset );

	m_pVertScrollBar->setRange( 0, pixelsneeded );
	m_pVertScrollBar->setValue( m_nTopOffset );
	m_pVertScrollBar->setPagesize( h2() - GetStartRow() );

	m_nLastVPixelsNeeded = pixelsneeded;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::RepositionHSlider( void )
{
	int pixelsneeded = ComputeHPixelsNeeded();

	if ( pixelsneeded <= w2() )
	{
		m_pHorzScrollBar->setVisible( false );
	}
	else
	{
		m_pHorzScrollBar->setVisible( true );
	}
	m_pHorzScrollBar->setBounds( 0, h2() - m_nScrollbarHeight, w2() - m_nScrollbarHeight, m_nScrollbarHeight );

	m_nLeftOffset = max( 0, m_nLeftOffset );
	m_nLeftOffset = min( pixelsneeded, m_nLeftOffset );

	m_pHorzScrollBar->setRange( 0, pixelsneeded );
	m_pHorzScrollBar->setValue( m_nLeftOffset );
	m_pHorzScrollBar->setPagesize( w2() );

	m_nLastHPixelsNeeded = pixelsneeded;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dirty - 
//-----------------------------------------------------------------------------
void CChoreoView::SetDirty( bool dirty )
{
	bool changed = dirty != m_bDirty;

	m_bDirty = dirty;

	if ( !dirty )
	{
		WipeUndo();
		redraw();
	}

	if ( changed )
	{
		SetPrefix( m_bDirty ? "* " : "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::New( void )
{
	if ( m_pScene )
	{
		Close( );
		if ( m_pScene )
		{
			return;
		}
	}

	char workingdir[ 256 ];
	Q_getwd( workingdir );
	strlwr( workingdir );
	COM_FixSlashes( workingdir );

	// Show file io
	bool inScenesAlready = false;
	if ( strstr( workingdir, va( "%s/scenes", GetGameDirectory() ) ) )
	{
		inScenesAlready = true;
	}
	const char *filename = mxGetSaveFileName( 
		this, 
		inScenesAlready ? "." : FacePoser_MakeWindowsSlashes( va( "%s/scenes/", GetGameDirectory() ) ), 
		"*.vcd" );
	if ( filename && filename[ 0 ] )
	{
		char scenefile[ 512 ];
		strcpy( scenefile, filename );
		StripExtension( scenefile );
		DefaultExtension( scenefile, ".vcd" );
		
		m_pScene = new CChoreoScene( this );
		g_MDLViewer->InitGridSettings();
		SetChoreoFile( scenefile );
		m_pScene->SetPrintFunc( Con_Printf );

		ShowButtons( true );

		SetDirty( false );
	}

	if ( !m_pScene )
		return;

	// Get first actor name
	CActorParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Create Actor" );
	strcpy( params.m_szName, "" );

	if ( !ActorProperties( &params ) )
		return;

	if ( strlen( params.m_szName ) <= 0 )
		return;

	SetDirty( true );

	PushUndo( "Create Actor" );

	Con_Printf( "Creating scene %s with actor '%s'\n", GetChoreoFile(), params.m_szName );

	CChoreoActor *actor = m_pScene->AllocActor();
	if ( actor )
	{
		actor->SetName( params.m_szName );
	}

	PushRedo( "Create Actor" );

	CreateSceneWidgets();
	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::Save( void )
{
	if ( !m_pScene )
		return;

	Con_Printf( "Saving changes to %s\n", GetChoreoFile() );

	MakeFileWriteable( GetChoreoFile() );
	if (!m_pScene->SaveToFile( GetChoreoFile() ))
	{
  		mxMessageBox( this, va( "Unable to write \"%s\"", GetChoreoFile() ),
  			"SaveToFile", MX_MB_OK | MX_MB_ERROR );
	}

	SetDirty( false );
	redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::SaveAs( void )
{
	if ( !m_pScene )
		return;

	char workingdir[ 256 ];
	Q_getwd( workingdir );
	strlwr( workingdir );
	COM_FixSlashes( workingdir );

	// Show file io
	bool inScenesAlready = false;
	if ( strstr( workingdir, va( "%s/scenes", GetGameDirectory() ) ) )
	{
		inScenesAlready = true;
	}
	const char *filename = mxGetSaveFileName( 
		this, 
		inScenesAlready ? "." : FacePoser_MakeWindowsSlashes( va( "%s/scenes/", GetGameDirectory() ) ), 
		"*.vcd" );
	if ( !filename || !filename[ 0 ] )
		return;

	char scenefile[ 512 ];
	strcpy( scenefile, filename );
	StripExtension( scenefile );
	DefaultExtension( scenefile, ".vcd" );
	
	// Strip out the game directory
	//
	char relative[ 512 ];
	strcpy( relative, scenefile );
	if ( strchr( scenefile, ':' ) )
	{
		filesystem->FullPathToRelativePath( scenefile, relative );
	}


	// Change filename
	SetChoreoFile( relative );

	Con_Printf( "Saving %s\n", GetChoreoFile() );

	MakeFileWriteable( GetChoreoFile() );
	if (!m_pScene->SaveToFile( GetChoreoFile() ))
	{
  		mxMessageBox( this, va( "Unable to write \"%s\"", GetChoreoFile() ),
  			"SaveToFile", MX_MB_OK | MX_MB_ERROR );
	}

	SetDirty( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::Load( void )
{
	const char *filename = NULL;
	
	filename = mxGetOpenFileName( 
		this, 
		// mxGetOpenFilename likes backslashes instead
		FacePoser_MakeWindowsSlashes( va( "%s/scenes/", GetGameDirectory() ) ),
		"*.vcd" );

	if ( !filename || !filename[ 0 ] )
		return;

	if ( m_pScene )
	{
		Close();
		if ( m_pScene )
		{
			return;
		}
	}

	LoadSceneFromFile( filename );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CChoreoView::LoadSceneFromFile( const char *filename )
{
	// Strip out the game directory
	//
	char relative[ 512 ];
	strcpy( relative, filename );
	if ( strchr( filename, ':' ) )
	{
		filesystem->FullPathToRelativePath( filename, relative );
	}

	m_pScene = LoadScene( filename );
	g_MDLViewer->InitGridSettings();
	if ( !m_pScene )
		return;

	ShowButtons( true );

	CChoreoWidget::m_pScene = m_pScene;
	SetChoreoFile( relative );

	bool cleaned = FixupSequenceDurations( m_pScene, false );

	SetDirty( cleaned );

	DeleteSceneWidgets();
	CreateSceneWidgets();

	// Force scroll bars to resize
	m_nLastHPixelsNeeded = -1;
	m_nLastVPixelsNeeded = -1;

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : closing - 
//-----------------------------------------------------------------------------
void CChoreoView::UnloadScene( void )
{
	InvalidateLayout();
	ReportSceneClearToTools();

	ClearStatusArea();

	delete m_pScene;
	m_pScene = NULL;
	SetDirty( false );
	SetChoreoFile( "" );
	g_MDLViewer->InitGridSettings();
	CChoreoWidget::m_pScene = NULL;

	DeleteSceneWidgets();

	m_pVertScrollBar->setVisible( false );
	m_pHorzScrollBar->setVisible( false );

	ShowButtons( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoView::DeleteChannel( CChoreoChannel *channel )
{
	if ( !channel || !m_pScene )
		return;

	SetDirty( true );

	PushUndo( "Delete Channel" );

	DeleteSceneWidgets();

	// Delete channel and it's children
	// Find the appropriate actor
	for ( int i = 0; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor *a = m_pScene->GetActor( i );
		if ( !a )
			continue;

		if ( a->FindChannelIndex( channel ) == -1 )
			continue;

		Con_Printf( "Deleting %s\n", channel->GetName() );
		a->RemoveChannel( channel );
			
		m_pScene->DeleteReferencedObjects( channel );
		break;
	}

	ReportSceneClearToTools();

	CreateSceneWidgets();

	PushRedo( "Delete Channel" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::NewChannel( void )
{
	if ( !m_pScene )
		return;

	if ( !m_pScene->GetNumActors() )
	{
		Con_Printf( "You must create an actor before you can add a channel\n" );
		return;
	}

	CChannelParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Create Channel" );
	strcpy( params.m_szName, "" );
	params.m_bShowActors = true;
	strcpy( params.m_szSelectedActor, "" );
	params.m_pScene = m_pScene;

	if ( !ChannelProperties( &params ) )
	{
		return;
	}

	if ( strlen( params.m_szName ) <= 0 )
	{
		return;
	}

	CChoreoActor *actor = m_pScene->FindActor( params.m_szSelectedActor );
	if ( !actor )
	{
		Con_Printf( "Can't add channel %s, actor %s doesn't exist\n", params.m_szName, params.m_szSelectedActor );
		return;
	}

	SetDirty( true );

	PushUndo( "Add Channel" );

	DeleteSceneWidgets();

	CChoreoChannel *channel = m_pScene->AllocChannel();
	if ( !channel )
	{
		Con_Printf( "Unable to allocate channel %s!\n", params.m_szName );
	}
	else
	{
		channel->SetName( params.m_szName );
		channel->SetActor( actor );
		actor->AddChannel( channel );
	}

	CreateSceneWidgets();

	PushRedo( "Add Channel" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoView::MoveChannelUp( CChoreoChannel *channel )
{
	SetDirty( true );

	PushUndo( "Move Channel Up" );

	DeleteSceneWidgets();

	// Find the appropriate actor
	for ( int i = 0; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor *a = m_pScene->GetActor( i );
		if ( !a )
			continue;

		int index = a->FindChannelIndex( channel );
		if ( index == -1 )
			continue;

		if ( index != 0 )
		{
			Con_Printf( "Moving %s up\n", channel->GetName() );
			a->SwapChannels( index, index - 1 );
		}
		break;
	}

	CreateSceneWidgets();

	PushRedo( "Move Channel Up" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoView::MoveChannelDown( CChoreoChannel *channel )
{
	SetDirty( true );

	PushUndo( "Move Channel Down" );

	DeleteSceneWidgets();

	// Find the appropriate actor
	for ( int i = 0; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor *a = m_pScene->GetActor( i );
		if ( !a )
			continue;

		int index = a->FindChannelIndex( channel );
		if ( index == -1 )
			continue;

		if ( index < a->GetNumChannels() - 1 )
		{
			Con_Printf( "Moving %s down\n", channel->GetName() );
			a->SwapChannels( index, index + 1 );
		}
		break;
	}

	CreateSceneWidgets();

	PushRedo( "Move Channel Down" );

	// Redraw
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoView::EditChannel( CChoreoChannel *channel )
{
	if ( !channel )
		return;

	CChannelParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Edit Channel" );
	strcpy( params.m_szName, channel->GetName() );

	if ( !ChannelProperties( &params ) )
		return;

	if ( strlen( params.m_szName ) <= 0 )
		return;

	SetDirty( true );

	PushUndo( "Edit Channel" );

	channel->SetName( params.m_szName );

	PushRedo( "Edit Channel" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoView::DeleteActor( CChoreoActor *actor )
{
	if ( !actor || !m_pScene )
		return;

	SetDirty( true );

	PushUndo( "Delete Actor" );

	DeleteSceneWidgets();

	// Delete channel and it's children
	// Find the appropriate actor
	Con_Printf( "Deleting %s\n", actor->GetName() );
	m_pScene->RemoveActor( actor );

	m_pScene->DeleteReferencedObjects( actor );

	ReportSceneClearToTools();

	CreateSceneWidgets();

	PushRedo( "Delete Actor" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::NewActor( void )
{
	if ( !m_pScene )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "You must load or create a scene file first\n" );
		return;
	}

	CActorParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Create Actor" );
	strcpy( params.m_szName, "" );

	if ( !ActorProperties( &params ) )
		return;

	if ( strlen( params.m_szName ) <= 0 )
		return;

	SetDirty( true );

	PushUndo( "Add Actor" );

	DeleteSceneWidgets();

	Con_Printf( "Adding new actor '%s'\n", params.m_szName );

	CChoreoActor *actor = m_pScene->AllocActor();
	if ( actor )
	{
		actor->SetName( params.m_szName );
	}

	CreateSceneWidgets();

	PushRedo( "Add Actor" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoView::MoveActorUp( CChoreoActor *actor )
{
	DeleteSceneWidgets();

	int index = m_pScene->FindActorIndex( actor );
	// found it and it's not first
	if ( index != -1 && index != 0 )
	{
		Con_Printf( "Moving %s up\n", actor->GetName() );

		SetDirty( true );

		PushUndo( "Move Actor Up" );

		m_pScene->SwapActors( index, index - 1 );

		PushRedo( "Move Actor Up" );
	}

	CreateSceneWidgets();
	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoView::MoveActorDown( CChoreoActor *actor )
{
	DeleteSceneWidgets();

	int index = m_pScene->FindActorIndex( actor );
	// found it and it's not first
	if ( index != -1 && ( index < m_pScene->GetNumActors() - 1 ) )
	{
		Con_Printf( "Moving %s down\n", actor->GetName() );
		
		SetDirty( true );
	
		PushUndo( "Move Actor Down" );

		m_pScene->SwapActors( index, index + 1 );

		PushRedo( "Move Actor Down" );
	}

	CreateSceneWidgets();
	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoView::EditActor( CChoreoActor *actor )
{
	if ( !actor )
		return;

	CActorParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Edit Actor" );
	strcpy( params.m_szName, actor->GetName() );

	if ( !ActorProperties( &params ) )
		return;

	if ( strlen( params.m_szName ) <= 0 )
		return;

	SetDirty( true );

	PushUndo( "Edit Actor" );

	actor->SetName( params.m_szName );

	PushRedo( "Edit Actor" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void CChoreoView::AddEvent( int type )
{
	int mx, my;
	mx = m_nClickedX;
	my = m_nClickedY;
	CChoreoChannelWidget *channel = m_pClickedChannel;
	if ( !channel || !channel->GetChannel() )
	{
		CChoreoActorWidget *actor = m_pClickedActor;
		if ( actor )
		{
			if ( actor->GetNumChannels() <= 0 )
				return;

			channel = actor->GetChannel( 0 );
			if ( !channel || !channel->GetChannel() )
				return;
		}
		else
		{
			return;
		}
	}

	// Convert click position local to this window
	POINT pt;
	pt.x = mx;
	pt.y = my;
	// Offset by label area
	pt.x += GetLabelWidth();

	CEventParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Create Event" );

	params.m_nType = type;
	params.m_pScene = m_pScene;

	params.m_bFixedLength = false;
	params.m_bResumeCondition = false;
	params.m_flStartTime = GetTimeValueForMouse( pt.x );
	switch ( type )
	{
	case CChoreoEvent::EXPRESSION:
	case CChoreoEvent::FLEXANIMATION:
	case CChoreoEvent::GESTURE:
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::LOOKAT:
	case CChoreoEvent::FACE:
	case CChoreoEvent::SUBSCENE:
	case CChoreoEvent::INTERRUPT:
		params.m_bHasEndTime = true;
		params.m_flEndTime = params.m_flStartTime + 0.5f;
		break;
	case CChoreoEvent::SPEAK:
		params.m_bFixedLength = true;
		params.m_bHasEndTime = false;
		params.m_flEndTime = -1.0f;
		break;
	default:
		params.m_bHasEndTime = false;
		params.m_flEndTime = -1.0f;
		break;
	}

	params.m_bUsesTag = false;

	while (1)
	{
		if (!EventProperties( &params ))
			return;

		if (strlen( params.m_szName ) <= 0 )
		{
			mxMessageBox( this, "Event must have a valid name",
				"AddEvent", MX_MB_OK | MX_MB_ERROR );
		}
		else if ( ( type != CChoreoEvent::FLEXANIMATION ) && 
				 ( type != CChoreoEvent::INTERRUPT ) &&
				( strlen( params.m_szParameters ) <= 0 ) )
		{
			mxMessageBox( this, va( "No parameters specified for %s\n", params.m_szName ),
				"AddEvent", MX_MB_OK | MX_MB_ERROR );
		}
		else
		{
			break;	
		}
	}

	SetDirty( true );

	PushUndo( "Add Event" );

	CChoreoEvent *event = m_pScene->AllocEvent();
	if ( event )
	{
		event->SetType( (CChoreoEvent::EVENTTYPE)type );
		event->SetName( params.m_szName );
		event->SetParameters( params.m_szParameters );
		event->SetParameters2( params.m_szParameters2 );
		event->SetStartTime( params.m_flStartTime );
		if ( params.m_bUsesTag )
		{
			event->SetUsingRelativeTag( true, params.m_szTagName, params.m_szTagWav );
		}
		else
		{
			event->SetUsingRelativeTag( false );
		}
		CChoreoChannel *pchannel = channel->GetChannel();

		event->SetChannel( pchannel );
		event->SetActor( pchannel->GetActor() );

		if ( params.m_bHasEndTime &&
			params.m_flEndTime != -1.0 &&
			params.m_flEndTime > params.m_flStartTime )
		{
			event->SetEndTime( params.m_flEndTime );
		}
		else
		{
			event->SetEndTime( -1.0f );
		}

		switch ( event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Just grab end time
				CChoreoScene *scene = LoadScene( event->GetParameters() );
				if ( scene )
				{
					event->SetEndTime( params.m_flStartTime + scene->FindStopTime() );
				}
				delete scene;
			}
			break;
		case CChoreoEvent::SEQUENCE:
			{
				CheckSequenceLength( event, false );
				AutoaddSequenceKeys( event);
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				DefaultGestureLength( event, false );
				AutoaddGestureEvents( event, false );
				// AutoaddSequenceKeys( event );
			}
			break;
		case CChoreoEvent::LOOKAT:
			{
				if ( params.usepitchyaw )
				{
					event->SetPitch( params.pitch );
					event->SetYaw( params.yaw );
				}
				else
				{
					event->SetPitch( 0 );
					event->SetYaw( 0 );
				}
			}
			break;
		case CChoreoEvent::SPEAK:
			{
				// Try and load wav to get length
				CAudioSource *wave = sound->LoadSound( va( "sound/%s", FacePoser_TranslateSoundName( event->GetParameters() ) ) );
				if ( wave )
				{
					event->SetEndTime( params.m_flStartTime + wave->GetRunningLength() );
					delete wave;
				}
			}
			break;
		}
		event->SnapTimes();

		DeleteSceneWidgets();

		// Add to appropriate channel
		pchannel->AddEvent( event );

		CreateSceneWidgets();

		// Redraw
		InvalidateLayout();
	}

	PushRedo( "Add Event" );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a scene "pause" event
//-----------------------------------------------------------------------------
void CChoreoView::AddGlobalEvent( CChoreoEvent::EVENTTYPE type )
{
	int mx, my;
	mx = m_nClickedX;
	my = m_nClickedY;

	// Convert click position local to this window
	POINT pt;
	pt.x = mx;
	pt.y = my;

	CGlobalEventParams params;
	memset( &params, 0, sizeof( params ) );

	params.m_nType = type;

	switch ( type )
	{
	default:
		Assert( 0 );
		strcpy( params.m_szDialogTitle, "???" );
		break;
	case CChoreoEvent::SECTION:
		{
			strcpy( params.m_szDialogTitle, "Add Pause Point" );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			strcpy( params.m_szDialogTitle, "Add Loop Point" );
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			strcpy( params.m_szDialogTitle, "Add Stop Point" );
		}
		break;
	}
	strcpy( params.m_szName, "" );
	strcpy( params.m_szAction, "" );

	params.m_flStartTime = GetTimeValueForMouse( pt.x );

	if ( !GlobalEventProperties( &params ) )
		return;

	if ( strlen( params.m_szName ) <= 0 )
	{
		Con_Printf( "Pause section event must have a valid name\n" );
		return;
	}

	if ( strlen( params.m_szAction ) <= 0 )
	{
		Con_Printf( "No action specified for section pause\n" );
		return;
	}

	char undotext[ 256 ];
	undotext[0]=0;
	switch( type )
	{
	default:
		Assert( 0 );
		break;
	case CChoreoEvent::SECTION:
		{
			Q_strcpy( undotext, "Add Section Pause" );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			Q_strcpy( undotext, "Add Loop Point" );
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			Q_strcpy( undotext, "Add Loop Point" );
		}
		break;
	}

	SetDirty( true );

	PushUndo( undotext );

	CChoreoEvent *event = m_pScene->AllocEvent();
	if ( event )
	{
		event->SetType( type );
		event->SetName( params.m_szName );
		event->SetParameters( params.m_szAction );
		event->SetStartTime( params.m_flStartTime );
		event->SetEndTime( -1.0f );

		switch ( type )
		{
		default:
			break;
		case CChoreoEvent::LOOP:
			{
				event->SetLoopCount( params.m_nLoopCount );
				event->SetParameters( va( "%f", params.m_flLoopTime ) );
			}
			break;
		}

		event->SnapTimes();

		DeleteSceneWidgets();

		CreateSceneWidgets();

		// Redraw
		InvalidateLayout();
	}

	PushRedo( undotext );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::EditGlobalEvent( CChoreoEvent *event )
{
	if ( !event )
		return;

	CGlobalEventParams params;
	memset( &params, 0, sizeof( params ) );

	params.m_nType = event->GetType();

	switch ( event->GetType() )
	{
	default:
		Assert( 0 );
		strcpy( params.m_szDialogTitle, "???" );
		break;
	case CChoreoEvent::SECTION:
		{
			strcpy( params.m_szDialogTitle, "Edit Pause Point" );
			strcpy( params.m_szAction, event->GetParameters() );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			strcpy( params.m_szDialogTitle, "Edit Loop Point" );
			strcpy( params.m_szAction, "" );
			params.m_flLoopTime = (float)atof( event->GetParameters() );
			params.m_nLoopCount = event->GetLoopCount();
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			strcpy( params.m_szDialogTitle, "Edit Stop Point" );
			strcpy( params.m_szAction, "" );
		}
		break;
	}

	strcpy( params.m_szName, event->GetName() );

	params.m_flStartTime = event->GetStartTime();

	if ( !GlobalEventProperties( &params ) )
		return;

	if ( strlen( params.m_szName ) <= 0 )
	{
		Con_Printf( "Event %s must have a valid name\n", event->GetName() );
		return;
	}

	if ( strlen( params.m_szAction ) <= 0 )
	{
		Con_Printf( "No action specified for %s\n", event->GetName() );
		return;
	}

	SetDirty( true );

	char undotext[ 256 ];
	undotext[0]=0;
	switch( event->GetType() )
	{
	default:
		Assert( 0 );
		break;
	case CChoreoEvent::SECTION:
		{
			Q_strcpy( undotext, "Edit Section Pause" );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			Q_strcpy( undotext, "Edit Loop Point" );
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			Q_strcpy( undotext, "Edit Stop Point" );
		}
		break;
	}

	PushUndo( undotext );

	event->SetName( params.m_szName );
	event->SetStartTime( params.m_flStartTime );
	event->SetEndTime( -1.0f );

	switch ( event->GetType() )
	{
	default:
		{
			event->SetParameters( params.m_szAction );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			event->SetLoopCount( params.m_nLoopCount );
			event->SetParameters( va( "%f", params.m_flLoopTime ) );
		}
		break;
	}

	event->SnapTimes();

	PushRedo( undotext );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::DeleteGlobalEvent( CChoreoEvent *event )
{
	if ( !event || !m_pScene )
		return;

	SetDirty( true );


	char undotext[ 256 ];
	undotext[0]=0;
	switch( event->GetType() )
	{
	default:
		Assert( 0 );
		break;
	case CChoreoEvent::SECTION:
		{
			Q_strcpy( undotext, "Delete Section Pause" );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			Q_strcpy( undotext, "Delete Loop Point" );
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			Q_strcpy( undotext, "Delete Stop Point" );
		}
		break;
	}

	PushUndo( undotext );

	DeleteSceneWidgets();

	Con_Printf( "Deleting %s\n", event->GetName() );

	m_pScene->DeleteReferencedObjects( event );

	CreateSceneWidgets();

	PushRedo( undotext );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::EditEvent( CChoreoEvent *event )
{
	if ( !event )
		return;

	CEventParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Edit Event" );

	// Copy in current even properties
	params.m_nType = event->GetType();

	switch ( params.m_nType )
	{
		case CChoreoEvent::EXPRESSION:
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::SPEAK:
		case CChoreoEvent::GESTURE:
		case CChoreoEvent::INTERRUPT:
			strcpy( params.m_szParameters2, event->GetParameters2() );
			strcpy( params.m_szParameters, event->GetParameters() );
			strcpy( params.m_szName, event->GetName() );
			break;
		case CChoreoEvent::FACE:
		case CChoreoEvent::MOVETO:
		case CChoreoEvent::LOOKAT:
		case CChoreoEvent::FIRETRIGGER:
		case CChoreoEvent::FLEXANIMATION:
		case CChoreoEvent::SUBSCENE:
			strcpy( params.m_szParameters, event->GetParameters() );
			strcpy( params.m_szName, event->GetName() );

			if ( params.m_nType == CChoreoEvent::LOOKAT )
			{
				if ( event->GetPitch() != 0 ||
					 event->GetYaw() != 0 )
				{
					params.usepitchyaw = true;
					params.pitch = event->GetPitch();
					params.yaw = event->GetYaw();
				}
			}
			break;
		default:
			Con_Printf( "Don't know how to edit event type %s\n",
				CChoreoEvent::NameForType( (CChoreoEvent::EVENTTYPE)params.m_nType ) );
			return;
	}

	params.m_pScene = m_pScene;
	params.m_pEvent = event;
	params.m_flStartTime = event->GetStartTime();
	params.m_flEndTime = event->GetEndTime();
	params.m_bHasEndTime = event->HasEndTime();

	params.m_bFixedLength = event->IsFixedLength();
	params.m_bResumeCondition = event->IsResumeCondition();
	params.m_bUsesTag = event->IsUsingRelativeTag();
	if ( params.m_bUsesTag )
	{
		strcpy( params.m_szTagName, event->GetRelativeTagName() );
		strcpy( params.m_szTagWav, event->GetRelativeWavName() );
	}

	while (1)
	{
		if (!EventProperties( &params ))
			return;

		if (strlen( params.m_szName ) <= 0 )
		{
			mxMessageBox( this, va( "Event %s must have a valid name", event->GetName() ),
				"AddEvent", MX_MB_OK | MX_MB_ERROR );
		}
		else if ( ( params.m_nType != CChoreoEvent::FLEXANIMATION ) && 
				( params.m_nType  != CChoreoEvent::INTERRUPT ) && 
				( strlen( params.m_szParameters ) <= 0 ) )
		{
			mxMessageBox( this, va( "No parameters specified for %s\n", params.m_szName ),
				"AddEvent", MX_MB_OK | MX_MB_ERROR );
		}
		else
		{
			break;
		}
	}


	SetDirty( true );

	PushUndo( "Edit Event" );

	event->SetName( params.m_szName );
	event->SetParameters( params.m_szParameters );
	event->SetParameters2( params.m_szParameters2 );
	event->SetStartTime( params.m_flStartTime );
	event->SetResumeCondition( params.m_bResumeCondition );
	if ( params.m_bUsesTag )
	{
		event->SetUsingRelativeTag( true, params.m_szTagName, params.m_szTagWav );
	}
	else
	{
		event->SetUsingRelativeTag( false );
	}

	if ( params.m_bHasEndTime &&
		params.m_flEndTime != -1.0 &&
		params.m_flEndTime > params.m_flStartTime )
	{
		float dt = params.m_flEndTime - event->GetEndTime();
		float newduration = event->GetDuration() + dt;
		RescaleRamp( event, newduration );
		switch ( event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::GESTURE:
			{
				RescaleGestureTimes( event, event->GetStartTime(), event->GetEndTime() + dt );
			}
			break;
		case CChoreoEvent::FLEXANIMATION:
			{
				RescaleExpressionTimes( event, event->GetStartTime(), event->GetEndTime() + dt );
			}
			break;
		}
		event->SetEndTime( params.m_flEndTime );
		event->SnapTimes();
		event->ResortRamp();
	}
	else
	{
		event->SetEndTime( -1.0f );
	}

	switch ( event->GetType() )
	{
	default:
		break;
	case CChoreoEvent::SPEAK:
		{
			// Try and load wav to get length
			CAudioSource *wave = sound->LoadSound( va( "sound/%s", FacePoser_TranslateSoundName( event->GetParameters() ) ) );
			if ( wave )
			{
				event->SetEndTime( params.m_flStartTime + wave->GetRunningLength() );
				delete wave;
			}
		}
		break;
	case CChoreoEvent::SUBSCENE:
		{
			// Just grab end time
			CChoreoScene *scene = LoadScene( event->GetParameters() );
			if ( scene )
			{
				event->SetEndTime( params.m_flStartTime + scene->FindStopTime() );
			}
			delete scene;
		}
		break;
	case CChoreoEvent::SEQUENCE:
		{
			CheckSequenceLength( event, false );
		}
		break;
	case CChoreoEvent::GESTURE:
		{
			CheckGestureLength( event, false );
		}
		break;
	case CChoreoEvent::LOOKAT:
		{
			if ( params.usepitchyaw )
			{
				event->SetPitch( params.pitch );
				event->SetYaw( params.yaw );
			}
			else
			{
				event->SetPitch( 0 );
				event->SetYaw( 0 );
			}
		}
		break;
	}

	event->SnapTimes();

	PushRedo( "Edit Event" );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::DeleteSelectedEvents( void )
{
	if ( !m_pScene )
		return;

	SetDirty( true );

	PushUndo( "Delete Events" );

	int deleteCount = 0;

	// Find the appropriate event by iterating across all actors and channels
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *channel = a->GetChannel( j );
			if ( !channel )
				continue;

			for ( int k = channel->GetNumEvents() - 1; k >= 0; k-- )
			{
				CChoreoEventWidget *event = channel->GetEvent( k );
				if ( !event->IsSelected() )
					continue;

				channel->GetChannel()->RemoveEvent( event->GetEvent() );
				m_pScene->DeleteReferencedObjects( event->GetEvent() );

				deleteCount++;
			}
		}
	}

	DeleteSceneWidgets();

	ReportSceneClearToTools();

	CreateSceneWidgets();

	PushRedo( "Delete Events" );

	Con_Printf( "Deleted <%i> events\n", deleteCount );

	// Redraw
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CChoreoView::LoadScene( char const *filename )
{
	// Strip out the game directory
	char relative[ 512 ];
	strcpy( relative,filename );
	if ( strchr( filename, ':' ) )
	{
		filesystem->FullPathToRelativePath( filename, relative );
	}

	if ( FPFullpathFileExists( va( "%s/%s", GetGameDirectory(), (char *)relative ) ) )
	{
		LoadScriptFile( va( "%s/%s", GetGameDirectory(), (char *)relative ) );

		CChoreoScene *scene = ChoreoLoadScene( this, &g_TokenProcessor, Con_Printf );
		return scene;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChoreoView::FixupSequenceDurations( CChoreoScene *scene, bool checkonly )
{
	bool bret = false;
	if ( !scene )
		return bret;

	int c = scene->GetNumEvents();
	for ( int i = 0; i < c; i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		switch ( event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SEQUENCE:
			{
				if ( CheckSequenceLength( event, checkonly ) )
				{
					bret = true;
				}
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				if ( CheckGestureLength( event, checkonly ) )
				{
					bret = true;
				}
			}
			break;
		}
	}

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: IChoreoEventCallback
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoView::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
	{
		return;
	}

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
	{
		return;
	}

	// Con_Printf( "%8.4f:  start %s\n", currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
		{
			ProcessSequence( scene, event );
		}
		break;
	case CChoreoEvent::SUBSCENE:
		{
			if ( !scene->IsSubScene() )
			{
				CChoreoScene *subscene = event->GetSubScene();
				if ( !subscene )
				{
					subscene = LoadScene( event->GetParameters() );
					subscene->SetSubScene( true );
					event->SetSubScene( subscene );
				}

				if ( subscene )
				{
					subscene->ResetSimulation( m_bForward );
				}
			}
		}
		break;
	case CChoreoEvent::SECTION:
		{
			ProcessPause( scene, event );
		}
		break;
	case CChoreoEvent::SPEAK:
		{
			if ( ShouldProcessSpeak() )
			{
				StudioModel *model = FindAssociatedModel( scene, event->GetActor() );

				CAudioMixer *mixer = event->GetMixer();
				if ( !mixer || !sound->IsSoundPlaying( mixer ) )
				{
					sound->PlaySound( 
						model, 
						va( "sound/%s", FacePoser_TranslateSoundName( event->GetParameters() ) ),
						&mixer );
					event->SetMixer( mixer );
				}

				if ( mixer )
				{
					mixer->SetDirection( m_flFrameTime >= 0.0f );
					float starttime, endtime;
					starttime = event->GetStartTime();
					endtime = event->GetEndTime();

					float soundtime = endtime - starttime;
					if ( soundtime > 0.0f )
					{
						float f = ( currenttime - starttime ) / soundtime;
						f = clamp( f, 0.0f, 1.0f );

						// Compute sample
						float numsamples = (float)mixer->GetSource()->SampleCount();

						int cursample = f * numsamples;
						cursample = clamp( cursample, 0, numsamples - 1 );
						mixer->SetSamplePosition( cursample );
						mixer->SetActive( true );
					}
				}
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			// Get a new markov group id each time we start a meta-event
			CExpClass *p = expressions->FindClass( event->GetParameters() );
			if ( p )
			{
				CExpression *exp = p->FindExpression( event->GetParameters2() );

				// Allow overrides
				if ( FacePoser_GetOverridesShowing() )
				{
					if ( exp && exp->HasOverride() && exp->GetOverride() )
					{
						exp = exp->GetOverride();
					}
				}

				if ( exp && exp->GetType() == CExpression::TYPE_MARKOV )
				{
					exp->NewMarkovIndex();
				}
			}
		}
		break;
	case CChoreoEvent::LOOP:
		{
			ProcessLoop( scene, event );
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			// Nothing, this is a symbolic event for keeping the vcd alive for ramping out after the last true event
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoView::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
	{
		return;
	}

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
	{
		return;
	}

	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
			CChoreoScene *subscene = event->GetSubScene();
			if ( subscene )
			{
				subscene->ResetSimulation();
			}
		}
		break;
	case CChoreoEvent::SPEAK:
		{
			CAudioMixer *mixer = event->GetMixer();
			if ( mixer && sound->IsSoundPlaying( mixer ) )
			{
				sound->StopSound( mixer );
			}
			event->SetMixer( NULL );
		}
		break;
	default:
		break;
	}

//	Con_Printf( "%8.4f:  finish %s\n", currenttime, event->GetDescription() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//			mx - 
//			my - 
//-----------------------------------------------------------------------------
int CChoreoView::GetTagUnderCursorPos( CChoreoEventWidget *event, int mx, int my )
{
	if ( !event )
	{
		return -1;
	}

	for ( int i = 0; i < event->GetEvent()->GetNumRelativeTags(); i++ )
	{
		CEventRelativeTag *tag = event->GetEvent()->GetRelativeTag( i );
		if ( !tag )
			continue;

		// Determine left edcge
		RECT bounds;
		bounds = event->getBounds();
		int left = bounds.left + (int)( tag->GetPercentage() * (float)( bounds.right - bounds.left ) + 0.5f );

		int tolerance = 3;

		if ( abs( mx - left ) < tolerance )
		{
			if ( abs( my - bounds.top ) < tolerance )
			{
				return i;
			}
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//			**actor - 
//			**channel - 
//			**event - 
//-----------------------------------------------------------------------------
void CChoreoView::GetObjectsUnderMouse( int mx, int my, CChoreoActorWidget **actor,
	CChoreoChannelWidget **channel, CChoreoEventWidget **event, CChoreoGlobalEventWidget **globalevent,
	int *clickedTag )
{
	if ( actor )
	{
		*actor = GetActorUnderCursorPos( mx, my );
	}
	if ( channel )
	{
		*channel = GetChannelUnderCursorPos( mx, my );
	}
	if ( event )
	{
		*event = GetEventUnderCursorPos( mx, my );
	}
	if ( globalevent )
	{
		*globalevent = GetGlobalEventUnderCursorPos( mx, my );
	}
	if ( clickedTag )
	{
		if ( event && *event )
		{
			*clickedTag = GetTagUnderCursorPos( *event, mx, my );
		}
		else
		{
			*clickedTag = -1;
		}
	}

	m_nSelectedEvents = CountSelectedEvents();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
// Output : CChoreoGlobalEventWidget
//-----------------------------------------------------------------------------
CChoreoGlobalEventWidget *CChoreoView::GetGlobalEventUnderCursorPos( int mx, int my )
{
	POINT check;
	check.x = mx;
	check.y = my;

	CChoreoGlobalEventWidget *event;
	for ( int i = 0; i < m_SceneGlobalEvents.Size(); i++ )
	{
		event = m_SceneGlobalEvents[ i ];
		if ( !event )
			continue;

		RECT bounds;
		event->getBounds( bounds );

		if ( PtInRect( &bounds, check ) )
		{
			return event;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Caller must first translate mouse into screen coordinates
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
CChoreoActorWidget *CChoreoView::GetActorUnderCursorPos( int mx, int my )
{
	POINT check;
	check.x = mx;
	check.y = my;

	CChoreoActorWidget *actor;
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		RECT bounds;
		actor->getBounds( bounds );

		if ( PtInRect( &bounds, check ) )
		{
			return actor;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Caller must first translate mouse into screen coordinates
// Input  : mx - 
//			my - 
// Output : CChoreoChannelWidget
//-----------------------------------------------------------------------------
CChoreoChannelWidget *CChoreoView::GetChannelUnderCursorPos( int mx, int my )
{
	CChoreoActorWidget *actor = GetActorUnderCursorPos( mx, my );
	if ( !actor )
	{
		return NULL;
	}

	POINT check;
	check.x = mx;
	check.y = my;

	CChoreoChannelWidget *channel;
	for ( int i = 0; i < actor->GetNumChannels(); i++ )
	{
		channel = actor->GetChannel( i );
		if ( !channel )
			continue;

		RECT bounds;
		channel->getBounds( bounds );

		if ( PtInRect( &bounds, check ) )
		{
			return channel;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Caller must first translate mouse into screen coordinates
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
CChoreoEventWidget *CChoreoView::GetEventUnderCursorPos( int mx, int my )
{
	CChoreoChannelWidget *channel = GetChannelUnderCursorPos( mx, my );
	if ( !channel )
	{
		return NULL;
	}

	POINT check;
	check.x = mx;
	check.y = my;

	CChoreoEventWidget *event;
	for ( int i = 0; i < channel->GetNumEvents(); i++ )
	{
		event = channel->GetEvent( i );
		if ( !event )
			continue;

		RECT bounds;
		event->getBounds( bounds );

		// Events get an expanded border
		InflateRect( &bounds, 8, 4 );

		if ( PtInRect( &bounds, check ) )
		{
			return event;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Select wave file for phoneme editing
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CChoreoView::SetCurrentWaveFile( const char *filename, CChoreoEvent *event )
{
	g_pPhonemeEditor->SetCurrentWaveFile( filename, false, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pfn - 
//			*param1 - 
//-----------------------------------------------------------------------------
void CChoreoView::TraverseWidgets( CVMEMBERFUNC pfn, CChoreoWidget *param1 )
{
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *actor = m_SceneActors[ i ];
		if ( !actor )
			continue;

		(this->*pfn)( actor, param1 );

		for ( int j = 0; j < actor->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *channel = actor->GetChannel( j );
			if ( !channel )
				continue;

			(this->*pfn)( channel, param1 );

			for ( int k = 0; k < channel->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *event = channel->GetEvent( k );
				if ( !event )
					continue;

				(this->*pfn)( event, param1 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *widget - 
//			*param1 - 
//-----------------------------------------------------------------------------
void CChoreoView::Deselect( CChoreoWidget *widget, CChoreoWidget *param1 )
{
	if ( widget->IsSelected() )
	{
		widget->SetSelected( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *widget - 
//			*param1 - 
//-----------------------------------------------------------------------------
void CChoreoView::Select( CChoreoWidget *widget, CChoreoWidget *param1 )
{
	if ( widget != param1 )
		return;

	if ( widget->IsSelected() )
		return;

	widget->SetSelected( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *widget - 
//			*param1 - 
//-----------------------------------------------------------------------------
void CChoreoView::SelectAllEvents( CChoreoWidget *widget, CChoreoWidget *param1 )
{
	CChoreoEventWidget *ew = dynamic_cast< CChoreoEventWidget * >( widget );
	if ( !ew )
		return;

	if ( widget->IsSelected() )
		return;

	widget->SetSelected( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//			*cl - 
//			*exp - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::CreateExpressionEvent( int mx, int my, CExpClass *cl, CExpression *exp )
{
	if ( !m_pScene )
		return false;

	if ( !exp )
		return false;

	// Convert screen to client
	POINT pt;
	pt.x = mx;
	pt.y = my;

	ScreenToClient( (HWND)getHandle(), &pt );

	if ( pt.x < 0 || pt.y < 0 )
		return false;

	if ( pt.x > w2() || pt.y > h2() )
		return false;

	// Find channel actor and time ( uses screen space coordinates )
	//
	CChoreoChannelWidget *channel = GetChannelUnderCursorPos( pt.x, pt.y );
	if ( !channel )
	{
		CChoreoActorWidget *actor = GetActorUnderCursorPos( pt.x, pt.y );
		if ( !actor )
			return false;

		// Grab first channel
		if ( !actor->GetNumChannels() )
			return false;

		channel = actor->GetChannel( 0 );
	}

	if ( !channel )
		return false;

	CChoreoChannel *pchannel = channel->GetChannel();
	if ( !pchannel )
	{
		Assert( 0 );
		return false;
	}

	CChoreoEvent *event = m_pScene->AllocEvent();
	if ( !event )
	{
		Assert( 0 );
		return false;
	}

	float starttime = GetTimeValueForMouse( pt.x, false );

	SetDirty( true );

	PushUndo( "Create Expression" );

	event->SetType( CChoreoEvent::EXPRESSION );
	event->SetName( exp->name );
	event->SetParameters( cl->GetName() );
	event->SetParameters2( exp->name );
	event->SetStartTime( starttime );
	event->SetChannel( pchannel );
	event->SetActor( pchannel->GetActor() );
	event->SetEndTime( starttime + 1.0f );

	event->SnapTimes();

	DeleteSceneWidgets();

	// Add to appropriate channel
	pchannel->AddEvent( event );

	CreateSceneWidgets();

	PushRedo( "Create Expression" );

	// Redraw
	InvalidateLayout();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::IsPlayingScene( void )
{
	return m_bSimulating;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::ResetTargetSettings( void )
{
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *w = m_SceneActors[ i ];
		if ( w )
		{
			w->ResetSettings();
		}
	}

	m_bHasLookTarget = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoView::UpdateCurrentSettings( void )
{
	StudioModel *defaultModel = models->GetActiveStudioModel();

	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *w = m_SceneActors[ i ];
		if ( !w )
			continue;

		if ( !w->GetActor()->GetActive() )
			continue;

		StudioModel *model = FindAssociatedModel( m_pScene, w->GetActor() );
		if ( !model )
			continue;

		studiohdr_t *hdr = model->getStudioHeader();
		if ( !hdr )
			continue;

		float *current = w->GetSettings();

		for ( int j = 0; j < hdr->numflexcontrollers; j++ )
		{
			int k = hdr->pFlexcontroller( j )->link;

			model->SetFlexController( j, current[ k ] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//			tagnum - 
//-----------------------------------------------------------------------------
void CChoreoView::DeleteEventRelativeTag( CChoreoEvent *event, int tagnum )
{
	if ( !event )
		return;

	CEventRelativeTag *tag = event->GetRelativeTag( tagnum );
	if ( !tag )
		return;

	SetDirty( true );

	PushUndo( "Delete Event Tag" );

	event->RemoveRelativeTag( tag->GetName() );

	m_pScene->ReconcileTags();

	PushRedo( "Delete Event Tag" );

	g_pPhonemeEditor->redraw();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::AddEventRelativeTag( void )
{
	CChoreoEventWidget *ew = m_pClickedEvent;
	if ( !ew )
		return;

	CChoreoEvent *event = ew->GetEvent();
	if ( !event->GetEndTime() )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Event Tag:  Can only tag events with an end time\n" );
		return;
	}

	CInputParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Event Tag Name" );
	strcpy( params.m_szPrompt, "Name:" );

	strcpy( params.m_szInputText, "" );

	if ( !InputProperties( &params ) )
		return;

	if ( strlen( params.m_szInputText ) <= 0 )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Event Tag Name:  No name entered!\n" );
		return;
	}
	
	RECT bounds = ew->getBounds();

	// Convert click to frac
	float frac = 0.0f;
	if ( bounds.right - bounds.left > 0 )
	{
		frac = (float)( m_nClickedX - bounds.left ) / (float)( bounds.right - bounds.left );
		frac = min( 1.0f, frac );
		frac = max( 0.0f, frac );
	}

	SetDirty( true );

	PushUndo( "Add Event Tag" );

	event->AddRelativeTag( params.m_szInputText, frac );

	PushRedo( "Add Event Tag" );

	InvalidateLayout();
	g_pPhonemeEditor->redraw();
	g_pExpressionTool->redraw();
	g_pGestureTool->redraw();
	g_pRampTool->redraw();
	g_pSceneRampTool->redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
// Output : CChoreoEventWidget
//-----------------------------------------------------------------------------
CChoreoEventWidget *CChoreoView::FindWidgetForEvent( CChoreoEvent *event )
{
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *c = a->GetChannel( j );
			if ( !c )
				continue;

			for ( int k = 0; k < c->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( e->GetEvent() != event )
					continue;

				return e;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::SelectAll( void )
{
	TraverseWidgets( SelectAllEvents, NULL );
	redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::DeselectAll( void )
{
	TraverseWidgets( Deselect, NULL );
	redraw();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CChoreoView::UpdateStatusArea( int mx, int my )
{
	FLYOVER fo;

	GetObjectsUnderMouse( mx, my, &fo.a, &fo.c, 
		&fo.e, &fo.ge, &fo.tag );

	if ( fo.a )
	{
		m_Flyover.a = fo.a;
	}
	if ( fo.e )
	{
		m_Flyover.e = fo.e;
	}
	if ( fo.c )
	{
		m_Flyover.c = fo.c;
	}
	if ( fo.ge )
	{
		m_Flyover.ge = fo.ge;
	}
	if ( fo.tag != -1 )
	{
		m_Flyover.tag = fo.tag;
	}

	RECT rcClip;
	GetClientRect( (HWND)getHandle(), &rcClip );
	rcClip.bottom -= m_nScrollbarHeight;
	rcClip.top = rcClip.bottom - m_nInfoHeight;
	rcClip.right -= m_nScrollbarHeight;

	CChoreoWidgetDrawHelper drawHelper( this, rcClip, COLOR_CHOREO_BACKGROUND );

	drawHelper.StartClipping( rcClip );

	RedrawStatusArea( drawHelper, rcClip );

	drawHelper.StopClipping();
	ValidateRect( (HWND)getHandle(), &rcClip );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::ClearStatusArea( void )
{
	memset( &m_Flyover, 0, sizeof( m_Flyover ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			rcStatus - 
//-----------------------------------------------------------------------------
void CChoreoView::RedrawStatusArea( CChoreoWidgetDrawHelper& drawHelper, RECT& rcStatus )
{
	drawHelper.DrawFilledRect( COLOR_CHOREO_BACKGROUND, rcStatus );

	drawHelper.DrawColoredLine( COLOR_INFO_BORDER, PS_SOLID, 1, rcStatus.left, rcStatus.top,
		rcStatus.right, rcStatus.top );

	RECT rcInfo = rcStatus;

	rcInfo.top += 2;

	if ( m_Flyover.e )
	{
		m_Flyover.e->redrawStatus( drawHelper, rcInfo );
	}

	if ( m_pScene )
	{
		char sz[ 512 ];

		int fontsize = 9;
		int fontweight = FW_NORMAL;

		RECT rcText;
		rcText = rcInfo;
		rcText.bottom = rcText.top + fontsize + 2;

		char const *mapname = m_pScene->GetMapname();
		if ( mapname )
		{
			sprintf( sz, "Associated .bsp:  %s", mapname[ 0 ] ? mapname : "none" );

			int len = drawHelper.CalcTextWidth( "Arial", fontsize, fontweight, sz );
			rcText.left = rcText.right - len - 10;

			drawHelper.DrawColoredText( "Arial", fontsize, fontweight, COLOR_INFO_TEXT, rcText, sz );

			OffsetRect( &rcText, 0, fontsize + 2 );
		}

		sprintf( sz, "Scene:  %s", GetChoreoFile() );

		int len = drawHelper.CalcTextWidth( "Arial", fontsize, fontweight, sz );
		rcText.left = rcText.right - len - 10;

		drawHelper.DrawColoredText( "Arial", fontsize, fontweight, COLOR_INFO_TEXT, rcText, sz );
	}

//	drawHelper.DrawColoredText( "Arial", 12, 500, RGB( 0, 0, 0 ), rcInfo, m_Flyover.e ? m_Flyover.e->GetEvent()->GetName() : "" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoView::MoveEventToBack( CChoreoEvent *event )
{
	// Now find channel widget
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *c = a->GetChannel( j );
			if ( !c )
				continue;

			for ( int k = 0; k < c->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( event == e->GetEvent() )
				{
					// Move it to back of channel's list
					c->MoveEventToTail( e );
					InvalidateLayout();
					return;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoView::GetEndRow( void )
{
	RECT rcClient;
	GetClientRect( (HWND)getHandle(), &rcClient );

	return rcClient.bottom - ( m_nInfoHeight + m_nScrollbarHeight );
}

// Undo/Redo
void CChoreoView::Undo( void )
{
	if ( m_UndoStack.Size() > 0 && m_nUndoLevel > 0 )
	{
		m_nUndoLevel--;
		CVUndo *u = m_UndoStack[ m_nUndoLevel ];
		Assert( u->undo );

		DeleteSceneWidgets();

		*m_pScene = *(u->undo);
		g_MDLViewer->InitGridSettings();

		CreateSceneWidgets();

		ReportSceneClearToTools();
		ClearStatusArea();
		m_pClickedActor = NULL;
		m_pClickedChannel = NULL;
		m_pClickedEvent = NULL;
		m_pClickedGlobalEvent = NULL; 
	}

	InvalidateLayout();
}

void CChoreoView::Redo( void )
{
	if ( m_UndoStack.Size() > 0 && m_nUndoLevel <= m_UndoStack.Size() - 1 )
	{
		CVUndo *u = m_UndoStack[ m_nUndoLevel ];
		Assert( u->redo );

		DeleteSceneWidgets();

		*m_pScene = *(u->redo);
		g_MDLViewer->InitGridSettings();

		CreateSceneWidgets();

		ReportSceneClearToTools();
		ClearStatusArea();
		m_pClickedActor = NULL;
		m_pClickedChannel = NULL;
		m_pClickedEvent = NULL;
		m_pClickedGlobalEvent = NULL; 

		m_nUndoLevel++;
	}

	InvalidateLayout();
}

static char *CopyString( const char *in )
{
	int len = strlen( in );
	char *n = new char[ len + 1 ];
	strcpy( n, in );
	return n;
}

void CChoreoView::PushUndo( char *description )
{
	Assert( !m_bRedoPending );
	m_bRedoPending = true;
	WipeRedo();

	// Copy current data
	CChoreoScene *u = new CChoreoScene( this );
	*u = *m_pScene;
	CVUndo *undo = new CVUndo;
	undo->undo = u;
	undo->redo = NULL;
	undo->udescription = CopyString( description );
	undo->rdescription = NULL;
	m_UndoStack.AddToTail( undo );
	m_nUndoLevel++;
}

void CChoreoView::PushRedo( char *description )
{
	Assert( m_bRedoPending );
	m_bRedoPending = false;

	// Copy current data
	CChoreoScene *r = new CChoreoScene( this );
	*r = *m_pScene;
	CVUndo *undo = m_UndoStack[ m_nUndoLevel - 1 ];
	undo->redo = r;
	undo->rdescription = CopyString( description );

	// Always redo here to reflect that someone has made a change
	redraw();
}

void CChoreoView::WipeUndo( void )
{
	while ( m_UndoStack.Size() > 0 )
	{
		CVUndo *u = m_UndoStack[ 0 ];
		delete u->undo;
		delete u->redo;
		delete[] u->udescription;
		delete[] u->rdescription;
		delete u;
		m_UndoStack.Remove( 0 );
	}
	m_nUndoLevel = 0;
}

void CChoreoView::WipeRedo( void )
{
	// Wipe everything above level
	while ( m_UndoStack.Size() > m_nUndoLevel )
	{
		CVUndo *u = m_UndoStack[ m_nUndoLevel ];
		delete u->undo;
		delete u->redo;
		delete[] u->udescription;
		delete[] u->rdescription;
		delete u;
		m_UndoStack.Remove( m_nUndoLevel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoView::GetUndoDescription( void )
{
	if ( m_nUndoLevel != 0 )
	{
		CVUndo *u = m_UndoStack[ m_nUndoLevel - 1 ];
		return u->udescription;
	}
	return "???undo";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoView::GetRedoDescription( void )
{
	if ( m_nUndoLevel != m_UndoStack.Size() )
	{
		CVUndo *u = m_UndoStack[ m_nUndoLevel ];
		return u->rdescription;
	}
	return "???redo";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::OnGestureTool( void )
{
	if ( m_pClickedEvent->GetEvent()->GetType() != CChoreoEvent::GESTURE )
		return;

	g_pGestureTool->SetEvent( m_pClickedEvent->GetEvent() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoView::OnExpressionTool( void )
{
	if ( m_pClickedEvent->GetEvent()->GetType() != CChoreoEvent::FLEXANIMATION )
		return;

	g_pExpressionTool->SetEvent( m_pClickedEvent->GetEvent() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CChoreoView::GetScene( void )
{
	return m_pScene;
}

bool CChoreoView::CanPaste( void )
{
	char const *copyfile = "scenes/copydata.txt";

	if ( !filesystem->FileExists( copyfile ) )
	{
		return false;
	}

	return true;
}

void CChoreoView::CopyEvents( void )
{
	if ( !m_pScene )
		return;

	char const *copyfile = "scenes/copydata.txt";

	CUtlVector< CChoreoEvent * > events;

	// Find selected eventss
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *c = a->GetChannel( j );
			if ( !c )
				continue;

			for ( int k = 0; k < c->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( !e->IsSelected() )
					continue;

				CChoreoEvent *event = e->GetEvent();
				if ( !event )
					continue;

				events.AddToTail( event );
			}
		}
	}

	if ( events.Size() > 0 )
	{
		m_pScene->ExportEvents( copyfile, events );
	}
	else
	{
		Con_Printf( "No events selected\n" );
	}
}

void CChoreoView::PasteEvents( void )
{
	if ( !m_pScene )
		return;

	if ( !m_pClickedActor || !m_pClickedChannel )
		return;

	if ( !CanPaste() )
		return;

	char const *copyfile = va( "%s/scenes/copydata.txt", GetGameDirectory() );

	LoadScriptFile( (char *)copyfile );

	DeselectAll();

	float starttime = GetTimeValueForMouse( m_nClickedX, false );

	SetDirty( true );

	PushUndo( "Paste Events" );

	m_pScene->ImportEvents( &g_TokenProcessor, m_pClickedActor->GetActor(), m_pClickedChannel->GetChannel(), starttime );

	PushRedo( "Paste Events" );

	CreateSceneWidgets();
	// Redraw
	InvalidateLayout();
}

void CChoreoView::ImportEvents( void )
{
	if ( !m_pScene )
		return;

	if ( !m_pClickedActor || !m_pClickedChannel )
		return;

	// Show file io
	const char *fullpath = mxGetOpenFileName( 
		0, 
		FacePoser_MakeWindowsSlashes( va( "%s/scenes/", GetGameDirectory() ) ), 
		"*.vce" );
	if ( !fullpath || !fullpath[ 0 ] )
		return;
	if ( !FPFullpathFileExists( fullpath ) )
		return;

	LoadScriptFile( (char *)fullpath );

	DeselectAll();

	float starttime = GetTimeValueForMouse( m_nClickedX, false );

	SetDirty( true );

	PushUndo( "Import Events" );

	m_pScene->ImportEvents( &g_TokenProcessor, m_pClickedActor->GetActor(), m_pClickedChannel->GetChannel(), starttime );

	PushRedo( "Import Events" );

	CreateSceneWidgets();
	// Redraw
	InvalidateLayout();

	Con_Printf( "Imported events from %s\n", fullpath );
}

void CChoreoView::ExportEvents( void )
{
	char workingdir[ 256 ];
	Q_getwd( workingdir );
	strlwr( workingdir );
	COM_FixSlashes( workingdir );

	// Show file io
	bool inScenesAlready = false;
	if ( strstr( workingdir, va( "%s/scenes", GetGameDirectory() ) ) )
	{
		inScenesAlready = true;
	}
	const char *fullpath = mxGetSaveFileName( 
		this, 
		inScenesAlready ? "." : FacePoser_MakeWindowsSlashes( va( "%s/scenes/", GetGameDirectory() ) ), 
		"*.vce" );
	if ( !fullpath || !fullpath[ 0 ] )
		return;

	// Strip game directory and slash
	char eventfilename[ 512 ];

	filesystem->FullPathToRelativePath( fullpath, eventfilename );

	StripExtension( eventfilename );
	DefaultExtension( eventfilename, ".vce" );

	Con_Printf( "Exporting events to %s\n", eventfilename );

	// Write to file
	CUtlVector< CChoreoEvent * > events;

	// Find selected eventss
	for ( int i = 0; i < m_SceneActors.Size(); i++ )
	{
		CChoreoActorWidget *a = m_SceneActors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannelWidget *c = a->GetChannel( j );
			if ( !c )
				continue;

			for ( int k = 0; k < c->GetNumEvents(); k++ )
			{
				CChoreoEventWidget *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( !e->IsSelected() )
					continue;

				CChoreoEvent *event = e->GetEvent();
				if ( !event )
					continue;

				events.AddToTail( event );
			}
		}
	}

	if ( events.Size() > 0 )
	{
		m_pScene->ExportEvents( eventfilename, events );
	}
	else
	{
		Con_Printf( "No events selected\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::IsProcessing( void )
{
	if ( !m_pScene )
		return false;

	if ( m_flScrub != m_flScrubTarget )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::ShouldProcessSpeak( void )
{
	if ( !g_pControlPanel->AllToolsDriveSpeech() )
	{
		if ( !IsActiveTool() )
			return false;
	}

	if ( IFacePoserToolWindow::IsAnyToolScrubbing() )
		return true;

	if ( IFacePoserToolWindow::IsAnyToolProcessing() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoView::ProcessSpeak( CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !ShouldProcessSpeak() )
		return;

	Assert( event->GetType() == CChoreoEvent::SPEAK );
	Assert( scene );

	float t = scene->GetTime();

	StudioModel *model = FindAssociatedModel( scene, event->GetActor() );

	CAudioMixer *mixer = event->GetMixer();
	if ( !mixer || !sound->IsSoundPlaying( mixer ) )
	{
		sound->PlaySound( 
			model, 
			va( "sound/%s", FacePoser_TranslateSoundName( event->GetParameters() ) ), 
			&mixer );
		event->SetMixer( mixer );
	}

	mixer = event->GetMixer();
	if ( !mixer )
		return;

	mixer->SetDirection( m_flFrameTime >= 0.0f );
	float starttime, endtime;
	starttime = event->GetStartTime();
	endtime = event->GetEndTime();

	float soundtime = endtime - starttime;
	if ( soundtime <= 0.0f )
		return;

	float f = ( t - starttime ) / soundtime;
	f = clamp( f, 0.0f, 1.0f );

	// Compute sample
	float numsamples = (float)mixer->GetSource()->SampleCount();

	int cursample = f * numsamples;
	cursample = clamp( cursample, 0, numsamples - 1 );

	int realsample = mixer->GetSamplePosition();

	int dsample = cursample - realsample;

	int onepercent = numsamples * 0.01f;

	if ( abs( dsample ) > onepercent )
	{
		mixer->SetSamplePosition( cursample );
	}
	mixer->SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CChoreoView::ProcessSubscene( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::SUBSCENE );

	CChoreoScene *subscene = event->GetSubScene();
	if ( !subscene )
		return;

	if ( subscene->SimulationFinished() )
		return;
	
	// Have subscenes think for appropriate time
	subscene->Think( m_flScrub );
}

void CChoreoView::PositionControls()
{
	int topx = GetCaptionHeight() + SCRUBBER_HEIGHT;

	int bx = 2;
	int bw = 16;

	m_btnPlay->setBounds( bx, topx + 4, 16, 16 );

	bx += bw + 2;

	m_btnPause->setBounds( bx, topx + 4, 16, 16 );
	bx += bw + 2;
	m_btnStop->setBounds( bx, topx + 4, 16, 16 );
	bx += bw + 2;
	m_pPlaybackRate->setBounds( bx, topx + 4, 100, 16 );
}

void CChoreoView::SetChoreoFile( char const *filename )
{
	strcpy( m_szChoreoFile, filename );
	if ( m_szChoreoFile[ 0 ] )
	{
		SetSuffix( va( " - %s", m_szChoreoFile ) );
	}
	else
	{
		SetSuffix( "" );
	}
}

char const *CChoreoView::GetChoreoFile( void ) const
{
	return m_szChoreoFile;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rcHandle - 
//-----------------------------------------------------------------------------
void CChoreoView::GetScrubHandleRect( RECT& rcHandle, bool clipped )
{
	float pixel = 0.0f;

	float time = 0.0f;

	if ( m_pScene )
	{
		float currenttime = m_flScrub;
		float starttime = m_flStartTime;
		float endtime = m_flEndTime;

		float screenfrac = ( currenttime - starttime ) / ( endtime - starttime );

		pixel = GetLabelWidth() + screenfrac * ( w2() - GetLabelWidth() );

		if ( clipped )
		{
			pixel = clamp( pixel, SCRUBBER_HANDLE_WIDTH/2, w2() - SCRUBBER_HANDLE_WIDTH/2 );
		}
	}

	rcHandle.left = pixel-SCRUBBER_HANDLE_WIDTH/2;
	rcHandle.right = pixel + SCRUBBER_HANDLE_WIDTH/2;
	rcHandle.top = 2 + GetCaptionHeight();
	rcHandle.bottom = rcHandle.top + SCRUBBER_HANDLE_HEIGHT;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rcArea - 
//-----------------------------------------------------------------------------
void CChoreoView::GetScrubAreaRect( RECT& rcArea )
{
	rcArea.left = 0;
	rcArea.right = w2();
	rcArea.top = 2 + GetCaptionHeight();
	rcArea.bottom = rcArea.top + SCRUBBER_HEIGHT - 4;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			rcHandle - 
//-----------------------------------------------------------------------------
void CChoreoView::DrawScrubHandle( CChoreoWidgetDrawHelper& drawHelper )
{
	RECT rcHandle;
	GetScrubHandleRect( rcHandle, true );

	HBRUSH br = CreateSolidBrush( RGB( 0, 150, 100 ) );

	drawHelper.DrawFilledRect( br, rcHandle );

	// 
	char sz[ 32 ];
	sprintf( sz, "%.3f", m_flScrub );

	int len = drawHelper.CalcTextWidth( "Arial", 9, 500, sz );

	RECT rcText = rcHandle;
	int textw = rcText.right - rcText.left;

	rcText.left += ( textw - len ) / 2;

	drawHelper.DrawColoredText( "Arial", 9, 500, RGB( 255, 255, 255 ), rcText, sz );

	DeleteObject( br );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoView::IsMouseOverScrubHandle( mxEvent *event )
{
	RECT rcHandle;
	GetScrubHandleRect( rcHandle, true );
	InflateRect( &rcHandle, 2, 2 );

	POINT pt;
	pt.x = (short)event->x;
	pt.y = (short)event->y;
	if ( PtInRect( &rcHandle, pt ) )
	{
		return true;
	}
	return false;
}

bool CChoreoView::IsMouseOverScrubArea( mxEvent *event )
{
	RECT rcArea;
	GetScrubAreaRect( rcArea );

	InflateRect( &rcArea, 2, 2 );

	POINT pt;
	pt.x = (short)event->x;
	pt.y = (short)event->y;
	if ( PtInRect( &rcArea, pt ) )
	{
		return true;
	}

	return false;
}

bool CChoreoView::IsScrubbing( void ) const
{
	bool scrubbing = ( m_nDragType == DRAGTYPE_SCRUBBER ) ? true : false;
	return scrubbing;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoView::Think( float dt )
{
	bool scrubbing = IFacePoserToolWindow::IsAnyToolScrubbing();

	ScrubThink( dt, scrubbing, this );
}

static int lastthinkframe = -1;
void CChoreoView::ScrubThink( float dt, bool scrubbing, IFacePoserToolWindow *invoker )
{
	// Make sure we don't get called more than once per frame
	int thisframe = g_MDLViewer->GetCurrentFrame();
	if ( thisframe == lastthinkframe )
		return;

	lastthinkframe = thisframe;

	if ( !m_pScene )
		return;

	if ( m_flScrubTarget == m_flScrub && !scrubbing )
	{
		// Act like it's paused
		if ( IFacePoserToolWindow::ShouldAutoProcess() )
		{
			m_bSimulating = true;
			SceneThink( m_flScrub );
		}

		if ( m_bSimulating && !m_bPaused )
		{
			//FinishSimulation();
		}
		return;
	}

	// Make sure we're solving head turns during playback
	g_viewerSettings.solveHeadTurn = 1;

	if ( m_bPaused )
	{
		SceneThink( m_flScrub );
		return;
	}

	// Make sure phonemes are loaded
	FacePoser_EnsurePhonemesLoaded();

	if ( !m_bSimulating )
	{
		m_bSimulating = true;
	}

	float d = m_flScrubTarget - m_flScrub;
	int sign = d > 0.0f ? 1 : -1;

	float maxmove = dt * m_flPlaybackRate;

	float prevScrub = m_flScrub;

	if ( sign > 0 )
	{
		if ( d < maxmove )
		{
			m_flScrub = m_flScrubTarget;
		}
		else
		{
			m_flScrub += maxmove;
		}
	}
	else
	{
		if ( -d < maxmove )
		{
			m_flScrub = m_flScrubTarget;
		}
		else
		{
			m_flScrub -= maxmove;
		}
	}

	m_flFrameTime = ( m_flScrub - prevScrub );

	SceneThink( m_flScrub );

	DrawScrubHandle();

	if ( scrubbing )
	{
		g_pMatSysWindow->Frame();
	}

	if ( invoker != g_pExpressionTool )
	{
		g_pExpressionTool->ForceScrubPositionFromSceneTime( m_flScrub );
	}
	if ( invoker != g_pGestureTool )
	{
		g_pGestureTool->ForceScrubPositionFromSceneTime( m_flScrub );
	}
	if ( invoker != g_pRampTool )
	{
		g_pRampTool->ForceScrubPositionFromSceneTime( m_flScrub );
	}
	if ( invoker != g_pSceneRampTool )
	{
		g_pSceneRampTool->ForceScrubPositionFromSceneTime( m_flScrub );
	}
}

void CChoreoView::DrawScrubHandle( void )
{
	if ( !m_bCanDraw )
		return;

	// Handle new time and
	RECT rcArea;
	GetScrubAreaRect( rcArea );

	CChoreoWidgetDrawHelper drawHelper( this, rcArea, COLOR_CHOREO_BACKGROUND );
	DrawScrubHandle( drawHelper );
}

void CChoreoView::SetScrubTime( float t )
{
	m_flScrub = t;
}

void CChoreoView::SetScrubTargetTime( float t )
{
	m_flScrubTarget = t;
}

void CChoreoView::ClampTimeToSelectionInterval( float& timeval )
{
	// FIXME hook this up later
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : show - 
//-----------------------------------------------------------------------------
void CChoreoView::ShowButtons( bool show )
{
	m_btnPlay->setVisible( show );
	m_btnPause->setVisible( show );
	m_btnStop->setVisible( show );
	m_pPlaybackRate->setVisible( show );
}
	
void CChoreoView::OnChangeScale( void )
{
	// Zoom time in  / out
	CInputParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Change Zoom" );
	strcpy( params.m_szPrompt, "New scale (e.g., 2.5x):" );

	Q_snprintf( params.m_szInputText, sizeof( params.m_szInputText ), "%.2f", (float)m_nTimeZoom / 100.0f );

	if ( !InputProperties( &params ) )
		return;

	m_nTimeZoom = clamp( (int)( 100.0f * atof( params.m_szInputText ) ), 1, MAX_TIME_ZOOM );

	m_nLeftOffset = 0;
	m_nLastHPixelsNeeded = -1;
	m_nLastVPixelsNeeded = -1;
	DeleteSceneWidgets();
	CreateSceneWidgets();
	InvalidateLayout();
	Con_Printf( "Zoom factor %i %%\n", m_nTimeZoom );
}

void CChoreoView::OnCheckSequenceLengths( void )
{
	if ( !m_pScene )
		return;

	Con_Printf( "Checking sequence durations...\n" );

	bool changed = FixupSequenceDurations( m_pScene, true );

	if ( !changed )
	{
		Con_Printf( "   no changes...\n" );
		return;
	}

	SetDirty( true );

	PushUndo( "Check sequence lengths" );

	FixupSequenceDurations( m_pScene, false );

	PushRedo( "Check sequence lengths" );

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CChoreoView::InvalidateTrackLookup_R( CChoreoScene *scene )
{
	// No need to undo since this data doesn't matter
	int c = scene->GetNumEvents();
	for ( int i = 0; i < c; i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		switch ( event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::FLEXANIMATION:
			{
				event->SetTrackLookupSet( false );
			}
			break;
		case CChoreoEvent::SUBSCENE:
			{
				CChoreoScene *sub = event->GetSubScene();
				// NOTE:  Don't bother loading it now if it's not on hand
				if ( sub )
				{
					InvalidateTrackLookup_R( sub );
				}
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Model changed so we'll have to re-index flex anim tracks
//-----------------------------------------------------------------------------
void CChoreoView::InvalidateTrackLookup( void )
{
	if ( !m_pScene )
		return;

	InvalidateTrackLookup_R( m_pScene );
}

void CChoreoView::ResetHeadTarget()
{
	m_HeadTarget.RemoveAll();
}

void CChoreoView::UpdateHeadTarget()
{
	int c = m_HeadTarget.Count();
	if ( c <= 0 )
		return;

	float turn = 0.0f;
	Vector eye;
	Vector head;
	eye.Init();
	head.Init();
	int i;
	for ( i = 0; i < c; i++ )
	{

		HeadTarget *target = &m_HeadTarget[ i ];

		turn += target->intensity;
		eye += target->intensity * target->eyes;
		head += target->intensity * target->head;
	}

	g_viewerSettings.flHeadTurn = clamp( turn, 0.0f, 1.0f );

	g_viewerSettings.vecHeadTarget = head;
	g_viewerSettings.vecEyeTarget = eye;
}

void CChoreoView::AddHeadTarget( float intensity, const Vector& eyeTarget, const Vector& headTarget )
{
	HeadTarget data;
	data.intensity = intensity;
	data.eyes = eyeTarget;
	data.head = headTarget;

	m_HeadTarget.AddToTail( data );
}

bool CChoreoView::IsRampOnly( void ) const
{
	return m_bRampOnly;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoView::ProcessInterrupt( CChoreoScene *scene, CChoreoEvent *event )
{
}
