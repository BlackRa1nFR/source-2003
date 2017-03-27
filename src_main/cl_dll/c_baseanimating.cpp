//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_baseanimating.h"
#include "c_Sprite.h"
#include "model_types.h"
#include "bone_setup.h"
#include "ivrenderview.h"
#include "r_efx.h"
#include "dlight.h"
#include "beamdraw.h"
#include "cl_animevent.h"
#include "engine/IEngineSound.h"
#include "c_te_legacytempents.h"
#include "c_clientstats.h"
#include "activitylist.h"
#include "animation.h"
#include "tier0/vprof.h"
#include "ClientEffectPrecacheSystem.h"
#include "ieffects.h"
#include "engine/ivmodelinfo.h"
#include "engine/IVDebugOverlay.h"
#include "c_te_effect_dispatch.h"
#include <KeyValues.h>
#include "c_rope.h"
#include "isaverestore.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar r_drawmrmmodels(  "r_drawmrmmodels", "1" );
static ConVar r_drawvehicles( "r_drawvehicles", "1" );

// Removed macro used by shared code stuff
#if defined( CBaseAnimating )
#undef CBaseAnimating
#endif


CLIENTEFFECT_REGISTER_BEGIN( PrecacheBaseAnimating )
CLIENTEFFECT_MATERIAL( "sprites/fire" )
CLIENTEFFECT_REGISTER_END()

BEGIN_RECV_TABLE_NOBASE( C_BaseAnimating, DT_ServerAnimationData )
	RecvPropFloat(RECVINFO(m_flCycle)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_BaseAnimating, DT_BaseAnimating, CBaseAnimating)
	RecvPropInt(RECVINFO(m_nSequence)),
	RecvPropInt(RECVINFO(m_nForceBone)),
	RecvPropVector(RECVINFO(m_vecForce)),
	RecvPropInt(RECVINFO(m_nSkin)),
	RecvPropInt(RECVINFO(m_nBody)),
	RecvPropInt(RECVINFO(m_nHitboxSet)),
	RecvPropFloat(RECVINFO(m_flModelScale)),
	RecvPropArray(RecvPropFloat(RECVINFO(m_flPoseParameter[0])), m_flPoseParameter),
	RecvPropFloat(RECVINFO(m_flPlaybackRate)),
	RecvPropArray(RecvPropFloat(RECVINFO(m_flEncodedController[0])), m_flEncodedController),

	RecvPropInt( RECVINFO( m_bClientSideAnimation )),
	RecvPropInt( RECVINFO( m_bClientSideFrameReset )),

	RecvPropInt( RECVINFO( m_nNewSequenceParity )),
	RecvPropInt( RECVINFO( m_nResetEventsParity )),

	RecvPropDataTable( "serveranimdata", 0, 0, &REFERENCE_RECV_TABLE( DT_ServerAnimationData ) ),
END_RECV_TABLE()

// Incremented each frame in InvalidateModelBones. Models compare this value to what it
// was last time they setup their bones to determine if they need to re-setup their bones.
static unsigned long	g_iModelBoneCounter = 0;

//-----------------------------------------------------------------------------
// Purpose: convert axis rotations to a quaternion
//-----------------------------------------------------------------------------
C_BaseAnimating::C_BaseAnimating()
{
#ifdef _DEBUG
	m_vecForce.Init();
#endif

	m_nPrevSequence = -1;
	m_nRestoreSequence = -1;
	m_pRagdoll		= NULL;
	m_builtRagdoll = false;
	int i;
	for ( i = 0; i < ARRAYSIZE( m_flEncodedController ); i++ )
	{
		m_flEncodedController[ i ] = 0.0f;
	}

	AddVar( m_flEncodedController, &m_iv_flEncodedController, LATCH_ANIMATION_VAR | EXCLUDE_AUTO_INTERPOLATE );
	AddVar( m_flPoseParameter, &m_iv_flPoseParameter, LATCH_ANIMATION_VAR | EXCLUDE_AUTO_INTERPOLATE );

	AddVar( &m_flCycle, &m_iv_flCycle, LATCH_ANIMATION_VAR | EXCLUDE_AUTO_INTERPOLATE );

	m_lastPhysicsBone = 0;
	m_iMostRecentModelBoneCounter = 0xFFFFFFFF;
	m_CachedBoneFlags = 0;

	m_vecPreRagdollMins = vec3_origin;
	m_vecPreRagdollMaxs = vec3_origin;

	m_bStoreRagdollInfo = false;
	m_pRagdollInfo = NULL;

	m_flPlaybackRate = 1.0f;

	m_nEventSequence = -1;

	m_pIk = NULL;

	// Assume false.  Derived classes might fill in a receive table entry
	// and in that case this would show up as true
	m_bClientSideAnimation = false;

	m_nPrevNewSequenceParity = -1;
	m_nPrevResetEventsParity = -1;
}

//-----------------------------------------------------------------------------
// Purpose: cleanup
//-----------------------------------------------------------------------------
C_BaseAnimating::~C_BaseAnimating()
{
	TermRopes();
	delete m_pRagdollInfo;
	Assert(!m_pRagdoll);
	delete m_pIk;
}


bool C_BaseAnimating::UsesFrameBufferTexture( void )
{
	studiohdr_t *pmodel  = modelinfo->GetStudiomodel( GetModel() );
	if ( !pmodel )
		return false;

	return ( pmodel->flags & STUDIOHDR_FLAGS_USES_FB_TEXTURE ) ? true : false;
}

//-----------------------------------------------------------------------------
// Should this object cast render-to-texture shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_BaseAnimating::ShadowCastType()
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	if (hdr->numanim == 0)
		return SHADOWS_RENDER_TO_TEXTURE;
		  
	// FIXME: Need to check bone controllers and pose parameters
	if ((hdr->numanim == 1) && (hdr->pAnimdesc(0)->numframes == 1))
		return SHADOWS_RENDER_TO_TEXTURE;

	// FIXME: Do something to check to see how many frames the current animation has
	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

//-----------------------------------------------------------------------------
// Purpose: convert axis rotations to a quaternion
//-----------------------------------------------------------------------------

studiohdr_t *C_BaseAnimating::OnNewModel()
{
	if ( !GetModel() )
		return NULL;
	
	studiohdr_t *hdr = modelinfo->GetStudiomodel( GetModel() );
	if (hdr == NULL)
		return NULL;

	InvalidateBoneCache();

	// Make sure m_CachedBones has space.
	if ( m_CachedBones.Count() != hdr->numbones )
	{
		m_CachedBones.SetSize( hdr->numbones );
	}

	// Don't reallocate unless a different size. 
	if ( m_Attachments.Count() != hdr->numattachments)
	{
		m_Attachments.SetSize( hdr->numattachments );

#ifdef _DEBUG
		// This is to make sure we don't use the attachment before its been set up
		for ( int i=0; i < m_Attachments.Count(); i++ )
		{
			float *pOrg = m_Attachments[i].m_vOrigin.Base();
			float *pAng = m_Attachments[i].m_angRotation.Base();
			pOrg[0] = pOrg[1] = pOrg[2] = VEC_T_NAN;
			pAng[0] = pAng[1] = pAng[2] = VEC_T_NAN;
		}
#endif

	}

	Assert( hdr->numposeparameters <= ARRAYSIZE( m_flPoseParameter ) );

	m_iv_flPoseParameter.SetMaxCount( hdr->numposeparameters );
	
	int i;
	for ( i = 0; i < hdr->numposeparameters ; i++ )
	{
		mstudioposeparamdesc_t *pPose = hdr->pPoseParameter( i );
		m_iv_flPoseParameter.SetLooping( i, pPose->loop != 0.0f );
	}

	int boneControllerCount = min( hdr->numbonecontrollers, ARRAYSIZE( m_flEncodedController ) );

	m_iv_flEncodedController.SetMaxCount( boneControllerCount );

	for ( i = 0; i < boneControllerCount ; i++ )
	{
		bool loop = (hdr->pBonecontroller( i )->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) != 0;
		m_iv_flEncodedController.SetLooping( i, loop );
	}

	InitRopes();
	return hdr;
}


studiohdr_t* C_BaseAnimating::GetModelPtr()
{ 
	if ( !GetModel() )
		return NULL;

	studiohdr_t *hdr = modelinfo->GetStudiomodel( GetModel() );
	return hdr;
}


void C_BaseAnimating::InitRopes()
{
	TermRopes();
	
	// Parse the keyvalues and see if they want to make ropes on this model.
	KeyValues * modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		// Do we have a build point section?
		KeyValues *pkvAllCables = modelKeyValues->FindKey("Cables");
		if ( pkvAllCables )
		{
			// Start grabbing the sounds and slotting them in
			for ( KeyValues *pSingleCable = pkvAllCables->GetFirstSubKey(); pSingleCable; pSingleCable = pSingleCable->GetNextKey() )
			{
				C_RopeKeyframe *pRope = C_RopeKeyframe::CreateFromKeyValues( this, pSingleCable );
				m_Ropes.AddToTail( pRope );
			}
		}
	}

	modelKeyValues->deleteThis();
}


void C_BaseAnimating::TermRopes()
{
	FOR_EACH_LL( m_Ropes, i )
		m_Ropes[i]->Release();

	m_Ropes.Purge();
}

void C_BaseAnimating::GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// interpolate two 0..1 encoded controllers to a single 0..1 controller
	int i;
	for( i=0; i < MAXSTUDIOBONECTRLS; i++)
	{
		controllers[ i ] = m_flEncodedController[ i ];
	}
}


