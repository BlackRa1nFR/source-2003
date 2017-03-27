//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "Point_Camera.h"

#define CAM_THINK_INTERVAL 0.05

// Spawnflags
#define SF_CAMERA_START_OFF				0x01

// UNDONE: Share properly with the client code!!!
#define POINT_CAMERA_MSG_SETACTIVE		1

// These are already built into CBaseEntity
//	DEFINE_KEYFIELD( CBaseEntity, m_iName, FIELD_STRING, "targetname" ),
//	DEFINE_KEYFIELD( CBaseEntity, m_iParent, FIELD_STRING, "parentname" ),
//	DEFINE_KEYFIELD( CBaseEntity, m_target, FIELD_STRING, "target" ),

LINK_ENTITY_TO_CLASS( point_camera, CPointCamera );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointCamera::CPointCamera()
{
	// Set these to opposites so that it'll be sent the first time around.
	m_bActive = false;
	m_bLastActive = true;
	m_bIsOn = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::Spawn( void )
{
	BaseClass::Spawn();

	if ( m_spawnflags & SF_CAMERA_START_OFF )
	{
		m_bIsOn = false;
	}
	else
	{
		m_bIsOn = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override ShouldTransmit since we want to be sent even though we don't have a model, etc.
//			All that matters is if we are in the pvs.
//-----------------------------------------------------------------------------
bool CPointCamera::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	// If the mapmaker's told the camera it's off, it enforces inactive state
	if ( !m_bIsOn )
	{
		m_bActive = false;
	}

	if ( m_bLastActive != m_bActive )
	{
		m_bLastActive = m_bActive.Get();
		return true;
	}
	
	if ( !engine->CheckAreasConnected( clientArea, pev->areanum) )
	{	
		// doors can legally straddle two areas, so
		// we may need to check another one
		if ( !pev->areanum2 || !engine->CheckAreasConnected( clientArea, pev->areanum2 ) )
		{
			return false;		// blocked by a door
		}
	}

	return IsInPVS( recipient, pvs );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputChangeFOV( inputdata_t &inputdata )
{
	// Parse the keyvalue data
	char parseString[255];

	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get FOV
	char *pszParam = strtok(parseString," ");
	if(pszParam)
	{
		m_TargetFOV = atof( pszParam );
	}
	else
	{
		// Assume no change
		m_TargetFOV = m_FOV;
	}

	// Get Time
	float flChangeTime;
	pszParam = strtok(NULL," ");
	if(pszParam)
	{
		flChangeTime = atof( pszParam );
	}
	else
	{
		// Assume 1 second.
		flChangeTime = 1.0;
	}

	m_DegreesPerSecond = ( m_TargetFOV - m_FOV ) / flChangeTime;

	SetThink( ChangeFOVThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::ChangeFOVThink( void )
{
	SetNextThink( gpGlobals->curtime + CAM_THINK_INTERVAL );

	float newFOV = m_FOV;

	newFOV += m_DegreesPerSecond * CAM_THINK_INTERVAL;

	if( m_DegreesPerSecond < 0 )
	{
		if( newFOV <= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}
	else
	{
		if( newFOV >= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}

	m_FOV = newFOV;
}

//-----------------------------------------------------------------------------
// Purpose: Turn this camera on, and turn all other cameras off
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOnAndTurnOthersOff( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "point_camera" )) != NULL)
	{
		CPointCamera *pCamera = (CPointCamera*)pEntity;
		pCamera->InputSetOff( inputdata );
	}

	// Now turn myself on
	m_bIsOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOn( inputdata_t &inputdata )
{
	m_bIsOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOff( inputdata_t &inputdata )
{
	m_bIsOn = false;
}

BEGIN_DATADESC( CPointCamera )

	// Save/restore Keyvalue fields
	DEFINE_KEYFIELD( CPointCamera, m_FOV,		FIELD_FLOAT, "FOV" ),
	DEFINE_KEYFIELD( CPointCamera, m_Resolution, FIELD_FLOAT, "resolution" ),
	DEFINE_FIELD( CPointCamera, m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CPointCamera, m_bLastActive,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CPointCamera, m_bIsOn,		FIELD_BOOLEAN ),

	DEFINE_FIELD( CPointCamera, m_TargetFOV,	FIELD_FLOAT ),
	DEFINE_FIELD( CPointCamera, m_DegreesPerSecond, FIELD_FLOAT ),

	DEFINE_FUNCTION( CPointCamera, ChangeFOVThink ),

	// Input
	DEFINE_INPUTFUNC( CPointCamera, FIELD_STRING, "ChangeFOV", InputChangeFOV ),
	DEFINE_INPUTFUNC( CPointCamera, FIELD_VOID, "SetOnAndTurnOthersOff", InputSetOnAndTurnOthersOff ),
	DEFINE_INPUTFUNC( CPointCamera, FIELD_VOID, "SetOn", InputSetOn ),
	DEFINE_INPUTFUNC( CPointCamera, FIELD_VOID, "SetOff", InputSetOff ),

END_DATADESC()



IMPLEMENT_SERVERCLASS_ST( CPointCamera, DT_PointCamera )
	SendPropFloat( SENDINFO( m_FOV ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_Resolution ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_bActive ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

