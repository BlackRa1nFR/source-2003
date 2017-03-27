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

#include "studio.h"
#include "imageloader.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "mathlib.h"
#include "cstudiorender.h"
#include "pixelwriter.h"
#include "vtf/vtf.h"

#define GLINT_SUPERSAMPLE_COUNT 2
#define GLINT_SUPERSAMPLE_COUNT_SQ (GLINT_SUPERSAMPLE_COUNT*GLINT_SUPERSAMPLE_COUNT)

void CStudioRender::SetEyeViewTarget( const Vector& viewtarget )
{
	VectorCopy( viewtarget, m_ViewTarget );
	// Con_DPrintf("view %.2f %.2f %.2f\n", g_StudioInternalState.viewtarget[0], g_StudioInternalState.viewtarget[1], g_StudioInternalState.viewtarget[2] );
}



void CStudioRender::SetFlexWeights( int numweights, const float *weights )
{
	for (int i = 0; i < numweights && i < MAXSTUDIOFLEXDESC; i++)
	{
		m_FlexWeights[i] = weights[i];
	}
}




#define sign( a ) (((a) < 0) ? -1 : (((a) > 0) ? 1 : 0 ))

void CStudioRender::R_StudioEyeballPosition( const mstudioeyeball_t *peyeball, eyeballstate_t *pstate )
{
	// Vector  forward;
	// Vector  org, right, up;

	pstate->peyeball = peyeball;

	Vector tmp;
	// move eyeball into worldspace
	{
		// Con_DPrintf("%.2f %.2f %.2f\n", peyeball->org[0], peyeball->org[1], peyeball->org[2] );

		VectorCopy( peyeball->org, tmp );
		tmp[0] += m_Config.fEyeShiftX * sign( tmp[0] );
		tmp[1] += m_Config.fEyeShiftY * sign( tmp[1] );
		tmp[2] += m_Config.fEyeShiftZ * sign( tmp[2] );
	}
	VectorTransform( tmp, m_BoneToWorld[peyeball->bone], pstate->org );
	VectorRotate( peyeball->up, m_BoneToWorld[peyeball->bone], pstate->up );

	// look directly at target
	VectorSubtract( m_ViewTarget, pstate->org, pstate->forward );
	VectorNormalize( pstate->forward );

	if (!m_Config.bEyeMove)
	{
		VectorRotate( peyeball->forward, m_BoneToWorld[peyeball->bone], pstate->forward );
		VectorScale( pstate->forward, -1 ,pstate->forward ); // ???
	}

	CrossProduct( pstate->forward, pstate->up, pstate->right );
	VectorNormalize( pstate->right );

	// shift N degrees off of the target
	float dz;
	dz = peyeball->zoffset;

	VectorMA( pstate->forward, peyeball->zoffset + dz, pstate->right, pstate->forward );

#if 0
	// add random jitter
	VectorMA( forward, RandomFloat( -0.02, 0.02 ), right, forward );
	VectorMA( forward, RandomFloat( -0.02, 0.02 ), up, forward );
#endif

	VectorNormalize( pstate->forward );
	// re-aim eyes 
	CrossProduct( pstate->forward, pstate->up, pstate->right );
	VectorNormalize( pstate->right );

	CrossProduct( pstate->right, pstate->forward, pstate->up );
	VectorNormalize( pstate->up );

	float scale = (1.0 / peyeball->iris_scale) + m_Config.fEyeSize;

	if (scale > 0)
		scale = 1.0 / scale;

	if (1)
	{
		Vector  headup;
		Vector  headforward;
		Vector	pos;

		float upperlid = DEG2RAD( 9.5 );
		float lowerlid = DEG2RAD( -26.4 );

		// get weighted position of eyeball angles based on the "raiser", "neutral", and "lowerer" controls
		upperlid = m_FlexWeights[peyeball->upperflexdesc[0]] * asin( peyeball->uppertarget[0] / peyeball->radius );
		upperlid += m_FlexWeights[peyeball->upperflexdesc[1]] * asin( peyeball->uppertarget[1] / peyeball->radius );
		upperlid += m_FlexWeights[peyeball->upperflexdesc[2]] * asin( peyeball->uppertarget[2] / peyeball->radius );

		lowerlid = m_FlexWeights[peyeball->lowerflexdesc[0]] * asin( peyeball->lowertarget[0] / peyeball->radius );
		lowerlid += m_FlexWeights[peyeball->lowerflexdesc[1]] * asin( peyeball->lowertarget[1] / peyeball->radius );
		lowerlid += m_FlexWeights[peyeball->lowerflexdesc[2]] * asin( peyeball->lowertarget[2] / peyeball->radius );

		// Con_DPrintf("%.1f %.1f\n", RAD2DEG( upperlid ), RAD2DEG( lowerlid ) );		

		float sinupper, cosupper, sinlower, coslower;
		SinCos( upperlid, &sinupper, &cosupper );
		SinCos( lowerlid, &sinlower, &coslower );

		// convert to head relative space
		VectorIRotate( pstate->up, m_BoneToWorld[peyeball->bone], headup );
		VectorIRotate( pstate->forward, m_BoneToWorld[peyeball->bone], headforward );

		// upper lid
		VectorScale( headup, sinupper * peyeball->radius, pos );
		VectorMA( pos, cosupper * peyeball->radius, headforward, pos );
		m_FlexWeights[peyeball->upperlidflexdesc] = DotProduct( pos, peyeball->up );

		// lower lid
		VectorScale( headup, sinlower * peyeball->radius, pos );
		VectorMA( pos, coslower * peyeball->radius, headforward, pos );
		m_FlexWeights[peyeball->lowerlidflexdesc] = DotProduct( pos, peyeball->up );
	}
	// Con_DPrintf("%.4f %.4f\n", m_FlexWeights[peyeball->upperlidflex], m_FlexWeights[peyeball->lowerlidflex] );

	VectorScale( &pstate->right[0], -scale, pstate->mat[0] );
	VectorScale( &pstate->up[0], -scale, pstate->mat[1] );

	pstate->mat[0][3] = -DotProduct( &pstate->org[0], pstate->mat[0] ) + 0.5f;
	pstate->mat[1][3] = -DotProduct( &pstate->org[0], pstate->mat[1] ) + 0.5f;

	// FIXME: push out vertices for cornea
}


