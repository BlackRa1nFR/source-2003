#include "cbase.h"
#include "rotorwash.h"

IMPLEMENT_SERVERCLASS_ST( CTERotorWash, DT_TERotorWash )
	SendPropVector( SENDINFO(m_vecDown), -1,	SPROP_COORD ),
	SendPropFloat( SENDINFO(m_flMaxAltitude), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

static CTERotorWash g_TERotorWash( "RotorWash" );

CTERotorWash::CTERotorWash( const char *name ) : BaseClass( name )
{
	m_vecDown.Init();
	m_flMaxAltitude	= 0.0f;
}

CTERotorWash::~CTERotorWash( void )
{
}

void CTERotorWash::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, (void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

void UTIL_RotorWash( const Vector &origin, const Vector &direction, float maxDistance )
{
	g_TERotorWash.m_vecOrigin		= origin;
	g_TERotorWash.m_vecDown			= direction;
	g_TERotorWash.m_flMaxAltitude	= maxDistance;

	CPVSFilter filter( origin );
	g_TERotorWash.Create( filter, 0.0f );
}


//===============================================
//===============================================
class CEnvRotorwash : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvRotorwash, CPointEntity );

	DECLARE_DATADESC();

	// Input handlers
	void InputDoEffect( inputdata_t &inputdata );
};

LINK_ENTITY_TO_CLASS( env_rotorwash, CEnvRotorwash );

BEGIN_DATADESC( CEnvRotorwash )

	DEFINE_INPUTFUNC( CEnvRotorwash, FIELD_VOID, "DoEffect", InputDoEffect ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEnvRotorwash::InputDoEffect( inputdata_t &inputdata )
{
	UTIL_RotorWash( GetAbsOrigin(), Vector( 0, 0, -1 ), 512 );	
	UTIL_RotorWash( GetAbsOrigin(), Vector( 0, 0, -1 ), 512 );	
}
