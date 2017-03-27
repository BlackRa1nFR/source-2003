//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           mdlviewer.cpp
// last modified:  Jun 03 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#include "cbase.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mx/mx.h>
#include <mx/mxTga.h>
#include <mx/mxEvent.h>
#include "mdlviewer.h"
#include "ViewerSettings.h"
#include "MatSysWin.h"
#include "ControlPanel.h"
#include "FlexPanel.h"
#include "StudioModel.h"
#include "cmdlib.h"
#include "mxExpressionTray.h"
#include "mxStatusWindow.h"
#include "ChoreoView.h"
#include "ifaceposersound.h"
#include "ifaceposerworkspace.h"
#include "expclass.h"
#include "PhonemeEditor.h"
#include "FileSystem.h"
#include "FileSystem_Tools.h"
#include "ExpressionTool.h"
#include "ControlPanel.h"
#include "choreowidgetdrawhelper.h"
#include "choreoviewcolors.h"
#include "tabwindow.h"
#include "faceposer_models.h"
#include "choiceproperties.h"
#include "choreoscene.h"
#include "choreoactor.h"
#include "vstdlib/strtools.h"
#include "InputProperties.h"
#include "GestureTool.h"
#include "SoundEmitterSystemBase.h"
#include "RampTool.h"
#include "SceneRampTool.h"
#include "vstdlib/icommandline.h"
#include "phonemeextractor.h"

#define WINDOW_TAB_OFFSET 24

MDLViewer *g_MDLViewer = 0;
char g_appTitle[] = "Half-Life Face Poser";
static char recentFiles[8][256] = { "", "", "", "", "", "", "", "" };

void
MDLViewer::initRecentFiles ()
{
	for (int i = 0; i < 8; i++)
	{
		if (strlen (recentFiles[i]))
		{
			mb->modify (IDC_FILE_RECENTMODELS1 + i, IDC_FILE_RECENTMODELS1 + i, recentFiles[i]);
		}
		else
		{
			mb->modify (IDC_FILE_RECENTMODELS1 + i, IDC_FILE_RECENTMODELS1 + i, "(empty)");
			mb->setEnabled (IDC_FILE_RECENTMODELS1 + i, false);
		}
	}
}



void
MDLViewer::loadRecentFiles ()
{
	char path[256];
	strcpy (path, mx::getApplicationPath ());
	strcat (path, "/hlmv.rf");
	FILE *file = fopen (path, "rb");
	if (file)
	{
		fread (recentFiles, sizeof recentFiles, 1, file);
		fclose (file);
	}
}



void
MDLViewer::saveRecentFiles ()
{
	char path[256];

	strcpy (path, mx::getApplicationPath ());
	strcat (path, "/hlmv.rf");

	FILE *file = fopen (path, "wb");
	if (file)
	{
		fwrite (recentFiles, sizeof recentFiles, 1, file);
		fclose (file);
	}
}

bool MDLViewer::Closing( void )
{
	Con_Printf( "Checking for sound script changes...\n" );

	// Save any changed sound script files
	int c = soundemitter->GetNumSoundScripts();
	for ( int i = 0; i < c; i++ )
	{
		if ( !soundemitter->IsSoundScriptDirty( i ) )
			continue;

		char const *scriptname = soundemitter->GetSoundScriptName( i );
		if ( !scriptname )
			continue;

		if ( !filesystem->FileExists( scriptname ) ||
			 !filesystem->IsFileWritable( scriptname ) )
		{
			continue;
		}

		soundemitter->SaveChangesToSoundScript( i );
	}

	SaveWindowPositions();

	models->SaveModelList();
	models->CloseAllModels();

	return true;
}

#define IDC_GRIDSETTINGS_FPS	1001
#define IDC_GRIDSETTINGS_SNAP	1002

class CFlatButton : public mxButton
{
public:
	CFlatButton( mxWindow *parent, int id )
		: mxButton( parent, 0, 0, 0, 0, "", id )
	{
		HWND wnd = (HWND)getHandle();
		DWORD exstyle = GetWindowLong( wnd, GWL_EXSTYLE );
		exstyle |= WS_EX_CLIENTEDGE;
		SetWindowLong( wnd, GWL_EXSTYLE, exstyle );

		DWORD style = GetWindowLong( wnd, GWL_STYLE );
		style &= ~WS_BORDER;
		SetWindowLong( wnd, GWL_STYLE, style );

	}
};

class CMDLViewerGridSettings : public mxWindow
{
public:
	typedef mxWindow BaseClass;

	CMDLViewerGridSettings( mxWindow *parent, int x, int y, int w, int h ) :
		mxWindow( parent, x, y, w, h )
	{
		FacePoser_AddWindowStyle( this, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
		m_btnFPS = new CFlatButton( this, IDC_GRIDSETTINGS_FPS );
		m_btnGridSnap = new CFlatButton( this, IDC_GRIDSETTINGS_SNAP );

	}

	void Init( void )
	{
		if ( g_pChoreoView )
		{
			CChoreoScene *scene = g_pChoreoView->GetScene();
			if ( scene )
			{
				char sz[ 256 ];
				Q_snprintf( sz, sizeof( sz ), "%i fps", scene->GetSceneFPS() );
				m_btnFPS->setLabel( sz );

				Q_snprintf( sz, sizeof( sz ), "snap: %s", scene->IsUsingFrameSnap() ? "on" : "off" );
				m_btnGridSnap->setLabel( sz );

				m_btnFPS->setVisible( true );
				m_btnGridSnap->setVisible( true );
				return;
			}
		}
		
		m_btnFPS->setVisible( false );
		m_btnGridSnap->setVisible( false );
	}

	virtual int handleEvent( mxEvent *event )
	{
		int iret = 0;
		switch ( event->event )
		{
		default:
			break;
		case mxEvent::Size:
			{
				int leftedge = w2() * 0.45f;
				m_btnFPS->setBounds( 0, 0, leftedge, h2() );
				m_btnGridSnap->setBounds( leftedge, 0, w2() - leftedge, h2() );
				iret = 1;
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
				case IDC_GRIDSETTINGS_FPS:
					{
						if ( g_pChoreoView )
						{
							CChoreoScene *scene = g_pChoreoView->GetScene();
							if ( scene )
							{
								int currentFPS = scene->GetSceneFPS();
								
								CInputParams params;
								memset( &params, 0, sizeof( params ) );
								
								strcpy( params.m_szDialogTitle, "Change FPS" );
								
								Q_snprintf( params.m_szInputText, sizeof( params.m_szInputText ),
									"%i", currentFPS );
								
								strcpy( params.m_szPrompt, "Current FPS:" );
								
								if ( InputProperties( &params ) )
								{
									int newFPS = atoi( params.m_szInputText );
									
									if ( ( newFPS > 0 ) && ( newFPS != currentFPS ) )
									{
										g_pChoreoView->SetDirty( true );
										g_pChoreoView->PushUndo( "Change Scene FPS" );
										scene->SetSceneFPS( newFPS );
										g_pChoreoView->PushRedo( "Change Scene FPS" );
										Init();

										Con_Printf( "FPS changed to %i\n", newFPS );
									}
								}
								
							}
						}
					}
					break;
				case IDC_GRIDSETTINGS_SNAP:
					{
						if ( g_pChoreoView )
						{
							CChoreoScene *scene = g_pChoreoView->GetScene();
							if ( scene )
							{
								g_pChoreoView->SetDirty( true );
								g_pChoreoView->PushUndo( "Change Snap Frame" );
								
								scene->SetUsingFrameSnap( !scene->IsUsingFrameSnap() );
								
								g_pChoreoView->PushRedo( "Change Snap Frame" );
								
								Init();

								Con_Printf( "Time frame snapping: %s\n",
									scene->IsUsingFrameSnap() ? "on" : "off" );
							}
						}


					}
					break;
				}
			}
		}
		return iret;
	}