void C_BaseAnimating::GetPoseParameters( float poseParameter[MAXSTUDIOPOSEPARAM])
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// interpolate pose parameters
	int i;
	for( i=0; i < hdr->numposeparameters; i++)
	{
		poseParameter[i] = m_flPoseParameter[i];
	}
}


float C_BaseAnimating::ClampCycle( float cycle, bool isLooping )
{
	if (isLooping) 
	{
		// FIXME: does this work with negative framerate?
		cycle = cycle - (int)cycle;
		if (cycle < 0)
			cycle += 1;
	}
	else 
	{
		cycle = clamp( cycle, 0.0, 0.999 );
	}
	return cycle;
}


void C_BaseAnimating::GetCachedBoneMatrix( int boneIndex, matrix3x4_t &out )
{
	MatrixCopy( m_CachedBones[ boneIndex ], out );
}

//-----------------------------------------------------------------------------
// Purpose:	move position and rotation transforms into global matrices
//-----------------------------------------------------------------------------
void C_BaseAnimating::BuildTransformations( Vector *pos, Quaternion *q, const matrix3x4_t &cameraTransform )
{
	VPROF_BUDGET( "C_BaseAnimating::BuildTransformations", VPROF_BUDGETGROUP_OTHER_ANIMATION );

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	matrix3x4_t bonematrix;
	bool boneSimulated[MAXSTUDIOBONES];

	// no bones have been simulated
	memset( boneSimulated, 0, sizeof(boneSimulated) );
	mstudiobone_t *pbones = hdr->pBone( 0 );

	if ( m_pRagdoll )
	{
		// simulate bones and update flags
		m_pRagdoll->RagdollBone( this, pbones, hdr->numbones, boneSimulated, m_CachedBones.Base() );
	}

	C_BaseAnimating *follow = NULL;
	if ( IsFollowingEntity() )
	{
		follow = FindFollowedEntity();
		Assert( follow != this );
		if ( follow )
		{
			follow->SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );
		}
	}

	for (int i = 0; i < hdr->numbones; i++) 
	{
		if ( follow )
		{
			studiohdr_t *fhdr = follow->GetModelPtr();
			if ( fhdr )
			{
				int j;
				mstudiobone_t *pfbones = fhdr->pBone( 0 );

				for (j = 0; j < fhdr->numbones; j++)
				{
					if ( stricmp(pbones[i].pszName(), pfbones[j].pszName() ) == 0 )
					{
						MatrixCopy( follow->m_CachedBones[ j ], m_CachedBones[ i ] );
						break;
					}
				}
				if ( j < fhdr->numbones )
					continue;
			}
		}

		// animate all non-simulated bones
		if ( boneSimulated[i] )
		{
			continue;
		}
		else if (CalcProceduralBone( hdr, i, m_CachedBones.Base() ))
		{
			continue;
		}
		else
		{
			QuaternionMatrix( q[i], bonematrix );

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1) 
			{
				ConcatTransforms( cameraTransform, bonematrix, m_CachedBones[i] );

				// Apply client-side effects to the transformation matrix
				// CL_FxTransform( this,pBoneToWorld[i] );
			} 
			else 
			{
				ConcatTransforms( m_CachedBones[pbones[i].parent], bonematrix, m_CachedBones[i] );
			}
		}
	}
}

void C_BaseAnimating::SaveRagdollInfo( int numbones, const matrix3x4_t &cameraTransform, matrix3x4_t *pBoneToWorld )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	if ( !m_pRagdollInfo )
	{
		m_pRagdollInfo = new RagdollInfo_t;
		Assert( m_pRagdollInfo );
		if ( !m_pRagdollInfo )
		{
			Msg( "Memory allocation of RagdollInfo_t failed!\n" );
			return;
		}
		memset( m_pRagdollInfo, 0, sizeof( *m_pRagdollInfo ) );
	}

	mstudiobone_t *pbones = hdr->pBone( 0 );

	m_pRagdollInfo->m_bActive = true;
	m_pRagdollInfo->m_flSaveTime = gpGlobals->curtime;
	m_pRagdollInfo->m_nNumBones = numbones;

	for ( int i = 0;  i < numbones; i++ )
	{
		matrix3x4_t inverted;
		matrix3x4_t output;

		if ( pbones[i].parent == -1 )
		{
			// Decompose into parent space
			MatrixInvert( cameraTransform, inverted );
		}
		else
		{
			MatrixInvert( pBoneToWorld[ pbones[ i ].parent ], inverted );
		}

		ConcatTransforms( inverted, pBoneToWorld[ i ], output );

		MatrixAngles( output, 
			m_pRagdollInfo->m_rgBoneQuaternion[ i ],
			m_pRagdollInfo->m_rgBonePos[ i ] );
	}
}

bool C_BaseAnimating::RetrieveRagdollInfo( Vector *pos, Quaternion *q )
{
	if ( !m_bStoreRagdollInfo || !m_pRagdollInfo || !m_pRagdollInfo->m_bActive )
		return false;

	for ( int i = 0; i < m_pRagdollInfo->m_nNumBones; i++ )
	{
		pos[ i ] = m_pRagdollInfo->m_rgBonePos[ i ];
		q[ i ] = m_pRagdollInfo->m_rgBoneQuaternion[ i ];
	}

	return true;
}

//-----------------------------------------------------------------------------
// Should we collide?
//-----------------------------------------------------------------------------

CollideType_t C_BaseAnimating::ShouldCollide( )
{
	if ( IsRagdoll() )
		return ENTITY_SHOULD_RESPOND;

	return BaseClass::ShouldCollide();
}

//-----------------------------------------------------------------------------
// Purpose: if the active sequence changes, keep track of the previous ones and decay them based on their decay rate
//-----------------------------------------------------------------------------
void C_BaseAnimating::MaintainSequenceTransitions( float flCycle, float flPoseParameter[], Vector pos[], Quaternion q[], int boneMask )
{
	VPROF( "C_BaseAnimating::MaintainSequenceTransitions" );

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// FIXME?: this should detect that what's been asked to be drawn isn't what was expected
	// due to not only sequence change, by frame index, rate, or whatever.  When that happens, 
	// it should insert the previous rules.

	if (m_animationQueue.Count() == 0)
	{
		m_animationQueue.AddToTail();
		m_animationQueue[0].nSequence = 0;
		m_animationQueue[0].flAnimtime = 0;
		m_animationQueue[0].flCycle = 0;
		m_animationQueue[0].flPlaybackrate = 0;
	}

	AnimationLayer_t *currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	bool newSeq = m_nNewSequenceParity != m_nPrevNewSequenceParity;
	m_nPrevNewSequenceParity = m_nNewSequenceParity;

	if (currentblend->flAnimtime && 
		(currentblend->nSequence != m_nSequence || newSeq))
	{
		mstudioseqdesc_t *pseqdesc = hdr->pSeqdesc( m_nSequence );
		// sequence changed
		if ((pseqdesc->flags & STUDIO_SNAP) || IsEffectActive(EF_NOINTERP))
		{
			// remove all entries
			m_animationQueue.RemoveAll();
		}
		else
		{
			currentblend->flFadeOuttime = pseqdesc->fadeouttime;
			/*
			// clip blends to time remaining
			if ( !IsSequenceLooping(currentblend->nSequence) )
			{
				float length = Studio_Duration( hdr, currentblend->nSequence, flPoseParameter ) / currentblend->flPlaybackrate;
				float timeLeft = (1.0 - currentblend->flCycle) * length;
				if (timeLeft < currentblend->flFadeOuttime)
					currentblend->flFadeOuttime = timeLeft;
			}
			*/
		}
		// push previously set sequence
		m_animationQueue.AddToTail();
		currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	}

	// keep track of current sequence
	currentblend->nSequence = m_nSequence;
	currentblend->flAnimtime = gpGlobals->curtime;
	currentblend->flCycle = flCycle;
	currentblend->flPlaybackrate = m_flPlaybackRate;

	// calc blending weights for previous sequences
	int i;
	AnimationLayer_t *blend;
	for (i = 0; i < m_animationQueue.Count() - 1;)
	{
 		float s;
		blend = &m_animationQueue[i];
		if (blend->flFadeOuttime <= 0.0f)
		{
			s = 0;
		}
		else
		{
			// blend in over 0.2 seconds
			s = 1.0 - (gpGlobals->curtime - blend->flAnimtime) / blend->flFadeOuttime;
			if (s > 0 && s <= 1.0)
			{
				// do a nice spline curve
				s = 3 * s * s - 2 * s * s * s;
			}
			else if ( s > 1.0f )
			{
				// Shouldn't happen, but maybe curtime is behind animtime?
				s = 1.0f;
			}
		}

		if (s > 0)
		{
			blend->flWeight = s;
			i++;
		}
		else
		{
			m_animationQueue.Remove( i );
		}
	}

	// process previous sequences
	for (i = m_animationQueue.Count() - 2; i >= 0; i--)
	{
		blend = &m_animationQueue[i];

		float dt = (gpGlobals->curtime - blend->flAnimtime);
		flCycle = blend->flCycle + dt * blend->flPlaybackrate * GetSequenceCycleRate( blend->nSequence );
		flCycle = ClampCycle( flCycle, IsSequenceLooping( blend->nSequence ) );

		AccumulatePose( hdr, m_pIk, pos, q, blend->nSequence, flCycle, flPoseParameter, boneMask, blend->flWeight );
	}
}




