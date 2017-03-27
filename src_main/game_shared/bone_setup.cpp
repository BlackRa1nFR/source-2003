
#include "tier0/dbg.h"
#include "mathlib.h"
#include "bone_setup.h"
#include <string.h>

#include "collisionutils.h"
#include "vstdlib/random.h"
#include "tier0/vprof.h"

void BuildBoneChain(
	const studiohdr_t *pStudioHdr,
	matrix3x4_t &rootxform,
	Vector pos[], 
	Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld );

//-----------------------------------------------------------------------------
// Purpose: return a sub frame rotation for a single bone
//-----------------------------------------------------------------------------
void CalcBoneQuaternion( const studiohdr_t *pStudioHdr, int frame, float s, 
						const mstudiobone_t *pbone, const mstudioanim_t *panim, Quaternion &q )
{

	int					j, k;
	Quaternion			q1, q2;
	RadianEuler			angle1(0,0,0), angle2(0,0,0);
	mstudioanimvalue_t	*panimvalue;

	if (!(panim->flags & STUDIO_ROT_ANIMATED))
	{
		q.Init( panim->u.pose.q[0], panim->u.pose.q[1], panim->u.pose.q[2], panim->u.pose.q[3] );
		return;
	}

	for (j = 0; j < 3; j++)
	{
		if (panim->u.offset[j+3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = panim->pAnimvalue( j+3 );
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}
	}

	Assert( angle1.IsValid() && angle2.IsValid() );
	if (angle1.x != angle2.x || angle1.y != angle2.y || angle1.z != angle2.z)
	{
		AngleQuaternion( angle1, q1 );
		AngleQuaternion( angle2, q2 );
		QuaternionBlend( q1, q2, s, q );
	}
	else
	{
		AngleQuaternion( angle1, q );
	}

	// align to unified bone
	if (!(panim->flags & STUDIO_DELTA) && (pbone->flags & BONE_FIXED_ALIGNMENT))
	{
		QuaternionAlign( pbone->qAlignment, q, q );
	}
}


static mstudiobonecontroller_t* FindController( const studiohdr_t *pStudioHdr, int iController)
{
	mstudiobonecontroller_t	*pbonecontroller = pStudioHdr->pBonecontroller( 0 );

	// find first controller that matches the index
	for (int i = 0; i < pStudioHdr->numbonecontrollers; i++, pbonecontroller++)
	{
		if (pbonecontroller->inputfield == iController)
			return pbonecontroller;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: return a sub frame position for a single bone
//-----------------------------------------------------------------------------
void CalcBonePosition( const studiohdr_t *pStudioHdr, int frame, float s, 
	const mstudiobone_t *pbone, const mstudioanim_t *panim, Vector &pos	)
{
	int					j, k;
	mstudioanimvalue_t	*panimvalue;

	if (!(panim->flags & STUDIO_POS_ANIMATED))
	{
		pos.Init( panim->u.pose.pos[0], panim->u.pose.pos[1], panim->u.pose.pos[2] );
		return;
	}

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->u.offset[j] != 0)
		{
			panimvalue = panim->pAnimvalue( j );
			/*
			if (i == 0 && j == 0)
				Con_DPrintf("%d  %d:%d  %f\n", frame, panimvalue->num.valid, panimvalue->num.total, s );
			*/
			
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
  				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0 - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void CalcRotations( const studiohdr_t *pStudioHdr,	Vector *pos, Quaternion *q, 
	const mstudioseqdesc_t *pseqdesc,
	const mstudioanimdesc_t *panimdesc, float cycle, int boneMask )
{
	int					i;
	mstudiobone_t *pbone = pStudioHdr->pBone( 0 );

	int					iFrame;
	float				s;

	float fFrame = cycle * (panimdesc->numframes - 1);

	iFrame = (int)fFrame;
	s = (fFrame - iFrame);

	mstudioanim_t *panim = panimdesc->pAnim( 0 );

	for (i = 0; i < pStudioHdr->numbones; i++, pbone++, panim++) 
	{
		if (pseqdesc->weight(i) > 0 && (pbone->flags & boneMask))
		{
			CalcBoneQuaternion( pStudioHdr, iFrame, s, pbone, panim, q[i] );
			CalcBonePosition  ( pStudioHdr, iFrame, s, pbone, panim, pos[i] );
		}
	}
}

// qt = ( s * p ) * q
void QuaternionSM( float s, const Quaternion &p, const Quaternion &q, Quaternion &qt )
{
	Quaternion		p1, q1;

	QuaternionScale( p, s, p1 );
	QuaternionMult( p1, q, q1 );
	QuaternionNormalize( q1 );
	qt[0] = q1[0];
	qt[1] = q1[1];
	qt[2] = q1[2];
	qt[3] = q1[3];
}

// qt = p * ( s * q )
void QuaternionMA( const Quaternion &p, float s, const Quaternion &q, Quaternion &qt )
{
	Quaternion		p1, q1;

	QuaternionScale( q, s, q1 );
	QuaternionMult( p, q1, p1 );
	QuaternionNormalize( p1 );
	qt[0] = p1[0];
	qt[1] = p1[1];
	qt[2] = p1[2];
	qt[3] = p1[3];
}


// qt = p * ( s * q )
void QuaternionAccumulate( const Quaternion &p, float s, const Quaternion &q, Quaternion &qt )
{
	QuaternionAlign( p, q, qt );

	qt[0] = p[0] + s * qt[0];
	qt[1] = p[1] + s * qt[1];
	qt[2] = p[2] + s * qt[2];
	qt[3] = p[3] + s * qt[3];
}

//-----------------------------------------------------------------------------
// Purpose: blend together q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------
void SlerpBones( 
	const studiohdr_t *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	const mstudioseqdesc_t *pseqdesc, 
	const Quaternion q2[MAXSTUDIOBONES], 
	const Vector pos2[MAXSTUDIOBONES], 
	float s,
	int boneMask )
{
	int			i;
	Quaternion		q3, q4;
	float		s1, s2;

	if (s <= 0.0f) 
	{
		return;
	}
	else if (s > 1.0f)
	{
		s = 1.0f;		
	}

	if (pseqdesc->flags & STUDIO_DELTA)
	{
		for (i = 0; i < pStudioHdr->numbones; i++)
		{
			// skip unused bones
			if (!(pStudioHdr->pBone(i)->flags & boneMask))
			{
				continue;
			}

			s2 = s * pseqdesc->weight( i );	// blend in based on this bones weight
			if (s2 > 0.0)
			{
				if (pseqdesc->flags & STUDIO_POST)
				{
					QuaternionMA( q1[i], s2, q2[i], q1[i] );

					// FIXME: are these correct?
					pos1[i][0] = pos1[i][0] + pos2[i][0] * s2;
					pos1[i][1] = pos1[i][1] + pos2[i][1] * s2;
					pos1[i][2] = pos1[i][2] + pos2[i][2] * s2;
				}
				else
				{
					QuaternionSM( s2, q2[i], q1[i], q1[i] );

					// FIXME: are these correct?
					pos1[i][0] = pos1[i][0] + pos2[i][0] * s2;
					pos1[i][1] = pos1[i][1] + pos2[i][1] * s2;
					pos1[i][2] = pos1[i][2] + pos2[i][2] * s2;
				}
			}
		}
	}
	else
	{
		for (i = 0; i < pStudioHdr->numbones; i++)
		{
			// skip unused bones
			if (!(pStudioHdr->pBone(i)->flags & boneMask))
			{
				continue;
			}

			s2 = s * pseqdesc->weight( i );	// blend in based on this animations weights
			if (s2 > 0.0)
			{
				s1 = 1.0 - s2;

				if (pStudioHdr->pBone(i)->flags & BONE_FIXED_ALIGNMENT)
				{
					QuaternionSlerpNoAlign( q2[i], q1[i], s1, q3 );
				}
				else
				{
					QuaternionSlerp( q2[i], q1[i], s1, q3 );
				}
				q1[i][0] = q3[0];
				q1[i][1] = q3[1];
				q1[i][2] = q3[2];
				q1[i][3] = q3[3];
				pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s2;
				pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s2;
				pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s2;
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: blend together q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------
void BlendBones( 
	const studiohdr_t *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	mstudioseqdesc_t *pseqdesc,
	const Quaternion q2[MAXSTUDIOBONES], 
	const Vector pos2[MAXSTUDIOBONES], 
	float s,
	int boneMask )
{
	int			i;
	Quaternion		q3;

	if (s <= 0)
	{
		return;
	}
	else if (s >= 1.0)
	{
		for (i = 0; i < pStudioHdr->numbones; i++)
		{
			// skip unused bones
			if (!(pStudioHdr->pBone(i)->flags & boneMask))
			{
				continue;
			}

			if (pseqdesc->weight( i ) > 0.0)
			{
				q1[i] = q2[i];
				pos1[i] = pos2[i];
			}
		}
		return;
	}

	float s2 = s;
	float s1 = 1.0 - s2;

	for (i = 0; i < pStudioHdr->numbones; i++)
	{
		// skip unused bones
		if (!(pStudioHdr->pBone(i)->flags & boneMask))
		{
			continue;
		}

		if (pseqdesc->weight( i ) > 0.0)
		{
			if (pStudioHdr->pBone(i)->flags & BONE_FIXED_ALIGNMENT)
			{
				QuaternionBlendNoAlign( q2[i], q1[i], s1, q3 );
			}
			else
			{
				QuaternionBlend( q2[i], q1[i], s1, q3 );
			}
			q1[i][0] = q3[0];
			q1[i][1] = q3[1];
			q1[i][2] = q3[2];
			q1[i][3] = q3[3];
			pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s2;
			pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s2;
			pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s2;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: resolve a global pose parameter to the specific setting for this sequence
//-----------------------------------------------------------------------------
void Studio_LocalPoseParameter( const studiohdr_t *pStudioHdr, const float poseParameter[], const mstudioseqdesc_t *pSeqDesc, int iLocalPose, float &flSetting, int &index )
{
	if (pSeqDesc->paramindex[iLocalPose] == -1)
	{
		flSetting = 0;
		index = 0;
		return;
	}

	mstudioposeparamdesc_t *pPose = pStudioHdr->pPoseParameter( pSeqDesc->paramindex[iLocalPose] );

	float flValue = poseParameter[pSeqDesc->paramindex[iLocalPose]];

	if (pPose->loop)
	{
		float wrap = (pPose->start + pPose->end) / 2.0 + pPose->loop / 2.0;
		float shift = pPose->loop - wrap;

		flValue = flValue - pPose->loop * floor((flValue + shift) / pPose->loop);
	}

	if (pSeqDesc->posekeyindex == 0)
	{
		float flLocalStart	= (pSeqDesc->paramstart[iLocalPose] - pPose->start) / (pPose->end - pPose->start);
		float flLocalEnd	= (pSeqDesc->paramend[iLocalPose] - pPose->start) / (pPose->end - pPose->start);

		// convert into local range
		flSetting = (flValue - flLocalStart) / (flLocalEnd - flLocalStart);

		// clamp.  This shouldn't ever need to happen if it's looping.
		if (flSetting < 0)
			flSetting = 0;
		if (flSetting > 1)
			flSetting = 1;

		index = 0;
		if (pSeqDesc->groupsize[iLocalPose] > 2 )
		{
			// estimate index
			index = (int)(flSetting * (pSeqDesc->groupsize[iLocalPose] - 1));
			if (index == pSeqDesc->groupsize[iLocalPose] - 1) index = pSeqDesc->groupsize[iLocalPose] - 2;
			flSetting = flSetting * (pSeqDesc->groupsize[iLocalPose] - 1) - index;
		}
	}
	else
	{
		flValue = flValue * (pPose->end - pPose->start) + pPose->start;
		index = 0;

		while (1)
		{
			flSetting = (flValue - pSeqDesc->poseKey( iLocalPose, index )) / (pSeqDesc->poseKey( iLocalPose, index + 1 ) - pSeqDesc->poseKey( iLocalPose, index ));
			if (index > 0 && flSetting < 0.0)
			{
				index--;
				continue;
			}
			else if (index < pSeqDesc->groupsize[iLocalPose] - 2 && flSetting > 1.0)
			{
				index++;
				continue;
			}
			break;
		}

		// clamp.
		if (flSetting < 0.0f)
			flSetting = 0.0f;
		if (flSetting > 1.0f)
			flSetting = 1.0f;
	}

	return;
}

void Studio_CalcBoneToBoneTransform( const studiohdr_t *pStudioHdr, int inputBoneIndex, int outputBoneIndex, matrix3x4_t& matrixOut )
{
	mstudiobone_t *pbone = pStudioHdr->pBone( inputBoneIndex );

	matrix3x4_t inputToPose;
	MatrixInvert( pbone->poseToBone, inputToPose );
	ConcatTransforms( pStudioHdr->pBone( outputBoneIndex )->poseToBone, inputToPose, matrixOut );
}

//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//-----------------------------------------------------------------------------
void InitPose(
	const studiohdr_t *pStudioHdr,
	Vector pos[], 
	Quaternion q[] 
	)
{
	for (int i = 0; i < pStudioHdr->numbones; i++)
	{
		mstudiobone_t *pbone = pStudioHdr->pBone( i );

		pos[i] = Vector( pbone->value[0], pbone->value[1], pbone->value[2] );

		AssertMsg( (STUDIO_VERSION == 35) || (STUDIO_VERSION == 36), "Remove the code after this line" );
		// FIXME!!!
		if ( pbone->quat.w == 0.0)
		{
			AngleQuaternion( RadianEuler( pbone->value[3], pbone->value[4], pbone->value[5] ), q[i] );
			QuaternionAlign( pbone->qAlignment, q[i], q[i] );
		}
		else
		{
			q[i] = pbone->quat;
		}
	}
}
	
	
//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//-----------------------------------------------------------------------------
bool CalcPoseSingle(
	const studiohdr_t *pStudioHdr,
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask
	)
{
	ASSERT_NO_REENTRY();
	
	mstudioseqdesc_t	*pseqdesc;
	static Vector		pos2[MAXSTUDIOBONES];
	static Quaternion	q2[MAXSTUDIOBONES];
	static Vector		pos3[MAXSTUDIOBONES];
	static Quaternion	q3[MAXSTUDIOBONES];

	if (sequence >= pStudioHdr->numseq) 
	{
		sequence = 0;
	}

	pseqdesc = pStudioHdr->pSeqdesc( sequence );

	int i0 = 0, i1 = 0;
	float s0 = 0, s1 = 0;
	mstudioanimdesc_t		*panim0;
	mstudioanimdesc_t		*panim1;

	Studio_LocalPoseParameter( pStudioHdr, poseParameter, pseqdesc, 0, s0, i0 );
	Studio_LocalPoseParameter( pStudioHdr, poseParameter, pseqdesc, 1, s1, i1 );

	if (cycle < 0 || cycle >= 1)
	{
		if (pseqdesc->flags & STUDIO_LOOPING)
		{
			cycle = cycle - (int)cycle;
			if (cycle < 0) cycle += 1;
		}
		else
		{
			cycle = max( 0.0, min( cycle, 0.9999 ) );
		}
	}

	if (pseqdesc->groupsize[1] == 1)
	{
		if (pseqdesc->groupsize[0] == 1)
		{
			panim0 = pStudioHdr->pAnimdesc(pseqdesc->anim[0][0]);
			CalcRotations( pStudioHdr, pos, q, pseqdesc, panim0, cycle, boneMask);
		}
		else
		{
			panim0 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0  ][i1  ]);
			panim1 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0+1][i1  ]);

			// remove "zero" positional blends
			if ((panim0->flags & STUDIO_ALLZEROS) && (s0 < 0.001))
			{
				return false;
			}
			if ((panim1->flags & STUDIO_ALLZEROS) && (s0 > 0.999))
			{
				return false;
			}

			CalcRotations( pStudioHdr, pos,  q,  pseqdesc, panim0, cycle, boneMask );
			CalcRotations( pStudioHdr, pos2, q2, pseqdesc, panim1, cycle, boneMask );

			BlendBones( pStudioHdr, q, pos, pseqdesc, q2, pos2, s0, boneMask );
		}
	}
	else
	{
		if (pseqdesc->groupsize[0] == 1)
		{
			panim0 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0  ][i1  ]);
			panim1 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0  ][i1+1]);

			CalcRotations( pStudioHdr, pos,  q,  pseqdesc, panim0, cycle, boneMask );
			CalcRotations( pStudioHdr, pos2, q2, pseqdesc, panim1, cycle, boneMask );

			BlendBones( pStudioHdr, q, pos, pseqdesc, q2, pos2, s0, boneMask );
		}
		else
		{
			panim0 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0  ][i1]);
			panim1 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0+1][i1]);

			CalcRotations( pStudioHdr, pos,  q,  pseqdesc, panim0, cycle, boneMask );
			CalcRotations( pStudioHdr, pos2, q2, pseqdesc, panim1, cycle, boneMask );

			BlendBones( pStudioHdr, q, pos, pseqdesc, q2, pos2, s0, boneMask );
			
			panim0 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0  ][i1+1]);
			panim1 = pStudioHdr->pAnimdesc(pseqdesc->anim[i0+1][i1+1]);

			CalcRotations( pStudioHdr, pos2, q2, pseqdesc, panim0, cycle, boneMask );
			CalcRotations( pStudioHdr, pos3, q3, pseqdesc, panim1, cycle, boneMask );

			BlendBones( pStudioHdr, q2, pos2, pseqdesc, q3, pos3, s0, boneMask );
			BlendBones( pStudioHdr, q, pos, pseqdesc, q2, pos2, s1, boneMask );
		}
	}

	return true;
}




