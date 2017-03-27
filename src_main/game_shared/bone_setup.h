//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BONE_SETUP_H
#define BONE_SETUP_H
#ifdef _WIN32
#pragma once
#endif


#include "studio.h"
#include "cmodel.h"
#include "utlvector.h"


class CBoneToWorld;
class CIKContext;


// This provides access to networked arrays, so if this code actually changes a value, 
// the entity is marked as changed.
class IParameterAccess
{
public:
	virtual float GetParameter( int iParam ) = 0;
	virtual void SetParameter( int iParam, float flValue ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: blends together all the bones from two p:q lists
//
// p1 = p1 * (1 - s) + p2 * s
// q1 = q1 * (1 - s) + q2 * s
//-----------------------------------------------------------------------------
void SlerpBones( 
	const studiohdr_t *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	const mstudioseqdesc_t *pseqdesc, 
	const Quaternion q2[MAXSTUDIOBONES], 
	const Vector pos2[MAXSTUDIOBONES], 
	float s,
	int boneMask
	);


void InitPose(
	const studiohdr_t *pStudioHdr,
	Vector pos[MAXSTUDIOBONES], 
	Quaternion q[MAXSTUDIOBONES]
	);

void CalcPose(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,			//optional
	Vector pos[MAXSTUDIOBONES], 
	Quaternion q[MAXSTUDIOBONES], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flWeight = 1.0f
	);

bool CalcPoseSingle(
	const studiohdr_t *pStudioHdr,
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask
	);

void AccumulatePose(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,			//optional
	Vector pos[MAXSTUDIOBONES], 
	Quaternion q[MAXSTUDIOBONES], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flWeight = 1.0f
	);

// takes a "controllers[]" array normalized to 0..1 and adds in the adjustments to pos[], and q[].
void CalcBoneAdj(
	const studiohdr_t *pStudioHdr,
	Vector pos[], 
	Quaternion q[], 
	const float controllers[],
	int boneMask
	);


// Given two samples of a bone separated in time by dt, 
// compute the velocity and angular velocity of that bone
void CalcBoneDerivatives( Vector &velocity, AngularImpulse &angVel, const matrix3x4_t &prev, const matrix3x4_t &current, float dt );
// Give a derivative of a bone, compute the velocity & angular velocity of that bone
void CalcBoneVelocityFromDerivative( const QAngle &vecAngles, Vector &velocity, AngularImpulse &angVel, const matrix3x4_t &current );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// ik info
struct iktarget_t
{
	struct x1 {
		// matrix3x4_t		worldTarget;
		float		time;
		Vector		pos;
		Quaternion	q;
	} latched;
	struct x2 {
		Vector		pos;
		Quaternion	q;
	} local;
	struct x3 {
		float		time;
		Vector		pos;
		Quaternion	q;
	} prev;
	struct x4 {
		float		latched;
		float		height;
		float		floor;
		float		radius;
		float		time;
		Vector		pos;
		Quaternion	q;
		float		flWeight;
	} est;
};


bool Studio_SolveIK( int iThigh, int iKnee, int iFoot, Vector &targetFoot, matrix3x4_t* pBoneToWorld );

bool Studio_SolveIK( int iThigh, int iKnee, int iFoot, Vector &targetFoot, Vector &targetKnee, matrix3x4_t* pBoneToWorld );

bool Studio_SolveIK( mstudioikchain_t *pikchain, Vector &targetFoot, matrix3x4_t* pBoneToWorld );

class CIKContext 
{
public:
	CIKContext( );
	void Init( const studiohdr_t *pStudioHdr, const QAngle &angles, const Vector &pos, float flTime );
	void AddDependencies(  int iSequence, float flCycle, const float poseParameters[], float flWeight = 1.0f );
	void SolveDependencies( Vector pos[], Quaternion q[] );

	void AddAutoplayLocks( Vector pos[], Quaternion q[] );
	void SolveAutoplayLocks( Vector pos[], Quaternion q[] );

	void AddSequenceLocks( mstudioseqdesc_t *pSeqDesc, Vector pos[], Quaternion q[] );
	void SolveSequenceLocks( mstudioseqdesc_t *pSeqDesc, Vector pos[], 	Quaternion q[] );
	
	CUtlVector< iktarget_t >	m_target;

private:

	studiohdr_t const *m_pStudioHdr;

	bool Estimate( int iSequence, float flCycle, int iTarget, const float poseParameter[], float flWeight = 1.0f ); 

	CUtlVector< mstudioikrule_t > m_ikRule;
	matrix3x4_t m_rootxform;

	float m_flPrevTime;
	float m_flTime;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// takes a "poseparameters[]" array normalized to 0..1 and layers on the sequences driven by them
void CalcAutoplaySequences(
	const studiohdr_t *pStudioHdr,
	CIKContext *pIKContext,		//optional
	Vector pos[], 
	Quaternion q[], 
	const float poseParameters[],
	int boneMask,
	float time
	);

// replaces the bonetoworld transforms for all bones that are procedural
bool CalcProceduralBone(
	const studiohdr_t *pStudioHdr,
	int iBone,
	matrix3x4_t *bonetoworld
	);

void Studio_BuildMatrices(
	const studiohdr_t *pStudioHdr,
	const QAngle& angles, 
	const Vector& origin, 
	const Vector pos[],
	const Quaternion q[],
	int iBone,
	matrix3x4_t bonetoworld[MAXSTUDIOBONES],
	int boneMask
	);


// Get a bone->bone relative transform
void Studio_CalcBoneToBoneTransform( const studiohdr_t *pStudioHdr, int inputBoneIndex, int outputBoneIndex, matrix3x4_t &matrixOut );

// Given a bone rotation value, figures out the value you need to give to the controller
// to have the bone at that value.
// [in]  flValue  = the desired bone rotation value
// [out] ctlValue = the (0-1) value to set the controller t.
// return value   = flValue, unwrapped to lie between the controller's start and end.
float Studio_SetController( const studiohdr_t *pStudioHdr, int iController, float flValue, float &ctlValue );


// Given a 0-1 controller value, maps it into the controller's start and end and returns the bone rotation angle.
// [in] ctlValue  = value in controller space (0-1).
// return value   = value in bone space
float Studio_GetController( const studiohdr_t *pStudioHdr, int iController, float ctlValue );

float Studio_GetPoseParameter( const studiohdr_t *pStudioHdr, int iParameter, float ctlValue );
float Studio_SetPoseParameter( const studiohdr_t *pStudioHdr, int iParameter, float flValue, float &ctlValue );

// converts a global 0..1 pose parameter into the local sequences blending value
void Studio_LocalPoseParameter( const studiohdr_t *pStudioHdr, const float poseParameter[], const mstudioseqdesc_t *pSeqDesc, int iLocalPose, float &flSetting, int &index );

void Studio_SeqAnims( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], mstudioanimdesc_t *panim[4], float *weight );
int Studio_MaxFrame( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] );
float Studio_FPS( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] );
float Studio_CPS( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] );
float Studio_Duration( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[] );
void Studio_MovementRate( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], Vector *pVec );

