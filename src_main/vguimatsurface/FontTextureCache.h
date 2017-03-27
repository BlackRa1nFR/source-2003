//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FONTTEXTURECACHE_H
#define FONTTEXTURECACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_surfacelib/FontManager.h"
#include "UtlRBTree.h"
#include <vgui/ISurface.h>

class ITexture;

//-----------------------------------------------------------------------------
// Purpose: manages texture memory for unicode fonts in vgui
//-----------------------------------------------------------------------------
class CFontTextureCache
{
public:
	CFontTextureCache();
	~CFontTextureCache();

	// returns a texture ID and a pointer to an array of 4 texture coords for the given character & font
	// uploads more texture if necessary
	bool GetTextureForChar(vgui::HFont font, vgui::FontDrawType_t type, wchar_t wch, int *textureID, float **texCoords);

private:
	// NOTE: If you change this, change s_pFontPageSize
	enum
	{
		FONT_PAGE_SIZE_16,
		FONT_PAGE_SIZE_32,
		FONT_PAGE_SIZE_64,
		FONT_PAGE_SIZE_128,

		FONT_PAGE_SIZE_COUNT,
	};

	// a single character in the cache
	typedef unsigned short HCacheEntry;
	struct CacheEntry_t
	{
		vgui::HFont font;
		wchar_t wch;
		unsigned char page;
		float texCoords[4];

		HCacheEntry nextEntry;	// doubly-linked list for use in the LRU
		HCacheEntry prevEntry;
	};
	
	// a single texture page
	struct Page_t
	{
		short textureID[vgui::FONT_DRAW_TYPE_COUNT];
		short fontHeight;
		short wide, tall;	// total size of the page
		short nextX, nextY;	// position to draw any new character positions
	};

	// allocates a new page for a given character
	bool AllocatePageForChar(int charWide, int charTall, int &pageIndex, int &drawX, int &drawY, int &twide, int &ttall);

	// Creates font materials
	void CreateFontMaterials( Page_t &page, ITexture *pFontTexture );

	// Computes the page size given a character height
	int ComputePageType( int charTall ) const;

	static bool CacheEntryLessFunc(const CacheEntry_t &lhs, const CacheEntry_t &rhs);

	// cache
	typedef CUtlVector<Page_t> FontPageList_t;

	CUtlRBTree<CacheEntry_t, HCacheEntry> m_CharCache;
	FontPageList_t m_PageList;
	int m_pCurrPage[FONT_PAGE_SIZE_COUNT];
	HCacheEntry m_LRUListHeadIndex;

	static int s_pFontPageSize[FONT_PAGE_SIZE_COUNT];
};


#endif // FONTTEXTURECACHE_H
