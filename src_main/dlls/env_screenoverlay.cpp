//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Entity to control screen overlays on a player
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvScreenOverlay : public CPointEntity
{
	DECLARE_CLASS( CEnvScreenOverlay, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvScreenOverlay();

	// Always transmit to clients
	virtual bool ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
	{
		return true;
	}

	void	InputStartOverlay( inputdata_t &inputdata );

protected:
	CNetworkArray( string_t, m_iszOverlayNames, MAX_SCREEN_OVERLAYS );
	CNetworkArray( float, m_flOverlayTimes, MAX_SCREEN_OVERLAYS );
	CNetworkVar( float, m_flStartTime );
};

LINK_ENTITY_TO_CLASS( env_screenoverlay, CEnvScreenOverlay );

BEGIN_DATADESC( CEnvScreenOverlay )

// Silence, Classcheck!
//	DEFINE_ARRAY( CEnvScreenOverlay, m_iszOverlayNames, FIELD_STRING, MAX_SCREEN_OVERLAYS ),
//	DEFINE_ARRAY( CEnvScreenOverlay, m_flOverlayTimes, FIELD_FLOAT, MAX_SCREEN_OVERLAYS ),

	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[0], FIELD_STRING, "OverlayName1" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[1], FIELD_STRING, "OverlayName2" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[2], FIELD_STRING, "OverlayName3" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[3], FIELD_STRING, "OverlayName4" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[4], FIELD_STRING, "OverlayName5" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[5], FIELD_STRING, "OverlayName6" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[6], FIELD_STRING, "OverlayName7" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[7], FIELD_STRING, "OverlayName8" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[8], FIELD_STRING, "OverlayName9" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_iszOverlayNames[9], FIELD_STRING, "OverlayName10" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[0], FIELD_FLOAT, "OverlayTime1" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[1], FIELD_FLOAT, "OverlayTime2" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[2], FIELD_FLOAT, "OverlayTime3" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[3], FIELD_FLOAT, "OverlayTime4" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[4], FIELD_FLOAT, "OverlayTime5" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[5], FIELD_FLOAT, "OverlayTime6" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[6], FIELD_FLOAT, "OverlayTime7" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[7], FIELD_FLOAT, "OverlayTime8" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[8], FIELD_FLOAT, "OverlayTime9" ),
	DEFINE_KEYFIELD( CEnvScreenOverlay, m_flOverlayTimes[9], FIELD_FLOAT, "OverlayTime10" ),

	DEFINE_FIELD( CEnvScreenOverlay, m_flStartTime, FIELD_TIME ),

	DEFINE_INPUTFUNC( CEnvScreenOverlay, FIELD_VOID, "StartOverlays", InputStartOverlay ),

END_DATADESC()

void SendProxy_String_tToString( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	string_t *pString = (string_t*)pData;
	pOut->m_pString = (char*)STRING( *pString );
}

IMPLEMENT_SERVERCLASS_ST( CEnvScreenOverlay, DT_EnvScreenOverlay )
	SendPropArray( SendPropString( SENDINFO_ARRAY( m_iszOverlayNames ), 0, SendProxy_String_tToString ), m_iszOverlayNames ),
	SendPropArray( SendPropFloat( SENDINFO_ARRAY( m_flOverlayTimes ), 10, SPROP_ROUNDDOWN ), m_flOverlayTimes ),
	SendPropFloat( SENDINFO(m_flStartTime), 10 ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvScreenOverlay::CEnvScreenOverlay( void )
{
	m_flStartTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvScreenOverlay::InputStartOverlay( inputdata_t &inputdata )
{
	if ( m_iszOverlayNames[0] == NULL_STRING )
	{
		Warning("env_screenoverlay %s has no overlays to display.\n", STRING(GetEntityName()) );
		return;
	}

	m_flStartTime = gpGlobals->curtime;
}