	bool PaintBackground( void )
	{
		CChoreoWidgetDrawHelper drawHelper( this );
		RECT rc;
		drawHelper.GetClientRect( rc );
		drawHelper.DrawFilledRect( GetSysColor( COLOR_BTNFACE ), rc );
		return false;
	}
	
private:

	CFlatButton	*m_btnFPS;
	CFlatButton	*m_btnGridSnap;
};

#define IDC_MODELTAB_LOAD			1000
#define IDC_MODELTAB_CLOSE			1001
#define IDC_MODELTAB_CLOSEALL		1002
#define IDC_MODELTAB_CENTERONFACE	1003
#define IDC_MODELTAB_ASSOCIATEACTOR 1004
#define IDC_MODELTAB_TOGGLE3DVIEW	1005
#define IDC_MODELTAB_SHOWALL		1006
#define IDC_MODELTAB_HIDEALL		1007
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMDLViewerModelTab : public CTabWindow
{
public:
	typedef CTabWindow BaseClass;

	CMDLViewerModelTab( mxWindow *parent, int x, int y, int w, int h, int id = 0, int style = 0 ) :
		CTabWindow( parent, x, y, w, h, id, style )
	{
		SetInverted( true );
	}

	virtual void ShowRightClickMenu( int mx, int my )
	{
		mxPopupMenu *pop = new mxPopupMenu();
		Assert( pop );

		char const *current = "";
		char const *filename = "";
		int idx = getSelectedIndex();
		if ( idx >= 0 )
		{
			current = models->GetModelName( idx );
			filename = models->GetModelFileName( idx );
		}

		if ( models->Count() < MAX_FP_MODELS )
		{
			pop->add( "Load Model...", IDC_MODELTAB_LOAD );
		}
		if ( idx >= 0 )
		{
			pop->add( va( "Close '%s'", current ), IDC_MODELTAB_CLOSE );
		}
		if ( models->Count() > 0 )
		{
			pop->add( "Close All", IDC_MODELTAB_CLOSEALL );
		}
		if ( idx >= 0 )
		{
			pop->addSeparator();
			pop->add( va( "Center %s's face", current ), IDC_MODELTAB_CENTERONFACE );

			CChoreoScene *scene = g_pChoreoView->GetScene();
			if ( scene )
			{
				// See if there is already an actor with this model associated
				int c = scene->GetNumActors();
				bool hasassoc = false;
				for ( int i = 0; i < c; i++ )
				{
					CChoreoActor *a = scene->GetActor( i );
					Assert( a );
				
					if ( stricmp( a->GetFacePoserModelName(), filename ) )
						continue;
					hasassoc = true;
					break;
				}

				if ( hasassoc )
				{
					pop->add( va( "Change associated actor for %s", current ), IDC_MODELTAB_ASSOCIATEACTOR );
				}
				else
				{
					pop->add( va( "Associate actor to %s", current ), IDC_MODELTAB_ASSOCIATEACTOR );
				}
			}

			pop->addSeparator();

			bool visible = models->IsModelShownIn3DView( idx );
			if ( visible )
			{
				pop->add( va( "Remove %s from 3D View", current ), IDC_MODELTAB_TOGGLE3DVIEW );
			}
			else
			{
				pop->add( va( "Show %s in 3D View", current ), IDC_MODELTAB_TOGGLE3DVIEW );
			}
		}
		if ( models->Count() > 0 )
		{
			pop->addSeparator();
			pop->add( "Show All", IDC_MODELTAB_SHOWALL );
			pop->add( "Hide All", IDC_MODELTAB_HIDEALL );
		}

		// Convert click position
		POINT pt;
		pt.x = mx;
		pt.y = my;

		// Convert coordinate space
		pop->popup( this, pt.x, pt.y );
	}

	virtual int handleEvent( mxEvent *event )
	{
		int iret = 0;
		switch ( event->event )
		{
		default:
			break;
		case mxEvent::Action:
			{
				iret = 1;
				switch ( event->action )
				{
				default:
					iret = 0;
					break;
				case IDC_MODELTAB_SHOWALL:
				case IDC_MODELTAB_HIDEALL:
					{
						bool show = ( event->action == IDC_MODELTAB_SHOWALL ) ? true : false;
						int c = models->Count();
						for ( int i = 0; i < c ; i++ )
						{
							models->ShowModelIn3DView( i, show );
						}
					}
					break;
				case IDC_MODELTAB_LOAD:
					{
						const char *ptr = mxGetOpenFileName(
							this, 
							FacePoser_MakeWindowsSlashes( va( "%s/models/", GetGameDirectory() ) ), 
							"*.mdl" );

						if (ptr)
						{
							g_MDLViewer->LoadModelFile( ptr );
						}
					}
					break;
				case IDC_MODELTAB_CLOSE:
					{
						int idx = getSelectedIndex();
						if ( idx >= 0 )
						{
							models->FreeModel( idx );
						}
					}
					break;
				case IDC_MODELTAB_CLOSEALL:
					{
						models->CloseAllModels();
					}
					break;
				case IDC_MODELTAB_CENTERONFACE:
					{
						g_pControlPanel->CenterOnFace();
					}
					break;
				case IDC_MODELTAB_TOGGLE3DVIEW:
					{
						int idx = getSelectedIndex();
						if ( idx >= 0 )
						{
							bool visible = models->IsModelShownIn3DView( idx );
							models->ShowModelIn3DView( idx, !visible );
						}
					}
					break;
				case IDC_MODELTAB_ASSOCIATEACTOR:
					{
						int idx = getSelectedIndex();
						if ( idx >= 0 )
						{
							char const *modelname = models->GetModelFileName( idx );

							CChoreoScene *scene = g_pChoreoView->GetScene();
							if ( scene )
							{
								CChoiceParams params;
								strcpy( params.m_szDialogTitle, "Associate Actor" );

								params.m_bPositionDialog = false;
								params.m_nLeft = 0;
								params.m_nTop = 0;
								strcpy( params.m_szPrompt, "Choose actor:" );

								params.m_Choices.RemoveAll();

								params.m_nSelected = -1;
								int oldsel = -1;

								int c = scene->GetNumActors();
								ChoiceText text;
								for ( int i = 0; i < c; i++ )
								{
									CChoreoActor *a = scene->GetActor( i );
									Assert( a );

									
									strcpy( text.choice, a->GetName() );

									if ( !stricmp( a->GetFacePoserModelName(), modelname ) )
									{
										params.m_nSelected = i;
										oldsel = -1;
									}

									params.m_Choices.AddToTail( text );
								}
		
								if ( ChoiceProperties( &params ) && 
									params.m_nSelected != oldsel )
								{
									
									// Chose something new...
									CChoreoActor *a = scene->GetActor( params.m_nSelected );
									
									g_pChoreoView->AssociateModelToActor( a, idx );
								}
							}
						}

					}
				}
			}
			break;
		}
		if ( iret )
			return iret;
		return BaseClass::handleEvent( event );
	}


	void HandleModelSelect( void )
	{
		int idx = getSelectedIndex();
		if ( idx < 0 )
			return;

		// FIXME: Do any necessary window resetting here!!!
		g_pControlPanel->OnModelChanged( models->GetModelFileName( idx ) );
	}

	void Init( void )
	{
		removeAll();
		
		int c = models->Count();
		int i;
		for ( i = 0; i < c ; i++ )
		{
			char const *name = models->GetModelName( i );
			add( name );
		}
	}
};

