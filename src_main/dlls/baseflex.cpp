/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== h_cycler.cpp ========================================================

  The Halflife Cycler NPCs

*/

#include "cbase.h"
#include "animation.h"
#include "baseflex.h"
#include "filesystem.h"
#include "studio.h"
#include "choreoevent.h"
#include "choreoscene.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "vstdlib/strtools.h"

static ConVar scene_allowoverrides( "scene_allowoverrides", "1", 0, "When playing back a choreographed scene, allow per-model expression overrides." );

// ---------------------------------------------------------------------
//
// CBaseFlex -- physically simulated brush rectangular solid
//
// ---------------------------------------------------------------------

int ArrayLengthSendProxy_FlexWeightLen( const void *pStruct, int objectID )
{
	CBaseFlex *pBaseFlex = (CBaseFlex*)pStruct;
	return pBaseFlex->GetNumFlexControllers();
}

// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CBaseFlex, DT_BaseFlex)
	SendPropVariableLengthArray(
		ArrayLengthSendProxy_FlexWeightLen,
		SendPropFloat	(SENDINFO_ARRAY(m_flexWeight),	12,	SPROP_ROUNDDOWN,	0.0f,	1.0f),
		m_flexWeight ),

	SendPropInt		(SENDINFO(m_blinktoggle), 1, SPROP_UNSIGNED ),
	SendPropVector	(SENDINFO(m_viewtarget), -1, SPROP_COORD),

#ifdef HL2_DLL
	SendPropFloat	( SENDINFO_VECTORELEM(m_vecViewOffset, 0), 8, 0,	-32.0, 32.0f),
	SendPropFloat	( SENDINFO_VECTORELEM(m_vecViewOffset, 1), 8, 0,	-32.0, 32.0f),
	SendPropFloat	( SENDINFO_VECTORELEM(m_vecViewOffset, 2), 16, 0,	-64.0f, 128.0f),
#endif
	
END_SEND_TABLE()


BEGIN_DATADESC( CBaseFlex )

	//						m_blinktoggle
	DEFINE_ARRAY( CBaseFlex, m_flexWeight, FIELD_FLOAT, 64 ),
	DEFINE_FIELD( CBaseFlex, m_viewtarget, FIELD_POSITION_VECTOR ),
	//						m_Expressions
	//						m_FileList

END_DATADESC()



LINK_ENTITY_TO_CLASS( funCBaseFlex, CBaseFlex ); // meaningless independant class!!

BEGIN_PREDICTION_DATA( CBaseFlex )

	DEFINE_FIELD( CBaseFlex, m_viewtarget, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseFlex, m_blinktoggle, FIELD_INTEGER ),
	DEFINE_ARRAY( CBaseFlex, m_flexWeight, FIELD_FLOAT, 64 ),
	// DEFINE_ARRAY( CBaseFlex, m_Expressions, CExpressionInfo,  MAX_EXPRESSIONS  ),
	// DEFINE_FIELD( CBaseFlex, m_FileList, CUtlVector < CFacialExpressionFile * > ),

END_PREDICTION_DATA()

CBaseFlex::CBaseFlex( void )
{
#ifdef _DEBUG
	// default constructor sets the viewtarget to NAN
	m_viewtarget.Init();
#endif
}

CBaseFlex::~CBaseFlex( void )
{
	DeleteSceneFiles();
}

void CBaseFlex::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );

	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		SetFlexWeight( i, 0.0 );
	}
}


void CBaseFlex::SetViewtarget( const Vector &viewtarget )
{
	m_viewtarget = viewtarget;	// bah
}

const Vector &CBaseFlex::GetViewtarget( )
{
	return m_viewtarget.Get();	// bah
}

void CBaseFlex::SetFlexWeight( char *szName, float value )
{
	SetFlexWeight( FindFlexController( szName ), value );
}

void CBaseFlex::SetFlexWeight( int index, float value )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		studiohdr_t *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			value = (value - pflexcontroller->min) / (pflexcontroller->max - pflexcontroller->min);
			value = clamp( value, 0.0, 1.0 );
		}

		m_flexWeight.Set( index, value );
	}
}

float CBaseFlex::GetFlexWeight( char *szName )
{
	return GetFlexWeight( FindFlexController( szName ) );
}

float CBaseFlex::GetFlexWeight( int index )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		studiohdr_t *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return 0;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			return m_flexWeight[index] * (pflexcontroller->max - pflexcontroller->min) + pflexcontroller->min;
		}
				
		return m_flexWeight[index];
	}
	return 0.0;
}

void CBaseFlex::Blink( )
{
	m_blinktoggle = ! m_blinktoggle;
}

