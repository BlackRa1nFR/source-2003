//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================
#ifndef C_BASEANIMATING_H
#define C_BASEANIMATING_H

#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "studio.h"
#include "UtlVector.h"
#include "ragdoll.h"
// Shared activities
#include "ai_activity.h"
#include "animationlayer.h"

#define LIPSYNC_POSEPARAM_NAME "mouth"
/*
class C_BaseClientShader
{
	virtual void RenderMaterial( C_BaseEntity *pEntity, int count, const vec4_t *verts, const vec4_t *normals, const vec2_t *texcoords, vec4_t *lightvalues );
};
*/

class IRagdoll;
class CIKContext;
class CIKState;
class ConVar;
class C_RopeKeyframe;

extern ConVar vcollide_wireframe;
extern ConVar vcollide_axes;



struct RagdollInfo_t
{
	bool		m_bActive;
	float		m_flSaveTime;
	int			m_nNumBones;
	Vector		m_rgBonePos[MAXSTUDIOBONES];
	Quaternion	m_rgBoneQuaternion[MAXSTUDIOBONES];
};


class CAttachmentData
{
public:
	Vector	m_vOrigin;
	QAngle	m_angRotation;
};


class C_BaseAnimating : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_BaseAnimating, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	enum
	{
		NUM_POSEPAREMETERS = 24,
		NUM_BONECTRLS = 4
	};

	C_BaseAnimating();
	~C_BaseAnimating();

	bool UsesFrameBufferTexture( void );

	virtual bool	Interpolate( float currentTime );
	virtual void	Simulate();	
	virtual void	Release();	

	float	GetAnimTimeInterval( void ) const;

	// Get bone controller values.
	virtual void	GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);
	virtual float	SetBoneController ( int iController, float flValue );

	// base model functionality
	float		  ClampCycle( float cycle, bool isLooping );
	virtual void GetPoseParameters( float poseParameter[MAXSTUDIOPOSEPARAM] );
	virtual void BuildTransformations( Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform );

	mstudioposeparamdesc_t *GetPoseParameterPtr( const char *pName );

	// model specific
	virtual bool SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void CalculateIKLocks( float currentTime );
	virtual int DrawModel( int flags );
	virtual int InternalDrawModel( int flags );
	
	//
	virtual void	ControlMouth();
	
	// Any client-specific visibility test implemented here.
	// return VIS_USE_ENGINE to let the engine cull the sequence box
	enum studiovisible_t
	{
		VIS_NOT_VISIBLE = 0,
		VIS_IS_VISIBLE,
		VIS_USE_ENGINE,
	};

	virtual studiovisible_t TestVisibility( void );

	virtual void DoAnimationEvents( void );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	// virtual	void AllocateMaterials( void );
	// virtual	void FreeMaterials( void );

	virtual studiohdr_t *OnNewModel( void );
	studiohdr_t	*GetModelPtr();

	// C_BaseClientShader **p_ClientShaders;

	virtual	void StandardBlendingRules( Vector pos[], Quaternion q[], float currentTime, int boneMask );

	void MaintainSequenceTransitions( float flCycle, float flPoseParameter[], Vector pos[], Quaternion q[], int boneMask );

	// Attachments
	int LookupAttachment( const char *pAttachmentName );
	int LookupRandomAttachment( const char *pAttachmentNameSubstring );

	int								LookupPoseParameter( const char *szName );
	float							SetPoseParameter( const char *szName, float flValue );
	float							SetPoseParameter( int iParameter, float flValue );

	//bool solveIK(float a, float b, const Vector &Foot, const Vector &Knee1, Vector &Knee2);
	//void DebugIK( mstudioikchain_t *pikchain );

	virtual void					PreDataUpdate( DataUpdateType_t updateType );
	virtual void					PostDataUpdate( DataUpdateType_t updateType );

	virtual void					OnPreDataChanged( DataUpdateType_t updateType );
	virtual void					OnDataChanged( DataUpdateType_t updateType );
	virtual void					AddEntity( void );

	virtual bool					IsSelfAnimating();
	virtual void					ResetLatched();

	// implements these so ragdolls can handle frustum culling & leaf visibility
	virtual void					GetRenderBounds( Vector& theMins, Vector& theMaxs );

	// Attachments.
	bool							GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool					GetAttachment( int number, matrix3x4_t &matrix );
	
	// Returns the attachment in local space
	bool							GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal );
	bool							GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles );

	// Should this object cast render-to-texture shadows?
	virtual ShadowType_t			ShadowCastType();

	// Should we collide?
	virtual CollideType_t			ShouldCollide( );

	virtual IPhysicsObject			*VPhysicsGetObject( void );
	virtual bool					TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool					TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	void							BecomeRagdollOnClient();
	virtual void					SaveRagdollInfo( int numbones, const matrix3x4_t &cameraTransform, matrix3x4_t *pBoneToWorld );
	virtual bool					RetrieveRagdollInfo( Vector *pos, Quaternion *q );
	void							ClearRagdoll();

	// For shadows rendering the correct body + sequence...
	virtual int GetBody()			{ return m_nBody; }

	inline float					GetPlaybackRate();
	inline void						SetPlaybackRate( float rate );

	inline int						GetSequence() { return m_nSequence; }
	inline void						SetSequence(int nSequence) { m_nSequence = nSequence; }
	inline void						ResetSequence(int nSequence);
	float							GetSequenceGroundSpeed( int iSequence );
	int								GetSequenceFlags( int iSequence );
	bool							IsSequenceLooping( int iSequence );
	float							GetSequenceMoveDist( int iSequence );
	void							GetSequenceLinearMotion( int iSequence, Vector *pVec );
	int								LookupSequence ( const char *label );
	int								LookupActivity( const char *label );
	char const						*GetSequenceName( int iSequence ); 
	char const						*GetSequenceActivityName( int iSequence );
	Activity						GetSequenceActivity( int iSequence );
	virtual void					StudioFrameAdvance(); // advance animation frame to some time in the future

	// Clientside animation
	virtual float					FrameAdvance( float flInterval = 0.0f );
	virtual float					GetSequenceCycleRate( int iSequence );
	virtual void					UpdateClientSideAnimation();

	void SetBodygroup( int iGroup, int iValue );
	int GetBodygroup( int iGroup );

	const char *GetBodygroupName( int iGroup );
	int FindBodygroupByName( const char *name );
	int GetBodygroupCount( int iGroup );
	int GetNumBodyGroups( void );

	void							SetHitboxSet( int setnum );
	void							SetHitboxSetByName( const char *setname );
	int								GetHitboxSet( void );
	char const						*GetHitboxSetName( void );
	int								GetHitboxSetCount( void );
	mstudiohitboxset_t				*GetTransformedHitboxSet( int nBoneMask );

	C_BaseAnimating*				FindFollowedEntity();

	virtual bool					IsActivityFinished( void ) { return m_bSequenceFinished; }
	inline bool						IsSequenceFinished( void ) { return m_bSequenceFinished; }
	inline bool						SequenceLoops( void ) { return m_bSequenceLoops; }

	// All view model attachments origins are stretched so you can place entities at them and
	// they will match up with where the attachment winds up being drawn on the view model, since
	// the view models are drawn with a different FOV.
	//
	// If you're drawing something inside of a view model's DrawModel() function, then you want the
	// original attachment origin instead of the adjusted one. To get that, call this on the 
	// adjusted attachment origin.
	virtual void					UncorrectViewModelAttachment( Vector &vOrigin ) {}

	// Call this if SetupBones() has already been called this frame but you need to move the
	// entity and rerender.
	void							InvalidateBoneCache();
	void							GetCachedBoneMatrix( int boneIndex, matrix3x4_t &out );

	// Used for debugging. Will produce asserts if someone tries to setup bones or
	// attachments before it's allowed.
	static void						AllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels );
	static void						PushAllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels );
	static void						PopBoneAccess( void );

	// Invalidate bone caches so all SetupBones() calls force bone transforms to be regenerated.
	static void						InvalidateBoneCaches();

	// Purpose: My physics object has been updated, react or extract data
	virtual void					VPhysicsUpdate( IPhysicsObject *pPhysics );

