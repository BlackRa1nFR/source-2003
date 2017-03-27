//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Dynamic light.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "dlight.h"


class CDynamicLight : public CBaseEntity
{
public:
	DECLARE_CLASS( CDynamicLight, CBaseEntity );

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	// Turn on and off the light
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

public:
	unsigned char m_ActualFlags;
	CNetworkVar( unsigned char, m_Flags );
	CNetworkVar( unsigned char, m_LightStyle );
	bool	m_On;
	CNetworkVar( float, m_Radius );
	CNetworkVar( int, m_Exponent );
	CNetworkVar( float, m_InnerAngle );
	CNetworkVar( float, m_OuterAngle );
	CNetworkVar( float, m_SpotRadius );
};

LINK_ENTITY_TO_CLASS(light_dynamic, CDynamicLight);

BEGIN_DATADESC( CDynamicLight )

	DEFINE_FIELD( CDynamicLight, m_ActualFlags, FIELD_CHARACTER ),
	DEFINE_FIELD( CDynamicLight, m_Flags, FIELD_CHARACTER ),
	DEFINE_FIELD( CDynamicLight, m_On, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUT( CDynamicLight, m_Radius,		FIELD_FLOAT,	"distance" ),
	DEFINE_INPUT( CDynamicLight, m_Exponent,	FIELD_INTEGER,	"brightness" ),
	DEFINE_INPUT( CDynamicLight, m_InnerAngle,	FIELD_FLOAT,	"_inner_cone" ),
	DEFINE_INPUT( CDynamicLight, m_OuterAngle,	FIELD_FLOAT,	"_cone" ),
	DEFINE_INPUT( CDynamicLight, m_SpotRadius,	FIELD_FLOAT,	"spotlight_radius" ),
	DEFINE_INPUT( CDynamicLight, m_LightStyle,	FIELD_CHARACTER,"style" ),
	
	// Input functions
	DEFINE_INPUTFUNC( CDynamicLight, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CDynamicLight, FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( CDynamicLight, FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CDynamicLight, DT_DynamicLight)
	SendPropInt( SENDINFO(m_Flags), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_LightStyle), 4, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_Radius), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_Exponent), 8),
	SendPropFloat( SENDINFO(m_InnerAngle), 8, 0, 0.0, 360.0f ),
	SendPropFloat( SENDINFO(m_OuterAngle), 8, 0, 0.0, 360.0f ),
	SendPropFloat( SENDINFO(m_SpotRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()


bool CDynamicLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "_light" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		SetRenderColor( tmp.r, tmp.g, tmp.b );
	}
	else if ( FStrEq( szKeyName, "pitch" ) )
	{
		float angle = atof(szValue);
		if ( angle )
		{
			QAngle angles = GetAbsAngles();
			angles[PITCH] = -angle;
			SetAbsAngles( angles );
		}
	}
	else if ( FStrEq( szKeyName, "spawnflags" ) )
	{
		m_ActualFlags = m_Flags = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//------------------------------------------------------------------------------
// Turn on and off the light
//------------------------------------------------------------------------------
void CDynamicLight::InputTurnOn( inputdata_t &inputdata )
{
	m_Flags = m_ActualFlags;
	m_On = true;
}

void CDynamicLight::InputTurnOff( inputdata_t &inputdata )
{
	// This basically shuts it off
	m_Flags = DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_NO_WORLD_ILLUMINATION;
	m_On = false;
}

void CDynamicLight::InputToggle( inputdata_t &inputdata )
{
	if (m_On)
	{
		InputTurnOff( inputdata );
	}
	else
	{
		InputTurnOn( inputdata );
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CDynamicLight::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	m_On = true;
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	Relink();
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}


