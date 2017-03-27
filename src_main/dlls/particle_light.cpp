//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particle_light.h"


LINK_ENTITY_TO_CLASS( env_particlelight, CParticleLight );


//Save/restore
BEGIN_DATADESC( CParticleLight )

	//Keyvalue fields
	DEFINE_KEYFIELD( CParticleLight, m_flIntensity,		FIELD_FLOAT,	"Intensity" ),
	DEFINE_KEYFIELD( CParticleLight, m_vColor,			FIELD_VECTOR,	"Color" ),
	DEFINE_KEYFIELD( CParticleLight, m_PSName,			FIELD_STRING,	"PSName" )

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
CParticleLight::CParticleLight()
{
	m_flIntensity = 5000;
	m_vColor.Init( 1, 0, 0 );
	m_PSName = NULL_STRING;
}


