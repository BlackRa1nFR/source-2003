F//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Box3D.h"
#include "MapDefs.h"		// dvs: For COORD_NOTINIT
#include "MapEntity.h"
#include "MapStudioModel.h"
#include "Render2D.h"
#include "Render3D.h"
#include "ViewerSettings.h"
#include "Worldcraft.h"
#include "materialsystem/IMesh.h"
#include "TextureSystem.h"
#include "Material.h"


#define STUDIO_RENDER_DISTANCE		400


IMPLEMENT_MAPCLASS(CMapStudioModel)


float CMapStudioModel::m_fRenderDistance = STUDIO_RENDER_DISTANCE;
BOOL CMapStudioModel::m_bAnimateModels = TRUE;


//-----------------------------------------------------------------------------
// Purpose: Factory function. Used for creating a CMapStudioModel from a set
//			of string parameters from the FGD file.
// Input  : *pInfo - Pointer to helper info class which gives us information
//				about how to create the class.
// Output : Returns a pointer to the class, NULL if an error occurs.
//-----------------------------------------------------------------------------
CMapClass *CMapStudioModel::CreateMapStudioModel(CHelperInfo *pHelperInfo, CMapEntity *pParent)
{
	POSITION pos;
	const char *pszModel = pHelperInfo->GetFirstParameter(pos);

	//
	// If we weren't passed a model name as an argument, get it from our parent
	// entity's "model" key.
	//
	if (pszModel == NULL)
	{
		pszModel = pParent->GetKeyValue("model");
	}

	//
	// If we have a model name, create a studio model object.
	//
	if (pszModel != NULL)
	{
		bool bLightProp = !stricmp(pHelperInfo->GetName(), "lightprop");
		bool bOrientedBounds = (bLightProp | !stricmp(pHelperInfo->GetName(), "studioprop"));
		return CreateMapStudioModel(pszModel, bOrientedBounds, bLightProp);
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Factory function. Creates a CMapStudioModel object from a relative
//			path to an MDL file.
// Input  : pszModelPath - Relative path to the .MDL file. The path is appended
//				to each path in the	application search path until the model is found.
//			bOrientedBounds - Whether the bounding box should consider the orientation of the model.
// Output : Returns a pointer to the newly created CMapStudioModel object.
//-----------------------------------------------------------------------------
CMapStudioModel *CMapStudioModel::CreateMapStudioModel(const char *pszModelPath, bool bOrientedBounds, bool bReversePitch)
{
	CMapStudioModel *pModel = new CMapStudioModel;

	if (pModel != NULL)
	{
		//
		// Try to load the model from each directory in the search path,
		// stopping if we ever succeed. Models are loaded relative to the
		// game directory, not the models directory.
		//
		char szPath[MAX_PATH];
		POSITION pos;
		bool bLoaded = false;

		APP()->GetFirstSearchDir(SEARCHDIR_DEFAULT, szPath, pos);
		while ((pos != NULL) && (!bLoaded))
		{
			strcat(szPath, pszModelPath);

			//
			// Reverse the slashes if necessary.
			//
			char *pch = szPath;
			while (*pch != '\0')
			{
				if (*pch == '/')
				{
					*pch = '\\';
				}
				pch++;
			}

			pModel->m_pStudioModel = CStudioModelCache::CreateModel(szPath);
			if (pModel->m_pStudioModel != NULL)
			{
				bLoaded = true;

				pModel->SetOrientedBounds(bOrientedBounds);
				pModel->ReversePitch(bReversePitch);

				pModel->CalcBounds();
			}
		
			APP()->GetNextSearchDir(SEARCHDIR_DEFAULT, szPath, pos);
		}

		if (!bLoaded)
		{
			delete pModel;
			pModel = NULL;
		}
	}

	return(pModel);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMapStudioModel::CMapStudioModel(void)
{
	Initialize();
	InitViewerSettings();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Releases the studio model cache reference.
//-----------------------------------------------------------------------------
CMapStudioModel::~CMapStudioModel(void)
{
	if (m_pStudioModel != NULL)
	{
		CStudioModelCache::Release(m_pStudioModel);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called by the renderer before every frame to animate the models.
//-----------------------------------------------------------------------------
void CMapStudioModel::AdvanceAnimation(float flInterval)
{
	if (m_bAnimateModels)
	{
		CStudioModelCache::AdvanceAnimation(flInterval);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bFullUpdate - 
//-----------------------------------------------------------------------------
void CMapStudioModel::CalcBounds(BOOL bFullUpdate)
{
	CMapClass::CalcBounds(bFullUpdate);

	Vector Mins(0, 0, 0);
	Vector Maxs(0, 0, 0);

	if (m_pStudioModel != NULL)
	{
		//
		// The 3D bounds are the bounds of the oriented model's first sequence, so that
		// frustum culling works properly in the 3D view.
		//
		m_pStudioModel->SetAngles(m_Angles);
		m_pStudioModel->ExtractBbox(m_CullBox.bmins, m_CullBox.bmaxs);

		if (m_bOrientedBounds)
		{
			//
			// Oriented bounds - the 2D bounds are the same as the 3D bounds.
			//
			Mins = m_CullBox.bmins;
			Maxs = m_CullBox.bmaxs;
		}
		else
		{
			//
			// The 2D bounds are the movement bounding box of the model, which is not affected
			// by the entity's orientation. This is used for character models for which we want
			// to render a meaningful collision box in Worldcraft.
			//
			m_pStudioModel->ExtractMovementBbox(Mins, Maxs);
		}

		Mins += m_Origin;
		Maxs += m_Origin;

		m_CullBox.bmins += m_Origin;
		m_CullBox.bmaxs += m_Origin;
	}

	//
	// If we do not yet have a valid bounding box, use a default box.
	//
	if ((Maxs - Mins) == Vector(0, 0, 0))
	{
		Mins = m_CullBox.bmins = m_Origin - Vector(10, 10, 10);
		Maxs = m_CullBox.bmaxs = m_Origin + Vector(10, 10, 10);
	}

	m_Render2DBox.UpdateBounds(Mins, Maxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapStudioModel::Copy(bool bUpdateDependencies)
{
	CMapStudioModel *pCopy = new CMapStudioModel;

	if (pCopy != NULL)
	{
		pCopy->CopyFrom(this, bUpdateDependencies);
	}

	return(pCopy);
}


//-----------------------------------------------------------------------------
// Purpose: Makes this an exact duplicate of pObject.
// Input  : pObject - Object to copy.
// Output : Returns this.
//-----------------------------------------------------------------------------
CMapClass *CMapStudioModel::CopyFrom(CMapClass *pObject, bool bUpdateDependencies)
{
	ASSERT(pObject->IsMapClass(MAPCLASS_TYPE(CMapStudioModel)));
	CMapStudioModel *pFrom = (CMapStudioModel *)pObject;

	CMapClass::CopyFrom(pObject, bUpdateDependencies);

	m_pStudioModel = pFrom->m_pStudioModel;
	if (m_pStudioModel != NULL)
	{
		CStudioModelCache::AddRef(m_pStudioModel);
	}

	m_Angles = pFrom->m_Angles;
	m_Skin = pFrom->m_Skin;
	m_bOrientedBounds = pFrom->m_bOrientedBounds;
	m_bReversePitch = pFrom->m_bReversePitch;

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//-----------------------------------------------------------------------------
void CMapStudioModel::EnableAnimation(BOOL bEnable)
{
	m_bAnimateModels = bEnable;
}


//-----------------------------------------------------------------------------
// Purpose: Returns this object's pitch, yaw, and roll.
//-----------------------------------------------------------------------------
void CMapStudioModel::GetAngles(QAngle &Angles)
{
	Angles = m_Angles;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapStudioModel::Initialize(void)
{
	ZeroVector(m_Angles);
	m_pStudioModel = NULL;
	m_Skin = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Notifies that this object's parent entity has had a key value change.
// Input  : szKey - The key that changed.
//			szValue - The new value of the key.
//-----------------------------------------------------------------------------
void CMapStudioModel::OnParentKeyChanged(LPCSTR szKey, LPCSTR szValue)
{
	if (!stricmp(szKey, "angles"))
	{
		sscanf(szValue, "%f %f %f", &m_Angles[PITCH], &m_Angles[YAW], &m_Angles[ROLL]);
		PostUpdate(Notify_Changed);
	}
	else if (!stricmp(szKey, "pitch"))
	{
		m_Angles[PITCH] = atof(szValue);
		PostUpdate(Notify_Changed);
	}
	else if (!stricmp(szKey, "skin"))
	{
		m_Skin = atoi(szValue);
		PostUpdate(Notify_Changed);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
bool CMapStudioModel::RenderPreload(CRender3D *pRender, bool bNewContext)
{
	return(m_pStudioModel != NULL);
}


//-----------------------------------------------------------------------------
// Draws basis vectors
//-----------------------------------------------------------------------------
static void DrawBasisVectors( CRender3D* pRender, const Vector &origin, const QAngle &angles)
{
	matrix3x4_t fCurrentMatrix;
	AngleMatrix(angles, fCurrentMatrix);

	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 3 );

	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.Position3f(origin[0], origin[1], origin[2]);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.Position3f(origin[0] + (100 * fCurrentMatrix[0][0]), 
		origin[1] + (100 * fCurrentMatrix[1][0]), origin[2] + (100 * fCurrentMatrix[2][0]));
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.Position3f(origin[0], origin[1], origin[2]);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.Position3f(origin[0] + (100 * fCurrentMatrix[0][1]), 
		origin[1] + (100 * fCurrentMatrix[1][1]), origin[2] + (100 * fCurrentMatrix[2][1]));
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 0, 255);
	meshBuilder.Position3f(origin[0], origin[1], origin[2]);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 0, 255);
	meshBuilder.Position3f(origin[0] + (100 * fCurrentMatrix[0][2]), 
		origin[1] + (100 * fCurrentMatrix[1][2]), origin[2] + (100 * fCurrentMatrix[2][2]));
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	pRender->SetRenderMode( RENDER_MODE_DEFAULT );
}


//-----------------------------------------------------------------------------
// It should render last if all its materials are translucent
//-----------------------------------------------------------------------------
bool CMapStudioModel::ShouldRenderLast()
{
	return m_pStudioModel->IsTranslucent();
}


//-----------------------------------------------------------------------------
// Purpose: Renders the studio model in the 2D views.
// Input  : pRender - Interface to the 2D renderer.
//-----------------------------------------------------------------------------
void CMapStudioModel::Render2D(CRender2D *pRender)
{
	//
	// Just render the forward vector if the object is selected.
	//
	if (IsSelected())
	{
		Vector Forward;
		AngleVectors(m_Angles, &Forward, NULL, NULL);

		pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 255, 255, 0);
		pRender->DrawLine(m_Origin, m_Origin + Forward * 24);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders the studio model in the 3D views.
// Input  : pRender - Interface to the 3D renderer.
//-----------------------------------------------------------------------------
void CMapStudioModel::Render3D(CRender3D *pRender)
{
	//
	// Set to the default rendering mode, unless we're in lightmap mode
	//
	if (pRender->GetCurrentRenderMode() == RENDER_MODE_LIGHTMAP_GRID)
		pRender->SetRenderMode(RENDER_MODE_TEXTURED);
	else
		pRender->SetRenderMode(RENDER_MODE_DEFAULT);

	//
	// Set up our angles for rendering.
	//
	QAngle vecAngles = m_Angles;
	if (m_bReversePitch)
	{
		vecAngles[PITCH] *= -1;
	}

	//
	// If we have a model, render it if it is close enough to the camera.
	//
	if (m_pStudioModel != NULL)
	{
		Vector ViewPoint;
		pRender->GetViewPoint(ViewPoint);

		if ((fabs(ViewPoint[0] - m_Origin[0]) < m_fRenderDistance) &&
			(fabs(ViewPoint[1] - m_Origin[1]) < m_fRenderDistance) &&
			(fabs(ViewPoint[2] - m_Origin[2]) < m_fRenderDistance))
		{
			//
			// Move the model to the proper place and orient it.
			//
			m_pStudioModel->SetAngles(vecAngles);
			m_pStudioModel->SetOrigin(m_Origin[0], m_Origin[1], m_Origin[2]);
			m_pStudioModel->SetSkin(m_Skin);

			pRender->BeginRenderHitTarget(this);
			m_pStudioModel->DrawModel(pRender);
			pRender->EndRenderHitTarget();

			if (IsSelected())
			{
				pRender->RenderWireframeBox(m_Render2DBox.bmins, m_Render2DBox.bmaxs, 255, 255, 0);
			}
		}
		else
		{
			pRender->BeginRenderHitTarget(this);
			pRender->RenderBox(m_Render2DBox.bmins, m_Render2DBox.bmaxs, r, g, b, GetSelectionState());
			pRender->EndRenderHitTarget();
		}
	}
	//
	// Else no model, render as a bounding box.
	//
	else
	{
		pRender->BeginRenderHitTarget(this);
		pRender->RenderBox(m_Render2DBox.bmins, m_Render2DBox.bmaxs, r, g, b, GetSelectionState());
		pRender->EndRenderHitTarget();
	}

	//
	// Draw our basis vectors.
	//
	if (IsSelected())
	{
		DrawBasisVectors( pRender, m_Origin, vecAngles );
	}

	pRender->SetRenderMode(RENDER_MODE_DEFAULT);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &File - 
//			bRMF - 
// Output : int
//-----------------------------------------------------------------------------
int CMapStudioModel::SerializeRMF(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &File - 
//			bRMF - 
// Output : int
//-----------------------------------------------------------------------------
int CMapStudioModel::SerializeMAP(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Angles - 
//-----------------------------------------------------------------------------
void CMapStudioModel::SetAngles(QAngle &Angles)
{
	m_Angles = Angles;

	//
	// Round very small angles to zero.
	//
	for (int nDim = 0; nDim < 3; nDim++)
	{
		if (fabs(m_Angles[nDim]) < 0.001)
		{
			m_Angles[nDim] = 0;
		}
	}

	while (m_Angles[YAW] < 0)
	{
		m_Angles[YAW] += 360;
	}

	//
	// Update the angles of our parent entity.
	//
	CMapEntity *pEntity = dynamic_cast<CMapEntity *>(Parent);
	if (pEntity != NULL)
	{
		char szValue[80];
		sprintf(szValue, "%g %g %g", m_Angles[0], m_Angles[1], m_Angles[2]);
		pEntity->NotifyChildKeyChanged(this, "angles", szValue);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the distance at which studio models become rendered as bounding
//			boxes. If this is set to zero, studio models are never rendered.
// Input  : fRenderDistance - Distance in world units.
//-----------------------------------------------------------------------------
void CMapStudioModel::SetRenderDistance(float fRenderDistance)
{
	m_fRenderDistance = fRenderDistance;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTransBox - 
//-----------------------------------------------------------------------------
void CMapStudioModel::DoTransform(Box3D *pTransBox)
{
	CMapClass::DoTransform(pTransBox);

	if (pTransBox->IsRotating())
	{
		//
		// Build a rotation matrix from the X, Y, Z angles.
		//
		QAngle RotateAngles;
		pTransBox->GetRotateAngles(RotateAngles);

		QAngle Angles;
		Angles[PITCH] = -RotateAngles[1]; // The engine (and Mathlib.c) negates pitch.
		Angles[YAW] = RotateAngles[2];
		Angles[ROLL] = RotateAngles[0];

		matrix3x4_t fRotateMatrix;
		AngleMatrix(Angles, fRotateMatrix);

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
		MatrixAngles(fMatrixNew, Angles);
		SetAngles(Angles);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//-----------------------------------------------------------------------------
void CMapStudioModel::DoTransFlip(const Vector &RefPoint)
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
	if (RefPoint.x != COORD_NOTINIT)
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
	if (RefPoint.y != COORD_NOTINIT)
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
	if (RefPoint.z != COORD_NOTINIT)
	{
		fCurrentMatrix[0][2] = -fCurrentMatrix[0][2];

		fCurrentMatrix[1][2] = -fCurrentMatrix[0][1];

		fCurrentMatrix[2][2] = -fCurrentMatrix[2][2];
	}

	//
	// Convert our new rotation matrix back to Euler angles.
	//
	QAngle Angles;
	MatrixAngles(fCurrentMatrix, Angles);
	SetAngles(Angles);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRefPoint - 
//			Angles - 
//-----------------------------------------------------------------------------
void CMapStudioModel::DoTransRotate(const Vector *pRefPoint, const QAngle &Angles)
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
	MatrixAngles(fMatrixNew, EngineAngles);
	SetAngles(EngineAngles);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CMapStudioModel::GetFrame(void)
{
	// TODO:
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFrame - 
//-----------------------------------------------------------------------------
void CMapStudioModel::SetFrame(int nFrame)
{
	// TODO:
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current sequence being used for rendering.
//-----------------------------------------------------------------------------
int CMapStudioModel::GetSequence(void)
{
	if (!m_pStudioModel)
	{
		return 0;
	}
	return m_pStudioModel->GetSequence();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CMapStudioModel::GetSequenceCount(void)
{
	if (!m_pStudioModel)
	{
		return 0;
	}
	return m_pStudioModel->GetSequenceCount();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//			szName - 
//-----------------------------------------------------------------------------
void CMapStudioModel::GetSequenceName(int nIndex, char *szName)
{
	if (m_pStudioModel)
	{
		m_pStudioModel->GetSequenceName(nIndex, szName);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void CMapStudioModel::SetSequence(int nIndex)
{
	if (m_pStudioModel)
	{
		m_pStudioModel->SetSequence(nIndex);
	}
}


