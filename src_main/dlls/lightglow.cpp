//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "sendproxy.h"


class CLightGlow : public CBaseEntity
{
public:
	DECLARE_CLASS( CLightGlow, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

					CLightGlow();
					
	virtual void	Activate();


public:
	CNetworkVar( int, m_Size );
};


LINK_ENTITY_TO_CLASS( env_lightglow, CLightGlow );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CLightGlow, DT_LightGlow )
	SendPropInt( SENDINFO(m_clrRender), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropInt( SENDINFO(m_Size), 9, SPROP_UNSIGNED ),
	SendPropVector(SENDINFO(m_vecOrigin), -1,  SPROP_COORD ),
	SendPropEHandle (SENDINFO_NAME(m_hMoveParent, moveparent)),
END_SEND_TABLE()


BEGIN_DATADESC( CLightGlow )

	DEFINE_KEYFIELD( CLightGlow, m_Size,		FIELD_INTEGER,	"GlowSize" ),

END_DATADESC()


CLightGlow::CLightGlow()
{
	m_Size = 10;
	NetworkStateManualMode( true );
}


void CLightGlow::Activate()
{
	BaseClass::Activate();
	
	NetworkStateChanged();
}
