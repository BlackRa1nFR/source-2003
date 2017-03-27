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
#include "IEffects.h"

//-----------------------------------------------------------------------------
// Purpose: Smoke TE
//-----------------------------------------------------------------------------
class C_TESmoke : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TESmoke, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TESmoke( void );
	virtual			~C_TESmoke( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	int				m_nModelIndex;
	float			m_fScale;
	int				m_nFrameRate;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESmoke::C_TESmoke( void )
{
	m_vecOrigin.Init();
	m_nModelIndex = 0;
	m_fScale = 0;
	m_nFrameRate = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESmoke::~C_TESmoke( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TESmoke::PostDataUpdate( DataUpdateType_t updateType )
{
	// The number passed down is 10 times smaller...
	g_pEffects->Smoke( m_vecOrigin, m_nModelIndex, m_fScale * 10.0f, m_nFrameRate );	
}

void TE_Smoke( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float scale, int framerate )
{
	// The number passed down is 10 times smaller...
	g_pEffects->Smoke( *pos, modelindex, scale * 10.0f, framerate );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TESmoke, DT_TESmoke, CTESmoke)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropFloat( RECVINFO(m_fScale )),
	RecvPropInt( RECVINFO(m_nFrameRate)),
END_RECV_TABLE()