//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void AddSequenceLayers(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flWeight
	)
{
	mstudioseqdesc_t	*pseqdesc = pStudioHdr->pSeqdesc( sequence );
	
	for (int i = 0; i < pseqdesc->numautolayers; i++)
	{
		mstudioautolayer_t *pLayer = pseqdesc->pAutolayer( i );

		float layerCycle = cycle;
		float layerWeight = flWeight;

		if (pLayer->start != pLayer->end)
		{
			float s = 1.0;

			if (cycle < pLayer->start)
				continue;
			if (cycle >= pLayer->end)
				continue;

			if (pLayer->flags & STUDIO_WEIGHT)
			{
				if (cycle < pLayer->peak && pLayer->start != pLayer->peak)
				{
					s = (cycle - pLayer->start) / (pLayer->peak - pLayer->start);
				}
				else if (cycle > pLayer->tail && pLayer->end != pLayer->tail)
				{
					s = (pLayer->end - cycle) / (pLayer->end - pLayer->tail);
				}

				if (pLayer->flags & STUDIO_SPLINE)
				{
					s = 3 * s * s - 2 * s * s * s;
				}

				layerWeight = flWeight * s;
			}
			else
			{
				layerWeight = flWeight;
			}

			layerCycle = (cycle - pLayer->start) / (pLayer->end - pLayer->start);
		}

		AccumulatePose( pStudioHdr, pIKContext, pos, q, pLayer->iSequence, layerCycle, poseParameter, boneMask, layerWeight );
	}
}


//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void CalcPose(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flWeight
	)
{
	mstudioseqdesc_t	*pseqdesc = pStudioHdr->pSeqdesc( sequence );

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	if ( pIKContext )
	{
		pIKContext->AddDependencies( sequence, cycle, poseParameter, flWeight );
	}
	
	CalcPoseSingle( pStudioHdr, pos, q, sequence, cycle, poseParameter, boneMask );

	// add any IK locks to prevent numautolayers from moving extremities 
	CIKContext seq_ik;
	if (pseqdesc->numiklocks)
	{
		seq_ik.Init( pStudioHdr, QAngle( 0, 0, 0 ), Vector( 0, 0, 0 ), 0.0 ); // local space relative so absolute position doesn't mater
		seq_ik.AddSequenceLocks( pseqdesc, pos, q );
	}

	AddSequenceLayers( 	pStudioHdr, pIKContext, pos, q, sequence, cycle, poseParameter, boneMask, flWeight );

	if (pseqdesc->numiklocks)
	{
		seq_ik.SolveSequenceLocks( pseqdesc, pos, q );
	}
}


//-----------------------------------------------------------------------------
// Purpose: accumulate a pose for a single sequence on top of existing animation
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void AccumulatePose(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flWeight
	)
{
	Vector		pos2[MAXSTUDIOBONES];
	Quaternion	q2[MAXSTUDIOBONES];

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	mstudioseqdesc_t	*pseqdesc = pStudioHdr->pSeqdesc( sequence );

	if (!CalcPoseSingle( pStudioHdr, pos2, q2, sequence, cycle, poseParameter, boneMask ))
	{
		return;
	}

	// add any IK locks to prevent extremities from moving
	CIKContext seq_ik;
	if (pseqdesc->numiklocks)
	{
		seq_ik.Init( pStudioHdr, QAngle( 0, 0, 0 ), Vector( 0, 0, 0 ), 0.0 );  // local space relative so absolute position doesn't mater
		seq_ik.AddSequenceLocks( pseqdesc, pos, q );
	}

	if ( pIKContext )
	{
		pIKContext->AddDependencies( sequence, cycle, poseParameter, flWeight );
	}

	SlerpBones( pStudioHdr, q, pos, pseqdesc, q2, pos2, flWeight, boneMask );

	AddSequenceLayers( 	pStudioHdr, pIKContext, pos, q, sequence, cycle, poseParameter, boneMask, flWeight );

	if (pseqdesc->numiklocks)
	{
		seq_ik.SolveSequenceLocks( pseqdesc, pos, q );
	}
}


//-----------------------------------------------------------------------------
// Purpose: blend together q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------
void CalcBoneAdj(
	const studiohdr_t *pStudioHdr,
	Vector pos[], 
	Quaternion q[], 
	const float controllers[],
	int boneMask
	)
{
	int					i, j, k;
	float				value;
	mstudiobonecontroller_t *pbonecontroller;
	Vector p0;
	RadianEuler a0;
	Quaternion q0;
	
	pbonecontroller = pStudioHdr->pBonecontroller( 0 );

	for (j = 0; j < pStudioHdr->numbonecontrollers; j++)
	{
		k = pbonecontroller[j].bone;

		if (pStudioHdr->pBone( k )->flags & boneMask)
		{
			i = pbonecontroller[j].inputfield;
			value = controllers[i];
			if (value < 0) value = 0;
			if (value > 1.0) value = 1.0;
			value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;

			switch(pbonecontroller[j].type & STUDIO_TYPES)
			{
			case STUDIO_XR: 
				a0.Init( value * (M_PI / 180.0), 0, 0 ); 
				AngleQuaternion( a0, q0 );
				QuaternionSM( 1.0, q0, q[k], q[k] );
				break;
			case STUDIO_YR: 
				a0.Init( 0, value * (M_PI / 180.0), 0 ); 
				AngleQuaternion( a0, q0 );
				QuaternionSM( 1.0, q0, q[k], q[k] );
				break;
			case STUDIO_ZR: 
				a0.Init( 0, 0, value * (M_PI / 180.0) ); 
				AngleQuaternion( a0, q0 );
				QuaternionSM( 1.0, q0, q[k], q[k] );
				break;
			case STUDIO_X:	
				pos[k].x += value;
				break;
			case STUDIO_Y:
				pos[k].y += value;
				break;
			case STUDIO_Z:
				pos[k].z += value;
				break;
			}
		}
	}
}


