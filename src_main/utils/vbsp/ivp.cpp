//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include "vector.h"
#include "bspfile.h"
#include "bsplib.h"
#include "cmdlib.h"
#include "physdll.h"
#include "utlvector.h"
#include "vbsp.h"
#include "phyfile.h"
#include <float.h>
#include "KeyValues.h"
#include "UtlBuffer.h"
#include "UtlSymbol.h"
#include "UtlRBTree.h"
#include <assert.h>
#include "ivp.h"
#include "disp_ivp.h"
#include "materialpatch.h"


// parameters for conversion to vphysics
#define VPHYSICS_SHRINK		0.25	// shrink BSP brushes by this much for collision
#define VPHYSICS_MERGE		0.01	// merge verts closer than this

void EmitPhysCollision( void );

IPhysicsCollision *physcollision = NULL;
extern IPhysicsSurfaceProps *physprops;

// a list of all of the materials in the world model
static CUtlVector<int> s_WorldPropList;

//-----------------------------------------------------------------------------
// Purpose: Write key/value pairs out to a memory buffer
//-----------------------------------------------------------------------------
CTextBuffer::CTextBuffer( void )
{
}
CTextBuffer::~CTextBuffer( void )
{
}

void CTextBuffer::WriteText( const char *pText )
{
	int len = strlen( pText );
	CopyData( pText, len );
}

void CTextBuffer::WriteIntKey( const char *pKeyName, int outputData )
{
	char tmp[1024];
	
	// FAIL!
	if ( strlen(pKeyName) > 1000 )
	{
		Msg("Error writing collision data %s\n", pKeyName );
		return;
	}
	sprintf( tmp, "\"%s\" \"%d\"\n", pKeyName, outputData );
	CopyData( tmp, strlen(tmp) );
}

void CTextBuffer::WriteStringKey( const char *pKeyName, const char *outputData )
{
	CopyStringQuotes( pKeyName );
	CopyData( " ", 1 );
	CopyStringQuotes( outputData );
	CopyData( "\n", 1 );
}

void CTextBuffer::WriteFloatKey( const char *pKeyName, float outputData )
{
	char tmp[1024];
	
	// FAIL!
	if ( strlen(pKeyName) > 1000 )
	{
		Msg("Error writing collision data %s\n", pKeyName );
		return;
	}
	sprintf( tmp, "\"%s\" \"%f\"\n", pKeyName, outputData );
	CopyData( tmp, strlen(tmp) );
}

void CTextBuffer::WriteFloatArrayKey( const char *pKeyName, const float *outputData, int count )
{
	char tmp[1024];
	
	// FAIL!
	if ( strlen(pKeyName) > 1000 )
	{
		Msg("Error writing collision data %s\n", pKeyName );
		return;
	}
	sprintf( tmp, "\"%s\" \"", pKeyName );
	for ( int i = 0; i < count; i++ )
	{
		char buf[80];

		sprintf( buf, "%f ", outputData[i] );
		strcat( tmp, buf );
	}
	strcat( tmp, "\"\n" );

	CopyData( tmp, strlen(tmp) );
}

void CTextBuffer::CopyStringQuotes( const char *pString )
{
	CopyData( "\"", 1 );
	CopyData( pString, strlen(pString) );
	CopyData( "\"", 1 );
}

void CTextBuffer::Terminate( void )
{
	CopyData( "\0", 1 );
}

void CTextBuffer::CopyData( const char *pData, int len )
{
	int offset = m_buffer.AddMultipleToTail( len );
	memcpy( m_buffer.Base() + offset, pData, len );
}


//-----------------------------------------------------------------------------
// Purpose: Writes a glview text file containing the collision surface in question
// Input  : *pCollide - 
//			*pFilename - 
//-----------------------------------------------------------------------------
void DumpCollideToGlView( CPhysCollide *pCollide, const char *pFilename )
{
	if ( !pCollide )
		return;

	Msg("Writing %s...\n", pFilename );
	Vector *outVerts;
	int vertCount = physcollision->CreateDebugMesh( pCollide, &outVerts );
	FILE *fp = fopen( pFilename, "w" );
	int triCount = vertCount / 3;
	int vert = 0;
	for ( int i = 0; i < triCount; i++ )
	{
		fprintf( fp, "3\n" );
		fprintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		fprintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		fprintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
	}
	fclose( fp );
	physcollision->DestroyDebugMesh( vertCount, outVerts );
}


void DumpCollideToPHY( CPhysCollide *pCollide, CTextBuffer *text, const char *pFilename )
{
	Msg("Writing %s...\n", pFilename );
	FILE *fp = fopen( pFilename, "wb" );
	phyheader_t header;
	header.size = sizeof(header);
	header.id = 0;
	header.checkSum = 0;
	header.solidCount = 1;
	fwrite( &header, sizeof(header), 1, fp );
	int size = physcollision->CollideSize( pCollide );
	fwrite( &size, sizeof(int), 1, fp );

	char *buf = (char *)stackalloc( size );
	physcollision->CollideWrite( buf, pCollide );
	fwrite( buf, size, 1, fp );

	fwrite( text->GetData(), text->GetSize(), 1, fp );
	fclose( fp );
}

CPhysCollisionEntry::CPhysCollisionEntry( CPhysCollide *pCollide )
{
	m_pCollide = pCollide;
}

unsigned int CPhysCollisionEntry::GetCollisionBinarySize() 
{ 
	return physcollision->CollideSize( m_pCollide ); 
}

unsigned int CPhysCollisionEntry::WriteCollisionBinary( char *pDest ) 
{ 
	return physcollision->CollideWrite( pDest, m_pCollide );
}

void CPhysCollisionEntry::DumpCollideFileName( const char *pName, int modelIndex, CTextBuffer *pTextBuffer )
{
	char tmp[128];
	sprintf( tmp, "%s%03d.phy", pName, modelIndex );
	DumpCollideToPHY( m_pCollide, pTextBuffer, tmp );
	sprintf( tmp, "%s%03d.txt", pName, modelIndex );
	DumpCollideToGlView( m_pCollide, tmp );
}


class CPhysCollisionEntrySolid : public CPhysCollisionEntry
{
public:
	CPhysCollisionEntrySolid( CPhysCollide *pCollide, const char *pMaterialName, float mass );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	float		m_volume;
	float		m_mass;
	const char *m_pMaterial;
};


CPhysCollisionEntrySolid::CPhysCollisionEntrySolid( CPhysCollide *pCollide, const char *pMaterialName, float mass )
	: CPhysCollisionEntry( pCollide )
{
	m_volume = physcollision->CollideVolume( m_pCollide );
	m_mass = mass;
	m_pMaterial = pMaterialName;
}

