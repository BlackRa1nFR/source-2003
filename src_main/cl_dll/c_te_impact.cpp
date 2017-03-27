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

extern IPhysicsSurfaceProps *physprops;

class C_TEImpact : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEImpact, C_BaseTempEntity );
	
	DECLARE_CLIENTCLASS();

	C_TEImpact( void );
	virtual	~C_TEImpact( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual void	Precache( void );

	virtual void	PlayImpactSound( trace_t &tr );
	virtual void	PerformCustomEffects( trace_t &tr, Vector &shotDir );
public:
	Vector			m_vecOrigin;
	Vector			m_vecNormal;
	int				m_iType;
	byte			m_ucFlags;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
C_TEImpact::C_TEImpact( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	
	m_iType		= -1;
	m_ucFlags	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
C_TEImpact::~C_TEImpact( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEImpact::Precache( void )
{
	//TODO: Precache all materials/sounds used by impacts here
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : unused - 
//-----------------------------------------------------------------------------
void C_TEImpact::PostDataUpdate( DataUpdateType_t updateType )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEImpact::PlayImpactSound( trace_t &tr )
{
}

//-----------------------------------------------------------------------------
// Purpose: Perform custom effects based on the Decal index
//-----------------------------------------------------------------------------
void C_TEImpact::PerformCustomEffects( trace_t &tr, Vector &shotDir )
{
}

//Receive data table
IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TEImpact, DT_TEImpact, CTEImpact)
	RecvPropVector( RECVINFO( m_vecOrigin ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iType ) ),
	RecvPropInt( RECVINFO( m_ucFlags ) ),
END_RECV_TABLE()
