//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: The application object.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include "BuildNum.h"
#include "Splash.h"
#include "Options.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "MapDoc.h"
#include "MapView3D.h"
#include "MapView2D.h"
#include "PakDoc.h"
#include "PakViewDirec.h"
#include "PakFrame.h"
#include "GlobalFunctions.h"
#include "Shell.h"
#include "ShellMessageWnd.h"
#include "Options.h"
#include "TextureSystem.h"
#include "ToolManager.h"
#include "Worldcraft.h"
#include "StudioModel.h"
#include "ibsplighting.h"
#include "statusbarids.h"
#include "vstdlib/icommandline.h"
#include "tier0/dbg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// dvs: hack
extern LPCTSTR GetErrorString(void);


static BOOL bMakeLib = FALSE;

static float fSequenceVersion = 0.2f;
static char *pszSequenceHdr = "Worldcraft Command Sequences\r\n\x1a";


CWorldcraft theApp;
COptions Options;

CShell g_Shell;
CShellMessageWnd g_ShellMessageWnd;


//-----------------------------------------------------------------------------
// Purpose: Outputs a formatted debug string.
// Input  : fmt - format specifier.
//			... - arguments to format.
//-----------------------------------------------------------------------------
void DBG(char *fmt, ...)
{
    char ach[128];
    va_list va;

    va_start(va, fmt);
    vsprintf(ach, fmt, va);
    va_end(va);
    OutputDebugString(ach);
}


class CWorldcraftCmdLine : public CCommandLineInfo
{
	public:

		CWorldcraftCmdLine(void)
		{
			m_bShowLogo = TRUE;
		}

		void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
		{
			void MakePrefabLibrary(LPCTSTR pszName);

			if (bFlag && !strcmp(lpszParam, "nologo"))
			{
				m_bShowLogo = FALSE;
			}
			else if (bFlag && !strcmpi(lpszParam, "makelib"))
			{
				bMakeLib = TRUE;
			}
			else if (!bFlag && bMakeLib)
			{
				MakePrefabLibrary(lpszParam);
			}
			else
			{
				CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
			}
		}

		BOOL m_bShowLogo;
};


BEGIN_MESSAGE_MAP(CWorldcraft, CWinApp)
	//{{AFX_MSG_MAP(CWorldcraft)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes member variables and creates a scratch
