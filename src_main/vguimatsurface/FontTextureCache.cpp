//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "FontTextureCache.h"
#include "MatSystemSurface.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include "vguimatsurfacestats.h"
#include "ImageLoader.h"
#include "vtf/vtf.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/ITexture.h"

extern CMatSystemSurface g_MatSystemSurface;

int CFontTextureCache::s_pFontPageSize[FONT_PAGE_SIZE_COUNT] = 
{
	16,
	32,
	64,
	128,
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFontTextureCache::CFontTextureCache() : m_CharCache(0, 256, CacheEntryLessFunc)
{
	CacheEntry_t listHead = { 0, 0 };
	m_LRUListHeadIndex = m_CharCache.Insert(listHead);

	m_CharCache[m_LRUListHeadIndex].nextEntry = m_LRUListHeadIndex;
	m_CharCache[m_LRUListHeadIndex].prevEntry = m_LRUListHeadIndex;

	for (int i = 0; i < FONT_PAGE_SIZE_COUNT; ++i)
	{
		m_pCurrPage[i] = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CFontTextureCache::~CFontTextureCache()
{
}

//-----------------------------------------------------------------------------
// Purpose: comparison function for cache entries
//-----------------------------------------------------------------------------
bool CFontTextureCache::CacheEntryLessFunc(CacheEntry_t const &lhs, CacheEntry_t const &rhs)
{
	if (lhs.font < rhs.font)
		return true;
	else if (lhs.font > rhs.font)
		return false;

	return (lhs.wch < rhs.wch);
}

//-----------------------------------------------------------------------------
// Purpose: returns the texture info for the given char & font
//-----------------------------------------------------------------------------
bool CFontTextureCache::GetTextureForChar(vgui::HFont font, vgui::FontDrawType_t type, wchar_t wch, int *textureID, float **texCoords)
{
	static CacheEntry_t cacheitem;
	
	if ( type == vgui::FONT_DRAW_DEFAULT )
	{
		type = g_MatSystemSurface.IsFontAdditive( font ) ? vgui::FONT_DRAW_ADDITIVE : vgui::FONT_DRAW_NONADDITIVE;
	}

	int typePage = (int)type - 1;
	typePage = clamp( typePage, 0, (int)vgui::FONT_DRAW_TYPE_COUNT - 1 );


	cacheitem.font = font;
	cacheitem.wch = wch;

	Assert( texCoords );

	*texCoords = cacheitem.texCoords;

	HCacheEntry cacheHandle = m_CharCache.Find(cacheitem);
	if (m_CharCache.IsValidIndex(cacheHandle))
	{
		// we have an entry already, return that
		int page = m_CharCache[cacheHandle].page;
		*textureID = m_PageList[page].textureID[typePage];
		*texCoords = m_CharCache[cacheHandle].texCoords;
		return true;
	}

	// need to generate a new char
	CWin32Font *winFont = FontManager().GetFontForChar(font, wch);
	if ( !winFont )
		return false;

	// get the char details
	int fontTall = winFont->GetHeight();
	int a, b, c;
	winFont->GetCharABCWidths(wch, a, b, c);
	int fontWide = b;

	// get a texture to render into
	int page, drawX, drawY, twide, ttall;
	if (!AllocatePageForChar(fontWide, fontTall, page, drawX, drawY, twide, ttall))
		return false;

	// create a buffer and render the character into it
	int nByteCount = s_pFontPageSize[FONT_PAGE_SIZE_COUNT-1] * s_pFontPageSize[FONT_PAGE_SIZE_COUNT-1] * 4;
	unsigned char* rgba = (unsigned char*)_alloca( nByteCount );
	memset(rgba, 0, nByteCount);
	winFont->GetCharRGBA(wch, 0, 0, fontWide, fontTall, rgba);

	// upload the new sub texture 
	// NOTE: both textureIDs reference the same ITexture, so we're ok)
	g_MatSystemSurface.DrawSetTexture(m_PageList[page].textureID[typePage]);
	g_MatSystemSurface.DrawSetSubTextureRGBA(m_PageList[page].textureID[typePage], drawX, drawY, rgba, fontWide, fontTall);

	// set the cache info
	cacheitem.page = page;
	
	double adjust =  0.0f; // the 0.5 texel offset is done in CMatSystemTexture::SetMaterial()

	cacheitem.texCoords[0] = (float)( (double)drawX / ((double)twide + adjust) );
	cacheitem.texCoords[1] = (float)( (double)drawY / ((double)ttall + adjust) );
	cacheitem.texCoords[2] = (float)( (double)(drawX + fontWide) / (double)twide );
	cacheitem.texCoords[3] = (float)( (double)(drawY + fontTall) / (double)ttall );
	m_CharCache.Insert(cacheitem);

	// return the data
	*textureID = m_PageList[page].textureID[typePage];
	// memcpy(texCoords, cacheitem.texCoords, sizeof(float) * 4);
	return true;
}


//-----------------------------------------------------------------------------
// Creates font materials
//-----------------------------------------------------------------------------
void CFontTextureCache::CreateFontMaterials( Page_t &page, ITexture *pFontTexture )
{
	// The normal material
	IMaterial *pMaterial = g_pMaterialSystem->CreateMaterial();
	pMaterial->SetShader( "UnlitGeneric" );
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_VERTEXCOLOR, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_VERTEXALPHA, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_NO_DEBUG_OVERRIDE, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_TRANSLUCENT, true ); 

	bool bFound;
	IMaterialVar *pMaterialVar = pMaterial->FindVar("$basetexture", &bFound, false );
	Assert( bFound );
	pMaterialVar->SetTextureValue( pFontTexture );

	// Take all of the new materialvars into account
	pMaterial->Refresh();

	int typePageNonAdditive = (int)vgui::FONT_DRAW_NONADDITIVE-1;

	g_MatSystemSurface.DrawSetTextureMaterial( page.textureID[typePageNonAdditive], pMaterial );
	pMaterial->DecrementReferenceCount();

	// The additive material
	pMaterial = g_pMaterialSystem->CreateMaterial();
	pMaterial->SetShader( "UnlitGeneric" );
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_VERTEXCOLOR, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_VERTEXALPHA, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_NO_DEBUG_OVERRIDE, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_TRANSLUCENT, true ); 
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_ADDITIVE, true ); 

	pMaterialVar = pMaterial->FindVar("$basetexture", &bFound, false );
	Assert( bFound );
	pMaterialVar->SetTextureValue( pFontTexture );

	// Take all of the new materialvars into account
	pMaterial->Refresh();

	int typePageAdditive = (int)vgui::FONT_DRAW_ADDITIVE-1;
	g_MatSystemSurface.ReferenceProceduralMaterial( page.textureID[typePageAdditive], page.textureID[typePageNonAdditive], pMaterial );
	pMaterial->DecrementReferenceCount();
}


