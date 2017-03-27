
#include "cbase.h"
#include "animation.h"
#include "studio.h"
#include "bone_setup.h"
#include "ai_basenpc.h"
#include "npcevent.h"



BEGIN_SIMPLE_DATADESC( CAnimationLayer )

	DEFINE_FIELD( CAnimationLayer, m_fFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CAnimationLayer, m_bSequenceFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAnimationLayer, m_bLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAnimationLayer, m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( CAnimationLayer, m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( CAnimationLayer, m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( CAnimationLayer, m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( CAnimationLayer, m_flBlendIn, FIELD_FLOAT ),
	DEFINE_FIELD( CAnimationLayer, m_flBlendOut, FIELD_FLOAT ),
	DEFINE_FIELD( CAnimationLayer, m_flKillRate, FIELD_FLOAT ),
	DEFINE_FIELD( CAnimationLayer, m_flKillDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CAnimationLayer, m_nActivity, FIELD_INTEGER ),
	DEFINE_FIELD( CAnimationLayer, m_nPriority, FIELD_INTEGER ),
	DEFINE_FIELD( CAnimationLayer, m_nOrder, FIELD_INTEGER ),
	DEFINE_FIELD( CAnimationLayer, m_flLastEventCheck, FIELD_FLOAT ),

END_DATADESC()


BEGIN_DATADESC( CBaseAnimatingOverlay )

	DEFINE_EMBEDDED_AUTO_ARRAY( CBaseAnimatingOverlay, m_AnimOverlay ),

	// DEFINE_FIELD( CBaseAnimatingOverlay, m_nActiveLayers, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseAnimatingOverlay, m_nActiveBaseLayers, FIELD_INTEGER ),

END_DATADESC()



BEGIN_PREDICTION_DATA( CBaseAnimatingOverlay )

/*
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[0].m_fFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[0].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[0].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[0].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[0].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[1].m_fFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[1].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[1].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[1].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[1].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[2].m_fFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[3].m_fFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[3].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[3].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[3].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( CBaseAnimatingOverlay, m_AnimOverlay[3].m_flWeight, FIELD_FLOAT),
*/

END_PREDICTION_DATA()


#define SEQUENCE_BITS		9
#define PLAYBACKRATE_BITS	15
#define ORDER_BITS			4
#define WEIGHT_BITS			8

/*
BEGIN_SEND_TABLE_NOBASE(CAnimationLayer, DT_Animationlayer)
	SendPropInt		(SENDINFO_NAME(m_nSequence,sequence),	SEQUENCE_BITS, SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_flCycle,cycle),		ANIMATION_CYCLE_BITS, SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_flPlaybackRate,playbackrate),	PLAYBACKRATE_BITS,	SPROP_ROUNDUP,	-4.0,	18.0f),
	SendPropFloat	(SENDINFO_NAME(m_flWeight,weight),		WEIGHT_BITS,	0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_nOrder,order0),		ORDER_BITS,	SPROP_UNSIGNED),
END_SEND_TABLE()


IMPLEMENT_SERVERCLASS_ST(CAI_BaseHumanoid, DT_BaseHumanoid)
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[0],m_AnimOverlay[0]), &REFERENCE_SEND_TABLE(DT_Animationlayer)),
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[1],m_AnimOverlay[1]), &REFERENCE_SEND_TABLE(DT_Animationlayer)),
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[2],m_AnimOverlay[2]), &REFERENCE_SEND_TABLE(DT_Animationlayer)),
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[3],m_AnimOverlay[3]), &REFERENCE_SEND_TABLE(DT_Animationlayer))
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[4],m_AnimOverlay[4]), &REFERENCE_SEND_TABLE(DT_Animationlayer))
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[5],m_AnimOverlay[5]), &REFERENCE_SEND_TABLE(DT_Animationlayer))
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[6],m_AnimOverlay[6]), &REFERENCE_SEND_TABLE(DT_Animationlayer))
	SendPropDataTable(SENDINFO_DT_NAME(m_AnimOverlay[7],m_AnimOverlay[7]), &REFERENCE_SEND_TABLE(DT_Animationlayer))
END_SEND_TABLE()
*/

IMPLEMENT_SERVERCLASS_ST(CBaseAnimatingOverlay, DT_BaseAnimatingOverlay)
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[0].m_nSequence,sequence0),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[0].m_flCycle,cycle0),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[0].m_flPlaybackRate,playbackrate0),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f), // NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[0].m_flWeight,weight0),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[0].m_nOrder,order0),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[1].m_nSequence,sequence1),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[1].m_flCycle,cycle1),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[1].m_flPlaybackRate,playbackrate1),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f), // NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[1].m_flWeight,weight1),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[1].m_nOrder,order1),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[2].m_nSequence,sequence2),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[2].m_flCycle,cycle2),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[2].m_flPlaybackRate,playbackrate2),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f),// NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[2].m_flWeight,weight2),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[2].m_nOrder,order2),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[3].m_nSequence,sequence3),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[3].m_flCycle,cycle3),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[3].m_flPlaybackRate,playbackrate3),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f),// NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[3].m_flWeight,weight3),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[3].m_nOrder,order3),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[4].m_nSequence,sequence4),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[4].m_flCycle,cycle4),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[4].m_flPlaybackRate,playbackrate4),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f), // NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[4].m_flWeight,weight4),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[4].m_nOrder,order4),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[5].m_nSequence,sequence5),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[5].m_flCycle,cycle5),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[5].m_flPlaybackRate,playbackrate5),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f), // NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[5].m_flWeight,weight5),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[5].m_nOrder,order5),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[6].m_nSequence,sequence6),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[6].m_flCycle,cycle6),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[6].m_flPlaybackRate,playbackrate6),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f),// NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[6].m_flWeight,weight6),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[6].m_nOrder,order6),					ORDER_BITS,				SPROP_UNSIGNED),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[7].m_nSequence,sequence7),				SEQUENCE_BITS,			SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[7].m_flCycle,cycle7),					ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[7].m_flPlaybackRate,playbackrate7),	PLAYBACKRATE_BITS,		SPROP_ROUNDUP,	-4.0,	28.0f),// NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly
	SendPropFloat	(SENDINFO_NAME(m_AnimOverlay[7].m_flWeight,weight7),				WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO_NAME(m_AnimOverlay[7].m_nOrder,order7),					ORDER_BITS,				SPROP_UNSIGNED),
