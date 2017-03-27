//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "view.h"
#include "c_sun.h"
#include "particles_simple.h"
#include "clienteffectprecachesystem.h"

#include "tier0/vprof.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectGlow )
CLIENTEFFECT_MATERIAL( "sun/overlay" )
CLIENTEFFECT_REGISTER_END()

CUtlLinkedList<CGlowOverlay*, unsigned short> CGlowOverlay::s_GlowOverlays;
ConVar cl_ShowSunVectors( "cl_ShowSunVectors", "0", 0 );

float g_flGlowObstructionDecayPerSecond = .2;

// Dot product space the overlays are drawn in.
// Here it's setup to allow you to see it if you're looking within 40 degrees of the source.
float g_flOverlayRange = cos( DEG2RAD( 40 ) );


// ----------------------------------------------------------------------------- //
// ----------------------------------------------------------------------------- //

void Do2DRotation( Vector vIn, Vector &vOut, float flDegrees, int i1, int i2, int i3 )
{
	float c, s;
	SinCos( DEG2RAD( flDegrees ), &s, &c );

	vOut[i1] = vIn[i1]*c - vIn[i2]*s;
	vOut[i2] = vIn[i1]*s + vIn[i2]*c;
	vOut[i3] = vIn[i3];
}


// ----------------------------------------------------------------------------- //
// ----------------------------------------------------------------------------- //

CGlowOverlay::CGlowOverlay()
{
	m_ListIndex = 0xFFFF;
	m_nSprites = 0;
	m_flGlowObstructionScale = -1;
	m_bDirectional = false;
	m_bInSky = false;
	m_bActivated = false;

	//Init our sprites
	for ( int i = 0; i < MAX_SPRITES; i++ )
	{
		m_Sprites[i].m_vColor.Init();
		m_Sprites[i].m_flHorzSize	= 1.0f;
		m_Sprites[i].m_flVertSize	= 1.0f;
		m_Sprites[i].m_pMaterial	= NULL;
	}
}


CGlowOverlay::~CGlowOverlay()
{
	if( m_ListIndex != 0xFFFF )
		s_GlowOverlays.Remove( m_ListIndex );
}


bool CGlowOverlay::Update()
{
	return true;
}


void CGlowOverlay::UpdateGlowObstruction( const Vector &vToGlow )
{
	float flScale;

	// Trace a ray at the object.
	trace_t trace;
	UTIL_TraceLine( CurrentViewOrigin(), CurrentViewOrigin() + vToGlow*16000, 
		CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &trace );
	
	if( m_bInSky )
	{
		bool bHitSky = !(trace.fraction < 1 && !(trace.surface.flags & SURF_SKY));
		flScale = (float)bHitSky;
	}
	else
	{
		// If it's not in the sky, then we need a valid position or else we don't
		// know what's in front of it.
		assert( !m_bDirectional );

		bool bVisible = (m_vPos - CurrentViewOrigin()).LengthSqr() < (trace.endpos - CurrentViewOrigin()).LengthSqr();
		flScale = (float)bVisible;
	}

	if( m_flGlowObstructionScale == -1 )
	{
		m_flGlowObstructionScale = flScale;
	}
	else
	{
		if( flScale > m_flGlowObstructionScale )
		{
			m_flGlowObstructionScale += gpGlobals->frametime / g_flGlowObstructionDecayPerSecond;
			m_flGlowObstructionScale = min( m_flGlowObstructionScale, flScale );
		}
		else
		{
			m_flGlowObstructionScale -= gpGlobals->frametime / g_flGlowObstructionDecayPerSecond;
			m_flGlowObstructionScale = max( m_flGlowObstructionScale, flScale );
		}
	}
}

void CGlowOverlay::CalcSpriteColorAndSize( 
	float flDot,
	CGlowSprite *pSprite, 
	float *flHorzSize, 
	float *flVertSize, 
	Vector *vColor )
{
	// The overlay is largest and completely translucent at g_flOverlayRange.
	// When the dot product is 1, then it's smaller and more opaque.
	static float flSizeAtOverlayRangeMul = 150;
	static float flSizeAtOneMul = 70;
	
	static float flOpacityAtOverlayRange = 0;
	static float flOpacityAtOne = 1;

	// Figure out how big and how opaque it will be.
	*flHorzSize = RemapVal( 
		flDot, 
		g_flOverlayRange, 
		1, 
		flSizeAtOverlayRangeMul * pSprite->m_flHorzSize, 
		flSizeAtOneMul * pSprite->m_flHorzSize );		

	*flVertSize = RemapVal( 
		flDot, 
		g_flOverlayRange, 
		1, 
		flSizeAtOverlayRangeMul * pSprite->m_flVertSize, 
		flSizeAtOneMul * pSprite->m_flVertSize );		
	
	float flOpacity = RemapVal( 
		flDot, 
		g_flOverlayRange, 
		1, 
		flOpacityAtOverlayRange, 
		flOpacityAtOne );		

	flOpacity = flOpacity * m_flGlowObstructionScale;
	*vColor = pSprite->m_vColor * flOpacity;
}


