//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPPOINTHANDLE_H
#define MAPPOINTHANDLE_H
#ifdef _WIN32
#pragma once
#endif

#include "MapHelper.h"
#include "ToolInterface.h"


class CHelperInfo;
class CRender2D;
class CRender3D;
class CMapPointHandle;


#define MAX_KEYNAME_SIZE	32


#define HANDLE_RADIUS	8


class CMapPointHandle : public CMapHelper
{

friend class CToolPointHandle;
friend class CMapAxisHandle;

typedef CMapHelper BaseClass;

public:

	DECLARE_MAPCLASS(CMapPointHandle)

	//
	// Factory for building from a list of string parameters.
	//
	static CMapClass *Create(CHelperInfo *pInfo, CMapEntity *pParent);

	inline int GetRadius(void);

	//
	// Construction/destruction:
	//
	CMapPointHandle(void);
	CMapPointHandle(const char *pszKey, bool bDrawLineToParent);
	~CMapPointHandle(void);

	void CalcBounds(BOOL bFullUpdate = FALSE);

	virtual CMapClass *Copy(bool bUpdateDependencies);
	virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

	virtual void Render2D(CRender2D *pRender);
	virtual void Render3D(CRender3D *pRender);

	virtual int SerializeRMF(fstream &File, BOOL bRMF);
	virtual int SerializeMAP(fstream &File, BOOL bRMF);

	// Overridden because origin helpers don't take the color of their parent entity.
	virtual void SetRenderColor(unsigned char red, unsigned char green, unsigned char blue);
	virtual void SetRenderColor(color32 rgbColor);

	virtual CMapClass *HitTest2D(CMapView2D *pView, const Vector2D &point, int &nHitData);

	virtual bool IsVisualElement(void) { return(false); } // Only visible if our parent is selected.
	virtual bool IsClutter(void) { return true; }
	virtual bool IsCulledByCordon(void) { return false; } // We don't hide unless our parent hides.
	
	virtual LPCTSTR GetDescription() { return("Point helper"); }

	virtual void OnAddToWorld(CMapWorld *pWorld);
	virtual void OnParentKeyChanged(const char *key, const char *value);
	virtual void OnUndoRedo(void);

	virtual void PostloadWorld(CMapWorld *pWorld);

	virtual CBaseTool *GetToolObject(int nHitData);

protected:

	// Called by the point handle tool while dragging.
	void UpdateOrigin(const Vector &vecOrigin);

	// Overridden to update our parent's keyvalue when we move.
	virtual void DoTransform(Box3D *t);
	virtual void DoTransMove(const Vector &Delta);
	virtual void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
	virtual void DoTransScale(const Vector &RefPoint, const Vector &Scale);
	virtual void DoTransFlip(const Vector &RefPoint);

private:

	void Initialize(void);
	void UpdateParentKey(void);

	char m_szKeyName[MAX_KEYNAME_SIZE];
	bool m_bDrawLineToParent;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMapPointHandle::GetRadius(void)
{
	return HANDLE_RADIUS;
}


#endif // MAPPOINTHANDLE_H
