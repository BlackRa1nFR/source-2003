//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "GameData.h"
#include "IEditorTexture.h"
#include "GlobalFunctions.h"
#include "worldcraft_mathlib.h"
#include "HelperFactory.h"
#include "MapAlignedBox.h"
#include "MapDefs.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapAnimator.h"
#include "MapSolid.h"
#include "MapWorld.h"
#include "Options.h"
#include "Render2D.h"
#include "SaveInfo.h"
#include "VisGroup.h"


IMPLEMENT_MAPCLASS(CMapEntity)


class CMapAnimator;
class CMapKeyFrame;


bool CMapEntity::s_bShowEntityNames = true;
bool CMapEntity::s_bShowEntityConnections = false;


static CMapObjectList FoundEntities;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			pKV - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL FindKeyValue(CMapEntity *pEntity, MDkeyvalue *pKV)
{
	LPCTSTR pszValue = pEntity->GetKeyValue(pKV->szKey);
	if (!pszValue || strcmpi(pszValue, pKV->szValue))
	{
		return TRUE;
	}

	FoundEntities.AddTail(pEntity);

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMapEntity::CMapEntity(void) : flags(0)
{
	m_pMoveParent = NULL;
	m_pAnimatorChild = NULL;
}
	

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CMapEntity::~CMapEntity(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Adds a bounding box helper to this entity. If this entity's class
//			specifies a bounding box, it will be the correct size.
// Input  : pClass - 
//-----------------------------------------------------------------------------
void CMapEntity::AddBoundBoxForClass(GDclass *pClass, bool bLoading)
{
	Vector Mins;
	Vector Maxs;

	//
	// If we have a class and it specifies a class, use that bounding box.
	//
	if ((pClass != NULL) && (pClass->HasBoundBox()))
	{
		pClass->GetBoundBox(Mins, Maxs);
	}
	//
	// Otherwise, use a default bounding box.
	//
	else
	{
		VectorFill(Mins, -8);
		VectorFill(Maxs, 8);
	}

	//
	// Create the box and add it as one of our children.
	//
	CMapAlignedBox *pBox = new CMapAlignedBox(Mins, Maxs);
	pBox->SetOrigin(m_Origin);
	
	pBox->SetShouldSerialize(FALSE);
	pBox->SetSelectionState(GetSelectionState());

	//
	// HACK: Make sure that the new child gets properly linked into the world.
	//		 This is not correct because it bypasses the doc's AddObjectToWorld code.
	//
	// Don't call AddObjectToWorld during VMF load because we don't want to call
	// OnAddToWorld during VMF load. We update our helpers during PostloadWorld.
	//
	CMapWorld *pWorld = (CMapWorld *)GetWorldObject(this);
	if ((!bLoading) && (pWorld != NULL))
	{
		pWorld->AddObjectToWorld(pBox, this);
	}
	else
	{
		AddChild(pBox);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets our child's render color to our render color.
// Input  : pChild - Child object being added.
//-----------------------------------------------------------------------------
void CMapEntity::AddChild(CMapClass *pChild)
{
	//
	// If we have a game data class, set the child's render color to the color
	// dictated by the game data class.
	//
	if (IsClass())
	{
		GDclass *pClass = GetClass();
		color32 rgbColor = pClass->GetColor();
		pChild->SetRenderColor(rgbColor);
	}
	//
	// If no class, set the color to the default entity color (purple).
	//
	else
	{
		pChild->SetRenderColor(220, 0, 180);
	}

	CMapClass::AddChild(pChild);

	//
	// Notify the new child of all our keys. Don't bother for solids.
	//
	if (dynamic_cast<CMapSolid*>(pChild) == NULL)
	{
		int nNumKeys = GetKeyValueCount();
		for (int i = 0; i < nNumKeys; i++)
		{
			MDkeyvalue KeyValue = m_KeyValues.GetKeyValue(i); 
			pChild->OnParentKeyChanged( KeyValue.szKey, KeyValue.szValue );
		}	
	}
}



//-----------------------------------------------------------------------------
// Purpose: Adds a helper object as a child of this entity.
// Input  : pHelper - The helper object.
//			bLoading - True if this is being called from Postload, false otherwise.
//-----------------------------------------------------------------------------
void CMapEntity::AddHelper(CMapClass *pHelper, bool bLoading)
{
	if (!IsPlaceholder())
	{
		//
		// Solid entities have no origin, so place the helper at our center.
		//
		Vector vecCenter;
		m_Render2DBox.GetBoundsCenter(vecCenter);
		pHelper->SetOrigin(vecCenter);
	}
	else
	{
		pHelper->SetOrigin(m_Origin);
	}

	pHelper->SetShouldSerialize(FALSE);
	pHelper->SetSelectionState(GetSelectionState());

	//
	// HACK: Make sure that the new child gets properly linked into the world.
	//		 This is not correct because it bypasses the doc's AddObjectToWorld code.
	//
	// Don't call AddObjectToWorld during VMF load because we don't want to call
	// OnAddToWorld during VMF load. We update our helpers during PostloadWorld.
	//
	CMapWorld *pWorld = (CMapWorld *)GetWorldObject(this);
	if ((!bLoading) && (pWorld != NULL))
	{
		pWorld->AddObjectToWorld(pHelper, this);
	}
	else
	{
		AddChild(pHelper);
	}

	//
	// dvs: HACK for animator children. Better for CMapEntity to have a SetAnimatorChild
	//		function that the CMapAnimator could call. Better still, eliminate the knowledge
	//		that CMapEntity has about its animator child.
	//
	CMapAnimator *pAnim = dynamic_cast<CMapAnimator *>(pHelper);
	if (pAnim != NULL)
	{
		m_pAnimatorChild = pAnim;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates all helper objects defined by the FGD and adds them as
//			children of this entity. Helper objects perform rendering, UI, and
//			bookkeeping functions for their parent entities. If the class
//			definition does not specify any helpers, or none of the helpers
//			could be added, a box helper is added so that the entity has some
//			visual representation.
// Inputs : pClass - 
//			bLoading - True if this is being called from Postload, false otherwise.
//-----------------------------------------------------------------------------
void CMapEntity::AddHelpersForClass(GDclass *pClass, bool bLoading)
{
	bool bAddedOneVisual = false;

	if (((pClass != NULL) && (pClass->HasBoundBox())))
	{
		AddBoundBoxForClass(pClass, bLoading);
		bAddedOneVisual = true;
	}

	//
	// If we have a game class from the FGD, add whatever helpers are declared in that
	// class definition.
	//
	if (pClass != NULL)
	{
		//
		// Add all the helpers that this class declares in the FGD.
		//
		GDclass *pClass = GetClass();
		POSITION pos;
		
		//
		// For every helper in the class definition...
		//
		CHelperInfo *pHelperInfo = pClass->GetFirstHelper(pos);
		while (pHelperInfo != NULL)
		{
			//
			// Create the helper and attach it to this entity.
			//
			CMapClass *pHelper = CHelperFactory::CreateHelper(pHelperInfo, this);
			if (pHelper != NULL)
			{
				AddHelper(pHelper, bLoading);
				if (pHelper->IsVisualElement())
				{
					bAddedOneVisual = true;
				}
			}

			pHelperInfo = pClass->GetNextHelper(pos);
		}
	
		//
		// Look for keys that define helpers.
		//
		// FIXME: make this totally data driven like the helper factory, or better
		//		  yet, like the LINK_ENTITY_TO_CLASS stuff in the game DLL
		int nVarCount = pClass->GetVariableCount();
		for (int i = 0; i < nVarCount; i++)
		{
			GDinputvariable *pVar = pClass->GetVariableAt(i);
			GDIV_TYPE eType = pVar->GetType();
		
			CHelperInfo HelperInfo;
			bool bCreate = false;
			switch (eType)
			{
				case ivOrigin:
				{
					const char *pszKey = pVar->GetName();
					HelperInfo.SetName("origin");
					HelperInfo.AddParameter(pszKey);
					bCreate = true;
					break;
				}

				case ivVecLine:
				{
					const char *pszKey = pVar->GetName();
					HelperInfo.SetName("vecline");
					HelperInfo.AddParameter(pszKey);
					bCreate = true;
					break;
				}

				case ivAxis:
				{
					const char *pszKey = pVar->GetName();
					HelperInfo.SetName("axis");
					HelperInfo.AddParameter(pszKey);
					bCreate = true;
					break;
				}
			}

			//
			// Create the helper and attach it to this entity.
			//
			if (bCreate)
			{
				CMapClass *pHelper = CHelperFactory::CreateHelper(&HelperInfo, this);
				if (pHelper != NULL)
				{
					AddHelper(pHelper, bLoading);
					if (pHelper->IsVisualElement())
					{
						bAddedOneVisual = true;
					}
				}
			}
		}
	}

	//
	// Any solid children we have will also work as visual elements.
	//
	if (!IsPlaceholder())
	{
		bAddedOneVisual = true;
	}
	//
	// If we have no game class and we are a point entity, add an "obsolete" sprite helper
	// so level designers know to update the entity.
	//
	else if (pClass == NULL)
	{
		CHelperInfo HelperInfo;
		HelperInfo.SetName("iconsprite");
		HelperInfo.AddParameter("sprites/obsolete.spr");

		CMapClass *pSprite = CHelperFactory::CreateHelper(&HelperInfo, this);
		if (pSprite != NULL)
		{
			AddHelper(pSprite, bLoading);
			bAddedOneVisual = true;
		}
	}
	
	//
	// If we still haven't added any visible helpers, we need to add a bounding box so that there
	// is some visual representation for this entity. We also add the bounding box if the
	// entity's class specifies a bounding box.
	//
	if (!bAddedOneVisual)
	{
		AddBoundBoxForClass(pClass, bLoading);
	}

	CalcBounds(TRUE);
	PostUpdate(Notify_Changed);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a deep copy of this object.
// Output : Returns a pointer to the new allocated object.
//-----------------------------------------------------------------------------
CMapClass *CMapEntity::Copy(bool bUpdateDependencies)
{
	CMapEntity *pNew = new CMapEntity;
	pNew->CopyFrom(this, bUpdateDependencies);
	return pNew;
}


//-----------------------------------------------------------------------------
// Purpose: Performs a deep copy of a given object into this object.
// Input  : pobj - Object to copy from.
// Output : Returns a pointer to this object.
//-----------------------------------------------------------------------------
CMapClass *CMapEntity::CopyFrom(CMapClass *pobj, bool bUpdateDependencies)
{
	ASSERT(pobj->IsMapClass(MAPCLASS_TYPE(CMapEntity)));
	CMapEntity *pFrom = (CMapEntity*) pobj;
	
	flags = pFrom->flags;
	
	m_Origin = pFrom->m_Origin;

	CMapClass::CopyFrom(pobj, bUpdateDependencies);

	//
	// Copy our keys. If our targetname changed we must relink all targetname pointers.
	//
	const char *pszOldTargetName = CEditGameClass::GetKeyValue("targetname");
	char szOldTargetName[MAX_IO_NAME_LEN];
	if (pszOldTargetName != NULL)
	{
		strcpy(szOldTargetName, pszOldTargetName);
	}

	CEditGameClass::CopyFrom(pFrom);
	const char *pszNewTargetName = CEditGameClass::GetKeyValue("targetname");

	if ((bUpdateDependencies) && (pszNewTargetName != NULL))
	{
		if (stricmp(szOldTargetName, pszNewTargetName) != 0)
		{
			UpdateAllDependencies(this);
		}
	}

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bFullUpdate - 
//-----------------------------------------------------------------------------
void CMapEntity::CalcBounds(BOOL bFullUpdate)
{
	CMapClass::CalcBounds(bFullUpdate);

	//
	// If we are a solid entity, set our origin to our bounds center.
	//
	if (IsSolidClass())
	{
		m_Render2DBox.GetBoundsCenter(m_Origin);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Debugging hook.
//-----------------------------------------------------------------------------
void CMapEntity::Debug(void)
{
	int nKeyValues = m_KeyValues.GetCount();
	for (int i = 0; i < nKeyValues; i++)
	{
		MDkeyvalue &KeyValue = m_KeyValues.GetKeyValue(i);
	}
}


//-----------------------------------------------------------------------------
// Purpose: If this entity has a name key, returns a string with "<name> <classname>"
//			in it. Otherwise returns a buffer with "<classname>" in it.
// Output : String description of the entity.
//-----------------------------------------------------------------------------
LPCTSTR CMapEntity::GetDescription(void)
{
	static char szBuf[128];
	const char *pszName = GetKeyValue("targetname");

	if (pszName != NULL)
	{
		sprintf(szBuf, "%s - %s", pszName, GetClassName());
	}
	else
	{
		strcpy(szBuf, GetClassName());
	}

	return(szBuf);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the color that this entity should use for rendering.
//-----------------------------------------------------------------------------
void CMapEntity::GetRenderColor(unsigned char &red, unsigned char &green, unsigned char &blue)
{
	if (IsSelected())
	{
		red = GetRValue(Options.colors.clrSelection);
		green = GetGValue(Options.colors.clrSelection);
		blue = GetBValue(Options.colors.clrSelection);
	}
	else
	{
		GDclass *pClass = GetClass();
		if (pClass)
		{
			color32 rgbColor = pClass->GetColor();

			red = rgbColor.r;
			green = rgbColor.g;
			blue = rgbColor.b;
		}
		else
		{
			red = GetRValue(Options.colors.clrEntity);
			green = GetGValue(Options.colors.clrEntity);
			blue = GetBValue(Options.colors.clrEntity);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the color that this entity should use for rendering.
//-----------------------------------------------------------------------------
color32 CMapEntity::GetRenderColor(void)
{
	color32 clr;

	GetRenderColor(clr.r, clr.g, clr.b);
	return clr;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the size of this object.
// Output : Size, in bytes, of this object, not including any dynamically
//			allocated data members.
//-----------------------------------------------------------------------------
size_t CMapEntity::GetSize(void)
{
	return(sizeof(*this));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapEntity::LoadVMF(CChunkFile *pFile)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("solid", (ChunkHandler_t)LoadSolidCallback, this);
	Handlers.AddHandler("hidden", (ChunkHandler_t)LoadHiddenCallback, this);
	Handlers.AddHandler("editor", (ChunkHandler_t)CMapClass::LoadEditorCallback, (CMapClass *)this);
	Handlers.AddHandler("connections", (ChunkHandler_t)LoadConnectionsCallback, (CEditGameClass *)this);

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadKeyCallback, this);
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pEntity - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapEntity::LoadKeyCallback(const char *szKey, const char *szValue, CMapEntity *pEntity)
{
	if (!stricmp(szKey, "id"))
	{
		pEntity->SetID(atoi(szValue));
	}
	else
	{
		//
		// While loading, set key values directly rather than via SetKeyValue. This avoids
		// all the unnecessary bookkeeping that goes on in SetKeyValue.
		//
		pEntity->m_KeyValues.SetValue(szKey, szValue);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bVisible - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapEntity::LoadHiddenCallback(CChunkFile *pFile, CMapEntity *pEntity)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("solid", (ChunkHandler_t)LoadSolidCallback, pEntity);
	Handlers.AddHandler("editor", (ChunkHandler_t)LoadEditorCallback, pEntity);

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk();
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pEntity - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapEntity::LoadSolidCallback(CChunkFile *pFile, CMapEntity *pEntity)
{
	CMapSolid *pSolid = new CMapSolid;

	bool bValid;
	ChunkFileResult_t eResult = pSolid->LoadVMF(pFile, bValid);

	if ((eResult == ChunkFile_Ok) && (bValid))
	{
		pEntity->AddChild(pSolid);
	}
	else
	{
		delete pSolid;
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Sets this entity's origin and updates the bounding box.
// Input  : o - Origin to set.
//-----------------------------------------------------------------------------
void CMapEntity::SetOrigin(Vector& o)
{
	CMapClass::SetOrigin(o);

	// dvs: is this still necessary?
	if (!(flags & flagPlaceholder))
	{
		// not a placeholder.. no origin.
		return;
	}

	CalcBounds( TRUE );
	PostUpdate(Notify_Changed);
}


//-----------------------------------------------------------------------------
// Purpose: Removes all of this entity's helpers.
// Input  : bRemoveSolidChildren - Whether to also remove any solid children. This
//			is true when changing from a solid entity to a point entity.
//-----------------------------------------------------------------------------
void CMapEntity::RemoveHelpers(bool bRemoveSolids)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		POSITION PrevPos = pos;

		CMapClass *pChild = Children.GetNext(pos);
		if (bRemoveSolids || ((dynamic_cast <CMapSolid *> (pChild)) == NULL))
		{
			Children.RemoveAt(PrevPos);
		}

		// LEAKLEAK: need to KeepForDestruction to avoid undo crashes, but how? where?
		//delete pChild;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szOldName - 
//			*szNewName - 
//-----------------------------------------------------------------------------
void CMapEntity::ReplaceTargetname(const char *szOldName, const char *szNewName)
{
	//
	// Replace any keys whose value matches the old name.
	//
	int nNumKeys = GetKeyValueCount();
	for (int i = 0; i < nNumKeys; i++)
	{
		MDkeyvalue KeyValue = m_KeyValues.GetKeyValue(i);
		if (!stricmp(KeyValue.szValue, szOldName))
		{
			SetKeyValue(KeyValue.szKey, szNewName);
		}
	}

	//
	// Replace any connections that target the old name.
	//
	POSITION pos = Connections_GetHeadPosition();
	while (pos != NULL)
	{
		CEntityConnection *pConn = Connections_GetNext(pos);
		if (!stricmp(pConn->GetTargetName(), szOldName))
		{
			pConn->SetTargetName(szNewName);
		}
	}
	
	CMapClass::ReplaceTargetname(szOldName, szNewName);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Inputs : pszClass - 
//			bLoading - True if this is being called from Postload, false otherwise.
//-----------------------------------------------------------------------------
void CMapEntity::SetClass(LPCTSTR pszClass, bool bLoading)
{
	ASSERT(pszClass);

	//
	// If we are just setting to the same class, don't do anything.
	//
	if (IsClass(pszClass))
	{
		return;
	}

	//
	// Copy class name & resolve GDclass pointer.
	//
	CEditGameClass::SetClass(pszClass, bLoading);

	//
	// If our new class is defined in the FGD, set our color and our default keys
	// from the class.
	//
	if (IsClass())
	{
		SetPlaceholder(!IsSolidClass());

		color32 rgbColor = m_pClass->GetColor();
		SetRenderColor(rgbColor);
		GetDefaultKeys();

		if (IsNodeClass() && (GetNodeID() == 0))
		{
			AssignNodeID();
		}
	}
	//
	// If not, use whether or not we have solid children to determine whether
	// we are a point entity or a solid entity.
	//
	else
	{
		SetPlaceholder(HasSolidChildren() ? FALSE : TRUE);
	}

	//
	// Add whatever helpers our class requires, or a default bounding box if
	// our class is unknown and we are a point entity.
	//
	UpdateHelpers(bLoading);

	//
	// HACK: If we are now a decal, make sure we have a valid texture.
	//
	if (!strcmp(pszClass, "infodecal"))
	{
		if (!GetKeyValue("texture"))
		{
			SetKeyValue("texture", "clip");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Assigns the next unique node ID to this entity.
//-----------------------------------------------------------------------------
void CMapEntity::AssignNodeID(void)
{
	char szID[80];
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	itoa(pDoc->GetNextNodeID(), szID, 10);
	SetKeyValue("nodeid", szID);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapEntity::EnsureUniqueNodeID(CMapWorld *pWorld)
{
	bool bBuildNewNodeID = true;
	int nOurNodeID = GetNodeID();
	if (nOurNodeID != 0)
	{
		//
		// We already have a node ID. Make sure that it is unique. If not,
		// we need to generate a new one.
		//
		bBuildNewNodeID = false;

		EnumChildrenPos_t pos;
		CMapClass *pChild = pWorld->GetFirstDescendent(pos);
		while (pChild != NULL)
		{
			CMapEntity *pEntity = dynamic_cast <CMapEntity *> (pChild);
			if ((pEntity != NULL) && (pEntity != this))
			{
				int nThisNodeID = pEntity->GetNodeID();
				if (nThisNodeID)
				{
					if (nThisNodeID == nOurNodeID)
					{
						bBuildNewNodeID = true;
						break;
					}
				}
			}

			pChild = pWorld->GetNextDescendent(pos);
		}
	}

	if (bBuildNewNodeID)
	{
		AssignNodeID();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called after the entire map has been loaded. This allows the object
//			to perform any linking with other map objects or to do other operations
//			that require all world objects to be present.
//-----------------------------------------------------------------------------
void CMapEntity::PostloadWorld(CMapWorld *pWorld)
{
	int nIndex;

	//
	// Set our origin from our "origin" key and discard the key.
	//
	const char *pszValue = m_KeyValues.GetValue("origin", &nIndex);
	if (pszValue != NULL)
	{
		Vector Origin;
		sscanf(pszValue, "%f %f %f", &Origin[0], &Origin[1], &Origin[2]);
		SetOrigin(Origin);
	}

	//
	// Set our angle from our "angle" key and discard the key.
	//
	pszValue = m_KeyValues.GetValue("angle", &nIndex);
	if (pszValue != NULL)
	{
		ImportAngle(atoi(pszValue));
		RemoveKey(nIndex);
	}

	//
	// Set the class name from our "classname" key and discard the key.
	// This also adds the helpers appropriate for the class.
	//
	pszValue = m_KeyValues.GetValue("classname", &nIndex);
	if (pszValue != NULL)
	{
		//
		// Copy the classname to a temp buffer because SetClass mucks with the
		// keyvalues and our pointer might become bad.
		//
		char szClassName[MAX_CLASS_NAME_LEN];
		strcpy(szClassName, pszValue);
		SetClass(szClassName, true);

		//
		// Need to re-get the index of the classname key since it may have changed
		// as a result of the above SetClass call.
		//
		pszValue = m_KeyValues.GetValue("classname", &nIndex);
		if (pszValue != NULL)
		{
			RemoveKey(nIndex);
		}
	}

	//
	// Now that we have set the class, remove the origin key if this entity isn't
	// supposed to expose it in the keyvalues list.
	//
	if (IsPlaceholder() && (!IsClass() || GetClass()->VarForName("origin") == NULL))
	{
		const char *pszValue = m_KeyValues.GetValue("origin", &nIndex);
		if (pszValue != NULL)
		{
			RemoveKey(nIndex);
		}
	}

	//
	// Must do this after assigning the class.
	//
	if (IsNodeClass() && (GetKeyValue("nodeid") == NULL))
	{
		AssignNodeID();
	}

	//
	// Call in all our children (some of which were created above).
	//
	CMapClass::PostloadWorld(pWorld);

	CalcBounds();
}


//-----------------------------------------------------------------------------
// Purpose: Insures that the entity has all the helpers that it needs (and no more
//			than it should) given its class.
//-----------------------------------------------------------------------------
void CMapEntity::UpdateHelpers(bool bLoading)
{
	//
	// If we have any helpers, delete them. Delete any solid children if we are
	// a point class.
	//
	RemoveHelpers(IsPlaceholder() == TRUE);

	//
	// Add the helpers appropriate for our current class.
	//	
	AddHelpersForClass(GetClass(), bLoading);
}


//-----------------------------------------------------------------------------
// Purpose: increments the numerals at the end of a string
//			appends 0 if no numerals exist
// Input  : newName - 
//-----------------------------------------------------------------------------
void IncrementStringName( CString &str )
{
	// walk backwards through the string looking for where the digits stop
	int pos = str.GetLength();
	while ( isdigit(str.GetAt(pos-1)) )
	{
		pos--;
	}

	// if no digits found, append a "0"
	if ( pos == str.GetLength() )
	{
		str += "0";
	}
	else
	{
		// get the number
		CString sNum = str.Right( str.GetLength() - pos );
		int iNum = atoi( sNum );

		// increment the number
		iNum++;

		// change the number in the string
		CString newStr;
		newStr.Format( "%s%d", (const char*)str.Left(pos), iNum );
		str = newStr;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Generates a new, unique targetname
//			a static function
// Input  : char *startName - 
//			*outputName - 
//			newNameBufferSize - 
//-----------------------------------------------------------------------------
void CMapEntity::GenerateNewTargetname( const char *startName, char *outputName, int newNameBufferSize )
{
	outputName[0] = 0;
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if ( !pDoc )
		return;
	
	CMapWorld *pWorld = pDoc->GetMapWorld();
	if ( !pWorld )
		return;

	// find the start name
	CString newName( startName );
	if ( newName.IsEmpty() )
	{
		newName = "entity";
	}
	
	// try to find entities that match the name
	CMapEntity *pEnt = NULL;
	do
	{
		// increment the entity name
		IncrementStringName( newName );

		pEnt = pWorld->FindChildByKeyValue( "targetname", newName );
	}
	while ( pEnt );

	strncpy( outputName, newName, newNameBufferSize );
	outputName[newNameBufferSize-1] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Allows the entity to update its key values based on a change in one
//			of its children. The child exposes the property as a key value pair.
// Input  : pChild - The child whose property changed.
//			szKey - The name of the property that changed.
//			szValue - The new value of the property.
//-----------------------------------------------------------------------------
void CMapEntity::NotifyChildKeyChanged(CMapClass *pChild, const char *szKey, const char *szValue)
{
	m_KeyValues.SetValue(szKey, szValue);

	//
	// Notify all our other non-solid children that a key has changed.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = Children.GetNext(pos);
		if ((pObject != pChild) && (pChild != NULL) && (dynamic_cast<CMapSolid *>(pObject) == NULL))
		{
			pObject->OnParentKeyChanged(szKey, szValue);
		}
	}

	CalcBounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapEntity::DeleteKeyValue(LPCSTR pszKey)
{
	char szOldValue[KEYVALUE_MAX_VALUE_LENGTH];
	const char *pszOld = GetKeyValue(pszKey);
	if (pszOld != NULL)
	{
		strcpy(szOldValue, pszOld);
	}
	else
	{
	  szOldValue[0] = '\0';
	}

	CEditGameClass::DeleteKeyValue(pszKey);

	OnKeyValueChanged(pszKey, szOldValue, "");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapEntity::SetKeyValue(LPCSTR pszKey, LPCSTR pszValue)
{
	//
	// Get the current value so we can tell if it is changing.
	//
	char szOldValue[KEYVALUE_MAX_VALUE_LENGTH];
	const char *pszOld = GetKeyValue(pszKey);
	if (pszOld != NULL)
	{
		strcpy(szOldValue, pszOld);
	}
	else
	{
	  szOldValue[0] = '\0';
	}

	CEditGameClass::SetKeyValue(pszKey, pszValue);

	OnKeyValueChanged(pszKey, szOldValue, pszValue);
}


//-----------------------------------------------------------------------------
// Purpose: Notifies the entity that it has been cloned.
// Input  : pClone - 
//-----------------------------------------------------------------------------
void CMapEntity::OnPreClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList)
{
	CMapClass::OnPreClone(pClone, pWorld, OriginalList, NewList);

	if (OriginalList.GetCount() == 1)
	{
		// dvs: TODO: make this FGD-driven instead of hardcoded, see also MapKeyFrame.cpp
		// dvs: TODO: use letters of the alphabet between adjacent numbers, ie path2a path2b, etc.
		if (!stricmp(GetClassName(), "path_corner") || !stricmp(GetClassName(), "path_track"))
		{
			//
			// Generate a new name for the clone.
			//
			CMapEntity *pNewEntity = dynamic_cast<CMapEntity*>(pClone);
			ASSERT(pNewEntity != NULL);
			if (!pNewEntity)
				return;

			// create a new targetname for the clone
			char newName[128];
			const char *oldName = GetKeyValue("targetname");
			if (!oldName || oldName[0] == 0)
				oldName = "path";

			CMapEntity::GenerateNewTargetname(oldName, newName, 127);
			pNewEntity->SetKeyValue("targetname", newName);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pClone - 
//			pWorld - 
//			OriginalList - 
//			NewList - 
//-----------------------------------------------------------------------------
void CMapEntity::OnClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList)
{
	CMapClass::OnClone(pClone, pWorld, OriginalList, NewList);

	if (OriginalList.GetCount() == 1)
	{
		if (!stricmp(GetClassName(), "path_corner") || !stricmp(GetClassName(), "path_track"))
		{
			// dvs: TODO: make this FGD-driven instead of hardcoded, see also MapKeyFrame.cpp
			// dvs: TODO: use letters of the alphabet between adjacent numbers, ie path2a path2b, etc.
			CMapEntity *pNewEntity = dynamic_cast<CMapEntity*>(pClone);
			ASSERT(pNewEntity != NULL);
			if (!pNewEntity)
				return;

			// Point the clone at what we were pointing at.
			const char *pszNext = GetKeyValue("target");
			if (pszNext)
			{
				pNewEntity->SetKeyValue("target", pszNext);
			}

			// Point this path corner at the clone.
			SetKeyValue("target", pNewEntity->GetKeyValue("targetname"));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			pszOldValue - 
//			pszValue - 
//-----------------------------------------------------------------------------
void CMapEntity::OnKeyValueChanged(const char *pszKey, const char *pszOldValue, const char *pszValue)
{
	//
	// notify all our children that a key has changed
	//
	POSITION pos = Children.GetHeadPosition();
	while ( pos != NULL )
	{
		CMapClass *pChild = Children.GetNext( pos );
		if ( pChild != NULL )
		{
			pChild->OnParentKeyChanged( pszKey, pszValue );
		}
	}

	//
	// Changing our movement parent. Store a pointer to the movement parent
	// for when we're playing animations.
	//
	if ( !stricmp(pszKey, "parentname") )
	{
		CMapWorld *pWorld = (CMapWorld *)GetWorldObject( this );
		if (pWorld != NULL)
		{
			m_pMoveParent = (CMapEntity *)UpdateDependency(m_pMoveParent, pWorld->FindChildByKeyValue( "targetname", pszValue));
		}
	}
	//
	// Changing our model - rebuild the helpers from scratch.
	// dvs: this could probably go away - move support into the helper code.
	//
	else if (!stricmp(pszKey, "model"))
	{
		if (stricmp(pszOldValue, pszValue) != 0)
		{
			// We don't call SetKeyValue during VMF load.
			UpdateHelpers(false);
		}
	}
	//
	// If our targetname has changed, we have to relink EVERYTHING, not
	// just our dependents, because someone else may point to our new targetname.
	//
	else if (!stricmp(pszKey, "targetname") && (stricmp(pszOldValue, pszValue) != 0))
	{
		UpdateAllDependencies(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity has any solid children. Entities of
//			classes that are not in the FGD are considered solid entities if
//			they have at least one solid child, point entities if not.
//-----------------------------------------------------------------------------
bool CMapEntity::HasSolidChildren(void)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		if ((dynamic_cast <CMapSolid *> (pChild)) != NULL)
		{
			return(true);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Called after this object is added to the world.
//
//			NOTE: This function is NOT called during serialization. Use PostloadWorld
//				  to do similar bookkeeping after map load.
//
// Input  : pWorld - The world that we have been added to.
//-----------------------------------------------------------------------------
void CMapEntity::OnAddToWorld(CMapWorld *pWorld)
{
	CMapClass::OnAddToWorld(pWorld);

	//
	// If we are a node class, we must insure that we have a valid unique ID.
	//
	if (IsNodeClass())
	{
		EnsureUniqueNodeID(pWorld);
	}

	//
	// If we have a targetname, relink all the targetname pointers in the world
	// because someone might be looking for our targetname.
	//
	if (GetKeyValue("targetname") != NULL)
	{
		UpdateAllDependencies(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - The object that changed.
//-----------------------------------------------------------------------------
void CMapEntity::OnNotifyDependent(CMapClass *pObject, Notify_Dependent_t eNotifyType)
{
	CMapClass::OnNotifyDependent(pObject, eNotifyType);

	if (eNotifyType == Notify_Removed)
	{
		//
		// Check for our move parent going away.
		//
		if (pObject == m_pMoveParent)
		{
			CMapWorld *pWorld = (CMapWorld *)GetWorldObject(this);
			const char *pszParentName = CEditGameClass::GetKeyValue("parentname");
			if ((pWorld != NULL) && (pszParentName != NULL))
			{
				m_pMoveParent = (CMapEntity *)UpdateDependency(m_pMoveParent, pWorld->FindChildByKeyValue( "targetname", pszParentName));
			}
			else
			{
				m_pMoveParent = (CMapEntity *)UpdateDependency(m_pMoveParent, NULL);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Iterates through an object, and all it's children, looking for an
//			entity with a matching key and value
// Input  : key - 
//			value - 
// Output : Returns a pointer to the entity found.
//-----------------------------------------------------------------------------
CMapEntity *CMapEntity::FindChildByKeyValue( LPCSTR key, LPCSTR value )
{
	if ((key == NULL) || (value == NULL))
	{
		return(NULL);
	}

	int index;
	LPCSTR val = CEditGameClass::GetKeyValue(key, &index);

	if ( val && value && !stricmp(value, val) )
	{
		return this;
	}

	return CMapClass::FindChildByKeyValue( key, value );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a coordinate frame to render in, if the entity is animating
// Input  : matrix - 
// Output : returns true if a new matrix is returned, false if it is just the identity
//-----------------------------------------------------------------------------
bool CMapEntity::GetTransformMatrix( matrix4_t& matrix )
{
	bool gotMatrix = false;

	// if we have a move parent, get its transformation matrix
	if ( m_pMoveParent )
	{
		 gotMatrix = m_pMoveParent->GetTransformMatrix( matrix );
	}

	if ( m_pAnimatorChild )
	{
		// return a matrix that will transform any vector into our (animated) space
		if ( gotMatrix )
		{
			// return ParentMatrix * OurMatrix
			matrix4_t tmpMat, animatorMat;
			bool gotAnimMatrix = m_pAnimatorChild->GetTransformMatrix( animatorMat );
			if ( !gotAnimMatrix )
			{
				// since we didn't get a new matrix from our child just return our parent's
				return true;
			}

			ConcatTransforms( matrix, animatorMat, tmpMat );
			memcpy( matrix.Base(), tmpMat.Base(), sizeof(tmpMat) );
		}
		else
		{
			// no parent, we're at the top of the game
			gotMatrix = m_pAnimatorChild->GetTransformMatrix( matrix );
		}
	}

	return gotMatrix;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapEntity::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	//
	// Check rules before saving this object.
	//
	if (!pSaveInfo->ShouldSaveObject(this))
	{
		return(ChunkFile_Ok);
	}

	ChunkFileResult_t eResult = ChunkFile_Ok;

	//
	// If it's a solidentity but it doesn't have any solids, 
	// don't save it.
	//
	if (!IsPlaceholder() && !Children.GetCount())
	{
		return(ChunkFile_Ok);
	}

	//
	// If we are hidden, place this object inside of a hidden chunk.
	//
	if (!IsVisible())
	{
		eResult = pFile->BeginChunk("hidden");
	}

	//
	// Begin this entity's scope.
	//
	eResult = pFile->BeginChunk("entity");

	//
	// Save the entity's ID.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("id", GetID());
	}

	//
	// Save our keys.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = CEditGameClass::SaveVMF(pFile, pSaveInfo);
	}

	//
	// If this is a point entity of an unknown type or a point entity that doesn't
	// declare an origin key, save our origin.
	//
	if (IsPlaceholder() && (!IsClass() || GetClass()->VarForName("origin") == NULL))
	{
		char szOrigin[80];
		sprintf(szOrigin, "%g %g %g", (double)m_Origin[0], (double)m_Origin[1], (double)m_Origin[2]);
		pFile->WriteKeyValue("origin", szOrigin);
	}

	//
	// Save all our descendents.
	//
	if (!(IsPlaceholder()))
	{
		EnumChildrenPos_t pos;
		ChunkFileResult_t eResult = ChunkFile_Ok;

		CMapClass *pChild = GetFirstDescendent(pos);
		while ((pChild != NULL) && (eResult == ChunkFile_Ok))
		{
			eResult = pChild->SaveVMF(pFile, pSaveInfo);
			pChild = GetNextDescendent(pos);
		}
	}

	//
	// Save our base class' information within our chunk.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = CMapClass::SaveVMF(pFile, pSaveInfo);
	}
	
	//
	// End this entity's scope.
	//
	if (eResult == ChunkFile_Ok)
	{
		pFile->EndChunk();
	}

	//
	// End the hidden chunk if we began it.
	//
	if (!IsVisible())
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Overloaded to use the color from our FGD definition.
//-----------------------------------------------------------------------------
void CMapEntity::SetColorFromVisGroups(void)
{
	if (m_pVisGroup != NULL)
	{
		//
		// Set our color to our visgroup color.
		//
		color32 rgbColor = m_pVisGroup->GetColor();
		SetRenderColor(rgbColor);
	}
	else
	{
		//
		// Set our color based on our FGD definition.
		//
		GDclass *pClass = GetClass();
		if (IsPlaceholder() && pClass)
		{
			color32 rgbColor = pClass->GetColor();
			SetRenderColor(rgbColor);
		}
		else
		{
			SetRenderColor(220, 0, 180);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pWorld - 
//			pObject - 
//-----------------------------------------------------------------------------
void CMapEntity::UpdateDependencies(CMapWorld *pWorld, CMapClass *pObject)
{
	CMapClass::UpdateDependencies(pWorld, pObject);

	//
	// If we have a movement parent, relink to our movement parent.
	//
	const char *pszParentName = CEditGameClass::GetKeyValue("parentname");
	if (pszParentName != NULL)
	{
		m_pMoveParent = (CMapEntity *)UpdateDependency(m_pMoveParent, pWorld->FindChildByKeyValue( "targetname", pszParentName));
	}
	else
	{
		m_pMoveParent = (CMapEntity *)UpdateDependency(m_pMoveParent, NULL);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Places the entity properly on a plane surface, at a given location
// Input:	pos - position on the plane
//			plane - surface plane to align to
//			align - alignment type (top, bottom)
// Output: 
//-----------------------------------------------------------------------------

#define	ALIGN_EPSILON	1	// World units

void CMapEntity::AlignOnPlane( Vector& pos, PLANE *plane, alignType_e align )
{
	float	fHeight = m_Render2DBox.bmaxs[2] - m_Render2DBox.bmins[2];
	float	fOffset;
	Vector	vecNewPos;

	//Depending on the alignment type, get the offset from the surface
	switch ( align )
	{
	case ALIGN_BOTTOM:
		fOffset = m_Origin[2] - m_Render2DBox.bmins[2];
		break;

	case ALIGN_TOP:
		fOffset = m_Render2DBox.bmaxs[2] - m_Origin[2];
		break;
	}

	//Push our point out and away from this surface
	VectorMA( pos, fOffset + ALIGN_EPSILON, plane->normal, vecNewPos );
	
	//Update the entity and children
	SetOrigin( vecNewPos );
}


//-----------------------------------------------------------------------------
// Purpose: Looks for an input with a given name in the entity list. ALL entities
//			in the list must have the given input for a match to be found.
// Input  : szInput - Name of the input.
// Output : Returns true if the input name was found in all entities, false if not.
//-----------------------------------------------------------------------------
bool CMapEntityList::HasInput(const char *szInput)
{
	GDclass *pLastClass = NULL;
	POSITION pos = m_List.GetHeadPosition();
	while (pos != NULL)
	{
		CMapEntity *pEntity = m_List.GetNext(pos);
		GDclass *pClass = pEntity->GetClass();
		if ((pClass != pLastClass) && (pClass != NULL))
		{
			if (pClass->FindInput(szInput) == NULL)
			{
				return(false);
			}

			//
			// Cheap optimization to help minimize redundant checks.
			//
			pLastClass = pClass;
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapEntityList::HasInputOfType(InputOutputType_t eType)
{
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the object that should be added to the selection
//			list because this object was clicked on with a given selection mode.
// Input  : eSelectMode - 
//-----------------------------------------------------------------------------
CMapClass *CMapEntity::PrepareSelection(SelectMode_t eSelectMode)
{
	//
	// Select up the hierarchy when in Groups selection mode if we belong to a group.
	//
	if ((eSelectMode == selectGroups) && (Parent != NULL) && !Parent->IsWorldObject())
	{
		return Parent->PrepareSelection(eSelectMode);
	}

	//
	// Don't select solid entities when in Solids selection mode. We'll select
	// their solid children.
	//
	if ((eSelectMode == selectSolids) && !IsPlaceholder())
	{
		return NULL;
	}

	return this;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapEntity::Render2D(CRender2D *pRender)
{
	// FIXME: we should just render all our children like we do in the 3D view
	Vector vecMins;
	Vector vecMaxs;
	GetRender2DBox(vecMins, vecMaxs);

	CPoint pt;
	CPoint pt2;
	pRender->TransformPoint3D(pt, vecMins);
	pRender->TransformPoint3D(pt2, vecMaxs);

	color32 rgbColor = GetRenderColor();
	pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, rgbColor.r, rgbColor.g, rgbColor.b);

	if (IsPlaceholder())
	{
		//
		// Draw the entity box.
		//
		RECT re;
		re.left = min(pt.x, pt2.x);
		re.right = max(pt.x, pt2.x) + 1;
		re.top = min(pt.y, pt2.y);
		re.bottom = max(pt.y, pt2.y) + 1;

		pRender->Rectangle(re, false);

		//
		// Draw center handle.
		//
		POINT ptCenter;
		ptCenter.x = (pt.x + pt2.x) / 2;
		ptCenter.y = (pt.y + pt2.y) / 2;

		pRender->MoveTo(ptCenter.x - 3, ptCenter.y - 3);
		pRender->LineTo(ptCenter.x + 4, ptCenter.y + 4);
		pRender->MoveTo(ptCenter.x + 3, ptCenter.y - 3);
		pRender->LineTo(ptCenter.x - 4, ptCenter.y + 4);
	}

 	//
	// Render the entity's name and class name if enabled.
	//
	if (s_bShowEntityNames && (pRender->GetZoom() >= 1))
	{
		CPoint ptText(0, 0);

		pRender->SetTextColor(rgbColor.r, rgbColor.g, rgbColor.b, GetRValue(Options.colors.clrBackground), GetGValue(Options.colors.clrBackground), GetBValue(Options.colors.clrBackground));

		const char *pszTargetName = GetKeyValue("targetname");
		if (pszTargetName != NULL)
		{
			ptText.x = pt.x + 2;
			ptText.y = pt.y + 2;
			pRender->DrawText(pszTargetName, ptText, 0, 0, CRender2D::TEXT_JUSTIFY_LEFT | CRender2D::TEXT_SINGLELINE);
		}

		const char *pszClassName = GetClassName();
		if (pszClassName != NULL)
		{
			ptText.x = pt.x + 2;
			ptText.y = pt2.y + 1;
			pRender->DrawText(pszClassName, ptText, 0, 0, CRender2D::TEXT_JUSTIFY_LEFT | CRender2D::TEXT_SINGLELINE);
		}

	}

	//
	// Draw the connections between entities and their targets if enabled.
	//
	if (s_bShowEntityConnections)
	{
		LPCTSTR pszTarget = GetKeyValue("target");
		
		if (pszTarget != NULL)
		{
			CMapWorld *pWorld = GetWorldObject(this);
			MDkeyvalue kv("targetname", pszTarget);

			CMapObjectList FoundEntities;
			FoundEntities.RemoveAll();
			pWorld->EnumChildren((ENUMMAPCHILDRENPROC)FindKeyValue, (DWORD)&kv, MAPCLASS_TYPE(CMapEntity));

			Vector vec;
			GetBoundsCenter(vec);
			pRender->TransformPoint3D(pt, vec);

			POSITION p = FoundEntities.GetHeadPosition();
			while (p)
			{
				CMapClass *pEntity = (CMapEntity *)FoundEntities.GetNext(p);
				pEntity->GetBoundsCenter(vec);
				pRender->TransformPoint3D(pt2, vec);
				pRender->MoveTo(pt);
				pRender->LineTo(pt2);
			}
		}
	}

	if ((GetSelectionState() != SELECT_NONE) && (GetKeyValue("angles") != NULL))
	{
		Vector vecOrigin;
		GetOrigin(vecOrigin);

		QAngle vecAngles;
		GetAngles(vecAngles);
		Vector vecForward;
		AngleVectors(vecAngles, &vecForward);

		pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 255, 255, 0);
		pRender->DrawLine(vecOrigin, vecOrigin + vecForward * 24);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether this entity snaps to half grid or not. Some entities,
//			such as hinges, need to snap to a 0.5 grid to center on geometry.
//-----------------------------------------------------------------------------
bool CMapEntity::ShouldSnapToHalfGrid()
{
	return (GetClass() && GetClass()->ShouldSnapToHalfGrid());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the integer value of the nodeid key of this entity.
//-----------------------------------------------------------------------------
int CMapEntity::GetNodeID(void)
{
	int nNodeID = 0;
	const char *pszNodeID = GetKeyValue("nodeid");
	if (pszNodeID)
	{
		nNodeID = atoi(pszNodeID);
	}
	return nNodeID;
}