#define IDC_TOOL_TOGGLEVISIBILITY	1000
#define IDC_TOOL_TOGGLELOCK			1001
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMDLViewerWindowTab : public CTabWindow
{
public:
	typedef CTabWindow BaseClass;

	CMDLViewerWindowTab( mxWindow *parent, int x, int y, int w, int h, int id = 0, int style = 0 ) :
		CTabWindow( parent, x, y, w, h, id, style )
	{
		SetInverted( true );

		m_nLastSelected = -1;
		m_flLastSelectedTime = -1;
	}

	virtual void ShowRightClickMenu( int mx, int my )
	{
		IFacePoserToolWindow *tool = GetSelectedTool();
		if ( !tool )
			return;

		mxWindow *toolw = tool->GetMxWindow();
		if ( !toolw )
			return;

		mxPopupMenu *pop = new mxPopupMenu();
		Assert( pop );

		bool isVisible = toolw->isVisible();
		bool isLocked = tool->IsLocked();

		pop->add( isVisible ? "Hide" : "Show", IDC_TOOL_TOGGLEVISIBILITY );
		pop->add( isLocked ? "Unlock" : "Lock", IDC_TOOL_TOGGLELOCK );

		// Convert click position
		POINT pt;
		pt.x = mx;
		pt.y = my;

		/*
		ClientToScreen( (HWND)getHandle(), &pt );
		ScreenToClient( (HWND)g_MDLViewer->getHandle(), &pt );
		*/

		// Convert coordinate space
		pop->popup( this, pt.x, pt.y );
	}

	virtual int	handleEvent( mxEvent *event )
	{
		int iret = 0;
		switch ( event->event )
		{
		case mxEvent::Action:
			{
				iret = 1;
				switch ( event->action )
				{
				default:
					iret = 0;
					break;
				case IDC_TOOL_TOGGLEVISIBILITY:
					{
						IFacePoserToolWindow *tool = GetSelectedTool();
						if ( tool )
						{
							mxWindow *toolw = tool->GetMxWindow();
							if ( toolw )
							{
								toolw->setVisible( !toolw->isVisible() );
								g_MDLViewer->UpdateWindowMenu();
							}
						}
					}
					break;
				case IDC_TOOL_TOGGLELOCK:
					{
						IFacePoserToolWindow *tool = GetSelectedTool();
						if ( tool )
						{
							tool->ToggleLockedState();
						}
					}
					break;
				}
			}
			break;
		default:
			break;
		}
		if ( iret )
			return iret;
		return BaseClass::handleEvent( event );
	}

	void	Init( void )
	{
		int c = IFacePoserToolWindow::GetToolCount();
		int i;
		for ( i = 0; i < c ; i++ )
		{
			IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
			add( tool->GetDisplayNameRoot() );
		}
	}

#define WINDOW_DOUBLECLICK_TIME 0.4

	void	HandleWindowSelect( void )
	{
		extern double realtime;
		IFacePoserToolWindow *tool = GetSelectedTool();
		if ( !tool )
			return;

		bool doubleclicked = false;

		double curtime = realtime;
		int clickedItem = getSelectedIndex();

		if ( clickedItem == m_nLastSelected )
		{
			if ( curtime < m_flLastSelectedTime + WINDOW_DOUBLECLICK_TIME )
			{
				doubleclicked = true;
			}
		}

		m_flLastSelectedTime = curtime;
		m_nLastSelected = clickedItem;

		mxWindow *toolw = tool->GetMxWindow();
		if ( !toolw )
			return;

		if ( doubleclicked )
		{
			toolw->setVisible( !toolw->isVisible() );
			m_flLastSelectedTime = -1;
		}

		if ( !toolw->isVisible() )
		{
			return;
		}

		// Move window to front
		HWND wnd = (HWND)tool->GetMxWindow()->getHandle();
		SetFocus( wnd );
		SetWindowPos( wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}

private:

	IFacePoserToolWindow *GetSelectedTool()
	{
		int idx = getSelectedIndex();
		int c = IFacePoserToolWindow::GetToolCount();
	
		if ( idx < 0 || idx >= c )
			return NULL;

		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( idx );
		return tool;
	}

	// HACKY double click handler
	int		m_nLastSelected;
	double	m_flLastSelectedTime;
};

//-----------------------------------------------------------------------------
// Purpose: The workspace is the parent of all of the tool windows
//-----------------------------------------------------------------------------
class CMDLViewerWorkspace : public mxWindow
{
public:
	CMDLViewerWorkspace( mxWindow *parent, int x, int y, int w, int h, const char *label = 0, int style = 0)
		: mxWindow( parent, x, y, w, h, label, style )
	{
		FacePoser_AddWindowStyle( this, WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool PaintBackground( void )
	{
		CChoreoWidgetDrawHelper drawHelper( this );
		RECT rc;
		drawHelper.GetClientRect( rc );
		drawHelper.DrawFilledRect( GetSysColor( COLOR_APPWORKSPACE ), rc );
		return false;
	}
};

void MDLViewer::LoadPosition( void )
{
	bool visible;
	bool locked;
	bool zoomed;
	int x, y, w, h;

	FacePoser_LoadWindowPositions( "MDLViewer", visible, x, y, w, h, locked, zoomed );

	if ( w == 0 || h == 0 )
	{
		zoomed = true;
		visible = true;
	}

	setBounds( x, y, w, h );
	if ( zoomed )
	{
		ShowWindow( (HWND)getHandle(), SW_SHOWMAXIMIZED );
	}
	else
	{
		setVisible( visible );
	}
}

void MDLViewer::SavePosition( void )
{
	bool visible;
	int xpos, ypos, width, height;

	visible = isVisible();
	xpos = x();
	ypos = y();
	width = w();
	height = h();

	// xpos and ypos are screen space
	POINT pt;
	pt.x = xpos;
	pt.y = ypos;

	// Convert from screen space to relative to client area of parent window so
	//  the setBounds == MoveWindow call will offset to the same location
	if ( getParent() )
	{
		ScreenToClient( (HWND)getParent()->getHandle(), &pt );
		xpos = (short)pt.x;
		ypos = (short)pt.y;
	}

	bool zoomed = IsZoomed( (HWND)getHandle() ) ? true : false;

	FacePoser_SaveWindowPositions( "MDLViewer", visible, xpos, ypos, width, height, false, zoomed );
}

MDLViewer::MDLViewer ()
: mxWindow (0, 0, 0, 0, 0, g_appTitle, mxWindow::Normal)
{
	int i;

	g_MDLViewer = this;

	FacePoser_MakeToolWindow( this, false );

	workspace = new CMDLViewerWorkspace( this, 0, 0, 500, 500, "" );
	windowtab = new CMDLViewerWindowTab( this, 0, 500, 500, 20, IDC_WINDOW_TAB );
	modeltab = new CMDLViewerModelTab( this, 500, 500, 100, 20, IDC_MODEL_TAB );
	gridsettings = new CMDLViewerGridSettings( this, 0, 500, 500, 20 );
	modeltab->SetRightJustify( true );

	g_pStatusWindow = new mxStatusWindow( workspace, 0, 0, 1024, 150, "" );
	g_pStatusWindow->setVisible( true );

	InitViewerSettings( "faceposer" );
	g_viewerSettings.speechapiindex = SPEECH_API_LIPSINC;
	g_viewerSettings.flHeadTurn = 0.0f;
	g_viewerSettings.m_iEditAttachment = -1;

	LoadPosition();
	// ShowWindow( (HWND)getHandle(), SW_SHOWMAXIMIZED );

	g_pStatusWindow->setBounds(  0, h2() - 150, w2(), 150 );

	Con_Printf( "MDLViewer started\n" );

	Con_Printf( "CSoundEmitterSystemBase::Init()\n" );
	soundemitter->BaseInit();

	Con_Printf( "Creating menu bar\n" );

	// create menu stuff
	mb = new mxMenuBar (this);
	menuFile = new mxMenu ();
	menuOptions = new mxMenu ();
	menuWindow = new mxMenu ();
	menuHelp = new mxMenu ();
	menuEdit = new mxMenu ();
	menuExpressions = new mxMenu();
	menuChoreography = new mxMenu();

	mb->addMenu ("File", menuFile);
	mb->addMenu( "Edit", menuEdit );
	mb->addMenu ("Options", menuOptions);
	mb->addMenu ( "Expression", menuExpressions );
	mb->addMenu ( "Choreography", menuChoreography );
	mb->addMenu ("Window", menuWindow);
	mb->addMenu ("Help", menuHelp);

	mxMenu *menuRecentModels = new mxMenu ();
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS1);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS2);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS3);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS4);

	menuFile->add ("Load Model...", IDC_FILE_LOADMODEL);
	menuFile->add( "Refresh\tF5", IDC_FILE_REFRESH );

	menuFile->addSeparator();

	menuFile->add ("Load Background Texture...", IDC_FILE_LOADBACKGROUNDTEX);
	menuFile->add ("Load Ground Texture...", IDC_FILE_LOADGROUNDTEX);
	menuFile->addSeparator ();
	menuFile->add ("Unload Ground Texture", IDC_FILE_UNLOADGROUNDTEX);
	menuFile->addSeparator ();
	menuFile->addMenu ("Recent Models", menuRecentModels);
	menuFile->addSeparator ();
	menuFile->add ("Exit", IDC_FILE_EXIT);

	menuFile->setEnabled(IDC_FILE_LOADBACKGROUNDTEX, false);
	menuFile->setEnabled(IDC_FILE_LOADGROUNDTEX, false);
	menuFile->setEnabled(IDC_FILE_UNLOADGROUNDTEX, false);

	menuEdit->add( "&Undo\tCtrl+Z", IDC_EDIT_UNDO );
	//menuFile->setEnabled( IDC_EDIT_UNDO, false );
	menuEdit->add( "&Redo\tCtrl+Y", IDC_EDIT_REDO );
	//menuFile->setEnabled( IDC_EDIT_REDO, false );

	menuEdit->addSeparator();

	menuEdit->add( "&Copy\tCtrl+C", IDC_EDIT_COPY );
	menuEdit->add( "&Paste\tCtrl+V", IDC_EDIT_PASTE );

	menuOptions->add ("Background Color...", IDC_OPTIONS_COLORBACKGROUND);
	menuOptions->add ("Ground Color...", IDC_OPTIONS_COLORGROUND);
	menuOptions->add ("Light Color...", IDC_OPTIONS_COLORLIGHT);
	menuOptions->addSeparator ();
	menuOptions->add ("Center View", IDC_OPTIONS_CENTERVIEW);
	menuOptions->add ("Center on Face", IDC_OPTIONS_CENTERONFACE );
