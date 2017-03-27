//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include <stdarg.h>
#include "baseflex.h"
#include "entitylist.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "choreoscene.h"
#include "ichoreoeventcallback.h"
#include "iscenetokenprocessor.h"
#include "studio.h"
#include "networkstringtable_gamedll.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"
#include "ai_navigator.h"
#include "saverestore_utlvector.h"



class CSceneEntity;
class CBaseFlex;

static ConVar scene_print( "scene_print", "0", 0, "When playing back a scene, print timing and event info to console." );

// Assume sound system is 100 msec lagged (only used if we can't find snd_mixahead cvar!)
#define SOUND_SYSTEM_LATENCY_DEFAULT ( 0.1f )

// Think every 50 msec (FIXME: Try 10hz?)
#define SCENE_THINK_INTERVAL 0.05

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFormat - 
//			... - 
// Output : static void
//-----------------------------------------------------------------------------
static void Scene_Printf( const char *pFormat, ... )
{
	if ( !scene_print.GetInt() )
		return;

	va_list marker;
	char msg[8192];

	va_start(marker, pFormat);
	Q_vsnprintf(msg, sizeof(msg), pFormat, marker);
	va_end(marker);	
	
	Msg( msg );
}

//-----------------------------------------------------------------------------
// Purpose: Embedded interface handler, just called method in m_pScene
//-----------------------------------------------------------------------------
class CChoreoEventCallback : public IChoreoEventCallback
{
public:
						CChoreoEventCallback( void );

	void				SetScene( CSceneEntity *scene );

	// Implements IChoreoEventCallback
	virtual void		StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void		EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void		ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool		CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

private:
	CSceneEntity		*m_pScene;
};

#define	SCENE_ACTION_UNKNOWN		0
#define SCENE_ACTION_CANCEL			1
#define SCENE_ACTION_RESUME			2

//-----------------------------------------------------------------------------
// Purpose: This class exists solely to call think on all scene entities in a deterministic order
//-----------------------------------------------------------------------------
class CSceneManager : public CLogicalEntity
{
	DECLARE_CLASS( CSceneManager, CLogicalEntity );
	DECLARE_DATADESC();

public:
	virtual void			Spawn()
	{
		BaseClass::Spawn();
		SetNextThink( gpGlobals->curtime );
	}

	virtual int				ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }

	virtual void			Think();

			void			ClearAllScenes();

			void			AddSceneEntity( CSceneEntity *scene );
			void			RemoveSceneEntity( CSceneEntity *scene );

			void			QueueRestoredSound( CBaseFlex *actor, char const *soundname, soundlevel_t soundlevel, float time_in_past );

			void			OnClientActive( CBasePlayer *player );