void CStudioRender::MaterialPlanerProjection( const matrix3x4_t& mat, int count, const Vector *psrcverts, Vector2D *pdesttexcoords )
{
	for (int i = 0; i < count; i++)
	{
		pdesttexcoords[i][0] = DotProduct( &psrcverts[i].x, mat[0] ) + mat[0][3];
		pdesttexcoords[i][1] = DotProduct( &psrcverts[i].x, mat[1] ) + mat[1][3];
	}
}


//-----------------------------------------------------------------------------
// Setup the flex verts for this rendering
//-----------------------------------------------------------------------------
void CStudioRender::R_StudioFlexVerts( mstudiomesh_t *pmesh )
{
	assert( pmesh );

	// There's a chance we can actually do the flex twice on a single mesh
	// since there's flexed HW + SW portions of the mesh.
	if (m_VertexCache.IsFlexComputationDone())
		return;

	// get pointers to geometry
	mstudiovertex_t *pVertices	= pmesh->Vertex(0);
	Vector4D *pstudiotangentS	= pmesh->TangentS( 0 );

	mstudioflex_t	*pflex = pmesh->pFlex( 0 );
	
	m_VertexCache.SetupComputation( pmesh, true );

	// apply flex weights
	int i, j, n;

	for (i = 0; i < pmesh->numflexes; i++)
	{
		float w = m_FlexWeights[pflex[i].flexdesc];

		if (w <= pflex[i].target0 || w >= pflex[i].target3)
		{
			// value outside of range
			continue;
		}
		else if (w < pflex[i].target1)
		{
			// 0 to 1 ramp
			w = (w - pflex[i].target0) / (pflex[i].target1 - pflex[i].target0);
		}
		else if (w > pflex[i].target2)
		{
			// 1 to 0 ramp
			w = (pflex[i].target3 - w) / (pflex[i].target3 - pflex[i].target2);
		}
		else
		{
			// plat
			w = 1.0;
		}

		if (w > -0.001 && w < 0.001)
			continue;

		mstudiovertanim_t *pvanim = pflex[i].pVertanim( 0 );

		for (j = 0; j < pflex[i].numverts; j++)
		{
			n = pvanim[j].index;
			mstudiovertex_t &vert = pVertices[n];

			// only flex the indicies that are (still) part of this mesh
			// if (n < (int)pmesh->numvertices)
			if (1) // need LOD info here
			{
				CachedVertex_t* pFlexedVertex;
				if (!m_VertexCache.IsVertexFlexed(n))
				{
					// Add a new flexed vert to the flexed vertex list
					pFlexedVertex = m_VertexCache.CreateFlexVertex(n);

					VectorCopy( vert.m_vecPosition, pFlexedVertex->m_Position );
					VectorCopy( vert.m_vecNormal, pFlexedVertex->m_Normal );
					Vector4DCopy( pstudiotangentS[n], pFlexedVertex->m_TangentS );
					Assert( pFlexedVertex->m_TangentS.w == -1.0f || pFlexedVertex->m_TangentS.w == 1.0f );
				}
				else
				{
					pFlexedVertex = m_VertexCache.GetFlexVertex(n);
				}

				Vector nDeltaScale;
				VectorMultiply( pvanim[j].ndelta, w, nDeltaScale );

				VectorMA( pFlexedVertex->m_Position, w, pvanim[j].delta, pFlexedVertex->m_Position );
				pFlexedVertex->m_Normal += nDeltaScale;
				pFlexedVertex->m_TangentS.AsVector3D() += nDeltaScale;
				Assert( pFlexedVertex->m_TangentS.w == -1.0f || pFlexedVertex->m_TangentS.w == 1.0f );
			}
		}
	}
}


