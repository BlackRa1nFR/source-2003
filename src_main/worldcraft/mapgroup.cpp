//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "MapGroup.h"
#include "SaveInfo.h"
#include "Worldcraft.h"


IMPLEMENT_MAPCLASS(CMapGroup)


//-----------------------------------------------------------------------------
// Purpose: Sets the new child's color to our own.
// Input  : pChild - Object being added to this group.
//-----------------------------------------------------------------------------
void CMapGroup::AddChild(CMapClass *pChild)
{
	pChild->SetRenderColor(r,g,b);
	CMapClass::AddChild(pChild);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pobj - 
// Output : CMapClass *
//-----------------------------------------------------------------------------
CMapClass *CMapGroup::CopyFrom(CMapClass *pobj, bool bUpdateDependencies)
{
	return(CMapClass::CopyFrom(pobj, bUpdateDependencies));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMapClass *
//-----------------------------------------------------------------------------
CMapClass *CMapGroup::Copy(bool bUpdateDependencies)
{
	CMapGroup *pNew = new CMapGroup;
	return(pNew->CopyFrom(this, bUpdateDependencies));
}


//-----------------------------------------------------------------------------
// Purpose: Returns a string describing this group.
//-----------------------------------------------------------------------------
LPCTSTR CMapGroup::GetDescription(void)
{
	static char szBuf[128];
	sprintf(szBuf, "group of %d objects", Children.GetCount());
	return(szBuf);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapGroup::LoadVMF(CChunkFile *pFile)
{
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("editor", (ChunkHandler_t)CMapClass::LoadEditorCallback, this);

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)CMapClass::LoadEditorKeyCallback, this);
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapGroup::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	//
	// Check rules before saving this object.
	//
	if (!pSaveInfo->ShouldSaveObject(this))
	{
		return(ChunkFile_Ok);
	}

	ChunkFileResult_t eResult = pFile->BeginChunk("group");

	//
	// Save the group's ID.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("id", GetID());
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = CMapClass::SaveVMF(pFile, pSaveInfo);
	}	

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}
