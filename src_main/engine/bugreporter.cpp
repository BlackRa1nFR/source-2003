//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#undef PROTECT_FILEIO_FUNCTIONS
#undef fopen

#include <windows.h>
#include <time.h>

#include "glquake.h"

#include "vid.h"
#include <vgui_controls/Frame.h>
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
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/TextEntry.h>
#include "enginebugreporter.h"

#include "utlsymbol.h"
#include "utldict.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "icliententitylist.h"
#include "bugreporter/bugreporter.h"
#include "icliententity.h"
#include "tier0/vcrmode.h"

#define DENY_SOUND		"common/bugreporter_failed"
#define SUCCEED_SOUND	"common/bugreporter_succeeded"

// Fixme, move these to buguiddata.res script file?
#define BUG_REPOSITORY_URL "\\\\ftknox\\pub\\scratch\\bugs"
#define REPOSITORY_VALIDATION_FILE "info.txt"

#define BUG_REPORTER_DLLNAME "bugreporter.dll" 

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBugUIPanel : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	CBugUIPanel( vgui::Panel *parent );
	~CBugUIPanel();

	virtual void	OnTick();

	// Command issued
	virtual void	OnCommand(const char *command);

	virtual void	SetVisible( bool visible );

	void			Init();
	void			Shutdown();

	virtual void	OnKeyCodeTyped(KeyCode code);

	void			ParseCommands();
//	DECLARE_PANELMAP();

protected:
	virtual void OnClose();

	vgui::TextEntry *m_pTitle;
	vgui::TextEntry *m_pDescription;

	vgui::Button *m_pTakeShot;
	vgui::Button *m_pSaveGame;
	vgui::Button *m_pSaveBSP;
	vgui::Button *m_pSaveVMF;

	vgui::Label  *m_pScreenShotURL;
	vgui::Label	 *m_pSaveGameURL;
	vgui::Label	 *m_pBSPURL;
	vgui::Label	 *m_pVMFURL;
	vgui::Label  *m_pPosition;
	vgui::Label	 *m_pOrientation;
	vgui::Label  *m_pLevelName;
	vgui::Label  *m_pBuildNumber;

	vgui::Label		*m_pSubmitter;

	vgui::ComboBox *m_pAssignTo;
	vgui::ComboBox *m_pSeverity;
	vgui::ComboBox *m_pReportType;
	vgui::ComboBox *m_pPriority;
	vgui::ComboBox *m_pGameArea;

	vgui::Button *m_pSubmit;
	vgui::Button *m_pCancel;
	vgui::Button *m_pClearForm;

	IBugReporter				*m_pBugReporter;
	CSysModule					*m_hBugReporter;

