//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================

#include "engine/ivmodelinfo.h"
#include "gl_model_private.h"
#include "modelloader.h"
#include "l_studio.h"
#include "cmodel_engine.h"
#include "server.h"
#include "r_local.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "lightcache.h"
#include "istudiorender.h"


//-----------------------------------------------------------------------------
// Gets the lighting center
//-----------------------------------------------------------------------------
static void R_StudioGetLightingCenter( studiohdr_t* pStudioHdr, const Vector& origin,
								const QAngle & angles, Vector* pLightingOrigin )
{
	Assert( pLightingOrigin );
	matrix3x4_t matrix;
	AngleMatrix( angles, origin, matrix );
	R_StudioCenter( pStudioHdr, matrix, *pLightingOrigin );
}

static int R_StudioBodyVariations( studiohdr_t *pstudiohdr )
{
	mstudiobodyparts_t *pbodypart;
	int i, count;

	if ( !pstudiohdr )
		return 0;

	count = 1;
	pbodypart = pstudiohdr->pBodypart( 0 );

	// Each body part has nummodels variations so there are as many total variations as there
	// are in a matrix of each part by each other part
	for ( i = 0; i < pstudiohdr->numbodyparts; i++ )
	{
		count = count * pbodypart[i].nummodels;
	}
	return count;
}

static int ModelFrameCount( model_t *model )
{
	int count = 1;

	if ( !model )
		return count;

	if ( model->type == mod_sprite )
	{
		return model->sprite.numframes;
	}
	else if ( model->type == mod_studio )
	{
		count = R_StudioBodyVariations( ( studiohdr_t * )modelloader->GetExtraData( model ) );
	}

	if ( count < 1 )
		count = 1;
	
	return count;
}



//-----------------------------------------------------------------------------
// shared implementation of IVModelInfo
//-----------------------------------------------------------------------------
class CModelInfo : public IVModelInfo
{
public:
	virtual const char *GetModelName( const model_t *model ) const;
	virtual void GetModelBounds( const model_t *model, Vector& mins, Vector& maxs ) const;
	virtual void GetModelRenderBounds( const model_t *model, int sequence, Vector& mins, Vector& maxs ) const;
	virtual int GetModelFrameCount( const model_t *model ) const;
	virtual int GetModelType( const model_t *model ) const;
	virtual void *GetModelExtraData( model_t const *model );
	virtual bool ModelHasMaterialProxy( const model_t *model ) const;
	virtual bool IsTranslucent( const model_t *model ) const;
	virtual bool IsModelVertexLit( const model_t *model ) const;
	virtual bool IsTranslucentTwoPass( const model_t *model ) const;
	virtual void RecomputeTranslucency( const model_t *model );
	virtual int	GetModelMaterialCount( const model_t *model ) const;
	virtual void GetModelMaterials( const model_t *model, int count, IMaterial** ppMaterials );
	virtual void GetIlluminationPoint( const model_t *model, const Vector& origin, 
		const QAngle& angles, Vector* pLightingOrigin );
	virtual int GetModelContents( int modelIndex ) const;
	vcollide_t *GetVCollide( const model_t *model ) const;
	vcollide_t *GetVCollide( int modelIndex ) const;
	virtual const char *GetModelKeyValueText( const model_t *model );
	virtual float GetModelRadius( const model_t *model );
	virtual studiohdr_t *GetStudiomodel( const model_t *mod );
};


const char *CModelInfo::GetModelName( const model_t *model ) const
{
	return model ? model->name : "?";
}

void CModelInfo::GetModelBounds( const model_t *model, Vector& mins, Vector& maxs ) const
{
	VectorCopy( model->mins, mins );
	VectorCopy( model->maxs, maxs );
}

void CModelInfo::GetModelRenderBounds( const model_t *model, int sequence, Vector& mins, Vector& maxs ) const
{
	if (!model)
	{
		mins.Init(0,0,0);
		maxs.Init(0,0,0);
		return;
	}

	switch( model->type )
	{
	case mod_studio:
		{
			studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( (model_t*)model );
			Assert( pStudioHdr );

			// NOTE: We're not looking at the sequence box here, although we could
			if (!VectorCompare( vec3_origin, pStudioHdr->view_bbmin ))
			{
				// clipping bounding box
				VectorCopy ( pStudioHdr->view_bbmin, mins);
				VectorCopy ( pStudioHdr->view_bbmin, maxs);
			}
			else
			{
				// movement bounding box
				VectorCopy ( pStudioHdr->hull_min, mins);
				VectorCopy ( pStudioHdr->hull_max, maxs);
			}

			// construct the base bounding box for this frame
			if ( sequence >= pStudioHdr->numseq) 
			{
				sequence = 0;
			}

			mstudioseqdesc_t *pseqdesc = pStudioHdr->pSeqdesc( sequence );
			VectorMin( pseqdesc->bbmin, mins, mins );
			VectorMax( pseqdesc->bbmax, maxs, maxs );
		}
		break;

	case mod_brush:
		VectorCopy( model->mins, mins );
		VectorCopy( model->maxs, maxs );
		break;

	default:
		mins.Init( 0, 0, 0 );
		maxs.Init( 0, 0, 0 );
		break;
	}
}