#ifdef WIN32
	menuOptions->addSeparator ();
	menuOptions->add ("Make Screenshot...", IDC_OPTIONS_MAKESCREENSHOT);
	//menuOptions->add ("Dump Model Info", IDC_OPTIONS_DUMP);
#endif

	menuExpressions->add( "New...", IDC_EXPRESSIONS_NEW );
	menuExpressions->addSeparator ();
	menuExpressions->add( "Load...", IDC_EXPRESSIONS_LOAD );
	menuExpressions->add( "Save", IDC_EXPRESSIONS_SAVE );
	menuExpressions->addSeparator ();
	menuExpressions->add( "Export to VFE", IDC_EXPRESSIONS_EXPORT );
	menuExpressions->addSeparator ();
	menuExpressions->add( "Close class", IDC_EXPRESSIONS_CLOSE );
	menuExpressions->add( "Close all classes", IDC_EXPRESSIONS_CLOSEALL );
	menuExpressions->addSeparator();
	menuExpressions->add( "Recreate all bitmaps", IDC_EXPRESSIONS_REDOBITMAPS );

	menuChoreography->add( "New...", IDC_CHOREOSCENE_NEW );
	menuChoreography->addSeparator();
	menuChoreography->add( "Load...", IDC_CHOREOSCENE_LOAD );
	menuChoreography->add( "Save", IDC_CHOREOSCENE_SAVE );
	menuChoreography->add( "Save As...", IDC_CHOREOSCENE_SAVEAS );
	menuChoreography->addSeparator();
	menuChoreography->add( "Close", IDC_CHOREOSCENE_CLOSE );
	menuChoreography->addSeparator();
	menuChoreography->add( "Add Actor...", IDC_CHOREOSCENE_ADDACTOR );