// REMOVED!!  Look in version 32 if you need it.
//static void R_StudioEyeballNormals( const mstudioeyeball_t *peyeball, int count, const Vector *psrcverts, Vector *pdestnorms )


// FIXME: The material system really needs to have a texel/pixel writer for textures that
// accepts values in lnear space and deals with format conversion, etc.
void CStudioRender::R_SetPixel( CPixelWriter& pixelWriter, float* pLinearTextureData, 
							    const Vector& color, float alpha )
{
	if (alpha < 0)
		return;

	pLinearTextureData[ 0 ] += color[0] * alpha;
	pLinearTextureData[ 1 ] += color[1] * alpha;
	pLinearTextureData[ 2 ] += color[2] * alpha;

	pixelWriter.WritePixel( LinearToTexture(pLinearTextureData[0]), 
		LinearToTexture(pLinearTextureData[1]), LinearToTexture(pLinearTextureData[2]) );
}


void CStudioRender::R_BoxPixel( CPixelWriter& pixelWriter, float x, float y, const Vector& color )
{
	int j;

	// NOTE: x is expected to appear in the range -.5 to .5
	x = (x + 0.5) * m_GlintWidth;
	y = (y + 0.5) * m_GlintHeight; 

	int w = (x + 0.5);
	int h = (y + 0.5); 

	float dw = (w - x);
	float dh = (h - y);

	if (w > 0 && w < m_GlintWidth && h > 0 && h < m_GlintHeight)
	{
		pixelWriter.Seek( w-1, h-1 );
		j = ((w - 1) + (h - 1) * m_GlintWidth) * 3;
		R_SetPixel( pixelWriter, &m_pGlintLinearScratch[ j ], color, (0.5 + dw) * (0.5 + dh) - 0.25 );
		R_SetPixel( pixelWriter, &m_pGlintLinearScratch[ j + 3 ], color, (0.5 - dw) * (0.5 + dh) - 0.25 );

		pixelWriter.Seek( w-1, h );
		j = ((w - 1) + h * m_GlintWidth) * 3;
		R_SetPixel( pixelWriter, &m_pGlintLinearScratch[ j ], color, (0.5 + dw) * (0.5 - dh) - 0.25 );
		R_SetPixel( pixelWriter, &m_pGlintLinearScratch[ j + 3 ], color, (0.5 - dw) * (0.5 - dh) - 0.25 );
	}
}


