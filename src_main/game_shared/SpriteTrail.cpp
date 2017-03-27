//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "SpriteTrail.h"

#define	MAX_SPRITE_TRAIL_POINTS	32

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CSpriteTrail::CSpriteTrail( void )
{
#if defined( CLIENT_DLL )
	m_vecSteps.EnsureCapacity( MAX_SPRITE_TRAIL_POINTS );
#endif
}

#if defined( CLIENT_DLL )

#include "clientsideeffects.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMesh.h"
#include "VMatrix.h"
#include "view.h"
#include "beamdraw.h"
#include "enginesprite.h"

#define SCREEN_SPACE_TRAILS 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSpriteTrail::UpdateTrail( void )
{
	// Can't update too quickly
	if ( m_flUpdateTime > gpGlobals->curtime )
		return;

#if SCREEN_SPACE_TRAILS

	VMatrix	viewMatrix;
	materials->GetMatrix( MATERIAL_VIEW, &viewMatrix );

	Vector	screenPos = viewMatrix * GetRenderOrigin();

#else

	Vector	screenPos = GetRenderOrigin();

#endif

	trailPoint_t	point;

	int	count = m_vecSteps.Count();

	trailPoint_t	*pLast = NULL;
	
	if ( count )
	{
		pLast = &m_vecSteps[ count-1 ];
	}

	if ( ( pLast == NULL ) || ( pLast->screenPos - screenPos ).Length() > 2 )
	{
		// Save off its screen position, not its world position
		point.screenPos = screenPos;
		point.dieTime	= m_flLifeTime;

		// If we're over our limit, steal the last point and put it up front
		if ( m_vecSteps.Count() >= MAX_SPRITE_TRAIL_POINTS )
		{
			m_vecSteps.Remove( 0 );
		}
		
		//FIXME: This is non-optimal
		m_vecSteps.AddToTail( point );
	}

	// Don't update again for a bit
	m_flUpdateTime = gpGlobals->curtime + ( m_flLifeTime / (float) MAX_SPRITE_TRAIL_POINTS );
}

extern CEngineSprite *Draw_SetSpriteTexture( const model_t *pSpriteModel, int frame, int rendermode );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSpriteTrail::DrawModel( int flags )
{
	if ( gpGlobals->frametime <= 0.0f )
		return 0;

	UpdateTrail();

	trailPoint_t	*point, *nextPoint;
	Vector			start, end;

	int iSize = m_vecSteps.Count();

	//Must have at least two points
	if ( iSize < 2 )
		return 1;

#if SCREEN_SPACE_TRAILS
	VMatrix	viewMatrix;
	materials->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	viewMatrix = viewMatrix.InverseTR();
#endif

	CEngineSprite *pSprite = Draw_SetSpriteTexture( GetModel(), m_flFrame, m_nRenderMode );

	if ( pSprite == NULL )
		return 0;

	// Specify all the segments.
	CBeamSegDraw segDraw;
	segDraw.Start( iSize, pSprite->GetMaterial() );
	
	//FIXME: Setup the first point, always eminating from the attachment point

	for ( int i = iSize-1; i >= 0; i-- )
	{
		point = &m_vecSteps[i];
		
		point->dieTime -= gpGlobals->frametime;

		// See if we're done with
		if ( point->dieTime <= 0.0f )
		{
			//FIXME: Push this back onto the top for use
			m_vecSteps.Remove( i );
		}

		float scalePerc = point->dieTime / m_flLifeTime;

		// See if there's another point after this, link them
		if ( i != 0 )
		{
			nextPoint = &m_vecSteps[i-1];

			CBeamSeg curSeg;

			//FIXME: Interpolate
	
#if SCREEN_SPACE_TRAILS
			start	= viewMatrix * point->screenPos;
			end		= viewMatrix * nextPoint->screenPos;
#else
			start	= point->screenPos;
			end		= nextPoint->screenPos;
#endif

			curSeg.m_vColor.x = (float) m_clrRender->r / 255.0f;
			curSeg.m_vColor.y = (float) m_clrRender->g / 255.0f;
			curSeg.m_vColor.z = (float) m_clrRender->b / 255.0f;
			curSeg.m_flAlpha  = ( (float) m_clrRender->a / 255.0f ) * scalePerc;

			curSeg.m_vPos		= start;
			curSeg.m_flWidth	= GetRenderScale() * scalePerc;
			curSeg.m_flTexCoord = 1.0f;	//FIXME: Real?

			segDraw.NextSeg( &curSeg );
		}
	}

	segDraw.End();

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector const&
//-----------------------------------------------------------------------------
Vector const &CSpriteTrail::GetRenderOrigin( void )
{
	static Vector vOrigin;
	vOrigin = GetAbsOrigin();

	if ( m_hAttachedToEntity )
	{
		C_BaseEntity *ent = m_hAttachedToEntity->GetBaseEntity();
		if ( ent )
		{
			QAngle dummyAngles;
			ent->GetAttachment( m_nAttachment, vOrigin, dummyAngles );
		}
	}

	return vOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSpriteTrail::ShouldDraw( void )
{
	return true;
}

#endif	//CLIENT_DLL

LINK_ENTITY_TO_CLASS( env_spritetrail, CSpriteTrail );

IMPLEMENT_NETWORKCLASS_ALIASED( SpriteTrail, DT_SpriteTrail );

BEGIN_NETWORK_TABLE( CSpriteTrail, DT_SpriteTrail )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO(m_flLifeTime),		0,	SPROP_NOSCALE ),
#else
	RecvPropFloat( RECVINFO(m_flLifeTime)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CSpriteTrail )
END_PREDICTION_DATA()

#if !defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//			animate - 
// Output : CSpriteTrail
//-----------------------------------------------------------------------------
CSpriteTrail *CSpriteTrail::SpriteTrailCreate( const char *pSpriteName, const Vector &origin, bool animate )
{
	CSpriteTrail *pSprite = CREATE_ENTITY( CSpriteTrail, "env_spritetrail" );

	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->SetSolid( SOLID_NONE );
	pSprite->SetMoveType( MOVETYPE_NOCLIP );
	
	UTIL_SetSize( pSprite, vec3_origin, vec3_origin );

	if ( animate )
		pSprite->TurnOn();

	return pSprite;
}

#endif	//CLIENT_DLL == false