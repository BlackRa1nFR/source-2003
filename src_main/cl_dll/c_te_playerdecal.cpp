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
#include "decals.h"
#include "materialsystem/IMaterialSystem.h"

//-----------------------------------------------------------------------------
// Purpose: Player Decal TE
//-----------------------------------------------------------------------------
class C_TEPlayerDecal : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerDecal, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEPlayerDecal( void );
	virtual			~C_TEPlayerDecal( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	int				m_nPlayer;
	Vector			m_vecOrigin;
	int				m_nEntity;

	const ConVar	*m_pDecals;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEPlayerDecal::C_TEPlayerDecal( void )
{
	m_nPlayer = 0;
	m_vecOrigin.Init();
	m_nEntity = 0;

	m_pDecals = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEPlayerDecal::~C_TEPlayerDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEPlayerDecal::Precache( void )
{
	m_pDecals = cvar->FindVar( "r_decals" );
}

char *COM_BinPrintf( unsigned char *buf, int nLen );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			delay - 
//			pos - 
//			player - 
//			entity - 
//-----------------------------------------------------------------------------
void TE_PlayerDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int player, int entity  )
{
	// No valid target?
	C_BaseEntity *ent = cl_entitylist->GetEnt( entity );
	if ( !ent )
		return;

	// Find player logo for shooter
	hud_player_info_t info;
	engine->GetPlayerInfo( player, &info );

	// Doesn't have a logo
	if ( !info.logo )
		return;

	bool found = false;
	IMaterial *logo = materials->FindMaterial( "decals/playerlogo", &found );
	if ( !found )
		return;

	// See if logo has been downloaded.
	char texname[ 512 ];
	Q_snprintf( texname, sizeof( texname ), "decals/downloads/%s",
		COM_BinPrintf( (byte *)&info.logo, 4 ) );

	ITexture *texture = materials->FindTexture( texname, &found );
	if ( !found ) 
		return;

	if ( !texture )
	{
		Assert( 0 );
		return;
	}

	color32 rgbaColor = { 255, 255, 255, 255 };
	effects->PlayerDecalShoot( 
		logo, 
		(void *)player,
		entity, 
		ent->GetModel(), 
		ent->GetAbsOrigin(), 
		ent->GetAbsAngles(), 
		*pos, 
		0, 
		0,
		rgbaColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEPlayerDecal::PostDataUpdate( DataUpdateType_t updateType )
{
	// Decals disabled?
	if ( m_pDecals && !m_pDecals->GetBool() )
		return;

	CLocalPlayerFilter filter;
	TE_PlayerDecal(  filter, 0.0f, &m_vecOrigin, m_nPlayer, m_nEntity );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEPlayerDecal, DT_TEPlayerDecal, CTEPlayerDecal)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(m_nEntity)),
	RecvPropInt( RECVINFO(m_nPlayer)),
END_RECV_TABLE()