int CBaseFlex::FindFlexController( const char *szName )
{
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		if (stricmp( GetFlexControllerName( i ), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Remove all active expressions
//-----------------------------------------------------------------------------
void CBaseFlex::ClearExpressions( CChoreoScene *scene )
{
	if ( !scene )
	{
		m_Expressions.RemoveAll();
		return;
	}

	for ( int i = 0 ; i < m_Expressions.Count(); i++ )
	{
		CExpressionInfo *info = &m_Expressions[ i ];

		Assert( info );
		Assert( info->m_pScene );
		Assert( info->m_pEvent );

		if ( info->m_pScene != scene )
			continue;

		switch ( info->m_pEvent->GetType() )
		{
		case CChoreoEvent::SEQUENCE: 
			{
				if (info->m_iLayer >= 0)
				{
					RemoveLayer( info->m_iLayer );
				}
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				if (info->m_iLayer >= 0)
				{
					RemoveLayer( info->m_iLayer );
				}
			}
			break;
		}

		// Free this slot
		info->m_pEvent		= NULL;
		info->m_pScene		= NULL;
		info->m_bStarted	= false;

		m_Expressions.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add string indexed scene/expression/duration to list of active expressions
// Input  : scenefile - 
//			expression - 
//			duration - 
//-----------------------------------------------------------------------------
void CBaseFlex::AddSceneEvent( CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !scene || !event )
	{
		Msg( "CBaseFlex::AddExpression:  scene or event was NULL!!!\n" );
		return;
	}


	CExpressionInfo info;

	info.m_pEvent		= event;
	info.m_pScene		= scene;
	info.m_bStarted	= false;

	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE: 
		{
			info.m_nSequence = LookupSequence( event->GetParameters() );
			info.m_iLayer = -1;

			if (info.m_nSequence >= 0)
			{
				info.m_iLayer = AddLayeredSequence( info.m_nSequence, scene->GetChannelIndex( event->GetChannel()) );
				SetLayerWeight( info.m_iLayer, 0.0 );
				info.m_flStartAnim = m_flAnimTime; // ??
			}
		}
		break;
	case CChoreoEvent::GESTURE:
		{
			info.m_nSequence = LookupSequence( event->GetParameters() );
			info.m_iLayer = -1;

			if (info.m_nSequence >= 0)
			{
				// this happens before StudioFrameAdvance()
				info.m_iLayer = AddLayeredSequence( info.m_nSequence, scene->GetChannelIndex( event->GetChannel()) );
				SetLayerDuration( info.m_iLayer, event->GetDuration() );
				SetLayerWeight( info.m_iLayer, 0.0 );

				bool looping = ((GetSequenceFlags( info.m_nSequence ) & STUDIO_LOOPING) != 0);
				if ( looping )
				{
					DevMsg( 1, "vcd error, gesture %s of model %s is marked as STUDIO_LOOPING!\n", 
						event->GetParameters(), GetModelName() );
				}

				SetLayerLooping( info.m_iLayer, false ); // force to not loop

				// figure out the animtime when this was frame 0
				float dt =  scene->GetTime() - event->GetStartTime();
				info.m_flStartAnim = m_flAnimTime - dt; // ??
				float flCycle = 0;

				// assuming anim time is going to advance 0.1 seconds, what should our new cycle be?
				float duration = event->GetDuration( );
				float orig_duration = SequenceDuration( info.m_nSequence );
				SetLayerCycle( info.m_iLayer, 0.0 );
				float flNextCycle = event->GetShiftedTimeFromReferenceTime( (m_flAnimTime - info.m_flStartAnim + 0.1) / duration );

				float rate = (flNextCycle - flCycle) * orig_duration / 0.1;
				SetLayerPlaybackRate( info.m_iLayer, rate );
			}
		}
		break;
	}

	m_Expressions.AddToTail( info );
}

//-----------------------------------------------------------------------------
// Purpose: Remove expression
// Input  : scenefile - 
//			expression - 
//-----------------------------------------------------------------------------
void CBaseFlex::RemoveSceneEvent( CChoreoEvent *event  )
{
	Assert( event );

	for ( int i = 0 ; i < m_Expressions.Count(); i++ )
	{
		CExpressionInfo *info = &m_Expressions[ i ];

		Assert( info );
		Assert( info->m_pEvent );

		if ( info->m_pEvent != event)
			continue;

		switch ( event->GetType() )
		{
		case CChoreoEvent::SEQUENCE: 
			{
				if (info->m_iLayer >= 0)
				{
					RemoveLayer( info->m_iLayer );
				}
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				if (info->m_iLayer >= 0)
				{
					RemoveLayer( info->m_iLayer );
				}
			}
			break;
		}

		// Free this slot
		info->m_pEvent		= NULL;
		info->m_pScene		= NULL;
		info->m_bStarted	= false;

		m_Expressions.Remove( i );
		return;
	}

	Assert( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Default implementation
//-----------------------------------------------------------------------------
void CBaseFlex::ProcessExpressions( void )
{
	// slowly decay to netural expression
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		SetFlexWeight( i, GetFlexWeight( i ) * 0.95 );
	}

	AddSceneExpressions();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void *CBaseFlex::FindSceneFile( const char *filename )
{
	// See if it's already loaded
	int i;
	for ( i = 0; i < m_FileList.Size(); i++ )
	{
		CFacialExpressionFile *file = m_FileList[ i ];
		if ( file && !stricmp( file->filename, filename ) )
		{
			return file->buffer;
		}
	}
	
	// Load file into memory
	FileHandle_t file = filesystem->Open( UTIL_VarArgs( "expressions/%s.vfe", filename ), "rb" );

	if ( !file )
		return NULL;

	int len = filesystem->Size( file );

	// read the file
	byte *buffer = new unsigned char[ len ];
	Assert( buffer );
	filesystem->Read( buffer, len, file );
	filesystem->Close( file );

	// Create scene entry
	CFacialExpressionFile *pfile = new CFacialExpressionFile();
	// Remember filename
	Q_strncpy( pfile->filename, filename ,sizeof(pfile->filename));
	// Remember data pointer
	pfile->buffer = buffer;
	// Add to list
	m_FileList.AddToTail( pfile );

	// Fill in translation table
	flexsettinghdr_t *pSettinghdr = ( flexsettinghdr_t * )pfile->buffer;
	Assert( pSettinghdr );
	for (i = 0; i < pSettinghdr->numkeys; i++)
	{
		*(pSettinghdr->pLocalToGlobal(i)) = FindFlexController( pSettinghdr->pLocalName( i ) );
	}

	// Return data
	return pfile->buffer;
}

//-----------------------------------------------------------------------------
// Purpose: Free up any loaded files
//-----------------------------------------------------------------------------
void CBaseFlex::DeleteSceneFiles( void )
{
	while ( m_FileList.Size() > 0 )
	{
		CFacialExpressionFile *file = m_FileList[ 0 ];
		m_FileList.Remove( 0 );
		delete[] file->buffer;
		delete file;
	}
}

static void Extract_FileBase( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( start < 0 || ( in[start] != '/' && in[start] != '\\' ) )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	Q_strncpy( out, &in[start], len+1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseFlex::AddSceneExpressions( void )
{
	// Iterate expressions and look for active slots
	for ( int i = 0; i < m_Expressions.Count(); i++ )
	{
		CExpressionInfo *info = &m_Expressions[ i ];
		Assert( info );

		// FIXME:  Need a safe handle to m_pEvent in case of memory deletion?
		CChoreoEvent *event = info->m_pEvent;
		Assert( event );

		CChoreoScene *scene = info->m_pScene;
		Assert( scene );

		switch ( event->GetType() )
		{
		case CChoreoEvent::FLEXANIMATION:
			{
				if ( event->HasEndTime() )
				{
					AddFlexAnimation( info );
				}
			}
			break;
		case CChoreoEvent::EXPRESSION:
			{
				// Expressions have to have an end time!!!
				if ( event->HasEndTime() )
				{
					// Look up the actual strings
					const char *scenefile	= event->GetParameters();
					const char *name		= event->GetParameters2();
					
					// Have to find both strings
					if ( scenefile && name )
					{
						// Find the scene file
						flexsettinghdr_t *pExpHdr = ( flexsettinghdr_t * )FindSceneFile( scenefile );
						if ( pExpHdr )
						{
							flexsettinghdr_t  *pOverrideHdr = NULL;
							
							// Find overrides, if any exist
							studiohdr_t	*hdr = GetModelPtr();
							
							if ( hdr && scene_allowoverrides.GetBool() )
							{
								char overridefile[ 512 ];
								char shortname[ 128 ];
								char modelname[ 128 ];
								
								//Q_strncpy( modelname, modelinfo->GetModelName( model ) ,sizeof(modelname));
								Q_strncpy( modelname, hdr->name ,sizeof(modelname));
								
								// Fix up the name
								Extract_FileBase( modelname, shortname );
								
								Q_snprintf( overridefile,sizeof(overridefile), "%s/%s", shortname, scenefile );
								
								pOverrideHdr = ( flexsettinghdr_t * )FindSceneFile( overridefile );
							}
							
							float scenetime = scene->GetTime();
							
							float scale = event->GetIntensity( scenetime );
							
							// Add the named expression
							AddExpression( name, scale, pExpHdr, pOverrideHdr, !info->m_bStarted );
						}
					}
				}
			}
			break;
		case CChoreoEvent::SEQUENCE:
			{
				AddFlexSequence( info );
			}
			break;
		case CChoreoEvent::GESTURE:
			{
				AddFlexGesture( info );
			}
			break;
		default:
			break;
		}

		/*
		float scenetime = scene->GetTime();

		float scale = event->GetIntensity( scenetime );
		Msg( "%s %f : ", event->GetName(), scale );
	
		// Add the named expression
		AddExpression( name, scale, pExpHdr, pOverrideHdr, !info->m_bStarted );
		*/

		info->m_bStarted = true;
	}
}

flexsetting_t const *CBaseFlex::FindNamedSetting( flexsettinghdr_t const *pSettinghdr, const char *expr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return NULL;
	}

	return pSetting;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void NewMarkovIndex( flexsettinghdr_t *pSettinghdr, flexsetting_t *pSetting )
{
	if ( pSetting->type != FS_MARKOV )
		return;

	int weighttotal = 0;
	int member = 0;
	for (int i = 0; i < pSetting->numsettings; i++)
	{
		flexmarkovgroup_t *group = pSetting->pMarkovGroup( i );
		if ( !group )
			continue;

		weighttotal += group->weight;
		if ( !weighttotal || random->RandomInt(0,weighttotal-1) < group->weight )
		{
			member = i;
		}
	}

	pSetting->currentindex = member;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CBaseFlex::AddFlexAnimation( CExpressionInfo *info )
{
	if ( !info )
		return;

	CChoreoEvent *event = info->m_pEvent;
	if ( !event )
		return;

	CChoreoScene *scene = info->m_pScene;
	if ( !scene )
		return;

	if ( !event->GetTrackLookupSet() )
	{
		// Create lookup data
		for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
		{
			CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
			if ( !track )
				continue;

			if ( track->IsComboType() )
			{
				char name[ 512 ];
				Q_strncpy( name, "right_" ,sizeof(name));
				strcat( name, track->GetFlexControllerName() );

				track->SetFlexControllerIndex( 0, FindFlexController( name ), 0 );

				Q_strncpy( name, "left_" ,sizeof(name));
				strcat( name, track->GetFlexControllerName() );

				track->SetFlexControllerIndex( 0, FindFlexController( name ), 1 );
			}
			else
			{
				track->SetFlexControllerIndex( 0, FindFlexController( (char *)track->GetFlexControllerName() ) );
			}
		}

		event->SetTrackLookupSet( true );
	}

	float scenetime = scene->GetTime();

	float weight = event->GetIntensity( scenetime );

	// Compute intensity for each track in animation and apply
	// Iterate animation tracks
	for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				int controller = track->GetFlexControllerIndex( side );

				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scenetime, side );
				if ( controller >= 0 )
				{
					float orig = GetFlexWeight( controller );
					SetFlexWeight( controller, orig * (1 - weight) + flIntensity * weight );
				}
			}
		}
		else
		{
			int controller = track->GetFlexControllerIndex( 0 );

			// Get spline intensity for controller
			float flIntensity = track->GetIntensity( scenetime, 0 );
			if ( controller >= 0 )
			{
				float orig = GetFlexWeight( controller );
				SetFlexWeight( controller, orig * (1 - weight) + flIntensity * weight );
			}
		}
	}

	info->m_bStarted = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expr - 
//			scale - 
//			*pSettinghdr - 
//			*pOverrideHdr - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CBaseFlex::AddExpression( const char *expr, float scale, 
	flexsettinghdr_t *pSettinghdr, flexsettinghdr_t *pOverrideHdr, bool newexpression )
{
	int i;
	flexsetting_t const *pSetting = NULL;

	// Find the named setting in the base
	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return;
	}

	// Update markov chain if needed
	if ( newexpression )
	{
		if ( pSetting->type == FS_MARKOV )
		{
			NewMarkovIndex( pSettinghdr, (flexsetting_t *)pSetting );
		}
	}

	// Resolve markov chain for the returned setting
	pSetting = pSettinghdr->pTranslatedSetting( i );

	// Check for overrides
	if ( pOverrideHdr )
	{
		// Get name from setting
		const char *resolvedName = pSetting->pszName();
		if ( resolvedName )
		{
			// See if resolvedName exists in the override file
			flexsetting_t const *override = FindNamedSetting( pOverrideHdr, resolvedName );
			if ( override )
			{
				// If so, point at the override file instead
				pSettinghdr = pOverrideHdr;
				pSetting	= override;
			}
		}
	}

	flexweight_t *pWeights = NULL;
	int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
	if ( !pWeights )
		return;

	for (i = 0; i < truecount; i++, pWeights++)
	{
		// Translate to local flex controller
		// this is translating from the settings's local index to the models local index
		int index = pSettinghdr->LocalToGlobal( pWeights->key );

		// FIXME: this is supposed to blend based on pWeight->influence, but the order is wrong...
		// float value = GetFlexWeight( index ) * (1 - scale * pWeights->influence) + scale * pWeights->weight;

		// Add scaled weighting in to total
		float value = GetFlexWeight( index ) + scale * pWeights->weight;
		SetFlexWeight( index, value );
	}
}





//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CBaseFlex::AddFlexGesture( CExpressionInfo *info )
{
	if ( !info )
		return;

	CChoreoEvent *event = info->m_pEvent;
	if ( !event )
		return;

	CChoreoScene *scene = info->m_pScene;
	if ( !scene )
		return;
	
	if (info->m_iLayer >= 0)
	{
		// this happens after StudioFrameAdvance()
		float duration = event->GetDuration( );
		float orig_duration = SequenceDuration( info->m_nSequence );

		// when we come by again after StudioFrameAdvance() has moved forward 0.1 seconds, what frame should we be on?
		float flCycle = GetLayerCycle( info->m_iLayer );
		float flNextCycle = event->GetShiftedTimeFromReferenceTime( (m_flAnimTime - info->m_flStartAnim + 0.1) / duration );

		// FIXME: what time should this use?
		SetLayerWeight( info->m_iLayer, event->GetIntensity( scene->GetTime() ) );

		float rate = (flNextCycle - flCycle) * orig_duration / 0.1;

		/*
		Msg( "%d : %.2f (%.2f) : %.3f %.3f : %.3f\n",
			info->m_iLayer,
			scene->GetTime(), 
			(scene->GetTime() - event->GetStartTime()) / duration,
			flCycle,
			flNextCycle,
			rate );
		*/

		SetLayerPlaybackRate( info->m_iLayer, rate );
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CBaseFlex::AddFlexSequence( CExpressionInfo *info )
{
	if ( !info )
		return;

	CChoreoEvent *event = info->m_pEvent;
	if ( !event )
		return;

	CChoreoScene *scene = info->m_pScene;
	if ( !scene )
		return;
	
	if (info->m_iLayer >= 0)
	{
		SetLayerWeight( info->m_iLayer, event->GetIntensity( scene->GetTime() ) );
	}
}




#if 0
	// FIXME: This is just an overlay now, shouldn't it be something different?

	CAI_BaseNPC *npc = actor->MyNPCPointer( );

	event->m_nSequence = actor->LookupSequence( event->GetParameters() );

	event->m_iLayer = -1;
	if (npc && event->m_nSequence >= 0)
	{
		event->m_iLayer = npc->AddBaseSequence( event->m_nSequence );
		npc->SetLayerWeight( event->m_iLayer, 0.0 );
	}

#if 0
	// FIXME: set as idle sequence
	actor->m_nSequence = actor->LookupSequence( event->GetParameters() );

	if (actor->m_nSequence == -1)
	{
		Warning( "%s: unknown scene sequence \"%s\"\n", actor->GetDebugName(), event->GetParameters() );
		Assert(0);
		actor->m_nSequence = 0;
		// return false;
	}

	//Msg( "%s (%s): started \"%s\":INT:%s\n", STRING( pTarget->m_iName ), pTarget->GetClassname(), STRING( iszSeq), (m_spawnflags & SF_SCRIPT_NOINTERRUPT) ? "No" : "Yes" );

	actor->m_flCycle = 0;
	actor->ResetSequenceInfo( );

	CAI_BaseNPC *npc = actor->MyNPCPointer( );
	if (npc)
	{
		int id = npc->GetScheduleID( "Scene_sequence" );
		npc->SetSchedule( id );
	}
#endif
}
#endif

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchProcessSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	CAI_BaseNPC *npc = actor->MyNPCPointer( );
	if (npc && event->m_iLayer >= 0 && event->m_flPrevAnim != npc->m_flAnimTime)
	{
		npc->SetLayerWeight( event->m_iLayer, event->GetIntensity( scene->GetTime() ) );
		event->m_flPrevAnim = npc->m_flAnimTime;
	}
}
#endif



// FIXME: delete this code

class CFlexCycler : public CBaseFlex
{
private:
	DECLARE_CLASS( CFlexCycler, CBaseFlex );
public:
	DECLARE_DATADESC();

	CFlexCycler() { m_iszSentence = NULL_STRING; m_sentence = 0; }
	void GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax);
	virtual int	ObjectCaps( void ) { return (BaseClass :: ObjectCaps() | FCAP_IMPULSE_USE); }
	int OnTakeDamage( const CTakeDamageInfo &info );
	void Spawn( void );
	void Think( void );

	virtual void ProcessExpressions( void );

	// Don't treat as a live target
	virtual bool IsAlive( void ) { return FALSE; }

	float m_flextime;
	int m_flexnum;
	float m_flextarget[64];
	float m_blinktime;
	float m_looktime;
	Vector m_lookTarget;
	float m_speaktime;
	int	m_istalking;
	int	m_phoneme;

	string_t m_iszSentence;
	int m_sentence;

	void SetFlexTarget( int flexnum );
	int LookupFlex( const char *szTarget );
};

BEGIN_DATADESC( CFlexCycler )

	DEFINE_FIELD( CFlexCycler, m_flextime, FIELD_TIME ),
	DEFINE_FIELD( CFlexCycler, m_flexnum, FIELD_INTEGER ),
	DEFINE_ARRAY( CFlexCycler, m_flextarget, FIELD_FLOAT, 64 ),
	DEFINE_FIELD( CFlexCycler, m_blinktime, FIELD_TIME ),
	DEFINE_FIELD( CFlexCycler, m_looktime, FIELD_TIME ),
	DEFINE_FIELD( CFlexCycler, m_lookTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CFlexCycler, m_speaktime, FIELD_TIME ),
	DEFINE_FIELD( CFlexCycler, m_istalking, FIELD_INTEGER ),
	DEFINE_FIELD( CFlexCycler, m_phoneme, FIELD_INTEGER ),
	DEFINE_KEYFIELD( CFlexCycler, m_iszSentence, FIELD_STRING, "Sentence" ),
	DEFINE_FIELD( CFlexCycler, m_sentence, FIELD_INTEGER ),

END_DATADESC()


//
// we should get rid of all the other cyclers and replace them with this.
//
class CGenericFlexCycler : public CFlexCycler
{
public:
	DECLARE_CLASS( CGenericFlexCycler, CFlexCycler );

	void Spawn( void ) { GenericCyclerSpawn( (char *)STRING( GetModelName() ), Vector(-16, -16, 0), Vector(16, 16, 72) ); }
};

LINK_ENTITY_TO_CLASS( cycler_flex, CGenericFlexCycler );



ConVar	flex_expression( "flex_expression","-" );
ConVar	flex_talk( "flex_talk","0" );

// Cycler member functions

void CFlexCycler :: GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax)
{
	if (!szModel || !*szModel)
	{
		Warning( "cycler at %.0f %.0f %0.f missing modelname", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	engine->PrecacheModel( szModel );
	SetModel( szModel );

	CFlexCycler::Spawn( );

	UTIL_SetSize(this, vecMin, vecMax);

	Vector vecEyeOffset;
	GetEyePosition( GetModelPtr(), vecEyeOffset );
	SetViewOffset( vecEyeOffset );

	InitBoneControllers();

	if (GetNumFlexControllers() < 5)
		Warning( "cycler_flex used on model %s without enough flexes.\n", szModel );
}

void CFlexCycler :: Spawn( )
{
	Precache();
	/*
	if ( m_spawnflags & FCYCLER_NOTSOLID )
	{
		SetSolid( SOLID_NOT );
	}
	else
	{
		SetSolid( SOLID_SLIDEBOX );
	}
	*/

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_NONE );
	m_takedamage		= DAMAGE_YES;
	m_iHealth			= 80000;// no cycler should die
	
	m_flPlaybackRate	= 1.0f;
	m_flGroundSpeed		= 0;


	SetNextThink( gpGlobals->curtime + 1.0f );

	ResetSequenceInfo( );

	m_flCycle = random->RandomFloat( 0, 1.0 );
}

const char *predef_flexcontroller_names[] = { 
	"right_lid_raiser",
	"left_lid_raiser",
	"right_lid_tightener",
	"left_lid_tightener",
	"right_lid_droop",
	"left_lid_droop",
	"right_inner_raiser",
	"left_inner_raiser",
	"right_outer_raiser",
	"left_outer_raiser",
	"right_lowerer",
	"left_lowerer",
	"right_cheek_raiser",
	"left_cheek_raiser",
	"wrinkler",
	"right_upper_raiser",
	"left_upper_raiser",
	"right_corner_puller",
	"left_corner_puller",
	"corner_depressor",
	"chin_raiser",
	"right_puckerer",
	"left_puckerer",
	"right_funneler",
	"left_funneler",
	"tightener",
	"jaw_clencher",
	"jaw_drop",
	"right_mouth_drop",
	"left_mouth_drop", 
	NULL };

float predef_flexcontroller_values[7][30] = {
/* 0 */	{ 0.700,0.560,0.650,0.650,0.650,0.585,0.000,0.000,0.400,0.040,0.000,0.000,0.450,0.450,0.000,0.000,0.000,0.750,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.150,1.000,0.000,0.000,0.000 }, 
/* 1 */	{ 0.450,0.450,0.450,0.450,0.000,0.000,0.000,0.000,0.300,0.300,0.000,0.000,0.250,0.250,0.000,0.000,0.000,0.750,0.750,0.000,0.000,0.000,0.000,0.400,0.400,0.000,1.000,0.000,0.050,0.050 }, 
/* 2 */	{ 0.200,0.200,0.500,0.500,0.150,0.150,0.100,0.100,0.150,0.150,0.000,0.000,0.700,0.700,0.000,0.000,0.000,0.750,0.750,0.000,0.200,0.000,0.000,0.000,0.000,0.000,0.850,0.000,0.000,0.000 }, 
/* 3 */	{ 0.000,0.000,0.000,0.000,0.000,0.000,0.300,0.300,0.000,0.000,0.000,0.000,0.000,0.000,0.100,0.000,0.000,0.000,0.000,0.700,0.300,0.000,0.000,0.200,0.200,0.000,0.000,0.300,0.000,0.000 }, 
/* 4 */	{ 0.450,0.450,0.000,0.000,0.450,0.450,0.000,0.000,0.000,0.000,0.450,0.450,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.300,0.000,0.000,0.000,0.000 }, 
/* 5 */	{ 0.000,0.000,0.350,0.350,0.150,0.150,0.300,0.300,0.450,0.450,0.000,0.000,0.200,0.200,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.200,0.200,0.000,0.000,0.300,0.000,0.000,0.000,0.000 }, 
/* 6 */	{ 0.000,0.000,0.650,0.650,0.750,0.750,0.000,0.000,0.000,0.000,0.300,0.300,0.000,0.000,0.000,0.250,0.250,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000 }
};

//-----------------------------------------------------------------------------
// Purpose: Changes sequences when shot
//-----------------------------------------------------------------------------
int CFlexCycler::OnTakeDamage( const CTakeDamageInfo &info )
{
	int nSequence = GetSequence() + 1;
	if (!IsValidSequence( nSequence ))
	{
		nSequence = 0;
	}

	ResetSequence( nSequence );
	m_flCycle = 0;

	return 0;
}


void CFlexCycler::SetFlexTarget( int flexnum )
{
	m_flextarget[flexnum] = random->RandomFloat( 0.5, 1.0 );

	const char *pszType = GetFlexControllerType( flexnum );

	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		if (i != flexnum)
		{
			const char *pszOtherType = GetFlexControllerType( i );
			if (stricmp( pszType, pszOtherType ) == 0)
			{
				m_flextarget[i] = 0;
			}
		}
	}

	// HACK, for now, consider then linked is named "right_" or "left_"
	if (strncmp( "right_", GetFlexControllerName( flexnum ), 6 ) == 0)
	{
		m_flextarget[flexnum+1] = m_flextarget[flexnum];
	}
	else if (strncmp( "left_", GetFlexControllerName( flexnum ), 5 ) == 0)
	{
		m_flextarget[flexnum-1] = m_flextarget[flexnum];
	}
}


int CFlexCycler::LookupFlex( const char *szTarget  )
{
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		const char *pszFlex = GetFlexControllerName( i );
		if (stricmp( szTarget, pszFlex ) == 0)
		{
			return i;
		}
	}
	return -1;
}