#ifdef WIN32
	menuHelp->add ("Goto Homepage...", IDC_HELP_GOTOHOMEPAGE);
	menuHelp->addSeparator ();
#endif
	menuHelp->add ("About...", IDC_HELP_ABOUT);

	// create the Material System window
	Con_Printf( "Creating 3D View\n" );
	g_pMatSysWindow = new MatSysWindow (workspace, 0, 0, 0, 0, "", mxWindow::Normal);

	Con_Printf( "Creating control panel\n" );
	g_pControlPanel = new ControlPanel (workspace);

	Con_Printf( "Creating phoneme editor\n" );
	g_pPhonemeEditor = new PhonemeEditor( workspace );

	Con_Printf( "Creating expression tool\n" );
	g_pExpressionTool = new ExpressionTool( workspace );

	Con_Printf( "Creating gesture tool\n" );
	g_pGestureTool = new GestureTool( workspace );

	Con_Printf( "Creating ramp tool\n" );
	g_pRampTool = new RampTool( workspace );

	Con_Printf( "Creating scene ramp tool\n" );
	g_pSceneRampTool = new SceneRampTool( workspace );

	Con_Printf( "Creating expression tray\n" );
	g_pExpressionTrayTool = new mxExpressionTray( workspace, IDC_EXPRESSIONTRAY );

	Con_Printf( "Creating flex slider window\n" );
	g_pFlexPanel = new FlexPanel( workspace );

	Con_Printf( "Creating choreography view\n" );
	g_pChoreoView = new CChoreoView( workspace, 200, 200, 400, 300, 0 );
	// Choreo scene file drives main window title name
	g_pChoreoView->SetUseForMainWindowTitle( true );

	Con_Printf( "IFacePoserToolWindow::Init\n" );

	IFacePoserToolWindow::Init();

	Con_Printf( "windowtab->Init\n" );
	
	windowtab->Init();

	Con_Printf( "loadRecentFiles\n" );

	loadRecentFiles ();
	initRecentFiles ();

	Con_Printf( "LoadWindowPositions\n" );

	LoadWindowPositions();

	Con_Printf( "RestoreThumbnailSize\n" );

	g_pExpressionTrayTool->RestoreThumbnailSize();

	Con_Printf( "Add Tool Windows\n" );

	int c = IFacePoserToolWindow::GetToolCount();
	for ( i = 0; i < c ; i++ )
	{
		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
		menuWindow->add( tool->GetToolName(), IDC_WINDOW_FIRSTTOOL + i );
	}

	menuWindow->addSeparator();
	menuWindow->add( "Cascade", IDC_WINDOW_CASCADE );
	menuWindow->addSeparator();
	menuWindow->add( "Tile", IDC_WINDOW_TILE );
	menuWindow->add( "Tile Horizontally", IDC_WINDOW_TILE_HORIZ );
	menuWindow->add( "Tile Vertically", IDC_WINDOW_TILE_VERT );
	menuWindow->addSeparator();
	menuWindow->add( "Hide All", IDC_WINDOW_HIDEALL );
	menuWindow->add( "Show All", IDC_WINDOW_SHOWALL );

	Con_Printf( "UpdateWindowMenu\n" );

	UpdateWindowMenu();

	m_nCurrentFrame = 0;

	Con_Printf( "gridsettings->Init()\n" );

	gridsettings->Init();

	Con_Printf( "Model viewer created\n" );
}

void MDLViewer::UpdateWindowMenu( void )
{
	int c = IFacePoserToolWindow::GetToolCount();
	for ( int i = 0; i < c ; i++ )
	{
		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
		menuWindow->setChecked( IDC_WINDOW_FIRSTTOOL + i, tool->GetMxWindow()->isVisible() );
	}
}

bool MDLViewer::CanClose()
{
	return true;
}

void MDLViewer::OnDelete()
{
	saveRecentFiles ();
	SaveViewerSettings( g_viewerSettings.modelFile );

#ifdef WIN32
	DeleteFile ("hlmv.cfg");
	DeleteFile ("midump.txt");
#endif

	g_MDLViewer = NULL;
}

