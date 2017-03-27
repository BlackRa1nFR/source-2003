//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "extinguisherjet.h"
#include "engine/IEngineSound.h"
#include "fire.h"
#include "ndebugoverlay.h"

extern ConVar fire_extinguisher_debug;

//Networking
IMPLEMENT_SERVERCLASS_ST( CExtinguisherJet, DT_ExtinguisherJet )
	SendPropInt(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_bUseMuzzlePoint), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nLength), 32, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nSize), 32, SPROP_UNSIGNED),
END_SEND_TABLE()

//Save/restore
BEGIN_DATADESC( CExtinguisherJet )

	//Regular fields
	DEFINE_FIELD( CExtinguisherJet, m_bEmit,		FIELD_BOOLEAN ),
	
	DEFINE_KEYFIELD( CExtinguisherJet, m_bEnabled,	FIELD_BOOLEAN, "enabled" ),
	DEFINE_KEYFIELD( CExtinguisherJet, m_nLength,	FIELD_INTEGER, "length" ),
	DEFINE_KEYFIELD( CExtinguisherJet, m_nSize,		FIELD_INTEGER, "size" ),
	DEFINE_KEYFIELD( CExtinguisherJet, m_nRadius,	FIELD_INTEGER, "radius" ),
	DEFINE_KEYFIELD( CExtinguisherJet, m_flStrength,FIELD_FLOAT,   "strength" ),

	DEFINE_FIELD( CExtinguisherJet, m_bAutoExtinguish,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CExtinguisherJet, m_bUseMuzzlePoint,	FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( CExtinguisherJet, FIELD_VOID, "Enable",	InputEnable ),
	DEFINE_INPUTFUNC( CExtinguisherJet, FIELD_VOID, "Disable",	InputDisable ),
	DEFINE_INPUTFUNC( CExtinguisherJet, FIELD_VOID, "Toggle",	InputToggle ),

	DEFINE_FUNCTION( CExtinguisherJet, ExtinguishThink ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( env_extinguisherjet, CExtinguisherJet );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CExtinguisherJet::CExtinguisherJet( void )
{
	m_bEmit				= false;
	m_bEnabled			= false;
	m_bAutoExtinguish	= true;

	m_nLength			= 128;
	m_nSize				= 8;
	m_flStrength		= 0.97f;	//FIXME: Stub numbers
	m_nRadius			= 32;

	// Send to the client even though we don't have a model
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherJet::Spawn( void )
{
	if ( m_bEnabled )
	{
		TurnOn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherJet::TurnOn( void )
{
	//Turn on sound
	if ( m_bEmit == false )
	{
		EmitSound( "ExtinguisherJet.TurnOn" );
		m_bEnabled = m_bEmit = true;
	}
	
	SetThink( ExtinguishThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherJet::TurnOff( void )
{
	//Turn off sound
	if ( m_bEmit )
	{
		EmitSound( "ExtinguisherJet.TurnOff" );
		m_bEnabled = m_bEmit = false;
	}
	
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CExtinguisherJet::InputEnable( inputdata_t &inputdata )
{
	TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CExtinguisherJet::InputDisable( inputdata_t &inputdata )
{
	TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CExtinguisherJet::InputToggle( inputdata_t &inputdata )
{
	if ( m_bEnabled )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherJet::Think( void )
{
	CBaseEntity::Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherJet::ExtinguishThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( m_bEnabled == false )
		return;

	if ( m_bAutoExtinguish == false )
		return;

	Vector	vTestPos;
	Vector	vForward, vRight, vUp;

	AngleVectors( GetAbsAngles(), &vForward );
	
	vTestPos = GetAbsOrigin() + ( vForward * m_nLength );

	trace_t	tr;
	UTIL_TraceLine( GetAbsOrigin(), vTestPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	//Extinguish the fire where we hit
	FireSystem_ExtinguishInRadius( tr.endpos, m_nRadius, m_flStrength );

	//Debug visualization
	if ( fire_extinguisher_debug.GetInt() )
	{
		int	radius = m_nRadius;

		NDebugOverlay::Line( GetAbsOrigin(), tr.endpos, 0, 0, 128, false, 0.1f );
		
		NDebugOverlay::Box( GetAbsOrigin(), Vector(-1, -1, -1), Vector(1, 1, 1), 0, 0, 128, false, 0.1f );
		NDebugOverlay::Box( tr.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 128, false, 0.1f );
		NDebugOverlay::Box( tr.endpos, Vector(-radius, -radius, -radius), Vector(radius, radius, radius), 0, 0, 255, false, 0.1f );
	}
}