private:

	void						DetermineSubmitterName();

	void						PopulateControls();

	void						OnTakeSnapshot();
	void						OnSaveGame();
	void						OnSaveBSP();
	void						OnSaveVMF();
	void						OnSubmit();
	void						OnClearForm();

	bool						IsValidSubmission( bool verbose );
	void						WipeData();

	void						GetDataFileBase( char const *suffix, char *buf, int bufsize );
	bool						UploadBugSubmission( 
									char const *levelname,
									int			bugId,

									char const *savefile, 
									char const *screenshot,
									char const *bsp,
									char const *vmf );
	bool						UploadFile( char const *local, char const *remote );

	void						DenySound();
	void						SuccessSound();

	bool						AutoFillToken( char const *token, bool partial );
	bool						Compare( char const *token, char const *value, bool partial );

	char const					*GetSubmitter();

	bool						m_bCanSubmit;
	bool						m_bValidated;

	char						m_szScreenShotName[ 256 ];
	char						m_szSaveGameName[ 256 ];
	char						m_szBSPName[ 256 ];
	char						m_szVMFName[ 256 ];

	bool						m_bTakingSnapshot;
	int							m_nSnapShotFrame;
};

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CBugUIPanel::CBugUIPanel( vgui::Panel *parent ) : 
	BaseClass( parent, "BugUIPanel")
{
	m_pBugReporter = NULL;
	m_hBugReporter = 0;

	m_bValidated = false;
	m_szScreenShotName[ 0 ] = 0;
	m_szSaveGameName[ 0 ] = 0;
	m_szBSPName[ 0 ] = 0;
	m_szVMFName[ 0 ] = 0;

	m_nSnapShotFrame = -1;
	m_bTakingSnapshot = false;

	m_bCanSubmit = false;

	int w = 600;
	int h = 400;

	int x = ( vid.width - w ) / 2;
	int y = ( vid.height - h ) / 2;

	SetTitle("Bug Reporter", true);

	m_pTitle = new vgui::TextEntry( this, "BugTitle" );
	m_pDescription = new vgui::TextEntry( this, "BugDescription" );
	m_pDescription->SetMultiline( true );
	m_pDescription->SetCatchEnterKey( true );
	m_pDescription->SetVerticalScrollbar( true );

	m_pScreenShotURL = new vgui::Label( this, "BugScreenShotURL", "" );
	m_pSaveGameURL = new vgui::Label( this, "BugSaveGameURL", "" );
	m_pBSPURL = new vgui::Label( this, "BugBSPURL", "" );
	m_pVMFURL = new vgui::Label( this, "BugVMFURL", "" );

	m_pTakeShot = new vgui::Button( this, "BugTakeShot", "Take shot" );
	m_pSaveGame = new vgui::Button( this, "BugSaveGame", "Save game" );
	m_pSaveBSP = new vgui::Button( this, "BugSaveBSP", "Include .bsp" );
	m_pSaveVMF = new vgui::Button( this, "BugSaveVMF", "Include .vmf" );

	m_pPosition = new vgui::Label( this, "BugPosition", "" );
	m_pOrientation = new vgui::Label( this, "BugOrientation", "" );
	m_pLevelName = new vgui::Label( this, "BugLevel", "" );
	m_pBuildNumber = new vgui::Label( this, "BugBuild", "" );

	m_pSubmitter = new Label(this, "BugSubmitter", "" );
	m_pAssignTo = new ComboBox(this, "BugOwner", 10, false);
	m_pSeverity = new ComboBox(this, "BugSeverity", 10, false);
	m_pReportType = new ComboBox(this, "BugReportType", 10, false);
	m_pPriority = new ComboBox(this, "BugPriority", 10, false);
	m_pGameArea = new ComboBox(this, "BugArea", 10, false);

	m_pSubmit = new vgui::Button( this, "BugSubmit", "Submit" );
	m_pCancel = new vgui::Button( this, "BugCancel", "Cancel" );
	m_pClearForm = new vgui::Button( this, "BugClearForm", "Clear Form" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	LoadControlSettings("Resource\\BugUIPanel.res");

	m_pBuildNumber->SetText( va( "Build # %d", build_number() ) );


	// Hidden by default
	SetVisible( false );

	SetSizeable( false );
	SetMoveable( true );

	SetBounds( x, y, w, h );
}

void CBugUIPanel::Init()
{
	Color clr( 50, 100, 255, 255 );

	Assert( !m_pBugReporter );

	m_hBugReporter = Sys_LoadModule( BUG_REPORTER_DLLNAME );
	if ( m_hBugReporter )
	{
		CreateInterfaceFn factory = Sys_GetFactory( m_hBugReporter );
		if ( factory )
		{
			m_pBugReporter = (IBugReporter *)factory( INTERFACEVERSION_BUGREPORTER, NULL );
			if( m_pBugReporter )
			{
				if ( m_pBugReporter->Init() )
				{
					m_bCanSubmit = true;
				}
				else
				{
					m_pBugReporter = NULL;
					Con_ColorPrintf( clr, "m_pBugReporter->Init() failed\n" );
				}
			}
			else
			{
				Con_ColorPrintf( clr, "Couldn't get interface '%s' from '%s'\n", INTERFACEVERSION_BUGREPORTER, BUG_REPORTER_DLLNAME );
			}
		}
		else
		{
			Con_ColorPrintf( clr, "Couldn't get factory '%s'\n", BUG_REPORTER_DLLNAME );
		}
	}
	else
	{
		Con_ColorPrintf( clr, "Couldn't load '%s'\n", BUG_REPORTER_DLLNAME );
	}

	if ( m_bCanSubmit )
	{
		PopulateControls();
	}
}

void CBugUIPanel::Shutdown()
{
	if ( m_pBugReporter )
	{
		m_pBugReporter->Shutdown();
	}

	m_pBugReporter = NULL;
	if ( m_hBugReporter )
	{
		Sys_UnloadModule( m_hBugReporter );
		m_hBugReporter = 0;
	}
}
/*
MessageMapItem_t CBugUIPanel::m_MessageMap[] =
{
	MAP_MESSAGE_CONSTCHARPTR( CBugUIPanel, "FileSelected", OnFileSelected, "fullpath" ),   
};

IMPLEMENT_PANELMAP(CDemoUIPanel, BaseClass );
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBugUIPanel::~CBugUIPanel()
{

}

void CBugUIPanel::OnTick()
{
	BaseClass::OnTick();

	if ( !IsVisible() )
	{
		if ( !m_bTakingSnapshot )
			return;

		if ( host_framecount < m_nSnapShotFrame + 2 )
			return;;

		m_bTakingSnapshot = false;
		m_nSnapShotFrame = -1;

		m_pScreenShotURL->SetText( m_szScreenShotName );

		SetVisible( true );
	}

	if ( !m_bCanSubmit )
	{
		Warning( "Bug UI disabled:  Couldn't log in to PVCS Tracker\n",
			BUG_REPOSITORY_URL );
		SetVisible( false );
		return;
	}

	if ( cls.state == ca_active )
	{
		Vector org = r_origin;
		QAngle ang;
		VectorAngles( vpn, ang );

		IClientEntity *localPlayer = entitylist->GetClientEntity( cl.playernum + 1 );
		if ( localPlayer )
		{
			org = localPlayer->GetAbsOrigin();
			// ang = localPlayer->GetAbsAngles();
		}

		m_pPosition->SetText( va( "%.2f %.2f %.2f", org.x, org.y, org.z ) );
		m_pOrientation->SetText( va( "%.2f %.2f %.2f", ang.x, ang.y, ang.z ) );

		char lvl[ 256 ];
		COM_FileBase( cl.levelname, lvl );
		m_pLevelName->SetText( lvl );
		m_pSaveGame->SetEnabled( true );
		m_pSaveBSP->SetEnabled( true );
		m_pSaveVMF->SetEnabled( true );
	}
	else
	{
		m_pPosition->SetText( "console" );
		m_pOrientation->SetText( "console" );
		m_pLevelName->SetText( "console" );
		m_pSaveGame->SetEnabled( false );
		m_pSaveBSP->SetEnabled( false );
		m_pSaveVMF->SetEnabled( false );
	}

	m_pSubmit->SetEnabled( IsValidSubmission( false ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *suffix - 
//			*buf - 
//			bufsize - 
//-----------------------------------------------------------------------------
void CBugUIPanel::GetDataFileBase( char const *suffix, char *buf, int bufsize )
{
	struct tm t;
	g_pVCR->Hook_LocalTime( &t );

	char who[ 128 ];
	Q_strncpy( who, suffix, sizeof( who ) );
	_strlwr( who );

	Q_snprintf( buf, bufsize, "%i_%02i_%02i_%02i_%02i_%02i_%s",
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 
		t.tm_hour, t.tm_min, t.tm_sec, 
		who );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBugUIPanel::OnTakeSnapshot()
{
	GetDataFileBase( GetSubmitter(), m_szScreenShotName, sizeof( m_szScreenShotName ) );

	m_nSnapShotFrame = host_framecount;
	m_bTakingSnapshot = true;

	SetVisible( false );

	Cbuf_AddText( va( "screenshot %s\n", m_szScreenShotName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBugUIPanel::OnSaveGame()
{
	GetDataFileBase( GetSubmitter(), m_szSaveGameName, sizeof( m_szSaveGameName ) );

	Cbuf_AddText( va( "save %s\n", m_szSaveGameName ) );

	m_pSaveGameURL->SetText( m_szSaveGameName );
}

void CBugUIPanel::OnSaveBSP()
{
	GetDataFileBase( GetSubmitter(), m_szBSPName, sizeof( m_szBSPName ) );
	m_pBSPURL->SetText( m_szBSPName );
}

void CBugUIPanel::OnSaveVMF()
{
	GetDataFileBase( GetSubmitter(), m_szVMFName, sizeof( m_szVMFName ) );
	m_pVMFURL->SetText( m_szVMFName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBugUIPanel::OnClose()
{
	SetVisible( false );
	return;
}

void CBugUIPanel::SetVisible( bool visible )
{
	BaseClass::SetVisible( visible );

	if ( visible )
	{
		if ( !m_bValidated )
		{
			m_bValidated = true;
			Init();
			DetermineSubmitterName();
		}
	}
}

void CBugUIPanel::WipeData()
{
	m_szScreenShotName[ 0 ] = 0;
	m_pScreenShotURL->SetText( "Screenshot file" );
	m_szSaveGameName[ 0 ] = 0;
	m_pSaveGameURL->SetText( "Save game file" );
	m_szBSPName[ 0 ] = 0;
	m_pBSPURL->SetText( ".bsp file" );
	m_szVMFName[ 0 ] = 0;
	m_pVMFURL->SetText( ".vmf file" );

//	m_pTitle->SetText( "" );
	m_pDescription->SetText( "" );
}

bool CBugUIPanel::IsValidSubmission( bool verbose )
{
	if ( !m_pBugReporter )
		return false;

	char title[ 256 ];
	char desc[ 4096 ];

	title[ 0 ] = 0;
	desc[ 0 ] = 0;

	m_pTitle->GetText( title, sizeof( title ) );
	if ( !title[ 0 ] )
	{
		if ( verbose ) 
		{
			Warning( "Bug must have a title\n" );
		}
		return false;
	}

	m_pDescription->GetText( desc, sizeof( desc ) );
	if ( !desc[ 0 ] )
	{
		if ( verbose ) 
		{
			Warning( "Bug must have a description\n" );
		}
		return false;
	}

	if ( m_pSeverity->GetActiveItem() < 0 )
	{
		if ( verbose ) 
		{
			Warning( "Severity not set!\n" );
		}
		return false;
	}

	if ( m_pAssignTo->GetActiveItem() < 0 )
	{
		if ( verbose ) 
		{
			Warning( "Owner not set!\n" );
		}
		return false;
	}

	char owner[ 256 ];
	Q_strncpy( owner, m_pBugReporter->GetDisplayName( m_pAssignTo->GetActiveItem() ), sizeof( owner ) );
	if ( !Q_stricmp( owner, "<<Unassigned>>" ) )
	{
		if ( verbose ) 
		{
			Warning( "Owner not set!\n" );
		}
		return false;
	}

	if ( m_pPriority->GetActiveItem() < 0 )
	{
		if ( verbose ) 
		{
			Warning( "Priority not set!\n" );
		}
		return false;
	}

	if ( m_pReportType->GetActiveItem() < 0 )
	{
		if ( verbose ) 
		{
			Warning( "Report Type not set!\n" );
		}
		return false;
	}

	if ( m_pGameArea->GetActiveItem() < 0 )
	{
		if ( verbose ) 
		{
			Warning( "Area not set!\n" );
		}
		return false;
	}

	return true;
}

void CBugUIPanel::OnSubmit()
{
	if ( !m_bCanSubmit )
	{
		Warning( "Can't submit bugs, either couldn't find PVCS tracker username, or can't write to %s\n",
			BUG_REPOSITORY_URL );
		return;
	}

	bool success = true;

	if ( !IsValidSubmission( true ) )
	{
		// Play deny sound
		DenySound();
		return;
	}

	char title[ 256 ];
	char desc[ 4096 ];
	char severity[ 128 ];
	char area[ 128 ];
	char priority[ 128 ];
	char assignedto[ 128 ];
	char level[ 128 ];
	char position[ 128 ];
	char orientation[ 128 ];
	char build[ 128 ];
	char reporttype[ 128 ];

	title[ 0 ] = 0;
	desc[ 0 ] = 0;
	severity[ 0 ] = 0;
	area[ 0 ] = 0;
	priority[ 0 ] = 0;
	assignedto[ 0 ] = 0;
	level[ 0 ] = 0;
	orientation[ 0 ] = 0;
	position[ 0 ] = 0;
	build[ 0 ] = 0;
	reporttype [ 0 ] = 0;

	m_pTitle->GetText( title, sizeof( title ) );

	Msg( "title:  %s\n", title );

	m_pDescription->GetText( desc, sizeof( desc ) );

	Msg( "description:  %s\n", desc );

	m_pLevelName->GetText( level, sizeof( level ) );
	m_pPosition->GetText( position, sizeof( position ) );
	m_pOrientation->GetText( orientation, sizeof( orientation ) );
	m_pBuildNumber->GetText( build, sizeof( build ) );

	m_pSeverity->GetText( severity, sizeof( severity ) );
	Msg( "severity %s\n", severity );

	m_pGameArea->GetText( area, sizeof( area ) );
	Msg( "area %s\n", area );

	m_pPriority->GetText( priority, sizeof( priority ) );
	Msg( "priority %s\n", priority );

	m_pAssignTo->GetText( assignedto, sizeof( assignedto ) );
	Msg( "owner %s\n", assignedto );

	m_pReportType->GetText( reporttype, sizeof( reporttype ) );
	Msg( "report type %s\n", reporttype );

	Msg( "submitter %s\n", m_pBugReporter->GetUserName() );

	Msg( "level %s\n", level );
	Msg( "position %s\n", position );
	Msg( "orientation %s\n", orientation );
	Msg( "build %s\n", build );
	
	if ( m_szSaveGameName[ 0 ] )
	{
		Msg( "save file save/%s.sav\n", m_szSaveGameName );
	}
	else
	{
		Msg( "no save game\n" );
	}

	if ( m_szScreenShotName[ 0 ] )
	{
		Msg( "screenshot screenshots/%s.tga\n", m_szScreenShotName );
	}
	else
	{
		Msg( "no screenshot\n" );
	}

	if ( m_szBSPName[ 0 ] )
	{
		Msg( "bsp file maps/%s.bsp\n", m_szBSPName );
	}

	if ( m_szVMFName[ 0 ] )
	{
		Msg( "vmf file maps/%s.vmf\n", m_szVMFName );
	}

	Assert( m_pBugReporter );

	// Stuff bug data files up to server
	m_pBugReporter->StartNewBugReport();

	m_pBugReporter->SetTitle( title );
	m_pBugReporter->SetDescription( desc );
	m_pBugReporter->SetLevel( level );
	m_pBugReporter->SetPosition( position );
	m_pBugReporter->SetOrientation( orientation );
	m_pBugReporter->SetBuildNumber( build );
	m_pBugReporter->SetSubmitter( NULL );
	m_pBugReporter->SetOwner( m_pBugReporter->GetUserNameForDisplayName( assignedto ) );
	m_pBugReporter->SetSeverity( severity );
	m_pBugReporter->SetPriority( priority );
	m_pBugReporter->SetArea( area );
	m_pBugReporter->SetReportType( reporttype );

	char fn[ 512 ];
	if ( m_szSaveGameName[ 0 ] )
	{
		Q_snprintf( fn, sizeof( fn ), "%s/%s.sav", BUG_REPOSITORY_URL, m_szSaveGameName );
		COM_FixSlashes( fn );
		m_pBugReporter->SetSaveGame( fn );
	}
	if ( m_szScreenShotName[ 0 ] )
	{
		Q_snprintf( fn, sizeof( fn ), "%s/%s.tga", BUG_REPOSITORY_URL, m_szScreenShotName );
		COM_FixSlashes( fn );
		m_pBugReporter->SetScreenShot( fn );
	}
	if ( m_szBSPName[ 0 ] )
	{
		Q_snprintf( fn, sizeof( fn ), "%s/%s.bsp", BUG_REPOSITORY_URL, m_szBSPName );
		COM_FixSlashes( fn );
		m_pBugReporter->SetBSPName( fn );
	}
	if ( m_szVMFName[ 0 ] )
	{
		Q_snprintf( fn, sizeof( fn ), "%s/%s.vmf", BUG_REPOSITORY_URL, m_szVMFName );
		COM_FixSlashes( fn );
		m_pBugReporter->SetVMFName( fn );
	}

	int bugId = -1;

	success = m_pBugReporter->CommitBugReport( bugId );
	if ( success )
	{
		if ( !UploadBugSubmission( level, bugId, m_szSaveGameName, m_szScreenShotName, m_szBSPName, m_szVMFName ) )
		{
			Warning( "Unable to upload saved game and screenshot to bug repository!\n" );
			success = false;
		}
	}
	else
	{
		Warning( "Unable to post bug report to database\n" );
	}

	if ( !success )
	{
		// Play deny sound
		DenySound();
		return;
	}

	// Close
	OnClose();
	WipeData();
	SuccessSound();
}

bool CBugUIPanel::UploadFile( char const *local, char const *remote )
{
	Msg( "Uploading %s to %s\n", local, remote );

	FILE *l, *r;

	l = fopen( local, "rb" );
	if ( !l )
	{
		Warning( "CBugUIPanel::UploadFile:  Unable to open local path '%s'\n", local );
		return false;
	}

	r = fopen( remote, "wb" );
	if ( !r )
	{
		Warning( "CBugUIPanel::UploadFile:  Unable to open remote path '%s'\n", remote );
		fclose( l );
		return false;
	}

	int size;
	fseek( l, 0, SEEK_END );
	size = ftell( l );
	fseek( l, 0, SEEK_SET );

	int remaining = size;

	while ( remaining > 0 )
	{
		char copybuf[ 4096 ];
		int copyamount = min( remaining, sizeof( copybuf ) );

		fread( copybuf, copyamount, 1, l );
		fwrite( copybuf, copyamount, 1, r );

		remaining -= copyamount;
	}

	fclose( r );
	fclose( l );

	return true;
}

static char const *TranslateBugDestinationPath( int bugId, char const *fn )
{
	return fn;
	/*
	if ( bugId == -1 )
		return fn;

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", bugId );
	return sz;
	*/
}

