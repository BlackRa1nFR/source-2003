//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( BEAMDRAW_H )
#define BEAMDRAW_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "vector.h"

#define NOISE_DIVISIONS		128

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

struct model_t;
struct BeamTrail_t;

//-----------------------------------------------------------------------------
// Purpose: Beams fill out this data structure
// This is also used for rendering
//-----------------------------------------------------------------------------

class Beam_t : public CDefaultClientRenderable
{
public:
	Beam_t();

	// Methods of IClientRenderable
	virtual const Vector&	GetRenderOrigin( void );
	virtual const QAngle&	GetRenderAngles( void );
	virtual void			GetRenderBounds( Vector& mins, Vector& maxs );
	virtual bool			ShouldCacheRenderInfo();
	virtual bool			ShouldDraw( void );
	virtual bool			IsTransparent( void );
	virtual int				DrawModel( int flags );
	virtual void			ComputeFxBlend( );
	virtual int				GetFxBlend( );
	virtual bool			LODTest();

	// Resets the beam state
	void			Reset();

	// Method to computing the bounding box
	void			ComputeBounds();

	// Bounding box...
	Vector			m_Mins;
	Vector			m_Maxs;
	ClientRenderHandle_t	m_Handle;

	// Data is below..

	// Next beam in list
	Beam_t*			next;

	// Type of beam
	int				type;
	int				flags;

	// Control points for the beam
	int				numAttachments;
	Vector			attachment[MAX_BEAM_ENTS];
	Vector			delta;

	// 0 .. 1 over lifetime of beam
	float			t;		
	float			freq;

	// Time when beam should die
	float			die;
	float			width;
	float			endWidth;
	float			fadeLength;
	float			amplitude;
	float			life;

	// Color
	float			r, g, b;
	float			brightness;

	// Speed
	float			speed;

	// Animation
	float			frameRate;
	float			frame;
	int				segments;

	// Attachment entities for the beam
	EHANDLE			entity[MAX_BEAM_ENTS];
	int				attachmentIndex[MAX_BEAM_ENTS];

	// Model info
	int				modelIndex;
	int				haloIndex;

	float			haloScale;
	int				frameCount;

	float			rgNoise[NOISE_DIVISIONS+1];

	// Popcorn trail for beam follows to use
	BeamTrail_t*	trail;

	// for TE_BEAMRINGPOINT
	float			start_radius;
	float			end_radius;

	// for FBEAM_ONLYNOISEONCE
	bool			m_bCalculatedNoise;
};

// ---------------------------------------------------------------- //
// CBeamSegDraw is a simple interface to beam rendering.
// ---------------------------------------------------------------- //
class CBeamSeg
{
public:
	Vector		m_vPos;
	Vector		m_vColor;
	float		m_flTexCoord;	// Y texture coordinate
	float		m_flWidth;
	float		m_flAlpha;
};

class CBeamSegDraw
{
public:
	// Pass null for pMaterial if you have already set the material you want.
	void			Start( int nSegs, IMaterial *pMaterial=0 );
	void			NextSeg( CBeamSeg *pSeg );
	void			End();


private:

	void			SpecifySeg( const Vector &vNextPos );
	void			ComputeNormal( const Vector &vStartPos, const Vector &vNextPos, Vector *pNormal );


private:
	CMeshBuilder	m_Mesh;

	Vector			m_vNormalLast;

	int				m_nTotalSegs;
	int				m_nSegsDrawn;
	
	CBeamSeg		m_Seg;
};



int ScreenTransform( const Vector& point, Vector& screen );

void DrawSegs( int noise_divisions, float *prgNoise, const model_t* spritemodel,
				float frame, int rendermode, const Vector& source, const Vector& delta, 
				float startWidth, float endWidth, float scale, float freq, float speed, int segments,
				int flags, float* color, float fadeLength );
void DrawTeslaSegs( int noise_divisions, float *prgNoise, const model_t* spritemodel,
				float frame, int rendermode, const Vector& source, const Vector& delta, 
				float startWidth, float endWidth, float scale, float freq, float speed, int segments,
				int flags, float* color, float fadeLength );
void DrawSplineSegs( int noise_divisions, float *prgNoise, 
				const model_t* beammodel, const model_t* halomodel, float flHaloScale,
				float frame, int rendermode, int numAttachments, Vector* attachment, 
				float startWidth, float endWidth, float scale, float freq, float speed, int segments,
				int flags, float* color, float fadeLength );
void BeamDrawHalo( const model_t* spritemodel, float frame, int rendermode, const Vector& source, 
				  float scale, float* color );
void DrawDisk( int noise_divisions, float *prgNoise, const model_t* spritemodel,
			  float frame, int rendermode, const Vector& source, const Vector& delta, 
			  float width, float scale, float freq, float speed, 
			  int segments, float* color );
void DrawCylinder( int noise_divisions, float *prgNoise, const model_t* spritemodel, 
				  float frame, int rendermode, const Vector& source, 
				  const Vector&  delta, float width, float scale, float freq, 
				  float speed, int segments, float* color );
void DrawRing( int noise_divisions, float *prgNoise, void (*pfnNoise)( float *noise, int divs, float scale ), 
			  const model_t* spritemodel, float frame, int rendermode, 
			  const Vector& source, const Vector& delta, float width, float amplitude, 
			  float freq, float speed, int segments, float* color );
void DrawBeamFollow( const model_t* spritemodel, BeamTrail_t* pHead, int frame, int rendermode, Vector& delta, 
					Vector& screen, Vector& screenLast, float die, const Vector& source, 
					int flags, float width, float amplitude, float freq, float* color );

//-----------------------------------------------------------------------------
// Assumes the material has already been bound
//-----------------------------------------------------------------------------
void DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color );

#endif // BEAMDRAW_H