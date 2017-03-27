//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Implements the Effects API
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
// Created:  YWB 9/5/2000
//=============================================================================


#include "quakedef.h"
#include "r_efx.h"
#include "r_efxextern.h"

// Effects API object.
static CVEfx efx;

// Engine internal accessor to effects api ( see cl_parsetent.cpp, etc. )
CVEfx *g_pEfx = &efx;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CVEfx::Draw_DecalIndexFromName( char *name )
{
	return ::Draw_DecalIndexFromName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : textureIndex - 
//			entity - 
//			modelIndex - 
//			position - 
//			flags - 
//-----------------------------------------------------------------------------
void CVEfx::DecalShoot( int textureIndex, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector *saxis, int flags)
{
	color32 white = {255,255,255,255};
	DecalColorShoot( textureIndex, entity, model, model_origin, model_angles, position, saxis, flags, white );
}

void CVEfx::DecalColorShoot( int textureIndex, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
	const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor)
{
	Vector localPosition = position;
	if ( entity ) 	// Not world?
	{
		matrix3x4_t matrix;
		AngleMatrix( model_angles, model_origin, matrix );
		VectorITransform( position, matrix, localPosition );
	}

	::R_DecalShoot( textureIndex, entity, model, localPosition, saxis, flags, rgbaColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *material - 
//			userdata - 
//			entity - 
//			*model - 
//			position - 
//			*saxis - 
//			flags - 
//			&rgbaColor - 
//-----------------------------------------------------------------------------
void CVEfx::PlayerDecalShoot( IMaterial *material, void *userdata, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
	const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor )
{
	Vector localPosition = position;
	if ( entity ) 	// Not world?
	{
		matrix3x4_t matrix;
		AngleMatrix( model_angles, model_origin, matrix );
		VectorITransform( position, matrix, localPosition );
	}

	R_PlayerDecalShoot( material, userdata, entity, model, position, saxis, flags, rgbaColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : key - 
// Output : dlight_t
//-----------------------------------------------------------------------------
dlight_t *CVEfx::CL_AllocDlight( int key )
{
	return ::CL_AllocDlight( key );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : key - 
// Output : dlight_t
//-----------------------------------------------------------------------------
dlight_t *CVEfx::CL_AllocElight( int key )
{
	return ::CL_AllocElight( key );
}

// Expose it to the client .dll
EXPOSE_SINGLE_INTERFACE( CVEfx, IVEfx, VENGINE_EFFECTS_INTERFACE_VERSION );
