//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Game & Client shared functions moved from physics.cpp
//
//=============================================================================
#include "cbase.h"
#include "vcollide_parse.h"
#include "filesystem.h"
#include "movevars_shared.h"
#include "engine/ivmodelinfo.h"
#include "physics.h"
#include "solidsetdefaults.h"
#include "model_types.h"
#include "bone_setup.h"

#ifndef CLIENT_DLL
#include "physics_saverestore.h"
#endif

//
IPhysics			*physics = NULL;
IPhysicsObject		*g_PhysWorldObject = NULL;
IPhysicsCollision	*physcollision = NULL;
IPhysicsEnvironment	*physenv = NULL;
IPhysicsSurfaceProps *physprops = NULL;

const objectparams_t g_PhysDefaultObjectParams =
{
	NULL,
	1.0, //mass
	1.0, // inertia
	0.1f, // damping
	0.1f, // rotdamping
	0.5, // rotIntertiaLimit
	"DEFAULT",
	NULL,// game data
	0.f, // volume (leave 0 if you don't have one or call physcollision->CollideVolume() to compute it)
	1.0f, // drag coefficient
	1.0f, // rolling drag
	true,// enable collisions?
};


void CSolidSetDefaults::ParseKeyValue( void *pData, const char *pKey, const char *pValue )
{
	if ( !Q_stricmp( pKey, "contents" ) )
	{
		m_contentsMask = atoi( pValue );
	}
}

void CSolidSetDefaults::SetDefaults( void *pData )
{
	solid_t *pSolid = (solid_t *)pData;
	pSolid->params = g_PhysDefaultObjectParams;
	m_contentsMask = CONTENTS_SOLID;
}

CSolidSetDefaults g_SolidSetup;

