//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Places "detail" objects which are client-only renderable things
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include "vbsp.h"
#include "bsplib.h"
#include "UtlVector.h"
#include "bspfile.h"
#include "gamebspfile.h"
#include "VPhysics_Interface.h"
#include "Studio.h"
#include "UtlBuffer.h"
#include "CollisionUtils.h"
#include <float.h>
#include "CModel.h"
#include "PhysDll.h"
#include "UtlSymbol.h"
#include "vstdlib/strtools.h"
#include "keyvalues.h"

IPhysicsCollision *s_pPhysCollision = NULL;

//-----------------------------------------------------------------------------
// These puppies are used to construct the game lumps
//-----------------------------------------------------------------------------
static CUtlVector<StaticPropDictLump_t>	s_StaticPropDictLump;
static CUtlVector<StaticPropLump_t>		s_StaticPropLump;
static CUtlVector<StaticPropLeafLump_t>	s_StaticPropLeafLump;


//-----------------------------------------------------------------------------
// Used to build the static prop
//-----------------------------------------------------------------------------
struct StaticPropBuild_t
{
	char const* m_pModelName;
	char const* m_pLightingOrigin;
	Vector	m_Origin;
	QAngle	m_Angles;
	int		m_Solid;
	int		m_Skin;
	int		m_Flags;
	float	m_FadeMinDist;
	float	m_FadeMaxDist;
	bool	m_FadesOut;
};
 

//-----------------------------------------------------------------------------
// Used to cache collision model generation
//-----------------------------------------------------------------------------
struct ModelCollisionLookup_t
{
	CUtlSymbol m_Name;
	CPhysCollide* m_pCollide;
};

static bool ModelLess( ModelCollisionLookup_t const& src1, ModelCollisionLookup_t const& src2 )
{
	return src1.m_Name < src2.m_Name;
}

static CUtlRBTree<ModelCollisionLookup_t, unsigned short>	s_ModelCollisionCache( 0, 32, ModelLess );
static CUtlVector<int>	s_LightingInfo;


//-----------------------------------------------------------------------------
// Gets the keyvalues from a studiohdr
//-----------------------------------------------------------------------------
bool StudioKeyValues( studiohdr_t* pStudioHdr, KeyValues *pValue )
{
	if ( !pStudioHdr )
		return false;

	return pValue->LoadFromBuffer( pStudioHdr->name, pStudioHdr->KeyValueText() );
}


