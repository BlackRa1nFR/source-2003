//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Sprite.h"
#include "Material.h"			// FIXME: we need to work only with IEditorTexture!
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialSystem.h"
#include "Render3d.h"


SpriteCache_t CSpriteCache::m_Cache[SPRITE_CACHE_SIZE];
int CSpriteCache::m_nItems = 0;


//-----------------------------------------------------------------------------
// Purpose: Returns an instance of a particular studio model. If the model is
//			in the cache, a pointer to that model is returned. If not, a new one
//			is created and added to the cache.
// Input  : pszModelPath - Full path of the .MDL file.
//-----------------------------------------------------------------------------
CSpriteModel *CSpriteCache::CreateSprite(const char *pszSpritePath)
{
	//
	// First look for the sprite in the cache. If it's there, increment the
	// reference count and return a pointer to the cached sprite.
	//
	for (int i = 0; i < m_nItems; i++)
	{
		if (!stricmp(pszSpritePath, m_Cache[i].pszPath))
		{
			m_Cache[i].nRefCount++;
			return(m_Cache[i].pSprite);
		}
	}

	//
	// If it isn't there, try to create one.
	//
	CSpriteModel *pSprite = new CSpriteModel;

	if (pSprite != NULL)
	{
		if (!pSprite->LoadSprite(pszSpritePath))
		{
			delete pSprite;
			pSprite = NULL;
		}
	}

	//
	// If we successfully created it, add it to the cache.
	//
	if (pSprite != NULL)
	{
		CSpriteCache::AddSprite(pSprite, pszSpritePath);
	}

	return(pSprite);
}


//-----------------------------------------------------------------------------
// Purpose: Adds the model to the cache, setting the reference count to one.
// Input  : pModel - Model to add to the cache.
//			pszSpritePath - The full path of the .MDL file, which is used as a
//				key in the sprite cache.
// Output : Returns TRUE if the sprite was successfully added, FALSE if we ran
//			out of memory trying to add the sprite to the cache.
//-----------------------------------------------------------------------------
bool CSpriteCache::AddSprite(CSpriteModel *pSprite, const char *pszSpritePath)
{
	//
	// Copy the sprite pointer.
	//
	m_Cache[m_nItems].pSprite = pSprite;

	//
	// Allocate space for and copy the model path.
	//
	m_Cache[m_nItems].pszPath = new char [strlen(pszSpritePath) + 1];
	if (m_Cache[m_nItems].pszPath != NULL)
	{
		strcpy(m_Cache[m_nItems].pszPath, pszSpritePath);
	}
	else
	{
		return(false);
	}

	m_Cache[m_nItems].nRefCount = 1;

	m_nItems++;

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Increments the reference count on a sprite in the cache. Called by
//			client code when a pointer to the sprite is copied, making that
//			reference independent.
// Input  : pModel - Sprite for which to increment the reference count.
//-----------------------------------------------------------------------------
void CSpriteCache::AddRef(CSpriteModel *pSprite)
{
	for (int i = 0; i < m_nItems; i++)
	{
		if (m_Cache[i].pSprite == pSprite)
		{
			m_Cache[i].nRefCount++;
			return;
		}
	}	
}


//-----------------------------------------------------------------------------
// Purpose: Called by client code to release an instance of a model. If the
//			model's reference count is zero, the model is freed.
// Input  : pModel - Pointer to the model to release.
//-----------------------------------------------------------------------------
void CSpriteCache::Release(CSpriteModel *pSprite)
{
	for (int i = 0; i < m_nItems; i++)
	{
		if (m_Cache[i].pSprite == pSprite)
		{
			m_Cache[i].nRefCount--;
			ASSERT(m_Cache[i].nRefCount >= 0);

			//
			// If this model is no longer referenced, free it and remove it
			// from the cache.
			//
			if (m_Cache[i].nRefCount <= 0)
			{
				//
				// Free the path, which was allocated by AddModel.
				//
				delete [] m_Cache[i].pszPath;
				delete m_Cache[i].pSprite;

				//
				// Decrement the item count and copy the last element in the cache over
				// this element.
				//
				m_nItems--;

				m_Cache[i].pSprite = m_Cache[m_nItems].pSprite;
				m_Cache[i].pszPath = m_Cache[m_nItems].pszPath;
				m_Cache[i].nRefCount = m_Cache[m_nItems].nRefCount;
			}

			break;
		}
	}	
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CSpriteModel::CSpriteModel(void) : m_pMaterial(0), 	m_NumFrames(-1)
{
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees the sprite image and descriptor.
//-----------------------------------------------------------------------------
CSpriteModel::~CSpriteModel(void)
{
	delete m_pMaterial;
}


//-----------------------------------------------------------------------------
// Sets the render mode
//-----------------------------------------------------------------------------
void CSpriteModel::SetRenderMode( int mode )
{
	if (m_pMaterial && m_pRenderModeVar)
	{
		m_pRenderModeVar->SetIntValue( mode );
		m_pMaterial->GetMaterial()->RecomputeStateSnapshots();
	}
}


//-----------------------------------------------------------------------------
// Binds a sprite
//-----------------------------------------------------------------------------
void CSpriteModel::Bind( CRender3D* pRender, int frame )
{
	if (m_pMaterial && m_pFrameVar)
	{
		m_pFrameVar->SetIntValue( frame );
		pRender->BindTexture( m_pMaterial );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads a sprite material.
// Input  : pszSpritePath - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSpriteModel::LoadSprite(const char *pszSpritePath)
{
	m_pMaterial = CMaterial::CreateMaterial( pszSpritePath, true );

	if( m_pMaterial )
	{
		m_Width = m_pMaterial->GetWidth();
		m_Height = m_pMaterial->GetHeight();
		// FIXME: m_NumFrames = m_pMaterial->GetMaterial()->GetNumAnimationFrames();
		m_pFrameVar = m_pMaterial->GetMaterial()->FindVar( "$spriteFrame", 0 );
		m_pRenderModeVar = m_pMaterial->GetMaterial()->FindVar( "$spriterendermode", 0 );

		bool found;
		IMaterialVar *orientationVar = m_pMaterial->GetMaterial()->FindVar( "$spriteOrientation", &found, false );
		if( found )
		{
			m_Type = orientationVar->GetIntValue();
		}
		else
		{
			m_Type = SPR_VP_PARALLEL_UPRIGHT;
		}

		IMaterialVar *pOriginVar = m_pMaterial->GetMaterial()->FindVar( "$spriteorigin", &found );
		Vector origin;
		if( !found || ( pOriginVar->GetType() != MATERIAL_VAR_TYPE_VECTOR ) )
		{
			origin[0] = -m_Width * 0.5f;
			origin[1] = m_Height * 0.5f;
		}
		else
		{
			Vector originVarValue;
			pOriginVar->GetVecValue( originVarValue.Base(), 3);
			origin[0] = -m_Width * originVarValue[0];
			origin[1] = m_Height * originVarValue[1];
		}

		m_Up = origin[1];
		m_Down = origin[1] - m_Height;
		m_Left = origin[0];
		m_Right = m_Width + origin[0];
	}

	return m_pMaterial != 0;
}


//-----------------------------------------------------------------------------
// Kind of a hack...
//-----------------------------------------------------------------------------
int CSpriteModel::GetFrameCount()
{
	// FIXME: Figure out the correct time to cache in this info
	if ((m_NumFrames < 0) && m_pMaterial)
	{
		m_NumFrames = m_pMaterial->GetMaterial()->GetNumAnimationFrames();
	}
	return (m_NumFrames < 0) ? 0 : m_NumFrames;
}