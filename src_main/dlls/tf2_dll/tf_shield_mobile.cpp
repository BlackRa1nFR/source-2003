//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:			The Escort's Shield weapon
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_shield.h"
#include "in_buttons.h"
#include "gamerules.h"
#include "tf_shieldshared.h"
#include "tf_shareddefs.h"

ConVar	shield_mobile_power( "shield_mobile_power","30", FCVAR_NONE, "Max power level of a escort's mobile projected shield." );
ConVar	shield_mobile_recharge_delay( "shield_mobile_recharge_delay","0.1", FCVAR_NONE, "Time after taking damage before mobile projected shields begin to recharge." );
ConVar	shield_mobile_recharge_amount( "shield_mobile_recharge_amount","2", FCVAR_NONE, "Power recharged each recharge tick for mobile projected shields." );
ConVar	shield_mobile_recharge_time( "shield_mobile_recharge_time","0.5", FCVAR_NONE, "Time between each recharge tick for mobile projected shields." );

//-----------------------------------------------------------------------------
// This is the version of the shield used by the 
//-----------------------------------------------------------------------------

class CShieldMobile;
class CShieldMobileActiveVertList : public IActiveVertList
{
public:
	void			Init( CShieldMobile *pShield );

// IActiveVertList overrides.
public:

	virtual int		GetActiveVertState( int iVert );
	virtual void	SetActiveVertState( int iVert, int bOn );

private:
	CShieldMobile	*m_pShield;
};


class CShieldMobile : public CShield, public IEntityEnumerator
{
public:
	DECLARE_CLASS( CShieldMobile, CShield );
	DECLARE_SERVERCLASS();
	friend class CShieldMobileActiveVertList;

	CShieldMobile();

	DECLARE_DATADESC();

public:
	void Spawn( void );
	void Precache( void );
	void ShieldThink( void );
	void SetCenterAngles( const QAngle &angles );
	virtual void SetAngularSpringConstant( float flConstant );
	virtual void SetFrontDistance( float flDistance );

	virtual void SetEMPed( bool isEmped );
	virtual void SetAlwaysOrient( bool bOrient ) { m_bAlwaysOrientToOwner = bOrient; }

	virtual int Width() { return SHIELD_NUM_HORIZONTAL_POINTS; }
	virtual int Height() { return SHIELD_NUM_VERTICAL_POINTS; }
	virtual bool IsPanelActive( int x, int y ) { return m_ShieldEffect.IsPanelActive( x, y ); }
	virtual const Vector& GetPoint( int x, int y ) { return m_ShieldEffect.GetPoint( x, y ); }

	virtual void SetThetaPhi( float flTheta, float flPhi );

public:
	// Inherited from IEntityEnumerator
	virtual bool EnumEntity( IHandleEntity *pHandleEntity ); 

private:
	// Teleport!
	void Teleported(  );

	// Bitfield indicating which vertices are active
	CNetworkArray( unsigned char, m_pVertsActive, SHIELD_NUM_CONTROL_POINTS >> 3 );

	CNetworkVar( unsigned char, m_ShieldState );

private:
	struct SweepContext_t
	{
		SweepContext_t( const CBaseEntity *passentity, int collisionGroup ) :
			m_Filter( passentity, collisionGroup ) {}

		CTraceFilterSimple m_Filter;
		Vector m_vecStartDelta;
		Vector m_vecEndDelta;
	};

	void ComputeBoundingBox( void );
	void DetermineObstructions( );

	virtual void SetObjectCollisionBox( void );

private:
	CShieldEffect				m_ShieldEffect;
	CShieldMobileActiveVertList	m_VertList;
	bool						m_bAlwaysOrientToOwner;
	float m_flFrontDistance;
	CNetworkVar( float, m_flTheta );
	CNetworkVar( float, m_flPhi );
	SweepContext_t *m_pEnumCtx;
};


