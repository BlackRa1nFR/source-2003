//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "glquake.h"
#include "cl_demoaction.h"
#include <KeyValues.h>
#include "cl_demoactionmanager.h"
#include "demo.h"
#include "shake.h"
#include "cdll_engine_int.h"
#include "tmessage.h"
#include "sound.h"
#include "soundflags.h"
#include "screen.h"
#include "cl_demoaction_types.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : demoframe - 
//			demotime - 
//-----------------------------------------------------------------------------
bool CDemoActionSkipAhead::Update( const DemoActionTimingContext& tc )
{
	// Not active yet
	if ( !BaseClass::Update( tc ) )
		return false;

	if ( GetActionFired() )
	{
		return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionSkipAhead::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetSkipToFrame( pInitData->GetInt( "skiptoframe", -1 ) );
	SetSkipToTime( pInitData->GetFloat( "skiptotime", -1.0f ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionSkipAhead::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	if ( m_nSkipToFrame != -1 )
	{
		BufPrintf( depth, buf, "skiptoframe \"%i\"\n",
			m_nSkipToFrame );
	}
	else
	{
		if ( m_flSkipToTime != -1.0f )
		{
			BufPrintf( depth, buf, "skiptotime \"%.3f\"\n",
				m_flSkipToTime );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//-----------------------------------------------------------------------------
void CDemoActionSkipAhead::SetSkipToFrame( int frame )
{
	m_bUsingSkipFrame = frame != -1;
	m_nSkipToFrame = frame;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
void CDemoActionSkipAhead::SetSkipToTime( float t )
{
	m_bUsingSkipFrame = !( t != -1.0f );
	m_flSkipToTime = t;
}

void CDemoActionSkipAhead::FireAction( void )
{
	// demo->StartSkippingAhead( m_bUsingSkipFrame, m_nSkipToFrame, m_flSkipToTime );
	// demo->PushDemoAction( GetActionName() );

	// Don't this this, instead wait for demo to notify us that it's finished skipping
	// SetFinishedAction( true );
	if ( m_bUsingSkipFrame )
	{
		demo->SkipToFrame( m_nSkipToFrame );
	}
	else
	{
		demo->SkipToTime( m_flSkipToTime );
	}
	SetFinishedAction( true );
	SCR_BeginLoadingPlaque ();
}

DECLARE_DEMOACTION( DEMO_ACTION_SKIPAHEAD, CDemoActionSkipAhead );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : demoframe - 
//			demotime - 
//-----------------------------------------------------------------------------
bool CDemoActionStopPlayback::Update( const DemoActionTimingContext& tc )
{
	// Not active yet
	if ( !BaseClass::Update( tc ) )
		return false;

	if ( GetActionFired() )
	{
		return true;
	}

	return true;
}

void CDemoActionStopPlayback::FireAction( void )
{
	if ( demo->IsPlayingBack() )
	{
		Cbuf_AddText( "disconnect\n" );
	}
	SetActionFired( true );
}

DECLARE_DEMOACTION( DEMO_ACTION_STOPPLAYBACK, CDemoActionStopPlayback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionPlayCommands::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetCommandStream( pInitData->GetString( "commands", "" ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *stream - 
//-----------------------------------------------------------------------------
void CDemoActionPlayCommands::SetCommandStream( char const *stream )
{
	Q_strncpy( m_szCommandStream, stream, sizeof( m_szCommandStream ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char
//-----------------------------------------------------------------------------
char const	*CDemoActionPlayCommands::GetCommandStream( void ) const
{
	return m_szCommandStream;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionPlayCommands::FireAction( void )
{
	if( GetCommandStream()[0] )
	{
		Cbuf_AddText( va( "%s\n", GetCommandStream() ) );
	}
	SetFinishedAction( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionPlayCommands::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	BufPrintf( depth, buf, "commands \"%s\"\n",
		GetCommandStream() );
}

DECLARE_DEMOACTION( DEMO_ACTION_PLAYCOMMANDS, CDemoActionPlayCommands );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionScreenFadeStart::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	float duration = pInitData->GetFloat( "duration", 0.0f );
	float holdTime = pInitData->GetFloat( "holdtime", 0.0f );
	int fadein = pInitData->GetInt( "FFADE_IN", 0 );
	int fadeout = pInitData->GetInt( "FFADE_OUT", 0 );
	int fademodulate = pInitData->GetInt( "FFADE_MODULATE", 0 );
	int fadestayout = pInitData->GetInt( "FFADE_STAYOUT", 0 );
	int fadepurge = pInitData->GetInt( "FFADE_PURGE", 0 );
	int r = pInitData->GetInt( "r", 255 );
	int g = pInitData->GetInt( "g", 255 );
	int b = pInitData->GetInt( "b", 255 );
	int a = pInitData->GetInt( "a", 255 );

	fade.duration = (unsigned short)((float)(1<<SCREENFADE_FRACBITS) * duration );
	fade.holdTime = (unsigned short)((float)(1<<SCREENFADE_FRACBITS) * holdTime );
	
	fade.fadeFlags = 0;

	if ( fadein )
	{
		fade.fadeFlags |= FFADE_IN;
	}
	if ( fadeout )
	{
		fade.fadeFlags |= FFADE_OUT;
	}
	if ( fademodulate )
	{
		fade.fadeFlags |= FFADE_MODULATE;
	}
	if ( fadestayout )
	{
		fade.fadeFlags |= FFADE_STAYOUT;
	}
	if ( fadepurge )
	{
		fade.fadeFlags |= FFADE_PURGE;
	}

	fade.r = r;
	fade.g = g;
	fade.b = b;
	fade.a = a;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : ScreenFade_t const
//-----------------------------------------------------------------------------
ScreenFade_t *CDemoActionScreenFadeStart::GetScreenFade( void )
{
	return &fade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionScreenFadeStart::FireAction( void )
{
	g_ClientDLL->View_Fade( (ScreenFade_t *)GetScreenFade() );
	SetFinishedAction( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionScreenFadeStart::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	ScreenFade_t const *f = GetScreenFade();

	float duration = f->duration * (1.0f/(float)(1<<SCREENFADE_FRACBITS));
	float holdTime = f->holdTime * (1.0f/(float)(1<<SCREENFADE_FRACBITS));
	int fadein = f->fadeFlags & FFADE_IN;
	int fadeout = f->fadeFlags & FFADE_OUT;
	int fademodulate = f->fadeFlags & FFADE_MODULATE;
	int fadestayout = f->fadeFlags & FFADE_STAYOUT;
	int fadepurge = f->fadeFlags & FFADE_PURGE;

	BufPrintf( depth, buf, "duration \"%.3f\"\n", duration );
	BufPrintf( depth, buf, "holdtime \"%.3f\"\n", holdTime );

	if ( fadein > 0 ) BufPrintf( depth, buf, "FFADE_IN \"1\"\n" );
	if ( fadeout > 0 ) BufPrintf( depth, buf, "FFADE_OUT \"1\"\n" );
	if ( fademodulate > 0 ) BufPrintf( depth, buf, "FFADE_MODULATE \"1\"\n" );
	if ( fadestayout > 0 ) BufPrintf( depth, buf, "FFADE_STAYOUT \"1\"\n" );
	if ( fadepurge > 0 ) BufPrintf( depth, buf, "FFADE_PURGE \"1\"\n" );

	BufPrintf( depth, buf, "r \"%i\"\n", f->r );
	BufPrintf( depth, buf, "g \"%i\"\n", f->g );
	BufPrintf( depth, buf, "b \"%i\"\n", f->b );
	BufPrintf( depth, buf, "a \"%i\"\n", f->a );
}

DECLARE_DEMOACTION( DEMO_ACTION_SCREENFADE_START, CDemoActionScreenFadeStart );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//-----------------------------------------------------------------------------
void CDemoActionTextMessageStart::SetMessageText( char const *text )
{
	Q_strncpy( m_szMessageText, text, sizeof( m_szMessageText ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *CDemoActionTextMessageStart::GetMessageText( void ) const
{
	return m_szMessageText;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *font - 
//-----------------------------------------------------------------------------
void CDemoActionTextMessageStart::SetFontName( char const *font )
{
	Q_strncpy( m_szVguiFont, font, sizeof( m_szVguiFont ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *CDemoActionTextMessageStart::GetFontName( void ) const
{
	if ( !Q_strcasecmp( "TextMessageDefault", m_szVguiFont ) )
	{
		return "";
	}
	return m_szVguiFont;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionTextMessageStart::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	message.fadein = pInitData->GetFloat( "fadein", 0.0f );
	message.fadeout = pInitData->GetFloat( "fadeout", 0.0f );
	message.holdtime = pInitData->GetFloat( "holdtime", 0.0f );
	message.fxtime = pInitData->GetFloat( "fxtime", 0.0f );

	message.x = pInitData->GetFloat( "x", 0 );
	message.y = pInitData->GetFloat( "y", 0 );

	SetMessageText( pInitData->GetString( "message", "" ) );
	SetFontName( pInitData->GetString( "font", "" ) );

	message.r1 = pInitData->GetInt( "r1", 255 );
	message.g1 = pInitData->GetInt( "g1", 255 );
	message.b1 = pInitData->GetInt( "b1", 255 );
	message.a1 = pInitData->GetInt( "a1", 255 );

	message.r2 = pInitData->GetInt( "r2", 255 );
	message.g2 = pInitData->GetInt( "g2", 255 );
	message.b2 = pInitData->GetInt( "b2", 255 );
	message.a2 = pInitData->GetInt( "a2", 255 );

	int fadeinout = pInitData->GetInt( "FADEINOUT", 0 );
	int fadeinoutflicker = pInitData->GetInt( "FLICKER", 0 );
	int fadewriteout = pInitData->GetInt( "WRITEOUT", 0 );

	message.effect = 0;

	if ( fadeinout )
	{
		message.effect = 0;
	}
	if ( fadeinoutflicker )
	{
		message.effect = 1;
	}
	if ( fadewriteout )
	{
		message.effect = 2;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : ScreenFade_t const
//-----------------------------------------------------------------------------
client_textmessage_t *CDemoActionTextMessageStart::GetTextMessage( void )
{
	return &message;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionTextMessageStart::FireAction( void )
{
	GetTextMessage()->pVGuiSchemeFontName = GetFontName();

	TextMessage_DemoMessageFull( GetMessageText(), GetTextMessage() );
	CL_HudMessage( (const char *)DEMO_MESSAGE );
	SetFinishedAction( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionTextMessageStart::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	client_textmessage_t const *tm = GetTextMessage();

	int fadeinout = tm->effect == 0 ? 1 : 0;
	int fadeinoutflicker = tm->effect == 1 ? 1 : 0;
	int fadewriteout  = tm->effect == 2 ? 1 : 0;

	BufPrintf( depth, buf, "message \"%s\"\n", GetMessageText() );
	BufPrintf( depth, buf, "font \"%s\"\n", GetFontName() );

	BufPrintf( depth, buf, "fadein \"%.3f\"\n", tm->fadein );
	BufPrintf( depth, buf, "fadeout \"%.3f\"\n", tm->fadeout );
	BufPrintf( depth, buf, "holdtime \"%.3f\"\n", tm->holdtime );
	BufPrintf( depth, buf, "fxtime \"%.3f\"\n", tm->fxtime );

	if ( fadeinout > 0 ) BufPrintf( depth, buf, "FADEINOUT \"1\"\n" );
	if ( fadeinoutflicker > 0 ) BufPrintf( depth, buf, "FLICKER \"1\"\n" );
	if ( fadewriteout > 0 ) BufPrintf( depth, buf, "WRITEOUT \"1\"\n" );

	BufPrintf( depth, buf, "x \"%f\"\n", tm->x );
	BufPrintf( depth, buf, "y \"%f\"\n", tm->y );

	BufPrintf( depth, buf, "r1 \"%i\"\n", tm->r1 );
	BufPrintf( depth, buf, "g1 \"%i\"\n", tm->g1 );
	BufPrintf( depth, buf, "b1 \"%i\"\n", tm->b1 );
	BufPrintf( depth, buf, "a1 \"%i\"\n", tm->a1 );

	BufPrintf( depth, buf, "r2 \"%i\"\n", tm->r2 );
	BufPrintf( depth, buf, "g2 \"%i\"\n", tm->g2 );
	BufPrintf( depth, buf, "b2 \"%i\"\n", tm->b2 );
	BufPrintf( depth, buf, "a2 \"%i\"\n", tm->a2 );

}

DECLARE_DEMOACTION( DEMO_ACTION_TEXTMESSAGE_START, CDemoActionTextMessageStart );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionCDTrackStart::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetTrack( pInitData->GetInt( "track", -1 ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : track - 
//-----------------------------------------------------------------------------
void CDemoActionCDTrackStart::SetTrack( int track )
{
	m_nCDTrack = track;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CDemoActionCDTrackStart::GetTrack( void ) const
{
	return m_nCDTrack;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionCDTrackStart::FireAction( void )
{
	if ( GetTrack() != -1 )
	{
		char szCommand[ 256 ];
		Q_snprintf( szCommand, sizeof( szCommand ), "cd stop\ncd play %i\n", GetTrack() );
		Cbuf_AddText( szCommand );
	}
	SetFinishedAction( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionCDTrackStart::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	BufPrintf( depth, buf, "track \"%i\"\n",
		GetTrack() );
}

DECLARE_DEMOACTION( DEMO_ACTION_PLAYCDTRACK_START, CDemoActionCDTrackStart );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionCDTrackStop::FireAction( void )
{
	Cbuf_AddText( "cd stop\n" );
	SetFinishedAction( true );
}

DECLARE_DEMOACTION( DEMO_ACTION_PLAYCDTRACK_STOP, CDemoActionCDTrackStop );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionPlaySoundStart::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetSoundName( pInitData->GetString( "sound", "" ) );

	// FIXME:  Could add parsing of attenuation, other sound flags

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *stream - 
//-----------------------------------------------------------------------------
void CDemoActionPlaySoundStart::SetSoundName( char const *name )
{
	Q_strncpy( m_szSoundName, name, sizeof( m_szSoundName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char
//-----------------------------------------------------------------------------
char const	*CDemoActionPlaySoundStart::GetSoundName( void ) const
{
	return m_szSoundName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionPlaySoundStart::FireAction( void )
{
	Vector vDummyOrigin;
	vDummyOrigin.Init();

	CSfxTable *pSound = (CSfxTable*)S_PrecacheSound(GetSoundName());
	if ( pSound )
	{
		S_StartDynamicSound( cl.viewentity, CHAN_AUTO, pSound, vDummyOrigin, 1.0, SNDLVL_IDLE, 0, PITCH_NORM );
	}

	SetFinishedAction( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionPlaySoundStart::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	BufPrintf( depth, buf, "sound \"%s\"\n", GetSoundName() );
}

DECLARE_DEMOACTION( DEMO_ACTION_PLAYSOUND_START, CDemoActionPlaySoundStart );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : demoframe - 
//			demotime - 
//-----------------------------------------------------------------------------
bool CBaseDemoActionWithStopTime::Update( const DemoActionTimingContext& tc )
{
	// Not active yet
	if ( !BaseClass::Update( tc ) )
		return false;

	if ( GetActionFired() )
	{
		if ( m_bUsingStopFrame )
		{
			if ( tc.curframe >= m_nStopFrame )
			{
				SetFinishedAction( true );
			}
		}
		else
		{
			if ( tc.curtime >= m_flStopTime )
			{
				SetFinishedAction( true );
			}
		}
		return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseDemoActionWithStopTime::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetStopFrame( pInitData->GetInt( "stopframe", -1 ) );
	SetStopTime( pInitData->GetFloat( "stoptime", -1.0f ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CBaseDemoActionWithStopTime::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	if ( m_nStopFrame != -1 )
	{
		BufPrintf( depth, buf, "stopframe \"%i\"\n",
			m_nStopFrame );
	}
	else
	{
		if ( m_flStopTime != -1.0f )
		{
			BufPrintf( depth, buf, "stoptime \"%.3f\"\n",
				m_flStopTime );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//-----------------------------------------------------------------------------
void CBaseDemoActionWithStopTime::SetStopFrame( int frame )
{
	m_bUsingStopFrame = frame != -1;
	m_nStopFrame = frame;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
void CBaseDemoActionWithStopTime::SetStopTime( float t )
{
	m_bUsingStopFrame = !( t != -1.0f );
	m_flStopTime = t;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDemoActionChangePlaybackRate::CDemoActionChangePlaybackRate()
{
	m_flPlaybackRate = 1.0f;
	m_flSavePlaybackRate = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionChangePlaybackRate::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetPlaybackRate( pInitData->GetFloat( "playbackrate", 1.0f ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionChangePlaybackRate::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	BufPrintf( depth, buf, "playbackrate \"%f\"\n", GetPlaybackRate() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//-----------------------------------------------------------------------------
void CDemoActionChangePlaybackRate::SetPlaybackRate( float rate )
{
	m_flPlaybackRate = clamp( rate, 0.001f, 1000.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
float CDemoActionChangePlaybackRate::GetPlaybackRate( void ) const
{
	return m_flPlaybackRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionChangePlaybackRate::FireAction( void )
{
	m_flSavePlaybackRate = demo->GetPlaybackRateModifier();
	demo->SetPlaybackRateModifier( m_flPlaybackRate );
	SetActionFired( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionChangePlaybackRate::OnActionFinished( void )
{
	demo->SetPlaybackRateModifier( m_flSavePlaybackRate );
}

DECLARE_DEMOACTION( DEMO_ACTION_CHANGEPLAYBACKRATE, CDemoActionChangePlaybackRate );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDemoActionPausePlayback::CDemoActionPausePlayback()
{
	m_flPauseTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInitData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionPausePlayback::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	SetPauseTime( pInitData->GetFloat( "pausetime", 1.0f ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CDemoActionPausePlayback::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	BufPrintf( depth, buf, "pausetime \"%f\"\n", GetPauseTime() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frame - 
//-----------------------------------------------------------------------------
void CDemoActionPausePlayback::SetPauseTime( float t )
{
	m_flPauseTime = clamp( t, 0.0f, 300.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
float CDemoActionPausePlayback::GetPauseTime( void ) const
{
	return m_flPauseTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemoActionPausePlayback::FireAction( void )
{
	demo->PausePlayback( GetPauseTime() );
	SetActionFired( true );
}

DECLARE_DEMOACTION( DEMO_ACTION_PAUSE, CDemoActionPausePlayback );

extern float	scr_fov_value;
extern float	src_demo_override_fov;
extern double	host_time;

CDemoActionZoom::CDemoActionZoom()
{
	m_bSpline = false;
	m_bStayout = false;

	m_flFinalFOV = 0;
	m_flFOVRateOut = 0;  // degress per second
	m_flFOVRateIn = 0;	 // degrees per second
	m_flHoldTime = 0;
	
	//
	m_flFOVStartTime = 0;
	m_flOriginalFOV = 0.0f;
}

bool CDemoActionZoom::Init( KeyValues *pInitData )
{
	if ( !BaseClass::Init( pInitData ) )
		return false;

	m_bSpline = pInitData->GetInt( "spline", 1 ) ? true : false;
	m_bStayout = pInitData->GetInt( "stayout", 1 ) ? true : false;

	m_flFinalFOV = pInitData->GetFloat( "finalfov", 0.0f );
	m_flFOVRateOut = pInitData->GetFloat( "fovrateout", 0.0f );
	m_flFOVRateIn = pInitData->GetFloat( "fovratein", 0.0f );
	m_flHoldTime = pInitData->GetFloat( "fovhold", 0.0f );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tc - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemoActionZoom::Update( const DemoActionTimingContext& tc )
{
	// Not active yet
	if ( !BaseClass::Update( tc ) )
		return false;

	if ( GetActionFired() )
	{
		// See if we're done yet
		float elapsed = host_time - m_flFOVStartTime;
		if ( elapsed > m_flFOVRateOut )
		{
			if ( m_bStayout )
			{
				src_demo_override_fov = m_flFinalFOV;
				SetFinishedAction( true );
				return true;
			}

			float intime = m_flFOVRateOut + m_flHoldTime;
			if ( elapsed <= intime )
			{
				// still going
				src_demo_override_fov = m_flFinalFOV;
				return true;
			}

			float finishtime = intime + m_flFOVRateIn;
			if ( elapsed > finishtime )
			{
				// Finished now
				src_demo_override_fov = 0.0f;
				SetFinishedAction( true );
				return true;
			}

			float frac = 0.0f;
			if ( m_flFOVRateIn > 0.0f )
			{
				frac = ( elapsed - intime ) / m_flFOVRateIn;
			}
			if ( m_bSpline )
			{
				frac = 3.0f * frac * frac - 2.0f * frac * frac * frac;
			}
			frac = clamp( frac, 0.0f, 1.0f );

			frac = 1.0f - frac;

			src_demo_override_fov = m_flOriginalFOV + frac * ( m_flFinalFOV - m_flOriginalFOV );
		}
		else
		{
			float frac = 0.0f;
			if ( m_flFOVRateOut > 0.0f )
			{
				frac = elapsed / m_flFOVRateOut;
			}
			if ( m_bSpline )
			{
				frac = 3.0f * frac * frac - 2.0f * frac * frac * frac;
			}
			frac = clamp( frac, 0.0f, 1.0f );

			src_demo_override_fov = m_flOriginalFOV + frac * ( m_flFinalFOV - m_flOriginalFOV );
		}

		return true;
	}

	return true;
}

void CDemoActionZoom::FireAction( void )
{
	m_flOriginalFOV		= scr_fov_value;
	src_demo_override_fov = m_flOriginalFOV;
	m_flFOVStartTime	= host_time;
}

void CDemoActionZoom::SaveKeysToBuffer( int depth, CUtlBuffer& buf )
{
	BaseClass::SaveKeysToBuffer( depth, buf );

	BufPrintf( depth, buf, "spline \"%i\"\n",
		m_bSpline ? 1 : 0 );
	BufPrintf( depth, buf, "stayout \"%i\"\n",
		m_bStayout ? 1 : 0 );

	BufPrintf( depth, buf, "finalfov \"%f\"\n",
		m_flFinalFOV );
	BufPrintf( depth, buf, "fovrateout \"%f\"\n",
		m_flFOVRateOut );
	BufPrintf( depth, buf, "fovratein \"%f\"\n",
		m_flFOVRateIn );
	BufPrintf( depth, buf, "fovhold \"%f\"\n",
		m_flHoldTime );
}

DECLARE_DEMOACTION( DEMO_ACTION_ZOOM, CDemoActionZoom );