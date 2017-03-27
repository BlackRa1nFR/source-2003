//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a particle system steam jet.
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "particle_prototype.h"
#include "baseparticleentity.h"
#include "particles_simple.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//==================================================
// C_SmokeStack
//==================================================

class C_SmokeStack : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_SmokeStack, C_BaseParticleEntity );

	C_SmokeStack();
	~C_SmokeStack();

	class SmokeStackParticle : public Particle
	{
	public:
		Vector		m_Velocity;
		Vector		m_vAccel;
		float		m_Lifetime;
	};

//C_BaseEntity
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw();


//IPrototypeAppEffect
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);
	virtual bool	GetPropEditInfo(RecvTable **ppTable, void **ppObj);


//IParticleEffect
public:
	virtual void	Update(float fTimeDelta);
	virtual bool	SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortKey);


//Stuff from the datatable
public:

	CParticleSphereRenderer	m_Renderer;

	float			m_SpreadSpeed;
	float			m_Speed;
	float			m_StartSize;
	float			m_EndSize;
	float			m_Rate;
	float			m_JetLength;	// Length of the jet. Lifetime is derived from this.

	int				m_bEmit;		// Emit particles?
	float			m_flBaseSpread;

	class CLightInfo
	{
	public:
		Vector		m_vPos;
		Vector		m_vColor;
		float		m_flIntensity;
	};

	CLightInfo		m_Lights[CParticleSphereRenderer::NUM_AMBIENT_LIGHTS];
	int				m_nLights;

	int				m_DirLightSource; // 0=bottom, 1=top
	Vector			m_DirLightColor;
	Vector			m_DirLightColor255;	// m_DirLightColor / 255.9
	Vector			m_vBaseColor;
	
	Vector			m_vWind;
	float			m_flTwist;

private:
	C_SmokeStack( const C_SmokeStack & );

	float			m_TwistMat[2][2];
	int				m_bTwist;

	float			m_flAlphaScale;
	float			m_InvLifetime;		// Calculated from m_JetLength / m_Speed;

	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle	m_MaterialHandle;
	TimedEvent		m_ParticleSpawn;
};


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(SmokeStack, C_SmokeStack);


// Datatable..
#define RECVLIGHTINFO(index) \
	RecvPropVector( RECVINFO(m_Lights[index].m_vPos) ),\
	RecvPropVector( RECVINFO(m_Lights[index].m_vColor) ),\
	RecvPropFloat( RECVINFO(m_Lights[index].m_flIntensity) )


BEGIN_RECV_TABLE_NOBASE( C_SmokeStack::CLightInfo, DT_SmokeStackLightInfo )
	RecvPropVector( RECVINFO( m_vPos ) ),
	RecvPropVector( RECVINFO( m_vColor ) ),
	RecvPropFloat( RECVINFO( m_flIntensity ) )
END_RECV_TABLE()


IMPLEMENT_CLIENTCLASS_DT(C_SmokeStack, DT_SmokeStack, CSmokeStack)
	RecvPropFloat(RECVINFO(m_SpreadSpeed), 0),
	RecvPropFloat(RECVINFO(m_Speed), 0),
	RecvPropFloat(RECVINFO(m_StartSize), 0),
	RecvPropFloat(RECVINFO(m_EndSize), 0),
	RecvPropFloat(RECVINFO(m_Rate), 0),
	RecvPropFloat(RECVINFO(m_JetLength), 0),
	RecvPropInt(RECVINFO(m_bEmit), 0),
	RecvPropFloat(RECVINFO(m_flBaseSpread)),
	RecvPropFloat(RECVINFO(m_flTwist)),
	RecvPropInt(RECVINFO(m_DirLightSource)),

	RecvPropVector(RECVINFO(m_DirLightColor)),
	RecvPropVector(RECVINFO(m_vWind)),
	
	RECVLIGHTINFO(0),
	RECVLIGHTINFO(1),
	RecvPropInt( RECVINFO(m_nLights) )
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_SmokeStack implementation.
// ------------------------------------------------------------------------- //
C_SmokeStack::C_SmokeStack()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;
	
	m_SpreadSpeed = 15;
	m_Speed = 30;
	m_StartSize = 10;
	m_EndSize = 15;
	m_Rate = 80;
	m_JetLength = 180;
	m_bEmit = true;

	m_flBaseSpread = 20;

	m_Lights[0].m_vPos.Init(0,0,-100);
	m_Lights[0].m_vColor.Init( 40, 40, 40 );
	m_Lights[0].m_flIntensity = 8000;

	m_Lights[1].m_vPos.Init(0, 25, 140);
	m_Lights[1].m_vColor.Init( 0, 200, 20 );
	m_Lights[1].m_flIntensity = 500;

	m_DirLightSource = 0;
	SetRenderColor( 0, 0, 0, 255 );
	m_vWind.Init();

	m_DirLightColor.Init( 255, 128, 0 );
	m_nLights = 0;

	m_flTwist = 0;

	// Don't even simulate when outside the PVS or frustum.
	m_ParticleEffect.SetAlwaysSimulate( false );
}


