
#include "glquake.h"
#include "draw.h"
#include "decal.h"
#include "gl_cvars.h"
#include "view.h"
#include "screen.h"
#include "vid.h"
#include "gl_matsysiface.h"
#include "cdll_int.h"
#include "materialsystem/imesh.h"

ConVar		mat_picmip( "mat_picmip", "0" );

static bool s_bDrawInitialized = false;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Draw_Init (void)
{
	s_bDrawInitialized = true;

	Decal_Init();

	// Gamma table isn't set yet, initialize it
	BuildGammaTable( 2.2f, 2.2f, 0.0f, 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Draw_Shutdown ( void )
{
	s_bDrawInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
void GL_UnloadMaterial( IMaterial *pMaterial )
{
	pMaterial->DecrementReferenceCount();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
// Output : IMaterial
//-----------------------------------------------------------------------------
static IMaterial *GL_LoadMaterialNoRef( const char *pName )
{
	IMaterial *material;
	static const char *lastFailedMaterialName = NULL;
	bool found;
	
	material = NULL;
	if( mat_loadtextures.GetInt() )
	{
		material = materialSystemInterface->FindMaterial( pName, &found );
	}
	else
	{
		material = g_materialEmpty;
	}
	
	return material;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
// Output : IMaterial
//-----------------------------------------------------------------------------
IMaterial *GL_LoadMaterial( const char *pName )
{
	IMaterial *material;
	
	material = GL_LoadMaterialNoRef( pName );
	if( material )
	{
		material->IncrementReferenceCount();
	}
	return material;
}

#ifndef SWDS
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			mins - 
//			maxs - 
//			angles- 
//			r - 
//			g - 
//			b - 
//-----------------------------------------------------------------------------
void Draw_WireframeBox( const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b )
{
	// Build a rotation matrix from angles
	matrix3x4_t fRotateMatrix;
	AngleMatrix(angles, fRotateMatrix);
		
	materialSystemInterface->Bind( g_materialWireframe );
	g_materialWireframe->ColorModulate( r*(1.0f/255.0f), g*(1.0f/255.0f), b*(1.0f/255.0f) );
	g_materialWireframe->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, 0 );

	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 12 );
	
	// Draw the box
	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			int x, y, z;
			x = j;
			y = (j+1)%3;
			z = (j+2)%3;

			Vector start;
			Vector end;
			VectorClear(start);
			VectorClear(end);
			start[x] = mins[x];
			end[x]	 = maxs[x];
			if ( i < 2 )
			{
				start[y] += mins[y];
				end[y] += mins[y];
			}
			else
			{
				start[y] += maxs[y];
				end[y] += maxs[y];
			}
			if ( i & 1 )
			{
				start[z] += mins[z];
				end[z] += mins[z];
			}
			else
			{
				start[z] += maxs[z];
				end[z] += maxs[z];
			}

			Vector tstart, tend;
			// Rotate the corner point
			VectorRotate( start, fRotateMatrix, tstart);
			VectorRotate( end, fRotateMatrix, tend);
			VectorAdd( tstart, origin, start );
			VectorAdd( tend, origin, end );

			meshBuilder.Position3fv( start.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( end.Base() );
			meshBuilder.AdvanceVertex();
		}
	}
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *verts - 
//			id0 - 
//			id1 - 
//			id2 - 
//			id3 - 
// Output : static
//-----------------------------------------------------------------------------
static void DrawIndexedQuad( Vector *verts, int id0, int id1, int id2, int id3 )
{
	CMeshBuilder meshBuilder;
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

	meshBuilder.Position3fv( verts[id0].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( verts[id1].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( verts[id3].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( verts[id2].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			mins - 
//			maxs - 
//			angles - 
//			r - 
//			g - 
//			b - 
//			a - 
//-----------------------------------------------------------------------------
void Draw_AlphaBox( const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a )
{

	// Build a rotation matrix from orientation
	matrix3x4_t fRotateMatrix;
	AngleMatrix(angles, fRotateMatrix);
		
	materialSystemInterface->Bind( g_materialTranslucentSingleColor );
	g_materialTranslucentSingleColor->ColorModulate( r*(1.0f/255.0f), g*(1.0f/255.0f), b*(1.0f/255.0f) );
	g_materialTranslucentSingleColor->AlphaModulate( a*(1.0f/255.0f) );
	g_materialTranslucentSingleColor->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, 0 );

	Vector verts[8];

	verts[0][0] = mins[0];
	verts[0][1] = mins[1];
	verts[0][2] = mins[2];
	
	verts[1][0] = maxs[0];
	verts[1][1] = mins[1];
	verts[1][2] = mins[2];
	
	verts[2][0] = maxs[0];
	verts[2][1] = maxs[1];
	verts[2][2] = mins[2];
	
	verts[3][0] = mins[0];
	verts[3][1] = maxs[1];
	verts[3][2] = mins[2];
	
	verts[4][0] = mins[0];
	verts[4][1] = mins[1];
	verts[4][2] = maxs[2];
	
	verts[5][0] = maxs[0];
	verts[5][1] = mins[1];
	verts[5][2] = maxs[2];
	
	verts[6][0] = maxs[0];
	verts[6][1] = maxs[1];
	verts[6][2] = maxs[2];
	
	verts[7][0] = mins[0];
	verts[7][1] = maxs[1];
	verts[7][2] = maxs[2];

	// Transform the verts
	Vector tverts[8];
	for (int i=0;i<8;i++)
	{
		VectorRotate( verts[i], fRotateMatrix, tverts[i]);
		VectorAdd(tverts[i],origin,tverts[i]);
	}

	DrawIndexedQuad( tverts, 0, 1, 2, 3 );
	DrawIndexedQuad( tverts, 7, 6, 5, 4 );
	DrawIndexedQuad( tverts, 1, 5, 6, 2 );
	DrawIndexedQuad( tverts, 4, 0, 3, 7 );
	DrawIndexedQuad( tverts, 3, 2, 6, 7 );
	DrawIndexedQuad( tverts, 4, 5, 1, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			dest - 
//			r - 
//			g - 
//			b - 
//			noDepthTest - 
//-----------------------------------------------------------------------------
void Draw_Line( const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest )
{
	materialSystemInterface->Bind( g_materialWireframe );
	g_materialWireframe->ColorModulate(	r * ( 1.0f/255.0f ), g * ( 1.0f/255.0f ), b * ( 1.0f/255.0f ) );
	g_materialWireframe->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, noDepthTest );

	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );
	meshBuilder.Position3fv( origin.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( dest.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draws a triangle
//-----------------------------------------------------------------------------
void Draw_Triangle( const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest )
{
	materialSystemInterface->Bind( g_materialTranslucentVertexColor );
	g_materialTranslucentVertexColor->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, noDepthTest );

	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	meshBuilder.Position3fv( p1.Base() );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( p2.Base() );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( p3.Base() );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}
#endif

