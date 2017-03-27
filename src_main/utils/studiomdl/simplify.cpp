/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/


//
// studiomdl.c: generates a studio .mdl file from a .qc script
// models/<scriptname>.mdl.
//


#pragma warning( disable : 4244 )
#pragma warning( disable : 4237 )
#pragma warning( disable : 4305 )


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <float.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "studio.h"
#include "studiomdl.h"
#include "bone_setup.h"
#include "vstdlib/strtools.h"
#include "vmatrix.h"

void ClearModel (void)
{

}

//----------------------------------------------------------------------
// underlay:
// studiomdl : delta = new_anim * ( -1 * base_anim )
// engine : result = (w * delta) * base_anim
// 
// overlay
// 
// studiomdl : delta = (-1 * base_anim ) * new_anim
// engine : result = base_anim * (w * delta)
//
//----------------------------------------------------------------------

void QuaternionSMAngles( float s, Quaternion const &p, Quaternion const &q, RadianEuler &angles )
{
	Quaternion qt;
	QuaternionSM( s, p, q, qt );
	QuaternionAngles( qt, angles );
}


void QuaternionMAAngles( Quaternion const &p, float s, Quaternion const &q, RadianEuler &angles )
{
	Quaternion qt;
	QuaternionMA( p, s, q, qt );
	QuaternionAngles( qt, angles );
}


// q = p * (-q * p)

//-----------------------------------------------------------------------------
// Purpose: subtract linear motion from root bone animations
//			fixup missing frames from looping animations
//			create "delta" animations
//-----------------------------------------------------------------------------
int g_rootIndex = 0;


void buildAnimationWeights( void );
void extractLinearMotion( s_animation_t *panim, int motiontype, int iStartFrame, int iEndFrame, int iSrcFrame, s_animation_t *pRefAnim, int iRefFrame /* , Vector pos, QAngle angles */ );
void fixupMissingFrame( s_animation_t *panim );
void realignLooping( s_animation_t *panim );
void extractUnusedMotion( s_animation_t *panim );

// TODO: psrc and pdest as terms are ambigious, replace with something better
void setAnimationWeight( s_animation_t *panim, int index );
void processMatch( s_animation_t *psrc, s_animation_t *pdest );
void processAutoorigin( s_animation_t *psrc, s_animation_t *pdest, int flags, int srcframe, int destframe );
void subtractBaseAnimations( s_animation_t *psrc, s_animation_t *pdest, int srcframe, int flags );
void fixupLoopingDiscontinuities( s_animation_t *panim, int start, int end );
void makeAngle( s_animation_t *panim, float angle );
void fixupIKErrors( s_animation_t *panim, s_ikrule_t *pRule );
void createDerivative( s_animation_t *panim, float scale );
void clearAnimations( s_animation_t *panim );

void linearDelta( s_animation_t *psrc, s_animation_t *pdest, int srcframe, int flags );
void splineDelta( s_animation_t *psrc, s_animation_t *pdest, int srcframe, int flags );
void reencodeAnimation( s_animation_t *panim, int frameskip );
void forceNumframes( s_animation_t *panim, int frames );

void forceAnimationLoop( s_animation_t *panim );

void processAnimations()
{ 
	int i, j;

	// find global root bone.
	if ( strlen( rootname ) )
	{
		g_rootIndex = findGlobalBone( rootname );
		if (g_rootIndex == -1)
			g_rootIndex = 0;
	}

	buildAnimationWeights( );

	for (i = 0; i < g_numani; i++)
	{
		s_animation_t *panim = g_panimation[i];

		extractUnusedMotion( panim ); // FIXME: this should be part of LinearMotion()

		setAnimationWeight( panim, 0 );

		int startframe = 0;

		if (panim->fudgeloop)
		{
			fixupMissingFrame( panim );
		}

		if (panim->motiontype)
		{
			extractLinearMotion( panim, panim->motiontype, startframe, panim->numframes - 1, panim->numframes - 1, panim, startframe );
			startframe = panim->numframes - 1;
		}

		for (j = 0; j < panim->numcmds; j++)
		{
			s_animcmd_t *pcmd = &panim->cmds[j];

			switch( pcmd->cmd )
			{
			case CMD_WEIGHTS:
				setAnimationWeight( panim, pcmd->u.weightlist.index );
				break;
			case CMD_SUBTRACT:
				panim->flags |= STUDIO_DELTA;
				subtractBaseAnimations( pcmd->u.subtract.ref, panim, pcmd->u.subtract.frame, pcmd->u.subtract.flags );
				break;
			case CMD_AO:
				processAutoorigin( pcmd->u.ao.ref, panim, pcmd->u.ao.motiontype, pcmd->u.ao.srcframe, pcmd->u.ao.destframe );
				break;
			case CMD_MATCH:
				processMatch( pcmd->u.match.ref, panim );
				break;
			case CMD_FIXUP:
				fixupLoopingDiscontinuities( panim, pcmd->u.fixuploop.start, pcmd->u.fixuploop.end );
				break;
			case CMD_ANGLE:
				makeAngle( panim, pcmd->u.angle.angle );
				break;
			case CMD_IKFIXUP:
				{
					s_ikrule_t ikrule = *(pcmd->u.ikfixup.pRule);
					fixupIKErrors( panim, &ikrule );
				}
				break;
			case CMD_IKRULE:
				// processed later
				break;
			case CMD_MOTION:
				{
					extractLinearMotion( 
						panim, 
						pcmd->u.motion.motiontype, 
						startframe, 
						pcmd->u.motion.iEndFrame, 
						pcmd->u.motion.iEndFrame, 
						panim, 
						startframe );
					startframe = pcmd->u.motion.iEndFrame;
				}
				break;
			case CMD_REFMOTION:
				{
					extractLinearMotion(
						panim, 
						pcmd->u.motion.motiontype, 
						startframe, 
						pcmd->u.motion.iEndFrame, 
						pcmd->u.motion.iSrcFrame, 
						pcmd->u.motion.pRefAnim, 
						pcmd->u.motion.iRefFrame );
					startframe = pcmd->u.motion.iEndFrame;
				}
				break;
			case CMD_DERIVATIVE:
				{
					createDerivative(
						panim, 
						pcmd->u.derivative.scale );
				}
				break;
			case CMD_NOANIMATION:
				{
					clearAnimations( panim );
				}
				break;
			case CMD_LINEARDELTA:
				{
					panim->flags |= STUDIO_DELTA;
					linearDelta( panim, panim, panim->numframes - 1, pcmd->u.linear.flags );
				}
				break;
			case CMD_COMPRESS:
				{
					reencodeAnimation( panim, pcmd->u.compress.frames );
				}
				break;
			case CMD_NUMFRAMES:
				{
					forceNumframes( panim, pcmd->u.numframes.frames );
				}
				break;
			}
		}

		realignLooping( panim );

		forceAnimationLoop( panim );
	}

	// merge weightlists
	for (i = 0; i < g_numseq; i++)
	{
		int k, n;
		for (n = 0; n < g_numbones; n++)
		{
			g_sequence[i].weight[n] = 0.0;

			for (j = 0; j < g_sequence[i].groupsize[0]; j++)
			{
				for (k = 0; k < g_sequence[i].groupsize[1]; k++)
				{
					g_sequence[i].weight[n] = max( g_sequence[i].weight[n], g_sequence[i].panim[j][k]->weight[n] );
				}
			}
		}
	}
}


/*
void lookupLinearMotion( s_animation_t *panim, int motiontype, int startframe, int endframe, Vector &p1, Vector &p2 )
{
	Vector p0 = panim->sanim[startframe][g_rootIndex].pos;
	p2 = panim->sanim[endframe][g_rootIndex].pos[0];

	float fFrame = (startframe + endframe) / 2.0;
	int iFrame = (int)fFrame;
	float s = fFrame - iFrame;

	p1 = panim->sanim[iFrame][g_rootIndex].pos * (1 - s) + panim->sanim[iFrame+1][g_rootIndex].pos * s;
}
*/


	// 0.375 * v1 + 0.125 * v2 - d2 = 0.5 * v1 + 0.5 * v2 - d3;

	// 0.375 * v1 - 0.5 * v1 = 0.5 * v2 - d3 - 0.125 * v2 + d2;
	// 0.375 * v1 - 0.5 * v1 = 0.5 * v2 - d3 - 0.125 * v2 + d2;
	// -0.125 * v1 = 0.375 * v2 - d3 + d2;
	// v1 = (0.375 * v2 - d3 + d2) / -0.125;

	// -3 * (0.375 * v2 - d3 + d2) + 0.125 * v2 - d2 = 0
	// -3 * (0.375 * v2 - d3 + d2) + 0.125 * v2 - d2 = 0
	// -1 * v2 + 3 * d3 - 3 * d2 - d2 = 0
	// v2 = 3 * d3 - 4 * d2

	// 0.5 * v1 + 0.5 * v2 - d3
	// -4 * (0.375 * v2 - d3 + d2) + 0.5 * v2 - d3 = 0
	// -1.5 * v2 + 4 * d3 - 4 * d2 + 0.5 * v2 - d3 = 0
	// v2 = 4 * d3 - 4 * d2 - d3 
	// v2 = 3 * d3 - 4 * d2

	// 0.5 * v1 + 0.5 * (3 * d3 - 4 * d2) - d3 = 0
	// v1 + (3 * d3 - 4 * d2) - 2 * d3 = 0
	// v1 = -3 * d3 + 4 * d2 + 2 * d3
	// v1 = -1 * d3 + 4 * d2



void ConvertToAnimLocal( s_animation_t *panim, Vector &pos, QAngle &angles )
{
	matrix3x4_t bonematrix;
	matrix3x4_t adjmatrix;

	// convert explicit position/angle into animation relative values
	AngleMatrix( angles, pos, bonematrix );
	AngleMatrix( panim->rotation, panim->adjust, adjmatrix );
	MatrixInvert( adjmatrix, adjmatrix );
	ConcatTransforms( adjmatrix, bonematrix, bonematrix );
	MatrixAngles( bonematrix, angles, pos );
	// pos = pos * panim->scale;
}

//-----------------------------------------------------------------------------
// Purpose: find the linear movement/rotation between two frames, subtract that 
//			out of the animation and add it back on as a "piecewise movement" command
//-----------------------------------------------------------------------------

void extractLinearMotion( s_animation_t *panim, int motiontype, int iStartFrame, int iEndFrame, int iSrcFrame, s_animation_t *pRefAnim, int iRefFrame /* , Vector pos, QAngle angles */ )
{
	int j, k;
	matrix3x4_t adjmatrix;

	// Can't extract motion with only 1 frame of animation!
	if ( panim->numframes <= 1 )
	{
		Error( "Can't extract motion from sequence %s (%s).  Check your QC options!\n", panim->name, panim->filename );
	}

	if (panim->numpiecewisekeys >= MAXSTUDIOMOVEKEYS)
	{
		Error( "Too many piecewise movement keys in %s (%s)\n", panim->name, panim->filename );
	}

	if (iEndFrame > panim->numframes - 1)
		iEndFrame = panim->numframes - 1;

	if (iSrcFrame > panim->numframes - 1)
		iSrcFrame = panim->numframes - 1;

	float fFrame = (iStartFrame + iSrcFrame) / 2.0;
	int iMidFrame = (int)fFrame;
	float s = fFrame - iMidFrame;

	// find rotation
	RadianEuler	rot( 0, 0, 0 );

	if (motiontype & (STUDIO_LXR | STUDIO_LYR | STUDIO_LZR))
	{
		Quaternion q0;
		Quaternion q1;
		Quaternion q2;

		AngleQuaternion( pRefAnim->sanim[iRefFrame][g_rootIndex].rot, q0 );
		AngleQuaternion( panim->sanim[iMidFrame][g_rootIndex].rot, q1 ); // only used for rotation checking
		AngleQuaternion( panim->sanim[iSrcFrame][g_rootIndex].rot, q2 );

		Quaternion deltaQ1;
		QuaternionMA( q1, -1, q0, deltaQ1 );
		Quaternion deltaQ2;
		QuaternionMA( q2, -1, q0, deltaQ2 );

		// FIXME: this is still wrong, but it should be slightly more robust
		RadianEuler a3;
		if (motiontype & STUDIO_LXR)
		{
			Quaternion q4;
			q4.Init( deltaQ2.x, 0, 0, deltaQ2.w );
			QuaternionNormalize( q4 );
			QuaternionAngles( q4, a3 );
			rot.x = a3.x;
		}
		if (motiontype & STUDIO_LYR)
		{
			Quaternion q4;
			q4.Init( 0, deltaQ2.y, 0, deltaQ2.w );
			QuaternionNormalize( q4 );
			QuaternionAngles( q4, a3 );
			rot.y = a3.y;
		}
		if (motiontype & STUDIO_LZR)
		{
			Quaternion q4;
			q4.Init( 0, 0, deltaQ2.z, deltaQ2.w );
			QuaternionNormalize( q4 );
			QuaternionAngles( q4, a3 );

			Quaternion q5;
			RadianEuler a5;
			q5.Init( 0, 0, deltaQ1.z, deltaQ1.w );
			QuaternionNormalize( q5 );
			QuaternionAngles( q5, a5 );
			if (a3.z > M_PI) a5.z -= 2*M_PI;
			if (a3.z < -M_PI) a5.z += 2*M_PI;
			if (a5.z > M_PI) a5.z -= 2*M_PI;
			if (a5.z < -M_PI) a5.z += 2*M_PI;
			// check for possible rotations >180 degrees
			if (a5.z > M_PI/4 && a3.z < 0)
			{
				a3.z += 2*M_PI;
			}
			if (a5.z < -M_PI/4 && a3.z > 0)
			{
				a3.z -= 2*M_PI;
			}

			rot.z = a3.z;
		}
	}

	// find movement
	Vector p0;
	AngleMatrix(rot, adjmatrix );
	VectorRotate( pRefAnim->sanim[iRefFrame][g_rootIndex].pos, adjmatrix, p0 );

	Vector p2 = panim->sanim[iSrcFrame][g_rootIndex].pos;
	Vector p1 = panim->sanim[iMidFrame][g_rootIndex].pos * (1 - s) + panim->sanim[iMidFrame+1][g_rootIndex].pos * s;

	// ConvertToAnimLocal( panim, pos, angles ); // FIXME: unused

	p2 = p2 - p0;
	p1 = p1 - p0;

	if (!(motiontype & STUDIO_LX)) { p2.x = 0; p1.x = 0; };
	if (!(motiontype & STUDIO_LY)) { p2.y = 0; p1.y = 0; };
	if (!(motiontype & STUDIO_LZ)) { p2.z = 0; p1.z = 0; };
	
	// printf("%s  %.1f %.1f %.1f\n", g_bonetable[g_rootIndex].name, p2.x, p2.y, p2.z );

	float d1 = p1.Length();
	float d2 = p2.Length();

	float v0 = -1 * d2 + 4 * d1;
	float v1 = 3 * d2 - 4 * d1;

	printf("%s : %d - %d : %.1f %.1f %.1f\n", panim->name, iStartFrame, iEndFrame, p2.x, p2.y, RAD2DEG( rot[2] ) );

	int numframes = iEndFrame - iStartFrame + 1;
	if (numframes < 1)
		return;

	float n = numframes - 1;

	//printf("%f %f : ", v0, v1 );

	if (v0 < 0.0f)
	{
		v0 = 0.0;
		v1 = p2.Length() * 2.0;
	}
	else if (v1 < 0.0)
	{
		v0 = p2.Length() * 2.0;
		v1 = 0.0;
	}
	else if ((v0+v1) > 0.01 && (fabs(v0-v1) / (v0+v1)) < 0.2)
	{
		// if they're within 10% of each other, assum no acceleration
		v0 = v1 = p2.Length();
	}

	//printf("%f %f\n", v0, v1 );

	Vector v = p2;
	VectorNormalize( v );

	Vector	adjpos;
	RadianEuler	adjangle;
	matrix3x4_t bonematrix;
	for (j = 0; j < numframes; j++)
	{	
		float t = (j / n);

		VectorScale( v, v0 * t + 0.5 * (v1 - v0) * t * t, adjpos );
		VectorScale( rot, t, adjangle );

		AngleMatrix( adjangle, adjpos, adjmatrix );
		MatrixInvert( adjmatrix, adjmatrix );

		for (k = 0; k < g_numbones; k++)
		{
			if (g_bonetable[k].parent == -1)
			{
				// printf(" %.1f %.1f %.1f : ", adjpos[0], adjpos[1], RAD2DEG( adjangle[2] ));

				// printf(" %.1f %.1f %.1f\n", adjpos[0], adjpos[1], adjpos[2] );

				AngleMatrix( panim->sanim[j+iStartFrame][k].rot, panim->sanim[j+iStartFrame][k].pos, bonematrix );
				ConcatTransforms( adjmatrix, bonematrix, bonematrix );

				MatrixAngles( bonematrix, panim->sanim[j+iStartFrame][k].rot, panim->sanim[j+iStartFrame][k].pos );
				// printf("%d : %.1f %.1f %.1f\n", j, panim->sanim[j+iStartFrame][k].pos.x, panim->sanim[j+iStartFrame][k].pos.y, RAD2DEG( panim->sanim[j+iStartFrame][k].rot.z ) );
			}
		}
	}

	for (; j+iStartFrame < panim->numframes; j++)
	{
		for (k = 0; k < g_numbones; k++)
		{
			if (g_bonetable[k].parent == -1)
			{
				AngleMatrix( panim->sanim[j+iStartFrame][k].rot, panim->sanim[j+iStartFrame][k].pos, bonematrix );
				ConcatTransforms( adjmatrix, bonematrix, bonematrix );
				MatrixAngles( bonematrix, panim->sanim[j+iStartFrame][k].rot, panim->sanim[j+iStartFrame][k].pos );
			}
		}
	}

	// create piecewise motion paths

	s_linearmove_t *pmove = &panim->piecewisemove[panim->numpiecewisekeys++];

	pmove->endframe = iEndFrame;

	pmove->flags = motiontype;

	// concatinate xforms
	if (panim->numpiecewisekeys > 1)
	{
		AngleMatrix( adjangle, adjpos, bonematrix );
		AngleMatrix( pmove[-1].rot, pmove[-1].pos, adjmatrix );
		ConcatTransforms( adjmatrix, bonematrix, bonematrix );
		MatrixAngles( bonematrix, pmove[0].rot, pmove[0].pos );
		pmove->vector = pmove[0].pos - pmove[-1].pos;
	}
	else
	{
		VectorCopy( adjpos, pmove[0].pos );
		VectorCopy( adjangle, pmove[0].rot );
		pmove->vector = pmove[0].pos;
	}
	VectorNormalize( pmove->vector );

	// printf("%d : %.1f %.1f %.1f\n", iEndFrame, pmove[0].pos.x, pmove[0].pos.y, RAD2DEG( pmove[0].rot.z ) ); 

	pmove->v0 = v0;
	pmove->v1 = v1;
}



//-----------------------------------------------------------------------------
// Purpose: process the "piecewise movement" commands and return where the animation
//			would move to on a given frame (assuming frame 0 is at the origin)
//-----------------------------------------------------------------------------

Vector calcPosition( s_animation_t *panim, int iFrame )
{
	Vector vecPos;
	
	vecPos.Init();

	if (panim->numpiecewisekeys == 0)
		return vecPos;

	int iLoops = 0;
	if (iFrame > panim->numframes - 1)
	{
		iLoops = 1;
		iFrame = iFrame - panim->numframes;
	}

	float	prevframe = 0.0f;

	for (int i = 0; i < panim->numpiecewisekeys; i++)
	{
		s_linearmove_t *pmove = &panim->piecewisemove[i];

		if (pmove->endframe >= iFrame)
		{
			float f = (iFrame - prevframe) / (pmove->endframe - prevframe);

			float d = pmove->v0 * f + 0.5 * (pmove->v1 - pmove->v0) * f * f;

			vecPos = vecPos + d * pmove->vector;
			if (iLoops != 0)
			{
				s_linearmove_t *pmove = &panim->piecewisemove[panim->numpiecewisekeys - 1];
				vecPos = vecPos + iLoops * pmove->pos; 
			}
			return vecPos;
		}
		else
		{
			prevframe = pmove->endframe;
			vecPos = pmove->pos;
		}
	}
	return vecPos;
}


//-----------------------------------------------------------------------------
// Purpose: calculate how far an animation travels between two frames
//-----------------------------------------------------------------------------

Vector calcMovement( s_animation_t *panim, int iFrom, int iTo )
{
	Vector p1 = calcPosition( panim, iFrom );
	Vector p2 = calcPosition( panim, iTo );

	return p2 - p1;
}

#if 0
	// FIXME: add in correct motion!!!
	int iFrame = pRule->peak - pRule->start - k;
	if (pRule->start + k > panim->numframes - 1)
	{
		iFrame = iFrame + 1;
	}
	Vector pos = footfall;
	if (panim->numframes > 1)
		pos = pos + panim->piecewisemove[0].pos * (iFrame) / (panim->numframes - 1.0f);
#endif

	
//-----------------------------------------------------------------------------
// Purpose: try to calculate a "missing" frame of animation, i.e the overlapping frame
//-----------------------------------------------------------------------------

void fixupMissingFrame( s_animation_t *panim )
{
	// the animations DIDN'T have the end frame the same as the start frame, so fudge it
	int size = g_numbones * sizeof( s_bone_t );
	int j = panim->numframes;

	float scale = 1 / (j - 1.0f);

	panim->sanim[j] = (s_bone_t *)kalloc( 1, size );

	Vector deltapos;

	for (int k = 0; k < g_numbones; k++)
	{
		VectorSubtract( panim->sanim[j-1][k].pos, panim->sanim[0][k].pos, deltapos );
		VectorMA( panim->sanim[j-1][k].pos, scale, deltapos, panim->sanim[j][k].pos );
		VectorCopy( panim->sanim[0][k].rot, panim->sanim[j][k].rot );
	}

	panim->numframes = j + 1;
}