C_SmokeStack::~C_SmokeStack()
{
	if(m_pParticleMgr)
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
}


//-----------------------------------------------------------------------------
// Purpose: Called after a data update has occured
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SmokeStack::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	m_nLights = clamp( m_nLights, 0, CParticleSphereRenderer::NUM_AMBIENT_LIGHTS );

	if(updateType == DATA_UPDATE_CREATED)
	{
		Start(&g_ParticleMgr, NULL);
	}

	// Recalulate lifetime in case length or speed changed.
	m_InvLifetime = m_Speed / m_JetLength;
}


//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the effect should be shown
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SmokeStack::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Starts the effect
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SmokeStack::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	pParticleMgr->AddEffect( &m_ParticleEffect, this );
	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/SmokeStack");
	m_ParticleSpawn.Init(m_Rate);

	m_InvLifetime = m_Speed / m_JetLength;

	m_pParticleMgr = pParticleMgr;

	// Figure out how we need to draw.
	IMaterial *pMaterial = m_MaterialHandle;
	if( pMaterial )
	{
		m_Renderer.Init( pMaterial );
	}
	
	if( m_DirLightSource == 0 )
		m_Renderer.SetCommonDirLight( pParticleMgr, CParticleMgr::POINTSOURCE_INDEX_BOTTOM );
	else
		m_Renderer.SetCommonDirLight( pParticleMgr, CParticleMgr::POINTSOURCE_INDEX_TOP );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **ppTable - 