//-----------------------------------------------------------------------------
// Makes sure the studio model is a static prop
//-----------------------------------------------------------------------------
bool IsStaticProp( studiohdr_t* pHdr )
{
 	if ( (pHdr->numbones > 1) || (pHdr->numanim > 1) || (pHdr->numflexrules > 0) ||
		 (pHdr->nummouths > 0) )
		return false;

	mstudiobone_t* pBone = pHdr->pBone(0);
	if ( pBone->bonecontroller[0] != -1 )
		return false;
	
	// Static props must have identity pose to bone
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (i == j)
			{
				if ( fabs( pBone->poseToBone[i][j] - 1.0f ) > 1e-3 )
					return false;
			}
			else
			{
				if ( fabs( pBone->poseToBone[i][j] ) > 1e-3 )
					return false;
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Add static prop model to the list of models
//-----------------------------------------------------------------------------

static int AddStaticPropDictLump( char const* pModelName )
{
	StaticPropDictLump_t dictLump;
	strncpy( dictLump.m_Name, pModelName, DETAIL_NAME_LENGTH );

	for (int i = s_StaticPropDictLump.Size(); --i >= 0; )
	{
		if (!memcmp(&s_StaticPropDictLump[i], &dictLump, sizeof(dictLump) ))
			return i;
	}

	return s_StaticPropDictLump.AddToTail( dictLump );
}


//-----------------------------------------------------------------------------
// Constructs the file name from the model name
//-----------------------------------------------------------------------------
static char const* ConstructFileName( char const* pModelName )
{
	static char buf[1024];
	sprintf( buf, "%s%s", gamedir, pModelName );
	return buf;
}


//-----------------------------------------------------------------------------
// Load studio model vertex data from a file...
//-----------------------------------------------------------------------------
bool LoadStudioModel( char const* pModelName, char const* pEntityType, CUtlBuffer& buf )
{
	// No luck, gotta build it	
	// Construct the file name...
	char const* pFileName = ConstructFileName( pModelName );

	FILE* fp;

	// load the model
	if( (fp = fopen( pFileName, "rb" )) == NULL)
		return false;

	// Get the file size
	fseek( fp, 0, SEEK_END );
	int size = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	if (size == 0)
	{
		fclose(fp);
		return false;
	}

	buf.EnsureCapacity( size );
	fread( buf.PeekPut(), size, 1, fp );
	fclose( fp );

	buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );

	// Check that it's valid
	if (strncmp ((const char *) buf.PeekGet(), "IDST", 4) &&
		strncmp ((const char *) buf.PeekGet(), "IDSQ", 4))
	{
		return false;
	}

	studiohdr_t* pHdr = (studiohdr_t*)buf.PeekGet();
	Studio_ConvertStudioHdrToNewVersion( pHdr );
	if (pHdr->version != STUDIO_VERSION)
	{
		return false;
	}

//	KeyValues *pKeyValue = new KeyValues(pHdr->name);
//	if ( StudioKeyValues( pHdr, pKeyValue ) )
//	{
		// do stuff...
//	}
//	pKeyValue->deleteThis();

	if (!IsStaticProp(pHdr))
	{
		Warning("Error! To use model \"%s\"\n"
			"      with %s, it must be compiled with $staticprop!\n", pFileName, pEntityType );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Computes a convex hull from a studio mesh
//-----------------------------------------------------------------------------
static CPhysConvex* ComputeConvexHull( mstudiomesh_t* pMesh )
{
	// Generate a list of all verts in the mesh
	Vector** ppVerts = (Vector**)stackalloc(pMesh->numvertices * sizeof(Vector*) );
	for (int i = 0; i < pMesh->numvertices; ++i)
	{
		ppVerts[i] = pMesh->Position(i);
	}

	// Generate a convex hull from the verts
	return s_pPhysCollision->ConvexFromVerts( ppVerts, pMesh->numvertices );
}


//-----------------------------------------------------------------------------
// Computes a convex hull from the studio model
//-----------------------------------------------------------------------------
CPhysCollide* ComputeConvexHull( studiohdr_t* pStudioHdr )
{
	CUtlVector<CPhysConvex*>	convexHulls;
	for (int body = 0; body < pStudioHdr->numbodyparts; ++body )
	{
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( body );
		for( int model = 0; model < pBodyPart->nummodels; ++model )
		{
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( model );
			for( int mesh = 0; mesh < pStudioModel->nummeshes; ++mesh )
			{
				// Make a convex hull for each mesh
				// NOTE: This won't work unless the model has been compiled
				// with $staticprop
				mstudiomesh_t *pStudioMesh = pStudioModel->pMesh( mesh );
				convexHulls.AddToTail( ComputeConvexHull( pStudioMesh ) );
			}
		}
	}

	// Convert an array of convex elements to a compiled collision model
	// (this deletes the convex elements)
	return s_pPhysCollision->ConvertConvexToCollide( convexHulls.Base(), convexHulls.Size() );
}


//-----------------------------------------------------------------------------
// Add, find collision model in cache
//-----------------------------------------------------------------------------
static CPhysCollide* GetCollisionModel( char const* pModelName )
{
	// Convert to a common string
	char* pTemp = (char*)_alloca(strlen(pModelName) + 1);
	strcpy( pTemp, pModelName );
	_strlwr( pTemp );

	char* pSlash = strchr( pTemp, '\\' );
	while( pSlash )
	{
		*pSlash = '/';
		pSlash = strchr( pTemp, '\\' );
	}

	// Find it in the cache
	ModelCollisionLookup_t lookup;
	lookup.m_Name = pTemp;
	int i = s_ModelCollisionCache.Find( lookup );
	if (i != s_ModelCollisionCache.InvalidIndex())
		return s_ModelCollisionCache[i].m_pCollide;

	// Load the studio model file
	CUtlBuffer buf;
	if (!LoadStudioModel(pModelName, "static_prop", buf))
	{
		Warning("Error loading studio model \"%s\"!\n", pModelName );

		// This way we don't try to load it multiple times
		lookup.m_pCollide = 0;
		s_ModelCollisionCache.Insert( lookup );

		return 0;
	}

	// Compute the convex hull of the model...
	studiohdr_t* pStudioHdr = (studiohdr_t*)buf.PeekGet();
	lookup.m_pCollide = ComputeConvexHull( pStudioHdr );
	s_ModelCollisionCache.Insert( lookup );

	// Debugging
	if (g_DumpStaticProps)
	{
		static int propNum = 0;
		char tmp[128];
		sprintf( tmp, "staticprop%03d.txt", propNum );
		DumpCollideToGlView( lookup.m_pCollide, tmp );
		++propNum;
	}

	// Insert into cache...
	return lookup.m_pCollide;
}


//-----------------------------------------------------------------------------
// Tests a single leaf against the static prop
//-----------------------------------------------------------------------------

static bool TestLeafAgainstCollide( int depth, int* pNodeList, 
	Vector const& origin, QAngle const& angles, CPhysCollide* pCollide )
{
	// Copy the planes in the node list into a list of planes
	float* pPlanes = (float*)_alloca(depth * 4 * sizeof(float) );
	int idx = 0;
	for (int i = depth; --i >= 0; ++idx )
	{
		int sign = (pNodeList[i] < 0) ? -1 : 1;
		int node = (sign < 0) ? - pNodeList[i] - 1 : pNodeList[i];
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		pPlanes[idx*4] = sign * pPlane->normal[0];
		pPlanes[idx*4+1] = sign * pPlane->normal[1];
		pPlanes[idx*4+2] = sign * pPlane->normal[2];
		pPlanes[idx*4+3] = sign * pPlane->dist;
	}

	// Make a convex solid out of the planes
	CPhysConvex* pPhysConvex = s_pPhysCollision->ConvexFromPlanes( pPlanes, depth, 0.0f );

	// This should never happen, but if it does, return no collision
	assert( pPhysConvex );
	if (!pPhysConvex)
		return false;

	CPhysCollide* pLeafCollide = s_pPhysCollision->ConvertConvexToCollide( &pPhysConvex, 1 );

	// Collide the leaf solid with the static prop solid
	trace_t	tr;
	s_pPhysCollision->TraceCollide( vec3_origin, vec3_origin, pLeafCollide, vec3_angle,
		pCollide, origin, angles, &tr );

	s_pPhysCollision->DestroyCollide( pLeafCollide );

	return (tr.startsolid != 0);
}

//-----------------------------------------------------------------------------
// Find all leaves that intersect with this bbox + test against the static prop..
//-----------------------------------------------------------------------------

static void ComputeConvexHullLeaves_R( int node, int depth, int* pNodeList,
	Vector const& mins, Vector const& maxs,
	Vector const& origin, QAngle const& angles,	CPhysCollide* pCollide,
	CUtlVector<unsigned short>& leafList )
{
	assert( pNodeList && pCollide );
	Vector cornermin, cornermax;

	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		// Arbitrary split plane here
		for (int i = 0; i < 3; ++i)
		{
			if (pPlane->normal[i] >= 0)
			{
				cornermin[i] = mins[i];
				cornermax[i] = maxs[i];
			}
			else
			{
				cornermin[i] = maxs[i];
				cornermax[i] = mins[i];
			}
		}

		if (DotProduct( pPlane->normal, cornermax ) <= pPlane->dist)
		{
			// Add the node to the list of nodes
			pNodeList[depth] = node;
			++depth;

			node = pNode->children[1];
		}
		else if (DotProduct( pPlane->normal, cornermin ) >= pPlane->dist)
		{
			// In this case, we are going in front of the plane. That means that
			// this plane must have an outward normal facing in the oppisite direction
			// We indicate this be storing a negative node index in the node list
			pNodeList[depth] = - node - 1;
			++depth;

			node = pNode->children[0];
		}
		else
		{
			// Here the box is split by the node. First, we'll add the plane as if its
			// outward facing normal is in the direction of the node plane, then
			// we'll have to reverse it for the other child...
			pNodeList[depth] = node;
			++depth;

			ComputeConvexHullLeaves_R( pNode->children[1], 
				depth, pNodeList, mins, maxs, origin, angles, pCollide, leafList );
			
			pNodeList[depth - 1] = - node - 1;
			ComputeConvexHullLeaves_R( pNode->children[0],
				depth, pNodeList, mins, maxs, origin, angles, pCollide, leafList );
			return;
		}
	}

	assert( pNodeList && pCollide );

	// Never add static props to solid leaves
	if ( (dleafs[-node-1].contents & CONTENTS_SOLID) == 0 )
	{
		if (TestLeafAgainstCollide( depth, pNodeList, origin, angles, pCollide ))
		{
			leafList.AddToTail( -node - 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Places Static Props in the level
//-----------------------------------------------------------------------------

static void ComputeStaticPropLeaves( CPhysCollide* pCollide, Vector const& origin, 
				QAngle const& angles, CUtlVector<unsigned short>& leafList )
{
	// Compute an axis-aligned bounding box for the collide
	Vector mins, maxs;
	s_pPhysCollision->CollideGetAABB( mins, maxs, pCollide, origin, angles );

	// Find all leaves that intersect with the bounds
	int tempNodeList[1024];
	ComputeConvexHullLeaves_R( 0, 0, tempNodeList, mins, maxs,
		origin, angles, pCollide, leafList );
}


//-----------------------------------------------------------------------------
// Computes the lighting origin
//-----------------------------------------------------------------------------
static bool ComputeLightingOrigin( StaticPropBuild_t const& build, Vector& lightingOrigin )
{
	for (int i = s_LightingInfo.Count(); --i >= 0; )
	{
		int entIndex = s_LightingInfo[i];

		// Check against all lighting info entities
		char const* pTargetName = ValueForKey( &entities[entIndex], "targetname" );
		if (!Q_strcmp(pTargetName, build.m_pLightingOrigin))
		{
			GetVectorForKey( &entities[entIndex], "origin", lightingOrigin );
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Places Static Props in the level
//-----------------------------------------------------------------------------
static void AddStaticPropToLump( StaticPropBuild_t const& build )
{
	// Get the collision model
	CPhysCollide* pConvexHull = GetCollisionModel( build.m_pModelName );
	if (!pConvexHull)
		return;

	// Compute the leaves the static prop's convex hull hits
	CUtlVector< unsigned short > leafList;
	ComputeStaticPropLeaves( pConvexHull, build.m_Origin, build.m_Angles, leafList );

	// Insert an element into the lump data...
	int i = s_StaticPropLump.AddToTail( );
	StaticPropLump_t& propLump = s_StaticPropLump[i];
	propLump.m_PropType = AddStaticPropDictLump( build.m_pModelName ); 
	VectorCopy( build.m_Origin, propLump.m_Origin );
	VectorCopy( build.m_Angles, propLump.m_Angles );
	propLump.m_FirstLeaf = s_StaticPropLeafLump.Size();
	propLump.m_LeafCount = leafList.Size();
	propLump.m_Solid = build.m_Solid;
	propLump.m_Skin = build.m_Skin;
	propLump.m_Flags = build.m_Flags;
	if (build.m_FadesOut)
		propLump.m_Flags |= STATIC_PROP_FLAG_FADES;
	propLump.m_FadeMinDist = build.m_FadeMinDist;
	propLump.m_FadeMaxDist = build.m_FadeMaxDist;

	if (build.m_pLightingOrigin && *build.m_pLightingOrigin)
	{
		if (ComputeLightingOrigin( build, propLump.m_LightingOrigin ))
		{
			propLump.m_Flags |= STATIC_PROP_USE_LIGHTING_ORIGIN;
		}
	}

	// Add the leaves to the leaf lump
	for (int j = 0; j < leafList.Size(); ++j)
	{
		StaticPropLeafLump_t insert;
		insert.m_Leaf = leafList[j];
		s_StaticPropLeafLump.AddToTail( insert );
	}
}


//-----------------------------------------------------------------------------
// Places static props in the lump
//-----------------------------------------------------------------------------

static void SetLumpData( )
{
	GameLumpHandle_t handle = GetGameLumpHandle(GAMELUMP_STATIC_PROPS);
	if (handle != InvalidGameLump())
		DestroyGameLump(handle);
	int dictsize = s_StaticPropDictLump.Size() * sizeof(StaticPropDictLump_t);
	int objsize = s_StaticPropLump.Size() * sizeof(StaticPropLump_t);
	int leafsize = s_StaticPropLeafLump.Size() * sizeof(StaticPropLeafLump_t);
	int size = dictsize + objsize + leafsize + 3 * sizeof(int);

	handle = CreateGameLump( GAMELUMP_STATIC_PROPS, size, 0, GAMELUMP_STATIC_PROPS_VERSION );

	// Serialize the data
	CUtlBuffer buf( GetGameLump(handle), size );
	buf.PutInt( s_StaticPropDictLump.Size() );
	if (dictsize)
		buf.Put( s_StaticPropDictLump.Base(), dictsize );
	buf.PutInt( s_StaticPropLeafLump.Size() );
	if (leafsize)
		buf.Put( s_StaticPropLeafLump.Base(), leafsize );
	buf.PutInt( s_StaticPropLump.Size() );
	if (objsize)
		buf.Put( s_StaticPropLump.Base(), objsize );
}


//-----------------------------------------------------------------------------
// Places Static Props in the level
//-----------------------------------------------------------------------------

void EmitStaticProps()
{
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( physicsFactory )
	{
		s_pPhysCollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
		if( !s_pPhysCollision )
			return;
	}

	// Generate a list of lighting origins, and strip them out
	int i;
	for ( i = 0; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!Q_strcmp(pEntity, "info_lighting"))
		{
			s_LightingInfo.AddToTail(i);
		}
	}

	// Emit specifically specified static props
	for ( i = 0; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!strcmp(pEntity, "static_prop") || !strcmp(pEntity, "prop_static"))
		{
			StaticPropBuild_t build;

			GetVectorForKey( &entities[i], "origin", build.m_Origin );
			GetAnglesForKey( &entities[i], "angles", build.m_Angles );
			build.m_pModelName = ValueForKey( &entities[i], "model" );
			build.m_Solid = IntForKey( &entities[i], "solid" );
			build.m_Skin = IntForKey( &entities[i], "skin" );
			build.m_FadeMaxDist = FloatForKey( &entities[i], "fademaxdist" );
			build.m_Flags = 0;//IntForKey( &entities[i], "spawnflags" ) & STATIC_PROP_WC_MASK;
			if (IntForKey( &entities[i], "disableshadows" ) == 1)
				build.m_Flags |= STATIC_PROP_NO_SHADOW;

			build.m_FadesOut = (build.m_FadeMaxDist > 0);
			build.m_pLightingOrigin = ValueForKey( &entities[i], "lightingorigin" );
			if (build.m_FadesOut)
			{			  
				build.m_FadeMinDist = FloatForKey( &entities[i], "fademindist" );
				if (build.m_FadeMinDist < 0)
					build.m_FadeMinDist = build.m_FadeMaxDist; 
			}
			else
			{
				build.m_FadeMinDist = 0;
			}

			AddStaticPropToLump( build );

			// strip this ent from the .bsp file
			entities[i].epairs = 0;
		}
	}

	// Strip out lighting origins; has to be done here because they are used when
	// static props are made
	for ( i = s_LightingInfo.Count(); --i >= 0; )
	{
		// strip this ent from the .bsp file
		entities[s_LightingInfo[i]].epairs = 0;
	}


	SetLumpData( );
}
