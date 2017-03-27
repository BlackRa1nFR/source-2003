//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "terrainmodmgr.h"
#include "terrainmodmgr_shared.h"
#include "gameinterface.h"
#include "player.h"
#include "usermessages.h"

void EncodeTerrainMod( 
	unsigned char type, 
	const Vector &vCenter, 
	const Vector &vNormal,
	unsigned short usRadius,
	const Vector &vecMin,
	const Vector &vecMax,
	float flStrength,
	unsigned char ucFlags )
{
	WRITE_BYTE( (unsigned char)type );

	WRITE_FLOAT( vCenter.x );
	WRITE_FLOAT( vCenter.y );
	WRITE_FLOAT( vCenter.z );

	WRITE_SHORT( usRadius );
	WRITE_FLOAT( vecMin.x );
	WRITE_FLOAT( vecMin.y );
	WRITE_FLOAT( vecMin.z );
	WRITE_FLOAT( vecMax.x );
	WRITE_FLOAT( vecMax.y );
	WRITE_FLOAT( vecMax.z );

	WRITE_FLOAT( flStrength );
	WRITE_BYTE( ucFlags );

	if( type == TMod_Suck && (ucFlags & CTerrainModParams::TMOD_SUCKTONORMAL) )
	{
		WRITE_FLOAT( vNormal.x );
		WRITE_FLOAT( vNormal.y );
		WRITE_FLOAT( vNormal.z );
	}
}


void TerrainMod_Add( TerrainModType type, const CTerrainModParams &params )
{
	// NOTE! a sudden urge to touch the great speckled green monkey must be curbed!

	unsigned short usRadius = (unsigned short)RemapVal( params.m_flRadius, MIN_TMOD_RADIUS, MAX_TMOD_RADIUS, 0, 65535.9f );
	unsigned char ucFlags = (unsigned char)params.m_Flags;

	// Write it to all clients and to the signon message.
	CRecipientFilter everyone;
	everyone.AddAllPlayers();

	UserMessageBegin( everyone, "TerrainMod" );
		EncodeTerrainMod( type, params.m_vCenter, params.m_vNormal, usRadius, params.m_vecMin, params.m_vecMax, params.m_flStrength, ucFlags );
	MessageEnd();

	CRecipientFilter initFilter;
	initFilter.MakeInitMessage();

	UserMessageBegin( initFilter, "TerrainMod" );
		EncodeTerrainMod( type, params.m_vCenter, params.m_vNormal, usRadius, params.m_vecMin, params.m_vecMax, params.m_flStrength, ucFlags );
	MessageEnd();


	// Make sure we have the exact same values as the client does.
	CTerrainModParams unpackedParams;
	unpackedParams.m_vCenter = params.m_vCenter;
	unpackedParams.m_flRadius = RemapVal( usRadius, 0, 65535, MIN_TMOD_RADIUS, MAX_TMOD_RADIUS );
	unpackedParams.m_vecMin = params.m_vecMin;
	unpackedParams.m_vecMax = params.m_vecMax;
	unpackedParams.m_flStrength = params.m_flStrength;
	unpackedParams.m_Flags = ucFlags;
	unpackedParams.m_vNormal = params.m_vNormal;

	// Move players out of the way so they don't get stuck on the terrain.
	float playerStartHeights[MAX_PLAYERS];

	int i;
	int nPlayers = min( MAX_PLAYERS, gpGlobals->maxClients );
	for( i=0; i < nPlayers; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i+1 );
	
		if( !pPlayer )
			continue;

		playerStartHeights[i] = pPlayer->GetAbsOrigin().z;

		// Cast a ray upwards to see if we can move the player out of the way.
		trace_t trace;
		UTIL_TraceEntity( 
			pPlayer, pPlayer->GetAbsOrigin(),
			pPlayer->GetAbsOrigin() + Vector( 0, 0, unpackedParams.m_flRadius*2 ),
			MASK_SOLID,
			&trace );

		pPlayer->SetLocalOrigin( trace.endpos );
	}
	
	// Apply the mod.
	engine->ApplyTerrainMod( type, unpackedParams );
	
	// Move players back down.
	for( i=0; i < nPlayers; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i+1 );
	
		if( !pPlayer )
			continue;
		
		// Cast a ray upwards to see if we can move the player out of the way.
		trace_t trace;
		UTIL_TraceEntity( 
			pPlayer,
			pPlayer->GetAbsOrigin(),
			Vector( pPlayer->GetAbsOrigin().x, pPlayer->GetAbsOrigin().y, playerStartHeights[i] ),
			MASK_SOLID,
			&trace );

		pPlayer->SetLocalOrigin( trace.endpos );
	}
}

