//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Box3D.h"
#include "GlobalFunctions.h"
#include "HelperInfo.h"
#include "materialsystem/IMaterialSystem.h"
#include "MainFrm.h"			// For refreshing the object properties dialog
#include "MapDoc.h"
#include "MapPointHandle.h"
#include "MapView2D.h"
#include "Material.h"
#include "Options.h"
#include "ObjectProperties.h"	// For refreshing the object properties dialog
#include "Render2D.h"
#include "Render3D.h"
#include "StatusBarIDs.h"		// For updating status bar text
#include "ToolManager.h"
#include "ToolPointHandle.h"


IMPLEMENT_MAPCLASS(CMapPointHandle);


//-----------------------------------------------------------------------------
// Purpose: Factory function. Used for creating a CMapPointHandle from a set
//			of string parameters from the FGD file.
// Input  : pInfo - Pointer to helper info class which gives us information
//				about how to create the class.
// Output : Returns a pointer to the class, NULL if an error occurs.
//-----------------------------------------------------------------------------
CMapClass *CMapPointHandle::Create(CHelperInfo *pHelperInfo, CMapEntity *pParent)
{
	static char *pszDefaultKeyName = "origin";

	bool bDrawLineToParent = !stricmp(pHelperInfo->GetName(), "vecline");

	POSITION pos;
	const char *pszKey = pHelperInfo->GetFirstParameter(pos);
	if (pszKey == NULL)
	{
		pszKey = pszDefaultKeyName;
	}

	CMapPointHandle *pBox = new CMapPointHandle(pszKey, bDrawLineToParent);
	pBox->SetRenderColor(255, 255, 255);
	return(pBox);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pfMins - 
//			pfMaxs - 
//-----------------------------------------------------------------------------
CMapPointHandle::CMapPointHandle(void)
{
	Initialize();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			bDrawLineToParent - 
//-----------------------------------------------------------------------------
CMapPointHandle::CMapPointHandle(const char *pszKey, bool bDrawLineToParent)
{
	Initialize();
	strcpy(m_szKeyName, pszKey);
	m_bDrawLineToParent = bDrawLineToParent;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapPointHandle::Initialize(void)
{
	m_szKeyName[0] = '\0';

	m_bDrawLineToParent = 0;

	r = 255;
	g = 255;
	b = 255;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapPointHandle::~CMapPointHandle(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bFullUpdate - 
//-----------------------------------------------------------------------------
void CMapPointHandle::CalcBounds(BOOL bFullUpdate)
{
	// We don't affect our parent's 2D render bounds.
	m_Render2DBox.ResetBounds();

	// Calculate 3D culling box.
	Vector Mins = m_Origin + Vector(2, 2, 2);
	Vector Maxs = m_Origin + Vector(2, 2, 2);
	m_CullBox.SetBounds(Mins, Maxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CMapClass *CMapPointHandle::Copy(bool bUpdateDependencies)
{
	CMapPointHandle *pCopy = new CMapPointHandle;

	if (pCopy != NULL)
	{
		pCopy->CopyFrom(this, bUpdateDependencies);
	}

	return(pCopy);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
// Output : 
//-----------------------------------------------------------------------------
CMapClass *CMapPointHandle::CopyFrom(CMapClass *pObject, bool bUpdateDependencies)
{
	ASSERT(pObject->IsMapClass(MAPCLASS_TYPE(CMapPointHandle)));
	CMapPointHandle *pFrom = (CMapPointHandle *)pObject;

	CMapClass::CopyFrom(pObject, bUpdateDependencies);

	strcpy(m_szKeyName, pFrom->m_szKeyName);

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nHitData - 
//-----------------------------------------------------------------------------
CBaseTool *CMapPointHandle::GetToolObject(int nHitData)
{
	CToolPointHandle *pTool = (CToolPointHandle *)g_pToolManager->GetToolForID(TOOL_POINT_HANDLE);
	pTool->Attach(this);
	return pTool;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - 
//			nData - 
// Output : 
//-----------------------------------------------------------------------------
CMapClass *CMapPointHandle::HitTest2D(CMapView2D *pView, const Vector2D &point, int &nHitData)
{
	Vector2D vecClient;
	pView->WorldToClient(vecClient, m_Origin);
	if (pView->CheckDistance(point, vecClient, HANDLE_RADIUS))
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		return this;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapPointHandle::Render2D(CRender2D *pRender)
{
	SelectionState_t eState = GetSelectionState();
	if (eState != SELECT_NONE)
	{
		if (eState == SELECT_MODIFY)
		{
			pRender->SetLineType(CRender2D::LINE_DOT, CRender2D::LINE_THIN, GetRValue(Options.colors.clrSelection), GetGValue(Options.colors.clrSelection), GetBValue(Options.colors.clrSelection));
		}
		else
		{
			pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, GetRValue(Options.colors.clrToolHandle), GetGValue(Options.colors.clrToolHandle), GetBValue(Options.colors.clrToolHandle));
		}

		CPoint ptClientOrigin;
		pRender->TransformPoint3D(ptClientOrigin, m_Origin);

		pRender->DrawEllipse(ptClientOrigin, HANDLE_RADIUS, HANDLE_RADIUS, false);
		pRender->DrawEllipse(ptClientOrigin, 1, 1, false);

		// Draw a line from origin helpers to their parent while they are being dragged.
		if ((Parent != NULL) && (m_bDrawLineToParent || (eState == SELECT_MODIFY)))
		{
			if (eState == SELECT_MODIFY)
			{
				pRender->SetLineType(CRender2D::LINE_DOT, CRender2D::LINE_THIN, GetRValue(Options.colors.clrSelection), GetGValue(Options.colors.clrSelection), GetBValue(Options.colors.clrSelection));
			}
			else
			{
				pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THICK, GetRValue(Options.colors.clrToolHandle), GetGValue(Options.colors.clrToolHandle), GetBValue(Options.colors.clrToolHandle));
			}

			Vector vecOrigin;
			Parent->GetOrigin(vecOrigin);
			pRender->DrawLine(m_Origin, vecOrigin);
		}

		if (eState == SELECT_MODIFY)
		{
			CPoint ptText(ptClientOrigin);
			ptText.y += HANDLE_RADIUS + 4;

			pRender->SetTextColor(GetRValue(Options.colors.clrToolHandle), GetGValue(Options.colors.clrToolHandle), GetBValue(Options.colors.clrToolHandle), GetRValue(Options.colors.clrBackground), GetGValue(Options.colors.clrBackground), GetBValue(Options.colors.clrBackground));

			char szText[100];
			sprintf(szText, "(%0.f, %0.f, %0.f)", m_Origin.x, m_Origin.y, m_Origin.z);
			pRender->DrawText(szText, ptText, 0, 0, CRender2D::TEXT_JUSTIFY_LEFT | CRender2D::TEXT_SINGLELINE);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapPointHandle::Render3D(CRender3D *pRender)
{
	if (GetSelectionState() != SELECT_NONE)
	{
		pRender->SetRenderMode(RENDER_MODE_WIREFRAME);

		Vector vecViewPoint;
		pRender->GetViewPoint(vecViewPoint);
		float flDist = (m_Origin - vecViewPoint).Length();

		pRender->RenderSphere(m_Origin, 0.04 * flDist, 12, 12, 128, 128, 255);

		MaterialSystemInterface()->PopMatrix();

		if ((Parent != NULL) && (m_bDrawLineToParent))
		{
			Vector vecOrigin;
			Parent->GetOrigin(vecOrigin);
			pRender->RenderLine(m_Origin, vecOrigin, 255, 255, 255);
		}

		pRender->SetRenderMode(RENDER_MODE_DEFAULT);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMapPointHandle::SerializeRMF(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMapPointHandle::SerializeMAP(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: Overridden because origin helpers don't take the color of their
//			parent entity.
// Input  : red, green, blue - 
//-----------------------------------------------------------------------------
void CMapPointHandle::SetRenderColor(unsigned char red, unsigned char green, unsigned char blue)
{
}


//-----------------------------------------------------------------------------
// Purpose: Overridden because origin helpers don't take the color of their
//			parent entity.
// Input  : red, green, blue - 
//-----------------------------------------------------------------------------
void CMapPointHandle::SetRenderColor(color32 rgbColor)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//-----------------------------------------------------------------------------
void CMapPointHandle::OnParentKeyChanged(const char *szKey, const char *szValue)
{
	if (stricmp(szKey, m_szKeyName) == 0)
	{
		sscanf(szValue, "%f %f %f", &m_Origin.x, &m_Origin.y, &m_Origin.z);
		CalcBounds();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecOrigin - 
//-----------------------------------------------------------------------------
void CMapPointHandle::UpdateOrigin(const Vector &vecOrigin)
{
	m_Origin = vecOrigin;
	CalcBounds();
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapPointHandle::UpdateParentKey(void)
{
	// Snap to prevent error creep.
	for (int i = 0; i < 3; i++)
	{
		m_Origin[i] = rint(m_Origin[i] / 0.01f) * 0.01f;
	}

	if (m_szKeyName[0])
	{
		CMapEntity *pEntity = dynamic_cast <CMapEntity *> (Parent);
		if (pEntity != NULL)
		{
			char szValue[KEYVALUE_MAX_VALUE_LENGTH];
			sprintf(szValue, "%g %g %g", (double)m_Origin.x, (double)m_Origin.y, (double)m_Origin.z);
			pEntity->NotifyChildKeyChanged(this, m_szKeyName, szValue);

		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTransBox - 
//-----------------------------------------------------------------------------
void CMapPointHandle::DoTransform(Box3D *pTransBox)
{
	BaseClass::DoTransform(pTransBox);
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Delta - 
//-----------------------------------------------------------------------------
void CMapPointHandle::DoTransMove(const Vector &Delta)
{
	BaseClass::DoTransMove(Delta);
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRefPoint - 
//			Angles - 
//-----------------------------------------------------------------------------
void CMapPointHandle::DoTransRotate(const Vector *pRefPoint, const QAngle &Angles)
{
	BaseClass::DoTransRotate(pRefPoint, Angles);
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//			Scale - 
//-----------------------------------------------------------------------------
void CMapPointHandle::DoTransScale(const Vector &RefPoint, const Vector &Scale)
{
	BaseClass::DoTransScale(RefPoint, Scale);
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//-----------------------------------------------------------------------------
void CMapPointHandle::DoTransFlip(const Vector &RefPoint)
{
	BaseClass::DoTransFlip(RefPoint);
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the keyvalue in our parent when we are	added to the world.
// Input  : pWorld - 
//-----------------------------------------------------------------------------
void CMapPointHandle::OnAddToWorld(CMapWorld *pWorld)
{
	BaseClass::OnAddToWorld(pWorld);
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: Called when we change because of an Undo or Redo.
//-----------------------------------------------------------------------------
void CMapPointHandle::OnUndoRedo(void)
{
	// We've changed but our parent entity may not have. Update our parent.
	UpdateParentKey();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the keyvalue in our parent after the map is loaded.
// Input  : pWorld - 
//-----------------------------------------------------------------------------
void CMapPointHandle::PostloadWorld(CMapWorld *pWorld)
{
	BaseClass::PostloadWorld(pWorld);
	UpdateParentKey();
}
