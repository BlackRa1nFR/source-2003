//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TERRAINMODMGR_H
#define TERRAINMODMGR_H
#ifdef _WIN32
#pragma once
#endif


#include "terrainmod.h"
#include "vector.h"

// Apply a terrain mod.
// The mod is applied on the server, and sent to all clients so they can apply it.
void TerrainMod_Add( TerrainModType type, const CTerrainModParams &params );
void TerrainMod_Apply( TerrainModType type, const CTerrainModParams &params );

#endif // TERRAINMODMGR_H
