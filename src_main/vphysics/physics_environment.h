//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H
#pragma once

#include "vphysics_interface.h"
#include "ivu_types.hxx"
#include "utlvector.h"

class IVP_Environment;
class CSleepObjects;
class CPhysicsListenerCollision;
class CPhysicsListenerConstraint;
class IVP_Listener_Collision;
class IVP_Listener_Constraint;
class IVP_Listener_Object;
class IVP_Controller;
class CPhysicsFluidController;
class CCollisionSolver;
class CPhysicsObject;
class CDeleteQueue;

class CPhysicsEnvironment : public IPhysicsEnvironment
{
public:
	CPhysicsEnvironment( void );
	~CPhysicsEnvironment( void );
	void			SetGravity( const Vector& gravityVector );
	IPhysicsObject	*CreatePolyObject( const CPhysCollide *pCollisionModel, int materialIndex, const Vector& position, const QAngle& angles, objectparams_t *pParams );
	IPhysicsObject	*CreatePolyObjectStatic( const CPhysCollide *pCollisionModel, int materialIndex, const Vector& position, const QAngle& angles, objectparams_t *pParams );
	IPhysicsSpring	*CreateSpring( IPhysicsObject *pObjectStart, IPhysicsObject *pObjectEnd, springparams_t *pParams );
	IPhysicsFluidController	*CreateFluidController( IPhysicsObject *pFluidObject, fluidparams_t *pParams );
	IPhysicsConstraint *CreateRagdollConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ragdollparams_t &ragdoll );

	virtual IPhysicsConstraint *CreateHingeConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_hingeparams_t &hinge );
	virtual IPhysicsConstraint *CreateFixedConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_fixedparams_t &fixed );
	virtual IPhysicsConstraint *CreateSlidingConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_slidingparams_t &sliding );
	virtual IPhysicsConstraint *CreateBallsocketConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ballsocketparams_t &ballsocket );
	virtual IPhysicsConstraint *CreatePulleyConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_pulleyparams_t &pulley );
	virtual IPhysicsConstraint *CreateLengthConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_lengthparams_t &length );

	virtual IPhysicsConstraintGroup *CreateConstraintGroup( void );
	virtual void DestroyConstraintGroup( IPhysicsConstraintGroup *pGroup );

	virtual void	EnableCollisions( IPhysicsObject *pObject1, IPhysicsObject *pObject2 );
	virtual void	DisableCollisions( IPhysicsObject *pObject1, IPhysicsObject *pObject2 );
	virtual bool	ShouldCollide( IPhysicsObject *pObject1, IPhysicsObject *pObject2 );

	void			Simulate( float deltaTime );
	float			GetSimulationTimestep( void );
	void			SetSimulationTimestep( float timestep );
	float			GetSimulationTime( void );
	int				GetTimestepsSimulatedLast();
	bool			IsInSimulation( void ) const;

	virtual void DestroyObject( IPhysicsObject * );
	virtual void DestroySpring( IPhysicsSpring * );
	virtual void DestroyFluidController( IPhysicsFluidController * );
	virtual void DestroyConstraint( IPhysicsConstraint * );

	virtual void SetCollisionEventHandler( IPhysicsCollisionEvent *pCollisionEvents );
	virtual void SetObjectEventHandler( IPhysicsObjectEvent *pObjectEvents );
	virtual void SetConstraintEventHandler( IPhysicsConstraintEvent *pConstraintEvents );

	virtual IPhysicsShadowController *CreateShadowController( IPhysicsObject *pObject, bool allowTranslation, bool allowRotation );
	virtual void DestroyShadowController( IPhysicsShadowController * );
	virtual IPhysicsMotionController *CreateMotionController( IMotionEvent *pHandler );
	virtual void DestroyMotionController( IPhysicsMotionController *pController );
	virtual IPhysicsPlayerController *CreatePlayerController( IPhysicsObject *pObject );
	virtual void DestroyPlayerController( IPhysicsPlayerController *pController );
	virtual IPhysicsVehicleController *CreateVehicleController( IPhysicsObject *pVehicleBodyObject, const vehicleparams_t &params, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace );
	virtual void DestroyVehicleController( IPhysicsVehicleController *pController );

	virtual void SetQuickDelete( bool bQuick )
	{
		m_deleteQuick = true;
	}
	virtual bool ShouldQuickDelete( void ) { return m_deleteQuick; }
	virtual void TraceBox( trace_t *ptr, const Vector &mins, const Vector &maxs, const Vector &start, const Vector &end );
	virtual void SetCollisionSolver( IPhysicsCollisionSolver *pCollisionSolver );
	virtual void GetGravity( Vector& gravityVector );
	virtual int	 GetActiveObjectCount( void );
	virtual void GetActiveObjects( IPhysicsObject **pOutputObjectList );

	IVP_Environment	*GetIVPEnvironment( void ) { return m_pPhysEnv; }
	void		ClearDeadObjects( void );
	IVP_Controller *GetDragController() { return m_pDragController; }
	virtual void SetAirDensity( float density );
	virtual float GetAirDensity( void );
	virtual void ResetSimulationClock( void );
	virtual IPhysicsObject *CreateSphereObject( float radius, int materialIndex, const Vector& position, const QAngle& angles, objectparams_t *pParams, bool isStatic );
	virtual void CleanupDeleteList();
	virtual void EnableDeleteQueue( bool enable ) { m_queueDeleteObject = enable; }
	// debug
	virtual bool IsCollisionModelUsed( CPhysCollide *pCollide );

	// scale per-psi sim quantities
	// This is 1 / number of PSIs during this call to Simulate
	inline float GetInvPSIScale( void ) { return m_invPSIscale; }
	inline int GetSimulatedPSIs( void ) { return m_simPSIs; }
	void EventPSI( void );

	// Save/restore
	bool Save( const physsaveparams_t &params  );
	void PreRestore( const physprerestoreparams_t &params );
	bool Restore( const physrestoreparams_t &params );
	void PostRestore();
	void PhantomAdd( CPhysicsObject *pObject );
	void PhantomRemove( CPhysicsObject *pObject );
	
private:
	bool							m_inSimulation;
	bool							m_queueDeleteObject;
	IVP_Environment					*m_pPhysEnv;
	IVP_Controller					*m_pDragController;
	CUtlVector<IPhysicsObject *>	m_objects;
	CUtlVector<IPhysicsObject *>	m_deadObjects;
	CUtlVector<CPhysicsFluidController *> m_fluids;
	CSleepObjects					*m_pSleepEvents;
	CPhysicsListenerCollision		*m_pCollisionListener;
	CCollisionSolver				*m_pCollisionSolver;
	CPhysicsListenerConstraint		*m_pConstraintListener;
	CDeleteQueue					*m_pDeleteQueue;
	bool							m_deleteQuick;
	int								m_simPSIs;		// number of PSIs this call to simulate
	int								m_simPSIcurrent;
	float							m_invPSIscale;  // inverse of simPSIs
};

extern IPhysicsEnvironment *CreatePhysicsEnvironment( void );

class IVP_Synapse_Friction;
class IVP_Real_Object;
extern IVP_Real_Object *GetOppositeSynapseObject( IVP_Synapse_Friction *pfriction );

#endif // PHYSICS_WORLD_H