bool CBugUIPanel::UploadBugSubmission( char const *levelname, int bugId, char const *savefile, char const *screenshot, char const *bsp, char const *vmf )
{
	bool bret = true;

	char localfile[ 512 ];
	char remotefile[ 512 ];

	if ( savefile && savefile[ 0 ] )
	{
		Q_snprintf( localfile, sizeof( localfile ), "%s/save/%s.sav", com_gamedir, savefile );
		Q_snprintf( remotefile, sizeof( remotefile ), "%s/%s.sav", BUG_REPOSITORY_URL, TranslateBugDestinationPath( bugId, savefile ) );
		COM_FixSlashes( localfile );
		COM_FixSlashes( remotefile );

		if ( !UploadFile( localfile, remotefile ) )
			bret = false;
	}

	if ( screenshot && screenshot[ 0 ] )
	{
		Q_snprintf( localfile, sizeof( localfile ), "%s/screenshots/%s.tga", com_gamedir, screenshot );
		Q_snprintf( remotefile, sizeof( remotefile ), "%s/%s.tga", BUG_REPOSITORY_URL, TranslateBugDestinationPath( bugId, screenshot ) );
		COM_FixSlashes( localfile );
		COM_FixSlashes( remotefile );
		
		if ( !UploadFile( localfile, remotefile ) )
			bret = false;
	}

	if ( bsp && bsp[ 0 ] )
	{
		Q_snprintf( localfile, sizeof( localfile ), "%s/maps/%s.bsp", com_gamedir, levelname );
		Q_snprintf( remotefile, sizeof( remotefile ), "%s/%s.bsp", BUG_REPOSITORY_URL, TranslateBugDestinationPath( bugId, bsp ) );
		COM_FixSlashes( localfile );
		COM_FixSlashes( remotefile );
		
		if ( !UploadFile( localfile, remotefile ) )
			bret = false;
	}

	if ( vmf && vmf[ 0 ] )
	{
		Q_snprintf( localfile, sizeof( localfile ), "%s/maps/%s.vmf", com_gamedir, levelname );
		Q_snprintf( remotefile, sizeof( remotefile ), "%s/%s.vmf", BUG_REPOSITORY_URL, TranslateBugDestinationPath( bugId, vmf ) );
		COM_FixSlashes( localfile );
		COM_FixSlashes( remotefile );
		
		if ( !UploadFile( localfile, remotefile ) )
			bret = false;
	}
	return true;
}