#define KERNEL_DIAMETER 2
#define KERNEL_TEXELS (GLINT_SUPERSAMPLE_COUNT * KERNEL_DIAMETER)
#define KERNEL_TEXEL_RADIUS (KERNEL_TEXELS / 2)

void CStudioRender::AddGlint( CPixelWriter &pixelWriter, float x, float y, const Vector& color )
{
	// NOTE: x,y is expected to appear in the range -.5 to .5

	float flWidth = GLINT_SUPERSAMPLE_COUNT * m_GlintWidth;
	float flHeight = GLINT_SUPERSAMPLE_COUNT * m_GlintHeight;
	
	x = (x + 0.5f) * flWidth;
	y = (y + 0.5f) * flHeight;

	int x0 = (int)x;
	int y0 = (int)y;
	int x1 = x0 + KERNEL_TEXEL_RADIUS;
	int y1 = y0 + KERNEL_TEXEL_RADIUS;
	x0 -= KERNEL_TEXEL_RADIUS;
	y0 -= KERNEL_TEXEL_RADIUS;

	if ( (x0 >= flWidth) || (x1 < 0) || (y0 >= flHeight) || (y1 < 0) )
		return;

	// Compute the intensity kernel...
	float ppIntensity[KERNEL_TEXELS + 1][KERNEL_TEXELS + 1];
	float flTotalIntensity = 0.0f;

	int xlow = x0;
	int ylow = y0;
	float cx = x - xlow;
	float cy = y - ylow;

	int i, j;
	for ( i = 0; i <= KERNEL_TEXELS; ++i )
	{
		float dy = i - cy;
		for ( j = 0; j <= KERNEL_TEXELS; ++j )
		{
			float dx = j - cx;
			float r2 = (dx * dx + dy * dy) / (KERNEL_TEXEL_RADIUS * KERNEL_TEXEL_RADIUS);
			if (r2 <= 1.0f)
			{
				ppIntensity[i][j] = exp( -(25 * r2) );
				flTotalIntensity += ppIntensity[i][j];
			}
			else
			{
				ppIntensity[i][j] = 0;
			}
			flTotalIntensity += ppIntensity[i][j];
		}
	}

	// Renormalize so we always have the same total intensity...
	flTotalIntensity = 1.0f / flTotalIntensity;
 	for ( i = 0; i <= KERNEL_TEXELS; ++i )
	{
		for ( j = 0; j <= KERNEL_TEXELS; ++j )
		{
			ppIntensity[i][j] *= flTotalIntensity;
		}
	}

	// Apply the intensity to the super-sampled pixels
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 >= flWidth) 
		x1 = flWidth - 1;
	if (y1 >= flHeight) 
		y1 = flHeight - 1;

	for ( i = y0; i <= y1; ++i )
	{
		int nRowIdx = i * flWidth * 4;
		for ( j = x0; j <= x1; ++j )
		{
			float flIntensity = ppIntensity[i - ylow][j - xlow];
			int nIndex = nRowIdx + j * 4;
			m_pGlintLinearScratch[nIndex + 0] += color[0] * flIntensity;
			m_pGlintLinearScratch[nIndex + 1] += color[1] * flIntensity;
			m_pGlintLinearScratch[nIndex + 2] += color[2] * flIntensity;
		}
	}

	// Lastly, apply the super-sampled pixels to the actual texture
	int u0 = (x0 / GLINT_SUPERSAMPLE_COUNT);
	int v0 = (y0 / GLINT_SUPERSAMPLE_COUNT);
	int u1 = (x1 / GLINT_SUPERSAMPLE_COUNT);
	int v1 = (y1 / GLINT_SUPERSAMPLE_COUNT);
	for (int v = v0; v <= v1; ++v )
	{
		pixelWriter.Seek( u0, v );

		for (int u = u0; u <= u1; ++u )
		{
			float color[3] = { 0, 0, 0 };

			int ix = u * GLINT_SUPERSAMPLE_COUNT;
			int iy = v * GLINT_SUPERSAMPLE_COUNT;

			for (int i = 0; i < GLINT_SUPERSAMPLE_COUNT; ++i )
			{
				for (int j = 0; j < GLINT_SUPERSAMPLE_COUNT; ++j )
				{
					int nIndex = ( (iy + i) * flWidth * 4 ) + ( (ix + j) * 4 );
					color[0] += m_pGlintLinearScratch[nIndex + 0];
					color[1] += m_pGlintLinearScratch[nIndex + 1];
					color[2] += m_pGlintLinearScratch[nIndex + 2];
				}
			}

			pixelWriter.WritePixel( LinearToTexture(color[0]), LinearToTexture(color[1]), LinearToTexture(color[2]) );
		}
	}
}


