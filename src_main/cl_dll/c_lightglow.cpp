//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "glow_overlay.h"


class C_LightGlow : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_LightGlow, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	
					C_LightGlow();


// C_BaseEntity overrides.
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );


public:
	
	int				m_Size;
	CGlowOverlay	m_Glow;
};

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_LightGlow, DT_LightGlow, CLightGlow )
	RecvPropInt( RECVINFO(m_clrRender), 0, RecvProxy_IntToColor32 ),
	RecvPropInt( RECVINFO( m_Size ) ),
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),
END_RECV_TABLE()



C_LightGlow::C_LightGlow()
{
	m_Size = 0;
	m_Glow.m_bDirectional = false;
	m_Glow.m_bInSky = false;
}


void C_LightGlow::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_Glow.m_vPos = GetAbsOrigin();

	if( updateType == DATA_UPDATE_CREATED )
	{
		// Setup our flare.
		Vector vColor(
			m_clrRender->r / 255.0f,
			m_clrRender->g / 255.0f,
			m_clrRender->b / 255.0f );

		m_Glow.m_nSprites = 2;

		m_Glow.m_Sprites[0].m_flHorzSize = m_Glow.m_Sprites[0].m_flVertSize = (float)m_Size / 100;
		m_Glow.m_Sprites[0].m_vColor = vColor;

		m_Glow.m_Sprites[1].m_flHorzSize = m_Glow.m_Sprites[1].m_flVertSize = (float)m_Size / 200;
		m_Glow.m_Sprites[1].m_vColor = vColor;

		m_Glow.Activate();
	}
}
