//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "glquake.h"
#include "cl_demosmootherpanel.h"
#include "vid.h"
#include "demo.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>

#include <vgui_controls/Controls.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui/IVgui.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/MenuButton.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/TextEntry.h>
#include <vgui/IInput.h>

#include "filesystem.h"
#include "filesystem_engine.h"
#include "cl_demosmoothing.h"
#include "cl_demoactionmanager.h"
#include "proto_version.h"
#include "iprediction.h"
#include "debugoverlay.h"
#include "draw.h"

using namespace vgui;

static float Ease_In( float t )
{
	float out = sqrt( t );
	return out;
}

static float Ease_Out( float t )
{
	float out = t * t;
	return out;
}

static float Ease_Both( float t )
{
	return SimpleSpline( t );
}

//-----------------------------------------------------------------------------
// Purpose: A menu button that knows how to parse cvar/command menu data from gamedir\scripts\debugmenu.txt
//-----------------------------------------------------------------------------
class CSmoothingTypeButton : public vgui::MenuButton
{
	typedef vgui::MenuButton BaseClass;

public:
	// Construction
	CSmoothingTypeButton( vgui::Panel *parent, const char *panelName, const char *text );

private:
	// Menu associated with this button
	Menu	*m_pMenu;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSmoothingTypeButton::CSmoothingTypeButton(Panel *parent, const char *panelName, const char *text)
	: BaseClass( parent, panelName, text )
{
	// Assume no menu
	m_pMenu = new Menu( this, "DemoSmootherTypeMenu" );

	m_pMenu->AddMenuItem( "Smooth Selection Angles", "smoothselectionangles", parent );
	m_pMenu->AddMenuItem( "Smooth Selection Origin", "smoothselectionorigin", parent );
	m_pMenu->AddMenuItem( "Linear Interp Angles", "smoothlinearinterpolateangles", parent );
	m_pMenu->AddMenuItem( "Linear Interp Origin", "smoothlinearinterpolateorigin", parent );
	m_pMenu->AddMenuItem( "Spline Angles", "splineangles", parent );
	m_pMenu->AddMenuItem( "Spline Origin", "splineorigin", parent );
	m_pMenu->AddMenuItem( "Look At Points", "lookatpoints", parent );
	m_pMenu->AddMenuItem( "Look At Points Spline", "lookatpointsspline", parent );
	m_pMenu->AddMenuItem( "Two Point Origin Ease Out", "origineaseout", parent );
	m_pMenu->AddMenuItem( "Two Point Origin Ease In", "origineasein", parent );
	m_pMenu->AddMenuItem( "Two Point Origin Ease In/Out", "origineaseboth", parent );
	
	m_pMenu->MakePopup();
	MenuButton::SetMenu(m_pMenu);
	SetOpenDirection(MenuButton::UP);
}

//-----------------------------------------------------------------------------
// Purpose: A menu button that knows how to parse cvar/command menu data from gamedir\scripts\debugmenu.txt
//-----------------------------------------------------------------------------
class CFixEdgeButton : public vgui::MenuButton
{
	typedef vgui::MenuButton BaseClass;

public:
	// Construction
	CFixEdgeButton( vgui::Panel *parent, const char *panelName, const char *text );

private:
	// Menu associated with this button
	Menu	*m_pMenu;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFixEdgeButton::CFixEdgeButton(Panel *parent, const char *panelName, const char *text)
	: BaseClass( parent, panelName, text )
{
	// Assume no menu
	m_pMenu = new Menu( this, "DemoSmootherEdgeFixType" );

	m_pMenu->AddMenuItem( "Smooth Left", "smoothleft", parent );
	m_pMenu->AddMenuItem( "Smooth Right", "smoothright", parent );
	m_pMenu->AddMenuItem( "Smooth Both", "smoothboth", parent );
	
	m_pMenu->MakePopup();
	MenuButton::SetMenu(m_pMenu);
	SetOpenDirection(MenuButton::UP);
}

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CDemoSmootherPanel::CDemoSmootherPanel( vgui::Panel *parent ) : Frame( parent, "DemoSmootherPanel")
{
	int w = 440;
	int h = 300;

	SetSize( w, h );

	SetTitle("Demo Smoother", true);

	m_pType = new CSmoothingTypeButton( this, "DemoSmootherType", "Process->" );

	m_pRevert = new vgui::Button( this, "DemoSmoothRevert", "Revert" );;
	m_pOK = new vgui::Button( this, "DemoSmoothOk", "OK" );
	m_pCancel = new vgui::Button( this, "DemoSmoothCancel", "Cancel" );

	m_pSave = new vgui::Button( this, "DemoSmoothSave", "Save" );
	m_pReloadFromDisk = new vgui::Button( this, "DemoSmoothReload", "Reload" );

	m_pStartFrame = new vgui::TextEntry( this, "DemoSmoothStartFrame" );
	m_pEndFrame = new vgui::TextEntry( this, "DemoSmoothEndFrame" );

	m_pPreviewOriginal = new vgui::Button( this, "DemoSmoothPreviewOriginal", "Show Original" );
	m_pPreviewProcessed = new vgui::Button( this, "DemoSmoothPreviewProcessed", "Show Processed" );

	m_pBackOff = new vgui::CheckButton( this, "DemoSmoothBackoff", "Back off" );	
	m_pHideLegend = new vgui::CheckButton( this, "DemoSmoothHideLegend", "Hide legend" );

	m_pHideOriginal = new vgui::CheckButton( this, "DemoSmoothHideOriginal", "Hide original" );
	m_pHideProcessed = new vgui::CheckButton( this, "DemoSmoothHideProcessed", "Hide processed" );

	m_pSelectionInfo = new vgui::Label( this, "DemoSmoothSelectionInfo", "" );
	m_pShowAllSamples = new vgui::CheckButton( this, "DemoSmoothShowAll", "Show All" );	
	m_pSelectSamples = new vgui::Button( this, "DemoSmoothSelect", "Select" );

	m_pPauseResume = new vgui::Button( this, "DemoSmoothPauseResume", "Pause" );
	m_pStepForward = new vgui::Button( this, "DemoSmoothStepForward", ">>" );
	m_pStepBackward = new vgui::Button( this, "DemoSmoothStepBackward", "<<" );

	m_pRevertPoint = new vgui::Button( this, "DemoSmoothRevertPoint", "Revert Pt." );
	m_pToggleKeyFrame = new vgui::Button( this, "DemoSmoothSetKeyFrame", "Mark Keyframe" );
	m_pToggleLookTarget = new vgui::Button( this, "DemoSmoothSetLookTarget", "Mark Look Target" );

	m_pUndo = new vgui::Button( this, "DemoSmoothUndo", "Undo" );
	m_pRedo = new vgui::Button( this, "DemoSmoothRedo", "Redo" );

	m_pNextKey = new vgui::Button( this, "DemoSmoothNextKey", "+Key" );
	m_pPrevKey = new vgui::Button( this, "DemoSmoothPrevKey", "-Key" );

	m_pNextTarget = new vgui::Button( this, "DemoSmoothNextTarget", "+Target" );
	m_pPrevTarget = new vgui::Button( this, "DemoSmoothPrevTarget", "-Target" );

	m_pDriveCamera = new vgui::Button( this, "DemoSmoothCamera", "Drive Fast" );
	m_pDriveCameraSlow = new vgui::Button( this, "DemoSmoothCameraSlow", "Drive Slow" );

	m_pMoveCameraToPoint = new vgui::Button( this, "DemoSmoothCameraAtPoint", "Set View" );

	m_pFixEdges = new CFixEdgeButton( this, "DemoSmoothFixFrameButton", "Edge->" );
	m_pFixEdgeFrames = new vgui::TextEntry( this, "DemoSmoothFixFrames" );

	m_pProcessKey = new vgui::Button( this, "DemoSmoothSaveKey", "Save Key" );

	m_pLockCamera = new vgui::CheckButton( this, "DemoSmoothLockCamera", "Lock camera" );

	m_pGotoFrame = new vgui::TextEntry( this, "DemoSmoothGotoFrame" );
	m_pGoto = new vgui::Button( this, "DemoSmoothGoto", "Jump To" );

	//m_pCurrentDemo = new vgui::Label( this, "DemoName", "" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	LoadControlSettings("Resource\\DemoSmootherPanel.res");

	/*
	int xpos, ypos;
	parent->GetPos( xpos, ypos );
	ypos += parent->GetTall();

	SetPos( xpos, ypos );
	*/

	OnRefresh();

	SetVisible( true );
	SetSizeable( false );
	SetMoveable( true );

	Reset();

	m_vecEyeOffset = Vector( 0, 0, 64 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDemoSmootherPanel::~CDemoSmootherPanel()
{
}

void CDemoSmootherPanel::Reset( void )
{
	demo->ClearSmoothingInfo( m_Smoothing );

	m_bPreviewing = false;
	m_bPreviewPaused = false;
	m_bPreviewOriginal = false;
	m_flPreviewStartTime = 0.0f;
	m_flPreviewCurrentTime = 0.0f;
	m_nPreviewLastFrame = 0;

	m_bHasSelection = false;
	memset( m_nSelection, 0, sizeof( m_nSelection ) );
	m_flSelectionTimeSpan = 0.0f;
	
	m_bInputActive = false;
	memset( m_nOldCursor, 0, sizeof( m_nOldCursor ) );

	m_ViewOrigin.Init();
	m_ViewAngles.Init();

	WipeUndo();
	WipeRedo();
	m_bRedoPending = false;
	m_nUndoLevel = 0;
	m_bDirty = false;
}


void CDemoSmootherPanel::OnTick()
{
	BaseClass::OnTick();

	HandleInput( m_pDriveCamera->IsSelected() || m_pDriveCameraSlow->IsSelected() );

	m_pUndo->SetEnabled( CanUndo() );
	m_pRedo->SetEnabled( CanRedo() );

	m_pPauseResume->SetEnabled( m_bPreviewing );
	m_pStepForward->SetEnabled( m_bPreviewing );
	m_pStepBackward->SetEnabled( m_bPreviewing );

	m_pSave->SetEnabled( m_bDirty );

	demosmoothing_t *p = GetCurrent();
	if ( p )
	{
		m_pToggleKeyFrame->SetEnabled( true );
		m_pToggleLookTarget->SetEnabled( true );

		m_pToggleKeyFrame->SetText( p->samplepoint ? "Delete Key" : "Make Key" );
		m_pToggleLookTarget->SetText( p->targetpoint ? "Delete Target" : "Make Target" );

		m_pProcessKey->SetEnabled( p->samplepoint );
	}
	else
	{
		m_pToggleKeyFrame->SetEnabled( false );
		m_pToggleLookTarget->SetEnabled( false );

		m_pProcessKey->SetEnabled( false );
	}

	if ( m_bPreviewing )
	{
		m_pPauseResume->SetText( m_bPreviewPaused ? "Resume" : "Pause" );
	}

	if ( !m_Smoothing.active )
	{
		m_pSelectionInfo->SetText( "No smoothing info loaded" );
		return;
	}

	if ( !demo->IsPlayingBack() )
	{
		m_pSelectionInfo->SetText( "Not playing back .dem" );
		return;
	}

	if ( !m_bHasSelection )
	{
		m_pSelectionInfo->SetText( "No selection." );
		return;
	}

	char sz[ 512 ];
	if ( m_bPreviewing )
	{
		Q_snprintf( sz, sizeof( sz ), "%.3f at frame %i %i to %i (%.3f s)", 
			m_flPreviewCurrentTime,
			m_nPreviewLastFrame,
			m_nSelection[ 0 ], m_nSelection[ 1 ], m_flSelectionTimeSpan );
	}
	else
	{
		Q_snprintf( sz, sizeof( sz ), "%i to %i (%.3f s)", m_nSelection[ 0 ], m_nSelection[ 1 ], m_flSelectionTimeSpan );
	}
	m_pSelectionInfo->SetText( sz );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoSmootherPanel::CanEdit()
{
	if ( !m_Smoothing.active )
		return false;

	if ( !demo->IsPlayingBack() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnCommand(const char *command)
{
	if ( !Q_strcasecmp( command, "cancel" ) )
	{
		OnRevert();
		MarkForDeletion();
		Reset();
		OnClose();
	}
	else if ( !Q_strcasecmp( command, "close" ) )
	{
		OnSave();
		MarkForDeletion();
		Reset();
		OnClose();
	}
	else if ( !Q_strcasecmp( command, "gotoframe" ) )
	{
		OnGotoFrame();
	}
	else if ( !Q_strcasecmp( command, "undo" ) )
	{
		Undo();
	}
	else if ( !Q_strcasecmp( command, "redo" ) )
	{
		Redo();
	}
	else if ( !Q_strcasecmp( command, "revert" ) )
	{
		OnRevert();
	}
	else if ( !Q_strcasecmp( command, "original" ) )
	{
		OnPreview( true );
	}
	else if ( !Q_strcasecmp( command, "processed" ) )
	{
		OnPreview( false );
	}
	else if ( !Q_strcasecmp( command, "save" ) )
	{
		OnSave();
	}
	else if ( !Q_strcasecmp( command, "reload" ) )
	{
		OnReload();
	}
	else if ( !Q_strcasecmp( command, "select" ) )
	{
		OnSelect();
	}
	else if ( !Q_strcasecmp( command, "togglepause" ) )
	{
		OnTogglePause();
	}
	else if ( !Q_strcasecmp( command, "stepforward" ) )
	{
		OnStep( true );
	}
	else if ( !Q_strcasecmp( command, "stepbackward" ) )
	{
		OnStep( false );
	}
	else if ( !Q_strcasecmp( command, "revertpoint" ) )
	{
		OnRevertPoint();
	}
	else if ( !Q_strcasecmp( command, "keyframe" ) )
	{
		OnToggleKeyFrame();
	}
	else if ( !Q_strcasecmp( command, "looktarget" ) )
	{
		OnToggleLookTarget();
	}
	else if ( !Q_strcasecmp( command, "nextkey" ) )
	{
		OnNextKey();
	}
	else if ( !Q_strcasecmp( command, "prevkey" ) )
	{
		OnPrevKey();
	}
	else if ( !Q_strcasecmp( command, "nexttarget" ) )
	{
		OnNextTarget();
	}
	else if ( !Q_strcasecmp( command, "prevtarget" ) )
	{
		OnPrevTarget();
	}
	else if ( !Q_strcasecmp( command, "smoothselectionangles" ) )
	{
		OnSmoothSelectionAngles();
	}
	else if ( !Q_strcasecmp( command, "smoothselectionorigin" ) )
	{
		OnSmoothSelectionOrigin();
	}
	else if ( !Q_strcasecmp( command, "smoothlinearinterpolateangles" ) )
	{
		OnLinearInterpolateAnglesBasedOnEndpoints();
	}
	else if ( !Q_strcasecmp( command, "smoothlinearinterpolateorigin" ) )
	{
		OnLinearInterpolateOriginBasedOnEndpoints();
	}
	else if ( !Q_strcasecmp( command, "splineorigin" ) )
	{
		OnSplineSampleOrigin();
	}
	else if ( !Q_strcasecmp( command, "splineangles" ) )
	{
		OnSplineSampleAngles();
	}
	else if ( !Q_strcasecmp( command, "lookatpoints" ) )
	{
		OnLookAtPoints( false );
	}
	else if ( !Q_strcasecmp( command, "lookatpointsspline" ) )
	{
		OnLookAtPoints( true );
	}
	else if ( !Q_strcasecmp( command, "smoothleft" ) )
	{
		OnSmoothEdges( true, false );
	}
	else if ( !Q_strcasecmp( command, "smoothright" ) )
	{
		OnSmoothEdges( false, true );
	}
	else if ( !Q_strcasecmp( command, "smoothboth" ) )
	{
		OnSmoothEdges( true, true );
	}
	else if ( !Q_strcasecmp( command, "origineasein" ) )
	{
		OnOriginEaseCurve( Ease_In );
	}
	else if ( !Q_strcasecmp( command, "origineaseout" ) )
	{
		OnOriginEaseCurve( Ease_Out );
	}
	else if ( !Q_strcasecmp( command, "origineaseboth" ) )
	{
		OnOriginEaseCurve( Ease_Both );
	}
	else if ( !Q_strcasecmp( command, "processkey" ) )
	{
		OnSaveKey();
	}
	else if ( !Q_strcasecmp( command, "setview" ) )
	{
		OnSetView();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnSave()
{
	if ( !m_Smoothing.active )
		return;

	demo->SaveSmoothingInfo( demoaction->GetCurrentDemoFile(), m_Smoothing );
	WipeUndo();
	m_bDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnReload()
{
	WipeUndo();
	WipeRedo();
	demo->LoadSmoothingInfo( demoaction->GetCurrentDemoFile(), m_Smoothing );
	m_bDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnVDMChanged( void )
{
	if ( IsVisible() )
	{
		OnReload();
	}
	else
	{
		Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnRevert()
{
	OnRefresh();
	if ( !m_Smoothing.active )
	{
		demo->LoadSmoothingInfo( demoaction->GetCurrentDemoFile(), m_Smoothing );
		WipeUndo();
		WipeRedo();
	}
	else
	{
		demo->ClearSmoothingInfo( m_Smoothing );
		WipeUndo();
		WipeRedo();
	}

	m_bDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnRefresh()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pScheme - 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CDemoSmootherPanel::GetStartFrame()
{
	char frame[ 32 ];
	m_pStartFrame->GetText( frame, sizeof( frame ) );
	return (int)atoi( frame );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CDemoSmootherPanel::GetEndFrame()
{
	char frame[ 32 ];
	m_pEndFrame->GetText( frame, sizeof( frame ) );
	return (int)atoi( frame );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : original - 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnPreview( bool original )
{
	if ( !CanEdit() )
		return;

	if ( !m_bHasSelection )
	{
		Con_Printf( "Must have smoothing selection active\n" );
		return;
	}

	m_bPreviewing = true;
	m_bPreviewPaused = false;
	m_bPreviewOriginal = original;
	SetLastFrame( false, max( 0, m_nSelection[0] - 10 ) );
	m_flPreviewStartTime = GetTimeForFrame( m_nPreviewLastFrame );
	m_flPreviewCurrentTime = m_flPreviewStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//			elapsed - 
//			info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoSmootherPanel::OverrideView( democmdinfo_t& info, int frame, float elapsed )
{
	if ( !CanEdit() )
		return false;

	if ( !demo->IsPlaybackPaused() )
		return false;

	if ( m_ViewOrigin != vec3_origin )
	{
		if ( ( m_pLockCamera->IsSelected() || m_bPreviewPaused ) )
		{
			if ( m_bPreviewing && !m_bPreviewPaused )
			{
				m_flPreviewCurrentTime += host_frametime;

				Vector renderOrigin;
				QAngle renderAngles;
				GetInterpolatedOriginAndAngles( false, renderOrigin, renderAngles );
			}

			g_pClientSidePrediction->SetViewOrigin( m_ViewOrigin );
			g_pClientSidePrediction->SetViewAngles( m_ViewAngles );
			g_pClientSidePrediction->SetLocalViewAngles( m_ViewAngles );
			VectorCopy( m_ViewAngles, cl.viewangles );
			
			return true;
		}
	}

	if ( m_bPreviewing )
	{
		if ( m_bPreviewPaused && GetCurrent() && GetCurrent()->samplepoint )
		{
			Vector org;
			QAngle ang;

			org = GetCurrent()->vecmoved;
			ang = GetCurrent()->angmoved;

			bool back_off = m_pBackOff->IsSelected();
			if ( back_off )
			{
				Vector fwd;
				AngleVectors( ang, &fwd, NULL, NULL );

				org = org - fwd * 75.0f;
			}

			g_pClientSidePrediction->SetViewOrigin( org );
			g_pClientSidePrediction->SetViewAngles( ang );
			g_pClientSidePrediction->SetLocalViewAngles( ang );
			VectorCopy( ang, cl.viewangles );

			return true;
		}

		// TODO:  Hook up previewing view
		if ( !m_bPreviewPaused )
		{
			m_flPreviewCurrentTime += host_frametime;
		}
		bool interpolated = GetInterpolatedViewPoint();
		return interpolated;
	}

	bool back_off = m_pBackOff->IsSelected();
	if ( back_off )
	{
		int c = m_Smoothing.smooth.Count();
		int useframe = frame;
		useframe = clamp( useframe, 0, c - 1 );

		if ( useframe < c && useframe >= 0 )
		{
			demosmoothing_t	*p = &m_Smoothing.smooth[ useframe ];
			Vector fwd;
			AngleVectors( p->info.viewAngles, &fwd, NULL, NULL );

			Vector renderOrigin;
			renderOrigin = p->info.viewOrigin - fwd * 75.0f;

			/*
			g_pClientSidePrediction->SetViewOrigin( outinfo.viewOrigin );
			g_pClientSidePrediction->SetViewAngles( outinfo.viewAngles );
			g_pClientSidePrediction->SetLocalViewAngles( outinfo.localViewAngles );
			VectorCopy( outinfo.viewAngles, cl.viewangles );
			*/

			g_pClientSidePrediction->SetViewOrigin( renderOrigin );
		}
	}

	return false;
}

void DrawVecForward( bool active, const Vector& origin, const QAngle& angles, int r, int g, int b )
{
	Vector fwd;
	AngleVectors( angles, &fwd, NULL, NULL );

	Vector end;
	end = origin + fwd * ( active ? 64 : 16 );

	Draw_Line( origin, end, r, g, b, false );
}

void GetColorForSample( bool original, bool samplepoint, bool targetpoint, demosmoothing_t *sample, int& r, int& g, int& b )
{
	if ( samplepoint && sample->samplepoint )
	{
		r = 0;
		g = 255;
		b = 0;
		return;
	}

	if ( targetpoint && sample->targetpoint )
	{
		r = 255;
		g = 0;
		b = 0;
		return;
	}

	if ( sample->selected )
	{
		if( original )
		{
			r = 255;
			g = 200;
			b = 100;
		}
		else
		{
			r = 200;
			g = 100;
			b = 255;
		}

		if ( sample->samplepoint || sample->targetpoint )
		{
			r = 255;
			g = 255;
			b = 0;
		}

		return;
	}

	if ( original )
	{
		r = g = b = 255;
	}
	else
	{
		r = 150;
		g = 255;
		b = 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			mins - 
//			maxs - 
//			angles - 
//			r - 
//			g - 
//			b - 
//			a - 
//-----------------------------------------------------------------------------
void Draw_Box( const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a )
{
	Draw_AlphaBox( origin, mins, maxs, angles, r, g, b, a);
	Draw_WireframeBox( origin, mins, maxs, angles, r, g, b );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *sample - 
//			*next - 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::DrawSmoothingSample( bool original, bool processed, int samplenumber, demosmoothing_t *sample, demosmoothing_t *next )
{
	int r, g, b;

	if ( original )
	{
		Draw_Line( sample->info.viewOrigin + m_vecEyeOffset, next->info.viewOrigin + m_vecEyeOffset,
			180, 180, 180, false );

		GetColorForSample( true, false, false, sample, r, g, b );

		DrawVecForward( false, sample->info.viewOrigin + m_vecEyeOffset, sample->info.viewAngles, r, g, b );
	}

	if ( processed && sample->info.flags != 0 )
	{
		Draw_Line( sample->info.GetViewOrigin() + m_vecEyeOffset, next->info.GetViewOrigin() + m_vecEyeOffset,
			255, 255, 180, false );

		GetColorForSample( false, false, false, sample, r, g, b );

		DrawVecForward( false, sample->info.GetViewOrigin() + m_vecEyeOffset, sample->info.GetViewAngles(), r, g, b );
	}
	if ( sample->samplepoint )
	{
		GetColorForSample( false, true, false, sample, r, g, b );
		Draw_Box( sample->vecmoved + m_vecEyeOffset, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), sample->angmoved, r, g, b, 127 );
		DrawVecForward( false, sample->vecmoved + m_vecEyeOffset, sample->angmoved, r, g, b );
	}

	if ( sample->targetpoint )
	{
		GetColorForSample( false, false, true, sample, r, g, b );
		Draw_Box( sample->vectarget, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), vec3_angle, r, g, b, 127 );
	}

	if ( samplenumber == m_nPreviewLastFrame + 1 )
	{
		r = 50;
		g = 100;
		b = 250;
		Draw_Box( sample->info.GetViewOrigin() + m_vecEyeOffset, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), sample->info.GetViewAngles(), r, g, b, 92 );
	}

	if ( sample->targetpoint )
	{
		r = 200;
		g = 200;
		b = 220;

		Draw_Line( sample->info.GetViewOrigin() + m_vecEyeOffset, sample->vectarget, r, g, b, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::DrawDebuggingInfo(  int frame, float elapsed )
{
	if ( !CanEdit() )
		return;

	if ( !IsVisible() )
		return;

	int c = m_Smoothing.smooth.Count();
	if ( c < 2 )
		return;

	int start = 0;
	int end = c - 1;

	bool showall = m_pShowAllSamples->IsSelected();
	if ( !showall )
	{
		start = max( frame - 200, 0 );
		end = min( frame + 200, c - 1 );
	}

	if ( m_bHasSelection && !showall )
	{
		start = max( m_nSelection[ 0 ] - 10, 0 );
		end = min( m_nSelection[ 1 ] + 10, c - 1 );
	}

	bool draworiginal = !m_pHideOriginal->IsSelected();
	bool drawprocessed = !m_pHideProcessed->IsSelected();
	int i;

	demosmoothing_t	*p = NULL;
	demosmoothing_t	*prev = NULL;
	for ( i = start; i < end; i++ )
	{
		p = &m_Smoothing.smooth[ i ];
		if ( prev && p )
		{
			DrawSmoothingSample( draworiginal, drawprocessed, i, prev, p );
		}
		prev = p;
	}

	Vector org;
	QAngle ang;

	if ( m_bPreviewing )
	{
		if ( GetInterpolatedOriginAndAngles( true, org, ang ) )
		{
			DrawVecForward( true, org + m_vecEyeOffset, ang, 200, 10, 50 );
		}
	}

	int useframe = frame;

	useframe = clamp( useframe, 0, c - 1 );
	if ( useframe < c )
	{
		p = &m_Smoothing.smooth[ useframe ];
		org = p->info.GetViewOrigin();
		ang = p->info.GetViewAngles();

		DrawVecForward( true, org + m_vecEyeOffset, ang, 100, 220, 250 );
		Draw_Box( org + m_vecEyeOffset, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), ang, 100, 220, 250, 127 );
	}

	DrawKeySpline();
	DrawTargetSpline();


	if ( !m_pHideLegend->IsSelected() )
	{
		DrawLegend( start, end );
	}
}

void CDemoSmootherPanel::OnSelect()
{
	if ( !CanEdit() )
		return;

	m_bHasSelection = false;
	m_flSelectionTimeSpan = 0.0f;
	memset( m_nSelection, 0, sizeof( m_nSelection ) );

	int start, end;
	start = GetStartFrame();
	end = GetEndFrame();

	int c = m_Smoothing.smooth.Count();
	if ( c < 2 )
		return;

	start = clamp( start, 0, c - 1 );
	end = clamp( end, 0, c - 1 );

	if ( start >= end )
		return;

	m_nSelection[ 0 ] = start;
	m_nSelection[ 1 ] = end;
	m_bHasSelection = true;

	demosmoothing_t	*startsample = &m_Smoothing.smooth[ start ];
	demosmoothing_t	*endsample = &m_Smoothing.smooth[ end ];

	m_bDirty = true;
	PushUndo( "select" );

	int i = 0;
	for ( i = 0; i < c; i++ )
	{
		if ( i >= start && i <= end )
		{
			m_Smoothing.smooth[ i ].selected = true;
		}
		else
		{
			m_Smoothing.smooth[ i ].selected = false;
		}
	}
	
	PushRedo( "select" );

	m_flSelectionTimeSpan = endsample->frametime - startsample->frametime;
}

float CDemoSmootherPanel::GetTimeForFrame( int frame )
{
	if ( !CanEdit() )
		return 0.0f;

	int c = m_Smoothing.smooth.Count();
	if ( c < 1 )
		return 0.0f;

	if ( frame < 0 )
		return m_Smoothing.smooth[ 0 ].frametime;

	if ( frame >= c )
		return m_Smoothing.smooth[ c - 1 ].frametime;


	return m_Smoothing.smooth[ frame ].frametime;
}

//-----------------------------------------------------------------------------
// Purpose: Interpolate Euler angles using quaternions to avoid singularities
// Input  : start - 
//			end - 
//			output - 
//			frac - 
//-----------------------------------------------------------------------------
static void InterpolateAngles( const QAngle& start, const QAngle& end, QAngle& output, float frac )
{
	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion( start, src );
	AngleQuaternion( end, dest );

	Quaternion result;

	// Slerp
	QuaternionSlerp( src, dest, frac, result );

	// Convert to euler
	QuaternionAngles( result, output );
}

bool CDemoSmootherPanel::GetInterpolatedOriginAndAngles( bool readonly, Vector& origin, QAngle& angles )
{
	origin.Init();
	angles.Init();

	Assert( m_bPreviewing );

	// Figure out the best samples
	int startframe	= m_nPreviewLastFrame;
	int nextframe	= startframe + 1;

	float t = m_flPreviewCurrentTime;

	int c = m_Smoothing.smooth.Count();

	do
	{
		if ( startframe >= c || nextframe >= c )
		{
			if ( !readonly )
			{
				//m_bPreviewing = false;
			}
			return false;
		}

		demosmoothing_t	*startsample = &m_Smoothing.smooth[ startframe ];
		demosmoothing_t	*endsample = &m_Smoothing.smooth[ nextframe ];

		if ( nextframe >= min( m_nSelection[1] + 10, c - 1 ) )
		{
			if ( !readonly )
			{
				OnPreview( m_bPreviewOriginal );
			}
			return false;
		}

		if ( endsample->frametime >= t )
		{
			// Found a spot
			float dt = endsample->frametime - startsample->frametime;
			// Should never occur!!!
			if ( dt <= 0.0f )
			{
				return false;
			}

			float frac = ( t - startsample->frametime ) / dt;

			frac = clamp( frac, 0.0f, 1.0f );

			// Compute render origin/angles
			Vector renderOrigin;
			QAngle renderAngles;

			if ( m_bPreviewOriginal )
			{
				VectorLerp( startsample->info.viewOrigin, endsample->info.viewOrigin, frac, renderOrigin );
				InterpolateAngles( startsample->info.viewAngles, endsample->info.viewAngles, renderAngles, frac );
			}
			else
			{
				VectorLerp( startsample->info.GetViewOrigin(), endsample->info.GetViewOrigin(), frac, renderOrigin );
				InterpolateAngles( startsample->info.GetViewAngles(), endsample->info.GetViewAngles(), renderAngles, frac );
			}

			origin = renderOrigin;
			angles = renderAngles;

			if ( !readonly )
			{
				SetLastFrame( false, startframe );
			}

			break;
		}

		startframe++;
		nextframe++;

	} while ( true );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
bool CDemoSmootherPanel::GetInterpolatedViewPoint( void )
{
	Assert( m_bPreviewing );

	Vector renderOrigin;
	QAngle renderAngles;

	bool bret = GetInterpolatedOriginAndAngles( false, renderOrigin, renderAngles );
	if ( !bret )
		return false;

	bool back_off = m_pBackOff->IsSelected();
	if ( back_off )
	{
		Vector fwd;
		AngleVectors( renderAngles, &fwd, NULL, NULL );

		renderOrigin = renderOrigin - fwd * 75.0f;
	}

		// Yuck
	g_pClientSidePrediction->SetViewOrigin( renderOrigin );
	g_pClientSidePrediction->SetViewAngles( renderAngles );
	g_pClientSidePrediction->SetLocalViewAngles( renderAngles );
	VectorCopy( renderAngles, cl.viewangles );

	return true;
}

void CDemoSmootherPanel::OnTogglePause()
{
	if ( !m_bPreviewing )
		return;

	m_bPreviewPaused = !m_bPreviewPaused;
}

void CDemoSmootherPanel::OnStep( bool forward )
{
	if ( !m_bPreviewing )
		return;

	if ( !m_bPreviewPaused )
		return;

	int c = m_Smoothing.smooth.Count();

	SetLastFrame( false, m_nPreviewLastFrame + ( forward ? 1 : -1 ) );
	SetLastFrame( false, clamp( m_nPreviewLastFrame, max( m_nSelection[ 0 ] - 10, 0 ), min( m_nSelection[ 1 ] + 10, c - 1 ) ) );
	m_flPreviewCurrentTime = GetTimeForFrame( m_nPreviewLastFrame );
}

void CDemoSmootherPanel::DrawLegend( int startframe, int endframe )
{
	int i;
	int skip = 20;

	bool back_off = m_pBackOff->IsSelected();

	for ( i = startframe; i <= endframe; i++ )
	{
		bool show = ( i % skip ) == 0;
		demosmoothing_t	*sample = &m_Smoothing.smooth[ i ];

		if ( sample->samplepoint || sample->targetpoint )
			show = true;

		if ( !show )
			continue;

		char sz[ 512 ];
		Q_snprintf( sz, sizeof( sz ), "%i - %.3f", sample->framenumber, sample->frametime );

		Vector fwd;
		AngleVectors( sample->info.GetViewAngles(), &fwd, NULL, NULL );

		CDebugOverlay::AddTextOverlay( sample->info.GetViewOrigin() + m_vecEyeOffset + fwd * ( back_off ? 5.0f : 50.0f ), 0, -1.0f, sz );
	}
}

#define EASE_TIME 0.2f

Quaternion SmoothAngles( CUtlVector< Quaternion >& stack )
{
	int c = stack.Count();
	Assert( c >= 1 );

	float weight = 1.0f / (float)c;

	Quaternion output;
	output.Init();
	
	int i;
	for ( i = 0; i < c; i++ )
	{
		Quaternion t = stack[ i ];
		QuaternionBlend( output, t, weight, output );
	}

	return output;
}

Vector SmoothOrigin( CUtlVector< Vector >& stack )
{
	int c = stack.Count();
	Assert( c >= 1 );

	Vector output;
	output.Init();
	
	int i;
	for ( i = 0; i < c; i++ )
	{
		Vector t = stack[ i ];
		VectorAdd( output, t, output );
	}

	VectorScale( output, 1.0f / (float)c, output );

	return output;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnSmoothSelectionAngles( void )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	CUtlVector< Quaternion > stack;

	m_bDirty = true;
	PushUndo( "smooth angles" );

	for ( i = 0; i < c; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];
		if ( !p->selected )
			continue;

		while ( stack.Count() > 10 )
		{
			stack.Remove( 0 );
		}

		Quaternion q;
		AngleQuaternion( p->info.GetViewAngles(), q );
		stack.AddToTail( q );

		p->info.flags |= FDEMO_USE_ANGLES2;

		Quaternion aveq = SmoothAngles( stack );

		QAngle outangles;
		QuaternionAngles( aveq, outangles );

		p->info.viewAngles2 = outangles;
		p->info.localViewAngles2 = outangles;
	}

	PushRedo( "smooth angles" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnSmoothSelectionOrigin( void )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	CUtlVector< Vector > stack;

	m_bDirty = true;
	PushUndo( "smooth origin" );

	for ( i = 0; i < c; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];
		if ( !p->selected )
			continue;

		if ( i < 2 )
			continue;

		if ( i >= c - 2 )
			continue;

		stack.RemoveAll();

		for ( int j = -2; j <= 2; j++ )
		{
			stack.AddToTail( m_Smoothing.smooth[ i + j ].info.GetViewOrigin() );
		}

		p->info.flags |= FDEMO_USE_ORIGIN2;

		Vector org = SmoothOrigin( stack );

		p->info.viewOrigin2 = org;
	}

	PushRedo( "smooth origin" );
}

void CDemoSmootherPanel::PerformLinearInterpolatedAngleSmoothing( int startframe, int endframe )
{
	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ startframe ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ endframe ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
	{
		dt = 1.0f;
	}

	CUtlVector< Quaternion > stack;

	Quaternion qstart, qend;
	AngleQuaternion( pstart->info.GetViewAngles(), qstart );
	AngleQuaternion( pend->info.GetViewAngles(), qend );

	int i;
	for ( i = startframe; i <= endframe; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		float elapsed = p->frametime - pstart->frametime;
		float frac = elapsed / dt;

		frac = clamp( frac, 0.0f, 1.0f );

		p->info.flags |= FDEMO_USE_ANGLES2;

		Quaternion interpolated;

		QuaternionSlerp( qstart, qend, frac, interpolated );

		QAngle outangles;
		QuaternionAngles( interpolated, outangles );

		p->info.viewAngles2 = outangles;
		p->info.localViewAngles2 = outangles;
	}
}

void CDemoSmootherPanel::OnLinearInterpolateAnglesBasedOnEndpoints( void )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	if ( c < 2 )
		return;

	m_bDirty = true;
	PushUndo( "linear interp angles" );

	PerformLinearInterpolatedAngleSmoothing( m_nSelection[ 0 ], m_nSelection[ 1 ] );

	PushRedo( "linear interp angles" );
}

void CDemoSmootherPanel::OnLinearInterpolateOriginBasedOnEndpoints( void )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	m_bDirty = true;
	PushUndo( "linear interp origin" );

	Vector vstart, vend;
	vstart = pstart->info.GetViewOrigin();
	vend = pend->info.GetViewOrigin();

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		float elapsed = p->frametime - pstart->frametime;
		float frac = elapsed / dt;

		frac = clamp( frac, 0.0f, 1.0f );

		p->info.flags |= FDEMO_USE_ORIGIN2;

		Vector interpolated;

		VectorLerp( vstart, vend, frac, interpolated );

		p->info.viewOrigin2 = interpolated;
	}

	PushRedo( "linear interp origin" );

}

void CDemoSmootherPanel::OnRevertPoint( void )
{
	demosmoothing_t *p = GetCurrent();
	if ( !p )
		return;

	m_bDirty = true;
	PushUndo( "revert point" );

	p->angmoved = p->info.GetViewAngles();
	p->vecmoved = p->info.GetViewOrigin();
	p->samplepoint = false;

	p->vectarget = p->info.GetViewOrigin();
	p->targetpoint = false;

	m_ViewOrigin = p->info.viewOrigin;
	m_ViewAngles = p->info.viewAngles;

	PushRedo( "revert point" );
}

demosmoothing_t *CDemoSmootherPanel::GetCurrent( void )
{
	if ( !CanEdit() )
		return NULL;

	int c = m_Smoothing.smooth.Count();
	if ( c < 1 )
		return NULL;

	int frame = clamp( m_nPreviewLastFrame, 0, c - 1 );

	return &m_Smoothing.smooth[ frame ];
}

void CDemoSmootherPanel::AddSamplePoints( bool usetarget, bool includeboundaries, CUtlVector< demosmoothing_t * >& points, int start, int end )
{
	points.RemoveAll();

	int i;
	for ( i = start; i <= end; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		if ( includeboundaries )
		{
			if ( i == start )
			{
				// Add it twice
				points.AddToTail( p );
				continue;
			}
			else if ( i == end )
			{
				// Add twice
				points.AddToTail( p );
				continue;
			}
		}

		if ( usetarget && p->targetpoint )
		{
			points.AddToTail( p );
		}
		if ( !usetarget && p->samplepoint )
		{
			points.AddToTail( p );
		}
	}
}

demosmoothing_t *CDemoSmootherPanel::GetBoundedSample( CUtlVector< demosmoothing_t * >& points, int sample )
{
	int c = points.Count();
	if ( sample < 0 )
		return points[ 0 ];
	else if ( sample >= c )
		return points[ c - 1 ];
	return points[ sample ];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//			points - 
//			prev - 
//			next - 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::FindSpanningPoints( float t, CUtlVector< demosmoothing_t * >& points, int& prev, int& next )
{
	prev = -1;
	next = 0;
	int c = points.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		demosmoothing_t	*p = points[ i ];
		
		if ( t < p->frametime )
			break;
	}

	next = i;
	prev = i - 1;

	next = clamp( next, 0, c - 1 );
	prev = clamp( prev, 0, c - 1 );
}

void CDemoSmootherPanel::OnSplineSampleOrigin( void )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	CUtlVector< demosmoothing_t * > points;
	AddSamplePoints( false, false, points, m_nSelection[ 0 ], m_nSelection[ 1 ] );

	if ( points.Count() <= 0 )
		return;

	m_bDirty = true;
	PushUndo( "spline origin" );

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		demosmoothing_t	*earliest;
		demosmoothing_t	*current;
		demosmoothing_t	*next;
		demosmoothing_t	*latest;
		
		int cur;
		int cur2;

		FindSpanningPoints( p->frametime, points, cur, cur2 );

		earliest = GetBoundedSample( points, cur - 1 );
		current = GetBoundedSample( points, cur );
		next = GetBoundedSample( points, cur2 );
		latest = GetBoundedSample( points, cur2 + 1 );

		float frac = 0.0f;
		float dt = next->frametime - current->frametime;
		if ( dt > 0.0f )
		{
			frac = ( p->frametime - current->frametime ) / dt;
		}

		frac = clamp( frac, 0.0f, 1.0f );

		Vector splined;

		Catmull_Rom_Spline_Normalize( earliest->vecmoved, current->vecmoved, next->vecmoved, latest->vecmoved, frac, splined );

		p->info.flags |= FDEMO_USE_ORIGIN2;
		p->info.viewOrigin2 = splined;
	}

	PushRedo( "spline origin" );

}

void CDemoSmootherPanel::OnSplineSampleAngles( void )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	CUtlVector< demosmoothing_t * > points;
	AddSamplePoints( false, false, points, m_nSelection[ 0 ], m_nSelection[ 1 ] );

	if ( points.Count() <= 0 )
		return;

	m_bDirty = true;
	PushUndo( "spline angles" );

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		demosmoothing_t	*current;
		demosmoothing_t	*next;
		
		int cur;
		int cur2;

		FindSpanningPoints( p->frametime, points, cur, cur2 );

		current = GetBoundedSample( points, cur );
		next = GetBoundedSample( points, cur2 );

		float frac = 0.0f;
		float dt = next->frametime - current->frametime;
		if ( dt > 0.0f )
		{
			frac = ( p->frametime - current->frametime ) / dt;
		}

		frac = clamp( frac, 0.0f, 1.0f );

		frac = SimpleSpline( frac );

		QAngle splined;

		InterpolateAngles( current->angmoved, next->angmoved, splined, frac );

		p->info.flags |= FDEMO_USE_ANGLES2;
		p->info.viewAngles2 = splined;
		p->info.localViewAngles2 = splined;
	}

	PushRedo( "spline angles" );
}

void CDemoSmootherPanel::OnLookAtPoints( bool spline )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	CUtlVector< demosmoothing_t * > points;
	AddSamplePoints( true, false, points, m_nSelection[ 0 ], m_nSelection[ 1 ] );

	if ( points.Count() < 1 )
		return;

	m_bDirty = true;
	PushUndo( "lookat points" );

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		demosmoothing_t	*earliest;
		demosmoothing_t	*current;
		demosmoothing_t	*next;
		demosmoothing_t	*latest;
		
		int cur;
		int cur2;

		FindSpanningPoints( p->frametime, points, cur, cur2 );

		earliest = GetBoundedSample( points, cur - 1 );
		current = GetBoundedSample( points, cur );
		next = GetBoundedSample( points, cur2 );
		latest = GetBoundedSample( points, cur2 + 1 );

		float frac = 0.0f;
		float dt = next->frametime - current->frametime;
		if ( dt > 0.0f )
		{
			frac = ( p->frametime - current->frametime ) / dt;
		}

		frac = clamp( frac, 0.0f, 1.0f );

		Vector splined;

		if ( spline )
		{
			Catmull_Rom_Spline_Normalize( earliest->vectarget, current->vectarget, next->vectarget, latest->vectarget, frac, splined );
		}
		else
		{
			Vector d = next->vectarget - current->vectarget;
			VectorMA( current->vectarget, frac, d, splined );
		}
		
		Vector vecToTarget = splined - ( p->info.GetViewOrigin() + m_vecEyeOffset );
		VectorNormalize( vecToTarget );

		QAngle angles;
		VectorAngles( vecToTarget, angles );

		p->info.flags |= FDEMO_USE_ANGLES2;
		p->info.viewAngles2 = angles;
		p->info.localViewAngles2 = angles;
	}

	PushRedo( "lookat points" );
}

void CDemoSmootherPanel::SetLastFrame( bool jumptotarget, int frame )
{
	bool changed = frame != m_nPreviewLastFrame;
	m_nPreviewLastFrame = frame;
	if ( changed && !m_pLockCamera->IsSelected()  )
	{
		// Reset default view/angles
		demosmoothing_t	*p = GetCurrent();
		if ( p )
		{
			if ( p->samplepoint && !jumptotarget )
			{
				m_ViewOrigin = p->vecmoved;
				m_ViewAngles = p->angmoved;
			}
			else if ( p->targetpoint && jumptotarget )
			{
				m_ViewOrigin = p->vectarget - m_vecEyeOffset;
			}
			else
			{
				if ( m_bPreviewing && m_bPreviewOriginal )
				{
					m_ViewOrigin = p->info.viewOrigin;
					m_ViewAngles = p->info.viewAngles;
				}
				else
				{
					m_ViewOrigin = p->info.GetViewOrigin();
					m_ViewAngles = p->info.GetViewAngles();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::HandleInput( bool active )
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

		bool shiftdown = vgui::input()->IsKeyDown( vgui::KEY_LSHIFT );
		shiftdown = shiftdown || vgui::input()->IsKeyDown( vgui::KEY_RSHIFT );

		float movespeed = ( shiftdown || m_pDriveCameraSlow->IsSelected() ) ? 10.0f : 400.0f;

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

// Undo/Redo
void CDemoSmootherPanel::Undo( void )
{
	if ( m_UndoStack.Size() > 0 && m_nUndoLevel > 0 )
	{
		m_nUndoLevel--;
		DemoSmoothUndo *u = m_UndoStack[ m_nUndoLevel ];
		Assert( u->undo );

		m_Smoothing = *(u->undo);
	}
	InvalidateLayout();
}

void CDemoSmootherPanel::Redo( void )
{
	if ( m_UndoStack.Size() > 0 && m_nUndoLevel <= m_UndoStack.Size() - 1 )
	{
		DemoSmoothUndo *u = m_UndoStack[ m_nUndoLevel ];
		Assert( u->redo );

		m_Smoothing = *(u->redo);
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

void CDemoSmootherPanel::PushUndo( char *description )
{
	Assert( !m_bRedoPending );
	m_bRedoPending = true;
	WipeRedo();

	// Copy current data
	CSmoothingContext *u = new CSmoothingContext;
	*u = m_Smoothing;
	DemoSmoothUndo *undo = new DemoSmoothUndo;
	undo->undo = u;
	undo->redo = NULL;
	undo->udescription = CopyString( description );
	undo->rdescription = NULL;
	m_UndoStack.AddToTail( undo );
	m_nUndoLevel++;
}

void CDemoSmootherPanel::PushRedo( char *description )
{
	Assert( m_bRedoPending );
	m_bRedoPending = false;

	// Copy current data
	CSmoothingContext *r = new CSmoothingContext;
	*r = m_Smoothing;
	DemoSmoothUndo *undo = m_UndoStack[ m_nUndoLevel - 1 ];
	undo->redo = r;
	undo->rdescription = CopyString( description );
}

void CDemoSmootherPanel::WipeUndo( void )
{
	while ( m_UndoStack.Size() > 0 )
	{
		DemoSmoothUndo *u = m_UndoStack[ 0 ];
		delete u->undo;
		delete u->redo;
		delete[] u->udescription;
		delete[] u->rdescription;
		delete u;
		m_UndoStack.Remove( 0 );
	}
	m_nUndoLevel = 0;
}

void CDemoSmootherPanel::WipeRedo( void )
{
	// Wipe everything above level
	while ( m_UndoStack.Size() > m_nUndoLevel )
	{
		DemoSmoothUndo *u = m_UndoStack[ m_nUndoLevel ];
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
const char *CDemoSmootherPanel::GetUndoDescription( void )
{
	if ( m_nUndoLevel != 0 )
	{
		DemoSmoothUndo *u = m_UndoStack[ m_nUndoLevel - 1 ];
		return u->udescription;
	}
	return "???undo";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CDemoSmootherPanel::GetRedoDescription( void )
{
	if ( m_nUndoLevel != m_UndoStack.Size() )
	{
		DemoSmoothUndo *u = m_UndoStack[ m_nUndoLevel ];
		return u->rdescription;
	}
	return "???redo";
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoSmootherPanel::CanRedo( void )
{
	if ( !m_UndoStack.Count() )
		return false;

	if ( m_nUndoLevel == m_UndoStack.Count()  )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoSmootherPanel::CanUndo( void )
{
	if ( !m_UndoStack.Count() )
		return false;

	if ( m_nUndoLevel == 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnToggleKeyFrame( void )
{
	demosmoothing_t *p = GetCurrent();
	if ( !p )
		return;

	m_bDirty = true;
	PushUndo( "toggle keyframe" );

	if ( !p->samplepoint )
	{
		p->angmoved = m_ViewAngles;
		p->vecmoved = m_ViewOrigin;
		p->samplepoint = true;
	}
	else
	{
		p->angmoved = p->info.GetViewAngles();
		p->vecmoved = p->info.GetViewOrigin();
		p->samplepoint = false;
	}

	PushRedo( "toggle keyframe" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnToggleLookTarget( void )
{
	demosmoothing_t *p = GetCurrent();
	if ( !p )
		return;

	m_bDirty = true;
	PushUndo( "toggle look target" );

	if ( !p->targetpoint )
	{
		Vector forward;
		AngleVectors( m_ViewAngles, &forward, NULL, NULL );

		p->vectarget = ( m_ViewOrigin + m_vecEyeOffset ) + 16.0f * forward;
		p->targetpoint = true;
	}
	else
	{
		p->vectarget = p->info.GetViewOrigin();
		p->targetpoint = false;
	}

	PushRedo( "toggle look target" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnNextKey()
{
	if( !m_bHasSelection )
		return;

	int start = m_nPreviewLastFrame + 1;
	int maxmove = m_nSelection[1] - m_nSelection[0] + 1;

	int moved = 0;

	while ( moved < maxmove )
	{
		demosmoothing_t *p = &m_Smoothing.smooth[ start ];
		if ( p->samplepoint )
		{
			SetLastFrame( false, start );
			break;
		}

		start++;

		if ( start > m_nSelection[1] )
			start = m_nSelection[0];

		moved++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnPrevKey()
{
	if( !m_bHasSelection )
		return;

	int start = m_nPreviewLastFrame - 1;
	int maxmove = m_nSelection[1] - m_nSelection[0] + 1;

	int moved = 0;

	while ( moved < maxmove )
	{
		demosmoothing_t *p = &m_Smoothing.smooth[ start ];
		if ( p->samplepoint )
		{
			SetLastFrame( false, start );
			break;
		}

		start--;

		if ( start < m_nSelection[0] )
			start = m_nSelection[1];

		moved++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnNextTarget()
{
	if( !m_bHasSelection )
		return;

	int start = m_nPreviewLastFrame + 1;
	int maxmove = m_nSelection[1] - m_nSelection[0] + 1;

	int moved = 0;

	while ( moved < maxmove )
	{
		demosmoothing_t *p = &m_Smoothing.smooth[ start ];
		if ( p->targetpoint )
		{
			SetLastFrame( true, start );
			break;
		}

		start++;

		if ( start > m_nSelection[1] )
			start = m_nSelection[0];

		moved++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnPrevTarget()
{
	if( !m_bHasSelection )
		return;

	int start = m_nPreviewLastFrame - 1;
	int maxmove = m_nSelection[1] - m_nSelection[0] + 1;

	int moved = 0;

	while ( moved < maxmove )
	{
		demosmoothing_t *p = &m_Smoothing.smooth[ start ];
		if ( p->targetpoint )
		{
			SetLastFrame( true, start );
			break;
		}

		start--;

		if ( start < m_nSelection[0] )
			start = m_nSelection[1];

		moved++;
	}
}

void CDemoSmootherPanel::DrawTargetSpline()
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	CUtlVector< demosmoothing_t * > points;
	AddSamplePoints( true, false, points, m_nSelection[ 0 ], m_nSelection[ 1 ] );

	if ( points.Count() < 1 )
		return;

	Vector previous(0,0,0);

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		demosmoothing_t	*earliest;
		demosmoothing_t	*current;
		demosmoothing_t	*next;
		demosmoothing_t	*latest;
		
		int cur;
		int cur2;

		FindSpanningPoints( p->frametime, points, cur, cur2 );

		earliest = GetBoundedSample( points, cur - 1 );
		current = GetBoundedSample( points, cur );
		next = GetBoundedSample( points, cur2 );
		latest = GetBoundedSample( points, cur2 + 1 );

		float frac = 0.0f;
		float dt = next->frametime - current->frametime;
		if ( dt > 0.0f )
		{
			frac = ( p->frametime - current->frametime ) / dt;
		}

		frac = clamp( frac, 0.0f, 1.0f );

		Vector splined;

		Catmull_Rom_Spline_Normalize( earliest->vectarget, current->vectarget, next->vectarget, latest->vectarget, frac, splined );

		if ( i > m_nSelection[0] )
		{
			Draw_Line( previous, splined, 0, 255, 0, false );
		}

		previous = splined;
	}
}

void CDemoSmootherPanel::DrawKeySpline()
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	CUtlVector< demosmoothing_t * > points;
	AddSamplePoints( false, false, points, m_nSelection[ 0 ], m_nSelection[ 1 ] );

	if ( points.Count() < 1 )
		return;

	Vector previous(0,0,0);

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		demosmoothing_t	*earliest;
		demosmoothing_t	*current;
		demosmoothing_t	*next;
		demosmoothing_t	*latest;
		
		int cur;
		int cur2;

		FindSpanningPoints( p->frametime, points, cur, cur2 );

		earliest = GetBoundedSample( points, cur - 1 );
		current = GetBoundedSample( points, cur );
		next = GetBoundedSample( points, cur2 );
		latest = GetBoundedSample( points, cur2 + 1 );

		float frac = 0.0f;
		float dt = next->frametime - current->frametime;
		if ( dt > 0.0f )
		{
			frac = ( p->frametime - current->frametime ) / dt;
		}

		frac = clamp( frac, 0.0f, 1.0f );

		Vector splined;

		Catmull_Rom_Spline_Normalize( earliest->vecmoved, current->vecmoved, next->vecmoved, latest->vecmoved, frac, splined );

		splined += m_vecEyeOffset;

		if ( i > m_nSelection[0] )
		{
			Draw_Line( previous, splined, 0, 255, 0, false );
		}

		previous = splined;
	}
}

void CDemoSmootherPanel::OnSmoothEdges( bool left, bool right )
{
	if ( !m_bHasSelection )
		return;

	if ( !left && !right )
		return;

	int c = m_Smoothing.smooth.Count();

	// Get number of frames
	char sz[ 512 ];
	m_pFixEdgeFrames->GetText( sz, sizeof( sz ) );

	int frames = atoi( sz );
	if ( frames <= 2 )
		return;

	m_bDirty = true;
	PushUndo( "smooth edges" );

	if ( left && m_nSelection[0] > 0 )
	{
		PerformLinearInterpolatedAngleSmoothing( m_nSelection[ 0 ] - 1, m_nSelection[ 0 ] + frames );
	}
	if ( right && m_nSelection[1] < c - 1 )
	{
		PerformLinearInterpolatedAngleSmoothing( m_nSelection[ 1 ] - frames, m_nSelection[ 1 ] + 1 );
	}

	PushRedo( "smooth edges" );
}

void CDemoSmootherPanel::OnSaveKey()
{
	if ( !m_bHasSelection )
		return;

	demosmoothing_t *p = GetCurrent();
	if ( !p )
		return;

	if ( !p->samplepoint )
		return;

	m_bDirty = true;
	PushUndo( "save key" );

	p->info.viewAngles2 = p->angmoved;
	p->info.localViewAngles2 = p->angmoved;
	p->info.viewOrigin2 = p->vecmoved;
	p->info.flags |= FDEMO_USE_ORIGIN2;
	p->info.flags |= FDEMO_USE_ANGLES2;
	
	PushRedo( "save key" );
}

void CDemoSmootherPanel::OnSetView()
{
	if ( !m_bHasSelection )
		return;

	demosmoothing_t *p = GetCurrent();
	if ( !p )
		return;

	m_ViewOrigin = p->info.GetViewOrigin();
	m_ViewAngles = p->info.GetViewAngles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoSmootherPanel::OnGotoFrame()
{
	int c = m_Smoothing.smooth.Count();
	if ( c < 2 )
		return;

	char sz[ 256 ];
	m_pGotoFrame->GetText( sz, sizeof( sz ) );
	int frame = atoi( sz );

	if ( !m_bPreviewing )
	{
		if ( !m_bHasSelection )
		{
			m_pStartFrame->SetText( va( "%i", 0 ) );
			m_pEndFrame->SetText( va( "%i", c - 1 ) );
			OnSelect();
		}
		OnPreview( false );
		OnTogglePause();
	}

	if ( !m_bPreviewing )
		return;

	SetLastFrame( false, frame );
	m_flPreviewStartTime = GetTimeForFrame( m_nPreviewLastFrame );
	m_flPreviewCurrentTime = m_flPreviewStartTime;
}

void CDemoSmootherPanel::OnOriginEaseCurve( EASEFUNC easefunc )
{
	if ( !m_bHasSelection )
		return;

	int c = m_Smoothing.smooth.Count();
	int i;

	if ( c < 2 )
		return;

	demosmoothing_t	*pstart		= &m_Smoothing.smooth[ m_nSelection[ 0 ] ];
	demosmoothing_t	*pend		= &m_Smoothing.smooth[ m_nSelection[ 1 ] ];

	float dt = pend->frametime - pstart->frametime;
	if ( dt <= 0.0f )
		return;

	m_bDirty = true;
	PushUndo( "ease origin" );

	Vector vstart, vend;
	vstart = pstart->info.GetViewOrigin();
	vend = pend->info.GetViewOrigin();

	for ( i = m_nSelection[0]; i <= m_nSelection[1]; i++ )
	{
		demosmoothing_t	*p = &m_Smoothing.smooth[ i ];

		float elapsed = p->frametime - pstart->frametime;
		float frac = elapsed / dt;

		// Apply ease function
		frac = (*easefunc)( frac );

		frac = clamp( frac, 0.0f, 1.0f );

		p->info.flags |= FDEMO_USE_ORIGIN2;

		Vector interpolated;

		VectorLerp( vstart, vend, frac, interpolated );

		p->info.viewOrigin2 = interpolated;
	}

	PushRedo( "ease origin" );
}
