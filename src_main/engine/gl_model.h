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
// $NoKeywords: $
//=============================================================================

#ifndef GL_MODEL_H
#define GL_MODEL_H

#ifdef _WIN32
#pragma once
#endif

struct mnode_t;
struct mleaf_t;
struct mtexinfo_t;
typedef struct mtexdata_s mtexdata_t;
struct model_t;

struct dvis_t;
struct dworldlight_t;
struct decallist_t;

bool	Mod_LoadStudioModel( model_t *mod, void *buffer, bool zerostructure );
void	Mod_UnloadStudioModel(model_t *mod);
void	Mod_ReleaseStudioModel(model_t* mod);

void	Map_VisClear( void );
void	Map_VisSetup( model_t *worldmodel, int visorigincount, const Vector origins[], bool forcenovis = false );
byte 	*Map_VisCurrent( void );
int		Map_VisCurrentCluster( void );

extern int			DecalListCreate( decallist_t *pList );

extern int		r_visframecount;

#include "modelloader.h"

#endif // GL_MODEL_H