END_SEND_TABLE()




CAnimationLayer::CAnimationLayer( )
{
	Init( );
}


void CAnimationLayer::Init( )
{
	m_fFlags = 0;
	m_flWeight = 0;
	m_bSequenceFinished = false;
	m_nActivity = ACT_INVALID;
	m_nPriority = 0;
	m_nOrder = CBaseAnimatingOverlay::MAX_OVERLAYS;
	m_flKillRate = 100.0;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------

void CAnimationLayer::StudioFrameAdvance( float flInterval, CBaseAnimating *pOwner )
{
	float flCycleRate = pOwner->GetSequenceCycleRate( m_nSequence );

	m_flCycle += flInterval * flCycleRate * m_flPlaybackRate;

	if (m_flCycle < 0.0)
	{
		if (m_bLooping)
		{
			m_flCycle -= (int)(m_flCycle);
		}
		else
		{
			m_flCycle = 0;
		}
	}
	else if (m_flCycle >= 1.0) 
	{
		m_bSequenceFinished = true;

		if (m_bLooping)
		{
			m_flCycle -= (int)(m_flCycle);
		}
		else
		{
			m_flCycle = 1.0;
		}
	}

	if (IsAutoramp())
	{
		m_flWeight = 1;
	
		// blend in?
		if ( m_flBlendIn != 0.0f )
		{
			if (m_flCycle < m_flBlendIn)
			{
				m_flWeight = m_flCycle / m_flBlendIn;
			}
		}
		
		// blend out?
		if ( m_flBlendOut != 0.0f )
		{
			if (m_flCycle > 1.0 - m_flBlendOut)
			{
				m_flWeight = (1.0 - m_flCycle) / m_flBlendOut;
			}
		}

		m_flWeight = 3.0 * m_flWeight * m_flWeight - 2.0 * m_flWeight * m_flWeight * m_flWeight;
		if (m_nSequence == 0)
			m_flWeight = 0;
	}
}


//------------------------------------------------------------------------------
// Purpose : advance the animation frame up to the current time
//			 if an flInterval is passed in, only advance animation that number of seconds
// Input   :
// Output  :
//------------------------------------------------------------------------------

void CBaseAnimatingOverlay::StudioFrameAdvance ()
{
	float flAdvance = GetAnimTimeInterval();

#ifdef _DEBUG
	{
		int i;
		// test sorting of the layers
		int layer[MAX_OVERLAYS];
		for (i = 0; i < MAX_OVERLAYS; i++)
		{
			layer[i] = MAX_OVERLAYS;
		}
		for (i = 0; i < MAX_OVERLAYS; i++)
		{
			if (m_AnimOverlay[ i ].m_nOrder < MAX_OVERLAYS)
			{
				Assert( layer[m_AnimOverlay[ i ].m_nOrder] == MAX_OVERLAYS );
				layer[m_AnimOverlay[ i ].m_nOrder] = i;
			}
		}
	}
#endif

	BaseClass::StudioFrameAdvance();

	for ( int i = 0; i < MAX_OVERLAYS; i++ )
	{
		if (m_AnimOverlay[ i ].IsActive())
		{
			if (m_AnimOverlay[ i ].IsKillMe())
			{
				if (m_AnimOverlay[ i ].m_flKillDelay > 0)
				{
					m_AnimOverlay[ i ].m_flKillDelay -= flAdvance;
					m_AnimOverlay[ i ].m_flKillDelay = clamp( 	m_AnimOverlay[ i ].m_flKillDelay, 0.0, 1.0 );
				}
				else if (m_AnimOverlay[ i ].m_flWeight != 0.0f)
				{
					// give it at least one frame advance cycle to propegate 0.0 to client
					m_AnimOverlay[ i ].m_flWeight -= m_AnimOverlay[ i ].m_flKillRate * flAdvance;
					m_AnimOverlay[ i ].m_flWeight = clamp( 	m_AnimOverlay[ i ].m_flWeight, 0.0, 1.0 );
				}
				else
				{
					// shift the other layers down in order
					for (int j = 0; j < MAX_OVERLAYS; j++ )
					{
						if ((m_AnimOverlay[ j ].IsActive()) && m_AnimOverlay[ j ].m_nOrder > m_AnimOverlay[ i ].m_nOrder)
						{
							m_AnimOverlay[ j ].m_nOrder--;
						}
					}
					m_AnimOverlay[ i ].Init( );
					continue;
				}
			}

			m_AnimOverlay[ i ].StudioFrameAdvance( flAdvance, this );
			if ( m_AnimOverlay[ i ].m_bSequenceFinished && (m_AnimOverlay[ i ].IsAutokill()) )
			{
				m_AnimOverlay[ i ].m_flWeight = 0.0f;
				m_AnimOverlay[ i ].KillMe();
			}
		}
	}
}



//=========================================================
// DispatchAnimEvents
//=========================================================
void CBaseAnimatingOverlay::DispatchAnimEvents ( CBaseAnimating *eventHandler )
{
	BaseClass::DispatchAnimEvents( eventHandler );

	for ( int i = 0; i < MAX_OVERLAYS; i++ )
	{
		if (m_AnimOverlay[ i ].IsActive())
		{
			m_AnimOverlay[ i ].DispatchAnimEvents( eventHandler, this );
		}
	}
}

void CAnimationLayer::DispatchAnimEvents( CBaseAnimating *eventHandler, CBaseAnimating *pOwner )
{
  	animevent_t	event;

	studiohdr_t *pstudiohdr = pOwner->GetModelPtr( );

	if ( !pstudiohdr )
	{
		Assert(!"CBaseAnimating::DispatchAnimEvents: model missing");
		return;
	}
	
	// don't fire events if the framerate is 0, and skip this altogether if there are no events
	if (m_flPlaybackRate == 0.0 || pstudiohdr->pSeqdesc( m_nSequence )->numevents == 0)
	{
		return;
	}

	// look from when it last checked to some short time in the future	
	float flCycleRate = pOwner->GetSequenceCycleRate( m_nSequence ) * m_flPlaybackRate;
	float flStart = m_flLastEventCheck;
	float flEnd = m_flCycle;

	if (!m_bLooping)
	{
		// fire off events early
		float flLastVisibleCycle = 1.0f - (pstudiohdr->pSeqdesc( m_nSequence )->fadeouttime) * flCycleRate;
		if (flEnd >= flLastVisibleCycle || flEnd < 0.0) 
		{
			m_bSequenceFinished = true;
			flEnd = 1.0f;
		}
	}
	m_flLastEventCheck = flEnd;

	/*
	if (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		Msg( "%s:%s : checking %.2f %.2f (%d)\n", STRING(GetModelName()), pstudiohdr->pSeqdesc( m_nSequence )->pszLabel(), flStart, flEnd, m_bSequenceFinished );
	}
	*/

	// FIXME: does not handle negative framerates!
	int index = 0;
	while ( (index = GetAnimationEvent( pstudiohdr, m_nSequence, &event, flStart, flEnd, index ) ) != 0 )
	{
		event.pSource = pOwner;
		// calc when this event should happen
		event.eventtime = pOwner->m_flAnimTime + (event.cycle - m_flCycle) / flCycleRate;

		// Msg( "dispatch %d (%d : %.2f)\n", index - 1, event.event, event.eventtime );
		eventHandler->HandleAnimEvent( &event );
	}
}



void CBaseAnimatingOverlay::GetSkeleton( Vector pos[], Quaternion q[], int boneMask )
{
	studiohdr_t *pStudioHdr = (studiohdr_t *)modelinfo->GetModelExtraData( GetModel() );
	if(!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetSkeleton() without a model");
		return;
	}

	CalcPose( pStudioHdr, m_pIk, pos, q, GetSequence(), m_flCycle, GetPoseParameterArray(), boneMask );

	// layers
	{
		for (int i = 0; i < MAX_OVERLAYS; i++)
		{
			if (m_AnimOverlay[i].m_flWeight > 0)
			{
				// UNDONE: Is it correct to use overlay weight for IK too?
				AccumulatePose( pStudioHdr, m_pIk, pos, q, m_AnimOverlay[i].m_nSequence, m_AnimOverlay[i].m_flCycle, GetPoseParameterArray(), boneMask, m_AnimOverlay[i].m_flWeight );
			}
		}
	}

	if ( m_pIk )
	{
		CIKContext auto_ik;
		auto_ik.Init( pStudioHdr, GetAbsAngles(), GetAbsOrigin(), gpGlobals->curtime );
		CalcAutoplaySequences( pStudioHdr, &auto_ik, pos, q, GetPoseParameterArray(), boneMask, gpGlobals->curtime );
	}
	else
	{
		CalcAutoplaySequences( pStudioHdr, NULL, pos, q, GetPoseParameterArray(), boneMask, gpGlobals->curtime );
	}
	CalcBoneAdj( pStudioHdr, pos, q, GetEncodedControllerArray(), boneMask );
}



//-----------------------------------------------------------------------------
// Purpose: zero's out all non-restore safe fields
// Output :
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::OnRestore( )
{
	int i;
	
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		if ( m_AnimOverlay[i].IsActive() && (m_AnimOverlay[i].m_fFlags & ANIM_LAYER_DONTRESTORE) )
		{
			m_AnimOverlay[i].KillMe();
		}
	}

	BaseClass::OnRestore();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::AddGestureSequence( int sequence, bool autokill /*= true*/ )
{
	int i = AddLayeredSequence( sequence, 0 );
	// No room?
	if ( IsValidLayer( i ) )
	{
		SetLayerAutokill( i, autokill );
	}

	return i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::AddGestureSequence( int nSequence, float flDuration, bool autokill /*= true*/ )
{
	int iLayer = AddGestureSequence( nSequence, autokill );
	Assert( iLayer != -1 );

	if (iLayer >= 0 && flDuration > 0)
	{
		m_AnimOverlay[iLayer].m_flPlaybackRate = SequenceDuration( nSequence ) / flDuration;
	}
	return iLayer;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CBaseAnimatingOverlay::AddGesture( Activity activity, bool autokill /*= true*/ )
{
	if ( IsPlayingGesture( activity ) )
	{
		return FindGestureLayer( activity );
	}

	int seq = SelectWeightedSequence( activity );
	if ( seq <= 0 )
	{
		const char *actname = CAI_BaseNPC::GetActivityName( activity );
		Msg( "CBaseAnimatingOverlay::AddGesture:  model %s missing activity %s\n", STRING(GetModelName()), actname );
		return -1;
	}

	int i = AddGestureSequence( seq, autokill );
	Assert( i != -1 );
	if ( i != -1 )
	{
		m_AnimOverlay[ i ].m_nActivity = activity;
	}

	return i;
}


int CBaseAnimatingOverlay::AddGesture( Activity activity, float flDuration, bool autokill /*= true*/ )
{
	int iLayer = AddGesture( activity, autokill );
	SetLayerDuration( iLayer, flDuration );

	return iLayer;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------

void CBaseAnimatingOverlay::SetLayerDuration( int iLayer, float flDuration )
{
	if (IsValidLayer( iLayer ) && flDuration > 0)
	{
		m_AnimOverlay[iLayer].m_flPlaybackRate = SequenceDuration( m_AnimOverlay[iLayer].m_nSequence ) / flDuration;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int	CBaseAnimatingOverlay::AddLayeredSequence( int sequence, int iPriority )
{
	int i = AllocateLayer( iPriority );
	Assert( IsValidLayer( i ) );
	// No room?
	if ( IsValidLayer( i ) )
	{
		m_AnimOverlay[i].m_flCycle = 0;
		m_AnimOverlay[i].m_flPlaybackRate = 1.0;
		m_AnimOverlay[i].m_nActivity = ACT_INVALID;
		m_AnimOverlay[i].m_nSequence = sequence;
		m_AnimOverlay[i].m_flWeight = 1.0f;
		m_AnimOverlay[i].m_flBlendIn = 0.0f;
		m_AnimOverlay[i].m_flBlendOut = 0.0f;
		m_AnimOverlay[i].m_bSequenceFinished = false;
		m_AnimOverlay[i].m_flLastEventCheck = 0;
		m_AnimOverlay[i].m_bLooping = ((GetSequenceFlags( sequence ) & STUDIO_LOOPING) != 0);
	}

	return i;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
bool CBaseAnimatingOverlay::IsValidLayer( int iLayer )
{
	return (iLayer >= 0 && iLayer < MAX_OVERLAYS && m_AnimOverlay[iLayer].IsActive());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::AllocateLayer( int iPriority )
{
	int i;

	// look for an open slot and for existing layers that are lower priority
	int iNewOrder = 0;
	int iOpenLayer = -1;
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		if ( m_AnimOverlay[i].IsActive() )
		{
			if (m_AnimOverlay[i].m_nPriority <= iPriority)
			{
				iNewOrder = max( iNewOrder, m_AnimOverlay[i].m_nOrder + 1 );
			}
		}
		else if (iOpenLayer == -1)
		{
			iOpenLayer = i;
		}
	}

	if (iOpenLayer == -1)
		return -1;


	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		if ( m_AnimOverlay[i].m_nOrder >= iNewOrder && m_AnimOverlay[i].m_nOrder < MAX_OVERLAYS)
		{
			m_AnimOverlay[i].m_nOrder++;
		}
	}

	m_AnimOverlay[iOpenLayer].m_fFlags = ANIM_LAYER_ACTIVE;
	m_AnimOverlay[iOpenLayer].m_nOrder = iNewOrder;
	m_AnimOverlay[iOpenLayer].m_nPriority = iPriority;

	return iOpenLayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
int	CBaseAnimatingOverlay::FindGestureLayer( Activity activity )
{
	for (int i = 0; i < MAX_OVERLAYS; i++)
	{
		if ( !(m_AnimOverlay[i].IsActive()) )
			continue;

		if ( m_AnimOverlay[i].IsKillMe() )
			continue;

		if ( m_AnimOverlay[i].m_nActivity == ACT_INVALID )
			continue;

		if ( m_AnimOverlay[i].m_nActivity == activity )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseAnimatingOverlay::IsPlayingGesture( Activity activity )
{
	return FindGestureLayer( activity ) != -1 ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RestartGesture( Activity activity, bool addifmissing /*=true*/, bool autokill /*=true*/ )
{
	int idx = FindGestureLayer( activity );
	if ( idx == -1 )
	{
		if ( addifmissing )
		{
			AddGesture( activity, autokill );
		}
		return;
	}

	m_AnimOverlay[ idx ].m_flCycle = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RemoveGesture( Activity activity )
{
	int iLayer = FindGestureLayer( activity );
	if ( iLayer == -1 )
		return;

	RemoveLayer( iLayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RemoveAllGestures( void )
{
	for (int i = 0; i < MAX_OVERLAYS; i++)
	{
		RemoveLayer( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerCycle( int iLayer, float flCycle )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (!m_AnimOverlay[iLayer].m_bLooping)
	{
		flCycle = clamp( flCycle, 0.0, 1.0 );
	}
	m_AnimOverlay[iLayer].m_flCycle = flCycle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseAnimatingOverlay::GetLayerCycle( int iLayer )
{
	if (!IsValidLayer( iLayer ))
		return 0.0;

	return m_AnimOverlay[iLayer].m_flCycle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerPlaybackRate( int iLayer, float flPlaybackRate )
{
	if (!IsValidLayer( iLayer ))
		return;

	Assert( flPlaybackRate > -1.0 && flPlaybackRate < 20.0);

	m_AnimOverlay[iLayer].m_flPlaybackRate = flPlaybackRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerWeight( int iLayer, float flWeight )
{
	if (!IsValidLayer( iLayer ))
		return;

	flWeight = clamp( flWeight, 0.0f, 1.0f );
	m_AnimOverlay[iLayer].m_flWeight = flWeight;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseAnimatingOverlay::GetLayerWeight( int iLayer )
{
	if (!IsValidLayer( iLayer ))
		return 0.0;

	return m_AnimOverlay[iLayer].m_flWeight;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerBlendIn( int iLayer, float flBlendIn )
{
	if (!IsValidLayer( iLayer ))
		return;

	m_AnimOverlay[iLayer].m_flBlendIn = flBlendIn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerBlendOut( int iLayer, float flBlendOut )
{
	if (!IsValidLayer( iLayer ))
		return;

	m_AnimOverlay[iLayer].m_flBlendOut = flBlendOut;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerAutokill( int iLayer, bool bAutokill )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (bAutokill)
	{
		m_AnimOverlay[iLayer].m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		m_AnimOverlay[iLayer].m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerLooping( int iLayer, bool bLooping )
{
	if (!IsValidLayer( iLayer ))
		return;

	m_AnimOverlay[iLayer].m_bLooping = bLooping;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerNoRestore( int iLayer, bool bNoRestore )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (bNoRestore)
	{
		m_AnimOverlay[iLayer].m_fFlags |= ANIM_LAYER_DONTRESTORE;
	}
	else
	{
		m_AnimOverlay[iLayer].m_fFlags &= ~ANIM_LAYER_DONTRESTORE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBaseAnimatingOverlay::GetLayerActivity( int iLayer )
{
	if (!IsValidLayer( iLayer ))
	{
		return ACT_INVALID;
	}
		
	return m_AnimOverlay[iLayer].m_nActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RemoveLayer( int iLayer, float flKillRate, float flKillDelay )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (flKillRate > 0)
	{
		m_AnimOverlay[iLayer].m_flKillRate = m_AnimOverlay[iLayer].m_flWeight / flKillRate;
	}
	else
	{
		m_AnimOverlay[iLayer].m_flKillRate = 100;
	}

	m_AnimOverlay[iLayer].m_flKillDelay = flKillDelay;

	m_AnimOverlay[iLayer].KillMe();
}




//-----------------------------------------------------------------------------