void CPhysCollisionEntrySolid::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	DumpCollideFileName( "collide", modelIndex, pTextBuffer );
}

void CPhysCollisionEntrySolid::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "solid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteFloatKey( "mass", m_mass );
	if ( m_pMaterial )
	{
		pTextBuffer->WriteStringKey( "surfaceprop", m_pMaterial );
	}
	if ( m_volume != 0.f )
	{
		pTextBuffer->WriteFloatKey( "volume", m_volume );
	}
	pTextBuffer->WriteText( "}\n" );
}


class CPhysCollisionEntryStaticSolid : public CPhysCollisionEntry
{
public:
	CPhysCollisionEntryStaticSolid ( CPhysCollide *pCollide, int contentsMask );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	int		m_contentsMask;
};


CPhysCollisionEntryStaticSolid ::CPhysCollisionEntryStaticSolid ( CPhysCollide *pCollide, int contentsMask )
	: CPhysCollisionEntry( pCollide ), m_contentsMask(contentsMask)
{
}

void CPhysCollisionEntryStaticSolid::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	char tmp[128];
	sprintf( tmp, "static%02d", modelIndex );
	DumpCollideFileName( tmp, collideIndex, pTextBuffer );
}

void CPhysCollisionEntryStaticSolid::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "staticsolid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteIntKey( "contents", m_contentsMask );
	pTextBuffer->WriteText( "}\n" );
}

CPhysCollisionEntryStaticMesh::CPhysCollisionEntryStaticMesh( CPhysCollide *pCollide, const char *pMaterialName )
	: CPhysCollisionEntry( pCollide )
{
	m_pMaterial = pMaterialName;
}

void CPhysCollisionEntryStaticMesh::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	char tmp[128];
	sprintf( tmp, "mesh%02d", modelIndex );
	DumpCollideFileName( tmp, collideIndex, pTextBuffer );
}

void CPhysCollisionEntryStaticMesh::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "staticsolid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteText( "}\n" );
}

class CPhysCollisionEntryFluid : public CPhysCollisionEntry
{
public:
	CPhysCollisionEntryFluid( CPhysCollide *pCollide, float density, float damping, const Vector &normal, float dist );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	float		m_density;
	float		m_damping;
	Vector 		m_surfaceNormal;
	float		m_surfaceDist;
};


CPhysCollisionEntryFluid::CPhysCollisionEntryFluid( CPhysCollide *pCollide, float density, float damping, const Vector &normal, float dist )
	: CPhysCollisionEntry( pCollide )
{
	m_density = density;
	m_damping = damping;
	m_surfaceNormal = normal;
	m_surfaceDist = dist;
}

void CPhysCollisionEntryFluid::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	char tmp[128];
	sprintf( tmp, "water%02d", modelIndex );
	DumpCollideFileName( tmp, collideIndex, pTextBuffer );
}

void CPhysCollisionEntryFluid::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "fluid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteFloatKey( "density", m_density );		// write out water density
	pTextBuffer->WriteFloatKey( "damping", m_damping );		// write out water damping
	float array[4];
	m_surfaceNormal.CopyToArray( array );
	array[3] = m_surfaceDist;
	pTextBuffer->WriteFloatArrayKey( "surfaceplane", array, 4 );		// write out water surface plane
	pTextBuffer->WriteFloatArrayKey( "currentvelocity", vec3_origin.Base(), 3 );		// write out water velocity
	pTextBuffer->WriteText( "}\n" );
}

// Get an index into the prop list of this prop (add it if necessary)
static int PropIndex( CUtlVector<int> &propList, int propIndex )
{
	for ( int i = 0; i < propList.Count(); i++ )
	{
		if ( propList[i] == propIndex )
			return i+1;
	}

	if ( propList.Count() < 126 )
	{
		return propList.AddToTail( propIndex )+1;
	}

	return 0;
}

int RemapWorldMaterial( int materialIndexIn )
{
	return PropIndex( s_WorldPropList, materialIndexIn );
}

typedef struct
{
	float normal[3];
	float dist;
} listplane_t;

static void AddListPlane( CUtlVector<listplane_t> *list, float x, float y, float z, float d )
{
	listplane_t plane;
	plane.normal[0] = x;
	plane.normal[1] = y;
	plane.normal[2] = z;
	plane.dist = d;

	list->AddToTail( plane );
}

class CPlaneList
{
public:

	CPlaneList( float shrink, float merge );
	~CPlaneList( void );

	void AddConvex( CPhysConvex *pConvex );

	// add the brushes to the model
	void AddBrushes( void );

	// Adds a single brush as a convex object
	void ReferenceBrush( int brushnumber );
	bool IsBrushReferenced( int brushnumber );

	void ReferenceLeaf( int leafIndex );
	bool IsLeafReferenced( int leafIndex );

	CUtlVector<CPhysConvex *>	m_convex;

	CUtlVector<int>				m_leafList;
	int							m_contentsMask;

	float						m_shrink;
	float						m_merge;
	bool						*m_brushAdded;
	float						m_totalVolume;
};

CPlaneList::CPlaneList( float shrink, float merge )
{
	m_shrink = shrink;
	m_merge = merge;
	m_contentsMask = MASK_SOLID;
	m_brushAdded = new bool[numbrushes];
	memset( m_brushAdded, 0, sizeof(bool) * numbrushes );
	m_totalVolume = 0;
	m_leafList.Purge();
}


CPlaneList::~CPlaneList( void )
{
	delete[] m_brushAdded;
}


void CPlaneList::AddConvex( CPhysConvex *pConvex )
{
	if ( pConvex )
	{
		m_totalVolume += physcollision->ConvexVolume( pConvex );
		m_convex.AddToTail( pConvex );
	}
}

// Adds a single brush as a convex object
void CPlaneList::ReferenceBrush( int brushnumber )
{
	if ( !(dbrushes[brushnumber].contents & m_contentsMask) )
		return;

	m_brushAdded[brushnumber] = true;

}


bool CPlaneList::IsBrushReferenced( int brushnumber )
{
	return m_brushAdded[brushnumber];
}


