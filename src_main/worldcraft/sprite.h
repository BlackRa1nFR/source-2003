//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef SPRITE_H
#define SPRITE_H
#pragma once

#include "mathlib.h"

class CTexture;
class CMaterial;
class IMaterialVar;
class CRender3D;


// must match definition in modelgen.h
enum synctype_t
{
	ST_SYNC=0, 
	ST_RAND 
};

#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4

#define SPR_NORMAL					0
#define SPR_ADDITIVE				1
#define SPR_INDEXALPHA				2
#define SPR_ALPHATEST				3


//-----------------------------------------------------------------------------
// From engine\GL_MODEL.H:
//-----------------------------------------------------------------------------

class CSpriteModel
{
	public:

		CSpriteModel(void);
		~CSpriteModel(void);
			
		bool LoadSprite(const char *pszSpritePath);

		int GetFrameCount(void);
		int GetWidth() const;
		int GetHeight() const;
		int GetType() const;

		void Bind( CRender3D* pRender, int frame );
		void GetRect( float& left, float& up,  float& right, float& down ) const;
		void SetRenderMode( int mode );

	protected:
		CMaterial*		m_pMaterial;
		IMaterialVar*	m_pFrameVar;
		IMaterialVar*	m_pRenderModeVar;
		int				m_NumFrames;
		int				m_Type;
		int				m_Width;
		int				m_Height;
		float			m_Up;
		float			m_Down;
		float			m_Left;
		float			m_Right;
};


//-----------------------------------------------------------------------------
// inline methods
//-----------------------------------------------------------------------------

inline int CSpriteModel::GetWidth() const
{
	return m_Width;
}

inline int CSpriteModel::GetHeight() const
{
	return m_Height;
}

inline int CSpriteModel::GetType() const
{
	return m_Type;
}

inline void CSpriteModel::GetRect( float& left, float& up, float& right, float& down ) const
{
	left = m_Left;
	right = m_Right;
	up = m_Up;
	down = m_Down;
}

//-----------------------------------------------------------------------------
// Sprite cache
//-----------------------------------------------------------------------------

struct SpriteCache_t
{
	CSpriteModel *pSprite;
	char *pszPath;
	int nRefCount;
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define SPRITE_CACHE_SIZE	1024

class CSpriteCache
{
	public:

		static CSpriteModel *CreateSprite(const char *pszSpritePath);
		static void AddRef(CSpriteModel *pSprite);
		static void Release(CSpriteModel *pSprite);

	protected:

		static bool AddSprite(CSpriteModel *pSprite, const char *pszSpritePath);
		static void RemoveSprite(CSpriteModel *pSprite);

		static SpriteCache_t m_Cache[SPRITE_CACHE_SIZE];
		static int m_nItems;
};


#endif // SPRITE_H

