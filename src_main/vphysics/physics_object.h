//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_OBJECT_H
#define PHYSICS_OBJECT_H

#ifdef _WIN32
#pragma once
#endif

#include "vphysics_interface.h"

class IVP_Real_Object;
class IVP_Environment;
class IVP_U_Float_Point;
class IVP_SurfaceManager;
class IVP_Controller;
class CPhysicsEnvironment;
struct vphysics_save_cphysicsobject_t;

enum
{
	OBJ_AWAKE = 0,			// awake, simulating
	OBJ_STARTSLEEP = 1,		// going to sleep, but not queried yet
	OBJ_SLEEP = 2,			// sleeping, no state changes since last query
};


class CPhysicsObject : public IPhysicsObject
{
public:
	CPhysicsObject( void );
	virtual ~CPhysicsObject( void );

	void			Init( IVP_Real_Object *pObject, int materialIndex, float volume, float drag, float angDrag, const Vector *massCenterOverride );

	// IPhysicsObject functions
	bool			IsStatic();
	bool			IsMoveable();
	void			Wake();
	void			Sleep( void );
	bool			IsAsleep( void );
	void			SetGameData( void *pAppData );
	void			*GetGameData( void ) const;
	void			SetCallbackFlags( unsigned short callbackflags );
	unsigned short	GetCallbackFlags( void );
	void			SetGameFlags( unsigned short userFlags );
	unsigned short	GetGameFlags( void ) const;
	void			SetMass( float mass );
	float			GetMass( void ) const;
	float			GetInvMass( void ) const;
	void			EnableCollisions( bool enable );
	// Enable / disable gravity for this object
	void			EnableGravity( bool enable );
	// Enable / disable air friction / drag for this object
	void			EnableDrag( bool enable );
	void			EnableMotion( bool enable );

	bool			IsCollisionEnabled();
	bool			IsGravityEnabled();
	bool			IsDragEnabled();
	bool			IsMotionEnabled();

	void			LocalToWorld( Vector &worldPosition, const Vector &localPosition );
	void			WorldToLocal( Vector &localPosition, const Vector &worldPosition );
	void			LocalToWorldVector( Vector &worldVector, const Vector &localVector );
	void			WorldToLocalVector( Vector &localVector, const Vector &worldVector );
	void			ApplyForceCenter( const Vector &forceVector );
	void			ApplyForceOffset( const Vector &forceVector, const Vector &worldPosition );
	void			ApplyTorqueCenter( const AngularImpulse & );
	void			CalculateForceOffset( const Vector &forceVector, const Vector &worldPosition, Vector &centerForce, AngularImpulse &centerTorque );
	void			CalculateVelocityOffset( const Vector &forceVector, const Vector &worldPosition, Vector &centerVelocity, AngularImpulse &centerAngularVelocity );
	void			GetPosition( Vector *worldPosition, QAngle *angles );
	void			GetPositionMatrix( matrix3x4_t& positionMatrix );
	void			SetPosition( const Vector &worldPosition, const QAngle &angles, bool isTeleport = false );
	void			SetPositionMatrix( const matrix3x4_t& matrix, bool isTeleport = false  );
	void			SetVelocity( const Vector *velocity, const AngularImpulse *angularVelocity );
	void			GetVelocity( Vector *velocity, AngularImpulse *angularVelocity );
	void			AddVelocity( const Vector *velocity, const AngularImpulse *angularVelocity );
	bool			GetContactPoint( Vector *contactPoint, IPhysicsObject **contactObject );
	void			SetShadow( const Vector &maxVelocity, const AngularImpulse &maxAngularVelocity, bool allowPhysicsMovement, bool allowPhysicsRotation );
	void			UpdateShadow( const Vector &targetPosition, const QAngle &targetAngles, bool tempDisableGravity, float timeOffset );
	void			RemoveShadowController();
	int				GetShadowPosition( Vector *position, QAngle *angles );
	
