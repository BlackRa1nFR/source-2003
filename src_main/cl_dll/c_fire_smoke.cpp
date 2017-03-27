//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "IViewRender.h"
#include "particles_simple.h"
#include "tempent.h"
#include "glow_overlay.h"
#include "ClientEffectPrecacheSystem.h"
#include "studio.h"
#include "bone_setup.h"
#include "engine/ivmodelinfo.h"
#include "view.h"


#define	PARTICLE_FIRE 0

CLIENTEFFECT_REGISTER_BEGIN( SmokeStackMaterials )
	CLIENTEFFECT_MATERIAL( "particle/SmokeStack" )
	CLIENTEFFECT_MATERIAL( "particle/fire" )
	CLIENTEFFECT_MATERIAL( "sprites/flamelet1" )
	CLIENTEFFECT_MATERIAL( "sprites/flamelet2" )
	CLIENTEFFECT_MATERIAL( "sprites/flamelet3" )
	CLIENTEFFECT_MATERIAL( "sprites/flamelet4" )
	CLIENTEFFECT_MATERIAL( "sprites/flamelet5" )
	CLIENTEFFECT_MATERIAL( "sprites/fire1" )
CLIENTEFFECT_REGISTER_END()


#define	NUM_CHILD_FLAMES	8

#define	SMOKE_RISE_RATE		92.0f
#define	SMOKE_LIFETIME		2.0f
#define	EMBER_LIFETIME		2.0f

#define	FLAME_CHILD_SPREAD	64.0f
#define	FLAME_SOURCE_HEIGHT	128.0f

class CFireOverlay;

//==================================================
// C_FireSmoke
//==================================================

//NOTENOTE: Mirrored in dlls/fire_smoke.h
#define	bitsFIRESMOKE_NONE				0x00000000
#define	bitsFIRESMOKE_ACTIVE			0x00000001
#define	bitsFIRESMOKE_SMOKE				0x00000002
#define	bitsFIRESMOKE_SMOKE_COLLISION	0x00000004
#define	bitsFIRESMOKE_GLOW				0x00000008


class C_FireSprite : public C_Sprite
{
	DECLARE_CLASS( C_FireSprite, C_Sprite );

public:
	Vector	m_vecMoveDir;
};


class C_FireSmoke : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_FireSmoke, C_BaseEntity );

	C_FireSmoke();
	~C_FireSmoke();

	void	Start( void );
	void	Simulate( void );

	void	StartClientOnly( void );
	void	RemoveClientOnly( void );

protected:
	void	Update( void );
	void	UpdateAnimation( void );
	void	UpdateScale( void );
	void	UpdateFlames( void );
	void	AddFlames( void );
	void	SpawnSmoke( void );
	void	FindClipPlane( void );
	
//C_BaseEntity
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw();

	float GetScale( void ) const { return m_flScaleRegister;	}
	
//From the server
public:
	float	m_flStartScale;
	float	m_flScale;
	float	m_flScaleTime;
	int		m_nFlags;
	int		m_nFlameModelIndex;

//Client-side only
public:
	float	m_flScaleRegister;
	float	m_flScaleStart;
	float	m_flScaleEnd;
	float	m_flScaleTimeStart;
	float	m_flScaleTimeEnd;
	float	m_flChildFlameSpread;

	VPlane	m_planeClip;
	float	m_flClipPerc;
	bool	m_bClipTested;

protected:

	void	UpdateEffects( void );

	CSmartPtr<CEmberEffect> m_pEmberEmitter;
	CSmartPtr<CLitSmokeEmitter> m_pSmokeEmitter;

	C_FireSprite		m_entFlames[NUM_CHILD_FLAMES];
	float				m_entFlameScales[NUM_CHILD_FLAMES];

	TimedEvent			m_tParticleSpawn;

	CFireOverlay		*m_pFireOverlay;

private:
	C_FireSmoke( const C_FireSmoke & );
};

#define	OVERLAY_MAX_VISIBLE_RANGE	512.0f

//Fire overlay
class CFireOverlay : public CGlowOverlay
{
public:
	
