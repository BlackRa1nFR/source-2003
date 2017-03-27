//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
// $NoKeywords: $
//=============================================================================

#ifndef CMODEL_ENGINE_H
#define CMODEL_ENGINE_H

#ifdef _WIN32
#pragma once
#endif


#include "cmodel_private.h"

class ICollideable;


cmodel_t	*CM_LoadMap( const char *name, qboolean clientload, unsigned *checksum );
void		CM_FreeMap( void );
cmodel_t	*CM_InlineModel( const char *name );	// *1, *2, etc
cmodel_t	*CM_InlineModelNumber( int index );	// 1, 2, etc
int			CM_InlineModelContents( int index );	// 1, 2, etc

int			CM_NumClusters( void );
char		*CM_EntityString( void );

// creates a clipping hull for an arbitrary box
int			CM_HeadnodeForBoxHull( const Vector& mins, const Vector& maxs );


// returns an ORed contents mask
int			CM_PointContents( const Vector &p, int headnode );
int			CM_TransformedPointContents( const Vector& p, int headnode, const Vector& origin, const QAngle& angles );

// sets the default values in a trace
void		CM_ClearTrace( trace_t *trace );

byte		*CM_ClusterPVS( int cluster );
byte		*CM_ClusterPAS( int cluster );
byte		*CM_Vis( byte *dest, int cluster, int visType );

int			CM_PointLeafnum( const Vector& p );

// This builds a subtree that lies within the bounding volume
void		CM_BuildBSPSubTree( const Vector& mins, const Vector& maxs );

// This here hooks in/unhooks the subtree for all computations
void		CM_BeginBSPSubTree( );
void		CM_EndBSPSubTree( );

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int			CM_BoxLeafnums( const Vector& mins, const Vector& maxs, int *list,
							int listsize, int *topnode );
int			CM_TransformedBoxContents( const Vector& pos, const Vector& mins, const Vector& maxs, int headnode, const Vector& origin, const QAngle& angles );

// Versions that accept rays...
void		CM_TransformedBoxTrace (const Ray_t& ray, int headnode, int brushmask, const Vector& origin, QAngle const& angles, trace_t& tr );
void		CM_BoxTrace (const Ray_t& ray, int headnode, int brushmask, bool computeEndpt, trace_t& tr );

int			CM_LeafContents( int leafnum );
int			CM_LeafCluster( int leafnum );
int			CM_LeafArea( int leafnum );

void		CM_SetAreaPortalState( int portalnum, int isOpen );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );

// Given a view origin (which tells us the area to start looking in) and a portal key,
// fill in the plane that leads out of this area (it points into whatever area it leads to).
bool		CM_GetAreaPortalPlane( const Vector &vViewOrigin, int portalKey, VPlane *pPlane );

qboolean	CM_HeadnodeVisible( int headnode, byte *visbits );
// Test to see if the given box is in the given PVS/PAS
int			CM_BoxVisible( const Vector& mins, const Vector& maxs, byte *visbits );

typedef struct cmodel_collision_s cmodel_collision_t;
vcollide_t *CM_GetVCollide( int modelIndex );
vcollide_t* CM_VCollideForModel( int modelindex, model_t* pModel );

void		CM_ApplyTerrainMod( ITerrainMod *pMod );

void		CM_WorldSpaceCenter( ICollideable *pCollideable, Vector *pCenter );
void		CM_WorldAlignBounds( ICollideable *pCollideable, Vector *pMins, Vector *pMaxs );

#endif // CMODEL_ENGINE_H
