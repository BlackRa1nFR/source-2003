//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_baseanimatingoverlay.h"
#include "bone_setup.h"
#include "tier0/vprof.h"
#include "engine/IVDebugOverlay.h"


C_BaseAnimatingOverlay::C_BaseAnimatingOverlay()
{
	memset(m_Layer, 0, sizeof(m_Layer));
	AddVar( m_Layer, &m_iv_Layer, LATCH_ANIMATION_VAR );
}


/*
BEGIN_RECV_TABLE_NOBASE(AnimationLayer_t, DT_Animationlayer)
	RecvPropInt(RECVINFO_NAME(nSequence,sequence)),
	RecvPropFloat(RECVINFO_NAME(flCycle,cycle)),
	RecvPropFloat(RECVINFO_NAME(flPlaybackrate,playbackrate)),
	RecvPropFloat(RECVINFO_NAME(flWeight,weight)),
	RecvPropInt(RECVINFO_NAME(flWeight,order))
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_BaseAnimatingOverlay, DT_BaseHumanoid, CAI_BaseHumanoid)
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[0][2],m_Layer0),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[1][2],m_Layer1),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[2][2],m_Layer2),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[3][2],m_Layer3),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[4][2],m_Layer3),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[5][2],m_Layer3),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[6][2],m_Layer3),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[7][2],m_Layer3),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
END_RECV_TABLE()
*/


IMPLEMENT_CLIENTCLASS_DT(C_BaseAnimatingOverlay, DT_BaseAnimatingOverlay, CBaseAnimatingOverlay)
	RecvPropInt(	RECVINFO_NAME(m_Layer[0].nSequence,sequence0)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[0].flCycle,cycle0)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[0].flPlaybackrate,playbackrate0)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[0].flWeight,weight0)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[0].nOrder,order0)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[1].nSequence,sequence1)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[1].flCycle,cycle1)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[1].flPlaybackrate,playbackrate1)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[1].flWeight,weight1)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[1].nOrder,order1)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[2].nSequence,sequence2)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[2].flCycle,cycle2)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[2].flPlaybackrate,playbackrate2)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[2].flWeight,weight2)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[2].nOrder,order2)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[3].nSequence,sequence3)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[3].flCycle,cycle3)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[3].flPlaybackrate,playbackrate3)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[3].flWeight,weight3)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[3].nOrder,order3)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[4].nSequence,sequence4)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[4].flCycle,cycle4)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[4].flPlaybackrate,playbackrate4)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[4].flWeight,weight4)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[4].nOrder,order4)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[5].nSequence,sequence5)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[5].flCycle,cycle5)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[5].flPlaybackrate,playbackrate5)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[5].flWeight,weight5)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[5].nOrder,order5)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[6].nSequence,sequence6)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[6].flCycle,cycle6)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[6].flPlaybackrate,playbackrate6)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[6].flWeight,weight6)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[6].nOrder,order6)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[7].nSequence,sequence7)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[7].flCycle,cycle7)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[7].flPlaybackrate,playbackrate7)),
	RecvPropFloat(	RECVINFO_NAME(m_Layer[7].flWeight,weight7)),
	RecvPropInt(	RECVINFO_NAME(m_Layer[7].nOrder,order7))
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseAnimatingOverlay )

/*
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].flPlaybackrate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].flPlaybackrate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].flPlaybackrate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].flPlaybackrate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].flWeight, FIELD_FLOAT),
*/