protected:
	// View models scale their attachment positions to account for FOV. To get the unmodified
	// attachment position (like if you're rendering something else during the view model's DrawModel call),
	// use TransformViewModelAttachmentToWorld.
	virtual void					FormatViewModelAttachment( int nAttachment, Vector &vecOrigin, QAngle &angle ) {}

	// View models say yes to this.
	virtual bool					IsViewModel() const;
	bool							IsBoneAccessAllowed() const;

private:
	virtual bool					CalcAttachments();
	bool							PutAttachment( int number, const Vector &origin, const QAngle &angles );

	void InitRopes();
	void TermRopes();

public:
	float							m_flGroundSpeed;	// computed linear movement rate for current sequence
	float							m_flLastEventCheck;	// cycle index of when events were last checked
	bool							m_bSequenceFinished;// flag set when StudioAdvanceFrame moves across a frame boundry
	bool							m_bSequenceLoops;	// true if the sequence loops

	Vector							m_vecForce;
	int								m_nForceBone;

	int								m_nSkin;
	// Object bodygroup
	int								m_nBody;

	// Hitbox set to use (default 0)
	int								m_nHitboxSet;

	float							m_flModelScale;
	// Animation blending factors
	float							m_flPoseParameter[MAXSTUDIOPOSEPARAM];
	CInterpolatedVarArray< float, MAXSTUDIOPOSEPARAM >		m_iv_flPoseParameter;

	int								m_nPrevSequence;
	int								m_nRestoreSequence;

	// Ropes that got spawned when the model was created.
	CUtlLinkedList<C_RopeKeyframe*,unsigned short> m_Ropes;

