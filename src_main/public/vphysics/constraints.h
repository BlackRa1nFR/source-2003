//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H
#ifdef _WIN32
#pragma once
#endif

#include "vphysics_interface.h"
#include "mathlib.h"

// Breakable constraints;
// 
//	forceLimit	- kg * in / s limit (N * conversion(in/m))
//	torqueLimit - kg * in^2 / s (Nm * conversion(in^2/m^2))

// 
// strength 0 - 1
struct constraint_breakableparams_t
{
	float		strength;				// strength of the constraint 0.0 - 1.0
	float		forceLimit;				// constraint force limit to break (0 means never break)
	float		torqueLimit;			// constraint torque limit to break (0 means never break)
	bool		isActive;

	inline void Defaults()
	{
		forceLimit = 0;
		torqueLimit = 0;
		strength = 1.0;
		isActive = true;
	}
};

//-----------------------------------------------------------------------------
// Purpose: constraint limit on a single rotation axis
//-----------------------------------------------------------------------------
struct constraint_axislimit_t
{
	float		minRotation;
	float		maxRotation;
	float		angularVelocity;		// desired angular velocity around hinge
	float		torque;					// torque to achieve angular velocity (use 0, torque for "friction")

	inline void SetAxisFriction( float rmin, float rmax, float friction )
	{
		minRotation = rmin;
		maxRotation = rmax;
		angularVelocity = 0;
		torque = friction;
	}
	inline void Defaults()
	{
		SetAxisFriction(0,0,0);
	}
};

// Builds a transform which maps points in the input object's local space 
// to the output object's local space
inline void BuildObjectRelativeXform( IPhysicsObject *pOutputSpace, IPhysicsObject *pInputSpace, matrix3x4_t &xformInToOut )
{
	matrix3x4_t outInv, tmp, input;
	pOutputSpace->GetPositionMatrix( tmp );
	MatrixInvert( tmp, outInv );
	pInputSpace->GetPositionMatrix( input );
	ConcatTransforms( outInv, input, xformInToOut );
}


//-----------------------------------------------------------------------------
// Purpose: special limited ballsocket constraint for ragdolls.
//			Has axis limits for all 3 axes.
//-----------------------------------------------------------------------------
struct constraint_ragdollparams_t
{
	matrix3x4_t			constraintToReference;// xform constraint space to refobject space
	matrix3x4_t			constraintToAttached;	// xform constraint space to attached object space
	int					parentIndex;				// NOTE: only used for parsing.  NEED NOT BE SET for create
	int					childIndex;					// NOTE: only used for parsing.  NEED NOT BE SET for create
	
	constraint_axislimit_t	axes[3];
	bool				onlyAngularLimits;			// only angular limits (not translation as well?)
	bool				isActive;