MDLViewer::~MDLViewer ()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MDLViewer::InitModelTab( void )
{
	modeltab->Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MDLViewer::InitGridSettings( void )
{
	gridsettings->Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int MDLViewer::GetActiveModelTab( void )
{
	return modeltab->getSelectedIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : modelindex - 
//-----------------------------------------------------------------------------
void MDLViewer::SetActiveModelTab( int modelindex )
{
	modeltab->select( modelindex );
	modeltab->HandleModelSelect();
}

//-----------------------------------------------------------------------------
// Purpose: Reloads the currently loaded model file.
//-----------------------------------------------------------------------------
void MDLViewer::Refresh( void )
{
	g_pMaterialSystem->ReloadTextures( );

	if ( recentFiles[0][0] != '\0' )
	{
		char szFile[MAX_PATH];
		strcpy( szFile, recentFiles[0] ); 
		LoadModelFile( szFile );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads the file and updates the MRU list.
// Input  : pszFile - File to load.
//-----------------------------------------------------------------------------
void MDLViewer::LoadModelFile( const char *pszFile )
{
	int i;
	
	models->LoadModel( pszFile );

	for (i = 0; i < 4; i++)
	{
		if (!mx_strcasecmp( recentFiles[i], pszFile ))
			break;
	}

	// swap existing recent file
	if (i < 4)
	{
		char tmp[256];
		strcpy (tmp, recentFiles[0]);
		strcpy (recentFiles[0], recentFiles[i]);
		strcpy (recentFiles[i], tmp);
	}

	// insert recent file
	else
	{
		for (i = 3; i > 0; i--)
			strcpy (recentFiles[i], recentFiles[i - 1]);

		strcpy( recentFiles[0], pszFile );
	}

	initRecentFiles ();
}

/*
void MDLViewer::SwitchToChoreoMode( bool choreo )
{
	g_pFlexPanel->setEnabled( !choreo );
	g_pChoreoView->setEnabled( choreo );

	g_pFlexPanel->setVisible( !choreo );
	g_pChoreoView->setVisible( choreo );

	// So we'll be in the same mode when we restart
	g_viewerSettings.application_mode = choreo ? 1 : 0;

	if (choreo)
	{
		g_viewerSettings.solveHeadTurn = 1;
		g_viewerSettings.flHeadTurn = 0.0;
		g_viewerSettings.vecHeadTarget = Vector(0,0,0);
		g_viewerSettings.vecEyeTarget = Vector(0,0,0);
	}
	else
	{
		g_viewerSettings.solveHeadTurn = 2;
		g_viewerSettings.flHeadTurn = 1.0;
		g_viewerSettings.vecHeadTarget = Vector(0,0,0);
		g_viewerSettings.vecEyeTarget = Vector(0,0,0);
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *wnd - 
//			x - 
//			y - 
// Output : static bool
//-----------------------------------------------------------------------------
static bool WindowContainsPoint( mxWindow *wnd, int x, int y )
{
	POINT pt;
	pt.x = (short)x;
	pt.y = (short)y;

	HWND window = (HWND)wnd->getHandle();
	if ( !window )
		return false;

	ScreenToClient( window, &pt );

	if ( pt.x < 0 )
		return false;
	if ( pt.y < 0 )
		return false;
	if ( pt.x > wnd->w() )
		return false;
	if ( pt.y > wnd->h() )
		return false;

	return true;
}

int MDLViewer::handleEvent (mxEvent *event)
{
	int iret = 0;

	switch (event->event)
	{
	case mxEvent::Size:
		{
			int width = w2();
			int height = h2();

			workspace->setBounds( 0, 0, width, height - WINDOW_TAB_OFFSET );
			int gridsettingswide = 100;
			int gridstart = width - gridsettingswide - 5;
			int windowwide = gridstart * 0.6f;
			int modelwide = gridstart * 0.4f;

			gridsettings->setBounds( gridstart, height - WINDOW_TAB_OFFSET + 1, gridsettingswide, WINDOW_TAB_OFFSET - 2 );

			windowtab->setBounds( 0, height - WINDOW_TAB_OFFSET, windowwide, WINDOW_TAB_OFFSET );
			modeltab->setBounds( windowwide, height - WINDOW_TAB_OFFSET, modelwide, WINDOW_TAB_OFFSET );

			iret = 1;
		}
		break;
	case mxEvent::Action:
		{
			iret = 1;
			switch (event->action)
			{
			case IDC_WINDOW_TAB:
				{
					windowtab->HandleWindowSelect();
				}
				break;
			case IDC_MODEL_TAB:
				{
					modeltab->HandleModelSelect();
				}
				break;
			case IDC_EDIT_COPY:
				{
					Copy();
				}
				break;
				
			case IDC_EDIT_PASTE:
				{
					Paste();
				}
				break;
				
			case IDC_EDIT_UNDO:
				{
					Undo();
				}
				break;
				
			case IDC_EDIT_REDO:
				{
					Redo();
				}
				break;
				
			case IDC_FILE_LOADMODEL:
				{
					const char *ptr = mxGetOpenFileName(
						this, 
						FacePoser_MakeWindowsSlashes( va( "%s/models/", GetGameDirectory() ) ),
						"*.mdl");
					if (ptr)
					{
						LoadModelFile( ptr );
					}
				}
				break;
				
			case IDC_FILE_REFRESH:
				{
					Refresh();
					break;
				}
				
			case IDC_FILE_LOADBACKGROUNDTEX:
			case IDC_FILE_LOADGROUNDTEX:
				{
					const char *ptr = mxGetOpenFileName (this, 0, "*.*");
					if (ptr)
					{
						if (0 /* g_pMatSysWindow->loadTexture (ptr, event->action - IDC_FILE_LOADBACKGROUNDTEX) */)
						{
							if (event->action == IDC_FILE_LOADBACKGROUNDTEX)
								g_pControlPanel->setShowBackground (true);
							else
								g_pControlPanel->setShowGround (true);
							
						}
						else
							mxMessageBox (this, "Error loading texture.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
					}
				}
				break;
				
			case IDC_FILE_UNLOADGROUNDTEX:
				{
					// g_pMatSysWindow->loadTexture (0, 1);
					g_pControlPanel->setShowGround (false);
				}
				break;
				
			case IDC_FILE_RECENTMODELS1:
			case IDC_FILE_RECENTMODELS2:
			case IDC_FILE_RECENTMODELS3:
			case IDC_FILE_RECENTMODELS4:
				{
					int i = event->action - IDC_FILE_RECENTMODELS1;
					
					LoadModelFile( recentFiles[ i ] );
					
					char tmp[256];			
					strcpy (tmp, recentFiles[0]);
					strcpy (recentFiles[0], recentFiles[i]);
					strcpy (recentFiles[i], tmp);
					
					initRecentFiles ();
					
					redraw ();
				}
				break;
				
			case IDC_FILE_EXIT:
				{
					redraw ();
					mx::quit ();
				}
				break;
				
			case IDC_OPTIONS_COLORBACKGROUND:
			case IDC_OPTIONS_COLORGROUND:
			case IDC_OPTIONS_COLORLIGHT:
				{
					float *cols[3] = { g_viewerSettings.bgColor, g_viewerSettings.gColor, g_viewerSettings.lColor };
					float *col = cols[event->action - IDC_OPTIONS_COLORBACKGROUND];
					int r = (int) (col[0] * 255.0f);
					int g = (int) (col[1] * 255.0f);
					int b = (int) (col[2] * 255.0f);
					if (mxChooseColor (this, &r, &g, &b))
					{
						col[0] = (float) r / 255.0f;
						col[1] = (float) g / 255.0f;
						col[2] = (float) b / 255.0f;
					}
				}
				break;
				
			case IDC_OPTIONS_CENTERVIEW:
				g_pControlPanel->centerView ();
				break;
				
			case IDC_OPTIONS_CENTERONFACE:
				g_pControlPanel->CenterOnFace();
				break;
				
			case IDC_OPTIONS_MAKESCREENSHOT:
				{
					char *ptr = (char *) mxGetSaveFileName (this, "", "*.tga");
					if (ptr)
					{
						if (!strstr (ptr, ".tga"))
							strcat (ptr, ".tga");
						// g_pMatSysWindow->dumpViewport (ptr);
					}
				}
				break;
				
			case IDC_OPTIONS_DUMP:
				g_pControlPanel->dumpModelInfo ();
				break;
				
#ifdef WIN32
			case IDC_HELP_GOTOHOMEPAGE:
				ShellExecute (0, "open", "http://www.swissquake.ch/chumbalum-soft/index.html", 0, 0, SW_SHOW);
				break;
#endif
				
			case IDC_HELP_ABOUT:
				mxMessageBox (this,
					"v0.1 (c) 2001, Valve, LLC.  All rights reserved.\r\nBuild Date: "__DATE__"",
					"Valve Face Poser", 
					MX_MB_OK | MX_MB_INFORMATION);
				break;
				
			case IDC_EXPRESSIONS_REDOBITMAPS:
				{
					bool saveOverrides = g_pExpressionTrayTool->GetOverridesShowing();
					g_pExpressionTrayTool->SetOverridesShowing( false );
					CExpClass *active = expressions->GetActiveClass();
					if ( active )
					{
						for ( int i = 0; i < active->GetNumExpressions() ; i++ )
						{
							CExpression *exp = active->GetExpression( i );
							if ( !exp )
								continue;
							
							active->SelectExpression( i );
							exp->CreateNewBitmap( models->GetActiveModelIndex() );
							
							if ( ! ( i % 5 ) )
							{
								g_pExpressionTrayTool->redraw();
							}
						}
						
						if ( active->HasOverrideClass() )
						{
							
							g_pExpressionTrayTool->SetOverridesShowing( true );
							
							CExpClass *oc = active->GetOverrideClass();
							for ( int i = 0; i < oc->GetNumExpressions() ; i++ )
							{
								CExpression *exp = oc->GetExpression( i );
								if ( !exp )
									continue;
								
								oc->SelectExpression( i );
								exp->CreateNewBitmap( models->GetActiveModelIndex() );
								
								if ( ! ( i % 5 ) )
								{
									g_pExpressionTrayTool->redraw();
								}
							}
						}
						active->SelectExpression( 0 );
					}
					
					g_pExpressionTrayTool->SetOverridesShowing( saveOverrides );
				}
				break;
			case IDC_EXPRESSIONS_NEW:
				{
					const char *filename = mxGetSaveFileName( 
						this, 
						FacePoser_MakeWindowsSlashes( va( "%s/expressions/", GetGameDirectory() ) ), 
						"*.txt" );
					if ( filename && filename[ 0 ] )
					{
						char classfile[ 512 ];
						strcpy( classfile, filename );
						StripExtension( classfile );
						DefaultExtension( classfile, ".txt" );
						
						expressions->CreateNewClass( classfile );
					}
				}
				break;
			case IDC_EXPRESSIONS_LOAD:
				{
					const char *filename = NULL;
					
					filename = mxGetOpenFileName( 
						this, 
						FacePoser_MakeWindowsSlashes( va( "%s/expressions/", GetGameDirectory() ) ), 
						"*.txt" );
					if ( filename && filename[ 0 ] )
					{
						expressions->LoadClass( filename );
					}
				}
				break;
				
			case IDC_EXPRESSIONS_SAVE:
				{
					CExpClass *active = expressions->GetActiveClass();
					if ( active )
					{
						active->Save();
						active->Export();
					}
				}
				break;
			case IDC_EXPRESSIONS_EXPORT:
				{
					CExpClass *active = expressions->GetActiveClass();
					if ( active )
					{
						active->Export();
					}
				}
				break;
			case IDC_EXPRESSIONS_CLOSE:
				g_pControlPanel->Close();
				break;
			case IDC_EXPRESSIONS_CLOSEALL:
				g_pControlPanel->Closeall();
				break;
			case IDC_CHOREOSCENE_NEW:
				g_pChoreoView->New();
				break;
			case IDC_CHOREOSCENE_LOAD:
				g_pChoreoView->Load();
				break;
			case IDC_CHOREOSCENE_SAVE:
				g_pChoreoView->Save();
				break;
			case IDC_CHOREOSCENE_SAVEAS:
				g_pChoreoView->SaveAs();
				break;
			case IDC_CHOREOSCENE_CLOSE:
				g_pChoreoView->Close();
				break;
			case IDC_CHOREOSCENE_ADDACTOR:
				g_pChoreoView->NewActor();
				break;
			case IDC_WINDOW_TILE:
				{
					OnTile();
				}
				break;
			case IDC_WINDOW_TILE_HORIZ:
				{
					OnTileHorizontally();
				}
				break;
			case IDC_WINDOW_TILE_VERT:
				{
					OnTileVertically();
				}
				break;
			case IDC_WINDOW_CASCADE:
				{
					OnCascade();
				}
				break;
			case IDC_WINDOW_HIDEALL:
				{
					OnHideAll();
				}
				break;
			case IDC_WINDOW_SHOWALL:
				{
					OnShowAll();
				}
				break;
			default:
				{
					iret = 0;
					int tool_number = event->action - IDC_WINDOW_FIRSTTOOL;
					int max_tools = IDC_WINDOW_LASTTOOL - IDC_WINDOW_FIRSTTOOL;
					
					if ( tool_number >= 0 && 
						tool_number <= max_tools && 
						tool_number < IFacePoserToolWindow::GetToolCount() )
					{
						iret = 1;
						IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( tool_number );
						if ( tool )
						{
							mxWindow *toolw = tool->GetMxWindow();
							
							bool wasvisible = toolw->isVisible();
							toolw->setVisible( !wasvisible );
							
							g_MDLViewer->UpdateWindowMenu();

						}
					}
				}
				break;
			} //switch (event->action)
		} // mxEvent::Action
		break;
	case KeyDown:
		{
			g_pMatSysWindow->handleEvent(event);
			iret = 1;
		}
		break;
	case mxEvent::Activate:
		{
			if (event->action)
			{
				mx::setIdleWindow( g_pMatSysWindow );
			}
			else
			{
				mx::setIdleWindow( 0 );
			}
			iret = 1;
		}
		break;
	} // event->event
	
	return iret;
}

