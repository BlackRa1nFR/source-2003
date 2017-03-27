//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ai_nav_property.h"

#define SF_NAV_START_ON		0x0001

class CLogicNavigation : public CLogicalEntity
{
	DECLARE_CLASS( CLogicNavigation, CLogicalEntity );

	bool KeyValue( const char *szKeyName, const char *szValue );
	void Activate( void );

private:
	// Inputs
	void InputTurnOn( inputdata_t &inputdata ) { TurnOn(); }
	void InputTurnOff( inputdata_t &inputdata ) { TurnOff(); }
	void InputToggle( inputdata_t &inputdata ) 
	{ 
		if ( m_isOn )
			TurnOff();
		else
			TurnOn();
	}

	void TurnOn();
	void TurnOff();

	DECLARE_DATADESC();

	bool				m_isOn;
	navproperties_t		m_navProperty;
};

LINK_ENTITY_TO_CLASS(logic_navigation, CLogicNavigation);


BEGIN_DATADESC( CLogicNavigation )

	DEFINE_FIELD( CLogicNavigation, m_isOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CLogicNavigation, m_navProperty, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC(CLogicNavigation, FIELD_VOID, "TurnOn", InputTurnOn),
	DEFINE_INPUTFUNC(CLogicNavigation, FIELD_VOID, "TurnOff", InputTurnOff),
	DEFINE_INPUTFUNC(CLogicNavigation, FIELD_VOID, "Toggle", InputToggle),

END_DATADESC()



bool CLogicNavigation::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "navprop") )
	{
		if ( FStrEq( szValue, "Ignore" ) )
		{
			m_navProperty = NAV_IGNORE;
		}
		else
		{
			DevMsg( 1, "Unknown nav property %s\n", szValue );
		}
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

void CLogicNavigation::Activate()
{
	BaseClass::Activate();
	
	if ( HasSpawnFlags( SF_NAV_START_ON ) )
	{
		TurnOn();
	}
}

void CLogicNavigation::TurnOn()
{
	if ( m_isOn )
		return;

	m_isOn = true;
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByName( pEntity, STRING(m_target), NULL ) ) != NULL )
	{
		NavPropertyAdd( pEntity, NAV_IGNORE );
	}
}

void CLogicNavigation::TurnOff()
{
	if ( !m_isOn )
		return;
	m_isOn = false;
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByName( pEntity, STRING(m_target), NULL ) ) != NULL )
	{
		NavPropertyRemove( pEntity, NAV_IGNORE );
	}
}

