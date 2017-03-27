/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/
// studio_render.cpp: routines for drawing Half-Life 3DStudio models
// updates:
// 1-4-99	fixed AdvanceFrame wraping bug

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include <windows.h> // for OutputDebugString. . has to be a better way!


#include "ViewerSettings.h"
#include "StudioModel.h"
#include "vphysics/constraints.h"
#include "physmesh.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "matsyswin.h"
#include "istudiorender.h"

#include "studio_render.h"
#include "materialsystem/IMesh.h"
#include "bone_setup.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "MDLViewer.h"

// FIXME:
extern ViewerSettings g_viewerSettings;
int g_dxlevel = 0;

#pragma warning( disable : 4244 ) // double to float

////////////////////////////////////////////////////////////////////////

Vector			g_flexedverts[MAXSTUDIOVERTS];
Vector			g_flexednorms[MAXSTUDIOVERTS];
int				g_flexages[MAXSTUDIOVERTS];

Vector			*g_pflexedverts;
Vector			*g_pflexednorms;
int				*g_pflexages;

int				g_smodels_total;				// cookie

matrix3x4_t		g_viewtransform;				// view transformation
//matrix3x4_t	g_posetoworld[MAXSTUDIOBONES];	// bone transformation matrix

static int			maxNumVertices;
static int			first = 1;
////////////////////////////////////////////////////////////////////////


mstudioseqdesc_t *StudioModel::GetSeqDesc( int seq )
{
	// these should be dynamcially loaded
	return m_pstudiohdr->pSeqdesc( seq );
}

mstudioanimdesc_t *StudioModel::GetAnimDesc( int anim )
{
	return m_pstudiohdr->pAnimdesc( anim );
}

