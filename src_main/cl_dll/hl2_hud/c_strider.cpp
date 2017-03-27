//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_ai_basenpc.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "fx_sparks.h"
#include "c_tracer.h"
#include "parsemsg.h"
#include "clientsideeffects.h"
#include "iefx.h"
#include "dlight.h"
#include "bone_setup.h"
#include "c_rope.h"
#include "fx_line.h"
#include "c_sprite.h"
#include "view.h"
#include "view_scene.h"
#include "materialsystem/imaterialvar.h"
#include "simple_keys.h"
#include "fx_envelope.h"
#include "iclientvehicle.h"

#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3

#define STOMP_IK_SLOT					11

const float	STRIDERFX_BIG_SHOT_TIME = 1.25;

class C_StriderFX : public C_EnvelopeFX
{
public:
	typedef C_EnvelopeFX	BaseClass;

	C_StriderFX();
	void			Update( C_BaseEntity *pOwner, const Vector &targetPos );

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( m_worldPosition, mins, maxs );
		AddPointToBounds( m_targetPosition, mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}

	virtual int	DrawModel( int flags );

	C_BaseEntity			*m_pOwner;
	Vector					m_targetPosition;
	Vector					m_beamEndPosition;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_Strider : public C_AI_BaseNPC, public IClientVehicle
{
	DECLARE_CLASS( C_Strider, C_AI_BaseNPC );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();


					C_Strider();
	virtual			~C_Strider();

	// model specific
	virtual void	ReceiveMessage( const char *msgname, int length, void *data );
	virtual void	CalculateIKLocks( float currentTime )
	{
		BaseClass::CalculateIKLocks( currentTime );
		if ( m_pIk && m_pIk->m_target.Count() )
		{
			// HACKHACK: Hardcoded 11???  Not a cleaner way to do this
			iktarget_t &target = m_pIk->m_target[STOMP_IK_SLOT];
			target.est.pos = m_vecHitPos;
		}
	}

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual void Relink();
	virtual void ClientThink();

// IClientVehicle overrides.
public:
	virtual void OnEnteredVehicle( C_BasePlayer *pPlayer ) {}
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	virtual void GetVehicleFOV( float &flFOV ) { return; }
	virtual void DrawHudElements() {}
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_DRIVER ) { return true; }

	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) 
	{
		int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
		Vector origin;
		QAngle angles;
		GetAttachmentLocal( eyeAttachmentIndex, origin, angles );

		// Limit the view yaw to the relative range specified
		float flAngleDiff = AngleDiff( pCmd->viewangles.y, angles.y );
		flAngleDiff = clamp( flAngleDiff, -90, 90 );
		pCmd->viewangles.y = angles.y + flAngleDiff;
	}
	virtual C_BasePlayer* GetPassenger( int nRole ) { return m_hPlayer; }
	virtual int	GetPassengerRole( C_BasePlayer *pEnt ) { return VEHICLE_DRIVER; }
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const {}
	virtual C_BaseEntity *GetVehicleEnt() { return this; }

	virtual IClientVehicle *GetClientVehicle() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}

private:
	C_Strider( const C_Strider & );
	C_StriderFX	m_cannonFX;
	Vector		m_vecHitPos;
	CInterpolatedVar< Vector >		m_iv_vecHitPos;
	Vector		m_vecRenderMins;
	Vector		m_vecRenderMaxs;

	float m_flNextRopeCutTime;

	CHandle<C_BasePlayer>	m_hPlayer;
};

IMPLEMENT_CLIENTCLASS_DT(C_Strider, DT_NPC_Strider, CNPC_Strider)
	RecvPropVector(RECVINFO(m_vecHitPos)),
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
END_RECV_TABLE()

C_StriderFX::C_StriderFX()
{
	m_pOwner = NULL;
	m_handle = INVALID_CLIENT_RENDER_HANDLE;
	m_active = false;
}

void C_StriderFX::Update( C_BaseEntity *pOwner, const Vector &targetPos )
{
	BaseClass::Update();

	m_pOwner = pOwner;
	
	if ( m_active )
	{
		m_targetPosition = targetPos;
	}
}