	inline void Defaults()
	{
		isActive = true;
		SetIdentityMatrix( constraintToReference );
		SetIdentityMatrix( constraintToAttached );
		parentIndex = -1;
		childIndex = -1;
		axes[0].Defaults();
		axes[1].Defaults();
		axes[2].Defaults();
		onlyAngularLimits = false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Used to init a hinge restricting the relative position and orientation
//			of two objects to rotation around a single axis
//-----------------------------------------------------------------------------
struct constraint_hingeparams_t
{
	Vector							worldPosition;			// position in world space on the hinge axis
	Vector							worldAxisDirection;		// unit direction vector of the hinge axis in world space
	constraint_axislimit_t			hingeAxis;
	constraint_breakableparams_t	constraint;

	inline void Defaults()
	{
		worldPosition.Init();
		worldAxisDirection.Init();
		hingeAxis.Defaults();
		constraint.Defaults();
	}
};


//-----------------------------------------------------------------------------
// Purpose: Used to init a constraint that fixes the position and orientation
//			of two objects relative to each other (like glue)
//-----------------------------------------------------------------------------
struct constraint_fixedparams_t
{
	matrix3x4_t						attachedRefXform;	// xform attached object space to ref object space
	constraint_breakableparams_t	constraint;
	
	inline void InitWithCurrentObjectState( IPhysicsObject *pRef, IPhysicsObject *pAttached )
	{
		BuildObjectRelativeXform( pRef, pAttached, attachedRefXform );
	}

	inline void Defaults()
	{
		SetIdentityMatrix( attachedRefXform );
		constraint.Defaults();
	}
};


//-----------------------------------------------------------------------------
// Purpose: Same parameters as fixed constraint, but torqueLimit has no effect
//-----------------------------------------------------------------------------
struct constraint_ballsocketparams_t
{
	Vector							constraintPosition[2];		// position of the constraint in each object's space 
	constraint_breakableparams_t	constraint;
	inline void Defaults()
	{
		constraint.Defaults();
		constraintPosition[0].Init();
		constraintPosition[1].Init();
	}
};

struct constraint_slidingparams_t
{
	matrix3x4_t						attachedRefXform;	// xform attached object space to ref object space
	Vector							slideAxisRef;				// unit direction vector of the slide axis in ref object space
	constraint_breakableparams_t	constraint;
	// NOTE: if limitMin == limitMax there is NO limit set!
	float							limitMin;				// minimum limit coordinate refAxisDirection space
	float							limitMax;				// maximum limit coordinate refAxisDirection space
	float							friction;				// friction on sliding

	inline void InitWithCurrentObjectState( IPhysicsObject *pRef, IPhysicsObject *pAttached, const Vector &slideDirWorldspace )
	{
		BuildObjectRelativeXform( pRef, pAttached, attachedRefXform );
		matrix3x4_t tmp;
		pRef->GetPositionMatrix( tmp );
		VectorIRotate( slideDirWorldspace, tmp, slideAxisRef );
	}

	inline void Defaults()
	{
		SetIdentityMatrix( attachedRefXform );
		slideAxisRef.Init();
		limitMin = limitMax = 0;
		friction = 0;
		constraint.Defaults();
	}
};

struct constraint_pulleyparams_t
{
	constraint_breakableparams_t	constraint;
	Vector	pulleyPosition[2];		// These are the pulley positions for the reference and attached objects in world space
	Vector	objectPosition[2];		// local positions of attachments to the ref,att objects
	float	totalLength;			// total rope length (include gearing!)
	float	gearRatio;				// gearing affects attached object ALWAYS
	bool	isRigid;

	inline void Defaults()
	{
		constraint.Defaults();
		totalLength = 1.0;
		gearRatio = 1.0;
		pulleyPosition[0].Init();
		pulleyPosition[1].Init();
		objectPosition[0].Init();
		objectPosition[1].Init();
		isRigid = false;
	}
};


struct constraint_lengthparams_t
{
	constraint_breakableparams_t	constraint;
	Vector	objectPosition[2];		// These are the positions for the reference and attached objects in local space
	float	totalLength;		// Length of rope/spring/constraint.  Distance to maintain
	bool	isRigid;			// if rigid, objects are not allowed to move closer than totalLength either

	void InitWorldspace( IPhysicsObject *pRef, IPhysicsObject *pAttached, const Vector &refPosition, const Vector &attachedPosition, bool rigid = false )
	{
		pRef->WorldToLocal( objectPosition[0], refPosition );
		pAttached->WorldToLocal( objectPosition[1], attachedPosition );
		totalLength = (refPosition - attachedPosition).Length();
		isRigid = rigid;
	}

	inline void Defaults()
	{
		constraint.Defaults();
		objectPosition[0].Init();
		objectPosition[1].Init();
		totalLength = 1;
		isRigid = false;
	}
};

class IPhysicsConstraint
{
public:
	virtual ~IPhysicsConstraint( void ) {}
	
	// NOTE: Constraints are active when created.  You can temporarily enable/disable them with these functions
	virtual void			Activate( void ) = 0;
	virtual void			Deactivate( void ) = 0;

	// set a pointer to the game object
	virtual void SetGameData( void *gameData ) = 0;

	// get a pointer to the game object
	virtual void *GetGameData( void ) = 0;

	// Get the parent/referenced object
	virtual IPhysicsObject *GetReferenceObject( void ) = 0;

	// Get the attached object
	virtual IPhysicsObject *GetAttachedObject( void ) = 0;
};


class IPhysicsConstraintGroup
{
public:
	virtual ~IPhysicsConstraintGroup( void ) {}
	virtual void Activate() = 0;
};


#endif // CONSTRAINTS_H