//-----------------------------------------------------------------------------
// Purpose: Do the default sequence blending rules as done in HL1
//-----------------------------------------------------------------------------
void C_BaseAnimating::StandardBlendingRules( Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	VPROF( "C_BaseAnimating::StandardBlendingRules" );

	float		poseparam[MAXSTUDIOPOSEPARAM];

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	if (m_nSequence >=  hdr->numseq) 
	{
		m_nSequence = 0;
	}

	GetPoseParameters( poseparam);

	// build root animation
	float fCycle = m_flCycle;

	InitPose( hdr, pos, q );
	AccumulatePose( hdr, m_pIk, pos, q, m_nSequence, fCycle, poseparam, boneMask );

	// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%30s %6.2f : %6.2f", hdr->pSeqdesc( m_nSequence )->pszLabel( ), fCycle, 1.0 );

	MaintainSequenceTransitions( fCycle, poseparam, pos, q, boneMask );

	CIKContext auto_ik;
	auto_ik.Init( hdr, GetRenderAngles(), GetRenderOrigin(), gpGlobals->curtime );
	CalcAutoplaySequences( hdr, &auto_ik, pos, q, poseparam, boneMask, currentTime );

	float controllers[MAXSTUDIOBONECTRLS];
	GetBoneControllers(controllers);
	CalcBoneAdj( hdr, pos, q, controllers, boneMask );
}


//-----------------------------------------------------------------------------
// Purpose: Put a value into an attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool C_BaseAnimating::PutAttachment( int number, const Vector &origin, const QAngle &angles )
{
	if ( number < 1 || number > m_Attachments.Count() )
	{
		return false;
	}

	m_Attachments[number-1].m_vOrigin = origin;
	m_Attachments[number-1].m_angRotation = angles;
	return true;
}


void C_BaseAnimating::SetupBones_AttachmentHelper()
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// calculate attachment points
	mstudioattachment_t *pattachment = hdr->pAttachment( 0 );
	matrix3x4_t world;
	for (int i = 0; i < hdr->numattachments; i++)
	{
		ConcatTransforms( m_CachedBones[ pattachment[i].bone ], pattachment[i].local, world ); 

		// FIXME: this shouldn't be here, it should client side on-demand only and hooked into the bone cache!!
		QAngle angles;
		Vector origin;
		MatrixAngles( world, angles, origin );
		FormatViewModelAttachment( i, origin, angles );
		PutAttachment( i + 1, origin, angles );
	}
}

bool C_BaseAnimating::CalcAttachments()
{
	VPROF( "C_BaseAnimating::CalcAttachments" );
	// Make sure m_CachedBones is valid.
	if ( !SetupBones( NULL, -1, BONE_USED_BY_ATTACHMENT, gpGlobals->curtime ) )
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	// Note: this could be more efficient, but we want the matrix3x4_t version of GetAttachment to be the origin of
	// attachment generation, so a derived class that wants to fudge attachments only 
	// has to reimplement that version. This also makes it work like the server in that regard.
	matrix3x4_t attachmentToWorld;
	if ( !GetAttachment( number, attachmentToWorld) )
		return false;

	MatrixAngles( attachmentToWorld, angles );
	MatrixPosition( attachmentToWorld, origin );
	return true;
}


// UNDONE: Should be able to do this directly!!!
//			Attachments begin as matrices!!
bool C_BaseAnimating::GetAttachment( int number, matrix3x4_t& matrix )
{
	if ( number < 1 || number > m_Attachments.Count() )
	{
		return false;
	}

	if ( !CalcAttachments() )
		return false;

	Vector &origin = m_Attachments[number-1].m_vOrigin;
	QAngle &angles = m_Attachments[number-1].m_angRotation;
	AngleMatrix( angles, origin, matrix );
	return true;
}

//-----------------------------------------------------------------------------
// Returns the attachment in local space
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal )
{
	matrix3x4_t attachmentToWorld;
	if (!GetAttachment(iAttachment, attachmentToWorld))
		return false;

	matrix3x4_t worldToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );
	ConcatTransforms( worldToEntity, attachmentToWorld, attachmentToLocal ); 
	return true;
}

bool C_BaseAnimating::GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles )
{
	matrix3x4_t attachmentToEntity;

	if (GetAttachmentLocal( iAttachment, attachmentToEntity ))
	{
		origin.Init( attachmentToEntity[0][3], attachmentToEntity[1][3], attachmentToEntity[2][3] );
		MatrixAngles( attachmentToEntity, angles );
		return true;
	}
	return false;
}



bool C_BaseAnimating::IsViewModel() const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
mstudiohitboxset_t *C_BaseAnimating::GetTransformedHitboxSet( int nBoneMask )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return NULL;
	}

	mstudiohitboxset_t *set = hdr->pHitboxSet( m_nHitboxSet );
	if ( set && set->numhitboxes )
	{
		studiocache_t *pcache = Studio_GetBoneCache( hdr, m_nSequence, m_flAnimTime, GetAbsAngles(), GetAbsOrigin(), nBoneMask );
		if ( !pcache )
		{
			matrix3x4_t bonetoworld[MAXSTUDIOBONES];

			SetupBones( bonetoworld, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, gpGlobals->curtime );
			pcache = Studio_SetBoneCache( hdr, m_nSequence, m_flAnimTime, GetAbsAngles(), GetAbsOrigin(), nBoneMask, bonetoworld );
		}

		matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
		Studio_LinkHitboxCache( hitboxbones, pcache, hdr, set );
	}

	return set;
}



void C_BaseAnimating::CalculateIKLocks( float currentTime )
{
	if (m_pIk) 
	{
		// In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
		// get in here during the view setup code, and it's not normally supposed to be able to access the spatial
		// partition that early in the rendering loop. So we allow access right here for that special case.
		SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
		partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );


		// FIXME: trace based on gravity or trace based on angles?
		Vector up;
		AngleVectors( GetRenderAngles(), NULL, NULL, &up );

		// FIXME: check number of slots?
		for (int i = 0; i < m_pIk->m_target.Count(); i++)
		{
			trace_t tr;
			iktarget_t *pTarget = &m_pIk->m_target[i];

			if (pTarget->est.time != currentTime)
				continue;

			Vector p1, p2;
			VectorMA( pTarget->est.pos, pTarget->est.height, up, p1 );
			VectorMA( pTarget->est.pos, -pTarget->est.height, up, p2 );

			float r = pTarget->est.radius;

			UTIL_TraceHull( p1, p2, Vector( -r, -r, 0 ), Vector( r, r, 1 ), 
				MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
			pTarget->est.pos = tr.endpos;

			/*
			debugoverlay->AddTextOverlay( p1, 0, 0, "%d", i );
			debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -2, -2, -2 ), Vector( 2, 2, 2), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );
			debugoverlay->AddBoxOverlay( pTarget->latched.pos, Vector( -2, -2, 2 ), Vector( 2, 2, 6), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
			*/
		}

		partition->SuppressLists( curSuppressed, true );
	}
}