private:

	struct CRestoreSceneSound
	{
		CRestoreSceneSound()
		{
			actor = NULL;
			soundname[ 0 ] = NULL;
			soundlevel = SNDLVL_NORM;
			time_in_past = 0.0f;
		}

		CHandle< CBaseFlex >	actor;
		char					soundname[ 128 ];
		soundlevel_t			soundlevel;
		float					time_in_past;
	};

	CUtlVector< CHandle< CSceneEntity > >	m_ActiveScenes;

	CUtlVector< CRestoreSceneSound >		m_QueuedSceneSounds;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CSceneManager )

	DEFINE_UTLVECTOR( CSceneManager, m_ActiveScenes,	FIELD_EHANDLE ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Singleton scene manager.  Created by first placed scene or recreated it it's deleted for some unknown reason
// Output : CSceneManager
//-----------------------------------------------------------------------------
CSceneManager *GetSceneManager()
{
	// Create it if it doesn't exist
	static CHandle< CSceneManager >	s_SceneManager;
	if ( s_SceneManager == NULL )
	{
		s_SceneManager = ( CSceneManager * )CreateEntityByName( "scene_manager" );
		Assert( s_SceneManager );
		if ( s_SceneManager )
		{
			s_SceneManager->Spawn();
		}
	}

	Assert( s_SceneManager );
	return s_SceneManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void SceneManager_ClientActive( CBasePlayer *player )
{
	Assert( GetSceneManager() );

	if ( GetSceneManager() )
	{
		GetSceneManager()->OnClientActive( player );
	}
}

//-----------------------------------------------------------------------------
// Purpose: FIXME, need to deal with save/restore
//-----------------------------------------------------------------------------
class CSceneEntity : public CLogicalEntity
{
	friend class CInstancedSceneEntity;
public:
	DECLARE_CLASS( CSceneEntity, CLogicalEntity );

							CSceneEntity( void );
							~CSceneEntity( void );
				
	virtual void			Activate();

	virtual	void			Precache( void );
	virtual void			Spawn( void );
	virtual void			UpdateOnRemove( void );

	virtual void			OnRestore();

	DECLARE_DATADESC();

	virtual void			OnSceneFinished( bool canceled, bool fireoutput );

	virtual void			DoThink( float frametime );
	virtual void			PauseThink( void );


	bool					IsInterruptable();
	virtual void			ClearInterrupt();
	virtual void			CheckInterruptCompletion();

	virtual bool			InterruptThisScene( CSceneEntity *otherScene );
	void					RequestCompletionNotification( CSceneEntity *otherScene );

	virtual void			NotifyOfCompletion( CSceneEntity *interruptor );

	// Inputs
	void InputStartPlayback( inputdata_t &inputdata );
	void InputPausePlayback( inputdata_t &inputdata );
	void InputResumePlayback( inputdata_t &inputdata );
	void InputCancelPlayback( inputdata_t &inputdata );
	// Not enabled, see note below in datadescription table
	void InputReloadScene( inputdata_t &inputdata );

	virtual void StartPlayback( void );
	virtual void PausePlayback( void );
	virtual void ResumePlayback( void );
	virtual void CancelPlayback( void );
	virtual void ReloadScene( void );

	bool		 ValidScene() const;

	// From scene callback object
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool			CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	// Scene load/unload
	CChoreoScene			*LoadScene( const char *filename );

	void					LoadSceneFromFile( const char *filename );
	void					UnloadScene( void );

	// Event handlers
	virtual void			DispatchStartExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchProcessLookAt( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, float flIntensity, const char *szOptions );
	virtual void			DispatchStartMoveTo( CChoreoScene *scene, CBaseFlex *actor, const char *parameters );
	virtual	void			DispatchStartSpeak( CChoreoScene *scene, CBaseFlex *actor, const char *parameters, soundlevel_t iSoundlevel );
	virtual void			DispatchStartFace( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, float flIntensity, const char *szOptions );
	virtual void			DispatchStartSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartSubScene( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartInterrupt( CChoreoScene *scene, CChoreoEvent *event );
	virtual void			DispatchEndInterrupt( CChoreoScene *scene, CChoreoEvent *event );

	// Global events
	virtual void			DispatchProcessLoop( CChoreoScene *scene, CChoreoEvent *event );
	virtual void			DispatchPauseScene( CChoreoScene *scene, const char *parameters );
	virtual void			DispatchStopPoint( CChoreoScene *scene, const char *parameters );

	float					EstimateLength( void );

// Data
public:
	string_t				m_iszSceneFile;

	string_t				m_iszTarget1;
	string_t				m_iszTarget2;
	string_t				m_iszTarget3;
	string_t				m_iszTarget4;

	EHANDLE					m_hTarget1;
	EHANDLE					m_hTarget2;
	EHANDLE					m_hTarget3;
	EHANDLE					m_hTarget4;

	bool					m_bIsPlayingBack;
	bool					m_bPaused;
	float					m_flCurrentTime;
	float					m_flFrameTime;

	bool					m_bAutomated;
	int						m_nAutomatedAction;
	float					m_flAutomationDelay;
	float					m_flAutomationTime;

public:
	virtual CBaseFlex		*FindNamedActor( const char *name );
	virtual CBaseEntity		*FindNamedEntity( const char *name, CBaseEntity *pActor = NULL );

private:

	// Prevent derived classed from using this!
	virtual void			Think( void ) {};


	void					ClearExpressions( CChoreoScene *scene );
	void					ClearSchedules( CChoreoScene *scene );

	float					GetSoundSystemLatency( void );
	void					PrecacheScene( CChoreoScene *scene );

	// The underlying scene we are editing
	CChoreoScene			*m_pScene;
	CChoreoEventCallback	m_SceneCallback;

	const ConVar			*m_pcvSndMixahead;

	COutputEvent			m_OnStart;
	COutputEvent			m_OnCompletion;
	COutputEvent			m_OnCanceled;
	COutputEvent			m_OnTrigger1;
	COutputEvent			m_OnTrigger2;
	COutputEvent			m_OnTrigger3;
	COutputEvent			m_OnTrigger4;

	int						m_nInterruptCount;
	bool					m_bInterrupted;
	CHandle< CSceneEntity >	m_hInterruptScene;

	bool					m_bInterruptSceneFinished;
	CUtlVector< CHandle< CSceneEntity > >	m_hNotifySceneCompletion;

	bool					m_bRestoring;
	float					m_flRestoreTime;
};

LINK_ENTITY_TO_CLASS( logic_choreographed_scene, CSceneEntity );
LINK_ENTITY_TO_CLASS( scripted_scene, CSceneEntity );

BEGIN_DATADESC( CSceneEntity )

	// Keys
	DEFINE_KEYFIELD( CSceneEntity, m_iszSceneFile, FIELD_STRING, "SceneFile" ),

	DEFINE_KEYFIELD( CSceneEntity, m_iszTarget1, FIELD_STRING, "target1" ),
	DEFINE_KEYFIELD( CSceneEntity, m_iszTarget2, FIELD_STRING, "target2" ),
	DEFINE_KEYFIELD( CSceneEntity, m_iszTarget3, FIELD_STRING, "target3" ),
	DEFINE_KEYFIELD( CSceneEntity, m_iszTarget4, FIELD_STRING, "target4" ),

	DEFINE_FIELD( CSceneEntity, m_hTarget1, FIELD_EHANDLE ),
	DEFINE_FIELD( CSceneEntity, m_hTarget2, FIELD_EHANDLE ),
	DEFINE_FIELD( CSceneEntity, m_hTarget3, FIELD_EHANDLE ),
	DEFINE_FIELD( CSceneEntity, m_hTarget4, FIELD_EHANDLE ),

	DEFINE_FIELD( CSceneEntity, m_bIsPlayingBack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSceneEntity, m_bPaused, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSceneEntity, m_flCurrentTime, FIELD_FLOAT ),  // relative, not absolute time
	DEFINE_FIELD( CSceneEntity, m_flFrameTime, FIELD_FLOAT ),  // last frametime
	DEFINE_FIELD( CSceneEntity, m_bAutomated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSceneEntity, m_nAutomatedAction, FIELD_INTEGER ),
	DEFINE_FIELD( CSceneEntity, m_flAutomationDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CSceneEntity, m_flAutomationTime, FIELD_FLOAT ),  // relative, not absolute time

	// DEFINE_FIELD( CSceneEntity, m_pScene ) // Special processing used for this

	// These are set up in the constructor
	// DEFINE_FIELD( CSceneEntity, m_SceneCallback, CChoreoEventCallback ),
	// DEFINE_FIELD( CSceneEntity, m_pcvSndMixahead, FIELD_XXXXX ),
	// DEFINE_FIELD( CSceneEntity, m_bRestoring, FIELD_BOOLEAN ),
	// DEFINE_FIELD( CSceneEntity, m_flRestoreTime, FIELD_FLOAT ),

	DEFINE_FIELD( CSceneEntity, m_nInterruptCount, FIELD_INTEGER ),
	DEFINE_FIELD( CSceneEntity, m_bInterrupted, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSceneEntity, m_hInterruptScene, FIELD_EHANDLE ),
	DEFINE_FIELD( CSceneEntity, m_bInterruptSceneFinished, FIELD_BOOLEAN ),
	DEFINE_UTLVECTOR( CSceneEntity, m_hNotifySceneCompletion, FIELD_EHANDLE ),

	// Inputs
	DEFINE_INPUTFUNC( CSceneEntity, FIELD_VOID, "Start", InputStartPlayback ),
	DEFINE_INPUTFUNC( CSceneEntity, FIELD_VOID, "Pause", InputPausePlayback ),
	DEFINE_INPUTFUNC( CSceneEntity, FIELD_VOID, "Resume", InputResumePlayback ),
	DEFINE_INPUTFUNC( CSceneEntity, FIELD_VOID, "Cancel", InputCancelPlayback ),

//	DEFINE_INPUTFUNC( CSceneEntity, FIELD_VOID, "Reload", ReloadScene ),

	// Outputs
	DEFINE_OUTPUT( CSceneEntity, m_OnStart, "OnStart"),
	DEFINE_OUTPUT( CSceneEntity, m_OnCompletion, "OnCompletion"),
	DEFINE_OUTPUT( CSceneEntity, m_OnCanceled, "OnCanceled"),
	DEFINE_OUTPUT( CSceneEntity, m_OnTrigger1, "OnTrigger1"),
	DEFINE_OUTPUT( CSceneEntity, m_OnTrigger2, "OnTrigger2"),
	DEFINE_OUTPUT( CSceneEntity, m_OnTrigger3, "OnTrigger3"),
	DEFINE_OUTPUT( CSceneEntity, m_OnTrigger4, "OnTrigger4"),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEventCallback::CChoreoEventCallback( void )
{
	m_pScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CChoreoEventCallback::SetScene( CSceneEntity *scene )
{
	m_pScene = scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoEventCallback::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !m_pScene )
		return;

	m_pScene->StartEvent( currenttime, scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoEventCallback::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !m_pScene )
		return;

	m_pScene->EndEvent( currenttime, scene, event );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CChoreoEventCallback::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !m_pScene )
		return;

	m_pScene->ProcessEvent( currenttime, scene, event );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
bool CChoreoEventCallback::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !m_pScene )
		return true;

	return m_pScene->CheckEvent( currenttime, scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSceneEntity::CSceneEntity( void )
{
	m_bIsPlayingBack	= false;
	m_bPaused			= false;
	m_iszSceneFile		= NULL_STRING;
	m_flCurrentTime		= 0.0f;

	m_bAutomated		= false;
	m_nAutomatedAction	= SCENE_ACTION_UNKNOWN;
	m_flAutomationDelay = 0.0f;
	m_flAutomationTime = 0.0f;

	ClearInterrupt();

	m_pScene			= NULL;
	m_SceneCallback.SetScene( this );

	m_pcvSndMixahead	= cvar->FindVar( "snd_mixahead" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSceneEntity::~CSceneEntity( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::UpdateOnRemove( void )
{
	UnloadScene();
	BaseClass::UpdateOnRemove();

	if ( GetSceneManager() )
	{
		GetSceneManager()->RemoveSceneEntity( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::Activate()
{
	BaseClass::Activate();

	if ( GetSceneManager() )
	{
		GetSceneManager()->AddSceneEntity( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CSceneEntity::GetSoundSystemLatency( void )
{
	if ( m_pcvSndMixahead )
	{
		return m_pcvSndMixahead->GetFloat();
	}
	
	// Assume 100 msec sound system latency
	return SOUND_SYSTEM_LATENCY_DEFAULT;
}
		
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CSceneEntity::PrecacheScene( CChoreoScene *scene )
{
	Assert( scene );

	// Iterate events and precache necessary resources
	for ( int i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SPEAK:
			{
				// Defined in SoundEmitterSystem.cpp
				// NOTE:  The .wav's associated with .vcds are forced to preload to avoid
				//  loading hitches during triggering
				PrecacheScriptSound( event->GetParameters(), true );
			}
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( !subscene )
					{
						subscene = LoadScene( event->GetParameters() );
						subscene->SetSubScene( true );
						event->SetSubScene( subscene );

						// Now precache it's resources, if any
						PrecacheScene( subscene );
					}
				}
			}
			break;
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::Precache( void )
{
	LoadSceneFromFile( STRING( m_iszSceneFile ) );
	if ( !m_pScene )
		return;

	PrecacheScene( m_pScene );
}

void CSceneEntity::OnRestore()
{
	if ( !m_pScene )
		return;

	if ( !m_bIsPlayingBack )
		return;

	float dt = SCENE_THINK_INTERVAL;

	bool paused = m_bPaused;

	// Jump ahead to m_flCurrentTime
	m_flRestoreTime = m_flCurrentTime;

	m_flCurrentTime = 0.0f;
	m_bPaused = false;

	m_pScene->ResetSimulation();
	ClearInterrupt();

	m_bRestoring = true;

	while ( m_flCurrentTime < m_flRestoreTime && 
		!m_bPaused )
	{
		float oldT = m_flCurrentTime;
		DoThink( dt );
		if ( m_flCurrentTime == oldT )
		{
			Assert( 0 );
			break;
		}
	}

	m_bRestoring = false;
	if ( paused )
	{
		PausePlayback();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::Spawn( void )
{
	Precache();
}

void CSceneEntity::PauseThink( void )
{
	if ( !m_pScene )
		return;

	// Stay paused if pause occurred from interrupt
	if ( m_bInterrupted )
		return;

	// FIXME:  Game code should check for AI waiting conditions being met, etc.
	//
	//
	//
	bool bAllFinished = m_pScene->CheckEventCompletion( );

	if (bAllFinished)
	{
		// Perform action
		switch ( m_nAutomatedAction )
		{
		case SCENE_ACTION_RESUME:
			ResumePlayback();
			break;
		case SCENE_ACTION_CANCEL:
			CancelPlayback();
			break;
		default:
			ResumePlayback();
			break;
		}

		// Reset
		m_bAutomated = false;
		m_nAutomatedAction = SCENE_ACTION_UNKNOWN;
		m_flAutomationTime = 0.0f;
		m_flAutomationDelay = 0.0f;
		return;
	}

	// Otherwise, see if resume/cancel is automatic and act accordingly if enough time
	//  has passed
	if ( !m_bAutomated )
		return;

	m_flAutomationTime += (gpGlobals->curtime - GetLastThink());

	if ( m_flAutomationDelay > 0.0f &&
		m_flAutomationTime < m_flAutomationDelay )
		return;

	// Perform action
	switch ( m_nAutomatedAction )
	{
	case SCENE_ACTION_RESUME:
		Scene_Printf( "Automatically resuming playback\n" );
		ResumePlayback();
		break;
	case SCENE_ACTION_CANCEL:
		Scene_Printf( "Automatically canceling playback\n" );
		CancelPlayback();
		break;
	default:
		Scene_Printf( "Unknown action %i, automatically resuming playback\n", m_nAutomatedAction );
		ResumePlayback();
		break;
	}

	// Reset
	m_bAutomated = false;
	m_nAutomatedAction = SCENE_ACTION_UNKNOWN;
	m_flAutomationTime = 0.0f;
	m_flAutomationDelay = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchPauseScene( CChoreoScene *scene, const char *parameters )
{
	// Don't pause during restore, since we'll be restoring the pause state already
	if ( m_bRestoring )
		return;

	// FIXME:  Hook this up to AI, etc. somehow, perhaps poll each actor for conditions using
	//  scene resume condition iterator
	PausePlayback();

	char token[1024];

	m_bAutomated		= false;
	m_nAutomatedAction	= SCENE_ACTION_UNKNOWN;
	m_flAutomationDelay = 0.0f;
	m_flAutomationTime = 0.0f;

	// Check for auto resume/cancel
	char *buffer = (char *)parameters;
	buffer = engine->COM_ParseFile( buffer, token );
	if ( !stricmp( token, "automate" ) )
	{
		buffer = engine->COM_ParseFile( buffer, token );
		if ( !stricmp( token, "Cancel" ) )
		{
			m_nAutomatedAction = SCENE_ACTION_CANCEL;
		}
		else if ( !stricmp( token, "Resume" ) )
		{
			m_nAutomatedAction = SCENE_ACTION_RESUME;
		}

		if ( m_nAutomatedAction != SCENE_ACTION_UNKNOWN )
		{
			buffer = engine->COM_ParseFile( buffer, token );
			m_flAutomationDelay = (float)atof( token );

			if ( m_flAutomationDelay > 0.0f )
			{
				// Success
				m_bAutomated = true;
				m_flAutomationTime = 0.0f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	// Don't restore this event since it's implied in the current "state" of the scene timer, etc.
	if ( m_bRestoring )
		return;

	Assert( scene );
	Assert( event->GetType() == CChoreoEvent::LOOP );

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
	m_flCurrentTime = backtime;
}

//-----------------------------------------------------------------------------
// Purpose: Stop point is a placeholder for extending the end time of the scene
//  It doesn't do anything
// Input  : *scene - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStopPoint( CChoreoScene *scene, const char *parameters )
{
	// Stop points are symbolic, no action required
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneEntity::IsInterruptable()
{
	return ( m_nInterruptCount > 0 ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartInterrupt( CChoreoScene *scene, CChoreoEvent *event )
{
	// Don't re-interrupt during restore
	if ( m_bRestoring )
		return;

	++m_nInterruptCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndInterrupt( CChoreoScene *scene, CChoreoEvent *event )
{
	// Don't re-interrupt during restore
	if ( m_bRestoring )
		return;

	--m_nInterruptCount;

	if ( m_nInterruptCount < 0 )
	{
		m_nInterruptCount = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( m_pScene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( event );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*actor2 - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchProcessLookAt( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, float flIntensity, const char *szOptions )
{
	if ( !actor || !actor2 )
		return;

	CAI_BaseNPC *npc = actor->MyNPCPointer( );

	if (npc)
	{
		npc->AddLookTarget( actor2, flIntensity, 0.2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move to spot/actor
// FIXME:  Need to allow this to take arbitrary amount of time and pause playback
//  while waiting for actor to move into position
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartMoveTo( CChoreoScene *scene, CBaseFlex *actor, const char *parameters )
{
	// FIXME:  How to save/restore moveto state?
	if ( m_bRestoring )
		return;

	CAI_BaseNPC *npc = actor->MyNPCPointer( );

	if (npc)
	{
		CBaseEntity *pTarget = FindNamedEntity( parameters, actor );
		if (pTarget)
		{
			// UNDONE: This should probably use ScheduledMoveToGoalEntity or ScheduledMoveToGoalPosition
			// so it can specify the schedule (interrupts,etc) and activity.

			// npc->SetMoveType( MOVETYPE_STEP ); // E3 Hack
			npc->SetTarget(pTarget);
			npc->SetSchedule( SCHED_SCENE_WALK );
			npc->m_flMoveWaitFinished = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Playback sound file that contains phonemes
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartSpeak( CChoreoScene *scene, CBaseFlex *actor, const char *parameters, soundlevel_t iSoundlevel )
{
	// Emit sound
	if ( actor )
	{
		CPASAttenuationFilter filter( actor );

		float soundtime = gpGlobals->curtime;

		if ( m_bRestoring )
		{
			// Need to queue sounds on restore because the player has not yet connected
			float time_in_past = m_flRestoreTime - m_flCurrentTime;

			GetSceneManager()->QueueRestoredSound( actor, parameters, iSoundlevel, time_in_past );

			return;
		}

		EmitSound( filter, actor->entindex(), CHAN_VOICE, parameters, 1, iSoundlevel, 0, PITCH_NORM, NULL, soundtime ); // 
	}
	// FIXME: hook into talker system
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*actor2 - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartFace( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, float flIntensity, const char *szOptions )
{
	// TODO: make use of flIntensity?
	CAI_BaseNPC *npc = actor->MyNPCPointer( );

	if (npc)
	{
		if (actor2)
		{
			npc->SetTarget(actor2);
			npc->SetSchedule( SCHED_SCENE_FACE_TARGET );
		}
	}

    /*
	CAI_BaseNPC *npc = actor->MyNPCPointer( );

	if (npc && npc->GetMotor())
	{
		npc->GetMotor()->SetIdealYawToTarget( actor2->Center() );
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CSceneEntity::EstimateLength( void )
{
	return ( m_pScene ) ? m_pScene->FindStopTime( ) : 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::DoThink( float frametime )
{
	CheckInterruptCompletion();

	if ( !m_pScene )
		return;

	if ( !m_bIsPlayingBack )
		return;

	if ( m_bPaused )
	{
		PauseThink();
		return;
	}

	m_flFrameTime = frametime;

	m_pScene->SetSoundFileStartupLatency( GetSoundSystemLatency() );

	// Tell scene to go
	m_pScene->Think( m_flCurrentTime );

	// Drive simulation time for scene
	m_flCurrentTime += m_flFrameTime;

	// Did we get to the end
	if ( !m_bPaused && m_pScene->SimulationFinished() )
	{
		OnSceneFinished( false, true );

		// Stop them from doing anything special
		ClearSchedules( m_pScene );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handlers
//-----------------------------------------------------------------------------
void CSceneEntity::InputStartPlayback( inputdata_t &inputdata )
{
	StartPlayback();
}

void CSceneEntity::InputPausePlayback( inputdata_t &inputdata )
{
	PausePlayback();
}

void CSceneEntity::InputResumePlayback( inputdata_t &inputdata )
{
	ResumePlayback();
}

void CSceneEntity::InputCancelPlayback( inputdata_t &inputdata )
{
	CancelPlayback();
}

void CSceneEntity::InputReloadScene( inputdata_t &inputdata )
{
	ReloadScene();
}

//-----------------------------------------------------------------------------
// Purpose: Initiate scene playback
//-----------------------------------------------------------------------------
void CSceneEntity::StartPlayback( void )
{
	if ( !m_pScene )
		return;

	if ( m_bIsPlayingBack )
		return;

	m_bIsPlayingBack	= true;
	m_bPaused			= false;
	m_flCurrentTime		= 0.0f;
	m_pScene->ResetSimulation();
	ClearInterrupt();

	// Put face back in neutral pose
	ClearExpressions( m_pScene );

	m_OnStart.FireOutput( this, this, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::PausePlayback( void )
{
	if ( !m_bIsPlayingBack )
		return;

	if ( m_bPaused )
		return;

	m_bPaused = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::ResumePlayback( void )
{
	if ( !m_bIsPlayingBack )
		return;

	if ( !m_bPaused )
		return;

	// FIXME:  Iterate using m_pScene->IterateResumeConditionEvents and 
	//  only resume if the event conditions have all been satisfied

	// FIXME:  Just resume for now
	m_pScene->ResumeSimulation();

	m_bPaused = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::CancelPlayback( void )
{
	if ( !m_bIsPlayingBack )
		return;

	m_bIsPlayingBack	= false;
	m_bPaused			= false;

	OnSceneFinished( true, false );
}

//-----------------------------------------------------------------------------
// Purpose: Reload from hard drive
//-----------------------------------------------------------------------------
void CSceneEntity::ReloadScene( void )
{
	CancelPlayback();

	Msg( "Reloading %s from disk\n", STRING( m_iszSceneFile ) );

	// Note:  LoadSceneFromFile calls UnloadScene()
	LoadSceneFromFile( STRING( m_iszSceneFile ) );
}

//-----------------------------------------------------------------------------
// Purpose: Query whether the scene actually loaded. Only meaninful after Spawn()
//-----------------------------------------------------------------------------
bool CSceneEntity::ValidScene() const
{
	return ( m_pScene != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActor - 
//			*scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartSubScene( CChoreoScene *scene, CBaseFlex *pActor, CChoreoEvent *event)
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
			subscene->ResetSimulation();
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: All events are leading edge triggered
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	CBaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor->GetName() );
		if (pActor == NULL)
		{
			Warning( "CSceneEntity unable find actor \"%s\"\n", actor->GetName() );
			return;
		}
	}

	Scene_Printf( "%8.4f:  start %s\n", currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
			if ( pActor )
			{
				DispatchStartSubScene( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor )
			{
				DispatchStartExpression( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor )
			{
				DispatchStartFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::LOOKAT:
		{
			if ( pActor )
			{
				CBaseEntity *pActor2 = NULL;
				pActor2 = FindNamedEntity( event->GetParameters( ), pActor );
				if ( pActor2 )
				{
					// Huh?
					DispatchProcessLookAt( scene, pActor, pActor2, event->GetIntensity( currenttime ), event->GetParameters2( ) );
				}
			}
		}
		break;
	case CChoreoEvent::SPEAK:
		{
			// Speaking is edge triggered

			// FIXME: dB hack.  soundlevel needs to be moved into inside of wav?
			soundlevel_t iSoundlevel = SNDLVL_TALKING;
			if (event->GetParameters2())
			{
				iSoundlevel = (soundlevel_t)atoi( event->GetParameters2() );
				if (iSoundlevel == SNDLVL_NONE)
					iSoundlevel = SNDLVL_TALKING;
			}

			DispatchStartSpeak( scene, pActor, event->GetParameters(), iSoundlevel );
		}
		break;
	case CChoreoEvent::MOVETO:
		{
			// Moveto is edge triggered
			// FIXME:  Correct?
			DispatchStartMoveTo( scene, pActor, event->GetParameters() );
		}
		break;
	case CChoreoEvent::FACE:
		{
			if ( pActor )
			{
				CBaseEntity *pActor2 = FindNamedEntity( event->GetParameters(), pActor );

				if ( pActor2 )
				{
					DispatchStartFace( scene, pActor, pActor2, event->GetIntensity( currenttime ), event->GetParameters2( ) );
				}
			}
		}
		break;
	case CChoreoEvent::GESTURE:
		{
			// Gesture is edge triggered
			// FIXME:  Correct?
			DispatchStartGesture( scene, pActor, event );
		}
		break;
	case CChoreoEvent::FIRETRIGGER:
		{
			// FIXME:  how do I decide who fired it??
			switch( atoi( event->GetParameters() ) )
			{
			case 1:
				m_OnTrigger1.FireOutput( pActor, this, 0 );
				break;
			case 2:
				m_OnTrigger2.FireOutput( pActor, this, 0 );
				break;
			case 3:
				m_OnTrigger3.FireOutput( pActor, this, 0 );
				break;
			case 4:
				m_OnTrigger4.FireOutput( pActor, this, 0 );
				break;
			}
		}
		break;
	case CChoreoEvent::SEQUENCE:
		{
			// Sequence is edge triggered
			// FIXME:  Correct?
			DispatchStartSequence( scene, pActor, event );
		}
		break;
	case CChoreoEvent::SECTION:
		{
			// Pauses scene playback
			DispatchPauseScene( scene, event->GetParameters() );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			DispatchProcessLoop( scene, event );
		}
		break;
	case CChoreoEvent::INTERRUPT:
		{
			DispatchStartInterrupt( scene, event );
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
void CSceneEntity::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	CBaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor->GetName() );
	}

	Scene_Printf( "%8.4f:  finish %s\n", currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor )
			{
				DispatchEndExpression( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor )
			{
				DispatchEndFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::GESTURE:
		{
			if ( pActor )
			{
				DispatchEndGesture( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SEQUENCE:
		{
			if ( pActor )
			{
				DispatchEndSequence( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SUBSCENE:
		{
			CChoreoScene *subscene = event->GetSubScene();
			if ( subscene )
			{
				subscene->ResetSimulation();
			}
		}
		break;
	case CChoreoEvent::INTERRUPT:
		{
			DispatchEndInterrupt( scene, event );
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper for parsing scene data file
//-----------------------------------------------------------------------------
class CSceneTokenProcessor : public ISceneTokenProcessor
{
public:
	const char	*CurrentToken( void );
	bool		GetToken( bool crossline );
	bool		TokenAvailable( void );
	void		Error( const char *fmt, ... );
	void		SetBuffer( char *buffer );
private:
	char		*m_pBuffer;
	char		m_szToken[ 1024 ];
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CSceneTokenProcessor::CurrentToken( void )
{
	return m_szToken;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : crossline - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::GetToken( bool crossline )
{
	// NOTE: crossline is ignored here, may need to implement if needed
	m_pBuffer = engine->COM_ParseFile( m_pBuffer, m_szToken );
	if ( strlen( m_szToken ) >= 0 )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::TokenAvailable( void )
{
	if ( m_pBuffer[ 0 ] == '\n' )
		return false;

	if (m_pBuffer[ 0 ] == ';' || m_pBuffer[ 0 ] == '#' ||		 // semicolon and # is comment field
		(m_pBuffer[ 0 ] == '/' && m_pBuffer[ 1 ] == '/'))		// also make // a comment field
		return false;

	return true;
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
	Q_vsnprintf( string, sizeof(string), fmt, argptr );
	va_end( argptr );

	Warning( "%s", string );
	Assert(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buffer - 
//-----------------------------------------------------------------------------
void CSceneTokenProcessor::SetBuffer( char *buffer )
{
	m_pBuffer = buffer;
}

static CSceneTokenProcessor g_TokenProcessor;

CChoreoScene *CSceneEntity::LoadScene( const char *filename )
{
	// Load the file
	// FIXME:  use shared file system
	char *buffer = (char *)engine->COM_LoadFile( filename, 2, NULL );
	if ( !buffer )
		return NULL;

	g_TokenProcessor.SetBuffer( buffer );
	CChoreoScene *scene = ChoreoLoadScene( &m_SceneCallback, &g_TokenProcessor, Scene_Printf );
	return scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CSceneEntity::LoadSceneFromFile( const char *filename )
{
	UnloadScene();

	m_pScene = LoadScene( filename );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::UnloadScene( void )
{
	if ( m_pScene )
	{
		ClearExpressions( m_pScene );
	}
	delete m_pScene;
	m_pScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame that an event is active (Start/EndEvent as also
//  called)
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CSceneEntity::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	CBaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor->GetName() );
		if (pActor == NULL)
		{
			Warning( "CSceneEntity unable find actor \"%s\"\n", actor->GetName() );
			return;
		}
	}

	switch ( event->GetType() )
	{
	case CChoreoEvent::LOOKAT:
		{
			if ( pActor )
			{
				// FIXME: We really need to be caching off the pActor computed from the StartEvent() version
				CBaseEntity *pActor2 = FindNamedEntity( event->GetParameters( ), pActor );
				if ( pActor2 )
				{
					DispatchProcessLookAt( scene, pActor, pActor2, event->GetIntensity( currenttime ), event->GetParameters2( ) );
				}
			}
		}
		break;

	case CChoreoEvent::SUBSCENE:
		{
			Assert( event->GetType() == CChoreoEvent::SUBSCENE );

			CChoreoScene *subscene = event->GetSubScene();
			if ( !subscene )
				return;

			if ( subscene->SimulationFinished() )
				return;
			
			// Have subscenes think for appropriate time
			subscene->Think( m_flFrameTime );
		}
		break;

	default:
		break;
	}
	
	return;
}



//-----------------------------------------------------------------------------
// Purpose: Called for events that are part of a pause condition
// Input  : *event - 
// Output : Returns true on event completed, false on non-completion.
//-----------------------------------------------------------------------------
bool CSceneEntity::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	CBaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor->GetName() );
		if (pActor == NULL)
		{
			Warning( "CSceneEntity unable find actor \"%s\"\n", actor->GetName() );
			return true;
		}
	}

	switch ( event->GetType() )
	{
	case CChoreoEvent::MOVETO:
		{
			CAI_BaseNPC *npc = pActor->MyNPCPointer( );

			if (npc)
			{
				// check movement, check arrival
				if (npc->GetNavigator()->IsGoalActive())
				{
					return false;
				}
			}
		}
		break;
	case CChoreoEvent::SUBSCENE:
		{
		}
		break;
	default:
		break;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseFlex *CSceneEntity::FindNamedActor( const char *name )
{
	CBaseEntity *entity = FindNamedEntity( name );

	if ( !entity )
	{
		// Couldn't find actor!
		return NULL;
	}

	// Make sure it can actually do facial animation, etc.
	CBaseFlex *flexEntity = dynamic_cast< CBaseFlex * >( entity );
	if ( !flexEntity )
	{
		// That actor was not a CBaseFlex!
		return NULL;
	}

	return flexEntity;
}


//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseEntity *CSceneEntity::FindNamedEntity( const char *name, CBaseEntity *pActor )
{
	CBaseEntity *entity = NULL;

	if ( !stricmp( name, "Player" ) || !stricmp( name, "!player" ))
	{
		entity = ( CBaseEntity * )UTIL_PlayerByIndex( 1 );
	}
	else if ( !stricmp( name, "!target1" ) )
	{
		if (m_hTarget1 == NULL)
		{
			m_hTarget1 = gEntList.FindEntityByName( NULL, m_iszTarget1, NULL );
		}
		return m_hTarget1;
	}
	else if ( !stricmp( name, "!target2" ) )
	{
		if (m_hTarget2 == NULL)
		{
			m_hTarget2 = gEntList.FindEntityByName( NULL, m_iszTarget2, NULL );
		}
		return m_hTarget2;
	}
	else if ( !stricmp( name, "!target3" ) )
	{
		if (m_hTarget3 == NULL)
		{
			m_hTarget3 = gEntList.FindEntityByName( NULL, m_iszTarget3, NULL );
		}
		return m_hTarget3;
	}
	else if ( !stricmp( name, "!target4" ) )
	{
		if (m_hTarget4 == NULL)
		{
			m_hTarget4 = gEntList.FindEntityByName( NULL, m_iszTarget4, NULL );
		}
		return m_hTarget4;
	}
	else if (pActor && pActor->MyNPCPointer())
	{
		entity = pActor->MyNPCPointer()->FindNamedEntity( name );
	}	
	else
	{
		entity = gEntList.FindEntityByName( NULL, name, pActor );
	}

	return entity;
}

//-----------------------------------------------------------------------------
// Purpose: Remove all "scene" expressions from all actors in this scene
//-----------------------------------------------------------------------------
void CSceneEntity::ClearExpressions( CChoreoScene *scene )
{
	if ( !m_pScene )
		return;

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor *actor = m_pScene->GetActor( i );
		if ( !actor )
			continue;

		CBaseFlex *pActor = FindNamedActor( actor->GetName() );
		if ( !pActor )
			continue;

		// Clear any existing expressions
		pActor->ClearExpressions( scene );
	}

	// Iterate events and precache necessary resources
	for ( i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( subscene )
					{
						ClearExpressions( subscene );
					}
				}
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Remove all imposed schedules from all actors in this scene
//-----------------------------------------------------------------------------
void CSceneEntity::ClearSchedules( CChoreoScene *scene )
{
	if ( !m_pScene )
		return;

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor *actor = m_pScene->GetActor( i );
		if ( !actor )
			continue;

		CBaseFlex *pActor = FindNamedActor( actor->GetName() );
		if ( !pActor )
			continue;

		CAI_BaseNPC *pNPC = pActor->MyNPCPointer();

		if ( pNPC )
		{
			if ( pNPC->IsCurSchedule( SCHED_SCENE_SEQUENCE ) )
				pNPC->ClearSchedule();
		}
		else
		{
			pActor->ResetSequence( pActor->SelectWeightedSequence( ACT_IDLE ) );
			pActor->m_flCycle = 0;
		}
		// Clear any existing expressions
	}

	// Iterate events and precache necessary resources
	for ( i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( subscene )
					{
						ClearSchedules( subscene );
					}
				}
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: If we are currently interruptable, pause this scene and wait for the other
//  scene to finish
// Input  : *otherScene - 
//-----------------------------------------------------------------------------
bool CSceneEntity::InterruptThisScene( CSceneEntity *otherScene )
{
	Assert( otherScene );

	if ( !IsInterruptable() )
	{
		return false;
	}

	// Already interrupted
	if ( m_bInterrupted )
	{
		return false;
	}

	m_bInterrupted		= true;
	m_hInterruptScene	= otherScene;

	// Ask other scene to tell us when it's finished or canceled
	otherScene->RequestCompletionNotification( this );

	PausePlayback();
	return true;
}

/*
void scene_interrupt()
{
	if ( engine->Cmd_Argc() != 3 )
		return;

	const char *scene1 = engine->Cmd_Argv(1);
	const char *scene2 = engine->Cmd_Argv(2);

	CSceneEntity *s1 = dynamic_cast< CSceneEntity * >( gEntList.FindEntityByName( NULL, scene1, NULL ) );
	CSceneEntity *s2 = dynamic_cast< CSceneEntity * >( gEntList.FindEntityByName( NULL, scene2, NULL ) );

	if ( !s1 || !s2 )
		return;

	if ( s1->InterruptThisScene( s2 ) )
	{
		s2->StartPlayback();
	}
}

static ConCommand interruptscene( "int", scene_interrupt, "interrupt scene 1 with scene 2.", FCVAR_CHEAT );
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::CheckInterruptCompletion()
{
	if ( !m_bInterrupted )
		return;

	// If the interruptor goes away it's the same as having that scene finish up...
	if ( m_hInterruptScene != NULL && 
		!m_bInterruptSceneFinished )
	{
		return;
	}

	m_bInterrupted = false;
	m_hInterruptScene = NULL;

	ResumePlayback();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::ClearInterrupt()
{
	m_nInterruptCount = 0;
	m_bInterrupted = false;
	m_hInterruptScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Another scene is asking us to notify upon completion
// Input  : *notify - 
//-----------------------------------------------------------------------------
void CSceneEntity::RequestCompletionNotification( CSceneEntity *notify )
{
	CHandle< CSceneEntity > h;
	h = notify;
	// Only add it once
	if ( m_hNotifySceneCompletion.Find( h ) == m_hNotifySceneCompletion.InvalidIndex() )
	{
		m_hNotifySceneCompletion.AddToTail( h );
	}
}

//-----------------------------------------------------------------------------
// Purpose: An interrupt scene has finished or been canceled, we can resume once we pick up this state in CheckInterruptCompletion
// Input  : *interruptor - 
//-----------------------------------------------------------------------------
void CSceneEntity::NotifyOfCompletion( CSceneEntity *interruptor )
{
	Assert( m_bInterrupted );
	Assert( m_hInterruptScene == interruptor );
	m_bInterruptSceneFinished = true;

	CheckInterruptCompletion();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a scene is completed or canceled
//-----------------------------------------------------------------------------
void CSceneEntity::OnSceneFinished( bool canceled, bool fireoutput )
{
	if ( !m_pScene )
		return;

	// Notify any listeners
	int c = m_hNotifySceneCompletion.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *ent = m_hNotifySceneCompletion[ i ].Get();
		if ( !ent )
			continue;

		ent->NotifyOfCompletion( this );
	}
	m_hNotifySceneCompletion.RemoveAll();

	// Clear simulation
	m_pScene->ResetSimulation();
	m_bIsPlayingBack = false;
	m_bPaused = false;
	m_flCurrentTime = 0.0f;
	
	// Clear interrupt state if we were interrupted for some reason
	ClearInterrupt();

	if ( fireoutput )
	{
		m_OnCompletion.FireOutput( this, this, 0 );
	}

	// Put face back in neutral pose
	ClearExpressions( m_pScene );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
class CInstancedSceneEntity : public CSceneEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CInstancedSceneEntity, CSceneEntity ); 
public:
	EHANDLE					m_hOwner;

	virtual void			DoThink( float frametime );
	virtual CBaseFlex		*FindNamedActor( const char *name );
	virtual CBaseEntity		*FindNamedEntity( const char *name );

	virtual void			DispatchStartMoveTo( CBaseFlex *actor, const char *parameters ) { /* suppress */ };
	virtual void			DispatchStartFace( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, float flIntensity, const char *szOptions ) { /* suppress */ };
	virtual void			DispatchStartSequence( CBaseFlex *actor, const char *parameters ) { /* suppress */ };
	virtual void			DispatchPauseScene( const char *parameters ) { /* suppress */ };
};


LINK_ENTITY_TO_CLASS( instanced_scripted_scene, CInstancedSceneEntity );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CInstancedSceneEntity )

	DEFINE_FIELD( CInstancedSceneEntity, m_hOwner,		FIELD_EHANDLE ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: create a one-shot scene, no movement, sequences, etc.
// Input  :
// Output :
//-----------------------------------------------------------------------------

float InstancedScriptedScene( CBaseFlex *pActor, const char *pszScene, EHANDLE *phSceneEnt )
{
	CInstancedSceneEntity *pScene = (CInstancedSceneEntity *)CBaseEntity::CreateNoSpawn( "instanced_scripted_scene", vec3_origin, vec3_angle );

	// FIXME: doesn't this need a constant pointer (char * const)?
	// FIXME: set with keyvalue???
	pScene->m_iszSceneFile = MAKE_STRING( pszScene );

	// FIXME: I should set my output to fire something that kills me....

	// FIXME: add a proper initialization function
	pScene->m_hOwner = pActor;
	pScene->Spawn();
	pScene->Activate();
	pScene->StartPlayback();

	if ( !pScene->ValidScene() )
	{
		Msg( "Unknown scene specified: \"%s\"\n", pszScene );
		if ( phSceneEnt )
		{
			*phSceneEnt = NULL;
		}
		return 0;
	}

	if ( phSceneEnt )
	{
		*phSceneEnt = pScene;
	}

	return pScene->EstimateLength();
}


//-----------------------------------------------------------------------------

void StopInstancedScriptedScene( CBaseFlex *pActor, EHANDLE hSceneEnt )
{
	CBaseEntity *pEntity = hSceneEnt;
	CInstancedSceneEntity *pScene = (CInstancedSceneEntity *)pEntity;

	if ( pScene )
	{
		Assert( dynamic_cast<CInstancedSceneEntity *>(pEntity) != NULL );
		pScene->CancelPlayback();
	}
}



//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CInstancedSceneEntity::DoThink( float frametime )
{
	CheckInterruptCompletion();

	if ( !m_pScene || !m_bIsPlayingBack || m_hOwner == NULL)
	{
		UTIL_Remove( this );
		return;
	}

	if ( m_bPaused )
	{
		PauseThink();
		return;
	}

	float dt = frametime;

	m_pScene->SetSoundFileStartupLatency( GetSoundSystemLatency() );

	// Tell scene to go
	m_pScene->Think( m_flCurrentTime );

	// Drive simulation time for scene
	m_flCurrentTime += dt;

	// Did we get to the end
	if ( m_pScene->SimulationFinished() )
	{
		OnSceneFinished( false, false );

		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseFlex *CInstancedSceneEntity::FindNamedActor( const char *name )
{
	if (m_hOwner != NULL)
	{
		CAI_BaseNPC	*npc = m_hOwner->MyNPCPointer();
		if (npc)
		{
			return npc;
		}
	}
	return BaseClass::FindNamedActor( name );
}


//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseEntity *CInstancedSceneEntity::FindNamedEntity( const char *name )
{
	CBaseEntity *pOther = NULL;

	if (m_hOwner != NULL)
	{
		CAI_BaseNPC	*npc = m_hOwner->MyNPCPointer();

		if (npc)
		{
			pOther = npc->FindNamedEntity( name );
		}
	}

	if (!pOther)
	{
		pOther = BaseClass::FindNamedEntity( name );
	}
	return pOther;
}



LINK_ENTITY_TO_CLASS( scene_manager, CSceneManager );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneManager::Think()
{
	// The manager is always thinking at 20 hz
	SetNextThink( gpGlobals->curtime + SCENE_THINK_INTERVAL );
	float frameTime = ( gpGlobals->curtime - GetLastThink() );
	frameTime = min( 0.1, frameTime );

	bool needCleanupPass = false;
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *scene = m_ActiveScenes[ i ].Get();
		if ( !scene )
		{
			needCleanupPass = true;
			continue;
		}

		scene->DoThink( frameTime );

		if ( m_ActiveScenes.Count() < c )
		{
			// Scene removed self while thinking. Adjust iteration.
			c = m_ActiveScenes.Count();
			i--;
		}
	}

	// Now delete any invalid ones
	if ( needCleanupPass )
	{
		for ( int i = c - 1; i >= 0; i-- )
		{
			CSceneEntity *scene = m_ActiveScenes[ i ].Get();
			if ( scene )
				continue;

			m_ActiveScenes.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneManager::ClearAllScenes()
{
	m_ActiveScenes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CSceneManager::AddSceneEntity( CSceneEntity *scene )
{
	CHandle< CSceneEntity > h;
	
	h = scene;

	// Already added/activated
	if ( m_ActiveScenes.Find( h ) != m_ActiveScenes.InvalidIndex() )
	{
		return;
	}

	m_ActiveScenes.AddToTail( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CSceneManager::RemoveSceneEntity( CSceneEntity *scene )
{
	CHandle< CSceneEntity > h;
	
	h = scene;

	Assert( m_ActiveScenes.Find( h ) != m_ActiveScenes.InvalidIndex() );

	m_ActiveScenes.FindAndRemove( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CSceneManager::OnClientActive( CBasePlayer *player )
{
	int c = m_QueuedSceneSounds.Count();
	for ( int i = 0; i < c; i++ )
	{
		CRestoreSceneSound *sound = &m_QueuedSceneSounds[ i ];

		if ( sound->actor == NULL )
			continue;

		// Blow off sounds more than 10 seconds in past
		if ( sound->time_in_past > 10.0f )
			continue;

		CPASAttenuationFilter filter( sound->actor );
		EmitSound( filter, sound->actor->entindex(), CHAN_VOICE, sound->soundname, 1, sound->soundlevel, 0, 
			PITCH_NORM, NULL, gpGlobals->curtime - sound->time_in_past ); // 
	}

	m_QueuedSceneSounds.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*soundname - 
//			soundlevel - 
//			soundtime - 
//-----------------------------------------------------------------------------
void CSceneManager::QueueRestoredSound( CBaseFlex *actor, char const *soundname, soundlevel_t soundlevel, float time_in_past )
{
	CRestoreSceneSound e;
	e.actor = actor;
	Q_strncpy( e.soundname, soundname, sizeof( e.soundname ) );
	e.soundlevel = soundlevel;
	e.time_in_past = time_in_past;

	m_QueuedSceneSounds.AddToTail( e );
}