END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimatingOverlay::StandardBlendingRules( Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	VPROF( "C_BaseAnimatingOverlay::StandardBlendingRules" );

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	float	poseparam[MAXSTUDIOPOSEPARAM];

	// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%s %6.3f : %6.3f %6.3f : %.3f\n", hdr->name, gpGlobals->curtime, m_flAnimTime, anim.prevanimtime, dadt );

	// interpolate pose parameters
	GetPoseParameters( poseparam );

	// build root animation
	float fCycle = m_flCycle;

	InitPose( hdr, pos, q );
	AccumulatePose( hdr, m_pIk, pos, q, GetSequence(), fCycle, poseparam, boneMask );

	// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%30s %6.2f : %6.2f", hdr->pSeqdesc( m_nSequence )->pszLabel( ), fCycle, 1.0 );

	MaintainSequenceTransitions( fCycle, poseparam, pos, q, boneMask );

	int i;

	// resort the layers
	int layer[MAX_OVERLAYS];
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		layer[i] = MAX_OVERLAYS;
	}
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		if (m_Layer[i].nOrder < MAX_OVERLAYS)
		{
			Assert( layer[m_Layer[i].nOrder] == MAX_OVERLAYS );
			layer[m_Layer[i].nOrder] = i;
		}
	}

	// add in the overlay layers
	int j;
	for (j = 0; j < MAX_OVERLAYS; j++)
	{
		i = layer[ j ];
		if (i < MAX_OVERLAYS)
		{
			float fWeight = m_Layer[i].flWeight;

			/*
			debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), -j - 1, 0, 
				"%2d %2d %2d : %6.2f %6.2f %6.2f : %6.2f %6.2f %6.2f : %6.2f", 
					m_Layer[i][0].nSequence, 
					m_Layer[i][1].nSequence, 
					m_Layer[i][2].nSequence, 
					m_Layer[i][0].flCycle, 
					m_Layer[i][1].flCycle, 
					m_Layer[i][2].flCycle, 
					m_Layer[i][0].flWeight,
					m_Layer[i][1].flWeight,
					m_Layer[i][2].flWeight,
					fWeight
					);
			DevMsg( 1 , "%.3f  %.3f  %.3f\n", currentTime, fWeight, dadt );
			*/
			if (fWeight > 0)
			{
				// check to see if the sequence changed
				// FIXME: move this to somewhere more reasonable
				if ( m_Layer[i].nSequence != m_iv_Layer.GetPrev( i )->nSequence )
				{
					// fake up previous cycle values.
					//float dt = (m_flAnimTime - m_flOldAnimTime);
					//float cps = GetSequenceCycleRate( m_Layer[i].nSequence ) * m_Layer[i].flPlaybackrate;

					/*
					// FIXME BUG BUG:  YWB, need to be able to re-determine this value
					m_Layer[i][1].flCycle = -1 * dt * cps;
					m_Layer[i][0].flCycle = -2 * dt * cps;
					*/
				}

				// do a nice spline interpolation of the values
				fCycle = m_Layer[ i ].flCycle;

				fCycle = ClampCycle( fCycle, IsSequenceLooping( m_Layer[i].nSequence ) );

				if (fWeight > 1)
					fWeight = 1;

				AccumulatePose( hdr, m_pIk, pos, q, m_Layer[i].nSequence, fCycle, poseparam, boneMask, fWeight );
				// Msg( "%30s %6.2f : %6.2f : %1d\n", hdr->pSeqdesc( m_Layer[i].nSequence )->pszLabel(), fCycle, fWeight, i );

//#define DEBUG_TF2_OVERLAYS
#if defined( DEBUG_TF2_OVERLAYS )
				engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", hdr->pSeqdesc( m_Layer[i].nSequence )->pszLabel(), fCycle, fWeight, i );
			}
			else
			{
				engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", "            ", 0.f, 0.f, i );
#endif
			}
		}
#if defined( DEBUG_TF2_OVERLAYS )
		else
		{
			engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", "            ", 0.f, 0.f, i );
		}
#endif
	}

	CIKContext auto_ik;
	auto_ik.Init( hdr, GetRenderAngles(), GetRenderOrigin(), gpGlobals->curtime );
	CalcAutoplaySequences( hdr, &auto_ik, pos, q, poseparam, boneMask, currentTime );

	float controllers[MAXSTUDIOBONECTRLS];
	GetBoneControllers(controllers);
	CalcBoneAdj( hdr, pos, q, controllers, boneMask );
}