//=============================================================================
// Shield effect
//=============================================================================
BEGIN_DATADESC( CShieldMobile )

	DEFINE_THINKFUNC( CShieldMobile, ShieldThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( shield_mobile, CShieldMobile );
//PRECACHE_WEAPON_REGISTER(shield_effect);

IMPLEMENT_SERVERCLASS_ST(CShieldMobile, DT_Shield_Mobile)

	SendPropInt		(SENDINFO(m_ShieldState),	1, SPROP_UNSIGNED ),
	SendPropArray(
		SendPropInt( SENDINFO_ARRAY(m_pVertsActive), 8, SPROP_UNSIGNED),
		m_pVertsActive),

	SendPropFloat( SENDINFO(m_flTheta), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flPhi), 8, SPROP_UNSIGNED ),

END_SEND_TABLE()

static void ShieldTraceLine(const Vector &vecStart, const Vector &vecEnd, 
					 unsigned int mask, int collisionGroup, trace_t *ptr)
{
	UTIL_TraceLine(vecStart, vecEnd, mask, 0, COLLISION_GROUP_NONE, ptr );
}

static void ShieldTraceHull(const Vector &vecStart, const Vector &vecEnd, 
					 const Vector &hullMin, const Vector &hullMax, 
					 unsigned int mask, int collisionGroup, trace_t *ptr)
{
	UTIL_TraceHull( vecStart, vecEnd, hullMin, hullMax, mask, 0, collisionGroup, ptr );
}



//-----------------------------------------------------------------------------
// CShieldMobileActiveVertList functions
//-----------------------------------------------------------------------------

void CShieldMobileActiveVertList::Init( CShieldMobile *pShield )
{
	m_pShield = pShield;
}


int CShieldMobileActiveVertList::GetActiveVertState( int iVert )
{
	return m_pShield->m_pVertsActive[iVert>>3] & (1 << (iVert & 7));
}


