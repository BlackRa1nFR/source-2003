#if !defined ( EFXH )
#define EFXH
#ifdef _WIN32
#pragma once
#endif

#include "iefx.h"

class IMaterial;

class CVEfx : public IVEfx
{
public:
	int				Draw_DecalIndexFromName	( char *name );
	void			DecalShoot				( int textureIndex, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector *saxis, int flags);
	void			DecalColorShoot			( int textureIndex, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor);
	void			PlayerDecalShoot		( IMaterial *material, void *userdata, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
													const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor );
	struct dlight_s	*CL_AllocDlight			( int key );
	struct dlight_s	*CL_AllocElight			( int key );
};

extern CVEfx *g_pEfx;

#endif