	//Constructor
	CFireOverlay( C_FireSmoke *owner )
	{
		m_pOwner	= owner;
		m_flScale	= 0.0f;
		m_nGUID		= random->RandomInt( -999999, 999999 );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Generate a flicker value
	// Output : scalar value
	//-----------------------------------------------------------------------------
	float GetFlickerScale( void )
	{
		float	result = 0.0f;

		float	time = Helper_GetTime() + m_nGUID;

		result = sin( time * 1000.0f );
		result += 0.5f * sin( time * 2000.0f );
		result -= 0.5f * cos( time * 8000.0f );
		
		return result;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Update the overlay
	//-----------------------------------------------------------------------------
	virtual bool Update( void )
	{
		if ( m_pOwner == NULL )
			return false;

		float scale	 = m_pOwner->GetScale();
		float dscale = scale - m_flScale;

		m_vPos[2] += dscale * FLAME_SOURCE_HEIGHT;
		m_flScale = scale;

		scale *= 0.75f;

		float flickerScale = GetFlickerScale();

		float newScale = scale + ( scale * flickerScale * 0.1f );

		m_Sprites[0].m_flHorzSize = ( newScale * 0.2f ) + ( m_Sprites[0].m_flHorzSize * 0.8f );
		m_Sprites[0].m_flVertSize = m_Sprites[0].m_flHorzSize * 1.5f;
		
		float	cameraDistance = ( CurrentViewOrigin() - (m_pOwner->GetAbsOrigin())).Length();

		if ( cameraDistance > OVERLAY_MAX_VISIBLE_RANGE )
			cameraDistance = OVERLAY_MAX_VISIBLE_RANGE;

		float alpha = 1.0f - ( cameraDistance / OVERLAY_MAX_VISIBLE_RANGE );

		Vector	newColor = m_vBaseColors[0] + ( m_vBaseColors[0] * flickerScale * 0.5f );
		m_Sprites[0].m_vColor = ( newColor * 0.1f ) + ( m_Sprites[0].m_vColor * 0.9f ) * alpha;

		return true;
	}

public:

	C_FireSmoke	*m_pOwner;
	Vector		m_vBaseColors[CGlowOverlay::MAX_SPRITES];
	float		m_flScale;
	int			m_nGUID;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRecvProp - 
//			*pStruct - 
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_Scale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_FireSmoke	*pFireSmoke	= (C_FireSmoke	*) pStruct;
	float scale				= pData->m_Value.m_Float;

	//If changed, update our internal information
	if ( ( pFireSmoke->m_flScale != scale ) && ( pFireSmoke->m_flScaleEnd != scale ) )
	{
		pFireSmoke->m_flScaleStart		= pFireSmoke->m_flScaleRegister;
		pFireSmoke->m_flScaleEnd		= scale;			
	}

	pFireSmoke->m_flScale = scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRecvProp - 
//			*pStruct - 
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_ScaleTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_FireSmoke	*pFireSmoke	= (C_FireSmoke	*) pStruct;
	float time				= pData->m_Value.m_Float;

	//If changed, update our internal information
	//if ( pFireSmoke->m_flScaleTime != time )
	{
		if ( time == -1.0f )
		{
			pFireSmoke->m_flScaleTimeStart	= Helper_GetTime()-1.0f;
			pFireSmoke->m_flScaleTimeEnd	= pFireSmoke->m_flScaleTimeStart;
		}
		else
		{
			pFireSmoke->m_flScaleTimeStart	= Helper_GetTime();
			pFireSmoke->m_flScaleTimeEnd	= Helper_GetTime() + time;
		}
	}

	pFireSmoke->m_flScaleTime = time;
}

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_FireSmoke, DT_FireSmoke, CFireSmoke )
	RecvPropFloat( RECVINFO( m_flStartScale )),
	RecvPropFloat( RECVINFO( m_flScale ), 0, RecvProxy_Scale ),
	RecvPropFloat( RECVINFO( m_flScaleTime ), 0, RecvProxy_ScaleTime ),
	RecvPropInt( RECVINFO( m_nFlags ) ),
	RecvPropInt( RECVINFO( m_nFlameModelIndex ) ),
END_RECV_TABLE()

//==================================================
// C_FireSmoke
//==================================================

C_FireSmoke::C_FireSmoke()
{
	//Server-side
	m_flStartScale		= 0.0f;
	m_flScale			= 0.0f;
	m_flScaleTime		= 0.0f;
	m_nFlags			= bitsFIRESMOKE_NONE;
	m_nFlameModelIndex	= 0;

	//Client-side
	m_flScaleRegister	= 0.0f;
	m_flScaleStart		= 0.0f;
	m_flScaleEnd		= 0.0f;
	m_flScaleTimeStart	= 0.0f;
	m_flScaleTimeEnd	= 0.0f;
	m_bClipTested		= false;

	m_flChildFlameSpread = FLAME_CHILD_SPREAD;
	
	//Clear all child flames
	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		m_entFlames[i].Clear();
	}

	m_pEmberEmitter = CEmberEffect::Create( "C_FireSmoke::m_pEmberEmitter" );
	m_pSmokeEmitter = CLitSmokeEmitter::Create( "C_FireSmoke::m_pSmokeEmitter" );
}