// --on gun
// warpy sprite bit
// darkening sprite
// glowy blue flare sprite
// bubble warpy sprite
// after glow sprite

// --on line of sight
// narrow beam
// wide beam

// --on impact point
// sparkly white bits
// sparkly white streaks
// pale blue particle steam

enum 
{
	STRIDERFX_WARP_SCALE = 0,
	STRIDERFX_DARKNESS,
	STRIDERFX_FLARE_COLOR,
	STRIDERFX_FLARE_SIZE,
	STRIDERFX_BUBBLE_SIZE,
	STRIDERFX_BUBBLE_REFRACT,

	STRIDERFX_NARROW_BEAM_COLOR,
	STRIDERFX_NARROW_BEAM_SIZE,
	
	STRIDERFX_WIDE_BEAM_COLOR,
	STRIDERFX_WIDE_BEAM_SIZE,

	STRIDERFX_AFTERGLOW_COLOR,

	STRIDERFX_WIDE_BEAM_LENGTH,

	STRIDERFX_SPARK_COUNT,
	STRIDERFX_STREAK_COUNT,
	STRIDERFX_STEAM_COUNT,


	// must be last
	STRIDERFX_PARAMETERS,
};

class CStriderFXEnvelope
{
public:
	CStriderFXEnvelope();

	void AddKey( int parameterIndex, const CSimpleKeyInterp &key )
	{
		Assert( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS );

		if ( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS )
		{
			m_parameters[parameterIndex].Insert( key );
		}

	}

	CSimpleKeyList		m_parameters[STRIDERFX_PARAMETERS];
};

// NOTE: Beam widths are half-widths or radii, so this is a beam that represents a cylinder with 2" radius
const float NARROW_BEAM_WIDTH = 2;
const float WIDE_BEAM_WIDTH = 16;
const float FLARE_SIZE = 128;
const float	DARK_SIZE = 64;
const float AFTERGLOW_SIZE = 64;

const float WARP_SIZE = 512;
const float WARP_REFRACT = .5f;
const float WARP_BUBBLE_SIZE = 256;
const float WARP_BUBBLE_REFRACT = 1.0f;

CStriderFXEnvelope::CStriderFXEnvelope()
{
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.25, KEY_SPLINE, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0.5, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.5, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 2.0, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0.5 ) );
	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 2.0, KEY_DECELERATE, 2 ) );

	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 1.3, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 2.0, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1.0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.5, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.5, KEY_DECELERATE, 2 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.75, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 2.1, KEY_SPLINE, 0 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 2.1, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.25, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.5, KEY_ACCELERATE, 0 ) );

	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.5, KEY_ACCELERATE, 0.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 2.1, KEY_LINEAR, 0 ) );

	//AddKey( STRIDERFX_SPARK_COUNT,
	//AddKey( STRIDERFX_STREAK_COUNT,
	//AddKey( STRIDERFX_STEAM_COUNT,
}

CStriderFXEnvelope g_StriderCannonEnvelope;

void ScaleColor( color32 &out, const color32 &in, float scale )
{
	out.r = (byte)(int)((float)in.r * scale);
	out.g = (byte)(int)((float)in.g * scale);
	out.b = (byte)(int)((float)in.b * scale);
	out.a = (byte)(int)((float)in.a * scale);
}

void DrawSpriteTangentSpace( const Vector &vecOrigin, float flWidth, float flHeight, color32 color )
{
	unsigned char pColor[4] = { color.r, color.g, color.b, color.a };

	// Generate half-widths
	flWidth *= 0.5f;
	flHeight *= 0.5f;

	// Compute direction vectors for the sprite
	Vector fwd, right( 1, 0, 0 ), up( 0, 1, 0 );
	VectorSubtract( CurrentViewOrigin(), vecOrigin, fwd );
	float flDist = VectorNormalize( fwd );
	if (flDist >= 1e-3)
	{
		CrossProduct( CurrentViewUp(), fwd, right );
		flDist = VectorNormalize( right );
		if (flDist >= 1e-3)
		{
			CrossProduct( fwd, right, up );
		}
		else
		{
			// In this case, fwd == g_vecVUp, it's right above or 
			// below us in screen space
			CrossProduct( fwd, CurrentViewRight(), up );
			VectorNormalize( up );
			CrossProduct( up, fwd, right );
		}
	}

	Vector left = -right;
	Vector down = -up;
	Vector back = -fwd;

	CMeshBuilder meshBuilder;
	Vector point;
	IMesh* pMesh = materials->GetDynamicMesh( );

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}