void CFlexCycler :: Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance ( );

	if (IsSequenceFinished() && !SequenceLoops())
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		m_flAnimTime = gpGlobals->curtime;
		m_flPlaybackRate = 1.0;
		m_bSequenceFinished = false;
		m_flLastEventCheck = 0;
		m_flCycle = 0;
	}

	// only do this if they have more than eyelid movement
	if (GetNumFlexControllers() > 2)
	{
		const char *pszExpression = flex_expression.GetString();

		if (pszExpression && pszExpression[0] == '+' && pszExpression[1] != '\0')
		{
			int i;
			int j = atoi( &pszExpression[1] );
			for ( i = 0; i < GetNumFlexControllers(); i++)
			{
				m_flextarget[m_flexnum] = 0;
			}

			for (i = 0; i < 35 && predef_flexcontroller_names[i]; i++)
			{
				m_flexnum = LookupFlex( predef_flexcontroller_names[i] );
				m_flextarget[m_flexnum] = predef_flexcontroller_values[j][i];
				// Msg( "%s %.3f\n", predef_flexcontroller_names[i], predef_flexcontroller_values[j][i] );
			}
		}
		else if (pszExpression && pszExpression[0] != '\0' && strcmp(pszExpression, "+") != 0)
		{
			char szExpression[128];
			char szTemp[32];

			Q_strncpy( szExpression, pszExpression ,sizeof(szExpression));
			char *pszExpression = szExpression;

			while (*pszExpression != '\0')
			{
				if (*pszExpression == '+')
					*pszExpression = ' ';
				
				pszExpression++;
			}

			pszExpression = szExpression;

			while (*pszExpression)
			{
				if (*pszExpression != ' ')
				{
					if (*pszExpression == '-')
					{
						for (int i = 0; i < GetNumFlexControllers(); i++)
						{
							m_flextarget[i] = 0;
						}
					}
					else if (*pszExpression == '?')
					{
						for (int i = 0; i < GetNumFlexControllers(); i++)
						{
							Msg( "\"%s\" ", GetFlexControllerName( i ) );
						}
						Msg( "\n" );
						flex_expression.SetValue( "" );
					}
					else
					{
						if (sscanf( pszExpression, "%s", szTemp ) == 1)
						{
							m_flexnum = LookupFlex( szTemp );

							if (m_flexnum != -1 && m_flextarget[m_flexnum] != 1)
							{
								m_flextarget[m_flexnum] = 1.0;
								// SetFlexTarget( m_flexnum );
							}
							pszExpression += strlen( szTemp ) - 1;
						}
					}
				}
				pszExpression++;
			}
		} 
		else if (m_flextime < gpGlobals->curtime)
		{
			// m_flextime = gpGlobals->curtime + 1.0; // RandomFloat( 0.1, 0.5 );
			m_flextime = gpGlobals->curtime + random->RandomFloat( 0.3, 0.5 ) * (30.0 / GetNumFlexControllers());
			m_flexnum = random->RandomInt( 0, GetNumFlexControllers() - 1 );

			// m_flexnum = (pflex->num + 1) % r_psubmodel->numflexes;

			if (m_flextarget[m_flexnum] == 1)
			{
				m_flextarget[m_flexnum] = 0;
				// pflex->time = cl.time + 0.1;
			}
			else if (stricmp( GetFlexControllerType( m_flexnum ), "phoneme" ) != 0)
			{
				if (strstr( GetFlexControllerName( m_flexnum ), "upper_raiser" ) == NULL)
				{
					Msg( "%s:%s\n", GetFlexControllerType( m_flexnum ), GetFlexControllerName( m_flexnum ) );
					SetFlexTarget( m_flexnum );
				}
			}

#if 0
			char szWhat[256];
			szWhat[0] = '\0';
			for (int i = 0; i < GetNumFlexControllers(); i++)
			{
				if (m_flextarget[i] == 1.0)
				{
					if (stricmp( GetFlexFacs( i ), "upper") != 0 && stricmp( GetFlexFacs( i ), "lower") != 0)
					{
						if (szWhat[0] == '\0')
							strcat( szWhat, "-" );
						else
							strcat( szWhat, "+" );
						strcat( szWhat, GetFlexFacs( i ) );
					}
				}
			}
			Msg( "%s\n", szWhat );
#endif
		}

		// slide it up.
		for (int i = 0; i < GetNumFlexControllers(); i++)
		{
			if (GetFlexWeight( i ) != m_flextarget[i])
				SetFlexWeight( i, GetFlexWeight( i ) + (m_flextarget[i] - GetFlexWeight( i )) / random->RandomFloat( 2.0, 4.0 ) );

			if (m_flexWeight[i] > 1)
				m_flexWeight.Set( i, 1 );
			if (m_flexWeight[i] < 0)
				m_flexWeight.Set( i, 0 );
		}

#if 1
		if (flex_talk.GetInt() == -1)
		{
			m_istalking = 1;
			char pszSentence[256];
			Q_snprintf( pszSentence,sizeof(pszSentence), "%s%d", STRING(m_iszSentence), m_sentence++ );
			int sentenceIndex = engine->SentenceIndexFromName( pszSentence );
			if (sentenceIndex >= 0)
			{
				Msg( "%d : %s\n", sentenceIndex, pszSentence );
				CPASAttenuationFilter filter( this );
				enginesound->EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, 1, SNDLVL_TALKING, 0, PITCH_NORM );
			}
			else
			{
				m_sentence = 0;
			}

			flex_talk.SetValue( "0" );
		} 
		else if (!FStrEq( flex_talk.GetString(), "0") )
		{
			int sentenceIndex = engine->SentenceIndexFromName( flex_talk.GetString() );
			if (sentenceIndex >= 0)
			{
				CPASAttenuationFilter filter( this );
				enginesound->EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, 1, SNDLVL_TALKING, 0, PITCH_NORM );
			}
			flex_talk.SetValue( "0" );
		}
