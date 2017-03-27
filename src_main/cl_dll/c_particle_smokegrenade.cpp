//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_smoke_trail.h"
#include "smoke_fog_overlay.h"
#include "engine/IEngineTrace.h"
#include "view.h"

// ------------------------------------------------------------------------- //
// Definitions
// ------------------------------------------------------------------------- //

#define ROTATION_SPEED				0.6
#define TRADE_DURATION_MIN			5
#define TRADE_DURATION_MAX			10
#define SMOKEGRENADE_PARTICLERADIUS	55

#define SMOKESPHERE_EXPAND_TIME		5.5		// Take N seconds to expand to SMOKESPHERE_MAX_RADIUS.
#define SMOKESPHERE_MAX_RADIUS		1000 //400

#define NUM_PARTICLES_PER_DIMENSION	6
#define SMOKEPARTICLE_OVERLAP		0

#define SMOKEPARTICLE_SIZE			55
#define NUM_MATERIAL_HANDLES		1


static Vector s_FadePlaneDirections[] =
{
	Vector( 1,0,0),
	Vector(-1,0,0),
	Vector(0, 1,0),
	Vector(0,-1,0),
	Vector(0,0, 1),
	Vector(0,0,-1)
};
#define NUM_FADE_PLANES	(sizeof(s_FadePlaneDirections)/sizeof(s_FadePlaneDirections[0]))


// ------------------------------------------------------------------------- //
// Classes
// ------------------------------------------------------------------------- //
class C_ParticleSmokeGrenade : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_ParticleSmokeGrenade, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();

					C_ParticleSmokeGrenade();
					~C_ParticleSmokeGrenade();

private:
	
	class SmokeGrenadeParticle : public Particle
	{
	public:
		float				m_RotationSpeed;
		float				m_CurRotation;
		float				m_FadeAlpha;		// Set as it moves around.
		unsigned char		m_ColorInterp;		// Amount between min and max colors.
		unsigned char		m_Color[4];
	};


public:

	// Optional call. It will use defaults if you don't call this.
	void			SetParams(
		);

	// Call this to move the source..
	void			SetPos(const Vector &pos);


// C_BaseEntity.
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw();



// IPrototypeAppEffect.
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);


// IParticleEffect.
public:
	virtual void	Update(float fTimeDelta);
	virtual bool	SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortKey);
	virtual void	NotifyRemove();


// Proxies.
public:

	static void		RecvProxy_CurrentStage(  const CRecvProxyData *pData, void *pStruct, void *pOut );


private:

	// The SmokeEmitter represents a grid in 3D space.
	class SmokeParticleInfo
	{
	public:
		SmokeGrenadeParticle	*m_pParticle;
		int						m_TradeIndex;		// -1 if not exchanging yet.
		float					m_TradeClock;		// How long since they started trading.
		float					m_TradeDuration;	// How long the trade will take to finish.
		float					m_FadeAlpha;		// Calculated from nearby world geometry.
		unsigned char			m_Color[4];
	};

	inline int					GetSmokeParticleIndex(int x, int y, int z)	{return z*m_xCount*m_yCount+y*m_yCount+x;}
	inline SmokeParticleInfo*	GetSmokeParticleInfo(int x, int y, int z)	{return &m_SmokeParticleInfos[GetSmokeParticleIndex(x,y,z)];}
	inline void					GetParticleInfoXYZ(int index, int &x, int &y, int &z)
	{
		z = index / (m_xCount*m_yCount);
		int zIndex = z*m_xCount*m_yCount;
		y = (index - zIndex) / m_yCount;
		int yIndex = y*m_yCount;
		x = index - zIndex - yIndex;
	}

	inline bool					IsValidXYZCoords(int x, int y, int z)
	{
		return x >= 0 && y >= 0 && z >= 0 && x < m_xCount && y < m_yCount && z < m_zCount;
	}

	inline Vector				GetSmokeParticlePos(int x, int y, int z)	
	{
		return m_SmokeBasePos + 
			Vector( ((float)x / (m_xCount-1)) * m_SpacingRadius * 2 - m_SpacingRadius,
				((float)y / (m_yCount-1)) * m_SpacingRadius * 2 - m_SpacingRadius,
				((float)z / (m_zCount-1)) * m_SpacingRadius * 2 - m_SpacingRadius);
	}

	inline Vector				GetSmokeParticlePosIndex(int index)
	{
		int x, y, z;
		GetParticleInfoXYZ(index, x, y, z);
		return GetSmokeParticlePos(x, y, z);
	}

	inline const Vector&		GetPos()	{ return GetAbsOrigin(); }

	// Start filling the smoke volume (and stop the smoke trail).
	void						FillVolume();