mstudioposeparamdesc_t *C_BaseAnimating::GetPoseParameterPtr( const char *pName )
{
	studiohdr_t *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
		  return NULL;

	for (int i = 0; i < pstudiohdr->numposeparameters; i++)
	{
		mstudioposeparamdesc_t *pPose = pstudiohdr->pPoseParameter( i );
		
		if ( pPose && ( stricmp( pstudiohdr->pPoseParameter( i )->pszName(), pName ) == 0 ) )
		{
			return pPose;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Do HL1 style lipsynch
//-----------------------------------------------------------------------------
void C_BaseAnimating::ControlMouth( void )
{
	studiohdr_t *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
		  return;

	mstudioposeparamdesc_t *pPose = GetPoseParameterPtr( LIPSYNC_POSEPARAM_NAME );

	if ( pPose )
	{
		float value = GetMouth()->mouthopen / 64.0;

		float raw = value;

		if ( value > 1.0 )  
			 value = 1.0;
		
		value = (1.0 - value) * pPose->start + value * pPose->end;

		//Adrian - Set the pose parameter value. 
		//It has to be called "mouth".
		SetPoseParameter( LIPSYNC_POSEPARAM_NAME, value ); 
		// Reset interpolation here since the client is controlling this rather than the server...
		int index = LookupPoseParameter( LIPSYNC_POSEPARAM_NAME );
		if ( index >= 0 )
		{
			m_iv_flPoseParameter.SetHistoryValuesForItem( index, raw );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do the default sequence blending rules as done in HL1
//-----------------------------------------------------------------------------
bool C_BaseAnimating::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	VPROF_BUDGET( "C_BaseAnimating::SetupBones", VPROF_BUDGETGROUP_OTHER_ANIMATION );

	Assert( IsBoneAccessAllowed() );

	boneMask = BONE_USED_BY_ANYTHING; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.
	
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return false;
	}

	if( m_iMostRecentModelBoneCounter != g_iModelBoneCounter )
	{
		// Clear out which bones we've touched this frame if this is 
		// the first time we've seen this object this frame.
		m_CachedBoneFlags = 0;
	}

	// Have we cached off all bones meeting the flag set?
	if( ( m_CachedBoneFlags & boneMask ) != boneMask )
	{
		MEASURE_TIMED_STAT( CS_BONE_SETUP_TIME );

		// Setup our transform based on render angles and origin.
		matrix3x4_t parentTransform;
		AngleMatrix( GetRenderAngles(), GetRenderOrigin(), parentTransform );

		
		if (hdr->flags & STUDIOHDR_FLAGS_STATIC_PROP)
		{
			MatrixCopy(	parentTransform, m_CachedBones[0] );
		}
		else
		{
			if (!m_pIk)
				m_pIk = new CIKContext;

			Vector		pos[MAXSTUDIOBONES];
			Quaternion	q[MAXSTUDIOBONES];

			m_pIk->Init( hdr, GetRenderAngles(), GetRenderOrigin(), currentTime );

			int bonesMaskNeedRecalc = boneMask & ~m_CachedBoneFlags;
			StandardBlendingRules( pos, q, currentTime, bonesMaskNeedRecalc );

			CalculateIKLocks( currentTime );
			m_pIk->SolveDependencies( pos, q );

			BuildTransformations( pos, q, parentTransform );
		}
		if( !( m_CachedBoneFlags & BONE_USED_BY_ATTACHMENT ) && ( boneMask & BONE_USED_BY_ATTACHMENT ) )
		{
			SetupBones_AttachmentHelper();
		}
	
	}

	ControlMouth();
	
	// Do they want to get at the bone transforms? If it's just making sure an aiment has 
	// its bones setup, it doesn't need the transforms yet.
	if ( pBoneToWorldOut )
	{
		if ( nMaxBones >= m_CachedBones.Count() )
		{
			memcpy( pBoneToWorldOut, m_CachedBones.Base(), sizeof( matrix3x4_t ) * m_CachedBones.Count() );
		}
		else
		{
			Warning( "SetupBones: invalid bone array size (%d - needs %d)\n", nMaxBones, m_CachedBones.Count() );
			return false;
		}
	}

	// Make sure that we know that we've already calculated some bone stuff this time around.
	m_iMostRecentModelBoneCounter = g_iModelBoneCounter;
	// OR in the stuff that we just calculated so that we don't calculate it again for the same model.
	m_CachedBoneFlags |= boneMask;
	return true;
}


C_BaseAnimating* C_BaseAnimating::FindFollowedEntity()
{

	C_BaseEntity *follow = GetFollowedEntity();
	if ( !follow )
		return NULL;

	if ( !follow->GetModel() )
	{
		Warning( "mod_studio: MOVETYPE_FOLLOW with no model.\n" );
		return NULL;
	}

	if ( modelinfo->GetModelType( follow->GetModel() ) != mod_studio )
	{
		Warning( "Attached %s (mod_studio) to %s (%d)\n", 
			modelinfo->GetModelName( GetModel() ), 
			modelinfo->GetModelName( follow->GetModel() ), 
			modelinfo->GetModelType( follow->GetModel() ) );
		return NULL;
	}

	return dynamic_cast< C_BaseAnimating* >( follow );
}



void C_BaseAnimating::InvalidateBoneCache()
{
	m_iMostRecentModelBoneCounter = g_iModelBoneCounter - 1;
}

// Causes an assert to happen if bones or attachments are used while this is false.
struct BoneAccess
{
	BoneAccess()
	{
		bAllowBoneAccessForNormalModels = false;
		bAllowBoneAccessForViewModels = false;
	}

	bool bAllowBoneAccessForNormalModels;
	bool bAllowBoneAccessForViewModels;
};

static CUtlVector< BoneAccess >		g_BoneAccessStack;
static BoneAccess g_BoneAcessBase;

bool C_BaseAnimating::IsBoneAccessAllowed() const
{
	if ( IsViewModel() )
		return g_BoneAcessBase.bAllowBoneAccessForViewModels;
	else
		return g_BoneAcessBase.bAllowBoneAccessForNormalModels;
}

// (static function)
void C_BaseAnimating::AllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels )
{
	Assert( g_BoneAccessStack.Count() == 0 );
	// Make sure it's empty...
	g_BoneAccessStack.RemoveAll();

	g_BoneAcessBase.bAllowBoneAccessForNormalModels = bAllowForNormalModels;
	g_BoneAcessBase.bAllowBoneAccessForViewModels   = bAllowForViewModels;
}

void C_BaseAnimating::PushAllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels )
{
	BoneAccess save = g_BoneAcessBase;
	g_BoneAccessStack.AddToTail( save );

	g_BoneAcessBase.bAllowBoneAccessForNormalModels = bAllowForNormalModels;
	g_BoneAcessBase.bAllowBoneAccessForViewModels = bAllowForViewModels;
}

void C_BaseAnimating::PopBoneAccess( void )
{
	int lastIndex = g_BoneAccessStack.Count() - 1;
	if ( lastIndex < 0 )
	{
		Assert( !"C_BaseAnimating::PopBoneAccess:  Stack is empty!!!" );
		return;
	}
	g_BoneAcessBase = g_BoneAccessStack[lastIndex ];
	g_BoneAccessStack.Remove( lastIndex );
}

// (static function)
void C_BaseAnimating::InvalidateBoneCaches()
{
	g_iModelBoneCounter++;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_BaseAnimating::DrawModel( int flags )
{
	VPROF_BUDGET( "C_BaseAnimating::DrawModel", VPROF_BUDGETGROUP_MODEL_RENDERING );
	MEASURE_TIMED_STAT( CS_DRAW_STUDIO_MODEL_TIME );
	if ( !m_bReadyToDraw )
		return 0;

	// Testing out different vehicle stuff.
	if ( GetClientVehicle() && !r_drawvehicles.GetInt() )
		return 0;

	int drawn = 0;

	if ( r_drawmrmmodels.GetInt() )
	{
		render->BeginDrawMRM();
		engineCache->EnterCriticalSection( );

		if ( !IsFollowingEntity() )
		{
			drawn = InternalDrawModel( flags );
		}
		else
		{
			// this doesn't draw unless master entity is visible and it's a studio model!!!
			C_BaseAnimating *follow = FindFollowedEntity();
			if ( follow )
			{
				// recompute master entity bone structure
				int baseDrawn = follow->DrawModel( STUDIO_FRUSTUMCULL );
				// draw entity
				// FIXME: Currently only draws if aiment is drawn.  
				// BUGBUG: Fixup bbox and do a separate cull for follow object
				if ( baseDrawn )
				{
					drawn = InternalDrawModel( STUDIO_RENDER );
				}
			}
		}

		engineCache->ExitCriticalSection( );
		render->EndDrawMRM();
	}

	// If we're visualizing our bboxes, draw them
	if ( m_bVisualizingBBox || m_bVisualizingAbsBox )
	{
		DrawBBoxVisualizations();
	}

	return drawn;
}


C_BaseAnimating::studiovisible_t C_BaseAnimating::TestVisibility( void )
{
	// Let the ragdolls cull themselves since the engine's sequence box will not 
	// properly account for physics simulation
	if ( IsRagdoll() )
	{
		Vector tmpmins, tmpmaxs;
		Vector origin = m_pRagdoll->GetRagdollOrigin();
		m_pRagdoll->GetRagdollBounds( tmpmins, tmpmaxs );
		tmpmins += origin;
		tmpmaxs += origin;
		if ( engine->CullBox( tmpmins, tmpmaxs ) )
		{
			return VIS_NOT_VISIBLE;
		}
		return VIS_IS_VISIBLE;
	}
	return VIS_USE_ENGINE;
}

ConVar vcollide_wireframe( "vcollide_wireframe", "0" );
ConVar vcollide_axes( "vcollide_axes", "0" );

//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_BaseAnimating::InternalDrawModel( int flags )
{
	VPROF( "C_BaseAnimating::InternalDrawModel" );

	if ( !GetModel() )
		return 0;

	// This should never happen, but if the server class hierarchy has bmodel entities derived from CBaseAnimating or does a
	//  SetModel with the wrong type of model, this could occur.
	if ( modelinfo->GetModelType( GetModel() ) != mod_studio )
	{
		return BaseClass::DrawModel( flags );
	}

	// Make sure hdr is valid for drawing
	if ( !GetModelPtr() )
		return 0;

	// UNDONE: With a bit of work on the model->world transform, we can probably
	// move the frustum culling into the client DLL entirely.  Then TestVisibility()
	// can just return true/false and only be called when frustumcull is set.
	if ( flags & STUDIO_FRUSTUMCULL )
	{
		switch ( TestVisibility() )
		{
		// not visible, don't draw
		case VIS_NOT_VISIBLE:
			return 0;
		
		// definitely visible, disable engine check
		case VIS_IS_VISIBLE:
			flags &= ~STUDIO_FRUSTUMCULL;
			break;
		
		default:
		case VIS_USE_ENGINE:
			break;
		}
	}

	int drawn = modelrender->DrawModel( 
		flags, 
		this,
		GetModelInstance(),
		index, 
		GetModel(),
		GetRenderOrigin(),
		GetRenderAngles(),
		m_nSequence, // only used for clipping.  Remove
		m_nSkin,
		m_nBody,
		m_nHitboxSet );

	if ( vcollide_wireframe.GetBool() )
	{
		if ( IsRagdoll() )
		{
			m_pRagdoll->DrawWireframe();
		}
		else
		{
			vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
			if ( pCollide && pCollide->solidCount == 1 )
			{
				static color32 debugColor = {0,255,255,0};
				matrix3x4_t matrix;
				AngleMatrix( GetAbsAngles(), GetAbsOrigin(), matrix );
				engine->DebugDrawPhysCollide( pCollide->solids[0], NULL, matrix, debugColor );
			}
		}
	}

	return drawn;
}


//-----------------------------------------------------------------------------
// Purpose: Called by networking code when an entity is new to the PVS or comes down with the EF_NOINTERP flag set.
//  The position history data is flushed out right after this call, so we need to store off the current data
//  in the latched fields so we try to interpolate
// Input  : *ent - 
//			full_reset - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::DoAnimationEvents( void )
{
	// don't fire events when paused
	if ( gpGlobals->frametime == 0.0f  )
		return;

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	bool watch = false; // Q_strstr( hdr->name, "rifle" ) ? true : false;

	Assert( hdr );

	float		poseparam[MAXSTUDIOPOSEPARAM];
	GetPoseParameters( poseparam );

	// build root animation
	float flEventCycle = m_flCycle;

	// add in muzzleflash effect
	if ( IsEffectActive(EF_MUZZLEFLASH) && m_Attachments.Count() > 0 )
	{
		Vector vAttachment;
		QAngle dummyAngles;
		GetAttachment( 1, vAttachment, dummyAngles );

		dlight_t *el = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + index );
		el->origin = vAttachment;
		el->radius = 100;
		el->decay = el->radius / 0.05f;
		el->die = gpGlobals->curtime + 0.05f;
		el->color.r = 255;
		el->color.g = 192;
		el->color.b = 64;
		el->color.exponent = 5;
		
		ActivateEffect( EF_MUZZLEFLASH, false );
	}

	// If we're invisible, don't process animation events.
	if ( !ShouldDraw() && !IsViewModel() )
		return;

	mstudioseqdesc_t *pseqdesc = hdr->pSeqdesc(  m_nSequence );
	mstudioevent_t *pevent = pseqdesc->pEvent( 0 );

	if (pseqdesc->numevents == 0)
		return;

	if ( watch )
	{
		Msg( "%i cycle %f\n", gpGlobals->framecount, m_flCycle );

	}
	bool resetEvents = m_nResetEventsParity != m_nPrevResetEventsParity;
	m_nPrevResetEventsParity = m_nResetEventsParity;

	if (m_nEventSequence != m_nSequence || resetEvents )
	{
		if ( watch )
		{
			Msg( "new seq %i old seq %i reset %s m_flCycle %f (time %.3f)\n",
				m_nSequence, m_nEventSequence,
				resetEvents ? "true" : "false",
				m_flCycle,
				gpGlobals->curtime);
		}

		m_nEventSequence = m_nSequence;
		flEventCycle = 0.0f;
		m_flPrevEventCycle = -0.01; // back up to get 0'th frame animations
	}

	// stalled?
	if (flEventCycle == m_flPrevEventCycle)
		return;

	if ( watch )
	{
		 Msg( "%i (seq %d cycle %.3f ) evcycle %.3f prevevcycle %.3f (time %.3f)\n", gpGlobals->tickcount, 
			 m_nSequence, m_flCycle, flEventCycle, m_flPrevEventCycle, gpGlobals->curtime );
	}

	// check for looping
	BOOL bLooped = false;
	if (flEventCycle <= m_flPrevEventCycle)
	{
		if (m_flPrevEventCycle - flEventCycle > 0.5)
		{
			bLooped = true;
		}
		else
		{
			// things have backed up, which is bad since it'll probably result in a hitch in the animation playback
			// but, don't play events again for the same time slice
			return;
		}
	}

	for (int i = 0; i < pseqdesc->numevents; i++)
	{
		// ignore all non-client-side events
		if ( pevent[i].event < 5000 )
			continue;

		// looped
		if (bLooped)
		{
			if ( (pevent[i].cycle > m_flPrevEventCycle || pevent[i].cycle <= flEventCycle) )
			{
			
				if ( watch )
				Msg( "%i FE %i Looped cycle %f, prev %f ev %f (time %.3f)\n",
					gpGlobals->tickcount,
					pevent[i].event,
					pevent[i].cycle,
					m_flPrevEventCycle,
					flEventCycle,
					gpGlobals->curtime );
				
				
				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].options );
			}
		}
		else
		{
			if ( (pevent[i].cycle > m_flPrevEventCycle && pevent[i].cycle <= flEventCycle) )
			{
				if ( watch )
				Msg( "%i FE %i Normal cycle %f, prev %f ev %f (time %.3f)\n",
					gpGlobals->tickcount,
					pevent[i].event,
					pevent[i].cycle,
					m_flPrevEventCycle,
					flEventCycle,
					gpGlobals->curtime );

				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].options );
			}
		}
	}

	m_flPrevEventCycle = flEventCycle;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *origin - 