void CPlaneList::AddBrushes( void )
{
	CUtlVector<listplane_t> temp;
	for ( int brushnumber = 0; brushnumber < numbrushes; brushnumber++ )
	{
		if ( IsBrushReferenced(brushnumber) )
		{
			for ( int i = 0; i < dbrushes[brushnumber].numsides; i++ )
			{
				dbrushside_t *pside = dbrushsides + i + dbrushes[brushnumber].firstside;
				dplane_t *pplane = dplanes + pside->planenum;
				AddListPlane( &temp, pplane->normal[0], pplane->normal[1], pplane->normal[2], pplane->dist - m_shrink );
			}
			CPhysConvex *pConvex = physcollision->ConvexFromPlanes( (float *)temp.Base(), temp.Count(), m_merge );
			if ( pConvex )
			{
				physcollision->SetConvexGameData( pConvex, brushnumber );
				AddConvex( pConvex );
			}
			temp.RemoveAll();
		}
	}
}


// If I have a list of leaves, make sure this leaf is in it.
// Otherwise, process all leaves
bool CPlaneList::IsLeafReferenced( int leafIndex )
{
	if ( !m_leafList.Count() )
		return true;

	for ( int i = 0; i < m_leafList.Count(); i++ )
	{
		if ( m_leafList[i] == leafIndex )
			return true;
	}

	return false;
}

// Add a leaf to my list of interesting leaves
void CPlaneList::ReferenceLeaf( int leafIndex )
{
	m_leafList.AddToTail( leafIndex );
}

