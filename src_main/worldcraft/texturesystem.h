//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Texture management functions. Exposes a list of available textures,
//			texture groups, and Most Recently Used textures.
//
// $NoKeywords: $
//=============================================================================

#ifndef TEXTURESYSTEM_H
#define TEXTURESYSTEM_H
#pragma once


#include <afxtempl.h>
#include "BlockArray.h"
#include "IEditorTexture.h"
#include "Material.h"

//-----------------------------------------------------------------------------
// Purpose: Defines the interface to a set of textures of a given texture format.
//			The textures are stored as an index into the global array of textures.
//-----------------------------------------------------------------------------
class CTextureGroup
{
public:
	CTextureGroup(const char *pszName);

	inline const char *GetName()
	{
		return(m_szName);
	}

	inline int GetCount(void)
	{
		return( m_nTextures );
	}

	inline TEXTUREFORMAT GetTextureFormat(void)
	{
		return(m_eTextureFormat);
	}

	inline void SetTextureFormat(TEXTUREFORMAT eTextureFormat)
	{
		m_eTextureFormat = eTextureFormat;
	}

	void AddTexture(IEditorTexture *pTexture);
	void Sort(void);

	IEditorTexture *GetTexture(int nIndex);
	IEditorTexture* GetTexture( char const* pName );

	// Used to lazily load in all the textures
	void LazyLoadTextures();

protected:
	char m_szName[MAX_PATH];
	TEXTUREFORMAT m_eTextureFormat;
	CTypedPtrArray<CPtrArray, IEditorTexture *> m_Textures;
	int m_nTextures;

	// Used to lazily load the textures in the group
	int	m_nTextureToLoad;
};


typedef struct tagGF
{
	CString filename;
	DWORD id;
	int fd;
	TEXTUREFORMAT format;
	BOOL bLoaded;

} GRAPHICSFILESTRUCT;


class CTextureSystem : public IMaterialEnumerator
{
public:
	CTextureSystem(void);
	virtual ~CTextureSystem(void);

	bool Initialize(const char *pszGameDir, HWND hwnd);
	void ShutDown(void);

	//
	// Exposes a list of all texture (WAD) files.
	//
	inline int FilesGetCount(void) const;
	inline void FilesGetInfo(GRAPHICSFILESTRUCT *pFileInfo, int nIndex) const;
	bool FindGraphicsFile(GRAPHICSFILESTRUCT *pFileInfo, DWORD id, int *piIndex = NULL);

	//
	// Exposes a list of all world textures.
	//
	inline int GetTextureCount(void) const;
	inline IEditorTexture *GetTexture(int nIndex);

	//
	// Exposes a list of texture groups (sets of textures of a given format).
	//
	inline POSITION GroupsGetHeadPosition(void) const;
	inline CTextureGroup *GroupsGetNext(POSITION &pos) const;
	void SetActiveGroup(const char *pcszName);

	//
	// Exposes a list of active textures based on the currently active texture group.
	//
	inline int GetActiveTextureCount(void) const;
	IEditorTexture *EnumActiveTextures(int *piIndex, TEXTUREFORMAT eDesiredFormat) const;
	IEditorTexture *FindActiveTexture(LPCSTR pszName, int *piIndex = NULL, BOOL bDummy = TRUE) const;
	BOOL IsGraphicsAvailable(TEXTUREFORMAT format);

	//
	// Exposes a list of Most Recently Used textures.
	//
	void AddMRU(IEditorTexture *pTex);
	inline POSITION MRUGetHeadPosition(void) const;
	inline IEditorTexture *MRUGetNext(POSITION &pos, TEXTUREFORMAT eDesiredFormat) const;

	//
	// Exposes a list of all unique keywords found in the master texture list.
	//
	POSITION KeywordsGetHeadPosition(void);
	char *KeywordsGetNext(POSITION &pos);

	//
	// Exposes a list of placeholder textures when texture files are missing.
	//
	IEditorTexture *AddDummy(LPCTSTR pszName, TEXTUREFORMAT eFormat);

	//
	// Load graphics files from options list.
	//
	void LoadAllGraphicsFiles(void);
	void InformPaletteChanged(void);