void CalcBoneDerivatives( Vector &velocity, AngularImpulse &angVel, const matrix3x4_t &prev, const matrix3x4_t &current, float dt )
{
	float scale = 1.0;
	if ( dt > 0 )
	{
		scale = 1.0 / dt;
	}
	
	Vector endPosition, startPosition, deltaAxis;
	QAngle endAngles, startAngles;
	float deltaAngle;

	MatrixAngles( prev, startAngles, startPosition );
	MatrixAngles( current, endAngles, endPosition );

	velocity = (endPosition - startPosition) * scale;
	RotationDeltaAxisAngle( startAngles, endAngles, deltaAxis, deltaAngle );
	angVel = deltaAxis * (deltaAngle * scale);
}

void CalcBoneVelocityFromDerivative( const QAngle &vecAngles, Vector &velocity, AngularImpulse &angVel, const matrix3x4_t &current )
{
	Vector vecLocalVelocity;
	AngularImpulse LocalAngVel;
	Quaternion q;
	float angle;
	MatrixAngles( current, q, vecLocalVelocity );
	QuaternionAxisAngle( q, LocalAngVel, angle );
	LocalAngVel *= angle;

	matrix3x4_t matAngles;
	AngleMatrix( vecAngles, matAngles );
	VectorTransform( vecLocalVelocity, matAngles, velocity );
	VectorTransform( LocalAngVel, matAngles, angVel );
}

class ik
{
public:
//-------- SOLVE TWO LINK INVERSE KINEMATICS -------------
// Author: Ken Perlin
//
// Given a two link joint from [0,0,0] to end effector position P,
// let link lengths be a and b, and let norm |P| = c.  Clearly a+b >= c.
//
// Problem: find a "knee" position Q such that |Q| = a and |P-Q| = b.
//
// In the case of a point on the x axis R = [c,0,0], there is a
// closed form solution S = [d,e,0], where |S| = a and |R-S| = b:
//
//    d2+e2 = a2                  -- because |S| = a
//    (c-d)2+e2 = b2              -- because |R-S| = b
//
//    c2-2cd+d2+e2 = b2           -- combine the two equations
//    c2-2cd = b2 - a2
//    c-2d = (b2-a2)/c
//    d - c/2 = (a2-b2)/c / 2
//
//    d = (c + (a2-b2/c) / 2      -- to solve for d and e.
//    e = sqrt(a2-d2)

   static float findD(float a, float b, float c) {
      return max(0, min(a, (c + (a*a-b*b)/c) / 2));
   }
   static float findE(float a, float d) { return sqrt(a*a-d*d); } 

// This leads to a solution to the more general problem:
//
//   (1) R = Mfwd(P)         -- rotate P onto the x axis
//   (2) Solve for S
//   (3) Q = Minv(S)         -- rotate back again

   static float Mfwd[3][3];
   static float Minv[3][3];

   static bool solve(float A, float B, float const P[], float const D[], float Q[]) {
      float R[3];
      defineM(P,D);
      rot(Minv,P,R);
      float d = findD(A,B,length(R));
      float e = findE(A,d);
      float S[3] = {d,e,0};
      rot(Mfwd,S,Q);
      return d > 0 && d < A;
   }

// If "knee" position Q needs to be as close as possible to some point D,
// then choose M such that M(D) is in the y>0 half of the z=0 plane.
//
// Given that constraint, define the forward and inverse of M as follows:

   static void defineM(float const P[], float const D[]) {
      float *X = Minv[0], *Y = Minv[1], *Z = Minv[2];

// Minv defines a coordinate system whose x axis contains P, so X = unit(P).
	  int i;
      for (i = 0 ; i < 3 ; i++)
         X[i] = P[i];
      normalize(X);

// Its y axis is perpendicular to P, so Y = unit( E - X(E·X) ).

      float dDOTx = dot(D,X);
      for (i = 0 ; i < 3 ; i++)
         Y[i] = D[i] - dDOTx * X[i];
      normalize(Y);

// Its z axis is perpendicular to both X and Y, so Z = X×Y.

      cross(X,Y,Z);

// Mfwd = (Minv)T, since transposing inverts a rotation matrix.

      for (i = 0 ; i < 3 ; i++) {
         Mfwd[i][0] = Minv[0][i];
         Mfwd[i][1] = Minv[1][i];
         Mfwd[i][2] = Minv[2][i];
      }
   }

//------------ GENERAL VECTOR MATH SUPPORT -----------

   static float dot(float const a[], float const b[]) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

   static float length(float const v[]) { return sqrt( dot(v,v) ); }

   static void normalize(float v[]) {
      float norm = length(v);
      for (int i = 0 ; i < 3 ; i++)
         v[i] /= norm;
   }

   static void cross(float const a[], float const b[], float c[]) {
      c[0] = a[1] * b[2] - a[2] * b[1];
      c[1] = a[2] * b[0] - a[0] * b[2];
      c[2] = a[0] * b[1] - a[1] * b[0];
   }

   static void rot(float const M[3][3], float const src[], float dst[]) {
      for (int i = 0 ; i < 3 ; i++)
         dst[i] = dot(M[i],src);
   }
};

float ik::Mfwd[3][3];
float ik::Minv[3][3];



