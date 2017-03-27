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
#include "cbase.h"
#include "c_basetempentity.h"
#include "particles_simple.h"

extern IVEngineClient *engine;

#define FOG_MAX_FOLLOW_VEL 60
#define FOG_NUM_RING_STEPS 15
//-----------------------------------------------------------------------------
// Purpose: Fog Ripple TE
//-----------------------------------------------------------------------------
class C_TEFogRipple : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFogRipple, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEFogRipple( void );
	virtual			~C_TEFogRipple( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	Vector			m_vecVelocity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEFogRipple::C_TEFogRipple( void )
{
	m_vecOrigin.Init();
	m_vecVelocity.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEFogRipple::~C_TEFogRipple( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
#define PI 3.14159265359

void C_TEFogRipple::PostDataUpdate( DataUpdateType_t updateType )
{
	//----------------
	// Smoke ring
	//----------------
	CSmartPtr<CSimpleEmitter> dustEmitter = CSimpleEmitter::Create( "C_TEFogRipple" );
	if ( !dustEmitter )
	{
		return;
	}

	// Put a cap on the velocity
	if (m_vecVelocity.Length() > FOG_MAX_FOLLOW_VEL)
	{
		VectorNormalize(m_vecVelocity);
		m_vecVelocity *= FOG_MAX_FOLLOW_VEL;
	}

	dustEmitter->SetSortOrigin( m_vecOrigin );

	PMaterialHandle hMaterial = dustEmitter->GetPMaterial( "particle/particle_fog" );

	float flStepSize	= 2*PI/FOG_NUM_RING_STEPS;
	float flCurAng		= random->RandomFloat(0,2*PI);

	for (int i = 0; i < FOG_NUM_RING_STEPS; i++)
	{
		Vector vDir;
		vDir[0] = cos(flCurAng);
		vDir[1] = sin(flCurAng);
		vDir[2] = 0;
		flCurAng	+= flStepSize;

		// Brightness despends on direction of motion
		// Particles are brightest the move orthogonally
		Vector vVelNorm = m_vecVelocity;
		VectorNormalize(vVelNorm);
		float flDot = 1-fabs(DotProduct(vDir,vVelNorm));

		if (flDot > 0.05)
		{
			SimpleParticle *pParticle = (SimpleParticle *) dustEmitter->AddParticle( sizeof(SimpleParticle), hMaterial, m_vecOrigin );

			if ( pParticle == NULL)
				return;

			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= random->RandomFloat(2,4);
			pParticle->m_uchStartSize	= random->RandomFloat(1,3);
			pParticle->m_uchEndSize		= random->RandomFloat(15,30);
			pParticle->m_vecVelocity	= (vDir * random->RandomFloat(25,35))+m_vecVelocity;
			pParticle->m_vecVelocity[2]	= -10;
			pParticle->m_uchStartAlpha	= 25+(75*flDot);
			pParticle->m_uchEndAlpha	= 0;
			pParticle->m_flRoll			= random->RandomFloat( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -5, 5 );
			pParticle->m_uchColor[0]	= 255.0f;
			pParticle->m_uchColor[1]	= 255.0f;
			pParticle->m_uchColor[2]	= 255.0f;
		}
	}
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEFogRipple, DT_TEFogRipple, CTEFogRipple)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecVelocity)),
END_RECV_TABLE()

void TE_FogRipple( IRecipientFilter& filter, float delay,
	const Vector* pos,const Vector* velocity)
{
	// Major hack to mimic receiving network message
	__g_C_TEFogRipple.m_vecOrigin = *pos;
	__g_C_TEFogRipple.m_vecVelocity = *velocity;
	__g_C_TEFogRipple.PostDataUpdate( DATA_UPDATE_CREATED );
}