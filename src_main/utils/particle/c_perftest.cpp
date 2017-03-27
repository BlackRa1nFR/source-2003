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


//==================================================
// C_PerfTest
//==================================================

class C_PerfTest : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLIENTCLASS();
	typedef C_BaseParticleEntity BaseClass;

	C_PerfTest();
	~C_PerfTest();

	class PerfTestParticle : public Particle
	{
	public:
		Vector		m_Pos;
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

	enum {NUM_LIGHTS=2};
	Vector			m_LightColor[NUM_LIGHTS];
	Vector			m_LightPos[NUM_LIGHTS];
	float			m_LightIntensity[NUM_LIGHTS];
	int				m_nParticles;

private:

	CParticleMgr	*m_pParticleMgr;
	PMaterialHandle	m_MaterialHandle;
};


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(PerfTest, C_PerfTest);


// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_PerfTest, DT_PerfTest, CPerfTest)
	RecvPropVector(RECVINFO(m_LightColor[0])),
	RecvPropVector(RECVINFO(m_LightPos[0])),
	RecvPropFloat(RECVINFO(m_LightIntensity[0])),

	RecvPropInt(RECVINFO(m_nParticles)),
	
	RecvPropVector(RECVINFO(m_LightColor[1])),
	RecvPropVector(RECVINFO(m_LightPos[1])),
	RecvPropFloat(RECVINFO(m_LightIntensity[1])),
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_PerfTest implementation.
// ------------------------------------------------------------------------- //
C_PerfTest::C_PerfTest()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;

	m_nParticles = 30000;
	
	m_LightPos[0].Init(0,0,0);
	m_LightColor[0].Init( 1, 0, 0 );
	m_LightIntensity[0] = 500 * 255.5;

	m_LightPos[1].Init(0, 25, 40);
	m_LightColor[1].Init( 0, 0, 1 );
	m_LightIntensity[1] = 1000 * 255.5;
}


C_PerfTest::~C_PerfTest()
{
}


//-----------------------------------------------------------------------------
// Purpose: Called after a data update has occured
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_PerfTest::PostDataUpdate(bool bnewentity)
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
bool C_PerfTest::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Starts the effect
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_PerfTest::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	pParticleMgr->AddEffect( &m_ParticleEffect, this );
	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/particle_sphere");

	m_pParticleMgr = pParticleMgr;

	// Make a bunch of particles.
	srand( 0 );
	for( int i=0; i < m_nParticles; i++ )
	{
		PerfTestParticle *pParticle = (PerfTestParticle*)m_ParticleEffect.AddParticle( sizeof(PerfTestParticle), m_MaterialHandle );

		static float spread=30;
		pParticle->m_Pos = Vector(
			FRand(-spread, spread),
			FRand(-spread, spread),
			FRand(-spread, spread));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **ppTable - 
//			**ppObj - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_PerfTest::GetPropEditInfo( RecvTable **ppTable, void **ppObj )
{
	//*ppTable = &REFERENCE_RECV_TABLE(DT_PerfTest);
	//*ppObj = this;
	//return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_PerfTest::Update(float fTimeDelta)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_PerfTest::SimulateAndRender(Particle *pInParticle, ParticleDraw *pDraw, float &sortKey)
{
	PerfTestParticle *pParticle = (PerfTestParticle*)pInParticle;

	// Render.
	Vector tPos;
	TransformParticle(m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos);
	sortKey = tPos.z;

	Vector vColor( 0, 0, 0 );
	for( int i=0; i < NUM_LIGHTS; i++ )
	{
		float fDist = pParticle->m_Pos.DistToSqr( m_LightPos[i] );
		float fAmt;
		if( fDist > 0.0001f )
			fAmt = m_LightIntensity[i] / fDist;
		else
			fAmt = 1000;

		vColor += m_LightColor[i] * fAmt;
	}
	vColor = vColor.Min( Vector(255.1,255.1,255.1) );

	RenderParticle_Color255Size(
		pDraw,
		tPos,
		vColor,		// color
		255,		// alpha
		1			// size
		);

	return true;
}