//-----------------------------------------------------------------------------
// Purpose: ugly hacky hlmv debugging code
//-----------------------------------------------------------------------------
#if 1
void drawLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration)
{
	 // drawLine( origin, dest, r, g, b, noDepthTest, duration );
}
#else
extern void drawLine( const Vector &p1, const Vector &p2, int r = 0, int g = 0, int b = 1 );
void drawLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration)
{
	drawLine( origin, dest, r, g, b );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: for a 2 bone chain, find the IK solution and reset the matrices
//-----------------------------------------------------------------------------
bool Studio_SolveIK( mstudioikchain_t *pikchain, Vector &targetFoot, matrix3x4_t *pBoneToWorld )
{
	return Studio_SolveIK( pikchain->pLink( 0 )->bone, pikchain->pLink( 1 )->bone, pikchain->pLink( 2 )->bone, targetFoot, pBoneToWorld );
}


bool Studio_SolveIK( int iThigh, int iKnee, int iFoot, Vector &targetFoot, matrix3x4_t *pBoneToWorld )
{
	Vector worldFoot, worldKnee, worldThigh;

	MatrixGetColumn( pBoneToWorld[ iThigh ], 3, worldThigh );
	MatrixGetColumn( pBoneToWorld[ iKnee ], 3, worldKnee );
	MatrixGetColumn( pBoneToWorld[ iFoot ], 3, worldFoot );

	drawLine( worldThigh, worldKnee, 0, 0, 255, true, 0 );
	drawLine( worldKnee, worldFoot, 0, 0, 255, true, 0 );

	Vector ikFoot, ikTargetKnee, ikKnee;

	ikFoot = targetFoot - worldThigh;
	ikKnee = worldKnee - worldThigh;

	float l1 = (worldKnee-worldThigh).Length();
	float l2 = (worldFoot-worldKnee).Length();
	float l3 = (worldFoot-worldThigh).Length();

	Vector ikHalf = (worldFoot-worldThigh) * (l1 / l3);

	// FIXME: what to do when the knee completely straight?
	Vector ikKneeDir = ikKnee - ikHalf;
	VectorNormalize( ikKneeDir );
	ikTargetKnee = ikKnee + ikKneeDir * l1;

	drawLine( worldKnee, worldThigh + ikTargetKnee, 0, 255, 0, true, 0);

	// leg too straight to figure out knee?
	if (l3 > (l1 + l2) * 0.9998)
	{
		return false;
	}

	// too far away? (0.9998 is about 1 degree)
	if (ikFoot.Length() > (l1 + l2) * 0.9998)
	{
		VectorNormalize( ikFoot );
		VectorScale( ikFoot, (l1 + l2) * 0.9998, ikFoot );
	}

	// too close?
	if (ikFoot.Length() < fabs(l1 - l2) * 1.01)
	{
		VectorNormalize( ikFoot );
		VectorScale( ikFoot, fabs(l1 - l2) * 1.01, ikFoot );
	}

	if (ik::solve( l1, l2, ikFoot.Base(), ikTargetKnee.Base(), ikKnee.Base() ))
	{
		matrix3x4_t& mWorldThigh = pBoneToWorld[ iThigh ];
		matrix3x4_t& mWorldKnee = pBoneToWorld[ iKnee ];
		matrix3x4_t& mWorldFoot = pBoneToWorld[ iFoot ];

		drawLine( worldThigh, ikKnee + worldThigh, 255, 0, 0, true, 0 );
		drawLine( ikKnee + worldThigh, ikFoot + worldThigh, 255, 0, 0, true,0 );

		Vector tmp1, tmp2, tmp3;

		// build transformation matrix for thigh
		tmp1 = ikKnee;
		VectorNormalize( tmp1 );
		MatrixSetColumn( tmp1, 0, mWorldThigh );

		MatrixGetColumn( mWorldThigh, 2, tmp3 );
		tmp2 = tmp3.Cross( tmp1 );
		VectorNormalize( tmp2 );
		MatrixSetColumn( tmp2, 1, mWorldThigh );

		tmp3 = tmp1.Cross( tmp2 );
		MatrixSetColumn( tmp3, 2, mWorldThigh );



		tmp1 = ikFoot - ikKnee;
		VectorNormalize(tmp1);
		MatrixSetColumn( tmp1, 0, mWorldKnee );

		MatrixGetColumn( mWorldKnee, 2, tmp3 );
		tmp2 = tmp3.Cross( tmp1 );
		VectorNormalize( tmp2 );
		MatrixSetColumn( tmp2, 1, mWorldKnee );

		tmp3 = tmp1.Cross( tmp2 );
		MatrixSetColumn( tmp3, 2, mWorldKnee );


		mWorldKnee[0][3] = ikKnee.x + worldThigh.x;
		mWorldKnee[1][3] = ikKnee.y + worldThigh.y;
		mWorldKnee[2][3] = ikKnee.z + worldThigh.z;

		mWorldFoot[0][3] = ikFoot.x + worldThigh.x;
		mWorldFoot[1][3] = ikFoot.y + worldThigh.y;
		mWorldFoot[2][3] = ikFoot.z + worldThigh.z;

		return true;
	}
	else
	{
		return false;
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


bool Studio_SolveIK( int iThigh, int iKnee, int iFoot, Vector &targetFoot, Vector &targetKnee, matrix3x4_t *pBoneToWorld )
{
	Vector worldFoot, worldKnee, worldThigh;

	MatrixGetColumn( pBoneToWorld[ iThigh ], 3, worldThigh );
	MatrixGetColumn( pBoneToWorld[ iKnee ], 3, worldKnee );
	MatrixGetColumn( pBoneToWorld[ iFoot ], 3, worldFoot );

	drawLine( worldThigh, worldKnee, 0, 0, 255, true, 0 );
	drawLine( worldKnee, worldFoot, 0, 0, 255, true, 0 );

	Vector ikFoot, ikTargetKnee, ikKnee;

	ikFoot = targetFoot - worldThigh;
	ikKnee = worldKnee - worldThigh;

	float l1 = (worldKnee-worldThigh).Length();
	float l2 = (worldFoot-worldKnee).Length();

	ikTargetKnee = targetKnee - worldThigh;

	// too far away? (0.9998 is about 1 degree)
	if (ikFoot.Length() > (l1 + l2) * 0.9998)
	{
		VectorNormalize( ikFoot );
		VectorScale( ikFoot, (l1 + l2) * 0.9998, ikFoot );
	}

	// too close?
	if (ikFoot.Length() < fabs(l1 - l2) * 1.01)
	{
		VectorNormalize( ikFoot );
		VectorScale( ikFoot, fabs(l1 - l2) * 1.01, ikFoot );
	}

	if (ik::solve( l1, l2, ikFoot.Base(), ikTargetKnee.Base(), ikKnee.Base() ))
	{
		matrix3x4_t& mWorldThigh = pBoneToWorld[ iThigh ];
		matrix3x4_t& mWorldKnee = pBoneToWorld[ iKnee ];
		matrix3x4_t& mWorldFoot = pBoneToWorld[ iFoot ];

		drawLine( worldThigh, ikKnee + worldThigh, 255, 0, 0, true, 0 );
		drawLine( ikKnee + worldThigh, ikFoot + worldThigh, 255, 0, 0, true,0 );

		Vector tmp1, tmp2, tmp3;

		// build transformation matrix for thigh
		tmp1 = ikKnee;
		VectorNormalize( tmp1 );
		MatrixSetColumn( tmp1, 0, mWorldThigh );

		MatrixGetColumn( mWorldThigh, 2, tmp3 );
		tmp2 = tmp3.Cross( tmp1 );
		VectorNormalize( tmp2 );
		MatrixSetColumn( tmp2, 1, mWorldThigh );

		tmp3 = tmp1.Cross( tmp2 );
		MatrixSetColumn( tmp3, 2, mWorldThigh );



		tmp1 = ikFoot - ikKnee;
		VectorNormalize(tmp1);
		MatrixSetColumn( tmp1, 0, mWorldKnee );

		MatrixGetColumn( mWorldKnee, 2, tmp3 );
		tmp2 = tmp3.Cross( tmp1 );
		VectorNormalize( tmp2 );
		MatrixSetColumn( tmp2, 1, mWorldKnee );

		tmp3 = tmp1.Cross( tmp2 );
		MatrixSetColumn( tmp3, 2, mWorldKnee );


		mWorldKnee[0][3] = ikKnee.x + worldThigh.x;
		mWorldKnee[1][3] = ikKnee.y + worldThigh.y;
		mWorldKnee[2][3] = ikKnee.z + worldThigh.z;

		mWorldFoot[0][3] = ikFoot.x + worldThigh.x;
		mWorldFoot[1][3] = ikFoot.y + worldThigh.y;
		mWorldFoot[2][3] = ikFoot.z + worldThigh.z;

		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

float Studio_IKRuleWeight( mstudioikrule_t &ikRule, float flCycle )
{
	if (ikRule.end > 1.0f && flCycle < ikRule.start)
	{
		flCycle = flCycle + 1.0f;
	}

	float value = 0.0f;
	if (flCycle < ikRule.start)
	{
		return 0.0f;
	}
	else if (flCycle < ikRule.peak )
	{
		value = (flCycle - ikRule.start) / (ikRule.peak - ikRule.start);
	}
	else if (flCycle < ikRule.tail )
	{
		return 1.0f;
	}
	else if (flCycle < ikRule.end )
	{
		value = 1.0f - ((flCycle - ikRule.tail) / (ikRule.end - ikRule.tail));
	}
	return 3.0f * value * value - 2.0f * value * value * value;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool Studio_IKShouldLatch( mstudioikrule_t &ikRule, float flCycle )
{
	if (ikRule.end > 1.0f && flCycle < ikRule.start)
	{
		flCycle = flCycle + 1.0f;
	}

	if (flCycle < ikRule.peak )
	{
		return false;
	}
	else if (flCycle < ikRule.end )
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void Studio_IKAnimationError( mstudioikrule_t *pRule, mstudioanimdesc_t *panim, float flCycle, Vector &pos, Quaternion &q, float &flWeight )
{
	float fraq = (panim->numframes - 1) * flCycle;
	int iFrame = (int)fraq;
	fraq = fraq - iFrame;

	if (pRule->end > 1.0f && flCycle < pRule->start)
	{
		iFrame = iFrame + panim->numframes;
	}

	flWeight = Studio_IKRuleWeight( *pRule, flCycle );

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	mstudioikerror_t *pError = pRule->pError( iFrame );
	if (fraq < 0.001)
	{
		q = pError[0].q;
		pos = pError[0].pos;
	}
	else
	{
		QuaternionBlend( pError[0].q, pError[1].q, fraq, q );
		pos = pError[0].pos * (1.0f - fraq) + pError[1].pos * fraq;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool Studio_IKSequenceError( const studiohdr_t *pStudioHdr, int iSequence, float flCycle, int iTarget, const float poseParameter[], mstudioikrule_t &ikRule )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	int i, j;

	memset( &ikRule, 0, sizeof(ikRule) );
	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );

	ikRule.start = ikRule.peak = ikRule.tail = ikRule.end = 0;

	// find overall influence
	for (i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			for (j = 0; j < panim[i]->numikrules; j++)
			{
				mstudioikrule_t *pRule = panim[i]->pIKRule( j );

				if (j /* pRule->index*/ == iTarget)
				{
					// BUGBUG: find the closest rule
					ikRule.start += pRule->start * weight[i];
					ikRule.peak += pRule->peak * weight[i];
					ikRule.tail += pRule->tail * weight[i];
					ikRule.end += pRule->end * weight[i];
					break;
				}
			}
		}
	}

	ikRule.flWeight = Studio_IKRuleWeight( ikRule, flCycle );
	if (ikRule.flWeight == 0.0f)
		return false;

	Assert( ikRule.flWeight > 0.0f );

	ikRule.pos.Init();
	ikRule.q.Init();

	// FIXME: add "latched" value
	ikRule.commit = Studio_IKShouldLatch( ikRule, flCycle );

	// find target error
	for (i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			for (j = 0; j < panim[i]->numikrules; j++)
			{
				if (j /*panim[i]->pIKRule( j )->index*/ == iTarget)
				{
					Vector pos1;
					Quaternion q1;
					float w;
					Studio_IKAnimationError( panim[i]->pIKRule( j ), panim[i], flCycle, pos1, q1, w );

					// FIXME: move this!!
					ikRule.chain = panim[i]->pIKRule( j )->chain;
					ikRule.bone = panim[i]->pIKRule( j )->bone;
					ikRule.type = panim[i]->pIKRule( j )->type;
					ikRule.slot = panim[i]->pIKRule( j )->slot;

					ikRule.pos = ikRule.pos + pos1 *  weight[i];
					QuaternionAccumulate( ikRule.q, weight[i], q1, ikRule.q );
				}
			}
		}
	}

	QuaternionNormalize( ikRule.q );
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool CIKContext::Estimate( 
	int iSequence, 
	float flCycle, 
	int iTarget, 
	const float poseParameter[], 
	float flWeight
	)
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	int i, j;

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	Studio_SeqAnims( m_pStudioHdr, iSequence, poseParameter, panim, weight );

	int slot = 0;
	Vector pos;
	Quaternion q;

	pos.Init();
	q.Init();
	float height = 0;
	float floor = 0;
	float radius = 0;

	float latched = false;

	// find overall influence
	for (i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			for (j = 0; j < panim[i]->numikrules; j++)
			{
				mstudioikrule_t *pRule = panim[i]->pIKRule( j );

				if (pRule->index == iTarget)
				{
					slot = pRule->slot;

					Vector deltaPos;
					QAngle deltaAngle;

					deltaPos.Init();

					AssertMsg( (STUDIO_VERSION == 35) || (STUDIO_VERSION == 36), "Remove the code after this line" );
					// hack in the contact point if the model hasn't been rebuilt
					float contact = pRule->contact;
					if (contact == 0.0 && pRule->peak != 0.0)
					{
						contact = pRule->peak;
					}
					AssertMsg( (STUDIO_VERSION == 35) || (STUDIO_VERSION == 36), "Remove the code before this line" );

					float flCheck = flCycle;
					if (flCheck < pRule->start)
							flCheck = flCheck + 1.0f;

					Studio_AnimMovement( panim[i], flCheck, contact, deltaPos, deltaAngle );
					pos = pos + (pRule->pos + deltaPos) * weight[i];
					QuaternionAccumulate( q, weight[i], pRule->q, q );
					height += pRule->height * weight[i];
					floor += pRule->floor * weight[i];
					radius += pRule->radius * weight[i];

					Assert( radius >= 0.0f );

					latched = Studio_IKShouldLatch( *pRule, flCycle );

					break;
				}
			}
		}
	}

	QuaternionNormalize( q );

	if (flWeight == 1.0f)
	{
		m_target[slot].local.q = q;
		m_target[slot].local.pos = pos;
		m_target[slot].est.height = height;
		m_target[slot].est.floor = floor;
		m_target[slot].est.radius = radius;
		m_target[slot].est.latched = latched;
	}
	else
	{
		QuaternionSlerp( m_target[slot].local.q, q, flWeight, m_target[slot].local.q );
		m_target[slot].local.pos = Lerp( flWeight, m_target[slot].local.pos, pos );
		m_target[slot].est.height = Lerp( flWeight, m_target[slot].est.height, height );
		m_target[slot].est.floor = Lerp( flWeight, m_target[slot].est.floor, floor );
		m_target[slot].est.radius = Lerp( flWeight, m_target[slot].est.radius, radius );
		m_target[slot].est.latched = Lerp( flWeight, m_target[slot].est.latched, latched );
	}

	m_target[slot].est.time = m_flTime;

	matrix3x4_t tmp1, tmp2;
	AngleMatrix( m_target[slot].local.q, m_target[slot].local.pos, tmp1 );
	ConcatTransforms( m_rootxform, tmp1, tmp2 );
	MatrixAngles( tmp2, m_target[slot].est.q, m_target[slot].est.pos );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


CIKContext::CIKContext()
{
	m_target.EnsureCapacity( 12 ); // FIXME: this sucks, shouldn't it be grown?
	m_flPrevTime = -1.0f;
	m_pStudioHdr = NULL;
	m_flTime = -1.0f;
}


void CIKContext::Init( const studiohdr_t *pStudioHdr, const QAngle &angles, const Vector &pos, float flTime )
{
	m_pStudioHdr = pStudioHdr;
	m_ikRule.RemoveAll(); // m_numikrules = 0;
	AngleMatrix( angles, pos, m_rootxform );
	m_flPrevTime = m_flTime;
	m_flTime = flTime;
}

void CIKContext::AddDependencies( 
	int iSequence, 
	float flCycle, 
	const float poseParameters[],
	float flWeight
	)
{
	int i;
	mstudioseqdesc_t *pseqdesc = m_pStudioHdr->pSeqdesc( iSequence );
	mstudioikrule_t ikrule;

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	// FIXME: add proper number of rules!!!
	for (i = 0; i < pseqdesc->numikrules; i++)
	{
		if (Studio_IKSequenceError( m_pStudioHdr, iSequence, flCycle, i, poseParameters, ikrule ))
		{
			// FIXME: Brutal hackery to prevent a crash
			if (m_target.Count() == 0)
			{
				m_target.SetSize(12);
				memset( m_target.Base(), 0, sizeof(m_target[0])*m_target.Count() );
			}

			ikrule.flWeight *= flWeight;
		
			m_ikRule.AddToTail( ikrule );

			// m_target[ikrule.slot].est.flWeight = flWeight; // !!!

			Estimate( iSequence, flCycle, i, poseParameters, flWeight );
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::AddAutoplayLocks( Vector pos[], Quaternion q[] )
{
	int i;
	static matrix3x4_t boneToWorld[MAXSTUDIOBONES];

	for (i = 0; i < m_pStudioHdr->numikautoplaylocks; i++)
	{
		mstudioiklock_t *plock = m_pStudioHdr->pIKAutoplayLock( i );
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( plock->chain );
		int bone = pchain->pLink( 2 )->bone;

		mstudioikrule_t ikrule;

		memset( &ikrule, 0, sizeof(ikrule) );
		ikrule.chain = plock->chain;
		ikrule.slot = i;
		ikrule.type = IK_WORLD;

		// eval current ik'd bone
		BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, bone, boneToWorld );

		MatrixAngles( boneToWorld[bone], ikrule.q, ikrule.pos );

		m_ikRule.AddToTail( ikrule );
	}
	return;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::AddSequenceLocks( mstudioseqdesc_t *pSeqDesc, Vector pos[], Quaternion q[] )
{
	int i;
	static matrix3x4_t boneToWorld[MAXSTUDIOBONES];

	for (i = 0; i < pSeqDesc->numiklocks; i++)
	{
		mstudioiklock_t *plock = pSeqDesc->pIKLock( i );
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( plock->chain );
		int bone = pchain->pLink( 2 )->bone;

		mstudioikrule_t ikrule;
		memset( &ikrule, 0, sizeof(ikrule) );

		ikrule.chain = i;
		ikrule.slot = i;
		ikrule.type = IK_WORLD;

		// eval current ik'd bone
		BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, bone, boneToWorld );

		MatrixAngles( boneToWorld[bone], ikrule.q, ikrule.pos );

		m_ikRule.AddToTail( ikrule );
	}

	return;
}

//-----------------------------------------------------------------------------
// Purpose: build boneToWorld transforms for a specific bone
//-----------------------------------------------------------------------------
void BuildBoneChain(
	const studiohdr_t *pStudioHdr,
	matrix3x4_t &rootxform,
	Vector pos[], 
	Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld )
{
	matrix3x4_t bonematrix;

	QuaternionMatrix( q[iBone], pos[iBone], bonematrix );

	int parent = pStudioHdr->pBone( iBone )->parent;
	if (parent == -1) 
	{
		ConcatTransforms( rootxform, bonematrix, pBoneToWorld[iBone] );
	}
	else
	{
		// evil recursive!!!
		BuildBoneChain( pStudioHdr, rootxform, pos, q, parent, pBoneToWorld );
		ConcatTransforms( pBoneToWorld[parent], bonematrix, pBoneToWorld[iBone]);
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn a specific bones boneToWorld transform into a pos and q in parents bonespace
//-----------------------------------------------------------------------------
void SolveBone( 
	const studiohdr_t *pStudioHdr,
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	Vector pos[], 
	Quaternion q[]
	)
{
	int iParent = pStudioHdr->pBone( iBone )->parent;

	matrix3x4_t worldToBone;
	MatrixInvert( pBoneToWorld[iParent], worldToBone );

	matrix3x4_t local;
	ConcatTransforms( worldToBone, pBoneToWorld[iBone], local );

	MatrixAngles( local, q[iBone], pos[iBone] );
}



void CIKContext::SolveDependencies(
	Vector pos[], 
	Quaternion q[]
	)
{
	ASSERT_NO_REENTRY();
	
	static matrix3x4_t boneToWorld[MAXSTUDIOBONES];
	matrix3x4_t worldTarget;
	int i;

	iktarget_t chainRule[8]; // allocate!!!

	// init chain rules
	for (i = 0; i < m_pStudioHdr->numikchains; i++)
	{
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( i );
		iktarget_t *pChainRule = &chainRule[ i ];
		int bone = pchain->pLink( 2 )->bone;

		pChainRule->est.flWeight = 0.0;

		// eval current ik'd bone
		BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, bone, boneToWorld );

		MatrixAngles( boneToWorld[bone], pChainRule->local.q, pChainRule->local.pos );
		pChainRule->est.pos = pChainRule->local.pos;
		pChainRule->est.q = pChainRule->local.q;
	}

	for (i = 0; i < m_ikRule.Count(); i++)
	{
		iktarget_t *pChainRule = &chainRule[ m_ikRule[i].chain ];

		switch( m_ikRule[i].type )
		{
		case IK_SELF:
			{
				// eval target bone space
				BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, m_ikRule[i].bone, boneToWorld );

				// xform IK target error into world space
				matrix3x4_t local;
				QuaternionMatrix( m_ikRule[i].q, m_ikRule[i].pos, local );
				ConcatTransforms( boneToWorld[m_ikRule[i].bone], local, worldTarget );
			}
			break;
		case IK_WORLD:
			{
				QuaternionMatrix( m_ikRule[i].q, m_ikRule[i].pos, worldTarget );
			}	
			break;
		case IK_GROUND:
			{
				matrix3x4_t footTarget;
				int slot = m_ikRule[i].slot;

				bool bIsLatched = (m_target[slot].est.latched == 1.0);
				bool bLatchAvail = (bIsLatched && m_target[slot].latched.time >= m_flPrevTime && m_flPrevTime >= m_flTime - 0.1);

				if (bIsLatched && bLatchAvail)
				{
					// use latched goal
					AngleMatrix( m_target[slot].latched.q, m_target[slot].latched.pos, footTarget );
				}
				else
				{
					// latch off current goal
					m_target[slot].latched.q = m_target[slot].est.q;
					m_target[slot].latched.pos = m_target[slot].est.pos;
					// use existing position
					AngleMatrix( m_target[slot].est.q, m_target[slot].est.pos, footTarget );
				}

				// xform IK target error into world space
				matrix3x4_t local;
				QuaternionMatrix( m_ikRule[i].q, m_ikRule[i].pos, local );
				ConcatTransforms( footTarget, local, worldTarget );

				if (bIsLatched)
				{
					m_target[slot].latched.time = m_flTime;
				}
			}
			break;
		case IK_RELEASE:
			{
				pChainRule->est.flWeight = pChainRule->est.flWeight * (1 - m_ikRule[i].flWeight) + m_ikRule[i].flWeight;

				pChainRule->est.pos = pChainRule->est.pos * (1.0 - m_ikRule[i].flWeight ) + pChainRule->local.pos * m_ikRule[i].flWeight;
				QuaternionSlerp( pChainRule->est.q, pChainRule->local.q, m_ikRule[i].flWeight, pChainRule->est.q );

				continue;
			}
			break;
		}

		pChainRule->est.flWeight = pChainRule->est.flWeight * (1 - m_ikRule[i].flWeight) + m_ikRule[i].flWeight;

		Vector p2;
		Quaternion q2;
		
		// target p and q
		MatrixAngles( worldTarget, q2, p2 );

		// blend in position and angles
		pChainRule->est.pos = pChainRule->est.pos * (1.0 - m_ikRule[i].flWeight ) + p2 * m_ikRule[i].flWeight;
		QuaternionSlerp( pChainRule->est.q, q2, m_ikRule[i].flWeight, pChainRule->est.q );
	}

	for (i = 0; i < m_pStudioHdr->numikchains; i++)
	{
		iktarget_t *pChainRule = &chainRule[ i ];
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( i );

		if (pChainRule->est.flWeight > 0.0)
		{
			// do exact IK solution
			// FIXME: once per link!
			Studio_SolveIK(pchain, pChainRule->est.pos, boneToWorld );

			Vector p3;
			MatrixGetColumn( boneToWorld[pchain->pLink( 2 )->bone], 3, p3 );
			QuaternionMatrix( pChainRule->est.q, p3, boneToWorld[pchain->pLink( 2 )->bone] );

			// rebuild chain
			SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
			SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
			SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
		}
	}


#if 0
		Vector p1, p2, p3;
		Quaternion q1, q2, q3;

		// current p and q
		MatrixAngles( boneToWorld[bone], q1, p1 );

		
		// target p and q
		MatrixAngles( worldTarget, q2, p2 );

		// blend in position and angles
		p3 = p1 * (1.0 - m_ikRule[i].flWeight ) + p2 * m_ikRule[i].flWeight;

		// do exact IK solution
		// FIXME: once per link!
		Studio_SolveIK(pchain, p3, boneToWorld );

		// force angle (bad?)
		QuaternionSlerp( q1, q2, m_ikRule[i].flWeight, q3 );
		MatrixGetColumn( boneToWorld[bone], 3, p3 );
		QuaternionMatrix( q3, p3, boneToWorld[bone] );

		// rebuild chain
		SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
		SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
		SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
#endif
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::SolveAutoplayLocks(
	Vector pos[], 
	Quaternion q[]
	)
{
	ASSERT_NO_REENTRY();
	
	static matrix3x4_t boneToWorld[MAXSTUDIOBONES];
	int i;

	for (i = 0; i < m_ikRule.Count(); i++)
	{
		mstudioiklock_t *plock = m_pStudioHdr->pIKAutoplayLock( i );
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( plock->chain );
		int bone = pchain->pLink( 2 )->bone;

		// eval current ik'd bone
		BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, bone, boneToWorld );

		Vector p1, p2, p3;
		Quaternion q1, q2, q3;

		// current p and q
		MatrixAngles( boneToWorld[bone], q1, p1 );

		// blend in position
		p3 = p1 * (1.0 - plock->flPosWeight ) + m_ikRule[i].pos * plock->flPosWeight;

		// do exact IK solution
		Studio_SolveIK(pchain, p3, boneToWorld );

		// slam orientation
		MatrixGetColumn( boneToWorld[bone], 3, p3 );
		QuaternionMatrix( m_ikRule[i].q, p3, boneToWorld[bone] );

		// rebuild chain
		q2 = q[ bone ];
		SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
		QuaternionSlerp( q[bone], q2, plock->flLocalQWeight, q[bone] );

		SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
		SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::SolveSequenceLocks(
	mstudioseqdesc_t *pSeqDesc,
	Vector pos[], 
	Quaternion q[]
	)
{
	ASSERT_NO_REENTRY();
	
	static matrix3x4_t boneToWorld[MAXSTUDIOBONES];
	int i;

	for (i = 0; i < m_ikRule.Count(); i++)
	{
		mstudioiklock_t *plock = pSeqDesc->pIKLock( i );
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( plock->chain );
		int bone = pchain->pLink( 2 )->bone;

		// eval current ik'd bone
		BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, bone, boneToWorld );

		Vector p1, p2, p3;
		Quaternion q1, q2, q3;

		// current p and q
		MatrixAngles( boneToWorld[bone], q1, p1 );

		// blend in position
		p3 = p1 * (1.0 - plock->flPosWeight ) + m_ikRule[i].pos * plock->flPosWeight;

		// do exact IK solution
		Studio_SolveIK(pchain, p3, boneToWorld );

		// slam orientation
		MatrixGetColumn( boneToWorld[bone], 3, p3 );
		QuaternionMatrix( m_ikRule[i].q, p3, boneToWorld[bone] );

		// rebuild chain
		q2 = q[ bone ];
		SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
		QuaternionSlerp( q[bone], q2, plock->flLocalQWeight, q[bone] );

		SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
		SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
	}
}

//-----------------------------------------------------------------------------
// Purpose: run all animations that automatically play and are driven off of poseParameters
//-----------------------------------------------------------------------------
void CalcAutoplaySequences(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,
	Vector pos[], 
	Quaternion q[], 
	const float poseParameters[],
	int boneMask,
	float time
	)
{
	ASSERT_NO_REENTRY();
	
	int			i;

	if ( pIKContext )
	{
		pIKContext->AddAutoplayLocks( pos, q );
	}

	for (i = 0; i < pStudioHdr->numseq; i++)
	{
		mstudioseqdesc_t *pseqdesc = pStudioHdr->pSeqdesc( i );
		if (pseqdesc->flags & STUDIO_AUTOPLAY)
		{
			float cycle = 0;
			float cps = Studio_CPS( pStudioHdr, i, poseParameters );
			cycle = time * cps;
			cycle = cycle - (int)cycle;

			AccumulatePose( pStudioHdr, NULL, pos, q, i, cycle, poseParameters, boneMask );
		}
	}

	if ( pIKContext )
	{
		pIKContext->SolveAutoplayLocks( pos, q );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Studio_BuildMatrices(
	const studiohdr_t *pStudioHdr,
	const QAngle& angles, 
	const Vector& origin, 
	const Vector pos[],
	const Quaternion q[],
	int iBone,
	matrix3x4_t bonetoworld[MAXSTUDIOBONES],
	int boneMask
	)
{
	int i, j;

	int					chain[MAXSTUDIOBONES];
	int					chainlength = 0;

	if (iBone < -1 || iBone >= pStudioHdr->numbones)
		iBone = 0;

	mstudiobone_t *pbones		= pStudioHdr->pBone( 0 );

	// build list of what bones to use
	if (iBone == -1)
	{
		// all bones
		chainlength = pStudioHdr->numbones;
		for (i = 0; i < pStudioHdr->numbones; i++)
		{
			chain[chainlength - i - 1] = i;
		}
	}
	else
	{
		// only the parent bones
		i = iBone;
		while (i != -1)
		{
			chain[chainlength++] = i;
			i = pbones[i].parent;
		}
	}

	matrix3x4_t bonematrix;
	matrix3x4_t rotationmatrix; // model to world transformation
	AngleMatrix( angles, origin, rotationmatrix);

	for (j = chainlength - 1; j >= 0; j--)
	{
		i = chain[j];
		if (pbones[i].flags & boneMask)
		{
			QuaternionMatrix( q[i], pos[i], bonematrix );

			if (pbones[i].parent == -1) 
			{
				ConcatTransforms (rotationmatrix, bonematrix, bonetoworld[i]);
			} 
			else 
			{
				ConcatTransforms (bonetoworld[pbones[i].parent], bonematrix, bonetoworld[i]);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: look at single column vector of another bones local transformation 
//			and generate a procedural transformation based on how that column 
//			points down the 6 cardinal axis (all negative weights are clamped to 0).
//-----------------------------------------------------------------------------

void DoAxisInterpBone(
	mstudiobone_t		*pbones,
	int	ibone,
	matrix3x4_t *bonetoworld
	)
{
	matrix3x4_t			bonematrix;
	Vector				control;

	mstudioaxisinterpbone_t *pProc = (mstudioaxisinterpbone_t *)pbones[ibone].pProcedure( );
	if (pProc && pbones[pProc->control].parent != -1)
	{
		Vector tmp;
		// pull out the control column
		tmp.x = bonetoworld[pProc->control][0][pProc->axis];
		tmp.y = bonetoworld[pProc->control][1][pProc->axis];
		tmp.z = bonetoworld[pProc->control][2][pProc->axis];

		// invert it back into parent's space.
		VectorIRotate( tmp, bonetoworld[pbones[pProc->control].parent], control );
#if 0
		matrix3x4_t	tmpmatrix;
		matrix3x4_t	controlmatrix;
		MatrixInvert( bonetoworld[pbones[pProc->control].parent], tmpmatrix );
		ConcatTransforms( tmpmatrix, bonetoworld[pProc->control], controlmatrix );

		// pull out the control column
		control.x = controlmatrix[0][pProc->axis];
		control.y = controlmatrix[1][pProc->axis];
		control.z = controlmatrix[2][pProc->axis];
#endif
	}
	else
	{
		// pull out the control column
		control.x = bonetoworld[pProc->control][0][pProc->axis];
		control.y = bonetoworld[pProc->control][1][pProc->axis];
		control.z = bonetoworld[pProc->control][2][pProc->axis];
	}

	Quaternion *q1, *q2, *q3;
	Vector *p1, *p2, *p3;

	// find axial control inputs
	float a1 = control.x;
	float a2 = control.y;
	float a3 = control.z;
	if (a1 >= 0) 
	{ 
		q1 = &pProc->quat[0];
		p1 = &pProc->pos[0];
	} 
	else 
	{ 
		a1 = -a1; 
		q1 = &pProc->quat[1];
		p1 = &pProc->pos[1];
	}

	if (a2 >= 0) 
	{ 
		q2 = &pProc->quat[2]; 
		p2 = &pProc->pos[2];
	} 
	else 
	{ 
		a2 = -a2; 
		q2 = &pProc->quat[3]; 
		p2 = &pProc->pos[3];
	}

	if (a3 >= 0) 
	{ 
		q3 = &pProc->quat[4]; 
		p3 = &pProc->pos[4];
	} 
	else 
	{ 
		a3 = -a3; 
		q3 = &pProc->quat[5]; 
		p3 = &pProc->pos[5];
	}

	// do a three-way blend
	Vector p;
	Quaternion v, tmp;
	if (a1 + a2 > 0)
	{
		float t = 1.0 / (a1 + a2 + a3);
		// FIXME: do a proper 3-way Quat blend!
		QuaternionSlerp( *q2, *q1, a1 / (a1 + a2), tmp );
		QuaternionSlerp( tmp, *q3, a3 * t, v );
		VectorScale( *p1, a1 * t, p );
		VectorMA( p, a2 * t, *p2, p );
		VectorMA( p, a3 * t, *p3, p );
	}
	else
	{
		QuaternionSlerp( *q3, *q3, 0, v ); // ??? no quat copy?
		p = *p3;
	}

	QuaternionMatrix( v, p, bonematrix );

	ConcatTransforms (bonetoworld[pbones[ibone].parent], bonematrix, bonetoworld[ibone]);
}



//-----------------------------------------------------------------------------
// Purpose: Generate a procedural transformation based on how that another bones 
//			local transformation matches a set of target orientations.
//-----------------------------------------------------------------------------
void DoQuatInterpBone(
	mstudiobone_t		*pbones,
	int	ibone,
	matrix3x4_t *bonetoworld
	)
{
	matrix3x4_t			bonematrix;
	Vector				control;

	mstudioquatinterpbone_t *pProc = (mstudioquatinterpbone_t *)pbones[ibone].pProcedure( );
	if (pProc && pbones[pProc->control].parent != -1)
	{
		Quaternion	src;
		float		weight[32];
		float		scale = 0.0;
		Quaternion	quat;
		Vector		pos;

		matrix3x4_t	tmpmatrix;
		matrix3x4_t	controlmatrix;
		MatrixInvert( bonetoworld[pbones[pProc->control].parent], tmpmatrix );
		ConcatTransforms( tmpmatrix, bonetoworld[pProc->control], controlmatrix );

		MatrixAngles( controlmatrix, src, pos ); // FIXME: make a version without pos

		int i;
		for (i = 0; i < pProc->numtriggers; i++)
		{
			float dot = fabs( QuaternionDotProduct( pProc->pTrigger( i )->trigger, src ) );
			// FIXME: a fast acos should be acceptable
			weight[i] = 1 - (2 * acos( dot ) * pProc->pTrigger( i )->inv_tolerance );
			weight[i] = max( 0, weight[i] );
			scale += weight[i];
		}

		if (scale <= 0.001)  // EPSILON?
		{
			AngleMatrix( pProc->pTrigger( 0 )->quat, pProc->pTrigger( 0 )->pos, bonematrix );
			ConcatTransforms (bonetoworld[pbones[ibone].parent], bonematrix, bonetoworld[ibone]);
			return;
		}

		scale = 1.0 / scale;

		quat.Init( 0, 0, 0, 0);
		pos.Init( );

		for (i = 0; i < pProc->numtriggers; i++)
		{
			if (weight[i])
			{
				float s = weight[i] * scale;
				mstudioquatinterpinfo_t *pTrigger = pProc->pTrigger( i );

				QuaternionAlign( pTrigger->quat, quat, quat );

				quat.x = quat.x + s * pTrigger->quat.x;
				quat.y = quat.y + s * pTrigger->quat.y;
				quat.z = quat.z + s * pTrigger->quat.z;
				quat.w = quat.w + s * pTrigger->quat.w;
				pos = pos + s * pTrigger->pos;
			}
		}
		Assert( QuaternionNormalize( quat ) != 0);
		QuaternionMatrix( quat, pos, bonematrix );
	}

	ConcatTransforms (bonetoworld[pbones[ibone].parent], bonematrix, bonetoworld[ibone]);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool CalcProceduralBone(
	const studiohdr_t *pStudioHdr,
	int iBone,
	matrix3x4_t *bonetoworld
	)
{
	mstudiobone_t		*pbones = pStudioHdr->pBone( 0 );

	if ( pbones[iBone].flags & BONE_ALWAYS_PROCEDURAL )
	{
		switch( pbones[iBone].proctype )
		{
		case STUDIO_PROC_AXISINTERP:
			DoAxisInterpBone( pbones, iBone, bonetoworld );
			return true;

		case STUDIO_PROC_QUATINTERP:
			DoQuatInterpBone( pbones, iBone, bonetoworld );
			return true;

		default:
			return false;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: converts a ranged bone controller value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetController( const studiohdr_t *pStudioHdr, int iController, float flValue, float &ctlValue )
{
	if (! pStudioHdr)
		return flValue;

	mstudiobonecontroller_t *pbonecontroller = FindController(pStudioHdr, iController);
	if(!pbonecontroller)
	{
		ctlValue = 0;
		return flValue;
	}

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		}
		else
		{
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	ctlValue = (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start);
	if (ctlValue < 0) ctlValue = 0;
	if (ctlValue > 1) ctlValue = 1;

	float flReturnVal = ((1.0 - ctlValue)*pbonecontroller->start + ctlValue *pbonecontroller->end);

	// ugly hack, invert value if a rotational controller and end < start
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR) &&
		pbonecontroller->end < pbonecontroller->start				)
	{
		flReturnVal *= -1;
	}
	
	return flReturnVal;
}


//-----------------------------------------------------------------------------
// Purpose: converts a 0..1 encoded bone controller value into a ranged value
// Output: 	returns ranged value
//-----------------------------------------------------------------------------

float Studio_GetController( const studiohdr_t *pStudioHdr, int iController, float ctlValue )
{
	if (!pStudioHdr)
		return 0.0;

	mstudiobonecontroller_t *pbonecontroller = FindController(pStudioHdr, iController);
	if(!pbonecontroller)
		return 0;

	return ctlValue * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}


//-----------------------------------------------------------------------------
// Purpose: converts a ranged pose parameter value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetPoseParameter( const studiohdr_t *pStudioHdr, int iParameter, float flValue, float &ctlValue )
{
	if (iParameter < 0 || iParameter >= pStudioHdr->numposeparameters)
	{
		return 0;
	}

	mstudioposeparamdesc_t *pPoseParam = pStudioHdr->pPoseParameter( iParameter );

	if (pPoseParam->loop)
	{
		float wrap = (pPoseParam->start + pPoseParam->end) / 2.0 + pPoseParam->loop / 2.0;
		float shift = pPoseParam->loop - wrap;

		flValue = flValue - pPoseParam->loop * floor((flValue + shift) / pPoseParam->loop);
	}

	ctlValue = (flValue - pPoseParam->start) / (pPoseParam->end - pPoseParam->start);

	if (ctlValue < 0) ctlValue = 0;
	if (ctlValue > 1) ctlValue = 1;

	return ctlValue * (pPoseParam->end - pPoseParam->start) + pPoseParam->start;
}


//-----------------------------------------------------------------------------
// Purpose: converts a 0..1 encoded pose parameter value into a ranged value
// Output: 	returns ranged value
//-----------------------------------------------------------------------------

float Studio_GetPoseParameter( const studiohdr_t *pStudioHdr, int iParameter, float ctlValue )
{
	if (iParameter < 0 || iParameter >= pStudioHdr->numposeparameters)
	{
		return 0;
	}

	mstudioposeparamdesc_t *pPoseParam = pStudioHdr->pPoseParameter( iParameter );

	return ctlValue * (pPoseParam->end - pPoseParam->start) + pPoseParam->start;
}

// Hit Testing code:
// 
//
struct bonecache_t
{
	int				bone;
	matrix3x4_t		matrix;
	bonecache_t		*pnext;
};

struct studiocache_t
{
	studiohdr_t		*pStudioHdr;
	int				sequence;
	float			animtime;
	QAngle			angles;
	Vector			origin;
	int				boneMask;
	bonecache_t		*bones;	// points to a linked list of matrix transforms for each bone
	
	// FIXME:  Cache controllers and poseparameters, too???
//	float			controllers[ MAXSTUDIOBONECTRLS ];
//	float			poseparam[ MAXSTUDIOPOSEPARAM ];
};

//-----------------------------------------------------------------------------
// Purpose: Singleton static object which sets up the studio bone cache
//-----------------------------------------------------------------------------
class CStudioBoneCache
{
public:
	// must be power of 2 or change the cache code
	enum
	{
		BONE_CACHE_SIZE = 512,
		MODEL_CACHE_SIZE = 16,
	};

	CStudioBoneCache()
	{
		studiomodelstart = 0;

		bonefreelist = studiobonecache;

		memset( studiobonecache, 0, sizeof(studiobonecache) );

		memset( studiomodelcache, 0, sizeof(studiomodelcache) );

		for ( int i = 0; i < BONE_CACHE_SIZE; i++ )
		{
			studiobonecache[i].pnext = &studiobonecache[i+1];
		}

		studiobonecache[BONE_CACHE_SIZE-1].pnext = NULL;
	}

	inline void BoneCacheFree( studiocache_t *pcache );
	inline void BoneCacheFreeLRU( void );
	matrix3x4_t *Studio_LookupCachedBone( studiocache_t *pCache, int iBone );
	void Studio_LinkHitboxCache( matrix3x4_t **bones, studiocache_t *pcache, studiohdr_t *pStudioHdr, mstudiohitboxset_t *set );
	studiocache_t *Studio_GetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask );
	studiocache_t *Studio_SetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask, matrix3x4_t *bonetoworld );

private:

	bonecache_t *bonefreelist;
	bonecache_t	studiobonecache[BONE_CACHE_SIZE];
	studiocache_t studiomodelcache[MODEL_CACHE_SIZE];
	int studiomodelstart;
};

// Construct a singleton
static CStudioBoneCache g_StudioBoneCache;

void CStudioBoneCache::BoneCacheFree( studiocache_t *pcache )
{
	bonecache_t *pbone = pcache->bones;
	bonecache_t *pnext;

	while ( pbone )
	{
		pnext = pbone->pnext;
		pbone->pnext = bonefreelist;
		bonefreelist = pbone;
		pbone = pnext;
	}
	pcache->pStudioHdr = NULL;
	pcache->bones = NULL;
}

void CStudioBoneCache::BoneCacheFreeLRU( void )
{
	for ( int i = studiomodelstart+1; i != studiomodelstart; i++ )
	{
		i &= (MODEL_CACHE_SIZE-1);
		BoneCacheFree( studiomodelcache + i );
		
		if ( bonefreelist )
			return;
	}
}

matrix3x4_t *CStudioBoneCache::Studio_LookupCachedBone( studiocache_t *pCache, int iBone )
{
	bonecache_t *pbone = pCache->bones;

	// FIXME: linear search.  This done much?
	while ( pbone )
	{
		if (pbone->bone == iBone)
			return &pbone->matrix;
		if (pbone->bone > iBone)
			return NULL;
		pbone = pbone->pnext;
	}
	return NULL;
}

void CStudioBoneCache::Studio_LinkHitboxCache( matrix3x4_t **bones, studiocache_t *pcache, studiohdr_t *pStudioHdr, mstudiohitboxset_t *set )
{
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		bones[i] = Studio_LookupCachedBone( pcache, set->pHitbox(i)->bone );
		Assert(bones[i]);
	}
}

studiocache_t *CStudioBoneCache::Studio_GetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask )
{
	// check for a cache hit
	int i = studiomodelstart;
	do
	{
		studiocache_t *pcache = studiomodelcache + i;
		if ( pcache->pStudioHdr == pStudioHdr &&
			pcache->sequence == sequence &&
			pcache->animtime == animtime &&
			pcache->angles == angles &&
			pcache->origin == origin && 
			(pcache->boneMask & boneMask) == boneMask )
		{
			return pcache;
		}
		i = (i-1)&(MODEL_CACHE_SIZE-1);
	} while ( i != studiomodelstart );

	return NULL;
}

studiocache_t *CStudioBoneCache::Studio_SetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask, matrix3x4_t *bonetoworld )
{
	// Get the LRU model entry and reuse it
	studiomodelstart = (studiomodelstart+1)&(MODEL_CACHE_SIZE-1);
	studiocache_t *pcache = studiomodelcache + studiomodelstart;
	BoneCacheFree( pcache );

	pcache->pStudioHdr = pStudioHdr;
	pcache->sequence = sequence;
	pcache->animtime = animtime;
	pcache->angles = angles;
	pcache->origin = origin;
	pcache->boneMask = boneMask;
	pcache->bones = NULL;

	bonecache_t *plast = NULL;
	for ( int i = 0; i < pStudioHdr->numbones; i++ )
	{
		// 
		if (!(pStudioHdr->pBone(i)->flags & boneMask))
		{
			// FIXME: temporary hack, if no flags set, assume it's a old version
			if (pStudioHdr->pBone(0)->flags & BONE_USED_MASK)
			{
				continue;
			}
		}

		// make sure there's a free bone for this box
		if ( !bonefreelist )
		{
			BoneCacheFreeLRU();
		}
		bonecache_t *pbone = bonefreelist;
		bonefreelist = bonefreelist->pnext;
		if ( pcache->bones == NULL )
		{
			pcache->bones = pbone;
		}
		else
		{
			plast->pnext = pbone;
		}

		pbone->bone = i;
		MatrixCopy( bonetoworld[i], pbone->matrix ); 
		plast = pbone;
	}
	plast->pnext = NULL;
	return pcache;
}


matrix3x4_t *Studio_LookupCachedBone( studiocache_t *pCache, int iBone )
{
	return g_StudioBoneCache.Studio_LookupCachedBone( pCache, iBone );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **bones - 
//			*pcache - 
//			*pStudioHdr - 
//			*set - 
//-----------------------------------------------------------------------------
void Studio_LinkHitboxCache( matrix3x4_t **bones, studiocache_t *pcache, studiohdr_t *pStudioHdr, mstudiohitboxset_t *set )
{
	g_StudioBoneCache.Studio_LinkHitboxCache( bones, pcache, pStudioHdr, set );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//			sequence - 
//			cycle - 
//			angles - 
//			origin - 
//			boneMask - 
// Output : studiocache_t
//-----------------------------------------------------------------------------
studiocache_t *Studio_GetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask )
{
	return g_StudioBoneCache.Studio_GetBoneCache( pStudioHdr, sequence, animtime, angles, origin, boneMask );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
studiocache_t *Studio_SetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask, matrix3x4_t *bonetoworld )
{
	return g_StudioBoneCache.Studio_SetBoneCache( pStudioHdr, sequence, animtime, angles, origin, boneMask, bonetoworld );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Studio_InvalidateBoneCache( studiocache_t *pcache )
{
	g_StudioBoneCache.BoneCacheFree( pcache );
}


#pragma warning (disable : 4701)


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static int ClipRayToHitbox( const Ray_t &ray, mstudiobbox_t *pbox, matrix3x4_t& matrix, trace_t &tr )
{
	// scale by current t so hits shorten the ray and increase the likelihood of early outs
	Vector delta2 = (0.5f * tr.fraction) * ray.m_Delta;

	// OPTIMIZE: Store this in the box instead of computing it here
	// compute center in local space
	Vector boxextents = (pbox->bbmin + pbox->bbmax) * 0.5; 
	Vector boxCenter;
	// transform to world space
	VectorTransform( boxextents, matrix, boxCenter );
	// calc extents from local center
	boxextents = pbox->bbmax - boxextents;
	// OPTIMIZE: This is optimized for world space.  If the transform is fast enough, it may make more
	// sense to just xform and call UTIL_ClipToBox() instead.  MEASURE THIS.

	// save the extents of the ray along 
	Vector extent, uextent;
	Vector segmentCenter = ray.m_Start + delta2 - boxCenter;

	extent.Init();

	// check box axes for separation
	for ( int j = 0; j < 3; j++ )
	{
		extent[j] = delta2.x * matrix[0][j] + delta2.y * matrix[1][j] +	delta2.z * matrix[2][j];
		uextent[j] = fabsf(extent[j]);
		float coord = segmentCenter.x * matrix[0][j] + segmentCenter.y * matrix[1][j] +	segmentCenter.z * matrix[2][j];
		coord = fabsf(coord);

		if ( coord > (boxextents[j] + uextent[j]) )
			return -1;
	}

	// now check cross axes for separation
	float tmp, cextent;
	Vector cross = delta2.Cross( segmentCenter );
	cextent = cross.x * matrix[0][0] + cross.y * matrix[1][0] + cross.z * matrix[2][0];
	cextent = fabsf(cextent);
	tmp = boxextents[1]*uextent[2] + boxextents[2]*uextent[1];
	if ( cextent > tmp )
		return -1;

	cextent = cross.x * matrix[0][1] + cross.y * matrix[1][1] + cross.z * matrix[2][1];
	cextent = fabsf(cextent);
	tmp = boxextents[0]*uextent[2] + boxextents[2]*uextent[0];
	if ( cextent > tmp )
		return -1;

	cextent = cross.x * matrix[0][2] + cross.y * matrix[1][2] + cross.z * matrix[2][2];
	cextent = fabsf(cextent);
	tmp = boxextents[0]*uextent[1] + boxextents[1]*uextent[0];
	if ( cextent > tmp )
		return -1;

	// !!! We hit this box !!! compute intersection point and return
	Vector start;
	// Compute ray start in bone space
	VectorITransform( ray.m_Start, matrix, start );
	// extent is delta2 in bone space, recompute delta in bone space
	VectorScale( extent, 2, extent );
	int hitside;
	// delta was prescaled by the current t, so no need to see if this intersection
	// is closer
	bool startsolid;
	float fraction;
	if ( !IntersectRayWithBox( start, extent, pbox->bbmin, pbox->bbmax, 0, fraction, hitside, startsolid ) )
		return -1;
	Assert( IsFinite(fraction) );
	tr.fraction *= fraction;
	tr.startsolid = startsolid;
	return hitside;
}

#pragma warning (default : 4701)


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool TraceToStudio( const Ray_t& ray, studiohdr_t *pStudioHdr, mstudiohitboxset_t *set, 
				   matrix3x4_t **hitboxbones, int fContentsMask, trace_t &tr )
{
	tr.fraction = 1.0;
	tr.startsolid = false;

	// no hit yet
	int hitbox = -1;
	int hitside = -1;

	// OPTIMIZE: Partition these?
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		// Filter based on contents mask
		int fBoneContents = pStudioHdr->pBone( pbox->bone )->contents;
		if ( ( fBoneContents & fContentsMask ) == 0 )
			continue;
		
		// columns are axes of the bones in world space, translation is in world space
		matrix3x4_t& matrix = *hitboxbones[i];

		int side = ClipRayToHitbox( ray, pbox, matrix, tr );
		if ( side >= 0 )
		{
			hitbox = i;
			hitside = side;
		}
	}

	if ( hitbox >= 0 )
	{
		tr.endpos = ray.m_Start + tr.fraction * ray.m_Delta;
		tr.hitgroup = set->pHitbox(hitbox)->group;
		tr.hitbox = hitbox;
		tr.contents = pStudioHdr->pBone( set->pHitbox(hitbox)->bone )->contents | CONTENTS_HITBOX;
		tr.physicsbone = pStudioHdr->pBone( set->pHitbox(hitbox)->bone )->physicsbone;
		Assert( tr.physicsbone >= 0 );

		matrix3x4_t& matrix = *hitboxbones[hitbox];
		if ( hitside >= 3 )
		{
			hitside -= 3;
			tr.plane.normal[0] = matrix[0][hitside];
			tr.plane.normal[1] = matrix[1][hitside];
			tr.plane.normal[2] = matrix[2][hitside];
			//tr.plane.dist = DotProduct( tr.plane.normal, Vector(matrix[0][3], matrix[1][3], matrix[2][3] ) ) + pbox->bbmax[hitside];
		}
		else
		{
			tr.plane.normal[0] = -matrix[0][hitside];
			tr.plane.normal[1] = -matrix[1][hitside];
			tr.plane.normal[2] = -matrix[2][hitside];
			//tr.plane.dist = DotProduct( tr.plane.normal, Vector(matrix[0][3], matrix[1][3], matrix[2][3] ) ) - pbox->bbmin[hitside];
		}
		// simpler plane constant equation
		tr.plane.dist = DotProduct( tr.endpos, tr.plane.normal );
		tr.plane.type = 3;
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: returns array of animations and weightings for a sequence based on current pose parameters
//-----------------------------------------------------------------------------

void Studio_SeqAnims( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], mstudioanimdesc_t *panim[4], float *weight )
{
	if (!pStudioHdr || iSequence >= pStudioHdr->numseq)
	{
		weight[0] = weight[1] = weight[2] = weight[3] = 0.0;
		return;
	}

	int i0 = 0, i1 = 0;
	float s0 = 0, s1 = 0;
	
	mstudioseqdesc_t *pseqdesc = pStudioHdr->pSeqdesc( iSequence );

	Studio_LocalPoseParameter( pStudioHdr, poseParameter, pseqdesc, 0, s0, i0 );
	Studio_LocalPoseParameter( pStudioHdr, poseParameter, pseqdesc, 1, s1, i1 );

	panim[0] = pStudioHdr->pAnimdesc( pseqdesc->anim[i0  ][i1  ] );
	weight[0] = (1 - s0) * (1 - s1);

	panim[1] = pStudioHdr->pAnimdesc( pseqdesc->anim[i0+1][i1  ] );
	weight[1] = (s0) * (1 - s1);

	panim[2] = pStudioHdr->pAnimdesc( pseqdesc->anim[i0  ][i1+1] );
	weight[2] = (1 - s0) * (s1);

	panim[3] = pStudioHdr->pAnimdesc( pseqdesc->anim[i0+1][i1+1] );
	weight[3] = (s0) * (s1);

	Assert( weight[0] >= 0.0f && weight[1] >= 0.0f && weight[2] >= 0.0f && weight[3] >= 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: returns max frame number for a sequence
//-----------------------------------------------------------------------------

int Studio_MaxFrame( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );

	float maxFrame = 0;
	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0)
		{
			maxFrame += panim[i]->numframes * weight[i];
		}
	}

	if ( maxFrame > 1 )
		maxFrame -= 1;
	
	return maxFrame;
}


//-----------------------------------------------------------------------------
// Purpose: returns frames per second of a sequence
//-----------------------------------------------------------------------------

float Studio_FPS( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );

	float t = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0)
		{
			t += panim[i]->fps * weight[i];
		}
	}
	return t;
}


//-----------------------------------------------------------------------------
// Purpose: returns cycles per second of a sequence
//-----------------------------------------------------------------------------

float Studio_CPS( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );

	float t = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0 && panim[i]->numframes > 1)
		{
			t += (panim[i]->fps / (panim[i]->numframes - 1)) * weight[i];
		}
	}
	return t;
}

//-----------------------------------------------------------------------------
// Purpose: returns length (in seconds) of a sequence
//-----------------------------------------------------------------------------

float Studio_Duration( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );

	float t = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0)
		{
			if ( panim[i]->fps != 0.0f )
			{
				t +=  ((panim[i]->numframes - 1) / panim[i]->fps) * weight[i];
			}
		}
	}

	return t;
}


//-----------------------------------------------------------------------------
// Purpose: calculate changes in position and angle relative to the start of an animations cycle
// Output:	updated position and angle, relative to the origin
//			returns false if animation is not a movement animation
//-----------------------------------------------------------------------------

bool Studio_AnimPosition( mstudioanimdesc_t *panim, float flCycle, Vector &vecPos, QAngle &vecAngle )
{
	float	prevframe = 0;
	vecPos.Init( );
	vecAngle.Init( );

	if (panim->nummovements == 0)
		return false;

	int iLoops = 0;
	if (flCycle > 1.0)
	{
		iLoops = (int)flCycle;
	}
	else if (flCycle < 0.0)
	{
		iLoops = (int)flCycle - 1;
	}
	flCycle = flCycle - iLoops;

	float	flFrame = flCycle * (panim->numframes - 1);

	for (int i = 0; i < panim->nummovements; i++)
	{
		mstudiomovement_t *pmove = panim->pMovement( i );

		if (pmove->endframe >= flFrame)
		{
			float f = (flFrame - prevframe) / (pmove->endframe - prevframe);

			float d = pmove->v0 * f + 0.5 * (pmove->v1 - pmove->v0) * f * f;

			vecPos = vecPos + d * pmove->vector;
			vecAngle.y = vecAngle.y * (1 - f) + pmove->angle * f;
			if (iLoops != 0)
			{
				mstudiomovement_t *pmove = panim->pMovement( panim->nummovements - 1 );
				vecPos = vecPos + iLoops * pmove->position; 
				vecAngle.y = vecAngle.y + iLoops * pmove->angle; 
			}
			return true;
		}
		else
		{
			prevframe = pmove->endframe;
			vecPos = pmove->position;
			vecAngle.y = pmove->angle;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: calculate instantaneous velocity at a given point in the animations cycle
// Output:	velocity vector, relative to identity orientation
//			returns false if animation is not a movement animation
//-----------------------------------------------------------------------------

bool Studio_AnimVelocity( mstudioanimdesc_t *panim, float flCycle, Vector &vecVelocity )
{
	float	prevframe = 0;

	float	flFrame = flCycle * (panim->numframes - 1);
	flFrame = flFrame - (int)(flFrame / (panim->numframes - 1));

	for (int i = 0; i < panim->nummovements; i++)
	{
		mstudiomovement_t *pmove = panim->pMovement( i );

		if (pmove->endframe >= flFrame)
		{
			float f = (flFrame - prevframe) / (pmove->endframe - prevframe);

			float vel = pmove->v0 * (1 - f) + pmove->v1 * f;
			// scale from per block to per cycle velocity
			vel = vel * (pmove->endframe - prevframe) / (panim->numframes - 1);

			vecVelocity = pmove->vector * vel;
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: calculate changes in position and angle between two points in an animation cycle
// Output:	updated position and angle, relative to CycleFrom being at the origin
//			returns false if animation is not a movement animation
//-----------------------------------------------------------------------------

bool Studio_AnimMovement( mstudioanimdesc_t *panim, float flCycleFrom, float flCycleTo, Vector &deltaPos, QAngle &deltaAngle )
{
	if (panim->nummovements == 0)
		return false;

	Vector startPos;
	QAngle startA;
	Studio_AnimPosition( panim, flCycleFrom, startPos, startA );

	Vector endPos;
	QAngle endA;
	Studio_AnimPosition( panim, flCycleTo, endPos, endA );

	Vector tmp = endPos - startPos;
	deltaAngle.y = endA.y - startA.y;
	VectorYawRotate( tmp, -startA.y, deltaPos );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: finds how much of an animation to play to move given linear distance
//-----------------------------------------------------------------------------

float Studio_FindAnimDistance( mstudioanimdesc_t *panim, float flDist )
{
	float	prevframe = 0;

	if (flDist <= 0)
		return 0.0;

	for (int i = 0; i < panim->nummovements; i++)
	{
		mstudiomovement_t *pmove = panim->pMovement( i );

		float flMove = (pmove->v0 + pmove->v1) * 0.5;

		if (flMove >= flDist)
		{
			float root1, root2;

			// d = V0 * t + 1/2 (V1-V0) * t^2
			if (SolveQuadratic( 0.5 * (pmove->v1 - pmove->v0), pmove->v0, -flDist, root1, root2 ))
			{
				float cpf = 1.0 / (panim->numframes - 1);  // cycles per frame

				return (prevframe + root1 * (pmove->endframe - prevframe)) * cpf;
			}
			return 0.0;
		}
		else
		{
			flDist -= flMove;
			prevframe = pmove->endframe;
		}
	}
	return 1.0;
}


//-----------------------------------------------------------------------------
// Purpose: calculate changes in position and angle between two points in a sequences cycle
// Output:	updated position and angle, relative to CycleFrom being at the origin
//			returns false if sequence is not a movement sequence
//-----------------------------------------------------------------------------

bool Studio_SeqMovement( const studiohdr_t *pStudioHdr, int iSequence, float flCycleFrom, float flCycleTo, const float poseParameter[], Vector &deltaPos, QAngle &deltaAngles )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );
	
	deltaPos.Init( );
	deltaAngles.Init( );

	bool found = false;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			Vector localPos;
			QAngle localAngles;

			localPos.Init();
			localAngles.Init();

			if (Studio_AnimMovement( panim[i], flCycleFrom, flCycleTo, localPos, localAngles ))
			{
				found = true;
				deltaPos = deltaPos + localPos * weight[i];
				// FIXME: this makes no sense
				deltaAngles = deltaAngles + localAngles * weight[i];
			}
		}
	}
	return found;
}


//-----------------------------------------------------------------------------
// Purpose: calculate instantaneous velocity at a given point in the sequence's cycle
// Output:	velocity vector, relative to identity orientation
//			returns false if sequence is not a movement sequence
//-----------------------------------------------------------------------------

bool Studio_SeqVelocity( const studiohdr_t *pStudioHdr, int iSequence, float flCycle, const float poseParameter[], Vector &vecVelocity )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );
	
	vecVelocity.Init( );

	bool found = false;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			Vector vecLocalVelocity;

			if (Studio_AnimVelocity( panim[i], flCycle, vecLocalVelocity ))
			{
				vecVelocity = vecVelocity + vecLocalVelocity * weight[i] * (panim[i]->numframes / panim[i]->fps);
				found = true;
			}
		}
	}
	return found;
}

//-----------------------------------------------------------------------------
// Purpose: finds how much of an sequence to play to move given linear distance
//-----------------------------------------------------------------------------

float Studio_FindSeqDistance( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], float flDist )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, iSequence, poseParameter, panim, weight );
	
	float flCycle = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			float flLocalCycle = Studio_FindAnimDistance( panim[i], flDist );
			flCycle = flCycle + flLocalCycle * weight[i];
		}
	}
	return flCycle;
}