//-----------------------------------------------------------------------------
// Purpose: This is the terrain mod used by objects, not players.
//-----------------------------------------------------------------------------
void TerrainMod_Apply( TerrainModType type, const CTerrainModParams &params )
{
	unsigned short usRadius = (unsigned short)RemapVal( params.m_flRadius, MIN_TMOD_RADIUS, MAX_TMOD_RADIUS, 0, 65535.9f );
	unsigned char ucFlags = (unsigned char)params.m_Flags;

	// Write it to all clients and to the signon message.
	CRecipientFilter everyone;
	everyone.AddAllPlayers();

	UserMessageBegin( everyone, "TerrainMod" );
		EncodeTerrainMod( type, params.m_vCenter, params.m_vNormal, usRadius, params.m_vecMin, params.m_vecMax, params.m_flStrength, ucFlags );
	MessageEnd();

	CRecipientFilter initFilter;
	initFilter.MakeInitMessage();

	UserMessageBegin( initFilter, "TerrainMod" );
		EncodeTerrainMod( type, params.m_vCenter, params.m_vNormal, usRadius, params.m_vecMin, params.m_vecMax, params.m_flStrength, ucFlags );
	MessageEnd();

	// Make sure we have the exact same values as the client does.
	CTerrainModParams unpackedParams;
	unpackedParams.m_vCenter = params.m_vCenter;
	unpackedParams.m_flRadius = RemapVal( usRadius, 0, 65535, MIN_TMOD_RADIUS, MAX_TMOD_RADIUS );
	unpackedParams.m_vecMin = params.m_vecMin;
	unpackedParams.m_vecMax = params.m_vecMax;
	unpackedParams.m_flStrength = params.m_flStrength;
	unpackedParams.m_Flags = ucFlags;
	unpackedParams.m_vNormal = params.m_vNormal;

	// Move players out of the way so they don't get stuck on the terrain.
	float playerStartHeights[MAX_PLAYERS];

	int i;
	int nPlayers = min( MAX_PLAYERS, gpGlobals->maxClients );
	for( i=0; i < nPlayers; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i+1 );
	
		if( !pPlayer )
			continue;

		playerStartHeights[i] = pPlayer->GetAbsOrigin().z;

		// Cast a ray upwards to see if we can move the player out of the way.
		trace_t trace;
		UTIL_TraceEntity( 
			pPlayer,
			pPlayer->GetAbsOrigin(),
			pPlayer->GetAbsOrigin() + Vector( 0, 0, unpackedParams.m_flRadius*2 ),
			MASK_SOLID,
			&trace );

		pPlayer->SetLocalOrigin( trace.endpos );
	}
	
	
	// Apply the mod.
	engine->ApplyTerrainMod( type, unpackedParams );

	
	// Move players back down.
	for( i=0; i < nPlayers; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i+1 );
	
		if( !pPlayer )
			continue;
		
		// Cast a ray upwards to see if we can move the player out of the way.
		trace_t trace;
		UTIL_TraceEntity( 
			pPlayer,
			pPlayer->GetAbsOrigin(),
			Vector( pPlayer->GetAbsOrigin().x, pPlayer->GetAbsOrigin().y, playerStartHeights[i] ),
			MASK_SOLID,
			&trace );

		pPlayer->SetLocalOrigin( trace.endpos );
	}
}