//-----------------------------------------------------------------------------
// Purpose: shift the frames of the animation so that it starts on the desired frame
//-----------------------------------------------------------------------------

void realignLooping( s_animation_t *panim )
{	
	int j, k;

	// realign looping animations
	if (panim->numframes > 1 && panim->looprestart)
	{
		for (k = 0; k < g_numbones; k++)
		{
			int n;

			Vector	shiftpos[MAXSTUDIOANIMATIONS];
			RadianEuler	shiftrot[MAXSTUDIOANIMATIONS];

			// printf("%f %f %f\n", motion[0], motion[1], motion[2] );
			for (j = 0; j < panim->numframes - 1; j++)
			{	
				n = (j + panim->looprestart) % (panim->numframes - 1);
				VectorCopy( panim->sanim[n][k].pos, shiftpos[j] );
				VectorCopy( panim->sanim[n][k].rot, shiftrot[j] );
			}

			n = panim->looprestart;
			j = panim->numframes - 1;
			VectorCopy( panim->sanim[n][k].pos, shiftpos[j] );
			VectorCopy( panim->sanim[n][k].rot, shiftrot[j] );

			for (j = 0; j < panim->numframes; j++)
			{	
				VectorCopy( shiftpos[j], panim->sanim[j][k].pos );
				VectorCopy( shiftrot[j], panim->sanim[j][k].rot );
			}
		}
	}
}

