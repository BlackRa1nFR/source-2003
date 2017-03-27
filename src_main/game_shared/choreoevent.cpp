//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "tier0/dbg.h"
#include <stdio.h>
#include <string.h>
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "mathlib.h"
#include "vstdlib/strtools.h"
#include "choreoscene.h"
#include "ichoreoeventcallback.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *owner - 
//			*name - 
//			percentage - 
//-----------------------------------------------------------------------------
CEventRelativeTag::CEventRelativeTag( CChoreoEvent *owner, const char *name, float percentage )
{
	Assert( owner );
	Assert( name );
	Assert( percentage >= 0.0f );
	Assert( percentage <= 1.0f );

	strcpy( m_szName, name );
	m_flPercentage = percentage;
	m_pOwner = owner;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CEventRelativeTag::CEventRelativeTag( const CEventRelativeTag& src )
{
	strcpy( m_szName, src.m_szName );
	m_flPercentage	= src.m_flPercentage;
	m_pOwner		= src.m_pOwner;
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CEventRelativeTag::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CEventRelativeTag::GetPercentage( void )
{
	return m_flPercentage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percentage - 
//-----------------------------------------------------------------------------
void CEventRelativeTag::SetPercentage( float percentage )
{
	m_flPercentage = percentage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CEventRelativeTag::GetOwner( void )
{
	return m_pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventRelativeTag::SetOwner( CChoreoEvent *event )
{
	m_pOwner = event;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the corrected time based on the owner's length and start time
// Output : float
//-----------------------------------------------------------------------------
float CEventRelativeTag::GetStartTime( void )
{
	Assert( m_pOwner );
	if ( !m_pOwner )
	{
		return 0.0f;
	}

	float ownerstart		= m_pOwner->GetStartTime();
	float ownerduration		= m_pOwner->GetDuration();

	return ( ownerstart + ownerduration * m_flPercentage );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *owner - 
//			*name - 
//			percentage - 
//-----------------------------------------------------------------------------
CFlexTimingTag::CFlexTimingTag( CChoreoEvent *owner, const char *name, float percentage, bool locked )
: BaseClass( owner, name, percentage )
{
	m_bLocked = locked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CFlexTimingTag::CFlexTimingTag( const CFlexTimingTag& src )
: BaseClass( src )
{
	m_bLocked = src.m_bLocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexTimingTag::GetLocked( void )
{
	return m_bLocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : locked - 
//-----------------------------------------------------------------------------
void CFlexTimingTag::SetLocked( bool locked )
{
	m_bLocked = locked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *owner - 
//			*name - 
//			percentage - 
//-----------------------------------------------------------------------------
CEventAbsoluteTag::CEventAbsoluteTag( CChoreoEvent *owner, const char *name, float t )
{
	Assert( owner );
	Assert( name );
	Assert( t >= 0.0f );

	strcpy( m_szName, name );
	m_flTagTime = t;
	m_pOwner = owner;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CEventAbsoluteTag::CEventAbsoluteTag( const CEventAbsoluteTag& src )
{
	strcpy( m_szName, src.m_szName );
	m_flTagTime		= src.m_flTagTime;
	m_pOwner		= src.m_pOwner;
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CEventAbsoluteTag::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CEventAbsoluteTag::GetTime( void )
{
	return m_flTagTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percentage - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetTime( float t )
{
	m_flTagTime = t;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CEventAbsoluteTag::GetOwner( void )
{
	return m_pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetOwner( CChoreoEvent *event )
{
	m_pOwner = event;
}

// FLEX ANIMATIONS
//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *event - 
//-----------------------------------------------------------------------------
CFlexAnimationTrack::CFlexAnimationTrack( CChoreoEvent *event )
{
	m_pEvent			= event;
	m_pControllerName	=	NULL;
	m_bActive			= false;
	m_bCombo			= false;
	m_nFlexControllerIndex[ 0 ] = m_nFlexControllerIndex[ 1 ] = -1;
	m_nFlexControllerIndexRaw[ 0 ] = m_nFlexControllerIndexRaw[ 1 ] = -1;

	// base track has range, combo is always 0..1
	m_flMin = 0.0f;
	m_flMax = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CFlexAnimationTrack::CFlexAnimationTrack( const CFlexAnimationTrack* src )
{
	m_pControllerName = NULL;
	SetFlexControllerName( src->m_pControllerName ? src->m_pControllerName : "" );

	m_bActive	= src->m_bActive;
	m_bCombo	= src->m_bCombo;

	for ( int t = 0; t < 2; t++ )
	{
		m_Samples[ t ].Purge();
		for ( int i = 0 ;i < src->m_Samples[ t ].Size(); i++ )
		{
			CExpressionSample s = src->m_Samples[ t ][ i ];
			m_Samples[ t ].AddToTail( s );
		}
	}

	for ( int side = 0; side < 2; side++ )
	{
		m_nFlexControllerIndex[ side ] = src->m_nFlexControllerIndex[ side ];
		m_nFlexControllerIndexRaw[ side ] = src->m_nFlexControllerIndexRaw[ side ];
	}

	m_flMin = src->m_flMin;
	m_flMax = src->m_flMax;
	m_pEvent = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFlexAnimationTrack::~CFlexAnimationTrack( void )
{
	delete[] m_pControllerName;

	for ( int t = 0; t < 2; t++ )
	{
		m_Samples[ t ].Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetEvent( CChoreoEvent *event )
{
	m_pEvent = event;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::Clear( void )
{
	for ( int t = 0; t < 2; t++ )
	{
		m_Samples[ t ].RemoveAll();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::RemoveSample( int index, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	m_Samples[ type ].Remove( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetFlexControllerName( const char *name )
{
	delete[] m_pControllerName;
	m_pControllerName = new char[ strlen( name ) + 1 ];
	strcpy( m_pControllerName, name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CFlexAnimationTrack::GetFlexControllerName( void )
{
	return m_pControllerName ? m_pControllerName : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CFlexAnimationTrack::GetNumSamples( int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	return m_Samples[ type ].Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CExpressionSample
//-----------------------------------------------------------------------------
CExpressionSample *CFlexAnimationTrack::GetSample( int index, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	if ( index < 0 || index >= GetNumSamples( type ) )
		return NULL;
	return &m_Samples[ type ][ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexAnimationTrack::IsTrackActive( void )
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : active - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetTrackActive( bool active )
{
	m_bActive = active;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetZeroValue( int type )
{
	float zero = 0.0f;
	if (type == 1)
	{
		zero = 0.5f;
	}
	else if (m_flMin != m_flMax)
	{
		zero = (0.0f - m_flMin) / (m_flMax - m_flMin);
	}
	return zero;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : number - 
// Output : CExpressionSample
//-----------------------------------------------------------------------------
CExpressionSample *CFlexAnimationTrack::GetBoundedSample( int number, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	// Search for two samples which span time f
	static CExpressionSample nullstart;
	nullstart.time = 0.0f;
	nullstart.value = GetZeroValue( type );
	static CExpressionSample nullend;
	nullend.time = m_pEvent->GetDuration();
	nullend.value = GetZeroValue( type );

	if ( number < 0 )
	{
		return &nullstart;
	}
	else if ( number >= GetNumSamples( type ) )
	{
		return &nullend;
	}
	
	return GetSample( number, type );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			type - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetIntensityInternal( float time, int type )
{
	float zeroValue = GetZeroValue( type );

	Assert( type == 0 || type == 1 );

	// find samples that span the time
	if ( !m_pEvent || !m_pEvent->HasEndTime() )
		return zeroValue;

	if ( time < m_pEvent->GetStartTime() )
		return zeroValue;
	if ( time > m_pEvent->GetEndTime() )
		return zeroValue;

	float elapsed = time - m_pEvent->GetStartTime();

	float retval = GetFracIntensity( elapsed, type );

	// scale
	if (m_flMin != m_flMax)
	{
		retval = retval * (m_flMax - m_flMin) + m_flMin;
	}
	return retval;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			type - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetFracIntensity( float time, int type )
{
	float zeroValue = GetZeroValue( type );

	Assert( type == 0 || type == 1 );

	// find samples that span the time
	if ( !m_pEvent || !m_pEvent->HasEndTime() )
		return zeroValue;

	if ( GetNumSamples( type ) < 1 )
	{
		return zeroValue;
	}

	int i;
	for ( i = -1 ; i < GetNumSamples( type ); i++ )
	{
		CExpressionSample *s = GetBoundedSample( i, type );
		CExpressionSample *n = GetBoundedSample( i + 1, type );
		if ( !s || !n )
			continue;

		if ( time >= s->time && time <= n->time )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -1, prev );
	start = max( -1, start );
	end = min( end, GetNumSamples( type ) );
	next = min( next, GetNumSamples( type ) );

	CExpressionSample *esPre = GetBoundedSample( prev, type );
	CExpressionSample *esStart = GetBoundedSample( start, type );
	CExpressionSample *esEnd = GetBoundedSample( end, type );
	CExpressionSample *esNext = GetBoundedSample( next, type );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_NormalizeX( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetSampleIntensity( float time )
{
	return GetIntensityInternal( time, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetBalanceIntensity( float time )
{
	if ( IsComboType() )
	{
		return GetIntensityInternal( time, 1 );
	}

	return 1.0f;
}

// For a given time, computes 0->1 intensity value for the slider
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetIntensity( float time, int side )
{
	float mag	= GetSampleIntensity( time );

	float scale = 1.0f;

	if ( IsComboType() )
	{
		float balance = GetBalanceIntensity( time );

		// Asking for left but balance is to right, then fall off as we go
		//  further right
		if ( side == 0 && balance > 0.5f )
		{
			scale = (1.0f - balance ) / 0.5f;
		}
		// Asking for right, but balance is left, fall off as we go left.
		else if ( side == 1 && balance < 0.5f )
		{
			scale = ( balance / 0.5f );
		}
	}

	return mag * scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			value - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::AddSample( float time, float value, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	CExpressionSample sample;
	sample.time = time;
	sample.value = value;
	sample.selected = false;

	m_Samples[ type ].AddToTail( sample );
	
	Resort( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::Resort( int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	for ( int i = 0; i < m_Samples[ type ].Size(); i++ )
	{
		for ( int j = i + 1; j < m_Samples[ type ].Size(); j++ )
		{
			CExpressionSample src = m_Samples[ type ][ i ];
			CExpressionSample dest = m_Samples[ type ][ j ];

			if ( src.time > dest.time )
			{
				m_Samples[ type ][ i ] = dest;
				m_Samples[ type ][ j ] = src;
			}
		}
	}

	// Make sure nothing is out of range
	RemoveOutOfRangeSamples( 0 );
	RemoveOutOfRangeSamples( 1 );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CFlexAnimationTrack::GetEvent( void )
{
	return m_pEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : side - 
// Output : int
//-----------------------------------------------------------------------------
int CFlexAnimationTrack::GetFlexControllerIndex( int side /*= 0*/ )
{
	Assert( side == 0 || side == 1 );

	if ( IsComboType() )
	{
		return m_nFlexControllerIndex[ side ];
	}
	
	return m_nFlexControllerIndex[ 0 ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : side - 
// Output : int
//-----------------------------------------------------------------------------
int CFlexAnimationTrack::GetRawFlexControllerIndex( int side /*= 0*/ )
{
	Assert( side == 0 || side == 1 );

	if ( IsComboType() )
	{
		return m_nFlexControllerIndexRaw[ side ];
	}
	
	return m_nFlexControllerIndexRaw[ 0 ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			side - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetFlexControllerIndex( int raw, int index, int side /*= 0*/ )
{
	Assert( side == 0 || side == 1 );

	m_nFlexControllerIndex[ side ] = index;
	// Model specific
	m_nFlexControllerIndexRaw[ side ] = raw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : combo - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetComboType( bool combo )
{
	m_bCombo = combo;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexAnimationTrack::IsComboType( void )
{
	return m_bCombo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetMin( float value )
{
	m_flMin = value;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetMax( float value )
{
	m_flMax = value;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetMin( int type )
{
	if (type == 0)
		return m_flMin;
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetMax( int type )
{
	if (type == 0)
		return m_flMax;
	else
		return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::RemoveOutOfRangeSamples( int type )
{
	Assert( m_pEvent );
	if ( !m_pEvent )
		return;

	Assert( m_pEvent->HasEndTime() );
	float duration = m_pEvent->GetDuration();

	int c = m_Samples[ type ].Size();
	for ( int i = c-1; i >= 0; i-- )
	{
		CExpressionSample src = m_Samples[ type ][ i ];
		if ( src.time < 0 ||
			 src.time > duration )
		{
			m_Samples[ type ].Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEvent::CChoreoEvent( CChoreoScene *scene )
{
	Init( scene );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*name - 
//-----------------------------------------------------------------------------
CChoreoEvent::CChoreoEvent( CChoreoScene *scene, EVENTTYPE type, const char *name )
{
	Init( scene );
	SetType( type );
	SetName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*name - 
//			*param - 
//-----------------------------------------------------------------------------
CChoreoEvent::CChoreoEvent( CChoreoScene *scene, EVENTTYPE type, const char *name, const char *param )
{
	Init( scene );
	SetType( type );
	SetName( name );
	SetParameters( param );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEvent::~CChoreoEvent( void )
{
	RemoveAllTracks();
	delete m_pSubScene;
}

//-----------------------------------------------------------------------------
// Purpose: Assignment
// Input  : src - 
// Output : CChoreoEvent&
//-----------------------------------------------------------------------------
CChoreoEvent& CChoreoEvent::operator=( const CChoreoEvent& src )
{
	m_pActor = NULL;
	m_pChannel = NULL;

	m_fType = src.m_fType;
	strcpy( m_szName, src.m_szName );
	strcpy( m_szParameters, src.m_szParameters );
	strcpy( m_szParameters2, src.m_szParameters2 );
	m_flStartTime = src.m_flStartTime;
	m_flEndTime = src.m_flEndTime;
	m_bFixedLength = src.m_bFixedLength;
	m_flGestureSequenceDuration = src.m_flGestureSequenceDuration;
	m_bResumeCondition = src.m_bResumeCondition;
	m_bUsesTag = src.m_bUsesTag;
	strcpy( m_szTagName, src.m_szTagName );
	strcpy( m_szTagWavName, src.m_szTagWavName );

	ClearAllRelativeTags();
	ClearAllTimingTags();
	int t;
	for ( t = 0; t < NUM_ABS_TAG_TYPES; t++ )
	{
		ClearAllAbsoluteTags( (AbsTagType)t );
	}

	int i;
	for ( i = 0; i < src.m_RelativeTags.Size(); i++ )
	{	
		CEventRelativeTag newtag( src.m_RelativeTags[ i ] );
		newtag.SetOwner( this );
		m_RelativeTags.AddToTail( newtag );
	}

	for ( i = 0; i < src.m_TimingTags.Size(); i++ )
	{	
		CFlexTimingTag newtag( src.m_TimingTags[ i ] );
		newtag.SetOwner( this );
		m_TimingTags.AddToTail( newtag );
	}
	for ( t = 0; t < NUM_ABS_TAG_TYPES; t++ )
	{
		for ( i = 0; i < src.m_AbsoluteTags[ t ].Size(); i++ )
		{
			CEventAbsoluteTag newtag( src.m_AbsoluteTags[ t ][ i ] );
			newtag.SetOwner( this );
			m_AbsoluteTags[ t ].AddToTail( newtag );
		}
	}

	RemoveAllTracks();

	for ( i = 0 ; i < src.m_FlexAnimationTracks.Size(); i++ )
	{
		CFlexAnimationTrack *newtrack = new CFlexAnimationTrack( src.m_FlexAnimationTracks[ i ] );
		newtrack->SetEvent( this );
		m_FlexAnimationTracks.AddToTail( newtrack );
	}

	m_bTrackLookupSet = src.m_bTrackLookupSet;

	// FIXME:  Use a safe handle?
	//m_pSubScene = src.m_pSubScene;

	m_bProcessing = src.m_bProcessing;
	m_pMixer = src.m_pMixer;

	m_pScene = src.m_pScene;

	m_nPitch = src.m_nPitch;
	m_nYaw = src.m_nYaw;

	m_nNumLoops = src.m_nNumLoops;
	m_nLoopsRemaining = src.m_nLoopsRemaining;

	// Copy ramp over
	m_Ramp.RemoveAll();
	for ( i = 0; i < src.m_Ramp.Count(); i++ )
	{
		CExpressionSample sample = src.m_Ramp[ i ];
		AddRamp( sample.time, sample.value, sample.selected );
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::Init( CChoreoScene *scene )
{
	m_fType				= UNSPECIFIED;
	m_szName[ 0 ]		= 0;
	m_szParameters[ 0 ] = 0;
	m_szParameters2[ 0 ]  = 0;

	m_flStartTime		= 0.0f;
	m_flEndTime			= -1.0f;

	m_pActor			= NULL;
	m_pChannel			= NULL;
	m_pScene			= scene;

	m_bFixedLength		= false;
	m_bResumeCondition	= false;
	SetUsingRelativeTag( false, 0, 0 );

	m_bTrackLookupSet	= false;

	m_pSubScene			= NULL;
	m_bProcessing		= false;
	m_pMixer  			= NULL;
	m_flGestureSequenceDuration = 0.0f;

	m_nPitch = m_nYaw = 0;

	m_nNumLoops = -1;
	m_nLoopsRemaining = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
CChoreoEvent::EVENTTYPE CChoreoEvent::GetType( void )
{
	return m_fType;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetType( EVENTTYPE type )
{
	m_fType = type;

	if ( m_fType == SPEAK ||
		m_fType == SUBSCENE )
	{
		m_bFixedLength = true;
	}
	else
	{
		m_bFixedLength = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetName( const char *name )
{
	Assert( strlen( name ) < MAX_CHOREOEVENT_NAME );
	strcpy( m_szName, name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *param - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetParameters( const char *param )
{
	Assert( strlen( param ) < MAX_PARAMETERS_STRING );
	strcpy( m_szParameters, param );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetParameters( void )
{
	return m_szParameters;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *param - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetParameters2( const char *param )
{
	Assert( strlen( param ) < MAX_PARAMETERS_STRING );
	strcpy( m_szParameters2, param );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetParameters2( void )
{
	return m_szParameters2;
}

//-----------------------------------------------------------------------------
// Purpose: debugging description
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetDescription( void )
{
	static char description[ 256 ];

	description[ 0 ] = 0;

	if ( !GetActor() )
	{
		Q_snprintf( description,sizeof(description), "global %s", m_szName );
	}
	else
	{
		Assert( m_pChannel );
		Q_snprintf( description,sizeof(description), "%s : %s : %s -- %s \"%s\"", m_pActor->GetName(), m_pChannel->GetName(), GetName(), NameForType( GetType() ), GetParameters() );
		if ( GetType() == EXPRESSION )
		{
			char sz[ 256 ];

			Q_snprintf( sz,sizeof(sz), " \"%s\"", GetParameters2() );
			Q_strncat( description, sz, sizeof(description) );
		}
	}

	return description;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : starttime - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetStartTime( float starttime )
{
	m_flStartTime = starttime;
	if ( m_flEndTime != -1.0f )
	{
		if ( m_flEndTime < m_flStartTime )
		{
			m_flEndTime = m_flStartTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetStartTime( void )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : endtime - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetEndTime( float endtime )
{
	bool changed = m_flEndTime != endtime;

	m_flEndTime = endtime;

	if ( endtime != -1.0f )
	{
		if ( m_flEndTime < m_flStartTime )
		{
			m_flEndTime = m_flStartTime;
		}

		if ( changed )
		{
			OnEndTimeChanged();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetEndTime( void )
{
	return m_flEndTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::HasEndTime( void )
{
	return m_flEndTime != -1.0f ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetCompletion( float time )
{
	float t = (time - m_flStartTime) / (m_flEndTime - m_flStartTime);

	if (t < 0.0f)
		return 0.0f;
	else if (t > 1.0f)
		return 1.0f;
	
	return t;
}

float CChoreoEvent::GetRampIntensity( float time )
{
	float zeroValue = 0.0f;

	// find samples that span the time
	if ( !HasEndTime() )
		return zeroValue;

	int rampCount = GetRampCount();
	if ( rampCount < 1 )
	{
		return zeroValue;
	}

	int i;
	for ( i = -1 ; i < rampCount; i++ )
	{
		CExpressionSample *s = GetBoundedRamp( i );
		CExpressionSample *n = GetBoundedRamp( i + 1 );
		if ( !s || !n )
			continue;

		if ( time >= s->time && time <= n->time )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -1, prev );
	start = max( -1, start );
	end = min( end, rampCount );
	next = min( next, rampCount );

	CExpressionSample *esPre = GetBoundedRamp( prev );
	CExpressionSample *esStart = GetBoundedRamp( start );
	CExpressionSample *esEnd = GetBoundedRamp( end );
	CExpressionSample *esNext = GetBoundedRamp( next );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_Normalize( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: Get intensity for event, bounded by scene global intensity
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetIntensity( float scenetime )
{
	float global_intensity = 1.0f;
	if ( m_pScene )
	{
		global_intensity = m_pScene->GetSceneRampIntensity( scenetime );
	}
	else
	{
		Assert( 0 );
	}

	float event_intensity = _GetIntensity( scenetime );

	return global_intensity * event_intensity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::_GetIntensity( float scenetime )
{
	// Convert to event local time
	float time = scenetime - m_flStartTime;

	float zeroValue = 0.0f;

	// find samples that span the time
	if ( !HasEndTime() )
		return zeroValue;

	int rampCount = GetRampCount();
	if ( rampCount < 1 )
	{
		// Full intensity
		return 1.0f;
	}

	int i;
	for ( i = -1 ; i < rampCount; i++ )
	{
		CExpressionSample *s = GetBoundedRamp( i );
		CExpressionSample *n = GetBoundedRamp( i + 1 );
		if ( !s || !n )
			continue;

		if ( time >= s->time && time <= n->time )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -1, prev );
	start = max( -1, start );
	end = min( end, rampCount );
	next = min( next, rampCount );

	CExpressionSample *esPre = GetBoundedRamp( prev );
	CExpressionSample *esStart = GetBoundedRamp( start );
	CExpressionSample *esEnd = GetBoundedRamp( end );
	CExpressionSample *esNext = GetBoundedRamp( next );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_Normalize( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoEvent::OffsetStartTime( float dt )
{
	SetStartTime( GetStartTime() + dt );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoEvent::OffsetEndTime( float dt )
{
	if ( HasEndTime() )
	{
		SetEndTime( GetEndTime() + dt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoEvent::OffsetTime( float dt )
{
	if ( HasEndTime() )
	{
		m_flEndTime += dt;
	}
	m_flStartTime += dt;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetActor( CChoreoActor *actor )
{
	m_pActor = actor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoEvent::GetActor( void )
{
	return m_pActor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetChannel( CChoreoChannel *channel )
{
	m_pChannel = channel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoEvent::GetChannel( void )
{
	return m_pChannel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetSubScene( CChoreoScene *scene )
{
	m_pSubScene = scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CChoreoEvent::GetSubScene( void )
{
	return m_pSubScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
CChoreoEvent::EVENTTYPE CChoreoEvent::TypeForName( const char *name )
{
	if ( !stricmp( name, "section" ) )
	{
		return SECTION;
	}
	else if ( !stricmp( name, "expression" ) )
	{
		return EXPRESSION;
	}
	else if ( !stricmp( name, "speak" ) )
	{
		return SPEAK;
	}
	else if ( !stricmp( name, "lookat" ) )
	{
		return LOOKAT;
	}
	else if ( !stricmp( name, "moveto" ) )
	{
		return MOVETO;
	}
	else if ( !stricmp( name, "gesture" ) )
	{
		return GESTURE;
	}
	else if ( !stricmp( name, "face" ) )
	{
		return FACE;
	}
	else if ( !stricmp( name, "firetrigger" ) )
	{
		return FIRETRIGGER;
	}
	else if ( !stricmp( name, "sequence" ) )
	{
		return SEQUENCE;
	}
	else if ( !stricmp( name, "flexanimation" ) )
	{
		return FLEXANIMATION;
	}
	else if ( !stricmp( name, "subscene" ) )
	{
		return SUBSCENE;
	}
	else if ( !stricmp( name, "loop" ) )
	{
		return LOOP;
	}
	else if ( !stricmp( name, "interrupt" ) )
	{
		return INTERRUPT;
	}
	else if ( !stricmp( name, "stoppoint" ) )
	{
		return STOPPOINT;
	}

	return UNSPECIFIED;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::NameForType( EVENTTYPE type )
{
	switch ( type )
	{
	default:
	case UNSPECIFIED:
		break;
	case SECTION:
		return "section";
	case EXPRESSION:
		return "expression";
	case LOOKAT:
		return "lookat";
	case MOVETO:
		return "moveto";
	case SPEAK:
		return "speak";
	case GESTURE:
		return "gesture";
	case FACE:
		return "face";
	case FIRETRIGGER:
		return "firetrigger";
	case SEQUENCE:
		return "sequence";
	case FLEXANIMATION:
		return "flexanimation";
	case SUBSCENE:
		return "subscene";
	case LOOP:
		return "loop";
	case INTERRUPT:
		return "interrupt";
	case STOPPOINT:
		return "stoppoint";
	}

	return "unspecified";
}

//-----------------------------------------------------------------------------
// Purpose: Is the event something that can be sized ( a .wav file, e.g. )
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsFixedLength( void )
{
	return m_bFixedLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : isfixedlength - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetFixedLength( bool isfixedlength )
{
	m_bFixedLength = isfixedlength;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : resumecondition - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetResumeCondition( bool resumecondition )
{
	m_bResumeCondition = resumecondition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsResumeCondition( void )
{
	return m_bResumeCondition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetDuration( void )
{
	if ( HasEndTime() )
	{
		return GetEndTime() - GetStartTime();
	}
	
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearAllRelativeTags( void )
{
	m_RelativeTags.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumRelativeTags( void )
{
	return m_RelativeTags.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tagnum - 
// Output : CEventRelativeTag
//-----------------------------------------------------------------------------
CEventRelativeTag *CChoreoEvent::GetRelativeTag( int tagnum )
{
	Assert( tagnum >= 0 && tagnum < m_RelativeTags.Size() );
	return &m_RelativeTags[ tagnum ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//			percentage - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddRelativeTag( const char *tagname, float percentage )
{
	CEventRelativeTag rt( this, tagname, percentage );
	m_RelativeTags.AddToTail( rt );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveRelativeTag( const char *tagname )
{
	for ( int i = 0; i < m_RelativeTags.Size(); i++ )
	{
		CEventRelativeTag *prt = &m_RelativeTags[ i ];
		if ( !prt )
			continue;

		if ( !stricmp( prt->GetName(), tagname ) )
		{
			m_RelativeTags.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
// Output : CEventRelativeTag *
//-----------------------------------------------------------------------------
CEventRelativeTag * CChoreoEvent::FindRelativeTag( const char *tagname )
{
	for ( int i = 0; i < m_RelativeTags.Size(); i++ )
	{
		CEventRelativeTag *prt = &m_RelativeTags[ i ];
		if ( !prt )
			continue;

		if ( !stricmp( prt->GetName(), tagname ) )
		{
			return prt;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsUsingRelativeTag( void )
{
	return m_bUsesTag;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : usetag - 
//			0 - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetUsingRelativeTag( bool usetag, const char *tagname /*= 0*/, 
	const char *wavname /* = 0 */ )
{
	m_bUsesTag = usetag;
	if ( tagname )
	{
		strcpy( m_szTagName, tagname );
	}
	else
	{
		strcpy( m_szTagName, "" );
	}
	if ( wavname )
	{
		strcpy( m_szTagWavName, wavname );
	}
	else
	{
		strcpy( m_szTagWavName, "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetRelativeTagName( void )
{
	return m_szTagName;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetRelativeWavName( void )
{
	return m_szTagWavName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearAllTimingTags( void )
{
	m_TimingTags.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumTimingTags( void )
{
	return m_TimingTags.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tagnum - 
// Output : CEventRelativeTag
//-----------------------------------------------------------------------------
CFlexTimingTag *CChoreoEvent::GetTimingTag( int tagnum )
{
	Assert( tagnum >= 0 && tagnum < m_TimingTags.Size() );
	return &m_TimingTags[ tagnum ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//			percentage - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddTimingTag( const char *tagname, float percentage, bool locked )
{
	CFlexTimingTag tt( this, tagname, percentage, locked );
	m_TimingTags.AddToTail( tt );

	// Sort tags
	CFlexTimingTag temp( (CChoreoEvent *)0x1, "", 0.0f, false );

	// ugly bubble sort
	for ( int i = 0; i < m_TimingTags.Size(); i++ )
	{
		for ( int j = i + 1; j < m_TimingTags.Size(); j++ )
		{
			CFlexTimingTag *t1 = &m_TimingTags[ i ];
			CFlexTimingTag *t2 = &m_TimingTags[ j ];

			if ( t1->GetPercentage() > t2->GetPercentage() )
			{
				temp = *t1;
				*t1 = *t2;
				*t2 = temp;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveTimingTag( const char *tagname )
{
	for ( int i = 0; i < m_TimingTags.Size(); i++ )
	{
		CFlexTimingTag *ptt = &m_TimingTags[ i ];
		if ( !ptt )
			continue;

		if ( !stricmp( ptt->GetName(), tagname ) )
		{
			m_TimingTags.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
// Output : CEventRelativeTag *
//-----------------------------------------------------------------------------
CFlexTimingTag * CChoreoEvent::FindTimingTag( const char *tagname )
{
	for ( int i = 0; i < m_TimingTags.Size(); i++ )
	{
		CFlexTimingTag *ptt = &m_TimingTags[ i ];
		if ( !ptt )
			continue;

		if ( !stricmp( ptt->GetName(), tagname ) )
		{
			return ptt;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::OnEndTimeChanged( void )
{
	int c = GetNumFlexAnimationTracks();
	for ( int i = 0; i < c; i++ )
	{
		CFlexAnimationTrack *track = GetFlexAnimationTrack( i );
		Assert( track );
		if ( !track )
			continue;

		track->Resort( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumFlexAnimationTracks( void )
{
	return m_FlexAnimationTracks.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CFlexAnimationTrack
//-----------------------------------------------------------------------------
CFlexAnimationTrack *CChoreoEvent::GetFlexAnimationTrack( int index )
{
	if ( index < 0 || index >= GetNumFlexAnimationTracks() )
		return NULL;
	return m_FlexAnimationTracks[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *controllername - 
// Output : CFlexAnimationTrack
//-----------------------------------------------------------------------------
CFlexAnimationTrack *CChoreoEvent::AddTrack( const char *controllername )
{
	CFlexAnimationTrack *newTrack = new CFlexAnimationTrack( this );
	newTrack->SetFlexControllerName( controllername );

	m_FlexAnimationTracks.AddToTail( newTrack );

	return newTrack;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveTrack( int index )
{	
	CFlexAnimationTrack *track = GetFlexAnimationTrack( index );
	if ( !track )
		return;

	m_FlexAnimationTracks.Remove( index );
	delete track;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveAllTracks( void )
{
	while ( GetNumFlexAnimationTracks() > 0 )
	{
		RemoveTrack( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *controllername - 
// Output : CFlexAnimationTrack
//-----------------------------------------------------------------------------
CFlexAnimationTrack *CChoreoEvent::FindTrack( const char *controllername )
{
	for ( int i = 0; i < GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *t = GetFlexAnimationTrack( i );
		if ( t && !stricmp( t->GetFlexControllerName(), controllername ) )
		{
			return t;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::GetTrackLookupSet( void )
{
	return m_bTrackLookupSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetTrackLookupSet( bool set )
{
	m_bTrackLookupSet = set;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsProcessing( void ) const
{
	return m_bProcessing;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::StartProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( !m_bProcessing );
	m_bProcessing = true;
	if ( cb )
	{
		cb->StartEvent( t, scene, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::ContinueProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( m_bProcessing );
	if ( cb )
	{
		cb->ProcessEvent( t, scene, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::StopProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( m_bProcessing );
	if ( cb )
	{
		cb->EndEvent( t, scene, this );
	}
	m_bProcessing = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
bool CChoreoEvent::CheckProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( !m_bProcessing );
	if ( cb )
	{
		return cb->CheckEvent( t, scene, this );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ResetProcessing( void )
{
	if ( GetType() == LOOP )
	{
		m_nLoopsRemaining = m_nNumLoops;
	}

	m_bProcessing = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mixer - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetMixer( CAudioMixer *mixer )
{
	m_pMixer = mixer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CAudioMixer
//-----------------------------------------------------------------------------
CAudioMixer *CChoreoEvent::GetMixer( void ) const
{
	return m_pMixer;
}

// Snap to scene framerate
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::SnapTimes()
{
	if ( HasEndTime() && !IsFixedLength() )
	{
		m_flEndTime = SnapTime( m_flEndTime );
	}
	float oldstart = m_flStartTime;
	m_flStartTime = SnapTime( m_flStartTime );

	// Don't snap end time for fixed length events, just set based on new start time
	if ( IsFixedLength() )
	{
		float dt = m_flStartTime - oldstart;
		m_flEndTime += dt;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::SnapTime( float t )
{
	CChoreoScene *scene = GetScene();
	if ( !scene)
	{
		Assert( 0 );
		return t;
	}

	return scene->SnapTime( t );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CChoreoEvent::GetScene( void )
{
	return m_pScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetScene( CChoreoScene *scene )
{
	m_pScene = scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CChoreoEvent::NameForAbsoluteTagType( AbsTagType t )
{
	switch ( t )
	{
	case PLAYBACK:
		return "playback_time";
	case SHIFTED:
		return "shifted_time";
	default:
		break;
	}

	return "AbsTagType(unknown)";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : AbsTagType
//-----------------------------------------------------------------------------
CChoreoEvent::AbsTagType CChoreoEvent::TypeForAbsoluteTagName( const char *name )
{
	if ( !Q_strcasecmp( name, "playback_time" ) )
	{
		return PLAYBACK;
	}
	else if ( !Q_strcasecmp( name, "shifted_time" ) )
	{
		return SHIFTED;
	}

	return (AbsTagType)-1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearAllAbsoluteTags( AbsTagType type )
{
	m_AbsoluteTags[ type ].Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumAbsoluteTags( AbsTagType type )
{
	return m_AbsoluteTags[ type ].Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			tagnum - 
// Output : CEventAbsoluteTag
//-----------------------------------------------------------------------------
CEventAbsoluteTag *CChoreoEvent::GetAbsoluteTag( AbsTagType type, int tagnum )
{
	Assert( tagnum >= 0 && tagnum < m_AbsoluteTags[ type ].Size() );
	return &m_AbsoluteTags[ type ][ tagnum ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
// Output : CEventAbsoluteTag
//-----------------------------------------------------------------------------
CEventAbsoluteTag *CChoreoEvent::FindAbsoluteTag( AbsTagType type, const char *tagname )
{
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ type ][ i ];
		if ( !ptag )
			continue;

		if ( !stricmp( ptag->GetName(), tagname ) )
		{
			return ptag;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddAbsoluteTag( AbsTagType type, const char *tagname, float t )
{
	CEventAbsoluteTag at( this, tagname, t );
	m_AbsoluteTags[ type ].AddToTail( at );

	// Sort tags
	CEventAbsoluteTag temp( (CChoreoEvent *)0x1, "", 0.0f );

	// ugly bubble sort
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		for ( int j = i + 1; j < m_AbsoluteTags[ type ].Size(); j++ )
		{
			CEventAbsoluteTag *t1 = &m_AbsoluteTags[ type ][ i ];
			CEventAbsoluteTag *t2 = &m_AbsoluteTags[ type ][ j ];

			if ( t1->GetTime() > t2->GetTime() )
			{
				temp = *t1;
				*t1 = *t2;
				*t2 = temp;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveAbsoluteTag( AbsTagType type, const char *tagname )
{
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ type ][ i ];
		if ( !ptag )
			continue;

		if ( !stricmp( ptag->GetName(), tagname ) )
		{
			m_AbsoluteTags[ type ].Remove( i );
			return;
		}
	}
}

float CChoreoEvent::GetBoundedAbsoluteTagTime( AbsTagType type, int tagnum )
{
	if ( tagnum <= -2 )
	{
		/*
		if (GetNumAbsoluteTags( type ) >= 1)
		{
			CEventAbsoluteTag *tag = GetAbsoluteTag( type, 0 );
			Assert( tag );
			return -tag->GetTime();
		}
		*/
		return -0.5f;
	}
	else if ( tagnum == -1 )
	{
		return 0.0f;
	}
	else if ( tagnum == GetNumAbsoluteTags( type ) )
	{
		return 1.0;
	}
	else if ( tagnum > GetNumAbsoluteTags( type ) )
	{
		/*
		if (GetNumAbsoluteTags( type ) >= 1)
		{
			CEventAbsoluteTag *tag = GetAbsoluteTag( type, tagnum - 2 );
			Assert( tag );
			return 2.0 - tag->GetTime();
		}
		*/
		return 1.5;
	}

	/*
	{
		float duration = GetDuration();
		
		if ( type == SHIFTED )
		{
			float seqduration;
			GetGestureSequenceDuration( seqduration );
			return seqduration;
		}
		return duration;
	}
	*/
	
	CEventAbsoluteTag *tag = GetAbsoluteTag( type, tagnum );
	Assert( tag );
	return tag->GetTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetShiftedTimeFromReferenceTime( float t )
{
	Assert( GetType() == GESTURE );
	if ( GetType() != GESTURE )
		return t;

	int count = GetNumAbsoluteTags( PLAYBACK );

	if ( count != GetNumAbsoluteTags( SHIFTED ) )
	{
		return t;
	}

	if ( count <= 0 )
	{
		return t;
	}

	if ( t <= 0.0f )
		return 0.0f;

	float s = 0.0f, n = 0.0f;

	int i;
	for ( i = -1 ; i < count; i++ )
	{
		s = GetBoundedAbsoluteTagTime( PLAYBACK, i );
		n = GetBoundedAbsoluteTagTime( PLAYBACK, i + 1 );

		if ( t >= s && t <= n )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -2, prev );
	start = max( -1, start );
	end = min( end, count );
	next = min( next, count + 1 );

	float dt = n - s;

	Vector vPre( GetBoundedAbsoluteTagTime( PLAYBACK, prev ), GetBoundedAbsoluteTagTime( SHIFTED, prev ), 0 );
	Vector vStart( GetBoundedAbsoluteTagTime( PLAYBACK, start ), GetBoundedAbsoluteTagTime( SHIFTED, start ), 0 );
	Vector vEnd( GetBoundedAbsoluteTagTime( PLAYBACK, end ), GetBoundedAbsoluteTagTime( SHIFTED, end ), 0 );
	Vector vNext( GetBoundedAbsoluteTagTime( PLAYBACK, next ), GetBoundedAbsoluteTagTime( SHIFTED, next ), 0 );

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( t - s ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_NormalizeX( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	return vOut.y;

	/*
	float duration;
	GetGestureSequenceDuration( duration );

	float retval = clamp( vOut.y, 0.0f, duration );
	return retval;
	*/
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetReferenceTimeFromShiftedTime( float t )
{
	Assert( GetType() == GESTURE );
	if ( GetType() != GESTURE )
		return t;

	int count = GetNumAbsoluteTags( PLAYBACK );

	if ( count != GetNumAbsoluteTags( SHIFTED ) )
	{
		return t;
	}

	if ( count <= 0 )
	{
		return t;
	}

	if ( t <= 0.0f )
		return 0.0f;

	float s = 0.0f, n = 0.0f;

	int i;
	for ( i = -1 ; i < count; i++ )
	{
		s = GetBoundedAbsoluteTagTime( PLAYBACK, i );
		n = GetBoundedAbsoluteTagTime( PLAYBACK, i + 1 );

		if ( t >= s && t <= n )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -2, prev );
	start = max( -1, start );
	end = min( end, count );
	next = min( next, count + 1 );

	float dt = n - s;

	Vector vPre( GetBoundedAbsoluteTagTime( SHIFTED, prev ), GetBoundedAbsoluteTagTime( PLAYBACK, prev ), 0 );
	Vector vStart( GetBoundedAbsoluteTagTime( SHIFTED, start ), GetBoundedAbsoluteTagTime( PLAYBACK, start ), 0 );
	Vector vEnd( GetBoundedAbsoluteTagTime( SHIFTED, end ), GetBoundedAbsoluteTagTime( PLAYBACK, end ), 0 );
	Vector vNext( GetBoundedAbsoluteTagTime( SHIFTED, next ), GetBoundedAbsoluteTagTime( PLAYBACK, next ), 0 );

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( t - s ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_NormalizeX( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	return vOut.y;

	/*
	float duration;
	GetGestureSequenceDuration( duration );

	float retval = clamp( vOut.y, 0.0f, duration );
	return retval;
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : duration - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetGestureSequenceDuration( float duration )
{
	m_flGestureSequenceDuration = duration;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : duration - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::GetGestureSequenceDuration( float& duration )
{
	bool valid = m_flGestureSequenceDuration != 0.0f;

	if ( !valid )
	{
		duration = GetDuration();
	}
	else
	{
		duration = m_flGestureSequenceDuration;
	}

	return valid;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pitch - 
//-----------------------------------------------------------------------------
int CChoreoEvent::GetPitch( void ) const
{
	return m_nPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pitch - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetPitch( int pitch )
{
	m_nPitch	= pitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : yaw - 
//-----------------------------------------------------------------------------
int CChoreoEvent::GetYaw( void ) const
{
	return m_nYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : yaw - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetYaw( int yaw )
{
	m_nYaw		= yaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//			-1 - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetLoopCount( int numloops )
{
	Assert( GetType() == LOOP );
	// Never below -1
	m_nNumLoops = max( numloops, -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumLoopsRemaining( void )
{
	Assert( GetType() == LOOP );

	return m_nLoopsRemaining;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : loops - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetNumLoopsRemaining( int loops )
{
	Assert( GetType() == LOOP );

	m_nLoopsRemaining = loops;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int	
//-----------------------------------------------------------------------------
int CChoreoEvent::GetLoopCount( void )
{
	Assert( GetType() == LOOP );
	return m_nNumLoops;
}

int	 CChoreoEvent::GetRampCount( void )
{
	return m_Ramp.Count();
}

CExpressionSample *CChoreoEvent::GetRamp( int index )
{
	if ( index < 0 || index >= GetRampCount() )
		return NULL;

	return &m_Ramp[ index ];
}

void CChoreoEvent::AddRamp( float time, float value, bool selected )
{
	CExpressionSample sample;

	sample.time = time;
	sample.value = value;
	sample.selected = selected;

	m_Ramp.AddToTail( sample );
}

void CChoreoEvent::DeleteRamp( int index )
{
	if ( index < 0 || index >= GetRampCount() )
		return;

	m_Ramp.Remove( index );
}

void CChoreoEvent::ClearRamp( void )
{
	m_Ramp.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ResortRamp( void )
{
	for ( int i = 0; i < m_Ramp.Size(); i++ )
	{
		for ( int j = i + 1; j < m_Ramp.Size(); j++ )
		{
			CExpressionSample src = m_Ramp[ i ];
			CExpressionSample dest = m_Ramp[ j ];

			if ( src.time > dest.time )
			{
				m_Ramp[ i ] = dest;
				m_Ramp[ j ] = src;
			}
		}
	}

	RemoveOutOfRangeRampSamples();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : number - 
// Output : CExpressionSample
//-----------------------------------------------------------------------------
CExpressionSample *CChoreoEvent::GetBoundedRamp( int number )
{
	// Search for two samples which span time f
	static CExpressionSample nullstart;
	nullstart.time = 0.0f;
	nullstart.value = 0.0f;
	static CExpressionSample nullend;
	nullend.time = GetDuration();
	nullend.value = 0.0f;

	if ( number < 0 )
	{
		return &nullstart;
	}
	else if ( number >= GetRampCount() )
	{
		return &nullend;
	}
	
	return GetRamp( number );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveOutOfRangeRampSamples( void )
{
	float duration = GetDuration();

	int c = GetRampCount();
	for ( int i = c-1; i >= 0; i-- )
	{
		CExpressionSample src = m_Ramp[ i ];
		if ( src.time < 0 ||
			 src.time > duration + 0.01 )
		{
			m_Ramp.Remove( i );
		}
	}
}
