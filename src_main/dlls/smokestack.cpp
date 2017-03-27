//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the server side of a steam jet particle system entity.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "smokestack.h"
#include "particle_light.h"


#define SENDLIGHTINFO(index) \
	SendPropVector( SENDINFO_NOCHECK(m_Lights[index].m_vPos), 0, SPROP_NOSCALE ),\
	SendPropVector( SENDINFO_NOCHECK(m_Lights[index].m_vColor), 0, SPROP_NOSCALE ),\
	SendPropFloat( SENDINFO_NOCHECK(m_Lights[index].m_flIntensity), 0, SPROP_NOSCALE )


//Networking
IMPLEMENT_SERVERCLASS_ST(CSmokeStack, DT_SmokeStack)
	SendPropFloat(SENDINFO(m_SpreadSpeed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Speed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Rate), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_JetLength), 0, SPROP_NOSCALE),
	SendPropInt(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flBaseSpread), 0, SPROP_NOSCALE),
	SendPropVector(SENDINFO(m_DirLightColor)),
	SendPropVector(SENDINFO(m_vWind), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flTwist), 0, SPROP_NOSCALE),
	SendPropInt(SENDINFO(m_DirLightSource), 1, SPROP_UNSIGNED),

	SENDLIGHTINFO(0),
	SENDLIGHTINFO(1),
	SendPropInt( SENDINFO(m_nLights), 3)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_smokestack, CSmokeStack );


//Save/restore

BEGIN_SIMPLE_DATADESC( CSmokeStackLightInfo )
	DEFINE_FIELD( CSmokeStackLightInfo,		m_vPos,			FIELD_POSITION_VECTOR	),
	DEFINE_FIELD( CSmokeStackLightInfo,		m_vColor,		FIELD_VECTOR	),
	DEFINE_FIELD( CSmokeStackLightInfo,		m_flIntensity,	FIELD_FLOAT	),
END_DATADESC()

BEGIN_DATADESC( CSmokeStack )

	//Keyvalue fields
	DEFINE_KEYFIELD( CSmokeStack, m_StartSize,		FIELD_FLOAT,	"StartSize" ),
	DEFINE_KEYFIELD( CSmokeStack, m_EndSize,		FIELD_FLOAT,	"EndSize" ),
	DEFINE_KEYFIELD( CSmokeStack, m_InitialState,	FIELD_BOOLEAN,	"InitialState" ),
	DEFINE_KEYFIELD( CSmokeStack, m_flBaseSpread,	FIELD_FLOAT,	"BaseSpread" ),
	DEFINE_KEYFIELD( CSmokeStack, m_flTwist,		FIELD_FLOAT,	"Twist" ),
	DEFINE_KEYFIELD( CSmokeStack, m_DirLightColor,	FIELD_VECTOR,	"DirLightColor" ),
	DEFINE_KEYFIELD( CSmokeStack, m_DirLightSource, FIELD_INTEGER,	"DirLightSource" ),

	DEFINE_KEYFIELD( CSmokeStack, m_WindAngle, FIELD_INTEGER,	"WindAngle" ),
	DEFINE_KEYFIELD( CSmokeStack, m_WindSpeed, FIELD_INTEGER,	"WindSpeed" ),

	//Regular fields
	DEFINE_FIELD( CSmokeStack,	m_nLights,	FIELD_INTEGER ),
	DEFINE_FIELD( CSmokeStack,	m_vWind,	FIELD_VECTOR ),
	DEFINE_FIELD( CSmokeStack,	m_bEmit,	FIELD_INTEGER ),
	DEFINE_EMBEDDED_AUTO_ARRAY( CSmokeStack, m_Lights ),

	// Inputs
	DEFINE_INPUT( CSmokeStack, m_JetLength, FIELD_FLOAT, "JetLength" ),
	DEFINE_INPUT( CSmokeStack, m_SpreadSpeed, FIELD_FLOAT, "SpreadSpeed" ),
	DEFINE_INPUT( CSmokeStack, m_Speed, FIELD_FLOAT, "Speed" ),
	DEFINE_INPUT( CSmokeStack, m_Rate, FIELD_FLOAT, "Rate" ),

	DEFINE_INPUTFUNC( CSmokeStack, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CSmokeStack, FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( CSmokeStack, FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
CSmokeStack::CSmokeStack()
{
	memset( m_Lights, 0, sizeof(m_Lights) ); 
	for ( int i=0; i < NUM_LIGHTS; i++ )
	{
		IMPLEMENT_NETWORKVAR_CHAIN( &m_Lights[i] );
	}

	m_nLights = 0;
	m_flTwist = 0;
	SetRenderColor( 0, 0, 0, 255 );
	m_vWind.GetForModify().Init();
	m_WindAngle = m_WindSpeed = 0;
}


CSmokeStack::~CSmokeStack()
{
}


void CSmokeStack::Spawn( void )
{
	if ( m_InitialState )
	{
		m_bEmit = true;
	}
}


void CSmokeStack::Activate()
{
	DetectInSkybox();

	// Find local lights.
	edict_t *pEdict = engine->PEntityOfEntIndex( 1 );
	// UNDONE: Seems like a bad way to do this?  Use NextEntByClass instead?
	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		if ( pEdict->free )	// Not in use
			continue;

		if( !FStrEq( PARTICLELIGHT_ENTNAME, STRING( pEdict->classname ) ) )
			continue;

		CParticleLight *pLight = (CParticleLight*)CBaseEntity::Instance( pEdict );
		if( !FStrEq( STRING(GetEntityName()), STRING(pLight->m_PSName) ) )
			continue;

		if( m_nLights < NUM_LIGHTS )
		{
			m_Lights[m_nLights].m_flIntensity = pLight->m_flIntensity;
			m_Lights[m_nLights].m_vColor = pLight->m_vColor;
			m_Lights[m_nLights].m_vPos = pLight->GetLocalOrigin();
			m_nLights += 1;
		}
	}

	BaseClass::Activate();
}


bool CSmokeStack::KeyValue( const char *szKeyName, const char *szValue )
{
	if( stricmp( szKeyName, "Wind" ) == 0 )
	{
		sscanf( szValue, "%f %f %f", &m_vWind.GetForModify().x, &m_vWind.GetForModify().y, &m_vWind.GetForModify().z );
		return true;
	}
	else if( stricmp( szKeyName, "WindAngle" ) == 0 )
	{
		m_WindAngle = atoi( szValue );
		RecalcWindVector();
		return true;
	}
	else if( stricmp( szKeyName, "WindSpeed" ) == 0 )
	{
		m_WindSpeed = atoi( szValue );
		RecalcWindVector();
		return true;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the steam jet on/off.
//-----------------------------------------------------------------------------
void CSmokeStack::InputToggle( inputdata_t &inputdata )
{
	m_bEmit = !m_bEmit;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning on the steam jet.
//-----------------------------------------------------------------------------
void CSmokeStack::InputTurnOn( inputdata_t &inputdata )
{
	m_bEmit = true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning off the steam jet.
//-----------------------------------------------------------------------------
void CSmokeStack::InputTurnOff( inputdata_t &inputdata )
{
	m_bEmit = false;
}


void CSmokeStack::RecalcWindVector()
{
	m_vWind = Vector(  
		cos( DEG2RAD( (float)m_WindAngle ) ) * m_WindSpeed, 
		sin( DEG2RAD( (float)m_WindAngle ) ) * m_WindSpeed,
		0 );
	
}


