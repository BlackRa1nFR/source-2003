//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VISGROUP_H
#define VISGROUP_H
#pragma once


#include "basetypes.h"


class CChunkFile;
class CSaveInfo;


enum ChunkFileResult_t;


class CVisGroup
{
	public:

		CVisGroup(void)
		{
			m_dwID = 0;

			m_rgbColor.r = 0;
			m_rgbColor.g = 0;
			m_rgbColor.b = 0;
			m_rgbColor.a = 0;
			
			m_bVisible = false;
			m_szName[0] = '\0';
		}

		DWORD GetID(void) { return(m_dwID); }
		void SetID(DWORD dwID) { m_dwID = dwID; }

		const char *GetName(void)
		{
			return(m_szName);
		}

		void SetName(const char *pszName)
		{
			if (pszName != NULL)
			{
				lstrcpyn(m_szName, pszName, sizeof(m_szName));
			}
		}

		inline color32 GetColor(void);
		inline void SetColor(color32 rgbColor);
		inline void SetColor(unsigned char red, unsigned char green, unsigned char blue);

		bool IsVisible(void);
		void SetVisible(bool bVisible) { m_bVisible = bVisible; }
		static bool IsShowAllActive(void);
		static void ShowAllVisGroups(bool bShow);

		//
		// Serialization.
		//
		ChunkFileResult_t LoadVMF(CChunkFile *pFile);
		ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);

	protected:

		static ChunkFileResult_t LoadKeyCallback(const char *szKey, const char *szValue, CVisGroup *pGroup);

		static bool g_bShowAll;

		char m_szName[128];
		color32 m_rgbColor;

		DWORD m_dwID;
		bool m_bVisible;
};


//-----------------------------------------------------------------------------
// Purpose: Returns the render color of this visgroup.
//-----------------------------------------------------------------------------
inline color32 CVisGroup::GetColor(void)
{
	return m_rgbColor;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the color of this visgroup.
//-----------------------------------------------------------------------------
inline void CVisGroup::SetColor(color32 rgbColor)
{
	m_rgbColor = rgbColor;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the color of this visgroup using RGB values.
//-----------------------------------------------------------------------------
inline void CVisGroup::SetColor(unsigned char red, unsigned char green, unsigned char blue)
{
	m_rgbColor.r = red;
	m_rgbColor.g = green;
	m_rgbColor.b = blue;
	m_rgbColor.a = 0;
}


#endif // VISGROUP_H
