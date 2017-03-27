//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "glquake.h"
#include "vid.h"
#include "demo.h"

#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IVgui.h>
#include <KeyValues.h>

#include <vgui_controls/BuildGroup.h>
#include <vgui_controls/Tooltip.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/TextEntry.h>
#include <vgui/IInput.h>

#include "cl_demoactionmanager.h"

#include "filesystem.h"
#include "filesystem_engine.h"
#include "cl_DemoEditorPanel.h"
#include "cl_demosmootherpanel.h"
#include "cl_demouipanel.h"
#include "iprediction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CDemoUIPanel::CDemoUIPanel( vgui::Panel *parent ) : vgui::Frame( parent, "DemoUIPanel")
{
	int w = 440;
	int h = 170;

	int x = ( vid.width - w ) / 2;
	int y = ( vid.height - h - 50 );

	SetTitle("Demo Playback", true);

	m_pPlayPauseResume = new vgui::Button( this, "DemoPlayPauseResume", "PlayPauseResume" );
	m_pStop = new vgui::Button( this, "DemoStop", "Stop" );
	m_pLoad = new vgui::Button( this, "DemoLoad", "Load..." );
	m_pEdit = new vgui::Button( this, "DemoEdit", "Edit..." );
	m_pSmooth = new vgui::Button( this, "DemoSmooth", "Smooth..." );

	m_pTimeDemo = new vgui::CheckButton( this, "DemoTimeDemo", "Timedemo" );
	m_pFastForward = new vgui::Button( this, "DemoFastForward", "Fast Fwd" );
	m_pAdvanceFrame =  new vgui::Button( this, "DemoAdvanceFrame", "Next Frame" );

	m_pCurrentDemo = new vgui::Label( this, "DemoName", "" );

	m_pProgress = new vgui::ProgressBar( this, "DemoProgress" );
	m_pProgress->SetSegmentInfo( 2, 2 );

	m_pDriveCamera = new vgui::Button( this, "DemoDriveCamera", "Drive" );

	m_pProgressLabelPercent = new vgui::Label( this, "DemoProgressLabel", "" );
	m_pProgressLabelFrame = new vgui::Label( this, "DemoProgressLabelFrame", "" );
	m_pProgressLabelTime = new vgui::Label( this, "DemoProgressLabelTime", "" );

	m_pSpeedScale = new vgui::Slider( this, "DemoSpeedScale" );
	// 1000 == 10x %
	m_pSpeedScale->SetRange( 0, 1000 );
	m_pSpeedScale->SetValue( 500 );

	m_pSpeedScaleLabel = new vgui::Label( this, "SpeedScale", "" );

	m_pGo = new vgui::Button( this, "DemoGo", "Go" );
	m_pGotoFrame = new vgui::TextEntry( this, "DemoGoToFrame" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	LoadControlSettings("Resource\\DemoUIPanel.res");

	SetVisible( true );
	SetSizeable( false );
	SetMoveable( true );

	SetBounds( x, y, w, h );

	m_ViewOrigin.Init();
	m_ViewAngles.Init();
	memset( m_nOldCursor, 0, sizeof( m_nOldCursor ) );
	m_bInputActive = false;
	m_bGrabNewViewSetup = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDemoUIPanel::~CDemoUIPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoUIPanel::OnClose()
{
//	GetDoc()->SaveDialogState(this, "Options");
//	BaseClass::OnClose();
	return;
}

void CDemoUIPanel::OnTick()
{
	BaseClass::OnTick();

	m_pCurrentDemo->SetText( demoaction->GetCurrentDemoFile() );
	bool hasdemo = demoaction->GetCurrentDemoFile()[0] ? true : false;
	
	//m_pLoad->SetEnabled( !demo->IsPlayingBack() );

	m_pEdit->SetEnabled( true );
	m_pSmooth->SetEnabled( true );

	if ( !hasdemo )
	{
		m_pPlayPauseResume->SetEnabled( false );
		m_pPlayPauseResume->SetText( "Play" );
		m_pStop->SetEnabled( false );
		m_pTimeDemo->SetEnabled( true );
		m_pDriveCamera->SetEnabled( false );
		m_pGo->SetEnabled( false );
	}
	else
	{
		m_pPlayPauseResume->SetEnabled( true );
		m_pStop->SetEnabled( true );
		// m_pEdit->SetEnabled( !demo->IsPlayingBack() );

		if ( !demo->IsPlayingBack() )
		{
			m_pPlayPauseResume->SetText( "Play" );
			m_pTimeDemo->SetEnabled( true );
			m_pDriveCamera->SetEnabled( false );
			m_pGo->SetEnabled( false );
		}
		else
		{
			m_pPlayPauseResume->SetText( demo->IsPlaybackPaused() ? "Resume" : "Pause" );
			m_pDriveCamera->SetEnabled( demo->IsPlaybackPaused() ? true : false );
			m_pTimeDemo->SetEnabled( false );
			m_pGo->SetEnabled( true );
		}

	}

	if ( demo->IsPlayingBack() )
	{
		m_pProgress->SetEnabled( true );
		m_pProgress->SetProgress( demo->GetProgress() );
		m_pProgressLabelPercent->SetText( va( "%.2f %%", 100.0f * demo->GetProgress() ) );
		int curframe, totalframes;
		demo->GetProgressFrame( curframe, totalframes );
		m_pProgressLabelFrame->SetText( va( "frame (%i/%i)", curframe, totalframes ) );

		float curtime, totaltime;
		demo->GetProgressTime( curtime, totaltime );

		m_pProgressLabelTime->SetText( va( "time (%.3f/%.3f)", curtime, totaltime ) );
	}
	else
	{
		m_pProgress->SetEnabled( false );
		m_pProgress->SetProgress( 0.0f );
		m_pProgressLabelPercent->SetText( "" );
		m_pProgressLabelFrame->SetText( "" );
		m_pProgressLabelTime->SetText( "" );
	}

	m_pFastForward->SetVisible( !m_pTimeDemo->IsSelected() );
	m_pFastForward->SetEnabled( demo->IsPlayingBack() && !demo->IsPlaybackPaused() );

	demo->SetFastForwarding( IsHoldingFastForward() );

	m_pAdvanceFrame->SetEnabled( demo->IsPlayingBack() && demo->IsPlaybackPaused() );

	float rate = GetPlaybackScale();
	rate *= 100.0f;

	m_pSpeedScaleLabel->SetText( va( "%.2f %%", rate ) );
}

// Command issued
void CDemoUIPanel::OnCommand(const char *command)
{
	if ( !Q_strcasecmp( command, "stop" ) )
	{
		Cbuf_AddText( "disconnect\n" );
	}
	else if ( !Q_strcasecmp( command, "play" ) )
	{
		if ( !demo->IsPlayingBack() )
		{
			char cmd[ 256 ];
			Q_snprintf( cmd, sizeof( cmd ), "%s", m_pTimeDemo->IsSelected() ? "timedemo" : "playdemo" );

			Cbuf_AddText( va( "%s %s\n", cmd, demoaction->GetCurrentDemoFile() ) );
		}
		else
		{
			Cbuf_AddText( !demo->IsPlaybackPaused() ? "demopause\n" : "demoresume\n" );
		}
	}
	else if ( !Q_strcasecmp( command, "load" ) )
	{
		OnLoad();
	}
	else if ( !Q_strcasecmp( command, "edit" ) )
	{
		OnEdit();
	}
	else if ( !Q_strcasecmp( command, "smooth" ) )
	{
		OnSmooth();
	}
	else if ( !Q_strcasecmp( command, "advanceframe" ) )
	{
		if ( demo->IsPlayingBack() && demo->IsPlaybackPaused() )
		{
			demo->AdvanceOneFrame();
			m_bGrabNewViewSetup = true;
		}
	}
	else if ( !Q_strcasecmp( command, "gotoframe" ) )
	{
		OnGotoFrame();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CDemoUIPanel::OnEdit()
{
	if ( m_hDemoEditor != NULL )
	{
		m_hDemoEditor->SetVisible( true );
		m_hDemoEditor->MoveToFront();
		m_hDemoEditor->OnVDMChanged();
		return;
	}

	m_hDemoEditor = new CDemoEditorPanel( this );
}

void CDemoUIPanel::OnSmooth()
{
	if ( m_hDemoSmoother != NULL )
	{
		m_hDemoSmoother->SetVisible( true );
		m_hDemoSmoother->MoveToFront();
		m_hDemoSmoother->OnVDMChanged();
		return;
	}

	m_hDemoSmoother = new CDemoSmootherPanel( this );
}

void CDemoUIPanel::OnLoad()
{
	if ( !m_hFileOpenDialog.Get() )
	{
		m_hFileOpenDialog = new FileOpenDialog( this, "Choose .dem file", true );
		if ( m_hFileOpenDialog != NULL )
		{
			m_hFileOpenDialog->AddFilter("*.dem", "Demo Files (*.dem)", true);
		}
	}
	if ( m_hFileOpenDialog )
	{
		char startPath[ MAX_PATH ];
		Q_strncpy( startPath, com_gamedir, sizeof( startPath ) );
		COM_FixSlashes( startPath );
		m_hFileOpenDialog->SetStartDirectory( startPath );
		m_hFileOpenDialog->DoModal( false );
	}
}

char *COM_FileExtension (char *in);

void CDemoUIPanel::OnFileSelected( char const *fullpath )
{
	if ( !fullpath || !fullpath[ 0 ] )
		return;

	char relativepath[ 512 ];
	g_pFileSystem->FullPathToRelativePath( fullpath, relativepath );

	if ( Q_strcasecmp( COM_FileExtension( relativepath ), "dem" ) )
	{
		return;
	}

	// It's a dem file
	Cbuf_AddText( va( "playdemo %s\n", relativepath ) );
	Cbuf_AddText( "demopauseafterinit\n" );

	if ( m_hFileOpenDialog != NULL )
	{
		m_hFileOpenDialog->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoUIPanel::OnVDMChanged( void )
{
	if ( m_hDemoEditor != NULL )
	{
		m_hDemoEditor->OnVDMChanged();
	}
	if ( m_hDemoSmoother != NULL )
	{
		m_hDemoSmoother->OnVDMChanged();
	}

	m_bGrabNewViewSetup = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoUIPanel::IsHoldingFastForward( void )
{
	if ( !m_pFastForward->IsVisible() )
		return false;

	if ( !m_pFastForward->IsEnabled() )
		return false;

	return m_pFastForward->IsSelected();
}

float CDemoUIPanel::GetPlaybackScale( void )
{
	float scale = 1.0f;
	float curval = (float)m_pSpeedScale->GetValue() ;

	if ( curval <= 500.0f )
	{
		scale = curval / 500.0f;
	}
	else
	{
		scale = 1.0f + ( curval - 500.0f ) / 100.0f;
	}
	return scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//			elapsed - 
//			info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoUIPanel::OverrideView( democmdinfo_t& info, int frame, float elapsed )
{
	if ( m_hDemoSmoother != NULL )
	{
		bool smoothing = m_hDemoSmoother->OverrideView( info, frame, elapsed );
		if ( smoothing )
			return true;
	}

	if ( demo->IsPlaybackPaused() )
	{
		if ( m_bGrabNewViewSetup )
		{
			m_bGrabNewViewSetup = false;
		//if ( !m_bInputActive && m_pDriveCamera->IsSelected() )
		//{
		//	// Latch last valid ones from engine
			m_ViewOrigin = info.GetViewOrigin();
			m_ViewAngles = info.GetViewAngles();
		//}
		}

		HandleInput( m_pDriveCamera->IsSelected() );

		if ( m_ViewOrigin != vec3_origin )
		{
			g_pClientSidePrediction->SetViewOrigin( m_ViewOrigin );
			g_pClientSidePrediction->SetViewAngles( m_ViewAngles );
			g_pClientSidePrediction->SetLocalViewAngles( m_ViewAngles );
			VectorCopy( m_ViewAngles, cl.viewangles );
			return true;
		}
	}
	else
	{
		m_ViewOrigin = info.GetViewOrigin();
		m_ViewAngles = info.GetViewAngles();
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//			elapsed - 
//			smoothing - 
//-----------------------------------------------------------------------------
void CDemoUIPanel::DrawDebuggingInfo( int frame, float elapsed )
{
	if ( m_hDemoSmoother != NULL )
	{
		m_hDemoSmoother->DrawDebuggingInfo( frame, elapsed );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoUIPanel::HandleInput( bool active )
{
	if ( m_bInputActive ^ active )
	{
		if ( m_bInputActive && !active )
		{
			// Restore mouse
			vgui::input()->SetCursorPos( m_nOldCursor[0], m_nOldCursor[1] );
		}
		else
		{
			vgui::input()->GetCursorPos( m_nOldCursor[0], m_nOldCursor[1] );
		}
	}

	if ( active )
	{
		float f = 0.0f;
		float s = 0.0f;
		float u = 0.0f;

		float movespeed = 400.0f;

		if ( vgui::input()->IsKeyDown( vgui::KEY_W ) )
		{
			f = movespeed * host_frametime;
		}
		if ( vgui::input()->IsKeyDown( vgui::KEY_S ) )
		{
			f = -movespeed * host_frametime;
		}
		if ( vgui::input()->IsKeyDown( vgui::KEY_A ) )
		{
			s = -movespeed * host_frametime;
		}
		if ( vgui::input()->IsKeyDown( vgui::KEY_D ) )
		{
			s = movespeed * host_frametime;
		}
		if ( vgui::input()->IsKeyDown( vgui::KEY_X ) )
		{
			u = movespeed * host_frametime;
		}
		if ( vgui::input()->IsKeyDown( vgui::KEY_Z ) )
		{
			u = -movespeed * host_frametime;
		}

		int mx, my;
		int dx, dy;

		vgui::input()->GetCursorPos( mx, my );

		dx = mx - m_nOldCursor[0];
		dy = my - m_nOldCursor[1];

		vgui::input()->SetCursorPos( m_nOldCursor[0], m_nOldCursor[1] );

		// Convert to pitch/yaw

		float pitch = (float)dy * 0.22f;
		float yaw = -(float)dx * 0.22;

		// Apply mouse
		m_ViewAngles.x += pitch;

		m_ViewAngles.x = clamp( m_ViewAngles.x, -89.0f, 89.0f );

		m_ViewAngles.y += yaw;
		if ( m_ViewAngles.y > 180.0f )
		{
			m_ViewAngles.y -= 360.0f;
		}
		else if ( m_ViewAngles.y < -180.0f )
		{
			m_ViewAngles.y += 360.0f;
		}

		// Now apply forward, side, up

		Vector fwd, side, up;

		AngleVectors( m_ViewAngles, &fwd, &side, &up );

		m_ViewOrigin += fwd * f;
		m_ViewOrigin += side * s;
		m_ViewOrigin += up * u;
	}

	m_bInputActive = active;
}

void CDemoUIPanel::OnGotoFrame( void )
{
	char sz[ 512 ];
	m_pGotoFrame->GetText( sz, sizeof( sz ) );
	int frame = atoi( sz );

	if ( !demo->IsPlayingBack() )
	{
		char cmd[ 256 ];
		Q_snprintf( cmd, sizeof( cmd ), "%s", m_pTimeDemo->IsSelected() ? "timedemo" : "playdemo" );

		Cbuf_AddText( va( "%s %s\n", cmd, demoaction->GetCurrentDemoFile() ) );
		Cbuf_AddText( va( "demoskip %i\n", frame ) );

		return;
	}

	int demoframe, demototalframes;

	demo->GetProgressFrame( demoframe, demototalframes );

	bool waspaused = demo->IsPlaybackPaused();
	if ( waspaused )
	{
		demo->ResumePlayback();
	}

	if ( frame < demoframe )
	{
		char cmd[ 256 ];
		Q_snprintf( cmd, sizeof( cmd ), "%s", m_pTimeDemo->IsSelected() ? "timedemo" : "playdemo" );

		Cbuf_AddText( "stop\n" );
		Cbuf_AddText( va( "%s %s\n", cmd, demoaction->GetCurrentDemoFile() ) );
		Cbuf_AddText( va( "demoskip %i %s\n", frame, waspaused ? "pause" : "" ) );
	}
	else if ( frame > demoframe )
	{
		demo->SkipToFrame( frame, waspaused );
	}
	// Already on correct frame, sigh...

	m_bGrabNewViewSetup = true;
}

MessageMapItem_t CDemoUIPanel::m_MessageMap[] =
{
	MAP_MESSAGE_CONSTCHARPTR( CDemoUIPanel, "FileSelected", OnFileSelected, "fullpath" ),   
};

IMPLEMENT_PANELMAP(CDemoUIPanel, BaseClass);