static void VisitLeaves_r( CPlaneList &planes, int node )
{
	if ( node < 0 )
	{
		int leafIndex = -1 - node;
		if ( planes.IsLeafReferenced(leafIndex) )
		{
			int i;

			// Add the solids in the "empty" leaf
			for ( i = 0; i < dleafs[leafIndex].numleafbrushes; i++ )
			{
				int brushIndex = dleafbrushes[dleafs[leafIndex].firstleafbrush + i];
				planes.ReferenceBrush( brushIndex );
			}
		}
	}
	else
	{
		dnode_t *pnode = dnodes + node;

		VisitLeaves_r( planes, pnode->children[0] );
		VisitLeaves_r( planes, pnode->children[1] );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static bool BuildFaceWindingData( dface_t *pFace, Vector points[4] )
{
    //
    // check for a quad surface
    //
    if( pFace->numedges != 4 )
        return false;

	for( int i = 0; i < 4; i++ )
	{
		int eIndex = dsurfedges[pFace->firstedge+i];
		if( eIndex < 0 )
		{
			VectorCopy( dvertexes[dedges[-eIndex].v[1]].point, points[i] );
		}
		else
		{
			VectorCopy( dvertexes[dedges[eIndex].v[0]].point, points[i] );
		}
	}

	return true;
}

struct waterleaf_t
{
	Vector	surfaceNormal;
	float	surfaceDist;
	float	minZ;
	bool	hasSurface;
	int		leafIndex;
	int		planenum;
	int		surfaceTexInfo; // if hasSurface == true, this is the texinfo index for the water material
};



// returns true if newleaf should appear before currentleaf in the list
static bool IsLowerLeaf( const waterleaf_t &newleaf, const waterleaf_t &currentleaf )
{
	if ( newleaf.hasSurface && currentleaf.hasSurface )
	{
		// the one with the upmost pointing z goes first
		float zdelta = newleaf.surfaceNormal.z - currentleaf.surfaceNormal.z;
		if ( zdelta <= 0.01f )
		{
			if ( newleaf.surfaceDist < currentleaf.surfaceDist )
				return true;
		}
		else if ( zdelta > 0 )
			return true;
	}
	else if ( newleaf.hasSurface )		// the leaf with a surface always goes first
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:   Water surfaces are stored in an RB tree and the tree is used to 
//  create one-off .vmt files embedded in the .bsp for each surface so that the
//  water depth effect occurs on a per-water surface level.
//-----------------------------------------------------------------------------
struct WaterTexInfo
{
	// The mangled new .vmt name ( materials/levelename/oldmaterial_depth_xxx ) where xxx is
	//  the water depth (as an integer )
	CUtlSymbol  m_FullName;

	// The original .vmt name
	CUtlSymbol	m_MaterialName;

	// The depth of the water this texinfo refers to
	int			m_nWaterDepth;

	// The texinfo id
	int			m_nTexInfo;

	// The subdivision size for the water surface
//	float		m_SubdivSize;
};

//-----------------------------------------------------------------------------
// Purpose: Helper for RB tree operations ( we compare full mangled names )
// Input  : src1 - 
//			src2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WaterLessFunc( WaterTexInfo const& src1, WaterTexInfo const& src2 )
{
	return src1.m_FullName < src2.m_FullName;
}

//-----------------------------------------------------------------------------
// Purpose: A growable RB tree of water surfaces
//-----------------------------------------------------------------------------
static CUtlRBTree< WaterTexInfo, int > g_WaterTexInfos( 0, 32, WaterLessFunc );

#if 0
float GetSubdivSizeForFogVolume( int fogVolumeID )
{
	assert( fogVolumeID >= 0 && fogVolumeID < g_WaterTexInfos.Count() );
	return g_WaterTexInfos[fogVolumeID].m_SubdivSize;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mapname - 
//			*materialname - 
//			waterdepth - 
//			*fullname - 
//-----------------------------------------------------------------------------
void GetWaterTextureName( char const *mapname, char const *materialname, int waterdepth, char *fullname  )
{
	char temp[ 512 ];

	// Construct the full name (prepend mapname to reduce name collisions)
	sprintf( temp, "maps/%s/%s_depth_%i", mapname, materialname, (int)waterdepth );

	// Make sure it's lower case
	strlwr( temp );

	strcpy( fullname, temp );
}

//-----------------------------------------------------------------------------
// Purpose: Called to write procedural materials in the rb tree to the embedded
//  pak file for this .bsp
//-----------------------------------------------------------------------------
void EmitWaterMaterialFile( WaterTexInfo *wti )
{
	char waterTextureName[512];
	if ( !wti )
	{
		return;
	}

	GetWaterTextureName( mapbase, wti->m_MaterialName.String(), ( int )wti->m_nWaterDepth, waterTextureName );
	
	// Convert to string
	char szDepth[ 32 ];
	sprintf( szDepth, "%i", wti->m_nWaterDepth );

	CreateMaterialPatch( wti->m_MaterialName.String(), waterTextureName, "$waterdepth",
		szDepth );
}

//-----------------------------------------------------------------------------
// Purpose: Takes the texinfo_t referenced by the .vmt and the computed depth for the
//  surface and looks up or creates a texdata/texinfo for the mangled one-off water .vmt file
// Input  : *pBaseInfo - 
//			depth - 
// Output : int
//-----------------------------------------------------------------------------
int FindOrCreateWaterTexInfo( texinfo_t *pBaseInfo, float depth )
{
	char fullname[ 512 ];
	char materialname[ 512 ];

	// Get the base texture/material name
	char const *name = TexDataStringTable_GetString( GetTexData( pBaseInfo->texdata )->nameStringTableID );

	GetWaterTextureName( mapbase, name, (int)depth, fullname );

	// See if we already have an entry for this depth
	WaterTexInfo lookup;
	lookup.m_FullName = fullname;
	int idx = g_WaterTexInfos.Find( lookup );

	// If so, return the existing entry texinfo index
	if ( idx != g_WaterTexInfos.InvalidIndex() )
	{
		return g_WaterTexInfos[ idx ].m_nTexInfo;
	}

	// Otherwise, fill in the rest of the data
	lookup.m_nWaterDepth = (int)depth;
	// Remember the current material name
	sprintf( materialname, "%s", name );
	strlwr( materialname );
	lookup.m_MaterialName = materialname;

	texinfo_t ti;
	// Make a copy
	ti = *pBaseInfo;
	// Create a texdata that is based on the underlying existing entry
	ti.texdata = FindAliasedTexData( fullname, GetTexData( pBaseInfo->texdata ) );

	// Find or create a new index
	lookup.m_nTexInfo = FindOrCreateTexInfo( ti );

	// Add the new texinfo to the RB tree
	idx = g_WaterTexInfos.Insert( lookup );

	// Msg( "created texinfo for %s\n", lookup.m_FullName.String() );

	// Go ahead and create the new vmt file.
	EmitWaterMaterialFile( &g_WaterTexInfos[idx] );

	// Return the new texinfo
	return g_WaterTexInfos[ idx ].m_nTexInfo;
}

//-----------------------------------------------------------------------------
// Purpose: Iterates through the list of water leaves and for each substitutes
//  the one-off textures for the original textures for the appropriate surfaces
// Input  : &list - 
// Output : static void
//-----------------------------------------------------------------------------
static void ApplyDepthMaterials( CUtlVector<waterleaf_t> &list )
{
	int i;
	
	for( i = 0; i < list.Count(); i++ )
	{
		waterleaf_t *pWaterLeaf = &list[i];
		dleaf_t *pLeaf = &dleafs[pWaterLeaf->leafIndex];
		int j;
		for( j = 0; j < pLeaf->numleaffaces; j++ )
		{
			unsigned short leafFaceID = dleaffaces[j+pLeaf->firstleafface];
			int k;
			for( k = 0; k < pLeaf->numleaffaces; k++ )
			{
				dface_t *pFace = &dfaces[leafFaceID+k];
				const textureref_t &textureRef = textureref[texinfo[pFace->texinfo].texdata];
				if( ( pFace->planenum == pWaterLeaf->planenum ) && 
					( textureRef.flags & SURF_WARP ) )
				{
					pFace->texinfo = pWaterLeaf->surfaceTexInfo;
				}
			}
		}
	}
}

static int FindWaterSurfaceTexInfo( int leafID, int planeNum )
{
	assert( leafID >= 0 && leafID < numleafs );
	assert( planeNum >= 0 && planeNum < numplanes );
	dleaf_t *pLeaf = dleafs + leafID;
	int i;
	for( i = 0; i < pLeaf->numleaffaces; i++ )
	{
		unsigned short leafFaceID = dleaffaces[i+pLeaf->firstleafface];
		// fixme: should we look at more faces here as below?
		dface_t *pFace = &dfaces[leafFaceID];
		// FIXME: why would the plane ever face the other way?
		// actually, it should always face the other way!  check for this (garymcthack)
		if( ( pFace->planenum & ~1 ) == ( planeNum & ~1 ) )
		{
			// found the right plane. . .  now make sure it's water
			assert( pFace->texinfo >= 0 && pFace->texinfo < numtexinfo );
			texinfo_t *pTexInfo = &texinfo[pFace->texinfo];
			if( pTexInfo->flags & SURF_WARP )
			{
				return pFace->texinfo;
			}
		}
	}
//	assert( 0 ); // we really should never get here.
	return -1;
}

static void FindVolumeID( CUtlVector<waterleaf_t> &list )
{
	int i;
	
	// clear 'em all.
	for( i = 0; i < numfaces; i++ )
	{
		dface_t *pFace = &dfaces[i];
		pFace->surfaceFogVolumeID = -1;
	}
	
	for( i = 0; i < list.Count(); i++ )
	{
		waterleaf_t *pWaterLeaf = &list[i];
		dleaf_t *pLeaf = &dleafs[pWaterLeaf->leafIndex];
		int j;
		for( j = 0; j < pLeaf->numleaffaces; j++ )
		{
			unsigned short leafFaceID = dleaffaces[j+pLeaf->firstleafface];
			int k;
			for( k = 0; k < pLeaf->numleaffaces; k++ )
			{
				dface_t *pFace = &dfaces[leafFaceID+k];
				// FIXME: why would the plane ever face the other way?
				// actually, it should always face the other way!  check for this (garymcthack)
				if( ( pFace->planenum & ~1 ) == ( pWaterLeaf->planenum & ~1 ) )
				{
					// found the right plane. . .  now make sure it's water
					assert( pFace->texinfo >= 0 && pFace->texinfo < numtexinfo );
					texinfo_t *pTexInfo = &texinfo[pFace->texinfo];
					if( pTexInfo->flags & SURF_WARP )
					{
						pFace->surfaceFogVolumeID = pLeaf->leafWaterDataID;
					}
				}
			}
		}
	}
}

// insert sort this leaf into the list and check for water surfaces in the leaf
static void AddLeaf( int leafIndex, CUtlVector<waterleaf_t> &list, int *clustermap )
{
	int i;

	waterleaf_t leaf;
	leaf.leafIndex = leafIndex;
	leaf.hasSurface = false;
	leaf.surfaceDist = 9999;
	leaf.surfaceNormal.Init( 0.f, 0.f, 1.f );
	leaf.planenum = -1;
	leaf.surfaceTexInfo = -1;
	dleaf_t *pleaf = dleafs + leafIndex;
	dcluster_t *cluster = dclusters + pleaf->cluster;

	int bestcluster = -1;


	// search the list of portals out of this cluster for one that leaves water
	for ( i = 0; i < cluster->numportals; i++ )
	{
		int portalIndex = dclusterportals[cluster->firstportal + i];
		dportal_t *portal = dportals + portalIndex;
		int cluster = portal->cluster[0];
		int planeIndex = portal->planenum;

		if ( portal->cluster[0] == pleaf->cluster )
		{
			// this portal goes into this leaf, not out so choose the opposite plane
			cluster = portal->cluster[1];
			
			// invert the last bit of planeIndex to choose the opposing plane
			planeIndex += (planeIndex & 1) ? -1 : 1;
		}

		if ( !(dleafs[clustermap[cluster]].contents & MASK_WATER) && !(dleafs[clustermap[cluster]].contents & MASK_SOLID) )
		{
			// this is a portal out of water.
			// Its plane is the water surface
			dplane_t *plane = dplanes + planeIndex;
			// If we already have one, then pick the one that points up the most and 
			// is the highest.
			if ( leaf.hasSurface )
			{
				if ( leaf.surfaceNormal.z > plane->normal.z )
					continue;
				if ( (leaf.surfaceNormal.z == plane->normal.z) && leaf.surfaceDist >= plane->dist )
					continue;
			}
			leaf.surfaceDist = plane->dist;
			leaf.surfaceNormal = plane->normal;
			leaf.hasSurface = true;
			leaf.planenum = planeIndex;

			bestcluster = clustermap[cluster];
		}
	}

	if ( bestcluster != -1 )
	{
		leaf.surfaceTexInfo = FindWaterSurfaceTexInfo( bestcluster, leaf.planenum );
	}

	// insertion sort the leaf (lowest leaves go first)
	// leaves that aren't actually on the surface of the water will have leaf.hasSurface == false.
	for ( i = 0; i < list.Count(); i++ )
	{
		if ( IsLowerLeaf( leaf, list[i] ) )
		{
			list.InsertBefore( i, leaf );
			return;
		}
	}

	// must the highest one, so stick it at the end.
	list.AddToTail( leaf );
}

// recursively build a list of leaves that contain water
static void WaterLeaf_r( int node, CUtlVector<waterleaf_t> &list, int *clustermap )
{
	if ( node < 0 )
	{
		int leafIndex = -1 - node;
		if ( dleafs[leafIndex].contents & MASK_WATER )
		{
			AddLeaf( leafIndex, list, clustermap );
		}
	}
	else
	{
		dnode_t *pnode = dnodes + node;
		WaterLeaf_r( pnode->children[0], list, clustermap );
		WaterLeaf_r( pnode->children[1], list, clustermap );
	}
}


// Returns true if the portal has any verts on the front side of the baseleaf's water surface plane
static bool PortalCrossesWater( waterleaf_t &baseleaf, dportal_t *portal )
{
	if ( baseleaf.hasSurface )
	{
		for ( int i = 0; i < portal->numportalverts; i++ )
		{
			dvertex_t *vert = &dvertexes[ dportalverts[portal->firstportalvert+i] ];
			float dot = DotProduct( vert->point, baseleaf.surfaceNormal ) - baseleaf.surfaceDist;
			
			// UNDONE: Tune this epsilon
			if ( dot > 0.25f )
				return true;
		}
	}

	return false;
}


static void DumpWaterLeafs( CUtlVector<waterleaf_t> &list )
{
	FILE *fp = fopen( "jay.txt", "w" );
	for ( int i = 0; i < list.Count(); i++ )
	{
		dleaf_t *pleaf = dleafs + list[i].leafIndex;
		dcluster_t *cluster = dclusters + pleaf->cluster;
		for ( int j = 0; j < cluster->numportals; j++ )
		{
			int portalIndex = dclusterportals[cluster->firstportal + j];
			dportal_t *portal = dportals + portalIndex;
			dvertex_t *basevert = &dvertexes[dportalverts[portal->firstportalvert]];
			for ( int k = 1; k < portal->numportalverts-1; k++ )
			{
				dvertex_t *vert1 = &dvertexes[dportalverts[portal->firstportalvert+k]];
				dvertex_t *vert2 = &dvertexes[dportalverts[portal->firstportalvert+k+1]];
				fprintf( fp, "3\n" );
				fprintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", basevert->point.x, basevert->point.y, basevert->point.z );
				fprintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", vert1->point.x, vert1->point.y, vert1->point.z );
				fprintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", vert2->point.x, vert2->point.y, vert2->point.z );
			}
		}
	}

	fclose( fp );
}

// flood fill the bsp using the cluster map
// don't cross the original water surface plane because the buoyancy solver needs
// a single water surface plane
static void LeafFlood_r( CPlaneList &planes, waterleaf_t &baseleaf, bool *clusterVisit, 
						 int *clusterMap, int leafIndex )
{
	int i;

	dleaf_t *pleaf = dleafs + leafIndex;
	
	// already visited
	if ( clusterVisit[pleaf->cluster] )
		return;

	dcluster_t *cluster = dclusters + pleaf->cluster;

	if ( leafIndex != baseleaf.leafIndex )
	{
		for ( i = 0; i < cluster->numportals; i++ )
		{
			int portalIndex = dclusterportals[cluster->firstportal + i];
			dportal_t *portal = dportals + portalIndex;
			
			// same surface . .can't possibly cross plane.
			if ( baseleaf.hasSurface && baseleaf.planenum == portal->planenum )
				continue;

			// If any portal crosses the water surface, don't flow through this leaf
			if ( PortalCrossesWater( baseleaf, portal ) )
				return;
		}
	}

	// this only works properly if there is one cluster per leaf
	clusterVisit[pleaf->cluster] = true;
	planes.ReferenceLeaf( leafIndex );

	for ( i = 0; i < cluster->numportals; i++ )
	{
		int portalIndex = dclusterportals[cluster->firstportal + i];
		dportal_t *portal = dportals + portalIndex;
		
		if ( portal->cluster[0] == pleaf->cluster )
		{
			LeafFlood_r( planes, baseleaf, clusterVisit, clusterMap, clusterMap[portal->cluster[1]] );
		}
		else
		{
			LeafFlood_r( planes, baseleaf, clusterVisit, clusterMap, clusterMap[portal->cluster[0]] );
		}
	}
}


static int FindOrCreateLeafWaterData( float surfaceZ, float minZ, int surfaceTexInfoID )
{
	int i;
	for( i = 0; i < numleafwaterdata; i++ )
	{
		dleafwaterdata_t *pLeafWaterData = &dleafwaterdata[i];
		if( pLeafWaterData->surfaceZ == surfaceZ &&
			pLeafWaterData->minZ == minZ &&
			pLeafWaterData->surfaceTexInfoID == surfaceTexInfoID )
		{
			return i;
		}
	}
	dleafwaterdata_t *pLeafWaterData = &dleafwaterdata[numleafwaterdata];
	pLeafWaterData->surfaceZ = surfaceZ;
	pLeafWaterData->minZ = minZ;
	pLeafWaterData->surfaceTexInfoID = surfaceTexInfoID;
	numleafwaterdata++;
	return numleafwaterdata - 1;
}

static void FixupLeafWaterData( CPlaneList &planes, int *leafIDToWaterLeaf, 
							    CUtlVector<waterleaf_t> &waterLeafs )
{
	int j;
	int texinfoID = -1;
	float surfaceZ;
	float minZ = FLT_MAX;
	int waterLeafCount = 0;
	for( j = 0; j < planes.m_leafList.Count(); j++ )
	{
		int waterLeafID = leafIDToWaterLeaf[planes.m_leafList[j]];
		if( waterLeafID == -1 )
		{
			continue;
		}

		int leafID = planes.m_leafList[j];
		assert( leafID >= 0 && leafID < numleafs );
		dleaf_t *pLeaf = &dleafs[leafID];
		if ( !(pLeaf->contents & CONTENTS_WATER) )
			continue;

		waterLeafCount++;
		if( pLeaf->mins[2] < minZ )
		{
			minZ = pLeaf->mins[2];
		}

		assert( waterLeafID >= 0 && waterLeafID < waterLeafs.Count() );
		waterleaf_t *pWaterLeaf = &waterLeafs[waterLeafID];
		if( pWaterLeaf->surfaceTexInfo != -1 )
		{
			// garymcthack : this should probably be a user warning/error.
			//assert( texinfoID == -1 || texinfoID == pWaterLeaf->surfaceTexInfo );
			texinfoID = pWaterLeaf->surfaceTexInfo;
			assert( pWaterLeaf->hasSurface );
			surfaceZ = pWaterLeaf->surfaceDist;
		}
		if( pLeaf->mins[2] < minZ )
		{
			minZ = pLeaf->mins[2];
		}
	}

	if ( waterLeafCount <= 0 )
		return;

	for( j = 0; j < planes.m_leafList.Count(); j++ )
	{
		int waterLeafID = leafIDToWaterLeaf[planes.m_leafList[j]];
		if( waterLeafID == -1 )
		{
			continue;
		}

		int leafID = planes.m_leafList[j];
		assert( leafID >= 0 && leafID < numleafs );
		dleaf_t *pLeaf = &dleafs[leafID];
		assert( waterLeafID >= 0 && waterLeafID < waterLeafs.Count() );
		waterleaf_t *pWaterLeaf = &waterLeafs[waterLeafID];
		assert( pWaterLeaf );
		// Create a one-off .vmt reference for water of the specified depth based on the original texinfoID for
		//  this water surface
		pWaterLeaf->surfaceTexInfo = FindOrCreateWaterTexInfo( &texinfo[ texinfoID ], surfaceZ - minZ );
	}

	assert( texinfoID != -1 );
	int leafWaterDataID = FindOrCreateLeafWaterData( surfaceZ, minZ, texinfoID );
	for( j = 0; j < planes.m_leafList.Count(); j++ )
	{
		int leafID = planes.m_leafList[j];
		assert( leafID >= 0 && leafID < numleafs );
		dleaf_t *pLeaf = &dleafs[leafID];
		pLeaf->leafWaterDataID = leafWaterDataID;
	}
}

static BuildLeafIDToWaterLeafMap( CUtlVector<waterleaf_t> &waterList, int *leafIDToWaterLeaf )
{
	int i;
	for( i = 0; i < numleafs; i++ )
	{
		leafIDToWaterLeaf[i] = -1;
	}
	for( i = 0; i < waterList.Count(); i++ )
	{
		leafIDToWaterLeaf[waterList[i].leafIndex] = i;
	}
}

static void ConvertWaterModelToPhysCollide( CUtlVector<CPhysCollisionEntry *> &collisionList, int modelIndex, 
										    float shrinkSize, float mergeTolerance )
{
	int i;
	dmodel_t *pModel = dmodels + modelIndex;
	CUtlVector<waterleaf_t> waterList;

	// map of each cluster to it's leaf
	int leafCluster[MAX_MAP_LEAFS];
	for ( i = 0; i < numleafs; i++ )
		leafCluster[i] = -1;

	for ( i = 1; i < numleafs; i++ )
	{
		if ( dleafs[i].cluster >= 0 )
		{
			if ( leafCluster[dleafs[i].cluster] >= 0 )
			{
				// Map probably leaked or is bad.  The cluster information is unreliable,
				// so we probably can't make fluid controllers out of volumes...
				Msg( "Bad water leaf!!\nNo water physics.\n" );
				return;
			}
			leafCluster[dleafs[i].cluster] = i;
		}
	}

	waterList.Purge();
	// get a list of all water leaves
	WaterLeaf_r( pModel->headnode, waterList, leafCluster );

	bool clusterVisit[MAX_MAP_LEAFS];
	// no water leaves
	if ( !waterList.Count() )
		return;

	for ( i = 0; i < numleafs; i++ )
		clusterVisit[i] = true;

	// clear the visit flag on all water clusters
	for ( i = 0; i < waterList.Count(); i++ )
	{
		dleaf_t *pleaf = &dleafs[waterList[i].leafIndex];
		if ( leafCluster[pleaf->cluster] != waterList[i].leafIndex )
		{
			// something hosed the list?!?!!?
			Error( "Water list error.\n" );
			return;
		}
		clusterVisit[pleaf->cluster] = false;
	}

	// At this point, we have a list of water leaves (waterList) that are 
	// sorted with the lowest surfaces first.
	for ( int startLeaf = 0; startLeaf < waterList.Count(); startLeaf++ )
	{
		dleaf_t *pleaf = dleafs + waterList[startLeaf].leafIndex;
		if ( clusterVisit[pleaf->cluster] )
			continue;

		CPlaneList planes( shrinkSize, mergeTolerance );
		// flood fill the bsp using the cluster map
		// don't cross the original water surface plane because the buoyancy solver needs
		// a single water surface plane
		LeafFlood_r( planes, waterList[startLeaf], clusterVisit, leafCluster, waterList[startLeaf].leafIndex );

		static int leafIDToWaterLeaf[MAX_MAP_LEAFS];
		BuildLeafIDToWaterLeafMap( waterList, leafIDToWaterLeaf );

		// Set the waterinfo data for each leaf.
		FixupLeafWaterData( planes, leafIDToWaterLeaf, waterList );
		
		planes.m_contentsMask = MASK_WATER;

		// Stick all of the brushes models in any "empty" leaves into the planelist.
		VisitLeaves_r( planes, pModel->headnode );
		planes.AddBrushes();

		int count = planes.m_convex.Count();
		if ( !count )
			continue;

		// Save off the plane of the surface for this group as well as the collision model
		// for all convex objects in the group.
		CPhysCollide *pCollide = physcollision->ConvertConvexToCollide( planes.m_convex.Base(), count );
		if ( pCollide )
		{
			// use defaults
			float damping = 0.01;
			float density = 1000;

			int waterSurfaceTexInfoID = -1;
			if ( waterSurfaceTexInfoID >= 0 )
			{
				// material override
				int texdata = texinfo[waterSurfaceTexInfoID].texdata;
				int prop = g_SurfaceProperties[texdata];
				physprops->GetPhysicsProperties( prop, &density, NULL, NULL, &damping );
			}
			collisionList.AddToTail( new CPhysCollisionEntryFluid( pCollide, density, damping, waterList[startLeaf].surfaceNormal, waterList[startLeaf].surfaceDist ) );
		}
	}

	FindVolumeID( waterList );

	// Now go through and swap in the special .vmt file references for water surfaces
	ApplyDepthMaterials( waterList );
}

// compute a normal for a triangle of the given three points (points are clockwise, normal points out)
static Vector TriangleNormal( const Vector &p0, const Vector &p1, const Vector &p2 )
{
	Vector e0 = p1 - p0;
	Vector e1 = p2 - p0;
	Vector normal = CrossProduct( e1, e0 );
	VectorNormalize( normal );
	
	return normal;
}


// find the side of the brush with the normal closest to the given normal
static dbrushside_t *FindBrushSide( int brushIndex, const Vector &normal )
{
	dbrush_t *pbrush = &dbrushes[brushIndex];
	dbrushside_t *out = NULL;
	float best = -1.f;

	for ( int i = 0; i < pbrush->numsides; i++ )
	{
		dbrushside_t *pside = dbrushsides + i + pbrush->firstside;
		dplane_t *pplane = dplanes + pside->planenum;
		float dot = DotProduct( normal, pplane->normal );
		if ( dot > best )
		{
			best = dot;
			out = pside;
		}
	}

	return out;
}



static void ConvertWorldBrushesToPhysCollide( CUtlVector<CPhysCollisionEntry *> &collisionList, float shrinkSize, float mergeTolerance, int contentsMask )
{
	CPlaneList planes( shrinkSize, mergeTolerance );

	planes.m_contentsMask = contentsMask;

	VisitLeaves_r( planes, dmodels[0].headnode );
	planes.AddBrushes();
	
	int count = planes.m_convex.Count();
	if ( count )
	{
		CPhysCollide *pCollide = physcollision->ConvertConvexToCollide( planes.m_convex.Base(), count );

		ICollisionQuery *pQuery = physcollision->CreateQueryModel( pCollide );
		int convex = pQuery->ConvexCount();
		for ( int i = 0; i < convex; i++ )
		{
			int triCount = pQuery->TriangleCount( i );
			int brushIndex = pQuery->GetGameData( i );

			Vector points[3];
			for ( int j = 0; j < triCount; j++ )
			{
				pQuery->GetTriangleVerts( i, j, points );
				Vector normal = TriangleNormal( points[0], points[1], points[2] );
				dbrushside_t *pside = FindBrushSide( brushIndex, normal );
				int prop = g_SurfaceProperties[texinfo[pside->texinfo].texdata];
				pQuery->SetTriangleMaterialIndex( i, j, RemapWorldMaterial( prop ) );
			}
		}
		physcollision->DestroyQueryModel( pQuery );
		pQuery = NULL;

		collisionList.AddToTail( new CPhysCollisionEntryStaticSolid( pCollide, contentsMask ) );
	}
}

// adds any world, terrain, and water collision models to the collision list
static void BuildWorldPhysModel( CUtlVector<CPhysCollisionEntry *> &collisionList, float shrinkSize, float mergeTolerance )
{
	ConvertWorldBrushesToPhysCollide( collisionList, shrinkSize, mergeTolerance, MASK_SOLID );
	ConvertWorldBrushesToPhysCollide( collisionList, shrinkSize, mergeTolerance, CONTENTS_PLAYERCLIP );
	ConvertWorldBrushesToPhysCollide( collisionList, shrinkSize, mergeTolerance, CONTENTS_MONSTERCLIP );

	// if there's terrain, save it off as a static mesh/polysoup
	Disp_AddCollisionModels( collisionList, &dmodels[0], MASK_SOLID );
	ConvertWaterModelToPhysCollide( collisionList, 0, shrinkSize, mergeTolerance );
}


// adds a collision entry for this brush model
static void ConvertModelToPhysCollide( CUtlVector<CPhysCollisionEntry *> &collisionList, int modelIndex, int contents, float shrinkSize, float mergeTolerance )
{
	CPlaneList planes( shrinkSize, mergeTolerance );

	planes.m_contentsMask = contents;

	dmodel_t *pModel = dmodels + modelIndex;
	VisitLeaves_r( planes, pModel->headnode );
	planes.AddBrushes();

	int count = planes.m_convex.Count();
	CPhysCollide *pCollide = physcollision->ConvertConvexToCollide( planes.m_convex.Base(), count );
	
	if ( !pCollide )
		return;

	struct 
	{
		int prop;
		float area;
	} proplist[256];
	int numprops = 0;

	// compute the array of props on the surface of this model
	int i;
	for ( i = 0; i < dmodels[modelIndex].numfaces; i++ )
	{
		dface_t *face = dfaces + i + dmodels[modelIndex].firstface;
		int texdata = texinfo[face->texinfo].texdata;
		int prop = g_SurfaceProperties[texdata];
		int j;
		for ( j = 0; j < numprops; j++ )
		{
			if ( proplist[j].prop == prop )
			{
				proplist[j].area += face->area;
				break;
			}
		}

		if ( (!numprops || j >= numprops) && numprops < ARRAYSIZE(proplist) )
		{
			proplist[numprops].prop = prop;
			proplist[numprops].area = face->area;
			numprops++;
		}
	}


	// choose the prop with the most surface area
	int maxIndex = -1;
	float maxArea = 0;
	float totalArea = 0;

	for ( i = 0; i < numprops; i++ )
	{
		if ( proplist[i].area > maxArea )
		{
			maxIndex = i;
			maxArea = proplist[i].area;
		}
		// add up the total surface area
		totalArea += proplist[i].area;
	}
	
	float mass = 1.0f;
	const char *pMaterial = "default";
	if ( maxIndex >= 0 )
	{
		int prop = proplist[maxIndex].prop;
		
		// use default if this material has no prop
		if ( prop < 0 )
			prop = 0;

		pMaterial = physprops->GetPropName( prop );
		float density, thickness;
		physprops->GetPhysicsProperties( prop, &density, &thickness, NULL, NULL );

		// if this is a "shell" material (it is hollow and encloses some empty space)
		// compute the mass with a constant surface thickness
		if ( thickness != 0 )
		{
			mass = totalArea * thickness * density * CUBIC_METERS_PER_CUBIC_INCH;
		}
		else
		{
			// material is completely solid, compute total mass as if constant density throughout.
			mass = planes.m_totalVolume * density * CUBIC_METERS_PER_CUBIC_INCH;
		}
	}

	// Clamp mass to 100,000 kg
	if ( mass > 1e5f )
	{
		mass = 1e5f;
	}

	collisionList.AddToTail( new CPhysCollisionEntrySolid( pCollide, pMaterial, mass ) );
}

static void ClearLeafWaterData( void )
{
	int i;

	for( i = 0; i < numleafs; i++ )
	{
		dleafs[i].leafWaterDataID = -1;
		dleafs[i].contents &= ~CONTENTS_TESTFOGVOLUME;
	}
}


// This is the only public entry to this file.
// The global data touched in the file is:
// from bsplib.h:
//		g_pPhysCollide		: This is an output from this file.
//		g_PhysCollideSize	: This is set in this file.
//		g_numdispinfo		: This is an input to this file.
//		g_dispinfo			: This is an input to this file.
//		numnodewaterdata	: This is an output from this file.
//		dleafwaterdata		: This is an output from this file.
// from vbsp.h:
//		g_SurfaceProperties : This is an input to this file.
// UNDONE: refactor this into something that handles objects
// that produce solids (model/mesh/water) more abstractly
// This shrinks everything by 1/4th of an inch, and merges points closer than 0.01in, 12in border
void EmitPhysCollision( void )
{
	ClearLeafWaterData();
	
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( physicsFactory )
	{
		physcollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
	}

	if ( !physcollision )
	{
		Warning("!!! WARNING: Can't build collision data!\n" );
		return;
	}

	CUtlVector<CPhysCollisionEntry *> collisionList[MAX_MAP_MODELS];
	CTextBuffer *pTextBuffer[MAX_MAP_MODELS];

	int physModelCount = 0, totalSize = 0;

	int start = I_FloatTime();

	Msg("Building Physics collision data...\n" );

	int i, j;
	for ( i = 0; i < nummodels; i++ )
	{
		// Build a list of collision models for this brush model section
		if ( i == 0 )
		{
			// world is the only model that processes water separately.
			// other brushes are assumed to be completely solid or completely liquid
			BuildWorldPhysModel( collisionList[i], VPHYSICS_SHRINK, VPHYSICS_MERGE );
		}
		else
		{
			ConvertModelToPhysCollide( collisionList[i], i, MASK_SOLID|MASK_WATER, VPHYSICS_SHRINK, VPHYSICS_MERGE );
		}
		
		pTextBuffer[i] = NULL;
		if ( !collisionList[i].Count() )
			continue;

		// if we've got collision models, write their script for processing in the game
		pTextBuffer[i] = new CTextBuffer;
		for ( j = 0; j < collisionList[i].Count(); j++ )
		{
			// dump a text file for visualization
			if ( dumpcollide )
			{
				collisionList[i][j]->DumpCollide( pTextBuffer[i], i, j );
			}
			// each model knows how to write its script
			collisionList[i][j]->WriteToTextBuffer( pTextBuffer[i], i, j );
			// total up the binary section's size
			totalSize += collisionList[i][j]->GetCollisionBinarySize() + sizeof(int);
		}

		if ( i == 0 && s_WorldPropList.Count() )
		{
			pTextBuffer[i]->WriteText( "materialtable {\n" );
			for ( j = 0; j < s_WorldPropList.Count(); j++ )
			{
				int propIndex = s_WorldPropList[j];
				if ( propIndex < 0 )
				{
					pTextBuffer[i]->WriteIntKey( "default", j+1 );
				}
				else
				{
					pTextBuffer[i]->WriteIntKey( physprops->GetPropName( propIndex ), j+1 );
				}
			}
			pTextBuffer[i]->WriteText( "}\n" );
		}

		pTextBuffer[i]->Terminate();

		// total lump size includes the text buffers (scripts)
		totalSize += pTextBuffer[i]->GetSize();

		physModelCount++;
	}

	//  add one for tail of list marker
	physModelCount++;	

	g_PhysCollideSize = totalSize + (physModelCount * sizeof(dphysmodel_t));

	// DWORD align the lump because AddLump assumes that it is DWORD aligned.
	g_pPhysCollide = (byte *)malloc(( g_PhysCollideSize + 3 ) & ~3 );
	memset( g_pPhysCollide, 0, g_PhysCollideSize );

	byte *ptr = g_pPhysCollide;
	for ( i = 0; i < nummodels; i++ )
	{
		if ( pTextBuffer[i] )
		{
			int j;

			dphysmodel_t model;

			model.modelIndex = i;
			model.solidCount = collisionList[i].Count();
			model.dataSize = sizeof(int) * model.solidCount;

			for ( j = 0; j < model.solidCount; j++ )
			{
				model.dataSize += collisionList[i][j]->GetCollisionBinarySize();
			}
			model.keydataSize = pTextBuffer[i]->GetSize();

			// store the header
			memcpy( ptr, &model, sizeof(model) );
			ptr += sizeof(model);

			for ( j = 0; j < model.solidCount; j++ )
			{
				int collideSize = collisionList[i][j]->GetCollisionBinarySize();

				// write size
				memcpy( ptr, &collideSize, sizeof(int) );
				ptr += sizeof(int);

				// now write the collision model
				collisionList[i][j]->WriteCollisionBinary( reinterpret_cast<char *>(ptr) );
				ptr += collideSize;
			}

			memcpy( ptr, pTextBuffer[i]->GetData(), pTextBuffer[i]->GetSize() );
			ptr += pTextBuffer[i]->GetSize();
		}

		delete pTextBuffer[i];
	}

	dphysmodel_t model;

	// Mark end of list
	model.modelIndex = -1;
	model.dataSize = -1;
	model.keydataSize = 0;
	model.solidCount = 0;
	memcpy( ptr, &model, sizeof(model) );
	ptr += sizeof(model);
	assert( (ptr-g_pPhysCollide) == g_PhysCollideSize);
	Msg("done (%d) (%d bytes)\n", (int)(I_FloatTime() - start), g_PhysCollideSize );

	// UNDONE: Collision models (collisionList) memory leak!
}
