//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPSPRITE_H
#define MAPSPRITE_H
#pragma once


#include "MapHelper.h"
#include "Sprite.h"


class CRender3D;
class CSpriteModel;


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CMapSprite : public CMapHelper
{
	public:

		//
		// Factories.
		//
		static CMapClass *CreateMapSprite(CHelperInfo *pHelperInfo, CMapEntity *pParent);
		static CMapSprite *CreateMapSprite(const char *pszSpritePath);

		//
		// Construction/destruction:
		//
		CMapSprite(void);
		~CMapSprite(void);

		DECLARE_MAPCLASS(CMapSprite)

		void CalcBounds(BOOL bFullUpdate = FALSE);

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		void Initialize(void);

		void Render3D(CRender3D *pRender);

		void GetAngles(QAngle &Angles);

		int SerializeRMF(fstream &File, BOOL bRMF);
		int SerializeMAP(fstream &File, BOOL bRMF);

		static void SetRenderDistance(float fRenderDistance);
		static void EnableAnimation(BOOL bEnable);

		bool ShouldRenderLast(void);

		bool IsVisualElement(void) { return(true); }
		
		LPCTSTR GetDescription() { return("Sprite"); }

		void OnParentKeyChanged(LPCSTR szKey, LPCSTR szValue);

	protected:

		//
		// Implements CMapAtom transformation functions.
		//
		void DoTransform(Box3D *t);
		void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
		void DoTransFlip(const Vector &RefPoint);

		int  GetNextSpriteFrame( CRender3D* pRender );
		void SetRenderMode( int mode );
		void SpriteColor(unsigned char *pColor, int eRenderMode, colorVec RenderColor, int alpha);
		void GetSpriteAxes(QAngle& Angles, int type, Vector& forward, Vector& right, Vector& up, Vector& ViewUp, Vector& ViewRight, Vector& ViewForward);

		QAngle m_Angles;
		CSpriteModel *m_pSpriteInfo;	// Pointer to a sprite model in the cache.
		int m_nCurrentFrame;			// Current sprite frame for rendering.
		float m_fSecondsPerFrame;		// How many seconds to render each frame before advancing.
		float m_fElapsedTimeThisFrame;	// How many seconds we have rendered this sprite frame so far.
		float m_fScale;					// Sprite scale along sprite axes.
		int m_eRenderMode;				// Our render mode (transparency, etc.).
		colorVec m_RenderColor;			// Our render color.
		bool m_bIsIcon;					// If true, this sprite is an iconic representation of an entity.
};

#endif // MAPSPRITE_H
