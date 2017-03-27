//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ICLIENTRENDERABLE_H
#define ICLIENTRENDERABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib.h"
#include "interface.h"
#include "iclientunknown.h"

struct model_t;
struct matrix3x4_t;


//-----------------------------------------------------------------------------
// Purpose: All client entities must implement this interface.
//-----------------------------------------------------------------------------
class IClientRenderable
{
public:
	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;

	// Data accessors
	virtual Vector const&			GetRenderOrigin( void ) = 0;
	virtual QAngle const&			GetRenderAngles( void ) = 0;
	virtual bool					ShouldDraw( void ) = 0;
	virtual bool					IsTransparent( void ) = 0;
	virtual bool					UsesFrameBufferTexture() = 0;

	// Render baby!
	virtual const model_t*			GetModel( ) = 0;
	virtual int						DrawModel( int flags ) = 0;

	// Get the body parameter
	virtual int		GetBody() = 0;

	// Determine alpha and blend amount for transparent objects based on render state info
	virtual void	ComputeFxBlend( ) = 0;
	virtual int		GetFxBlend( void ) = 0;

	// Determine the color modulation amount
	virtual void	GetColorModulation( float* color ) = 0;

	// Returns false if the entity shouldn't be drawn due to LOD.
	virtual bool	LODTest() = 0;

	// Call this to get the current bone transforms for the model.
	// currentTime parameter will affect interpolation
	// nMaxBones specifies how many matrices pBoneToWorldOut can hold. (Should be greater than or
	// equal to studiohdr_t::numbones. Use MAXSTUDIOBONES to be safe.)
	virtual bool	SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime ) = 0;

	virtual void	SetupWeights( void ) = 0;
	virtual void	DoAnimationEvents( void ) = 0;

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs ) = 0;

	// A box that is guaranteed to surround the entity.
	// It may not be particularly tight-fitting depending on performance-related questions
	// It may also fluctuate in size at any time
	virtual const Vector&	EntitySpaceSurroundingMins() const = 0;
	virtual const Vector&	EntitySpaceSurroundingMaxs() const = 0;
	virtual const Vector&	WorldSpaceSurroundingMins() const = 0;
	virtual const Vector&	WorldSpaceSurroundingMaxs() const = 0;
	
	// Say yes if this thing is changeable - then the leaf system will remember
	// the data it needs to link the entity into leaves (like position, bounds,
	// RenderGroup, etc) so it can detect changes.
	//
	// Detail and static props say no here because they are stuck in the leaves
	// and forgotten about.
	//
	// Particle systems, entities, and tempents say yes here.
	virtual bool	ShouldCacheRenderInfo() = 0;

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveShadows() = 0;
};


// This class can be used to implement default versions of some of the 
// functions of IClientRenderable.
class CDefaultClientRenderable : public IClientUnknown, public IClientRenderable
{
public:
	virtual const Vector &			GetRenderOrigin( void ) = 0;
	virtual const QAngle &			GetRenderAngles( void ) = 0;
	virtual bool					ShouldDraw( void ) = 0;
	virtual bool					IsTransparent( void ) = 0;
	virtual bool					UsesFrameBufferTexture( void ) { return false; }

	// Get the body parameter
	virtual int						GetBody() { return 0; }

	virtual const model_t*			GetModel( )				{ return NULL; }
	virtual int						DrawModel( int flags )	{ return 0; }
	virtual void					ComputeFxBlend( )		{ return; }
	virtual int						GetFxBlend( )			{ return 255; }
	virtual bool					LODTest()				{ return true; }
	virtual bool					SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )	{ return true; }
	virtual void					SetupWeights( void )							{}
	virtual void					DoAnimationEvents( void )						{}

	virtual const Vector&	EntitySpaceSurroundingMins() const	{ return vec3_origin; }
	virtual const Vector&	EntitySpaceSurroundingMaxs() const	{ return vec3_origin; }
	virtual const Vector&	WorldSpaceSurroundingMins() const	{ return vec3_origin; }
	virtual const Vector&	WorldSpaceSurroundingMaxs() const	{ return vec3_origin; }

	// Determine the color modulation amount
	virtual void	GetColorModulation( float* color )
	{
		assert(color);
		color[0] = color[1] = color[2] = 1.0f;
	}

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveShadows() { return false; }


// IClientUnknown implementation.
public:
	virtual void SetRefEHandle( const CBaseHandle &handle )	{ Assert( false ); }
	virtual const CBaseHandle& GetRefEHandle() const		{ Assert( false ); return *((CBaseHandle*)0); }

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetClientCollideable()	{ return 0; }
	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientNetworkable*	GetClientNetworkable()	{ return 0; }
	virtual IClientEntity*		GetIClientEntity()		{ return 0; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return 0; }
	virtual IClientThinkable*	GetClientThinkable()	{ return 0; }
};


#endif // ICLIENTRENDERABLE_H
