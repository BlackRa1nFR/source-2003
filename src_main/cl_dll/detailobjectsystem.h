//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#if !defined( DETAILOBJECTSYSTEM_H )
#define DETAILOBJECTSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "IGameSystem.h"
#include "IClientEntityInternal.h"
#include "engine/IVModelRender.h"
#include "vector.h"

struct model_t;


//-----------------------------------------------------------------------------
// Renderable detail objects
//-----------------------------------------------------------------------------
class CBaseStaticModel : public IClientUnknown, public IClientRenderable
{
public:
	DECLARE_CLASS_NOBASE( CBaseStaticModel );

	// constructor, destructor
	CBaseStaticModel();
	virtual ~CBaseStaticModel();

	// Initialization
	virtual bool Init( int index, const Vector& org, const QAngle& angles, model_t* pModel );
	void SetAlpha( unsigned char alpha ) { m_Alpha = alpha; }


// IClientUnknown overrides.
public:

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetClientCollideable()	{ return 0; }		// Static props DO implement this.
	virtual IClientNetworkable*	GetClientNetworkable()	{ return 0; }
	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientEntity*		GetIClientEntity()		{ return 0; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return 0; }
	virtual IClientThinkable*	GetClientThinkable()	{ return 0; }


// IClientRenderable overrides.
public:

	virtual int						GetBody() { return 0; }
	virtual const Vector&			GetRenderOrigin( );
	virtual const QAngle&			GetRenderAngles( );
	virtual bool					ShouldDraw();
	virtual bool					IsTransparent( void );
	virtual const model_t*			GetModel( );
	virtual int						DrawModel( int flags );
	virtual void					ComputeFxBlend( );
	virtual int						GetFxBlend( );
	virtual void					GetColorModulation( float* color );
	virtual bool					LODTest();
	virtual bool					SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void					SetupWeights( void );
	virtual void					DoAnimationEvents( void );
	virtual void					GetRenderBounds( Vector& mins, Vector& maxs );
	virtual const Vector&			EntitySpaceSurroundingMins() const;
	virtual const Vector&			EntitySpaceSurroundingMaxs() const;
	virtual const Vector&			WorldSpaceSurroundingMins() const;
	virtual const Vector&			WorldSpaceSurroundingMaxs() const;
	virtual bool					ShouldCacheRenderInfo();
	virtual bool					ShouldReceiveShadows();
	virtual bool					UsesFrameBufferTexture();


protected:
	Vector	m_Origin;
	QAngle	m_Angles;
	model_t* m_pModel;
	ModelInstanceHandle_t m_ModelInstance;
	unsigned char	m_Alpha;
};


//-----------------------------------------------------------------------------
// Responsible for managing detail objects
//-----------------------------------------------------------------------------
class IDetailObjectSystem : public IGameSystem
{
public:
    // Gets a particular detail object
	virtual IClientRenderable* GetDetailModel( int idx ) = 0;

	// Gets called each view
	virtual void BuildDetailObjectRenderLists( ) = 0;

	// Renders all opaque detail objects in a particular set of leaves
	virtual void RenderOpaqueDetailObjects( int nLeafCount, int *pLeafList ) = 0;

	// Renders all translucent detail objects in a particular set of leaves
	virtual void RenderTranslucentDetailObjects( const Vector &viewOrigin, const Vector &viewForward, int nLeafCount, int *pLeafList ) = 0;
};


//-----------------------------------------------------------------------------
// System for dealing with detail objects
//-----------------------------------------------------------------------------
IDetailObjectSystem* DetailObjectSystem();


#endif // DETAILOBJECTSYSTEM_H

