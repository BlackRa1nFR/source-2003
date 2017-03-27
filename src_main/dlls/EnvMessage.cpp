//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "EnvMessage.h"
#include "engine/IEngineSound.h"


LINK_ENTITY_TO_CLASS( env_message, CMessage );

BEGIN_DATADESC( CMessage )

	DEFINE_KEYFIELD( CMessage, m_iszMessage, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( CMessage, m_sNoise, FIELD_SOUNDNAME, "messagesound" ),
	DEFINE_KEYFIELD( CMessage, m_MessageAttenuation, FIELD_INTEGER, "messageattenuation" ),
	DEFINE_KEYFIELD( CMessage, m_MessageVolume, FIELD_FLOAT, "messagevolume" ),

	DEFINE_FIELD( CMessage, m_Radius, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( CMessage, FIELD_VOID, "ShowMessage", InputShowMessage ),

	DEFINE_OUTPUT(CMessage, m_OnShowMessage, "OnShowMessage"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessage::Spawn( void )
{
	Precache();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	switch( m_MessageAttenuation )
	{
	case 1: // Medium radius
		m_Radius = ATTN_STATIC;
		break;
	
	case 2:	// Large radius
		m_Radius = ATTN_NORM;
		break;

	case 3:	//EVERYWHERE
		m_Radius = ATTN_NONE;
		break;
	
	default:
	case 0: // Small radius
		m_Radius = SNDLVL_IDLE;
		break;
	}
	m_MessageAttenuation = 0;

	// Remap volume from [0,10] to [0,1].
	m_MessageVolume *= 0.1;

	// No volume, use normal
	if ( m_MessageVolume <= 0 )
	{
		m_MessageVolume = 1.0;
	}
}


void CMessage::Precache( void )
{
	if ( m_sNoise != NULL_STRING )
		enginesound->PrecacheSound( STRING(m_sNoise) );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CMessage::InputShowMessage( inputdata_t &inputdata )
{
	CBaseEntity *pPlayer = NULL;

	if ( m_spawnflags & SF_MESSAGE_ALL )
	{
		UTIL_ShowMessageAll( STRING( m_iszMessage ) );
	}
	else
	{
		if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
		{
			pPlayer = inputdata.pActivator;
		}
		else
		{
			pPlayer = CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
		}

		if ( pPlayer )
		{
			UTIL_ShowMessage( STRING( m_iszMessage ), pPlayer );
		}
	}

	if ( m_sNoise != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound( filter, entindex(), CHAN_BODY, STRING(m_sNoise), m_MessageVolume, m_Radius );
	}

	if ( m_spawnflags & SF_MESSAGE_ONCE )
	{
		UTIL_Remove( this );
	}

	m_OnShowMessage.FireOutput( inputdata.pActivator, this );
}


void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	inputdata_t inputdata;

	inputdata.pActivator	= NULL;
	inputdata.pCaller		= NULL;

	InputShowMessage( inputdata );
}
