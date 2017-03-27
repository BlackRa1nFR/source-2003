//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "ChunkFile.h"
#include "VisGroup.h"


bool CVisGroup::g_bShowAll = false;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CVisGroup::LoadKeyCallback(const char *szKey, const char *szValue, CVisGroup *pGroup)
{
	if (!stricmp(szKey, "name"))
	{
		pGroup->SetName(szValue);
	}
	else if (!stricmp(szKey, "visgroupid"))
	{
		pGroup->SetID(atoi(szValue));
	}
	else if (!stricmp(szKey, "color"))
	{
		unsigned char chRed;
		unsigned char chGreen;
		unsigned char chBlue;

		CChunkFile::ReadKeyValueColor(szValue, chRed, chGreen, chBlue);
		pGroup->SetColor(chRed, chGreen, chBlue);
	}
	else if (!stricmp(szKey, "visible"))
	{
		pGroup->SetVisible(atoi(szValue) == 1);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CVisGroup::LoadVMF(CChunkFile *pFile)
{
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadKeyCallback, this);
	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not this visgroup is currently visible.
//-----------------------------------------------------------------------------
bool CVisGroup::IsVisible(void)
{
	if (g_bShowAll)
	{
		return(true);
	}

	return(m_bVisible);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not visgroup visibility is being overridden by
//			the "Show All" button.
//-----------------------------------------------------------------------------
bool CVisGroup::IsShowAllActive(void)
{
	return(g_bShowAll);
}


//-----------------------------------------------------------------------------
// Purpose: Saves this visgroup.
// Input  : pFile - File to save into.
// Output : Returns ChunkFile_Ok on success, an error code on failure.
//-----------------------------------------------------------------------------
ChunkFileResult_t CVisGroup::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("visgroup");

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValue("name", GetName());
	}

	if (eResult == ChunkFile_Ok)
	{
		DWORD dwID = GetID();
		eResult = pFile->WriteKeyValueInt("visgroupid", dwID);
	}
	
	if (eResult == ChunkFile_Ok)
	{
		color32 rgbColor = GetColor();
		eResult = pFile->WriteKeyValueColor("color", rgbColor.r, rgbColor.g, rgbColor.b);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueBool("visible", IsVisible());
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Overrides normal visgroup visibility, making all visgroups visible. 
//-----------------------------------------------------------------------------
void CVisGroup::ShowAllVisGroups(bool bShow)
{
	g_bShowAll = bShow;
}


