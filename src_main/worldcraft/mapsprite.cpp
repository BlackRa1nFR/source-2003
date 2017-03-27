//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "worldcraft_mathlib.h"
#include "Box3D.h"
#include "BSPFile.h"
#include "const.h"
#include "MapDefs.h"		// dvs: For COORD_NOTINIT
#include "MapEntity.h"
#include "MapSprite.h"
#include "Render3D.h"
#include "Worldcraft.h"
#include "Texture.h"
#include "TextureSystem.h"
#include "materialsystem/IMesh.h"
#include "Material.h"

IMPLEMENT_MAPCLASS(CMapSprite)


//-----------------------------------------------------------------------------
// Purpose: Factory function. Used for creating a CMapSprite from a set
//			of string parameters from the FGD file.
// Input  : *pInfo - Pointer to helper info class which gives us information
//				about how to create the class.
// Output : Returns a pointer to the class, NULL if an error occurs.
//-----------------------------------------------------------------------------
CMapClass *CMapSprite::CreateMapSprite(CHelperInfo *pHelperInfo, CMapEntity *pParent)
{
	POSITION pos;
	const char *pszSprite = pHelperInfo->GetFirstParameter(pos);

	//
	// If we weren't passed a sprite name as an argument, get it from our parent
	// entity's "model" key.
	//
	if (pszSprite == NULL)
	{
		pszSprite = pParent->GetKeyValue("model");
	}

	// HACK?
	// When loading sprites, it can be the case that 'materials' is prepended
	// This is because we have to look in the materials directory for sprites
	// Remove the materials prefix...
	if (pszSprite)
	{
		if (!strnicmp(pszSprite, "materials", 9) && ((pszSprite[9] == '/') || (pszSprite[9] == '\\')) )
			pszSprite += 10;
	}

	//
	// If we have a sprite name, create a sprite object.
	//
	CMapSprite *pSprite = NULL;

	if (pszSprite != NULL)
	{
		pSprite = CreateMapSprite(pszSprite);
		if (pSprite != NULL)
		{
			//
			// Icons are alpha tested.
			//
			if (!stricmp(pHelperInfo->GetName(), "iconsprite"))
			{
				pSprite->SetRenderMode( kRenderTransAlpha );
				pSprite->m_bIsIcon = true;
			}
			else
			{
				// FIXME: Gotta do this a little better
				// This initializes the render mode in the sprite
				pSprite->SetRenderMode( pSprite->m_eRenderMode );
			}
		}
	}

	return(pSprite);
}