//			*angles - 
//			event - 
//			*options - 
//			numAttachments - 
//			attachments[] - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	int entId = render->GetViewEntity();
	Vector vecForward, attachOrigin;
	QAngle attachAngles; 

	switch( event )
	{
	// Spark
	case CL_EVENT_SPARK0:
		GetAttachment( 1, attachOrigin, attachAngles );
		AngleVectors( attachAngles, &vecForward );
		g_pEffects->Sparks( attachOrigin, atoi( options ), 1, &vecForward );
		break;

	// Sound
	case CL_EVENT_SOUND:		// Client side sound
		{
			CLocalPlayerFilter filter;

			if ( m_Attachments.Count() > 0)
			{
				GetAttachment( 1, attachOrigin, attachAngles );
				EmitSound( filter, entId, options, &attachOrigin );
			}
			else
			{
				EmitSound( filter, entId, options );
			}
		}
		break;

	// Eject brass
	case CL_EVENT_EJECTBRASS1:
		if ( m_Attachments.Count() > 0 )
		{
			Vector attachOrigin;
			QAngle attachAngles; 
			
			GetAttachment( 2, attachOrigin, attachAngles );
			tempents->EjectBrass( attachOrigin, attachAngles, GetAbsAngles(), atoi( options ) );
		}
		break;

	// Generic dispatch effect hook
	case CL_EVENT_DISPATCHEFFECT0:
	case CL_EVENT_DISPATCHEFFECT1:
	case CL_EVENT_DISPATCHEFFECT2:
	case CL_EVENT_DISPATCHEFFECT3:
	case CL_EVENT_DISPATCHEFFECT4:
	case CL_EVENT_DISPATCHEFFECT5:
	case CL_EVENT_DISPATCHEFFECT6:
	case CL_EVENT_DISPATCHEFFECT7:
	case CL_EVENT_DISPATCHEFFECT8:
	case CL_EVENT_DISPATCHEFFECT9:
		{
			int iAttachment = -1;

			// First person muzzle flashes
			switch (event) 
			{
			case CL_EVENT_DISPATCHEFFECT0:
				iAttachment = 0;
				break;

			case CL_EVENT_DISPATCHEFFECT1:
				iAttachment = 1;
				break;

			case CL_EVENT_DISPATCHEFFECT2:
				iAttachment = 2;
				break;

			case CL_EVENT_DISPATCHEFFECT3:
				iAttachment = 3;
				break;

			case CL_EVENT_DISPATCHEFFECT4:
				iAttachment = 4;
				break;

			case CL_EVENT_DISPATCHEFFECT5:
				iAttachment = 5;
				break;

			case CL_EVENT_DISPATCHEFFECT6:
				iAttachment = 6;
				break;

			case CL_EVENT_DISPATCHEFFECT7:
				iAttachment = 7;
				break;

			case CL_EVENT_DISPATCHEFFECT8:
				iAttachment = 8;
				break;

			case CL_EVENT_DISPATCHEFFECT9:
				iAttachment = 9;
				break;
			}

			if ( iAttachment != -1 && m_Attachments.Count() > iAttachment )
			{
				GetAttachment( iAttachment+1, attachOrigin, attachAngles );

				// Fill out the generic data
				CEffectData data;
				data.m_vOrigin = attachOrigin;
				data.m_vAngles = attachAngles;
				AngleVectors( attachAngles, &data.m_vNormal );
				data.m_nEntIndex = entindex();

				DispatchEffect( options, data );
			}
		}
		break;

	// Old muzzleflashes
	case CL_EVENT_MUZZLEFLASH0:
	case CL_EVENT_MUZZLEFLASH1:
	case CL_EVENT_MUZZLEFLASH2:
	case CL_EVENT_MUZZLEFLASH3:
	case CL_EVENT_NPC_MUZZLEFLASH0:
	case CL_EVENT_NPC_MUZZLEFLASH1:
	case CL_EVENT_NPC_MUZZLEFLASH2:
	case CL_EVENT_NPC_MUZZLEFLASH3:
		{
			int iAttachment = -1;
			bool bFirstPerson = true;

			// First person muzzle flashes
			switch (event) 
			{
			case CL_EVENT_MUZZLEFLASH0:
				iAttachment = 0;
				break;

			case CL_EVENT_MUZZLEFLASH1:
				iAttachment = 1;
				break;

			case CL_EVENT_MUZZLEFLASH2:
				iAttachment = 2;
				break;

			case CL_EVENT_MUZZLEFLASH3:
				iAttachment = 3;
				break;

			// Third person muzzle flashes
			case CL_EVENT_NPC_MUZZLEFLASH0:
				iAttachment = 0;
				bFirstPerson = false;
				break;

			case CL_EVENT_NPC_MUZZLEFLASH1:
				iAttachment = 1;
				bFirstPerson = false;
				break;

			case CL_EVENT_NPC_MUZZLEFLASH2:
				iAttachment = 2;
				bFirstPerson = false;
				break;

			case CL_EVENT_NPC_MUZZLEFLASH3:
				iAttachment = 3;
				bFirstPerson = false;
				break;
			}

			if ( iAttachment != -1 && m_Attachments.Count() > iAttachment )
			{
				GetAttachment( iAttachment+1, attachOrigin, attachAngles );
				tempents->MuzzleFlash( attachOrigin, attachAngles, atoi( options ), entId, bFirstPerson );
			}
		}
		break;

	default:
		break;
	}
}