//-----------------------------------------------------------------------------
// glint
//-----------------------------------------------------------------------------
class CGlintTextureRegenerator : public ITextureRegenerator
{
public:
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
	{
		// We don't need to reconstitute the bits after a task switch
		// since we reconstitute them every frame they are used anyways
		if (!m_pStudioRender)
			return;

		if ( (!m_pStudioRender->m_pGlintLinearScratch) ||
			(m_pStudioRender->m_GlintWidth != pVTFTexture->Width()) || 
			(m_pStudioRender->m_GlintHeight != pVTFTexture->Height()) )
		{
			if (m_pStudioRender->m_pGlintLinearScratch)
				delete[] m_pStudioRender->m_pGlintLinearScratch;

			m_pStudioRender->m_GlintWidth = pVTFTexture->Width();
			m_pStudioRender->m_GlintHeight = pVTFTexture->Height();

			// supersampling
			m_pStudioRender->m_pGlintLinearScratch = new float[GLINT_SUPERSAMPLE_COUNT_SQ * pVTFTexture->Width() * pVTFTexture->Height() * 4];
		}

		// NOTE: See version 25 for lots of #if 0ed out stuff I removed
		Vector viewdelta;
		VectorSubtract( *m_pROrigin, m_pState->org, viewdelta );
		VectorNormalize( viewdelta );

		// hack cornea position
		float iris_radius = m_pState->peyeball->radius * (6.0 / 12.0);
		float cornea_radius = m_pState->peyeball->radius * (8.0 / 12.0);

		Vector cornea;
		// position on eyeball that matches iris radius
		float er = (iris_radius / m_pState->peyeball->radius);
		er = FastSqrt( 1 - er * er );

		// position on cornea sphere that matches iris radius
		float cr = (iris_radius / cornea_radius);
		cr = FastSqrt( 1 - cr * cr );

		float r = (er * m_pState->peyeball->radius - cr * cornea_radius);
		VectorScale( m_pState->forward, r, cornea );

		// get offset for center of cornea
		float dx, dy;
		dx = DotProduct( *m_pVRight, cornea );
		dy = DotProduct( *m_pVUp, cornea );

		// move cornea to world space
		VectorAdd( cornea, m_pState->org, cornea );

		Vector delta, intensity;
		Vector reflection, coord;

		// clear glint texture
		unsigned char *pTextureData = pVTFTexture->ImageData( 0, 0, 0 );
		int nImageSize = pVTFTexture->ComputeMipSize( 0 );
		memset( pTextureData, 0, nImageSize );
		memset( m_pStudioRender->m_pGlintLinearScratch, 0, GLINT_SUPERSAMPLE_COUNT_SQ * pVTFTexture->Width() * pVTFTexture->Height() * sizeof(float) * 4 );

		CPixelWriter pixelWriter;
		pixelWriter.SetPixelMemory( pVTFTexture->Format(), pTextureData, pVTFTexture->RowSizeInBytes( 0 ) );

		int i = 0;
		while (m_pStudioRender->R_LightGlintPosition( i, cornea, delta, intensity ))
		{
			VectorNormalize( delta );
			if (DotProduct( delta, m_pState->forward ) > 0)
			{
				VectorAdd( delta, viewdelta, reflection );
				VectorNormalize( reflection );

				coord[0] = dx + cornea_radius * DotProduct( *m_pVRight, reflection );
				coord[1] = dy + cornea_radius * DotProduct( *m_pVUp, reflection );

				// NOTE: AddGlint is a more expensive solution but it looks better close-up
				m_pStudioRender->AddGlint( pixelWriter, coord[0], coord[1], intensity );
//				m_pStudioRender->R_BoxPixel( pixelWriter, coord[0], coord[1], intensity );

				if (m_pStudioRender->R_LightGlintPosition( i, m_pState->org, delta, intensity ))
				{
					VectorNormalize( delta );
					if (DotProduct( delta, m_pState->forward ) < er)
					{
						coord[0] = m_pState->peyeball->radius * DotProduct( *m_pVRight, reflection );
						coord[1] = m_pState->peyeball->radius * DotProduct( *m_pVUp, reflection );

						// NOTE: AddGlint is a more expensive solution but it looks better close-up
						m_pStudioRender->AddGlint( pixelWriter, coord[0], coord[1], intensity );
//						m_pStudioRender->R_BoxPixel( pixelWriter, coord[0], coord[1], intensity );
					}
				}
			}
			i++;
		}
	}

