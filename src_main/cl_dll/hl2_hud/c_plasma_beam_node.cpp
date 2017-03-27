//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particles_simple.h"
#include "c_tracer.h"
#include "particle_collision.h"
#include "view.h"


#define	PLASMASPARK_LIFETIME 0.5
#define SPRAYS_PER_THINK		12

class  C_PlasmaBeamNode;

//##################################################################
//
// > CPlasmaSpray
//
//##################################################################
class CPlasmaSpray : public CSimpleEmitter
{
public:
	CPlasmaSpray( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static CSmartPtr<CPlasmaSpray>	Create( const char *pDebugName );
	void					Think( void );
	void					UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	bool					SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortDist );
	EHANDLE					m_pOwner;
	CParticleCollision		m_ParticleCollision;

private:
	CPlasmaSpray( const CPlasmaSpray & );
};

//##################################################################
//
// PlasmaBeamNode - generates plasma spray
//
//##################################################################
class C_PlasmaBeamNode : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PlasmaBeamNode, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_PlasmaBeamNode();
	~C_PlasmaBeamNode(void);

public:
	void			ClientThink(void);
	void			AddEntity( void );
	void			OnDataChanged(DataUpdateType_t updateType);
	bool			ShouldDraw();
	bool			m_bSprayOn;
	CSmartPtr<CPlasmaSpray>	m_pFirePlasmaSpray;
};

//##################################################################
//
// > CPlasmaSpray
//
//##################################################################

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
CSmartPtr<CPlasmaSpray> CPlasmaSpray::Create( const char *pDebugName )
{
	CPlasmaSpray *pRet = new CPlasmaSpray( pDebugName );
	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
void CPlasmaSpray::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	Vector		vGravity = Vector(0,0,-1000);
	float		flFrametime = gpGlobals->frametime;
	vGravity	= flFrametime * vGravity;
	pParticle->m_vecVelocity += vGravity;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CPlasmaSpray::SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortDist )
{
	SimpleParticle* pSimpleParticle = (SimpleParticle*)pParticle;

	//Should this particle die?
	pSimpleParticle->m_flLifetime += pDraw->GetTimeDelta();

	C_PlasmaBeamNode* pNode = (C_PlasmaBeamNode*)((C_BaseEntity*)m_pOwner);
	if ( pSimpleParticle->m_flLifetime >= pSimpleParticle->m_flDieTime )
	{
		return false;
	}
	// If owner is gone or spray off remove me
	else if (pNode == NULL || !pNode->m_bSprayOn)
	{
		return false;
	}

	float scale = random->RandomFloat( 0.02, 0.08 );

	// NOTE: We need to do everything in screen space
	Vector  delta;
	Vector	start;
	TransformParticle(g_ParticleMgr.GetModelView(), pSimpleParticle->m_Pos, start);

	Vector3DMultiply( CurrentWorldToViewMatrix(), pSimpleParticle->m_vecVelocity, delta );

	delta[0] *= scale;
	delta[1] *= scale;
	delta[2] *= scale;

	// See c_tracer.* for this method
	Tracer_Draw( pDraw, start, delta, random->RandomInt( 2, 8 ), 0 );

	//Simulate the movement with collision
	trace_t trace;
	float   timeDelta = pDraw->GetTimeDelta();
	m_ParticleCollision.MoveParticle( pSimpleParticle->m_Pos, pSimpleParticle->m_vecVelocity, NULL, timeDelta, &trace );

	return true;
}


//##################################################################
//
// PlasmaBeamNode - generates plasma spray
//
//##################################################################

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
C_PlasmaBeamNode::C_PlasmaBeamNode(void)
{
	m_bSprayOn			= false;
	m_pFirePlasmaSpray = CPlasmaSpray::Create( "C_PlasmaBeamNode" );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
C_PlasmaBeamNode::~C_PlasmaBeamNode(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PlasmaBeamNode::AddEntity( void )
{
	m_pFirePlasmaSpray->SetSortOrigin( GetAbsOrigin() );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_PlasmaBeamNode::OnDataChanged(DataUpdateType_t updateType)
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		Vector vMoveDir = GetAbsVelocity();
		float  flVel = VectorNormalize(vMoveDir);
		m_pFirePlasmaSpray->m_ParticleCollision.Setup( GetAbsOrigin(), &vMoveDir, 0.3, 
											flVel-50, flVel+50, 800, 0.5 );
		SetNextClientThink(gpGlobals->curtime + 0.01);
	}
	C_BaseEntity::OnDataChanged(updateType);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool C_PlasmaBeamNode::ShouldDraw()
{
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_PlasmaBeamNode::ClientThink(void)
{
	if (!m_bSprayOn)
	{
		return;
	}
	
	trace_t trace;
	Vector vEndTrace = GetAbsOrigin() + (0.3*GetAbsVelocity());
	UTIL_TraceLine( GetAbsOrigin(), vEndTrace, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &trace );
	if ( trace.fraction != 1.0f || trace.startsolid)
	{
		m_bSprayOn = false;
		return;
	}

	PMaterialHandle handle = m_pFirePlasmaSpray->GetPMaterial( "sprites/plasmaember" );
	for (int i=0;i<SPRAYS_PER_THINK;i++)
	{
		SimpleParticle	*sParticle;

		//Make a new particle
		if ( random->RandomInt( 0, 2 ) == 0 )
		{
			float ranx = random->RandomFloat( -28.0f, 28.0f );
			float rany = random->RandomFloat( -28.0f, 28.0f );
			float ranz = random->RandomFloat( -28.0f, 28.0f );

			Vector vNewPos	=  GetAbsOrigin();
			Vector vAdd		=  Vector(GetAbsAngles().x,GetAbsAngles().y,GetAbsAngles().z)*random->RandomFloat(-60,120);
			vNewPos			+= vAdd;

			sParticle = (SimpleParticle *) m_pFirePlasmaSpray->AddParticle( sizeof(SimpleParticle), handle, vNewPos );
			
			sParticle->m_flLifetime		= 0.0f;
			sParticle->m_flDieTime		= PLASMASPARK_LIFETIME;

			sParticle->m_vecVelocity	= GetAbsVelocity();
			sParticle->m_vecVelocity.x	+= ranx;
			sParticle->m_vecVelocity.y	+= rany;
			sParticle->m_vecVelocity.z	+= ranz;
			m_pFirePlasmaSpray->m_pOwner	=  this;
		}
	}

	SetNextClientThink(gpGlobals->curtime + 0.05);
}

IMPLEMENT_CLIENTCLASS_DT(C_PlasmaBeamNode, DT_PlasmaBeamNode, CPlasmaBeamNode )
	RecvPropVector	(RECVINFO(m_vecVelocity)),
	RecvPropInt		(RECVINFO(m_bSprayOn)),
END_RECV_TABLE()