void Strider_DrawSprite( const Vector &vecOrigin, float size, const color32 &color, bool glow )
{
	if ( glow )
	{
		if ( GlowSightDistance( vecOrigin ) <= 0 )
			return;
	}

	DrawSpriteTangentSpace( vecOrigin, size, size, color );
}


void Strider_DrawLine( const Vector &start, const Vector &end, float width, IMaterial *pMaterial, const color32 &color )
{
	FX_DrawLine( start, end, width, pMaterial, color );
}

int	C_StriderFX::DrawModel( int )
{
	static color32 white = {255,255,255,255};
	Vector params[STRIDERFX_PARAMETERS];
	bool hasParam[STRIDERFX_PARAMETERS];

	if ( !m_active )
		return 1;

	C_BaseEntity *ent = cl_entitylist->GetEnt( m_entityIndex );
	if ( ent )
	{
		QAngle angles;
		ent->GetAttachment( m_attachment, m_worldPosition, angles );
	}

	Vector test;
	m_t += gpGlobals->frametime;
	if ( m_tMax > 0 )
	{
		m_t = clamp( m_t, 0, m_tMax );
		m_beamEndPosition = m_worldPosition;
	}
	float t = m_t;

	bool hasAny = false;
	memset( hasParam, 0, sizeof(hasParam) );
	for ( int i = 0; i < STRIDERFX_PARAMETERS; i++ )
	{
		hasParam[i] = g_StriderCannonEnvelope.m_parameters[i].Interp( params[i], t );
		hasAny = hasAny || hasParam[i];
	}

	// draw the narrow beam
	if ( hasParam[STRIDERFX_NARROW_BEAM_COLOR] && hasParam[STRIDERFX_NARROW_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "sprites/bluelaser1", 0, 0 );
		float width = NARROW_BEAM_WIDTH * params[STRIDERFX_NARROW_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_NARROW_BEAM_COLOR].x;
		ScaleColor( color, white, bright );

		Strider_DrawLine( m_beamEndPosition, m_targetPosition, width, pMat, color );
	}

	// draw the wide beam
	if ( hasParam[STRIDERFX_WIDE_BEAM_COLOR] && hasParam[STRIDERFX_WIDE_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblacklargebeam", 0, 0 );
		float width = WIDE_BEAM_WIDTH * params[STRIDERFX_WIDE_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_WIDE_BEAM_COLOR].x;
		ScaleColor( color, white, bright );
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}

		Strider_DrawLine( wideBeamEnd, m_targetPosition, width, pMat, color );
	}

// after glow sprite
	bool updated = false;
// warpy sprite bit
	if ( hasParam[STRIDERFX_WARP_SCALE] && !hasParam[STRIDERFX_BUBBLE_SIZE] )
	{
		if ( !updated )
		{
			updated = true;
			materials->Flush();
			UpdateRefractTexture();
		}

		IMaterial *pMat = materials->FindMaterial( "effects/strider_pinch_dudv", 0, 0 );
		float size = WARP_SIZE;
		float refract = params[STRIDERFX_WARP_SCALE].x * WARP_REFRACT;

		materials->Bind( pMat, (IClientRenderable*)this );
		IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
		pVar->SetFloatValue( refract );
		Strider_DrawSprite( m_worldPosition, size, white, false );
	}
// darkening sprite
// glowy blue flare sprite
	if ( hasParam[STRIDERFX_FLARE_COLOR] && hasParam[STRIDERFX_FLARE_SIZE] && hasParam[STRIDERFX_DARKNESS] )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", 0, 0 );
		float size = FLARE_SIZE * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_FLARE_COLOR].x;
		ScaleColor( color, white, bright );
		color.a = (int)(255 * params[STRIDERFX_DARKNESS].x);
		materials->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( m_worldPosition, size, color, true );
	}