void MDLViewer::SaveWindowPositions( void )
{
	// Save the model viewer position
	SavePosition();

	int c = IFacePoserToolWindow::GetToolCount();
	for ( int i = 0; i < c; i++ )
	{
		IFacePoserToolWindow *w = IFacePoserToolWindow::GetTool( i );
		w->SavePosition();
	}
}

void MDLViewer::LoadWindowPositions( void )
{
	// NOTE: Don't do this here, we do the mdlviewer position earlier in startup
	// LoadPosition();

	int w = this->w();
	int h = this->h();

	g_viewerSettings.width = w;
	g_viewerSettings.height = h;

	int c = IFacePoserToolWindow::GetToolCount();
	for ( int i = 0; i < c; i++ )
	{
		IFacePoserToolWindow *w = IFacePoserToolWindow::GetTool( i );
		w->LoadPosition();
	}
}

void
MDLViewer::redraw ()
{
}

void MDLViewer::Paste( void )
{
	g_pControlPanel->Paste();
}

void MDLViewer::Copy( void )
{
	g_pControlPanel->Copy();
}

void MDLViewer::Undo( void )
{
	g_pControlPanel->Undo();
}

void MDLViewer::Redo( void )
{
	g_pControlPanel->Redo();
}

int MDLViewer::GetCurrentFrame( void )
{
	return m_nCurrentFrame;
}

