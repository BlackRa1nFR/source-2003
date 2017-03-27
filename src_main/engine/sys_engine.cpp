#include "winquake.h"
#include "quakedef.h"
#include "dll_state.h"
#include <assert.h>
#include "engine_launcher_api.h"
#include "iengine.h"
#include "ivideomode.h"
#include "igame.h"
#include "vmodes.h"
#include "modes.h"
#include "cd.h"
#include "sys.h"
#include "host.h"
#include "keys.h"
#include "cdll_int.h"
#include "host_state.h"
#include "cdll_engine_int.h"
#include "sys_dll.h"
#include "vgui_int.h"
#include "tier0/vprof.h"
#include "profile.h"
#include "gl_matsysiface.h"
#include "vprof_engine.h"

#ifdef _WIN32
#include <ddraw.h>
#include <d3d.h>
#endif

void GameSetState(int iState );

void Sys_ShutdownGame( void );
int Sys_InitGame( CreateInterfaceFn appSystemFactory, 
			char const* pBaseDir, void *pwnd, int bIsDedicated );

// sleep time when not focus
#define NOT_FOCUS_SLEEP	50				

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEngine : public IEngine
{
public:
					CEngine( void );
	virtual			~CEngine( void );

	bool			Load( bool dedicated, const char *basedir );

	void			Unload( void );
	void			SetState(int iState);
	int				GetState( void );
	int				Frame( void );

	double			GetFrameTime( void );
	double			GetCurTime( void );
	
	void			TrapKey_Event( int key, bool down );
	void			TrapMouse_Event( int buttons, bool down );

	void			StartTrapMode( void );
	bool			IsTrapping( void );
	bool			CheckDoneTrapping( int& buttons, int& key );

	int				GetQuitting( void );
	void			SetQuitting( int quittype );

private:
	int				m_nQuitting;

	int				m_nDLLState;

	double			m_fCurTime;
	double			m_fFrameTime;
	double			m_fOldTime;


	bool			m_bTrapMode;
	bool			m_bDoneTrapping;
	int				m_nTrapKey;
	int				m_nTrapButtons;
};

static CEngine g_Engine;

