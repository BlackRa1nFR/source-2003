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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "FX_DiscreetLine.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"
#include "view.h"

/*
==================================================
CFXLine
==================================================
*/

CFXDiscreetLine::CFXDiscreetLine( const char *name, const Vector& start, const Vector& direction, 
	float velocity, float length, float clipLength, float scale, float life, const char *shader )
: CClientSideEffect( name )
{
	assert( materials );
	if ( materials == NULL )
		return;

	// Create a material...
	m_pMaterial = materials->FindMaterial( shader, NULL );
	m_pMaterial->IncrementReferenceCount();

	m_vecOrigin			= start;
	m_vecDirection		= direction;
	m_fVelocity			= velocity;
	m_fClipLength		= clipLength;
	m_fScale			= scale;
	m_fLife				= life;
	m_fStartTime		= 0.0f;
	m_fLength			= length;
}

CFXDiscreetLine::~CFXDiscreetLine( void )
{
	Destroy();
}

/*
==================================================
Draw
==================================================
*/

void CFXDiscreetLine::Draw( double frametime )
{
	Vector			lineDir, viewDir, cross;

	Vector			vecEnd, vecStart;
	Vector tmp;

	// Update the effect
	Update( frametime );

	// Calculate our distance along our path
	float	sDistance = m_fVelocity * m_fStartTime;
	float	eDistance = sDistance - m_fLength;
	
	//Clip to start
	sDistance = max( 0.0f, sDistance );
	eDistance = max( 0.0f, eDistance );

	if ( ( sDistance == 0.0f ) && ( eDistance == 0.0f ) )
		return;

	// Clip it
	if ( m_fClipLength != 0.0f )
	{
		sDistance = min( sDistance, m_fClipLength );
		eDistance = min( eDistance, m_fClipLength );
	}

	// Get our delta to calculate the tc offset
	float	dDistance	= fabs( sDistance - eDistance );
	float	dTotal		= ( m_fLength != 0.0f ) ? m_fLength : 0.01f;
	float	fOffset		= ( dDistance / dTotal );

	// Find our points along our path
	VectorMA( m_vecOrigin, sDistance, m_vecDirection, vecEnd );
	VectorMA( m_vecOrigin, eDistance, m_vecDirection, vecStart );

	//Setup our info for drawing the line
	VectorSubtract( vecEnd, vecStart, lineDir );
	VectorSubtract( vecEnd, CurrentViewOrigin(), viewDir );
	
	cross = lineDir.Cross( viewDir );
	VectorNormalize( cross );

	//Bind the material
	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_pMaterial );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	//FIXME: for now no coloration
	VectorMA( vecStart, -m_fScale, cross, tmp );	
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( vecStart,  m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( vecEnd, m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, fOffset );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( vecEnd, -m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, fOffset );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

/*
==================================================
IsActive
==================================================
*/

bool CFXDiscreetLine::IsActive( void )
{
	return ( m_fLife > 0.0 ) ? true : false;
}

/*
==================================================
Destroy
==================================================
*/

void CFXDiscreetLine::Destroy( void )
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

void CFXDiscreetLine::Update( double frametime )
{
	m_fStartTime += frametime;
	m_fLife -= frametime;

	//Move our end points
	//VectorMA( m_vecStart, frametime, m_vecStartVelocity, m_vecStart );
	//VectorMA( m_vecEnd, frametime, m_vecStartVelocity, m_vecEnd );
}