mstudioanim_t *StudioModel::GetAnim( int anim )
{
	mstudioanimdesc_t *panimdesc = GetAnimDesc( anim );
	return panimdesc->pAnim( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Keeps a global clock to autoplay sequences to run from
//			Also deals with speedScale changes
//-----------------------------------------------------------------------------
float GetAutoPlayTime( void )
{
	static float g_prevTicks;
	static float g_time;

	g_time += ( (GetTickCount() - g_prevTicks) / 1000.0f ) * g_viewerSettings.speedScale;
	g_prevTicks = GetTickCount();

	return g_time;
}

void StudioModel::AdvanceFrame( float dt )
{
	if (dt > 0.1)
		dt = 0.1f;

	m_dt = dt;

	float t = GetDuration( );

	if (t > 0)
	{
		if (dt > 0)
		{
			m_cycle += dt / t;
			m_sequencetime += dt;

			// wrap
			m_cycle -= (int)(m_cycle);
		}
	}
	else
	{
		m_cycle = 0;
	}

	
	for (int i = 0; i < MAXSTUDIOANIMLAYERS; i++)
	{
		t = GetDuration( m_Layer[i].m_sequence );
		if (t > 0)
		{
			if (dt > 0)
			{
				m_Layer[i].m_cycle += (dt / t) * m_Layer[i].m_playbackrate;
				m_Layer[i].m_cycle -= (int)(m_Layer[i].m_cycle);
			}
		}
		else
		{
			m_Layer[i].m_cycle = 0;
		}
	}
}

float StudioModel::GetCycle( void )
{
	return m_cycle;
}

float StudioModel::GetFrame( void )
{
	return GetCycle() * GetMaxFrame();
}

int StudioModel::GetMaxFrame( void )
{
	return Studio_MaxFrame( m_pstudiohdr, m_sequence, m_poseparameter );
}

int StudioModel::SetFrame( int frame )
{
	if ( !m_pstudiohdr )
		return 0;

	if ( frame <= 0 )
		frame = 0;

	int maxFrame = GetMaxFrame();
	if ( frame >= maxFrame )
	{
		frame = maxFrame;
		m_cycle = 0.99999;
		return frame;
	}

	m_cycle = frame / (float)maxFrame;
	return frame;
}


float StudioModel::GetCycle( int iLayer )
{
	if (iLayer == 0)
	{
		return m_cycle;
	}
	else if (iLayer <= MAXSTUDIOANIMLAYERS)
	{
		int index = iLayer - 1;
		return m_Layer[index].m_cycle;
	}
	return 0;
}


float StudioModel::GetFrame( int iLayer )
{
	return GetCycle( iLayer ) * GetMaxFrame( iLayer );
}


int StudioModel::GetMaxFrame( int iLayer )
{
	if (iLayer == 0)
	{
		return Studio_MaxFrame( m_pstudiohdr, m_sequence, m_poseparameter );
	}
	else if (iLayer <= MAXSTUDIOANIMLAYERS)
	{
		int index = iLayer - 1;
		return Studio_MaxFrame( m_pstudiohdr, m_Layer[index].m_sequence, m_poseparameter );
	}
	return 0;
}


int StudioModel::SetFrame( int iLayer, int frame )
{
	if ( !m_pstudiohdr )
		return 0;

	if ( frame <= 0 )
		frame = 0;

	int maxFrame = GetMaxFrame( iLayer );
	float cycle = 0;
	if (maxFrame)
	{
		if ( frame >= maxFrame )
		{
			frame = maxFrame;
			cycle = 0.99999;
		}
		cycle = frame / (float)maxFrame;
	}

	if (iLayer == 0)
	{
		m_cycle = cycle;
	}
	else if (iLayer <= MAXSTUDIOANIMLAYERS)
	{
		int index = iLayer - 1;
		m_Layer[index].m_cycle = cycle;
	}

	return frame;
}



//-----------------------------------------------------------------------------
// Purpose: Maps from local axis (X,Y,Z) to Half-Life (PITCH,YAW,ROLL) axis/rotation mappings
//-----------------------------------------------------------------------------
static int RemapAxis( int axis )
{
	switch( axis )
	{
	case 0:
		return 2;
	case 1:
		return 0;
	case 2:
		return 1;
	}

	return 0;
}

void StudioModel::Physics_SetPreview( int previewBone, int axis, float t )
{
	m_physPreviewBone = previewBone;
	m_physPreviewAxis = axis;
	m_physPreviewParam = t;
}


void StudioModel::OverrideBones( bool *override )
{
	matrix3x4_t basematrix;
	matrix3x4_t bonematrix;

	QAngle tmp;
	// offset for the base pose to world transform of 90 degrees around up axis
	tmp[0] = 0; tmp[1] = 90; tmp[2] = 0;
	AngleMatrix( tmp, bonematrix );
	ConcatTransforms( g_viewtransform, bonematrix, basematrix );

	for ( int i = 0; i < m_pPhysics->Count(); i++ )
	{
		CPhysmesh *pmesh = m_pPhysics->GetMesh( i );
		// BUGBUG: Cache this if you care about performance!
		int boneIndex = FindBone(pmesh->m_boneName);

		if ( boneIndex >= 0 )
		{
			matrix3x4_t *parentMatrix = &basematrix;
			override[boneIndex] = true;
			int parentBone = -1;
			if ( pmesh->m_constraint.parentIndex >= 0 )
			{
				parentBone = FindBone( m_pPhysics->GetMesh(pmesh->m_constraint.parentIndex)->m_boneName );
			}
			if ( parentBone >= 0 )
			{
				parentMatrix = m_pStudioRender->GetBoneToWorld( parentBone );
			}

			if ( m_physPreviewBone == i )
			{
				matrix3x4_t tmpmatrix;
				QAngle rot;
				constraint_axislimit_t *axis = pmesh->m_constraint.axes + m_physPreviewAxis;

				int hlAxis = RemapAxis( m_physPreviewAxis );
				rot.Init();
				rot[hlAxis] = axis->minRotation + (axis->maxRotation - axis->minRotation) * m_physPreviewParam;
				AngleMatrix( rot, tmpmatrix );
				ConcatTransforms( pmesh->m_matrix, tmpmatrix, bonematrix );
			}
			else
			{
				MatrixCopy( pmesh->m_matrix, bonematrix );
			}

			ConcatTransforms(*parentMatrix, bonematrix, *m_pStudioRender->GetBoneToWorld( boneIndex ));
		}
	}
}


static CIKContext ik;


void StudioModel::SetUpBones ( void )
{
	int					i, j;

	mstudiobone_t		*pbones;

	static Vector		pos[MAXSTUDIOBONES];
	matrix3x4_t			bonematrix;
	static Quaternion	q[MAXSTUDIOBONES];
	bool				override[MAXSTUDIOBONES];

	// For blended transitions
	static Vector		pos2[MAXSTUDIOBONES];
	static Quaternion	q2[MAXSTUDIOBONES];

	mstudioseqdesc_t	*pseqdesc;
	pseqdesc = m_pstudiohdr->pSeqdesc( m_sequence );

	QAngle a1;
	Vector p1;
	MatrixAngles( g_viewtransform, a1 );
	MatrixPosition( g_viewtransform, p1 );
	CIKContext *pIK = NULL;
	if ( g_viewerSettings.enableIK )
	{
		pIK = &ik;
		ik.Init( m_pstudiohdr, a1, p1, 0.0 );
	}

	InitPose(  m_pstudiohdr, pos, q );
	
	AccumulatePose( m_pstudiohdr, pIK, pos, q, m_sequence, m_cycle, m_poseparameter, BONE_USED_BY_ANYTHING );

	if ( g_viewerSettings.blendSequenceChanges &&
		m_sequencetime < m_blendtime && 
		m_prevsequence != m_sequence &&
		m_prevsequence < m_pstudiohdr->numseq &&
		!(pseqdesc->flags & STUDIO_SNAP) )
	{
		// Make sure frame is valid
		pseqdesc = m_pstudiohdr->pSeqdesc( m_prevsequence );
		if ( m_prevcycle >= 1.0 )
		{
			m_prevcycle = 0.0f;
		}

		float s = 1.0 - ( m_sequencetime / m_blendtime );
		s = 3 * s * s - 2 * s * s * s;

		AccumulatePose( m_pstudiohdr, NULL, pos, q, m_prevsequence, m_prevcycle, m_poseparameter, BONE_USED_BY_ANYTHING, s );
		// Con_DPrintf("%d %f : %d %f : %f\n", pev->sequence, f, pev->prevsequence, pev->prevframe, s );
	}
	else
	{
		m_prevcycle = m_cycle;
	}

	int iMaxPriority = 0;
	for (i = 0; i < MAXSTUDIOANIMLAYERS; i++)
	{
		if (m_Layer[i].m_weight > 0)
		{
			iMaxPriority = max( m_Layer[i].m_priority, iMaxPriority );
		}
	}

	for (j = 0; j <= iMaxPriority; j++)
	{
		for (i = 0; i < MAXSTUDIOANIMLAYERS; i++)
		{
			if (m_Layer[i].m_priority == j && m_Layer[i].m_weight > 0)
			{
				AccumulatePose( m_pstudiohdr, pIK, pos, q, m_Layer[i].m_sequence, m_Layer[i].m_cycle, m_poseparameter, BONE_USED_BY_ANYTHING, m_Layer[i].m_weight );
			}
		}
	}

	if (g_viewerSettings.solveHeadTurn != 0)
	{
		GetBodyPoseParametersFromFlex( );
	}

	SetHeadPosition( pos, q );

	CIKContext auto_ik;
	auto_ik.Init( m_pstudiohdr, a1, p1, 0.0 );

	CalcAutoplaySequences( m_pstudiohdr, &auto_ik, pos, q, m_poseparameter, BONE_USED_BY_ANYTHING, GetAutoPlayTime() );

	CalcBoneAdj( m_pstudiohdr, pos, q, m_controller, BONE_USED_BY_ANYTHING );

	if (pIK)
	{
		pIK->SolveDependencies( pos, q );
	}

	pbones = m_pstudiohdr->pBone( 0 );

	memset( override, 0, sizeof(bool)*m_pstudiohdr->numbones );

	if ( g_viewerSettings.showPhysicsPreview )
	{
		OverrideBones( override );
	}

	for (i = 0; i < m_pstudiohdr->numbones; i++) 
	{
		if ( override[i] )
		{
			continue;
		}
		else if (CalcProceduralBone( m_pstudiohdr, i, m_pStudioRender->GetBoneToWorldArray() ))
		{
			continue;
		}
		else
		{
			QuaternionMatrix( q[i], bonematrix );

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];
			if (pbones[i].parent == -1) 
			{
				ConcatTransforms (g_viewtransform, bonematrix, *m_pStudioRender->GetBoneToWorld( i ));
				// MatrixCopy(bonematrix, g_bonetoworld[i]);
			} 
			else 
			{
				ConcatTransforms (*m_pStudioRender->GetBoneToWorld( pbones[i].parent ), bonematrix, *m_pStudioRender->GetBoneToWorld( i ) );
			}
		}
	}

	if (g_viewerSettings.showAttachments)
	{
		// drawTransform( m_pStudioRender->GetBoneToWorld( 0 ) );
	}
}



/*
================
StudioModel::SetupLighting
	set some global variables based on entity position
inputs:
outputs:
================
*/
void StudioModel::SetupLighting ( )
{
	LightDesc_t light;

	light.m_Type = MATERIAL_LIGHT_DIRECTIONAL;
	light.m_Attenuation0 = 1.0f;
	light.m_Attenuation1 = 0.0;
	light.m_Attenuation2 = 0.0;
	light.m_Color[0] = g_viewerSettings.lColor[0];
	light.m_Color[1] = g_viewerSettings.lColor[1];
	light.m_Color[2] = g_viewerSettings.lColor[2];
	light.m_Range = 2000;

	// DEBUG: Spin the light around the head for debugging
	// g_viewerSettings.lightrot = Vector( 0, 0, 0 );
	// g_viewerSettings.lightrot.y = fmod( (90 * GetTickCount( ) / 1000.0), 360.0);

	AngleVectors( g_viewerSettings.lightrot, &light.m_Direction, NULL, NULL );
	m_pStudioRender->SetLocalLights( 1, &light );
	int i;
	for( i = 0; i < m_pStudioRender->GetNumAmbientLightSamples(); i++ )
	{
		m_AmbientLightColors[i][0] = g_viewerSettings.aColor[0];
		m_AmbientLightColors[i][1] = g_viewerSettings.aColor[1];
		m_AmbientLightColors[i][2] = g_viewerSettings.aColor[2];

		m_TotalLightColors[i][0] = m_AmbientLightColors[i][0] + 
			-light.m_Attenuation0 * DotProduct( light.m_Direction, m_pStudioRender->GetAmbientLightDirections()[i] ) * 
			light.m_Color[0];
		m_TotalLightColors[i][1] = m_AmbientLightColors[i][1] + 
			-light.m_Attenuation0 * DotProduct( light.m_Direction, m_pStudioRender->GetAmbientLightDirections()[i] ) * 
			light.m_Color[1];
		m_TotalLightColors[i][2] = m_AmbientLightColors[i][2] + 
			-light.m_Attenuation0 * DotProduct( light.m_Direction, m_pStudioRender->GetAmbientLightDirections()[i] ) * 
			light.m_Color[2];
	}
	m_pStudioRender->SetAmbientLightColors( m_AmbientLightColors, m_TotalLightColors );
}


int FindBoneIndex( studiohdr_t *pstudiohdr, const char *pName )
{
	mstudiobone_t *pbones = pstudiohdr->pBone( 0 );
	for (int i = 0; i < pstudiohdr->numbones; i++)
	{
		if ( !strcmpi( pName, pbones[i].pszName() ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Find the named bone index, -1 if not found
// Input  : *pName - bone name
//-----------------------------------------------------------------------------
int StudioModel::FindBone( const char *pName )
{
	return FindBoneIndex( m_pstudiohdr, pName );
}


int StudioModel::Physics_GetBoneIndex( const char *pName )
{
	for (int i = 0; i < m_pPhysics->Count(); i++)
	{
		CPhysmesh *pmesh = m_pPhysics->GetMesh(i);
		if ( !strcmpi( pName, pmesh[i].m_boneName ) )
			return i;
	}

	return -1;
}


/*
=================
StudioModel::SetupModel
	based on the body part, figure out which mesh it should be using.
inputs:
	currententity
outputs:
	pstudiomesh
	pmdl
=================
*/

void StudioModel::SetupModel ( int bodypart )
{
	int index;

	if (bodypart > m_pstudiohdr->numbodyparts)
	{
		// Con_DPrintf ("StudioModel::SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	mstudiobodyparts_t   *pbodypart = m_pstudiohdr->pBodypart( bodypart );

	index = m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	m_pmodel = pbodypart->pModel( index );

	if(first){
		maxNumVertices = m_pmodel->numvertices;
		first = 0;
	}
}


static IMaterial *g_pAlpha;


//-----------------------------------------------------------------------------
// Draws a box, not wireframed
//-----------------------------------------------------------------------------

void StudioModel::drawBox (Vector const *v, float const * color )
{
	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;

	// The four sides
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 * 4 );
	for (int i = 0; i < 10; i++)
	{
		meshBuilder.Position3fv (v[i & 7].Base() );
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();

	// top and bottom
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

	meshBuilder.Position3fv (v[6].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[0].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[4].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[2].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

	meshBuilder.Position3fv (v[1].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[7].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[3].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[5].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Draws a wireframed box
//-----------------------------------------------------------------------------

void StudioModel::drawWireframeBox (Vector const *v, float const* color )
{
	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;

	// The four sides
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 4 );
	for (int i = 0; i < 10; i++)
	{
		meshBuilder.Position3fv (v[i & 7].Base());
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();

	// top and bottom
	meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 4 );

	meshBuilder.Position3fv (v[6].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[0].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[2].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[4].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[6].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 4 );

	meshBuilder.Position3fv (v[1].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[7].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[5].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[3].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[1].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Draws the position and axies of a transformation matrix, x=red,y=green,z=blue
//-----------------------------------------------------------------------------
void StudioModel::drawTransform( matrix3x4_t& m, float flLength )
{
	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	for (int k = 0; k < 3; k++)
	{
		static unsigned char color[3][3] =
		{
			{ 255, 0, 0 },
			{ 0, 255, 0 },
			{ 0, 0, 255 }
		};

		meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

		meshBuilder.Color3ubv( color[k] );
		meshBuilder.Position3f( m[0][3], m[1][3], m[2][3]);
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3ubv( color[k] );
		meshBuilder.Position3f( m[0][3] + m[0][k] * flLength, m[1][3] + m[1][k] * flLength, m[2][3] + m[2][k] * flLength);
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}

void StudioModel::drawLine( Vector const &p1, Vector const &p2, int r, int g, int b )
{
	g_pMaterialSystem->Bind( g_materialLines );

	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

	meshBuilder.Color3ub( r, g, b );
	meshBuilder.Position3f( p1.x, p1.y, p1.z );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub( r, g, b );
	meshBuilder.Position3f(  p2.x, p2.y, p2.z );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draws a transparent box with a wireframe outline
//-----------------------------------------------------------------------------
void StudioModel::drawTransparentBox( Vector const &bbmin, Vector const &bbmax, 
					const matrix3x4_t& m, float const *color, float const *wirecolor )
{
	Vector v[8], v2[8];

	v[0][0] = bbmin[0];
	v[0][1] = bbmax[1];
	v[0][2] = bbmin[2];

	v[1][0] = bbmin[0];
	v[1][1] = bbmin[1];
	v[1][2] = bbmin[2];

	v[2][0] = bbmax[0];
	v[2][1] = bbmax[1];
	v[2][2] = bbmin[2];

	v[3][0] = bbmax[0];
	v[3][1] = bbmin[1];
	v[3][2] = bbmin[2];

	v[4][0] = bbmax[0];
	v[4][1] = bbmax[1];
	v[4][2] = bbmax[2];

	v[5][0] = bbmax[0];
	v[5][1] = bbmin[1];
	v[5][2] = bbmax[2];

	v[6][0] = bbmin[0];
	v[6][1] = bbmax[1];
	v[6][2] = bbmax[2];

	v[7][0] = bbmin[0];
	v[7][1] = bbmin[1];
	v[7][2] = bbmax[2];

	VectorTransform (v[0], m, v2[0]);
	VectorTransform (v[1], m, v2[1]);
	VectorTransform (v[2], m, v2[2]);
	VectorTransform (v[3], m, v2[3]);
	VectorTransform (v[4], m, v2[4]);
	VectorTransform (v[5], m, v2[5]);
	VectorTransform (v[6], m, v2[6]);
	VectorTransform (v[7], m, v2[7]);
	
	g_pMaterialSystem->Bind( g_pAlpha );
	drawBox( v2, color );

	g_pMaterialSystem->Bind( g_materialBones );
	drawWireframeBox( v2, wirecolor );
}


#define	MAXPRINTMSG	4096
static void StudioRender_Warning( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

//	Msg( mwWarning, "StudioRender: %s", msg );
	OutputDebugString( msg );
}

void StudioModel::UpdateStudioRenderConfig( bool bFlat, bool bWireframe, bool bNormals )
{
	StudioRenderConfig_t config;
	memset( &config, 0, sizeof( config ) );
	config.pConPrintf = StudioRender_Warning;
	config.pConDPrintf = StudioRender_Warning;
	config.fEyeShiftX = 0.0f;
	config.fEyeShiftY = 0.0f;
	config.fEyeShiftZ = 0.0f;
	config.fEyeSize = 0;
	config.gamma = 2.2f;
	config.texGamma = 2.2f;
	config.brightness = 0.0f;
	if( g_viewerSettings.overbright )
	{
//		OutputDebugString( "overbright 2\n" );
		config.overbrightFactor = 2.0f;
	}
	else
	{
//		OutputDebugString( "overbright 1\n" );
		config.overbrightFactor = 1.0f;
	}
	config.modelLightBias = 1.0f;
	config.eyeGloss = 1;
	config.drawEntities = 1;
	config.skin = 0;
	config.fullbright = 0;
	config.bEyeMove = true;
	if( g_viewerSettings.renderMode == RM_WIREFRAME || g_viewerSettings.softwareSkin )
	{
		config.bSoftwareSkin = true;
	}
	else
	{
		config.bSoftwareSkin = false;
	}
	config.bSoftwareLighting = false;
	config.bNoHardware = false;
	config.bNoSoftware = false;
	config.bTeeth = true;
	config.bEyes = true;
	config.bFlex = true;
	if( bWireframe )
	{
		config.bWireframe = true;
	}
	else
	{
		config.bWireframe = false;
	}
	if ( bNormals )
	{
		config.bNormals = true;
	}
	else
	{
		config.bNormals = false;
	}
	config.bUseAmbientCube = true;
	config.bShowEnvCubemapOnly = false;
	m_pStudioRender->UpdateConfig( config );

	MaterialSystem_Config_t matSysConfig;
	extern void InitMaterialSystemConfig(MaterialSystem_Config_t *pConfig);
	InitMaterialSystemConfig( &matSysConfig );
	if( g_viewerSettings.renderMode == RM_FLATSHADED ||
		g_viewerSettings.renderMode == RM_SMOOTHSHADED )
	{
		matSysConfig.bLightingOnly = true;
	}
	matSysConfig.dxSupportLevel = g_dxlevel;
	g_pMaterialSystem->UpdateConfig( &matSysConfig, false );
}


//-----------------------------------------------------------------------------
// Draws the skeleton
//-----------------------------------------------------------------------------

void StudioModel::DrawBones( )
{
	// draw bones
	if (!g_viewerSettings.showBones && (g_viewerSettings.highlightBone < 0))
		return;

	mstudiobone_t *pbones = m_pstudiohdr->pBone( 0 );

	g_pMaterialSystem->Bind( g_materialBones );

	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	bool drawRed = (g_viewerSettings.highlightBone >= 0);

	for (int i = 0; i < m_pstudiohdr->numbones; i++)
	{
		if (pbones[i].parent >= 0)
		{
			int j = pbones[i].parent;
			if ((g_viewerSettings.highlightBone < 0 ) || (j == g_viewerSettings.highlightBone))
			{
				meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

				if (drawRed)
					meshBuilder.Color3ub( 255, 255, 0 );
				else
					meshBuilder.Color3ub( 0, 255, 255 );
				meshBuilder.Position3f( (*m_pStudioRender->GetBoneToWorld( j ))[0][3], (*m_pStudioRender->GetBoneToWorld( j ))[1][3], (*m_pStudioRender->GetBoneToWorld( j ))[2][3]);
				meshBuilder.AdvanceVertex();

				if (drawRed)
					meshBuilder.Color3ub( 255, 255, 0 );
				else
					meshBuilder.Color3ub( 0, 255, 255 );
				meshBuilder.Position3f( (*m_pStudioRender->GetBoneToWorld( i ))[0][3], (*m_pStudioRender->GetBoneToWorld( i ))[1][3], (*m_pStudioRender->GetBoneToWorld( i ))[2][3]);
				meshBuilder.AdvanceVertex();

				meshBuilder.End();
				pMesh->Draw();
			}
		}

		if (g_viewerSettings.highlightBone >= 0)
		{
			if (i != g_viewerSettings.highlightBone)
				continue;
		}

		drawTransform( *m_pStudioRender->GetBoneToWorld( i ) );
	}


	if (g_viewerSettings.highlightBone >= 0)
	{
		int k, j, n;
		for (i = 0; i < m_pstudiohdr->numbodyparts; i++)
		{
			for (j = 0; j < m_pstudiohdr->pBodypart( i )->nummodels; j++)
			{
				mstudiomodel_t *pModel = m_pstudiohdr->pBodypart( i )->pModel( j );

				meshBuilder.Begin( pMesh, MATERIAL_POINTS, 1 );

				for (k = 0; k < pModel->numvertices; k++)
				{
					for (n = 0; n < pModel->BoneWeights( k )->numbones; n++)
					{
						if (pModel->BoneWeights( k )->bone[n] == g_viewerSettings.highlightBone)
						{
							Vector tmp;
							Transform( *pModel->Position( k ), pModel->BoneWeights( k ), tmp );

							meshBuilder.Color3ub( 0, 255, 255 );
							meshBuilder.Position3f( tmp.x, tmp.y, tmp.z );
							meshBuilder.AdvanceVertex();
							break;
						}
					}
				}

				meshBuilder.End();
				pMesh->Draw();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Draws attachments
//-----------------------------------------------------------------------------

void StudioModel::DrawAttachments( )
{
	if (!g_viewerSettings.showAttachments)
		return;

	g_pMaterialSystem->Bind( g_materialBones );

	for (int i = 0; i < m_pstudiohdr->numattachments; i++)
	{
		mstudioattachment_t *pattachments = m_pstudiohdr->pAttachment( 0 );

		matrix3x4_t world;
		ConcatTransforms( *m_pStudioRender->GetBoneToWorld( pattachments[i].bone ), pattachments[i].local, world );

		drawTransform( world );
	}
}


void StudioModel::DrawEditAttachment()
{
	int iEditAttachment = g_viewerSettings.m_iEditAttachment;
	if ( iEditAttachment >= 0 && iEditAttachment < m_pstudiohdr->numattachments )
	{
		g_pMaterialSystem->Bind( g_materialBones );
		
		mstudioattachment_t *pAttachment = m_pstudiohdr->pAttachment( iEditAttachment );

		matrix3x4_t world;
		ConcatTransforms( *m_pStudioRender->GetBoneToWorld( pAttachment->bone ), pAttachment->local, world );

		drawTransform( world );
	}
}


//-----------------------------------------------------------------------------
// Draws hitboxes
//-----------------------------------------------------------------------------


static float hullcolor[8][4] = 
{
	{ 1.0, 1.0, 1.0, 1.0 },
	{ 1.0, 0.5, 0.5, 1.0 },
	{ 0.5, 1.0, 0.5, 1.0 },
	{ 1.0, 1.0, 0.5, 1.0 },
	{ 0.5, 0.5, 1.0, 1.0 },
	{ 1.0, 0.5, 1.0, 1.0 },
	{ 0.5, 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0, 1.0 }
};


void StudioModel::DrawHitboxes( )
{
	if (!g_pAlpha)
	{
		g_pAlpha = g_pMaterialSystem->FindMaterial("debug/debughitbox", NULL, false);
	}

	if (g_viewerSettings.showHitBoxes || (g_viewerSettings.highlightHitbox >= 0))
	{
		int hitboxset = g_MDLViewer->GetCurrentHitboxSet();

		for (unsigned short j = m_HitboxSets[ hitboxset ].Head(); j != m_HitboxSets[ hitboxset ].InvalidIndex(); j = m_HitboxSets[ hitboxset ].Next(j) )
		{
			// Only draw one hitbox if we've selected it.
			if ((g_viewerSettings.highlightHitbox >= 0) && 
				(g_viewerSettings.highlightHitbox != j))
				continue;

			mstudiobbox_t *pBBox = &m_HitboxSets[ hitboxset ][j];

			float interiorcolor[4];
			int c = pBBox->group % 8;
			interiorcolor[0] = hullcolor[c][0] * 0.7;
			interiorcolor[1] = hullcolor[c][1] * 0.7;
			interiorcolor[2] = hullcolor[c][2] * 0.7;
			interiorcolor[3] = hullcolor[c][3] * 0.4;

			drawTransparentBox( pBBox->bbmin, pBBox->bbmax, *m_pStudioRender->GetBoneToWorld( pBBox->bone ), interiorcolor, hullcolor[ c ] );
		}
	}

	/*
	float color2[] = { 0, 0.7, 1, 0.6 };
	float wirecolor2[] = { 0, 1, 1, 1.0 };
	drawTransparentBox( m_pstudiohdr->min, m_pstudiohdr->max, g_viewtransform, color2, wirecolor2 );
	*/

	if (g_viewerSettings.showSequenceBoxes)
	{
		float color[] = { 0.7, 1, 0, 0.6 };
		float wirecolor[] = { 1, 1, 0, 1.0 };

		drawTransparentBox( m_pstudiohdr->pSeqdesc( m_sequence )->bbmin, m_pstudiohdr->pSeqdesc( m_sequence )->bbmax, g_viewtransform, color, wirecolor );
	}
}

//-----------------------------------------------------------------------------
// Draws the physics model
//-----------------------------------------------------------------------------

void StudioModel::DrawPhysicsModel( )
{
	if (!g_viewerSettings.showPhysicsModel)
		return;

	if ( g_viewerSettings.renderMode == RM_WIREFRAME && m_pPhysics->Count() == 1 )
	{
		// show the convex pieces in solid
		DrawPhysConvex( m_pPhysics->GetMesh(0), g_materialFlatshaded );
	}
	else
	{
		for (int i = 0; i < m_pPhysics->Count(); i++)
		{
			float red[] = { 1.0, 0, 0, 0.25 };
			float yellow[] = { 1.0, 1.0, 0, 0.5 };

			CPhysmesh *pmesh = m_pPhysics->GetMesh(i);
			int boneIndex = FindBone(pmesh->m_boneName);

			if ( boneIndex >= 0 )
			{
				if ( (i+1) == g_viewerSettings.highlightPhysicsBone )
				{
					DrawPhysmesh( pmesh, boneIndex, g_materialBones, red );
				}
				else
				{
					if ( g_viewerSettings.highlightPhysicsBone < 1 )
					{
						// yellow for most
						DrawPhysmesh( pmesh, boneIndex, g_materialBones, yellow );
					}
				}
			}
			else
			{
				DrawPhysmesh( pmesh, -1, g_materialBones, red );
			}
		}
	}
}




void StudioModel::SetViewTarget( void )
{
	int iEyeUpdown = LookupFlexController( "eyes_updown" );

	int iEyeRightleft = LookupFlexController( "eyes_rightleft" );

	int iEyeAttachment = LookupAttachment( "eyes" );
	
	if (iEyeUpdown == -1 || iEyeRightleft == -1 || iEyeAttachment == -1)
		return;

	Vector local;
	Vector tmp;

	mstudioattachment_t *patt = m_pstudiohdr->pAttachment( iEyeAttachment );
	matrix3x4_t attToWorld;
	ConcatTransforms( *m_pStudioRender->GetBoneToWorld( patt->bone ), patt->local, attToWorld ); 

	// look forward
	local = Vector( 32, 0, 0 );
	if (g_viewerSettings.flHeadTurn != 0.0f)
	{
		// aim the eyes
		VectorITransform( g_viewerSettings.vecEyeTarget, attToWorld, local );
	}
	
	float flDist = local.Length();

	VectorNormalize( local );

	// calculate animated eye deflection
	Vector eyeDeflect;
	QAngle eyeAng( GetFlexController(iEyeUpdown), GetFlexController(iEyeRightleft), 0 );

	// debugoverlay->AddTextOverlay( m_vecOrigin + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f", eyeAng.x, eyeAng.y );

	AngleVectors( eyeAng, &eyeDeflect );
	eyeDeflect.x = 0;

	// reduce deflection the more the eye is off center
	// FIXME: this angles make no damn sense
	eyeDeflect = eyeDeflect * (local.x * local.x);
	local = local + eyeDeflect;
	VectorNormalize( local );

	// check to see if the eye is aiming outside a 30 degree cone
	if (local.x < 0.866) // cos(30)
	{
		// if so, clamp it to 30 degrees offset
		// debugoverlay->AddTextOverlay( m_vecOrigin + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f %.2f", local.x, local.y, local.z );
		local.x = 0;
		float d = local.LengthSqr();
		if (d > 0.0)
		{
			d = sqrt( (1.0 - 0.866 * 0.866) / (local.y*local.y + local.z*local.z) );
			local.x = 0.866;
			local.y = local.y * d;
			local.z = local.z * d;
		}
		else
		{
			local.x = 1.0;
		}
	}
	local = local * flDist;
	VectorTransform( local, attToWorld, tmp );

	m_pStudioRender->SetEyeViewTarget( tmp );
}


float UTIL_VecToYaw( const matrix3x4_t& matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float y = matrix[0][1] * tmp.x + matrix[1][1] * tmp.y + matrix[2][1] * tmp.z;

	if (x == 0.0f && y == 0.0f)
		return 0.0f;
	
	float yaw = atan2( -y, x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float UTIL_VecToPitch( const matrix3x4_t& matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float z = matrix[0][2] * tmp.x + matrix[1][2] * tmp.y + matrix[2][2] * tmp.z;

	if (x == 0.0f && z == 0.0f)
		return 0.0f;
	
	float pitch = atan2( z, x );

	pitch = RAD2DEG(pitch);

	if (pitch < 0)
		pitch += 360;

	return pitch;
}


float UTIL_AngleDiff( float destAngle, float srcAngle )
{
	float delta;

	delta = destAngle - srcAngle;
	if ( destAngle > srcAngle )
	{
		while ( delta >= 180 )
			delta -= 360;
	}
	else
	{
		while ( delta <= -180 )
			delta += 360;
	}
	return delta;
}


void StudioModel::UpdateBoneChain(
	Vector pos[], 
	Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld )
{
	matrix3x4_t bonematrix;

	QuaternionMatrix( q[iBone], pos[iBone], bonematrix );

	int parent = m_pstudiohdr->pBone( iBone )->parent;
	if (parent == -1) 
	{
		ConcatTransforms( g_viewtransform, bonematrix, pBoneToWorld[iBone] );
	}
	else
	{
		// evil recursive!!!
		UpdateBoneChain( pos, q, parent, pBoneToWorld );
		ConcatTransforms( pBoneToWorld[parent], bonematrix, pBoneToWorld[iBone] );
	}
}




void StudioModel::GetBodyPoseParametersFromFlex( )
{
	float flGoal;

	flGoal = GetFlexController( "move_rightleft" );
	SetPoseParameter( "body_trans_Y", flGoal );
	
	flGoal = GetFlexController( "move_forwardback" );
	SetPoseParameter( "body_trans_X", flGoal );

	flGoal = GetFlexController( "move_updown" );
	SetPoseParameter( "body_lift", flGoal );

	flGoal = GetFlexController( "body_rightleft" );
	SetPoseParameter( "body_yaw", flGoal );

	flGoal = GetFlexController( "body_updown" );
	SetPoseParameter( "body_pitch", flGoal );

	flGoal = GetFlexController( "body_tilt" );
	SetPoseParameter( "body_roll", flGoal );

	flGoal = GetFlexController( "chest_rightleft" );
	SetPoseParameter( "spine_yaw", flGoal );

	flGoal = GetFlexController( "chest_updown" );
	SetPoseParameter( "spine_pitch", flGoal );

	flGoal = GetFlexController( "chest_tilt" );
	SetPoseParameter( "spine_roll", flGoal );

	flGoal = GetFlexController( "head_forwardback" );
	SetPoseParameter( "neck_trans", flGoal );
}




void StudioModel::SetHeadPosition( Vector pos[], Quaternion q[] )
{
	static Vector		pos2[MAXSTUDIOBONES];
	static Quaternion	q2[MAXSTUDIOBONES];
	Vector vTargetPos = Vector( 0, 0, 0 );

	if (g_viewerSettings.solveHeadTurn == 0)
		return;

	if (m_dt == 0.0f)
	{
		m_dt = m_dt;
	}
	else
	{
		m_dt = m_dt;
	}

	// GetAttachment( "eyes", vEyePosition, vEyeAngles );
	int iEyeAttachment = LookupAttachment( "eyes" );
	if (iEyeAttachment == -1)
		return;

	mstudioattachment_t *patt = m_pstudiohdr->pAttachment( iEyeAttachment );

	int iLoops = 1;
	float dt = m_dt;
	if (g_viewerSettings.solveHeadTurn == 2)
	{
		iLoops = 100;
		dt = 0.1;
	}

	Vector vDefault;
	CalcDefaultView( patt, pos, q, vDefault );

	float flMoved = 0.0f;
	do 
	{
		for (int j = 0; j < m_pstudiohdr->numbones; j++)
		{
			pos2[j] = pos[j];
			q2[j] = q[j];
		}

		CalcAutoplaySequences( m_pstudiohdr, NULL, pos2, q2, m_poseparameter, BONE_USED_BY_ANYTHING, GetAutoPlayTime() );
		UpdateBoneChain( pos2, q2, patt->bone, m_pStudioRender->GetBoneToWorldArray() );
		matrix3x4_t attToWorld;
		ConcatTransforms( *m_pStudioRender->GetBoneToWorld( patt->bone ), patt->local, attToWorld );

		vTargetPos = g_viewerSettings.vecHeadTarget * g_viewerSettings.flHeadTurn + vDefault * (1 - g_viewerSettings.flHeadTurn);

		flMoved = SetHeadPosition( attToWorld, vTargetPos, dt );
	} while (flMoved > 1.0 && --iLoops > 0);

	// Msg( "yaw %f pitch %f\n", vEyeAngles.y, vEyeAngles.x );
}



float StudioModel::SetHeadPosition( matrix3x4_t& attToWorld, Vector const &vTargetPos, float dt )
{
	float flDiff;
	int iPose;
	float flWeight;
	Vector vEyePosition;
	QAngle vEyeAngles;
	float flPrev;
	float flMoved = 0.0f;

	MatrixAngles( attToWorld, vEyeAngles, vEyePosition );

	// Msg( "yaw %f pitch %f\n", vEyeAngles.y, vEyeAngles.x );

#if 1
	//--------------------------------------
	// Set head yaw
	//--------------------------------------
	float flActualYaw = UTIL_VecToYaw(attToWorld, vTargetPos - vEyePosition);
	float flDesiredYaw = GetFlexController( "head_rightleft" );

	flDiff = UTIL_AngleDiff( flDesiredYaw, flActualYaw );
	flWeight = 1.0 - ExponentialDecay( 0.1, 0.1, dt );
	iPose = LookupPoseParameter( "head_yaw" );
	flPrev = GetPoseParameter( iPose );
	SetPoseParameter( iPose, flPrev + flDiff * flWeight );
	flMoved += fabs( GetPoseParameter( iPose ) - flPrev );
#endif

#if 0
	//--------------------------------------
	// Lag body yaw
	//--------------------------------------
	float flHead = GetPoseParameter( iPose );
	iPose = LookupPoseParameter( "spine_yaw" );
	flWeight = 1.0 - ExponentialDecay( 0.875, 0.1, dt );
	if (fabs( GetPoseParameter( iPose ) + flHead * flWeight) < 45)
	{
		SetPoseParameter( iPose, GetPoseParameter( iPose ) + flHead * flWeight );
	}
#endif

#if 1
	//--------------------------------------
	// Set head pitch
	//--------------------------------------
	float flActualPitch = UTIL_VecToPitch( attToWorld, vTargetPos - vEyePosition );
	float flDesiredPitch = GetFlexController( "head_updown" );

	flDiff = UTIL_AngleDiff( flDesiredPitch, flActualPitch );
	flWeight = 1.0 - ExponentialDecay( 0.1, 0.1, dt );
	iPose = LookupPoseParameter( "head_pitch" );
	flPrev = GetPoseParameter( iPose );
	SetPoseParameter( iPose, flPrev + flDiff * flWeight );
	flMoved += fabs( GetPoseParameter( iPose ) - flPrev );
#endif

	// Msg("rightleft %.1f updown %.1f\n", GetFlexWeight( "head_rightleft" ), GetFlexWeight( "head_updown" ) );

	//iPose = LookupPoseParameter( "body_pitch" );
	//SetPoseParameter( iPose, GetPoseParameter( iPose ) + flDiff / 8.0 );

#if 1
	//--------------------------------------
	// Set head roll
	//--------------------------------------
	float	flDesiredRoll	= GetFlexController( "head_tilt" );
	flDiff = UTIL_AngleDiff( flDesiredRoll, vEyeAngles.z );

	iPose = LookupPoseParameter( "head_roll" );
	flPrev = GetPoseParameter( iPose );
	flWeight = 1.0 - ExponentialDecay( 0.1, 0.1, dt );
	SetPoseParameter( iPose, flPrev + flDiff * flWeight );
	flMoved += fabs( GetPoseParameter( iPose ) - flPrev );
#endif

	return flMoved;
}



void StudioModel::CalcDefaultView( mstudioattachment_t *patt, Vector pos[], Quaternion q[], Vector &vDefault )
{
	Vector		pos2[MAXSTUDIOBONES];
	Quaternion	q2[MAXSTUDIOBONES];
	float tmpPoseParameter[MAXSTUDIOPOSEPARAM];

	int i;
	for ( i = 0; i < m_pstudiohdr->numposeparameters; i++)
	{
		// tmpPoseParameter[i] = m_poseparameter[i];
		Studio_SetPoseParameter( m_pstudiohdr, i, 0.0, tmpPoseParameter[i] );
	}

	/*
	if ((i = LookupPoseParameter( "head_yaw")) != -1)
	{
		Studio_SetPoseParameter( m_pstudiohdr, i, 0.0, tmpPoseParameter[i] );
	}
	if ((i = LookupPoseParameter( "head_pitch")) != -1)
	{
		Studio_SetPoseParameter( m_pstudiohdr, i, 0.0, tmpPoseParameter[i] );
	}
	if ((i = LookupPoseParameter( "head_roll")) != -1)
	{
		Studio_SetPoseParameter( m_pstudiohdr, i, 0.0, tmpPoseParameter[i] );
	}
	*/

	for (int j = 0; j < m_pstudiohdr->numbones; j++)
	{
		pos2[j] = pos[j];
		q2[j] = q[j];
	}

	CalcAutoplaySequences( m_pstudiohdr, NULL, pos2, q2, tmpPoseParameter, BONE_USED_BY_ANYTHING, GetAutoPlayTime() );

	UpdateBoneChain( pos2, q2, patt->bone, m_pStudioRender->GetBoneToWorldArray() );

	matrix3x4_t attToWorld;
	ConcatTransforms( *m_pStudioRender->GetBoneToWorld( patt->bone ), patt->local, attToWorld );

	VectorTransform( Vector( 100, 0, 0 ), attToWorld, vDefault );
}

/*
================
StudioModel::DrawModel
inputs:
	currententity
	r_entorigin
================
*/
int StudioModel::DrawModel( )
{
	if (!m_pstudiohdr)
		return 0;

	g_smodels_total++; // render data cache cookie

	UpdateStudioRenderConfig( g_viewerSettings.renderMode == RM_FLATSHADED ||
			g_viewerSettings.renderMode == RM_SMOOTHSHADED, 
			g_viewerSettings.renderMode == RM_WIREFRAME, 
			g_viewerSettings.showNormals ); // garymcthack - should really only do this once a frame and at init time.

	if (m_pstudiohdr->numbodyparts == 0)
		return 0;

	// Construct a transform to apply to the model. The camera is stuck in a fixed position
	AngleMatrix( g_viewerSettings.rot, g_viewtransform );

	Vector vecModelOrigin;
	VectorMultiply( g_viewerSettings.trans, -1.0f, vecModelOrigin );
	MatrixSetColumn( vecModelOrigin, 3, g_viewtransform );
	
	// These values HAVE to be sent down for LOD to work correctly.
	Vector viewOrigin, viewRight, viewUp, viewPlaneNormal;
	m_pStudioRender->SetViewState( vec3_origin, Vector(0, 1, 0), Vector(0, 0, 1), Vector( 1, 0, 0 ) );

	//	m_pStudioRender->SetEyeViewTarget( viewOrigin );
	
	SetUpBones ( );

	SetViewTarget( );

	SetupLighting( );

	extern float g_flexdescweight[MAXSTUDIOFLEXDESC]; // garymcthack

	RunFlexRules();
	m_pStudioRender->SetFlexWeights( MAXSTUDIOFLEXDESC, g_flexdescweight );
	
	m_pStudioRender->SetAlphaModulation( 1.0f );

	int count = 0;

	DrawModelInfo_t info;
	info.m_pStudioHdr = m_pstudiohdr;
	info.m_pHardwareData = &m_HardwareData;
	info.m_Decals = STUDIORENDER_DECAL_INVALID;
	info.m_Skin = m_skinnum;
	info.m_Body = m_bodynum;
	info.m_HitboxSet = g_MDLViewer->GetCurrentHitboxSet();
	info.m_pClientEntity = NULL;
	info.m_Lod = g_viewerSettings.autoLOD ? -1 : g_viewerSettings.lod;
	info.m_ppColorMeshes = NULL;

	count = m_pStudioRender->DrawModel( info, vecModelOrigin, &m_LodUsed, &m_LodMetric );

	DrawBones();
	DrawAttachments();
	DrawEditAttachment();
	DrawHitboxes();
	DrawPhysicsModel();

	return count;
}


void StudioModel::DrawPhysmesh( CPhysmesh *pMesh, int boneIndex, IMaterial* pMaterial, float* color )
{
	matrix3x4_t *pMatrix;
	if ( boneIndex >= 0 )
	{
		pMatrix = m_pStudioRender->GetBoneToWorld( boneIndex );
	}
	else
	{
		pMatrix = &g_viewtransform;
	}

	g_pMaterialSystem->Bind( pMaterial );
	IMesh* pMatMesh = g_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMatMesh, MATERIAL_TRIANGLES, pMesh->m_vertCount/3 );

	int vertIndex = 0;
	for ( int i = 0; i < pMesh->m_vertCount; i+=3 )
	{
		Vector v;
		
		VectorTransform (pMesh->m_pVerts[vertIndex], *pMatrix, v);
		meshBuilder.Position3fv( v.Base() );
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();

		vertIndex ++;
		VectorTransform (pMesh->m_pVerts[vertIndex], *pMatrix, v);
		meshBuilder.Position3fv( v.Base() );
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();

		vertIndex ++;
		VectorTransform (pMesh->m_pVerts[vertIndex], *pMatrix, v);
		meshBuilder.Position3fv( v.Base() );							 
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();

		vertIndex ++;
	}
	meshBuilder.End();
	pMatMesh->Draw();
}


void RandomColor( float *color, int key )
{
	static bool first = true;
	static colorVec colors[256];

	if ( first )
	{
		int r, g, b;
		first = false;
		for ( int i = 0; i < 256; i++ )
		{
			do 
			{
				r = rand()&255;
				g = rand()&255;
				b = rand()&255;
			} while ( (r+g+b)<256 );
			colors[i].r = r;
			colors[i].g = g;
			colors[i].b = b;
			colors[i].a = 255;
		}
	}

	int index = key & 255;
	color[0] = colors[index].r * (1.f / 255.f);
	color[1] = colors[index].g * (1.f / 255.f);
	color[2] = colors[index].b * (1.f / 255.f);
	color[3] = colors[index].a * (1.f / 255.f);
}

void StudioModel::DrawPhysConvex( CPhysmesh *pMesh, IMaterial* pMaterial )
{
	matrix3x4_t &matrix = g_viewtransform;

	g_pMaterialSystem->Bind( pMaterial );

	int vertIndex = 0;
	for ( int i = 0; i < pMesh->m_pCollisionModel->ConvexCount(); i++ )
	{
		float color[4];
		RandomColor( color, i );
		IMesh* pMatMesh = g_pMaterialSystem->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		int triCount = pMesh->m_pCollisionModel->TriangleCount( i );
		meshBuilder.Begin( pMatMesh, MATERIAL_TRIANGLES, triCount );

		for ( int j = 0; j < triCount; j++ )
		{
			Vector objectSpaceVerts[3];
			pMesh->m_pCollisionModel->GetTriangleVerts( i, j, objectSpaceVerts );

			for ( int k = 0; k < 3; k++ )
			{
				Vector v;
				
				VectorTransform (objectSpaceVerts[k], matrix, v);
				meshBuilder.Position3fv( v.Base() );
				meshBuilder.Color4fv( color );
				meshBuilder.AdvanceVertex();
			}
		}
		meshBuilder.End();
		pMatMesh->Draw();
	}
}


void StudioModel::Transform( Vector const &in1, mstudioboneweight_t const *pboneweight, Vector &out1 )
{
	if (pboneweight->numbones == 1)
	{
		VectorTransform( in1, *m_pStudioRender->GetPoseToWorld(pboneweight->bone[0]), out1 );
	}
	else
	{
		Vector out2;

		VectorFill( out1, 0 );

		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorTransform( in1, *m_pStudioRender->GetPoseToWorld(pboneweight->bone[i]), out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
	}
}


void StudioModel::Rotate( Vector const &in1, mstudioboneweight_t const *pboneweight, Vector &out1 )
{
	if (pboneweight->numbones == 1)
	{
		VectorRotate( in1, *m_pStudioRender->GetPoseToWorld(pboneweight->bone[0]), out1 );
	}
	else
	{
		Vector out2;

		VectorFill( out1, 0 );

		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorRotate( in1, *m_pStudioRender->GetPoseToWorld(pboneweight->bone[i]), out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
		VectorNormalize( out1 );
	}
}







/*
================

================
*/


int StudioModel::GetLodUsed( void )
{
	return m_LodUsed;
}

float StudioModel::GetLodMetric( void )
{
	return m_LodMetric;
}


const char *StudioModel::GetKeyValueText( int iSequence )
{
	return Studio_GetKeyValueText( m_pstudiohdr, iSequence );
}


