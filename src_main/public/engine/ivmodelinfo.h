//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IVMODELINFO_H
#define IVMODELINFO_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class KeyValues;
struct vcollide_t;
struct model_t;
class Vector;
class QAngle;
class CGameTrace;
typedef CGameTrace trace_t;
struct studiohdr_t;


//-----------------------------------------------------------------------------
// Model info interface
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VMODELINFO_CLIENT_INTERFACE_VERSION	"VModelInfoClient001"
#define VMODELINFO_SERVER_INTERFACE_VERSION	"VModelInfoServer001"

class IVModelInfo
{
public:
	virtual							~IVModelInfo( void ) { }

	virtual const model_t			*GetModel( int modelindex ) const = 0;
	// Returns index of model by name
	virtual int						GetModelIndex( const char *name ) const = 0;

	// Returns name of model
	virtual const char				*GetModelName( const model_t *model ) const = 0;
	virtual vcollide_t				*GetVCollide( const model_t *model ) const = 0;
	virtual vcollide_t				*GetVCollide( int modelindex ) const = 0;
	virtual void					GetModelBounds( const model_t *model, Vector& mins, Vector& maxs ) const = 0;
	virtual	void					GetModelRenderBounds( const model_t *model, int sequence, Vector& mins, Vector& maxs ) const = 0;
	virtual int						GetModelFrameCount( const model_t *model ) const = 0;
	virtual int						GetModelType( const model_t *model ) const = 0;
	virtual void					*GetModelExtraData( const model_t *model ) = 0;
	virtual bool					ModelHasMaterialProxy( const model_t *model ) const = 0;
	virtual bool					IsTranslucent( model_t const* model ) const = 0;
	virtual bool					IsTranslucentTwoPass( const model_t *model ) const = 0;
	virtual void					RecomputeTranslucency( const model_t *model ) = 0;
	virtual int						GetModelMaterialCount( const model_t* model ) const = 0;
	virtual void					GetModelMaterials( const model_t *model, int count, IMaterial** ppMaterial ) = 0;
	virtual bool					IsModelVertexLit( const model_t *model ) const = 0;
	virtual const char				*GetModelKeyValueText( const model_t *model ) = 0;
	virtual float					GetModelRadius( const model_t *model ) = 0;

	// Available on client only!!!
	virtual void					GetModelMaterialColorAndLighting( const model_t *model, Vector const& origin,
										QAngle const& angles, trace_t* pTrace, 
										Vector& lighting, Vector& matColor ) = 0;
	virtual void					GetIlluminationPoint( const model_t *model, Vector const& origin, 
										QAngle const& angles, Vector* pLightingCenter ) = 0;

	virtual int						GetModelContents( int modelIndex ) const = 0;
	virtual studiohdr_t				*GetStudiomodel( const model_t *mod ) = 0;
};

#endif // IVMODELINFO_H