C_FireSmoke::~C_FireSmoke()
{
	if ( m_pFireOverlay != NULL )
	{
		delete m_pFireOverlay;
		m_pFireOverlay = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::Simulate( void )
{
	//Only do this if we're active
	if (( m_nFlags & bitsFIRESMOKE_ACTIVE ) == false )
		return;
	
	Update();
	AddFlames();
}

#define	FLAME_ALPHA_START	0.9f
#define FLAME_ALPHA_END		1.0f

#define	FLAME_TRANS_START	0.75f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::AddFlames( void )
{
#if PARTICLE_FIRE

	if ( ( gpGlobals->frametime != 0.0f ) && ( m_flScaleRegister > 0.0f ) )
	{

		Vector	offset;
		float	scalar;

		scalar = 32.0f*m_flScaleRegister;
		offset[0] = Helper_RandomFloat( -scalar, scalar );
		offset[1] = Helper_RandomFloat( -scalar, scalar );
		offset[2] = 0.0f;

		CSmartPtr<CSimpleEmitter> pEmitter = CSimpleEmitter::Create( "C_FireSmoke" );
		pEmitter->SetSortOrigin( GetAbsOrigin()+offset );

		SimpleParticle *sParticle;

		//for ( int i = 0; i < 1; i++ )
		{
			scalar = 32.0f*m_flScaleRegister;
			offset[0] = Helper_RandomFloat( -scalar, scalar );
			offset[1] = Helper_RandomFloat( -scalar, scalar );
			offset[2] = 12.0f*m_flScaleRegister;

			sParticle = (SimpleParticle *) pEmitter->AddParticle( sizeof(SimpleParticle), pEmitter->GetPMaterial( VarArgs("sprites/flamelet%d", Helper_RandomInt( 1, 5 ) ) ), GetAbsOrigin()+offset );

			if ( sParticle )
			{
				sParticle->m_flLifetime		= 0.0f;
				sParticle->m_flDieTime		= 0.25f;

				sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
				sParticle->m_flRollDelta	= Helper_RandomFloat( -4.0f, 4.0f );

				float alpha = Helper_RandomInt( 128, 255 );

				sParticle->m_uchColor[0]	= alpha;
				sParticle->m_uchColor[1]	= alpha;
				sParticle->m_uchColor[2]	= alpha;
				sParticle->m_uchStartAlpha	= 255;
				sParticle->m_uchEndAlpha	= 0;
				sParticle->m_uchStartSize	= 64.0f*m_flScaleRegister;
				sParticle->m_uchEndSize		= 0;
				
				float speedScale = ((GetAbsOrigin()+offset)-GetAbsOrigin()).Length2D() / (32.0f*m_flScaleRegister);
				sParticle->m_vecVelocity	= Vector( Helper_RandomFloat( -32.0f, 32.0f ), Helper_RandomFloat( -32.0f, 32.0f ), Helper_RandomFloat( 32.0f, 128.0f )*speedScale );
			}
		}

		pEmitter->Release();
	}

#endif

//#if !PARTICLE_FIRE

	float alpha	= 1.0f;

	//Update the child flame alpha
	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		if ( m_entFlames[i].GetScale() > 0.0f )
		{
			m_entFlames[i].SetRenderColor( ( 255.0f * alpha ), ( 255.0f * alpha ), ( 255.0f * alpha ) );
			m_entFlames[i].SetBrightness( 255.0f * alpha );

			view->AddVisibleEntity( &m_entFlames[i] );
		}
	}

//#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_FireSmoke::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdateEffects();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateEffects( void )
{
	if ( m_pEmberEmitter.IsValid() )
	{
		m_pEmberEmitter->SetSortOrigin( GetAbsOrigin() );
	}

	if ( m_pSmokeEmitter.IsValid() )
	{
		m_pSmokeEmitter->SetSortOrigin( GetAbsOrigin() );
	}

	if ( m_pFireOverlay != NULL )
	{
		m_pFireOverlay->m_vPos = GetAbsOrigin();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_FireSmoke::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::Start( void )
{
	// Setup the render handles for stuff we want in the client leaf list.
	int i;
	for ( i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		m_entFlames[i].SetupEntityRenderHandle( RENDER_GROUP_TRANSLUCENT_ENTITY );
	}

	//Various setup info
	m_tParticleSpawn.Init( 10.0f );

	QAngle	offset;
	model_t	*pModel; 
	int		maxFrames;

	//Setup the child flames
	for ( i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		//Setup our offset angles
		offset[0] = 0.0f;
		offset[1] = Helper_RandomFloat( 0, 360 );
		offset[2] = 0.0f;

  		AngleVectors( offset, &m_entFlames[i].m_vecMoveDir );
		
		pModel		= (model_t *) modelinfo->GetModel( m_nFlameModelIndex );
		maxFrames	= modelinfo->GetModelFrameCount( pModel );

		//Setup all the information for the client entity
		m_entFlames[i].SetModelByIndex( m_nFlameModelIndex );
		m_entFlames[i].SetLocalOrigin( GetLocalOrigin() );
		m_entFlames[i].m_flFrame			= Helper_RandomInt( 0.0f, maxFrames - 1 );
		m_entFlames[i].m_flSpriteFramerate	= Helper_RandomInt( 24, 30 );
		m_entFlames[i].SetScale( m_flStartScale );
		m_entFlames[i].m_nRenderMode		= kRenderTransAdd;
		m_entFlames[i].m_nRenderFX			= kRenderFxNone;
		m_entFlames[i].SetRenderColor( 255, 255, 255, 255 );
		m_entFlames[i].SetBrightness( 255 );

		m_entFlames[i].index				= -1;
		
		if ( i == 0 )
		{
			m_entFlameScales[i] = 1.0f;
		}
		else
		{
			//Keep a scale offset
			m_entFlameScales[i] = 1.0f - ( ( (float) i / (float) NUM_CHILD_FLAMES ) );
		}
	}

	//Only make the glow if we've requested it
	if ( m_nFlags & bitsFIRESMOKE_GLOW )
	{
		//Create the fire overlay
		if ( m_pFireOverlay = new CFireOverlay( this ) )
		{
			m_pFireOverlay->m_vPos		= GetAbsOrigin();
			m_pFireOverlay->m_nSprites	= 1;
			
			m_pFireOverlay->m_vBaseColors[0].Init( 0.4f, 0.2f, 0.05f );
			m_pFireOverlay->Activate();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: FIXME: what's the right way to do this?
//-----------------------------------------------------------------------------
void C_FireSmoke::StartClientOnly( void )
{
	Start();

	ClientEntityList().AddNonNetworkableEntity(	this );
	m_Partition = partition->CreateHandle( GetIClientUnknown() );
	view->AddVisibleEntity( this );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::RemoveClientOnly(void)
{
	ClientThinkList()->RemoveThinkable( GetClientHandle() );

	// Remove from the client entity list.
	ClientEntityList().RemoveEntity( GetClientHandle() );

	partition->Remove( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, m_Partition );

	RemoveFromLeafSystem();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateAnimation( void )
{
	int		numFrames;
	float	frametime	= Helper_GetFrameTime();

	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		m_entFlames[i].m_flFrame += m_entFlames[i].m_flSpriteFramerate * frametime;

		numFrames = modelinfo->GetModelFrameCount( m_entFlames[i].GetModel() );

		if ( m_entFlames[i].m_flFrame >= numFrames )
		{
			m_entFlames[i].m_flFrame = m_entFlames[i].m_flFrame - (int)(m_entFlames[i].m_flFrame);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateFlames( void )
{
	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		float	newScale = m_flScaleRegister * m_entFlameScales[i];
		Vector	dir;

		dir[2] = 0.0f;
		VectorNormalize( dir );
		dir[2] = 0.0f;

		Vector offset = GetAbsOrigin();
		offset[2] += FLAME_SOURCE_HEIGHT * m_entFlames[i].GetScale();

		//NOTENOTE: Sprite renderer assumes a scale of 0.0 means 1.0
		m_entFlames[i].SetScale( max(0.000001,newScale) );

		Assert( !m_entFlames[i].GetMoveParent() );
		if ( i != 0 )
		{
			m_entFlames[i].SetLocalOrigin( offset + ( m_entFlames[i].m_vecMoveDir * (m_entFlames[i].GetScale() * m_flChildFlameSpread) ) );
		}
		else
		{
			m_entFlames[i].SetLocalOrigin( offset );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateScale( void )
{
	float	time = Helper_GetTime();

	if ( m_flScaleRegister != m_flScaleEnd )
	{
		//See if we're done scaling
		if ( time > m_flScaleTimeEnd )
		{
			m_flScaleRegister = m_flStartScale = m_flScaleEnd;
		}
		else
		{
			//Lerp the scale and set it 
			float	timeFraction = 1.0f - ( m_flScaleTimeEnd - time ) / ( m_flScaleTimeEnd - m_flScaleTimeStart );	
			float	newScale = m_flScaleStart + ( ( m_flScaleEnd - m_flScaleStart ) * timeFraction );

			m_flScaleRegister = m_flStartScale = newScale;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::Update( void )
{
	//If we haven't already, find the clip plane for smoke effects
	if ( ( m_nFlags & bitsFIRESMOKE_SMOKE ) && ( m_bClipTested == false ) )
	{
		FindClipPlane();
	}

	//Update all our parts
	UpdateEffects();
	UpdateScale();
	UpdateAnimation();
	UpdateFlames();

	//See if we should emit smoke
	if ( m_nFlags & bitsFIRESMOKE_SMOKE )
	{
		float tempDelta = Helper_GetFrameTime();

		while( m_tParticleSpawn.NextEvent( tempDelta ) )
		{
			SpawnSmoke();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::FindClipPlane( void )
{
	m_bClipTested	= true;
	m_flClipPerc	= 1.0f;

	trace_t	tr;
	Vector	end( 0.0f, 0.0f, SMOKE_RISE_RATE*SMOKE_LIFETIME );
	
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin()+end, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	//If the ceiling is too close, reject smoke
	if ( tr.fraction < 0.5f )
	{
		m_nFlags &= ~bitsFIRESMOKE_SMOKE;
		return;
	}

	if ( tr.fraction < 1.0f )
	{
		m_planeClip.Init( tr.plane.normal, tr.plane.dist );
		m_nFlags		|= bitsFIRESMOKE_SMOKE_COLLISION;
		m_flClipPerc	= tr.fraction * 0.5f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn smoke (...duh)
//-----------------------------------------------------------------------------

void C_FireSmoke::SpawnSmoke( void )
{
	SimpleParticle	*sParticle;
	float			scalar;
	Vector			offset;

	scalar = 32.0f*m_flScaleRegister;
	offset[0] = Helper_RandomFloat( -scalar, scalar );
	offset[1] = Helper_RandomFloat( -scalar, scalar );
	offset[2] = scalar + ( Helper_RandomFloat( -scalar*0.25f, scalar*0.25f ) );

	//
	// Embers
	//

	sParticle = (SimpleParticle *) m_pEmberEmitter->AddParticle( sizeof(SimpleParticle), m_pEmberEmitter->GetPMaterial( "particle/fire"), GetAbsOrigin()+offset );
	
	if( sParticle )
	{
		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= EMBER_LIFETIME;

		sParticle->m_flRoll			= 0;
		sParticle->m_flRollDelta	= 0;

		scalar = Helper_RandomFloat( 0.5f, 2.0f );

		sParticle->m_uchColor[0]	= min( 255, Helper_RandomFloat( 185.0f, 190.0f ) * scalar );
		sParticle->m_uchColor[1]	= min( 255, Helper_RandomFloat( 140.0f, 165.0f ) * scalar );
		sParticle->m_uchColor[2]	= min( 255, 65.0f * scalar );
		sParticle->m_uchStartAlpha	= 255;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= 1;
		sParticle->m_uchEndSize		= 0;
		
		sParticle->m_vecVelocity	= Vector( Helper_RandomFloat( -16.0f, 16.0f ), Helper_RandomFloat( -16.0f, 16.0f ), Helper_RandomFloat( 92.0f, 128.0f ) );
	}

	//
	// Lit smoke
	//

	offset[2] += 32.0f;

	m_pSmokeEmitter->SetDirectionalLight( GetAbsOrigin(), Vector( 1.0f, 0.5f, 0.2f ), 2500 );

	CLitSmokeEmitter::LitSmokeParticle	*pParticle;
	pParticle = (CLitSmokeEmitter::LitSmokeParticle*) m_pSmokeEmitter->AddParticle( 
		sizeof(CLitSmokeEmitter::LitSmokeParticle), 
		m_pSmokeEmitter->GetPMaterial("particle/SmokeStack"), 
		GetAbsOrigin()+offset );

	if ( pParticle )
	{
		pParticle->m_flLifetime = 0;
		pParticle->m_flDieTime	= SMOKE_LIFETIME*m_flClipPerc;

		pParticle->m_vecVelocity = Vector( random->RandomFloat( -16.0f, 16.0f ), random->RandomFloat( -16.0f, 16.0f ), random->RandomFloat( SMOKE_RISE_RATE-16.0f, SMOKE_RISE_RATE ) );
		
		int	color = random->RandomInt( 8, 32 );
		pParticle->m_uchColor[0] = color;
		pParticle->m_uchColor[1] = color;
		pParticle->m_uchColor[2] = color;
		pParticle->m_uchColor[3] = random->RandomInt( 64, 200 );

		pParticle->m_uchStartSize	= random->RandomFloat( 12.0f, 16.0f );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4.0f;
	}
}


//
// Entity flame, client-side implementation
//

#define	NUM_FLAMELETS	5
#define NUM_HITBOX_FIRES	10

class C_EntityFlame : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EntityFlame, C_BaseEntity );

	C_EntityFlame( void );
	~C_EntityFlame( void );

	void OnDataChanged( DataUpdateType_t updateType );
	void Simulate( void );

protected:

	void AttachToHitBoxes( void );
	void UpdateHitBoxFlames( void );
	void DeleteHitBoxFlames( void );

	float			m_flSize;
	CSmartPtr<CEmberEffect> m_pEmitter;
	TimedEvent		m_ParticleSpawn;

	EHANDLE			m_hEntAttached;				// The entity that we are burning (attached to).
	bool			m_bAttachedToHitboxes;
	const model_t	*m_pCachedModel;				// Holds the model pointer to detect when it changes

	PMaterialHandle	m_MaterialHandle[NUM_FLAMELETS];

	// For attaching to the hitboxes of an animating model.
	C_FireSmoke *m_pFireSmoke[NUM_HITBOX_FIRES];
	Vector m_vecFireOrigin[NUM_HITBOX_FIRES];
	int m_nHitbox[NUM_HITBOX_FIRES];
};

IMPLEMENT_CLIENTCLASS_DT( C_EntityFlame, DT_EntityFlame, CEntityFlame )
	RecvPropFloat(RECVINFO(m_flSize)),
	RecvPropEHandle(RECVINFO(m_hEntAttached)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityFlame::C_EntityFlame( void )
{
	m_flSize	= 4.0f;
	m_pEmitter	= NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityFlame::~C_EntityFlame( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_EntityFlame::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetupEntityRenderHandle( RENDER_GROUP_TRANSLUCENT_ENTITY );

		C_BaseEntity *pEnt = m_hEntAttached;
		
		// FIXME: this should be IsBaseAnimating
		if (pEnt->IsBaseCombatCharacter())
		{
			AttachToHitBoxes();
		}
		else
		{
			m_ParticleSpawn.Init( 60 ); //Events per second

			m_pEmitter = CEmberEffect::Create("C_EntityFlame::Create");
			
			Assert( m_pEmitter.IsValid() );
			if ( m_pEmitter.IsValid() )
			{
				for ( int i = 1; i < NUM_FLAMELETS+1; i++ )
				{
					m_MaterialHandle[i-1] = m_pEmitter->GetPMaterial( VarArgs( "sprites/flamelet%d", i ) );
				}

				m_pEmitter->SetSortOrigin( GetAbsOrigin() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::Simulate( void )
{
	if (m_bAttachedToHitboxes)
	{
		UpdateHitBoxFlames();
	}
	else
	{
		m_pEmitter->SetSortOrigin( GetAbsOrigin() );

		float tempDelta = gpGlobals->frametime;
		
		SimpleParticle *pParticle;

		Vector	offset;

		while( m_ParticleSpawn.NextEvent( tempDelta ) )
		{
			offset.Random( -m_flSize, m_flSize );

			pParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_MaterialHandle[random->RandomInt( 0, NUM_FLAMELETS-1 )], GetAbsOrigin() + offset );

			if ( pParticle )
			{
				pParticle->m_flDieTime	= 0.4f;
				pParticle->m_flLifetime	= 0.0f;
				
				pParticle->m_flRoll		= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta= random->RandomFloat( -2.0f, 2.0f );
				
				pParticle->m_uchStartSize	= random->RandomInt( 4, 6 );
				pParticle->m_uchEndSize		= random->RandomInt( 12, 16 );
				pParticle->m_uchStartAlpha	= 255;
				pParticle->m_uchEndAlpha	= 0;
				
				int	cScale = 255;//random->RandomInt( 192, 255 );
				pParticle->m_uchColor[0]	= cScale;
				pParticle->m_uchColor[1]	= cScale;
				pParticle->m_uchColor[2]	= cScale;
				
				Vector dir;

				dir.x = random->RandomFloat( -1.0f, 1.0f );
				dir.y = random->RandomFloat( -1.0f, 1.0f );
				dir.z = random->RandomFloat( 0.5f, 1.0f );

				pParticle->m_vecVelocity	= dir * random->RandomInt( 4, 32 );
				pParticle->m_vecVelocity[2] = random->RandomInt( 32, 64 );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the volume of the given box in cubic inches.
//-----------------------------------------------------------------------------
inline float CalcBoxVolume(const Vector &mins, const Vector &maxs)
{
	return (maxs.x - mins.x) * (maxs.y - mins.y) * (maxs.z - mins.z);
}


//
// Used for sorting hitboxes by volume.
//
struct HitboxVolume_t
{
	int nIndex;			// The index of the hitbox in the model.
	float flVolume;		// The volume of the hitbox in cubic inches.
};


//-----------------------------------------------------------------------------
// Purpose: Callback function to sort hitboxes by decreasing volume.
//			To mix up the sort results a little we pick a random result for
//			boxes within 50 cubic inches of another.
//-----------------------------------------------------------------------------
int SortHitboxVolumes(HitboxVolume_t *elem1, HitboxVolume_t *elem2)
{
	if (elem1->flVolume > elem2->flVolume + 50)
	{
		return -1;
	}

	if (elem1->flVolume < elem2->flVolume + 50)
	{
		return 1;
	}

	if (elem1->flVolume != elem2->flVolume)
	{
		return random->RandomInt(-1, 1);
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Attaches fire to the hitboxes of an animating character. The fire
//			is distributed based on hitbox volumes -- it attaches to the larger
//			hitboxes first.
//-----------------------------------------------------------------------------
void C_EntityFlame::AttachToHitBoxes( void )
{
	m_pCachedModel = NULL;

	C_BaseCombatCharacter *pAnimating = (C_BaseCombatCharacter *)m_hEntAttached.Get();
	if (!pAnimating || !pAnimating->GetModel())
	{
		return;
	}

	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( pAnimating->GetModel() );
	if (!pStudioHdr)
	{
		return;
	}

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimating->m_nHitboxSet );
	if ( !set )
	{
		return;
	}

	if ( !set->numhitboxes )
	{
		return;
	}

	m_pCachedModel = pAnimating->GetModel();

	int boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;
	studiocache_t *pcache = Studio_GetBoneCache( pStudioHdr, pAnimating->GetSequence(), pAnimating->m_flAnimTime, pAnimating->GetAbsAngles(), pAnimating->GetAbsOrigin(), boneMask );
	if ( !pcache )
	{
		matrix3x4_t bonetoworld[MAXSTUDIOBONES];

		pAnimating->SetupBones( bonetoworld, MAXSTUDIOBONES, boneMask, gpGlobals->curtime );
		pcache = Studio_SetBoneCache( pStudioHdr, pAnimating->GetSequence(), pAnimating->m_flAnimTime, pAnimating->GetAbsAngles(), pAnimating->GetAbsOrigin(), boneMask, bonetoworld );
	}
	matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
	Studio_LinkHitboxCache( hitboxbones, pcache, pStudioHdr, set );

	//
	// Sort the hitboxes by volume.
	//
	HitboxVolume_t hitboxvolume[MAXSTUDIOBONES];
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pBox = set->pHitbox(i);
		hitboxvolume[i].nIndex = i;
		hitboxvolume[i].flVolume = CalcBoxVolume(pBox->bbmin, pBox->bbmax);
	}
	qsort(hitboxvolume, set->numhitboxes, sizeof(hitboxvolume[0]), (int (__cdecl *)(const void *, const void *))SortHitboxVolumes);

	//
	// Attach fire to the hitboxes.
	//
	for ( i = 0; i < NUM_HITBOX_FIRES; i++ )
	{
		//
		// Pick the 5 biggest hitboxes, or random ones if there are less than 5 hitboxes,
		// then pick random ones after that.
		//
		if (( i < 5 ) && ( i < set->numhitboxes ))
		{
			m_nHitbox[i] = hitboxvolume[i].nIndex;
		}
		else
		{
			m_nHitbox[i] = random->RandomInt( 0, set->numhitboxes - 1 );
		}
		mstudiobbox_t *pBox = set->pHitbox(m_nHitbox[i]);

		m_pFireSmoke[i] = new C_FireSmoke;

		//
		// Calculate a position within the hitbox to place the fire.
		//
		m_vecFireOrigin[i] = Vector(random->RandomFloat(pBox->bbmin.x, pBox->bbmax.x), random->RandomFloat(pBox->bbmin.y, pBox->bbmax.y), random->RandomFloat(pBox->bbmin.z, pBox->bbmax.z));
		Vector vecAbsOrigin;
		VectorTransform(m_vecFireOrigin[i], *hitboxbones[m_nHitbox[i]], vecAbsOrigin);
		m_pFireSmoke[i]->SetLocalOrigin( vecAbsOrigin );

		//
		// The first 2 fires emit smoke, the rest do not.
		//
		m_pFireSmoke[i]->m_nFlags = bitsFIRESMOKE_ACTIVE | bitsFIRESMOKE_GLOW;
		if ( i < 2 )
		{
			m_pFireSmoke[i]->m_nFlags |= bitsFIRESMOKE_SMOKE;
		}

		m_pFireSmoke[i]->m_nFlameModelIndex	= modelinfo->GetModelIndex("sprites/fire1.vmt");
		m_pFireSmoke[i]->m_flScale = 0;
		m_pFireSmoke[i]->m_flStartScale = 0;
		m_pFireSmoke[i]->m_flScaleTime = 1.5;
		m_pFireSmoke[i]->m_flScaleRegister = 0.1;
		m_pFireSmoke[i]->m_flChildFlameSpread = 20.0;
		m_pFireSmoke[i]->m_flScaleStart = 0;
		m_pFireSmoke[i]->m_flScaleEnd = 0.00012f * hitboxvolume[i].flVolume;
		m_pFireSmoke[i]->m_flScaleTimeStart = Helper_GetTime();
		m_pFireSmoke[i]->m_flScaleTimeEnd = Helper_GetTime() + 2.0;

		m_pFireSmoke[i]->StartClientOnly();
	}

	m_bAttachedToHitboxes = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::DeleteHitBoxFlames(void)
{
	for ( int i = 0; i < NUM_HITBOX_FIRES; i++ )
	{
		m_pFireSmoke[i]->RemoveClientOnly();
		delete m_pFireSmoke[i];
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::UpdateHitBoxFlames( void )
{
	C_BaseCombatCharacter *pAnimating = (C_BaseCombatCharacter *)m_hEntAttached.Get();
	if (!pAnimating)
	{
		return;
	}

	if (pAnimating->GetModel() != m_pCachedModel)
	{
		if (m_pCachedModel != NULL)
		{
			// The model changed, we must reattach the flames.
			DeleteHitBoxFlames();
			AttachToHitBoxes();
		}
		
		if (m_pCachedModel == NULL)
		{
			// We tried to reattach and failed.
			return;
		}
	}

	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( pAnimating->GetModel() );
	if (!pStudioHdr)
	{
		return;
	}

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimating->m_nHitboxSet );
	if ( !set )
	{
		return;
	}

	if ( !set->numhitboxes )
	{
		return;
	}

	int boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;
	studiocache_t *pcache = Studio_GetBoneCache( pStudioHdr, pAnimating->GetSequence(), pAnimating->m_flAnimTime, pAnimating->GetAbsAngles(), pAnimating->GetAbsOrigin(), boneMask );
	if ( !pcache )
	{
		matrix3x4_t bonetoworld[MAXSTUDIOBONES];

		pAnimating->SetupBones( bonetoworld, MAXSTUDIOBONES, boneMask, gpGlobals->curtime );
		pcache = Studio_SetBoneCache( pStudioHdr, pAnimating->GetSequence(), pAnimating->m_flAnimTime, pAnimating->GetAbsAngles(), pAnimating->GetAbsOrigin(), boneMask, bonetoworld );
	}

	matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
	Studio_LinkHitboxCache( hitboxbones, pcache, pStudioHdr, set );

	for ( int i = 0; i < NUM_HITBOX_FIRES; i++ )
	{
		Vector vecAbsOrigin;
		VectorTransform(m_vecFireOrigin[i], *hitboxbones[m_nHitbox[i]], vecAbsOrigin);

		m_pFireSmoke[i]->SetLocalOrigin(vecAbsOrigin);
	}
}


//CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectStriderKill )
//CLIENTEFFECT_MATERIAL( "effects/spark" )
//CLIENTEFFECT_REGISTER_END()
//
//
//class CStriderKillEffect : public CParticleEffect
//{
//public:
//	
//	CStriderKillEffect( const char *pDebugName ) : CParticleEffect( pDebugName ) {}
//	
//	static CStriderKillEffect *Create( const char *pDebugName )
//	{
//		return new CStriderKillEffect( pDebugName );
//	}
//
//	void Update( float fTimeDelta )
//	{
//		C_BaseCombatCharacter *pAnimating = (C_BaseCombatCharacter *)m_hEntAttached.Get();
//		if (!pAnimating)
//		{
//			return;
//		}
//
//		if (pAnimating->model != m_pCachedModel)
//		{
//			if (m_pCachedModel != NULL)
//			{
//				// The model changed, we must reattach the sparks.
//				DeleteHitBoxFlames();
//				AttachToHitBoxes();
//			}
//			
//			if (m_pCachedModel == NULL)
//			{
//				// We tried to reattach and failed.
//				return;
//			}
//		}
//
//		studiohdr_t *pStudioHdr = modelrender->GetStudiomodel( pAnimating->model );
//		if (!pStudioHdr)
//		{
//			return;
//		}
//
//		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimating->m_nHitboxSet );
//		if ( !set )
//		{
//			return;
//		}
//
//		if ( !set->numhitboxes )
//		{
//			return;
//		}
//
//		int boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;
//		studiocache_t *pcache = Studio_GetBoneCache( pStudioHdr, pAnimating->GetSequence(), pAnimating->m_flAnimTime, pAnimating->GetAbsAngles(), pAnimating->GetAbsOrigin(), boneMask );
//		if ( !pcache )
//		{
//			matrix3x4_t bonetoworld[MAXSTUDIOBONES];
//
//			pAnimating->SetupBones( bonetoworld, MAXSTUDIOBONES, boneMask, gpGlobals->curtime );
//			pcache = Studio_SetBoneCache( pStudioHdr, pAnimating->GetSequence(), pAnimating->m_flAnimTime, pAnimating->GetAbsAngles(), pAnimating->GetAbsOrigin(), boneMask, bonetoworld );
//		}
//
//		matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
//		Studio_LinkHitboxCache( hitboxbones, pcache, pStudioHdr, set );
//
//		//for ( int i = 0; i < NUM_HITBOX_SPARKS; i++ )
//		//{
//			//Vector vecAbsOrigin;
//			//VectorTransform(m_vecFireOrigin[i], *hitboxbones[m_nHitbox[i]], vecAbsOrigin);
//			//m_pFireSmoke[i]->SetLocalOrigin(vecAbsOrigin);
//		//}
//	}
//
//	bool SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey)
//	{
//		SimpleParticle *pParticle = (SimpleParticle *) pInParticle;
//		float timeDelta = pDraw->GetTimeDelta();
//
//		Vector	tPos;
//		TransformParticle( g_ParticleMgr.GetModelView(), pParticle->m_Pos, tPos );
//		sortKey = (int) tPos.z;
//
//		RenderParticle_ColorSizeAngle(
//			pDraw,
//			tPos,
//			UpdateColor( pParticle, timeDelta ),
//			UpdateAlpha( pParticle, timeDelta ) * GetAlphaDistanceFade( tPos, m_flNearClipMin, m_flNearClipMax ),
//			UpdateScale( pParticle, timeDelta ),
//			UpdateRoll( pParticle, timeDelta ) );
//
//		//Should this particle die?
//		pParticle->m_flLifetime += timeDelta;
//
//		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
//			return false;
//
//		return true;
//	}
//
//
//private:
//
//	EHANDLE m_hEntAttached;
//
//	CStriderKillEffect( const CStriderKillEffect & );
//};
//
//
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			normal - 
//			scale - 
//-----------------------------------------------------------------------------
//void FX_StriderKill( CBaseAnimating *pAnimating )
//{
//	if ( cl_show_bloodspray.GetBool() == false )
//		return;
//
//	debugoverlay->AddLineOverlay( origin, origin + normal * 72, 255, 255, 255, true, 10 ); 
//
//	Vector offset;
//	float spread	= 0.2f;
//	
//	Vector color = Vector( 0.25f, 0.0f, 0.0f );
//	float colorRamp;
//
//	int i;
//
//	Vector	vForward, vRight, vUp;
//	Vector	offDir;
//
//	CSmartPtr<CStriderKillEffect> pSimple = CStriderKillEffect::Create( "striderkill" );
//	if ( !pSimple )
//		return;
//
//	pSimple->SetSortOrigin( pAnimating->GetAbsOrigin() );
//
//	PMaterialHandle	hMaterial = g_ParticleMgr.GetPMaterial( "effects/spark" );
//
//	SimpleParticle	*pParticle;
//
//	for ( i = 0; i < NUM_HITBOX_SPARKS; i++ )
//	{
//		offset = origin;
//		offset[0] += random->RandomFloat( -2.0f, 2.0f ) * scale;
//		offset[1] += random->RandomFloat( -2.0f, 2.0f ) * scale;
//
//		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, offset );
//
//		if ( pParticle != NULL )
//		{
//			pParticle->m_flLifetime = 0.0f;
//			pParticle->m_flDieTime	= 0.75f;
//
//			spread = 1.0f;
//			pParticle->m_vecVelocity.Random( -spread, spread );
//			pParticle->m_vecVelocity += normal;
//			VectorNormalize( pParticle->m_vecVelocity );
//
//			pParticle->m_flGravity = 0;
//			
//			colorRamp = random->RandomFloat( 0.75f, 1.25f );
//
//			pParticle->m_uchColor[0]	= min( 1.0f, color[0] * colorRamp ) * 255.0f;
//			pParticle->m_uchColor[1]	= min( 1.0f, color[1] * colorRamp ) * 255.0f;
//			pParticle->m_uchColor[2]	= min( 1.0f, color[2] * colorRamp ) * 255.0f;
//			
//			pParticle->m_uchStartSize	= random->RandomFloat( scale * 0.5, scale * 2 );
//			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
//			
//			pParticle->m_uchStartAlpha	= random->RandomInt( 128, 255 );
//			pParticle->m_uchEndAlpha	= 0;
//			
//			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
//			pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
//		}
//	}
//}