//-----------------------------------------------------------------------------
// Purpose: lookup attachment by name
//-----------------------------------------------------------------------------

int Studio_FindAttachment( const studiohdr_t *pStudioHdr, const char *pAttachmentName )
{
	if ( pStudioHdr )
	{
		// Extract the bone index from the name
		for (int i = 0; i < pStudioHdr->numattachments; i++)
		{
			if (!stricmp(pAttachmentName,pStudioHdr->pAttachment(i)->pszName( ))) 
			{
				return i;
			}
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: lookup attachments by substring. Randomly return one of the matching attachments.
//-----------------------------------------------------------------------------

int Studio_FindRandomAttachment( const studiohdr_t *pStudioHdr, const char *pAttachmentName )
{
	if ( pStudioHdr )
	{
		// First move them all matching attachments into a list
		CUtlVector<int> matchingAttachments;

		// Extract the bone index from the name
		for (int i = 0; i < pStudioHdr->numattachments; i++)
		{
			if ( strstr( pStudioHdr->pAttachment(i)->pszName(), pAttachmentName ) ) 
			{
				matchingAttachments.AddToTail(i);
			}
		}

		// Then randomly return one of the attachments
		if ( matchingAttachments.Size() > 0 )
			return matchingAttachments[ RandomInt( 0, matchingAttachments.Size()-1 ) ];
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: lookup bone by name
//-----------------------------------------------------------------------------

int Studio_BoneIndexByName( const studiohdr_t *pStudioHdr, const char *pName )
{
	// Extract the bone index from the name
	mstudiobone_t *pbones = pStudioHdr->pBone( 0 );
	for ( int i = 0; i < pStudioHdr->numbones; i++ )
	{
		if (!stricmp(pName,pbones[i].pszName( ))) 
			return i;
	}

	return -1;
}

const char *Studio_GetDefaultSurfaceProps( studiohdr_t *pstudiohdr )
{
	return pstudiohdr->pszSurfaceProp();
}

float Studio_GetMass( studiohdr_t *pstudiohdr )
{
	return pstudiohdr->mass;
}

//-----------------------------------------------------------------------------
// Purpose: return pointer to sequence key value buffer
//-----------------------------------------------------------------------------

const char *Studio_GetKeyValueText( const studiohdr_t *pStudioHdr, int iSequence )
{
	if (pStudioHdr)
	{
		if (iSequence >= 0 && iSequence < pStudioHdr->numseq)
		{
			return pStudioHdr->pSeqdesc( iSequence )->KeyValueText();
		}
	}
	return NULL;
}