// State variables from server.
public:
	
	unsigned char		m_CurrentStage;
	Vector				m_SmokeBasePos;

	// It will fade out during this time.
	float				m_FadeStartTime;
	float				m_FadeEndTime;
	float				m_FadeAlpha;	// Calculated from the fade start/end times each frame.


private:
						C_ParticleSmokeGrenade( const C_ParticleSmokeGrenade & );

	bool				m_bStarted;

	PMaterialHandle		m_MaterialHandles[NUM_MATERIAL_HANDLES];

	SmokeParticleInfo	m_SmokeParticleInfos[NUM_PARTICLES_PER_DIMENSION*NUM_PARTICLES_PER_DIMENSION*NUM_PARTICLES_PER_DIMENSION];
	int					m_xCount, m_yCount, m_zCount;
	float				m_SpacingRadius;

	Vector				m_MinColor;
	Vector				m_MaxColor;

	float				m_ExpandTimeCounter;	// How long since we started expanding.	
	float				m_ExpandRadius;			// How large is our radius.

	// How long the effect has been around.
	float				m_LifetimeCounter;

	C_SmokeTrail		m_SmokeTrail;
};


// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(SmokeGrenade, C_ParticleSmokeGrenade);


// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_ParticleSmokeGrenade, DT_ParticleSmokeGrenade, ParticleSmokeGrenade)
	RecvPropFloat(RECVINFO(m_FadeStartTime)),
	RecvPropFloat(RECVINFO(m_FadeEndTime)),
	RecvPropInt(RECVINFO(m_CurrentStage), 0, &C_ParticleSmokeGrenade::RecvProxy_CurrentStage),
END_RECV_TABLE()


// ------------------------------------------------------------------------- //
// Helpers.
// ------------------------------------------------------------------------- //

static inline void InterpColor(unsigned char dest[4], unsigned char src1[4], unsigned char src2[4], float percent)
{
	dest[0] = (unsigned char)(src1[0] + (src2[0] - src1[0]) * percent);
	dest[1] = (unsigned char)(src1[1] + (src2[1] - src1[1]) * percent);
	dest[2] = (unsigned char)(src1[2] + (src2[2] - src1[2]) * percent);
}


static inline int GetWorldPointContents(const Vector &vPos)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return 0;
	#else
		return enginetrace->GetPointContents( vPos );
	#endif
}

static inline void WorldTraceLine( const Vector &start, const Vector &end, int contentsMask, trace_t *trace )
{
	#if defined(PARTICLEPROTOTYPE_APP)
		trace->fraction = 1;
	#else
		UTIL_TraceLine(start, end, contentsMask, NULL, COLLISION_GROUP_NONE, trace);
	#endif
}

static inline Vector EngineGetLightForPoint(const Vector &vPos)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return Vector(1,1,1);
	#else
		return engine->GetLightForPoint(vPos, true);
	#endif
}

static inline const Vector& EngineGetVecRenderOrigin()
{
	#if defined(PARTICLEPROTOTYPE_APP)
		static Vector dummy(0,0,0);
		return dummy;
	#else
		return CurrentViewOrigin();
	#endif
}

static inline float& EngineGetSmokeFogOverlayAlpha()
{
	#if defined(PARTICLEPROTOTYPE_APP)
		static float dummy;
		return dummy;
	#else
		return g_SmokeFogOverlayAlpha;
	#endif
}

static inline C_BaseEntity* ParticleGetEntity(int index)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return NULL;
	#else
		return cl_entitylist->GetEnt(index);
	#endif
}



// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_ParticleSmokeGrenade::C_ParticleSmokeGrenade()
{
	memset(m_MaterialHandles, 0, sizeof(m_MaterialHandles));

	m_MinColor.Init(0.5, 0.5, 0.5);
	m_MaxColor.Init(0.6, 0.6, 0.6 );

	m_ExpandRadius = 0;
	m_ExpandTimeCounter = 0;
	m_FadeStartTime = 25;
	m_FadeEndTime = 30;
	m_LifetimeCounter = 0;

	m_CurrentStage = 0;

	m_bStarted = false;
}


C_ParticleSmokeGrenade::~C_ParticleSmokeGrenade()
{
	g_ParticleMgr.RemoveEffect( &m_ParticleEffect );
}


void C_ParticleSmokeGrenade::SetParams(
	)
{
}


void C_ParticleSmokeGrenade::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED )
	{
		Start(&g_ParticleMgr, NULL);
	}
}


bool C_ParticleSmokeGrenade::ShouldDraw()
{
	return true;
}


void C_ParticleSmokeGrenade::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_LifetimeCounter = 0;
	m_SmokeTrail.Start(pParticleMgr, pArgs);

	m_SmokeTrail.m_ParticleLifetime = 0.5;
	m_SmokeTrail.SetSpawnRate(40);
	m_SmokeTrail.m_MinSpeed = 0;
	m_SmokeTrail.m_MaxSpeed = 0;
	m_SmokeTrail.m_StartSize = 3;
	m_SmokeTrail.m_EndSize = 10;
	m_SmokeTrail.m_SpawnRadius = 0;

	m_SmokeTrail.SetLocalOrigin( GetAbsOrigin() );

	for(int i=0; i < NUM_MATERIAL_HANDLES; i++)
	{
		char str[256];
		Q_snprintf(str, sizeof( str ), "particle/particle_smokegrenade%d", i+1);
		m_MaterialHandles[i] = m_ParticleEffect.FindOrAddMaterial(str);
	}

	if( m_CurrentStage == 2 )
	{
		FillVolume();
	}

	// Go straight into "fill volume" mode if they want.
	if(pArgs)
	{
		if(pArgs->FindArg("-FillVolume"))
		{
			FillVolume();
		}
	}

	m_bStarted = true;
}