	// Used to add all the world materials into the material list
	bool EnumMaterial( const char *pMaterialName, int nContext );

	// Used to lazily load in all the textures
	void LazyLoadTextures();

	// Registers the keywords as existing in a particular material
	void RegisterTextureKeywords( IEditorTexture *pTexture );

protected:

	int AddTexture(IEditorTexture *pTexture);

	DWORD LoadGraphicsFile(CString filename);
	void LoadGraphicsFileWAD3(GRAPHICSFILESTRUCT *pFile, int fd, CTextureGroup *pGroup);
	void LoadGraphicsFileMaterials(void);

	int GraphicFileHandleFromID(DWORD id);

	//
	// Array of open graphics files.
	//
	CArray<GRAPHICSFILESTRUCT, GRAPHICSFILESTRUCT&> m_GraphicsFiles;

	//
	// Master array of textures.
	//
	BlockArray<IEditorTexture *, 64, 256> m_Textures;
	int m_nTextures;

	//
	// List of Dummies and MRU textures.
	//
	ITextureList m_Dummies;
	ITextureList m_MRU;

	//
	// List of groups (sets of textures of a given texture format). Only one
	// group can be active at a time, based on the game configuration.
	//
	CTypedPtrList<CPtrList, CTextureGroup *> m_Groups;
	CTextureGroup *m_pActiveGroup;
	CTextureGroup *m_pAllGroup;

	//
	// List of keywords found in all textures.
	//
	CTypedPtrList<CPtrList, char *> m_Keywords;
};


//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline int
//-----------------------------------------------------------------------------
int CTextureSystem::FilesGetCount(void) const
{
	return(m_GraphicsFiles.GetSize());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFileInfo - 
//			nIndex - 
// Output : inline void
//-----------------------------------------------------------------------------
void CTextureSystem::FilesGetInfo(GRAPHICSFILESTRUCT *pFileInfo, int nIndex) const
{
	if (pFileInfo != NULL)
	{
		*pFileInfo = m_GraphicsFiles[nIndex];
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the number of textures in the active group.
//-----------------------------------------------------------------------------
int CTextureSystem::GetActiveTextureCount(void) const
{
	if (m_pActiveGroup != NULL)
	{
		return(m_pActiveGroup->GetCount());
	}

	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the total number of textures in the texture system.
//-----------------------------------------------------------------------------
int CTextureSystem::GetTextureCount(void) const
{
	return(m_nTextures);
}


//-----------------------------------------------------------------------------
// Purpose: Retrieves a texture by index.
// Input  : nIndex - Index into master list of textures.
// Output : Returns a pointer to the requested texture.
//-----------------------------------------------------------------------------
IEditorTexture *CTextureSystem::GetTexture(int nIndex)
{
	return(m_Textures[nIndex]);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline POSITION
//-----------------------------------------------------------------------------
POSITION CTextureSystem::GroupsGetHeadPosition(void) const
{
	int nCount = m_Groups.GetCount();
	return(m_Groups.GetHeadPosition());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos - 
// Output : inline CTextureGroup
//-----------------------------------------------------------------------------
CTextureGroup *CTextureSystem::GroupsGetNext(POSITION &pos) const
{
	return(m_Groups.GetNext(pos));
}


//-----------------------------------------------------------------------------
// Purpose: Initiates an iteration of the MRU list.
//-----------------------------------------------------------------------------
POSITION CTextureSystem::MRUGetHeadPosition(void) const
{
	return(m_MRU.GetHeadPosition());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next texture in the MRU of the given format.
// Input  : pos - Iterator.
//			eDesiredFormat - Texture format to return.
// Output : Pointer to the texture.
//-----------------------------------------------------------------------------
IEditorTexture *CTextureSystem::MRUGetNext(POSITION &pos, TEXTUREFORMAT eDesiredFormat) const
{
	while (pos != NULL)
	{
		IEditorTexture *pTex = m_MRU.GetNext(pos);
		if (pTex->GetTextureFormat() == eDesiredFormat)
		{
			return(pTex);
		}
	}

	return(NULL);
}


extern CTextureSystem g_Textures;


#endif // TEXTURESYSTEM_H
