//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cdll_int.h"
#include "cdll_util.h"
#include "ParticleMgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "c_baseentity.h"
#include "c_baseparticleentity.h"
#include "imaterialproxy.h"
#include "imaterialvar.h"
#include "ParticleSphereRenderer.h"


//==================================================
// C_SphereTest
//==================================================

// zAngle's range is [-PI/2,PI/2].
// When zAngle is 0, the position is in the middle of the sphere (vertically).
// When zAngle is  PI/2, the position is at the top of the sphere.
// When zAngle is -PI/2, the position is at the bottom of the sphere.
inline Vector CalcSphereVecAngles2(float xyAngle, float zAngle, float fRadius)
{
	Vector vec;

	vec.x = cos(xyAngle) * cos(zAngle);
	vec.y = sin(xyAngle) * cos(zAngle);
	vec.z = sin(zAngle);

	return vec * fRadius;
}


class C_SphereTest : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLIENTCLASS();
	typedef C_BaseParticleEntity BaseClass;

	C_SphereTest();
	~C_SphereTest();

	class SphereTestParticle : public Particle
	{
	public:
		float		m_Alpha;
		float		m_AlphaVel;

		float		m_Theta;
		float		m_ThetaVel;

		float		m_Dist;
	};

//C_BaseEntity
public:

	virtual void	PostDataUpdate( bool bnewentity );
	virtual bool	ShouldDraw();


//IPrototypeAppEffect
public:
	virtual void		Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);
	virtual bool		GetPropEditInfo(RecvTable **ppTable, void **ppObj);


//IParticleEffect
public:
	virtual void	Update(float fTimeDelta);
	virtual bool	SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortKey);


//Stuff from the datatable
public:

	Vector			m_LightColor[CParticleSphereRenderer::NUM_TOTAL_LIGHTS];
	Vector			m_LightPos[CParticleSphereRenderer::NUM_TOTAL_LIGHTS];
	float			m_LightIntensity[CParticleSphereRenderer::NUM_TOTAL_LIGHTS];
	int				m_nParticles;
	float			m_flLightIndex;
	unsigned char	m_cDirLightColor[4];
	CParticleSphereRenderer	m_Renderer;

private:

	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle	m_MaterialHandle;
};


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(SphereTest, C_SphereTest);


// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_SphereTest, DT_SphereTest, CSphereTest)
	RecvPropVector(RECVINFO(m_LightColor[0])),
	RecvPropVector(RECVINFO(m_LightPos[0])),
	RecvPropFloat(RECVINFO(m_LightIntensity[0])),

	RecvPropInt(RECVINFO(m_nParticles)),
	
	RecvPropVector(RECVINFO(m_LightColor[1])),
	RecvPropVector(RECVINFO(m_LightPos[1])),
	RecvPropFloat(RECVINFO(m_LightIntensity[1])),

	RecvPropVector(RECVINFO(m_LightColor[2])),
	RecvPropFloat(RECVINFO(m_LightIntensity[2])),
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_SphereTest implementation.
// ------------------------------------------------------------------------- //
C_SphereTest::C_SphereTest()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;

	m_nParticles = 4 * 1024;
	
	m_LightPos[0].Init(40,0,0);
	m_LightColor[0].Init( 1, 0, 0 );
	m_LightIntensity[0] = 500;

	m_LightPos[1].Init(0, 25, 40);
	m_LightColor[1].Init( 0, 0, 1 );
	m_LightIntensity[1] = 700;

	m_LightColor[2].Init(1, 0.5, 0);
	m_LightIntensity[2] = 1200;
}


C_SphereTest::~C_SphereTest()
{
}


//-----------------------------------------------------------------------------
// Purpose: Called after a data update has occured
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SphereTest::PostDataUpdate(bool bnewentity)
{
	C_BaseEntity::PostDataUpdate(bnewentity);

	if(bnewentity)
	{
		Start(&g_ParticleMgr, NULL);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the effect should be shown
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SphereTest::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Starts the effect
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SphereTest::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	pParticleMgr->AddEffect( &m_ParticleEffect, this );
	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/dx8_particle_sphere");
	if( m_MaterialHandle != INVALID_MATERIAL_HANDLE )
	{
		IMaterial *pMaterial = m_ParticleEffect.GetMaterialFromPMaterial( m_MaterialHandle );
		if( pMaterial )
		{
			m_Renderer.Init( pMaterial );
		}
	}

	m_pParticleMgr = pParticleMgr;

	// Make a bunch of particles.
	srand( 0 );
	for( int i=0; i < m_nParticles; i++ )
	{
		SphereTestParticle *pParticle = (SphereTestParticle*)m_ParticleEffect.AddParticle( sizeof(SphereTestParticle), m_MaterialHandle );

		static float spread=30;
		static float minVel = 0.01, maxVel = 0.3;
		pParticle->m_Alpha = FRand( 0, 6.28 );
		pParticle->m_Theta = FRand( 0, 6.28 );
		pParticle->m_AlphaVel = FRand( minVel, maxVel );
		pParticle->m_ThetaVel = FRand( minVel, maxVel );
		pParticle->m_Dist = FRand( spread*0.1f, spread );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **ppTable - 
//			**ppObj - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SphereTest::GetPropEditInfo( RecvTable **ppTable, void **ppObj )
{
	*ppTable = &REFERENCE_RECV_TABLE(DT_SphereTest);
	*ppObj = this;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------

// For the particle app.
extern Vector g_DirLightPos;

void C_SphereTest::Update(float fTimeDelta)
{
	// Pass light info to the CParticleSphereRenderer.
	for( int i=0; i < CParticleSphereRenderer::NUM_TOTAL_LIGHTS; i++ )
	{
		m_Renderer.Light(i).m_vPos = m_LightPos[i];
		m_Renderer.Light(i).m_vColor = m_LightColor[i];
		m_Renderer.Light(i).m_flIntensity = m_LightIntensity[i];
	}

	m_Renderer.DirectionalLight().m_vPos = g_DirLightPos;

	m_Renderer.Update( m_pParticleMgr );
}


bool C_SphereTest::SimulateAndRender(Particle *pInParticle, ParticleDraw *pDraw, float &sortKey)
{
	SphereTestParticle *pParticle = (SphereTestParticle*)pInParticle;

	Vector pos = CalcSphereVecAngles2( pParticle->m_Alpha, pParticle->m_Theta, pParticle->m_Dist );
	pParticle->m_Alpha += pParticle->m_AlphaVel * pDraw->GetTimeDelta();
	pParticle->m_Theta += pParticle->m_ThetaVel * pDraw->GetTimeDelta();

	// Render.
	Vector tPos;
	TransformParticle(m_pParticleMgr->GetModelView(), pos, tPos);
	sortKey = tPos.z;

	m_Renderer.RenderParticle( pDraw, pos, tPos, 255.4f, 1.0f );
	return true;
}