#else
		if (flex_talk.GetInt())
		{
			if (m_speaktime < gpGlobals->curtime)
			{
				if (m_phoneme == 0)
				{
					for (m_phoneme = 0; m_phoneme < GetNumFlexControllers(); m_phoneme++)
					{
						if (stricmp( GetFlexFacs( m_phoneme ), "27") == 0)
							break;
					}
				}
				m_istalking = !m_istalking;
				if (m_istalking)
				{
					m_looktime = gpGlobals->curtime - 1.0;
					m_speaktime = gpGlobals->curtime + random->RandomFloat( 0.5, 2.0 );
				}
				else
				{
					m_speaktime = gpGlobals->curtime + random->RandomFloat( 1.0, 3.0 );
				}
			}

			for (i = m_phoneme; i < GetNumFlexControllers(); i++)
			{
				SetFlexWeight( i, 0.0f );
			}

			if (m_istalking)
			{
				m_flextime = gpGlobals->curtime + random->RandomFloat( 0.0, 0.2 );
				m_flexWeight[random->RandomInt(m_phoneme, GetNumFlexControllers()-1)] = random->RandomFloat( 0.5, 1.0 );
				float mouth = random->RandomFloat( 0.0, 1.0 );
				float jaw = random->RandomFloat( 0.0, 1.0 );

				m_flexWeight[m_phoneme - 2] = jaw * (mouth);
				m_flexWeight[m_phoneme - 1] = jaw * (1.0 - mouth);
			}
		}
		else
		{
			m_istalking = 0;
		}