//-----------------------------------------------------------------------------
// Computes the page size given a character height
//-----------------------------------------------------------------------------
int CFontTextureCache::ComputePageType( int charTall ) const
{
	for (int i = 0; i < FONT_PAGE_SIZE_COUNT; ++i)
	{
		if (charTall < s_pFontPageSize[i] )
			return i;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: allocates a new page for a given character
//-----------------------------------------------------------------------------
bool CFontTextureCache::AllocatePageForChar(int charWide, int charTall, int &pageIndex, int &drawX, int &drawY, int &twide, int &ttall)
{
	// see if there is room in the last page for this character
	int nPageType = ComputePageType( charTall );
	pageIndex = m_pCurrPage[nPageType];

	int nNextX = 0;
	bool bNeedsNewPage = true;
	if (pageIndex > -1)
	{
		Page_t &page = m_PageList[ pageIndex ];

		nNextX = page.nextX + charWide;

		// make sure we have room on the current line of the texture page
		if (nNextX > page.wide)
		{
			// move down a line
			page.nextX = 0;
			nNextX = charWide;
			page.nextY += page.fontHeight + 1;
		}

		bNeedsNewPage = ((page.nextY + page.fontHeight + 1) > page.tall);
	}
	
	if (bNeedsNewPage)
	{
		// allocate a new page
		pageIndex = m_PageList.AddToTail();
		Page_t &newPage = m_PageList[pageIndex];
		m_pCurrPage[nPageType] = pageIndex;

		for (int i = 0; i < FONT_DRAW_TYPE_COUNT; ++i )
		{
			newPage.textureID[i] = g_MatSystemSurface.CreateNewTextureID( true );
		}

		newPage.fontHeight = s_pFontPageSize[nPageType];
		newPage.wide = 256;
		newPage.tall = 256;
		newPage.nextX = 0;
		newPage.nextY = 0;

		nNextX = charWide;

		ITexture *pTexture = g_pMaterialSystem->CreateProceduralTexture( "", 256, 256, IMAGE_FORMAT_RGBA8888, 
			TEXTUREFLAGS_POINTSAMPLE | TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | 
			TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_PROCEDURAL | TEXTUREFLAGS_SINGLECOPY );

		CreateFontMaterials( newPage, pTexture );

		pTexture->DecrementReferenceCount();

		// upload a purely black page (only need to do this once since they share textures)
		unsigned char rgba[256 * 256 * 4];
		memset(rgba, 0, sizeof(rgba));
		int typePageNonAdditive = (int)(vgui::FONT_DRAW_NONADDITIVE)-1;
		g_MatSystemSurface.DrawSetTextureRGBA(newPage.textureID[typePageNonAdditive], rgba, newPage.wide, newPage.tall, false, false);
	}

	// output the position
	Page_t &page = m_PageList[ pageIndex ];
	drawX = page.nextX;
	drawY = page.nextY;
	twide = page.wide;
	ttall = page.tall;

	// Update the next position to draw in
	page.nextX = nNextX + 1;

	return true;
}
