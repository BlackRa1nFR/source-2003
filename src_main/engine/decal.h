#include "gl_model_private.h"

#ifndef DECAL_H
#define DECAL_H
#pragma once

#ifndef DRAW_H
#include "draw.h"
#endif

void Decal_Init( void );
void Decal_Shutdown( void );

extern IMaterial			*Draw_DecalMaterial( int index );
extern int					Draw_DecalIndexFromName( char *name );
extern void					Draw_DecalSetName( int decal, char *name );
extern void					R_DecalShoot( int textureIndex, int entity, const model_t *model, const Vector &position, const float *saxis, int flags, const color32 &rgbaColor );
extern int					Draw_DecalMax( void );


#endif		// DECAL_H
