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
#include "iefx.h"

// UNDONE:  Get rid of this?
#define FDECAL_PERMANENT			0x01

//-----------------------------------------------------------------------------
// Purpose: BSP Decal TE
//-----------------------------------------------------------------------------
class C_TEBSPDecal : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEBSPDecal, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEBSPDecal( void );
	virtual			~C_TEBSPDecal( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecOrigin;
	int				m_nEntity;
	int				m_nIndex;

	const ConVar	*m_pDecals;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBSPDecal::C_TEBSPDecal( void )
{
	m_vecOrigin.Init();
	m_nEntity = 0;
	m_nIndex = 0;

	m_pDecals = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBSPDecal::~C_TEBSPDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEBSPDecal::Precache( void )
{
	m_pDecals = cvar->FindVar( "r_decals" );
}

void TE_BSPDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int entity, int index )
{
	C_BaseEntity *ent;
	if ( !( ent = cl_entitylist->GetEnt( entity ) ) )
	{
		DevMsg( 1, "Decal: entity = %i", entity );
		return;
	}

	const ConVar *decals = cvar->FindVar( "r_decals" );

	if ( decals && decals->GetInt() )
	{
		effects->DecalShoot( index, entity, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), *pos, 0, FDECAL_PERMANENT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBSPDecal::PostDataUpdate( DataUpdateType_t updateType )
{
	C_BaseEntity *ent;
	if ( !( ent = cl_entitylist->GetEnt( m_nEntity ) ) )
	{
		DevMsg( 1, "Decal: entity = %i", m_nEntity );
		return;
	}

	if ( m_pDecals && m_pDecals->GetInt() )
	{
		effects->DecalShoot( m_nIndex, m_nEntity, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), m_vecOrigin, 0, FDECAL_PERMANENT );
	}
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBSPDecal, DT_TEBSPDecal, CTEBSPDecal)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(m_nEntity)),
	RecvPropInt( RECVINFO(m_nIndex)),
END_RECV_TABLE()

