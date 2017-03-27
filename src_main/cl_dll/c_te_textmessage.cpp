//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_basetempentity.h"
#include "client_textmessage.h"

#define NETWORK_MESSAGE1 "__NETMESSAGE__1"
#define NETWORK_MESSAGE2 "__NETMESSAGE__2"
#define NETWORK_MESSAGE3 "__NETMESSAGE__3"
#define NETWORK_MESSAGE4 "__NETMESSAGE__4"
#define MAX_NETMESSAGE	4

static const char *gNetworkMessageNames[MAX_NETMESSAGE] = { NETWORK_MESSAGE1, NETWORK_MESSAGE2, NETWORK_MESSAGE3, NETWORK_MESSAGE4 };

void DispatchHudText( const char *pszName,  int iSize, void *pbuf );

//-----------------------------------------------------------------------------
// Purpose: TextMessage
//-----------------------------------------------------------------------------
class C_TETextMessage : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TETextMessage, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TETextMessage( void );
	virtual			~C_TETextMessage( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nChannel;
	float			x, y;
	int				m_nEffects;
	int				r1, g1, b1, a1;
	int				r2, g2, b2, a2;
	float			m_fFadeInTime;
	float			m_fFadeOutTime;
	float			m_fHoldTime;
	float			m_fFxTime;
	char			m_szMessage[ 150 ];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETextMessage::C_TETextMessage( void )
{
	m_nChannel = 0;
	x = 0.0;
	y = 0.0;
	m_nEffects = 0;
	r1 = g1 = b1 = a1 = 0;
	r2 = g2 = b2 = a2 = 0;
	m_fFadeInTime = 0.0;
	m_fFadeOutTime = 0.0;
	m_fHoldTime = 0.0;
	m_fFxTime = 0.0;
	memset( m_szMessage, 0, sizeof( m_szMessage ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETextMessage::~C_TETextMessage( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TETextMessage::PostDataUpdate( DataUpdateType_t updateType )
{
	int channel;
	client_textmessage_t *pNetMessage;

// Position command $position x y 
// x & y are from 0 to 1 to be screen resolution independent
// -1 means center in each dimension
// Effect command $effect <effect number>
// effect 0 is fade in/fade out
// effect 1 is flickery credits
// effect 2 is write out (training room)
// Text color r g b command $color
// Text color r g b command $color2
// fadein time fadeout time / hold time
// $fadein (message fade in time - per character in effect 2)
// $fadeout (message fade out time)
// $holdtime (stay on the screen for this long)
	channel = m_nChannel % MAX_NETMESSAGE;	// Pick the buffer
	
	pNetMessage = TextMessageGet( gNetworkMessageNames[ channel ] );
	if ( !pNetMessage || !pNetMessage->pMessage )
		return;

	pNetMessage->x = x;
	pNetMessage->y = y;

	pNetMessage->effect = m_nEffects;

	pNetMessage->r1 = r1;
	pNetMessage->g1 = g1;
	pNetMessage->b1 = b1;
	pNetMessage->a1 = a1;

	pNetMessage->r2 = r2;
	pNetMessage->g2 = g2;
	pNetMessage->b2 = b2;
	pNetMessage->a2 = a2;

	pNetMessage->fadein = m_fFadeInTime;
	pNetMessage->fadeout = m_fFadeOutTime;
	pNetMessage->holdtime = m_fHoldTime;

	if ( pNetMessage->effect == 2 )
		pNetMessage->fxtime = m_fFxTime;

	pNetMessage->pName = gNetworkMessageNames[ channel ];

	Q_strncpy( (char *)pNetMessage->pMessage, m_szMessage, sizeof( m_szMessage ) );

	DispatchHudText( "HudText", Q_strlen( pNetMessage->pName ) + 1, (void *)pNetMessage->pName );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TETextMessage, DT_TETextMessage, CTETextMessage)
	RecvPropInt( RECVINFO(m_nChannel)),
	RecvPropFloat( RECVINFO(x )),
	RecvPropFloat( RECVINFO(y )),
	RecvPropInt( RECVINFO(m_nEffects)),
	RecvPropInt( RECVINFO(r1)),
	RecvPropInt( RECVINFO(g1)),
	RecvPropInt( RECVINFO(b1)),
	RecvPropInt( RECVINFO(a1)),
	RecvPropInt( RECVINFO(r2)),
	RecvPropInt( RECVINFO(g2)),
	RecvPropInt( RECVINFO(b2)),
	RecvPropInt( RECVINFO(a2)),
	RecvPropFloat( RECVINFO(m_fFadeInTime )),
	RecvPropFloat( RECVINFO(m_fFadeOutTime )),
	RecvPropFloat( RECVINFO(m_fHoldTime )),
	RecvPropFloat( RECVINFO(m_fFxTime )),
	RecvPropString( RECVINFO_STRING(m_szMessage )),
END_RECV_TABLE()

void TE_TextMessage( IRecipientFilter& filter, float delay,
	const struct hudtextparms_s *tp, const char *pMessage )
{
	DevMsg( 1, "TE_TextMessage:  Not ported\n" );
}