// bubble warpy sprite
	if ( hasParam[STRIDERFX_BUBBLE_SIZE] )
	{
		if ( !updated )
		{
			updated = true;
			materials->Flush();
			UpdateRefractTexture();
		}
		IMaterial *pMat = materials->FindMaterial( "effects/strider_bulge_dudv", 0, 0 );
		float refract = WARP_BUBBLE_REFRACT * params[STRIDERFX_BUBBLE_REFRACT].x;
		float size = WARP_BUBBLE_SIZE * params[STRIDERFX_BUBBLE_SIZE].x;
		IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
		pVar->SetFloatValue( refract );

		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}
		materials->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( wideBeamEnd, size, white, false );
	}
	if ( hasParam[STRIDERFX_AFTERGLOW_COLOR] )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", 0, 0 );
		float size = AFTERGLOW_SIZE;// * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_AFTERGLOW_COLOR].x;
		ScaleColor( color, white, bright );

		materials->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( m_worldPosition, size, color, true );

		dlight_t *dl = effects->CL_AllocDlight( m_entityIndex );
		dl->origin = m_worldPosition;
		dl->color.r = 40;
		dl->color.g = 60;
		dl->color.b = 255;
		dl->color.exponent = 5;
		dl->radius = bright * 128;
		dl->die = gpGlobals->curtime + 0.001;
	}

	if ( m_t >= 4.0 && !hasAny )
	{
		EffectShutdown();
	}
	return 1;
}




//-----------------------------------------------------------------------------
// Purpose: Strider class implementation
//-----------------------------------------------------------------------------
C_Strider::C_Strider()
{
	AddVar( &m_vecHitPos, &m_iv_vecHitPos, LATCH_ANIMATION_VAR );

	m_flNextRopeCutTime = 0;
}

C_Strider::~C_Strider()
{
}

void C_Strider::ReceiveMessage( const char *msgname, int length, void *data )
{
	BEGIN_READ( data, length );
	int messageType = READ_BYTE();
	switch( messageType )
	{
	case STRIDER_MSG_STREAKS:
		{
			Vector	pos;
			READ_VEC3COORD( pos );
			m_cannonFX.SetRenderOrigin( pos );
			m_cannonFX.EffectInit( entindex(), LookupAttachment( "BigGun" ) );
			m_cannonFX.LimitTime( STRIDERFX_BIG_SHOT_TIME );
		}
		break;

	case STRIDER_MSG_BIG_SHOT:
		{
			Vector tmp;
			READ_VEC3COORD( tmp );
			m_cannonFX.SetTime( STRIDERFX_BIG_SHOT_TIME );
			m_cannonFX.LimitTime( 0 );
		}
		break;

	case STRIDER_MSG_DEAD:
		{
			m_cannonFX.EffectShutdown();
		}
		break;
	}
}


void C_Strider::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}
	m_cannonFX.Update( this, m_vecHitPos );
}


//-----------------------------------------------------------------------------
// Blarff!!! Remove this when we find a better way of solving the IK problem
//-----------------------------------------------------------------------------
void C_Strider::Relink()
{
}