void CShieldMobileActiveVertList::SetActiveVertState( int iVert, int bOn )
{
	m_pShield->NetworkStateChanged();

	unsigned char val;	
	if ( bOn )
		val = m_pShield->m_pVertsActive[iVert>>3] | (1 << (iVert & 7));
	else
		val = m_pShield->m_pVertsActive[iVert>>3] & ~(1 << (iVert & 7));

	m_pShield->m_pVertsActive.Set( iVert>>3, val );
}


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CShieldMobile::CShieldMobile() : m_ShieldEffect( ShieldTraceLine, ShieldTraceHull )
{
	m_VertList.Init( this );
	m_flFrontDistance = 0;
	SetThetaPhi( SHIELD_INITIAL_THETA, SHIELD_INITIAL_PHI );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CShieldMobile::Precache( void )
{
	m_ShieldEffect.SetActiveVertexList( &m_VertList );
	m_ShieldEffect.SetCollisionGroup( TFCOLLISION_GROUP_SHIELD );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CShieldMobile::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	m_ShieldEffect.Spawn(GetAbsOrigin(), GetAbsAngles());
	m_ShieldState = 0;
	SetAlwaysOrient( false );

	// All movement occurs during think
	SetMoveType( MOVETYPE_NONE );

	SetThink( ShieldThink );
	SetNextThink( gpGlobals->curtime + 0.01f );
}


//-----------------------------------------------------------------------------
// Called when the shield is EMPed
//-----------------------------------------------------------------------------
void CShieldMobile::SetEMPed( bool isEmped )
{
	CShield::SetEMPed(isEmped);
	if (IsEMPed())
		m_ShieldState |= SHIELD_MOBILE_EMP;
	else
		m_ShieldState &= ~SHIELD_MOBILE_EMP;
}

void CShieldMobile::SetThetaPhi( float flTheta, float flPhi ) 
{ 
	m_flTheta = flTheta;
	m_flPhi = flPhi;
	m_ShieldEffect.SetThetaPhi( m_flTheta, m_flPhi ); 
}

//-----------------------------------------------------------------------------
// Set the shield angles
//-----------------------------------------------------------------------------
void CShieldMobile::SetCenterAngles( const QAngle &angles )
{
	m_ShieldEffect.SetAngles( angles );
}


void CShieldMobile::SetAngularSpringConstant( float flConstant )
{
	m_ShieldEffect.SetAngularSpringConstant( flConstant );
}


void CShieldMobile::SetFrontDistance( float flDistance )
{
	m_flFrontDistance = flDistance;
}


//-----------------------------------------------------------------------------
// Computes the shield bounding box
//-----------------------------------------------------------------------------
void CShieldMobile::ComputeBoundingBox( void )
{
	Vector mins, maxs;
	m_ShieldEffect.ComputeBounds(mins, maxs); 
	UTIL_SetSize( this, mins, maxs );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CShieldMobile::SetObjectCollisionBox( void )
{
	if ( !pev )
		return;

	SetAbsMins( GetAbsOrigin() + WorldAlignMins() );
	SetAbsMaxs( GetAbsOrigin() + WorldAlignMaxs() );
}

//-----------------------------------------------------------------------------
// Determines shield obstructions 
//-----------------------------------------------------------------------------
void CShieldMobile::DetermineObstructions( )
{
//	Vector mins, maxs;
//	m_ShieldEffect.ComputeBounds( mins, maxs );
//	VectorMin( mins, m_ShieldEffect.GetCurrentPosition(), mins );
//	VectorMax( maxs, m_ShieldEffect.GetCurrentPosition(), maxs );

//	engine->BeginTraceSubtree( mins, maxs );

	m_ShieldEffect.ComputeVertexActivity();

//	engine->EndTraceSubtree( );

}


//-----------------------------------------------------------------------------
// Called by the enumerator call in ShieldThink 
//-----------------------------------------------------------------------------
bool CShieldMobile::EnumEntity( IHandleEntity *pHandleEntity )
{
	CBaseEntity *pOther = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
	if (!pOther)
		return true;

	// Blow off non-solid things
	if ( !::IsSolid(pOther->GetSolid(), pOther->GetSolidFlags()) )
		return true;

	// No model, blow it off
	if ( !pOther->GetModelIndex() )
		return true;
	
	// Blow off point-sized things....
	if ( pOther->GetSize() == vec3_origin )
		return true;

	// Don't bother if we shouldn't be colliding with this guy...
	if (!m_pEnumCtx->m_Filter.ShouldHitEntity( pOther, MASK_SOLID ))
		return true;

	// The shield is in its final position, so we're gonna have to determine the
	// point of collision by working in the space of the final position....
	// We do this by moving the obstruction by the relative movement amount...
	Vector vecStart, vecEnd;
	VectorAdd( pOther->GetAbsOrigin(), m_pEnumCtx->m_vecStartDelta, vecStart );
	VectorAdd( vecStart, m_pEnumCtx->m_vecEndDelta, vecEnd );

	Ray_t ray;
	ray.Init( vecStart, vecEnd, pOther->WorldAlignMins(), pOther->WorldAlignMaxs() );

	trace_t tr;
	if (TestCollision( ray, pOther->PhysicsSolidMaskForEntity(), tr ))
	{
		// Ok, we got a collision. Let's indicate it happened...
		// At the moment, we'll report the collision point as being on the
		// surface of the shield in its final position, which is kind of bogus...
		pOther->PhysicsImpact( this, tr );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Update the shield position: 
//-----------------------------------------------------------------------------
void CShieldMobile::ShieldThink( void )
{
	CBaseEntity *owner = GetOwnerEntity();
	Vector origin;
	if ( owner )
	{
		if ( m_bAlwaysOrientToOwner )
		{
			SetCenterAngles( owner->GetAbsAngles() );
		}

		origin = owner->EyePosition();
		if ( m_flFrontDistance )
		{
			Vector vForward;
			AngleVectors( m_ShieldEffect.GetAngles(), &vForward );
			origin += vForward * m_flFrontDistance;
		}
	}
	else
	{
		Assert( 0 );
		origin = vec3_origin;
	}

	Vector vecOldOrigin = m_ShieldEffect.GetCurrentPosition();
	
	Vector vecDelta;
	VectorSubtract( origin, vecOldOrigin, vecDelta );

	float flMaxDist = 100 + m_flFrontDistance;
	if (vecDelta.LengthSqr() > flMaxDist * flMaxDist )
	{
		Teleported();
		SetNextThink( gpGlobals->curtime + 0.01f )
		return;
	}

	m_ShieldEffect.SetOrigin( origin );
	m_ShieldEffect.Simulate(gpGlobals->frametime);
	DetermineObstructions();
	SetAbsOrigin( m_ShieldEffect.GetCurrentPosition() );
	SetAbsAngles( m_ShieldEffect.GetCurrentAngles() );

	// Compute a composite bounding box surrounding the initial + new positions..
	Vector vecCompositeMins = WorldAlignMins() + vecOldOrigin;
	Vector vecCompositeMaxs = WorldAlignMaxs() + vecOldOrigin;

	ComputeBoundingBox();

	// Sweep the shield through the world + touch things it hits...
	SweepContext_t ctx( this, GetCollisionGroup() );
	VectorSubtract( GetAbsOrigin(), vecOldOrigin, ctx.m_vecStartDelta );

	if (ctx.m_vecStartDelta != vec3_origin)
	{
		// FIXME: Brutal hack; needed because IntersectRayWithTriangle misses stuff
		// especially with short rays; I'm not sure what to do about this.
		// This basically simulates a shield thickness of 15 units
		ctx.m_vecEndDelta = ctx.m_vecStartDelta;
		VectorNormalize( ctx.m_vecEndDelta );
		ctx.m_vecEndDelta *= -15.0f;

		Vector vecNewMins = WorldAlignMins() + GetAbsOrigin();
		Vector vecNewMaxs = WorldAlignMaxs() + GetAbsOrigin();
		VectorMin( vecCompositeMins, vecNewMins, vecCompositeMins );
		VectorMax( vecCompositeMaxs, vecNewMaxs, vecCompositeMaxs );

		m_pEnumCtx = &ctx;
		enginetrace->EnumerateEntities( vecCompositeMins, vecCompositeMaxs, this );
	}

	// Always think
	SetNextThink( gpGlobals->curtime + 0.01f );
}

	 
//-----------------------------------------------------------------------------
// Teleport!
//-----------------------------------------------------------------------------
void CShieldMobile::Teleported(  )
{
	CBaseEntity *owner = GetOwnerEntity();
	if (!owner)
		return;

	m_ShieldEffect.SetCurrentAngles( owner->GetAbsAngles() );

	Vector origin;
	origin = owner->EyePosition();
	if ( m_flFrontDistance )
	{
		Vector vForward;
		AngleVectors( m_ShieldEffect.GetCurrentAngles(), &vForward );
		origin += vForward * m_flFrontDistance;
	}

	m_ShieldEffect.SetCurrentPosition( origin );
}


//-----------------------------------------------------------------------------
// Purpose: Create a mobile version of the shield
//-----------------------------------------------------------------------------
CShield *CreateMobileShield( CBaseEntity *owner, float flFrontDistance )
{
	CShieldMobile *pShield = (CShieldMobile*)CreateEntityByName("shield_mobile");

	pShield->SetOwnerEntity( owner );
	pShield->SetCenterAngles( owner->GetAbsAngles() );
	pShield->SetLocalAngles( owner->GetAbsAngles() );
	pShield->SetFrontDistance( flFrontDistance );

	// Start it in the right place
	Vector vForward;
	AngleVectors( owner->GetAbsAngles(), &vForward );
	Vector vecOrigin = owner->EyePosition() + (vForward * flFrontDistance);
	UTIL_SetOrigin( pShield, vecOrigin );

	pShield->ChangeTeam( owner->GetTeamNumber() );
	pShield->SetupRecharge( shield_mobile_power.GetFloat(), shield_mobile_recharge_delay.GetFloat(), shield_mobile_recharge_amount.GetFloat(), shield_mobile_recharge_time.GetFloat() );
	pShield->Spawn();

	return pShield;
}