void CBugUIPanel::OnCommand( char const *command )
{
	if ( !Q_strcasecmp( command, "submit" ) )
	{
		OnSubmit();
	}
	else if ( !Q_strcasecmp( command, "cancel" ) )
	{
		OnClose();
		WipeData();
	}
	else if ( !Q_strcasecmp( command, "snapshot" ) )
	{
		OnTakeSnapshot();
	}
	else if ( !Q_strcasecmp( command, "savegame" ) )
	{
		OnSaveGame();
	}
	else if ( !Q_strcasecmp( command, "savebsp" ) )
	{
		OnSaveBSP();
	}
	else if ( !Q_strcasecmp( command, "savevmf" ) )
	{
		OnSaveVMF();
	}
	else if ( !Q_strcasecmp( command, "clearform" ) )
	{
		OnClearForm();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CBugUIPanel::OnClearForm()
{
	WipeData();

	m_pTitle->SetText( "" );
	m_pDescription->SetText( "" );

	m_pAssignTo->ActivateItem( 0 );
	m_pSeverity->ActivateItem( 0 );
	m_pReportType->ActivateItem( 0 );
	m_pPriority->ActivateItem( 0 );
	m_pGameArea->ActivateItem( 0 );
}

void CBugUIPanel::DetermineSubmitterName()
{
	if ( !m_pBugReporter )
		return;

	Color clr( 100, 200, 255, 255 );
	Con_ColorPrintf( clr, "PVCS Tracker Username '%s' -- '%s'\n", m_pBugReporter->GetUserName(), m_pBugReporter->GetUserName_Display() );

	// See if we can see the bug repository right now
	char fn[ 512 ];
	Q_snprintf( fn, sizeof( fn ), "%s/%s", BUG_REPOSITORY_URL, REPOSITORY_VALIDATION_FILE );
	COM_FixSlashes( fn );

	FILE *fp = fopen( fn, "rb" );
	if ( fp )
	{
		Con_ColorPrintf( clr, "PVCS Repository '%s'\n", BUG_REPOSITORY_URL );
		fclose( fp );
	}
	else
	{
		Warning( "Unable to see '%s', check permissions and network connectivity\n", fn );
		m_bCanSubmit = false;
	}

	m_pSubmitter->SetText( m_pBugReporter->GetUserName_Display() );
}

void CBugUIPanel::PopulateControls()
{
	if ( !m_pBugReporter )
		return;

	m_pAssignTo->DeleteAllItems();
	int i;
	int c = m_pBugReporter->GetDisplayNameCount();
	for ( i = 0; i < c; i++ )
	{
		m_pAssignTo->AddItem( m_pBugReporter->GetDisplayName( i ), NULL );
	}
	m_pAssignTo->ActivateItem( 0 );
	
	m_pSeverity->DeleteAllItems();
	c = m_pBugReporter->GetSeverityCount();
	for ( i = 0; i < c; i++ )
	{
		m_pSeverity->AddItem( m_pBugReporter->GetSeverity( i ), NULL );
	}
	m_pSeverity->ActivateItem( 0 );

	m_pReportType->DeleteAllItems();
	c = m_pBugReporter->GetReportTypeCount();
	for ( i = 0; i < c; i++ )
	{
		m_pReportType->AddItem( m_pBugReporter->GetReportType( i ), NULL );
	}
	m_pReportType->ActivateItem( 0 );

	m_pPriority->DeleteAllItems();
	c = m_pBugReporter->GetPriorityCount();
	for ( i = 0; i < c; i++ )
	{
		m_pPriority->AddItem( m_pBugReporter->GetPriority( i ), NULL );
	}
	m_pPriority->ActivateItem( 0 );

	m_pGameArea->DeleteAllItems();
	c = m_pBugReporter->GetAreaCount();
	for ( i = 0; i < c; i++ )
	{
		m_pGameArea->AddItem( m_pBugReporter->GetArea( i ), NULL );
	}
	m_pGameArea->ActivateItem( 0 );
}

char const *CBugUIPanel::GetSubmitter()
{
	if ( !m_bCanSubmit )
		return "";
	return m_pBugReporter->GetUserName();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBugUIPanel::DenySound()
{
	Cbuf_AddText( va( "play %s\n", DENY_SOUND ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBugUIPanel::SuccessSound()
{
	Color clr( 50, 255, 100, 255 );

	Con_ColorPrintf( clr, "Bug submission succeeded\n" );
	Cbuf_AddText( va( "play %s\n", SUCCEED_SOUND ) );
}

void CBugUIPanel::OnKeyCodeTyped(KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		OnClose();
		WipeData();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

void CBugUIPanel::ParseCommands()
{
	if ( !m_bCanSubmit )
		return;

	for ( int i = 1; i < Cmd_Argc(); i++ )
	{
		bool success = AutoFillToken( Cmd_Argv( i ), false );
		if ( !success )
		{
			// Try partials
			success = AutoFillToken( Cmd_Argv( i ), true );
			if ( !success )
			{
				Msg( "Unable to determine where to set bug parameter '%s', ignoring...\n", Cmd_Argv( i ) );
			}
		}
	}
}

bool CBugUIPanel::Compare( char const *value, char const *token, bool partial )
{
	if ( !partial )
	{
		if ( !Q_stricmp( value, token ) )
			return true;
	}
	else
	{
		if ( Q_stristr( value, token ) )
		{
			return true;
		}
	}

	return false;
}


bool CBugUIPanel::AutoFillToken( char const *token, bool partial )
{
	// Search for token in all dropdown lists and fill it in if we find it
	if ( !m_pBugReporter )
		return true;

	int i;
	int c;
	
	c = m_pBugReporter->GetDisplayNameCount();
	for ( i = 0; i < c; i++ )
	{
		if ( Compare( m_pBugReporter->GetDisplayName( i ), token, partial ) )
		{
			m_pAssignTo->ActivateItem( i );
			return true;
		}
	}
	
	c = m_pBugReporter->GetSeverityCount();
	for ( i = 0; i < c; i++ )
	{
		if ( Compare( m_pBugReporter->GetSeverity( i ), token, partial ) )
		{
			m_pSeverity->ActivateItem( i );
			return true;
		}
	}

	c = m_pBugReporter->GetReportTypeCount();
	for ( i = 0; i < c; i++ )
	{
		if ( Compare( m_pBugReporter->GetReportType( i ), token, partial ) )
		{
			m_pReportType->ActivateItem( i );
			return true;
		}
	}

	c = m_pBugReporter->GetPriorityCount();
	for ( i = 0; i < c; i++ )
	{
		if ( Compare( m_pBugReporter->GetPriority( i ), token, partial ) )
		{
			m_pPriority->ActivateItem( i );
			return true;
		}
	}

	c = m_pBugReporter->GetAreaCount();
	for ( i = 0; i < c; i++ )
	{
		if ( Compare( m_pBugReporter->GetArea( i ), token, partial ) )
		{
			m_pGameArea->ActivateItem( i );
			return true;
		}
	}

	return false;
}

static CBugUIPanel *g_pBugUI = NULL;

class CEngineBugReporter : public IEngineBugReporter
{
public:
	virtual void		Init( void );
	virtual void		Shutdown( void );

	virtual void		InstallBugReportingUI( vgui::Panel *parent );
	virtual bool		ShouldPause() const;
};

static CEngineBugReporter g_BugReporter;
IEngineBugReporter *bugreporter = &g_BugReporter;

void CEngineBugReporter::Init( void )
{
}

void CEngineBugReporter::Shutdown( void )
{
	if ( g_pBugUI )
	{
		g_pBugUI->Shutdown();
	}
}

void CEngineBugReporter::InstallBugReportingUI( vgui::Panel *parent )
{
	if ( g_pBugUI )
		return;

	g_pBugUI = new CBugUIPanel( parent );
	Assert( g_pBugUI );
}

bool CEngineBugReporter::ShouldPause() const
{
	return g_pBugUI && g_pBugUI->IsVisible() && (cl.maxclients == 1);
}

void ShowHideBugReporter()
{
	if ( !g_pBugUI )
		return;

	bool wasvisible = g_pBugUI->IsVisible();

	g_pBugUI->SetVisible( !wasvisible );

	if ( !wasvisible && Cmd_Argc() > 1 )
	{
		g_pBugUI->ParseCommands();
	}
}


static ConCommand bug( "bug", ShowHideBugReporter );
