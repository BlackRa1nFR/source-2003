//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GLOW_OVERLAY_H
#define GLOW_OVERLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "utllinkedlist.h"
#include "materialsystem/imaterial.h"

extern float g_flOverlayRange;

class CGlowSprite
{
public:
	Vector				m_vColor;		// 0-1
	float				m_flHorzSize;	// Horizontal and vertical sizes.
	float				m_flVertSize;	// 1 = size of the sun
	IMaterial			*m_pMaterial;	// Material to use
};


class CGlowOverlay
{
public:

					CGlowOverlay();
	virtual			~CGlowOverlay();

	// Return false to remove (and delete) the overlay.
	virtual bool	Update();


// Data the creator should fill in.
public:

	enum
	{
		MAX_SPRITES = 4
	};
	
	// Position of the light source. If m_bDirectional is set, then it ignores
	// this and uses m_vDirection as the direction of the light source.
	Vector		m_vPos;

	// Optional direction. Used for really far away things like the sun.
	// The direction should point AT the light source.
	bool		m_bDirectional;
	Vector		m_vDirection;

	// If this is set, then the overlay is only visible if the ray to it hits the sky.
	bool		m_bInSky;

	CGlowSprite	m_Sprites[MAX_SPRITES];
	int			m_nSprites;


public:

	// After creating the overlay, call this to add it to the active list.
	// You can also call Activate and Deactivate as many times as you want.
	void			Activate();
	void			Deactivate();
	
	// Render all the active overlays.
	static void		DrawOverlays();

protected:

	void			UpdateGlowObstruction( const Vector &vToGlow );

	virtual void	CalcSpriteColorAndSize( 
		float flDot,
		CGlowSprite *pSprite, 
		float *flHorzSize, 
		float *flVertSize, 
		Vector *vColor );

	virtual void	CalcBasis( 
		const Vector &vToGlow,
		float flHorzSize,
		float flVertSize,
		Vector &vBasePt,
		Vector &vUp,
		Vector &vRight );

	virtual void	Draw();
	
	float			m_flGlowObstructionScale;	

private:
	static CUtlLinkedList<CGlowOverlay*, unsigned short>	s_GlowOverlays;

	bool			m_bActivated;
	unsigned short	m_ListIndex; // index into s_GlowOverlays.
};

//Override for warping
class CWarpOverlay : public CGlowOverlay
{
protected:
	
	virtual void Draw( void );
};

#endif // GLOW_OVERLAY_H
