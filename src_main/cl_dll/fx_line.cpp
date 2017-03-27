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
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "materialsystem/IMaterial.h"
#include "clientsideeffects.h"
#include "FX_Line.h"
#include "materialsystem/IMesh.h"
#include "view.h"


/*
==================================================
CFXLine
==================================================
*/

CFXLine::CFXLine( const char *name, Vector& start, Vector& end, Vector& start_vel, 
				 Vector& end_vel, float scale, float life, const char *shader, 
				 unsigned int flags )
: CFXStaticLine( name, start, end, scale, life, shader, flags )
{
	//Fill in the rest of the fields
	m_vecStartVelocity	= start_vel;
	m_vecEndVelocity	= end_vel;
}

CFXLine::~CFXLine( void )
{
	Destroy();
}

/*
==================================================
Draw
==================================================
*/

void CFXLine::Draw( double frametime )
{
	static color32 white = {255,255,255,255};

	// Update the effect
	Update( frametime );

	FX_DrawLine( m_vecStart, m_vecEnd, m_fScale, m_pMaterial, white );
}

/*
==================================================
IsActive
==================================================
*/

bool CFXLine::IsActive( void )
{
	return ( m_fLife > 0.0 ) ? true : false;
}

/*
==================================================
Destroy
==================================================
*/

void CFXLine::Destroy( void )
{
	//Release the material
	if ( m_pMaterial != NULL )
	{
		m_pMaterial->DecrementReferenceCount();
		m_pMaterial = NULL;
	}
}

/*
==================================================
Update
==================================================
*/

void CFXLine::Update( double frametime )
{
	m_fLife -= frametime;

	//Move our end points
	VectorMA( m_vecStart, frametime, m_vecStartVelocity, m_vecStart );
	VectorMA( m_vecEnd, frametime, m_vecStartVelocity, m_vecEnd );
}


void FX_DrawLine( const Vector &start, const Vector &end, float scale, IMaterial *pMaterial, const color32 &color )
{
	Vector			lineDir, viewDir;
	//Get the proper orientation for the line
	VectorSubtract( end, start, lineDir );
	VectorSubtract( end, CurrentViewOrigin(), viewDir );
	
	Vector cross = lineDir.Cross( viewDir );

	VectorNormalize( cross );

	//Bind the material
	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, pMaterial );
	CMeshBuilder meshBuilder;

	Vector			tmp;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	VectorMA( start, -scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( start, scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( end, scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( end, -scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

