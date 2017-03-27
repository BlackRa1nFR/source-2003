//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a sphere helper for entities that have a radius of effect.
//			Renders only when the parent entity is selected.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Box3D.h"
#include "HelperInfo.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/IMesh.h"
#include "MapDoc.h"
#include "MapSphere.h"
#include "MapView2D.h"
#include "Material.h"
#include "MathLib.h"
#include "Render2D.h"
#include "Render3D.h"
#include "ToolManager.h"
#include "ToolSphere.h"


IMPLEMENT_MAPCLASS(CMapSphere)


//
// For rendering in 3D.
//
#define MORPH_VERTEX_DIST	6


//-----------------------------------------------------------------------------
// Purpose: Factory function. Used for creating a CMapSphere helper from a
//			set of string parameters from the FGD file.
// Input  : pInfo - Pointer to helper info class which gives us information
//				about how to create the helper.
// Output : Returns a pointer to the helper, NULL if an error occurs.
//-----------------------------------------------------------------------------
CMapClass *CMapSphere::Create(CHelperInfo *pHelperInfo, CMapEntity *pParent)
{
	CMapSphere *pSphere = new CMapSphere;
	if (pSphere != NULL)
	{
		POSITION pos;
	
		//
		// The first parameter should be the key name to represent. If it isn't
		// there we assume "radius".
		//
		const char *pszKeyName = pHelperInfo->GetFirstParameter(pos);
		if (pszKeyName != NULL)
		{
			strcpy(pSphere->m_szKeyName, pszKeyName);
		}
		else
		{
			strcpy(pSphere->m_szKeyName, "radius");
		}

		//
		// Extract the line color from the parameter list.
		//
		unsigned char chRed = 255;
		unsigned char chGreen = 255;
		unsigned char chBlue = 255;

		const char *pszParam = pHelperInfo->GetNextParameter(pos);
		if (pszParam != NULL)
		{
			chRed = atoi(pszParam);
		}

		pszParam = pHelperInfo->GetNextParameter(pos);
		if (pszParam != NULL)
		{
			chGreen = atoi(pszParam);
		}

		pszParam = pHelperInfo->GetNextParameter(pos);
		if (pszParam != NULL)
		{
			chBlue = atoi(pszParam);
		}

		pSphere->SetRenderColor(chRed, chGreen, chBlue);
	}

	return pSphere;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMapSphere::CMapSphere(void)
{
	m_szKeyName[0] = '\0';

	m_flRadius = 0;

	r = 255;
	g = 255;
	b = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CMapSphere::~CMapSphere(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bFullUpdate - 
//-----------------------------------------------------------------------------
void CMapSphere::CalcBounds(BOOL bFullUpdate)
{
	CMapClass::CalcBounds(bFullUpdate);

	//
	// Pretend we're a point so that we don't change our parent entity bounds
	// in the 2D view.
	//
	m_Render2DBox.ResetBounds();
	m_Render2DBox.UpdateBounds(m_Origin);

	//
	// Build our bounds for frustum culling in the 3D views.
	//
	m_CullBox.ResetBounds();
	Vector mins = m_Origin - Vector(m_flRadius, m_flRadius, m_flRadius);
	Vector maxs = m_Origin + Vector(m_flRadius, m_flRadius, m_flRadius);
	m_CullBox.UpdateBounds(mins, maxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapSphere::Copy(bool bUpdateDependencies)
{
	CMapSphere *pCopy = new CMapSphere;

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
CMapClass *CMapSphere::CopyFrom(CMapClass *pObject, bool bUpdateDependencies)
{
	ASSERT(pObject->IsMapClass(MAPCLASS_TYPE(CMapSphere)));
	CMapSphere *pFrom = (CMapSphere *)pObject;

	CMapClass::CopyFrom(pObject, bUpdateDependencies);

	m_flRadius = pFrom->m_flRadius;
	strcpy(m_szKeyName, pFrom->m_szKeyName);

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the radius of the sphere helper and updates our parent
//			entity's keyvalue.
//-----------------------------------------------------------------------------
void CMapSphere::SetRadius(float flRadius)
{
	m_flRadius = rint(flRadius);

	CMapEntity *pEntity = dynamic_cast <CMapEntity *>(Parent);
	if (pEntity != NULL)
	{
		char szValue[80];
		sprintf(szValue, "%g", m_flRadius);
		pEntity->NotifyChildKeyChanged(this, m_szKeyName, szValue);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets the tool object for a given context data from HitTest2D.
//-----------------------------------------------------------------------------
CBaseTool *CMapSphere::GetToolObject(int nHitData)
{
	CToolSphere *pTool = (CToolSphere *)g_pToolManager->GetToolForID(TOOL_SPHERE);
	pTool->Attach(this);
	return pTool;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - point in client coordinates
// Output : 
//-----------------------------------------------------------------------------
CMapClass *CMapSphere::HitTest2D(CMapView2D *pView, const Vector2D &vecPoint, int &nHitData)
{
	Vector2D vecClientOrigin;
	pView->WorldToClient(vecClientOrigin, m_Origin);

	Vector vecRadius = m_Origin;
	vecRadius[pView->axHorz] += m_flRadius;

	Vector2D vecClientRadius;
	pView->WorldToClient(vecClientRadius, vecRadius);

	int nRadius = abs(vecClientRadius.x - vecClientOrigin.x);

	vecClientRadius.x = nRadius;
	vecClientRadius.y = nRadius;

	Vector2D vecClientMin = vecClientOrigin - vecClientRadius;
	Vector2D vecClientMax = vecClientOrigin + vecClientRadius;

	//
	// Check the four resize handles.
	//
	Vector2D vecTemp(vecClientOrigin.x, vecClientMin.y - 8);

	if (pView->CheckDistance(vecPoint, vecTemp, 6))
	{
		// Top handle
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		return this;
	}

	vecTemp.x = vecClientOrigin.x;
	vecTemp.y = vecClientMax.y + 8;
	if (pView->CheckDistance(vecPoint, vecTemp, 6))
	{
		// Bottom handle
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		return this;
	}

	vecTemp.x = vecClientMin.x - 8;
	vecTemp.y = vecClientOrigin.y;
	if (pView->CheckDistance(vecPoint, vecTemp, 6))
	{
		// Left handle
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
		return this;
	}

	vecTemp.x = vecClientMax.x + 8;
	vecTemp.y = vecClientOrigin.y;
	if (pView->CheckDistance(vecPoint, vecTemp, 6))
	{
		// Right handle
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
		return this;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Notifies that this object's parent entity has had a key value change.
// Input  : szKey - The key that changed.
//			szValue - The new value of the key.
//-----------------------------------------------------------------------------
void CMapSphere::OnParentKeyChanged(const char *szKey, const char *szValue)
{
	if (!stricmp(szKey, m_szKeyName))
	{
		m_flRadius = atof(szValue);
		PostUpdate(Notify_Changed);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//			pMesh - 
//			ScreenPos - 
//			color - 
//-----------------------------------------------------------------------------
void CMapSphere::RenderHandle(CRender3D *pRender, IMesh *pMesh, const Vector2D &ScreenPos, unsigned char *color)
{
	//
	// Render the edge handle as a box.
	//
	pRender->BeginParallel();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_POLYGON, 4 );

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_LINE_LOOP, 4 );

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	pRender->EndParallel();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapSphere::Render2D(CRender2D *pRender)
{
	if (GetSelectionState() != SELECT_NONE)
	{
		pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 255, 255, 255);

		CPoint ptClientOrigin;
		pRender->TransformPoint3D(ptClientOrigin, m_Origin);

		CPoint ptClientRadius;
		pRender->TransformPoint3D(ptClientRadius, Vector(m_flRadius, m_flRadius, m_flRadius));
		ptClientRadius.x = abs(ptClientRadius.x);
		ptClientRadius.y = abs(ptClientRadius.y);

		CPoint ptClientMin = ptClientOrigin - ptClientRadius;
		CPoint ptClientMax = ptClientOrigin + ptClientRadius;

		pRender->DrawEllipse(ptClientOrigin, ptClientRadius.x, ptClientRadius.y, false);
		pRender->DrawEllipse(ptClientOrigin, 1, 1, false);

		//
		// Draw the four resize handles.
		//
		pRender->SetFillColor(255, 255, 255);

		CPoint ptTemp;
		ptTemp.x = ptClientOrigin.x;
		ptTemp.y = ptClientMin.y - 8;
		pRender->DrawPoint(ptTemp, 6);

		ptTemp.x = ptClientOrigin.x;
		ptTemp.y = ptClientMax.y + 8;
		pRender->DrawPoint(ptTemp, 6);

		ptTemp.x = ptClientMin.x - 8;
		ptTemp.y = ptClientOrigin.y;
		pRender->DrawPoint(ptTemp, 6);

		ptTemp.x = ptClientMax.x + 8;
		ptTemp.y = ptClientOrigin.y;
		pRender->DrawPoint(ptTemp, 6);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders the wireframe sphere.
// Input  : pRender - Interface to renderer.
//-----------------------------------------------------------------------------
void CMapSphere::Render3D(CRender3D *pRender)
{
	if (Parent->IsSelected() && (m_flRadius != 0))
	{
		pRender->RenderWireframeSphere(m_Origin, m_flRadius, 12, 12, 255, 255, 0);

//		MaterialSystemInterface()->PopMatrix();

		//
		// Render the center handle.
		//
//		pRender->SetRenderMode(RENDER_MODE_DEFAULT);
//		pRender->SetRenderMode(RENDER_MODE_SELECTION_OVERLAY);

//		Vector2D vecScreen;
//		unsigned char colorHandle[] = { 255, 255, 255, 255 };
//		IMesh *pMesh = MaterialSystemInterface()->GetDynamicMesh();

//		Vector Point = m_Origin;

//		matrix4_t inv;
//		memcpy(&inv, &ViewMatrix, 16 * sizeof(float) );
//		MatrixInvert( inv );
//		MatrixMultiplyPoint(ViewPos, Point, ViewMatrix);
//
//		if (ViewPos[2] < 0)
//		{
//			pRender->WorldToScreen(vecScreen, Point);
//			RenderHandle(pRender, pMesh, vecScreen, colorHandle);
//		}
	}
}


