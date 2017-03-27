//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Texture management functions. Exposes a list of available textures,
//			texture groups, and Most Recently Used textures.
//
//=============================================================================

#include "stdafx.h"
#include <process.h>
#include <afxtempl.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include "DummyTexture.h"		// Specific IEditorTexture implementation
#include "FaceEditSheet.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "Material.h"			// Specific IEditorTexture implementation
#include "Options.h"
#include "TextureSystem.h"
#include "WADTexture.h"			// Specific IEditorTexture implementation
#include "WADTypes.h"
#include "Worldcraft.h"


#pragma warning(disable:4244)


#define _GraphicCacheAllocate(n)	malloc(n)
#define IsSortChr(ch) ((ch == '-') || (ch == '+'))


//-----------------------------------------------------------------------------
// Stuff for loading WAD3 files.
//-----------------------------------------------------------------------------
typedef struct
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} WAD3lumpinfo_t;


//-----------------------------------------------------------------------------
// List of global graphics
//-----------------------------------------------------------------------------
CTextureSystem g_Textures;


//-----------------------------------------------------------------------------
// Purpose: Constructor. Creates the "All" group and sets it as the active group.
//-----------------------------------------------------------------------------
CTextureSystem::CTextureSystem(void)
{
	m_pAllGroup = m_pActiveGroup = new CTextureGroup("All Textures");
	m_Groups.AddTail(m_pAllGroup);

	m_nTextures = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees the list of groups and dummy textures.
//-----------------------------------------------------------------------------
CTextureSystem::~CTextureSystem(void)
{
	//
	// Delete all the texture groups.
	//
	POSITION p = m_Groups.GetHeadPosition();
	while(p)
	{
		delete m_Groups.GetNext(p);
	}

	//
	// Delete dummy textures.
	//
	POSITION pos = m_Dummies.GetHeadPosition();
	while (pos != NULL)
	{
		IEditorTexture *pTex = m_Dummies.GetNext(pos);
		delete pTex;
	}
	m_Dummies.RemoveAll();

	//
	// Delete all the textures from the master list.
	//
	for (int i = 0; i < m_nTextures; i++)
	{
		IEditorTexture *pTex = m_Textures[i];
		delete pTex;
	}

	//
	// Delete the keywords.
	//
	pos = m_Keywords.GetHeadPosition();
	while (pos != NULL)
	{
		char *pszKeyword = m_Keywords.GetNext(pos);
		delete pszKeyword;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Adds a texture to the master list of textures.
// Input  : pTexture - Pointer to texture to add.
// Output : Returns the index of the texture in the master texture list.
//-----------------------------------------------------------------------------
int CTextureSystem::AddTexture(IEditorTexture *pTexture)
{
	int nIndex = m_nTextures;

	m_Textures[nIndex] = pTexture;
	m_nTextures++;

	return(nIndex);
}


//-----------------------------------------------------------------------------
// Purpose: Begins iterating the list of texture/material keywords.
//-----------------------------------------------------------------------------
POSITION CTextureSystem::KeywordsGetHeadPosition(void)
{
	return(m_Keywords.GetHeadPosition());
}


//-----------------------------------------------------------------------------
// Purpose: Continues iterating the list of texture/material keywords.
//-----------------------------------------------------------------------------
char *CTextureSystem::KeywordsGetNext(POSITION &pos)
{
	return(m_Keywords.GetNext(pos));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *piIndex - 
//			bUseMRU - 
// Output : 
//-----------------------------------------------------------------------------
IEditorTexture *CTextureSystem::EnumActiveTextures(int *piIndex, TEXTUREFORMAT eDesiredFormat) const
{
	ASSERT(piIndex != NULL);
	
	if (piIndex != NULL)
	{
		if (m_pActiveGroup != NULL)
		{
			IEditorTexture *pTex = NULL;

			do
			{
				pTex = m_pActiveGroup->GetTexture(*piIndex);
				if (pTex != NULL)
				{
					(*piIndex)++;

					if ((eDesiredFormat == tfNone) || (pTex->GetTextureFormat() == eDesiredFormat))
					{
						return(pTex);
					}
				}
			} while (pTex != NULL);
		}
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Initializes the texture system.
//-----------------------------------------------------------------------------
bool CTextureSystem::Initialize(const char *pszGameDir, HWND hwnd)
{
	CWADTexture::Initialize();

	//
	// Initialize the materials system.
	//
	#ifndef SDK_BUILD
	if (pszGameDir != NULL)
	{
		char szMaterialsDir[MAX_PATH];
		_snprintf(szMaterialsDir, MAX_PATH-1, "%s\\materials", pszGameDir);

		CMaterial::Initialize(szMaterialsDir, hwnd);
	}
	#endif // SDK_BUILD

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Shuts down the texture system.
//-----------------------------------------------------------------------------
void CTextureSystem::ShutDown(void)
{
	CWADTexture::ShutDown();

	#ifndef SDK_BUILD
	CMaterial::ShutDown();
	#endif // SDK_BUILD
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			piIndex - 
//			bDummy - 
// Output : 
//-----------------------------------------------------------------------------
IEditorTexture *CTextureSystem::FindActiveTexture(LPCSTR pszName, int *piIndex, BOOL bDummy) const
{
	static IEditorTexture *pLastTex = NULL;
	static int nLastIndex = -1;

	//
	// Check the cache first.
	//
	if (pLastTex && !stricmp(pszName, pLastTex->GetName()))
	{
		if (piIndex)
		{
			*piIndex = nLastIndex;
		}

		return(pLastTex);
	}

	int iIndex = 0;
	IEditorTexture *pTex;

	while (pTex = EnumActiveTextures(&iIndex, g_pGameConfig->GetTextureFormat()))
	{
		if (!strcmpi(pszName, pTex->GetName()))
		{
			if (piIndex)
			{
				*piIndex = iIndex;
			}

			pLastTex = pTex;
			nLastIndex = iIndex;

			return(pTex);
		}
	}

	//
	// Let's try again, this time with \textures\ decoration
	// TODO: remove this?
	//
	{
		iIndex = 0;
		char szBuf[512];

		sprintf(szBuf, "textures\\%s", pszName);

		for (int i = strlen(szBuf) -1; i >= 0; i--)
		{
			if (szBuf[i] == '/')
				szBuf[i] = '\\';
		}

		strlwr(szBuf);

		while (pTex = EnumActiveTextures(&iIndex, g_pGameConfig->GetTextureFormat()))
		{
			if (strstr(pTex->GetName(), szBuf))
			{
				if (piIndex)
				{
					*piIndex = iIndex;
				}

				pLastTex = pTex;
				nLastIndex = iIndex;

				return(pTex);
			}
		}
	}

	//
	// Caller doesn't want dummies.
	//
	if (!bDummy)
	{
		return(NULL);
	}

	ASSERT(!piIndex);

	//
	// Check the list of dummies for a texture with the same name and texture format.
	//
	POSITION p = m_Dummies.GetHeadPosition();
	while (p)
	{
		IEditorTexture *pTex = m_Dummies.GetNext(p);
		if ((!strcmpi(pszName, pTex->GetName())) && (pTex->GetTextureFormat() == g_pGameConfig->GetTextureFormat()))
		{
			pLastTex = pTex;
			nLastIndex = -1;
			return(pTex);
		}
	}

	//
	// Not found; add a dummy as a placeholder for the missing texture.
	//
	pTex = g_Textures.AddDummy(pszName, g_pGameConfig->GetTextureFormat());

	if (pTex != NULL)
	{
		pLastTex = pTex;
		nLastIndex = -1;
	}

	return(pTex);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTex - 
//-----------------------------------------------------------------------------
void CTextureSystem::AddMRU(IEditorTexture *pTex)
{
	POSITION p;

	if (p = m_MRU.Find(pTex))
	{
		m_MRU.RemoveAt(p);
	}
	else if (m_MRU.GetCount() == 8)
	{
		m_MRU.RemoveTail();
	}

	m_MRU.AddHead(pTex);
}


//-----------------------------------------------------------------------------
// Purpose: Change palette on all textures.
// Input  : 
// dvs: need to handle a palette change for Quake support
//-----------------------------------------------------------------------------
void CTextureSystem::InformPaletteChanged()
{
//	int nGraphics = GetCount();
//
//	for (int i = 0; i < nGraphics; i++)
//	{
//		IEditorTexture *pTex = &GetAt(i);
//	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *pcszName - 
//-----------------------------------------------------------------------------
void CTextureSystem::SetActiveGroup(const char *pcszName)
{
	int iCount = m_Groups.GetCount();
	CTextureGroup *pGroup;
	char szBuf[MAX_PATH];
	sprintf(szBuf, "textures\\%s", pcszName);

	for (int i = 0; i < iCount; i++)
	{
		POSITION p = m_Groups.FindIndex(i);
		ASSERT(p);
		pGroup = m_Groups.GetAt(p);
		if (!strcmpi(pGroup->GetName(), pcszName))
		{
			m_pActiveGroup = pGroup;
			return;
		}

		if (strstr(pGroup->GetName(), pcszName))
		{
			m_pActiveGroup = pGroup;
			return;
		}

	}
	TRACE0("No Group Found!");
}


//-----------------------------------------------------------------------------
// Purpose: Loads textures from all texture files.
//-----------------------------------------------------------------------------
void CTextureSystem::LoadAllGraphicsFiles(void)
{ 
	for (int i = 0; i < Options.textures.nTextureFiles; i++)
	{
		LoadGraphicsFile(Options.textures.TextureFiles[i]);
	}

	#ifndef SDK_BUILD
	LoadGraphicsFileMaterials();
	#endif // SDK_BUILD

	m_pAllGroup->Sort();

	//
	// Tell GUI we've changed.
	// dvs: it would be better to do this from the calling code.
	//
	if (GetMainWnd())
	{
		GetMainWnd()->m_TextureBar.NotifyGraphicsChanged();
		GetMainWnd()->m_pFaceEditSheet->NotifyGraphicsChanged();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Adds a placeholder texture for a texture that exists in the map, but
//			was not found on disk.
// Input  : pszName - Name of missing texture.
// Output : Returns a pointer to the new dummy texture.
//-----------------------------------------------------------------------------
IEditorTexture *CTextureSystem::AddDummy(LPCTSTR pszName, TEXTUREFORMAT eFormat)
{
	IEditorTexture *pTex = new CDummyTexture(pszName, eFormat);
	m_Dummies.AddTail(pTex);

	return(pTex);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : elem1 - 
//			elem2 - 
// Output : static int __cdecl
//-----------------------------------------------------------------------------
static int __cdecl SortGraphicsProc(const void *elem1, const void *elem2)
{
	IEditorTexture *pElem1 = *((IEditorTexture **)elem1);
	IEditorTexture *pElem2 = *((IEditorTexture **)elem2);

	ASSERT((pElem1 != NULL) && (pElem2 != NULL));
	if ((pElem1 == NULL) || (pElem2 == NULL))
	{
		return(0);
	}

	const char *pszName1 = pElem1->GetName();
	const char *pszName2 = pElem2->GetName();

	char ch1 = pszName1[0];
	char ch2 = pszName2[0];

	if (IsSortChr(ch1) && !IsSortChr(ch2))
	{
		int iFamilyLen = strlen(pszName1+2);
		int iFamily = strnicmp(pszName1+2, pszName2, iFamilyLen);
		if (!iFamily)
		{
			return(-1);	// same family - put elem1 before elem2
		}
		return(iFamily);	// sort normally
	}
	else if (!IsSortChr(ch1) && IsSortChr(ch2))
	{
		int iFamilyLen = strlen(pszName2+2);
		int iFamily = strnicmp(pszName1, pszName2+2, iFamilyLen);
		if (!iFamily)
		{
			return(1);	// same family - put elem2 before elem1
		}
		return(iFamily);	// sort normally
	}
	else if (IsSortChr(ch1) && IsSortChr(ch2))
	{
		// do family name sorting
		int iFamily = strcmpi(pszName1+2, pszName2+2);

		if (!iFamily)
		{
			// same family - sort by number
			return pszName1[1] - pszName2[1];
		}

		// different family
		return(iFamily);
	}

	return(strcmpi(pszName1, pszName2));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sizeSrc - 
//			sizeDest - 
//			*src - 
//			*dest - 
//-----------------------------------------------------------------------------
void ScaleBitmap(CSize sizeSrc, CSize sizeDest, char *src, char *dest)
{
    int i;
    int e_y = (sizeSrc.cy << 1) - sizeDest.cy;
    int sizeDest2_y = (sizeDest.cy << 1);
	int sizeSrc2_y = sizeSrc.cy << 1;
	int srcline = 0, destline = 0;
	char *srclinep, *destlinep;
	int e_x = (sizeSrc.cx << 1) - sizeDest.cx;
	int sizeDest2_x = (sizeDest.cx << 1);
	int sizeSrc2_x = sizeSrc.cx << 1;

    for( i = 0; i < sizeDest.cy; i++ )
    {
		// scale by X
		{
			srclinep = src + (srcline * sizeSrc.cx);
			destlinep = dest + (destline * sizeDest.cx);

			int i;

			for( i = 0; i < sizeDest.cx; i++ )
			{
				*destlinep = *srclinep;

				while( e_x >= 0 )
				{
					++srclinep;
					e_x -= sizeDest2_x;
				}

				++destlinep;
				e_x += sizeSrc2_x;
			}
		}

        while( e_y >= 0 )
        {
            ++srcline;
            e_y -= sizeDest2_y;
        }

        ++destline;
        e_y += sizeSrc2_y;
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
// Output : int
//-----------------------------------------------------------------------------
int CTextureSystem::GraphicFileHandleFromID(DWORD id)
{
	GRAPHICSFILESTRUCT FileInfo;
	if (FindGraphicsFile(&FileInfo, id))
	{
		return(FileInfo.fd);
	}

	return(-1);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//			*piIndex - 
// Output : GRAPHICSFILESTRUCT *
//-----------------------------------------------------------------------------
bool CTextureSystem::FindGraphicsFile(GRAPHICSFILESTRUCT *pFileInfo, DWORD id, int *piIndex)
{
	for (int i = 0; i < m_GraphicsFiles.GetSize(); i++)
	{
		if (m_GraphicsFiles[i].id == id)
		{
			if (piIndex)
			{
				piIndex[0] = i;
			}

			if (pFileInfo != NULL)
			{
				*pFileInfo = m_GraphicsFiles[i];
			}

			return(true);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			fd - 
//			pGroup - 
//-----------------------------------------------------------------------------
void CTextureSystem::LoadGraphicsFileWAD3(GRAPHICSFILESTRUCT *pFile, int fd, CTextureGroup *pGroup)
{
	// read wad header
	wadinfo_t hdr;
	_lseek(fd, 0, SEEK_SET);
	_read(fd, (char*)&hdr, sizeof hdr);

	_lseek(fd, hdr.infotableofs, SEEK_SET);

	// allocate directory memory.
	WAD3lumpinfo_t *dir = new WAD3lumpinfo_t[hdr.numlumps];
		
	// read entries.
	_read(fd, dir, sizeof(WAD3lumpinfo_t) * hdr.numlumps);

	// load graphics!
	for (int i = 0; i < hdr.numlumps; i++)
	{
		if (dir[i].type == TYP_MIPTEX)
		{
			_lseek(fd, dir[i].filepos, SEEK_SET);

			CWADTexture *pNew = new CWADTexture;
			if (pNew != NULL)
			{
				if (pNew->Init(fd, pFile->id, FALSE, dir[i].name))
				{
					pNew->SetTextureFormat(pFile->format);

					//
					// Add the texture to master list of textures.
					//
					int nIndex = AddTexture(pNew);

					//
					// Add the texture's index to the given group and to the "All" group.
					//
					pGroup->AddTexture(pNew);
					if (pGroup != m_pAllGroup)
					{
						m_pAllGroup->AddTexture(pNew);
					}
				}
				else
				{
					delete pNew;
				}
			}
		}
	}

	// free memory
	delete[] dir;
}


//-----------------------------------------------------------------------------
// Purpose: Loads all textures in a given graphics file and returns an ID for
//			the file.
// Input  : filename - Full path of graphics file to load.
// Output : Returns the file ID.
//-----------------------------------------------------------------------------
DWORD CTextureSystem::LoadGraphicsFile(CString filename)
{
	static DWORD __GraphFileID = 1;	// must start at 1.

	//
	// Make sure it's not already there.
	//
	int i = m_GraphicsFiles.GetSize() - 1;
	while (i > -1)
	{
		if (!strcmp(m_GraphicsFiles[i].filename, filename))
		{
			return(m_GraphicsFiles[i].id);
		}

		i--;
	}

	//
	// Is this a WAD file?
	//
	DWORD dwAttrib = GetFileAttributes(filename);
	if (dwAttrib == 0xFFFFFFFF)
	{
		return(0);
	}

	GRAPHICSFILESTRUCT gf;

	if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		// open the file, and add it to the GraphicFileList array
		gf.fd = _open(filename, _O_BINARY | _O_RDONLY);
		if (gf.fd == -1)
		{
			// todo: if errno is "out of handles", close some other
			// graphics files.

			// StatusMsg(IDS_ERROPENGRAPHFILE, errno);
			return 0;	// could not open
		}

		char buf[4];
		_read(gf.fd, buf, 4);

		//
		// Make sure the file is in a format that we can read.
		//
		if (!memcmp(buf, "WAD3", 4))
		{
			gf.format = tfWAD3;
		}
		else
		{
			CString str;

			str.Format("The file \"%s\" is not a valid WAD3 file and will not be used.", filename);
			AfxMessageBox(str, MB_ICONEXCLAMATION | MB_OK);
			_close(gf.fd);
			return(0);
		}
	}

	// got it -- setup the rest of the gf structure
	gf.id = __GraphFileID++;
	gf.filename = filename;
	gf.bLoaded = FALSE;

	//
	// Add file to list of texture files.
	//
	m_GraphicsFiles.Add(gf);

	//
	// Create a new texture group for the file.
	//
	CTextureGroup *pGroup = new CTextureGroup(filename);
	pGroup->SetTextureFormat(gf.format);
	m_Groups.AddTail(pGroup);
	
	//
	// Load the textures from the file and place them in the texture group.
	//
	LoadGraphicsFileWAD3(&gf, gf.fd, pGroup);
	gf.bLoaded = TRUE;

	//
	// Sort this group's list
	//
	pGroup->Sort();

	return(gf.id);
}


//-----------------------------------------------------------------------------
// Purpose: Determines whether or not there is at least one available texture
//			group for a given texture format.
// Input  : format - Texture format to look for.
// Output : Returns TRUE if textures of a given format are available, FALSE if not.
//-----------------------------------------------------------------------------
BOOL CTextureSystem::IsGraphicsAvailable(TEXTUREFORMAT format)
{
	POSITION p = m_Groups.GetHeadPosition();
	while (p)
	{
		CTextureGroup *pGroup = m_Groups.GetNext(p);
		if (pGroup->GetTextureFormat() == format)
		{
			return(TRUE);
		}
	}

	return(FALSE);
}


//-----------------------------------------------------------------------------
// Used to add all the world materials into the material list
//-----------------------------------------------------------------------------
bool CTextureSystem::EnumMaterial( const char *pMaterialName, int nContext )
{
	CTextureGroup *pGroup = (CTextureGroup *)nContext;
	CMaterial *pMaterial = CMaterial::CreateMaterial(pMaterialName, false);
	if (pMaterial != NULL)
	{
		int nIndex = AddTexture(pMaterial);

		// Add the texture's index to the given group and to the "All" group.
		pGroup->AddTexture(pMaterial);
		if (pGroup != m_pAllGroup)
		{
			m_pAllGroup->AddTexture(pMaterial);
		}
	}
	return true;
}


//-----------------------------------------------------------------------------
// Registers the keywords as existing in a particular material
//-----------------------------------------------------------------------------
void CTextureSystem::RegisterTextureKeywords( IEditorTexture *pTexture )
{
	//
	// Add any new keywords from this material to the list of keywords.
	//
	char szKeywords[MAX_PATH];
	pTexture->GetKeywords(szKeywords);
	if (szKeywords[0] != '\0')
	{
		char *pch = strtok(szKeywords, " ,;");
		while (pch != NULL)
		{
			// dvs: hide in a Find function
			bool bFound = false;
			POSITION pos = m_Keywords.GetHeadPosition();
			while (pos != NULL)
			{
				char *pszTest = m_Keywords.GetNext(pos);
				if (!stricmp(pszTest, pch))
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				char *pszKeyword = new char[strlen(pch) + 1];
				strcpy(pszKeyword, pch);
				m_Keywords.AddTail(pszKeyword);
			}

			pch = strtok(NULL, " ,;");
		}
	}
}


//-----------------------------------------------------------------------------
// Used to lazily load in all the textures
//-----------------------------------------------------------------------------
void CTextureSystem::LazyLoadTextures()
{
	Assert( m_pAllGroup );
	m_pAllGroup->LazyLoadTextures();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureSystem::LoadGraphicsFileMaterials(void)
{
	//
	// Create a new group for materials.
	//
	CTextureGroup *pGroup = new CTextureGroup("VMT");
	pGroup->SetTextureFormat(tfVMT);
	m_Groups.AddTail(pGroup);

	//
	// Add all the materials to the group.
	//
	CMaterial::EnumerateMaterials( this, (int)pGroup, INCLUDE_WORLD_MATERIALS ); 
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
// Input  : pszName - Name of group, ie "Materials" or "u:\hl\tfc\tfc.wad".
//-----------------------------------------------------------------------------
CTextureGroup::CTextureGroup(const char *pszName)
{
	strcpy(m_szName, pszName);
	m_eTextureFormat = tfNone;
	m_nTextures = 0;
	m_nTextureToLoad = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Adds a texture to this group.
// Input  : pTexture - Texture to add.
//-----------------------------------------------------------------------------
void CTextureGroup::AddTexture(IEditorTexture *pTexture)
{
	m_Textures.SetAtGrow(m_nTextures, pTexture);
	m_nTextures++;
}


//-----------------------------------------------------------------------------
// Purpose: Sorts the group.
//-----------------------------------------------------------------------------
void CTextureGroup::Sort(void)
{
	qsort(m_Textures.GetData(), m_nTextures, sizeof(IEditorTexture *), SortGraphicsProc);

	// Changing the order means we don't know where we should be loading from
	m_nTextureToLoad = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Retrieves a texture by index.
// Input  : nIndex - Index of the texture in this group.
//-----------------------------------------------------------------------------
IEditorTexture *CTextureGroup::GetTexture(int nIndex)
{
	if ((nIndex >= m_nTextures) || (nIndex < 0))
	{
		return(NULL);
	}

	return(m_Textures[nIndex]);
}


//-----------------------------------------------------------------------------
// finds a texture by name
//-----------------------------------------------------------------------------
IEditorTexture* CTextureGroup::GetTexture( char const* pName )
{
	for (int i = 0; i < m_nTextures; ++i)
	{
		if (!strcmp(pName, m_Textures[i]->GetName()))
			return m_Textures[i];
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Used to lazily load in all the textures
//-----------------------------------------------------------------------------
void CTextureGroup::LazyLoadTextures()
{
	// Load at most once per call
	while (m_nTextureToLoad < m_nTextures)
	{
		if (!m_Textures[m_nTextureToLoad]->IsLoaded())
		{
			m_Textures[m_nTextureToLoad]->Load();
			++m_nTextureToLoad;
			return;
		}

		// This one was already loaded; skip it
		++m_nTextureToLoad;
	}
}

