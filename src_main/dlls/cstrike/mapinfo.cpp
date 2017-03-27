//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "mapinfo.h"


LINK_ENTITY_TO_CLASS( info_map_parameters, CMapInfo );


CMapInfo *g_pMapInfo = NULL;


CMapInfo::CMapInfo()
{
	if ( g_pMapInfo )
	{
		// Should only be one of these.
		Warning( "Warning: Multiple info_map_parameters entities in map!\n" );
	}
	else
	{
		g_pMapInfo = this;
	}
}


CMapInfo::~CMapInfo()
{
	if ( g_pMapInfo == this )
		g_pMapInfo = NULL;
}
 

bool CMapInfo::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "buying"))
	{
		m_iBuyingStatus = atoi(szValue);
		return true;
	}
	else if (FStrEq(szKeyName, "bombradius"))
	{
		m_flBombRadius = (float)(atoi(szValue));
		if (m_flBombRadius > 2048)
			m_flBombRadius = 2048;
		
		return true;
	}
	
	return false;
}


void CMapInfo::Spawn( void )
{ 
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	m_fEffects |= EF_NODRAW;
}