//			**ppObj - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SmokeStack::GetPropEditInfo( RecvTable **ppTable, void **ppObj )
{
	*ppTable = &REFERENCE_RECV_TABLE(DT_SmokeStack);
	*ppObj = this;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SmokeStack::Update(float fTimeDelta)
{
	if( !m_pParticleMgr )
	{
		assert(false);
		return;
	}

	m_DirLightColor255 = m_DirLightColor / 255.0f;

	// Don't spawn particles unless we're visible.
	if( m_bEmit && m_ParticleEffect.WasDrawnPrevFrame() )
	{
		// Add new particles.																	
		Vector forward, right, up;
		AngleVectors(GetAbsAngles(), &forward, &right, &up);			

		float tempDelta = fTimeDelta;
		while(m_ParticleSpawn.NextEvent(tempDelta))
		{
			// Make a new particle.
			if(SmokeStackParticle *pParticle = (SmokeStackParticle*)m_ParticleEffect.AddParticle(sizeof(SmokeStackParticle), m_MaterialHandle))
			{
				float angle = FRand( 0, M_PI_F );
				
				pParticle->m_Pos = GetAbsOrigin() +
					right * (cos( angle ) * m_flBaseSpread) +
					forward * (sin( angle ) * m_flBaseSpread);

				pParticle->m_Velocity = 
					FRand(-m_SpreadSpeed,m_SpreadSpeed) * right +
					FRand(-m_SpreadSpeed,m_SpreadSpeed) * forward +
					m_Speed * up;

				pParticle->m_vAccel = m_vWind;
				pParticle->m_Lifetime = 0;
			}
		}
	}

	// Setup the twist matrix.
	float flTwist = (m_flTwist * (M_PI_F * 2.f) / 360.0f) * Helper_GetFrameTime();
	if( m_bTwist = !!flTwist )
	{
		m_TwistMat[0][0] =  cos(flTwist);
		m_TwistMat[0][1] =  sin(flTwist);
		m_TwistMat[1][0] = -sin(flTwist);
		m_TwistMat[1][1] =  cos(flTwist);
	}

	m_flAlphaScale = (float)m_clrRender->a;
	m_vBaseColor.Init( m_clrRender->r, m_clrRender->g, m_clrRender->b );

	// Set the colors in the CParticleRenderer.
	int i;
	for( i=0; i < m_nLights; i++ )
	{
		CParticleSphereRenderer::CLightInfo *pInfo = &m_Renderer.AmbientLight(i);

		pInfo->m_vPos = m_Lights[i].m_vPos;
		pInfo->m_vColor = m_Lights[i].m_vColor * (1.0f / 255.9f);
		pInfo->m_flIntensity = m_Lights[i].m_flIntensity;
	}
	for( int j=i; j < CParticleSphereRenderer::NUM_AMBIENT_LIGHTS; j++ )
	{
		m_Renderer.Light(j).m_flIntensity = 0;
	}
	m_Renderer.DirectionalLight().m_vColor = m_DirLightColor / 255.1f;

	m_Renderer.Update( m_pParticleMgr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SmokeStack::SimulateAndRender(Particle *pInParticle, ParticleDraw *pDraw, float &sortKey)
{
	SmokeStackParticle *pParticle = (SmokeStackParticle*)pInParticle;

	// Should this particle die?
	pParticle->m_Lifetime += pDraw->GetTimeDelta();

	float tLifetime = pParticle->m_Lifetime * m_InvLifetime;
	if( tLifetime >= 1 )
		return false;
	
	// Transform.						   
	Vector tPos;
	if( m_bTwist )
	{
		Vector vTwist(
			pParticle->m_Pos.x - GetAbsOrigin().x,
			pParticle->m_Pos.y - GetAbsOrigin().y,
			0);

		pParticle->m_Pos.x = vTwist.x * m_TwistMat[0][0] + vTwist.y * m_TwistMat[0][1] + GetAbsOrigin().x;
		pParticle->m_Pos.y = vTwist.x * m_TwistMat[1][0] + vTwist.y * m_TwistMat[1][1] + GetAbsOrigin().y;
	}

	TransformParticle( m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos );
	sortKey = tPos.z;

	pParticle->m_Pos = pParticle->m_Pos + 
		pParticle->m_Velocity * pDraw->GetTimeDelta() + 
		pParticle->m_vAccel * (0.5f * pDraw->GetTimeDelta() * pDraw->GetTimeDelta());

	pParticle->m_Velocity += pParticle->m_vAccel * pDraw->GetTimeDelta();

	// Figure out its alpha. Squaring it after it gets halfway through its lifetime
	// makes it get translucent and fade out for a longer time.
	//float alpha = cosf( -M_PI_F + tLifetime * M_PI_F * 2.f ) * 0.5f + 0.5f;
	float alpha = TableCos( -M_PI_F + tLifetime * M_PI_F * 2.f ) * 0.5f + 0.5f;
	if( tLifetime > 0.5f )
		alpha *= alpha;

	m_Renderer.RenderParticle_AddColor(
		pDraw,
		pParticle->m_Pos,
		tPos,
		alpha * m_flAlphaScale,
		FLerp(m_StartSize, m_EndSize, tLifetime),
		m_vBaseColor
	);

	return true;
}

//
// CLitSmokeEmitter
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *materialName - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::Init( const char *materialName, Vector sortOrigin )
{
	PMaterialHandle	mat = GetPMaterial( materialName );
	IMaterial *pMaterial = mat;

	if ( pMaterial )
	{
		m_Renderer.Init( pMaterial );
	}

	m_Renderer.SetCommonDirLight( &g_ParticleMgr, CParticleMgr::POINTSOURCE_INDEX_TOP );

	SetSortOrigin( sortOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			color - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::SetDirectionalLight( Vector position, Vector color, float intensity )
{
	m_Renderer.DirectionalLight().m_flIntensity	= intensity;
	m_Renderer.DirectionalLight().m_vColor		= color;
	m_Renderer.DirectionalLight().m_vPos		= position;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			color - 
//			intensity - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::SetLight( int lightIndex, Vector position, Vector color, float intensity )
{
	m_Renderer.Light( lightIndex ).m_vPos			= position;
	m_Renderer.Light( lightIndex ).m_vColor			= color;
	m_Renderer.Light( lightIndex ).m_flIntensity	= intensity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flTimeDelta - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::Update( float flTimeDelta )
{
	if ( flTimeDelta > 0.0f )
	{
		//m_Renderer.DirectionalLight().m_vColor *= 0.9f;
	}

	m_Renderer.Update( &g_ParticleMgr );
	CSimpleEmitter::Update( flTimeDelta );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CLitSmokeEmitter::SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey )
{
	LitSmokeParticle *pParticle = (LitSmokeParticle *) pInParticle;

	// Should this particle die?
	pParticle->m_flLifetime += pDraw->GetTimeDelta();

	if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		return false;
	
	float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;

	// Transform.						   
	Vector tPos;

	TransformParticle( g_ParticleMgr.GetModelView(), pParticle->m_Pos, tPos );
	sortKey = tPos.z;

	pParticle->m_Pos = pParticle->m_Pos + ( pParticle->m_vecVelocity * pDraw->GetTimeDelta() );

	//float	alpha255 = (float) pParticle->m_uchColor[3] * tLifetime;

	/*
	float alpha255 = cos( -M_PI + tLifetime * M_PI * 2 ) * 0.5f + 0.5f;
	
	if( tLifetime > 0.5 )
		alpha255 *= alpha255;

	alpha255 *= 255.0f;
	*/
	
	float alpha255 = ( ( (float) pParticle->m_uchColor[3]/255.0f ) * sin( M_PI_F * tLifetime ) ) * 255.0f;

	Vector	color255 = Vector( pParticle->m_uchColor[0], pParticle->m_uchColor[1], pParticle->m_uchColor[2] ) * tLifetime;

	m_Renderer.RenderParticle_AddColor (
		pDraw,
		pParticle->m_Pos,
		tPos,
		alpha255,
		FLerp( pParticle->m_uchStartSize, pParticle->m_uchEndSize, tLifetime ),
		color255
	);

	return true;
}

