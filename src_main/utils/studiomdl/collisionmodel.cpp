//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Builds physics collision models from studio model source
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

// NOTE: The term joint here is used to mean a bone, collision model, and a joint.
// Each "joint" is the collision geometry at a named bone (or set of bones that have been merged)
// and the joint (with constraints) between that set and its parent.  The root "joint" has
// no constraints.
// I chose to refer to them as joints to avoid confusion.  Yes they encompass bones and joints,
// but they use the same names, and the data is actually linked.

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>

#include "vphysics/constraints.h"
#include "collisionmodel.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "studio.h"
#include "studiomdl.h"
#include "physdll.h"
#include "phyfile.h"
#include "utlvector.h"
#include "vcollide_parse.h"
#include "vstdlib/strtools.h"

// these functions just wrap atoi/atof and check for NULL
static float Safe_atof( const char *pString );
static int Safe_atoi( const char *pString );

IPhysicsCollision *physcollision = NULL;
IPhysicsSurfaceProps *physprops = NULL;

//-----------------------------------------------------------------------------
// Purpose: Contains a single convex element of a physical collision system
//-----------------------------------------------------------------------------
class CPhysCollisionModel
{
public:
	CPhysCollisionModel( void )
	{
		memset( this, 0, sizeof(*this) );
	}

	const char	*m_parent;
	const char	*m_name;

	// physical properties stored on disk
	float		m_mass;
	float		m_volume;
	float		m_surfaceArea;
	float		m_damping;
	float		m_rotdamping;
	float		m_inertia;
	float		m_drag;
	float		m_rollingDrag;

	// these tune the model building process, they don't go in the file
	float		m_massBias;

	CPhysCollide	*m_pCollisionData;
	CPhysCollisionModel	*m_pNext;
};

enum jointlimit_t
{
	JOINT_FREE = 0,
	JOINT_FIXED = 1,
	JOINT_LIMIT = 2,
};


//-----------------------------------------------------------------------------
// Purpose: element of a list of constraints for a jointed model
//-----------------------------------------------------------------------------
class CJointConstraint
{
public:
	CJointConstraint( void )
	{
		m_pJointName = NULL;
	}

	CJointConstraint( const char *pName, int axis, jointlimit_t type, float min, float max, float friction )
		: m_axis(axis), m_jointType(type), m_limitMin(min), m_limitMax(max), m_friction(friction)
	{
		m_pJointName = pName;
	}

	const char		*m_pJointName;
	int				m_axis;
	jointlimit_t	m_jointType;
	float			m_limitMin;
	float			m_limitMax;
	float			m_friction;

	CJointConstraint *m_pNext;
};

struct mergelist_t
{
	char		*pParent;
	char		*pChild;
};

//-----------------------------------------------------------------------------
// Purpose: Search a source for a bone with a specified name
// Input  : *pSource - 
//			*pName - 
// Output : int boneIndex, -1 if none
//-----------------------------------------------------------------------------
int FindLocalBoneNamed( const s_source_t *pSource, const char *pName )
{
	if ( pName )
	{
		int i;
		for ( i = 0; i < pSource->numbones; i++ )
		{
			if ( !strcmpi( pName, pSource->localBone[i].name ) )
				return i;
		}

		pName = RenameBone( pName );

		for ( i = 0; i < pSource->numbones; i++ )
		{
			if ( !strcmpi( pName, pSource->localBone[i].name ) )
				return i;
		}
	}

	return -1;
}


// Returns the index to pName in g_bonetable
int FindBoneInTable( const char *pName )
{
	return findGlobalBone( pName );
}


//-----------------------------------------------------------------------------
// Purpose: Contains a complete physical joint system with constraint relationships
//-----------------------------------------------------------------------------
// This class is really just a namespace for a set of globals...
class CJointedModel
{
public:
	s_source_t				*m_pModel;
	int						m_collisionCount;
	CPhysCollisionModel		*m_pCollisionList;
	float					m_totalMass;
	int						m_bonemap[MAXSTUDIOSRCBONES];
	CJointConstraint		*m_pConstraintList;
	int						m_constraintCount;
	int						m_totalVerts;
	char					m_rootName[128];
	bool					m_allowConcave;

	float					m_defaultDamping;
	float					m_defaultRotdamping;
	float					m_defaultInertia;
	float					m_defaultDrag;
	float					m_defaultRollingDrag;
	CUtlVector<char>		m_textCommands;
	CUtlVector<mergelist_t> m_mergeList;

	CJointedModel( void );

	void SetSource( s_source_t *pmodel );

	void InitBoneMap( void );
	void SkipBone( int boneIndex );
	void MergeBones( int parent, int child );
	void AddMergeCommand( char const *pParent, char const *pChild );
	bool ShouldProcessBone( int boneIndex );
	int BoneIndex( const char *pName );
	int	RemapBone( int boneIndex ) const;
	void AppendCollisionModel( CPhysCollisionModel *pCollide );
	void UnlinkCollisionModel( CPhysCollisionModel *pCollide );
	CPhysCollisionModel *GetCollisionModel( const char *pName );
	void AddConstraint( const char *pJointName, int axis, jointlimit_t jointType, float limitMin, float limitMax, float friction );
	int CollisionIndex( const char *pName );
	void SortCollisionList( void );

	void AllowConcave( void ) { m_allowConcave = true; }
	void Simplify();
	void DefaultDamping( float damping );
	void DefaultRotdamping( float rotdamping );
	void DefaultInertia( float inertia );
	void DefaultDrag( float drag );
	void DefaultRollingDrag( float rollingDrag );
	void SetTotalMass( float mass );
	void SetAutoMass( void );
	void SetCollisionModelDefaults( CPhysCollisionModel *pModel );

	void JointDamping( const char *pJointName, float damping );
	void JointRotdamping( const char *pJointName, float rotdamping );
	void JointInertia( const char *pJointName, float inertia );
	void JointMassBias( const char *pJointName, float massBias );

	void AddText( const char *pText )
	{
		int len = strlen(pText);
		int count = m_textCommands.Size();
		m_textCommands.AddMultipleToTail( len );
		memcpy( m_textCommands.Base() + count, pText, len );
	}
	void ComputeMass( void );

};


CJointedModel g_JointedModel;
bool g_bJointed = false;

CJointedModel::CJointedModel( void )
{
	m_pModel = NULL;

	m_collisionCount = 0;
	m_pCollisionList = NULL;
	m_totalMass = 1.0;

	memset( m_bonemap, 0, sizeof(m_bonemap) );
	m_pConstraintList = NULL;
	m_constraintCount = 0;
	
	m_totalVerts = 0;

	// UNDONE: Move these defaults elsewhere?  They are all overrideable by the QC/script
	m_defaultDamping = 0;
	m_defaultRotdamping = 0;
	m_defaultInertia = 1.0;
	m_defaultDrag = -1;
	m_defaultRollingDrag = -1;
	m_allowConcave = false;
}



void CJointedModel::SetSource( s_source_t *pmodel )
{
	m_pModel = pmodel;
	InitBoneMap();
	m_totalVerts = pmodel->numvertices;
}

void CJointedModel::InitBoneMap( void )
{
	for ( int i = 0; i < m_pModel->numbones; i++ )
	{
		m_bonemap[i] = i;
	}
}

void CJointedModel::SkipBone( int boneIndex )
{
	if ( boneIndex >= 0 )
		m_bonemap[boneIndex] = -1;
}

void CJointedModel::AddMergeCommand( char const *pParent, char const *pChild )
{
	int i = m_mergeList.AddToTail();
	m_mergeList[i].pParent = strdup(pParent);
	m_mergeList[i].pChild = strdup(pChild);
}

void CJointedModel::MergeBones( int parent, int child )
{
	if ( parent < 0 || child < 0 )
		return;

	int map = parent;
	int safety = 0;
	while ( m_bonemap[map] != map )
	{
		map = m_bonemap[map];
		safety++;
		// infinite loop?
		if ( safety > m_pModel->numbones )
			break;

		if ( map < 0 )
			break;
	}

	m_bonemap[child] = map;
}


bool CJointedModel::ShouldProcessBone( int boneIndex )
{
	if ( boneIndex >= 0 )
	{
		if ( m_bonemap[boneIndex] == boneIndex )
			return true;
	}
	return false;
}