bool C_BaseAnimating::IsSelfAnimating()
{
	if ( m_bClientSideAnimation )
		return true;

	// Yes, we use animtime.
	int iMoveType = GetMoveType();
	if ( iMoveType != MOVETYPE_STEP && 
		  iMoveType != MOVETYPE_NONE && 
		  iMoveType != MOVETYPE_WALK &&
		  iMoveType != MOVETYPE_FLY &&
		  iMoveType != MOVETYPE_FLYGRAVITY )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called by networking code when an entity is new to the PVS or comes down with the EF_NOINTERP flag set.
//  The position history data is flushed out right after this call, so we need to store off the current data
//  in the latched fields so we try to interpolate
// Input  : *ent - 
//			full_reset - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::ResetLatched( void )
{
	// Reset the IK
	if ( m_pIk )
	{
		delete m_pIk;
		m_pIk = NULL;
	}

	BaseClass::ResetLatched();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseAnimating::Interpolate( float currentTime )
{
	// ragdolls don't need interpolation
	if ( m_pRagdoll )
		return true;

	if ( !BaseClass::Interpolate( currentTime ) )
		return false;

	if ( GetPredictable() || IsClientCreated() )
		return true;

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return 0.0f;
	}
	
	m_iv_flPoseParameter.Interpolate( this, currentTime );
	m_iv_flEncodedController.Interpolate( this, currentTime );

	if ( !m_bClientSideAnimation )
	{
		m_iv_flCycle.SetLooping( IsSequenceLooping( m_nSequence ) );
		m_iv_flCycle.Interpolate( this, currentTime );
	}
	return true;
}

//-----------------------------------------------------------------------------
// returns true if we're currently being ragdolled
//-----------------------------------------------------------------------------
bool C_BaseAnimating::IsRagdoll() const
{
	return m_pRagdoll && (m_nRenderFX == kRenderFxRagdoll);
}


//-----------------------------------------------------------------------------
// implements these so ragdolls can handle frustum culling & leaf visibility
//-----------------------------------------------------------------------------
void C_BaseAnimating::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	if ( IsRagdoll() )
	{
		m_pRagdoll->GetRagdollBounds( theMins, theMaxs );
	}
	else
	{
		// NOTE: Don't use the base class GetRenderBounds, we need to
		// take the sequence into account, and we know we're a studio model
		// at this stage.
		modelinfo->GetModelRenderBounds( GetModel(), m_nSequence, theMins, theMaxs );
	}
}


