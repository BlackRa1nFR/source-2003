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
// Purpose: Breakable Model TE
//-----------------------------------------------------------------------------
class C_TEBreakModel : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEBreakModel, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEBreakModel( void );
	virtual			~C_TEBreakModel( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	Vector			m_vecSize;
	Vector			m_vecVelocity;
	int				m_nRandomization;
	int				m_nModelIndex;
	int				m_nCount;
	float			m_fTime;
	int				m_nFlags;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBreakModel::C_TEBreakModel( void )
{
	m_vecOrigin.Init();
	m_vecSize.Init();
	m_vecVelocity.Init();
	m_nModelIndex		= 0;
	m_nRandomization	= 0;
	m_nCount			= 0;
	m_fTime				= 0.0;
	m_nFlags			= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBreakModel::~C_TEBreakModel( void )
{
}

void TE_BreakModel( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* size, const Vector* vel, int modelindex, int randomization,
	int count, float time, int flags )
{
	tempents->BreakModel( *pos, *size, *vel,
		randomization, time, count, modelindex, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBreakModel::PostDataUpdate( DataUpdateType_t updateType )
{
	tempents->BreakModel( m_vecOrigin, m_vecSize, m_vecVelocity,
		m_nRandomization, m_fTime, m_nCount, m_nModelIndex, m_nFlags );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBreakModel, DT_TEBreakModel, CTEBreakModel)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecSize)),
	RecvPropVector( RECVINFO(m_vecVelocity)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nRandomization)),
	RecvPropInt( RECVINFO(m_nCount)),
	RecvPropFloat( RECVINFO(m_fTime)),
	RecvPropInt( RECVINFO(m_nFlags)),
END_RECV_TABLE()