void CGlowOverlay::CalcBasis( 
	const Vector &vToGlow,
	float flHorzSize,
	float flVertSize,
	Vector &vBasePt,
	Vector &vUp,
	Vector &vRight )
{
	static float flOverlayDist = 100;	
	vBasePt = CurrentViewOrigin() + vToGlow * flOverlayDist;
	
	vUp.Init( 0, 0, 1 );
	
	vRight = vToGlow.Cross( vUp );
	VectorNormalize( vRight );

	vUp = vRight.Cross( vToGlow );
	VectorNormalize( vUp );

	vRight *= flHorzSize;
	vUp *= flVertSize;
}


void CGlowOverlay::Draw()
{
	extern ConVar	r_drawsprites;
	if( !r_drawsprites.GetBool() )
	{
		return;
	}
	
	// Get the vector to the sun.
	Vector vToGlow;
	
	if( m_bDirectional )
		vToGlow = m_vDirection;
	else
		vToGlow = m_vPos - CurrentViewOrigin();

	VectorNormalize( vToGlow );

	float flDot = vToGlow.Dot( CurrentViewForward() );

	if( flDot <= g_flOverlayRange )
		return;

	UpdateGlowObstruction( vToGlow );
	if( m_flGlowObstructionScale == 0 )
		return;
	
	extern ConVar mat_wireframe;
	bool bWireframe = mat_wireframe.GetBool();
	
	for( int iSprite=0; iSprite < m_nSprites; iSprite++ )
	{
		CGlowSprite *pSprite = &m_Sprites[iSprite];

		// Figure out the color and size to draw it.
		float flHorzSize, flVertSize;
		Vector vColor;
		CalcSpriteColorAndSize( flDot, pSprite, &flHorzSize, &flVertSize, &vColor );
	
		// Setup the basis to draw the sprite.
		Vector vBasePt, vUp, vRight;
		CalcBasis( vToGlow, flHorzSize, flVertSize, vBasePt, vUp, vRight );

		// Get our material (deferred default load)
		if ( m_Sprites[iSprite].m_pMaterial == NULL )
		{
			m_Sprites[iSprite].m_pMaterial = materials->FindMaterial( "sun\\overlay", NULL );
		}

		// Draw the sprite.
		IMesh *pMesh = materials->GetDynamicMesh( false, 0, 0, m_Sprites[iSprite].m_pMaterial );

		CMeshBuilder builder;
		builder.Begin( pMesh, MATERIAL_QUADS, 1 );
		
		Vector vPt;
		
		vPt = vBasePt - vRight + vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 0, 1 );
		builder.AdvanceVertex();
		
		vPt = vBasePt + vRight + vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 1, 1 );
		builder.AdvanceVertex();
		
		vPt = vBasePt + vRight - vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 1, 0 );
		builder.AdvanceVertex();
		
		vPt = vBasePt - vRight - vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 0, 0 );
		builder.AdvanceVertex();
		
		builder.End( false, true );

		if( bWireframe )
		{
			IMaterial *pWireframeMaterial = materials->FindMaterial( "debug/debugwireframevertexcolor", NULL );
			materials->Bind( pWireframeMaterial );
			
			// Draw the sprite.
			IMesh *pMesh = materials->GetDynamicMesh( false, 0, 0, pWireframeMaterial );
			
			CMeshBuilder builder;
			builder.Begin( pMesh, MATERIAL_QUADS, 1 );
			
			Vector vPt;
			
			vPt = vBasePt - vRight + vUp;
			builder.Position3fv( vPt.Base() );
			builder.Color3f( 1.0f, 0.0f, 0.0f );
			builder.AdvanceVertex();
			
			vPt = vBasePt + vRight + vUp;
			builder.Position3fv( vPt.Base() );
			builder.Color3f( 1.0f, 0.0f, 0.0f );
			builder.AdvanceVertex();
			
			vPt = vBasePt + vRight - vUp;
			builder.Position3fv( vPt.Base() );
			builder.Color3f( 1.0f, 0.0f, 0.0f );
			builder.AdvanceVertex();
			
			vPt = vBasePt - vRight - vUp;
			builder.Position3fv( vPt.Base() );
			builder.Color3f( 1.0f, 0.0f, 0.0f );
			builder.AdvanceVertex();
			
			builder.End( false, true );
		}
	}
}


void CGlowOverlay::Activate()
{
	m_bActivated = true;
	if( m_ListIndex == 0xFFFF )
		m_ListIndex = s_GlowOverlays.AddToTail( this );
}


void CGlowOverlay::Deactivate()
{
	m_bActivated = false;
}


void CGlowOverlay::DrawOverlays()
{
	VPROF("CGlowOverlay::DrawOverlays()");

	unsigned short iNext;
	for( unsigned short i=s_GlowOverlays.Head(); i != s_GlowOverlays.InvalidIndex(); i = iNext )
	{
		iNext = s_GlowOverlays.Next( i );
		CGlowOverlay *pOverlay = s_GlowOverlays[i];
		
		if( !pOverlay->m_bActivated )
			continue;

		if( pOverlay->Update() )
		{
			pOverlay->Draw();
		}
		else
		{
			delete pOverlay;
		}
	}
}