//-----------------------------------------------------------------------------
// Purpose: Recompute my rendering box
//-----------------------------------------------------------------------------
void C_Strider::ClientThink()
{
	int i;
	Vector vecMins, vecMaxs;
	Vector vecAbsMins, vecAbsMaxs;
	studiocache_t *pcache;
	int boneMask;
	matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
	matrix3x4_t worldToStrider, hitboxToStrider;
	Vector vecBoxMins, vecBoxMaxs;
	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	mstudiohitboxset_t *set;

	// The reason why this is here, as opposed to in SetObjectCollisionBox,
	// is because of IK. The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the Relink phase.

	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( GetModel() );
	if (!pStudioHdr)
		goto doneWithComputation;

	set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		goto doneWithComputation;

	boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;
	pcache = Studio_GetBoneCache( pStudioHdr, GetSequence(), m_flAnimTime, GetAbsAngles(), GetAbsOrigin(), boneMask );
	if ( !pcache )
	{
		matrix3x4_t bonetoworld[MAXSTUDIOBONES];

		// FIXME: Do I need to update m_CachedBoneFlags here?
		SetupBones( bonetoworld, MAXSTUDIOBONES, boneMask, gpGlobals->curtime );
		pcache = Studio_SetBoneCache( pStudioHdr, GetSequence(), m_flAnimTime, GetAbsAngles(), GetAbsOrigin(), boneMask, bonetoworld );
	}
	Studio_LinkHitboxCache( hitboxbones, pcache, pStudioHdr, set );

	// Compute a box in world space that surrounds this entity
	vecAbsMins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	vecAbsMaxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	m_vecRenderMins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_vecRenderMaxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	MatrixInvert( EntityToWorldTransform(), worldToStrider );

	for ( i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		ConcatTransforms( worldToStrider, *hitboxbones[i], hitboxToStrider );

		TransformAABB( *hitboxbones[i], pbox->bbmin, pbox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );
		VectorMin( vecAbsMins, vecBoxAbsMins, vecAbsMins );
		VectorMax( vecAbsMaxs, vecBoxAbsMaxs, vecAbsMaxs );

		TransformAABB( hitboxToStrider, pbox->bbmin, pbox->bbmax, vecBoxMins, vecBoxMaxs );
		VectorMin( m_vecRenderMins, vecBoxMins, m_vecRenderMins );
		VectorMax( m_vecRenderMaxs, vecBoxMaxs, m_vecRenderMaxs );
	}

	VectorSubtract( vecAbsMins, GetAbsOrigin(), vecMins );
	VectorSubtract( vecAbsMaxs, GetAbsOrigin(), vecMaxs );

	SetCollisionBounds( vecMins, vecMaxs );
	SetAbsMins( vecAbsMins );
	SetAbsMaxs( vecAbsMaxs );

	// UNDONE: Disabled this until we can get closer to a final map and tune
#if 0
	// Cut ropes.
	if ( gpGlobals->curtime >= m_flNextRopeCutTime )
	{
		// Blow the bbox out a little.
		Vector vExtendedMins = vecMins - Vector( 50, 50, 50 );
		Vector vExtendedMaxs = vecMaxs + Vector( 50, 50, 50 );

		C_RopeKeyframe *ropes[512];
		int nRopes = C_RopeKeyframe::GetRopesIntersectingAABB( ropes, ARRAYSIZE( ropes ), GetAbsOrigin() + vExtendedMins, GetAbsOrigin() + vExtendedMaxs );
		for ( int i=0; i < nRopes; i++ )
		{
			C_RopeKeyframe *pRope = ropes[i];

			if ( pRope->GetEndEntity() )
			{
				Vector vPos;
				if ( pRope->GetEndPointPos( 1, vPos ) )
				{
					// Detach the endpoint.
					pRope->SetEndEntity( NULL );
					
					// Make some spark effect here..
					g_pEffects->Sparks( vPos );
				}				
			}
		}

		m_flNextRopeCutTime = gpGlobals->curtime + 0.5;
	}
#endif

doneWithComputation:	
	partition->ElementMoved( m_Partition, GetAbsMins(), GetAbsMaxs() );

	// True argument because the origin may have stayed the same, but the size is expected to always change
	g_pClientShadowMgr->UpdateShadow( GetShadowHandle(), true );
}


//-----------------------------------------------------------------------------
// Purpose: Recompute my rendering box
//-----------------------------------------------------------------------------
void C_Strider::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins = m_vecRenderMins;
	theMaxs = m_vecRenderMaxs;
}


void C_Strider::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = GetPassenger( nRole );
	Assert( pPlayer );
	*pAbsAngles = pPlayer->EyeAngles();
	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	QAngle tmp;
	GetAttachment( eyeAttachmentIndex, *pAbsOrigin, tmp );
}