//-----------------------------------------------------------------------------
// Purpose: Factory. Use this to construct CMapSprite objects, since the
//			constructor is protected.
//-----------------------------------------------------------------------------
CMapSprite *CMapSprite::CreateMapSprite(const char *pszSpritePath)
{
	CMapSprite *pSprite = new CMapSprite;

	if (pSprite != NULL)
	{
		char szPath[MAX_PATH];

		pSprite->Initialize();

		// HACK: Remove the extension, this is for backward compatability
		// It's trying to load a .spr, but we're giving it a .vmt.
		strcpy( szPath, pszSpritePath );
		char* pDot = strrchr( szPath, '.' );
		if (pDot)
			*pDot = 0;

		pSprite->m_pSpriteInfo = CSpriteCache::CreateSprite(szPath);
		if (pSprite->m_pSpriteInfo)
		{
			pSprite->CalcBounds();
		}
	}

	return(pSprite);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMapSprite::CMapSprite(void)
{
	Initialize();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CMapSprite::~CMapSprite(void)
{
	CSpriteCache::Release(m_pSpriteInfo);
}


//-----------------------------------------------------------------------------
// Sets the render mode
//-----------------------------------------------------------------------------

void CMapSprite::SetRenderMode( int eRenderMode )
{
	m_eRenderMode = eRenderMode;
	if (m_pSpriteInfo)
		m_pSpriteInfo->SetRenderMode( m_eRenderMode );
}

//-----------------------------------------------------------------------------
// Purpose: Calculates our bounding box based on the sprite dimensions.
// Input  : bFullUpdate - Whether we should recalculate our childrens' bounds.
//-----------------------------------------------------------------------------
void CMapSprite::CalcBounds(BOOL bFullUpdate)
{
	CMapClass::CalcBounds(bFullUpdate);

	float fRadius = 8;
	
	if (m_pSpriteInfo)
	{
		fRadius = max(m_pSpriteInfo->GetWidth(), m_pSpriteInfo->GetHeight()) * m_fScale / 2.0;
		if (fRadius == 0)
		{
			fRadius = 8;
		}
	}

	//
	// Build our bounds for frustum culling in the 3D view.
	//
	Vector Mins = m_Origin - Vector(fRadius, fRadius, fRadius);
	Vector Maxs = m_Origin + Vector(fRadius, fRadius, fRadius);
	m_CullBox.UpdateBounds(Mins, Maxs);

	//
	// Build our bounds for 2D rendering. We keep sprites small in the 2D views no
	// matter how large they are scaled.
	//
	if (!m_bIsIcon)
	{
		fRadius = 2;
	}

	Mins = m_Origin - Vector(fRadius, fRadius, fRadius);
	Maxs = m_Origin + Vector(fRadius, fRadius, fRadius);
	m_Render2DBox.UpdateBounds(Mins, Maxs);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a copy of this object.
// Output : Pointer to the new object.
//-----------------------------------------------------------------------------
CMapClass *CMapSprite::Copy(bool bUpdateDependencies)
{
	CMapSprite *pCopy = new CMapSprite;

	if (pCopy != NULL)
	{
		pCopy->CopyFrom(this, bUpdateDependencies);
	}

	return(pCopy);
}


//-----------------------------------------------------------------------------
// Purpose: Turns this into a duplicate of the given object.
// Input  : pObject - Pointer to the object to copy from.
// Output : Returns a pointer to this object.
//-----------------------------------------------------------------------------
CMapClass *CMapSprite::CopyFrom(CMapClass *pObject, bool bUpdateDependencies)
{
	CMapSprite *pFrom = dynamic_cast<CMapSprite *>(pObject);
	ASSERT(pObject != NULL);

	if (pObject != NULL)
	{
		CMapClass::CopyFrom(pObject, bUpdateDependencies);

		m_Angles = pFrom->m_Angles;

		m_pSpriteInfo = pFrom->m_pSpriteInfo;
		CSpriteCache::AddRef(pFrom->m_pSpriteInfo);

		m_nCurrentFrame = pFrom->m_nCurrentFrame;
		m_fSecondsPerFrame = pFrom->m_fSecondsPerFrame;
		m_fElapsedTimeThisFrame = pFrom->m_fElapsedTimeThisFrame;
		m_fScale = pFrom->m_fScale;
		SetRenderMode( pFrom->m_eRenderMode );
		m_RenderColor = pFrom->m_RenderColor;
		m_bIsIcon = pFrom->m_bIsIcon;
	}

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//-----------------------------------------------------------------------------
void CMapSprite::EnableAnimation(BOOL bEnable)
{
	//m_bAnimateModels = bEnable;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Angles - 
//-----------------------------------------------------------------------------
void CMapSprite::GetAngles(QAngle &Angles)
{
	Angles = m_Angles;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapSprite::Initialize(void)
{
	ZeroVector(m_Angles);

	m_eRenderMode = kRenderNormal;

	m_RenderColor.r = 255;
	m_RenderColor.g = 255;
	m_RenderColor.b = 255;

	m_fSecondsPerFrame = 1;
	m_fElapsedTimeThisFrame = 0;
	m_nCurrentFrame = 0;

	m_fScale = 0.25;

	m_bIsIcon = false;
}


//-----------------------------------------------------------------------------
// Updates time and returns the next frame
//-----------------------------------------------------------------------------
int CMapSprite::GetNextSpriteFrame( CRender3D* pRender )
{
	//
	// Determine whether we need to advance to the next frame based on our
	// sprite framerate and the elapsed time.
	//
	int nNumFrames = m_pSpriteInfo->GetFrameCount();
	if (nNumFrames > 1)
	{
		float fElapsedTime = pRender->GetElapsedTime();
		m_fElapsedTimeThisFrame += fElapsedTime;

		while (m_fElapsedTimeThisFrame > m_fSecondsPerFrame)
		{
			m_nCurrentFrame++;
			m_fElapsedTimeThisFrame -= m_fSecondsPerFrame;
		}

		m_nCurrentFrame %= nNumFrames;
	}

	return m_nCurrentFrame;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapSprite::Render3D(CRender3D *pRender)
{
	int nPasses;
	if ((GetSelectionState() != SELECT_NONE) && (!m_bIsIcon))
	{
		if (pRender->NeedsOverlay())
			nPasses = 3;
		else
			nPasses = 2;
	}
	else
	{
		nPasses = 1;
	}

	//
	// If we have a sprite, render it.
	//
	if (m_pSpriteInfo)
	{
		//
		// Only sprite icons can be clicked on, sprite preview objects cannot.
		//
		if (m_bIsIcon)
		{
			pRender->BeginRenderHitTarget(this);
		}

		m_pSpriteInfo->Bind(pRender, GetNextSpriteFrame(pRender));
		for (int nPass = 0; nPass < nPasses; nPass++)
		{
			if (nPass == 0)
			{
				// First pass uses the default rendering mode.
				// unless that mode is texture
				if (pRender->GetCurrentRenderMode() == RENDER_MODE_LIGHTMAP_GRID)
					pRender->SetRenderMode(RENDER_MODE_TEXTURED);
				else
					pRender->SetRenderMode(RENDER_MODE_DEFAULT);
			}
			else
			{
				if (nPass == nPasses - 1)
				{
					// last pass uses wireframe rendering mode.
					pRender->SetRenderMode(RENDER_MODE_WIREFRAME);
				}
				else
				{
					pRender->SetRenderMode(RENDER_MODE_SELECTION_OVERLAY);
				}
			}

			Vector point, forward, right, up;
			float fScale;
			float fBlend;
			unsigned char color[4];


			if (m_fScale > 0)
			{
				fScale = m_fScale;
			}
			else
			{
				fScale = 1.0;
			}

			// dvs: lots of things contribute to blend factor. See r_blend in engine.
			//if (m_eRenderMode == kRenderNormal)
			{
				fBlend = 1.0;
			}

			SpriteColor( color, m_eRenderMode, m_RenderColor, fBlend * 255);

			//
			// If selected, render a yellow wireframe box.
			//
			if (GetSelectionState() != SELECT_NONE)
			{
				if (m_bIsIcon)
					pRender->RenderWireframeBox(m_Render2DBox.bmins, m_Render2DBox.bmaxs, 255, 255, 0);
				else
				{
					color[0] = 255;
					color[1] = color[2] = 0;
				}
			}

			Vector ViewUp;
			Vector ViewRight;
			Vector ViewForward;

			pRender->GetViewUp(ViewUp);
			pRender->GetViewRight(ViewRight);
			pRender->GetViewForward(ViewForward);

			GetSpriteAxes(m_Angles, m_pSpriteInfo->GetType(), forward, right, up, ViewUp, ViewRight, ViewForward);

			//
			// If selected, render the sprite with a yellow wireframe around it.
			//
			if ((GetSelectionState() != SELECT_NONE) && (nPass > 0))
			{
				color[0] = color[1] = 255; color[2] = 0;
			}

			IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

			float spriteLeft, spriteUp, spriteRight, spriteDown;
			m_pSpriteInfo->GetRect( spriteLeft, spriteUp, spriteRight, spriteDown );

			MaterialPrimitiveType_t type = (nPass > 0) ? MATERIAL_LINE_LOOP : MATERIAL_POLYGON;

			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, type, 4 );

			VectorMA(m_Origin, spriteDown * fScale, up, point);
			VectorMA(point, spriteLeft * fScale, right, point);
			meshBuilder.Position3fv(point.Base());
			meshBuilder.TexCoord2f(0, 0, 1);
			meshBuilder.Color3ub( color[0], color[1], color[2] );
			meshBuilder.AdvanceVertex();

			VectorMA(m_Origin, spriteUp * fScale, up, point);
			VectorMA(point, spriteLeft * fScale, right, point);
			meshBuilder.Position3fv(point.Base());
			meshBuilder.TexCoord2f(0, 0, 0);
			meshBuilder.Color3ub( color[0], color[1], color[2] );
			meshBuilder.AdvanceVertex();

			VectorMA(m_Origin, spriteUp * fScale, up, point);
			VectorMA(point, spriteRight * fScale, right, point);
			meshBuilder.Position3fv(point.Base());
			meshBuilder.TexCoord2f(0, 1, 0);
			meshBuilder.Color3ub( color[0], color[1], color[2] );
			meshBuilder.AdvanceVertex();

			VectorMA(m_Origin, spriteDown * fScale, up, point);
			VectorMA(point, spriteRight * fScale, right, point);
			meshBuilder.Position3fv(point.Base());
			meshBuilder.TexCoord2f(0, 1, 1);
			meshBuilder.Color3ub( color[0], color[1], color[2] );
			meshBuilder.AdvanceVertex();

			meshBuilder.End();
			pMesh->Draw();
		}

		//
		// Only sprite icons can be clicked on, sprite preview objects cannot.
		//
		if (m_bIsIcon)
		{
			pRender->EndRenderHitTarget();
		}

		pRender->SetRenderMode(RENDER_MODE_DEFAULT);
	}
	//
	// Else no sprite, render as a bounding box.
	//
	else if (m_bIsIcon)
	{
		pRender->BeginRenderHitTarget(this);
		pRender->RenderBox(m_Render2DBox.bmins, m_Render2DBox.bmaxs, r, g, b, GetSelectionState());
		pRender->EndRenderHitTarget();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &File - 
//			bRMF - 
// Output : int
//-----------------------------------------------------------------------------
int CMapSprite::SerializeRMF(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &File - 
//			bRMF - 
// Output : int
//-----------------------------------------------------------------------------
int CMapSprite::SerializeMAP(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTransBox - 
//-----------------------------------------------------------------------------
void CMapSprite::DoTransform(Box3D *pTransBox)
{
	CMapClass::DoTransform(pTransBox);

	if (pTransBox->IsRotating())
	{
		//
		// Build a rotation matrix from the X, Y, Z angles.
		//
		QAngle RotateAngles;
		pTransBox->GetRotateAngles(RotateAngles);

		QAngle EngineAngles;
		EngineAngles[PITCH] = -RotateAngles[1]; // The engine (and Mathlib.c) negates pitch.
		EngineAngles[YAW] = RotateAngles[2];
		EngineAngles[ROLL] = RotateAngles[0];

		matrix3x4_t fRotateMatrix;
		AngleMatrix(EngineAngles, fRotateMatrix);

		//
		// Build a matrix representing our current orientation.
		//
		matrix3x4_t fCurrentMatrix;
		AngleMatrix(m_Angles, fCurrentMatrix);

		//
		// Rotate our orientation matrix and convert it back to Euler angles.
		//
		matrix3x4_t fMatrixNew;
		ConcatTransforms(fRotateMatrix, fCurrentMatrix, fMatrixNew);
		MatrixAngles(fMatrixNew, m_Angles);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//-----------------------------------------------------------------------------
void CMapSprite::DoTransFlip(const Vector &RefPoint)
{
	CMapClass::DoTransFlip(RefPoint);

	//
	// Generate a matrix from our current Euler angles.
	//
	matrix3x4_t fCurrentMatrix;
	AngleMatrix(m_Angles, fCurrentMatrix);

	//
	// Flipping along the X axis: Negate the X component of the model's forward
	// vector, the Y component of the model's right vector, and the X and Y components
	// of the model's up vector.
	//
	if (RefPoint[0] != COORD_NOTINIT)
	{
		fCurrentMatrix[0][0] = -fCurrentMatrix[0][0];

		fCurrentMatrix[1][1] = -fCurrentMatrix[1][1];

		fCurrentMatrix[2][0] = -fCurrentMatrix[2][0];
		fCurrentMatrix[2][1] = -fCurrentMatrix[2][1];
	}

	//
	// Flipping along the Y axis: Negate the Y component of the model's forward
	// vector, the X component of the model's right vector, and the X and Y components
	// of the model's up vector.
	//
	if (RefPoint[1] != COORD_NOTINIT)
	{
		fCurrentMatrix[0][1] = -fCurrentMatrix[0][1];

		fCurrentMatrix[1][0] = -fCurrentMatrix[1][0];

		fCurrentMatrix[2][0] = -fCurrentMatrix[2][0];
		fCurrentMatrix[2][1] = -fCurrentMatrix[2][1];
	}

	//
	// Flipping along the Z axis: Negate the Z component of the model's forward
	// vector, the X, Y, and Z components of the model's right vector, and the Z component
	// of the model's up vector.
	//
	if (RefPoint[2] != COORD_NOTINIT)
	{
		fCurrentMatrix[0][2] = -fCurrentMatrix[0][2];

		fCurrentMatrix[1][2] = -fCurrentMatrix[0][1];

		fCurrentMatrix[2][2] = -fCurrentMatrix[2][2];
	}

	//
	// Convert our new rotation matrix back to Euler angles.
	//
	MatrixAngles(fCurrentMatrix, m_Angles);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRefPoint - 
//			Angles - 
//-----------------------------------------------------------------------------
void CMapSprite::DoTransRotate(const Vector *pRefPoint, const QAngle &Angles)
{
	CMapClass::DoTransRotate(pRefPoint, Angles);

	if (pRefPoint == NULL)
	{
		pRefPoint = &m_Origin;
	}

	//
	// Get our current rotation matrix from our Euler angles.
	//
	matrix3x4_t fCurrentMatrix;
	AngleMatrix(m_Angles, fCurrentMatrix);

	//
	// Convert the X, Y, Z rotations to a rotation matrix, then apply it
	// to our current rotation matrix.
	//
	QAngle EngineAngles;
	EngineAngles[PITCH] = -Angles[1]; // The engine (and Mathlib.c) negates pitch.
	EngineAngles[YAW] = Angles[2];
	EngineAngles[ROLL] = Angles[0];

	matrix3x4_t fMatrix;
	matrix3x4_t fMatrixNew;
	AngleMatrix(EngineAngles, fMatrix);
	ConcatTransforms(fMatrix, fCurrentMatrix, fMatrixNew);

	//
	// Convert our new rotation matrix back to Euler angles.
	//
	MatrixAngles(fMatrixNew, m_Angles);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			type - 
//			forward - 
//			right - 
//			up - 
//-----------------------------------------------------------------------------
void CMapSprite::GetSpriteAxes(QAngle& Angles, int type, Vector& forward, Vector& right, Vector& up, Vector& ViewUp, Vector& ViewRight, Vector& ViewForward)
{
	int				i;
	float			dot, angle, sr, cr;
	Vector			tvec;

	// Automatically roll parallel sprites if requested
	if (Angles[2] != 0 && type == SPR_VP_PARALLEL )
	{
		type = SPR_VP_PARALLEL_ORIENTED;
	}

	switch (type)
	{
		case SPR_FACING_UPRIGHT:
		{
			// generate the sprite's axes, with vup straight up in worldspace, and
			// r_spritedesc.vright perpendicular to modelorg.
			// This will not work if the view direction is very close to straight up or
			// down, because the cross product will be between two nearly parallel
			// vectors and starts to approach an undefined state, so we don't draw if
			// the two vectors are less than 1 degree apart
			tvec[0] = -m_Origin[0];
			tvec[1] = -m_Origin[1];
			tvec[2] = -m_Origin[2];
			VectorNormalize (tvec);
			dot = tvec[2];	// same as DotProduct (tvec, r_spritedesc.vup) because
							//  r_spritedesc.vup is 0, 0, 1
			if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
				return;
			up[0] = 0;
			up[1] = 0;
			up[2] = 1;
			right[0] = tvec[1];
									// CrossProduct(r_spritedesc.vup, -modelorg,
			right[1] = -tvec[0];
									//              r_spritedesc.vright)
			right[2] = 0;
			VectorNormalize (right);
			forward[0] = -right[1];
			forward[1] = right[0];
			forward[2] = 0;
						// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
						//  r_spritedesc.vpn)
			break;
		}

		case SPR_VP_PARALLEL:
		{
			// generate the sprite's axes, completely parallel to the viewplane. There
			// are no problem situations, because the sprite is always in the same
			// position relative to the viewer
			for (i=0 ; i<3 ; i++)
			{
				up[i] = ViewUp[i];
				right[i] = ViewRight[i];
				forward[i] = ViewForward[i];
			}
			break;
		}
	
		case SPR_VP_PARALLEL_UPRIGHT:
		{
			// generate the sprite's axes, with vup straight up in worldspace, and
			// r_spritedesc.vright parallel to the viewplane.
			// This will not work if the view direction is very close to straight up or
			// down, because the cross product will be between two nearly parallel
			// vectors and starts to approach an undefined state, so we don't draw if
			// the two vectors are less than 1 degree apart
			dot = ViewForward[2];	// same as DotProduct (vpn, r_spritedesc.vup) because
									//  r_spritedesc.vup is 0, 0, 1
			if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
				return;
			
			up[0] = 0;
			up[1] = 0;
			up[2] = 1;
			
			right[0] = ViewForward[1];
			right[1] = -ViewForward[0];
			right[2] = 0;
			VectorNormalize (right);

			forward[0] = -right[1];
			forward[1] = right[0];
			forward[2] = 0;
			break;
		}
	
		case SPR_ORIENTED:
		{
			// generate the sprite's axes, according to the sprite's world orientation
			AngleVectors(Angles, &forward, &right, &up);
			break;
		}

		case SPR_VP_PARALLEL_ORIENTED:
		{
			// generate the sprite's axes, parallel to the viewplane, but rotated in
			// that plane around the center according to the sprite entity's roll
			// angle. So vpn stays the same, but vright and vup rotate
			angle = Angles[ROLL] * (M_PI*2 / 360);
			sr = sin(angle);
			cr = cos(angle);

			for (i=0 ; i<3 ; i++)
			{
				forward[i] = ViewForward[i];
				right[i] = ViewRight[i] * cr + ViewUp[i] * sr;
				up[i] = ViewRight[i] * -sr + ViewUp[i] * cr;
			}
			break;
		}

		default:
		{
			//Sys_Error ("R_DrawSprite: Bad sprite type %d", type);
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pColor - 
//			pEntity - 
//			alpha - 
//-----------------------------------------------------------------------------
void CMapSprite::SpriteColor(unsigned char *pColor, int eRenderMode, colorVec RenderColor, int alpha)
{
	int a;

	if ((eRenderMode == kRenderTransAdd) || (eRenderMode == kRenderGlow) || (eRenderMode == kRenderWorldGlow))
	{
		a = alpha;
	}
	else
	{
		a = 256;
	}
	
	if ((RenderColor.r == 0) && (RenderColor.g == 0) && (RenderColor.b == 0))
	{
		pColor[0] = pColor[1] = pColor[2] = (255 * a) >> 8;
	}
	else
	{
		pColor[0] = ((int)RenderColor.r * a)>>8;
		pColor[1] = ((int)RenderColor.g * a)>>8;
		pColor[2] = ((int)RenderColor.b * a)>>8;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Notifies that this object's parent entity has had a key value change.
// Input  : szKey - The key that changed.
//			szValue - The new value of the key.
//-----------------------------------------------------------------------------
void CMapSprite::OnParentKeyChanged(LPCSTR szKey, LPCSTR szValue)
{
	if (!stricmp(szKey, "framerate"))
	{
		float fFramesPerSecond = atof(szValue);
		if (fabs(fFramesPerSecond) > 0.001)
		{
			m_fSecondsPerFrame = 1 / fFramesPerSecond;
		}
	}
	else if (!stricmp(szKey, "scale"))
	{
		m_fScale = atof(szValue);
		if (m_fScale == 0)
		{
			m_fScale = 1;
		}

		PostUpdate(Notify_Changed);
	}
	else if (!stricmp(szKey, "rendermode"))
	{
		switch (atoi(szValue))
		{
			case 0: // "Normal"
			{
				SetRenderMode( kRenderNormal );
				break;
			}

			case 1: // "Color"
			{
				SetRenderMode( kRenderTransColor );
				break;
			}

			case 2: // "Texture"
			{
				SetRenderMode( kRenderNormal );
				break;
			}

			case 3: // "Glow"
			{
				SetRenderMode( kRenderGlow );
				break;
			}

			case 4: // "Solid"
			{
				SetRenderMode( kRenderNormal );
				break;
			}

			case 5: // "Additive"
			{
				SetRenderMode( kRenderTransAdd );
				break;
			}
		}
	}
	//
	// If we are the child of a light entity and its color is changing, change our render color.
	//
	else if (!stricmp(szKey, "_light"))
	{
		sscanf(szValue, "%d %d %d", &m_RenderColor.r, &m_RenderColor.g, &m_RenderColor.b);
	}
	else if (!stricmp(szKey, "angles"))
	{
		sscanf(szValue, "%f %f %f", &m_Angles[PITCH], &m_Angles[YAW], &m_Angles[ROLL]);
		PostUpdate(Notify_Changed);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapSprite::ShouldRenderLast(void)
{
	return(true);
}

