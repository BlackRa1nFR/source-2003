//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "model_types.h"
#include "vcollide.h"
#include "vcollide_parse.h"
#include "solidsetdefaults.h"
#include "bone_setup.h"
#include "engine/ivmodelinfo.h"
#include "physics.h"
#include "view.h"
#include "clienteffectprecachesystem.h"
#include "c_physicsprop.h"

IMPLEMENT_CLIENTCLASS_DT(C_PhysicsProp, DT_PhysicsProp, CPhysicsProp)
	RecvPropFloat( RECVINFO( m_fadeMinDist ) ), 
	RecvPropFloat( RECVINFO( m_fadeMaxDist ) ), 
END_RECV_TABLE()

#ifdef _DEBUG

ConVar r_FadePhysProps( "r_FadePhysProps", "1" );

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PhysicsProp::C_PhysicsProp( void )
{
	m_pPhysicsObject = NULL;
	m_bClientsideOnly = false;
	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PhysicsProp::~C_PhysicsProp( void )
{
	if ( m_bClientsideOnly )
	{
		VPhysicsDestroyObject();
	}
}

bool C_PhysicsProp::ShouldFade( void )
{
	// is this object set up to fade?
	if( m_fadeMinDist <= 0 && m_fadeMaxDist <= 0 )
	{
		// The object isn't fadable:
		return false;
	}

	return true;
}


bool C_PhysicsProp::ComputeFade( void )
{
	return false;

	if( !ShouldFade() )
		return false;

	float min = m_fadeMinDist;
	float max = m_fadeMaxDist;


#ifdef _DEBUG
	if( !r_FadePhysProps.GetBool() )
	{
		m_nRenderMode = kRenderNormal;
		SetRenderColorA( 255.f );
		return true;
	}
#endif
	// Generally this should never happen:
	if( min > max )
	{
		swap( min, max );
	}

	// If a negative value is provided for the min fade distance, then base it off the max.
	if( min < 0 )
	{
		min = max - 400;
		if( min < 0 ) 
			min = 0;
	}

	float currentDistance = (CurrentViewOrigin() - GetAbsOrigin()).Length();

	// If I'm inside the minimum range than don't resort to alpha trickery
	if( currentDistance < min )
	{
		m_nRenderMode = kRenderNormal;
		SetRenderColorA( 255.f );
		return true;
	}

	// Am I out of fade range?
	if( currentDistance > max )
	{
		m_nRenderMode = kRenderTransAlpha;
		SetRenderColorA( 0.f );
		return true;

	}

	SetRenderColorA( Lerp(( ( currentDistance - min ) / ( max - min ) ), 255, 0 ) );
	m_nRenderMode = kRenderTransAlpha;

	// The object is fadable:
	return true;
}

void C_PhysicsProp::ComputeFxBlend( void )
{
	ComputeFade();
	BaseClass::ComputeFxBlend();
}