	// We've got a global instance, no need to delete it
	virtual void Release() {}

	const eyeballstate_t *m_pState;
	const Vector *m_pVRight;
	const Vector *m_pVUp;
	const Vector *m_pROrigin;
	CStudioRender *m_pStudioRender;
};

static CGlintTextureRegenerator s_GlintTextureRegen;
 
void CStudioRender::PrecacheGlint()
{
	if (!m_pGlintTexture)
	{		
		// Get the texture that we are going to be updating procedurally.
		m_pGlintTexture = m_pMaterialSystem->CreateProceduralTexture( 
			"glint", 32, 32, IMAGE_FORMAT_BGRA8888, TEXTUREFLAGS_NOMIP );
		m_pGlintTexture->SetTextureRegenerator( &s_GlintTextureRegen );
	}
}

void CStudioRender::UncacheGlint()
{
	if (m_pGlintLinearScratch)
	{
		delete[] m_pGlintLinearScratch;
		m_pGlintLinearScratch = NULL;
	}

	if (m_pGlintTexture)
	{
		m_pGlintTexture->SetTextureRegenerator( NULL );
		m_pGlintTexture->DecrementReferenceCount();
		m_pGlintTexture = NULL;
	}
}

void CStudioRender::R_StudioEyeballGlint( const eyeballstate_t *pstate, IMaterialVar *pGlintVar, 
							const Vector& vright, const Vector& vup, const Vector& r_origin )
{
	// Get the glint material
	PrecacheGlint();

	// Set up the texture regenerator
	s_GlintTextureRegen.m_pVRight = &vright;
	s_GlintTextureRegen.m_pVUp = &vup;
	s_GlintTextureRegen.m_pROrigin = &r_origin;
	s_GlintTextureRegen.m_pState = pstate;
	s_GlintTextureRegen.m_pStudioRender = this;

	// This will cause the glint texture to be re-generated and then downloaded
	m_pGlintTexture->Download( );

	// This is necessary to make sure we don't reconstitute the bits
	// after coming back from a task switch
	s_GlintTextureRegen.m_pStudioRender = false;

	// NOTE: Have to bind *after* the redownload, because this ensures
	// we get the correct copy of the glint texture
	pGlintVar->SetTextureValue( m_pGlintTexture );
}