//			buffer for use when loading WAD files.
//-----------------------------------------------------------------------------
CWorldcraft::CWorldcraft(void)
{
	m_bActiveApp = true;
	m_bEnable3DRender = true;
	m_SuppressVideoAllocation = false;
	m_bForceRenderNextFrame = false;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees scratch buffer used when loading WAD files.
//			Deletes all command sequences used when compiling maps.
//-----------------------------------------------------------------------------
CWorldcraft::~CWorldcraft(void)
{
	//
	// Delete the command sequences.
	//
	int nSequenceCount = m_CmdSequences.GetSize();
	for (int i = 0; i < nSequenceCount; i++)
	{
		CCommandSequence *pSeq = m_CmdSequences[i];
		if (pSeq != NULL)
		{
			delete pSeq;
			m_CmdSequences[i] = NULL;
		}
	}

	g_Textures.ShutDown();
}


//-----------------------------------------------------------------------------
// Purpose: Adds a backslash to the end of a string if there isn't one already.
// Input  : psz - String to add the backslash to.
//-----------------------------------------------------------------------------
static void EnsureTrailingBackslash(char *psz)
{
	if ((psz[0] != '\0') && (psz[strlen(psz) - 1] != '\\'))
	{
		strcat(psz, "\\");
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tweaks our data members to enable us to import old Worldcraft settings
//			from the registry.
//-----------------------------------------------------------------------------
static const char *s_pszOldAppName = NULL;
void CWorldcraft::BeginImportWCSettings(void)
{
	s_pszOldAppName = m_pszAppName;
	m_pszAppName = "Worldcraft";
	SetRegistryKey("Valve");
}


//-----------------------------------------------------------------------------
// Purpose: Tweaks our data members to enable us to import old Valve Hammer Editor
//			settings from the registry.
//-----------------------------------------------------------------------------
void CWorldcraft::BeginImportVHESettings(void)
{
	s_pszOldAppName = m_pszAppName;
	m_pszAppName = "Valve Hammer Editor";
	SetRegistryKey("Valve");
}


//-----------------------------------------------------------------------------
// Purpose: Restores our tweaked data members to their original state.
//-----------------------------------------------------------------------------
void CWorldcraft::EndImportSettings(void)
{
	m_pszAppName = s_pszOldAppName;
	SetRegistryKey("Valve");
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eDir - 
//			szDir - 
//-----------------------------------------------------------------------------
void CWorldcraft::AppendSearchSubDir(SearchDir_t eDir, char *szDir)
{
	EnsureTrailingBackslash(szDir);

	switch (eDir)
	{
		//
		// Get the wads directory with a trailing backslash. This will be under
		// the mod directory (or the game directory if there is no mod directory).
		//
		case SEARCHDIR_WADS:
		{
			strcat(szDir, "wads\\");
			break;
		}

		//
		// Get the sprites directory with a trailing backslash. This will be under
		// the mod directory (or the game directory if there is no mod directory).
		//
		case SEARCHDIR_SPRITES:
		{
			strcat(szDir, "materials\\sprites\\");
			break;
		}

		//
		// Get the sounds directory with a trailing backslash. This will be under
		// the mod directory (or the game directory if there is no mod directory).
		//
		case SEARCHDIR_SOUNDS:
		{
			strcat(szDir, "sound\\");
			break;
		}

		//
		// Get the models directory with a trailing backslash. This will be under
		// the mod directory (or the game directory if there is no mod directory).
		//
		case SEARCHDIR_MODELS:
		{
			strcat(szDir, "models\\");
			break;
		}

		//
		// Get the scenes directory with a trailing backslash. This will be under
		// the mod directory (or the game directory if there is no mod directory).
		//
		case SEARCHDIR_SCENES:
		{
			strcat(szDir, "scenes\\");
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Retrieves various important directories.
// Input  : dir - Enumerated directory to retrieve.
//			p - Pointer to buffer that receives the full path to the directory.
//-----------------------------------------------------------------------------
void CWorldcraft::GetDirectory(DirIndex_t dir, char *p)
{
	switch (dir)
	{
		case DIR_PROGRAM:
		{
			strcpy(p, m_szAppDir);
			EnsureTrailingBackslash(p);
			break;
		}

		case DIR_PREFABS:
		{
			strcpy(p, m_szAppDir);
			EnsureTrailingBackslash(p);
			strcat(p, "Prefabs");

			//
			// Make sure the prefabs directory exists.
			//
			if ((_access( p, 0 )) == -1)
			{
				CreateDirectory(p, NULL);
			}
			break;
		}

		//
		// Get the game directory with a trailing backslash. This is
		// where the base game's resources are, such as "C:\Half-Life\valve\".
		//
		case DIR_GAME_EXE:
		{
			strcpy(p, g_pGameConfig->m_szGameExeDir);
			EnsureTrailingBackslash(p);
			break;
		}

		//
		// Get the mod directory with a trailing backslash. This is where
		// the mod's resources are, such as "C:\Half-Life\tfc\".
		//
		case DIR_MOD:
		{
			strcpy(p, g_pGameConfig->m_szModDir);
			EnsureTrailingBackslash(p);
			break;
		}

		//
		// Get the game directory with a trailing backslash. This is where
		// the game is installed, such as "C:\Half-Life\".
		//
		case DIR_GAME:
		{
			strcpy(p, g_pGameConfig->m_szGameDir);
			EnsureTrailingBackslash(p);
			break;
		}

	}
}


//-----------------------------------------------------------------------------
// Purpose: Always returns the game directory.
// Input  : szDir - Buffer to receive first search directory.
//			pos - Iterator.
//-----------------------------------------------------------------------------
void CWorldcraft::GetFirstSearchDir(SearchDir_t eDir, char *szDir, POSITION &pos)
{
	//
	// First, return the mod directory. Eventually this may be a list of
	// search directories configurable by the user.
	//
	GetDirectory(DIR_MOD, szDir);
	pos = (POSITION)1;

	//
	// If there's no mod directory, skip to the next search directory.
	//
	if (szDir[0] == '\0')
	{
		GetNextSearchDir(eDir, szDir, pos);
	}
	else
	{
		AppendSearchSubDir(eDir, szDir);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next directory in the search path.
// Input  : szDir - Buffer to receive next search directory.
//			pos - Iterator filled out by a previous call to GetFirst/NextSearchDir.
//-----------------------------------------------------------------------------
void CWorldcraft::GetNextSearchDir(SearchDir_t eDir, char *szDir, POSITION &pos)
{
	//
	// Second, return the base game directory.
	//
	if ((int)pos == 1)
	{
		GetDirectory(DIR_GAME, szDir);
		pos = (POSITION)((int)pos + 1);
	}
	//
	// Last, return the Worldcraft directory.
	//
	else if ((int)pos == 2)
	{
		GetDirectory(DIR_PROGRAM, szDir);
		pos = (POSITION)((int)pos + 1);
	}
	else
	{
		pos = NULL;
	}

	if (pos != NULL)
	{
		AppendSearchSubDir(eDir, szDir);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns a color from the application configuration storage.
//-----------------------------------------------------------------------------
COLORREF CWorldcraft::GetProfileColor(const char *pszSection, const char *pszKey, int r, int g, int b)
{
	int newR, newG, newB;
	
	CString strDefault;
	CString strReturn;
	char szBuff[128];
	sprintf(szBuff, "%i %i %i", r, g, b);

	strDefault = szBuff;

	strReturn = GetProfileString(pszSection, pszKey, strDefault);

	if (strReturn.IsEmpty())
		return 0;

	// Parse out colors.
	char *pStart;
	char *pCurrent;
	pStart = szBuff;
	pCurrent = pStart;
	
	strcpy( szBuff, (char *)(LPCSTR)strReturn );

	while (*pCurrent && *pCurrent != ' ')
		pCurrent++;

	*pCurrent++ = 0;
	newR = atoi(pStart);

	pStart = pCurrent;
	while (*pCurrent && *pCurrent != ' ')
		pCurrent++;

	*pCurrent++ = 0;
	newG = atoi(pStart);

	pStart = pCurrent;
	while (*pCurrent)
		pCurrent++;

	*pCurrent++ = 0;
	newB = atoi(pStart);

	return COLORREF(RGB(newR, newG, newB));
}


//-----------------------------------------------------------------------------
// Purpose: Launches the help system for the specified help topic.
// Input  : pszTopic - Topic to open.
//-----------------------------------------------------------------------------
void CWorldcraft::Help(const char *pszTopic)
{
	//
	// Get the directory that the help file should be in (our program directory).
	//
	char szHelpDir[MAX_PATH];
	GetDirectory(DIR_PROGRAM, szHelpDir);

	//
	// Find the application that is associated with compiled HTML files.
	//
	char szHelpExe[MAX_PATH];
	HINSTANCE hResult = FindExecutable("wc.chm", szHelpDir, szHelpExe);
	if (hResult > (HINSTANCE)32)
	{
		//
		// Build the full topic with which to launch the help application.
		//
		char szParam[2 * MAX_PATH];
		strcpy(szParam, szHelpDir);
		strcat(szParam, "wc.chm");
		if (pszTopic != NULL)
		{
			strcat(szParam, "::/");
			strcat(szParam, pszTopic);
		}

		//
		// Launch the help application for the given topic.
		//
		hResult = ShellExecute(NULL, "open", szHelpExe, szParam, szHelpDir, SW_SHOW);
	}

	if (hResult <= (HINSTANCE)32)
	{
		char szError[MAX_PATH];
		sprintf(szError, "The help system could not be launched. The the following error was returned:\n%s (0x%X)", GetErrorString(), hResult);
		AfxMessageBox(szError);
	}
}


static SpewRetval_t WorldcraftDbgOutput( SpewType_t spewType, char const *pMsg )
{
	// FIXME: The messages we're getting from the material system
	// are ones that we really don't care much about.
	// I'm disabling this for now, we need to decide about what to do with this

	switch( spewType )
	{
	case SPEW_ERROR:
		MessageBox( NULL, (LPCTSTR)pMsg, "Fatal Error", MB_OK | MB_ICONINFORMATION );
#ifdef _DEBUG
		return SPEW_DEBUGGER;
#else
		return SPEW_ABORT;
#endif

	default:
		OutputDebugString( pMsg );
		return (spewType == SPEW_ASSERT) ? SPEW_DEBUGGER : SPEW_CONTINUE; 
	}
}


//-----------------------------------------------------------------------------
// Purpose: Intializes this instance of Worldcraft.
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CWorldcraft::InitInstance(void)
{
	SpewOutputFunc( WorldcraftDbgOutput );
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	int nSize = sizeof(CMapClass);

	//
	// Check for 15-bit color or higher.
	//
	HDC hDC = ::CreateCompatibleDC(NULL);
	if (hDC)
	{
		int bpp = GetDeviceCaps(hDC, BITSPIXEL);
		if (bpp < 15)
		{
			AfxMessageBox("Your screen must be in 16-bit color or higher to run Worldcraft.");
			return FALSE;
		}
		::DeleteDC(hDC);
	}

	//
	//
	// Create a custom window class for this application so that engine's
	// FindWindow will find us.
	//
	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));
    wndcls.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc   = AfxWndProc;
    wndcls.hInstance     = AfxGetInstanceHandle();
    wndcls.hIcon         = LoadIcon(IDR_MAINFRAME);
    wndcls.hCursor       = LoadCursor( IDC_ARROW );
    wndcls.hbrBackground = (HBRUSH)0; //  (COLOR_WINDOW + 1);
    wndcls.lpszMenuName  = "IDR_MAINFRAME";
	wndcls.cbWndExtra    = 0;
	 
	// HL Shell class name
    wndcls.lpszClassName = "VALVEWORLDCRAFT";

    // Register it, exit if it fails    
	if(!AfxRegisterClass(&wndcls))
	{
		AfxMessageBox("Could not register Worldcraft's window class");
		return FALSE;
	}

	srand(time(NULL));

	// ensure we're in the same directory as the .EXE
	char *p;
	GetModuleFileName(NULL, m_szAppDir, MAX_PATH);

	// Fake the command-line interface to think it was run with no arguments 
	// This will get all the paths set up correctly.
	CommandLine()->CreateCmdLine( m_szAppDir );

	p = strrchr(m_szAppDir, '\\');
	if(p)
	{
		// chop off \wc.exe
		p[0] = 0;
	}

	mychdir(m_szAppDir);

	SetRegistryKey("Valve");
	WriteProfileString("General", "Directory", m_szAppDir);

	//
	// Create a window to receive shell commands from the engine, and attach it
	// to our shell processor.
	//
	g_ShellMessageWnd.Create();
	g_ShellMessageWnd.SetShell(&g_Shell);

	//
	// Create the tool manager singleton.
	//
	CToolManager::Init();

	if (bMakeLib)
	{
		return(FALSE);	// made library .. don't want to enter program
	}

	//
	// Create and optionally display the splash screen.
	//
	{
		CWorldcraftCmdLine cmdInfo;
		ParseCommandLine(cmdInfo);

		CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowLogo);
	}

	LoadSequences();	// load cmd sequences - different from options because
						//  users might want to share (darn registry)

	// other init:
	randomize();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	pMapDocTemplate = new CMultiDocTemplate(
		IDR_MAPDOC,
		RUNTIME_CLASS(CMapDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CMapView2D));
	AddDocTemplate(pMapDocTemplate);

	// register shell file types
	RegisterShellFileTypes();

	//
	// Initialize the rich edit control so we can use it in the entity help dialog.
	//
	AfxInitRichEdit();

	//
	// Create main MDI Frame window. Must be done AFTER registering the multidoc template!
	//
	CMainFrame *pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME, WS_MAXIMIZE | WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE))
	{
		return(FALSE);
	}
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if(!cmdInfo.m_strFileName.IsEmpty())
	{
		// we don't want to make a new file (default behavior if no file
		//  is specified on the commandline.)

		// Dispatch commands specified on the command line
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;
	}

	//
	// The main window has been initialized, so show and update it.
	//
	m_nCmdShow = SW_SHOWMAXIMIZED;
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	CSplashWnd::ShowSplashScreen();

	//
	// Load the options.
	//
	Options.Init();

	//
	// Initialize the texture manager and load all textures.
	//
	if (!g_Textures.Initialize(g_pGameConfig->m_szGameDir, m_pMainWnd->m_hWnd))
	{
		Msg(mwError, "Failed to initialize texture system.");
	}
	else
	{
		//
		// Initialize studio model rendering (must happen after g_Textures.Initialize since
		// g_Textures.Initialize kickstarts the material system and sets up g_MaterialSystemClientFactory)
		//
		StudioModel::Initialize();
	}

	g_Textures.LoadAllGraphicsFiles();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWorldcraft::ExitInstance()
{
	//
	// Delete the tool manager singleton.
	//
	CToolManager::Shutdown();

   return CWinApp::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CStatic	m_cRedHerring;
	CButton	m_Order;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	afx_msg void OnOrder();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDX - 
//-----------------------------------------------------------------------------
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_REDHERRING, m_cRedHerring);
	DDX_Control(pDX, IDC_ORDER, m_Order);
	//}}AFX_DATA_MAP
}

#include <process.h>


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAboutDlg::OnOrder() 
{
	char szBuf[MAX_PATH];
	GetWindowsDirectory(szBuf, MAX_PATH);
	strcat(szBuf, "\\notepad.exe");
	_spawnl(_P_NOWAIT, szBuf, szBuf, "order.txt", NULL);
}


#define DEMO_VERSION	0

// 1, 4, 8, 17, 0, 0 // Encodes "Valve"

#if DEMO_VERSION

char gVersion[] = {                        
#if DEMO_VERSION==1
	7, 38, 68, 32, 4, 77, 12, 1, 0 // Encodes "PC Gamer Demo"
#elif DEMO_VERSION==2
	7, 38, 68, 32, 4, 77, 12, 0, 0 // Encodes "PC Games Demo"
#elif DEMO_VERSION==3
	20, 10, 9, 23, 16, 84, 12, 1, 0, 38, 65, 25, 6, 1, 11, 119, 50, 11, 21, 9, 68, 0 // Encodes "Computer Gaming World Demo"
#elif DEMO_VERSION==4
	25, 0, 28, 19, 72, 103, 12, 29, 69, 19, 65, 0, 6, 0, 2, 0		// Encodes "Next-Generation Demo"
#elif DEMO_VERSION==5
	20, 10, 9, 23, 16, 84, 12, 1, 0, 38, 65, 25, 10, 79, 41, 57, 17, 1, 21, 17, 65, 0, 29, 77, 4, 78, 0, 0 // Encodes "Computer Game Entertainment"
#elif DEMO_VERSION==6
	20, 10, 9, 23, 16, 84, 12, 1, 0, 0, 78, 16, 79, 33, 9, 35, 69, 52, 11, 4, 89, 12, 1, 0 // Encodes "Computer and Net Player"
#elif DEMO_VERSION==7
	50, 72, 52, 43, 36, 121, 0 // Encodes "e-PLAY"
#elif DEMO_VERSION==8
	4, 17, 22, 6, 17, 69, 14, 10, 0, 49, 76, 1, 28, 0 // Encodes "Strategy Plus"
#elif DEMO_VERSION==9
	7, 38, 68, 42, 4, 71, 8, 9, 73, 15, 69, 0 // Encodes "PC Magazine"
#elif DEMO_VERSION==10
	5, 10, 8, 11, 12, 78, 14, 83, 115, 21, 79, 26, 10, 0 // Encodes "Rolling Stone"
#elif DEMO_VERSION==11
	16, 4, 9, 2, 22, 80, 6, 7, 0 // Encodes "Gamespot"
#endif
};

static char gKey[] = "Wedge is a tool";	// Decrypt key

// XOR a string with a key
void Encode( char *pstring, char *pkey, int strLen )
{
	int i, len;
	len = strlen( pkey );
	for ( i = 0; i < strLen; i++ )
		pstring[i] ^= pkey[ i % len ];
}
#endif // DEMO_VERSION


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CAboutDlg::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	m_Order.SetRedraw(FALSE);

#if DEMO_VERSION
	static BOOL bFirst = TRUE;
	if(bFirst)
	{
		Encode(gVersion, gKey, sizeof(gVersion)-1);
		bFirst = FALSE;
	}
	CString str;
	str.Format("%s Demo", gVersion);
	m_cRedHerring.SetWindowText(str);
#endif // DEMO_VERSION

	//
	// Display the build number.
	//
	CWnd *pWnd = GetDlgItem(IDC_BUILD_NUMBER);
	if (pWnd != NULL)
	{
		char szTemp1[MAX_PATH];
		char szTemp2[MAX_PATH];
		int nBuild = build_number();
		pWnd->GetWindowText(szTemp1, sizeof(szTemp1));
		sprintf(szTemp2, szTemp1, nBuild);
		pWnd->SetWindowText(szTemp2);
	}

	//
	// For SDK builds, append "SDK" to the version number.
	//
#ifdef SDK_BUILD
	char szTemp[MAX_PATH];
	GetWindowText(szTemp, sizeof(szTemp));
	strcat(szTemp, " SDK");
	SetWindowText(szTemp);
#endif // SDK_BUILD

	return TRUE;
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_ORDER, OnOrder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWorldcraft::OnAppAbout(void)
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWorldcraft::OnFileNew(void)
{
	pMapDocTemplate->OpenDocumentFile(NULL);
	if(Options.general.bLoadwinpos && Options.general.bIndependentwin)
	{
		::GetMainWnd()->LoadWindowStates();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWorldcraft::OnFileOpen(void)
{
	static char szInitialDir[MAX_PATH] = "";
	if (szInitialDir[0] == '\0')
	{
		strcpy(szInitialDir, g_pGameConfig->szMapDir);
	}

	// TODO: need to prevent (or handle) opening VMF files when using old map file formats
	CFileDialog dlg(TRUE, NULL, NULL, OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_NOCHANGEDIR, "Valve Map Files (*.vmf)|*.vmf|Worldcraft RMFs (*.rmf)|*.rmf|Worldcraft Maps (*.map)|*.map||");
	dlg.m_ofn.lpstrInitialDir = szInitialDir;
	int iRvl = dlg.DoModal();

	if (iRvl == IDCANCEL)
	{
		return;
	}

	//
	// Get the directory they browsed to for next time.
	//
	CString str = dlg.GetPathName();
	int nSlash = str.ReverseFind('\\');
	if (nSlash != -1)
	{
		strcpy(szInitialDir, str.Left(nSlash));
	}

	if (str.Find('.') == -1)
	{
		switch (dlg.m_ofn.nFilterIndex)
		{
			case 1:
			{
				str += ".vmf";
				break;
			}

			case 2:
			{
				str += ".rmf";
				break;
			}

			case 3:
			{
				str += ".map";
				break;
			}
		}
	}

	OpenDocumentFile(str);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lpszFileName - 
// Output : CDocument*
//-----------------------------------------------------------------------------
CDocument* CWorldcraft::OpenDocumentFile(LPCTSTR lpszFileName) 
{
	if(GetFileAttributes(lpszFileName) == 0xFFFFFFFF)
	{
		AfxMessageBox("That file does not exist.");
		return NULL;
	}

	CDocument *pDoc = pMapDocTemplate->OpenDocumentFile(lpszFileName);

	if(Options.general.bLoadwinpos && Options.general.bIndependentwin)
	{
		::GetMainWnd()->LoadWindowStates();
	}

	return pDoc;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pMsg - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CWorldcraft::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following lines were added by the Splash Screen component.
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;

	return CWinApp::PreTranslateMessage(pMsg);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWorldcraft::LoadSequences(void)
{
	ifstream file("CmdSeq.wc", ios::in | ios::binary | 
		ios::nocreate);
	
	if(!file.is_open())
		return;	// none to load

	// skip past header & version
	float fThisVersion;

	file.seekg(strlen(pszSequenceHdr));
	file.read((char*)&fThisVersion, sizeof fThisVersion);

	// read number of sequences
	DWORD dwSize;
	int nSeq;

	file.read((char*)&dwSize, sizeof dwSize);
	nSeq = dwSize;

	for(int i = 0; i < nSeq; i++)
	{
		CCommandSequence *pSeq = new CCommandSequence;
		file.read(pSeq->m_szName, 128);

		// read commands in sequence
		file.read((char*)&dwSize, sizeof dwSize);
		int nCmd = dwSize;
		CCOMMAND cmd;
		for(int iCmd = 0; iCmd < nCmd; iCmd++)
		{
			if(fThisVersion < 0.2f)
			{
				file.read((char*)&cmd, sizeof(CCOMMAND)-1);
				cmd.bNoWait = FALSE;
			}
			else
			{
				file.read((char*)&cmd, sizeof(CCOMMAND));
			}
			pSeq->m_Commands.Add(cmd);
		}

		m_CmdSequences.Add(pSeq);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWorldcraft::SaveSequences(void)
{
	ofstream file("CmdSeq.wc", ios::out | ios::binary);

	// write header
	file.write(pszSequenceHdr, strlen(pszSequenceHdr));
	// write out version
	file.write((char*)&fSequenceVersion, sizeof(float));

	// write out each sequence..
	int i, nSeq = m_CmdSequences.GetSize();
	DWORD dwSize = nSeq;
	file.write((char*)&dwSize, sizeof dwSize);
	for(i = 0; i < nSeq; i++)
	{
		CCommandSequence *pSeq = m_CmdSequences[i];

		// write name of sequence
		file.write(pSeq->m_szName, 128);
		// write number of commands
		int nCmd = pSeq->m_Commands.GetSize();
		dwSize = nCmd;
		file.write((char*)&dwSize, sizeof dwSize);
		// write commands .. 
		for(int iCmd = 0; iCmd < nCmd; iCmd++)
		{
			CCOMMAND &cmd = pSeq->m_Commands[iCmd];
			file.write((char*)&cmd, sizeof cmd);
		}
	}
}


void CWorldcraft::SetForceRenderNextFrame()
{
	m_bForceRenderNextFrame = true;
}


bool CWorldcraft::GetForceRenderNextFrame()
{
	return m_bForceRenderNextFrame;
}


//-----------------------------------------------------------------------------
// Purpose: Performs idle processing. Runs the frame and does MFC idle processing.
// Input  : lCount - The number of times OnIdle has been called in succession,
//				indicating the relative length of time the app has been idle without
//				user input.
// Output : Returns TRUE if there is more idle processing to do, FALSE if not.
//-----------------------------------------------------------------------------
BOOL CWorldcraft::OnIdle(LONG lCount)
{
	static int lastPercent = -20000;
	int curPercent = -10000;

	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if( pDoc )
	{
		IBSPLighting *pLighting = pDoc->GetBSPLighting();

		if( pLighting )
		{
			// Update 5x / second.
			static DWORD lastTime = 0;

			DWORD curTime = GetTickCount();
			if( curTime - lastTime < 200 )
			{
				curPercent = lastPercent; // no change
			}
			else
			{
				curPercent = (int)( pLighting->GetPercentComplete() * 10000.0f );
				lastTime = curTime;
			}

			// Redraw the views when new lightmaps are ready.
			if( pLighting->CheckForNewLightmaps() )
			{
				SetForceRenderNextFrame();
				pDoc->UpdateAllViews( NULL, MAPVIEW_UPDATE_DISPLAY );
			}
		}
	}

	// Update the status text.
	if( curPercent == -10000 )
	{
		SetStatusText( SBI_LIGHTPROGRESS, "<->" );
	}
	else if( curPercent != lastPercent )
	{
		char str[256];
		sprintf( str, "%.2f%%", curPercent / 100.0f );
		SetStatusText( SBI_LIGHTPROGRESS, str );
	}

	lastPercent = curPercent;	
	return(CWinApp::OnIdle(lCount));
}


//-----------------------------------------------------------------------------
// Purpose: Renders the realtime views.
//-----------------------------------------------------------------------------
void CWorldcraft::RunFrame(void)
{
	//
	// Only do realtime stuff when we are the active application.
	//
	if (CMapDoc::GetActiveMapDoc() && 
		m_bActiveApp && 
		m_bEnable3DRender )
	{
		// get the time
		CMapDoc::GetActiveMapDoc()->UpdateCurrentTime();

		// run any animation
		CMapDoc::GetActiveMapDoc()->UpdateAnimation();

		// redraw the 3d views
		CMapDoc::GetActiveMapDoc()->Update3DViews();
	}

	// No matter what, we want to keep caching in materials...
	g_Textures.LazyLoadTextures();

	m_bForceRenderNextFrame = false;
}


//-----------------------------------------------------------------------------
// Purpose: Overloaded Run so that we can control the frameratefor realtime
//			rendering in the 3D view.
// Output : As MFC CWinApp::Run.
//-----------------------------------------------------------------------------
int CWorldcraft::Run(void)
{
	ASSERT_VALID(this);

	// For tracking the idle time state
	bool bIdle = true;
	long lIdleCount = 0;

	//
	// Acquire and dispatch messages until a WM_QUIT message is received.
	//
	for (;;)
	{
		if (!IsActiveApp())
		{
			Sleep(50);
		}

		RunFrame();

		if (bIdle && !OnIdle(lIdleCount++))
		{
			bIdle = false;
		}

		//
		// Pump messages until the message queue is empty.
		//
		while (::PeekMessage(&m_msgCur, NULL, NULL, NULL, PM_REMOVE))
		{
			if (m_msgCur.message == WM_QUIT)
			{
				return(ExitInstance());
			}

			if (!PreTranslateMessage(&m_msgCur))
			{
				::TranslateMessage(&m_msgCur);
				::DispatchMessage(&m_msgCur);
			}

			// Reset idle state after pumping idle message.
			if (IsIdleMessage(&m_msgCur))
			{
				bIdle = true;
				lIdleCount = 0;
			}
		}
	}

	ASSERT(FALSE);  // not reachable
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if Worldcraft is the active app, false if not.
//-----------------------------------------------------------------------------
bool CWorldcraft::IsActiveApp(void)
{
	return(m_bActiveApp);
}


//-----------------------------------------------------------------------------
// Purpose: Called from CMainFrame::OnSysCommand, this informs the app when it
//			is minimized and unminimized. This allows us to stop rendering the
//			3D view when we are minimized.
// Input  : bMinimized - TRUE when minmized, FALSE otherwise.
//-----------------------------------------------------------------------------
void CWorldcraft::OnActivateApp(bool bActive)
{
	m_bActiveApp = bActive;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//-----------------------------------------------------------------------------
bool CWorldcraft::Is3DRenderEnabled(void)
{
	return(m_bEnable3DRender);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//-----------------------------------------------------------------------------
void CWorldcraft::Enable3DRender(bool bEnable)
{
	m_bEnable3DRender = bEnable;
}


//-----------------------------------------------------------------------------
// Purpose: Called from the shell to relinquish our video memory in favor of the
//			engine. This is called by the engine when it starts up.
//-----------------------------------------------------------------------------
void CWorldcraft::ReleaseVideoMemory()
{
   POSITION pos = GetFirstDocTemplatePosition();

   while (pos)
   {
      CDocTemplate* pTemplate = (CDocTemplate*)GetNextDocTemplate(pos);
      POSITION pos2 = pTemplate->GetFirstDocPosition();
      while (pos2)
      {
         CDocument * pDocument;
         if ((pDocument=pTemplate->GetNextDoc(pos2)) != NULL)
		 {
			 static_cast<CMapDoc*>(pDocument)->ReleaseVideoMemory();
		 }
      }
   }
} 

void CWorldcraft::SuppressVideoAllocation( bool bSuppress )
{
	m_SuppressVideoAllocation = bSuppress;
} 

bool CWorldcraft::CanAllocateVideo() const
{
	return !m_SuppressVideoAllocation;
}