void C_ParticleSmokeGrenade::Update(float fTimeDelta)
{
	m_LifetimeCounter += fTimeDelta;
	
	// Update the smoke trail.
	C_BaseEntity *pAimEnt = GetFollowedEntity();
	if ( pAimEnt )
	{
		Vector forward, right, up;

		// Update the smoke particle color.
		if(m_CurrentStage == 0)
		{
			m_SmokeTrail.m_StartColor = EngineGetLightForPoint(GetAbsOrigin()) * 0.5f;
			m_SmokeTrail.m_EndColor = m_SmokeTrail.m_StartColor;
		}

		// Spin the smoke trail.
		AngleVectors(pAimEnt->GetAbsAngles(), &forward, &right, &up);
		m_SmokeTrail.m_VelocityOffset = forward * 30 + GetAbsVelocity();

		m_SmokeTrail.SetLocalOrigin( GetAbsOrigin() );
		m_SmokeTrail.Update(fTimeDelta);
	}	
	
	
	// Update our fade alpha.
	if(m_LifetimeCounter < m_FadeStartTime)
	{
		m_FadeAlpha = 1;
	}
	else if(m_LifetimeCounter < m_FadeEndTime)
	{
		float fadePercent = (m_LifetimeCounter - m_FadeStartTime) / (m_FadeEndTime - m_FadeStartTime);
		m_FadeAlpha = cos(fadePercent * 3.14159) * 0.5 + 0.5;
	}
	else
	{
		m_FadeAlpha = 0;
	}

	// Scale by the amount the sphere has grown.
	m_FadeAlpha *= m_ExpandRadius / SMOKESPHERE_MAX_RADIUS;

	if(m_CurrentStage == 1)
	{
		// Update the expanding sphere.
		m_ExpandTimeCounter += fTimeDelta;
		if(m_ExpandTimeCounter > SMOKESPHERE_EXPAND_TIME)
			m_ExpandTimeCounter = SMOKESPHERE_EXPAND_TIME;

		m_ExpandRadius = SMOKESPHERE_MAX_RADIUS * (float)sin(m_ExpandTimeCounter * 3.14159265358 * 0.5 / SMOKESPHERE_EXPAND_TIME);

		// Add our influence to the global smoke fog alpha.
		float testDist = (EngineGetVecRenderOrigin() - m_SmokeBasePos).Length();
		float fadeEnd = m_ExpandRadius * 0.75;
		if(testDist < fadeEnd)
		{
			EngineGetSmokeFogOverlayAlpha() += 1 - testDist / fadeEnd;
		}	


		// This is used to randomize the direction it chooses to move a particle in.
		int offsetLookup[3] = {-1,0,1};

		// Update all the moving traders and establish new ones.
		int nTotal = m_xCount * m_yCount * m_zCount;
		for(int i=0; i < nTotal; i++)
		{
			SmokeParticleInfo *pInfo = &m_SmokeParticleInfos[i];

			if(!pInfo->m_pParticle)
				continue;
		
			if(pInfo->m_TradeIndex == -1)
			{
				pInfo->m_pParticle->m_FadeAlpha = pInfo->m_FadeAlpha;
				pInfo->m_pParticle->m_Color[0] = pInfo->m_Color[0];
				pInfo->m_pParticle->m_Color[1] = pInfo->m_Color[1];
				pInfo->m_pParticle->m_Color[2] = pInfo->m_Color[2];

				// Is there an adjacent one that's not trading?
				int x, y, z;
				GetParticleInfoXYZ(i, x, y, z);

				int xCountOffset = rand();
				int yCountOffset = rand();
				int zCountOffset = rand();

				bool bFound = false;
				for(int xCount=0; xCount < 3 && !bFound; xCount++)
				{
					for(int yCount=0; yCount < 3 && !bFound; yCount++)
					{
						for(int zCount=0; zCount < 3; zCount++)
						{
							int testX = x + offsetLookup[(xCount+xCountOffset) % 3];
							int testY = y + offsetLookup[(yCount+yCountOffset) % 3];
							int testZ = z + offsetLookup[(zCount+zCountOffset) % 3];

							if(testX == x && testY == y && testZ == z)
								continue;

							if(IsValidXYZCoords(testX, testY, testZ))
							{
								SmokeParticleInfo *pOther = GetSmokeParticleInfo(testX, testY, testZ);
								if(pOther->m_pParticle && pOther->m_TradeIndex == -1)
								{
									// Ok, this one is looking to trade also.
									pInfo->m_TradeIndex = GetSmokeParticleIndex(testX, testY, testZ);
									pOther->m_TradeIndex = i;
									pInfo->m_TradeClock = pOther->m_TradeClock = 0;
									pInfo->m_TradeDuration = FRand(TRADE_DURATION_MIN, TRADE_DURATION_MAX);
									
									bFound = true;
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				SmokeParticleInfo *pOther = &m_SmokeParticleInfos[pInfo->m_TradeIndex];
				assert(pOther->m_TradeIndex == i);
				
				// This makes sure the trade only gets updated once per frame.
				if(pInfo < pOther)
				{
					// Increment the trade clock..
					pInfo->m_TradeClock = (pOther->m_TradeClock += fTimeDelta);
					int x, y, z;
					GetParticleInfoXYZ(i, x, y, z);
					Vector myPos = GetSmokeParticlePos(x, y, z);
					
					int otherX, otherY, otherZ;
					GetParticleInfoXYZ(pInfo->m_TradeIndex, otherX, otherY, otherZ);
					Vector otherPos = GetSmokeParticlePos(otherX, otherY, otherZ);

					// Is the trade finished?
					if(pInfo->m_TradeClock >= pInfo->m_TradeDuration)
					{
						pInfo->m_TradeIndex = pOther->m_TradeIndex = -1;
						
						pInfo->m_pParticle->m_Pos = otherPos;
						pOther->m_pParticle->m_Pos = myPos;

						SmokeGrenadeParticle *temp = pInfo->m_pParticle;
						pInfo->m_pParticle = pOther->m_pParticle;
						pOther->m_pParticle = temp;
					}
					else
					{			
						// Ok, move them closer.
						float percent = (float)cos(pInfo->m_TradeClock * 2 * 1.57079632f / pInfo->m_TradeDuration);
						percent = percent * 0.5 + 0.5;
						
						pInfo->m_pParticle->m_FadeAlpha  = pInfo->m_FadeAlpha + (pOther->m_FadeAlpha - pInfo->m_FadeAlpha) * (1 - percent);
						pOther->m_pParticle->m_FadeAlpha = pInfo->m_FadeAlpha + (pOther->m_FadeAlpha - pInfo->m_FadeAlpha) * percent;

						InterpColor(pInfo->m_pParticle->m_Color,  pInfo->m_Color, pOther->m_Color, 1-percent);
						InterpColor(pOther->m_pParticle->m_Color, pInfo->m_Color, pOther->m_Color, percent);

						pInfo->m_pParticle->m_Pos  = myPos + (otherPos - myPos) * (1 - percent);
						pOther->m_pParticle->m_Pos = myPos + (otherPos - myPos) * percent;
					}
				}
			}
		}
	}
}


bool C_ParticleSmokeGrenade::SimulateAndRender(Particle *pBaseParticle, ParticleDraw *pDraw, float &sortKey)
{
	SmokeGrenadeParticle* pParticle = (SmokeGrenadeParticle*)pBaseParticle;

	// Draw.
	float len = (pParticle->m_Pos - m_SmokeBasePos).Length();
	if(len > m_ExpandRadius)
	{
		Vector vTemp;
		TransformParticle(g_ParticleMgr.GetModelView(), pParticle->m_Pos, vTemp);
		sortKey = vTemp.z;		
		return true;
	}

	// This smooths out the growing sphere. Rather than having particles appear in one spot as the sphere
	// expands, they stay at the borders.
	Vector renderPos;
	if(len > m_ExpandRadius * 0.5f)
	{
		renderPos = m_SmokeBasePos + ((pParticle->m_Pos - m_SmokeBasePos) * (m_ExpandRadius * 0.5f)) / len;
	}
	else
	{
		renderPos = pParticle->m_Pos;
	}		

	// Figure out the alpha based on where it is in the sphere.
	float alpha = 1 - len / m_ExpandRadius;
	
	// This changes the ramp to be very solid in the core, then taper off.
	static float testCutoff=0.7;
	if(alpha > testCutoff)
	{
		alpha = 1;
	}
	else
	{
		// at testCutoff it's 1, at 0, it's 0
		alpha = alpha / testCutoff;
	}

	// Fade out globally.
	alpha *= m_FadeAlpha;


	pParticle->m_CurRotation += pParticle->m_RotationSpeed * pDraw->GetTimeDelta();

	// Apply the precalculated fade alpha from world geometry.
	alpha *= pParticle->m_FadeAlpha;
	
	// TODO: optimize this whole routine!
	Vector color = m_MinColor + (m_MaxColor - m_MinColor) * (pParticle->m_ColorInterp / 255.1f);
	color.x *= pParticle->m_Color[0] / 255.0f;
	color.y *= pParticle->m_Color[1] / 255.0f;
	color.z *= pParticle->m_Color[2] / 255.0f;
	
	Vector tRenderPos;
	TransformParticle(g_ParticleMgr.GetModelView(), renderPos, tRenderPos);
	sortKey = tRenderPos.z;

	RenderParticle_ColorSizeAngle(
		pDraw,
		tRenderPos,
		color,
		alpha * GetAlphaDistanceFade(tRenderPos, 10, 30),	// Alpha
		SMOKEPARTICLE_SIZE,
		pParticle->m_CurRotation
		);

	return true;
}


void C_ParticleSmokeGrenade::NotifyRemove()
{
	m_xCount = m_yCount = m_zCount = 0;
}


void C_ParticleSmokeGrenade::RecvProxy_CurrentStage(  const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_ParticleSmokeGrenade *pGrenade = (C_ParticleSmokeGrenade*)pStruct;
	Assert( pOut == &pGrenade->m_CurrentStage );

	if ( pGrenade && pGrenade->m_CurrentStage == 0 && pData->m_Value.m_Int == 1 )
	{
		if( pGrenade->m_bStarted )
			pGrenade->FillVolume();
		else
			pGrenade->m_CurrentStage = 2;
	}
}


void C_ParticleSmokeGrenade::FillVolume()
{
	m_CurrentStage = 1;
	m_SmokeBasePos = GetPos();
	m_SmokeTrail.SetEmit(false);
	m_ExpandTimeCounter = m_ExpandRadius = 0;

	// Spawn all of our particles.
	float overlap = SMOKEPARTICLE_OVERLAP;

	m_SpacingRadius = (SMOKEGRENADE_PARTICLERADIUS - overlap) * NUM_PARTICLES_PER_DIMENSION * 0.5f;
	m_xCount = m_yCount = m_zCount = NUM_PARTICLES_PER_DIMENSION;

	float invNumPerDimX = 1.0f / (m_xCount-1);
	float invNumPerDimY = 1.0f / (m_yCount-1);
	float invNumPerDimZ = 1.0f / (m_zCount-1);

	Vector vPos;
	Vector basePos = m_SmokeBasePos;
	for(int x=0; x < m_xCount; x++)
	{
		vPos.x = basePos.x + ((float)x * invNumPerDimX) * m_SpacingRadius * 2 - m_SpacingRadius;

		for(int y=0; y < m_yCount; y++)
		{
			vPos.y = basePos.y + ((float)y * invNumPerDimY) * m_SpacingRadius * 2 - m_SpacingRadius;
							  
			for(int z=0; z < m_zCount; z++)
			{
				vPos.z = basePos.z + ((float)z * invNumPerDimZ) * m_SpacingRadius * 2 - m_SpacingRadius;

				if(SmokeParticleInfo *pInfo = GetSmokeParticleInfo(x,y,z))
				{
					int contents = GetWorldPointContents(vPos);
					if(contents & CONTENTS_SOLID)
					{
						pInfo->m_pParticle = NULL;
					}
					else
					{
						SmokeGrenadeParticle *pParticle = 
							(SmokeGrenadeParticle*)m_ParticleEffect.AddParticle(sizeof(SmokeGrenadeParticle), m_MaterialHandles[rand() % NUM_MATERIAL_HANDLES]);

						if(pParticle)
						{
							pParticle->m_Pos = vPos;
							pParticle->m_ColorInterp = (unsigned char)((rand() * 255) / RAND_MAX);
							pParticle->m_RotationSpeed = FRand(-ROTATION_SPEED, ROTATION_SPEED); // Rotation speed.
							pParticle->m_CurRotation = FRand(-6, 6);
						}

						#ifdef _DEBUG
							int testX, testY, testZ;
							int index = GetSmokeParticleIndex(x,y,z);
							GetParticleInfoXYZ(index, testX, testY, testZ);
							assert(testX == x && testY == y && testZ == z);
						#endif

						Vector vColor = EngineGetLightForPoint(vPos);
						pInfo->m_Color[0] = (unsigned char)(vColor.x * 255.9f);
						pInfo->m_Color[1] = (unsigned char)(vColor.y * 255.9f);
						pInfo->m_Color[2] = (unsigned char)(vColor.z * 255.9f);

						// Cast some rays and if it's too close to anything, fade its alpha down.
						pInfo->m_FadeAlpha = 1;

						for(int i=0; i < NUM_FADE_PLANES; i++)
						{
							trace_t trace;
							WorldTraceLine(vPos, vPos + s_FadePlaneDirections[i] * 100, MASK_SOLID_BRUSHONLY, &trace);
							if(trace.fraction < 1.0f)
							{
								float dist = DotProduct(trace.plane.normal, vPos) - trace.plane.dist;
								if(dist < 0)
								{
									pInfo->m_FadeAlpha = 0;
								}
								else if(dist < SMOKEPARTICLE_SIZE)
								{
									float alphaScale = dist / SMOKEPARTICLE_SIZE;
									alphaScale *= alphaScale * alphaScale;
									pInfo->m_FadeAlpha *= alphaScale;
								}
							}
						}

						pInfo->m_pParticle = pParticle;
						pInfo->m_TradeIndex = -1;
					}
				}
			}
		}
	}
}