ConVar phys_rolling_drag( "phys_rolling_drag", "1", FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &mins - 
//			&maxs - 
// Output : CPhysCollide
//-----------------------------------------------------------------------------
CPhysCollide *PhysCreateBbox( const Vector &mins, const Vector &maxs )
{
	// VPHYSICS caches/cleans up these
	CPhysCollide *pResult = physcollision->BBoxToCollide( mins, maxs );

#ifndef CLIENT_DLL
	g_pPhysSaveRestoreManager->NoteBBox( mins, maxs, pResult );
#endif
	
	return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			&mins - 
//			&maxs - 
//			&origin - 
//			isStatic - 
// Output : static IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreateBox( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, bool isStatic )
{
	int modelIndex = pEntity->GetModelIndex();
	const char *pSurfaceProps = "flesh";
	solid_t solid;
	PhysGetDefaultAABBSolid( solid );
	Vector dims = maxs - mins;
	solid.params.volume = dims.x * dims.y * dims.z;

	if ( modelIndex )
	{
		const model_t *model = modelinfo->GetModel( modelIndex );
		if ( model )
		{
			if ( modelinfo->GetModelType( model ) == mod_studio )
			{
				studiohdr_t *pstudiohdr = static_cast< studiohdr_t * >( modelinfo->GetModelExtraData( model ) );
				pSurfaceProps = Studio_GetDefaultSurfaceProps( pstudiohdr );
			}
		}
	}
	strcpy( solid.surfaceprop, pSurfaceProps );

	CPhysCollide *pCollide = PhysCreateBbox( mins, maxs );
	if ( !pCollide )
		return NULL;
	
	return PhysModelCreateCustom( pEntity, pCollide, origin, vec3_angle, STRING(pEntity->GetModelName()), isStatic, &solid );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &solid - 
//			*pEntity - 
//			modelIndex - 
//			solidIndex - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, int modelIndex, int solidIndex )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( modelIndex );
	if ( !pCollide )
		return false;

	bool parsed = false;

	memset( &solid, 0, sizeof(solid) );
	solid.params = g_PhysDefaultObjectParams;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t tmpSolid;
			memset( &tmpSolid, 0, sizeof(tmpSolid) );
			tmpSolid.params = g_PhysDefaultObjectParams;

			pParse->ParseSolid( &tmpSolid, &g_SolidSetup );

			if ( solidIndex < 0 || tmpSolid.index == solidIndex )
			{
				parsed = true;
				solid = tmpSolid;
				// just to be sure we aren't ever getting a non-zero solid by accident
				Assert( solidIndex >= 0 || solid.index == 0 );
				break;
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	// collisions are off by default
	solid.params.enableCollisions = true;

	solid.params.pGameData = static_cast<void *>(pEntity);
	solid.params.pName = STRING(pEntity->GetModelName());
	return parsed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &solid - 
//			*pEntity - 
//			modelIndex - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool PhysModelParseSolid( solid_t &solid, CBaseEntity *pEntity, int modelIndex )
{
	return PhysModelParseSolidByIndex( solid, pEntity, modelIndex, -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &solid - 
//			*pEntity - 
//			*pCollide - 
//			solidIndex - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, vcollide_t *pCollide, int solidIndex )
{
	bool parsed = false;

	memset( &solid, 0, sizeof(solid) );
	solid.params = g_PhysDefaultObjectParams;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t tmpSolid;
			memset( &tmpSolid, 0, sizeof(tmpSolid) );
			tmpSolid.params = g_PhysDefaultObjectParams;

			pParse->ParseSolid( &tmpSolid, &g_SolidSetup );

			if ( solidIndex < 0 || tmpSolid.index == solidIndex )
			{
				parsed = true;
				solid = tmpSolid;
				// just to be sure we aren't ever getting a non-zero solid by accident
				Assert( solidIndex >= 0 || solid.index == 0 );
				break;
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	// collisions are off by default
	solid.params.enableCollisions = true;

	solid.params.pGameData = static_cast<void *>(pEntity);
	solid.params.pName = STRING(pEntity->GetModelName());
	return parsed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			modelIndex - 
//			&origin - 
//			&angles - 
//			*pSolid - 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreate( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles, solid_t *pSolid )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( modelIndex );
	if ( !pCollide || !pCollide->solidCount )
		return NULL;
	
	solid_t tmpSolid;
	if ( !pSolid )
	{
		pSolid = &tmpSolid;
		if ( !PhysModelParseSolidByIndex( tmpSolid, pEntity, pCollide, -1 ) )
			return NULL;
	}

	int surfaceProp = -1;
	if ( pSolid->surfaceprop[0] )
	{
		surfaceProp = physprops->GetSurfaceIndex( pSolid->surfaceprop );
	}
	IPhysicsObject *pObject = physenv->CreatePolyObject( pCollide->solids[pSolid->index], surfaceProp, origin, angles, &pSolid->params );
	//PhysCheckAdd( pObject, STRING(pEntity->m_iClassname) );

#ifndef CLIENT_DLL
	if ( pObject )
		g_pPhysSaveRestoreManager->AssociateModel( pObject, modelIndex);
#endif
	return pObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			modelIndex - 
//			&origin - 
//			&angles - 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreateUnmoveable( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( modelIndex );
	if ( !pCollide || !pCollide->solidCount )
		return NULL;

	solid_t solid;

	if ( !PhysModelParseSolidByIndex( solid, pEntity, pCollide, -1 ) )
		return NULL;

	// collisions are off by default
	solid.params.enableCollisions = true;
	//solid.params.mass = 1.0;
	int surfaceProp = -1;
	if ( solid.surfaceprop[0] )
	{
		surfaceProp = physprops->GetSurfaceIndex( solid.surfaceprop );
	}
	solid.params.pGameData = static_cast<void *>(pEntity);
	solid.params.pName = STRING(pEntity->GetModelName());
	IPhysicsObject *pObject = physenv->CreatePolyObjectStatic( pCollide->solids[0], surfaceProp, origin, angles, &solid.params );
	//PhysCheckAdd( pObject, STRING(pEntity->m_iClassname) );
#ifndef CLIENT_DLL
	if ( pObject )
		g_pPhysSaveRestoreManager->AssociateModel( pObject, modelIndex);
#endif
	return pObject;
}


//-----------------------------------------------------------------------------
// Purpose: Create a vphysics object based on an existing collision model
// Input  : *pEntity - 
//			*pModel - 
//			&origin - 
//			&angles - 
//			*pName - 
//			isStatic - 
//			*pSolid - 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreateCustom( CBaseEntity *pEntity, const CPhysCollide *pModel, const Vector &origin, const QAngle &angles, const char *pName, bool isStatic, solid_t *pSolid )
{
	solid_t tmpSolid;
	if ( !pSolid )
	{
		PhysGetDefaultAABBSolid( tmpSolid );
		pSolid = &tmpSolid;
	}
	int surfaceProp = physprops->GetSurfaceIndex( pSolid->surfaceprop );
	pSolid->params.pGameData = static_cast<void *>(pEntity);
	pSolid->params.pName = pName;
	IPhysicsObject *pObject = NULL;
	if ( isStatic )
	{
		pObject = physenv->CreatePolyObjectStatic( pModel, surfaceProp, origin, angles, &pSolid->params );
	}
	else
	{
		pObject = physenv->CreatePolyObject( pModel, surfaceProp, origin, angles, &pSolid->params );
	}
#ifndef CLIENT_DLL
	if ( pObject )
		g_pPhysSaveRestoreManager->AssociateModel( pObject, pModel);
#endif
	return pObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			radius - 
//			&origin - 
//			&solid - 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysSphereCreate( CBaseEntity *pEntity, float radius, const Vector &origin, solid_t &solid )
{
	int surfaceProp = -1;
	if ( solid.surfaceprop[0] )
	{
		surfaceProp = physprops->GetSurfaceIndex( solid.surfaceprop );
	}

	IPhysicsObject *pObject = physenv->CreateSphereObject( radius, surfaceProp, origin, vec3_angle, &solid.params, false );

#ifndef CLIENT_DLL
	// UNDONE: save/restore radius?
	if ( pObject )
	{
		//g_pPhysSaveRestoreManager->AssociateModel( pObject, radius );
	}
#endif

	return pObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PhysGetDefaultAABBSolid( solid_t &solid )
{
	solid.params = g_PhysDefaultObjectParams;
	solid.params.mass = 85.0f;
	solid.params.inertia = 1e24f;
	strcpy( solid.surfaceprop, "default" );
}

//-----------------------------------------------------------------------------
// Purpose: Destroy a physics object
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void PhysDestroyObject( IPhysicsObject *pObject )
{
#ifndef CLIENT_DLL
	g_pPhysSaveRestoreManager->ForgetModel( pObject );
#endif
	physenv->DestroyObject( pObject );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void PhysApplyRollingDrag( IPhysicsObject *pObject, float deltaCollisionTime )
{
	if ( !pObject->IsMoveable() )
		return;

	// HACKHACK: JAY: Havok physics has a tendency to make objects roll
	// If the collision "looks like rolling" then damp out some of the
	// angular velocity

	// NOTE: These numbers are hand-tuned for a pretty wide range of
	// our objects.  But this is just a heuristic!

	float drag = pObject->GetRollingDrag();
	if ( drag != 0 )
	{
		// damp out 15-40% of the angular velocity on repeated collisions
		AngularImpulse angEnd;
		pObject->GetVelocity( NULL, &angEnd );
		float angVel = angEnd.Length();
		if ( angVel > 1 && (angVel < 360 || deltaCollisionTime < 0.25) )
		{
			float fraction = RemapVal( deltaCollisionTime, 0, 1.5, 0.4, 0.15 ) * drag;
			angVel = clamp( angVel, 0, 720 );
			// anything under 360 deg/s gets damped 100%.
			if ( angVel >= 360 )
			{
				// drop to 10% at 720 deg/s
				fraction *= RemapVal( angVel, 360, 720, 1, 0.1 );
			}

			if ( fraction > 1 )
			{
				fraction = 1;
			}
			angEnd *= -fraction;
			pObject->AddVelocity( NULL, &angEnd );
		}
	}
}

void PhysParseSurfaceData( const char *pFileName, IPhysicsSurfaceProps *pProps, IFileSystem *pFileSystem )
{
	// Load file into memory
	FileHandle_t file = pFileSystem->Open( pFileName, "rb" );

	if ( file )
	{
		int len = pFileSystem->Size( file );

		// read the file
		char *buffer = (char *)stackalloc( len+1 );
		pFileSystem->Read( buffer, len, file );
		pFileSystem->Close( file );
		buffer[len] = 0;
		pProps->ParseSurfaceData( pFileName, buffer );
		// buffer is on the stack, no need to free
	}
}

IPhysicsObject *PhysCreateWorld_Shared( CBaseEntity *pWorld, vcollide_t *pWorldCollide, const objectparams_t &defaultParams )
{
	solid_t solid;
	fluid_t fluid;

	int surfaceData = physprops->GetSurfaceIndex( "default" );

	objectparams_t params = defaultParams;
	params.pGameData = static_cast<void *>(pWorld);
	params.pName = "world";

	IPhysicsObject *pWorldPhysics = physenv->CreatePolyObjectStatic( 
		pWorldCollide->solids[0], surfaceData, vec3_origin, vec3_angle, &params );

	// hint - saves vphysics some work
	pWorldPhysics->SetCallbackFlags( pWorldPhysics->GetCallbackFlags() | CALLBACK_NEVER_DELETED );

	//PhysCheckAdd( world, "World" );
	// walk the world keys in case there are some fluid volumes to create
	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pWorldCollide->pKeyValues );

	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();

		if ( !strcmpi( pBlock, "solid" ) || !strcmpi( pBlock, "staticsolid" ) )
		{
			solid.params = defaultParams;
			pParse->ParseSolid( &solid, &g_SolidSetup );
			solid.params.enableCollisions = true;
			solid.params.pGameData = static_cast<void *>(pWorld);
			solid.params.pName = "world";
			int surfaceData = physprops->GetSurfaceIndex( "default" );
			// create this as part of the world
			IPhysicsObject *pObject = physenv->CreatePolyObjectStatic( pWorldCollide->solids[solid.index], 
				surfaceData, vec3_origin, vec3_angle, &solid.params );
			pObject->SetCallbackFlags( pObject->GetCallbackFlags() | CALLBACK_NEVER_DELETED );
			Assert( g_SolidSetup.GetContentsMask() != 0 );
			pObject->SetContents( g_SolidSetup.GetContentsMask() );

			if ( !pWorldPhysics )
			{
				pWorldPhysics = pObject;
			}
		}
		else if ( !strcmpi( pBlock, "fluid" ) )
		{
			pParse->ParseFluid( &fluid, NULL );

			// create a fluid for floating
			if ( fluid.index > 0 )
			{
				solid.params = defaultParams;	// copy world's params
				solid.params.enableCollisions = true;
				solid.params.pName = "fluid";
				solid.params.pGameData = static_cast<void *>(pWorld);
				fluid.params.pGameData = static_cast<void *>(pWorld);
				int surfaceData = physprops->GetSurfaceIndex( "water" );
				// create this as part of the world
				IPhysicsObject *pWater = physenv->CreatePolyObjectStatic( pWorldCollide->solids[fluid.index], 
					surfaceData, vec3_origin, vec3_angle, &solid.params );

				pWater->SetCallbackFlags( pWater->GetCallbackFlags() | CALLBACK_NEVER_DELETED );
				physenv->CreateFluidController( pWater, &fluid.params );
			}
		}
		else if ( !strcmpi( pBlock, "materialtable" ) )
		{
			int surfaceTable[128];
			memset( surfaceTable, 0, sizeof(surfaceTable) );

			pParse->ParseSurfaceTable( surfaceTable, NULL );
			physprops->SetWorldMaterialIndexTable( surfaceTable, 128 );
		}
		else
		{
			// unknown chunk???
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	return pWorldPhysics;
}