int CModelInfo::GetModelFrameCount( const model_t *model ) const
{
	return ModelFrameCount( ( model_t *)model );
}

int CModelInfo::GetModelType( const model_t *model ) const
{
	return model ? model->type : -1;
}

void *CModelInfo::GetModelExtraData( model_t const *model )
{
	return modelloader->GetExtraData( (model_t *)model );
}

bool CModelInfo::ModelHasMaterialProxy( const model_t *model ) const
{
	return (model && (model->flags & MODELFLAG_MATERIALPROXY));
}

bool CModelInfo::IsTranslucent( const model_t *model ) const
{
	return (model && (model->flags & MODELFLAG_TRANSLUCENT));
}

bool CModelInfo::IsModelVertexLit( const model_t *model ) const
{
	return (model && (model->flags & MODELFLAG_VERTEXLIT));
}

bool CModelInfo::IsTranslucentTwoPass( const model_t *model ) const
{
	return (model && (model->flags & MODELFLAG_TRANSLUCENT_TWOPASS));
}

void CModelInfo::RecomputeTranslucency( const model_t *model )
{
	if ( model )
		Mod_RecomputeTranslucency( (model_t *)model );
}

int	CModelInfo::GetModelMaterialCount( const model_t *model ) const
{
	if (!model)
		return 0;
	return Mod_GetMaterialCount( (model_t *)model );
}

void CModelInfo::GetModelMaterials( const model_t *model, int count, IMaterial** ppMaterials )
{
	if (model)
		Mod_GetModelMaterials( (model_t *)model, count, ppMaterials );
}

void CModelInfo::GetIlluminationPoint( const model_t *model, const Vector& origin, 
	const QAngle& angles, Vector* pLightingOrigin )
{
	Assert( model->type == mod_studio );
	studiohdr_t* pStudioHdr = (studiohdr_t*)GetModelExtraData(model);
	if (pStudioHdr)
		R_StudioGetLightingCenter( pStudioHdr, origin, angles, pLightingOrigin );
	else
		*pLightingOrigin = origin;
}

int CModelInfo::GetModelContents( int modelIndex ) const
{
	const model_t *pModel = GetModel( modelIndex );
	if ( pModel )
	{
		switch( pModel->type )
		{
		case mod_brush:
			return CM_InlineModelContents( modelIndex-1 );
		
		// BUGBUG: Studio contents?
		case mod_studio:
			return CONTENTS_SOLID;
		}
	}
	return 0;
}

vcollide_t *CModelInfo::GetVCollide( const model_t *model ) const
{
	if ( !model )
		return NULL;

	if ( model->type == mod_studio )
	{
		return Mod_VCollide( const_cast<model_t *>(model) );
	}

	int i = GetModelIndex( model->name );
	if ( i >= 0 )
	{
		return GetVCollide( i );
	}

	return NULL;
}

vcollide_t *CModelInfo::GetVCollide( int modelIndex ) const
{
	extern vcollide_t *Mod_VCollide( model_t *pModel );
	// First model (index 0 )is is empty
	// Second model( index 1 ) is the world, then brushes/submodels, then players, etc.
	// So, we must subtract 1 from the model index to map modelindex to CM_ index
	// in cmodels, 0 is the world, then brushes, etc.
	if ( modelIndex < MAX_MODELS )
	{
		const model_t *pModel = GetModel( modelIndex );
		if ( pModel )
		{
			switch( pModel->type )
			{
			case mod_brush:
				return CM_GetVCollide( modelIndex-1 );
			case mod_studio:
				return Mod_VCollide( (model_t*)pModel );
			}
		}
		else
		{
			// we may have the cmodels loaded and not know the model/mod->type yet
			return CM_GetVCollide( modelIndex-1 );
		}
	}
	return NULL;
}

// Client must instantiate a KeyValues, which will be filled by this method
const char *CModelInfo::GetModelKeyValueText( const model_t *model )
{
	if (!model || model->type != mod_studio)
		return NULL;

	studiohdr_t* pStudioHdr = (studiohdr_t*)GetModelExtraData( model );
	if (!pStudioHdr)
		return NULL;

	return pStudioHdr->KeyValueText();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *model - 
// Output : float
//-----------------------------------------------------------------------------
float CModelInfo::GetModelRadius( const model_t *model )
{
	if ( !model )
		return 0.0f;
	return model->radius;
}


//-----------------------------------------------------------------------------
// Lovely studiohdrs
//-----------------------------------------------------------------------------
studiohdr_t *CModelInfo::GetStudiomodel( const model_t *model )
{
	if ( model->type == mod_studio )
	{
		return (studiohdr_t *)modelloader->GetExtraData( (model_t *)model );
	}
	return NULL;
}



//-----------------------------------------------------------------------------
// implementation of IVModelInfo for server
//-----------------------------------------------------------------------------
class CModelInfoServer : public CModelInfo
{
public:
	virtual const model_t *GetModel( int modelindex ) const;
	virtual int GetModelIndex( const char *name ) const;
	virtual void GetModelMaterialColorAndLighting( const model_t *model, const Vector& origin,
		const QAngle& angles, trace_t* pTrace, Vector& lighting, Vector& matColor );
};

const model_t *CModelInfoServer::GetModel( int modelindex ) const
{
	return sv.GetModel( modelindex );
}

int CModelInfoServer::GetModelIndex( const char *name ) const
{
	return sv.LookupModelIndex( name );
}

void CModelInfoServer::GetModelMaterialColorAndLighting( const model_t *model, const Vector& origin,
	const QAngle& angles, trace_t* pTrace, Vector& lighting, Vector& matColor )
{
	Msg( "GetModelMaterialColorAndLighting:  Available on client only!\n" );
}


static CModelInfoServer	g_ModelInfoServer;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CModelInfoServer, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION, g_ModelInfoServer );