void MDLViewer::Think( float dt )
{
	++m_nCurrentFrame;

	// Iterate across tools
	IFacePoserToolWindow::ToolThink( dt );

	sound->Update( dt );
}

static int CountVisibleTools( void )
{
	int i;
	int c = IFacePoserToolWindow::GetToolCount();
	int viscount = 0;

	for ( i = 0; i < c; i++ )
	{
		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
		mxWindow *w = tool->GetMxWindow();
		if ( !w->isVisible() )
			continue;

		viscount++;
	}

	return viscount;
}

void MDLViewer::OnCascade()
{
	int i;
	int c = IFacePoserToolWindow::GetToolCount();
	int viscount = CountVisibleTools();

	int x = 0, y = 0;

	int offset = 20;

	int wide = workspace->w2() - viscount * offset;
	int tall = ( workspace->h2() - viscount * offset ) / 2;

	for ( i = 0; i < c; i++ )
	{
		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
		mxWindow *w = tool->GetMxWindow();
		if ( !w->isVisible() )
			continue;

		w->setBounds( x, y, wide, tall );
		x += offset;
		y += offset;
	}
}

void MDLViewer::OnTile()
{
	int c = CountVisibleTools();

	int rows = (int)sqrt( c );
	rows = clamp( rows, 1, rows );

	int cols  = 1;
	while ( rows * cols < c )
	{
		cols++;
	}

	DoTile( rows, cols );
}

void MDLViewer::OnTileHorizontally()
{
	int c = CountVisibleTools();

	DoTile( c, 1 );
}

void MDLViewer::OnTileVertically()
{
	int c = CountVisibleTools();

	DoTile( 1, c );
}

void MDLViewer::OnHideAll()
{
	int c = IFacePoserToolWindow::GetToolCount();
	for ( int i = 0; i < c; i++ )
	{
		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
		mxWindow *w = tool->GetMxWindow();

		w->setVisible( false );
	}

	UpdateWindowMenu();
}

void MDLViewer::OnShowAll()
{
	int c = IFacePoserToolWindow::GetToolCount();
	for ( int i = 0; i < c; i++ )
	{
		IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( i );
		mxWindow *w = tool->GetMxWindow();

		w->setVisible( true );
	}

	UpdateWindowMenu();
}

void MDLViewer::DoTile( int x, int y )
{
	int c = IFacePoserToolWindow::GetToolCount();

	int vis = CountVisibleTools();

	if ( x < 1 )
		x = 1;
	if ( y < 1 )
		y = 1;

	int wide = workspace->w2() / y;
	int tall = workspace->h2() / x;

	int obj = 0;

	for ( int row = 0 ; row < x ; row++ )
	{
		for  ( int col = 0; col < y; col++ )
		{
			bool found = false;
			while ( 1 )
			{
				if ( obj >= c )
					break;

				IFacePoserToolWindow *tool = IFacePoserToolWindow::GetTool( obj++ );
				mxWindow *w = tool->GetMxWindow();
				if ( w->isVisible() )
				{
					w->setBounds( col * wide, row * tall, wide, tall );

					found = true;
					break;
				}
			}

			if ( !found )
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Not used by faceposer
// Output : int
//-----------------------------------------------------------------------------
int MDLViewer::GetCurrentHitboxSet(void)
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool MDLViewer::PaintBackground( void )
{
	CChoreoWidgetDrawHelper drawHelper( this );

	RECT rc;
	drawHelper.GetClientRect( rc );

	drawHelper.DrawFilledRect( COLOR_CHOREO_BACKGROUND, rc );
	return false;
}


SpewRetval_t HLFacePoserSpewFunc( SpewType_t spewType, char const *pMsg )
{
	switch (spewType)
	{
	case SPEW_ERROR:
		MessageBox(NULL, pMsg, "FATAL ERROR", MB_OK);
		return SPEW_ABORT;

	case SPEW_LOG:
		return SPEW_CONTINUE;

	case SPEW_WARNING:
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, pMsg );
		return SPEW_CONTINUE;

	default:
		Con_Printf(pMsg);
#ifdef _DEBUG
		return spewType == SPEW_ASSERT ? SPEW_DEBUGGER : SPEW_CONTINUE;
#else
		return SPEW_CONTINUE;
#endif
	}
}


char cmdline[1024] = "";

main (int argc, char *argv[])
{
	int i;
	CommandLine()->CreateCmdLine( argc, argv );
	SpewOutputFunc( HLFacePoserSpewFunc );
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	CoInitialize(NULL);

	// make sure, we start in the right directory
	char szName[256];
	strcpy (szName, mx::getApplicationPath() );

	mx::init (argc, argv);

	sound->Init();

	FileSystem_Init( true );
	filesystem = (IFileSystem *)(FileSystem_GetFactory()( FILESYSTEM_INTERFACE_VERSION, NULL ));
	if ( !filesystem )
	{
		AssertMsg( 0, "Failed to create/get IFileSystem" );
		return 1;
	}

	char workingdir[ 256 ];
	workingdir[0] = 0;
	Q_getwd( workingdir );

	// If they didn't specify -game on the command line, use VPROJECT.
	CmdLib_InitFileSystem( workingdir, true );

	IFacePoserToolWindow::EnableToolRedraw( false );

	new MDLViewer ();

	g_MDLViewer->setMenuBar (g_MDLViewer->getMenuBar ());
	
	g_pStudioModel->Init();

	bool modelloaded = false;
	for ( i = 1; i < CommandLine()->ParmCount(); i++ )
	{
		if ( Q_stristr (CommandLine()->GetParm( i ), ".mdl") )
		{
			modelloaded = true;
			g_MDLViewer->LoadModelFile( CommandLine()->GetParm( i ) );
			break;
		}
	}

	models->LoadModelList();

	if ( models->Count() == 0 )
	{
		g_pFlexPanel->initFlexes( );
	}

	// Load expressions from last time
	int files = workspacefiles->GetNumStoredFiles( IWorkspaceFiles::EXPRESSION );
	for ( i = 0; i < files; i++ )
	{
		expressions->LoadClass( workspacefiles->GetStoredFile( IWorkspaceFiles::EXPRESSION, i ) );
	}

	IFacePoserToolWindow::EnableToolRedraw( true );

	int retval = mx::run();

	soundemitter->BaseShutdown();

	if (g_pStudioModel)
	{
		g_pStudioModel->Shutdown();
		g_pStudioModel = NULL;
	}

	if (g_pMaterialSystem)
	{
		g_pMaterialSystem->Shutdown();
		g_pMaterialSystem = NULL;
	}

	FileSystem_Term();

	CoUninitialize();

	return retval;
}
