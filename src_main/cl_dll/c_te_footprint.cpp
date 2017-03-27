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

//-----------------------------------------------------------------------------
// Purpose: Footprint Decal TE
//-----------------------------------------------------------------------------

class C_TEFootprintDecal : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFootprintDecal, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEFootprintDecal( void );
	virtual			~C_TEFootprintDecal( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecOrigin;
	Vector			m_vecDirection;
	Vector			m_vecStart;
	int				m_nEntity;
	int				m_nIndex;
	char			m_chMaterialType;

	const ConVar	*m_pDecals;
};

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEFootprintDecal, DT_TEFootprintDecal, CTEFootprintDecal)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecDirection)),
	RecvPropInt( RECVINFO(m_nEntity)),
	RecvPropInt( RECVINFO(m_nIndex)),
	RecvPropInt( RECVINFO(m_chMaterialType)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

C_TEFootprintDecal::C_TEFootprintDecal( void )
{
	m_vecOrigin.Init();
	m_vecStart.Init();
	m_nEntity = 0;
	m_nIndex = 0;
	m_chMaterialType = 'C';

	m_pDecals = NULL;
}

C_TEFootprintDecal::~C_TEFootprintDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void C_TEFootprintDecal::Precache( void )
{
	m_pDecals = cvar->FindVar( "r_decals" );
}

//-----------------------------------------------------------------------------
// Do stuff when data changes
//-----------------------------------------------------------------------------

void C_TEFootprintDecal::PostDataUpdate( DataUpdateType_t updateType )
{
	// FIXME: Make this choose the decal based on material type
	if ( m_pDecals && m_pDecals->GetInt() )
	{
		C_BaseEntity *ent = cl_entitylist->GetEnt( m_nEntity );
		if ( ent )
		{
			effects->DecalShoot( m_nIndex, 
				m_nEntity, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), m_vecOrigin, &m_vecDirection, 0 );
		}
	}
}

void TE_FootprintDecal( IRecipientFilter& filter, float delay, const Vector *origin, const Vector* right, 
	int entity, int index, unsigned char materialType )
{
	const ConVar *decals =cvar->FindVar( "r_decals" );

	// FIXME: Make this choose the decal based on material type
	if ( decals && decals->GetInt() )
	{
		C_BaseEntity *ent = cl_entitylist->GetEnt( entity );
		if ( ent )
		{
			effects->DecalShoot( index, entity, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), *origin, right, 0 );
		}
	}
}