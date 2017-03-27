//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================


#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "isys.h"
#include "conproc.h"
#include "dedicated.h"
#include "exefuncs.h"
#include "launcher_int.h"
#include "dll_state.h"
#include "engine_hlds_api.h"
#include "checksum_md5.h"
#include "idedicatedexports.h"
#include "tier0/vcrmode.h"
#include "tier0/dbg.h"
#include "ifilesystem.h"
#include "mathlib.h"
#include "interface.h"
#include "vstdlib/strtools.h"
#include "vstdlib/ICommandLine.h"


bool InitInstance( );
void ProcessConsoleInput( void );


static const char *g_pszengine = "engine_i486.so";
IDedicatedServerAPI *engine = NULL;

static char g_szEXEName[ 256 ];

static char	console_text[256];
static int	console_textlen;

//-----------------------------------------------------------------------------
// Modules...
//-----------------------------------------------------------------------------
static CSysModule *s_hMatSystemModule;
static CreateInterfaceFn s_MaterialSystemFactory;
static CSysModule *s_hEngineModule = NULL;
static CreateInterfaceFn s_EngineFactory;

//-----------------------------------------------------------------------------
// Implementation of IVCRHelpers.
//-----------------------------------------------------------------------------

class CVCRHelpers : public IVCRHelpers
{
public:
        virtual void    ErrorMessage( const char *pMsg )
        {
                printf( "ERROR: %s\n", pMsg );
        }

        virtual void*   GetMainWindow()
        {
                return 0;
        }
};
static CVCRHelpers g_VCRHelpers;


//-----------------------------------------------------------------------------
// Purpose: Implements OS Specific layer ( loosely )
//-----------------------------------------------------------------------------
class CSys : public ISys
{
public:
	bool		GameInit( CreateInterfaceFn dedicatedFactory );
	void		GameShutdown( void );

	void		Sleep( int msec );
	bool		GetExecutableName( char *out );
	void		ErrorMessage( int level, const char *msg );

	void		WriteStatusText( char *szText );
	void		UpdateStatus( int force );

	long		LoadLibrary( char *lib );
	void		FreeLibrary( long library );
	void		*GetProcAddress( long library, const char *name );

	bool		CreateConsoleWindow( void );
	void		DestroyConsoleWindow( void );

	void		ConsoleOutput ( char *string );
	char		*ConsoleInput (void);
	void		Printf(char *fmt, ...);
};

static CSys g_Sys;
ISys *sys = &g_Sys;

