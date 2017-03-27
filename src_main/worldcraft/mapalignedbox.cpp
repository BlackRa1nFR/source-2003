//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Box3D.h"
#include "HelperInfo.h"
#include "MapAlignedBox.h"
#include "MapEntity.h"
#include "Options.h"
#include "Render2D.h"
#include "Render3D.h"


IMPLEMENT_MAPCLASS(CMapAlignedBox)


//-----------------------------------------------------------------------------
// Purpose: Factory function. Used for creating a CMapAlignedBox from a set
//			of string parameters from the FGD file.
// Input  : *pInfo - Pointer to helper info class which gives us information
//				about how to create the class.
// Output : Returns a pointer to the class, NULL if an error occurs.
//-----------------------------------------------------------------------------
CMapClass *CMapAlignedBox::Create(CHelperInfo *pHelperInfo, CMapEntity *pParent)
{
	if (stricmp(pHelperInfo->GetName(), "wirebox") == 0)
	{
		const char *pMinsKeyName, *pMaxsKeyName;
		POSITION pos;

		if ((pMinsKeyName = pHelperInfo->GetFirstParameter(pos)) &&
			(pMaxsKeyName = pHelperInfo->GetNextParameter(pos)))
		{
			CMapAlignedBox *pBox = new CMapAlignedBox(pMinsKeyName, pMaxsKeyName);
			pBox->m_bWireframe = true;
			return pBox;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		//
		// Extract the box mins and maxs from the input parameter list.
		//
		Vector Mins;
		Vector Maxs;
		int nParam = 0;

		POSITION pos;
		const char *pszParam = pHelperInfo->GetFirstParameter(pos);
		while (pszParam != NULL)
		{
			if (nParam < 3)
			{
				Mins[nParam] = atof(pszParam);
			}
			else if (nParam < 6)
			{
				Maxs[nParam % 3] = atof(pszParam);
			}
			else
			{
				break;
			}

			nParam++;

			pszParam = pHelperInfo->GetNextParameter(pos);
		}

		CMapAlignedBox *pBox = NULL;

		//
		// If we got enough parameters to define the box, create it.
		//
		if (nParam >= 6)
		{
			pBox = new CMapAlignedBox(Mins, Maxs);
		}

		return(pBox);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapAlignedBox::CMapAlignedBox(void)
{
	m_bUseKeyName = false;
	m_bWireframe = false;

	ZeroVector(m_Mins);
	ZeroVector(m_Maxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pfMins - 
//			pfMaxs - 
//-----------------------------------------------------------------------------
CMapAlignedBox::CMapAlignedBox(Vector &Mins, Vector &Maxs)
{
	m_bUseKeyName = false;
	m_bWireframe = false;

	m_Mins = Mins;
	m_Maxs = Maxs;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMinsKeyName - 
//			*pMaxsKeyName - 
//-----------------------------------------------------------------------------
CMapAlignedBox::CMapAlignedBox(const char *pMinsKeyName, const char *pMaxsKeyName)
{
	m_bUseKeyName = true;
	m_bWireframe = false;
	
	strncpy(m_MinsKeyName, pMinsKeyName, sizeof(m_MinsKeyName)-1);
	strncpy(m_MaxsKeyName, pMaxsKeyName, sizeof(m_MaxsKeyName)-1);

	m_MinsKeyName[sizeof(m_MinsKeyName)-1] = 0;
	m_MaxsKeyName[sizeof(m_MaxsKeyName)-1] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapAlignedBox::~CMapAlignedBox(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bFullUpdate - 
//-----------------------------------------------------------------------------
void CMapAlignedBox::CalcBounds(BOOL bFullUpdate)
{
	Vector AbsMins = m_Origin + m_Mins;
	Vector AbsMaxs = m_Origin + m_Maxs;

	m_Render2DBox.ResetBounds();
	m_Render2DBox.UpdateBounds(AbsMins, AbsMaxs);

	m_CullBox = m_Render2DBox;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapAlignedBox::Copy(bool bUpdateDependencies)
{
	CMapAlignedBox *pCopy = new CMapAlignedBox;

	if (pCopy != NULL)
	{
		pCopy->CopyFrom(this, bUpdateDependencies);
	}

	return(pCopy);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapAlignedBox::CopyFrom(CMapClass *pObject, bool bUpdateDependencies)
{
	ASSERT(pObject->IsMapClass(MAPCLASS_TYPE(CMapAlignedBox)));
	CMapAlignedBox *pFrom = (CMapAlignedBox *)pObject;

	CMapClass::CopyFrom(pObject, bUpdateDependencies);

	m_bUseKeyName = pFrom->m_bUseKeyName;
	m_bWireframe = pFrom->m_bWireframe;

	strncpy(m_MinsKeyName, pFrom->m_MinsKeyName, sizeof(m_MinsKeyName)-1);
	strncpy(m_MaxsKeyName, pFrom->m_MaxsKeyName, sizeof(m_MaxsKeyName)-1);
	m_MinsKeyName[sizeof(pFrom->m_MinsKeyName)-1] = 0;
	m_MaxsKeyName[sizeof(pFrom->m_MaxsKeyName)-1] = 0;

	m_Mins = pFrom->m_Mins;
	m_Maxs = pFrom->m_Maxs;

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapAlignedBox::Render2D(CRender2D *pRender)
{
	int nHorz = pRender->GetAxisHorz();
	int nVert = pRender->GetAxisVert();
	int nThird = pRender->GetAxisThird();

	Vector p[5];

	p[0][nHorz] = m_Render2DBox.bmins[nHorz];
	p[0][nVert] = m_Render2DBox.bmins[nVert];
	p[0][nThird] = 0;

	p[1][nHorz] = m_Render2DBox.bmaxs[nHorz];
	p[1][nVert] = m_Render2DBox.bmins[nVert];
	p[1][nThird] = 0;

	p[2][nHorz] = m_Render2DBox.bmaxs[nHorz];
	p[2][nVert] = m_Render2DBox.bmaxs[nVert];
	p[2][nThird] = 0;

	p[3][nHorz] = m_Render2DBox.bmins[nHorz];
	p[3][nVert] = m_Render2DBox.bmaxs[nVert];
	p[3][nThird] = 0;

	p[4][nHorz] = m_Render2DBox.bmins[nHorz];
	p[4][nVert] = m_Render2DBox.bmins[nVert];
	p[4][nThird] = 0;

	if (!IsSelected())
	{
	    pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, r, g, b);
	}
	else
	{
	    pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, GetRValue(Options.colors.clrSelection), GetGValue(Options.colors.clrSelection), GetBValue(Options.colors.clrSelection));
	}

	pRender->DrawLineLoop(5, p, false, 1);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapAlignedBox::Render3D(CRender3D *pRender)
{
	pRender->SetRenderMode(RENDER_MODE_DEFAULT);

	if (!m_bWireframe)
	{
		pRender->BeginRenderHitTarget(this);
		pRender->RenderBox(m_CullBox.bmins, m_CullBox.bmaxs, r, g, b, GetSelectionState());
		pRender->EndRenderHitTarget();
	}
	else if (GetSelectionState() != SELECT_NONE)
	{
		pRender->RenderWireframeBox(m_CullBox.bmins, m_CullBox.bmaxs, 255, 255, 0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : File - 
//			bRMF - 
// Output : int
//-----------------------------------------------------------------------------
int CMapAlignedBox::SerializeRMF(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : File - 
//			bRMF - 
// Output : int
//-----------------------------------------------------------------------------
int CMapAlignedBox::SerializeMAP(fstream &File, BOOL bRMF)
{
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : key - 
//			value - 
//-----------------------------------------------------------------------------
void CMapAlignedBox::OnParentKeyChanged( LPCSTR key, LPCSTR value )
{
	if (!m_bUseKeyName)
	{
		return;
	}

	if (stricmp(key, m_MinsKeyName) == 0)
	{
		sscanf(value, "%f %f %f", &m_Mins[0], &m_Mins[1], &m_Mins[2]);
	}
	else if (stricmp(key, m_MaxsKeyName) == 0)
	{
		sscanf(value, "%f %f %f", &m_Maxs[0], &m_Maxs[1], &m_Maxs[2]);
	}

	PostUpdate(Notify_Changed);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *t - 
//-----------------------------------------------------------------------------
//void CMapAlignedBox::DoTransform(Box3D *t)
//{
//	if (!m_bWireframe)
//	{
//		// Only wireframe boxes can be transformed.
//		return;
//	}
//
//	Vector vecPoints[] =
//	{
//		Vector(m_CullBox.bmins.x, m_CullBox.bmins.y, m_CullBox.bmins.z),
//		Vector(m_CullBox.bmins.x, m_CullBox.bmaxs.y, m_CullBox.bmins.z),
//		Vector(m_CullBox.bmins.x, m_CullBox.bmaxs.y, m_CullBox.bmaxs.z),
//		Vector(m_CullBox.bmins.x, m_CullBox.bmins.y, m_CullBox.bmaxs.z),
//		Vector(m_CullBox.bmaxs.x, m_CullBox.bmins.y, m_CullBox.bmins.z),
//		Vector(m_CullBox.bmaxs.x, m_CullBox.bmaxs.y, m_CullBox.bmins.z),
//		Vector(m_CullBox.bmaxs.x, m_CullBox.bmins.y, m_CullBox.bmaxs.z),
//		Vector(m_CullBox.bmaxs.x, m_CullBox.bmaxs.y, m_CullBox.bmaxs.z),
//	};
//
//	BoundBox OldBox = m_CullBox;
//
//	m_CullBox.ResetBounds();
//	for (int i = 0; i < ARRAYSIZE(vecPoints); i++)
//	{
//		t->TranslatePoint(vecPoints[i]);
//		m_CullBox.UpdateBounds(vecPoints[i]);
//	}
//
//	Vector vecNewMin = m_Mins + (m_CullBox.bmins - OldBox.bmins);
//	Vector vecNewMax = m_Maxs + (m_CullBox.bmaxs - OldBox.bmaxs);
//
//	UpdateBox(vecNewMin, vecNewMax);
//
//	CMapClass::DoTransform(t);
//}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//			Scale - 
//-----------------------------------------------------------------------------
//void CMapAlignedBox::DoTransScale(const Vector &RefPoint, const Vector &Scale)
//{
//	if (!m_bWireframe)
//	{
//		// Only wireframe boxes can be scaled.
//		return;
//	}
//
//	BoundBox OldBox = m_CullBox;
//
//	m_CullBox.bmins = RefPoint + ((m_CullBox.bmins - RefPoint) * Scale);
//	m_CullBox.bmaxs = RefPoint + ((m_CullBox.bmaxs - RefPoint) * Scale);
//
//	Vector vecNewMin = m_Mins + (m_CullBox.bmins - OldBox.bmins);
//	Vector vecNewMax = m_Maxs + (m_CullBox.bmaxs - OldBox.bmaxs);
//
//	UpdateBox(vecNewMin, vecNewMax);
//
//	CMapClass::DoTransScale(RefPoint, Scale);
//}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecMins - 
//			&vecMaxs - 
//-----------------------------------------------------------------------------
//void CMapAlignedBox::UpdateBox(const Vector &vecMins, const Vector &vecMaxs)
//{
//	m_Mins = vecMins;
//	m_Maxs = vecMaxs;
//
//	CMapEntity *pEntity = dynamic_cast<CMapEntity *>(Parent);
//	if (pEntity != NULL)
//	{
//		char szValue[80];
//		sprintf(szValue, "%g %g %g", (double)m_Mins.x, (double)m_Mins.y, (double)m_Mins.z);
//		pEntity->NotifyChildKeyChanged(this, m_MinsKeyName, szValue);
//
//		sprintf(szValue, "%g %g %g", (double)m_Maxs.x, (double)m_Maxs.y, (double)m_Maxs.z);
//		pEntity->NotifyChildKeyChanged(this, m_MaxsKeyName, szValue);
//	}
//}

