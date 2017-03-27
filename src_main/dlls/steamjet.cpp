//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the server side of a steam jet particle system entity.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "steamjet.h"


//Networking
IMPLEMENT_SERVERCLASS_ST(CSteamJet, DT_SteamJet)
	SendPropFloat(SENDINFO(m_SpreadSpeed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Speed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Rate), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_JetLength), 0, SPROP_NOSCALE),
	SendPropInt(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_bFaceLeft), 1, SPROP_UNSIGNED), // For support of legacy env_steamjet, which faced left instead of forward.
	SendPropInt(SENDINFO(m_nType), 32, SPROP_UNSIGNED),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_steam, CSteamJet );
LINK_ENTITY_TO_CLASS( env_steamjet, CSteamJet ); // For support of legacy env_steamjet, which faced left instead of forward.

//Save/restore
BEGIN_DATADESC( CSteamJet )

	//Keyvalue fields
	DEFINE_KEYFIELD( CSteamJet, m_StartSize,	FIELD_FLOAT,	"StartSize" ),
	DEFINE_KEYFIELD( CSteamJet, m_EndSize,		FIELD_FLOAT,	"EndSize" ),
	DEFINE_KEYFIELD( CSteamJet, m_InitialState,	FIELD_BOOLEAN,	"InitialState" ),
	DEFINE_KEYFIELD( CSteamJet, m_nType,		FIELD_INTEGER,	"Type" ),

	//Regular fields
	DEFINE_FIELD( CSteamJet, m_bEmit, FIELD_INTEGER ),
	DEFINE_FIELD( CSteamJet, m_bFaceLeft, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUT( CSteamJet, m_JetLength, FIELD_FLOAT, "JetLength" ),
	DEFINE_INPUT( CSteamJet, m_SpreadSpeed, FIELD_FLOAT, "SpreadSpeed" ),
	DEFINE_INPUT( CSteamJet, m_Speed, FIELD_FLOAT, "Speed" ),
	DEFINE_INPUT( CSteamJet, m_Rate, FIELD_FLOAT, "Rate" ),

	DEFINE_INPUTFUNC( CSteamJet, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CSteamJet, FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( CSteamJet, FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
void CSteamJet::Spawn( void )
{
	//
	// Legacy env_steamjet pointed left instead of forward.
	//
	if ( FClassnameIs( this, "env_steamjet" ))
	{
		m_bFaceLeft = true;
	}

	if ( m_InitialState )
	{
		m_bEmit = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when entities generically target this entity.
//-----------------------------------------------------------------------------
void CSteamJet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( useType == USE_OFF )
	{
		m_bEmit = false;
	}
	else if( useType == USE_ON )
	{
		m_bEmit = true;
	}
	else if( useType == USE_SET )
	{
		m_bEmit = !!(int)value;
	}
	else if( useType == USE_TOGGLE )
	{
		m_bEmit = !m_bEmit;
	}
	else
	{
		Assert(false);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the steam jet on/off.
//-----------------------------------------------------------------------------
void CSteamJet::InputToggle(inputdata_t &data)
{
	m_bEmit = !m_bEmit;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning on the steam jet.
//-----------------------------------------------------------------------------
void CSteamJet::InputTurnOn(inputdata_t &data)
{
	m_bEmit = true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning off the steam jet.
//-----------------------------------------------------------------------------
void CSteamJet::InputTurnOff(inputdata_t &data)
{
	m_bEmit = false;
}