// void Studio_Movement( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], Vector *pVec );

//void Studio_AnimPosition( mstudioanimdesc_t *panim, float flCycle, Vector &vecPos, Vector &vecAngle );
//void Studio_AnimVelocity( mstudioanimdesc_t *panim, float flCycle, Vector &vecVelocity );
//float Studio_FindAnimDistance( mstudioanimdesc_t *panim, float flDist );
bool Studio_AnimMovement( mstudioanimdesc_t *panim, float flCycleFrom, float flCycleTo, Vector &deltaPos, QAngle &deltaAngle );
bool Studio_SeqMovement( const studiohdr_t *pStudioHdr, int iSequence, float flCycleFrom, float flCycleTo, const float poseParameter[], Vector &deltaMovement, QAngle &deltaAngle );
bool Studio_SeqVelocity( const studiohdr_t *pStudioHdr, int iSequence, float flCycle, const float poseParameter[], Vector &vecVelocity );
float Studio_FindSeqDistance( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], float flDist );
float Studio_FindSeqVelocity( const studiohdr_t *pStudioHdr, int iSequence, const float poseParameter[], float flVelocity );
int Studio_FindAttachment( const studiohdr_t *pStudioHdr, const char *pAttachmentName );
int Studio_FindRandomAttachment( const studiohdr_t *pStudioHdr, const char *pAttachmentName );
int Studio_BoneIndexByName( const studiohdr_t *pStudioHdr, const char *pName );
const char *Studio_GetDefaultSurfaceProps( studiohdr_t *pstudiohdr );
float Studio_GetMass( studiohdr_t *pstudiohdr );
const char *Studio_GetKeyValueText( const studiohdr_t *pStudioHdr, int iSequence );

struct studiocache_t;
// Call this to get the existing cache
studiocache_t *Studio_GetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask );
// If there is no existing cache, setup the bones and call this to build a cache
studiocache_t *Studio_SetBoneCache( studiohdr_t *pStudioHdr, int sequence, float animtime, const QAngle& angles, const Vector& origin, int boneMask, matrix3x4_t *bonetoworld );
// removes this cache entry from the valid cache list
void Studio_InvalidateBoneCache( studiocache_t *pcache );


matrix3x4_t *Studio_LookupCachedBone( studiocache_t *pCache, int iBone );
void Studio_LinkHitboxCache( matrix3x4_t **bones, studiocache_t *pcache, studiohdr_t *pStudioHdr, mstudiohitboxset_t *set );

// Given a ray, trace for an intersection with this studiomodel.  Get the array of bones from StudioSetupHitboxBones
bool TraceToStudio( const Ray_t& ray, studiohdr_t *pStudioHdr, mstudiohitboxset_t *set, matrix3x4_t **hitboxbones, int fContentsMask, trace_t &trace );


void QuaternionSM( float s, const Quaternion &p, const Quaternion &q, Quaternion &qt );
void QuaternionMA( const Quaternion &p, float s, const Quaternion &q, Quaternion &qt );

#endif // BONE_SETUP_H