	IPhysicsShadowController *GetShadowController( void ) const;
	Vector			GetInertia( void ) const;
	Vector			GetInvInertia( void ) const;
	void			GetVelocityAtPoint( const Vector &worldPosition, Vector &velocity );
	void			SetInertia( const Vector &inertia );
	void			GetDamping( float *speed, float *rot );
	void			SetDamping( const float *speed, const float *rot );
	const CPhysCollide	*GetCollide( void ) const;
	float			CalculateLinearDrag( const Vector &unitDirection ) const;
	float			CalculateAngularDrag( const Vector &objectSpaceRotationAxis ) const;

	float			GetDragInDirection( const IVP_U_Float_Point &dir ) const;
	float			GetAngularDragInDirection( const IVP_U_Float_Point &angVelocity ) const;
	char const		*GetName();
	int				GetMaterialIndex() const { return GetMaterialIndexInternal(); }
	void			SetMaterialIndex( int materialIndex );
	float			GetEnergy();
	void			RecheckCollisionFilter();
	float			ComputeShadowControl( const hlshadowcontrol_params_t &params, float secondsToArrival, float dt );
	float			GetRollingDrag() { return m_rollingDrag; }
	void			SetRollingDrag( float drag ) { m_rollingDrag = drag; }
	void			SetDragCoefficient( float *pDrag, float *pAngularDrag );
	float			GetSphereRadius();
	void			BecomeTrigger();
	void			RemoveTrigger();
	bool			IsTrigger();
	bool			IsFluid();
	void			SetBuoyancyRatio( float ratio );
	unsigned int	GetContents() { return m_contentsMask; }
	void			SetContents( unsigned int contents );

	// local functions
	inline	IVP_Real_Object *GetObject( void ) const { return m_pObject; }
	inline int		CallbackFlags( void ) { return m_callbacks; }
	void			NotifySleep( void );
	void			NotifyWake( void );
	int				GetSleepState( void ) const { return m_sleepState; }

	inline int		GetMaterialIndexInternal( void ) const { return m_materialIndex; }
	inline int		GetActiveIndex( void ) { return m_activeIndex; }
	inline void		SetActiveIndex( int index ) { m_activeIndex = index; }
	inline float	GetBuoyancyRatio( void ) { return m_buoyancyRatio; }

	IVP_SurfaceManager *GetSurfaceManager( void ) const;

	void			WriteToTemplate( vphysics_save_cphysicsobject_t &objectTemplate );
	void			InitFromTemplate( CPhysicsEnvironment *pEnvironment, void *pGameData, const vphysics_save_cphysicsobject_t &objectTemplate );

private:
	// NOTE: Local to vphysics, used to save/restore shadow controller
	void			RestoreShadowController( IPhysicsShadowController *pShadowController );
	friend bool RestorePhysicsObject( const physrestoreparams_t &params, CPhysicsObject **ppObject );

	CPhysicsEnvironment	*GetVPhysicsEnvironment();
	bool				IsControlling( IVP_Controller *pController );
	float				GetVolume();
	void				SetVolume( float volume );

	void			*m_pGameData;
	IVP_Real_Object	*m_pObject;
	short			m_sleepState;
	// HACKHACK: Public for now
public:
	short			m_collideType;
private:
	unsigned short	m_materialIndex;
	unsigned short	m_activeIndex;

	unsigned short	m_callbacks;
	unsigned short	m_gameFlags;
	unsigned int	m_contentsMask;
	
	float			m_volume;
	float			m_buoyancyRatio;
	float			m_dragCoefficient;
	float			m_angDragCoefficient;
	float			m_rollingDrag;
	Vector			m_dragBasis;
	Vector			m_angDragBasis;
	Vector			m_massCenterOverride;
	IPhysicsShadowController	*m_pShadow;
	bool			m_shadowTempGravityDisable;
};

extern CPhysicsObject *CreatePhysicsObject( CPhysicsEnvironment *pEnvironment, const CPhysCollide *pCollisionModel, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams, bool isStatic );
extern CPhysicsObject *CreatePhysicsSphere( CPhysicsEnvironment *pEnvironment, float radius, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams, bool isStatic );

#endif // PHYSICS_OBJECT_H
