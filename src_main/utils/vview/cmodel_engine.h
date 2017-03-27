//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef CMODEL_ENGINE_H
#define CMODEL_ENGINE_H
#pragma once

#ifndef CMODEL_H
#include "cmodel.h"
#endif

cmodel_t	*CM_LoadMap( char *name, qboolean clientload, unsigned *checksum );
cmodel_t	*CM_InlineModel( char *name );	// *1, *2, etc
cmodel_t	*CM_InlineModelNumber( int index );	// 1, 2, etc

int			CM_NumClusters( void );
int			CM_NumInlineModels( void );
char		*CM_EntityString( void );

// creates a clipping hull for an arbitrary box
int			CM_HeadnodeForBox( Vector& mins, Vector& maxs );


// returns an ORed contents mask
int			CM_PointContents( Vector& p, int headnode );
int			CM_TransformedPointContents( Vector& p, int headnode, Vector& origin, Vector& angles );

trace_t		CM_BoxTrace (const Vector& start, const Vector& end,
						  const Vector& mins, const Vector& maxs,
						  int headnode, int brushmask);
trace_t		CM_TransformedBoxTrace( Vector& start, Vector& end,
						  Vector& mins, Vector& maxs,
						  int headnode, int brushmask,
						  Vector& origin, Vector& angles );

byte		*CM_ClusterPVS( int cluster );
byte		*CM_ClusterPAS( int cluster );

int			CM_PointLeafnum( Vector& p );

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int			CM_BoxLeafnums( Vector& mins, Vector& maxs, int *list,
							int listsize, int *topnode );

int			CM_LeafContents( int leafnum );
int			CM_LeafCluster( int leafnum );
int			CM_LeafArea( int leafnum );

void		CM_SetAreaPortalState( int portalnum, qboolean open );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );
qboolean	CM_HeadnodeVisible( int headnode, byte *visbits );

#if 0
void		CM_WritePortalState( FILE *f );
void		CM_ReadPortalState( FILE *f );
#endif

#endif // CMODEL_ENGINE_H