int CJointedModel::BoneIndex( const char *pName )
{
	pName = RenameBone( pName );
	for ( int boneIndex = 0; boneIndex < m_pModel->numbones; boneIndex++ )
	{
		if ( !strcmpi( m_pModel->localBone[boneIndex].name, pName ) )
			return boneIndex;
	}

	return -1;
}

int	CJointedModel::RemapBone( int boneIndex ) const
{
	if ( boneIndex >= 0 )
		return m_bonemap[boneIndex];
	return boneIndex;
}

void CJointedModel::AppendCollisionModel( CPhysCollisionModel *pCollide )
{
	pCollide->m_pNext = m_pCollisionList;
	m_pCollisionList = pCollide;
	m_collisionCount++;
}


void CJointedModel::UnlinkCollisionModel( CPhysCollisionModel *pCollide )
{
	CPhysCollisionModel **pList = &m_pCollisionList;

	if ( !pCollide )
		return;

	while ( *pList )
	{
		CPhysCollisionModel *pNode = *pList;
		if ( pNode == pCollide )
		{
			*pList = pCollide->m_pNext;
			m_collisionCount--;
			pCollide->m_pNext = NULL;
			return;
		}
		pList = &pNode->m_pNext;
	}
}

int CJointedModel::CollisionIndex( const char *pName )
{
	CPhysCollisionModel *pList = m_pCollisionList;
	int index = 0;
	while ( pList )
	{
		if ( !strcmpi( pName, pList->m_name ) )
			return index;
		
		pList = pList->m_pNext;
		index++;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Sort the list so that parents come before their children
//-----------------------------------------------------------------------------
void CJointedModel::SortCollisionList( void )
{
	if ( !m_collisionCount )
		return;

	CPhysCollisionModel **pArray;
	pArray = new CPhysCollisionModel *[m_collisionCount];
	CPhysCollisionModel *pList = m_pCollisionList;
	
	// make an array to make sorting easier
	int i = 0;

	while ( pList )
	{
		pArray[i++] = pList;
		pList = pList->m_pNext;
	}

	// really stupid bubble sort!
	// this is really inefficient but it was easy to code and there are never
	// more than 20 elements or so.
	bool swapped = true;

	while ( swapped )
	{
		swapped = false;
		// loop over all solids and swap any parent/child pairs that are out of order
		for ( i = 0; i < m_collisionCount; i++ )
		{
			CPhysCollisionModel *pPhys = pArray[i];
			if ( !pPhys->m_parent )
				continue;

			// find the parent
			int j;
			for ( j = 0; j < m_collisionCount; j++ )
			{
				if ( j == i )
					continue;

				if ( !strcmpi( pPhys->m_parent, pArray[j]->m_name ) )
					break;
			}

			// if the child came before the parent, then swap the parent and child positions
			if ( j > i && j < m_collisionCount )
			{
				swapped = true;
				pArray[i] = pArray[j];
				pArray[j] = pPhys;
			}
		}
	}

	// link up the sorted list
	for ( i = 0; i < m_collisionCount-1; i++ )
	{
		pArray[i]->m_pNext = pArray[i+1];
	}
	// terminate
	pArray[i]->m_pNext = NULL;
	// point the list to first joint
	m_pCollisionList = pArray[0];

	// delete the working array
	delete[] pArray;
}

// called before processing, after the model has been simplified.
// Update internal state due to simplification
void CJointedModel::Simplify()
{
	for ( int i = 0; i < m_pModel->numbones; i++ )
	{
		if ( m_pModel->boneLocalToGlobal[i] < 0 )
		{
			SkipBone(i);
		}
	}

	extern int g_rootIndex;
	const char *pAnimationRootBone = g_bonetable[g_rootIndex].name;

	// merge this root bone with the root of animation
	MergeBones( FindLocalBoneNamed( m_pModel, pAnimationRootBone ), FindLocalBoneNamed( m_pModel, m_rootName ) );

}


CPhysCollisionModel *CJointedModel::GetCollisionModel( const char *pName )
{
	CPhysCollisionModel *pList = m_pCollisionList;
	while ( pList )
	{
		if ( !strcmpi( pName, pList->m_name ) )
			return pList;
		
		pList = pList->m_pNext;
	}

	return NULL;
}

void CJointedModel::AddConstraint( const char *pJointName, int axis, jointlimit_t jointType, float limitMin, float limitMax, float friction )
{
	CJointConstraint *pConstraint = new CJointConstraint( pJointName, axis, jointType, limitMin, limitMax, friction );

	// link it in
	pConstraint->m_pNext = m_pConstraintList;
	m_pConstraintList = pConstraint;
	m_constraintCount++;
}

void CJointedModel::DefaultDamping( float damping )
{
	m_defaultDamping = damping;
}

void CJointedModel::DefaultRotdamping( float rotdamping )
{
	m_defaultRotdamping = rotdamping;
}

void CJointedModel::DefaultInertia( float inertia )
{
	m_defaultInertia = inertia;
}

void CJointedModel::SetTotalMass( float mass )
{
	m_totalMass = mass;
}

void CJointedModel::SetAutoMass( void )
{
	m_totalMass = -1;
}

void CJointedModel::SetCollisionModelDefaults( CPhysCollisionModel *pModel )
{
	pModel->m_damping = m_defaultDamping;
	pModel->m_inertia = m_defaultInertia;
	pModel->m_rotdamping = m_defaultRotdamping;
	pModel->m_massBias = 1.0;
	
	// not written unless modified
	pModel->m_drag = m_defaultDrag;
	pModel->m_rollingDrag = m_defaultRollingDrag;
}



void CJointedModel::ComputeMass( void )
{
	// already set
	if ( m_totalMass >= 0 )
		return;

	CPhysCollisionModel *pList = m_pCollisionList;
	m_totalMass = 0;

	while ( pList )
	{
		char* pSurfaceProps = GetSurfaceProp( pList->m_name );
		int index = physprops->GetSurfaceIndex( pSurfaceProps );
		float density, thickness;
		physprops->GetPhysicsProperties( index, &density, &thickness, NULL, NULL );

		if ( thickness > 0 )
		{
			m_totalMass += pList->m_surfaceArea * thickness * CUBIC_METERS_PER_CUBIC_INCH * density;
		}
		else
		{
			// density is in kg/m^3, volume is in in^3
			m_totalMass += pList->m_volume * CUBIC_METERS_PER_CUBIC_INCH * density;
		}
		pList = pList->m_pNext;
	}

	if( !g_quiet )
	{
		printf("Computed Mass: %.2f kg\n", m_totalMass );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates a collision object using the defaults in joints
// Input  : &joints - joint system to create the model in
//			*pJointName - name to give this model
// Output : static CPhysCollisionModel
//-----------------------------------------------------------------------------
static CPhysCollisionModel *InitCollisionModel( CJointedModel &joints, const char *pJointName )
{
	CPhysCollisionModel *pModel = joints.GetCollisionModel( pJointName );
	if ( !pModel )
	{
		int boneIndex = joints.BoneIndex( pJointName );
		if ( boneIndex < 0 )
			return NULL;

		pModel = new CPhysCollisionModel;
		// this name is the same as pJointName, but guaranteed to be non-volatile (we'd have to copy pJointName)
		pModel->m_name = joints.m_pModel->localBone[boneIndex].name;	
		pModel->m_parent = joints.m_pModel->localBone[joints.m_pModel->localBone[boneIndex].parent].name;

		joints.SetCollisionModelDefaults( pModel );
		joints.AppendCollisionModel( pModel );
	}

	return pModel;
}

void CJointedModel::JointDamping( const char *pJointName, float damping )
{
	CPhysCollisionModel *pModel = InitCollisionModel( *this, pJointName );
	if ( pModel )
	{
		pModel->m_damping = damping;
	}
}

void CJointedModel::JointRotdamping( const char *pJointName, float rotdamping )
{
	CPhysCollisionModel *pModel = InitCollisionModel( *this, pJointName );
	if ( pModel )
	{
		pModel->m_rotdamping = rotdamping;
	}
}

void CJointedModel::JointMassBias( const char *pJointName, float massBias )
{
	CPhysCollisionModel *pModel = InitCollisionModel( *this, pJointName );
	if ( pModel )
	{
		pModel->m_massBias = massBias;
	}
}

void CJointedModel::JointInertia( const char *pJointName, float inertia )
{
	CPhysCollisionModel *pModel = InitCollisionModel( *this, pJointName );
	if ( pModel )
	{
		pModel->m_inertia = inertia;
	}
}


void CJointedModel::DefaultDrag( float drag )
{
	m_defaultDrag = drag;
}

void CJointedModel::DefaultRollingDrag( float rollingDrag )
{
	m_defaultRollingDrag = rollingDrag;
}

// ----------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Transforms the source's verts into "world" space
// Input  : *psource - 
//			*worldVerts - 
//-----------------------------------------------------------------------------
void ConvertToWorldSpace( s_source_t *psource, Vector *worldVerts )
{
	int i, n;

	matrix3x4_t boneToWorld[MAXSTUDIOSRCBONES];	// bone transformation matrix

	CalcBoneTransforms( g_panimation[0], 0, boneToWorld );

	for (i = 0; i < psource->numvertices; i++)
	{
		Vector tmp,tmp2;
		worldVerts[i] = Vector( 0, 0, 0 );
		
		for (n = 0; n < psource->localBoneweight[i].numbones; n++)
		{
			// convert to Half-Life world space
			// convert vertex into original models' bone local space
			int localBone = psource->localBoneweight[i].bone[n];
			int globalBone = psource->boneLocalToGlobal[localBone];
			assert( localBone >= 0 );
			assert( globalBone >= 0 );

			matrix3x4_t boneToPose;
			ConcatTransforms( psource->boneToPose[localBone], g_bonetable[globalBone].srcRealign, boneToPose );
			VectorITransform( psource->vertex[i], boneToPose, tmp2 );

			// now transform to that bone's world-space position in this animation
			VectorTransform(tmp2, boneToWorld[globalBone], tmp );
			VectorMA( worldVerts[i], psource->localBoneweight[i].weight[n], tmp, worldVerts[i] );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Transforms the set of verts into the space of a particular bone
// Input  : *psource - 
//			boneIndex - 
//			*boneVerts - 
//-----------------------------------------------------------------------------
void ConvertToBoneSpace( s_source_t *psource, int boneIndex, Vector *boneVerts )
{
	int i;

	int remapIndex = psource->boneLocalToGlobal[boneIndex];
	matrix3x4_t boneToPose;
	if ( remapIndex < 0 )
	{
		Msg("Error! physics for unused bone %s\n", psource->localBone[boneIndex].name );
		MatrixCopy( psource->boneToPose[boneIndex], boneToPose );
	}
	else
	{
		ConcatTransforms( psource->boneToPose[boneIndex], g_bonetable[remapIndex].srcRealign, boneToPose );
	}

	for (i = 0; i < psource->numvertices; i++)
	{
		VectorITransform(psource->vertex[i], boneToPose, boneVerts[i] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Test this face to see if any of its verts are assigned to a particular bone
// Input  : &joints - 
//			*pmodel - 
//			*face - 
//			boneIndex - 
// Output : Returns true if this face has a vert assigned to boneIndex
//-----------------------------------------------------------------------------
bool FaceHasVertOnBone( const CJointedModel &joints, s_source_t *pmodel, s_face_t *face, int boneIndex )
{
	int j;
	s_boneweight_t *pweight;

	pweight = pmodel->globalBoneweight + face->a;
	for ( j = 0; j < pweight->numbones; j++ )
	{
		// Discover the local bone index for this bone
		int localBone = pmodel->boneGlobalToLocal[ pweight->bone[j] ];
		
		// assigned to boneIndex?
		if ( joints.RemapBone( localBone ) == boneIndex )
		{
			return true;
		}
	}

	pweight = pmodel->globalBoneweight + face->b;
	for ( j = 0; j < pweight->numbones; j++ )
	{
		// Discover the local bone index for this bone
		int localBone = pmodel->boneGlobalToLocal[ pweight->bone[j] ];

		// assigned to boneIndex?
		if ( joints.RemapBone( localBone ) == boneIndex )
		{
			return true;
		}
	}

	pweight = pmodel->globalBoneweight + face->c;
	for ( j = 0; j < pweight->numbones; j++ )
	{
		// Discover the local bone index for this bone
		int localBone = pmodel->boneGlobalToLocal[ pweight->bone[j] ];

		// assigned to boneIndex?
		if ( joints.RemapBone( localBone ) == boneIndex )
		{
			return true;
		}
	}

	return false;

}

//-----------------------------------------------------------------------------
// Purpose: Fixup the pointers in this face to reference the mesh globally (source relative)
//			(faces are mesh relative, each source has several meshes)
// Input  : *pout - 
//			*pmesh - 
//			*pin - 
//-----------------------------------------------------------------------------
void GlobalFace( s_face_t *pout, s_mesh_t *pmesh, s_face_t *pin )
{
	pout->a = pmesh->vertexoffset + pin->a;
	pout->b = pmesh->vertexoffset + pin->b;
	pout->c = pmesh->vertexoffset + pin->c;
}


//-----------------------------------------------------------------------------
// Purpose: Copy all verts assigned to this bone.
//			NOTE: Leaves gaps in the model around joints
// Input  : **verts - 
//			*worldVerts - 
//			&joints - 
//			boneIndex - 
// Output : int vertCount
//-----------------------------------------------------------------------------
int CopyVertsByBone( Vector **verts, Vector *worldVerts, const CJointedModel &joints, int boneIndex )
{
	int vertCount = 0;
	s_source_t *pmodel = joints.m_pModel;

	// loop through each vert to find those assigned to this bone
	for ( int i = 0; i < pmodel->numvertices; i++ )
	{
		s_boneweight_t *pweight = pmodel->globalBoneweight + i;

		// look at each assignment for this vert
		for ( int j = 0; j < pweight->numbones; j++ )
		{
			// Discover the local bone index for this bone
			int localBone = pmodel->boneGlobalToLocal[ pweight->bone[j] ];

			// assigned to boneIndex?
			if ( joints.RemapBone( localBone ) == boneIndex )
			{
				// add this vert to model
				verts[vertCount++] = &worldVerts[i];
			}
		}
	}

	return vertCount;
}


//-----------------------------------------------------------------------------
// Purpose: Copy all verts that are referenced by a face which has a vert assigned
//			to this bone.
//			NOTE: convex hulls of each bone will overlap at the joints
// Input  : **verts - 
//			*worldVerts - 
//			&joints - 
//			boneIndex - 
// Output : int
//-----------------------------------------------------------------------------
int CopyFaceVertsByBone( Vector **verts, Vector *worldVerts, const CJointedModel &joints, int boneIndex )
{
	int vertCount = 0;
	s_source_t *pmodel = joints.m_pModel;

	int *vertChecked = new int[pmodel->numvertices];
	for ( int b = 0; b < pmodel->numvertices; b++ )
	{
		vertChecked[b] = 0;
	}

	for ( int i = 0; i < pmodel->nummeshes; i++ )
	{
		s_mesh_t *pmesh = pmodel->mesh + pmodel->meshindex[i];
		for ( int j = 0; j < pmesh->numfaces; j++ )
		{
			s_face_t *face = pmodel->face + pmesh->faceoffset + j;
			s_face_t globalFace;
			GlobalFace( &globalFace, pmesh, face );
			if ( FaceHasVertOnBone( joints, pmodel, &globalFace, boneIndex ) )
			{
				if ( !vertChecked[globalFace.a] )
				{
					// add this vert to model
					verts[vertCount++] = &worldVerts[globalFace.a];
				}
				if ( !vertChecked[globalFace.b] )
				{
					// add this vert to model
					verts[vertCount++] = &worldVerts[globalFace.b];
				}
				if ( !vertChecked[globalFace.c] )
				{
					// add this vert to model
					verts[vertCount++] = &worldVerts[globalFace.c];
				}
				// mark these verts so you only add them once
				vertChecked[globalFace.a] = 1;
				vertChecked[globalFace.b] = 1;
				vertChecked[globalFace.c] = 1;
			}
		}
	}

	delete[] vertChecked;
	return vertCount;
}



//-----------------------------------------------------------------------------
// Purpose: Find all verts that differ only by texture coordinates - this allows
//			us to ignore texture coordinates on collision models
// Input  : *weldTable - output table
//			*pmodel - input model
//-----------------------------------------------------------------------------
void BuildVertWeldTable( int *weldTable, s_source_t *pmodel )
{
	for ( int i = 0; i < pmodel->numvertices; i++ )
	{
		bool found = false;
		for ( int j = 0; j < i; j++ )
		{
			if ( VectorCompare( pmodel->vertex[j], pmodel->vertex[i] ) &&
				 DotProduct( pmodel->normal[j], pmodel->normal[i] ) > normal_blend )
			{
				found = true;
				weldTable[i] = j;
				break;
			}
		}

		if ( !found )
		{
			weldTable[i] = i;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: marks all verts with a unique ID.  Each set of connected verts has
//			the same ID.  IDs are the index of the lowest numbered face on the 
//			mesh
// Input  : *vertID - array that holds IDs
//			*pmodel - model to process
//-----------------------------------------------------------------------------
void MarkConnectedMeshes( int *vertID, s_source_t *pmodel, int *vertMap )
{
	int i;

	// mark all verts as max faceid + 1
	for ( i = 0; i < pmodel->numvertices; i++ )
	{
		// If these verts have been welded to a lower-index vert, mark them
		// as already processed to avoid making additional convex objects out of them.
		if ( vertMap[i] != i )
		{
			vertID[i] = -1;
		}
		else
		{
			vertID[i] = pmodel->numfaces+1;
		}
	}

	int marked = 0;
	int faceid = 0;
	// iterate the face list, minimizing the vertID at each vert
	// until we have an iteration where no vertIDs are changed
	do 
	{
		marked = 0;
		faceid = 0;

		for ( i = 0; i < pmodel->nummeshes; i++ )
		{
			s_mesh_t *pmesh = pmodel->mesh + pmodel->meshindex[i];
			for ( int j = 0; j < pmesh->numfaces; j++ )
			{
				s_face_t *face = pmodel->face + pmesh->faceoffset + j;
				s_face_t globalFace;
				GlobalFace( &globalFace, pmesh, face );

				// account for welding
				globalFace.a = vertMap[globalFace.a];
				globalFace.b = vertMap[globalFace.b];
				globalFace.c = vertMap[globalFace.c];

				// find min(faceid, vertID[a], vertID[b], vertID[c]);
				int newid = min(faceid, vertID[globalFace.a]);
				newid = min( newid, vertID[globalFace.b]);
				newid = min( newid, vertID[globalFace.c]);
				
				// mark all verts with the minimum, count the number we had to mark
				if ( vertID[globalFace.a] != newid )
				{
					vertID[globalFace.a] = newid;
					marked++;
				}
				if ( vertID[globalFace.b] != newid )
				{
					vertID[globalFace.b] = newid;
					marked++;
				}
				if ( vertID[globalFace.c] != newid )
				{
					vertID[globalFace.c] = newid;
					marked++;
				}
				faceid++;
			}
		}
	} while ( marked != 0 );
}



//-----------------------------------------------------------------------------
// Purpose: Finds a CPhysCollisionModel in a linked list of models.
// Input  : *pHead - 
//			*pName - 
// Output : CPhysCollisionModel
//-----------------------------------------------------------------------------
CPhysCollisionModel *FindObjectInList( CPhysCollisionModel *pHead, const char *pName )
{
	while ( pHead )
	{
		if ( !strcmpi( pName, pHead->m_name ) )
			break;
		pHead = pHead->m_pNext;
	}

	return pHead;
}


//-----------------------------------------------------------------------------
// Purpose: Fix all bones to reference the remapped/collapsed bone structure
// Input  : *pSource - 
//			*pList - 
//-----------------------------------------------------------------------------
void FixBoneList( int *boneMap, const s_source_t *pSource, CPhysCollisionModel *pList )
{
	if ( !g_bJointed )
		return;

	CPhysCollisionModel *pmodel = pList;
	while ( pmodel )
	{
		int nodeIndex = FindLocalBoneNamed( pSource, pmodel->m_name );
		if ( nodeIndex < 0 )
		{
			printf("Physics for unknown bone %s\n", pmodel->m_name );
		}
		else
		{
			int count = 0;
			// remove simplified bones
			while ( pSource->boneLocalToGlobal[nodeIndex] < 0 )
			{
				if ( count++ > MAXSTUDIOSRCBONES )
					break;

				// simplified out, move up to the parent
				nodeIndex = pSource->localBone[nodeIndex].parent;
			}

			if ( nodeIndex >= 0 )
			{
				pmodel->m_name = pSource->localBone[nodeIndex].name;
				pmodel->m_parent = NULL;
				int parentIndex = pSource->localBone[nodeIndex].parent;
				if ( parentIndex >= 0 )
				{
					parentIndex = boneMap[parentIndex];
					pmodel->m_parent = pSource->localBone[parentIndex].name;
				}
			}
			else
			{
				printf("Physics for unknown bone %s\n", pmodel->m_name );
			}
		}

		pmodel = pmodel->m_pNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fixup all references to parents by walking up on models whose parents
//			have no collision geometry.  Bones without geometry cannot be physically
//			simulated, so they must be removed.
//			NOTE: This is broken.  It won't work for tree structures with an empty parent
//			(i.e. 2 children attached to a parent bone that has no physics geometry - thus empty)
//			It will not convert that parent into a constraint between 2 children
// Input  : *pList - 
//			*pSource - 
//			*pParentName - 
// Output : const char
//-----------------------------------------------------------------------------
const char *FixParent( CPhysCollisionModel *pList, s_source_t *pSource, const char *pParentName )
{
	while ( pParentName )
	{
		if ( FindObjectInList( pList, pParentName ) )
		{
			return pParentName;
		}
		int nodeIndex = FindLocalBoneNamed( pSource, pParentName );
		if ( nodeIndex < 0 )
			return NULL;
		int parentIndex = pSource->localBone[nodeIndex].parent;
		if ( parentIndex < 0 )
		{
			break;
		}

		pParentName = pSource->localBone[parentIndex].name;
	}

	return NULL;
}


CPhysCollisionModel *CreatePhysModel( CPhysCollisionModel *pBase, CPhysConvex **pElements, int elementCount )
{
	if ( !pBase )
	{
		// UNDONE: Memory leak
		pBase = new CPhysCollisionModel;
	}

	// NOTE: Must do this before building collide
	pBase->m_volume = 0;
	pBase->m_surfaceArea = 0;
	for ( int i = 0; i < elementCount; i++ )
	{
		pBase->m_volume += physcollision->ConvexVolume( pElements[i] );
		pBase->m_surfaceArea += physcollision->ConvexSurfaceArea( pElements[i] );
	}

	// THIS DESTROYS pConvex!!
	pBase->m_pCollisionData = physcollision->ConvertConvexToCollide( pElements, elementCount );

	return pBase;
}


//-----------------------------------------------------------------------------
// Purpose: Build a jointed collision model with constraints
// Input  : &joints - 
// Output : int
//-----------------------------------------------------------------------------
int ProcessJointedModel( CJointedModel &joints )
{
	Vector *boneVerts = new Vector[joints.m_pModel->numvertices];
	Vector **verts = new Vector *[joints.m_pModel->numvertices];
	int vertCount;

	if( !g_quiet )
	{
		printf("Processing jointed collision model\n" );
	}
	// loop through each bone and form a convex element
	for ( int boneIndex = 0; boneIndex < joints.m_pModel->numbones; boneIndex++ )
	{
		if ( !joints.ShouldProcessBone( boneIndex ) )
			continue;

		CPhysCollisionModel *pPhys = InitCollisionModel( joints, joints.m_pModel->localBone[boneIndex].name );
		ConvertToBoneSpace( joints.m_pModel, boneIndex, boneVerts );
		vertCount = CopyFaceVertsByBone( verts, boneVerts, joints, boneIndex );
		//vertCount = CopyVertsByBone( verts, boneVerts, joints, boneIndex );
//		printf("Bone %s has %d verts\n", joints.m_pModel->localBone[boneIndex].name, vertCount );
		// if verts were attached to this bone, build a convex element from those verts
		if ( vertCount )
		{
			CPhysConvex *pConvex = physcollision->ConvexFromVerts( verts, vertCount );
			
			// If this was a valid volume, add it to the list
			if ( pConvex )
			{
				// THIS DESTROYS pConvex!!
				pPhys = CreatePhysModel( pPhys, &pConvex, 1 );
				pConvex = NULL;

				pPhys->m_mass = 1.0;
				pPhys->m_name = joints.m_pModel->localBone[boneIndex].name;
				pPhys->m_parent = joints.m_pModel->localBone[joints.m_pModel->localBone[boneIndex].parent].name;

				if( !g_quiet )
				{
					printf("%-24s (%3d verts) volume: %4.2f\n", pPhys->m_name, vertCount, pPhys->m_volume );
				}
				joints.UnlinkCollisionModel( pPhys );
				joints.AppendCollisionModel( pPhys );
				
				// NOTE: Clear this so it isn't deleted!!!!
				pPhys = NULL;
			}
		}

		// If we still have a collision model, it wasn't put in the tree!!
		if ( pPhys )
		{
			// remove it from the list
			joints.UnlinkCollisionModel( pPhys );
			// delete it
			delete pPhys;
		}
	}

	// free index buffer
	delete[] boneVerts;
	delete[] verts;

	return 1;
}

#if 0
void DumpToGLView( char const *pName, s_source_t *pmodel, Vector *worldVerts, int *used )
{
	int i;

	for ( i = 0; i < pmodel->numvertices; i++ )
		used[i] = -1;

	FILE *fp = fopen( pName, "w" );
	
	// dump the model to a glview file
	for ( i = 0; i < pmodel->nummeshes; i++ )
	{
		s_mesh_t *pmesh = pmodel->mesh + pmodel->meshindex[i];
		for ( int j = 0; j < pmesh->numfaces; j++ )
		{
			s_face_t *face = pmodel->face + pmesh->faceoffset + j;
			s_face_t globalFace;
			GlobalFace( &globalFace, pmesh, face );

			fprintf( fp, "3\n" );
			fprintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", worldVerts[globalFace.b].x, worldVerts[globalFace.b].y, worldVerts[globalFace.b].z );
			fprintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", worldVerts[globalFace.a].x, worldVerts[globalFace.a].y, worldVerts[globalFace.a].z );
			fprintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", worldVerts[globalFace.c].x, worldVerts[globalFace.c].y, worldVerts[globalFace.c].z );
			used[globalFace.a] = 0;
			used[globalFace.b] = 0;
			used[globalFace.c] = 0;
		}
	}

	// dump a triangle expanded around each vert to the file (to show degenerate tris' verts).
	for ( i = 0; i < pmodel->numvertices; i++ )
	{
		if ( used[i] < 0 )
			continue;

		fprintf( fp, "3\n" );
		Vector vert;
		vert = worldVerts[i] + Vector(0,0,5);
		fprintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", vert.x, vert.y, vert.z );
		vert = worldVerts[i] + Vector(5,0,-5);
		fprintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", vert.x, vert.y, vert.z );
		vert = worldVerts[i] + Vector(-5,0,-5);
		fprintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", vert.x, vert.y, vert.z );

	}

	fclose( fp );
}
#endif

bool IsApproximatelyPlanar( Vector **verts, int vertCount, float epsilon )
{
	if ( vertCount < 4 )
		return true;

	// If we're using an un-welded model, then this may generate a degenerate normal
	// loop to search for an actual plane
	int v0 = 1, v1 = 2;
	Vector normal;
	while ( v0 < vertCount && v1 < vertCount )
	{
		Vector edge0 = *verts[v0] - *verts[0];
		Vector edge1 = *verts[v1] - *verts[0];

		normal = CrossProduct( edge0, edge1 );
		float len = VectorNormalize( normal );
		if ( len > 0.001 )
			break;
		if ( edge0.Length() < 0.001 )
		{
			// verts[0] and v0 are coincident, try new verts
			v0++;
			v1++;
		}
		else
		{
			// v0 seems fine, try a new v1 -- it's probably coincident with v0
			v1++;
		}
	}

	// form the plane and project all of the verts into it
	float dist = DotProduct( normal, *verts[0] );

	for ( int i = 0; i < vertCount; i++ )
	{
		float d = DotProduct( *verts[i], normal ) - dist;
		// at least one vert out of the plane, we've got something 3 dimensional
		if ( fabsf(d) > epsilon )
			return false;
	}
	return true;
}

int ProcessSingleBody( CJointedModel &joints )
{
	// HACKHACK: This "bad" model will work correctly, but it will have really bad performance.
	// Force a single convex so that the modeller will have to fix the model to get the correct results
	// Or you can force this to compile by running with -fullcollide or removing this code.
	// In general, none of our models should need more than 20 convex subparts.  If they do, raise this limit.
	extern bool g_badCollide;

	s_source_t *pmodel = joints.m_pModel;
	// THIS CODE IS ONLY EXECUTED ON PROPS - i.e. NON-JOINTED MODELS
	Vector *worldVerts = new Vector[pmodel->numvertices];
	ConvertToWorldSpace( pmodel, worldVerts );
	Vector **verts = new Vector *[pmodel->numvertices];
	int *vertID = new int[pmodel->numvertices];
	CUtlVector<CPhysConvex *> elements;

	int vertCount;
	int i;

	memset( vertID, 0, sizeof(int)*pmodel->numvertices);

	//	DumpToGLView( "gl.txt", pmodel, worldVerts, vertID );

	// extract sets of disjoint meshes from the model
	if ( joints.m_allowConcave )
	{
		// build a table to remap verts that only differ by texture coordinates
		int *vertMap = new int[pmodel->numvertices];
		BuildVertWeldTable( vertMap, pmodel );
		// Find all of the sub groups in the object
		MarkConnectedMeshes( vertID, pmodel, vertMap );
		delete[] vertMap;
	}

	// loop through each bone and form a convex element
	for ( i = 0; i < pmodel->numvertices; i++ )
	{
		// already processed this group
		if ( vertID[i] < 0 || vertID[i] > pmodel->numfaces )
			continue;

		vertCount = 0;

		int id = vertID[i];

		for ( int j = i; j < pmodel->numvertices; j++ )
		{
			if ( vertID[j] == id )
			{
				verts[vertCount++] = &worldVerts[j];
				// don't reuse this vert
				vertID[j] = -1;
			}
		}

		if ( vertCount > 2 )
		{
			// HACKHACK: A heuristic to detect models without smoothing groups set
			// UNDONE: Do a BSP to decompose arbitrary models to convex?
			if ( IsApproximatelyPlanar( verts, vertCount, 0.1 ) )
			{
				printf("WARNING: Bad collision model, check your smoothing groups!!!\n\07\nTruncating model!!!!\n" );
				printf( "%s has bad smoothing groups\n", pmodel->filename );
				elements.Purge();
				// just build one part
				for ( int i = 0; i < pmodel->numvertices; i++ )
				{
					verts[i] = &worldVerts[i];
				}
				CPhysConvex *pConvex = physcollision->ConvexFromVerts( verts, pmodel->numvertices );
				if ( pConvex )
				{
					elements.AddToTail( pConvex );
				}
				break;
			}

			CPhysConvex *pConvex = physcollision->ConvexFromVerts( verts, vertCount );
			
			// If this was a valid volume, add it to the list
			if ( pConvex )
			{
				elements.AddToTail( pConvex );
			}
		}

	}

	// build the collision model of the union of the convex parts
	if ( elements.Size() )
	{
		if ( elements.Size() > 20 )
		{
			if ( !g_badCollide )
			{
				printf("WARNING: COSTLY COLLISION MODEL!!!! (%d parts)\n\07\nTruncating model!!!!\n", elements.Size() );
				elements.Purge();
				// just build one part
				for ( int i = 0; i < pmodel->numvertices; i++ )
				{
					verts[i] = &worldVerts[i];
				}
				CPhysConvex *pConvex = physcollision->ConvexFromVerts( verts, pmodel->numvertices );
				if ( pConvex )
				{
					elements.AddToTail( pConvex );
				}
			}
			else
			{
				printf("WARNING: COSTLY COLLISION MODEL!!!!\nTruncation DISABLED!!!!\n" );
			}
		}

		if( !g_quiet )
		{
			printf("Model has %d convex sub-parts\n", elements.Size() );
		}
	
		CPhysCollisionModel *pPhys = CreatePhysModel( NULL, elements.Base(), elements.Size() );
		joints.SetCollisionModelDefaults( pPhys );

		// Init mass, write routine will distribute the total mass
		pPhys->m_mass = 1.0;
		char tmp[512];
		ExtractFileBase( pmodel->filename, tmp, sizeof( tmp ) );
		
		// UNDONE: Memory leak
		char *out = new char[strlen(tmp)+1];
		strcpy( out, tmp );
		pPhys->m_name = out;
		pPhys->m_parent = NULL;

		joints.AppendCollisionModel( pPhys );
	}

	// free index buffer
	delete[] worldVerts;
	delete[] verts;
	delete[] vertID;

	return 1;
}


#define MAX_ARGS	16
#define ARG_SIZE	256

//-----------------------------------------------------------------------------
// Purpose: HACKETY HACK - get the args into a buffer.
//			This checks for overflow, but it's not very robust - shouldn't be necessary though
// Input  : pArgs[][ARG_SIZE] - 
//			maxCount - array size of pargs
// Output : int - count actually used
//-----------------------------------------------------------------------------
int ReadArgs( char pArgs[][ARG_SIZE], int maxCount )
{
	int argCount = 0;

	while ( argCount < maxCount && TokenAvailable() )
	{
		GetToken(false);
		strncpy( pArgs[argCount], token, ARG_SIZE );
		argCount++;
	}

	return argCount;
}


//-----------------------------------------------------------------------------
// Purpose: Simple atof wrapper to keep from crashing on bad user input
// Input  : *pString - 
// Output : float
//-----------------------------------------------------------------------------
float Safe_atof( const char *pString )
{
	if ( !pString )
		return 0;

	return atof(pString);
}

//-----------------------------------------------------------------------------
// Purpose: Simple atoi wrapper to avoid crashing on bad user input
// Input  : *pString - 
// Output : int
//-----------------------------------------------------------------------------
int Safe_atoi( const char *pString )
{
	if ( !pString )
		return 0;

	return atoi(pString);
}


//-----------------------------------------------------------------------------
// Purpose: Add a constraint to our joint system
// Input  : &joints - 
//			*pJointName - 
//			*pJointAxis - 
//			*pJointType - 
//			*pLimitMin - 
//			*pLimitMax - 
//-----------------------------------------------------------------------------
void CCmd_JointConstrain( CJointedModel &joints, const char *pJointName, const char *pJointAxis, const char *pJointType, const char *pLimitMin, const char *pLimitMax, const char *pFriction )
{
	float limitMin = Safe_atof(pLimitMin);
	float limitMax = Safe_atof(pLimitMax);
	float friction = Safe_atof(pFriction);
	
	int axis = -1;
	int jointIndex = FindLocalBoneNamed( joints.m_pModel, pJointName );
	if ( jointIndex < 0 )
	{
		printf("Can't find joint %s\n", pJointName );
		return;
	}
	pJointName = joints.m_pModel->localBone[jointIndex].name;

	if ( pJointAxis )
	{
		axis = tolower(pJointAxis[0]) - 'x';
	}
	if ( axis < 0 || axis > 2 )
	{
		printf("Invalid joint constraint for %s\n", pJointName );
		return;
	}

	jointlimit_t jointType = JOINT_FREE;
	if ( !stricmp( pJointType, "free" ) )
	{
		jointType = JOINT_FREE;
	}
	else if ( !stricmp( pJointType, "fixed" ) )
	{
		jointType = JOINT_FIXED;
	}
	else if ( !stricmp( pJointType, "limit" ) )
	{
		jointType = JOINT_LIMIT;
	}
	else
	{
		printf("Unknown joint type %s (must be free, fixed, or limit)\n", pJointType );
		return;
	}
	joints.AddConstraint( pJointName, axis, jointType, limitMin, limitMax, friction );
}


//-----------------------------------------------------------------------------
// Purpose: Remove a joint from the system (don't create physical geometry for it)
// Input  : &joints - 
//			args[][ARG_SIZE] - 
//			argCount - 
//-----------------------------------------------------------------------------
// UNDONE: Automatically skip joints that will have mass that is too low?
void CCmd_JointSkip( CJointedModel &joints, const char *pName )
{
	int boneIndex = FindLocalBoneNamed( joints.m_pModel, pName );
	if ( boneIndex < 0 )
	{
		printf("Can't skip joint %s, not found\n", pName );
	}
	else
	{
//			printf("skipping joint %s\n", pName );
		joints.SkipBone( boneIndex );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the object's mass.  The code will distribute this mass to each
//			part based on the collision model's volume
// Input  : &joints - 
//			*pMass - 
//-----------------------------------------------------------------------------
void CCmd_TotalMass( CJointedModel &joints, const char *pMass )
{
	joints.SetTotalMass( Safe_atof(pMass) );
}


//-----------------------------------------------------------------------------
// Purpose: verts from the bone named pChild are added to the collision model of pParent
// Input  : *pmodel - source model
//			*pParent - destination bone name
//			*pChild - source bone name
//-----------------------------------------------------------------------------
void CCmd_JointMerge( CJointedModel &joints, const char *pParent, const char *pChild )
{
	joints.AddMergeCommand( pParent, pChild );
	joints.MergeBones( FindLocalBoneNamed( joints.m_pModel, pParent ), FindLocalBoneNamed( joints.m_pModel, pChild ) );
}


void CCmd_JointRoot( CJointedModel &joints, const char *pBone )
{
	// save the root bone name
	strcpy( joints.m_rootName, pBone );
}


//-----------------------------------------------------------------------------
// Purpose: Parses all legal commands inside the $collisionjoints {} block
// Input  : &joints - 
//-----------------------------------------------------------------------------
void ParseCollisionCommands( CJointedModel &joints )
{
	char command[512];

	char args[MAX_ARGS][ARG_SIZE];
	int argCount;

	while( GetToken( true ) )
	{
		if ( !strcmp( token, "}" ) )
			return;

		strcpy( command, token );

		if ( !stricmp( command, "$mass" ) )
		{
			argCount = ReadArgs( args, 1 );
			CCmd_TotalMass( joints, args[0] );
		}
		// default properties
		else if ( !stricmp( command, "$automass" ) )
		{
			joints.SetAutoMass();
		}
		else if ( !stricmp( command, "$inertia" ) )
		{
			argCount = ReadArgs( args, 1 );
			joints.DefaultInertia( Safe_atof( args[0] ) );
		}
		else if ( !stricmp( command, "$damping" ) )
		{
			argCount = ReadArgs( args, 1 );
			joints.DefaultDamping( Safe_atof( args[0] ) );
		}
		else if ( !stricmp( command, "$rotdamping" ) )
		{
			argCount = ReadArgs( args, 1 );
			joints.DefaultRotdamping( Safe_atof( args[0] ) );
		}
		else if ( !stricmp( command, "$drag" ) )
		{
			argCount = ReadArgs( args, 1 );
			joints.DefaultDrag( Safe_atof( args[0] ) );
		}
		else if ( !stricmp( command, "$rollingDrag" ) )
		{
			argCount = ReadArgs( args, 1 );
			joints.DefaultRollingDrag( Safe_atof( args[0] ) );
		}
		else if ( !stricmp( command, "$concave" ) )
		{
			joints.AllowConcave();
		}
		// joint commands
		else if ( !strcmp( command, "$jointskip" ) )
		{
			argCount = ReadArgs( args, 1 );
			CCmd_JointSkip( joints, args[0] );
		}
		else if ( !stricmp( command, "$jointmerge" ) )
		{
			argCount = ReadArgs( args, 2 );
			CCmd_JointMerge( joints, args[0], args[1] );
		}
		else if ( !stricmp( command, "$rootbone" ) )
		{
			argCount = ReadArgs( args, 1 );
			CCmd_JointRoot( joints, args[0] );
		}
		else if ( !stricmp( command, "$jointconstrain" ) )
		{
			argCount = ReadArgs( args, 6 );
			char *pFriction = args[5];
			if ( argCount < 6 )
			{
				pFriction = "1.0";
			}
			CCmd_JointConstrain( joints, args[0], args[1], args[2], args[3], args[4], pFriction );
		}
		// joint properties
		else if ( !stricmp( command, "$jointinertia" ) )
		{
			argCount = ReadArgs( args, 2 );
			joints.JointInertia( args[0], Safe_atof( args[1] ) );
		}
		else if ( !stricmp( command, "$jointdamping" ) )
		{
			argCount = ReadArgs( args, 2 );
			joints.JointDamping( args[0], Safe_atof( args[1] ) );
		}
		else if ( !stricmp( command, "$jointrotdamping" ) )
		{
			argCount = ReadArgs( args, 2 );
			joints.JointRotdamping( args[0], Safe_atof( args[1] ) );
		}
		else if ( !stricmp( command, "$jointmassbias" ) )
		{
			argCount = ReadArgs( args, 2 );
			joints.JointMassBias( args[0], Safe_atof( args[1] ) );
		}
		else
		{
			printf("Unknown command %s in collision series\n", command );
		}
	}
}


int Cmd_CollisionText( void )
{
	int level = 1;

	if ( !GetToken( true ) )
		return 0;

	if ( token[0] != '{' )
		return 0;


	while ( GetToken(true) )
	{
		if ( !strcmp( token, "}" ) )
		{
			level--;
			if ( level <= 0 )
				break;
			g_JointedModel.AddText( " }\n" );
		}
		else if ( !strcmp( token, "{" ) )
		{
			g_JointedModel.AddText( "{" );
			level++;
		}
		else
		{
			// tokens inside braces  are quoted
			if ( level > 1 )
			{
				g_JointedModel.AddText( "\"" );
				g_JointedModel.AddText( token );
				g_JointedModel.AddText( "\" " );
			}
			else
			{
				g_JointedModel.AddText( token );
				g_JointedModel.AddText( " " );
			}
		}
	}

	return 1;
}

static bool LoadSurfaceProps( const char *pMaterialFilename )
{
	if ( !physprops )
		return false;

	// already loaded
	if ( physprops->SurfacePropCount() )
		return false;

	char buf[1024];
	strcpy( buf, gamedir );
	strcat( buf, pMaterialFilename );
	FILE *fp = fopen( buf, "rb" );

	if ( !fp )
	{
		// try base game dir
		strcpy( buf, basegamedir );
		strcat( buf, pMaterialFilename );
		fp = fopen( buf, "rb" );
		if ( !fp )
			return false;
	}

	fseek( fp, 0, SEEK_END );

	int len = ftell(fp);
	fseek( fp, 0, SEEK_SET );

	char *pText = new char[len+1];
	fread( pText, len, 1, fp );
	fclose( fp );
	pText[len]=0;

	physprops->ParseSurfaceData( pMaterialFilename, pText );

	delete[] pText;

	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Entry point for script processing.  Delegate to necessary subroutines.
//			Parse the collisionmodel {} and collisionjoints {} chunks
// Input  : separateJoints - whether this has a constraint system or not (true if it does)
// Output : int
//-----------------------------------------------------------------------------
int Cmd_CollisionModel( bool separateJoints )
{
	char name[512];
	s_source_t *pmodel;

	// name
	if (!GetToken(false)) return 0;

	strcpyn( name, token );

	char dirname[512];
	strcpy( dirname, basegamedir );
	StripLastDir( dirname );

	strcat( dirname, "VPHYSICS.DLL" );
	PhysicsDLLPath( dirname );

	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( !physicsFactory )
		return 0;

	physcollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
	physprops = (IPhysicsSurfaceProps *)physicsFactory( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL );
	LoadSurfaceProps( "scripts/surfaceproperties.txt" );

	pmodel = Load_Source( name, "SMD" );
	if ( !pmodel )
		return 0;

	// all bones map to themselves by default
	g_JointedModel.SetSource( pmodel );
	
	bool parseCommands = false;

	// If the next token is a { that means a data block for the collision model
	if (GetToken(true))
	{
		if ( !strcmp( token, "{" ) )
		{
			parseCommands = true;
		}
		else
		{
			UnGetToken();
		}
	}

	if ( parseCommands )
	{
		ParseCollisionCommands( g_JointedModel );
	}

	g_bJointed = separateJoints;

	// collision script is stored in g_JointedModel for later processing
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Walk the list of models, add up the volume
// Input  : *pList - 
// Output : float
//-----------------------------------------------------------------------------
float TotalVolume( CPhysCollisionModel *pList )
{
	float volume = 0;
	while ( pList )
	{
		volume += pList->m_volume * pList->m_massBias;
		pList = pList->m_pNext;
	}

	return volume;
}

//-----------------------------------------------------------------------------
// Purpose: Write key/value pairs out to a file
// Input  : *fp - output file
//			*pKeyName - key name
//			outputData - type specific output data
//-----------------------------------------------------------------------------
void KeyWriteInt( FILE *fp, const char *pKeyName, int outputData )
{
	fprintf( fp, "\"%s\" \"%d\"\n", pKeyName, outputData );
}

void KeyWriteString( FILE *fp, const char *pKeyName, const char *outputData )
{
	fprintf( fp, "\"%s\" \"%s\"\n", pKeyName, outputData );
}

void KeyWriteVector3( FILE *fp, const char *pKeyName, const Vector& outputData )
{
	fprintf( fp, "\"%s\" \"%f %f %f\"\n", pKeyName, outputData[0], outputData[1], outputData[2] );
}

void KeyWriteQAngle( FILE *fp, const char *pKeyName, const QAngle& outputData )
{
	fprintf( fp, "\"%s\" \"%f %f %f\"\n", pKeyName, outputData[0], outputData[1], outputData[2] );
}

void KeyWriteFloat( FILE *fp, const char *pKeyName, float outputData )
{
	fprintf( fp, "\"%s\" \"%f\"\n", pKeyName, outputData );
}


void FixCollisionHierarchy( CJointedModel &joints )
{
	if ( joints.m_pCollisionList )
	{
		CPhysCollisionModel *pPhys = joints.m_pCollisionList;

		FixBoneList( joints.m_bonemap, joints.m_pModel, joints.m_pCollisionList );
		// Point parents at joints that are actually in the model
		for ( ;pPhys; pPhys = pPhys->m_pNext )
		{
			pPhys->m_parent = FixParent( joints.m_pCollisionList, joints.m_pModel, pPhys->m_parent );
		}

		// sort the list so parents come before children
		joints.SortCollisionList();
		// Now remap the constraints to bones to 
		// Now that bones are in order, set physics indices in main bone structure

		CJointConstraint *pList = g_JointedModel.m_pConstraintList;
		while ( pList )
		{
			pList->m_pJointName = FixParent( joints.m_pCollisionList, joints.m_pModel, pList->m_pJointName );
			pList = pList->m_pNext;
		}

		pPhys = joints.m_pCollisionList;
		int i;
		for ( i = 0; i < g_numbones; i++ )
		{
			g_bonetable[i].physicsBoneIndex = -1;
		}
		int index = 0;
		while ( pPhys )
		{
			int boneIndex = FindBoneInTable( pPhys->m_name );
			if ( boneIndex >= 0 )
			{
				g_bonetable[boneIndex].physicsBoneIndex = index;
			}
			pPhys = pPhys->m_pNext;
			index ++;
		}
		for ( i = 0; i < g_numbones; i++ )
		{
			// if no bone was set, set to parent bone
			if ( g_bonetable[i].physicsBoneIndex < 0 )
			{
				int index = g_bonetable[i].parent;
				int bone = -1;
				while ( index >= 0 )
				{
					bone = g_bonetable[index].physicsBoneIndex;
					if ( bone >= 0 )
						break;
					index = g_bonetable[index].parent;
				}

				// found one?
				if ( bone >= 0 )
				{
					g_bonetable[i].physicsBoneIndex = bone;
				}
				else
				{
					// just set physics to affect root
					g_bonetable[i].physicsBoneIndex = 0;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds the physics/collision model.
//			This must execute after the model has been simplified!!
//-----------------------------------------------------------------------------
void BuildCollisionModel( void )
{
	// no collision model referenced
	if ( !g_JointedModel.m_pModel )
		return;

	g_JointedModel.Simplify();
	if ( g_bJointed )
	{
		ProcessJointedModel( g_JointedModel );
	}
	else
	{
		ProcessSingleBody( g_JointedModel );
	}
	FixCollisionHierarchy( g_JointedModel );
	if( !g_quiet )
	{
		printf("Collision model completed.\n" );
	}
}

void BuildRagdollConstraint( CPhysCollisionModel *pPhys, constraint_ragdollparams_t &ragdoll )
{
	memset( &ragdoll, 0, sizeof(ragdoll) );
	ragdoll.parentIndex = g_JointedModel.CollisionIndex(pPhys->m_parent);
	ragdoll.childIndex = g_JointedModel.CollisionIndex(pPhys->m_name);
	if ( ragdoll.parentIndex < 0 || ragdoll.childIndex < 0 )
	{
		printf("ERROR: Constraint between bone %s and %s\n", pPhys->m_name, pPhys->m_parent );
		if ( ragdoll.childIndex < 0 )
			printf("ERROR: \"%s\" does not appear in collision model!!!\n", pPhys->m_name );
		if ( ragdoll.parentIndex < 0 )
			printf("ERROR: \"%s\" does not appear in collision model!!!\n", pPhys->m_parent );
		Error("Bad constraint in ragdoll\n");
	}
	CJointConstraint *pList = g_JointedModel.m_pConstraintList;
	while ( pList )
	{
		int index = g_JointedModel.CollisionIndex(pList->m_pJointName);
		CPhysCollisionModel *pListModel = g_JointedModel.GetCollisionModel(pList->m_pJointName);
		if ( index < 0 )
		{
			Error("ERROR: Rotation constraint on bone \"%s\" which does not appear in collision model!!!\n", pList->m_pJointName );
		}
		else if ( (!pListModel->m_parent || g_JointedModel.CollisionIndex(pListModel->m_parent) < 0) && strcmpi( pList->m_pJointName, g_JointedModel.m_rootName ) )
		{
			Error("ERROR: Rotation constraint on bone \"%s\" which has no parent!!!\n", pList->m_pJointName );
		}
		else if ( index == ragdoll.childIndex )
		{
			float limitMin = 0, limitMax = 0;
			switch ( pList->m_jointType )
			{
			case JOINT_LIMIT:
				ragdoll.axes[pList->m_axis].SetAxisFriction( pList->m_limitMin, pList->m_limitMax, pList->m_friction );
				break;
			case JOINT_FIXED:
				ragdoll.axes[pList->m_axis].SetAxisFriction( 0,0,0 );
				break;
			case JOINT_FREE:
				ragdoll.axes[pList->m_axis].SetAxisFriction( -360, 360, pList->m_friction );
				break;
			}
		}
		pList = pList->m_pNext;
	}
}

float GetCollisionModelMass()
{
	return g_JointedModel.m_totalMass;
}

//-----------------------------------------------------------------------------
// Purpose: Write out any data that's been saved in the globals
//-----------------------------------------------------------------------------
void WriteCollisionModel( long checkSum )
{
	if ( g_JointedModel.m_pCollisionList )
	{
		CPhysCollisionModel *pPhys = g_JointedModel.m_pCollisionList;

		char filename[512];

		strcpy( filename, gamedir );
		if( *g_pPlatformName )
		{
			strcat( filename, "platform_" );
			strcat( filename, g_pPlatformName );
			strcat( filename, "/" );	
		}
		strcat( filename, "models/" );	
		strcat( filename, outname );	

		g_JointedModel.ComputeMass();

		float volume = TotalVolume( pPhys );
		if ( volume <= 0 )
			volume = 1;
		if( !g_quiet )
		{
			printf("Collision model volume %.2f in^3\n", volume );
		}

		SetExtension( filename, filename, ".phy" );
		FILE *fp = fopen( filename, "wb" );
		if ( fp )
		{
			// write out the collision header (size is version)
			phyheader_t header;
			header.size = sizeof(header);
			header.id = 0;
			header.checkSum = checkSum;

			header.solidCount = 0;
			pPhys = g_JointedModel.m_pCollisionList;
			while ( pPhys )
			{
				header.solidCount++;
				pPhys = pPhys->m_pNext;
			}

			fwrite( &header, sizeof(header), 1, fp );

			// Write out the binary physics collision data
			pPhys = g_JointedModel.m_pCollisionList;
			while ( pPhys )
			{
				int size = physcollision->CollideSize( pPhys->m_pCollisionData );
				fwrite( &size, sizeof(int), 1, fp );
				char *buf = (char *)stackalloc( size );
				physcollision->CollideWrite( buf, pPhys->m_pCollisionData );
				fwrite( buf, size, 1, fp );
				pPhys = pPhys->m_pNext;
			}

			// write out the properties of each solid
			int solidIndex = 0;
			pPhys = g_JointedModel.m_pCollisionList;
			while ( pPhys )
			{
				pPhys->m_mass = ((pPhys->m_volume * pPhys->m_massBias) / volume) * g_JointedModel.m_totalMass;
				if ( pPhys->m_mass < 1.0 )
					pPhys->m_mass = 1.0;

				fprintf( fp, "solid {\n" );
				KeyWriteInt( fp, "index", solidIndex );
				KeyWriteString( fp, "name", pPhys->m_name );
				if ( pPhys->m_parent )
				{
					KeyWriteString( fp, "parent", pPhys->m_parent );
				}
			
				KeyWriteFloat( fp, "mass", pPhys->m_mass );
				//KeyWriteFloat( fp, "volume", pPhys->m_volume );

				char* pSurfaceProps = GetSurfaceProp( pPhys->m_name );

				KeyWriteString( fp, "surfaceprop", pSurfaceProps );
				KeyWriteFloat( fp, "damping", pPhys->m_damping );
				KeyWriteFloat( fp, "rotdamping", pPhys->m_rotdamping );
				
				if ( pPhys->m_drag >= 0 )
				{
					KeyWriteFloat( fp, "drag", pPhys->m_drag );
				}
				if ( pPhys->m_rollingDrag >= 0 )
				{
					KeyWriteFloat( fp, "rollingDrag", pPhys->m_rollingDrag );
				}
				KeyWriteFloat( fp, "inertia", pPhys->m_inertia );
				KeyWriteFloat( fp, "volume", pPhys->m_volume );
				if ( pPhys->m_massBias != 1.0f )
				{
					KeyWriteFloat( fp, "massbias", pPhys->m_massBias );
				}

				fprintf( fp, "}\n" );
				pPhys = pPhys->m_pNext;
				solidIndex++;

			}

			// by default, write constraints from each limb to its parent
			pPhys = g_JointedModel.m_pCollisionList;
			while ( pPhys )
			{
				if ( pPhys->m_parent )
				{
					constraint_ragdollparams_t ragdoll;
					BuildRagdollConstraint( pPhys, ragdoll );
					
					fprintf( fp, "ragdollconstraint {\n" );
					KeyWriteInt( fp, "parent", ragdoll.parentIndex );
					KeyWriteInt( fp, "child", ragdoll.childIndex );
					KeyWriteFloat( fp, "xmin", ragdoll.axes[0].minRotation );
					KeyWriteFloat( fp, "xmax", ragdoll.axes[0].maxRotation );
					KeyWriteFloat( fp, "xfriction", ragdoll.axes[0].torque );
					KeyWriteFloat( fp, "ymin", ragdoll.axes[1].minRotation );
					KeyWriteFloat( fp, "ymax", ragdoll.axes[1].maxRotation );
					KeyWriteFloat( fp, "yfriction", ragdoll.axes[1].torque );
					KeyWriteFloat( fp, "zmin", ragdoll.axes[2].minRotation );
					KeyWriteFloat( fp, "zmax", ragdoll.axes[2].maxRotation );
					KeyWriteFloat( fp, "zfriction", ragdoll.axes[2].torque );
					fprintf( fp, "}\n" );
				}
				pPhys = pPhys->m_pNext;

			}

			// block that is only parsed by the editor
			fprintf( fp, "editparams {\n" );
			KeyWriteString( fp, "rootname", g_JointedModel.m_rootName );
			KeyWriteFloat( fp, "totalmass", g_JointedModel.m_totalMass );
			if ( g_JointedModel.m_allowConcave )
			{
				KeyWriteInt( fp, "concave", 1 );
			}
			for ( int k = 0; k < g_JointedModel.m_mergeList.Count(); k++ )
			{
				char buf[512];
				Q_snprintf( buf, sizeof(buf), "%s,%s", g_JointedModel.m_mergeList[k].pParent, g_JointedModel.m_mergeList[k].pChild );
				KeyWriteString( fp, "jointmerge", buf );
			}

			fprintf( fp, "}\n" );

			char terminator = 0;
			if ( g_JointedModel.m_textCommands.Size() )
			{
				fwrite( g_JointedModel.m_textCommands.Base(), g_JointedModel.m_textCommands.Size(), 1, fp );
			}
			fwrite( &terminator, sizeof(terminator), 1, fp );
			fclose( fp );
		}
		else
		{
			printf("Error writing %s!!!\n", filename );
		}
	}
}


