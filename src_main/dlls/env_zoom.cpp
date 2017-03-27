//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

class CEnvZoom : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvZoom, CPointEntity );

	void	InputZoom( inputdata_t &inputdata );

private:

	float	m_flSpeed;
	int		m_nFOV;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( env_zoom, CEnvZoom );

BEGIN_DATADESC( CEnvZoom )

	DEFINE_KEYFIELD( CEnvZoom, m_flSpeed, FIELD_FLOAT, "Rate" ),
	DEFINE_KEYFIELD( CEnvZoom, m_nFOV, FIELD_INTEGER, "FOV" ),

	DEFINE_INPUTFUNC( CEnvZoom, FIELD_VOID, "Zoom", InputZoom ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvZoom::InputZoom( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 )));

	if ( pPlayer != NULL )
	{
		//Stuff the values
		pPlayer->SetFOV( m_nFOV, m_flSpeed );
	}
}
