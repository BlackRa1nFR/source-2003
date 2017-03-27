//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
// Implementation of the main material system interface
//=============================================================================

#if !defined( IEFX_H )
#define IEFX_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "vector.h"

struct model_t;

class IMaterial;

//-----------------------------------------------------------------------------
// Purpose: Exposes effects api to client .dll
//-----------------------------------------------------------------------------
class IVEfx
{
public:
	// Retrieve decal texture index from decal by name
	virtual	int				Draw_DecalIndexFromName	( char *name ) = 0;

	// Apply decal
	virtual	void			DecalShoot				( int textureIndex, int entity, 
		const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
		const Vector& position, const Vector *saxis, int flags ) = 0;

	// Apply colored decal
	virtual	void			DecalColorShoot				( int textureIndex, int entity, 
		const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
		const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor  ) = 0;

	virtual void			PlayerDecalShoot( IMaterial *material, void *userdata, int entity, const model_t *model, 
		const Vector& model_origin, const QAngle& model_angles, 
		const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor ) = 0;

	// Allocate a dynamic world light ( key is the entity to whom it is associated )
	virtual	struct dlight_s	*CL_AllocDlight			( int key ) = 0;

	// Allocate a dynamic entity light ( key is the entity to whom it is associated )
	virtual	struct dlight_s	*CL_AllocElight			( int key ) = 0;
};

#define VENGINE_EFFECTS_INTERFACE_VERSION "VEngineEffects001"

extern IVEfx *effects;

#endif // IEFX_H