void CStudioRender::ComputeGlintTextureProjection( eyeballstate_t const* pState, 
							const Vector& vright, const Vector& vup, matrix3x4_t& mat )
{
	// project eyeball into screenspace texture
	float scale = 1.0 / (pState->peyeball->radius * 2);
	VectorScale( &vright.x, scale, mat[0] );
	VectorScale( &vup.x, scale, mat[1] );

	mat[0][3] = -DotProduct( pState->org.Base(), mat[0] ) + 0.5;
	mat[1][3] = -DotProduct( pState->org.Base(), mat[1] ) + 0.5;
}


/*
void R_MouthLighting( int count, const Vector *psrcverts, const Vector *psrcnorms, Vector4D *pdestlightvalues )
{
	Vector forward;

	if (m_pStudioHdr->nummouths < 1) return;

	mstudiomouth_t *pMouth = r_pstudiohdr->pMouth( 0 ); // FIXME: this needs to get the mouth index from the shader

	float fIllum = m_FlexWeights[pMouth->flexdesc];
	if (fIllum < 0) fIllum = 0;
	if (fIllum > 1) fIllum = 1;
	fIllum = LinearToTexture( fIllum ) / 255.0;


	VectorRotate( pMouth->forward, g_StudioInternalState.boneToWorld[ pMouth->bone ], forward );
	
	for (int i = 0; i < count; i++)
	{
		float dot = -DotProduct( psrcnorms[i], forward );
		if (dot > 0)
		{
			dot = LinearToTexture( dot ) / 255.0; // FIXME: this isn't robust
			VectorScale( pdestlightvalues[i], dot, pdestlightvalues[i] );
		}
		else
			VectorFill( pdestlightvalues[i], 0 );

		VectorScale( pdestlightvalues[i], fIllum, pdestlightvalues[i] );
	}
}
*/

void CStudioRender::R_MouthComputeLightingValues( float& fIllum, Vector& forward )
{
	// FIXME: this needs to get the mouth index from the shader
	mstudiomouth_t *pMouth = m_pStudioHdr->pMouth( 0 ); 

	fIllum = m_FlexWeights[pMouth->flexdesc];
	if (fIllum < 0) fIllum = 0;
	if (fIllum > 1) fIllum = 1;
	fIllum = LinearToTexture( fIllum ) / 255.0;

	VectorRotate( pMouth->forward, m_BoneToWorld[ pMouth->bone ], forward );
}

void CStudioRender::R_MouthLighting( float fIllum, const Vector& normal, const Vector& forward, Vector& light )
{
	float dot = -DotProduct( normal, forward );
	if (dot > 0)
	{
		dot = LinearToTexture( dot ) / 255.0; // FIXME: this isn't robust
		VectorScale( light, dot * fIllum, light );
	}
	else
	{
		VectorFill( light, 0 );
	}
}

void CStudioRender::R_MouthSetupVertexShader( IMaterial* pMaterial )
{
	if (!pMaterial)
		return;

	// FIXME: this needs to get the mouth index from the shader
	mstudiomouth_t *pMouth = m_pStudioHdr->pMouth( 0 ); 

	// Don't deal with illum gamma, we apply it at a different point
	// for vertex shaders
	float fIllum = m_FlexWeights[pMouth->flexdesc];
	if (fIllum < 0) fIllum = 0;
	if (fIllum > 1) fIllum = 1;

	Vector forward;
	VectorRotate( pMouth->forward, m_BoneToWorld[ pMouth->bone ], forward );
	forward *= -1;

	bool found;
	IMaterialVar* pIllumVar = pMaterial->FindVar( "$illumfactor", &found, false );
	if (found)
	{
		pIllumVar->SetFloatValue( fIllum );
	}

	IMaterialVar* pFowardVar = pMaterial->FindVar( "$forward", &found, false );
	if (found)
	{
		pFowardVar->SetVecValue( forward.Base(), 3 );
	}
}