// Expose IVModelInfo to the engine
IVModelInfo *modelinfo = &g_ModelInfoServer;


#ifndef SWDS
//-----------------------------------------------------------------------------
// implementation of IVModelInfo for client
//-----------------------------------------------------------------------------
class CModelInfoClient : public CModelInfo
{
public:
	virtual const model_t *GetModel( int modelindex ) const;
	virtual int GetModelIndex( const char *name ) const;
	virtual void GetModelMaterialColorAndLighting( const model_t *model, const Vector& origin,
		const QAngle& angles, trace_t* pTrace, Vector& lighting, Vector& matColor );
};

const model_t *CModelInfoClient::GetModel( int modelindex ) const
{
	return cl.GetModel( modelindex );
}

int CModelInfoClient::GetModelIndex( const char *name ) const
{
	return cl.LookupModelIndex( name );
}


//-----------------------------------------------------------------------------
// A method to get the material color + texture coordinate
//-----------------------------------------------------------------------------
IMaterial* BrushModel_GetLightingAndMaterial( const Vector &start, 
	const Vector &end, Vector &diffuseLightColor, Vector &baseColor)
{
	float textureS, textureT;
	IMaterial *material;
	int surfID = R_LightVec( start, end, true, diffuseLightColor, &textureS, &textureT );
	if( !IS_SURF_VALID( surfID ) || !MSurf_TexInfo( surfID ) )
	{
//		Con_Printf( "didn't hit anything\n" );
		return 0;
	}
	else
	{
		material = MSurf_TexInfo( surfID )->material;
		material->GetLowResColorSample( textureS, textureT, baseColor.Base() );
//		Con_Printf( "%s: diff: %f %f %f base: %f %f %f\n", material->GetName(), diffuseLightColor[0], diffuseLightColor[1], diffuseLightColor[2], baseColor[0], baseColor[1], baseColor[2] );
		return material;
	}
}

void CModelInfoClient::GetModelMaterialColorAndLighting( const model_t *model, const Vector & origin,
	const QAngle & angles, trace_t* pTrace, Vector& lighting, Vector& matColor )
{
	switch( model->type )
	{
	case mod_brush:
		{
			Vector origin_l, delta, delta_l;
			VectorSubtract( pTrace->endpos, pTrace->startpos, delta );

			// subtract origin offset
			VectorSubtract (pTrace->startpos, origin, origin_l);

			// rotate start and end into the models frame of reference
			if (angles[0] || angles[1] || angles[2])
			{
				Vector forward, right, up;
				AngleVectors (angles, &forward, &right, &up);

				// transform the direction into the local space of this entity
				delta_l[0] = DotProduct (delta, forward);
				delta_l[1] = -DotProduct (delta, right);
				delta_l[2] = DotProduct (delta, up);
			}
			else
			{
				VectorCopy( delta, delta_l );
			}

			Vector end_l;
			VectorMA( origin_l, 1.1f, delta_l, end_l );

			R_LightVecUseModel( ( model_t * )model );
			BrushModel_GetLightingAndMaterial( origin_l, end_l, lighting, matColor );
			R_LightVecUseModel();
			return;
		}

	case mod_studio:
		{
			// FIXME: Need some way of getting the material!
			matColor.Init( 0.5f, 0.5f, 0.5f );

			// Get the lighting at the point
			LightingState_t lightingState;
			LightcacheGet( pTrace->endpos, lightingState );

			// Convert the light parameters into something studiorender can digest
			LightDesc_t desc[MAXLOCALLIGHTS];
			int count = 0;
			for (int i = 0; i < lightingState.numlights; ++i)
			{
				if (WorldLightToMaterialLight( lightingState.locallight[i], desc[count] ))
				{
					++count;
				}
			}

			// Ask studiorender to figure out the lighting
			g_pStudioRender->ComputeLighting( lightingState.r_boxcolor,
				count, desc, pTrace->endpos, pTrace->plane.normal, lighting );
			return;
		}
	}
}

static CModelInfoClient	g_ModelInfoClient;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CModelInfoClient, IVModelInfo, VMODELINFO_CLIENT_INTERFACE_VERSION, g_ModelInfoClient );

// Expose IVModelInfo to the engine
IVModelInfo *modelinfoclient = &g_ModelInfoClient;
#endif