private:
	// Current animation sequence
	int								m_nSequence;

public:
	// Animation playback framerate
	float							m_flPlaybackRate;
	// Current cycle location from server
	float							m_flCycle;
	CInterpolatedVar< float	>		m_iv_flCycle;

	// event processing info
	float							m_flPrevEventCycle;
	int								m_nEventSequence;

	float							m_flEncodedController[MAXSTUDIOBONECTRLS];	
	CInterpolatedVarArray< float, MAXSTUDIOBONECTRLS >		m_iv_flEncodedController;

	CUtlVector< AnimationLayer_t >	m_animationQueue;

	CIKContext						*m_pIk;

	// Clientside animation
	bool							m_bClientSideAnimation;
	bool							m_bClientSideFrameReset;
	bool							m_bLastClientSideFrameReset;

	int								m_nNewSequenceParity;
	int								m_nResetEventsParity;

	int								m_nPrevNewSequenceParity;
	int								m_nPrevResetEventsParity;

protected:
	// returns true if we're currently being ragdolled
	bool IsRagdoll() const;

	IRagdoll						*m_pRagdoll;
	int								m_lastPhysicsBone;
	bool							m_builtRagdoll;
	Vector							m_vecPreRagdollMins;
	Vector							m_vecPreRagdollMaxs;

	// Decomposed ragdoll info
	bool							m_bStoreRagdollInfo;
	RagdollInfo_t					*m_pRagdollInfo;

	// Is bone cache valid
	// bone transformation matrix
	CUtlVector< matrix3x4_t >		m_CachedBones;
	int								m_CachedBoneFlags; // This contains BONE_USED_BY_* from studio.h
	unsigned long					m_iMostRecentModelBoneCounter;


private:	
	
	// Calculated attachment points
	CUtlVector<CAttachmentData>		m_Attachments;

	float							m_flOldCycle;
	void							SetupBones_AttachmentHelper();

// For prediction
public:
	int								SelectWeightedSequence ( int activity );
	void							ResetSequenceInfo( void );
	float							SequenceDuration( void ) { return SequenceDuration( GetSequence() ); }
	float							SequenceDuration( int iSequence );
	int								FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir );
};


//-----------------------------------------------------------------------------
// Purpose: Serves the 90% case of calling SetSequence / ResetSequenceInfo.
//-----------------------------------------------------------------------------
inline void C_BaseAnimating::ResetSequence(int nSequence)
{
	m_nSequence = nSequence;
	ResetSequenceInfo();
}

inline float C_BaseAnimating::GetPlaybackRate()
{
	return m_flPlaybackRate;
}

inline void C_BaseAnimating::SetPlaybackRate( float rate )
{
	m_flPlaybackRate = rate;
}

// FIXME: move these to somewhere that makes sense
void GetColumn( matrix3x4_t& src, int column, Vector &dest );
void SetColumn( Vector &src, int column, matrix3x4_t& dest );

EXTERN_RECV_TABLE(DT_BaseAnimating);

#endif // C_BASEANIMATING_H
