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
#include "c_te_legacytempents.h"

//-----------------------------------------------------------------------------
// Purpose: Kills Player Attachments
//-----------------------------------------------------------------------------
class C_TEKillPlayerAttachments : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEKillPlayerAttachments, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEKillPlayerAttachments( void );
	virtual			~C_TEKillPlayerAttachments( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nPlayer;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEKillPlayerAttachments::C_TEKillPlayerAttachments( void )
{
	m_nPlayer = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEKillPlayerAttachments::~C_TEKillPlayerAttachments( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEKillPlayerAttachments::PostDataUpdate( DataUpdateType_t updateType )
{
	tempents->KillAttachedTents( m_nPlayer );
}

void TE_KillPlayerAttachments( IRecipientFilter& filter, float delay,
	int player )
{
	tempents->KillAttachedTents( player );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEKillPlayerAttachments, DT_TEKillPlayerAttachments, CTEKillPlayerAttachments)
	RecvPropInt( RECVINFO(m_nPlayer)),
END_RECV_TABLE()