void extractUnusedMotion( s_animation_t *panim )
{
	int j, k;

	int type = panim->motiontype;

	for (k = 0; k < g_numbones; k++)
	{
		if (g_bonetable[k].parent == -1)
		{
			float	motion[6];
			motion[0] = panim->sanim[0][k].pos[0];
			motion[1] = panim->sanim[0][k].pos[1];
			motion[2] = panim->sanim[0][k].pos[2];
			motion[3] = panim->sanim[0][k].rot[0];
			motion[4] = panim->sanim[0][k].rot[1];
			motion[5] = panim->sanim[0][k].rot[2];

			for (j = 0; j < panim->numframes; j++)
			{	
				if (type & STUDIO_X)
					panim->sanim[j][k].pos[0] = motion[0];
				if (type & STUDIO_Y)
					panim->sanim[j][k].pos[1] = motion[1];
				if (type & STUDIO_Z)
					panim->sanim[j][k].pos[2] = motion[2];
				if (type & STUDIO_XR)
					panim->sanim[j][k].rot[0] = motion[3];
				if (type & STUDIO_YR)
					panim->sanim[j][k].rot[1] = motion[4];
				if (type & STUDIO_ZR)
					panim->sanim[j][k].rot[2] = motion[5];
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: find the difference between the src and dest animations, then add that
//			difference to all the frames of the dest animation.
//-----------------------------------------------------------------------------

void processMatch( s_animation_t *psrc, s_animation_t *pdest )
{
	int j, k;

	// process "match"
	Vector delta_pos[MAXSTUDIOSRCBONES];
	Quaternion delta_q[MAXSTUDIOSRCBONES];

	for (k = 0; k < g_numbones; k++)
	{
		VectorSubtract( psrc->sanim[0][k].pos, pdest->sanim[0][k].pos, delta_pos[k] );
		QuaternionSM( -1, pdest->sanim[0][k].rot, psrc->sanim[0][k].rot, delta_q[k] );
	}

	// printf("%.2f %.2f %.2f\n", adj.x, adj.y, adj.z );
	for (j = 0; j < pdest->numframes; j++)
	{
		for (k = 0; k < g_numbones; k++)
		{
			if (pdest->weight[k] > 0)
			{
				VectorAdd( pdest->sanim[j][k].pos, delta_pos[k], pdest->sanim[j][k].pos );
				QuaternionMAAngles( pdest->sanim[j][k].rot, 1.0, delta_q[k], pdest->sanim[j][k].rot );
			}
		}	
	}
}


//-----------------------------------------------------------------------------
// Purpose: match one animations position/orientation to another animations position/orientation
//-----------------------------------------------------------------------------

void processAutoorigin( s_animation_t *psrc, s_animation_t *pdest, int motiontype, int srcframe, int destframe )
{	
	int j, k;
	matrix3x4_t adjmatrix;

	// find rotation
	RadianEuler	rot( 0, 0, 0 );

	if (motiontype & (STUDIO_LXR | STUDIO_LYR | STUDIO_LZR | STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		Quaternion q0;
		Quaternion q2;

		AngleQuaternion( psrc->sanim[srcframe][g_rootIndex].rot, q0 );
		AngleQuaternion( pdest->sanim[destframe][g_rootIndex].rot, q2 );

		Quaternion deltaQ2;
		QuaternionMA( q2, -1, q0, deltaQ2 );

		// FIXME: this is still wrong, but it should be slightly more robust
		RadianEuler a3;
		if (motiontype & (STUDIO_LXR | STUDIO_XR))
		{
			Quaternion q4;
			q4.Init( deltaQ2.x, 0, 0, deltaQ2.w );
			QuaternionNormalize( q4 );
			QuaternionAngles( q4, a3 );
			rot.x = a3.x;
		}
		if (motiontype & (STUDIO_LYR | STUDIO_YR))
		{
			Quaternion q4;
			q4.Init( 0, deltaQ2.y, 0, deltaQ2.w );
			QuaternionNormalize( q4 );
			QuaternionAngles( q4, a3 );
			rot.y = a3.y;
		}
		if (motiontype & (STUDIO_LZR | STUDIO_ZR))
		{
			Quaternion q4;
			q4.Init( 0, 0, deltaQ2.z, deltaQ2.w );
			QuaternionNormalize( q4 );
			QuaternionAngles( q4, a3 );
			rot.z = a3.z;
		}
	}


	// find movement
	Vector p0 = psrc->sanim[srcframe][g_rootIndex].pos;
	Vector p2;
	AngleMatrix(rot, adjmatrix );
	VectorRotate( pdest->sanim[destframe][g_rootIndex].pos, adjmatrix, p2 );

	Vector adj = p0 - p2;

	if (!(motiontype & (STUDIO_X | STUDIO_LX)))
		adj.x = 0;
	if (!(motiontype & (STUDIO_Y | STUDIO_LY)))
		adj.y = 0;
	if (!(motiontype & (STUDIO_Z | STUDIO_LZ)))
		adj.z = 0;

	PositionMatrix( adj, adjmatrix );
	// printf("%.2f %.2f %.2f\n", adj.x, adj.y, adj.z );

	for (k = 0; k < g_numbones; k++)
	{
		if (g_bonetable[k].parent == -1)
		{
			for (j = 0; j < pdest->numframes; j++)
			{
				matrix3x4_t bonematrix;
				AngleMatrix( pdest->sanim[j][k].rot, pdest->sanim[j][k].pos, bonematrix );
				ConcatTransforms( adjmatrix, bonematrix, bonematrix );
				MatrixAngles( bonematrix, pdest->sanim[j][k].rot, pdest->sanim[j][k].pos );
			}
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: subtract one animaiton from animation to create an animation of the "difference"
//-----------------------------------------------------------------------------

void subtractBaseAnimations( s_animation_t *psrc, s_animation_t *pdest, int srcframe, int flags )
{
	int j, k;

 	// create delta animations
	s_bone_t src[MAXSTUDIOSRCBONES];

	for (k = 0; k < g_numbones; k++)
	{
		VectorCopy( psrc->sanim[srcframe][k].pos, src[k].pos );
		VectorCopy( psrc->sanim[srcframe][k].rot, src[k].rot );
	}

	for (k = 0; k < g_numbones; k++)
	{
		for (j = 0; j < pdest->numframes; j++)
		{	
			if (pdest->weight[k] > 0)
			{
				/*
				printf("%2d : %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
					k,
					src[k].pos[0], src[k].pos[1], src[k].pos[2], 
					src[k].rot[0], src[k].rot[1], src[k].rot[2] ); 

				printf("     %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
					RAD2DEG(pdest->sanim[j][k].pos[0]), RAD2DEG(pdest->sanim[j][k].pos[1]), RAD2DEG(pdest->sanim[j][k].pos[2]), 
					RAD2DEG(pdest->sanim[j][k].rot[0]), RAD2DEG(pdest->sanim[j][k].rot[1]), RAD2DEG(pdest->sanim[j][k].rot[2]) ); 
				*/

				// calc differences between two rotations
				if (flags & STUDIO_POST)
				{
					// find pdest in src's reference frame  
					QuaternionSMAngles( -1, src[k].rot, pdest->sanim[j][k].rot, pdest->sanim[j][k].rot );
					VectorSubtract( pdest->sanim[j][k].pos, src[k].pos, pdest->sanim[j][k].pos );
				}
				else
				{
					// find src in pdest's reference frame?
					QuaternionMAAngles( pdest->sanim[j][k].rot, -1, src[k].rot, pdest->sanim[j][k].rot );
					VectorSubtract( src[k].pos, pdest->sanim[j][k].pos, pdest->sanim[j][k].pos );
				}

				/*
				printf("     %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
					pdest->sanim[j][k].pos[0], pdest->sanim[j][k].pos[1], pdest->sanim[j][k].pos[2], 
					RAD2DEG(pdest->sanim[j][k].rot[0]), RAD2DEG(pdest->sanim[j][k].rot[1]), RAD2DEG(pdest->sanim[j][k].rot[2]) ); 
				*/
			}
		}
	}

#if 0
	// cleanup weightlists
	for (k = 0; k < g_numbones; k++)
	{
		panim->weight[k] = 0.0;
	}

	for (k = 0; k < g_numbones; k++)
	{
		if (g_weightlist[panim->weightlist].weight[k] > 0.0)
		{
			for (j = 0; j < panim->numframes; j++)
			{	
				if (fabs(panim->sanim[j][k].pos[0]) > 0.001 || 
					fabs(panim->sanim[j][k].pos[1]) > 0.001 || 
					fabs(panim->sanim[j][k].pos[2]) > 0.001 || 
					fabs(panim->sanim[j][k].rot[0]) > 0.001 || 
					fabs(panim->sanim[j][k].rot[1]) > 0.001 || 
					fabs(panim->sanim[j][k].rot[2]) > 0.001)
				{
					panim->weight[k] = g_weightlist[panim->weightlist].weight[k];
					break;
				}
			}
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void QuaternionSlerp( const RadianEuler &r0, const RadianEuler &r1, float t, RadianEuler &r2 )
{
	Quaternion q0, q1, q2;
	AngleQuaternion( r0, q0 );
	AngleQuaternion( r1, q1 );
	QuaternionSlerp( q0, q1, t, q2 );
	QuaternionAngles( q2, r2 );
}


//-----------------------------------------------------------------------------
// Purpose: subtract each frame running interpolation of the first frame to the last frame
//-----------------------------------------------------------------------------

void linearDelta( s_animation_t *psrc, s_animation_t *pdest, int srcframe, int flags )
{
	int j, k;

 	// create delta animations
	s_bone_t src0[MAXSTUDIOSRCBONES];
	s_bone_t src1[MAXSTUDIOSRCBONES];

	for (k = 0; k < g_numbones; k++)
	{
		VectorCopy( psrc->sanim[0][k].pos, src0[k].pos );
		VectorCopy( psrc->sanim[0][k].rot, src0[k].rot );
		VectorCopy( psrc->sanim[srcframe][k].pos, src1[k].pos );
		VectorCopy( psrc->sanim[srcframe][k].rot, src1[k].rot );
	}

	for (k = 0; k < g_numbones; k++)
	{
		for (j = 0; j < pdest->numframes; j++)
		{	
			float s = (float)j / (pdest->numframes - 1);

			// make it a spline curve
			if (flags & STUDIO_SPLINE)
			{
				s = 3 * s * s - 2 * s * s * s;
			}

			if (pdest->weight[k] > 0)
			{
				/*
				printf("%2d : %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
					k,
					src[k].pos[0], src[k].pos[1], src[k].pos[2], 
					src[k].rot[0], src[k].rot[1], src[k].rot[2] ); 

				printf("     %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
					RAD2DEG(pdest->sanim[j][k].pos[0]), RAD2DEG(pdest->sanim[j][k].pos[1]), RAD2DEG(pdest->sanim[j][k].pos[2]), 
					RAD2DEG(pdest->sanim[j][k].rot[0]), RAD2DEG(pdest->sanim[j][k].rot[1]), RAD2DEG(pdest->sanim[j][k].rot[2]) ); 
				*/

				s_bone_t src;

				src.pos = src0[k].pos * (1 - s) + src1[k].pos * s;
				QuaternionSlerp( src0[k].rot, src1[k].rot, s, src.rot );

				// calc differences between two rotations
				if (flags & STUDIO_POST)
				{
					// find pdest in src's reference frame  
					QuaternionSMAngles( -1, src.rot, pdest->sanim[j][k].rot, pdest->sanim[j][k].rot );
					VectorSubtract( pdest->sanim[j][k].pos, src.pos, pdest->sanim[j][k].pos );
				}
				else
				{
					// find src in pdest's reference frame?
					QuaternionMAAngles( pdest->sanim[j][k].rot, -1, src.rot, pdest->sanim[j][k].rot );
					VectorSubtract( src.pos, pdest->sanim[j][k].pos, pdest->sanim[j][k].pos );
				}

				/*
				printf("     %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
					pdest->sanim[j][k].pos[0], pdest->sanim[j][k].pos[1], pdest->sanim[j][k].pos[2], 
					RAD2DEG(pdest->sanim[j][k].rot[0]), RAD2DEG(pdest->sanim[j][k].rot[1]), RAD2DEG(pdest->sanim[j][k].rot[2]) ); 
				*/
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn the animation into a lower fps encoded version
//-----------------------------------------------------------------------------

void reencodeAnimation( s_animation_t *panim, int frameskip )
{
	int j, k, n;

	n = 1;
	for (j = frameskip; j < panim->numframes; j += frameskip)
	{	
		for (k = 0; k < g_numbones; k++)
		{
			panim->sanim[n][k] = panim->sanim[j][k];
		}
		n++;
	}
	panim->numframes = n;

	panim->fps = panim->fps / frameskip;
}


//-----------------------------------------------------------------------------
// Purpose: subtract each frame from the previous to calculate the animations derivative
//-----------------------------------------------------------------------------

void forceNumframes( s_animation_t *panim, int numframes )
{
	int j;

	clearAnimations( panim );

	// FIXME: evil
	for (j = 1; j < numframes; j++)
	{	
		panim->sanim[j] = panim->sanim[0];
	}

	panim->numframes = numframes;
}


//-----------------------------------------------------------------------------
// Purpose: subtract each frame from the previous to calculate the animations derivative
//-----------------------------------------------------------------------------

void createDerivative( s_animation_t *panim, float scale )
{
	int j, k;

	s_bone_t orig[MAXSTUDIOSRCBONES];

	j = panim->numframes - 1;
	if (panim->flags & STUDIO_LOOPING)
	{
		j--;
	}

	for (k = 0; k < g_numbones; k++)
	{
		VectorCopy( panim->sanim[j][k].pos, orig[k].pos );
		VectorCopy( panim->sanim[j][k].rot, orig[k].rot );
	}

	for (j = panim->numframes - 1; j >= 0; j--)
	{	
		s_bone_t *psrc;
		s_bone_t *pdest;

		if (j - 1 >= 0)
		{
			psrc = panim->sanim[j-1];
		}
		else
		{
			psrc = orig;
		}
		pdest = panim->sanim[j];

		for (k = 0; k < g_numbones; k++)
		{
			if (panim->weight[k] > 0)
			{
				/*
				{
					printf("%2d : %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
						k,
						psrc[k].pos[0], psrc[k].pos[1], psrc[k].pos[2], 
						RAD2DEG(psrc[k].rot[0]), RAD2DEG(psrc[k].rot[1]), RAD2DEG(psrc[k].rot[2]) );

					printf("     %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
						pdest[k].pos[0], pdest[k].pos[1], pdest[k].pos[2], 
						RAD2DEG(pdest[k].rot[0]), RAD2DEG(pdest[k].rot[1]), RAD2DEG(pdest[k].rot[2]) );
				}
				*/

				// find pdest in src's reference frame  
				QuaternionSMAngles( -1, psrc[k].rot, pdest[k].rot, pdest[k].rot );
				VectorSubtract( pdest[k].pos, psrc[k].pos, pdest[k].pos );

				// rescale results (not sure what basis physics system is expecting)
				{
					// QuaternionScale( pdest[k].rot, scale, pdest[k].rot );
					Quaternion q;
					AngleQuaternion( pdest[k].rot, q );
					QuaternionScale( q, scale, q );
					QuaternionAngles( q, pdest[k].rot );
					VectorScale( pdest[k].pos, scale, pdest[k].pos );
				}

				/*
				{
					printf("     %7.2f %7.2f %7.2f  %7.2f %7.2f %7.2f\n", 
						pdest[k].pos[0], pdest[k].pos[1], pdest[k].pos[2], 
						RAD2DEG(pdest[k].rot[0]), RAD2DEG(pdest[k].rot[1]), RAD2DEG(pdest[k].rot[2]) ); 
				}
				*/
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: subtract each frame from the previous to calculate the animations derivative
//-----------------------------------------------------------------------------

void clearAnimations( s_animation_t *panim )
{
	panim->flags |= STUDIO_DELTA;
	panim->flags |= STUDIO_ALLZEROS;
	
	panim->numframes = 1;
	panim->startframe = 0;
	panim->endframe = 1;
	
	int k;

	for (k = 0; k < g_numbones; k++)
	{
		panim->sanim[0][k].pos = Vector( 0, 0, 0 );
		panim->sanim[0][k].rot = RadianEuler( 0, 0, 0 );
		panim->weight[k] = 0.0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: build transforms in source space, assuming source bones
//-----------------------------------------------------------------------------
void BuildRawTransforms( s_source_t const *psource, int frame, float scale, Vector const &shift, RadianEuler const &rotate, matrix3x4_t* boneToWorld )
{
	int k;
	Vector tmp;
	Vector pos;
	RadianEuler rot;
	matrix3x4_t bonematrix;
	
	matrix3x4_t rootxform;

	AngleMatrix( rotate, rootxform );

	// build source space local to world transforms
	for (k = 0; k < psource->numbones; k++)
	{
		VectorScale( psource->rawanim[frame][k].pos, scale, pos );
		VectorCopy( psource->rawanim[frame][k].rot, rot );

		if (psource->localBone[k].parent == -1)
		{
			// translate
			VectorSubtract( pos, shift, tmp );

			// rotate
			VectorRotate( tmp, rootxform, pos );

			matrix3x4_t m;
			AngleMatrix( rot, m );
			ConcatTransforms( rootxform, m, bonematrix );
			MatrixAngles( bonematrix, rot );
			clip_rotations( rot );
		}

		AngleMatrix( rot, pos, bonematrix );

		if (psource->localBone[k].parent == -1)
		{
			MatrixCopy( bonematrix, boneToWorld[k] );
		}
		else
		{
			ConcatTransforms( boneToWorld[psource->localBone[k].parent], bonematrix, boneToWorld[k] );
			// ConcatTransforms( worldToBone[psource->localBone[k].parent], boneToWorld[k], bonematrix );
			// B * C => A
			// C <= B-1 * A
		}
	}
	
	
}


void BuildRawTransforms( s_source_t const *psource, int frame, matrix3x4_t* boneToWorld )
{
	BuildRawTransforms( psource, frame, 1.0f, Vector( 0, 0, 0 ), RadianEuler( 0, 0, 0 ), boneToWorld );
}


//-----------------------------------------------------------------------------
// Purpose: convert source bone animation into global bone animation
//-----------------------------------------------------------------------------
void TranslateAnimations( s_source_t const *psource, const matrix3x4_t *srcBoneToWorld, matrix3x4_t *destBoneToWorld )
{
	matrix3x4_t bonematrix;

	for (int k = 0; k < g_numbones; k++)
	{
		int q = psource->boneGlobalToLocal[k];
		/*
		if (g_bonetable[k].split)
		{
			q = psource->boneGlobalToLocal[k+1];
		}
		*/

		if (q == -1)
		{
			// unknown bone, copy over defaults
			if (g_bonetable[k].parent >= 0)
			{
				AngleMatrix( g_bonetable[k].rot, g_bonetable[k].pos, bonematrix );
				ConcatTransforms( destBoneToWorld[g_bonetable[k].parent], bonematrix, destBoneToWorld[k] );
			}
			else
			{
				AngleMatrix( g_bonetable[k].rot, g_bonetable[k].pos, destBoneToWorld[k] );
			}
		}
		else
		{
			ConcatTransforms( srcBoneToWorld[q], g_bonetable[k].srcRealign, destBoneToWorld[k] );

			/*
			if (g_bonetable[k].split)
			{
				matrix3x4_t worldToBone;

				// convert my transform into parent relative space
				MatrixInvert( destBoneToWorld[g_bonetable[k].parent], worldToBone );
				ConcatTransforms( worldToBone, destBoneToWorld[k], bonematrix );

				// turn it into half of delta from base position
				Quaternion q1, q2, q3;
				Vector p1, p2;
				MatrixAngles( bonematrix, q1, p1 );
				//QAngle ang;
				//QuaternionAngles( q1, ang );
				//printf( "%.1f %.1f %.1f : ", ang.x, ang.y, ang.z );

				MatrixAngles( g_bonetable[k].rawLocal, q2, p2 );

				// QuaternionSlerp( q2, q1, 0.5, q3 );

				// QuaternionAngles( q3, ang );
				// printf( "%.1f %.1f %.1f\n", ang.x, ang.y, ang.z );
				Quaternion q4;
				QuaternionMA( q1, -1.0, q2, q3 );
				QuaternionScale( q3, 0.5, q3 );
				QuaternionMult( q2, q3, q4 );

				QuaternionMatrix( q3, p1 * 0.5, bonematrix );

				ConcatTransforms( destBoneToWorld[g_bonetable[k].parent], bonematrix, destBoneToWorld[k] );
			}
			*/
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: convert source bone animation into global bone animation
//-----------------------------------------------------------------------------
void ConvertAnimation( s_source_t const *psource, int frame, float scale, Vector const &shift, RadianEuler const &rotate, s_bone_t *dest )
{
	int k;
	matrix3x4_t srcBoneToWorld[MAXSTUDIOSRCBONES];
	//matrix3x4_t srcWorldToBone[MAXSTUDIOSRCBONES];
	matrix3x4_t destBoneToWorld[MAXSTUDIOSRCBONES];
	matrix3x4_t destWorldToBone[MAXSTUDIOSRCBONES];

	matrix3x4_t bonematrix;

	BuildRawTransforms( psource, frame, scale, shift, rotate, srcBoneToWorld );

	/*
	for (k = 0; k < psource->numbones; k++)
	{
		MatrixInvert( srcBoneToWorld[k], srcWorldToBone[k] );
	}
	*/

	TranslateAnimations( psource, srcBoneToWorld, destBoneToWorld );

	for (k = 0; k < g_numbones; k++)
	{
		MatrixInvert( destBoneToWorld[k], destWorldToBone[k] );
	}

	// convert source_space_local_to_world transforms to shared_space_local_to_world transforms
	for (k = 0; k < g_numbones; k++)
	{
		if (g_bonetable[k].parent == -1)
		{
			MatrixCopy( destBoneToWorld[k], bonematrix );
		}
		else
		{
			// convert my transform into parent relative space
			ConcatTransforms( destWorldToBone[g_bonetable[k].parent], destBoneToWorld[k], bonematrix );

			// printf("%s : %s\n", psource->localBone[q2].name, psource->localBone[q].name );

			// B * C => A
			// C <= B-1 * A
		}

		MatrixAngles( bonematrix, dest[k].rot, dest[k].pos );
		
		clip_rotations( dest[k].rot );
	}
}

//-----------------------------------------------------------------------------
// Purpose: copy the raw animation data from the source files into the individual animations
//-----------------------------------------------------------------------------
void RemapAnimations(void)
{
	int i, j;

	// copy source animations
	for (i = 0; i < g_numani; i++)
	{
		s_animation_t *panim = g_panimation[i];

		s_source_t *psource = panim->source;

		int size = g_numbones * sizeof( s_bone_t );

		int n = panim->startframe - psource->startframe;
		// printf("%s %d:%d\n", g_panimation[i]->filename, g_panimation[i]->startframe, psource->startframe );
		for (j = 0; j < panim->numframes; j++)
		{
			panim->sanim[j] = (s_bone_t *)kalloc( 1, size );

			ConvertAnimation( psource, n + j, panim->scale, panim->adjust, panim->rotation, panim->sanim[j] );
		}
	}
}

void buildAnimationWeights()
{
	int i, j, k;

	// initialize weightlist 0
	for (j = 0; j < g_numbones; j++)
	{
		g_weightlist[0].weight[j] = 1.0;
	}

	// rlink animation weights
	for (i = 1; i < g_numweightlist; i++)
	{
		// initialize weights
		for (j = 0; j < g_numbones; j++)
		{
			if (g_bonetable[j].parent != -1)
			{
				// set child bones to uninitialized
				g_weightlist[i].weight[j] = -1;
			}
			else
			{
				// set root bones to 0
				g_weightlist[i].weight[j] = 0;
			}
		}

		// match up weights
		for (j = 0; j < g_weightlist[i].numbones; j++)
		{
			k = findGlobalBone( g_weightlist[i].bonename[j] );
			if (k == -1)
			{
				Error("unknown bone reference '%s' in weightlist '%s'\n", g_weightlist[i].bonename[j], g_weightlist[i].name );
			}
			g_weightlist[i].weight[k] = g_weightlist[i].boneweight[j];
		}

		// copy weights forward
		for (j = 0; j < g_numbones; j++)
		{
			if (g_weightlist[i].weight[j] < 0.0)
			{
				if (g_bonetable[j].parent != -1)
				{
					g_weightlist[i].weight[j] = g_weightlist[i].weight[g_bonetable[j].parent];
				}
			}
		}
	}
}

void setAnimationWeight( s_animation_t *panim, int index )
{
	// copy weightlists to animations
	for (int k = 0; k < g_numbones; k++)
	{
		panim->weight[k] = g_weightlist[index].weight[k];
	}
}

void addDeltas( s_animation_t *panim, int frame, float s, Vector delta_pos[], Quaternion delta_q[] )
{
	for (int k = 0; k < g_numbones; k++)
	{
		if (panim->weight[k] > 0)
		{
			QuaternionSMAngles( s, delta_q[k], panim->sanim[frame][k].rot, panim->sanim[frame][k].rot );
			VectorMA( panim->sanim[frame][k].pos, s, delta_pos[k], panim->sanim[frame][k].pos );
		}
	}
}


void fixupLoopingDiscontinuities( s_animation_t *panim, int start, int end )
{
	int j, k, m, n;

	// fix C0 errors on looping animations
	m = panim->numframes - 1;

	Vector delta_pos[MAXSTUDIOSRCBONES];
	Quaternion delta_q[MAXSTUDIOSRCBONES];

	for (k = 0; k < g_numbones; k++)
	{
		VectorSubtract( panim->sanim[m][k].pos, panim->sanim[0][k].pos, delta_pos[k] );
		QuaternionMA( panim->sanim[m][k].rot, -1, panim->sanim[0][k].rot, delta_q[k] );
		QAngle ang;
		QuaternionAngles( delta_q[k], ang );
		// printf("%2d  %.1f %.1f %.1f\n", k, ang.x, ang.y, ang.z );
	}

	// FIXME: figure out S
	float s = 0;
	float nf = end - start;
	
	for (j = start + 1; j <= 0; j++)
	{	
		n = j - start;
		s = (n / nf);
		s = 3 * s * s - 2 * s * s * s;
		// printf("%d : %d (%lf)\n", m+j, n, -s );
		addDeltas( panim, m+j, -s, delta_pos, delta_q );
	}

	for (j = 0; j < end; j++)
	{	
		n = end - j;
		s = (n / nf);
		s = 3 * s * s - 2 * s * s * s;
		//printf("%d : %d (%lf)\n", j, n, s );
		addDeltas( panim, j, s, delta_pos, delta_q );
	}
}

void forceAnimationLoop( s_animation_t *panim )
{
	int k, m, n;

	// force looping animations to be looping
	if (panim->flags & STUDIO_LOOPING)
	{
		n = 0;
		m = panim->numframes - 1;

		for (k = 0; k < g_numbones; k++)
		{
			int type = panim->motiontype;

			if (!(type & STUDIO_LX))
				panim->sanim[m][k].pos[0] = panim->sanim[n][k].pos[0];
			if (!(type & STUDIO_LY))
				panim->sanim[m][k].pos[1] = panim->sanim[n][k].pos[1];
			if (!(type & STUDIO_LZ))
				panim->sanim[m][k].pos[2] = panim->sanim[n][k].pos[2];

			if (!(type & STUDIO_LXR))
				panim->sanim[m][k].rot[0] = panim->sanim[n][k].rot[0];
			if (!(type & STUDIO_LYR))
				panim->sanim[m][k].rot[1] = panim->sanim[n][k].rot[1];
			if (!(type & STUDIO_LZR))
				panim->sanim[m][k].rot[2] = panim->sanim[n][k].rot[2];
		}
	}

	// printf("\n");
}


void makeAngle( s_animation_t *panim, float angle )
{
	float da = 0.0f;

	Vector pos = panim->piecewisemove[panim->numpiecewisekeys-1].pos;
	if (pos[0] != 0 || pos[1] != 0)
	{
		float a = atan2( pos[1], pos[0] ) * (180 / M_PI);
		da = angle - a;
	}

	/*
	if (da > -0.01 && da < 0.01)
		return;
	*/

	matrix3x4_t rootxform;
	matrix3x4_t src;
	matrix3x4_t dest;

	for (int i = 0; i < panim->numpiecewisekeys; i++)
	{
		VectorYawRotate( panim->piecewisemove[i].pos, da, panim->piecewisemove[i].pos );
		VectorYawRotate( panim->piecewisemove[i].vector, da, panim->piecewisemove[i].vector );
	}

	AngleMatrix( QAngle( 0, da, 0), rootxform );

	for (int j = 0; j < panim->numframes; j++)
	{
		for (int k = 0; k < g_numbones; k++)
		{
			if (g_bonetable[k].parent == -1)
			{
				AngleMatrix( panim->sanim[j][k].rot, panim->sanim[j][k].pos, src );
				ConcatTransforms( rootxform, src, dest );
				MatrixAngles( dest, panim->sanim[j][k].rot, panim->sanim[j][k].pos );
			}
		}
	}

	// FIXME: not finished
}


void solveBone( 
	s_animation_t *panim,
	int iFrame,
	int	iBone,
	matrix3x4_t* pBoneToWorld
	)
{
	int iParent = g_bonetable[iBone].parent;

	matrix3x4_t worldToBone;
	MatrixInvert( pBoneToWorld[iParent], worldToBone );

	matrix3x4_t local;
	ConcatTransforms( worldToBone, pBoneToWorld[iBone], local );

	iFrame = iFrame % panim->numframes;

	MatrixAngles( local, panim->sanim[iFrame][iBone].rot, panim->sanim[iFrame][iBone].pos );
}


//-----------------------------------------------------------------------------
// Purpose: calc the influence of a ik rule for a specific point in the animation cycle
//-----------------------------------------------------------------------------

float IKRuleWeight( s_ikrule_t *pRule, float flCycle )
{
	if (pRule->end > 1.0f && flCycle < pRule->start)
	{
		flCycle = flCycle + 1.0f;
	}

	float value = 0.0f;
	if (flCycle < pRule->start)
	{
		return 0.0f;
	}
	else if (flCycle < pRule->peak )
	{
		value = (flCycle - pRule->start) / (pRule->peak - pRule->start);
	}
	else if (flCycle < pRule->tail )
	{
		return 1.0f;
	}
	else if (flCycle < pRule->end )
	{
		value = 1.0f - ((flCycle - pRule->tail) / (pRule->end - pRule->tail));
	}
	return 3.0f * value * value - 2.0f * value * value * value;
}


//-----------------------------------------------------------------------------
// Purpose: Lock the ik target to a specific location in order to clean up bad animations (shouldn't be needed).
//-----------------------------------------------------------------------------
void fixupIKErrors( s_animation_t *panim, s_ikrule_t *pRule )
{
	int k;

	if (pRule->start == 0 && pRule->peak == 0 && pRule->tail == 0 && pRule->end == 0)
	{
		pRule->tail = panim->numframes - 1;
		pRule->end = panim->numframes - 1;
	}

	// check for wrapping
	if (pRule->peak < pRule->start)
	{
		pRule->peak += panim->numframes - 1;
	}
	if (pRule->tail < pRule->peak)
	{
		pRule->tail += panim->numframes - 1;
	}
	if (pRule->end < pRule->tail)
	{
		pRule->end += panim->numframes - 1;
	}

	if (pRule->contact == -1)
	{
		pRule->contact = pRule->peak;
	}

	pRule->numerror = pRule->end - pRule->start + 1;
	if (pRule->end >= panim->numframes)
		pRule->numerror = pRule->numerror + 2;

	switch( pRule->type )
	{
	case IK_SELF:
#if 0
		// this code has never been run.....
		{
			matrix3x4_t boneToWorld[MAXSTUDIOBONES];
			matrix3x4_t worldToBone;
			matrix3x4_t local;
			Vector targetPos;
			Quaternion targetQuat;

			pRule->bone = findGlobalBone( pRule->bonename );
			if (pRule->bone == -1)
			{
				Error("unknown bone '%s' in ikrule\n", pRule->bonename );
			}

			matrix3x4_t srcBoneToWorld[MAXSTUDIOBONES];
			BuildRawTransforms( panim->source, pRule->contact + panim->startframe - panim->source->startframe, srcBoneToWorld );
			TranslateAnimations( panim->source, srcBoneToWorld, boneToWorld );

			MatrixInvert( boneToWorld[pRule->bone], worldToBone );
			ConcatTransforms( worldToBone, boneToWorld[g_ikchain[pRule->chain].link[2]], local );
			MatrixAngles( local, targetQuat, targetPos );

			for (k = 0; k < pRule->numerror; k++)
			{
				BuildRawTransforms( panim->source, k + pRule->start + panim->startframe - panim->source->startframe, srcBoneToWorld );
				TranslateAnimations( panim->source, srcBoneToWorld, boneToWorld );

				float cycle = (panim->numframes <= 1) ? 0 : (k + pRule->start) / (panim->numframes - 1);
				float s = IKRuleWeight( pRule, cycle );

				Quaternion curQuat;
				Vector curPos;

				// convert into rule bone space
				MatrixInvert( boneToWorld[pRule->bone], worldToBone );
				ConcatTransforms( worldToBone, boneToWorld[g_ikchain[pRule->chain].link[2]], local );
				MatrixAngles( local, curQuat, curPos );

				// find blended rule bone relative position
				Vector rulePos = curPos * s + targetPos * (1.0 - s);
				Quaternion ruleQuat;
				QuaternionSlerp( curQuat, targetQuat, s, ruleQuat );
				QuaternionMatrix( ruleQuat, rulePos, local );

				Vector worldPos;
				VectorTransform( rulePos, boneToWorld[pRule->bone], worldPos );

				// printf("%d (%d) : %.1f %.1f %1.f\n", k + pRule->start, pRule->peak, pos.x, pos.y, pos.z );
				Studio_SolveIK(
					g_ikchain[pRule->chain].link[0],
					g_ikchain[pRule->chain].link[1],
					g_ikchain[pRule->chain].link[2],
					worldPos,
					boneToWorld );

				// slam final matrix
				// FIXME: this isn't taking into account the IK may have failed
				ConcatTransforms( boneToWorld[pRule->bone], local, boneToWorld[g_ikchain[pRule->chain].link[2]] );

				solveBone( panim, k + pRule->start, g_ikchain[pRule->chain].link[0], boneToWorld );  
				solveBone( panim, k + pRule->start, g_ikchain[pRule->chain].link[1], boneToWorld );  
				solveBone( panim, k + pRule->start, g_ikchain[pRule->chain].link[2], boneToWorld );  
			}
		}
#endif
		break;
	case IK_WORLD:
	case IK_GROUND:
		{
			matrix3x4_t boneToWorld[MAXSTUDIOBONES];

			int bone = g_ikchain[pRule->chain].link[2];
			CalcBoneTransforms( panim, pRule->contact, boneToWorld );
			// FIXME: add in motion

			Vector footfall;
			MatrixGetColumn( boneToWorld[bone], 3, footfall );

			for (k = 0; k < pRule->numerror; k++)
			{
				CalcBoneTransforms( panim, k + pRule->start, boneToWorld );

				float cycle = (panim->numframes <= 1) ? 0 : (k + pRule->start) / (panim->numframes - 1);
				float s = IKRuleWeight( pRule, cycle );

				Vector orig;
				MatrixPosition( boneToWorld[g_ikchain[pRule->chain].link[2]], orig );

				Vector pos = (footfall + calcMovement( panim, k + pRule->start, pRule->contact )) * s + orig * (1.0 - s);

				// printf("%d (%d) : %.1f %.1f %1.f\n", k + pRule->start, pRule->peak, pos.x, pos.y, pos.z );

				Studio_SolveIK(
					g_ikchain[pRule->chain].link[0],
					g_ikchain[pRule->chain].link[1],
					g_ikchain[pRule->chain].link[2],
					pos,
					boneToWorld );

				solveBone( panim, k + pRule->start, g_ikchain[pRule->chain].link[0], boneToWorld );  
				solveBone( panim, k + pRule->start, g_ikchain[pRule->chain].link[1], boneToWorld );  
				solveBone( panim, k + pRule->start, g_ikchain[pRule->chain].link[2], boneToWorld );  
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: map the vertex animations to their equivalent vertex in the base animations
//-----------------------------------------------------------------------------

void RemapVertexAnimations(void)
{
	int i, j, k;
	int n, m;
	s_source_t	*pvsource;	// vertex animation source
	s_source_t	*pmsource;	// original model source
	Vector		tmp;
	float		dist, minDist, dot;
	Vector		modelpos[MAXSTUDIOVERTS];
	int			imap[MAXSTUDIOVERTS];
	float 		imapdist[MAXSTUDIOVERTS];	// distance from src vert to vanim vert
	float 		imapdot[MAXSTUDIOVERTS];	// dot product of src norm to vanim normal
	int			*mapp[MAXSTUDIOVERTS*4];

	// for all the sources of flexes, find a mapping of vertex animations to base model.
	// There can be multiple "vertices" in the base model for each animated vertex since vertices 
	// are duplicated along material boundaries.
	for (i = 0; i < g_numflexkeys; i++)
	{
		pvsource = g_flexkey[i].source;

		// skip if it's already been done or if has doesn't have any animations
		if (pvsource->vanim_flag)
			continue;

		pmsource = g_model[g_flexkey[i].imodel]->source;

		// translate g_model into current frame 0 animation space
		// not possible.  In the sample the bones were deleted.  Assume they are in the correct space
		for (j = 0; j < pmsource->numvertices; j++)
		{
			// k = pmsource->boneweight[j].bone[0]; // BUG

			// NOTE: these bones have already been "fixed"
			// VectorTransform( pmsource->vertex[j], pmsource->bonefixup[k].m, modelpos[j] );
			// VectorAdd( modelpos[j], pmsource->bonefixup[k].worldorg, modelpos[j] );
			// printf("%4d  %6.2f %6.2f %6.2f\n", j, modelpos[j][0], modelpos[j][1], modelpos[j][2] );
			VectorCopy( pmsource->vertex[j], modelpos[j] );
		}

		// flag all the vertices that animate
		pvsource->vanim_flag = (int *)kalloc( pvsource->numvertices, sizeof( int ));
		for (n = i; n < g_numflexkeys; n++)
		{
			// make sure it's the current flex file and that it's not frame 0 (happens with eyeball stuff).
			if (g_flexkey[n].source == pvsource && g_flexkey[n].frame != 0)
			{
				k = g_flexkey[n].frame;
				for (m = 0; m < pvsource->numvanims[k]; m++)
				{
					pvsource->vanim_flag[pvsource->vanim[k][m].vertex] = 1;
				}
			}
		}

		// find frame 0 vertices to closest g_model vertex
		for (j = 0; j < pmsource->numvertices; j++)
		{
			imapdist[j] = 1E30;
			imapdot[j] = -1.0;
			imap[j] = -1;
		}

		for (j = 0; j < pvsource->numvertices; j++)
		{
			// don't check if it doesn't animate
			if (0 && !pvsource->vanim_flag[j])
				continue;

			minDist = 1E30;
			n = -1;
			for (k = 0; k < pmsource->numvertices; k++)
			{
				VectorSubtract( modelpos[k], pvsource->vanim[0][j].pos, tmp );
				dist = VectorLength( tmp );
				dot = DotProduct( pmsource->normal[k], pvsource->vanim[0][j].normal );
				if (dist < 0.15)
				{
					if (dist < imapdist[k] || (dist == imapdist[k] && dot >= imapdot[k]))
					{
						imapdist[k] = dist;
						imapdot[k] = dot;
						imap[k] = j;
					}
					if (dist < minDist)
					{
						minDist = dist;
						n = j;
					}
				}
			}
			if (minDist > 0.01)
			{
				// printf("vert %d dist %.4f\n", j, minDist );
				// printf("%.4f %.4f %.4f\n", pvsource->vanim[0][j].pos[0], pvsource->vanim[0][j].pos[1], pvsource->vanim[0][j].pos[2] );
			}

			// VectorSubtract( modelpos[n], pvsource->vanim[0][j].pos, matchdelta[j] );

			if (n == -1)
			{
				// printf("no match for animated vertex %d : %.4f %.4f %.4f\n", j, pvsource->vanim[0][j].pos[0], pvsource->vanim[0][j].pos[1], pvsource->vanim[0][j].pos[2] );
			}
		}
		/*
		for (j = 0; j < pmsource->numvertices; j++)
		{
			printf("%4d : %7.4f  %7.4f : %5d", j, imapdist[j], imapdot[j], imap[j] );
			printf(" : %8.4f %8.4f %8.4f", modelpos[j][0], modelpos[j][1], modelpos[j][2] );
			printf("\n");
		}
		*/

		/*
		for (j = 0; j < pmsource->numvertices; j++)
		{
			if (fabs( modelpos[j][2] - 64.36) > 0.01)
				continue;

			printf("%4d : %8.4f %8.4f %8.4f\n", j, modelpos[j][0], modelpos[j][1], modelpos[j][2] );
		}

		for (j = 0; j < pvsource->numvertices; j++)
		{
			if (!pvsource->vanim_flag[j])
				continue;

			printf("%4d : %8.2f %8.2f %8.2f : ", j, pvsource->vanim[0][j].pos[0], pvsource->vanim[0][j].pos[1], pvsource->vanim[0][j].pos[2] );
			for (k = 0; k < pmsource->numvertices; k++)
			{
				if (imap[k] == j)
					printf(" %d", k );
			}
			printf("\n");
		}
		*/

		// count mappings
		n = 0;
		pvsource->vanim_mapcount = (int *)kalloc( pvsource->numvertices, sizeof( int ));
		for (j = 0; j < pmsource->numvertices; j++)
		{
			if (imap[j] != -1)
			{
				pvsource->vanim_mapcount[imap[j]]++;
				n++;
			}
		}

		pvsource->vanim_map = (int **)kalloc( pvsource->numvertices, sizeof( int * ));
		int *vmap = (int *)kalloc( n, sizeof( int ) );
		int *vsrcmap = vmap;

		// build mapping arrays
		for (j = 0; j < pvsource->numvertices; j++)
		{
			if (pvsource->vanim_mapcount[j])
			{
				pvsource->vanim_map[j] = vmap;
				mapp[j] = vmap;
				vmap += pvsource->vanim_mapcount[j];
			}
			else if (pvsource->vanim_flag[j])
			{
				// printf("%d animates but no matching vertex\n", j );
			}
		}

		for (j = 0; j < pmsource->numvertices; j++)
		{
			if (imap[j] != -1)
			{
				*(mapp[imap[j]]++) = j;
			}
		}
	}

#if 0
	s_vertanim_t *defaultanims = NULL;

	if (g_defaultflexkey)
	{
		defaultanims = g_defaultflexkey->source->vanim[g_defaultflexkey->frame];
	}
	else
	{
		defaultanims = g_flexkey[0].source->vanim[0];
	}
#endif

	// reset model to be default animations
	if (g_defaultflexkey)
	{
		pvsource = g_defaultflexkey->source;
		pmsource = g_model[g_defaultflexkey->imodel]->source;

		int numsrcanims = pvsource->numvanims[g_defaultflexkey->frame];
		s_vertanim_t *psrcanim = pvsource->vanim[g_defaultflexkey->frame];

		for (m = 0; m < numsrcanims; m++)
		{
			if (pvsource->vanim_mapcount[psrcanim->vertex]) // bah, only do it for ones that found a match!
			{
				for (n = 0; n < pvsource->vanim_mapcount[psrcanim->vertex]; n++)
				{
					// copy "default" pos to original model
					k = pvsource->vanim_map[psrcanim->vertex][n];
					VectorCopy( psrcanim->pos, pmsource->vertex[k] );
					VectorCopy( psrcanim->normal, pmsource->normal[k] );

					// copy "default" pos to frame 0 of vertex animation source
					// FIXME: this needs to copy to all sources of vertex animation.
					// FIXME: the "default" pose needs to be in each vertex animation source since it's likely that the vertices won't be numbered the same in each file.
					VectorCopy( psrcanim->pos, pvsource->vanim[0][psrcanim->vertex].pos );
					VectorCopy( psrcanim->normal, pvsource->vanim[0][psrcanim->vertex].normal );
				}
			}
			psrcanim++;
		}
	}

#if 0
	for (i = 0; i < 300; i++)
	{
		// int x = 400 + 150 * sin( i * M_PI / 30.0 );
		// int y = 300 - 150 * cos( i * M_PI / 30.0 );
		// printf("[MOUSE_POS] %d,%d\n[DELAY] 1000\n", x, y );
		printf("[MOUSE_POS] %d,%d\n[DELAY] 1000\n", (int)((i / 300.0) * 800), 300 );
	}
#endif


	for (i = 0; i < g_numflexkeys; i++)
	{
		pvsource = g_flexkey[i].source;
		pmsource = g_model[g_flexkey[i].imodel]->source;

		int numsrcanims = pvsource->numvanims[g_flexkey[i].frame];
		s_vertanim_t *psrcanim = pvsource->vanim[g_flexkey[i].frame];

		// frame 0 is special.  Always assume zero vertex animations
		if (g_flexkey[i].frame == 0)
		{
			numsrcanims = 0;
		}

		// count total possible remapped animations
		int numdestanims = 0;
		for (m = 0; m < numsrcanims; m++)
		{
			numdestanims += pvsource->vanim_mapcount[psrcanim[m].vertex];
		}

		// allocate room to all possible resulting deltas
		s_vertanim_t *pdestanim = (s_vertanim_t *)kalloc( numdestanims, sizeof( s_vertanim_t ) );
		g_flexkey[i].vanim = pdestanim;

		for (m = 0; m < numsrcanims; m++, psrcanim++)
		{
			Vector delta;
			Vector ndelta;
			float scale = 1.0;

			if (g_flexkey[i].split > 0)
			{
				if (psrcanim->pos.x > g_flexkey[i].split) 
				{
					scale = 0;
				}
				else if (psrcanim->pos.x < -g_flexkey[i].split) 
				{
					scale = 1.0;
				}
				else 
				{
					float t = (g_flexkey[i].split - psrcanim->pos.x) / (2.0 * g_flexkey[i].split);
					scale = 3 * t * t - 2 * t * t * t;
					// printf( "%.1f : %.2f\n", psrcanim->pos.x, scale );
				}
			}
			else if (g_flexkey[i].split < 0)
			{
				if (psrcanim->pos.x < g_flexkey[i].split) 
				{
					scale = 0;
				}
				else if (psrcanim->pos.x > -g_flexkey[i].split) 
				{
					scale = 1.0;
				}
				else 
				{
					float t = (g_flexkey[i].split - psrcanim->pos.x) / (2.0 * g_flexkey[i].split);
					scale = 3 * t * t - 2 * t * t * t;
					// printf( "%.1f : %.2f\n", psrcanim->pos.x, scale );
				}
			}

			if (scale > 0 && pvsource->vanim_mapcount[psrcanim->vertex]) // bah, only do it for ones that found a match!
			{
				j = pvsource->vanim_map[psrcanim->vertex][0];

				//VectorSubtract( psrcanim->pos, pvsource->vanim[0][psrcanim->vertex].pos, tmp );
				//VectorTransform( tmp, pmsource->bonefixup[k].im, delta );
				VectorSubtract( psrcanim->pos, pvsource->vanim[0][psrcanim->vertex].pos, delta );

				//VectorSubtract( psrcanim->normal, pvsource->vanim[0][psrcanim->vertex].normal, tmp );
				//VectorTransform( tmp, pmsource->bonefixup[k].im, ndelta );
				VectorSubtract( psrcanim->normal, pvsource->vanim[0][psrcanim->vertex].normal, ndelta );

				// if the changes are too small, skip 'em
				if (DotProduct( delta, delta ) > 0.001 || DotProduct( ndelta, ndelta ) > 0.001)
				{
					for (n = 0; n < pvsource->vanim_mapcount[psrcanim->vertex]; n++)
					{
						pdestanim->vertex = pvsource->vanim_map[psrcanim->vertex][n];
						VectorScale( delta, scale, pdestanim->pos );
						VectorScale( ndelta, scale, pdestanim->normal );

						/*
						printf("%4d  %6.2f %6.2f %6.2f : %4d  %5.2f %5.2f %5.2f\n", 
							pdestanim->vertex, 
							// pmsource->vertex[pdestanim->vertex][0], pmsource->vertex[pdestanim->vertex][1], pmsource->vertex[pdestanim->vertex][2],
							modelpos[pdestanim->vertex][0], modelpos[pdestanim->vertex][1], modelpos[pdestanim->vertex][2],
							psrcanim->vertex,					
							pdestanim->pos[0], pdestanim->pos[1], pdestanim->pos[2] );
						*/
						g_flexkey[i].numvanims++;
						pdestanim++;
					}
				}
			}
		}
	}
}


// Finds the bone index for a particular source
extern int FindLocalBoneNamed( const s_source_t *pSource, const char *pName );

//-----------------------------------------------------------------------------
// Purpose: finds the bone index in the global bone table
//-----------------------------------------------------------------------------

int findGlobalBone( const char *name )
{
	int k;

	name = RenameBone( name );

	for (k = 0; k < g_numbones; k++)
	{
		if (stricmp( g_bonetable[k].name, name ) == 0)
		{
			return k;
		}
	}
	
	return -1;
}


bool IsGlobalBoneXSI( const char *name, const char *bonename )
{
	name = RenameBone( name );

	int len = strlen( name );

	int len2 = strlen( bonename );
	if (len2 > len)
	{
		if (bonename[len2-len-1] == '.')
		{
			if (stricmp( &bonename[len2-len], name ) == 0)
			{
				return true;
			}
		}
	}
	
	return false;
}



int findGlobalBoneXSI( const char *name )
{
	int k;

	name = RenameBone( name );

	int len = strlen( name );

	for (k = 0; k < g_numbones; k++)
	{
		if (IsGlobalBoneXSI( name, g_bonetable[k].name ))
		{
			return k;
		}
	}
	
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Acculumate quaternions and try to find the swept area of rotation 
//			so that a "midpoint" of the rotation area can be found
//-----------------------------------------------------------------------------

void findAnimQuaternionAlignment( int k, int i, Quaternion &qBase, Quaternion &qMin, Quaternion &qMax )
{
	int j;

	AngleQuaternion( g_panimation[i]->sanim[0][k].rot, qBase );
	qMin = qBase;
	float dMin = 1.0;
	qMax = qBase;
	float dMax = 1.0;
	
	for (j = 1; j < g_panimation[i]->numframes; j++)
	{
		Quaternion q;

		AngleQuaternion( g_panimation[i]->sanim[j][k].rot, q );
		QuaternionAlign( qBase, q, q );

		float d0 = QuaternionDotProduct( q, qBase );
		float d1 = QuaternionDotProduct( q, qMin );
		float d2 = QuaternionDotProduct( q, qMax );

		/*
		if (i != 0) 
			printf("%f %f %f : %f\n", d0, d1, d2, QuaternionDotProduct( qMin, qMax ) );
		*/
		if (d1 >= d0)
		{
			if (d0 < dMin)
			{
				qMin = q;
				dMin = d0;
				if (dMax == 1.0)
				{
					QuaternionMA( qBase, -0.01, qMin, qMax );
					QuaternionAlign( qBase, qMax, qMax );
				}
			}
		}
		else if (d2 >= d0)
		{
			if (d0 < dMax)
			{
				qMax = q;
				dMax = d0;
			}
		}

		/*
		if (i != 0) 
			printf("%f ", QuaternionDotProduct( qMin, qMax ) );
		*/

		QuaternionSlerpNoAlign( qMin, qMax, 0.5, qBase );
		Assert( qBase.IsValid() );

		/*
		if (i != 0)
		{
			QAngle ang;
			QuaternionAngles( qMin, ang );
			printf("(%.1f %.1f %.1f) ", ang.x, ang.y, ang.z );
			QuaternionAngles( qMax, ang );
			printf("(%.1f %.1f %.1f) ", ang.x, ang.y, ang.z );
			QuaternionAngles( qBase, ang );
			printf("(%.1f %.1f %.1f)\n", ang.x, ang.y, ang.z );
		}
		*/

		dMin = QuaternionDotProduct( qBase, qMin );
		dMax = QuaternionDotProduct( qBase, qMax );
	}

	// printf("%s (%s): %.3f :%.3f\n", g_bonetable[k].name, g_panimation[i]->name, QuaternionDotProduct( qMin, qMax ), QuaternionDotProduct( qMin, qBase ) );
	/*
	if (i != 0) 
		exit(0);
	*/
}


//-----------------------------------------------------------------------------
// Purpose: For specific bones, try to find the total valid area of rotation so 
//			that their mid point of rotation can be used at run time to "pre-align"
//			the quaternions so that rotations > 180 degrees don't get blended the 
//			"short way round".
//-----------------------------------------------------------------------------

void limitBoneRotations( void )
{
	int i, j, k;

	for (i = 0; i < g_numlimitrotation; i++)
	{
		Quaternion qBase;

		k = findGlobalBone( g_limitrotation[i].name );
		if (k == -1)
		{
			Error("unknown bone \"%s\" in $limitrotation\n", g_limitrotation[i].name );
		}

		AngleQuaternion( g_bonetable[k].rot, qBase );

		if (g_limitrotation[i].numseq == 0)
		{
			for (j = 0; j < g_numani; j++)
			{
				if (!(g_panimation[j]->flags & STUDIO_DELTA) && g_panimation[j]->numframes > 3)
				{
					Quaternion qBase2, qMin2, qMax2;
					findAnimQuaternionAlignment( k, j, qBase2, qMin2, qMax2 );

					QuaternionAdd( qBase, qBase2, qBase );
				}
			}
			QuaternionNormalize( qBase );
		}
		else
		{
			for (j = 0; j < g_limitrotation[i].numseq; j++)
			{

			}
		}

		/*
		QAngle ang;
		QuaternionAngles( qBase, ang );
		printf("%s : (%.1f %.1f %.1f) \n", g_bonetable[k].name, ang.x, ang.y, ang.z );
		*/

		g_bonetable[k].qAlignment = qBase;
		g_bonetable[k].flags |= BONE_FIXED_ALIGNMENT;

		// QuaternionAngles( qBase, g_panimation[0]->sanim[0][k].rot );
	}
}





//-----------------------------------------------------------------------------
// Purpose: For specific bones, try to find the total valid area of rotation so 
//			that their mid point of rotation can be used at run time to "pre-align"
//			the quaternions so that rotations > 180 degrees don't get blended the 
//			"short way round".
//-----------------------------------------------------------------------------

void limitIKChainLength( void )
{
	int i, j, k;
	matrix3x4_t boneToWorld[MAXSTUDIOSRCBONES];	// bone transformation matrix

	for (k = 0; k < g_numikchains; k++)
	{
		bool needsFixup = false;
		bool hasKnees = false;

		Vector kneeDir = Vector( 0, 0, 0 );

		for (i = 0; i < g_numani; i++)
		{
			s_animation_t *panim = g_panimation[i];

			if (panim->flags & STUDIO_DELTA)
				continue;

			for (j = 0; j < panim->numframes; j++)
			{
				CalcBoneTransforms( panim, j, boneToWorld );

				Vector worldThigh;
				Vector worldKnee;
				Vector worldFoot;

				MatrixPosition( boneToWorld[ g_ikchain[k].link[0] ], worldThigh );
				MatrixPosition( boneToWorld[ g_ikchain[k].link[1] ], worldKnee );
				MatrixPosition( boneToWorld[ g_ikchain[k].link[2] ], worldFoot );

				float l1 = (worldKnee-worldThigh).Length();
				float l2 = (worldFoot-worldKnee).Length();
				float l3 = (worldFoot-worldThigh).Length();

				Vector ikKnee = worldKnee - worldThigh;
				Vector ikHalf = (worldFoot-worldThigh) * (l1 / l3);

				// FIXME: what to do when the knee completely straight?
				Vector ikKneeDir = ikKnee - ikHalf;
				VectorNormalize( ikKneeDir );
				// ikTargetKnee = ikKnee + ikKneeDir * l1;

				// leg too straight to figure out knee?
				if (l3 > (l1 + l2) * 0.999)
				{
					needsFixup = true;
				}
				else
				{
					Vector tmp;
					VectorIRotate( ikKneeDir, boneToWorld[ g_ikchain[k].link[0] ], tmp );
					kneeDir += tmp;
					hasKnees = true;
				}
			}
		}

		if (!needsFixup)
			continue;

		if (!hasKnees)
		{
			printf( "ik rules but no clear knee direction\n");
			continue;
		}

		for (i = 0; i < g_numani; i++)
		{
			s_animation_t *panim = g_panimation[i];

			if (panim->flags & STUDIO_DELTA)
				continue;

			for (j = 0; j < panim->numframes; j++)
			{
				CalcBoneTransforms( panim, j, boneToWorld );

				Vector worldThigh;
				Vector worldKnee;
				Vector worldFoot;

				MatrixPosition( boneToWorld[ g_ikchain[k].link[0] ], worldThigh );
				MatrixPosition( boneToWorld[ g_ikchain[k].link[1] ], worldKnee );
				MatrixPosition( boneToWorld[ g_ikchain[k].link[2] ], worldFoot );

				float l1 = (worldKnee-worldThigh).Length();
				float l2 = (worldFoot-worldKnee).Length();
				float l3 = (worldFoot-worldThigh).Length();

				// leg too straight to figure out knee?
				if (l3 > (l1 + l2) * 0.999)
				{
					// printf("%s : %d\n", panim->name, j );

					Vector targetFoot = worldFoot - worldThigh;
					VectorNormalize( targetFoot );
					VectorScale( targetFoot, l3 * 0.999, targetFoot );
					targetFoot = targetFoot + worldThigh;

					Vector targetKnee;
					VectorTransform( kneeDir, boneToWorld[ g_ikchain[k].link[0] ], targetKnee );
					
					Studio_SolveIK( g_ikchain[k].link[0], g_ikchain[k].link[1], g_ikchain[k].link[2], targetFoot, targetKnee, boneToWorld );

					solveBone( panim, j, g_ikchain[k].link[0], boneToWorld );  
					solveBone( panim, j, g_ikchain[k].link[1], boneToWorld );  
					solveBone( panim, j, g_ikchain[k].link[2], boneToWorld );  
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: build "next node" table that links every transition "node" to 
//			every other transition "node", if possible
//-----------------------------------------------------------------------------
void MakeTransitions( )
{
	int i, j, k;
	int iHit;

	// add in direct node transitions
	for (i = 0; i < g_numseq; i++)
	{
		if (g_sequence[i].entrynode != g_sequence[i].exitnode)
		{
			g_xnode[g_sequence[i].entrynode-1][g_sequence[i].exitnode-1] = g_sequence[i].exitnode;
			if (g_sequence[i].nodeflags)
			{
				g_xnode[g_sequence[i].exitnode-1][g_sequence[i].entrynode-1] = g_sequence[i].entrynode;
			}
		}
	}

	// calculate multi-stage transitions 
	do 
	{
		iHit = 0;
		for (i = 1; i <= g_numxnodes; i++)
		{
			for (j = 1; j <= g_numxnodes; j++)
			{
				// if I can't go there directly
				if (i != j && g_xnode[i-1][j-1] == 0)
				{
					for (k = 1; k <= g_numxnodes; k++)
					{
						// but I found someone who knows how that I can get to
						if (g_xnode[k-1][j-1] > 0 && g_xnode[i-1][k-1] > 0)
						{
							// then go to them
							g_xnode[i-1][j-1] = -g_xnode[i-1][k-1];
							iHit = 1;
							break;
						}
					}
				}
			}
		}
		// reset previous pass so the links can be used in the next pass
		for (i = 1; i <= g_numxnodes; i++)
		{
			for (j = 1; j <= g_numxnodes; j++)
			{
				g_xnode[i-1][j-1] = abs( g_xnode[i-1][j-1] );
			}
		}
	}
	while (iHit);

	// add in allowed "skips"
	for (i = 0; i < g_numxnodeskips; i++)
	{
		g_xnode[g_xnodeskip[i][0]-1][g_xnodeskip[i][1]-1] = 0;
	}

#if 0
	for (j = 1; j <= g_numxnodes; j++)
	{
		printf("%2d : %s\n", j, g_xnodename[j] );
	}
	printf("    " );
	for (j = 1; j <= g_numxnodes; j++)
	{
		printf("%2d ", j );
	}
	printf("\n" );

	for (i = 1; i <= g_numxnodes; i++)
	{
		printf("%2d: ", i );
		for (j = 1; j <= g_numxnodes; j++)
		{
			printf("%2d ", g_xnode[i-1][j-1] );
		}
		printf("\n" );
	}
	exit(0);
#endif
}


int VectorCompareEpsilon(const Vector& v1, const Vector& v2, float epsilon)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (fabs(v1[i] - v2[i]) > epsilon)
			return 0;
			
	return 1;
}

int RadianEulerCompareEpsilon(const RadianEuler& v1, const RadianEuler& v2, float epsilon)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
	{
		// clamp to 2pi
		float a1 = fmod(v1[i],2*M_PI);
		float a2 = fmod(v2[i],2*M_PI);
		float delta =  fabs(a1-a2);
		
		// use the smaller angle (359 == 1 degree off)
		if ( delta > M_PI )
		{
			delta = 2*M_PI - delta;
		}

		if (delta > epsilon)
			return 0;
	}
			
	return 1;
}

bool AnimationDifferent( const Vector& startPos, const RadianEuler& startRot, const Vector& pos, const RadianEuler& rot )
{
	if ( !VectorCompareEpsilon( startPos, pos, 0.01 ) )
		return true;
	if ( !RadianEulerCompareEpsilon( startRot, rot, 0.01 ) )
		return true;

	return false;
}

bool BoneHasAnimation( const char *pName )
{
	bool first = true;
	Vector pos;
	RadianEuler rot;

	if ( !g_numani )
		return false;

	int globalIndex = findGlobalBone( pName );

	// don't check root bones for animation
	if (globalIndex >= 0 && g_bonetable[globalIndex].parent == -1)
	{
		return true;
	}

	// find used bones per g_model
	for (int i = 0; i < g_numani; i++)
	{
		s_source_t *psource = g_panimation[i]->source;
		int boneIndex = FindLocalBoneNamed(psource, pName);

		// not in this source?
		if (boneIndex < 0)
			continue;

		// this is not right, but enough of the bones are moved unintentionally between
		// animations that I put this in to catch them.
		first = true;
		int n = g_panimation[i]->startframe - psource->startframe;
		// printf("%s %d:%d\n", g_panimation[i]->filename, g_panimation[i]->startframe, psource->startframe );
		for (int j = 0; j < g_panimation[i]->numframes; j++)
		{
			if ( first )
			{
				VectorCopy( psource->rawanim[j+n][boneIndex].pos, pos );
				VectorCopy( psource->rawanim[j+n][boneIndex].rot, rot );
				first = false;
			}
			else
			{
				if ( AnimationDifferent( pos, rot, psource->rawanim[j+n][boneIndex].pos, psource->rawanim[j+n][boneIndex].rot ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool BoneHasAttachments( char const *pname )
{
	for (int k = 0; k < g_numattachments; k++)
	{
		if ( !stricmp( g_attachment[k].bonename, pname ) )
		{
			return true;
		}
	}
	return false;
}

bool BoneIsProcedural( char const *pname )
{
	int k;

	for (k = 0; k < g_numaxisinterpbones; k++)
	{
		if (! stricmp( g_axisinterpbones[k].bonename, pname ) )
		{
			return true;
		}
	}

	for (k = 0; k < g_numquatinterpbones; k++)
	{
		if (IsGlobalBoneXSI( g_quatinterpbones[k].bonename, pname ) )
		{
			return true;
		}
	}

	return false;
}


bool BoneShouldCollapse( char const *pname )
{
	int k;

	for (k = 0; k < g_numcollapse; k++)
	{
		if (stricmp( g_collapse[k], pname ) == 0)
		{
			return true;
		}
	}

	return (!BoneHasAnimation( pname ) && !BoneIsProcedural( pname ) /* && !BoneHasAttachments( pname ) */);
}

//-----------------------------------------------------------------------------
// Purpose: Collapse vertex assignments up to parent on bones that are not needed
//			This can optimize a model substantially if the animator is using
//			lots of helper bones with no animation.
//-----------------------------------------------------------------------------
void CollapseBones( void )
{
	int i, j, k;
	int count = 0;

	for (k = 0; k < g_numbones; k++)
	{
		bool collapse = true;

		for (i = 0; collapse && i < g_numsources; i++)
		{
			s_source_t *psource = g_source[i];
			
			if ( !BoneShouldCollapse( g_bonetable[k].name ) )
			{
				collapse = false;
			}
		}

		if (collapse)
		{
			count++;

			if( !g_quiet )
			{
				printf("collapsing %s\n", g_bonetable[k].name );
			}

			g_numbones--;
			int m = g_bonetable[k].parent;

			for (j = k; j < g_numbones; j++)
			{
				g_bonetable[j] = g_bonetable[j+1];
				if (g_bonetable[j].parent == k)
				{
					g_bonetable[j].parent = m;
				} 
				else if (g_bonetable[j].parent >= k)
				{
					g_bonetable[j].parent = g_bonetable[j].parent - 1;
				}
			}
			k--;
		}
	}

	if( !g_quiet && count)
	{
		printf("Collapsed %d bones\n", count );
	}
}

//-----------------------------------------------------------------------------
// Purpose: replace all animation, rotation and translation, etc. with a single bone
//-----------------------------------------------------------------------------
void MakeStaticProp()
{

	int i, j, k;
	matrix3x4_t rotated;

	AngleMatrix( g_defaultrotation, rotated );

	// FIXME: missing attachment point recalcs!

	// replace bone 0 with "static_prop" bone and attach everything to it.
	for (i = 0; i < g_numsources; i++)
	{
		s_source_t *psource = g_source[i];

		strcpy( psource->localBone[0].name, "static_prop" );
		psource->localBone[0].parent = -1;

		for (k = 1; k < psource->numbones; k++)
		{
			psource->localBone[k].parent = -1;
		}

		if (psource->localBoneweight)
		{
			rotated[0][3] = g_defaultadjust[0];
			rotated[1][3] = g_defaultadjust[1];
			rotated[2][3] = g_defaultadjust[2];

			Vector mins, maxs;
			ClearBounds( mins, maxs );

			for (j = 0; j < psource->numvertices; j++)
			{
				for (k = 0; k < psource->localBoneweight[j].numbones; k++)
				{
					// attach everything to root
					psource->localBoneweight[j].bone[k] = 0;
				}

				// shift everything into identity space
				Vector tmp;
				VectorTransform( psource->vertex[j], rotated, tmp );
				VectorCopy( tmp, psource->vertex[j] );

				// incrementally compute identity space bbox
				AddPointToBounds( psource->vertex[j], mins, maxs );
				VectorRotate( psource->normal[j], rotated, tmp );
				VectorCopy( tmp, psource->normal[j] );
			}

			if ( g_centerstaticprop )
			{
				Vector propCenter = -0.5f * (mins + maxs);
				for ( j = 0; j < psource->numvertices; j++ )
				{
					psource->vertex[j] += propCenter;
				}
				// now add an attachment point to store this offset
				Q_strncpy( g_attachment[g_numattachments].name, "placementOrigin", sizeof(g_attachment[g_numattachments].name) );
				Q_strncpy( g_attachment[g_numattachments].bonename, "static_prop", sizeof(g_attachment[g_numattachments].name) );
				g_attachment[g_numattachments].bone = 0;
				g_attachment[g_numattachments].type = 0;
				AngleMatrix( vec3_angle, propCenter, g_attachment[g_numattachments].local );
				g_numattachments++;
			}
		}

		// force the animation to be identity
		psource->rawanim[0][0].pos = Vector( 0, 0, 0 );
		psource->rawanim[0][0].rot = RadianEuler( 0, 0, 0 );
	
		// make an identity boneToPose transform
		AngleMatrix( QAngle( 0, 0, 0 ), psource->boneToPose[0] );
		
		// make it all a single frame animation
		psource->numframes = 1;
		psource->startframe = 0;
		psource->endframe = 1;
	}

	// throw away all animations
	g_numani = 1;
	g_panimation[0]->numframes = 1;
	g_panimation[0]->startframe = 0;
	g_panimation[0]->endframe = 1;
	g_panimation[0]->rotation = RadianEuler( 0, 0, 0 );
	g_panimation[0]->adjust = Vector( 0, 0, 0 );

	// throw away all vertex animations
	g_numflexkeys = 0;
	g_defaultflexkey = NULL;

	// Recalc attachment points:
	for( i = 0; i < g_numattachments; i++ )
	{
		if( g_centerstaticprop && ( i == g_numattachments - 1 ) )
			continue;
		
		ConcatTransforms( rotated, g_attachment[i].local, g_attachment[i].local );

		Q_strncpy( g_attachment[i].bonename, "static_prop", sizeof(g_attachment[i].name) );
		g_attachment[i].bone = 0;
		g_attachment[i].type = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: set "boneref" for all the source bones used by vertices, attachments, eyeballs, etc.
//-----------------------------------------------------------------------------

void TagUsedBones( )
{
	int i, j, k;
	int n;
	int	iError = 0;

	// find used bones per g_model
	for (i = 0; i < g_numsources; i++)
// FIXME: This breaks body groups (example . .zombie classic)
//	for (i = 0; i < g_nummodelsbeforeLOD; i++)
	{
		s_source_t *psource = g_source[i];

		for (k = 0; k < MAXSTUDIOSRCBONES; k++)
		{
			psource->boneref[k] = 0;
		}

		if (psource->localBoneweight)
		{
			for (j = 0; j < psource->numvertices; j++)
			{
				for (k = 0; k < psource->localBoneweight[j].numbones; k++)
				{
					psource->boneref[psource->localBoneweight[j].bone[k]] |= BONE_USED_BY_VERTEX_LOD0;
				}
			}
		}
	}

	// find used bones per g_model
	for (i = 0; i < g_numsources; i++)
	{
		s_source_t *psource = g_source[i];

		// FIXME: this is in the wrong place.  The attachment may be rigid and it never defined in a reference file
		for (k = 0; k < g_numattachments; k++)
		{
			for (j = 0; j < psource->numbones; j++)
			{
				if ( !stricmp( g_attachment[k].bonename, psource->localBone[j].name ) )
				{
					// this bone is a keeper with or without associated vertices
					// because an attachment point depends on it.
					if (g_attachment[k].type & IS_RIGID)
					{
						for (n = j; n != -1; n = psource->localBone[n].parent)
						{
							if (psource->boneref[n] & BONE_USED_BY_VERTEX_LOD0)
							{
								psource->boneref[n] |= BONE_USED_BY_ATTACHMENT;
								break;
							}
						}
					}
					else
					{
						psource->boneref[j] |= BONE_USED_BY_ATTACHMENT;
					}
				}
			}
		}

		for (k = 0; k < g_nummouths; k++)
		{
			for (j = 0; j < psource->numbones; j++)
			{
				if ( !stricmp( g_mouth[k].bonename, psource->localBone[j].name ) )
				{
					// this bone is a keeper with or without associated vertices
					// because a mouth shader depends on it.
					psource->boneref[j] |= BONE_USED_BY_ATTACHMENT;
				}
			}
		}

		for (k = 0; k < MAXSTUDIOSRCBONES; k++)
		{
			// tag parent bones as used if child has been used
			if (psource->boneref[k])
			{
				n = psource->localBone[k].parent;
				while (n != -1)
				{
					psource->boneref[n] |= psource->boneref[k];
					n = psource->localBone[n].parent;
				}
			}
		}
	}

	// tag all eyeball bones
	for (i = 0; i < g_nummodelsbeforeLOD; i++)
	{
		s_source_t *psource = g_model[i]->source;
		for (k = 0; k < g_model[i]->numeyeballs; k++)
		{
			psource->boneref[g_model[i]->eyeball[k].bone] |= BONE_USED_BY_ATTACHMENT;
		}
	}		
}


//-----------------------------------------------------------------------------
// Purpose: change the names in the source files for bones that max auto-renamed on us
//-----------------------------------------------------------------------------
void RenameBones( )
{
	int i, j, k;

	// rename source bones if needed
	for (i = 0; i < g_numsources; i++)
	{
		for (j = 0; j < g_source[i]->numbones; j++)
		{
			for (k = 0; k < g_numrenamedbones; k++)
			{
				if (!stricmp( g_source[i]->localBone[j].name, g_renamedbone[k].from))
				{
					strcpy( g_source[i]->localBone[j].name, g_renamedbone[k].to );
					break;
				}
			}
		}
	}
}


const char *RenameBone( const char *pName )
{
	int k;
	for (k = 0; k < g_numrenamedbones; k++)
	{
		if (!stricmp( pName, g_renamedbone[k].from))
		{
			return g_renamedbone[k].to;
		}
	}
	return pName;
}

//-----------------------------------------------------------------------------
// Purpose: look through all the sources and build a table of used bones
//-----------------------------------------------------------------------------
int BuildGobalBonetable( )
{
	int i, j, k;
	int m, n;
	int	iError = 0;

	// union of all used bones
	g_numbones = 0;
	for (i = 0; i < g_numsources; i++)
	{
		s_source_t *psource = g_source[i];
		for (j = 0; j < psource->numbones; j++)
		{
			if (psource->boneref[j])
			{
				k = findGlobalBone( psource->localBone[j].name );
				if (k == -1)
				{
					// create new bone
					k = g_numbones;
					strcpyn( g_bonetable[k].name, psource->localBone[j].name );
					if ((n = psource->localBone[j].parent) != -1)
						g_bonetable[k].parent		= findGlobalBone( psource->localBone[n].name );
					else
						g_bonetable[k].parent		= -1;
					g_bonetable[k].bonecontroller	= 0;
					g_bonetable[k].flags			|= psource->boneref[j];
					AngleMatrix( psource->rawanim[0][j].rot, psource->rawanim[0][j].pos, g_bonetable[k].rawLocal );
					// printf("%d : %s (%s)\n", k, g_bonetable[k].name, g_bonetable[g_bonetable[k].parent].name );
					g_numbones++;
				}
				else
				{
					// double check parent assignments
					n = psource->localBone[j].parent;
					if (n != -1)
						n = findGlobalBone( psource->localBone[n].name );
	 				m = g_bonetable[k].parent;

#if 0
					if (n != m)
					{
						printf("illegal parent bone replacement in file \"%s\"\n\t\"%s\" has \"%s\", previously was \"%s\"\n", 
							psource->filename, 
							psource->localBone[j].name, 
							(n != -1) ? g_bonetable[n].name : "ROOT",
							(m != -1) ? g_bonetable[m].name : "ROOT" );
						iError++;
					}
#endif
					// accumlate flags
					g_bonetable[k].flags			|= psource->boneref[j];
				}
			}
		}
	}
	return iError;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void BuildGlobalBoneToPose( )
{
	int k;

	// build reference pose
	for (k = 0; k < g_numbones; k++)
	{
		if (g_bonetable[k].parent == -1)
		{
			MatrixCopy( g_bonetable[k].rawLocal, g_bonetable[k].boneToPose );
		}
		else
		{
			ConcatTransforms (g_bonetable[g_bonetable[k].parent].boneToPose, g_bonetable[k].rawLocal, g_bonetable[k].boneToPose);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void RebuildLocalPose( )
{
	int k;

	matrix3x4_t boneToPose[MAXSTUDIOBONES];

	// build reference pose
	for (k = 0; k < g_numbones; k++)
	{
		MatrixCopy( g_bonetable[k].boneToPose, boneToPose[k] );
	}

	matrix3x4_t poseToBone[MAXSTUDIOBONES];

	// rebuild local pose
	for (k = 0; k < g_numbones; k++)
	{
		if (g_bonetable[k].parent == -1)
		{
			MatrixCopy( boneToPose[k], g_bonetable[k].rawLocal );
		}
		else
		{
			ConcatTransforms (poseToBone[g_bonetable[k].parent], boneToPose[k], g_bonetable[k].rawLocal );
		}
		MatrixAngles( g_bonetable[k].rawLocal, g_bonetable[k].rot, g_bonetable[k].pos );
		MatrixCopy( boneToPose[k], g_bonetable[k].boneToPose );
		MatrixInvert( boneToPose[k], poseToBone[k] );

		// printf("%d \"%s\" %d\n", k, g_bonetable[k].name, g_bonetable[k].parent );
	}
	//exit(0);

}


//-----------------------------------------------------------------------------
// Purpose: attach bones to different parents if needed
//-----------------------------------------------------------------------------

void EnforceHeirarchy( )
{
	int i, j, k;

	// force changes to hierarchy
	for (i = 0; i < g_numforcedhierarchy; i++)
	{
		j = findGlobalBone( g_forcedhierarchy[i].parentname );
		k = findGlobalBone( g_forcedhierarchy[i].childname );

		if (j == -1 && strlen( g_forcedhierarchy[i].parentname ) > 0 )
		{
			Error( "unknown bone: \"%s\" in forced hierarchy\n", g_forcedhierarchy[i].parentname );
		}
		if (k == -1)
		{
			Error( "unknown bone: \"%s\" in forced hierarchy\n", g_forcedhierarchy[i].childname );
		}
		
		/*
		if (j > k)
		{
			Error( "parent \"%s\" declared after child \"%s\" in forced hierarchy\n", g_forcedhierarchy[i].parentname, g_forcedhierarchy[i].childname );
		}
		*/

		/*
		if (strlen(g_forcedhierarchy[i].subparentname) != 0)
		{
			int n, m;

			m = findGlobalBone( g_forcedhierarchy[i].subparentname );
			if (m != -1)
			{
				Error( "inserted bone \"%s\" matches name of existing bone in hierarchy\n", g_forcedhierarchy[i].parentname, g_forcedhierarchy[i].subparentname );
			}

			printf("inserting bone \"%s\"\n", g_forcedhierarchy[i].subparentname );

			// shift the bone list up
			for (n = g_numbones; n > k; n--)
			{
				g_bonetable[n] = g_bonetable[n-1];
				if (g_bonetable[n].parent >= k)
				{
					g_bonetable[n].parent = g_bonetable[n].parent + 1;
				}
				MatrixCopy( boneToPose[n-1], boneToPose[n] );
			}
			g_numbones++;

			// add the bone
			strcpy( g_bonetable[k].name, g_forcedhierarchy[i].subparentname );
			g_bonetable[k].parent = j;
			g_bonetable[k].split = true;
			g_bonetable[k+1].parent = k;

			// split the bone
			Quaternion q1, q2;
			Vector p;
			MatrixAngles( boneToPose[k], q1, p );  // FIXME: badly named!

			// !!!!
			// QuaternionScale( q1, 0.5, q2 );
			// q2.Init( 0, 0, 0, 1 );
			// AngleQuaternion( QAngle( 0, 0, 0 ), q2 );
			//QuaternionMatrix( q2, p, boneToPose[k] );
			QuaternionMatrix( q1, p, boneToPose[k] );
			QuaternionMatrix( q1, p, boneToPose[k+1] );
		}
		else
		*/
		{
			g_bonetable[k].parent = j;
		}
	}


	// resort hierarchy
	bool bSort = true;
	int count = 0;

	while (bSort) 
	{
		count++;
		bSort = false;
		for (i = 0; i < g_numbones; i++)
		{
			if (g_bonetable[i].parent > i)
			{
				// swap
				j = g_bonetable[i].parent;
				s_bonetable_t tmp;
				tmp = g_bonetable[i];
				g_bonetable[i] = g_bonetable[j];
				g_bonetable[j] = tmp;

				// relink parents
				for (k = i; k < g_numbones; k++)
				{
					if (g_bonetable[k].parent == i)
					{
						g_bonetable[k].parent = j;
					} 
					else if (g_bonetable[k].parent == j)
					{
						g_bonetable[k].parent = i;
					}
				}

				bSort = true;
			}
		}
		if (count > 1000)
		{
			Error( "Circular bone heirarchy\n");
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: find procedural bones and tag for inclusion even if they don't animate
//-----------------------------------------------------------------------------

void TagProceduralBones( )
{
	int j;

	// look for AxisInterp bone definitions
	int numaxisinterpbones = 0;
	for (j = 0; j < g_numaxisinterpbones; j++)
	{
		g_axisinterpbones[j].bone = findGlobalBone( g_axisinterpbones[j].bonename );
		g_axisinterpbones[j].control = findGlobalBone( g_axisinterpbones[j].controlname );

		if (g_axisinterpbones[j].bone == -1) 
		{
			if (!g_quiet)
			{
				printf("axisinterpbone \"%s\" unused\n", g_axisinterpbones[j].bonename );
			}
			continue; // optimized out, don't complain
		}

		if (g_axisinterpbones[j].control == -1)
		{
			Error( "Missing control bone \"%s\" for procedural bone \"%s\"\n", g_axisinterpbones[j].bonename, g_axisinterpbones[j].controlname );
		}

		g_bonetable[g_axisinterpbones[j].bone].flags |= BONE_ALWAYS_PROCEDURAL; // ??? what about physics rules
		g_axisinterpbonemap[numaxisinterpbones++] = j;
	}
	g_numaxisinterpbones = numaxisinterpbones;

	// look for QuatInterp bone definitions
	int numquatinterpbones = 0;
	for (j = 0; j < g_numquatinterpbones; j++)
	{
		g_quatinterpbones[j].bone = findGlobalBoneXSI( g_quatinterpbones[j].bonename );
		g_quatinterpbones[j].control = findGlobalBoneXSI( g_quatinterpbones[j].controlname );

		if (g_quatinterpbones[j].bone == -1) 
		{
			if (!g_quiet)
			{
				printf("quatinterpbone \"%s\" unused\n", g_quatinterpbones[j].bonename );
			}
			continue; // optimized out, don't complain
		}

		if (g_quatinterpbones[j].control == -1)
		{
			Error( "Missing control bone \"%s\" for procedural bone \"%s\"\n", g_quatinterpbones[j].bonename, g_quatinterpbones[j].controlname );
		}

		g_bonetable[g_quatinterpbones[j].bone].flags |= BONE_ALWAYS_PROCEDURAL; // ??? what about physics rules
		g_quatinterpbonemap[numquatinterpbones++] = j;
	}
	g_numquatinterpbones = numquatinterpbones;
}



//-----------------------------------------------------------------------------
// Purpose: convert original procedural bone info into correct values for existing skeleton
//-----------------------------------------------------------------------------

void RemapProceduralBones( )
{
	int j;

	// look for QuatInterp bone definitions
	for (j = 0; j < g_numquatinterpbones; j++)
	{
		s_quatinterpbone_t *pInterp = &g_quatinterpbones[g_quatinterpbonemap[j]];

		int origParent = findGlobalBoneXSI( pInterp->parentname );
		int origControlParent = findGlobalBoneXSI( pInterp->controlparentname );

		if (origParent == -1 || origControlParent == -1)
		{
			Error( "unknown procedual bone remapping\n" );
		}

		if ( g_bonetable[pInterp->bone].parent != origParent)
		{
			Error( "unknown procedual bone parent remapping\n" );
		}

		if ( g_bonetable[pInterp->control].parent != origControlParent)
		{
			Error( "unknown procedual bone control parent remapping\n" );
		}

		for (int k = 0; k < pInterp->numtriggers; k++)
		{
			Quaternion q1;
			AngleQuaternion( RadianEuler( 0, 0, 0 ), q1 );
			Quaternion q2 = pInterp->trigger[k];

			matrix3x4_t srcBoneToWorld;
			matrix3x4_t destBoneToWorld;
			Vector tmp;

			QuaternionMatrix( pInterp->trigger[k], srcBoneToWorld );
			ConcatTransforms( srcBoneToWorld, g_bonetable[pInterp->control].srcRealign, destBoneToWorld );
			MatrixAngles( destBoneToWorld, pInterp->trigger[k], tmp );

			tmp = pInterp->pos[k] + pInterp->basepos + g_bonetable[pInterp->control].pos * pInterp->percentage;

			QuaternionMatrix( pInterp->quat[k], tmp, srcBoneToWorld );
			ConcatTransforms( srcBoneToWorld, g_bonetable[pInterp->bone].srcRealign, destBoneToWorld );
			MatrixAngles( destBoneToWorld, pInterp->quat[k], pInterp->pos[k] );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: go through all source files and link local bone indices and global bonetable indicies
//-----------------------------------------------------------------------------
int MapSourcesToGlobalBonetable( )
{
	int i, j, k;
	int	iError = 0;

	// map each source bone list to master list
	for (i = 0; i < g_numsources; i++)
	{
		for (k = 0; k < MAXSTUDIOSRCBONES; k++)
		{
			g_source[i]->boneGlobalToLocal[k] = -1;
			g_source[i]->boneLocalToGlobal[j] = -1;
		}
		for (j = 0; j < g_source[i]->numbones; j++)
		{
			k = findGlobalBone( g_source[i]->localBone[j].name );
			
			if (k == -1)
			{
				int m = g_source[i]->localBone[j].parent;
				while ( m != -1 && (k = findGlobalBone( g_source[i]->localBone[m].name )) == -1 )
				{
					m = g_source[i]->localBone[m].parent;
				}
				if (k == -1)
				{
					/*
					if (!g_quiet)
					{
						printf("unable to find connection for collapsed bone \"%s\" \n", g_source[i]->localBone[j].name );
					}
					*/
					k = 0;
				}
				g_source[i]->boneLocalToGlobal[j] = k;
			}
			else
			{
#if 0
				char *szAnim = "ROOT";
				char *szNode = "ROOT";

				// whoa, check parent connections!
				if (g_source[i]->localBone[j].parent != -1)
					szAnim = g_source[i]->localBone[g_source[i]->localBone[j].parent].name;
				
				if (g_bonetable[k].parent != -1)
					szNode = g_bonetable[g_bonetable[k].parent].name;


				// garymcthack - This gets tripped on the antlion if there are any lods at all.
				// I don't understand why.

				if (stricmp(szAnim, szNode) && !(g_bonetable[k].flags & BONE_ALWAYS_PROCEDURAL))
				{
					printf("illegal parent bone replacement in g_sequence \"%s\"\n\t\"%s\" has \"%s\", reference has \"%s\"\n", 
						g_source[i]->filename, 
						g_source[i]->localBone[j].name, 
						szAnim,
						szNode );
					iError++;
				}
#endif
				g_source[i]->boneLocalToGlobal[j] = k;
				g_source[i]->boneGlobalToLocal[k] = j;
			}
		}
	}
	return iError;
}



//-----------------------------------------------------------------------------
// Purpose: go through bone and find any that arent aligned on the X axis
//-----------------------------------------------------------------------------
void RealignBones( )
{
	int k;

	int childbone[MAXSTUDIOBONES];
	for (k = 0; k < g_numbones; k++)
	{
		childbone[k] = -1;
	}

	// force bones with IK rules to realign themselves
	for (int i = 0; i < g_numikchains; i++)
	{
		k = g_ikchain[i].link[0];
		if (childbone[k] == -1 || childbone[k] == g_ikchain[i].link[1])
		{
			childbone[k] = g_ikchain[i].link[1];
		}
		else
		{
			Error("Trying to realign bone \"%s\" with two children \"%s\", \"%s\"\n", 
				g_bonetable[k].name, g_bonetable[childbone[k]].name, g_bonetable[g_ikchain[i].link[1]].name );
		}

		k = g_ikchain[i].link[1];
		if (childbone[k] == -1 || childbone[k] == g_ikchain[i].link[2])
		{
			childbone[k] = g_ikchain[i].link[2];
		}
		else
		{
			Error("Trying to realign bone \"%s\" with two children \"%s\", \"%s\"\n", 
				g_bonetable[k].name, g_bonetable[childbone[k]].name, g_bonetable[g_ikchain[i].link[2]].name );
		}
	}

	if (g_realignbones)
	{
		int children[MAXSTUDIOBONES];

		// count children
		for (k = 0; k < g_numbones; k++)
		{
			children[k] = 0;
		}
		for (k = 0; k < g_numbones; k++)
		{
			if (g_bonetable[k].parent != -1)
			{
				children[g_bonetable[k].parent]++;
			}
		}

		// if my parent bone only has one child, then tell it to align to me
		for (k = 0; k < g_numbones; k++)
		{
			if (g_bonetable[k].parent != -1 && children[g_bonetable[k].parent] == 1)
			{
				childbone[g_bonetable[k].parent] = k;
			}
		}
	}

	matrix3x4_t boneToPose[MAXSTUDIOBONES];

	for (k = 0; k < g_numbones; k++)
	{
		MatrixCopy( g_bonetable[k].boneToPose, boneToPose[k] );
	}

	// look for bones that aren't on a primary X axis
	for (k = 0; k < g_numbones; k++)
	{
		// printf("%s  %.4f %.4f %.4f  (%d)\n", g_bonetable[k].name, g_bonetable[k].pos.x, g_bonetable[k].pos.y, g_bonetable[k].pos.z, children[k] );
		if (childbone[k] != -1)
		{
			float d = g_bonetable[childbone[k]].pos.Length();

			// check to see that it's on positive X
			if (d - g_bonetable[childbone[k]].pos.x > 0.01)
			{
				Vector v2;
				Vector v3;
				// printf("%s:%s  %.4f %.4f %.4f\n", g_bonetable[k].name, g_bonetable[childbone[k]].name, g_bonetable[childbone[k]].pos.x, g_bonetable[childbone[k]].pos.y, g_bonetable[childbone[k]].pos.z );

				Vector forward, left, up;

				// calc X axis
				MatrixGetColumn( g_bonetable[childbone[k]].boneToPose, 3, v2 );
				MatrixGetColumn( g_bonetable[k].boneToPose, 3, v3 );
				forward = v2 - v3;
				VectorNormalize( forward );

				// try to align to existing bone/boundingbox by finding most perpendicular 
				// existing axis and aligning the new Z axis to it.
				Vector forward2, left2, up2;
				MatrixGetColumn( boneToPose[k], 0, forward2 );
				MatrixGetColumn( boneToPose[k], 1, left2 );
				MatrixGetColumn( boneToPose[k], 2, up2 );
				float d1 = fabs(DotProduct( forward, forward2 ));
				float d2 = fabs(DotProduct( forward, left2 ));
				float d3 = fabs(DotProduct( forward, up2 ));
				if (d1 <= d2 && d1 <= d3)
				{
					up = CrossProduct( forward, forward2 );
					VectorNormalize( up );
				}
				else if (d2 <= d1 && d2 <= d3)
				{
					up = CrossProduct( forward, left2 );
					VectorNormalize( up );
				}
				else
				{
					up = CrossProduct( forward, up2 );
					VectorNormalize( up );
				}
				left = CrossProduct( up, forward );

				// setup matrix
				MatrixSetColumn( forward, 0, boneToPose[k] );
				MatrixSetColumn( left, 1, boneToPose[k] );
				MatrixSetColumn( up, 2, boneToPose[k] );

				// check orthonormality of matrix
				d =   fabs( DotProduct( forward, left ) ) 
					+ fabs( DotProduct( left, up ) ) 
					+ fabs( DotProduct( up, forward ) )
					+ fabs( DotProduct( boneToPose[k][0], boneToPose[k][1] ) )
					+ fabs( DotProduct( boneToPose[k][1], boneToPose[k][2] ) )
					+ fabs( DotProduct( boneToPose[k][2], boneToPose[k][0] ) );

				if (d > 0.0001)
				{
					Error( "error with realigning bone %s\n", g_bonetable[k].name );
				}

				// printf("%f %f %f\n", DotProduct( boneToPose[k][0], boneToPose[k][1] ), DotProduct( boneToPose[k][1], boneToPose[k][2] ), DotProduct( boneToPose[k][2], boneToPose[k][0] ) );

				// printf("%f %f %f\n", DotProduct( forward, left ), DotProduct( left, up ), DotProduct( up, forward ) );

				// VectorMatrix( forward, boneToPose[k] );

				MatrixSetColumn( v3, 3, boneToPose[k] );
			}
		}
	}

	for (i = 0; i < g_numforcedrealign; i++)
	{
		k = findGlobalBone( g_forcedrealign[i].name );
		if (k == -1)
		{
			Error( "unknown bone %s in $forcedrealign\n", g_forcedrealign[i].name );
		}

		matrix3x4_t local;
		matrix3x4_t tmp;

		AngleMatrix( g_forcedrealign[i].rot, local );
		ConcatTransforms( boneToPose[k], local, tmp );
		MatrixCopy( tmp, boneToPose[k] );
	}

	// build realignment transforms
	for (k = 0; k < g_numbones; k++)
	{
		matrix3x4_t poseToBone;

		MatrixInvert( g_bonetable[k].boneToPose, poseToBone );
		ConcatTransforms( poseToBone, boneToPose[k], g_bonetable[k].srcRealign );

		MatrixCopy( boneToPose[k], g_bonetable[k].boneToPose );
	}

	// printf("\n");

	// rebuild default angles, position, etc.
	for (k = 0; k < g_numbones; k++)
	{
		matrix3x4_t bonematrix;
		if (g_bonetable[k].parent == -1)
		{
			MatrixCopy( g_bonetable[k].boneToPose, bonematrix );
		}
		else
		{
			matrix3x4_t poseToBone;
			// convert my transform into parent relative space
			MatrixInvert( g_bonetable[g_bonetable[k].parent].boneToPose, poseToBone );
			ConcatTransforms( poseToBone, g_bonetable[k].boneToPose, bonematrix );
		}

		MatrixAngles( bonematrix, g_bonetable[k].rot, g_bonetable[k].pos );
	}

	// exit(0);

	// printf("\n");

	// build reference pose
	for (k = 0; k < g_numbones; k++)
	{
		matrix3x4_t bonematrix;
		AngleMatrix( g_bonetable[k].rot, g_bonetable[k].pos, bonematrix );
		// MatrixCopy( g_bonetable[k].rawLocal, bonematrix );
		if (g_bonetable[k].parent == -1)
		{
			MatrixCopy( bonematrix, g_bonetable[k].boneToPose );
		}
		else
		{
			ConcatTransforms (g_bonetable[g_bonetable[k].parent].boneToPose, bonematrix, g_bonetable[k].boneToPose);
		}
		/*
		Vector v1;
		MatrixGetColumn( g_bonetable[k].boneToPose, 3, v1 );
		printf("%s  %.4f %.4f %.4f\n", g_bonetable[k].name, v1.x, v1.y, v1.z );
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: find all the different bones used in all the source files and map everything
//			to a common bonetable.
//-----------------------------------------------------------------------------
void RemapBones( )
{
	int	iError = 0;

	if (g_staticprop)
	{
		MakeStaticProp( );
	}
	else if ( g_centerstaticprop )
	{
		printf("WARNING: Ignoring option $autocenter.  Only supported on $staticprop models!!!\n" );
	}


	TagUsedBones( );

	RenameBones( );

	iError = BuildGobalBonetable( );

	BuildGlobalBoneToPose( );

	if (g_collapse_bones)
	{
		CollapseBones( );
	}

	/*
	for (i = 0; i < g_numbones; i++)
	{
		printf("%2d %s %d\n", i, g_bonetable[i].name, g_bonetable[i].parent );
	}
	*/

	EnforceHeirarchy( );

	RebuildLocalPose( );

	TagProceduralBones( );

	if (iError && !(ignore_warnings))
	{
		exit( 1 );
	}

	if (g_numbones >= MAXSTUDIOBONES)
	{
		Error( "Too many bones used in model, used %d, max %d\n", g_numbones, MAXSTUDIOBONES );
	}

	MapSourcesToGlobalBonetable( );

	if (iError && !(ignore_warnings))
	{
		exit( 1 );
	}
}



//-----------------------------------------------------------------------------
// Purpose: calculate the bone to world transforms for a processed animation
//-----------------------------------------------------------------------------
void CalcBoneTransforms( s_animation_t *panimation, int frame, matrix3x4_t* pBoneToWorld )
{
	frame = frame % panimation->numframes;

	for (int k = 0; k < g_numbones; k++)
	{
		Vector angle;
		matrix3x4_t bonematrix;

		if (!(panimation->flags & STUDIO_DELTA))
		{
			AngleMatrix( panimation->sanim[frame][k].rot, panimation->sanim[frame][k].pos, bonematrix );
		}
		else
		{
			Quaternion q1, q2, q3;
			Vector p3;

			//AngleQuaternion( g_bonetable[k].rot, q1 );
			AngleQuaternion( g_panimation[0]->sanim[0][k].rot, q1 );
			AngleQuaternion( panimation->sanim[frame][k].rot, q2 );

			float s = panimation->weight[k];

			QuaternionMA( q1, s, q2, q3 );
			//p3 = g_bonetable[k].pos + s * panimation->sanim[frame][k].pos;
			p3 = g_panimation[0]->sanim[0][k].pos + s * panimation->sanim[frame][k].pos;

			AngleMatrix( q3, p3, bonematrix );
		}

		if (g_bonetable[k].parent == -1)
		{
			MatrixCopy( bonematrix, pBoneToWorld[k] );
		}
		else
		{
			ConcatTransforms (pBoneToWorld[g_bonetable[k].parent], bonematrix, pBoneToWorld[k]);
		}
	}
}


void CalcBonePos( s_animation_t *panimation, int frame, int bone, Vector &pos )
{
	matrix3x4_t boneToWorld[MAXSTUDIOSRCBONES];	// bone transformation matrix

	CalcBoneTransforms( panimation, frame, boneToWorld );

	pos.x = boneToWorld[bone][0][3];
	pos.y = boneToWorld[bone][1][3];
	pos.z = boneToWorld[bone][2][3];
}


#define SMALL_FLOAT 1e-12

// NOTE: This routine was taken (and modified) from NVidia's BlinnReflection demo
// Creates basis vectors, based on a vertex and index list.
// See the NVidia white paper 'GDC2K PerPixel Lighting' for a description
// of how this computation works
static void CalcTriangleTangentSpace( s_source_t *pSrc, int v1, int v2, int v3, 
									  Vector &sVect, Vector &tVect )
{
/*
	static bool firstTime = true;
	static FILE *fp = NULL;
	if( firstTime )
	{
		firstTime = false;
		fp = fopen( "crap.out", "w" );
	}
*/
    
	/* Compute the partial derivatives of X, Y, and Z with respect to S and T. */
	Vector2D t0( pSrc->texcoord[v1][0], pSrc->texcoord[v1][1] );
	Vector2D t1( pSrc->texcoord[v2][0], pSrc->texcoord[v2][1] );
	Vector2D t2( pSrc->texcoord[v3][0], pSrc->texcoord[v3][1] );
	Vector p0( pSrc->vertex[v1][0], pSrc->vertex[v1][1], pSrc->vertex[v1][2] );
	Vector p1( pSrc->vertex[v2][0], pSrc->vertex[v2][1], pSrc->vertex[v2][2] );
	Vector p2( pSrc->vertex[v3][0], pSrc->vertex[v3][1], pSrc->vertex[v3][2] );

	sVect.Init( 0.0f, 0.0f, 0.0f );
	tVect.Init( 0.0f, 0.0f, 0.0f );

	// x, s, t
	Vector edge01 = Vector( p1.x - p0.x, t1.x - t0.x, t1.y - t0.y );
	Vector edge02 = Vector( p2.x - p0.x, t2.x - t0.x, t2.y - t0.y );

	Vector cross;
	CrossProduct( edge01, edge02, cross );
	if( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.x += -cross.y / cross.x;
		tVect.x += -cross.z / cross.x;
	}

	// y, s, t
	edge01 = Vector( p1.y - p0.y, t1.x - t0.x, t1.y - t0.y );
	edge02 = Vector( p2.y - p0.y, t2.x - t0.x, t2.y - t0.y );

	CrossProduct( edge01, edge02, cross );
	if( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.y += -cross.y / cross.x;
		tVect.y += -cross.z / cross.x;
	}
	
	// z, s, t
	edge01 = Vector( p1.z - p0.z, t1.x - t0.x, t1.y - t0.y );
	edge02 = Vector( p2.z - p0.z, t2.x - t0.x, t2.y - t0.y );

	CrossProduct( edge01, edge02, cross );
	if( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.z += -cross.y / cross.x;
		tVect.z += -cross.z / cross.x;
	}

	// Normalize sVect and tVect
	VectorNormalize( sVect );
	VectorNormalize( tVect );

/*
	// Calculate flat normal
	Vector flatNormal;
	edge01 = p1 - p0;
	edge02 = p2 - p0;
	CrossProduct( edge02, edge01, flatNormal );
	VectorNormalize( flatNormal );
	
	// Get the average position
	Vector avgPos = ( p0 + p1 + p2 ) / 3.0f;

	// Draw the svect
	Vector endS = avgPos + sVect * .2f;
	fprintf( fp, "2\n" );
	fprintf( fp, "%f %f %f 1.0 0.0 0.0\n", endS[0], endS[1], endS[2] );
	fprintf( fp, "%f %f %f 1.0 0.0 0.0\n", avgPos[0], avgPos[1], avgPos[2] );
	
	// Draw the tvect
	Vector endT = avgPos + tVect * .2f;
	fprintf( fp, "2\n" );
	fprintf( fp, "%f %f %f 0.0 1.0 0.0\n", endT[0], endT[1], endT[2] );
	fprintf( fp, "%f %f %f 0.0 1.0 0.0\n", avgPos[0], avgPos[1], avgPos[2] );
	
	// Draw the normal
	Vector endN = avgPos + flatNormal * .2f;
	fprintf( fp, "2\n" );
	fprintf( fp, "%f %f %f 0.0 0.0 1.0\n", endN[0], endN[1], endN[2] );
	fprintf( fp, "%f %f %f 0.0 0.0 1.0\n", avgPos[0], avgPos[1], avgPos[2] );
	
	// Draw the wireframe of the triangle in white.
	fprintf( fp, "2\n" );
	fprintf( fp, "%f %f %f 1.0 1.0 1.0\n", p0[0], p0[1], p0[2] );
	fprintf( fp, "%f %f %f 1.0 1.0 1.0\n", p1[0], p1[1], p1[2] );
	fprintf( fp, "2\n" );
	fprintf( fp, "%f %f %f 1.0 1.0 1.0\n", p1[0], p1[1], p1[2] );
	fprintf( fp, "%f %f %f 1.0 1.0 1.0\n", p2[0], p2[1], p2[2] );
	fprintf( fp, "2\n" );
	fprintf( fp, "%f %f %f 1.0 1.0 1.0\n", p2[0], p2[1], p2[2] );
	fprintf( fp, "%f %f %f 1.0 1.0 1.0\n", p0[0], p0[1], p0[2] );

	// Draw a slightly shrunken version of the geometry to hide surfaces
	Vector tmp0 = p0 - flatNormal * .1f;
	Vector tmp1 = p1 - flatNormal * .1f;
	Vector tmp2 = p2 - flatNormal * .1f;
	fprintf( fp, "3\n" );
	fprintf( fp, "%f %f %f 0.1 0.1 0.1\n", tmp0[0], tmp0[1], tmp0[2] );
	fprintf( fp, "%f %f %f 0.1 0.1 0.1\n", tmp1[0], tmp1[1], tmp1[2] );
	fprintf( fp, "%f %f %f 0.1 0.1 0.1\n", tmp2[0], tmp2[1], tmp2[2] );
		
	fflush( fp );
*/
}

typedef CUtlVector<int> CIntVector;

static void CalcModelTangentSpaces( s_source_t *pSrc )
{
	// Build a map from vertex to a list of triangles that share the vert.
//Vector &b = pSrc->vertex[pMesh->vertexoffset + pFace->b];


	int meshID;
	for( meshID = 0; meshID < pSrc->nummeshes; meshID++ )
	{
		s_mesh_t *pMesh = &pSrc->mesh[pSrc->meshindex[meshID]];
		CUtlVector<CIntVector> vertToTriMap;
		vertToTriMap.AddMultipleToTail( pMesh->numvertices );
		int triID;
		for( triID = 0; triID < pMesh->numfaces; triID++ )
		{
			s_face_t *pFace = &pSrc->face[triID + pMesh->faceoffset];
			vertToTriMap[pFace->a].AddToTail( triID );
			vertToTriMap[pFace->b].AddToTail( triID );
			vertToTriMap[pFace->c].AddToTail( triID );
		}

		// Calculate the tangent space for each triangle.
		CUtlVector<Vector> triSVect;
		CUtlVector<Vector> triTVect;
		triSVect.AddMultipleToTail( pMesh->numfaces );
		triTVect.AddMultipleToTail( pMesh->numfaces );
		for( triID = 0; triID < pMesh->numfaces; triID++ )
		{
			s_face_t *pFace = &pSrc->face[triID + pMesh->faceoffset];
			CalcTriangleTangentSpace( pSrc, 
				pMesh->vertexoffset + pFace->a, 
				pMesh->vertexoffset + pFace->b, 
				pMesh->vertexoffset + pFace->c, 
				triSVect[triID], triTVect[triID] );
		}	

		// calculate an average tangent space for each vertex.
		int vertID;
		for( vertID = 0; vertID < pMesh->numvertices; vertID++ )
		{
			const Vector &normal	= pSrc->normal[vertID+pMesh->vertexoffset];
			Vector4D &finalSVect			= pSrc->tangentS[vertID+pMesh->vertexoffset];
			Vector sVect, tVect;

			sVect.Init( 0.0f, 0.0f, 0.0f );
			tVect.Init( 0.0f, 0.0f, 0.0f );
			for( triID = 0; triID < vertToTriMap[vertID].Size(); triID++ )
			{
				sVect += triSVect[vertToTriMap[vertID][triID]];
				tVect += triTVect[vertToTriMap[vertID][triID]];
			}
			// make an orthonormal system.
			// need to check if we are left or right handed.
			Vector tmpVect;
			CrossProduct( sVect, tVect, tmpVect );
			bool leftHanded = DotProduct( tmpVect, normal ) < 0.0f;
			// calculate as righthanded
			if( !leftHanded )
			{
				CrossProduct( normal, sVect, tVect );
				CrossProduct( tVect, normal, sVect );
				VectorNormalize( sVect );
				VectorNormalize( tVect );
				finalSVect[0] = sVect[0];
				finalSVect[1] = sVect[1];
				finalSVect[2] = sVect[2];
				finalSVect[3] = 1.0f;
			}
			else
			{
				CrossProduct( sVect, normal, tVect );
				CrossProduct( normal, tVect, sVect );
				VectorNormalize( sVect );
				VectorNormalize( tVect );
				finalSVect[0] = sVect[0];
				finalSVect[1] = sVect[1];
				finalSVect[2] = sVect[2];
				finalSVect[3] = -1.0f;
			}
		}
	}
}

static void CalcTangentSpaces( void )
{
	int modelID;

	// todo: need to fixup the firstref/lastref stuff . . do we really need it anymore?
	for( modelID = 0; modelID < g_nummodelsbeforeLOD; modelID++ )
	{
		CalcModelTangentSpaces( g_model[modelID]->source );
	}
}

//-----------------------------------------------------------------------------
// When read off disk, s_source_t contains bone indices local to the source
// we need to make the bone indices use the global bone list
//-----------------------------------------------------------------------------

void RemapVertices( )
{
	for (int i = 0; i < g_numsources; i++)
	{
		s_source_t *psource = g_source[i];

		if (!psource->localBoneweight)
			continue;

		Vector tmp1, tmp2, vdest, ndest;

		matrix3x4_t srcBoneToWorld[MAXSTUDIOSRCBONES];
		matrix3x4_t destBoneToWorld[MAXSTUDIOSRCBONES];

		BuildRawTransforms( psource, 0, srcBoneToWorld );
		TranslateAnimations( psource, srcBoneToWorld, destBoneToWorld );
		
		// allocate memory for vertex remapping
		psource->globalBoneweight = (s_boneweight_t *)kalloc( psource->numvertices, sizeof( s_boneweight_t ) );

		for (int j = 0; j < psource->numvertices; j++)
		{
			vdest.Init();
			ndest.Init();

			int n;
			for (n = 0; n < psource->localBoneweight[j].numbones; n++)
			{
				// src bone
				int q = psource->localBoneweight[j].bone[n];
				// mapping to global bone
				int k = psource->boneLocalToGlobal[q];
				
				if (k == -1)
				{
					k = 0;
					// printf("%s:%s (%d) missing global\n", psource->filename, psource->localBone[q].name, q );
				}

				Assert( k != -1 );

				int m;
				for (m = 0; m < psource->globalBoneweight[j].numbones; m++)
				{
					if (k == psource->globalBoneweight[j].bone[m])
					{
						// bone got collapsed out
						psource->globalBoneweight[j].weight[m] += psource->localBoneweight[j].weight[n];
						break;
					}
				}
				if (m == psource->globalBoneweight[j].numbones)
				{
					// add new bone
					psource->globalBoneweight[j].bone[m] = k;
					psource->globalBoneweight[j].weight[m] = psource->localBoneweight[j].weight[n];
					psource->globalBoneweight[j].numbones++;
				}

				// convert vertex into original models' bone local space
				VectorITransform( psource->vertex[j], destBoneToWorld[k], tmp1 );
				// convert that into global world space using stardard pose
				VectorTransform( tmp1, g_bonetable[k].boneToPose, tmp2 );
				// accumulate
				VectorMA( vdest, psource->localBoneweight[j].weight[n], tmp2, vdest );

				// convert normal into original models' bone local space
				VectorIRotate( psource->normal[j], destBoneToWorld[k], tmp1 );
				// convert that into global world space using stardard pose
				VectorRotate( tmp1, g_bonetable[k].boneToPose, tmp2 );
				// accumulate
				VectorMA( ndest, psource->localBoneweight[j].weight[n], tmp2, ndest );
			}

			// printf("%d  %.2f %.2f %.2f\n", j, vdest.x, vdest.y, vdest.z );

			// save, normalize
			VectorCopy( vdest, psource->vertex[j] );
			VectorNormalize( ndest );
			VectorCopy( ndest, psource->normal[j] );
		}
	}
}

//-----------------------------------------------------------------------------
// Links bone controllers
//-----------------------------------------------------------------------------

static void FindAutolayers()
{
	for (int i = 0; i < g_numseq; i++)
	{
		for (int k = 0; k < g_sequence[i].numautolayers; k++)
		{
			int j;
			for ( j = 0; j < g_numseq; j++)
			{
				if (stricmp( g_sequence[i].autolayer[k].name, g_sequence[j].name) == 0)
				{
					g_sequence[i].autolayer[k].sequence = j;
					break;
				}
			}
			if (j == g_numseq)
			{
				Error( "sequence \"%s\" cannot find autolayer sequence \"%s\"\n",
					g_sequence[i].name, g_sequence[i].autolayer[k].name );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Links bone controllers
//-----------------------------------------------------------------------------

static void LinkBoneControllers()
{
	for (int i = 0; i < g_numbonecontrollers; i++)
	{
		int j = findGlobalBone( g_bonecontroller[i].name );
		if (j == -1)
		{
			Error("unknown g_bonecontroller link '%s'\n", g_bonecontroller[i].name );
		}
		g_bonecontroller[i].bone = j;
	}
}

//-----------------------------------------------------------------------------
// Links screen aligned bones
//-----------------------------------------------------------------------------

static void TagScreenAlignedBones()
{
	for (int i = 0; i < g_numscreenalignedbones; i++)
	{
		int j = findGlobalBone( g_screenalignedbone[i].name );
		if (j == -1)
		{
			Error("unknown g_screenalignedbone link '%s'\n", g_screenalignedbone[i].name );
		}

		g_bonetable[j].flags |= g_screenalignedbone[i].flags;
		printf("tagging bone: %s as screen aligned (index %i, flags:%x)\n", g_bonetable[j].name, j, g_bonetable[j].flags );
	}
}

//-----------------------------------------------------------------------------
// Links attachments
//-----------------------------------------------------------------------------

static void LinkAttachments()
{
	int i, j, k;

	// attachments may be connected to bones that can be optimized out
	// so search through all the sources and move to a valid location

	matrix3x4_t boneToPose;
	matrix3x4_t world;
	matrix3x4_t poseToBone;

	for (i = 0; i < g_numattachments; i++)
	{
		bool found = false;
		// search through known bones
		for (k = 0; k < g_numbones; k++)
		{
			if ( !stricmp( g_attachment[i].bonename, g_bonetable[k].name ))
			{
				g_attachment[i].bone = k;
				MatrixCopy( g_bonetable[k].boneToPose, boneToPose );
				MatrixInvert( boneToPose, poseToBone );
				// printf("%s : %d\n", g_bonetable[k].name, k );
				found = true;
				break;
			}
		}

		if (!found)
		{
			// search all the loaded sources for the first occurance of the named bone
			for (j = 0; j < g_numsources && !found; j++)
			{
				for (k = 0; k < g_source[j]->numbones && !found; k++)
				{
					if ( !stricmp( g_attachment[i].bonename, g_source[j]->localBone[k].name ) )
					{
						MatrixCopy( g_source[j]->boneToPose[k], boneToPose );

						// check to make sure that this bone is actually referenced in the output model
						// if not, try parent bone until we find a referenced bone in this chain
						while (k != -1 && g_source[j]->boneGlobalToLocal[g_source[j]->boneLocalToGlobal[k]] != k)
						{
							k = g_source[j]->localBone[k].parent;
						}
						if (k == -1)
						{
							Error( "unable to find valid bone for attachment %s:%s\n", 
								g_attachment[i].name,
								g_attachment[i].bonename );
						}

						MatrixInvert( g_source[j]->boneToPose[k], poseToBone );
						g_attachment[i].bone = g_source[j]->boneLocalToGlobal[k];
						found = true;
					}
				}
			}
		}

		if (!found)
		{
			Error("unknown attachment link '%s'\n", g_attachment[i].bonename );
		}
		// printf("%s: %s / %s\n", g_attachment[i].name, g_attachment[i].bonename, g_bonetable[g_attachment[i].bone].name );

		if (g_attachment[i].type & IS_ABSOLUTE)
		{
			MatrixCopy( g_attachment[i].local, world );
		}
		else
		{
			ConcatTransforms( boneToPose, g_attachment[i].local, world );
		}

		ConcatTransforms( poseToBone, world, g_attachment[i].local );
	}

	// flag all bones used by attachments
	for (i = 0; i < g_numattachments; i++)
	{
		j = g_attachment[i].bone;
		while (j != -1)
		{
			g_bonetable[j].flags |= BONE_USED_BY_ATTACHMENT;
			j = g_bonetable[j].parent;
		}
	}
}

//-----------------------------------------------------------------------------
// Links mouths
//-----------------------------------------------------------------------------

static void LinkMouths()
{
	for (int i = 0; i < g_nummouths; i++)
	{
		int j;
		for ( j = 0; j < g_numbones; j++)
		{
			if (g_mouth[i].bonename[0] && stricmp( g_mouth[i].bonename, g_bonetable[j].name) == 0)
				break;
		}
		if (j >= g_numbones)
		{
			Error("unknown mouth link '%s'\n", g_mouth[i].bonename );
		}
		g_mouth[i].bone = j;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static float CalcPoseParameterValue( int control, RadianEuler &angle, Vector &pos )
{
	switch( control )
	{
	case STUDIO_X:
		return pos.x;
	case STUDIO_Y:
		return pos.y;
	case STUDIO_Z:
		return pos.z;
	case STUDIO_XR:
		return RAD2DEG( angle.x );
	case STUDIO_YR:
		return RAD2DEG( angle.y );
	case STUDIO_ZR:
		return RAD2DEG( angle.z );
	}
	return 0.0;
}

static void CalcPoseParameters( void )
{
	int i;
	matrix3x4_t boneToWorld[MAXSTUDIOBONES];
	RadianEuler angles;
	Vector pos;

	for (i = 0; i < g_numseq; i++)
	{
		s_sequence_t *pseq = &g_sequence[i];

		for (int iPose = 0; iPose < 2; iPose++)
		{
			if (pseq->groupsize[iPose] > 1)
			{
				if (pseq->paramattachment[iPose] != -1)
				{
					int j0 = pseq->paramindex[iPose];
					int n0 = pseq->paramattachment[iPose];
					int k0 = g_attachment[n0].bone;

					matrix3x4_t boneToWorldRel;
					matrix3x4_t boneToWorldMid;
					matrix3x4_t worldToBoneMid;
					matrix3x4_t boneRel;

					// printf("%s\n", pseq->name );

					// FIXME: the $calcblend should set what animation to use as a reference
					CalcBoneTransforms( g_panimation[0], 0, boneToWorld );
					ConcatTransforms( boneToWorld[k0], g_attachment[n0].local, boneToWorldMid );
					MatrixAngles( boneToWorldMid, angles, pos );
					// printf("%6.2f %6.2f %6.2f : %6.2f %6.2f %6.2f\n", RAD2DEG( angles.x ), RAD2DEG( angles.y ), RAD2DEG( angles.z ), pos.x, pos.y, pos.z );
					MatrixInvert( boneToWorldMid, worldToBoneMid );

					int m[2];
					m[1-iPose] = (pseq->groupsize[1-iPose] - 1) / 2;
					for (m[iPose] = 0; m[iPose] < pseq->groupsize[iPose]; m[iPose]++)
					{
						CalcBoneTransforms( pseq->panim[m[0]][m[1]], 0, boneToWorld );
						ConcatTransforms( boneToWorld[k0], g_attachment[n0].local, boneToWorldRel );
						ConcatTransforms( worldToBoneMid, boneToWorldRel, boneRel );
						MatrixAngles( boneRel, angles, pos );
						// printf("%6.2f %6.2f %6.2f : %6.2f %6.2f %6.2f\n", RAD2DEG( angles.x ), RAD2DEG( angles.y ), RAD2DEG( angles.z ), pos.x, pos.y, pos.z );

						if (iPose == 0)
						{
							pseq->param0[m[iPose]] = CalcPoseParameterValue( pseq->paramcontrol[iPose], angles, pos );
						}
						else
						{
							pseq->param1[m[iPose]] = CalcPoseParameterValue( pseq->paramcontrol[iPose], angles, pos );
						}


						// pseq->param1[i0][i1] = CalcPoseParameterValue( pseq->paramcontrol[1], angles, pos );

						if (m[iPose] == 0)
						{
							pseq->paramstart[iPose] = pseq->param0[m[iPose]];
						}
						if (m[iPose] == pseq->groupsize[iPose] - 1)
						{
							pseq->paramend[iPose] = pseq->param0[m[iPose]];
						}
					}
					printf("%s : %6.2f %6.2f\n", pseq->name, pseq->paramstart[iPose], pseq->paramend[iPose] );

					if (fabs( pseq->paramstart[iPose] - pseq->paramend[iPose]) < 0.01 )
					{
						Error( "calcblend failed in %s\n", pseq->name );
					}

					g_pose[j0].min = min( g_pose[j0].min, pseq->paramstart[iPose] );
					g_pose[j0].max = max( g_pose[j0].max, pseq->paramstart[iPose] );
					g_pose[j0].min = min( g_pose[j0].min, pseq->paramend[iPose] );
					g_pose[j0].max = max( g_pose[j0].max, pseq->paramend[iPose] );
				}
				else
				{

					for (int m = 0; m < pseq->groupsize[iPose]; m++)
					{
						float f = (m / (float)(pseq->groupsize[iPose] - 1.0));
						if (iPose == 0)
						{
							pseq->param0[m] = pseq->paramstart[iPose] * (1.0 - f) + pseq->paramend[iPose] * f;
						}
						else
						{
							pseq->param1[m] = pseq->paramstart[iPose] * (1.0 - f) + pseq->paramend[iPose] * f;
						}
					}
				}
			}
		}
	}
	// exit(0);
}



//-----------------------------------------------------------------------------
// Link ikchains
//-----------------------------------------------------------------------------

static void LinkIKChains( )
{
	int i, k;

	// create IK links
	for (i = 0; i < g_numikchains; i++)
	{
		g_ikchain[i].numlinks = 3;

		k = findGlobalBone( g_ikchain[i].bonename );
		if (k == -1)
		{
			Error("unknown bone '%s' in ikchain '%s'\n", g_ikchain[i].bonename, g_ikchain[i].name );
		}
		g_ikchain[i].link[2] = k;

		k = g_bonetable[k].parent;
		if (k == -1)
		{
			Error("ikchain '%s' too close to root, no parent knee/elbow\n", g_ikchain[i].name );
		}
		g_ikchain[i].link[1] = k;

		k = g_bonetable[k].parent;
		if (k == -1)
		{
			Error("ikchain '%s' too close to root, no parent hip/shoulder\n", g_ikchain[i].name );
		}
		g_ikchain[i].link[0] = k;

		// FIXME: search for toes
	}
}

//-----------------------------------------------------------------------------
// Link ikchains
//-----------------------------------------------------------------------------

static void LinkIKLocks( )
{
	int i, j;

	// create IK links
	for (i = 0; i < g_numikautoplaylocks; i++)
	{
		for (j = 0; j < g_numikchains; j++)
		{
			if (stricmp( g_ikchain[j].name, g_ikautoplaylock[i].name) == 0)
			{
				break;
			}
		}
		if (j == g_numikchains)
		{
			Error("unknown chain '%s' in ikautoplaylock\n", g_ikautoplaylock[i].name );
		}

		g_ikautoplaylock[i].chain = j;
	}

	int k;

	for (k = 0; k < g_numseq; k++)
	{
		for (i = 0; i < g_sequence[k].numiklocks; i++)
		{
			for (j = 0; j < g_numikchains; j++)
			{
				if (stricmp( g_ikchain[j].name, g_sequence[k].iklock[i].name) == 0)
				{
					break;
				}
			}
			if (j == g_numikchains)
			{
				Error("unknown chain '%s' in sequence iklock\n", g_sequence[k].iklock[i].name );
			}

			g_sequence[k].iklock[i].chain = j;
		}
	}
}

//-----------------------------------------------------------------------------
// Process IK links
//-----------------------------------------------------------------------------

s_ikrule_t *FindPrevIKRule( s_animation_t *panim, int iRule )
{
	int i, j;

	s_ikrule_t *pRule = &panim->ikrule[iRule];

	for (i = 1; i < panim->numikrules; i++)
	{
		j =  ( iRule - i + panim->numikrules) % panim->numikrules;
		if (panim->ikrule[j].chain == pRule->chain)
			return &panim->ikrule[j];
	}
	return pRule;
}

s_ikrule_t *FindNextIKRule( s_animation_t *panim, int iRule )
{
	int i, j;

	s_ikrule_t *pRule = &panim->ikrule[iRule];

	for (i = 1; i < panim->numikrules; i++)
	{
		j =  (iRule + i ) % panim->numikrules;
		if (panim->ikrule[j].chain == pRule->chain)
			return &panim->ikrule[j];
	}
	return pRule;
}

//-----------------------------------------------------------------------------
// Purpose: go through all the IK rules and calculate the animated path the IK'd 
//			end point moves relative to its IK target.
//-----------------------------------------------------------------------------

static void ProcessIKRules( )
{
	int i, j, k;

	// copy source animations
	for (i = 0; i < g_numani; i++)
	{
		s_animation_t *panim = g_panimation[i];

		for (j = 0; j < panim->numcmds; j++)
		{
			if (panim->cmds[j].cmd != CMD_IKRULE)
				continue;

			if (panim->numikrules >= MAXSTUDIOIKRULES)
			{
				Error("Too many IK rules in %s (%s)\n", panim->name, panim->filename );
			}
			s_ikrule_t *pRule = &panim->ikrule[panim->numikrules++];

			// make a copy of the rule;
			*pRule = *panim->cmds[j].u.ikrule.pRule;
		}

		for (j = 0; j < panim->numikrules; j++)
		{
			s_ikrule_t *pRule = &panim->ikrule[j];

			if (pRule->start == 0 && pRule->peak == 0 && pRule->tail == 0 && pRule->end == 0)
			{
				pRule->tail = panim->numframes - 1;
				pRule->end = panim->numframes - 1;
			}

			if (pRule->peak == -1)
			{
				pRule->start = 0;
				pRule->peak = 0;
			}

			if (pRule->tail == -1)
			{
				pRule->tail = panim->numframes - 1;
				pRule->end = panim->numframes - 1;
			}

			if (pRule->contact == -1)
			{
				pRule->contact = pRule->peak;
			}

			// huh, make up start and end numbers
			if (pRule->start == -1)
			{
				s_ikrule_t *pPrev = FindPrevIKRule( panim, j );

				if (pPrev->slot == pRule->slot)
				{
					if (pRule->peak < pPrev->tail)
					{
						pRule->start = pRule->peak + (pPrev->tail - pRule->peak) / 2;
					}
					else
					{
						pRule->start = pRule->peak + (pPrev->tail - pRule->peak + panim->numframes - 1) / 2;
					}
					pRule->start = (pRule->start + panim->numframes / 2) % (panim->numframes - 1);
					pPrev->end = (pRule->start + panim->numframes - 2) % (panim->numframes - 1);
				}
				else
				{
					pRule->start = pPrev->tail;
					pPrev->end = pRule->peak;
				}
				// printf("%s : %d (%d) : %d %d %d %d\n", panim->name, pRule->chain, panim->numframes - 1, pRule->start, pRule->peak, pRule->tail, pRule->end );
			}

			// huh, make up start and end numbers
			if (pRule->end == -1)
			{
				s_ikrule_t *pNext = FindNextIKRule( panim, j );

				if (pNext->slot == pRule->slot)
				{
					if (pNext->peak < pRule->tail)
					{
						pNext->start = pNext->peak + (pRule->tail - pNext->peak) / 2;
					}
					else
					{
						pNext->start = pNext->peak + (pRule->tail - pNext->peak + panim->numframes - 1) / 2;
					}
					pNext->start = (pNext->start + panim->numframes / 2) % (panim->numframes - 1);
					pRule->end = (pNext->start + panim->numframes - 2) % (panim->numframes - 1);
				}
				else
				{
					pNext->start = pRule->tail;
					pRule->end = pNext->peak;
				}
				// printf("%s : %d (%d) : %d %d %d %d\n", panim->name, pRule->chain, panim->numframes - 1, pRule->start, pRule->peak, pRule->tail, pRule->end );
			}

			// check for wrapping
			if (pRule->peak < pRule->start)
			{
				pRule->peak += panim->numframes - 1;
			}
			if (pRule->tail < pRule->peak)
			{
				pRule->tail += panim->numframes - 1;
			}
			if (pRule->end < pRule->tail)
			{
				pRule->end += panim->numframes - 1;
			}
			if (pRule->contact < pRule->start)
			{
				pRule->contact += panim->numframes - 1;
			}

			// printf("%s : %d (%d) : %d %d %d %d\n", panim->name, pRule->chain, panim->numframes - 1, pRule->start, pRule->peak, pRule->tail, pRule->end );

			pRule->numerror = pRule->end - pRule->start + 1;
			if (pRule->end >= panim->numframes)
				pRule->numerror = pRule->numerror + 2;

			pRule->pError = (s_ikerror_t *)kalloc( pRule->numerror, sizeof( s_ikerror_t ));

			switch( pRule->type )
			{
			case IK_SELF:
				{
					matrix3x4_t boneToWorld[MAXSTUDIOBONES];
					matrix3x4_t worldToBone;
					matrix3x4_t local;

					pRule->bone = findGlobalBone( pRule->bonename );
					if (pRule->bone == -1)
					{
						Error("unknown bone '%s' in ikrule\n", pRule->bonename );
					}

					for (k = 0; k < pRule->numerror; k++)
					{
						if (1)
						{
							CalcBoneTransforms( panim, k + pRule->start, boneToWorld );
						}
						else
						{
							matrix3x4_t srcBoneToWorld[MAXSTUDIOBONES];
							BuildRawTransforms( panim->source, k + pRule->start + panim->startframe - panim->source->startframe, srcBoneToWorld );
							TranslateAnimations( panim->source, srcBoneToWorld, boneToWorld );
						}

						MatrixInvert( boneToWorld[pRule->bone], worldToBone );
						ConcatTransforms( worldToBone, boneToWorld[g_ikchain[pRule->chain].link[2]], local );

						MatrixAngles( local, pRule->pError[k].q, pRule->pError[k].pos );

						/*
						printf("%d  %.1f %.1f %.1f : %.1f %.1f %.1f\n", 
							k,
							pRule->pError[k].pos.x, pRule->pError[k].pos.y, pRule->pError[k].pos.z, 
							ang.x, ang.y, ang.z );
						*/
					}
				}
				break;
			case IK_WORLD:
				break;
			case IK_GROUND:
				{
					matrix3x4_t boneToWorld[MAXSTUDIOBONES];
					matrix3x4_t worldToBone;
					matrix3x4_t local;

					int bone = g_ikchain[pRule->chain].link[2];
					CalcBoneTransforms( panim, pRule->contact, boneToWorld );
					// FIXME: add in motion

					Vector footfall;
					MatrixGetColumn( boneToWorld[bone], 3, footfall );
					footfall.z = pRule->floor;

					AngleMatrix( RadianEuler( 0, 0, 0 ), footfall, local );
					MatrixInvert( local, worldToBone );

					pRule->pos = footfall;
					pRule->q = RadianEuler( 0, 0, 0 );
					
#if 0
					printf("%d  %.1f %.1f %.1f\n", 
						pRule->peak,
						pRule->pos.x, pRule->pos.y, pRule->pos.z );
#endif

					for (k = 0; k < pRule->numerror; k++)
					{
						CalcBoneTransforms( panim, k + pRule->start, boneToWorld );

						Vector pos = pRule->pos + calcMovement( panim, pRule->start + k, pRule->contact );

						AngleMatrix( pRule->q, pos, local );
						MatrixInvert( local, worldToBone );

						// calc position error
						ConcatTransforms( worldToBone, boneToWorld[bone], local );
						MatrixAngles( local, pRule->pError[k].q, pRule->pError[k].pos );

#if 0
						QAngle ang;
						QuaternionAngles( pRule->pError[k].q, ang );
						printf("%d  %.1f %.1f %.1f : %.1f %.1f %.1f\n", 
							k + pRule->start,
							pRule->pError[k].pos.x, pRule->pError[k].pos.y, pRule->pError[k].pos.z, 
							ang.x, ang.y, ang.z );
#endif
					}
				}
				break;
			case IK_RELEASE:
				break;
			}
		}
	}
	// exit(0);


	// realign IK across multiple animations
	for (i = 0; i < g_numseq; i++)
	{
		for (j = 0; j < g_sequence[i].groupsize[0]; j++)
		{
			for (k = 0; k < g_sequence[i].groupsize[1]; k++)
			{
				g_sequence[i].numikrules = max( g_sequence[i].numikrules, g_sequence[i].panim[j][k]->numikrules );
			}
		}

		// FIXME: this doesn't check alignment!!!
		for (j = 0; j < g_sequence[i].groupsize[0]; j++)
		{
			for (k = 0; k < g_sequence[i].groupsize[1]; k++)
			{
				for (int n = 0; n < g_sequence[i].panim[j][k]->numikrules; n++)
				{
					g_sequence[i].panim[j][k]->ikrule[n].index = n;
				}
			}
		}
	}

}


//-----------------------------------------------------------------------------
// CompressAnimations
//-----------------------------------------------------------------------------

static void CompressAnimations( )
{
	int i, j, k, n, m;

	// find scales for all bones
	for (j = 0; j < g_numbones; j++)
	{
		// printf("%s : ", g_bonetable[j].name );
		for (k = 0; k < 6; k++)
		{
			float minv, maxv, scale;

			if (k < 3) 
			{
				minv = -128.0;
				maxv = 128.0;
			}
			else
			{
				minv = -M_PI / 8.0;
				maxv = M_PI / 8.0;
			}

			for (i = 0; i < g_numani; i++)
			{

				s_source_t *psource = g_panimation[i]->source;

				for (n = 0; n < g_panimation[i]->numframes; n++)
				{
					float v;
					switch(k)
					{
					case 0: 
					case 1: 
					case 2: 
						v = ( g_panimation[i]->sanim[n][j].pos[k] - g_bonetable[j].pos[k] ); 
						break;
					case 3:
					case 4:
					case 5:
						v = ( g_panimation[i]->sanim[n][j].rot[k-3] - g_bonetable[j].rot[k-3] ); 
						while (v >= M_PI)
							v -= M_PI * 2;
						while (v < -M_PI)
							v += M_PI * 2;
						break;
					}
					if (v < minv)
						minv = v;
					if (v > maxv)
						maxv = v;
				}
			}
			if (minv < maxv)
			{
				if (-minv> maxv)
				{
					scale = minv / -32768.0;
				}
				else
				{
					scale = maxv / 32767;
				}
			}
			else
			{
				scale = 1.0 / 32.0;
			}
			switch(k)
			{
			case 0: 
			case 1: 
			case 2: 
				g_bonetable[j].posscale[k] = scale;
				break;
			case 3:
			case 4:
			case 5:
				// printf("(%.1f %.1f)", RAD2DEG(minv), RAD2DEG(maxv) );
				// printf("(%.1f)", RAD2DEG(maxv-minv) );
				g_bonetable[j].rotscale[k-3] = scale;
				break;
			}
			// printf("%.0f ", 1.0 / scale );
		}
		// printf("\n" );
	}


	// reduce animations
	for (i = 0; i < g_numani; i++)
	{
		s_source_t *psource = g_panimation[i]->source;

		if (g_bCheckLengths)
		{
			printf("%s\n", g_panimation[i]->name ); 
		}

		if (g_panimation[i]->numframes == 1)
		{
			continue;
		}

		for (j = 0; j < g_numbones; j++)
		{
			// skip bones that are always procedural
			if (g_bonetable[j].flags & BONE_ALWAYS_PROCEDURAL)
			{
				// g_panimation[i]->weight[j] = 0.0;
				continue;
			}

			// skip bones that have no influence
			if (g_panimation[i]->weight[j] < 0.001)
				continue;

			float checkmin[6], checkmax[6];
			for (k = 0; k < 6; k++)
			{
				checkmin[k] = 9999;
				checkmax[k] = -9999;
			}

			for (k = 0; k < 6; k++)
			{
				mstudioanimvalue_t	*pcount, *pvalue;
				float v;
				short value[MAXSTUDIOANIMATIONS];
				mstudioanimvalue_t data[MAXSTUDIOANIMATIONS];

				// find deltas from default pose
				for (n = 0; n < g_panimation[i]->numframes; n++)
				{
					switch(k)
					{
					case 0: /* X Position */
					case 1: /* Y Position */
					case 2: /* Z Position */
						value[n] = ( g_panimation[i]->sanim[n][j].pos[k] - g_bonetable[j].pos[k] ) / g_bonetable[j].posscale[k]; 
						checkmin[k] = min( value[n] * g_bonetable[j].posscale[k], checkmin[k] );
						checkmax[k] = max( value[n] * g_bonetable[j].posscale[k], checkmax[k] );
						break;
					case 3: /* X Rotation */
					case 4: /* Y Rotation */
					case 5: /* Z Rotation */
						v = ( g_panimation[i]->sanim[n][j].rot[k-3] - g_bonetable[j].rot[k-3] ); 
						while (v >= M_PI)
							v -= M_PI * 2;
						while (v < -M_PI)
							v += M_PI * 2;

						checkmin[k] = min( v, checkmin[k] );
						checkmax[k] = max( v, checkmax[k] );
						value[n] = v / g_bonetable[j].rotscale[k-3]; 
						break;
					}
				}
				if (n == 0)
					Error("no animation frames: \"%s\"\n", psource->filename );

				// FIXME: this compression algorithm needs work

				// initialize animation RLE block
				g_panimation[i]->numanim[j][k] = 0;

				memset( data, 0, sizeof( data ) ); 
				pcount = data; 
				pvalue = pcount + 1;

				pcount->num.valid = 1;
				pcount->num.total = 1;
				pvalue->value = value[0];
				pvalue++;

				// build a RLE of deltas from the default pose
				for (m = 1; m < n; m++)
				{
					if (pcount->num.total == 255)
					{
						// chain too long, force a new entry
						pcount = pvalue;
						pvalue = pcount + 1;
						pcount->num.valid++;
						pvalue->value = value[m];
						pvalue++;
					} 
					// insert value if they're not equal, 
					// or if we're not on a run and the run is less than 3 units
					else if ((value[m] != value[m-1]) 
						|| ((pcount->num.total == pcount->num.valid) && ((m < n - 1) && value[m] != value[m+1])))
					{
						if (pcount->num.total != pcount->num.valid)
						{
							//if (j == 0) printf("%d:%d   ", pcount->num.valid, pcount->num.total ); 
							pcount = pvalue;
							pvalue = pcount + 1;
						}
						pcount->num.valid++;
						pvalue->value = value[m];
						pvalue++;
					}
					pcount->num.total++;
				}
				//if (j == 0) printf("%d:%d\n", pcount->num.valid, pcount->num.total ); 

				g_panimation[i]->numanim[j][k] = pvalue - data;
				if (g_panimation[i]->numanim[j][k] == 2 && value[0] == 0)
				{
					g_panimation[i]->numanim[j][k] = 0;
				}
				else
				{
					g_panimation[i]->anim[j][k] = (mstudioanimvalue_t *)kalloc( pvalue - data, sizeof( mstudioanimvalue_t ) );
					memmove( g_panimation[i]->anim[j][k], data, (pvalue - data) * sizeof( mstudioanimvalue_t ) );
				}
				// printf("%d(%d) ", g_source[i]->panim[q]->numanim[j][k], n );
			}

			if (g_bCheckLengths)
			{
				char *tmp[6] = { "X", "Y", "Z", "XR", "YR", "ZR" };
				n = 0;
				for (k = 0; k < 3; k++)
				{
					if (checkmin[k] != 0)
					{
						if (n == 0)
							printf("%s :", g_bonetable[j].name );
					
						printf("%s(%.1f: %.1f %.1f) ", tmp[k], g_bonetable[j].pos[k], checkmin[k], checkmax[k] );
						n = 1;
					}
				}
				if (n)
					printf("\n");
			}
		}
	}
}


void SimplifyModel (void)
{
	int i, j, k;
	int n, m;
	int	iError = 0;

	if (g_numseq == 0)
	{
		Error( "model has no sequences\n");
	}

	// have to load the lod sources before remapping bones so that the remap
	// happens for all LODs.
	LoadLODSources();

	RemapBones( );

	LinkIKChains();

	LinkIKLocks();

	RealignBones( );

	ConvertBoneTreeCollapsesToReplaceBones();

	RemapVertices();
	
	UnifyLODs(); // This also marks bones as being used by lods.
	
	if( g_bPrintBones )
	{
		printf( "Hardware bone usage:\n" );
	}
	SpewBoneUsageStats();

	MarkParentBoneLODs();

	if( g_bPrintBones )
	{
		printf( "CPU bone usage:\n" );
	}
	SpewBoneUsageStats();

	RemapAnimations( );

	processAnimations( );

	limitBoneRotations( );

	limitIKChainLength( );

	MakeTransitions( );
	RemapVertexAnimations( );

	CalcTangentSpaces();

	FindAutolayers( );

	// link bonecontrollers
	LinkBoneControllers();

	// link screen aligned bones
	TagScreenAlignedBones();

	// link attachments
	LinkAttachments();

	// link mouths
	LinkMouths();

	RemapProceduralBones();

	ProcessIKRules();

	CalcPoseParameters( );

	// relink per-model attachments, eyeballs
	for (i = 0; i < g_nummodelsbeforeLOD; i++)
	{
		s_source_t *psource = g_model[i]->source;
		for (j = 0; j < g_model[i]->numattachments; j++)
		{
			k = findGlobalBone( g_model[i]->attachment[j].bonename );
			if (k == -1)
			{
				Error("unknown model attachment link '%s'\n", g_model[i]->attachment[j].bonename );
			}
			g_model[i]->attachment[j].bone = j;
		}

		for (j = 0; j < g_model[i]->numeyeballs; j++)
		{
			g_model[i]->eyeball[j].bone = psource->boneLocalToGlobal[g_model[i]->eyeball[j].bone];
		}
	}

	// set hitgroups
	for (k = 0; k < g_numbones; k++)
	{
		g_bonetable[k].group = -9999;
	}
	for (j = 0; j < g_numhitgroups; j++)
	{
		k = findGlobalBone( g_hitgroup[j].name );
		if (k != -1)
		{
			g_bonetable[k].group = g_hitgroup[j].group;
		}
		else
		{
			Error( "cannot find bone %s for hitgroup %d\n", g_hitgroup[j].name, g_hitgroup[j].group );
		}
	}
	for (k = 0; k < g_numbones; k++)
	{
		if (g_bonetable[k].group == -9999)
		{
			if (g_bonetable[k].parent != -1)
				g_bonetable[k].group = g_bonetable[g_bonetable[k].parent].group;
			else
				g_bonetable[k].group = 0;
		}
	}

	if ( g_hitboxsets.Size() == 0 )
	{
		int index = g_hitboxsets.AddToTail();

		s_hitboxset *set = &g_hitboxsets[ index ];
		memset( set, 0, sizeof( *set) );
		strcpy( set->hitboxsetname, "default" );

		gflags |= STUDIOHDR_FLAGS_AUTOGENERATED_HITBOX;

		// find intersection box volume for each bone
		for (k = 0; k < g_numbones; k++)
		{
			for (j = 0; j < 3; j++)
			{
				g_bonetable[k].bmin[j] = 0.0;
				g_bonetable[k].bmax[j] = 0.0;
			}
		}
		// try all the connect vertices
		for (i = 0; i < g_nummodelsbeforeLOD; i++)
		{
			Vector	p;
			s_source_t *psource = g_model[i]->source;
			for (j = 0; j < psource->numvertices; j++)
			{
				for (n = 0; n < psource->globalBoneweight[j].numbones; n++)
				{
					k = psource->globalBoneweight[j].bone[n];
					VectorITransform( psource->vertex[j], g_bonetable[k].boneToPose, p );

					if (p[0] < g_bonetable[k].bmin[0]) g_bonetable[k].bmin[0] = p[0];
					if (p[1] < g_bonetable[k].bmin[1]) g_bonetable[k].bmin[1] = p[1];
					if (p[2] < g_bonetable[k].bmin[2]) g_bonetable[k].bmin[2] = p[2];
					if (p[0] > g_bonetable[k].bmax[0]) g_bonetable[k].bmax[0] = p[0];
					if (p[1] > g_bonetable[k].bmax[1]) g_bonetable[k].bmax[1] = p[1];
					if (p[2] > g_bonetable[k].bmax[2]) g_bonetable[k].bmax[2] = p[2];
				}
			}
		}
		// add in all your children as well
		for (k = 0; k < g_numbones; k++)
		{
			if ((j = g_bonetable[k].parent) != -1)
			{
				if (g_bonetable[k].pos[0] < g_bonetable[j].bmin[0]) g_bonetable[j].bmin[0] = g_bonetable[k].pos[0];
				if (g_bonetable[k].pos[1] < g_bonetable[j].bmin[1]) g_bonetable[j].bmin[1] = g_bonetable[k].pos[1];
				if (g_bonetable[k].pos[2] < g_bonetable[j].bmin[2]) g_bonetable[j].bmin[2] = g_bonetable[k].pos[2];
				if (g_bonetable[k].pos[0] > g_bonetable[j].bmax[0]) g_bonetable[j].bmax[0] = g_bonetable[k].pos[0];
				if (g_bonetable[k].pos[1] > g_bonetable[j].bmax[1]) g_bonetable[j].bmax[1] = g_bonetable[k].pos[1];
				if (g_bonetable[k].pos[2] > g_bonetable[j].bmax[2]) g_bonetable[j].bmax[2] = g_bonetable[k].pos[2];
			}
		}

		for (k = 0; k < g_numbones; k++)
		{
			if (g_bonetable[k].bmin[0] < g_bonetable[k].bmax[0] - 1
				&& g_bonetable[k].bmin[1] < g_bonetable[k].bmax[1] - 1
				&& g_bonetable[k].bmin[2] < g_bonetable[k].bmax[2] - 1)
			{
				set->hitbox[set->numhitboxes].bone = k;
				set->hitbox[set->numhitboxes].group = g_bonetable[k].group;
				VectorCopy( g_bonetable[k].bmin, set->hitbox[set->numhitboxes].bmin );
				VectorCopy( g_bonetable[k].bmax, set->hitbox[set->numhitboxes].bmax );

				if (dump_hboxes)
				{
					printf("$hbox %d \"%s\" %.2f %.2f %.2f  %.2f %.2f %.2f\n",
						set->hitbox[set->numhitboxes].group,
						g_bonetable[set->hitbox[set->numhitboxes].bone].name, 
						set->hitbox[set->numhitboxes].bmin[0], set->hitbox[set->numhitboxes].bmin[1], set->hitbox[set->numhitboxes].bmin[2],
						set->hitbox[set->numhitboxes].bmax[0], set->hitbox[set->numhitboxes].bmax[1], set->hitbox[set->numhitboxes].bmax[2] );

				}
				set->numhitboxes++;
			}
		}
	}
	else
	{
		gflags &= ~STUDIOHDR_FLAGS_AUTOGENERATED_HITBOX;

		for (int s = 0; s < g_hitboxsets.Size(); s++ )
		{
			s_hitboxset *set = &g_hitboxsets[ s ];

			for (j = 0; j < set->numhitboxes; j++)
			{
				k = findGlobalBone( set->hitbox[j].name );
				if (k != -1)
				{
					set->hitbox[j].bone = k;
				}
				else
				{
					Error( "cannot find bone %s for bbox\n", set->hitbox[j].name );
				}
			}
		}
	}

	for (int s = 0; s < g_hitboxsets.Size(); s++ )
	{
		s_hitboxset *set = &g_hitboxsets[ s ];

		// flag all bones used by hitboxes
		for (j = 0; j < set->numhitboxes; j++)
		{
			k = set->hitbox[j].bone;
			while (k != -1)
			{
				g_bonetable[k].flags |= BONE_USED_BY_HITBOX;
				k = g_bonetable[k].parent;
			}
		}
	}

	CompressAnimations( );

	// exit(0);

	// find bounding box for each g_sequence
	for (i = 0; i < g_numani; i++)
	{
		Vector bmin, bmax;
		
		s_source_t *psource = g_panimation[i]->source;

		// find intersection box volume for each bone
		for (j = 0; j < 3; j++)
		{
			bmin[j] = 9999.0;
			bmax[j] = -9999.0;
		}

		for (j = 0; j < g_panimation[i]->numframes; j++)
		{
			matrix3x4_t bonetransform[MAXSTUDIOBONES];	// bone transformation matrix
			matrix3x4_t posetransform[MAXSTUDIOBONES];	// bone transformation matrix
			matrix3x4_t bonematrix;						// local transformation matrix
			Vector pos;

			for (k = 0; k < g_numbones; k++)
			{
				AngleMatrix( g_panimation[i]->sanim[j][k].rot, g_panimation[i]->sanim[j][k].pos, bonematrix );

				if (g_bonetable[k].parent == -1)
				{
					MatrixCopy( bonematrix, bonetransform[k] );
				}
				else
				{
					ConcatTransforms (bonetransform[g_bonetable[k].parent], bonematrix, bonetransform[k]);
				}

				MatrixInvert( g_bonetable[k].boneToPose, bonematrix );
				ConcatTransforms (bonetransform[k], bonematrix, posetransform[k]);
			}

			for (k = 0; k < g_nummodelsbeforeLOD; k++)
			{
				s_source_t *psource = g_model[k]->source;
				for (n = 0; n < psource->numvertices; n++)
				{
					Vector tmp;
					pos = Vector( 0, 0, 0 );
					for (m = 0; m < psource->globalBoneweight[n].numbones; m++)
					{
						tmp;
						VectorTransform( psource->vertex[n], posetransform[psource->globalBoneweight[n].bone[m]], tmp ); // bug: should use all bones!
						VectorMA( pos, psource->globalBoneweight[n].weight[m], tmp, pos );
					}

					if (pos[0] < bmin[0]) bmin[0] = pos[0];
					if (pos[1] < bmin[1]) bmin[1] = pos[1];
					if (pos[2] < bmin[2]) bmin[2] = pos[2];
					if (pos[0] > bmax[0]) bmax[0] = pos[0];
					if (pos[1] > bmax[1]) bmax[1] = pos[1];
					if (pos[2] > bmax[2]) bmax[2] = pos[2];
				}
			}
		}

		VectorCopy( bmin, g_panimation[i]->bmin );
		VectorCopy( bmax, g_panimation[i]->bmax );

		/*
		printf("%s : %.0f %.0f %.0f %.0f %.0f %.0f\n", 
			g_panimation[i]->name, bmin[0], bmax[0], bmin[1], bmax[1], bmin[2], bmax[2] );
		*/

		// printf("%s  %.2f\n", g_sequence[i].name, g_sequence[i].panim[0]->pos[9][0][0] / g_bonetable[9].pos[0] );
	}

	for (i = 0; i < g_numseq; i++)
	{
		Vector bmin, bmax;
		
		// find intersection box volume for each bone
		for (j = 0; j < 3; j++)
		{
			bmin[j] = 9999.0;
			bmax[j] = -9999.0;
		}

		for (j = 0; j < g_sequence[i].groupsize[0]; j++)
		{
			for (k = 0; k < g_sequence[i].groupsize[1]; k++)
			{
				s_animation_t *panim = g_sequence[i].panim[j][k];

				if (panim->bmin[0] < bmin[0]) bmin[0] = panim->bmin[0];
				if (panim->bmin[1] < bmin[1]) bmin[1] = panim->bmin[1];
				if (panim->bmin[2] < bmin[2]) bmin[2] = panim->bmin[2];
				if (panim->bmax[0] > bmax[0]) bmax[0] = panim->bmax[0];
				if (panim->bmax[1] > bmax[1]) bmax[1] = panim->bmax[1];
				if (panim->bmax[2] > bmax[2]) bmax[2] = panim->bmax[2];
			}
		}

		VectorCopy( bmin, g_sequence[i].bmin );
		VectorCopy( bmax, g_sequence[i].bmax );
	}

	// find center of domain
	if (!illumpositionset)
	{
		// Only use the 0th sequence; that should be the idle sequence
		VectorFill( illumposition, 0 );
		if (g_numseq != 0)
		{
			VectorAdd( g_sequence[0].bmin, g_sequence[0].bmax, illumposition );
			illumposition *= 0.5f;
		}
		illumpositionset = true;
	}

#if 0
	// auto groups
	if (numseqgroups == 1 && maxseqgroupsize < 1024 * 1024) 
	{	
		int current = 0;

		numseqgroups = 2;

		for (i = 0; i < g_numseq; i++)
		{
			int accum = 0;

			if (g_sequence[i].activityname == -1)
			{
				for (q = 0; q < g_sequence[i].numblends; q++)
				{
					for (j = 0; j < numbones; j++)
					{
						for (k = 0; k < 6; k++)
						{
							accum += g_sequence[i].panim[q]->numanim[j][k] * sizeof( mstudioanimvalue_t );
						}
					}
				}
				accum += g_sequence[i].numblends * numbones * sizeof( mstudioanim_t );
			
				if (current && current + accum > maxseqgroupsize)
				{
					numseqgroups++;
					current = accum;
				}
				else
				{
					current += accum;
				}
				// printf("%d %d %d\n", numseqgroups, current, accum );
				g_sequence[i].seqgroup = numseqgroups - 1;
			}
			else
			{
				g_sequence[i].seqgroup = 0;
			}
		}
	}
#endif
}