//-----------------------------------------------------------------------------
// Purpose: My physics object has been updated, react or extract data
//-----------------------------------------------------------------------------
void C_BaseAnimating::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	// FIXME: Should make sure the physics objects being passed in
	// is the ragdoll physics object, but I think it's pretty safe not to check
	if (IsRagdoll())
	{	 
		m_pRagdoll->VPhysicsUpdate( pPhysics );
		SetAbsOrigin( m_pRagdoll->GetRagdollOrigin() );
		SetAbsAngles( vec3_angle );

		Vector mins, maxs;
		m_pRagdoll->GetRagdollBounds( mins, maxs );
		SetCollisionBounds( mins, maxs );
		Relink();

		return;
	}

	BaseClass::VPhysicsUpdate( pPhysics );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	if ( m_bClientSideAnimation )
	{
		m_flOldCycle = m_flCycle;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( m_bClientSideAnimation )
	{
		m_flCycle = m_flOldCycle;
	}

	// reset prev cycle if new sequence
	if (m_nNewSequenceParity != m_nPrevNewSequenceParity)
	{
		m_iv_flCycle.Reset();
	}

	/*
	studiohdr_t *hdr = GetModelPtr();
	if (hdr && stricmp( hdr->name, "player.mdl") != 0)
	{
		Msg("PostDataUpdate : %d  %.3f : %.3f %.3f : %d:%d %s\n", 
			m_nSequence, m_flAnimTime, 
			m_flCycle, m_flPrevCycle, 
			m_nNewSequenceParity, m_nPrevNewSequenceParity, hdr->name );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bLastClientSideFrameReset = m_bClientSideFrameReset;
}

void C_BaseAnimating::BecomeRagdollOnClient()
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	if ( m_pRagdoll || m_builtRagdoll )
		return;

	m_builtRagdoll = true;

	// Store off our old mins & maxs
	m_vecPreRagdollMins = WorldAlignMins();
	m_vecPreRagdollMaxs = WorldAlignMaxs();

	matrix3x4_t preBones[MAXSTUDIOBONES];

	// Force MOVETYPE_STEP interpolation
	MoveType_t savedMovetype = GetMoveType();
	SetMoveType( MOVETYPE_STEP );

	// HACKHACK: force time to last interpolation position
	m_flPlaybackRate = 1;
	float prevanimtime = gpGlobals->curtime - 0.1f;

	Interpolate( prevanimtime );
	// Setup previous bone state to extrapolate physics velocity
	SetupBones( preBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, prevanimtime );
	// blow the cached prev bones
	InvalidateBoneCache();
	// reset absorigin/absangles
	Interpolate( gpGlobals->curtime );

	// Now do the current bone setup
	SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

	m_pRagdoll = CreateRagdoll( this, hdr, m_vecForce, m_nForceBone, preBones, m_CachedBones.Base(), gpGlobals->curtime - prevanimtime );
	// Cache off ragdoll bone positions/quaternions
	if ( m_bStoreRagdollInfo && m_pRagdoll )
	{
		matrix3x4_t parentTransform;
		AngleMatrix( GetAbsAngles(), GetAbsOrigin(), parentTransform );
		// FIXME/CHECK:  This might be too expensive to do every frame???
		SaveRagdollInfo( hdr->numbones, parentTransform, m_CachedBones.Base() );
	}
	SetMoveType( savedMovetype );

	// Now set the dieragdoll sequence to get transforms for all
	// non-simulated bones
	m_nRestoreSequence = m_nSequence;
	m_nSequence = LookupSequence( "ACT_DIERAGDOLL" );
	m_nPrevSequence = m_nSequence;
	m_flPlaybackRate = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::OnDataChanged( DataUpdateType_t updateType )
{
	// don't let server change sequences after becoming a ragdoll
	if ( m_pRagdoll && m_nSequence != m_nPrevSequence )
	{
		m_nSequence = m_nPrevSequence;
		m_flPlaybackRate = 0;
	}

	if ( !m_pRagdoll && m_nRestoreSequence != -1 )
	{
		m_nSequence = m_nRestoreSequence;
		m_nRestoreSequence = -1;
	}

	if (updateType == DATA_UPDATE_CREATED)
	{
		m_nPrevSequence = -1;
		m_nRestoreSequence = -1;
	}



	bool modelchanged = false;

	// UNDONE: The base class does this as well.  So this is kind of ugly
	// but getting a model by index is pretty cheap...
	const model_t *pModel = modelinfo->GetModel( GetModelIndex() );
	
	if ( pModel != GetModel() )
	{
		modelchanged = true;
	}

	BaseClass::OnDataChanged( updateType );

	if ( (updateType == DATA_UPDATE_CREATED) || modelchanged )
	{
		ResetLatched();
	}

	// If there's a significant change, make sure the shadow updates
	if ( modelchanged || (m_nSequence != m_nPrevSequence))
	{
		g_pClientShadowMgr->UpdateShadow( GetShadowHandle(), true );
		m_nPrevSequence = m_nSequence;
	}

	// Only need to think if animating client side
	if ( m_bClientSideAnimation )
	{
		// Check to see if we should reset our frame
		if ( m_bClientSideFrameReset != m_bLastClientSideFrameReset )
		{
			m_flCycle = 0;
		}
	}
	// build a ragdoll if necessary
	if ( m_nRenderFX == kRenderFxRagdoll && !m_builtRagdoll )
	{
		MoveToLastReceivedPosition( true );
		GetAbsOrigin();
		ResetLatched();
		BecomeRagdollOnClient();
	}

	if ( m_pRagdoll && m_nRenderFX != kRenderFxRagdoll )
	{
		ClearRagdoll();
	}

	// If ragdolling and get EF_NOINTERP, we probably were dead and are now respawning,
	//  don't do blend out of ragdoll at respawn spot.
	if ( IsEffectActive( EF_NOINTERP ) && 
		m_pRagdollInfo && 
		m_pRagdollInfo->m_bActive )
	{
		// Just turn off ragdoll blending immediately.
		m_pRagdollInfo->m_bActive = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::AddEntity( void )
{
	// Server says don't interpolate this frame, so set previous info to new info.
	if ( IsEffectActive(EF_NOINTERP) )
	{
		ResetLatched();
	}

	BaseClass::AddEntity();
}

//-----------------------------------------------------------------------------
// Purpose: Get the index of the attachment point with the specified name
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupAttachment( const char *pAttachmentName )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return -1;
	}

	// NOTE: Currently, the network uses 0 to mean "no attachment" 
	// thus the client must add one to the index of the attachment
	// UNDONE: Make the server do this too to be consistent.
	return Studio_FindAttachment( hdr, pAttachmentName ) + 1;
}

//-----------------------------------------------------------------------------
// Purpose: Get a random index of an attachment point with the specified substring in its name
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupRandomAttachment( const char *pAttachmentNameSubstring )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return -1;
	}

	// NOTE: Currently, the network uses 0 to mean "no attachment" 
	// thus the client must add one to the index of the attachment
	// UNDONE: Make the server do this too to be consistent.
	return Studio_FindRandomAttachment( hdr, pAttachmentNameSubstring ) + 1;
}

void C_BaseAnimating::UpdateClientSideAnimation()
{
	// Update client side animation
	if ( m_bClientSideAnimation )
	{
		// latch old values
		int flags = LATCH_ANIMATION_VAR;
		OnLatchInterpolatedVariables( flags );
		// move frame forward
		FrameAdvance( gpGlobals->frametime );
	}

	if ( m_pRagdoll && ( m_nRenderFX != kRenderFxRagdoll ) )
	{
		ClearRagdoll();
	}
}


void C_BaseAnimating::Simulate()
{
	DoAnimationEvents();

	BaseClass::Simulate();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *C_BaseAnimating::VPhysicsGetObject( void ) 
{ 
	if ( IsRagdoll() )
		return m_pRagdoll->GetElement( m_lastPhysicsBone );

	return NULL;
}


bool C_BaseAnimating::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	if ( ray.m_IsRay && IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ))
	{
		if (!TestHitboxes( ray, fContentsMask, tr ))
			return true;

		return tr.DidHit();
	}

	if ( !ray.m_IsRay && IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ))
	{
		// What do we do if we've got custom collisions but we're tracing a box against us?
		// Naturally, we throw up our hands, say we collided, but don't fill in trace info
		return true;
	}

	// We shouldn't get here.
	Assert(0);
	return false;
}


// UNDONE: This almost works.  The client entities have no control over their solid box
// Also they have no ability to expose FSOLID_ flags to the engine to force the accurate
// collision tests.
// Add those and the client hitboxes will be robust
bool C_BaseAnimating::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	VPROF( "C_BaseAnimating::TestCollision" );

	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( GetModel() );
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	// Use vcollide for box traces.
	if ( !ray.m_IsRay )
		return false;

	// This *has* to be true for the existing code to function correctly.
	Assert( ray.m_StartOffset == vec3_origin );

	int boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;
	studiocache_t *pcache = Studio_GetBoneCache( pStudioHdr, m_nSequence, m_flAnimTime, GetAbsAngles(), GetAbsOrigin(), boneMask );
	if ( !pcache )
	{
		matrix3x4_t bonetoworld[MAXSTUDIOBONES];

		// FIXME: Do I need to update m_CachedBoneFlags here?
		SetupBones( bonetoworld, MAXSTUDIOBONES, boneMask, gpGlobals->curtime );
		pcache = Studio_SetBoneCache( pStudioHdr, m_nSequence, m_flAnimTime, GetAbsAngles(), GetAbsOrigin(), boneMask, bonetoworld );
	}
	matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
	Studio_LinkHitboxCache( hitboxbones, pcache, pStudioHdr, set );

	if ( TraceToStudio( ray, pStudioHdr, set, hitboxbones, fContentsMask, tr ) )
	{
		mstudiobbox_t *pbox = set->pHitbox( tr.hitbox );
		mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = physprops->GetSurfaceIndex( pBone->pszSurfaceProp() );
		m_lastPhysicsBone = tr.physicsbone;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Check sequence framerate
// Input  : iSequence - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseAnimating::GetSequenceCycleRate( int iSequence )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return 0.0f;
	}

	return Studio_CPS( hdr, iSequence, m_flPoseParameter );
}

float C_BaseAnimating::GetAnimTimeInterval( void ) const
{
#define MAX_ANIMTIME_INTERVAL 0.2f

	float flInterval = min( gpGlobals->curtime - m_flAnimTime, MAX_ANIMTIME_INTERVAL );
	return flInterval;
}

//=========================================================
// StudioFrameAdvance - advance the animation frame up some interval (default 0.1) into the future
//=========================================================
void C_BaseAnimating::StudioFrameAdvance()
{
	if ( m_bClientSideAnimation )
		return;

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
		return;

	bool watch = 0;//Q_strstr( hdr->name, "objects/human_obj_powerpack_build.mdl" ) ? true : false;

	//if (!anim.prevanimtime)
	//{
		//anim.prevanimtime = m_flAnimTime = gpGlobals->curtime;
	//}

	// How long since last animtime
	float flInterval = GetAnimTimeInterval();

	if (flInterval <= 0.001)
	{
		// Msg("%s : %s : %5.3f (skip)\n", STRING(pev->classname), GetSequenceName( m_nSequence ), m_flCycle );
		return;
	}

	//anim.prevanimtime = m_flAnimTime;
	m_flCycle += flInterval * GetSequenceCycleRate( m_nSequence ) * m_flPlaybackRate;
	m_flAnimTime = gpGlobals->curtime;

	if ( watch )
	{
		Msg("%s %6.3f : %6.3f (%.3f)\n", GetClassname(), gpGlobals->curtime, m_flAnimTime, flInterval );
	}

	if (m_flCycle < 0.0 || m_flCycle >= 1.0) 
	{
		if (m_bSequenceLoops)
			m_flCycle -= (int)(m_flCycle);
		else
			m_flCycle = (m_flCycle < 0.0f) ? 0.0f : 1.0f;
		m_bSequenceFinished = true;	// just in case it wasn't caught in GetEvents
	}

	m_flGroundSpeed = GetSequenceGroundSpeed( m_nSequence );

	if ( watch )
	{
		Msg("%s : %s : %5.1f\n", GetClassname(), GetSequenceName( m_nSequence ), m_flCycle );
	}
}

float C_BaseAnimating::GetSequenceGroundSpeed( int iSequence )
{
	float t = SequenceDuration( iSequence );

	if (t > 0)
	{
		return GetSequenceMoveDist( iSequence ) / t;
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : float
//-----------------------------------------------------------------------------
float C_BaseAnimating::GetSequenceMoveDist( int iSequence )
{
	Vector				vecReturn;
	
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, m_flPoseParameter, &vecReturn );

	return vecReturn.Length();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//			*pVec - 
//
//-----------------------------------------------------------------------------
void C_BaseAnimating::GetSequenceLinearMotion( int iSequence, Vector *pVec )
{
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, m_flPoseParameter, pVec );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseAnimating::FrameAdvance( float flInterval )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
		return 0.0f;
	bool watch = false; // Q_strstr( hdr->name, "commando" ) ? true : false;

	float curtime = gpGlobals->curtime;

	if (flInterval == 0.0)
	{
		flInterval = ( curtime - m_flAnimTime );
		if (flInterval <= 0.001)
		{
			m_flAnimTime = curtime;
			return 0.0;
		}
	}
	if (! m_flAnimTime)
		flInterval = 0.0;

	float cyclerate = GetSequenceCycleRate( m_nSequence );
	float addcycle = flInterval * cyclerate * m_flPlaybackRate;
	m_flCycle += addcycle;
	//m_iv_flCycle.NoteChanged( this, LATCH_ANIMATION_VAR, gpGlobals->curtime );
	m_flAnimTime = curtime;

	if ( watch )
	{
		Msg("%i CLIENT Time: %6.3f : (Interval %f) : cycle %f rate %f add %f\n", 
			gpGlobals->tickcount,
			gpGlobals->curtime, 
			flInterval, 
			m_flCycle,
			cyclerate,
			addcycle );
	}

	if (m_flCycle < 0.0 || m_flCycle >= 1.0) 
	{
		bool seqloops = IsSequenceLooping( m_nSequence );

		if (seqloops)
		{
			m_flCycle -= (int)(m_flCycle);
		}
		else
		{
			m_flCycle = (m_flCycle < 0.0) ? 0 : 1.0f;
		}
	}

	return flInterval;
}

BEGIN_PREDICTION_DATA( C_BaseAnimating )

	DEFINE_PRED_FIELD( C_BaseAnimating, m_nSkin, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseAnimating, m_nBody, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( C_BaseAnimating, m_nHitboxSet, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseAnimating, m_flModelScale, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseAnimating, m_nSequence, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( C_BaseAnimating, m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( C_BaseAnimating, m_flCycle, FIELD_FLOAT, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
//	DEFINE_PRED_ARRAY( C_BaseAnimating, m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY_TOL( C_BaseAnimating, m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_INSENDTABLE, 0.02f ),

	DEFINE_FIELD( C_BaseAnimating, m_nPrevSequence, FIELD_INTEGER ),
	//DEFINE_FIELD( C_BaseAnimating, m_flPrevEventCycle, FIELD_FLOAT ),
	//DEFINE_FIELD( C_BaseAnimating, m_flEventCycle, FIELD_FLOAT ),
	//DEFINE_FIELD( C_BaseAnimating, m_nEventSequence, FIELD_INTEGER ),

	DEFINE_PRED_FIELD( C_BaseAnimating, m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseAnimating, m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	//DEFINE_FIELD( C_BaseAnimating, m_nPrevNewSequenceParity, FIELD_INTEGER ),
	//DEFINE_FIELD( C_BaseAnimating, m_nPrevResetEventsParity, FIELD_INTEGER ),

	// DEFINE_PRED_FIELD( C_BaseAnimating, m_vecForce, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	// DEFINE_PRED_FIELD( C_BaseAnimating, m_nForceBone, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	// DEFINE_PRED_FIELD( C_BaseAnimating, m_bClientSideAnimation, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	// DEFINE_PRED_FIELD( C_BaseAnimating, m_bClientSideFrameReset, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	
	// DEFINE_FIELD( C_BaseAnimating, m_pRagdollInfo, RagdollInfo_t ),
	// DEFINE_FIELD( C_BaseAnimating, m_CachedBones, CUtlVector < CBoneCacheEntry > ),
	// DEFINE_FIELD( C_BaseAnimating, m_pActualAttachmentAngles, FIELD_VECTOR ),
	// DEFINE_FIELD( C_BaseAnimating, m_pActualAttachmentOrigin, FIELD_VECTOR ),

	// DEFINE_FIELD( C_BaseAnimating, m_animationQueue, CUtlVector < AnimationLayer_t > ),
	// DEFINE_FIELD( C_BaseAnimating, m_pIk, CIKContext ),
	// DEFINE_FIELD( C_BaseAnimating, m_bLastClientSideFrameReset, FIELD_BOOLEAN ),
	// DEFINE_FIELD( C_BaseAnimating, hdr, studiohdr_t ),
	// DEFINE_FIELD( C_BaseAnimating, m_pRagdoll, IRagdoll ),
	// DEFINE_FIELD( C_BaseAnimating, m_lastPhysicsBone, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseAnimating, m_bStoreRagdollInfo, FIELD_BOOLEAN ),

END_PREDICTION_DATA()

// Stubs for weapon prediction
void C_BaseAnimating::ResetSequenceInfo( void )
{
	if (m_nSequence == -1)
	{
		// This shouldn't happen.  Setting m_nSequence blindly is a horrible coding practice.
		m_nSequence = 0;
	}

	m_flGroundSpeed = GetSequenceGroundSpeed( m_nSequence );
	m_bSequenceLoops = IsSequenceLooping( m_nSequence );
	// m_flAnimTime = gpGlobals->time;
	m_flPlaybackRate = 1.0;
	m_bSequenceFinished = false;
	m_flLastEventCheck = 0;

	m_nNewSequenceParity = ( ++m_nNewSequenceParity ) & EF_PARITY_MASK;
	m_nResetEventsParity = ( ++m_nResetEventsParity ) & EF_PARITY_MASK;
}

//=========================================================
//=========================================================

int C_BaseAnimating::GetSequenceFlags( int iSequence )
{
	return ::GetSequenceFlags( GetModelPtr(), iSequence );
}

bool C_BaseAnimating::IsSequenceLooping( int iSequence )
{
	return (GetSequenceFlags( iSequence ) & STUDIO_LOOPING) != 0;
}

float C_BaseAnimating::SequenceDuration( int iSequence )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return 0.1f;
	}

	studiohdr_t* pstudiohdr = hdr;
	if (iSequence >= pstudiohdr->numseq || iSequence < 0 )
	{
		DevWarning( 2, "C_BaseAnimating::SequenceDuration( %d ) out of range\n", iSequence );
		return 0.1;
	}

	return Studio_Duration( pstudiohdr, iSequence, m_flPoseParameter );

}

int C_BaseAnimating::FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return -1;
	}

	if (piDir == NULL)
	{
		int iDir = 1;
		int sequence = ::FindTransitionSequence( hdr, iCurrentSequence, iGoalSequence, &iDir );
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return ::FindTransitionSequence( hdr, iCurrentSequence, iGoalSequence, piDir );

}

void C_BaseAnimating::SetBodygroup( int iGroup, int iValue )
{
	Assert( GetModelPtr() );

	::SetBodygroup( GetModelPtr( ), m_nBody, iGroup, iValue );
}

int C_BaseAnimating::GetBodygroup( int iGroup )
{
	Assert( GetModelPtr() );

	return ::GetBodygroup( GetModelPtr( ), m_nBody, iGroup );
}

const char *C_BaseAnimating::GetBodygroupName( int iGroup )
{
	Assert( GetModelPtr() );

	return ::GetBodygroupName( GetModelPtr( ), iGroup );
}

int C_BaseAnimating::FindBodygroupByName( const char *name )
{
	Assert( GetModelPtr() );

	return ::FindBodygroupByName( GetModelPtr( ), name );
}

int C_BaseAnimating::GetBodygroupCount( int iGroup )
{
	Assert( GetModelPtr() );

	return ::GetBodygroupCount( GetModelPtr( ), iGroup );
}

int C_BaseAnimating::GetNumBodyGroups( void )
{
	Assert( GetModelPtr() );

	return ::GetNumBodyGroups( GetModelPtr( ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : setnum - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetHitboxSet( int setnum )
{
#ifdef _DEBUG
	studiohdr_t *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	if (setnum > pStudioHdr->numhitboxsets)
	{
		// Warn if an bogus hitbox set is being used....
		static bool s_bWarned = false;
		if (!s_bWarned)
		{
			Warning("Using bogus hitbox set in entity %s!\n", GetClassname() );
			s_bWarned = true;
		}
		setnum = 0;
	}
#endif

	m_nHitboxSet = setnum;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *setname - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetHitboxSetByName( const char *setname )
{
	m_nHitboxSet = FindHitboxSetByName( GetModelPtr(), setname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseAnimating::GetHitboxSet( void )
{
	return m_nHitboxSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *C_BaseAnimating::GetHitboxSetName( void )
{
	return ::GetHitboxSetName( GetModelPtr(), m_nHitboxSet );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseAnimating::GetHitboxSetCount( void )
{
	return ::GetHitboxSetCount( GetModelPtr() );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : int C_BaseAnimating::SelectWeightedSequence
//-----------------------------------------------------------------------------
int C_BaseAnimating::SelectWeightedSequence ( int activity )
{
	Assert( activity != ACT_INVALID );

	return ::SelectWeightedSequence( GetModelPtr(), activity );

}

//=========================================================
//=========================================================
int C_BaseAnimating::LookupPoseParameter( const char *szName )
{
	studiohdr_t *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		return 0;
	}

	for (int i = 0; i < pstudiohdr->numposeparameters; i++)
	{
		if (stricmp( pstudiohdr->pPoseParameter( i )->pszName(), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "poseparameter %s couldn't be mapped!!!\n", szName ) );
	return -1; // Error
}

//=========================================================
//=========================================================
float C_BaseAnimating::SetPoseParameter( const char *szName, float flValue )
{
	return SetPoseParameter( LookupPoseParameter( szName ), flValue );
}

float C_BaseAnimating::SetPoseParameter( int iParameter, float flValue )
{
	studiohdr_t *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		Assert(!"C_BaseAnimating::SetPoseParameter: model missing");
		return flValue;
	}

	if (iParameter >= 0)
	{
		float flNewValue;
		flValue = Studio_SetPoseParameter( pstudiohdr, iParameter, flValue, flNewValue );
		m_flPoseParameter[ iParameter ] = flNewValue;
	}

	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *label - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupSequence( const char *label )
{
	Assert( GetModelPtr() );
	return ::LookupSequence( GetModelPtr(), label );
}

void C_BaseAnimating::Release()
{
	ClearRagdoll();
	BaseClass::Release();
}

//-----------------------------------------------------------------------------
// Purpose: Clear current ragdoll
//-----------------------------------------------------------------------------
void C_BaseAnimating::ClearRagdoll()
{
	if ( m_pRagdoll )
	{
		delete m_pRagdoll;
		m_pRagdoll = NULL;

		// If we have ragdoll mins/maxs, we've just come out of ragdoll, so restore them
		if ( m_vecPreRagdollMins != vec3_origin || m_vecPreRagdollMaxs != vec3_origin )
		{
			SetCollisionBounds( m_vecPreRagdollMins, m_vecPreRagdollMaxs );
			Relink();
		}
	}
	m_builtRagdoll = false;
}

//-----------------------------------------------------------------------------
// Purpose: Looks up an activity by name.
// Input  : label - Name of the activity, ie "ACT_IDLE".
// Output : Returns the activity ID or ACT_INVALID.
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupActivity( const char *label )
{
	Assert( GetModelPtr() );
	return ::LookupActivity( GetModelPtr(), label );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *C_BaseAnimating::GetSequenceActivityName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceActivityName( GetModelPtr(), iSequence );
}

//=========================================================
//=========================================================
float C_BaseAnimating::SetBoneController ( int iController, float flValue )
{
	Assert( GetModelPtr() );

	studiohdr_t *pmodel = (studiohdr_t*)GetModelPtr();

	Assert(iController >= 0 && iController < NUM_BONECTRLS);

	float controller = m_flEncodedController[iController];
	float retVal = Studio_SetController( pmodel, iController, flValue, controller );
	m_flEncodedController[iController] = controller;
	return retVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *C_BaseAnimating::GetSequenceName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceName( GetModelPtr(), iSequence );
}

Activity C_BaseAnimating::GetSequenceActivity( int iSequence )
{
	if( iSequence == -1 )
	{
		return ACT_INVALID;
	}

	if ( !GetModelPtr() )
		return ACT_INVALID;

	return (Activity)::GetSequenceActivity( GetModelPtr(), iSequence );
}


//-----------------------------------------------------------------------------
// Purpose: Clientside bone follower class. Used just to visualize them.
//			Bone followers WON'T be sent to the client if VISUALIZE_FOLLOWERS is
//			undefined in the server's physics_bone_followers.cpp
//-----------------------------------------------------------------------------
class C_BoneFollower : public C_BaseEntity
{
	DECLARE_CLASS( C_BoneFollower, C_BaseEntity );
	DECLARE_CLIENTCLASS();
public:
	C_BoneFollower( void )
	{
	}

	bool	ShouldDraw( void );
	int		DrawModel( int flags );

private:
	int m_modelIndex;
	int m_solidIndex;
};

IMPLEMENT_CLIENTCLASS_DT( C_BoneFollower, DT_BoneFollower, CBoneFollower )
	RecvPropInt( RECVINFO( m_modelIndex ) ),
	RecvPropInt( RECVINFO( m_solidIndex ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Returns whether object should render.
//-----------------------------------------------------------------------------
bool C_BoneFollower::ShouldDraw( void )
{
	return ( vcollide_wireframe.GetBool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BoneFollower::DrawModel( int flags )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( m_modelIndex );
	if ( pCollide )
	{
		static color32 debugColor = {0,255,255,0};
		matrix3x4_t matrix;
		AngleMatrix( GetAbsAngles(), GetAbsOrigin(), matrix );
		engine->DebugDrawPhysCollide( pCollide->solids[m_solidIndex], NULL, matrix, debugColor );
	}
	return 1;
}