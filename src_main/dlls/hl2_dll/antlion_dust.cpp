#include "cbase.h"
#include "antlion_dust.h"

IMPLEMENT_SERVERCLASS_ST( CTEAntlionDust, DT_TEAntlionDust )
	SendPropVector( SENDINFO(m_vecOrigin)),
	SendPropVector( SENDINFO(m_vecAngles)),	
END_SEND_TABLE()

CTEAntlionDust::CTEAntlionDust( const char *name ) : BaseClass( name )
{
}

CTEAntlionDust::~CTEAntlionDust( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Create the temp ent
//-----------------------------------------------------------------------------
void CTEAntlionDust::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, (void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

static CTEAntlionDust g_TEAntlionDust( "AntlionDust" );

//-----------------------------------------------------------------------------
// Purpose: Creates antlion dust effect
// Input  : &origin - position
//			&angles - angles
//-----------------------------------------------------------------------------
void UTIL_CreateAntlionDust( const Vector &origin, const QAngle &angles )
{
	g_TEAntlionDust.m_vecOrigin = origin;
	g_TEAntlionDust.m_vecAngles = angles;

	//Send it
	CPVSFilter filter( origin );
	g_TEAntlionDust.Create( filter, 0.0f );
}