//-----------------------------------------------------------------------------
// Purpose:
// Input  : msec
// Output : 
//-----------------------------------------------------------------------------
void CSys::Sleep( int msec )
{
    usleep(msec * 1000);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : handle, function name-
// Output : void *
//-----------------------------------------------------------------------------
void *CSys::GetProcAddress( long library, const char *name )
{
	return dlsym( library, name );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *lib -
// Output : long
//-----------------------------------------------------------------------------
long CSys::LoadLibrary( char *lib )
{
	void *hDll = NULL;

    char    cwd[1024];
    char    absolute_lib[1024];
    
    if (!getcwd(cwd, sizeof(cwd)))
        ErrorMessage(1, "Sys_LoadLibrary: Couldn't determine current directory.");
        
    if (cwd[strlen(cwd)-1] == '/')
        cwd[strlen(cwd)-1] = 0;
        
    sprintf(absolute_lib, "%s/%s", cwd, lib);
    
    hDll = dlopen( absolute_lib, RTLD_NOW );
    if ( !hDll )
    {
        ErrorMessage( 1, dlerror() );
    }   
	return (long)hDll;
}

void CSys::FreeLibrary( long library )
{
	if ( !library )
		return;

	dlclose( (void *)library );
}

bool CSys::GetExecutableName( char *out )
{
	char *name = strrchr(g_szEXEName, '/' );
	if ( name )
	{
		strcpy( out, name + 1);
		return true;
	}
	else
	{
		return false;
	}
}

/*
==============
ErrorMessage

Engine is erroring out, display error in message box
==============
*/
void CSys::ErrorMessage( int level, const char *msg )
{
	printf( "%s\n", msg );
	exit( -1 );
}

void CSys::UpdateStatus( int force )
{
}

/*
================
ConsoleOutput

Print text to the dedicated console
================
*/
void CSys::ConsoleOutput (char *string)
{
	printf( "%s", string );
	fflush( stdout );
}

/*
==============
Printf

Engine is printing to console
==============
*/
void CSys::Printf(char *fmt, ...)
{
	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start (argptr, fmt);
	vsprintf (szText, fmt, argptr);
	va_end (argptr);

	// Get Current text and append it.
	ConsoleOutput( szText );
}

/*
================
ConsoleInput

================
*/
char *CSys::ConsoleInput( void )
{
	struct timeval	tvTimeout;
	fd_set			fdSet;
	unsigned char	ch;
 
	FD_ZERO( &fdSet );
	FD_SET( STDIN_FILENO, &fdSet );

	tvTimeout.tv_sec        = 0;
	tvTimeout.tv_usec       = 0;

	if ( select( 1, &fdSet, NULL, NULL, &tvTimeout ) == -1 || !FD_ISSET( STDIN_FILENO, &fdSet ) )
		return NULL;

	console_textlen = 0;

	// We're going to shove a newline onto the end of the input later in
	// ProcessConsoleInput(), so only accept 254 chars instead of 255.    -LH

	while ( read( STDIN_FILENO, &ch, 1 ) )
	{
		if ( ( ch == 10 ) || ( console_textlen == 254 ) )
		{
			// For commands longer than we can accept, consume the remainder of the input buffer
			if ( ( console_textlen == 254 ) && ( ch != 10 ) )
			{
				while ( read( STDIN_FILENO, &ch, 1 ) )
				{
					if ( ch == 10 )
						break;
				}
			}

			//Null terminate string and return
			console_text[ console_textlen ] = 0;
			console_textlen = 0;
			return console_text;
		}

		console_text[ console_textlen++ ] = ch;
	}

	return NULL;
}

/*
==============
WriteStatusText

==============
*/
void CSys::WriteStatusText( char *szText )
{
}

/*
==============
CreateConsoleWindow

Create console window ( overridable? )
==============
*/
bool CSys::CreateConsoleWindow( void )
{
	return true;
}

/*
==============
DestroyConsoleWindow

==============
*/
void CSys::DestroyConsoleWindow( void )
{
}

/*
================
GameInit
================
*/
bool CSys::GameInit( CreateInterfaceFn dedicatedFactory )
{
       // Load up the material system
        s_hMatSystemModule = FileSystem_LoadModule("materialsystem_i486.so");
        if (!s_hMatSystemModule)
                return false;

        s_MaterialSystemFactory = Sys_GetFactory(s_hMatSystemModule);
        if (!s_MaterialSystemFactory)
                return false;

        s_hEngineModule = FileSystem_LoadModule( g_pszengine );
        if ( !s_hEngineModule )
        {
                return false;
        }

        s_EngineFactory = Sys_GetFactory( s_hEngineModule );
        if ( !s_EngineFactory )
        {
                return false;
        }

        return true;
}

/*
==============
GameShutdown

==============
*/
void CSys::GameShutdown( void )
{
     if ( s_hEngineModule )
     {
     	FileSystem_UnloadModule( s_hEngineModule );
     }

     if (s_hMatSystemModule)
     	FileSystem_UnloadModule(s_hMatSystemModule);

}

//-----------------------------------------------------------------------------
// An aggregate class factory encompassing all factories in all loaded DLLs
//-----------------------------------------------------------------------------
void* DedicatedFactory(const char *pName, int *pReturnCode)
{
        void *pRetVal = NULL;

        // FIXME: Really, we should just load the DLLs in order
        // and search in the order loaded. This is sort of a half-measure hack
        // to get to where we need to go

        // First ask ourselves
        pRetVal = Sys_GetFactoryThis()( pName, pReturnCode );
        if (pRetVal)
                return pRetVal;

        // First ask the file system...
        pRetVal = GetFileSystemFactory()( pName, pReturnCode );
        if (pRetVal)
                return pRetVal;

        // Next the material system...
        pRetVal = s_MaterialSystemFactory( pName, pReturnCode );
        if (pRetVal)
                return pRetVal;

        // Then the engine
        pRetVal = s_EngineFactory( pName, pReturnCode );
        if (pRetVal)
        	return pRetVal;

        return NULL;
}

class CDedicatedExports : public IDedicatedExports
{
public:
        void Sys_Printf( char *text )
        {
                sys->Printf( text );
        }
};

EXPOSE_SINGLE_INTERFACE( CDedicatedExports, IDedicatedExports, VENGINE_DEDICATEDEXPORTS_API_VERSION );

SpewRetval_t DedicatedSpewOutputFunc( SpewType_t spewType, char const *pMsg )
{
        printf( "%s", pMsg );
        if( spewType == SPEW_ASSERT )
        {
                return SPEW_DEBUGGER;
        }
        else if( spewType == SPEW_ERROR )
        {
                return SPEW_ABORT;
        }
        else
        {
                return SPEW_CONTINUE;
        }
}

//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
static const char *UTIL_GetExecutableDir( )
{
        static char     exedir[ MAX_PATH ];

	strcpy( exedir, g_szEXEName );

        char *pSlash;
        char *pSlash2;
        pSlash = strrchr( exedir,'\\' );
        pSlash2 = strrchr( exedir,'/' );
        if ( pSlash2 > pSlash )
        {
                pSlash = pSlash2;
        }
        if (pSlash)
        {
                *pSlash = 0;
        }

        return exedir;
}

//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
const char *UTIL_GetBaseDir( void )
{
        static char     basedir[ MAX_PATH ];

        char const *pOverrideDir = CommandLine()->CheckParm( "-basedir" );
        if ( pOverrideDir )
                return pOverrideDir;

        basedir[ 0 ] = 0;
        const char *pExeDir = UTIL_GetExecutableDir( );
        if ( pExeDir )
        {
                strcpy( basedir, pExeDir );
                char *p = Q_stristr( basedir, "bin" );
                if ( p && ( p - basedir ) == (int)(strlen( basedir ) - 3 ) )
                {
                        --p;
                        *p = 0;
                }
        }

        return basedir;
}

int main(int argc, char **argv)
{
	int iret = -1;
	int bDone = 0;

	strcpy(g_szEXEName, *argv);
	// Store off command line for argument searching
	BuildCmdLine(argc, argv);

        MathLib_Init( 2.2f, 2.2f, 0.0f, 1.0f );

        // Hook the debug output stuff.
        SpewOutputFunc( DedicatedSpewOutputFunc );

        // Store off command line for argument searching
        CommandLine()->CreateCmdLine( g_pVCR->Hook_GetCommandLine() );

        // Start VCR mode?
        const char *filename;
        if( CommandLine()->CheckParm( "-vcrrecord", &filename ) )
        {
                if ( !g_pVCR->Start( filename, true, &g_VCRHelpers ) )
                {
                        Error( "-vcrrecord: can't open '%s' for writing.\n", filename );
                        return -1;
                }
        }
        else if( CommandLine()->CheckParm( "-vcrplayback", &filename ) )
        {
                if ( !g_pVCR->Start( filename, false, &g_VCRHelpers ) )
                {
                        Error( "-vcrplayback: can't open '%s' for reading.\n", filename );
                        return -1;
                }
        }

        // Rehook the command line through VCR mode.
        CommandLine()->CreateCmdLine( g_pVCR->Hook_GetCommandLine() );

        // Required for DLL loading to happen
        // But actually, we're fucked if this hasn't already happened, since
        // we've got statically linked DLLs...
        const char *pExeDir = UTIL_GetExecutableDir();
        chdir( pExeDir );

        // Set default game directory
        const char *pGameDir = NULL;
        CommandLine()->CheckParm( "-game", &pGameDir );
        if ( !CommandLine()->CheckParm( "-defaultgamedir" ) )
        {
                CommandLine()->AppendParm( "-defaultgamedir", "hl2" );
        }

	if ( !InitInstance() )
	{
		goto cleanup;
	}

	if ( !sys->CreateConsoleWindow() )
	{
		goto cleanup;
	}

	if ( !sys->GameInit( DedicatedFactory ) )
	{
		goto cleanup;
	}

        engine = (IDedicatedServerAPI *)DedicatedFactory( VENGINE_HLDS_API_VERSION, NULL );
        if ( !engine )
        {
                goto cleanup;
        }

        if ( !engine->Init( UTIL_GetBaseDir(), DedicatedFactory ) )
        {
                goto cleanup;
        }



   	while ( 1 )
   	{
	
		if ( bDone );
			break;
	
		
                ProcessConsoleInput();

                if ( !engine->RunFrame() )
                {
                        bDone = true;
                }

                sys->UpdateStatus( 0  /* don't force */ );
	}
	

/*
	char *p;
		static double oldtime = 0.0;

		double newtime;
		double dtime;

		// Try to allow other apps to get some CPU
		sys->Sleep( 1 );

		if ( !engine )
			break;

		while ( 1 )
		{
			newtime = engine->Sys_FloatTime();
			if ( newtime < oldtime )
			{
				oldtime = newtime - 0.05;
			}
			
			dtime = newtime - oldtime;

			if ( dtime > 0.001 )
				break;

			// Running really fast, yield some time to other apps
			sys->Sleep( 1 );
		}
		
		ProcessConsoleInput();

		Eng_Frame( 0, dtime );

		oldtime = newtime;
	}
	
*/
	engine->Shutdown();
	
	sys->GameShutdown();

	sys->DestroyConsoleWindow();

	iret = 1;

cleanup:

	// Shut down the file system
	FileSystem_Shutdown();

	return iret;
}