#endif

		// blink
		if (m_blinktime < gpGlobals->curtime)
		{
			m_blinktoggle = !m_blinktoggle;
			m_blinktime = gpGlobals->curtime + random->RandomFloat( 1.5, 4.5 );
		}
	}


	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	CBaseEntity *pPlayer = (CBaseEntity *)UTIL_PlayerByIndex( 1 );
	if (pPlayer)
	{
		if (pPlayer->GetAbsVelocity().Length() != 0 && DotProduct( forward, pPlayer->EyePosition() - EyePosition()) > 0.5)
		{
			m_lookTarget = pPlayer->EyePosition();
			m_looktime = gpGlobals->curtime + random->RandomFloat(2.0,4.0);
		}
		else if (m_looktime < gpGlobals->curtime)
		{
			if ((!m_istalking) && random->RandomInt( 0, 1 ) == 0)
			{
				m_lookTarget = EyePosition() + forward * 128 + right * random->RandomFloat(-64,64) + up * random->RandomFloat(-32,32);
				m_looktime = gpGlobals->curtime + random->RandomFloat(0.3,1.0);

				if (m_blinktime - 0.5 < gpGlobals->curtime)
				{
					m_blinktoggle = !m_blinktoggle;
				}
			}
			else
			{
				m_lookTarget = pPlayer->EyePosition();
				m_looktime = gpGlobals->curtime + random->RandomFloat(1.0,4.0);
			}
		}

#if 0
		float dt = acos( DotProduct( (m_lookTarget - EyePosition()).Normalize(), (m_viewtarget - EyePosition()).Normalize() ) );

		if (dt > M_PI / 4)
		{
			dt = (M_PI / 4) * dt;
			m_viewtarget = ((1 - dt) * m_viewtarget + dt * m_lookTarget);
		}
#endif

		m_viewtarget = m_lookTarget;
	}

	// Handle any facial animation from scene playback
	AddSceneExpressions();
}


void CFlexCycler::ProcessExpressions( void )
{
	// Don't do anything since we handle facial stuff in Think()
}


