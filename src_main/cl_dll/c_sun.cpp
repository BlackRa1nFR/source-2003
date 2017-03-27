//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_sun.h"



IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_Sun, DT_Sun, CSun )
	RecvPropInt( RECVINFO(m_clrRender), 0, RecvProxy_IntToColor32 ),
	RecvPropVector( RECVINFO( m_vDirection ) ),
	RecvPropInt( RECVINFO( m_nLayers ) ),
	RecvPropInt( RECVINFO( m_bOn ) ),
	RecvPropInt( RECVINFO( m_HorzSize ) ),
	RecvPropInt( RECVINFO( m_VertSize ) )
END_RECV_TABLE()



C_Sun::C_Sun()
{
	m_Overlay.m_bDirectional = true;
	m_Overlay.m_bInSky = true;
}


C_Sun::~C_Sun()
{
}


void C_Sun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Setup all our parameters.
	Vector vColor(
		m_clrRender->r / 255.0f,
		m_clrRender->g / 255.0f,
		m_clrRender->b / 255.0f );

	float sizeMul[] = {0.5, 0.3, 0.2, 0.1};

	m_Overlay.m_vDirection = m_vDirection;

	m_Overlay.m_nSprites = min( m_nLayers, CGlowOverlay::MAX_SPRITES );
	for( int i=0; i < m_Overlay.m_nSprites; i++ )
	{
		m_Overlay.m_Sprites[i].m_vColor = vColor;
		m_Overlay.m_Sprites[i].m_flHorzSize = (m_HorzSize / 100.0f) * sizeMul[i];
		m_Overlay.m_Sprites[i].m_flVertSize = (m_VertSize / 100.0f) * sizeMul[i];
	}

	// Either activate or deactivate.
	if( m_bOn )
		m_Overlay.Activate();
	else
		m_Overlay.Deactivate();
}



