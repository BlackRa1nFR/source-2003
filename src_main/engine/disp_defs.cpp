//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "disp_defs.h"


// ---------------------------------------------------------------------- //
// Global tables.
// ---------------------------------------------------------------------- //

// These map CCoreDispSurface neighbor orientations (which are actually edge indices)
// into our 'degrees of rotation' representation.
int g_CoreDispNeighborOrientationMap[4][4] =
{
	{ORIENTATION_CCW_180, ORIENTATION_CCW_270, ORIENTATION_CCW_0,   ORIENTATION_CCW_90},
	{ORIENTATION_CCW_90,  ORIENTATION_CCW_180, ORIENTATION_CCW_270, ORIENTATION_CCW_0},
	{ORIENTATION_CCW_0,   ORIENTATION_CCW_90,  ORIENTATION_CCW_180, ORIENTATION_CCW_270},
	{ORIENTATION_CCW_270, ORIENTATION_CCW_0,   ORIENTATION_CCW_90,  ORIENTATION_CCW_180}
};



// ---------------------------------------------------------------------- //
// Global variables.
// ---------------------------------------------------------------------- //

float						g_flDispToleranceSqr;
CUtlVector<unsigned char>	g_DispLMAlpha;
CUtlVector<unsigned char>	g_DispLightmapSamplePositions;
CUtlVector<CDispGroup*>		g_DispGroups;

ConVar						r_DispUpdateAll( "r_DispUpdateAll", "0" );
ConVar						r_DispFullRadius( "r_DispFullRadius", "400", 0, "Radius within which a displacement will stay at its highest LOD" );
ConVar						r_DispRadius( "r_DispRadius", "500" );
ConVar						r_DispTolerance("r_DispTolerance", "5");
ConVar						r_DispLockLOD( "r_DispLockLOD", "0" );
ConVar						r_DispEnableLOD( "r_DispEnableLOD", "0" );

CMemoryPool					g_DecalFragmentPool( sizeof(CDispDecalFragment), 64 );

bool						g_bDispOrthoRender = false;

ConVar						r_DrawDisp( "r_DrawDisp", "1" );

ConVar						r_DispWalkable( "r_DispWalkable", "0" );
ConVar						r_DispBuildable( "r_DispBuildable", "0" );
ConVar r_DispUseStaticMeshes( "r_DispUseStaticMeshes", "1", 0, "High end machines use static meshes. Low end machines use temp meshes." );
