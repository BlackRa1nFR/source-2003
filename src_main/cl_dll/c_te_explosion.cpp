//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Client explosions
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tempentity.h"  // FLAGS
#include "c_te_particlesystem.h"
#include "RagdollExplosionEnumerator.h"
#include "glow_overlay.h"
#include "fx_explosion.h"

#define	CLIENT_EXPLOSION_FORCE_SCALE	500

#define	OLD_EXPLOSION	0

// Enumator class for ragdolls being affected by explosive forces
CRagdollExplosionEnumerator::CRagdollExplosionEnumerator( Vector origin, float radius, float magnitude )
{
	m_vecOrigin		= origin;
	m_flMagnitude	= magnitude;
	m_flRadius		= radius;
}

// Actual work code
IterationRetval_t CRagdollExplosionEnumerator::EnumElement( IHandleEntity *pHandleEntity )
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
	if ( pEnt == NULL )
		return ITERATION_CONTINUE;

	C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

	if ( pModel == NULL )
		return ITERATION_CONTINUE;
	
	IPhysicsObject	*pPhysicsObject = pModel->VPhysicsGetObject();
	
	if ( pPhysicsObject == NULL )
		return ITERATION_CONTINUE;

	Vector	position;

	pPhysicsObject->GetPosition( &position, NULL );

	trace_t	tr;
	UTIL_TraceLine( m_vecOrigin, position, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
		return ITERATION_CONTINUE;	

	Vector	dir		= position - m_vecOrigin;
	float	dist	= VectorNormalize( dir );
	float	force	= m_flMagnitude - ( ( m_flMagnitude / m_flRadius ) * dist );
	
	if ( force <= 0 )
		return ITERATION_CONTINUE;

	dir *= CLIENT_EXPLOSION_FORCE_SCALE * force;

	//Send the ragdoll the explosion force
	pPhysicsObject->ApplyForceCenter( dir );

	return ITERATION_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: Explosion TE
//-----------------------------------------------------------------------------
class C_TEExplosion : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TEExplosion, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

					C_TEExplosion( void );
	virtual			~C_TEExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual	bool	SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey );

public:
	void			AffectRagdolls( void );

	int				m_nModelIndex;
	float			m_fScale;
	int				m_nFrameRate;
	int				m_nFlags;
	Vector			m_vecNormal;
	char			m_chMaterialType;
	int				m_nRadius;
	int				m_nMagnitude;

	//CParticleCollision	m_ParticleCollision;
	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle		m_MaterialHandle;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEExplosion::C_TEExplosion( void )
{
	m_nModelIndex = 0;
	m_fScale = 0;
	m_nFrameRate = 0;
	m_nFlags = 0;
	m_vecNormal.Init();
	m_chMaterialType = 'C';
	m_nRadius = 0;
	m_nMagnitude = 0;

	m_pParticleMgr		= NULL;
	m_MaterialHandle	= INVALID_MATERIAL_HANDLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEExplosion::~C_TEExplosion( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEExplosion::AffectRagdolls( void )
{
	if ( ( m_nRadius == 0 ) || ( m_nMagnitude == 0 ) )
		return;

	CRagdollExplosionEnumerator	ragdollEnum( m_vecOrigin, m_nRadius, m_nMagnitude );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_RESPONSIVE_EDICTS, m_vecOrigin, m_nRadius, false, &ragdollEnum );
}

//
// CExplosionOverlay
//

bool CExplosionOverlay::Update( void )
{
	m_flLifetime += gpGlobals->frametime;
	
	static float flTotalLifetime = 0.1f;
	static Vector vColor( 0.4, 0.25, 0 );

	if ( m_flLifetime < flTotalLifetime )
	{
		float flColorScale = 1.0f - ( m_flLifetime / flTotalLifetime );

		for( int i=0; i < m_nSprites; i++ )
		{
			m_Sprites[i].m_vColor = m_vBaseColors[i] * flColorScale;
			
			m_Sprites[i].m_flHorzSize += 16.0f * gpGlobals->frametime;
			m_Sprites[i].m_flVertSize += 16.0f * gpGlobals->frametime;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	AffectRagdolls();

#if OLD_EXPLOSION

	if (m_fScale != 0)
	{
		tempents->Sprite_Explode( tempents->DefaultSprite( m_vecOrigin, m_nModelIndex, m_nFrameRate ), m_fScale, m_nFlags );

		if ( !(m_nFlags & TE_EXPLFLAG_NODLIGHTS) )
		{
			dlight_t	*dl;

			// big flash
			dl = effects->CL_AllocDlight (LIGHT_INDEX_TE_DYNAMIC);
			VectorCopy (m_vecOrigin, dl->origin);
			dl->radius = 200;
			dl->color.r = dl->color.g = 250;
			dl->color.b = 150;
			dl->die = gpGlobals->curtime + 0.01;
			dl->decay = 800;

			
			// red glow
			dl = effects->CL_AllocDlight (LIGHT_INDEX_TE_DYNAMIC+1);
			VectorCopy (m_vecOrigin, dl->origin);
			dl->radius = 150;
			dl->color.r = 255;
			dl->color.g = 190;
			dl->color.b = 40;
			dl->die = gpGlobals->curtime + 1.0;
			dl->decay = 200;
		}
	}

	FX_Explosion( m_vecOrigin, m_vecNormal, m_chMaterialType );

#else

	//Explosion overlay
#if 0
	if ( CExplosionWarpOverlay *pOverlay = new CExplosionWarpOverlay )
	{
		pOverlay->m_flLifetime	= 0;
		pOverlay->m_vPos		= m_vecOrigin;
		pOverlay->m_nSprites	= 1;
		pOverlay->m_flTotalLifetime = 0.2f;  // seconds, baby!
		pOverlay->Activate();
		bool found;
		pOverlay->m_pWarpMaterial = materials->FindMaterial( "effects/explosionwarp", &found, true );
	}
#else

	if ( !( m_nFlags & TE_EXPLFLAG_NOFIREBALL ) )
	{
		if ( CExplosionOverlay *pOverlay = new CExplosionOverlay )
		{
			pOverlay->m_flLifetime	= 0;
			pOverlay->m_vPos		= m_vecOrigin;
			pOverlay->m_nSprites	= 1;
			
			pOverlay->m_vBaseColors[0].Init( 1.0f, 0.9f, 0.7f );

			pOverlay->m_Sprites[0].m_flHorzSize = 0.05f;
			pOverlay->m_Sprites[0].m_flVertSize = pOverlay->m_Sprites[0].m_flHorzSize*0.5f;

			pOverlay->Activate();
		}
	}
#endif

	BaseExplosionEffect().Create( m_vecOrigin, m_nMagnitude, m_fScale, m_nFlags );

#endif

}

//-----------------------------------------------------------------------------
// Purpose: Simulate and render the particle in this system
// Input  : *pInParticle - particle to consider
//			*pDraw - drawing utilities
//			&sortKey - sorting key
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TEExplosion::SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey )
{
	return false;
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEExplosion, DT_TEExplosion, CTEExplosion)
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropFloat( RECVINFO(m_fScale )),
	RecvPropInt( RECVINFO(m_nFrameRate)),
	RecvPropInt( RECVINFO(m_nFlags)),
	RecvPropVector( RECVINFO(m_vecNormal)),
	RecvPropInt( RECVINFO(m_chMaterialType)),
	RecvPropInt( RECVINFO(m_nRadius)),
	RecvPropInt( RECVINFO(m_nMagnitude)),
END_RECV_TABLE()


void TE_Explosion( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float scale, int framerate, int flags, int radius, int magnitude, 
	const Vector* normal = NULL, unsigned char materialType = 'C' )
{
	// Major hack to access singleton object for doing this event (simulate receiving network message)
	__g_C_TEExplosion.m_nModelIndex = modelindex;
	__g_C_TEExplosion.m_fScale = scale;
	__g_C_TEExplosion.m_nFrameRate = framerate;
	__g_C_TEExplosion.m_nFlags = flags;
	__g_C_TEExplosion.m_vecNormal = *normal;
	__g_C_TEExplosion.m_chMaterialType = materialType;
	__g_C_TEExplosion.m_nRadius = radius;
	__g_C_TEExplosion.m_nMagnitude = magnitude;

	__g_C_TEExplosion.PostDataUpdate( DATA_UPDATE_CREATED );

}