IEngine *eng = ( IEngine * )&g_Engine;
//IEngineAPI *engine = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEngine::CEngine( void )
{
	m_nDLLState			= 0;

	m_fFrameTime		= 0.0f;
	m_fOldTime			= 0.0f;

	m_bTrapMode			= false;
	m_bDoneTrapping		= false;
	m_nTrapKey			= 0;
	m_nTrapButtons		= 0;	

	m_nQuitting			= QUIT_NOTQUITTING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEngine::~CEngine( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngine::Unload( void )
{
	Sys_ShutdownGame();

	m_nDLLState			= DLL_INACTIVE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::Load( bool dedicated, const char *rootdir )
{
	bool success = false;

	// Activate engine
	SetState( DLL_ACTIVE );

	if ( Sys_InitGame( 
		g_AppSystemFactory,
		rootdir, 
		game->GetMainWindowAddress(), 
		dedicated ) )
	{
		success = true;
	}

	if ( success )
	{
		// Always try and load in kb and cvar settings
		// DON'T DO THIS!!!  We've already loaded it in Host_Init!
		// Leaving here for now in case we need this for some reason.
#ifndef SWDS
		Host_ReadConfiguration();  // FIXME!!!!!!  Getting rid of this doesn't load the config when loading a map directly, but it is screwed with timedemo
#endif
		UpdateMaterialSystemConfig();
	}

	return success;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CEngine::Frame( void )
{
#ifndef SWDS
	PreUpdateProfile();
	ClientDLL_FrameStageNotify( FRAME_START );
	PostUpdateProfile();
#endif

	{ // profile scope

	VPROF( "CEngine::Frame" );
#ifndef SWDS
	cdaudio->Frame();
#endif

	// yield the CPU for a little while when paused, minimized, or not the focus
	// FIXME:  Move this to main windows message pump?
	if ( !game->IsActiveApp() )
	{
		game->SleepUntilInput( NOT_FOCUS_SLEEP );
	}

	m_fCurTime		= Sys_FloatTime();

	// Set frame time
	m_fFrameTime	= m_fCurTime - m_fOldTime;

	// If the time is < 0, that means we've restarted. 
	// Set the new time high enough so the engine will run a frame
	if ( m_fFrameTime < 0.001f )
	{
		return m_nDLLState;
	}

	// Remember old time
	m_fOldTime		= m_fCurTime;

	if ( m_nDLLState )
	{
		// Run the engine frame
		int iState = ::HostState_Frame( m_fFrameTime, m_nDLLState );

		// Has the state changed?
		if ( iState != m_nDLLState )
		{
			SetState( iState );

			switch( m_nDLLState )
			{
			case DLL_INACTIVE:
				break;
			case DLL_CLOSE:
				SetQuitting( QUIT_TODESKTOP );
				break;
			case DLL_RESTART:
				SetQuitting( QUIT_RESTART );
				break;
			}
		}
	}

	} // profile scope

	return ( m_nDLLState );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iState - 
//-----------------------------------------------------------------------------
void CEngine::SetState( int iState )
{
	m_nDLLState = iState;
	GameSetState( iState );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CEngine::GetState( void )
{
	return m_nDLLState;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : double
//-----------------------------------------------------------------------------
double CEngine::GetFrameTime( void )
{
	return m_fFrameTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : double
//-----------------------------------------------------------------------------
double CEngine::GetCurTime( void )
{
	return m_fCurTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : key - 
//			down - 
//-----------------------------------------------------------------------------
void CEngine::TrapKey_Event( int key, bool down )
{
#ifndef SWDS
	if ( VGui_IsGameUIVisible() || VGui_IsDebugSystemVisible() )
	{
		if ( down && key == K_ESCAPE )
		{
			if ( VGui_IsGameUIVisible() )
			{
				VGui_HideGameUI();
			}
			
			if ( VGui_IsDebugSystemVisible() )
			{
				VGui_HideDebugSystem();
			}
			return;
		}

		if ( down && key == K_F1 )
		{
			if ( VGui_IsShiftKeyDown() )
			{
				if ( VGui_IsDebugSystemVisible() )
				{
					VGui_HideDebugSystem();
				}
				else
				{
					VGui_ShowDebugSystem();
				}
			}
			return;
		}

		// Swallow ev
		if ( down && key != K_ESCAPE && key != '`' ) // only sink down events, let up ones go into the engine to
													 // to trigger key releases
		{
			return;
		}
	}

	// Only key down events are trapped
	if ( m_bTrapMode && down )
	{
		m_bTrapMode			= false;
		m_bDoneTrapping		= true;
		
		m_nTrapKey			= key;
		m_nTrapButtons		= 0;
		return;
	}

	Key_Event( key, down ? 1 : 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buttons - 
//			down - 
//-----------------------------------------------------------------------------
void CEngine::TrapMouse_Event( int buttons, bool down )
{
#ifndef SWDS
	if ( VGui_IsGameUIVisible() || VGui_IsDebugSystemVisible() )
		return;

	// buttons == 0 for mouse move events
	if ( m_bTrapMode && buttons && !down )
	{
		m_bTrapMode			= false;
		m_bDoneTrapping		= true;
		
		m_nTrapKey			= 0;
		m_nTrapButtons		= buttons;
		return;
	}

	if ( g_ClientDLL )
	{
		g_ClientDLL->IN_MouseEvent( buttons, down );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngine::StartTrapMode( void )
{
	if ( m_bTrapMode )
		return;

	m_bDoneTrapping = false;
	m_bTrapMode = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::IsTrapping( void )
{
	return m_bTrapMode;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buttons - 
//			key - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::CheckDoneTrapping( int& buttons, int& key )
{
	if ( m_bTrapMode )
		return false;

	if ( !m_bDoneTrapping )
		return false;

	key			= m_nTrapKey;
	buttons		= m_nTrapButtons;

	// Reset since we retrieved the results
	m_bDoneTrapping = false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Flag that we are in the process of quiting
//-----------------------------------------------------------------------------
void CEngine::SetQuitting( int quittype )
{
	m_nQuitting = quittype;
}

//-----------------------------------------------------------------------------
// Purpose: Check whether we are ready to exit
//-----------------------------------------------------------------------------
int CEngine::GetQuitting( void )
{
	return m_nQuitting;
}
