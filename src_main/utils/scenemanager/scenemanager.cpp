// scenemanager.cpp : Defines the entry point for the console application.
//

#include "cbase.h"
#include "workspacemanager.h"
#include "FileSystem.h"
#include "FileSystem_Tools.h"
#include "cmdlib.h"
#include "vstdlib/random.h"
#include "SoundEmitterSystemBase.h"
#include "iscenemanagersound.h"

char cmdline[1024] = "";

static CUniformRandomStream g_Random;
IUniformRandomStream *random = &g_Random;

IFileSystem *filesystem = NULL;

static CSoundEmitterSystemBase g_SoundEmitter;
CSoundEmitterSystemBase *soundemitter = &g_SoundEmitter;

SpewRetval_t SceneManagerSpewFunc( SpewType_t spewType, char const *pMsg )
{
	switch (spewType)
	{
	case SPEW_ERROR:
		{
			MessageBox(NULL, pMsg, "FATAL ERROR", MB_OK);
		}
		return SPEW_ABORT;
	case SPEW_WARNING:
		{
			Con_ColorPrintf( 255, 0, 0, pMsg );
		}
		break;
	case SPEW_ASSERT:
		{
			Con_ColorPrintf( 255, 0, 0, pMsg );
		}
#ifdef _DEBUG
		return SPEW_DEBUGGER;
#else
		return SPEW_CONTINUE;
#endif
	default:
		{
			Con_Printf(pMsg);
		}
		break;
	}

	return SPEW_CONTINUE;
}

int main(int argc, char* argv[])
{
	SpewOutputFunc( SceneManagerSpewFunc );

	//
	// make sure, we start in the right directory
	//
	char szName[256];

	strcpy (szName, mx::getApplicationPath ());

	if (argc > 1)
	{
		strcpy (cmdline, argv[1]);
		for (int i = 2; i < argc; i++)
		{
			strcat (cmdline, " ");
			strcat (cmdline, argv[i]);
		}
	}

	mx::init (argc, argv);

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

	char *vproject = getenv("VPROJECT");
	if ( !vproject )
	{
		mxMessageBox( NULL, "You must set VPROJECT to run scenemanager.exe", "SceneManager", MB_OK );
		return -1;
	}

	// If they didn't specify -game on the command line, use VPROJECT.
	CmdLib_InitFileSystem( workingdir, true );

	sound->Init();

	CWorkspaceManager *sm = new CWorkspaceManager();

	bool workspace_loaded = false;
	for ( int i = 1; i < argc; i++ )
	{
		if ( !workspace_loaded && strstr (argv[i], ".vsw") )
		{
			workspace_loaded = true;

			// Strip game directory and slash
			char workspace_name[ 512 ];
			filesystem->FullPathToRelativePath( argv[ i ], workspace_name );

			sm->AutoLoad( workspace_name );
		}
	}

	if ( !workspace_loaded )
	{
		sm->AutoLoad( NULL );
	}
	
	int retval = mx::run ();
	
	sound->Shutdown();

	soundemitter->BaseShutdown();

	FileSystem_Term();

	return retval;
}
