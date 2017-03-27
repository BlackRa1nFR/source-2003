//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: The document. Exposes functions for object creation, deletion, and
//			manipulation. Holds the current tool. Handles GUI messages that are
//			view-independent.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <direct.h>
#include <io.h>
#include <mmsystem.h>
#include <process.h>
#include <direct.h>
#include "BuildNum.h"
#include "CustomMessages.h"
#include "EditGameConfigs.h"
#include "EditPrefabDlg.h"
#include "EntityReportDlg.h"
#include "FaceEditSheet.h"
#include "GameData.h"
#include "GlobalFunctions.h"
#include "GotoBrushDlg.h"
#include "HelpID.h"
#include "History.h"
#include "MainFrm.h"
#include "MapAnimator.h"
#include "MapCheckDlg.h"
#include "MapDefs.h"		// dvs: For COORD_NOTINIT
#include "MapDisp.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapGroup.h"
#include "MapInfoDlg.h"
#include "MapSolid.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "MapWorld.h"
#include "ObjectProperties.h"
#include "OptionProperties.h"
#include "Options.h"
#include "ProcessWnd.h"
#include "PasteSpecialDlg.h"
#include "Prefabs.h"
#include "Prefab3D.h"
#include "ReplaceTexDlg.h"
#include "RunMap.h"
#include "RunMapExpertDlg.h"
#include "SaveInfo.h"

#include "ToolBlock.h"
#include "ToolCamera.h"
#include "ToolClipper.h"
#include "ToolCordon.h"
#include "ToolEntity.h"
#include "ToolMorph.h"
#include "ToolOverlay.h"
#include "ToolPath.h"
#include "ToolSelection.h"
#include "ToolMagnify.h"
#include "ToolMaterial.h"
#include "ToolManager.h"

#include "SelectEntityDlg.h"
#include "Shell.h"
#include "StatusBarIDs.h"
#include "StrDlg.h"
#include "TextureSystem.h"
#include "TextureConverter.h"
#include "TransformDlg.h"
#include "VisGroup.h"
#include "Worldcraft.h"
#include "ibsplighting.h"
#include "camera.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define KeyInt( key, dest ) \
	if (stricmp(szKey, key) != 0) \
		; \
	else \
	{ \
		CChunkFile::ReadKeyValueInt(szValue, dest); \
	}

#define KeyBool( key, dest ) \
	if (stricmp(szKey, key) != 0) \
		; \
	else \
	{ \
		CChunkFile::ReadKeyValueBool(szValue, dest); \
	}


#define MAX_REPLACE_LINE_LENGTH 256


extern BOOL bSaveVisiblesOnly;
extern CShell g_Shell;


IMPLEMENT_DYNCREATE(CMapDoc, CDocument)

BEGIN_MESSAGE_MAP(CMapDoc, CDocument)
	//{{AFX_MSG_MAP(CMapDoc)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_MAP_SNAPTOGRID, OnMapSnaptogrid)
	ON_COMMAND(ID_MAP_ENTITY_GALLERY, OnMapEntityGallery)
	ON_UPDATE_COMMAND_UI(ID_MAP_SNAPTOGRID, OnUpdateMapSnaptogrid)
	ON_COMMAND(ID_EDIT_APPLYTEXTURE, OnEditApplytexture)
	ON_COMMAND(ID_TOOLS_SUBTRACTSELECTION, OnToolsSubtractselection)
	ON_COMMAND(ID_MAP_ENABLELIGHTPREVIEW, OnEnableLightPreview)
	ON_COMMAND(ID_ENABLE_LIGHT_PREVIEW_CUSTOM_FILENAME, OnEnableLightPreviewCustomFilename)
	ON_COMMAND(ID_MAP_DISABLELIGHTPREVIEW, OnDisableLightPreview)
	ON_COMMAND(ID_MAP_UPDATELIGHTPREVIEW, OnUpdateLightPreview)
	ON_COMMAND(ID_MAP_TOGGLELIGHTPREVIEW, OnToggleLightPreview)
	ON_COMMAND(ID_MAP_ABORTLIGHTCALCULATION, OnAbortLightCalculation)
	ON_COMMAND(ID_EDIT_COPYWC, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPYWC, OnUpdateEditFunction)
	ON_COMMAND(ID_EDIT_PASTEWC, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEWC, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_CUTWC, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUTWC, OnUpdateEditFunction)
	ON_COMMAND(ID_TOOLS_GROUP, OnToolsGroup)
	ON_COMMAND(ID_TOOLS_UNGROUP, OnToolsUngroup)
	ON_COMMAND(ID_VIEW_GRID, OnViewGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRID, OnUpdateViewGrid)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECTALL, OnUpdateEditFunction)
	ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE, OnUpdateEditFunction)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_MAP_GRIDLOWER, OnMapGridlower)
	ON_COMMAND(ID_MAP_GRIDHIGHER, OnMapGridhigher)
	ON_COMMAND(ID_EDIT_TOWORLD, OnEditToWorld)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_FILE_EXPORTAGAIN, OnFileExportAgain)
	ON_COMMAND(ID_EDIT_MAPPROPERTIES, OnEditMapproperties)
	ON_COMMAND(ID_FILE_CONVERT_WAD, OnFileConvertWAD)
	ON_UPDATE_COMMAND_UI(ID_FILE_CONVERT_WAD, OnUpdateFileConvertWAD)
	ON_COMMAND(ID_FILE_RUNMAP, OnFileRunmap)
	ON_COMMAND(ID_TOOLS_HIDEITEMS, OnToolsHideitems)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_HIDEITEMS, OnUpdateToolsHideitems)
	ON_COMMAND(ID_TOOLS_HIDE_ENTITY_NAMES, OnToolsHideEntityNames)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_HIDE_ENTITY_NAMES, OnUpdateToolsHideEntityNames)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditFunction)
	ON_COMMAND(ID_MAP_INFORMATION, OnMapInformation)
	ON_COMMAND(ID_VIEW_CENTERONSELECTION, OnViewCenteronselection)
	ON_COMMAND(ID_VIEW_CENTER3DVIEWSONSELECTION, Center3DViewsOnSelection)
	ON_COMMAND(ID_EDIT_PASTESPECIAL, OnEditPastespecial)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL, OnUpdateEditPastespecial)
	ON_COMMAND(ID_EDIT_SELNEXT, OnEditSelnext)
	ON_COMMAND(ID_EDIT_SELPREV, OnEditSelprev)
	ON_COMMAND(ID_VIEW_HIDESELECTEDOBJECTS, OnViewHideselectedobjects)
	ON_COMMAND(ID_VIEW_SHOWHIDDENOBJECTS, OnViewShowhiddenobjects)
	ON_COMMAND(ID_MAP_CHECK, OnMapCheck)
	ON_COMMAND(ID_VIEW_SHOWCONNECTIONS, OnViewShowconnections)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWCONNECTIONS, OnUpdateViewShowconnections)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_TOOLS_CREATEPREFAB, OnToolsCreateprefab)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_CREATEPREFAB, OnUpdateEditFunction)
	ON_COMMAND(ID_INSERTPREFAB_ORIGINAL, OnInsertprefabOriginal)
	ON_COMMAND(ID_EDIT_REPLACETEX, OnEditReplacetex)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACETEX, OnUpdateEditFunction)
	ON_COMMAND(ID_TOOLS_HOLLOW, OnToolsHollow)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_HOLLOW, OnUpdateEditFunction)
	ON_COMMAND(ID_TOOLS_SNAPSELECTEDTOGRID, OnToolsSnapselectedtogrid)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SPLITFACE, OnUpdateToolsSplitface)
	ON_COMMAND(ID_TOOLS_SPLITFACE, OnToolsSplitface)
	ON_COMMAND(ID_TOOLS_TRANSFORM, OnToolsTransform)
	ON_COMMAND(ID_TOOLS_TOGGLETEXLOCK, OnToolsToggletexlock)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_TOGGLETEXLOCK, OnUpdateToolsToggletexlock)
	ON_COMMAND(ID_TOOLS_TEXTUREALIGN, OnToolsTextureAlignment)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_TEXTUREALIGN, OnUpdateToolsTextureAlignment)
	ON_COMMAND(ID_TOGGLE_CORDON, OnToggleCordon)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_CORDON, OnUpdateToggleCordon)
	ON_COMMAND(ID_VIEW_HIDENONSELECTEDOBJECTS, OnViewHidenonselectedobjects)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HIDENONSELECTEDOBJECTS, OnUpdateViewHidenonselectedobjects)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_HELPERS, OnUpdateViewShowHelpers)
	ON_COMMAND(ID_VIEW_SHOW_HELPERS, OnViewShowHelpers)
	ON_COMMAND(ID_TOGGLE_GROUPIGNORE, OnToggleGroupignore)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_GROUPIGNORE, OnUpdateToggleGroupignore)
	ON_COMMAND(ID_VSCALE_TOGGLE, OnVscaleToggle)
	ON_COMMAND(ID_MAP_ENTITYREPORT, OnMapEntityreport)
	ON_COMMAND(ID_TOGGLE_SELECTBYHANDLE, OnToggleSelectbyhandle)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_SELECTBYHANDLE, OnUpdateToggleSelectbyhandle)
	ON_COMMAND(ID_TOGGLE_INFINITESELECT, OnToggleInfiniteselect)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_INFINITESELECT, OnUpdateToggleInfiniteselect)
	ON_COMMAND(ID_FILE_EXPORTTODXF, OnFileExporttodxf)
	ON_UPDATE_COMMAND_UI(ID_EDIT_APPLYTEXTURE, OnUpdateEditApplytexture)
	ON_COMMAND(ID_VIEW_HIDEPATHS, OnViewHidepaths)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HIDEPATHS, OnUpdateViewHidepaths)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SNAPSELECTEDTOGRID, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_TRANSFORM, OnUpdateEditFunction)
	ON_COMMAND(ID_EDIT_CLEARSELECTION, OnEditClearselection)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEARSELECTION, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SUBTRACTSELECTION, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_GROUP, OnUpdateGroupEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_UNGROUP, OnUpdateGroupEditFunction)
	ON_UPDATE_COMMAND_UI(ID_EDIT_TOWORLD, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MAPPROPERTIES, OnUpdateEditFunction)
	ON_COMMAND(ID_MAP_LOADPOINTFILE, OnMapLoadpointfile)
	ON_COMMAND(ID_MAP_UNLOADPOINTFILE, OnMapUnloadpointfile)
	ON_COMMAND(ID_TOGGLE_3D_GRID, OnToggle3DGrid)
	ON_UPDATE_COMMAND_UI(ID_TOGGLE_3D_GRID, OnUpdateToggle3DGrid)
	ON_COMMAND(ID_EDIT_TOENTITY, OnEditToEntity)
	ON_UPDATE_COMMAND_UI(ID_EDIT_TOENTITY, OnUpdateEditFunction)
	ON_COMMAND_EX(ID_EDIT_UNDO, OnUndoRedo)
	ON_COMMAND_EX(ID_EDIT_REDO, OnUndoRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateUndoRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateUndoRedo)
	ON_COMMAND(ID_VSCALE_CHANGED, OnChangeVertexscale)
	ON_COMMAND(ID_GOTO_BRUSH, OnMapGotoBrush)
	ON_COMMAND(ID_EDIT_FINDENTITIES, OnEditFindEntities)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FINDENTITIES, OnUpdateEditFunction)
	ON_COMMAND( ID_TOOLS_DISP_SOLIDDRAW, OnToggleDispSolidMask )
	ON_UPDATE_COMMAND_UI( ID_TOOLS_DISP_SOLIDDRAW, OnUpdateToggleSolidMask )	
	ON_COMMAND( ID_TOOLS_DISP_DRAWWALKABLE, OnToggleDispDrawWalkable )
	ON_UPDATE_COMMAND_UI( ID_TOOLS_DISP_DRAWWALKABLE, OnUpdateToggleDispDrawWalkable )	
	ON_COMMAND( ID_TOOLS_DISP_DRAWBUILDABLE, OnToggleDispDrawBuildable )
	ON_UPDATE_COMMAND_UI( ID_TOOLS_DISP_DRAWBUILDABLE, OnUpdateToggleDispDrawBuildable )	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


static CTypedPtrList<CPtrList, CMapDoc*> s_ActiveDocs;
CMapDoc *CMapDoc::m_pMapDoc = NULL;


//
// Clipboard. Global to all documents to allow copying from one document and
// pasting in another.
//
struct Clipboard_t
{
	CMapObjectList Objects;
	CMapWorld *pSourceWorld;
	BoundBox Bounds;
	Vector vecOriginalCenter;
};

static Clipboard_t s_Clipboard;


struct BatchReplaceTextures_t
{
	char szFindTexName[MAX_REPLACE_LINE_LENGTH];		// Texture to find.
	char szReplaceTexName[MAX_REPLACE_LINE_LENGTH];	// Texture to replace the found texture with.
};


struct AddNonSelectedInfo_t
{
	CVisGroup *pGroup;
	int nCount;
	CMapObjectList *pList;
	BoundBox *pBox;
	CMapWorld *pWorld;
};


struct ReplaceTexInfo_t
{
	char szFind[128];
	char szReplace[128];
	int iAction;
	int nReplaced;
	int iFindLen;	// strlen(szFind) - for speed
	CMapWorld *pWorld;
	CMapDoc *pDoc;
	BOOL bMarkOnly;
	BOOL bHidden;

};


struct FindObjectNumber_t
{
	int nObjectNumber;			// Object number to find.
	BoundBox *pFindInBox;		// If not NULL, provides a bounding box to search within.
	BOOL bVisiblesOnly;			// Whether to only search visible objects.
	CMapWorld *pWorld;			// Pointer to world object. Used for visiblility determination.
	int nObjectCount;			// Current object counter used by the callback function.
	CMapClass *pObjectFound;	// Points to object found, NULL if unsuccessful.
};


struct FindEntity_t
{
	char szClassName[MAX_PATH];	// 
	Vector Pos;					// 
	CMapEntity *pEntityFound;	// Points to object found, NULL if unsuccessful.
};


struct SelectBoxInfo_t
{
	CMapDoc *pDoc;
	BoundBox *pBox;
	BOOL bInside;
	SelectMode_t eSelectMode;
};


struct ExportDXFInfo_t
{
	int nObject;
	BOOL bVisOnly;
	CMapWorld *pWorld;
	FILE *fp;
};


//-----------------------------------------------------------------------------
// Purpose: Constructor. Attaches all tools members to this document. Adds this
//			document to the list of active documents.
//-----------------------------------------------------------------------------
CMapDoc::CMapDoc(void)
{
	int nSize = sizeof(CMapFace);
	nSize = sizeof(CMapSolid);

	m_bLoading = false;
	m_pWorld = NULL;

	//
	// Create all the tools.
	//
	m_pToolSelection = new Selection3D;
	m_pToolBlock = new CToolBlock;
	m_pToolMarker = new Marker3D;
	m_pToolCamera = new Camera3D;
	m_pToolMorph = new Morph3D;
	m_pToolClipper = new Clipper3D;
	m_pToolCordon = new Cordon3D;
	m_pToolPath = new Path3D;
	m_pToolOverlay = new CToolOverlay;

	//
	// Attach the document to all the old style tools.
	//
	m_pToolSelection->SetDocument(this);
	m_pToolBlock->SetDocument(this);
	m_pToolMarker->SetDocument(this);
	m_pToolCamera->SetDocument(this);
	m_pToolMorph->SetDocument(this);
	m_pToolClipper->SetDocument(this);
	m_pToolCordon->SetDocument(this);
	m_pToolPath->SetDocument(this);
	m_pToolOverlay->SetDocument(this);

	//
	// Set up undo/redo system.
	//
	m_pUndo = new CHistory;
	m_pRedo = new CHistory;

	m_pUndo->SetDocument(this);
	m_pUndo->SetOpposite(TRUE, m_pRedo);

	m_pRedo->SetDocument(this);
	m_pRedo->SetOpposite(FALSE, m_pUndo);

	ASSERT(GetMainWnd());

	m_bHidePaths = false;
	m_bHideItems = false;
	m_bSnapToGrid = false;
	m_bShowGrid = true;
	m_nGridSpacing = Options.view2d.iDefaultGrid;
	m_bShow3DGrid = false;

	m_nDocVersion = 0;

	m_nNextMapObjectID = 1;
	m_nNextNodeID = 1;

	m_pGame = NULL;
	m_nPFPoints = 0;
	m_pPFPoints = NULL;

	m_flAnimationTime = 0.0f;

	OnMapSnaptogrid();

	m_bEditingPrefab = false;
	m_bPrefab = false;

	m_bIsAnimating = false;

	m_strLastExportFileName = "";

	m_bDispSolidDrawMask = true;
	m_bDispDrawWalkable = false;
	m_bDispDrawBuildable = false;

	ClearHitList();

	s_ActiveDocs.AddTail(this);

	m_pBSPLighting = 0;

	m_SmoothingGroupVisual = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CMapDoc::~CMapDoc(void)
{
	GetMainWnd()->pObjectProperties->AbortData();

	//
	// Remove this doc from the list of active docs.
	//
	POSITION p = s_ActiveDocs.Find(this);
	ASSERT(p != NULL);
	if (p != NULL)
	{
		s_ActiveDocs.RemoveAt(p);
	}

	if (this == GetActiveMapDoc())
	{
		SetActiveMapDoc(NULL);
	}

	DeleteContents();

	delete m_pUndo;
	delete m_pRedo;

	delete m_pToolSelection;
	delete m_pToolBlock;
	delete m_pToolMarker;
	delete m_pToolCamera;
	delete m_pToolMorph;
	delete m_pToolClipper;
	delete m_pToolCordon;
	delete m_pToolPath;
	delete m_pToolOverlay;

	OnDisableLightPreview();
}


//-----------------------------------------------------------------------------
// Purpose: Adds the given object to the list if it is a leaf object (no children).
// Input  : pObject - Object to add to the list.
//			pList - List to put the children in.
// Output : Returns TRUE to continue enumerating when called from EnumChildren.
//-----------------------------------------------------------------------------
BOOL CMapDoc::AddLeavesToListCallback(CMapClass *pObject, CMapObjectList *pList)
{
	if (pObject->GetChildCount() == 0)
	{
		pList->AddTail(pObject);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Callback for adding unselected objects to a group.
// Input  : pObject - Object to consider for addition to the group.
//			pInfo - Structure to receive information about the enumeration.
// Output : Returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
BOOL CMapDoc::AddUnselectedToGroupCallback(CMapClass *pObject, AddNonSelectedInfo_t *pInfo)
{
	if (pObject->IsSelected() || !pObject->IsVisible())
	{
		return(TRUE);
	}

	pObject->SetVisGroup(pInfo->pGroup);
	pInfo->nCount++;
	pInfo->pList->AddTail(pObject);

	Vector mins;
	Vector maxs;
	pObject->GetRender2DBox(mins, maxs);
	pInfo->pBox->UpdateBounds(mins, maxs);

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Called after loading a VMF file. Assigns all objects to their proper
//			visgroups. It also finds the next unique ID to use when creating new objects.
//-----------------------------------------------------------------------------
void CMapDoc::AssignToGroups(void)
{
	EnumChildrenPos_t pos;
	CMapClass *pChild = m_pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		//
		// HACK: If this entity needs a node ID and doesn't have one, set it now. 
		//		 Would be better implemented more generically.
		//
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if (pEntity != NULL)
		{
			if (pEntity->IsNodeClass() && (pEntity->GetNodeID() == 0))
			{
				int nID = GetNextNodeID();
				char szID[80];
				itoa(nID, szID, 10);
				pEntity->SetKeyValue("nodeid", szID);
			}
		}

		//
		// Assign the object to its visgroup, if any.
		//
		const char *pszVisGroupID = pChild->GetEditorKeyValue("visgroupid");
		if (pszVisGroupID != NULL)
		{
			unsigned int nID = (unsigned int)atoi(pszVisGroupID);
			CVisGroup *pVisGroup = VisGroups_GroupForID(nID);
			pChild->SetVisGroup(pVisGroup);
		}

		pChild->RemoveEditorKeys();
		pChild = m_pWorld->GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Begins a remote shell editing session. This causes the GUI to be
//			disabled to avoid version mismatches between Worldcraft and the shell
//			client.
//-----------------------------------------------------------------------------
void CMapDoc::BeginShellSession(void)
{
	//
	// Disable all our views.
	//
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		pView->EnableWindow(FALSE);
	}

	//
	// Set the modified flag to update our version number. This marks the working
	// version of the map in memory as different from the saved version on disk.
	//
	SetModifiedFlag();

	GetMainWnd()->BeginShellSession();
}


Vector CMapDoc::GetSelectedCenter()
{
	Vector vRet;
	if (g_pToolManager->GetToolID() == TOOL_MORPH)
	{
		m_pToolMorph->GetSelectedCenter(vRet);
	}
	else if ((g_pToolManager->GetToolID() == TOOL_PATH) && m_pToolPath->GetSelectedCount())
	{
		m_pToolPath->GetSelectedCenter(vRet);
	}
	else if (g_pToolManager->GetToolID() == TOOL_BLOCK)
	{
		 m_pToolBlock->GetBoundsCenter(vRet);
	}
	else if (!m_pToolSelection->IsEmpty())
	{
		m_pToolSelection->GetBoundsCenter(vRet);
	}
	else
	{
		m_pWorld->GetBoundsCenter(vRet);
	}
	return vRet;
}


//-----------------------------------------------------------------------------
// Purpose: Centers the selection in all the 2D views.
//-----------------------------------------------------------------------------
void CMapDoc::CenterSelection(void)
{
	VIEW2DINFO vi;
	vi.wFlags = VI_CENTER;
	vi.ptCenter = GetSelectedCenter();

	SetView2dInfo(vi);
}


void CMapDoc::Center3DViewsOnSelection()
{
	Center3DViewsOn( GetSelectedCenter() );
}


//-----------------------------------------------------------------------------
// Purpose: Called after loading a VMF file. Finds the next unique IDs to use
//			when creating new objects, nodes, and faces.
//-----------------------------------------------------------------------------
void CMapDoc::CountGUIDs(void)
{
	CMapObjectPairList GroupedObjects;

	// This increments the CMapWorld face ID but it doesn't matter since we're setting it below.
	int nNextFaceID = m_pWorld->FaceID_GetNext();

	EnumChildrenPos_t pos;
	CMapClass *pChild = m_pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		//
		// Keep track of the highest numbered object ID in this document.
		//
		if (pChild->GetID() >= m_nNextMapObjectID)
		{
			m_nNextMapObjectID = pChild->GetID() + 1;
		}

		//
		// Keep track of the highest numbered node ID in this document.
		//
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if (pEntity != NULL)
		{
			//
			// Blah. Classes aren't assigned until PostLoadWorld, so we have to
			// look at our classname keyvalue to determine whether we are a node class.
			//
			const char *pszClass = pEntity->GetKeyValue("classname");
			if (pEntity->IsNodeClass(pszClass))
			{
				int nID = pEntity->GetNodeID();
				if (nID >= m_nNextNodeID)
				{
					m_nNextNodeID = nID + 1;
				}
			}
		}
		else
		{
			//
			// Keep track of the highest numbered face ID in this document.
			//
			CMapSolid *pSolid = dynamic_cast<CMapSolid *>(pChild);
			if (pSolid != NULL)
			{
				for (int nFace = 0; nFace < pSolid->GetFaceCount(); nFace++)
				{
					CMapFace *pFace = pSolid->GetFace(nFace);
					int nFaceID = pFace->GetFaceID();
					if (nFaceID >= nNextFaceID)
					{
						nNextFaceID = nFaceID + 1;
					}
				}
			}
		}

		pChild = m_pWorld->GetNextDescendent(pos);
	}

	m_pWorld->FaceID_SetNext(nNextFaceID);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszClassName - 
//			x - 
//			y - 
//			z - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
CMapEntity *CMapDoc::CreateEntity(const char *pszClassName, float x, float y, float z)
{
	CMapEntity *pEntity = new CMapEntity;
	if (pEntity != NULL)
	{
		GetHistory()->MarkUndoPosition(NULL, "New Entity");

		pEntity->SetPlaceholder(TRUE);

		Vector Pos;
		Pos[0] = x;
		Pos[1] = y;
		Pos[2] = z;
		pEntity->SetOrigin(Pos);

		pEntity->SetClass(pszClassName);

		AddObjectToWorld(pEntity);
		GetHistory()->KeepNew(pEntity);

		//
		// Update all the views.
		//
		UpdateBox ub;
		CMapObjectList ObjectList;
		ObjectList.AddTail(pEntity);
		ub.Objects = &ObjectList;
		ub.Box = *((BoundBox *)pEntity);
		UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);

		SetModifiedFlag();
	}

	return(pEntity);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszClassName - 
//			x - 
//			y - 
//			z - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
// dvs: Seems better to move some of this functionality into DeleteObject and replace calls to this with FindEntity/DeleteObject calls.
//      Probably need to replicate all functions in the doc as follows: DeleteObject, DeleteObjectList to optimize updates. Either that
//		or we need a way to lock and unlock updates.
bool CMapDoc::DeleteEntity(const char *pszClassName, float x, float y, float z)
{
	CMapEntity *pEntity = FindEntity(pszClassName, x, y, z);
	if (pEntity != NULL)
	{
		GetHistory()->MarkUndoPosition(NULL, "Delete");

		DeleteObject(pEntity);

		//
		// If the object is in the selection, remove it from the selection.
		//
		Selection_Remove(pEntity);

		UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);

		SetModifiedFlag();

		//
		// Make sure the object properties dialog is not referencing
		// deleted object.
		//
		GetMainWnd()->pObjectProperties->AbortData();
		GetMainWnd()->SetFocus();
		return(true);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Ends a remote shell editing session. This enables the GUI that was
//			disabled by BeginShellSession.
//-----------------------------------------------------------------------------
void CMapDoc::EndShellSession(void)
{
	//
	// Enable all our views.
	//
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		pView->EnableWindow(TRUE);
	}

	GetMainWnd()->EndShellSession();
}


//-----------------------------------------------------------------------------
// Purpose: Finds a brush by number, within a given entity. Note that the world
//			entity is not a CMapEntity, hence the parent entity is passed in as
//			a CMapClass instead of a CMapEntity.
// Input  : pEntity - A pointer to the object to search in.
//			nBrushNumber - The ordinal of the brush within the entity in the MAP file.
//			pBoundBox - The cordon bounds to use, NULL if none.
//			bVisiblesOnly - Whether to limit search to visible objects.
// Output : Returns a pointer to the entity (or the world) indicated by nEntityNumber.
//-----------------------------------------------------------------------------
CMapSolid *CMapDoc::FindBrushNumber(CMapClass *pEntity, int nBrushNumber, BoundBox *pBoundBox, BOOL bVisiblesOnly)
{
	FindObjectNumber_t FindInfo;

	memset(&FindInfo, 0, sizeof(FindInfo));
	FindInfo.bVisiblesOnly = bVisiblesOnly;
	FindInfo.pFindInBox = pBoundBox;
	FindInfo.pWorld = m_pWorld;
	FindInfo.nObjectNumber = nBrushNumber;

	//
	// If we have an entity to look in, search for the brush.
	//
	if (pEntity != NULL)
	{
		pEntity->EnumChildrenRecurseGroupsOnly((ENUMMAPCHILDRENPROC)FindObjectNumberCallback, (DWORD)&FindInfo, MAPCLASS_TYPE(CMapSolid));
	}

	if (FindInfo.pObjectFound != NULL)
	{
		ASSERT(FindInfo.pObjectFound->IsMapClass(MAPCLASS_TYPE(CMapSolid)));
	}

	return((CMapSolid *)FindInfo.pObjectFound);
}


//-----------------------------------------------------------------------------
// Purpose: Finds an object in the map by number. The number corresponds to the
//			order in which the object is written to the map file. Thus, through
//			this function, brushes and entities can be located by ordinal, as
//			reported by the MAP compile tools.
// Input  : pObject - Object being checked for a match.
//			pFindInfo - Structure containing the search criterea.
// Output : Returns FALSE if this is the object that we are looking for, TRUE
//			to continue iterating.
//-----------------------------------------------------------------------------
BOOL CMapDoc::FindEntityCallback(CMapClass *pObject, FindEntity_t *pFindInfo)
{
	CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);

	if (pEntity != NULL)
	{
		Vector Pos;
		pEntity->GetOrigin(Pos);

		// HACK: Round to origin integers since entity origins are rounded when
		//       saving to MAP file. This makes finding entities from the engine
		//       in Worldcraft work.
		Pos[0] = rint(Pos[0]);
		Pos[1] = rint(Pos[1]);
		Pos[2] = rint(Pos[2]);

		if (VectorCompare(Pos, pFindInfo->Pos))
		{
			if (stricmp(pEntity->GetClassName(), pFindInfo->szClassName) == 0)
			{
				pFindInfo->pEntityFound = pEntity;
				return(FALSE);
			}
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Finds an object in the map by number. The number corresponds to the
//			order in which the object is written to the map file. Thus, through
//			this function, brushes and entities can be located by ordinal, as
//			reported by the MAP compile tools.
// Input  : pObject - Object being checked for a match.
//			nObjectNumber - Object number being searched for.
// Output : Returns FALSE if this is the object that we are looking for, TRUE
//			to continue iterating.
//-----------------------------------------------------------------------------
BOOL CMapDoc::FindObjectNumberCallback(CMapClass *pObject, FindObjectNumber_t *pFindInfo)
{
	if ((pFindInfo->pFindInBox == NULL) || pObject->IsIntersectingBox(pFindInfo->pFindInBox->bmins, pFindInfo->pFindInBox->bmaxs))
	{
		if (!pFindInfo->bVisiblesOnly || pObject->IsVisible())
		{
			if (pFindInfo->nObjectCount == pFindInfo->nObjectNumber)
			{
				//
				// Found it!
				//
				pFindInfo->pObjectFound = pObject;

				return(FALSE);
			}

			pFindInfo->nObjectCount++;
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity by classname and position. Some ambiguity if two
//			entities of the same name are at the same position.
// Input  : pszClassName - Class name of entity to find, ie "info_node".
//			x, y, z - Position of entity in world coordinates.
// Output : Returns a pointer to the entity, NULL if none was found.
//-----------------------------------------------------------------------------
CMapEntity *CMapDoc::FindEntity(const char *pszClassName, float x, float y, float z)
{
	CMapEntity *pEntity = NULL;

	if (pszClassName != NULL)
	{
		FindEntity_t FindInfo;

		memset(&FindInfo, 0, sizeof(FindInfo));
		strcpy(FindInfo.szClassName, pszClassName);

		// dvs: HACK - only find by integer coordinates because Worldcraft rounds
		//		entity origins when saving the MAP file.
		FindInfo.Pos[0] = rint(x);
		FindInfo.Pos[1] = rint(y);
		FindInfo.Pos[2] = rint(z);

		m_pWorld->EnumChildren((ENUMMAPCHILDRENPROC)FindEntityCallback, (DWORD)&FindInfo, MAPCLASS_TYPE(CMapEntity));

		if (FindInfo.pEntityFound != NULL)
		{
			ASSERT(FindInfo.pEntityFound->IsMapClass(MAPCLASS_TYPE(CMapEntity)));
		}

		pEntity = FindInfo.pEntityFound;
	}

	return(pEntity);
}


//-----------------------------------------------------------------------------
// Purpose: Finds all entities in the map with a given class name.
// Input  : pFound - List of entities with the class name.
//			pszClassName - Class name to match, case insensitive.
// Output : Returns true if any matches were found, false if not.
//-----------------------------------------------------------------------------
bool CMapDoc::FindEntitiesByClassName(CMapEntityList *pFound, const char *pszClassName)
{
	pFound->RemoveAll();

	EnumChildrenPos_t pos;
	CMapClass *pChild = m_pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if (pEntity != NULL)
		{
			const char *pszThisClass = pEntity->GetClassName();

			if (pszThisClass != NULL)
			{
				if ((pszClassName != NULL) && (!stricmp(pszClassName, pszThisClass)))
				{
					pFound->AddTail(pEntity);
				}
			}
			else if (pszClassName == NULL)
			{
				pFound->AddTail(pEntity);
			}
		}

		pChild = m_pWorld->GetNextDescendent(pos);
	}

	return(pFound->GetCount() != 0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFound - 
//			*pszTargetName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapDoc::FindEntitiesByKeyValue(CMapEntityList *pFound, const char *pszKey, const char *pszValue)
{
	pFound->RemoveAll();

	EnumChildrenPos_t pos;
	CMapClass *pChild = m_pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if (pEntity != NULL)
		{
			const char *pszThisValue = pEntity->GetKeyValue(pszKey);

			if (pszThisValue != NULL)
			{
				if ((pszValue != NULL) && (!stricmp(pszValue, pszThisValue)))
				{
					pFound->AddTail(pEntity);
				}
			}
			else if (pszValue == NULL)
			{
				pFound->AddTail(pEntity);
			}
		}

		pChild = m_pWorld->GetNextDescendent(pos);
	}

	return(pFound->GetCount() != 0);
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity by number. Note that entity zero is the world, which
//			is not actually a CMapEntity! That is why this returns a CMapClass.
// Input  : nEntityNumber - The ordinal of the entity in the MAP file.
//			pBoundBox - The cordon bounds to use, NULL if none.
//			bVisiblesOnly - Whether to limit search to visible objects.
// Output : Returns a pointer to the entity (or the world) indicated by nEntityNumber.
//-----------------------------------------------------------------------------
CMapClass *CMapDoc::FindEntityNumber(int nEntityNumber, BoundBox *pBoundBox, BOOL bVisiblesOnly)
{
	CMapClass *pEntity;

	//
	// The world entity is entity zero, so if the entity is nonzero, find that entity.
	//
	if (nEntityNumber != 0)
	{
		FindObjectNumber_t FindInfo;

		memset(&FindInfo, 0, sizeof(FindInfo));
		FindInfo.nObjectNumber = nEntityNumber;
		FindInfo.bVisiblesOnly = bVisiblesOnly;
		FindInfo.pFindInBox = pBoundBox;
		FindInfo.pWorld = m_pWorld;
		FindInfo.nObjectCount = 1;

		m_pWorld->EnumChildren((ENUMMAPCHILDRENPROC)FindObjectNumberCallback, (DWORD)&FindInfo, MAPCLASS_TYPE(CMapEntity));

		if (FindInfo.pObjectFound != NULL)
		{
			ASSERT(FindInfo.pObjectFound->IsMapClass(MAPCLASS_TYPE(CMapEntity)));
		}

		pEntity = FindInfo.pObjectFound;
	}
	//
	// If the entity is zero, look for the brush in the world entity.
	//
	else
	{
		pEntity = m_pWorld;
	}

	return(pEntity);
}


//-----------------------------------------------------------------------------
// Purpose: Callback for forcing an object's visibility to true or false.
// Input  : pObject - Object whose visiblity to set.
//			bVisible - true to make visible, false to make invisible.
// Output : Returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
BOOL CMapDoc::ForceVisibilityCallback(CMapClass *pObject, bool bVisible)
{
	pObject->SetVisible(bVisible);
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
POSITION CMapDoc::GetFirstDocPosition(void)
{
	return(s_ActiveDocs.GetHeadPosition());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapDoc *CMapDoc::GetNextDoc(POSITION &p)
{
	return(s_ActiveDocs.GetNext(p));
}


//-----------------------------------------------------------------------------
// Purpose: Invokes a dialog for finding entities by targetname.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditFindEntities(void)
{
	CStrDlg dlg(0, "", "Targetname to find:", "Find Entities");
	if (dlg.DoModal() == IDOK)
	{
		CMapEntityList Found;
		FindEntitiesByKeyValue(&Found, "targetname", dlg.m_string);
		if (Found.GetCount() != 0)
		{
			CMapObjectList Select;
			POSITION pos = Found.GetHeadPosition();
			while (pos != NULL)
			{
				CMapEntity *pEntity = Found.GetNext(pos);
				Select.AddTail(pEntity);
			}

			SelectObjectList(&Select);
			CenterSelection();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Brings up the 'Go to brush number' dialog and selects the indicated
//			brush, if it can be found.
//-----------------------------------------------------------------------------
void CMapDoc::OnMapGotoBrush(void)
{
	CGotoBrushDlg dlg;

	if (dlg.DoModal() == IDOK)
	{
		//
		// Set up the bounding box to use if the cordon is enabled. Also set up
		// whether we are searching only in objects or not. These settings help
		// insure that our iteration of the brushes here is identical to the
		// iteration when the map file was saved, preserving entity / brush numbers.
		//
		CMapClass *pEntity = FindEntityNumber(dlg.m_nEntityNumber, Cordon_IsCordoning() ? m_pToolCordon : NULL, dlg.m_bVisiblesOnly);

		//
		// If we found the entity, look for the brush by number.
		//		
		if (pEntity != NULL)
		{
			CMapSolid *pSolid = FindBrushNumber(pEntity, dlg.m_nBrushNumber, Cordon_IsCordoning() ? m_pToolCordon : NULL, dlg.m_bVisiblesOnly);

			//
			// If we found the brush, select it and center the 2D views on it.
			//
			if (pSolid != NULL)
			{
				SelectObject(pSolid, scSelect | scClear | scUpdateDisplay);
				OnViewCenteronselection();
			}
			else
			{
				AfxMessageBox("That brush number does not exist.");
			}
		}
		else
		{
			AfxMessageBox("That entity number does not exist.");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : TRUE if the document was successfully initialized; otherwise FALSE.
//-----------------------------------------------------------------------------
BOOL CMapDoc::OnNewDocument(void)
{
	if (!CDocument::OnNewDocument())
	{
		return(FALSE);
	}

	if (!SelectDocType())
	{
		return(FALSE);
	}

	Initialize();

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Creates an empty world and initializes the path list.
//-----------------------------------------------------------------------------
void CMapDoc::Initialize(void)
{
	ASSERT(!m_pWorld);

	m_pWorld = new CMapWorld;
	m_pWorld->CullTree_Build();
	m_pToolPath->SetPathList(&m_pWorld->m_Paths);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadCamerasCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	return(pDoc->m_pToolCamera->LoadVMF(pFile));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadCordonCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	return(pDoc->m_pToolCordon->LoadVMF(pFile));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadEntityCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	CMapEntity *pEntity = new CMapEntity;

	ChunkFileResult_t eResult = pEntity->LoadVMF(pFile);

	if (eResult == ChunkFile_Ok)
	{
		CMapWorld *pWorld = pDoc->GetMapWorld();
		pWorld->AddChild(pEntity);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadHiddenCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("entity", (ChunkHandler_t)CMapDoc::LoadEntityCallback, pDoc);

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk();
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			szChunkName - 
//			eError - 
// Output : Returns true to continue loading, false to stop loading.
//-----------------------------------------------------------------------------
bool CMapDoc::HandleLoadError(CChunkFile *pFile, const char *szChunkName, ChunkFileResult_t eError, CMapDoc *pDoc)
{
	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Loads this document from a VMF file.
// Input  : pszFileName - Full path of file to load.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapDoc::LoadVMF(const char *pszFileName)
{
	//
	// Create a new world to hold the loaded objects.
	//
	if (m_pWorld == NULL)
	{
		m_pWorld = new CMapWorld;
	}

	//
	// Open the file.
	//
	CChunkFile File;
	ChunkFileResult_t eResult = File.Open(pszFileName, ChunkFile_Read);

	//
	// Read the file.
	//
	if (eResult == ChunkFile_Ok)
	{
		//
		// Set up handlers for the subchunks that we are interested in.
		//
		CChunkHandlerMap Handlers;
		Handlers.AddHandler("world", (ChunkHandler_t)CMapDoc::LoadWorldCallback, this);
		Handlers.AddHandler("hidden", (ChunkHandler_t)CMapDoc::LoadHiddenCallback, this);
		Handlers.AddHandler("entity", (ChunkHandler_t)CMapDoc::LoadEntityCallback, this);
		Handlers.AddHandler("cameras", (ChunkHandler_t)CMapDoc::LoadCamerasCallback, this);
		Handlers.AddHandler("versioninfo", (ChunkHandler_t)CMapDoc::LoadVersionInfoCallback, this);
		Handlers.AddHandler("visgroups", (ChunkHandler_t)CMapDoc::LoadVisGroupsCallback, this);
		Handlers.AddHandler("viewsettings", (ChunkHandler_t)CMapDoc::LoadViewSettingsCallback, this);
		Handlers.AddHandler("cordon", (ChunkHandler_t)CMapDoc::LoadCordonCallback, this);
		Handlers.SetErrorHandler((ChunkErrorHandler_t)CMapDoc::HandleLoadError, this);

		File.PushHandlers(&Handlers);

		SetActiveMapDoc( this );
		m_bLoading = true;

		//
		// Read the sub-chunks. We ignore keys in the root of the file, so we don't pass a
		// key value callback to ReadChunk.
		//
		while (eResult == ChunkFile_Ok)
		{
			eResult = File.ReadChunk();
		}

		if (eResult == ChunkFile_EOF)
		{
			eResult = ChunkFile_Ok;
		}

		File.PopHandlers();
		m_bLoading = false;
	}

	if (eResult == ChunkFile_Ok)
	{
		Postload();
	}
	else
	{
		GetMainWnd()->MessageBox(File.GetErrorText(eResult), "Error loading file", MB_OK | MB_ICONEXCLAMATION);
	}

	return(eResult == ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pLoadInfo - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadVersionInfoCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadVersionInfoKeyCallback, pDoc));
}


//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues when loading the version info chunk of VMF files.
// Input  : szKey - Key to handle.
//			szValue - Value of key.
//			pDoc - Document being loaded.
// Output : Returns ChunkFile_Ok if all is well.
//-----------------------------------------------------------------------------

ChunkFileResult_t CMapDoc::LoadVersionInfoKeyCallback(const char *szKey, const char *szValue, CMapDoc *pDoc)
{
	KeyInt( "mapversion", pDoc->m_nDocVersion);
	KeyBool( "prefab", pDoc->m_bPrefab);

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadVisGroupCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	CVisGroup *pVisGroup = new CVisGroup;
	ChunkFileResult_t eResult = pVisGroup->LoadVMF(pFile);

	if (eResult == ChunkFile_Ok)
	{
		pDoc->VisGroups_AddGroup(pVisGroup);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadVisGroupsCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("visgroup", (ChunkHandler_t)LoadVisGroupCallback, pDoc);
	
	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk();
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadViewSettingsKeyCallback(const char *szKey, const char *szValue, CMapDoc *pDoc)
{
	KeyBool( "bSnapToGrid", pDoc->m_bSnapToGrid);
	KeyBool( "bShowGrid", pDoc->m_bShowGrid);
	KeyInt( "nGridSpacing", pDoc->m_nGridSpacing);
	KeyBool( "bShow3DGrid", pDoc->m_bShow3DGrid);

	return(ChunkFile_Ok);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadViewSettingsCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadViewSettingsKeyCallback, pDoc));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::LoadWorldCallback(CChunkFile *pFile, CMapDoc *pDoc)
{
	CMapWorld *pWorld = pDoc->GetMapWorld();
	ChunkFileResult_t eResult = pWorld->LoadVMF(pFile);
	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Called after loading a map file.
//-----------------------------------------------------------------------------
void CMapDoc::Postload(void)
{
	//
	// Report any noncritical loading errors here.
	//
	if (CMapSolid::GetBadSolidCount() > 0)
	{
		char szError[80];
		sprintf(szError, "For your information, %d solid(s) were not loaded due to errors in the file.", CMapSolid::GetBadSolidCount());
		GetMainWnd()->MessageBox(szError, "Warning", MB_ICONINFORMATION);
	}

	//
	// Count GUIDs before calling PostLoadWorld because objects that need to generate GUIDs
	// may do so in PostLoadWorld.
	//
	CountGUIDs();
	m_pWorld->PostloadWorld();
	AssignToGroups();
	UpdateVisibilityAll();

	// update displacement neighbors
	IWorldEditDispMgr *pDispMgr = GetActiveWorldEditDispManager();
	if( pDispMgr )
	{
		int count = pDispMgr->WorldCount();
		for( int ndx = 0; ndx < count; ndx++ )
		{
			CMapDisp *pDisp = pDispMgr->GetFromWorld( ndx );
			if( pDisp )
			{
				CMapFace *pFace = ( CMapFace* )pDisp->GetParent();
				pDispMgr->FindWorldNeighbors( pFace->GetDisp() );
			}
		}
	}

	//
	// Do batch search and replace of textures from trans.txt if it exists.
	//
	char translationFilename[MAX_PATH];
	_snprintf( translationFilename, MAX_PATH-1, "%s/materials/trans.txt", g_pGameConfig->m_szGameDir );
	FILE *searchReplaceFP;
	searchReplaceFP = fopen( translationFilename, "r" );
	if( searchReplaceFP )
	{
		BatchReplaceTextures( searchReplaceFP );
		fclose( searchReplaceFP );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapDoc::SelectDocType(void)
{
	// if no game configs are set up, we must set them up
	if (Options.configs.nConfigs == 0)
	{
		if (AfxMessageBox(IDS_NO_CONFIGS_AVAILABLE, MB_YESNO) == IDYES)
		{
			APP()->Help("Worldcraft_Setup_Guide.htm");
		}

		COptionProperties dlg("Configure Worldcraft", NULL, 0);
		dlg.DoModal();
		if (Options.configs.nConfigs == 0)
		{
			return FALSE;
		}
	}

	//
	// Prompt the user to select a game configuration.
	//
	CEditGameConfigs dlg(TRUE);
	if (dlg.DoModal() != IDOK)
	{
		return FALSE;
	}
	CGameConfig *pGame = dlg.GetSelectedGame();
	SetActiveGame(pGame);

	//
	// Try to find some textures that this game can use.
	//
	if (!g_Textures.IsGraphicsAvailable(g_pGameConfig->GetTextureFormat()))
	{
		AfxMessageBox(IDS_NO_TEXTURES_AVAILABLE);

		COptionProperties dlg("Configure Worldcraft", NULL, 0);
		dlg.DoModal();
		if (!g_Textures.IsGraphicsAvailable(g_pGameConfig->GetTextureFormat()))
		{
			return FALSE;
		}
	}

	m_pGame = pGame;

	if (GetActiveMapDoc() == this)
	{
		SetActiveMapDoc(this);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: set up this document to edit a prefab data .. when the object is saved,
//			save it back to the library instead of to a file.
// Input  : dwPrefabID - 
//-----------------------------------------------------------------------------
void CMapDoc::EditPrefab3D(DWORD dwPrefabID)
{
	CPrefab3D *pPrefab = (CPrefab3D *)CPrefab::FindID(dwPrefabID);
	ASSERT(pPrefab);

	// set up local variables
	m_dwPrefabID = dwPrefabID;
	m_dwPrefabLibraryID = pPrefab->GetLibraryID();
	m_bEditingPrefab = TRUE;

	SetPathName(pPrefab->GetName(), FALSE);
	SetTitle(pPrefab->GetName());
	
	// copy prefab data to world
	if (!pPrefab->IsLoaded())
	{
		pPrefab->Load();
	}

	//
	// Copying into world, so we update the object dependencies to insure
	// that any object references in the prefab get resolved.
	//
	m_pWorld->CopyFrom(pPrefab->GetWorld(), false);
	m_pWorld->CopyChildrenFrom(pPrefab->GetWorld(), false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : file - 
//			fIsStoring - 
//			bRMF - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapDoc::Serialize(fstream& file, BOOL fIsStoring, BOOL bRMF)
{
	SetActiveMapDoc(this);

	// check for editing prefab
	if(m_bEditingPrefab)
	{
		// save prefab in library
		CPrefabLibrary *pLibrary = CPrefabLibrary::FindID(m_dwPrefabLibraryID);
		if(!pLibrary)
		{
			static int id = 1;

			AfxMessageBox("The library this prefab object belongs to has been\n"
				"deleted. This document will now behave as a regular file\n"
				"document.");
			m_bEditingPrefab = FALSE;
			
			CString str;
			str.Format("Prefab%d.rmf", id++);
			SetPathName(str);
			return 1;
		}

		CPrefab3D *pPrefab = (CPrefab3D *)CPrefab::FindID(m_dwPrefabID);
		if (!pPrefab)
		{
			// Not found, create a new prefab.
			pPrefab = new CPrefabRMF;
		}

		pPrefab->SetWorld(m_pWorld);
		m_pWorld = NULL;

		pLibrary->Add(pPrefab);
		pLibrary->Save();

		return 1;
	}

	GetHistory()->Pause();

	if(bRMF)
	{
		if (m_pWorld->SerializeRMF(file, fIsStoring) < 0)
		{
			AfxMessageBox("There was a file error.", MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}

		char sig[8] = "DOCINFO";

		if(fIsStoring)
		{
			file.write(sig, sizeof sig);
			
			m_pToolCamera->SerializeRMF(file, fIsStoring);
		}
		else
		{
			char buf[sizeof sig];
			memset(buf, 0, sizeof buf);
			file.read(buf, sizeof buf);
			if(memcmp(buf, sig, sizeof sig))
				goto Done;

			m_pToolCamera->SerializeRMF(file, fIsStoring);
		}

Done:;
	}
	else
	{
		CMapObjectList CordonList;
		CMapWorld *pCordonWorld = NULL;
		BoundBox CordonBox;

		if (Cordon_IsCordoning())
		{
			//
			// Create "cordon world", add its objects to our real world, create a list in
			// CordonList so we can remove them again.
			//
			pCordonWorld = m_pToolCordon->CreateTempWorld();
			POSITION pos;
			CMapClass *pobj = pCordonWorld->GetFirstChild(pos);
			while (pobj != NULL)
			{
				pobj->SetTemporary(TRUE);
				m_pWorld->AddObjectToWorld(pobj);

				CordonList.AddTail(pobj);
				pCordonWorld->GetNextChild(pos);
			}

			//
			// HACK: (not mine) - make the cordon bounds bigger so that the cordon brushes
			// overlap the cordon bounds during serialization.
			//
			CordonBox = *m_pToolCordon;
 			for (int i = 0; i < 3; i++)
			{
				CordonBox.bmins[i] -= 1.f;
				CordonBox.bmaxs[i] += 1.f;
			}
		}

		if (fIsStoring)
		{
			void SetMapFormat(MAPFORMAT mf);
			SetMapFormat(m_pGame->mapformat);
		}

		if (m_pWorld->SerializeMAP(file, fIsStoring, Cordon_IsCordoning() ? &CordonBox : NULL) < 0)
		{
			AfxMessageBox("There was a file error.", MB_OK | MB_ICONEXCLAMATION);
			return(FALSE);
		}

		//
		// Remove cordon objects.
		//
		if (Cordon_IsCordoning())
		{
			POSITION pos = CordonList.GetHeadPosition();
			while (pos != NULL)
			{
				CMapClass *pobj = CordonList.GetNext(pos);
				m_pWorld->RemoveChild(pobj);
			}
			delete pCordonWorld;
		}
	}

	GetHistory()->Resume();

	if (!fIsStoring)
	{
		UpdateVisibilityAll();
		GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);
	}

	return TRUE;
}


#ifdef _DEBUG
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::AssertValid(void) const
{
	CDocument::AssertValid();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dc - 
//-----------------------------------------------------------------------------
void CMapDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG



//-----------------------------------------------------------------------------
// Purpose: Frees all dynamically allocated memory from this document.
//-----------------------------------------------------------------------------
void CMapDoc::DeleteContents(void)
{
	//
	// Don't leave pointers to deleted worlds lying around!
	//
	if (s_Clipboard.pSourceWorld == m_pWorld)
	{
		s_Clipboard.pSourceWorld = NULL;
	}

	//
	// The path tool holds pointer to the list of paths in the world, so clear that
	// out before deleting the world!
	//
	m_pToolPath->SetPathList(NULL);

	delete m_pWorld;
	m_pWorld = NULL;

	//
	// Delete VisGroups.
	//
	POSITION pos = VisGroups_GetHeadPosition();
	while (pos != NULL)
	{
		CVisGroup *pGroup = VisGroups_GetNext(pos);
		delete pGroup;
	}
	m_VisGroups.RemoveAll();

	m_pToolSelection->RemoveAll();

	CDocument::DeleteContents();

	CMainFrame *pwndMain = GetMainWnd();
	if (pwndMain != NULL)
	{
		pwndMain->OnDeleteActiveDocument();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
// Output : Returns a pointer to the visgroup with the given ID, NULL if none.
//-----------------------------------------------------------------------------
CVisGroup *CMapDoc::VisGroups_GroupForID(DWORD id)
{
	CVisGroup *pGroup;
	if (!m_VisGroups.Lookup(id, pGroup))
	{
		return(NULL);
	}
	return(pGroup);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapDoc::SaveModified(void)
{
	if (!IsModified())
		return TRUE;        // ok to continue

	// editing prefab and modified - update data?
	if(m_bEditingPrefab)
	{
		switch(AfxMessageBox("Do you want to save the changes to "
			"this prefab object?", MB_YESNOCANCEL))
		{
		case IDYES:
			{
			fstream file;
			Serialize(file, 0, 0);
			return TRUE;
			}
		case IDNO:
			return TRUE;	// no save
		case IDCANCEL:
			return FALSE;	// forget this cmd
		}
	}

	return CDocument::SaveModified();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lpszPathName - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	Initialize();

	if (!SelectDocType())
	{
		return FALSE;
	}

	//
	// Look for either the RMF or MAP extension to indicate an old file format.
	//
	BOOL bRMF = FALSE;
	BOOL bMAP = FALSE;

	if (!stricmp(lpszPathName + strlen(lpszPathName) - 3, "rmf"))
	{
		bRMF = TRUE;
	}
	else if (!stricmp(lpszPathName + strlen(lpszPathName) - 3, "map"))
	{
		bMAP = TRUE;
	}

	//
	// Call any per-class PreloadWorld functions here.
	//
	CMapSolid::PreloadWorld();

	if ((bRMF) || (bMAP))
	{
		fstream file(lpszPathName, ios::in | ios::binary | ios::nocreate);
		if (!file.is_open())
		{
			return(FALSE);
		}

		if (!Serialize(file, FALSE, bRMF))
		{
			return(FALSE);
		}
	}
	else
	{
		if (!LoadVMF(lpszPathName))
		{
			return(FALSE);
		}
	}

	SetModifiedFlag(FALSE);
	Msg(mwStatus, "Opened %s", lpszPathName);
	SetActiveMapDoc(this);


	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the document is closed.
//-----------------------------------------------------------------------------
void CMapDoc::OnCloseDocument(void)
{
	//
	// Deactivate the current tool now because doing it later can cause side-effects.
	//
	ToolID_t eTool = g_pToolManager->GetToolID();
	g_pToolManager->SetTool(TOOL_NONE);

	//
	// Call DeleteContents ourselves because in the framework implementation
	// of OnCloseDocument the doc window is closed first, which activates the
	// document beneath us. This is bad because we must be the active document
	// during the close process for things like displacements to clean themselves
	// up properly.
	//
	SetActiveMapDoc(this);
	DeleteContents();
	CDocument::OnCloseDocument();

	//
	// We have now switched to either the document beneath us or no document at all.
	// Reactivate the tool so that it is preserved as we go to the next document.
	//
	g_pToolManager->SetTool(eTool);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lpszPathName - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	if( m_pBSPLighting )
		m_pBSPLighting->Serialize();

	// UNDONE: prefab serialization must be redone
	if (m_bEditingPrefab)
	{
		fstream file;
		Serialize(file, 0, 0);
		SetModifiedFlag(FALSE);
		OnCloseDocument();
		return(TRUE);
	}

	//
	// If a file with the same name exists, back it up before saving the new one.
	//
	char szFile[MAX_PATH];
	strcpy(szFile, lpszPathName);
	szFile[strlen(szFile) - 1] = 'x';

	if (access(lpszPathName, 0) != -1)
	{
		if (!CopyFile(lpszPathName, szFile, FALSE))
		{
			DWORD dwError = GetLastError();

			char szError[_MAX_PATH];
			wsprintf(szError, "Worldcraft was unable to backup the existing file \"%s\" (Error: 0x%lX). Please verify that the there is space on the hard drive and that the path still exists.", lpszPathName, dwError);
			AfxMessageBox(szError);
			return(FALSE);
		}
	}

	//
	// Use the file extension to determine how to save the file.
	//
	BOOL bRMF = FALSE;
	BOOL bMAP = FALSE;
	if (!stricmp(lpszPathName + strlen(lpszPathName) - 3, "rmf"))
	{
		bRMF = TRUE;
	}
	else if (!stricmp(lpszPathName + strlen(lpszPathName) - 3, "map"))
	{
		bMAP = TRUE;
	}

	//
	// HalfLife 2 and beyond use heirarchical chunk files.
	//
	if (((m_pGame->mapformat == mfHalfLife2) || (m_pGame->mapformat == mfTeamFortress2)) && (!bRMF) && (!bMAP))
	{
		BOOL bSaved = FALSE;

		BeginWaitCursor();
		if (SaveVMF(lpszPathName, 0))
		{
			bSaved = TRUE;
			SetModifiedFlag(FALSE);
		}
		EndWaitCursor();

		return(bSaved);
	}

	//
	// Half-Life used RMFs and MAPs.
	//
	fstream file(lpszPathName, ios::out | ios::binary);
	if (!file.is_open())
	{
		char szError[_MAX_PATH];
		wsprintf(szError, "Worldcraft was unable to open the file \"%s\" for writing. Please verify that the file is writable and that the path exists.", lpszPathName);
		AfxMessageBox(szError);
		return(FALSE);
	}

	BeginWaitCursor();
	if (!Serialize(file, TRUE, bRMF))
	{
		EndWaitCursor();
		return(FALSE);
	}
	EndWaitCursor();

	SetModifiedFlag(FALSE);
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : st2 - 
//			st1 - 
// Output : DWORD
//-----------------------------------------------------------------------------
DWORD SubTime(SYSTEMTIME& st2, SYSTEMTIME& st1)
{
	DWORD dwMil = 0;

	if(st2.wMinute != st1.wMinute)
	{
		dwMil += (59 - st1.wSecond) * 1000;
	}
	if(st2.wSecond != st1.wSecond)
	{
		dwMil += 1000 - st1.wMilliseconds;
	}

	if(!dwMil)
	{
		dwMil = st2.wMilliseconds - st1.wMilliseconds;
	}
	else
		dwMil += st2.wMilliseconds;

	return dwMil;
}


//-----------------------------------------------------------------------------
// This is stupid -- all of the rotations, etc -- should go through one pipe!!!
//-----------------------------------------------------------------------------
void CMapDoc::UpdateDispFaces( Selection3D *selection )
{
	UpdateDispTrans( selection );
}


//-----------------------------------------------------------------------------
// Purpose: Forces a render of all the 3D views. Called from OnIdle to render
//			the 3D views.
//-----------------------------------------------------------------------------
void CMapDoc::Update3DViews(void)
{
	//
	// Make sure the document is up to date.
	//
	Update();

	POSITION p = GetFirstViewPosition();
	while (p)
	{
		CView *pView = GetNextView(p);
		if (pView->IsKindOf(RUNTIME_CLASS(CMapView3D)))
		{
			CMapView3D *pView3D = (CMapView3D *)pView;

			if (pView3D->IsActive())
			{
				pView3D->ProcessInput();
			}

			if( m_pBSPLighting && !APP()->GetForceRenderNextFrame() )
			{
				Sleep( 30 );
			}
			else
			{
				pView3D->Invalidate(false);
				pView3D->UpdateWindow();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::UpdateAllCameras(Camera3D &camera)
{
	CAMSTRUCT camView;
	camera.GetCameraPos(&camView);
	Vector vecPos = camView.position;
	Vector vecLookAt = camView.look;

	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		CMapView3D *pView3D = dynamic_cast <CMapView3D *>(pView);
		if (pView3D != NULL)
		{
			pView3D->SetCamera(vecPos, vecLookAt);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//-----------------------------------------------------------------------------
void CMapDoc::ToolUpdateViews(int cmd)
{
	POSITION p = GetFirstViewPosition();

	while (p)
	{
		CView *pView = GetNextView(p);

		if (pView->IsKindOf(RUNTIME_CLASS(CMapView2D)))
		{
			CMapView2D *pView2D = (CMapView2D*) pView;
			pView2D->SetUpdateFlag(cmd);
		}
	}

	UpdateStatusbar();
}


//-----------------------------------------------------------------------------
// Purpose: used during iteration, tells an map entity to 
//-----------------------------------------------------------------------------
static BOOL _UpdateAnimation( CMapClass *mapClass, float animTime )
{
	mapClass->UpdateAnimation( animTime );
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Sets up for drawing animated objects
//			Needs to be called each frame before any animating object are rendered
//-----------------------------------------------------------------------------
void CMapDoc::UpdateAnimation( void )
{
	#ifndef SDK_BUILD
	GetMainWnd()->m_AnimationDlg.RunFrame();
	#endif // SDK_BUILD

	// check to see if the animation needs to be updated
	if ( !IsAnimating() )
		return;

	// if the animation time is 0, turn it off
	if ( GetAnimationTime() == 0.0f )
	{
		m_bIsAnimating = false;
	}

	// get current animation time from animation toolbar
	union {
		float fl;
		DWORD dw;
	} animTime;
	
	animTime.fl = GetAnimationTime();

	// iterate through all CMapEntity object and update their animation frame matrix
	m_pWorld->EnumChildren( ENUMMAPCHILDRENPROC(_UpdateAnimation), animTime.dw, MAPCLASS_TYPE(CMapAnimator) );

}


//-----------------------------------------------------------------------------
// Purpose: Sets the current time in the animation
// Input  : time - a time, from 0 to 1
//-----------------------------------------------------------------------------
void CMapDoc::SetAnimationTime( float time )
{
	m_flAnimationTime = time;

	if ( m_flAnimationTime != 0.0f )
	{
		m_bIsAnimating = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets the current time and stores it in the doc, for use during the frame
//-----------------------------------------------------------------------------
void CMapDoc::UpdateCurrentTime( void )
{
	m_flCurrentTime = (float)timeGetTime() / 1000;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			pInfo - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL SelectInBox(CMapClass *pObject, SelectBoxInfo_t *pInfo)
{
	//
	// Skip hidden objects.
	//
	if (!pObject->IsVisible())
	{
		return TRUE;
	}

	//
	// Skip anything with children. We only are interested in leaf objects because
	// PrepareSelection will call up to tree to get the proper ancestor.
	//
	if (pObject->GetChildCount())
	{
		return TRUE;
	}

	//
	// Skip groups. Groups are selected via their members through PrepareSelection.
	//
	if (pObject->IsGroup())
	{
		// Shouldn't ever have empty groups lying around!
		ASSERT(false);
		return TRUE;
	}

	//
	// Skip clutter helpers.
	//
	if (pObject->IsClutter())
	{
		return TRUE;
	}

	// FIXME: We're calling PrepareSelection on nearly everything in the world,
	// then doing the box test against the object that we get back from that!
	// We should use the octree to cull out most of the world up front.
	CMapClass *pSelObject = pObject->PrepareSelection(pInfo->eSelectMode);
	if (pSelObject)
	{
		if (Options.view2d.bSelectbyhandles)
		{
			Vector ptCenter;
			pObject->GetBoundsCenter(ptCenter);
			if (pInfo->pBox->ContainsPoint(ptCenter))
			{
				pInfo->pDoc->SelectObject(pSelObject, CMapDoc::scSelect);
			}

			return TRUE;
		}

		bool bSelect;
		if (pInfo->bInside)
		{
			bSelect = pObject->IsInsideBox(pInfo->pBox->bmins, pInfo->pBox->bmaxs);
		}
		else
		{
			bSelect = pObject->IsIntersectingBox(pInfo->pBox->bmins, pInfo->pBox->bmaxs);
		}

		if (bSelect)
		{
			pInfo->pDoc->SelectObject(pSelObject, CMapDoc::scSelect);
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pBox - 
//			bInsideOnly - 
//-----------------------------------------------------------------------------
void CMapDoc::SelectRegion(BoundBox *pBox, BOOL bInsideOnly)
{
	SelectBoxInfo_t info;
	info.pDoc = this;
	info.pBox = pBox;	
	info.bInside = bInsideOnly;
	info.eSelectMode = Selection_GetMode();

	m_pWorld->EnumChildren((ENUMMAPCHILDRENPROC)SelectInBox, (DWORD)&info);

	SelectObject(NULL, scUpdateDisplay);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::UpdateStatusbar(void)
{
	if (g_pToolManager->GetToolID() == TOOL_FACEEDIT_MATERIAL)
	{
		CString str;
		str.Format("%d faces selected", GetMainWnd()->m_pFaceEditSheet->GetFaceListCount() );
		SetStatusText(SBI_SELECTION, str);
		SetStatusText(SBI_SIZE, "");
		return;
	}

	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		pTool->UpdateStatusBar();
	}

	CString str;
	int nCount = Selection_GetCount();
	switch (nCount)
	{
		case 0:
		{
			str = "no selection.";
			break;
		}

		case 1:
		{
			CMapClass *pobj = Selection_GetObject(0);
			str = pobj->GetDescription();
			break;
		}

		default:
		{
			str.Format("%d objects selected.", nCount);
			break;
		}
	}

	SetStatusText(SBI_SELECTION, str);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pobj - 
//			cmd - 
//-----------------------------------------------------------------------------
void CMapDoc::SelectObject(CMapClass *pobj, int cmd)
{
	APP()->SetForceRenderNextFrame();
	
	BOOL bClear = (cmd & scClear) ? TRUE : FALSE;
	BOOL bUpdate = (cmd & scUpdateDisplay) ? TRUE : FALSE;
	BOOL bAlreadySelected;

	// bounds of all objects whose state has changed since last
	// screen update:
	static UpdateBox ub;

	ub.Box.UpdateBounds(m_pToolSelection);

	// make sure these are not set so we can do a direct compare
	cmd &= ~(scClear | scUpdateDisplay);

	if (cmd & (scSelect | scUnselect | scToggle))
	{
		if (cmd == scSelect && bClear)
		{
			// want to clear, and want to select - set empty first
			m_pToolSelection->SetEmpty();
		}

		bAlreadySelected = m_pToolSelection->IsSelected(pobj);
		if(cmd == scToggle)
			cmd = bAlreadySelected ? scUnselect : scSelect;

		bClear = FALSE;		// we test down below too

		// select or unselect now
		switch (cmd)
		{
			case scSelect:
			{
				if (!bAlreadySelected)
				{
					m_pToolSelection->Add(pobj);
					pobj->SetSelectionState(SELECT_NORMAL);

					Vector mins;
					Vector maxs;
					pobj->GetRender2DBox(mins, maxs);
					ub.Box.UpdateBounds(mins, maxs);
				}
				break;
			}
			case scUnselect:
			{
				if(bAlreadySelected)
				{
					m_pToolSelection->Remove(pobj);
					pobj->SetSelectionState(SELECT_NONE);

					Vector mins;
					Vector maxs;
					pobj->GetRender2DBox(mins, maxs);
					ub.Box.UpdateBounds(mins, maxs);
				}
				break;
			}
		}
	}
	else if (!bUpdate && (g_pToolManager->GetToolID() == TOOL_POINTER) && (m_pToolSelection->IsBoxSelecting()))
	{
		ToolUpdateViews(CMapView2D::updEraseTool);
		m_pToolSelection->EndBoxSelection(); // dvsFIXME: move into the selection tool
	}

	if (bClear)
	{
		m_pToolSelection->SetEmpty();
	}

	// update display?
	if (bUpdate)
	{
		// this is not correct, but 2d draw doesn't use object list anyway.
		ub.Objects = Selection_GetList();

		UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_COLOR_ONLY);
		UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_UPDATE_2D, &ub);

		GetMainWnd()->pObjectProperties->LoadData(Selection_GetList(), true);

		UpdateStatusbar();
		ub.Box.ResetBounds();

		// update animation toolbar
		#ifndef SDK_BUILD
		GetMainWnd()->m_AnimationDlg.SelectionChanged( *Selection_GetList() );
		#endif // SDK_BUILD
	}
}


//-----------------------------------------------------------------------------
// Purpose: Clears the current selection and selects everything in the given list.
// Input  : pList - Objects to select.
//-----------------------------------------------------------------------------
void CMapDoc::SelectObjectList(CMapObjectList *pList)
{
	SelectMode_t eSelectMode = Selection_GetMode();

	//
	// Clear the current selection.
	//
	SelectObject(NULL, scClear);

	if ((pList != NULL) && (pList->GetCount() != 0))
	{
		POSITION pos = pList->GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = pList->GetNext(pos);
			CMapClass *pSelObject = pObject->PrepareSelection(eSelectMode);
			if (pSelObject)
			{
				SelectObject(pSelObject, scSelect);
			}
		}
	}

	SelectObject(NULL, scUpdateDisplay);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eNewTool - 
//-----------------------------------------------------------------------------
void CMapDoc::ActivateTool(ToolID_t eNewTool)
{
	ToolUpdateViews(CMapView2D::updTool);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDoc - 
//-----------------------------------------------------------------------------
void CMapDoc::SetActiveMapDoc(CMapDoc *pDoc)
{
	if (pDoc == m_pMapDoc)
	{
		//
		// Only do the work when the doc actually changes.
		// FIXME: We set the active doc before loading for displacements (and maybe other
		//        things), but visgroups aren't available until after map load. We have to refresh
		//		  the visgroups here or they won't be correct after map load.
		GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);
		return;
	}

	m_pMapDoc = pDoc;
	
	//
	// Set our tools as the active toolset in the tool manager.
	//
	// FIXME: The tools should all live in the tool manager, but since
	//		  some of them contain document state rather than merely modify it
	//		  (like the selection set, and the cordon bounds), we have to switch
	//		  between sets of tools when the active doc changes.
	//
	//		  Once all the tools live in the tool manager, we can automate their
	//		  construction and addition to the tools list.
	//
	ToolID_t eTool = g_pToolManager->GetToolID();
	g_pToolManager->RemoveAllTools();

	if (pDoc != NULL)
	{
		g_pToolManager->AddTool(pDoc->m_pToolSelection);
		g_pToolManager->AddTool(pDoc->m_pToolBlock);
		g_pToolManager->AddTool(pDoc->m_pToolMarker);
		g_pToolManager->AddTool(pDoc->m_pToolCamera);
		g_pToolManager->AddTool(pDoc->m_pToolMorph);
		g_pToolManager->AddTool(pDoc->m_pToolClipper);
		g_pToolManager->AddTool(pDoc->m_pToolCordon);
		g_pToolManager->AddTool(pDoc->m_pToolPath);
		g_pToolManager->AddTool(pDoc->m_pToolOverlay);

		g_pToolManager->SetTool(eTool);
	}

	//
	// Set the new document in the shell.
	//
	g_Shell.SetDocument(pDoc);

	//
	// Set the history to the document's history.
	//
	if (pDoc != NULL)
	{
		CHistory::SetHistory(pDoc->GetDocHistory());
		pDoc->SetUndoActive(GetMainWnd()->IsUndoActive() == TRUE);
	}
	else
	{
		CHistory::SetHistory(NULL);
	}

	//
	// Notify that the active document has changed.
	//
	GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);

	//
	// Set global game config to type found in doc.
	//
	CGameConfig *pOldGame = GetActiveGame();
	if (pDoc != NULL)
	{
		SetActiveGame(pDoc->GetGame());
	}
	else
	{
		SetActiveGame(NULL);
	}

	//
	// Update everything the first time we create a document or when the
	// game configuration changes between documents.
	//
	static bool bFirst = true;
	if ((pOldGame != GetActiveGame()) || (bFirst))
	{
		bFirst = false;
		GetMainWnd()->GlobalNotify(WM_GAME_CHANGED);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapWorld *GetActiveWorld(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		return(pDoc->GetMapWorld());
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
IWorldEditDispMgr *GetActiveWorldEditDispManager( void )
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if( pDoc  )
	{
		CMapWorld *pWorld = pDoc->GetMapWorld();
		if( pWorld )
		{
			return pWorld->GetWorldEditDispManager();
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Deletes the object by removing it from its parent. The object is
//			kept in the Undo history. If the object being deleted is the only
//			child of a solid entity, that entity is also deleted.
// Input  : pObject - The object to delete.
//-----------------------------------------------------------------------------
void CMapDoc::DeleteObject(CMapClass *pObject)
{
	GetHistory()->KeepForDestruction(pObject);

	//
	// If the object is being morphed, deselect the object from the
	// morph tool before deleting it.
	//
	if (m_pToolMorph->IsMorphing((CMapSolid *)pObject))
	{
		m_pToolMorph->SelectObject((CMapSolid *)pObject, Morph3D::scUnselect);
	}

	CMapClass *pParent = pObject->GetParent();

	RemoveObjectFromWorld(pObject, true);

	//
	// If we are deleting the last child of a solid entity, or the last member of 
	// a group, delete the parent object also. This avoids ghost objects at the origin.
	//
	if (pParent)
	{
		if (pParent->IsGroup())
		{
			if (pParent->GetChildCount() == 0)
			{
				DeleteObject(pParent);
			}
		}
		else
		{
			CMapEntity *pParentEntity = dynamic_cast <CMapEntity *>(pParent);
			if (pParentEntity != NULL)
			{
				if (!pParentEntity->IsPlaceholder() && !pParentEntity->HasSolidChildren())
				{
					DeleteObject(pParentEntity);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Call this to delete multiple objects in a single operation.
// Input  : List - 
//-----------------------------------------------------------------------------
void CMapDoc::DeleteObjectList(CMapObjectList &List)
{
	POSITION pos = List.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = List.GetNext(pos);
		ASSERT(pObject != NULL);

		DeleteObject(pObject);
	}

	m_pToolSelection->SetEmpty();
	ToolUpdateViews(CMapView2D::updEraseTool);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);
	SetModifiedFlag();

	//
	// Make sure object properties dialog is not referencing the
	// deleted object.
	//
	GetMainWnd()->pObjectProperties->AbortData();
	GetMainWnd()->SetFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Delete selected objects.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditDelete(void)
{
	if (g_pToolManager->GetToolID() == TOOL_MORPH)
	{
		// Can't delete stuff while morphing.
		return;
	}

	if (g_pToolManager->GetToolID() == TOOL_PATH)
	{
		m_pToolPath->DeleteSelected();
		ToolUpdateViews(CMapView2D::updTool);
		return;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Delete");

	//
	// Delete objects in selection.
	//
	int nCount = Selection_GetCount();
	for (int i = 0; i < nCount; i++)
	{
		CMapClass *pobj = Selection_GetObject(i);
		DeleteObject(pobj);
	}

	Selection_RemoveAll();
	Selection_UpdateBounds();

	ToolUpdateViews(CMapView2D::updEraseTool);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);

	SetModifiedFlag();

	// make sure objectproperties dialog is not referencing
	// selected object.
	GetMainWnd()->pObjectProperties->AbortData();
	GetMainWnd()->SetFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Invokes the search/replace dialog.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditReplace(void)
{
	GetMainWnd()->ShowSearchReplaceDialog();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapSnaptogrid(void)
{
	m_bSnapToGrid = !m_bSnapToGrid;

	CString strSnap;
	strSnap.Format(" Snap: %s Grid: %d ", m_bSnapToGrid ? "On" : "Off", m_nGridSpacing);
	SetStatusText(SBI_SNAP, strSnap);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateMapSnaptogrid(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bSnapToGrid);
}


//-----------------------------------------------------------------------------
// Purpose: Deselects everything.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditClearselection(void)
{
	if (g_pToolManager->GetToolID() == TOOL_MORPH)
	{
		// clear morph
		m_pToolMorph->SetEmpty();

		UpdateBox ub;
		ub.Objects = NULL;
		m_pToolMorph->GetMorphBounds(ub.Box.bmins, ub.Box.bmaxs, true);
		UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_DISPLAY, &ub);
	}
	else if (g_pToolManager->GetToolID() != TOOL_FACEEDIT_MATERIAL)
	{
		if (Selection_GetCount() > 2)
		{
			GetHistory()->MarkUndoPosition(Selection_GetList(), "Clear Selection");
		}

		SelectObject(NULL, scUpdateDisplay | scClear);
	}
	else
	{
		SelectFace(NULL, 0, scUpdateDisplay | scClear);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSolid - 
//			pszTexture - 
// Output : Returns TRUE to continue iterating.
//-----------------------------------------------------------------------------
static BOOL ApplyTextureToSolid(CMapSolid *pSolid, LPCTSTR pszTexture)
{
	pSolid->SetTexture(pszTexture);
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the Apply Current Texture toolbar button.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateEditApplytexture(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( ( g_pToolManager->GetToolID() != TOOL_FACEEDIT_MATERIAL ) && !GetMainWnd()->IsShellSessionActive() );
}


//-----------------------------------------------------------------------------
// Purpose: Applies the current default texture to all faces of all selected solids.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditApplytexture(void)
{
	GetHistory()->MarkUndoPosition(Selection_GetList(), "Apply Texture");

	// texturebar.cpp:
	LPCTSTR GetDefaultTextureName();

	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = Selection_GetObject(i);
		if (pobj->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
		{
			GetHistory()->Keep(pobj);
			((CMapSolid*)pobj)->SetTexture(GetDefaultTextureName());
		}

		pobj->EnumChildren((ENUMMAPCHILDRENPROC)ApplyTextureToSolid, (DWORD)GetDefaultTextureName(), MAPCLASS_TYPE(CMapSolid));
	}

	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_COLOR_ONLY);

	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Callback for EnumChildren. Adds the object to the given list.
// Input  : pObject - Object to add to the list.
//			pList - List to add the object to.
// Output : Returns TRUE to continue iterating.
//-----------------------------------------------------------------------------
static BOOL CopyObjectsToList(CMapClass *pObject, CMapObjectList *pList)
{
	pList->AddTail(pObject);
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Makes all selected brushes the children of a solid entity. The
//			class will be the default solid class from the game configuration.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditToEntity(void)
{
	extern GameData *pGD;

	CMapEntity *pNewEntity = NULL;
	BOOL bMadeEntity = TRUE;
	BOOL bUseSelectionDialog = FALSE;

	//
	// Build a list of every solid in the selection, whether part of a solid entity or not.
	//
	CMapObjectList newobjects;
	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);

		//
		// If the object is a solid, add it to our list.
		//
		if (pObject->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
		{
			newobjects.AddTail(pObject);
		}
		//
		// If the object is a group, add any solids in the group to our list.
		//
		else if (pObject->IsMapClass(MAPCLASS_TYPE(CMapGroup)))
		{
			pObject->EnumChildren(ENUMMAPCHILDRENPROC(CopyObjectsToList), DWORD(&newobjects), MAPCLASS_TYPE(CMapSolid));
		}
		//
		// If the object is an entity, add any solid children of the entity to our list.
		//
		else if (pObject->IsMapClass(MAPCLASS_TYPE(CMapEntity)))
		{
			pObject->EnumChildren(ENUMMAPCHILDRENPROC(CopyObjectsToList), DWORD(&newobjects), MAPCLASS_TYPE(CMapSolid));

			//
			// See if there is more than one solid entity selected. If so, we'll need to prompt the user
			// to pick one.
			//
			CMapEntity *pEntity = (CMapEntity *)pObject;
			if (!pEntity->IsPlaceholder())
			{
				//
				// Already found an eligible entity, so we want
				// to call up the entity selection dialog. 
				//
				if (pNewEntity != NULL)
				{
					bUseSelectionDialog = TRUE;
				}

				pNewEntity = pEntity;
			}
		}
	}

	//
	// If the list is empty, we have nothing to do.
	//
	POSITION p = newobjects.GetHeadPosition();
	if (!p)
	{
		AfxMessageBox("There are no eligible selected objects.");
		return;
	}

	//
	// If already have an entity selected, ask if they want to 
	// add solids to it.
	//
	if (pNewEntity && !bUseSelectionDialog)
	{
		CString str;
		str.Format("You have selected an existing entity (a '%s'.)\n"
			"Would you like to add the selected solids to the existing entity?\n"
			"If you select 'No', a new entity will be created.",
			pNewEntity->GetClassName());
	
		if (AfxMessageBox(str, MB_YESNO) == IDNO)
		{
			pNewEntity = NULL;	// it'll be made down there
		}
		else
		{
			bMadeEntity = FALSE;
		}
	}
	//
	// If there were multiple solid entities selected, bring up the selection dialog.
	//
	else if (bUseSelectionDialog)
	{
		CSelectEntityDlg dlg(Selection_GetList());
		GetMainWnd()->pObjectProperties->ShowWindow(SW_SHOW);
		if (dlg.DoModal() == IDCANCEL)
		{
			return;	// forget about it
		}
		pNewEntity = dlg.m_pFinalEntity;
		bMadeEntity = FALSE;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "To Entity");
	GetHistory()->Keep(Selection_GetList());

	//
	// If they haven't already picked an entity to add the solids to, create a new
	// solid entity.
	//
	if (!pNewEntity)
	{
		pNewEntity = new CMapEntity;
		bMadeEntity = TRUE;
	}

	//
	// Add all the solids in our list to the solid entity.
	//
	while (p)
	{
		CMapClass *pObject = newobjects.GetNext(p);
		CMapClass *pOldParent = pObject->GetParent();

		//
		// If the solid is changing parents...
		//
		if (pOldParent != pNewEntity)
		{
			ASSERT(pOldParent != NULL);
			if (pOldParent != NULL)
			{
				//
				// Remove the solid from its current parent.
				//
				pOldParent->RemoveChild(pObject);

				//
				// If this solid was the child of a solid entity, check to see if the entity has
				// any children left - if not, we remove it from the world because it's useless without
				// solid children.
				//
				CMapEntity *pOldParentEnt = dynamic_cast<CMapEntity *>(pOldParent);
				if (pOldParentEnt && (!pOldParentEnt->IsPlaceholder()) && (!pOldParentEnt->HasSolidChildren()))
				{
					DeleteObject(pOldParentEnt);
				}
			}

			//
			// Add the solid as a child of the new parent entity.
			//
			pNewEntity->AddChild(pObject);
		}
	}

	//
	// If we created a new entity, add it to the world.
	//
	if (bMadeEntity)
	{
		pNewEntity->SetPlaceholder(FALSE);
		pNewEntity->SetClass(g_pGameConfig->szDefaultSolid);
		AddObjectToWorld(pNewEntity);

		//
		// Don't keep our children because they are not new to the world.
		//
		GetHistory()->KeepNew(pNewEntity, false);
	}

	SelectObject(pNewEntity, scClear | scSelect | scUpdateDisplay);

	UpdateBox box;
	box.Objects = Selection_GetList();
	m_pToolSelection->GetBounds(box.Box.bmins, box.Box.bmaxs);

	g_pToolManager->SetTool(TOOL_POINTER);

	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY, &box);

	if (bMadeEntity)
	{
		GetMainWnd()->pObjectProperties->ShowWindow(SW_SHOW);
		GetMainWnd()->pObjectProperties->SetActiveWindow();
		SelectObject(NULL, scUpdateDisplay);
	}
	
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Moves all solid children of selected entities to the world.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditToWorld(void)
{
	CMapObjectList SelList;
	SelList.AddTail(Selection_GetList());

	POSITION p = SelList.GetHeadPosition();
	if (p != NULL)
	{
		GetHistory()->MarkUndoPosition(Selection_GetList(), "To World");
		GetHistory()->Keep(Selection_GetList());

		//
		// Remove selection rect from screen & clear selection list.
		//
		SelectObject(NULL, scClear | scUpdateDisplay);

		while (p != NULL)
		{
			CMapClass *pObject = SelList.GetNext(p);
			CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);

			//
			// If this is a solid entity, move all its children to the world.
			//
			if ((pEntity != NULL) && (!pEntity->IsPlaceholder()))
			{
				//
				// Build a list of the entity's solid children.
				//
				CUtlVector <CMapClass *> ChildList;
				POSITION p2;
				CMapClass *pChild = pEntity->GetFirstChild(p2);
				while (pChild != NULL)
				{
					if ((dynamic_cast<CMapSolid *>(pChild)) != NULL)
					{
						ChildList.AddToTail(pChild);
					}
					pChild = pEntity->GetNextChild(p2);
				}

				//
				// Detach all the children from the entity. This throws out
				// all non-solid children, since they aren't in our list.
				//
				pEntity->RemoveAllChildren();

				//
				// Move the entity's former solid children to the world.
				//
				int nChildCount = ChildList.Count();
				for (int i = 0; i < nChildCount; i++)
				{
					pChild = ChildList.Element(i);

					m_pWorld->AddChild(pChild);
					pChild->SetVisGroup(pEntity->GetVisGroup());
					pChild->SetRenderColor(0, 100 + (random() % 156), 100 + (random() % 156));
					SelectObject(pChild, scSelect);
				}

				//
				// The entity is empty; delete it.
				//
				DeleteObject(pEntity);
			}
		}
	}

	GetMainWnd()->pObjectProperties->AbortData();

	UpdateBox box;
	box.Objects = Selection_GetList();
	m_pToolSelection->GetBounds(box.Box.bmins, box.Box.bmaxs);

	g_pToolManager->SetTool(TOOL_POINTER);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY, &box);
	
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Subtracts the first object in the selection set (by index) from
//			all solids in the world.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsSubtractselection(void)
{
	if (Selection_GetCount == 0)
	{
		return;
	}

	//
	// Subtract with the first object in the selection list.
	//
	CMapClass *pSubtractWith = Selection_GetObject(0);
	if (pSubtractWith == NULL)
	{
		return;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Carve");
	GetHistory()->Keep(Selection_GetList());

	//
	// Build a list of every solid in the world.
	//
	CMapObjectList WorldSolids;
	EnumChildrenPos_t pos;
	CMapClass *pChild = m_pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapSolid *pSolid = dynamic_cast <CMapSolid *> (pChild);
		if (pSolid != NULL)
		{
			WorldSolids.AddTail(pSolid);
		}

		pChild = m_pWorld->GetNextDescendent(pos);
	}

	if (WorldSolids.GetCount() == 0)
	{
		return;
	}

	//
	// Subtract the 'subtract with' object from every solid in the world.
	//
	POSITION p = WorldSolids.GetHeadPosition();
	while (p != NULL)
	{
		CMapSolid *pSubtractFrom = (CMapSolid *)WorldSolids.GetNext(p);
		CMapClass *pDestParent = pSubtractFrom->GetParent();

		//
		// Perform the subtraction. If the two objects intersected...
		//
		CMapObjectList Outside;
		if (pSubtractFrom->Subtract(NULL, &Outside, pSubtractWith))
		{
			if (Outside.GetCount() > 0)
			{
				CMapClass *pResult;

				//
				// If the subtraction resulted in more than one object, create a group
				// to place the results in.
				//
				if (Outside.GetCount() > 1)
				{
					pResult = (CMapClass *)(new CMapGroup);
					POSITION pos2 = Outside.GetHeadPosition();
					while (pos2 != NULL)
					{
						CMapClass *pTemp = Outside.GetNext(pos2);
						pResult->AddChild(pTemp);
					}
				}
				//
				// Otherwise, the results are the single object.
				//
				else if (Outside.GetCount() == 1)
				{
					POSITION pos2 = Outside.GetHeadPosition();
					pResult = Outside.GetAt(pos2);
				}

				//
				// Replace the 'subtract from' object with the subtraction results.
				//
				DeleteObject(pSubtractFrom);
				AddObjectToWorld(pResult, pDestParent);
				GetHistory()->KeepNew(pResult);
			}
		}
	}

	ToolUpdateViews(CMapView2D::updEraseTool);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);

	SetModifiedFlag();

	//
	// Make sure object properties dialog is not referencing
	// selected objects.
	//
	GetMainWnd()->pObjectProperties->AbortData();
	GetMainWnd()->SetFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Copies the selected objects to the clipboard.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditCopy(void)
{
	BeginWaitCursor();

	//
	// Delete the contents of the clipboard.
	//
	POSITION p = s_Clipboard.Objects.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pobj = s_Clipboard.Objects.GetNext(p);
		delete pobj;
	}
	s_Clipboard.Objects.RemoveAll();

	m_pToolSelection->GetBoundsCenter(s_Clipboard.vecOriginalCenter);
	m_pToolSelection->GetBounds(s_Clipboard.Bounds.bmins, s_Clipboard.Bounds.bmaxs);

	GetHistory()->Pause();

	s_Clipboard.pSourceWorld = m_pWorld;

	//
	// Copy the selected objects to the clipboard.
	//
	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = Selection_GetObject(i);
		CMapClass *pNewobj = pobj->Copy(false);

		//
		// Prune the object from the world tree without calling RemoveObjectFromWorld.
		// This prevents CopyChildrenFrom from updating the culling tree.
		//
		//pNewobj->SetObjParent(NULL);

		//
		// Copy all the children from the original object into the copied object.
		//
		pNewobj->CopyChildrenFrom(pobj, false);

		//
		// Remove the copied object from the world.
		//
		RemoveObjectFromWorld(pNewobj, true);

		pNewobj->SetVisGroup(NULL);
		s_Clipboard.Objects.AddTail(pNewobj);
	}

	GetHistory()->Resume();

	EndWaitCursor();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptOrg - 
//-----------------------------------------------------------------------------
void CMapDoc::GetBestVisiblePoint(Vector& ptOrg)
{
	// create paste position at center of 1st 2d view
	POSITION p = MRU2DViews.GetHeadPosition();
	while(p)
	{
		CMapView2D *pView = MRU2DViews.GetNext(p);
		pView->GetCenterPoint(ptOrg);
	}

	for(int i = 0; i < 3; i++)
	{
		if(ptOrg[i] == COORD_NOTINIT)
			ptOrg[i] = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets a point on the screen to paste to. Functionalized because it
//			is called from OnEditPaste and OnEditPasteSpecial.
//-----------------------------------------------------------------------------
void CMapDoc::GetBestPastePoint(Vector &vecPasteOrigin)
{
	//
	// Start with a visible grid point near the center of the screen.
	//
	vecPasteOrigin = Vector(COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT);
	GetBestVisiblePoint(vecPasteOrigin);
	Snap(vecPasteOrigin);

	//
	// Offset the center relative to the grid the same as it was originally.
	//
	Vector vecSnappedOriginalCenter = s_Clipboard.vecOriginalCenter;
	Snap(vecSnappedOriginalCenter);
	vecPasteOrigin += s_Clipboard.vecOriginalCenter - vecSnappedOriginalCenter;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Objects - 
//			pSourceWorld - 
//			pDestWorld - 
//			vecOffset - 
//			vecRotate - 
//			pParent - 
//-----------------------------------------------------------------------------
void CMapDoc::Paste(CMapObjectList &Objects, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, Vector vecOffset, QAngle vecRotate, CMapClass *pParent)
{
	//
	// Copy the objects in the clipboard and build a list of objects to paste
	// into the world.
	//
	CMapObjectList PasteList;
	POSITION pos = Objects.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pOriginal = Objects.GetNext(pos);
		CMapClass *pCopy = pOriginal->Copy(false);

		pCopy->CopyChildrenFrom(pOriginal, false);
		PasteList.AddTail(pCopy);
	}

	//
	// Notification happens in two-passes. The first pass lets objects generate new unique
	// IDs in the destination world, the second pass lets objects fixup references to other
	// objects in the clipboard.
	//
	POSITION p1 = Objects.GetHeadPosition();
	POSITION p2 = PasteList.GetHeadPosition();
	while (p1 != NULL)
	{
		CMapClass *pOriginal = Objects.GetNext(p1);
		CMapClass *pCopy = PasteList.GetNext(p2);

		pOriginal->OnPrePaste(pCopy, pSourceWorld, pDestWorld, Objects, PasteList);
	}

	//
	// Add the objects to the world.
	//
	pos = PasteList.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pCopy = PasteList.GetNext(pos);

		if (vecOffset != vec3_origin)
		//if ((vecOffset[0] != 0) || (vecOffset[1] != 0) || (vecOffset[2] != 0))
		{
			pCopy->TransMove(vecOffset);
		}

		if (vecRotate != vec3_angle)
		//if ((vecRotate[0] != 0) || (vecRotate[1] != 0) || (vecRotate[2] != 0))
		{
			Vector ptCenter;
			pCopy->GetBoundsCenter(ptCenter);
			pCopy->TransRotate(&ptCenter, vecRotate);
		}

		AddObjectToWorld(pCopy, pParent);
	}

	//
	// Do the second pass of notification. The second pass of notification lets objects
	// fixup references to other objects that were pasted. We don't do it in the loop above
	// because then not all the pasted objects would be in the world yet.
	//
	p1 = Objects.GetHeadPosition();
	p2 = PasteList.GetHeadPosition();
	while ((p1 != NULL) && (p2 != NULL))
	{
		CMapClass *pOriginal = Objects.GetNext(p1);
		CMapClass *pCopy = PasteList.GetNext(p2);

		pOriginal->OnPaste(pCopy, pSourceWorld, pDestWorld, Objects, PasteList);

		//
		// Semi-HACK: If we aren't pasting into a group, keep the new object in the Undo stack.
		// Otherwise, we'll keep the group in OnEditPasteSpecial.
		//
		if ((pParent == NULL) || (pParent == pDestWorld))
		{
			GetHistory()->KeepNew(pCopy);
	 		SelectObject(pCopy, scSelect);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Pastes the clipboard contents into the active world.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditPaste(void)
{
	BeginWaitCursor();
	GetHistory()->MarkUndoPosition(Selection_GetList(), "Paste");

	// first, clear selection so we can select all pasted objects
	SelectObject(NULL, scClear | scUpdateDisplay);

	//
	// Build a translation that will put the pasted objects in the center of the view.
	//
	Vector vecPasteOffset;
	GetBestPastePoint(vecPasteOffset);
	vecPasteOffset -= s_Clipboard.vecOriginalCenter;

	//
	// Paste the objects into the active world.
	//
	CMapWorld *pWorld = GetActiveWorld();
	Paste(s_Clipboard.Objects, s_Clipboard.pSourceWorld, pWorld, vecPasteOffset);

	UpdateBox box;
	box.Objects = Selection_GetList();
	m_pToolSelection->GetBounds(box.Box.bmins, box.Box.bmaxs);

	g_pToolManager->SetTool(TOOL_POINTER);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_3D | MAPVIEW_UPDATE_OBJECTS, &box);
	SelectObject(NULL, scUpdateDisplay);

	SetModifiedFlag();
	EndWaitCursor();
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the Paste menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateEditPaste(CCmdUI *pCmdUI) 
{
	pCmdUI->Enable( s_Clipboard.Objects.GetCount() &&
		            ( g_pToolManager->GetToolID() != TOOL_FACEEDIT_MATERIAL ) &&
					!GetMainWnd()->IsShellSessionActive() );
}


//-----------------------------------------------------------------------------
// Purpose: Handles the Edit | Cut command. Copies the selection to the clipboard,
//			then deletes it from the document.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditCut(void)
{
	OnEditCopy();
	OnEditDelete();
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the various Edit menu items.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateGroupEditFunction(CCmdUI* pCmdUI) 
{
	//
	// Edit functions are disabled when we're applying textures or editing via a shell session.
	//
	pCmdUI->Enable( ( g_pToolManager->GetToolID() != TOOL_FACEEDIT_MATERIAL ) &&
		            !GetMainWnd()->IsShellSessionActive() );
}


//-----------------------------------------------------------------------------
// Purpose: Creates a new group and adds all selected items to it.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsGroup(void)
{
	if (Selection_GetCount() == 0)
	{
		AfxMessageBox("No objects are selected.");
		return;
	}

	if ((Selection_GetMode() == selectSolids) && !Options.general.bGroupWhileIgnore)
	{
		return;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Group");
	GetHistory()->Keep(Selection_GetList());

	//
	// Create a new group containing the selected objects.
	//
	CMapGroup *pGroup = new CMapGroup;
	AddObjectToWorld(pGroup);

	pGroup->SetRenderColor(100 + (random() % 156), 100 + (random() % 156), 0);

	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = Selection_GetObject(i);
		if (pobj->GetParent() != NULL)
		{
			pobj->GetParent()->RemoveChild(pobj);
		}
		pGroup->AddChild(pobj);
	}

	//
	// Keep the group as a new object. Don't keep its children here,
	// because they are not new.
	//
	GetHistory()->KeepNew(pGroup, false);

	//
	// Clear selection and add the new group to it.
	//
	m_pToolSelection->SetEmpty();
	SelectObject(pGroup, scSelect | scUpdateDisplay);
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Ungroups all selected objects.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsUngroup(void)
{
	if ((Selection_GetMode() == selectSolids) && !Options.general.bGroupWhileIgnore)
	{
		return;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Ungroup");
	GetHistory()->Keep(Selection_GetList());
	
	// create new selected list
	CMapObjectList NewSelList;
	NewSelList.AddTail(Selection_GetList());
	m_pToolSelection->SetEmpty();

	POSITION p = NewSelList.GetHeadPosition();
	while(p)
	{
		CMapClass *pobj = NewSelList.GetNext(p);
		if(!pobj->IsMapClass(MAPCLASS_TYPE(CMapGroup)))
		{
			// make sure it is selected in the map
			SelectObject(pobj, scSelect);
			continue;
		}

		//
		// Build a list of the group's children.
		//
		CUtlVector <CMapClass *> ChildList;
		POSITION p2;
		CMapClass *pChild = pobj->GetFirstChild(p2);
		while (pChild != NULL)
		{
			ChildList.AddToTail(pChild);
			pChild = pobj->GetNextChild(p2);
		}

		//
		// Detach the children from the group.
		//
		pobj->RemoveAllChildren();

		//
		// Move the group's former children to the group's parent.
		//
		int nChildCount = ChildList.Count();
		for (int i = 0; i < nChildCount; i++)
		{		
			pChild = ChildList.Element(i);

			pobj->GetParent()->AddChild(pChild);
			pChild->SetVisGroup(pobj->GetVisGroup());
			pChild->SetRenderColor(0, 100 + (random() % 156), 100 + (random() % 156));
			SelectObject(pChild, scSelect);
		}

		//
		// The group is empty; delete it.
		//
		DeleteObject(pobj);
	}

	GetMainWnd()->pObjectProperties->AbortData();
	SelectObject(NULL, scUpdateDisplay);
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Toggles the visibility of the grid in the 2D views.
//-----------------------------------------------------------------------------
void CMapDoc::OnViewGrid(void)
{
	m_bShowGrid = !m_bShowGrid;
	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the check state of the Show Grid toolbar button and menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateViewGrid(CCmdUI *pCmdUI) 
{
	pCmdUI->SetCheck(m_bShowGrid);
}


//-----------------------------------------------------------------------------
// Purpose: Selects all objects that are not hidden.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditSelectall(void)
{
	if (g_pToolManager->GetToolID() == TOOL_MORPH)
	{
		// select all vertices
		m_pToolMorph->SelectHandle(NULL, Morph3D::scSelectAll);
		ToolUpdateViews(CMapView2D::updTool);
		return;
	}

	POSITION pos;
	CMapClass *pobj = m_pWorld->GetFirstChild(pos);
	while (pobj != NULL)
	{
		if (pobj->IsVisible())
		{
			SelectObject(pobj, scSelect);
		}

		pobj = m_pWorld->GetNextChild(pos);
	}

	SelectObject(NULL, scUpdateDisplay);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szPathName - 
// Output : 
//-----------------------------------------------------------------------------
char *StripFileName(char *szPathName)
{
	char *pchSlash = strrchr(szPathName, '\\');
	if (pchSlash != NULL)
	{
		*pchSlash = '\0';
	}

	return(szPathName);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnFileSaveAs(void)
{
	CWorldcraft *pApp = (CWorldcraft *)AfxGetApp();

	static char szBaseDir[MAX_PATH] = "";

	//
	// The default directory for the Save As dialog is either:
	// 1. The directory from which the document was loaded.
	// 2. The last directory they saved any document into.
	// 3. The maps directory as set up in Options | Game Configurations.
	//
	CString str = GetPathName();
	if (str.Find('.') != -1)
	{
		str = str.Left(str.Find('.'));
		strcpy(szBaseDir, str);
		StripFileName(szBaseDir);
	}
	else if (szBaseDir[0] =='\0')
	{
		strcpy(szBaseDir, m_pGame->szMapDir);
	}

	char *pszFilter;
	if ((m_pGame->mapformat == mfHalfLife2) || (m_pGame->mapformat == mfTeamFortress2))
	{
		pszFilter = "Valve Map Files (*.vmf)|*.vmf||";
	}
	else
	{
		pszFilter = "Worldcraft Maps (*.rmf)|*.rmf|Game Maps (*.map)|*.map||";
	}

	CFileDialog dlg(FALSE, NULL, str, OFN_LONGNAMES | OFN_NOCHANGEDIR |	OFN_HIDEREADONLY, pszFilter);
	dlg.m_ofn.lpstrInitialDir = szBaseDir;
	int rvl = dlg.DoModal();
	
	if (rvl == IDCANCEL)
	{
		return;
	}

	str = dlg.GetPathName();

	//
	// Save the default directory for next time.
	//
	strcpy(szBaseDir, str);
	StripFileName(szBaseDir);

	if (str.Find('.') == -1)
	{
		if ((m_pGame->mapformat == mfHalfLife2) || (m_pGame->mapformat == mfTeamFortress2))
		{
			str += ".vmf";
		}
		else
		{
			switch (dlg.m_ofn.nFilterIndex)
			{
				case 1:
				{
					str += ".rmf";
					break;
				}
				case 2:
				{
					str += ".map";
					break;
				}
			}
		}
	}

	SetPathName(str);
	OnSaveDocument(str);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnFileSave(void)
{
	DWORD dwAttrib = GetFileAttributes(GetPathName());
	if (dwAttrib & FILE_ATTRIBUTE_READONLY)
	{
		// we do not have read-write access or the file does not (now) exist
		OnFileSaveAs();
	}
	else
	{
		if(m_strPathName.IsEmpty())
		{
			OnFileSaveAs();
		}
		else
		{
			OnSaveDocument(GetPathName());
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the File | Save menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	pCmdUI->SetText(m_bEditingPrefab ? "Update Prefab\tCtrl+S" : "&Save\tCtrl+S");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapGridlower(void)
{
	if (m_nGridSpacing <= 1)
	{
		return;
	}

	m_nGridSpacing = m_nGridSpacing / 2;
	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY);

	CString strSnap;
	strSnap.Format(" Snap: %s Grid: %d ", m_bSnapToGrid ? "On" : "Off", m_nGridSpacing);
	SetStatusText(SBI_SNAP, strSnap);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapGridhigher(void)
{
	if(m_nGridSpacing >= 512)
		return;

	m_nGridSpacing = m_nGridSpacing * 2;
	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY);

	CString strSnap;
	strSnap.Format(" Snap: %s Grid: %d ", m_bSnapToGrid ? "On" : "Off", m_nGridSpacing);
	SetStatusText(SBI_SNAP, strSnap);
}


class CExportDlg : public CFileDialog
{
public:
	CExportDlg(CString& strFile, LPCTSTR pszExt, LPCTSTR pszDesc) : 
	  CFileDialog(FALSE, pszExt, strFile,
		OFN_NOCHANGEDIR | OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_EXPLORER | 
		OFN_ENABLETEMPLATE,	pszDesc) 
	{
		m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_MAPEXPORT);
		bVisibles = FALSE;
	}

	afx_msg BOOL OnInitDialog()
	{
		m_Visibles.SubclassDlgItem(IDC_SAVEVISIBLES, this);
		m_Visibles.SetCheck(bVisibles);

		return TRUE;
	}

	afx_msg void OnToggleVisibles()
	{ 
		bVisibles = m_Visibles.GetCheck();
	}

	CButton m_Visibles;
	BOOL bVisibles;

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CExportDlg, CFileDialog)
	ON_BN_CLICKED(IDC_SAVEVISIBLES, OnToggleVisibles)
END_MESSAGE_MAP()

static BOOL bLastVis;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnFileExport(void)
{
	//
	// If we haven't saved the file yet, save it now.
	//
	CString strFile = GetPathName();
	if (strFile.IsEmpty())
	{
		OnFileSave();
		strFile = GetPathName();
		if (strFile.IsEmpty())
		{
			return;
		}
	}

	//
	// Build a name for the exported file.
	//
	int iIndex = strFile.Find('.');

	char *pszFilter;
	char *pszExtension;
	if ((m_pGame->mapformat == mfHalfLife2) || (m_pGame->mapformat == mfTeamFortress2))
	{
		strFile.SetAt(iIndex, '\0');
		
		pszFilter = "Valve Map Files (*.vmf)|*.vmf||";
		pszExtension = "vmf";
	}
	else
	{
		//
		// Use the same filename with a .map extension.
		//
		strcpy(strFile.GetBuffer(1) + iIndex, ".map");
		strFile.ReleaseBuffer();

		pszFilter = "Game Maps (*.map)|*.map||";
		pszExtension = "map";
	}

	//
	// Bring up a dialog to allow them to name the exported file.
	//
	CExportDlg dlg(strFile, pszExtension, pszFilter);

	dlg.m_ofn.lpstrTitle = "Export As";
	dlg.bVisibles = bLastVis;

	if (dlg.DoModal() == IDOK)
	{
		BOOL bModified = IsModified();

		if (strFile.CompareNoCase(dlg.GetPathName()) == 0)
		{
			if (GetMainWnd()->MessageBox("You are about to export over your current work file. Some data loss will occur if you have any objects hidden. Continue?", "Export Warning", MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
			{
				return;
			}
		}
	
		bSaveVisiblesOnly = dlg.bVisibles;
		m_strLastExportFileName = dlg.GetPathName();

		OnSaveDocument(dlg.GetPathName());
		
		bSaveVisiblesOnly = FALSE;

		SetModifiedFlag(bModified);

		bLastVis = dlg.bVisibles;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Exports using the last exported pathname. Brings up the Export dialog
//			only if this document has never been exported.
//-----------------------------------------------------------------------------
void CMapDoc::OnFileExportAgain(void)
{
	CString strFile = m_strLastExportFileName;

	//	
	// If we have never exported this map, bring up the Export dialog.
	//
	if (strFile.IsEmpty())
	{
		OnFileExport();
		return;
	}

	BOOL bModified = IsModified();

	bSaveVisiblesOnly = bLastVis;
	OnSaveDocument(strFile);
	bSaveVisiblesOnly = FALSE;

	SetModifiedFlag(bModified);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnEditMapproperties(void)
{
	CMapObjectList tmpList;
	tmpList.AddTail(m_pWorld);
	GetMainWnd()->pObjectProperties->LoadData(&tmpList, true);
	GetMainWnd()->pObjectProperties->ShowWindow(SW_SHOW);
}


//-----------------------------------------------------------------------------
// Purpose: Converts a map's textures from WAD3 to VMT.
//-----------------------------------------------------------------------------
void CMapDoc::OnFileConvertWAD( void )
{
	CTextureConverter::ConvertWorldTextures( m_pWorld );
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the File | Convert WAD -> VMT menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateFileConvertWAD( CCmdUI * pCmdUI ) 
{
	pCmdUI->Enable( ( m_pWorld != NULL ) && ( g_pGameConfig->GetTextureFormat() == tfVMT ) );
}


//-----------------------------------------------------------------------------
// Purpose: Does a normal map compile.
//-----------------------------------------------------------------------------
void CMapDoc::OnFileRunmap(void)
{
	// check for texture wads first.. can only run if some
	// texture wads are defined!
	if (!Options.textures.nTextureFiles)
	{
		AfxMessageBox("There are no texture files defined yet. Add some texture files before you run the map.");
		GetMainWnd()->Configure();
		return;
	}

	CRunMap dlg;
	CRunMapExpertDlg dlgExpert;

	CString strFile = GetPathName();

	// make sure it is saved first
	if (strFile.IsEmpty())
	{
		OnFileSave();
		strFile = GetPathName();
		if (strFile.IsEmpty())
		{
			return;
		}
	}
	
	strFile.MakeLower();
	int iPos;
	BOOL bSave = IsModified();

	if ((iPos = strFile.Find(".rmf")) != -1)
	{
		if (bSave)
		{	// save RMF file.
			OnSaveDocument(strFile);
		}

		strcpy(strFile.GetBuffer(0)+iPos, ".map");
		strFile.ReleaseBuffer();
		bSave = TRUE;
	}
	
	// make "bsp" string
	CString strBspFile(strFile);
	iPos = strBspFile.Find(".map");
	strcpy(strBspFile.GetBuffer(0)+iPos, ".bsp");
	strBspFile.ReleaseBuffer();
	
	// if no bsp file, make sure it's checked
	if (GetFileAttributes(strBspFile) == 0xFFFFFFFF)
	{
		dlg.m_iCSG = 1;
		dlg.m_iQBSP = 1;
	}

	while (1)
	{
		if (AfxGetApp()->GetProfileInt("Run Map", "Mode", 0) == 0)
		{
			// run normal dialog
			if(dlg.DoModal() == IDCANCEL)
				return;
			// switching mode?
			if(dlg.m_bSwitchMode)
			{
				dlg.m_bSwitchMode = FALSE;
				AfxGetApp()->WriteProfileInt("Run Map", "Mode", 1);
			}
			else 
			{
				dlg.SaveToIni();
				break;	// clicked OK
			}
		}
		else
		{
			// run expert dialog
			if(dlgExpert.DoModal() == IDCANCEL)
				return;
			// switching mode?
			if(dlgExpert.m_bSwitchMode)
			{
				AfxGetApp()->WriteProfileInt("Run Map", "Mode", 0);
				dlgExpert.m_bSwitchMode = FALSE;
			}
			else if(dlgExpert.m_pActiveSequence) // clicked ok
			{
				// run the commands in the active sequence
				bSaveVisiblesOnly = dlgExpert.m_bVisibleOnly;
				OnSaveDocument(strFile);
				RunCommands(dlgExpert.m_pActiveSequence->m_Commands, 
					strFile);
				return;
			}
			else return;
		}
	}

	if (GetFileAttributes(strBspFile) == 0xFFFFFFFF)
	{
		if (!dlg.m_iCSG)
		{
			dlg.m_iCSG = 1;
		}

		if (!dlg.m_iQBSP)
		{
			dlg.m_iQBSP = 1;
		}
	}

	bSaveVisiblesOnly = dlg.m_bSaveVisiblesOnly;

	// save MAP document.
	if ((dlg.m_iCSG) || (dlg.m_iQBSP))
	{
		OnSaveDocument(strFile);
	}

	bSaveVisiblesOnly = FALSE;

	CCOMMAND cmd;
	memset(&cmd, 0, sizeof cmd);
	cmd.bEnable = TRUE;
	cmd.bUseProcessWnd = TRUE;
	cmd.bLongFilenames = TRUE;

	CCommandArray cmds;

	// Change to the game drive and directory.
	cmd.iSpecialCmd = CCChangeDir;
	strcpy(cmd.szParms, m_pGame->m_szGameExeDir);
	strcpy(cmd.szRun, "Change Directory");
	cmds.Add(cmd);
	cmd.iSpecialCmd = 0;

	// Copy MAP file to BSP directory in game for compilation.
	cmd.iSpecialCmd = CCCopyFile;
	strcpy(cmd.szRun, "Copy File");
	sprintf(cmd.szParms, "$path\\$file.map $bspdir\\$file.map");
	cmds.Add(cmd);
	cmd.iSpecialCmd = 0;

	// csg
	if ((dlg.m_iCSG) && (m_pGame->m_szCSG[0] != '\0'))
	{
		strcpy(cmd.szRun, m_pGame->m_szCSG);
		sprintf(cmd.szParms, "%s$bspdir\\$file", dlg.m_iCSG == 2 ? "-onlyents " : "");
		cmds.Add(cmd);
	}

	// bsp
	if ((dlg.m_iQBSP) && (m_pGame->szBSP[0] != '\0'))
	{
		strcpy(cmd.szRun, m_pGame->szBSP);
		strcpy(cmd.szParms, "$bspdir\\$file");

		// check for bsp existence only in quake maps, because
		//  we're using worldcraft's utilities
		if (g_pGameConfig->mapformat == mfQuake)
		{
			cmd.bEnsureCheck = TRUE;
			strcpy(cmd.szEnsureFn, "$bspdir\\$file.bsp");
		}

		cmds.Add(cmd);
 
		cmd.bEnsureCheck = FALSE;
	}

	// vis
	if ((dlg.m_iVis) && (m_pGame->szVIS[0] != '\0'))
	{
		strcpy(cmd.szRun, m_pGame->szVIS);
		sprintf(cmd.szParms, "%s$bspdir\\$file", dlg.m_iVis == 2 ? "-fast " : "");
		cmds.Add(cmd);
	}

	// rad
	if ((dlg.m_iLight) && (m_pGame->szLIGHT[0] != '\0'))
	{
		strcpy(cmd.szRun, m_pGame->szLIGHT);
		sprintf(cmd.szParms, "%s$bspdir\\$file", dlg.m_iLight == 2 ? "-extra " : "");
		cmds.Add(cmd);
	}

	// Run the game.
	if (dlg.m_bNoQuake == FALSE)
	{
		cmd.bUseProcessWnd = FALSE;
		cmd.bNoWait = TRUE;
		strcpy(cmd.szRun, m_pGame->szExecutable);
		sprintf(cmd.szParms, "%s +map $file", dlg.m_strQuakeParms);
		cmds.Add(cmd);
	}

	APP()->Enable3DRender(false);
	RunCommands(cmds, GetPathName());
	APP()->Enable3DRender(true);
}


//-----------------------------------------------------------------------------
// Purpose: Updates the title of the doc based on the filename and the
//			active view type.
// Input  : pView - 
//-----------------------------------------------------------------------------
void CMapDoc::UpdateTitle(CView *pView)
{
	CString str, strFile = GetPathName();
	LPCTSTR pszFilename = strFile;
	int iPos = strFile.ReverseFind('\\');
	if (iPos != -1)
	{
		pszFilename = strFile.GetBuffer(0) + iPos + 1;
	}
	else
	{
		pszFilename = "Untitled";
	}

	char *pViewType = NULL;
	CMapView2D *pView2D = dynamic_cast <CMapView2D *> (pView);
	if (pView2D != NULL)
	{
		switch (pView2D->GetDrawType())
		{
			case VIEW2D_XY:
			{
				pViewType = "Top";
				break;
			}
			
			case VIEW2D_XZ:
			{
				pViewType = "Right";
				break;
			}
			
			case VIEW2D_YZ:
			{
				pViewType = "Front";
				break;
			}
		}
	}

	CMapView3D *pView3D = dynamic_cast <CMapView3D *> (pView);
	if (pView3D != NULL)
	{
		switch (pView3D->GetDrawType())
		{
			case VIEW3D_WIREFRAME:
			{
				pViewType = "Wireframe";
				break;
			}

			case VIEW3D_POLYGON:
			{
				pViewType = "Polygon";
				break;
			}

			case VIEW3D_TEXTURED:
			{
				pViewType = "Textured";
				break;
			}

			case VIEW3D_LIGHTMAP_GRID:
			{
				pViewType = "Lightmap grid";
				break;
			}

			case VIEW3D_SMOOTHING_GROUP:
			{
				pViewType = "Smoothing Group";
				break;
			}
		}
	}

	if (pViewType)
	{
		str.Format("%s - %s", pszFilename, pViewType);
	}
	else
	{
		str.Format("%s", pszFilename);
	}

	SetTitle(str);
}


//-----------------------------------------------------------------------------
// Purpose: Toggles the state of Hide Items. When enabled, entities are hidden.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsHideitems(void)
{
	m_bHideItems = !m_bHideItems;

	UpdateVisibilityAll();
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the Tools | Hide Items menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToolsHideitems(CCmdUI *pCmdUI) 
{
	pCmdUI->Enable(!GetMainWnd()->IsShellSessionActive());
	pCmdUI->SetCheck(m_bHideItems ? TRUE : FALSE);
}


//-----------------------------------------------------------------------------
// Purpose: Hides and shows entity names in the 2D views.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsHideEntityNames(void)
{
	bool bShowEntityNames = !CMapEntity::GetShowEntityNames();
	CMapEntity::ShowEntityNames(bShowEntityNames);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_UPDATE_2D);
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the Tools | Hide Entity Names menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToolsHideEntityNames(CCmdUI *pCmdUI) 
{
	pCmdUI->Enable(!GetMainWnd()->IsShellSessionActive());
	pCmdUI->SetCheck(CMapEntity::GetShowEntityNames() ? FALSE : TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pView - 
//-----------------------------------------------------------------------------
void CMapDoc::SetMRU(CMapView2D *pView)
{
	RemoveMRU(pView);
	MRU2DViews.AddHead(pView);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pView - 
//-----------------------------------------------------------------------------
void CMapDoc::RemoveMRU(CMapView2D *pView)
{
	POSITION p = MRU2DViews.Find(pView);
	if(p)
		MRU2DViews.RemoveAt(p);
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of all Edit menu items and toolbar buttons.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateEditFunction(CCmdUI *pCmdUI) 
{
	pCmdUI->Enable( ( g_pToolManager->GetToolID() != TOOL_FACEEDIT_MATERIAL ) &&
		            !GetMainWnd()->IsShellSessionActive() );
}


//-----------------------------------------------------------------------------
// Purpose: This is called for each doc when the texture application mode changes.
// Input  : bApplicator - TRUE if entering texture applicator mode, FALSE if
//			leaving texture applicator mode.
//-----------------------------------------------------------------------------
void CMapDoc::UpdateForApplicator(BOOL bApplicator)
{
	if (bApplicator)
	{
		//
		// Build a list of all selected solids.
		//
		CMapObjectList Solids;

		int nSelCount = Selection_GetCount();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = Selection_GetObject(i);
			if (pObject != NULL)
			{
				CMapSolid *pSolid = dynamic_cast<CMapSolid*>(pObject);

				if (pSolid != NULL)
				{
					Solids.AddTail(pSolid);
				}

				pObject->EnumChildren((ENUMMAPCHILDRENPROC)AddLeavesToListCallback, (DWORD)&Solids, MAPCLASS_TYPE(CMapSolid));
			}
		}

		//
		// Clear the object selection.
		//
		SelectObject(NULL, scClear);

		//
		// Select all faces of all solids that were selected originally. Disable updates
		// in the face properties dialog beforehand or this could take a LONG time.
		//
		HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
		GetMainWnd()->m_pFaceEditSheet->EnableUpdate( false );

		bool bFirst = true;
		POSITION pos = Solids.GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = Solids.GetNext(pos);
			CMapSolid *pSolid = dynamic_cast<CMapSolid*>(pObject);
			ASSERT(pSolid != NULL);

			if (pSolid != NULL)
			{
				SelectFace(pSolid, -1, CMapDoc::scSelect | (bFirst ? CMapDoc::scClear : 0));
				bFirst = false;
			}
		}

		GetMainWnd()->m_pFaceEditSheet->EnableUpdate( true );
		SetCursor(hCursorOld);

		UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY);
	}
	else
	{
		//
		// Remove all faces from the dialog's list and update their selection state to be
		// not selected, then update the display.
		//
		GetMainWnd()->m_pFaceEditSheet->ClickFace( NULL, -1, CFaceEditSheet::cfClear );
		UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY);
		UpdateStatusbar();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSolid - 
//			iFace - 
//			cmd - 
//-----------------------------------------------------------------------------
void CMapDoc::SelectFace(CMapSolid *pSolid, int iFace, int cmd)
{
	APP()->SetForceRenderNextFrame();

	bool bFirst = true;
	if(iFace == -1 && pSolid)
	{
		// Get draw solid/disp mask.
		bool bDispSolidMask = CMapDoc::GetActiveMapDoc()->IsDispSolidDrawMask() && pSolid->HasDisp();

		BOOL bUpdateDisplay = (cmd & scUpdateDisplay) ? TRUE : FALSE;
		cmd &= ~scUpdateDisplay;

		// select entire object
		int nFaces = pSolid->GetFaceCount();
		for(int i = 0; i < nFaces; i++)
		{
			if ( bDispSolidMask )
			{
				CMapFace *pFace = pSolid->GetFace( i );
				if( pFace && pFace->HasDisp() )
				{
					SelectFace(pSolid, i, cmd);
					if ( bFirst )
					{
						cmd &= ~CMapDoc::scClear;
						bFirst = false;
					}
				}
			}
			else
			{
				SelectFace(pSolid, i, cmd);
				if ( bFirst )
				{	
					cmd &= ~CMapDoc::scClear;
					bFirst = false;
				}
			}
		}

		if(bUpdateDisplay)
			SelectFace(NULL, 0, scUpdateDisplay);

		return;
	}

	CFaceEditSheet *pSheet = GetMainWnd()->m_pFaceEditSheet;
	UINT uFaceCmd = 0;
	
	if(cmd & scClear)
	{
		uFaceCmd |= CFaceEditSheet::cfClear;
	}
	if(cmd & scToggle)
	{
		uFaceCmd |= CFaceEditSheet::cfToggle;
	}
	if(cmd & scSelect)
	{
		uFaceCmd |= CFaceEditSheet::cfSelect;
	}
	if(cmd & scUnselect)
	{
		uFaceCmd |= CFaceEditSheet::cfUnselect;
	}

	//
	// Change the click mode to ModeSelect if the scNoLift flag is set.
	//
	int nClickMode;

	if (cmd & scNoLift) // dvs: this is lame
	{
		nClickMode = CFaceEditSheet::ModeSelect;
	}
	else
	{
		nClickMode = -1;
	}

	//
	// Check the current click mode and perform the texture application if appropriate.
	//
	BOOL bApply = FALSE;

	if (!(cmd & scNoApply))
	{
		int iFaceMode = pSheet->GetClickMode(); 
		bApply = ( ( iFaceMode == CFaceEditSheet::ModeApply ) || ( iFaceMode == CFaceEditSheet::ModeApplyAll ) );

		if (bApply && pSolid)
		{
			GetHistory()->MarkUndoPosition(NULL, "Apply texture");
			GetHistory()->Keep(pSolid);
		}
	}

	pSheet->ClickFace( pSolid, iFace, uFaceCmd, nClickMode );

	// update display?
	if(bApply || cmd & scUpdateDisplay)
	{
		UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_COLOR_ONLY);
		UpdateStatusbar();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapInformation(void)
{
	CMapInfoDlg dlg(m_pWorld);
	dlg.DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: Forces a render of all the 3D views. Called from OnIdle to render
//			the 3D views.
//-----------------------------------------------------------------------------
void CMapDoc::SetActiveView(CMapView *pViewActivate)
{
	POSITION p = GetFirstViewPosition();

	while (p)
	{
		CMapView *pView = (CMapView *)GetNextView(p);
		pView->Activate(pView == pViewActivate);
	}
}


//-----------------------------------------------------------------------------
// releases video memory
//-----------------------------------------------------------------------------
void CMapDoc::ReleaseVideoMemory( )
{
	POSITION p = GetFirstViewPosition();

	while (p)
	{
		CMapView3D *pView = dynamic_cast<CMapView3D *>(GetNextView(p));
		if (pView)
			pView->ReleaseVideoMemory();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vi - 
//-----------------------------------------------------------------------------
void CMapDoc::SetView2dInfo(VIEW2DINFO& vi)
{
	POSITION p = GetFirstViewPosition();
	while(p)
	{
		CView *pView = GetNextView(p);
		if(!pView->IsKindOf(RUNTIME_CLASS(CMapView2D)))
			continue;

		CMapView2D *pView2D = (CMapView2D*) pView;

		pView2D->SetRedraw(FALSE);

		// set zoom value
		if(vi.wFlags & VI_ZOOM)
		{
			pView2D->SetZoom(vi.fZoom);
		}

		// center on point
		if(vi.wFlags & VI_CENTER)
		{
			pView2D->CenterView(&vi.ptCenter);
		}

		pView2D->SetRedraw(TRUE);
		pView2D->Invalidate();
	}
}


void CMapDoc::Center3DViewsOn( const Vector &vPos )
{
	POSITION p = GetFirstViewPosition();
	while(p)
	{
		CView *pView = GetNextView(p);
		if(!pView->IsKindOf(RUNTIME_CLASS(CMapView3D)))
			continue;

		CMapView3D *pView3D = (CMapView3D*) pView;
	
		pView3D->SetRedraw( false );

		Vector vForward;
		pView3D->GetCamera()->GetViewForward( vForward );

		pView3D->SetCamera( vPos - Vector( 0, 100, 0 ), vPos );

		pView3D->SetRedraw( true );
		pView3D->Invalidate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Centers the 2D views on selected objects.
//-----------------------------------------------------------------------------
void CMapDoc::OnViewCenteronselection(void)
{
	CenterSelection();
}


//-----------------------------------------------------------------------------
// Purpose: Hollows selected solids by carving them with a scaled version of
//			themselves.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsHollow(void)
{
	//
	// Confirm the operation if there is more than one object selected.
	//
	if (Selection_GetCount() > 1)
	{
		if (AfxMessageBox("Do you want to turn each of the selected solids into a hollow room?", MB_YESNO) == IDNO)
		{
			return;
		}
	}

	//
	// Prompt for the wall thickness, which is remembered from one hollow to another.
	//
	static int iWallWidth = 32;
	char szBuf[128];
	itoa(iWallWidth, szBuf, 10);
	CStrDlg dlg(CStrDlg::Spin, szBuf, "How thick do you want the walls? Use a negative number to hollow outward.", "Worldcraft");
	dlg.SetRange(-1024, 1024, 4);
	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}
	iWallWidth = atoi(dlg.m_string);

	if (abs(iWallWidth) < 2)
	{
		AfxMessageBox("The width of the walls must be less than -1 or greater than 1.");
		return;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Hollow");
	ToolUpdateViews(CMapView2D::updEraseTool);

	//
	// Build a list of all solids in the selection.
	//
	CMapObjectList SelectedSolids;
	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);

		CMapSolid *pSolid = dynamic_cast <CMapSolid *> (pObject);
		if (pSolid != NULL)
		{
			SelectedSolids.AddTail(pSolid);
		}

		EnumChildrenPos_t pos2;
		CMapClass *pChild = pObject->GetFirstDescendent(pos2);
		while (pChild != NULL)
		{
			CMapSolid *pSolid = dynamic_cast <CMapSolid *> (pChild);
			if (pSolid != NULL)
			{
				SelectedSolids.AddTail(pSolid);
			}
			pChild = pObject->GetNextDescendent(pos2);
		}
	}

	//
	// Carve every solid in the selection with a scaled copy of itself. This accomplishes
	// the goal of hollowing them.
	//
	CMapSolid ScaledCopy;
	POSITION pos = SelectedSolids.GetHeadPosition();
	while (pos != NULL)
	{
		CMapSolid *pSelectedSolid = (CMapSolid *)SelectedSolids.GetNext(pos);
		CMapClass *pDestParent = pSelectedSolid->GetParent();

		GetHistory()->Keep(pSelectedSolid);

		ScaledCopy.CopyFrom(pSelectedSolid, false);
		ScaledCopy.SetObjParent(NULL);

		//
		// Get bounds of the solid to be hollowed and calculate scaling required to
		// reduce by iWallWidth.
		//
		BoundBox box;
		Vector ptCenter;
		Vector vecScale;

		pSelectedSolid->GetRender2DBox(box.bmins, box.bmaxs);
		for (int i = 0; i < 3; i++)
		{
			float fHalf = (box.bmaxs[i] - box.bmins[i]) / 2;
			vecScale[i] = (fHalf - iWallWidth) / fHalf;
		}

		ScaledCopy.GetBoundsCenter(ptCenter);
		ScaledCopy.TransScale(ptCenter, vecScale);
		
		//
		// Set up the operands for the subtraction operation.
		//
		CMapSolid *pSubtractWith;
		CMapSolid *pSubtractFrom;

		if (iWallWidth > 0)
		{
			pSubtractFrom = pSelectedSolid;
			pSubtractWith = &ScaledCopy;
		}
		//
		// Negative wall widths reverse the subtraction.
		//
		else
		{
			pSubtractFrom = &ScaledCopy;
			pSubtractWith = pSelectedSolid;
		}

		//
		// Perform the subtraction. If the two objects intersected...
		//
		CMapObjectList Outside;
		if (pSubtractFrom->Subtract(NULL, &Outside, pSubtractWith))
		{
			//
			// If there were pieces outside the 'subtract with' object...
			//
			if (Outside.GetCount() > 0)
			{
				CMapClass *pResult;

				//
				// If the subtraction resulted in more than one object, create a group
				// to place the results in.
				//
				if (Outside.GetCount() > 1)
				{
					pResult = (CMapClass *)(new CMapGroup);
					POSITION pos2 = Outside.GetHeadPosition();
					while (pos2 != NULL)
					{
						CMapClass *pTemp = Outside.GetNext(pos2);
						pResult->AddChild(pTemp);
					}
				}
				//
				// Otherwise, the results are the single object.
				//
				else if (Outside.GetCount() == 1)
				{
					POSITION pos2 = Outside.GetHeadPosition();
					pResult = Outside.GetAt(pos2);
				}

				//
				// Replace the current solid with the subtraction results.
				//
				DeleteObject(pSelectedSolid);
				AddObjectToWorld(pResult, pDestParent);
				GetHistory()->KeepNew(pResult);
			}
		}
	}

	//
	// Objects in selection no longer exist.
	//
	m_pToolSelection->SetEmpty();

	ToolUpdateViews(CMapView2D::updEraseTool);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);

	SetModifiedFlag();

	//
	// Make sure object properties dialog is not referencing
	// selected objects.
	//
	GetMainWnd()->pObjectProperties->AbortData();
	GetMainWnd()->SetFocus();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnEditPastespecial(void)
{
	CPasteSpecialDlg dlg(GetMainWnd(), &s_Clipboard.Bounds);
	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}
	dlg.SaveToIni();

	BeginWaitCursor();
	GetHistory()->MarkUndoPosition(Selection_GetList(), "Paste");

	// first, clear selection so we can select all pasted objects
	SelectObject(NULL, scClear | scUpdateDisplay);

	//
	// Build a paste translation.
	//
	Vector vecPasteOffset( COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT );

	if (!dlg.m_bCenterOriginal)
	{
		GetBestPastePoint(vecPasteOffset);
		vecPasteOffset -= s_Clipboard.vecOriginalCenter;
	}
	else
	{
		vecPasteOffset[0] = dlg.m_iOffsetX;
		vecPasteOffset[1] = dlg.m_iOffsetY;
		vecPasteOffset[2] = dlg.m_iOffsetZ;
	}

	//
	// Build the paste rotation angles.
	//
	QAngle vecPasteAngles;
	vecPasteAngles[0] = dlg.m_fRotateX;
	vecPasteAngles[1] = dlg.m_fRotateY;
	vecPasteAngles[2] = dlg.m_fRotateZ;

	CMapWorld *pWorld = GetActiveWorld();

	CMapClass *pParent = NULL;
	CMapGroup *pGroup = NULL;

	if (dlg.m_bGroup)
	{
		pGroup = new CMapGroup;
		pParent = (CMapClass *)pGroup;
		AddObjectToWorld(pGroup, pWorld);
	}
	
	BOOL bOldTexLock = Options.SetLockingTextures(TRUE);

	for (int i = 0; i < dlg.m_iCopies; i++)
	{
		//
		// Paste the objects with the current offset and rotation.
		//
		Paste(s_Clipboard.Objects, s_Clipboard.pSourceWorld, pWorld, vecPasteOffset, vecPasteAngles, pParent);

		//
		// Increment the paste offset.
		//
		vecPasteOffset[0] += dlg.m_iOffsetX;
		vecPasteOffset[1] += dlg.m_iOffsetY;
		vecPasteOffset[2] += dlg.m_iOffsetZ;

		//
		// Increment the paste angles.
		//
		vecPasteAngles[0] += dlg.m_fRotateX;
		vecPasteAngles[1] += dlg.m_fRotateY;
		vecPasteAngles[2] += dlg.m_fRotateZ;
	}

	//
	// If we pasted into a group, keep the group now.
	//
	if (pGroup != NULL)
	{
		GetHistory()->KeepNew(pGroup);
		SelectObject(pGroup, scSelect);
	}

	UpdateBox box;
	box.Objects = Selection_GetList();
	m_pToolSelection->GetBounds(box.Box.bmins, box.Box.bmaxs);

	g_pToolManager->SetTool(TOOL_POINTER);

	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_UPDATE_OBJECTS, &box);

	SetModifiedFlag();
	EndWaitCursor();
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the Edit | Paste Special menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateEditPastespecial(CCmdUI *pCmdUI) 
{
	pCmdUI->Enable((s_Clipboard.Objects.GetCount() != 0) && !GetMainWnd()->IsShellSessionActive());
}


//-----------------------------------------------------------------------------
// Purpose: Does the undo or redo and restores the selection to the state at
//			which it was marked in the Undo system.
// Input  : nID - ID_EDIT_UNDO or ID_EDIT_REDO.
// Output : Always returns TRUE.
//-----------------------------------------------------------------------------
BOOL CMapDoc::OnUndoRedo(UINT nID)
{
	//
	// Morph operations are not undo-friendly because they use a non-CMapClass
	// derived object (SSolid) to store intermediate object state.
	//
	if (g_pToolManager->GetToolID() == TOOL_MORPH)
	{
		AfxMessageBox("You must exit morph mode to undo changes you've made.");
		return(TRUE);
	}

	//
	// Empty the selection. It will be replaced with whatever the Undo system
	// says should be selected.
	//
	SelectObject(NULL, scClear);
	
	//
	// Do the undo/redo.
	//
	CMapObjectList NewSelection;
	if (nID == ID_EDIT_UNDO)
	{
		m_pUndo->Undo(&NewSelection);
	}
	else
	{
		m_pRedo->Undo(&NewSelection);
	}

	if (m_pToolSelection->IsBoxSelecting())
	{
		m_pToolSelection->EndBoxSelection();
	}

	//
	// Change the selection to the objects that the undo system says
	// should be selected now.
	//
	POSITION p = NewSelection.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pObject = NewSelection.GetNext(p);
		SelectObject(pObject, scSelect);
	}

	//
	// The undo system doesn't keep track of selected faces, so clear the
	// face selection just to be safe.
	//
	if( g_pToolManager->GetToolID() == TOOL_FACEEDIT_MATERIAL )
	{
		SelectFace(NULL, -1, scClear);
	}

	m_pToolSelection->RemoveDead();
	CMapClass::UpdateAllDependencies(NULL);

	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY | MAPVIEW_UPDATE_OBJECTS, NULL);
	GetMainWnd()->pObjectProperties->LoadData(Selection_GetList(), false);

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the Undo/Redo menu item.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateUndoRedo(CCmdUI *pCmdUI) 
{
	CHistory *pHistory = (pCmdUI->m_nID == ID_EDIT_UNDO) ? m_pUndo : m_pRedo;
	char *pszAction = (pCmdUI->m_nID == ID_EDIT_UNDO) ? "Undo" : "Redo";
	char *pszHotkey = (pCmdUI->m_nID == ID_EDIT_UNDO) ? "Ctrl+Z" : "Ctrl+Y";

	if (pHistory->IsUndoable())
	{
		pCmdUI->Enable(!GetMainWnd()->IsShellSessionActive());

		CString str;
		str.Format("%s %s\t%s", pszAction, pHistory->GetCurTrackName(), pszHotkey);
		pCmdUI->SetText(str);
	}
	else
	{
		CString str;
		str.Format("Can't %s\t%s", pszAction, pszHotkey);
		pCmdUI->SetText(str);
		pCmdUI->Enable(FALSE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::ClearHitList(void)
{
	HitSel.RemoveAll();
	nHitSel = 0;
	iCurHit = -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CMapDoc::AddHit(CMapClass *pObject)
{
	if (!HitSel.Find(pObject))
	{
		HitSel.AddTail(pObject);
		++nHitSel;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : POSITION
//-----------------------------------------------------------------------------
POSITION CMapDoc::GetFirstHitPosition(void)
{
	return HitSel.GetHeadPosition();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : p - 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapDoc::GetNextHit(POSITION& p)
{
	return HitSel.GetNext(p);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iIndex - 
//			bUpdateViews - 
//-----------------------------------------------------------------------------
void CMapDoc::SetCurHit(int iIndex, BOOL bUpdateViews)
{
	if (nHitSel == 0)
	{
		return;
	}
	
	int iOldHit = iCurHit;
	if (iIndex == hitNext)
	{
		// hit next object
		iIndex = iCurHit + 1;
		if (iIndex >= nHitSel)
		{
			iIndex = 0;
		}
	}
	else if (iIndex == hitPrev)
	{
		// hit prev object
		iIndex = iCurHit - 1;
		if (iIndex < 0)
		{
			iIndex = nHitSel - 1;
		}
	}

	CMapClass *pObject;

	// toggle old sel
	if (iOldHit != -1)
	{
		pObject = HitSel.GetAt(HitSel.FindIndex(iOldHit));
		SelectObject(pObject, scToggle);
	}

	// toggle new sel
	pObject = HitSel.GetAt(HitSel.FindIndex(iIndex));
	SelectObject(pObject, scToggle);

	iCurHit = iIndex;

	if (bUpdateViews)
	{
		SelectObject(NULL, scUpdateDisplay);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			pWorld - 
//-----------------------------------------------------------------------------
bool CMapDoc::ExpandTargetNameKeywords(char *szNewTargetName, const char *szOldTargetName, CMapWorld *pWorld)
{
	const char *pszKeyword = strstr(szOldTargetName, "&i");
	if (pszKeyword != NULL)
	{
		char szPrefix[100];
		char szSuffix[100];

		strncpy(szPrefix, szOldTargetName, pszKeyword - szOldTargetName);
		szPrefix[pszKeyword - szOldTargetName] = '\0';

		strcpy(szSuffix, pszKeyword + 2);

		int nHighestIndex = 0;

		POSITION pos;
		CMapEntity *pEntity = pWorld->EntityList_GetFirst(pos);
		while (pEntity != NULL)
		{
			const char *pszTargetName = pEntity->GetKeyValue("targetname");

			//
			// If this entity has a targetname, check to see if it is of the
			// form <prefix><number><suffix>. If so, it must be counted as
			// we search for the highest instance number. 
			//
			if (pszTargetName != NULL)
			{
				char szTemp[MAX_PATH];
				strcpy(szTemp, pszTargetName);

				int nPrefixLen = strlen(szPrefix);
				int nSuffixLen = strlen(szSuffix);

				int nFullLen = strlen(szTemp);

				//
				// It must be longer than the prefix and the suffix combined to be
				// of the form <prefix><number><suffix>.
				//
				if (nFullLen > nPrefixLen + nSuffixLen)
				{
					char *pszTempSuffix = szTemp + nFullLen - nSuffixLen;

					//
					// If the prefix and the suffix match ours, extract the instance number
					// from between them and check it against our highest instance number.
					//
					if ((strnicmp(szTemp, szPrefix, nPrefixLen) == 0) && (stricmp(pszTempSuffix, szSuffix) == 0))
					{
						*pszTempSuffix = '\0';

						bool bAllDigits = true;
						for (int i = 0; i < (int)strlen(&szTemp[nPrefixLen]); i++)
						{
							if (!isdigit(szTemp[nPrefixLen + i]))
							{
								bAllDigits = false;
								break;
							}
						}

						if (bAllDigits)
						{
							int nIndex = atoi(&szTemp[nPrefixLen]);
							if (nIndex > nHighestIndex)
							{
								nHighestIndex = nIndex;
							}
						}
					}
				}
			}

			pEntity = pWorld->EntityList_GetNext(pos);
		}

		sprintf(szNewTargetName, "%s%d%s", szPrefix, nHighestIndex + 1, szSuffix);
	
		return(true);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			pWorld - 
//-----------------------------------------------------------------------------
bool CMapDoc::DoExpandKeywords(CMapClass *pObject, CMapWorld *pWorld, char *szOldKeyword, char *szNewKeyword)
{
	CEditGameClass *pEditGameClass = dynamic_cast <CEditGameClass *>(pObject);
	if (pEditGameClass != NULL)
	{
		const char *pszOldTargetName = pEditGameClass->GetKeyValue("targetname");
		if (pszOldTargetName != NULL)
		{
			char szNewTargetName[MAX_PATH];
			if (ExpandTargetNameKeywords(szNewTargetName, pszOldTargetName, pWorld))
			{
				strcpy(szOldKeyword, pszOldTargetName);
				strcpy(szNewKeyword, szNewTargetName);
				pEditGameClass->SetKeyValue("targetname", szNewTargetName);
				return(true);
			}
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the children of the given object and expands any keywords
//			in the object's properties.
// Input  : pObject - 
//			pWorld - 
//-----------------------------------------------------------------------------
void CMapDoc::ExpandObjectKeywords(CMapClass *pObject, CMapWorld *pWorld)
{
	char szOldName[MAX_PATH];
	char szNewName[MAX_PATH];

	if (DoExpandKeywords(pObject, pWorld, szOldName, szNewName))
	{
		pObject->ReplaceTargetname(szOldName, szNewName);
	}

	//
	// Expand keywords in this object's children as well.
	//
	EnumChildrenPos_t pos;
	CMapClass *pChild = pObject->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		if (DoExpandKeywords(pChild, pWorld, szOldName, szNewName))
		{
			pObject->ReplaceTargetname(szOldName, szNewName);
		}

		pChild = pObject->GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnEditSelnext(void)
{
	// hit next object
	SetCurHit(hitNext);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnEditSelprev(void)
{
	// hit prev object
	SetCurHit(hitPrev);
}


//-----------------------------------------------------------------------------
// Purpose: Adds all selected objects to a new VisGroup and hides the group.
//-----------------------------------------------------------------------------
void CMapDoc::OnViewHideselectedobjects(void)
{
	if (!Selection_GetCount())
	{
		return;
	}

	//
	// First, a preliminary check to see if there are any 
	// eligible objects selected.
	//
	int iCount = 0;
	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);
		if (pObject->GetParent() == m_pWorld)
		{
			++iCount;
		}
	}

	if (!iCount)
	{
		AfxMessageBox("There are no eligible selected objects. The only objects\n"
					  "that can be put in a Visible Group are objects that are not\n"
					  "part of another entity or Object Group.");
		return;
	}
	else if (iCount != Selection_GetCount())
	{
		AfxMessageBox("Some objects weren't put in the new Visible Group because\n"
					  "they are part of an entity or Object Group.");
	}

	//
	// Create a group in which to place the selected objects.
	//
	CString str;
	str.Format("%d object%s", iCount, iCount == 1 ? "" : "s");
	CVisGroup *pVisGroup = VisGroups_AddGroup(str);
	pVisGroup->SetVisible(false);

	//
	// Place the selected objects in the new group.
	//
	nSelCount = Selection_GetCount();
	for (i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);
		if (pObject->GetParent() == m_pWorld)
		{
			pObject->SetVisGroup(pVisGroup);
		}
	}

	GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);

	//
	// Clean up any visgroups with no members.
	//
	VisGroups_PurgeGroups();

	//
	// Create update box to be passed to view update.
	//
	UpdateBox ub;
	CMapObjectList Objects;
	Objects.AddTail(Selection_GetList());
	ub.Objects = &Objects;
	m_pToolSelection->GetBounds(ub.Box.bmins, ub.Box.bmaxs);

	//
	// Clear the selection.
	//
	ToolUpdateViews(CMapView2D::updEraseTool);
	m_pToolSelection->SetEmpty();
	
	//
	// Update object visiblity and refresh views.
	//
	UpdateVisibilityAll();

	GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);

	// dvs: this should be done inside of selection.SetEmpty, but need to investigate ramifications of that
	GetMainWnd()->pObjectProperties->LoadData(Selection_GetList(), true);

	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY, &ub);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_COLOR_ONLY);
}


//-----------------------------------------------------------------------------
// Purpose: Adds all unselected objects to a new VisGroup and hides the group.
//-----------------------------------------------------------------------------
void CMapDoc::OnViewHidenonselectedobjects(void)
{
	if (!Selection_GetCount())
	{
		return;
	}

	//
	// Create a group in which to place the unselected objects.
	//
	CVisGroup *pVisGroup = VisGroups_AddGroup("new group");
	pVisGroup->SetVisible(false);

	//
	// Add all the unselected objects to the group.
	//
	AddNonSelectedInfo_t info;
	CMapObjectList Objects;
	BoundBox box;
	info.pList = &Objects;
	info.pBox = &box;
	info.nCount = 0;
	info.pGroup = pVisGroup;
	info.pWorld = m_pWorld;
	m_pWorld->EnumChildren((ENUMMAPCHILDRENPROC)AddUnselectedToGroupCallback, (DWORD)&info);

	//
	// If no unselected objects were found, delete the group and exit.
	//
	if (!info.nCount)
	{
		m_VisGroups.RemoveKey(pVisGroup->GetID());
		return;
	}

	//
	// Name the new group.
	//
	CString str;
	str.Format("%d object%s", info.nCount, info.nCount == 1 ? "" : "s");
	pVisGroup->SetName(str);

	//
	// Clean up any visgroups with no members.
	//
	VisGroups_PurgeGroups();

	//
	// Create update box to be passed to view update.
	//
	UpdateBox ub;
	ub.Objects = &Objects;
	ub.Box = box;

	//
	// Update object visiblity and refresh views.
	//
	UpdateVisibilityAll();

	GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);

	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY, &ub);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_COLOR_ONLY);
}


//-----------------------------------------------------------------------------
// Purpose: Reflects the current handle mode of the selection tool.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateViewShowHelpers(CCmdUI *pCmdUI) 
{
//	if (pCmdUI->m_nID == ID_VIEW_SELECTION_ONLY)
//	{
//		pCmdUI->SetCheck(m_pToolSelection->GetHandleMode() == HandleMode_SelectionOnly);
//	}
//	else if (pCmdUI->m_nID == ID_VIEW_HELPERS_ONLY)
//	{
		pCmdUI->SetCheck((m_pToolSelection->GetHandleMode() == HandleMode_HelpersOnly) || (m_pToolSelection->GetHandleMode() == HandleMode_Both));
//	}
//	else if (pCmdUI->m_nID == ID_VIEW_SELECTION_AND_HELPERS)
//	{
//		pCmdUI->SetCheck(m_pToolSelection->GetHandleMode() == HandleMode_Both);
//	}

	pCmdUI->Enable(!GetMainWnd()->IsShellSessionActive());
}


//-----------------------------------------------------------------------------
// Purpose: Cycles the selection handle mode between selection handles, helper
//			handles, or both.
//-----------------------------------------------------------------------------
void CMapDoc::OnViewShowHelpers(void)
{
	// FIXME: this only sets the handle mode for the active document's selection tool!
	Options.SetShowHelpers(!Options.GetShowHelpers());
	m_pToolSelection->SetHandleMode(Options.GetShowHelpers() ? HandleMode_Both : HandleMode_SelectionOnly);
	UpdateVisibilityAll();
}


//-----------------------------------------------------------------------------
// Purpose: Manages the state of the View | Hide Unselected menu item and toolbar button.
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateViewHidenonselectedobjects(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!GetMainWnd()->IsShellSessionActive());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnViewShowhiddenobjects(void)
{
	// show all groups
	POSITION p = m_VisGroups.GetStartPosition();
	WORD id;
	CVisGroup *pGroup;
	BOOL bNeedUpdate = FALSE;
	while(p)
	{
		m_VisGroups.GetNextAssoc(p, id, pGroup);
		if (!pGroup->IsVisible())
		{
			pGroup->SetVisible(true);
			bNeedUpdate = TRUE;
		}
	}

	if(!bNeedUpdate)
	{
		AfxMessageBox("There are no hidden objects.");
		return;
	}

	GetMainWnd()->m_FilterControl.UpdateGroupList();

	UpdateVisibilityAll();
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_DISPLAY);
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapCheck(void)
{
	CMapCheckDlg dlg;
	dlg.DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnViewShowconnections(void)
{
	bool bShow = CMapEntity::GetShowEntityConnections();
	CMapEntity::ShowEntityConnections(!bShow);

	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_UPDATE_2D);
}


//-----------------------------------------------------------------------------
// Purpose: Puts one of every point entity in the current FGD set in the current
//			map on a 128 grid.
//-----------------------------------------------------------------------------
void CMapDoc::OnMapEntityGallery(void)
{
	if (GetMainWnd()->MessageBox("This will place one of every possible point entity in the current map! Performing this operation in an empty map is recommended. Continue?", "Create Entity Gallery", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
	{
		int x = -1024;
		int y = -1024;

		POSITION p = pGD->Classes.GetHeadPosition();
		CString str;
		while(p)
		{
			GDclass *pc = pGD->Classes.GetNext(p);
			if (!pc->IsBaseClass())
			{
				if (!pc->IsSolidClass())
				{
					if (!pc->IsClass("worldspawn"))
					{
						CreateEntity(pc->GetName(), x, y, 0);
						x += 128;
						if (x > 1024)
						{
							x = -1024;
							y += 128;
						}
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateViewShowconnections(CCmdUI *pCmdUI) 
{
	pCmdUI->SetCheck(CMapEntity::GetShowEntityConnections());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszFileName - 
//			nSize - 
//-----------------------------------------------------------------------------
bool GetSaveAsFilename(const char *pszBaseDir, char *pszFileName, int nSize)
{
	CString str;
	CFileDialog dlg(FALSE, NULL, str, OFN_LONGNAMES | OFN_NOCHANGEDIR |	OFN_HIDEREADONLY, "Valve Map Files (*.vmf)|*.vmf||");
	dlg.m_ofn.lpstrInitialDir = pszBaseDir;
	int nRet = dlg.DoModal();

	if (nRet != IDCANCEL)
	{
		str = dlg.GetPathName();
		
		if (str.Find('.') == -1)
		{
			str += ".vmf";
		}

		lstrcpyn(pszFileName, str, nSize);
		return(true);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Takes the current selection and saves it as a prefab. The user is
//			prompted for a folder under the prefabs folder in which to place
//			the prefab.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsCreateprefab(void)
{
	if (!Selection_GetCount())
	{
		AfxMessageBox("This feature creates a prefab with the selected objects. You must select some objects before you can use it.", MB_ICONINFORMATION | MB_OK);
		return;
	}

	//
	// Get a file to save the prefab into. The first time through the default folder
	// is the prefabs folder.
	//
	static char szBaseDir[MAX_PATH] = "";
	if (szBaseDir[0] == '\0')
	{
		APP()->GetDirectory(DIR_PREFABS, szBaseDir);
	}

	char szFilename[MAX_PATH];
	if (!GetSaveAsFilename(szBaseDir, szFilename, sizeof(szFilename)))
	{
		return;
	}

	//
	// Save the default folder for next time.
	//
	strcpy(szBaseDir, szFilename);
	char *pch = strrchr(szBaseDir, '\\');
	if (pch != NULL)
	{
		*pch = '\0';
	}

	//
	// Create a prefab world to contain the selected items. Add the selected
	// items to the new world.
	//
	CMapWorld *pNewWorld = new CMapWorld;
	pNewWorld->SetTemporary(TRUE);
	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);
		CMapClass *pNew = pObject->Copy(false);

		// HACK: prune the object from the tree without doing any notification
		//       this prevents CopyChildrenFrom from updating the current world's culling tree
		pNew->SetObjParent(NULL);

		pNew->CopyChildrenFrom(pObject, false);
		pNewWorld->AddObjectToWorld(pNew);
	}
	pNewWorld->CalcBounds(TRUE);

	//
	// Create a prefab object and attach the world to it.
	//
	CPrefabVMF *pPrefab = new CPrefabVMF;
	pPrefab->SetWorld(pNewWorld);
	pPrefab->SetFilename(szFilename);

	//
	// Save the world to the chosen filename.
	//
	CChunkFile File;
	ChunkFileResult_t eResult = File.Open(szFilename, ChunkFile_Write);

	if (eResult == ChunkFile_Ok)
	{
		CSaveInfo SaveInfo;
		SaveInfo.SetVisiblesOnly(false);

		//
		// Write the map file version.
		//
		if (eResult == ChunkFile_Ok)
		{
			// HACK: make sure we save it as a prefab, not as a normal map
			bool bPrefab = m_bPrefab;
			m_bPrefab = true;
			eResult = SaveVersionInfoVMF(&File);
			m_bPrefab = bPrefab;
		}

		//
		// Save the world.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = pNewWorld->SaveVMF(&File, &SaveInfo, false);
		}
	}

	//
	// Try to locate the prefab library that corresponds to the folder where
	// we saved the prefab. If it doesn't exist, try refreshing the prefab library
	// list. Maybe the user just created the folder during this save.
	//
	CPrefabLibrary *pLibrary = CPrefabLibrary::FindOpenLibrary(szBaseDir);
	if (pLibrary == NULL)
	{
		delete pPrefab;

		//
		// This will take care of finding the prefab and adding it to the list.
		//
		CPrefabLibrary::LoadAllLibraries();
	}
	else
	{
		pLibrary->Add(pPrefab);
	}

	//
	// Update the object bar so the new prefab shows up.
	//
	GetMainWnd()->m_ObjectBar.UpdateListForTool(g_pToolManager->GetToolID());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnInsertprefabOriginal(void)
{
	int iCurTool = g_pToolManager->GetToolID();
	if ((iCurTool != TOOL_POINTER) && (iCurTool != TOOL_BLOCK) && (iCurTool != TOOL_ENTITY))
	{
		return;
	}

	BoundBox box;
	if (GetMainWnd()->m_ObjectBar.GetPrefabBounds(&box) == FALSE)
	{
		return;	// not a prefab listing
	}
		
	Vector pt( COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT );

	if (iCurTool != TOOL_ENTITY)
	{
		GetBestVisiblePoint(pt);
	}
	else
	{
		m_pToolMarker->GetPos(pt);
	}

	Vector ptCenter;
	box.GetBoundsCenter(ptCenter);

	for(int i = 0; i < 3; i++)
	{
		box.bmins[i] += pt[i] - ptCenter[i];
		box.bmaxs[i] += pt[i] - ptCenter[i];
	}

	// create object
	box.SnapToGrid(m_nGridSpacing);	// snap to grid first
	CMapClass *pObject = GetMainWnd()->m_ObjectBar.CreateInBox(&box);
	if (pObject == NULL)
	{
		return;
	}

	ExpandObjectKeywords(pObject, m_pWorld);

	GetHistory()->MarkUndoPosition(NULL, "Insert Prefab");
	AddObjectToWorld(pObject);
	GetHistory()->KeepNew(pObject);
	
	UpdateBox ub;
	CMapObjectList ObjectList;
	ObjectList.AddTail(pObject);
	ub.Objects = &ObjectList;

	Vector mins;
	Vector maxs;
	pObject->GetRender2DBox(mins, maxs);
	ub.Box.SetBounds(mins, maxs);

	SelectObject(pObject, CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay);
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_3D, &ub);

	// set modified
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Find a substring within a string:
// Input  : *pszSub - 
//			*pszMain - 
// Output : static char *
//-----------------------------------------------------------------------------
static char * FindInString(char *pszSub, char *pszMain)
{
	char *p = pszMain;
	int nSub = strlen(pszSub);
	
	char ch1 = toupper(pszSub[0]);

	while(p[0])
	{
		if(ch1 == toupper(p[0]))
		{
			if(!strnicmp(pszSub, p, nSub))
				return p;
		}
		++p;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSolid - 
//			*pInfo - 
// Output : static BOOL
//-----------------------------------------------------------------------------
static BOOL ReplaceTexFunc(CMapSolid *pSolid, ReplaceTexInfo_t *pInfo)
{
	// make sure it's visible
	if (!pInfo->bHidden && !pSolid->IsVisible())
	{
		return TRUE;
	}

	int nFaces = pSolid->GetFaceCount();
	char *p;
	BOOL bSaved = FALSE;
	BOOL bMarkOnly = pInfo->bMarkOnly;
	for(int i = 0; i < nFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		char *pszFaceTex = pFace->texture.texture;

		BOOL bDoMarkSolid = FALSE;

		switch(pInfo->iAction)
		{
			case 0:	// replace exact matches only:
			{
				if(!strcmpi(pszFaceTex, pInfo->szFind))
				{
					if(bMarkOnly)
					{
						bDoMarkSolid = TRUE;
						break;
					}

					if(!bSaved)
					{
						bSaved = TRUE;
						GetHistory()->Keep(pSolid);
					}
					pFace->SetTexture(pInfo->szReplace);
					++pInfo->nReplaced;
				}
				break;
			}
			case 1:	// find partials, replace entire string:
			{
				p = FindInString(pInfo->szFind, pszFaceTex);
				if(p)
				{
					if(bMarkOnly)
					{
						bDoMarkSolid = TRUE;
						break;
					}

					if(!bSaved)
					{
						bSaved = TRUE;
						GetHistory()->Keep(pSolid);
					}
					pFace->SetTexture(pInfo->szReplace);
					++pInfo->nReplaced;
				}
				break;
			}
			case 2:	// find partials, substitute replacement:
			{
				p = FindInString(pInfo->szFind, pszFaceTex);
				if(p)
				{
					if(bMarkOnly)
					{
						bDoMarkSolid = TRUE;
						break;
					}

					if(!bSaved)
					{
						bSaved = TRUE;
						GetHistory()->Keep(pSolid);
					}
					// create a new string
					char szNewTex[128];
					strcpy(szNewTex, pszFaceTex);
					strcpy(szNewTex + int(p - pszFaceTex), pInfo->szReplace);
					strcat(szNewTex, pszFaceTex + int(p - pszFaceTex) + pInfo->iFindLen);
					pFace->SetTexture(szNewTex);
					++pInfo->nReplaced;
				}
				break;
			}
		}

		if (bDoMarkSolid)
		{
			if( g_pToolManager->GetToolID() == TOOL_FACEEDIT_MATERIAL )
			{
				pInfo->pDoc->SelectFace(pSolid, i, CMapDoc::scSelect);
			}
			else
			{
				pInfo->pDoc->SelectObject(pSolid, CMapDoc::scSelect);
			}
			++pInfo->nReplaced;
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszFind - 
//			pszReplace - 
//			bEverything - 
//			iAction - 
//			bHidden - 
//-----------------------------------------------------------------------------
void CMapDoc::ReplaceTextures(LPCTSTR pszFind, LPCTSTR pszReplace, BOOL bEverything, int iAction, BOOL bHidden)
{
	CFaceEditSheet *pSheet = GetMainWnd()->m_pFaceEditSheet;
	HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
	pSheet->EnableUpdate( false );

	// set up info struct to pass to callback
	ReplaceTexInfo_t info;
	strcpy(info.szFind, pszFind);
	strcpy(info.szReplace, pszReplace);
	info.pDoc = this;
	info.bHidden = bHidden;
	if (iAction & 0x100)
	{
		iAction &= ~0x100;
		info.bMarkOnly = TRUE;
		info.bHidden = FALSE;	// do not mark hidden objects
	}
	else
	{
		info.bMarkOnly = FALSE;
	}

	info.iAction = iAction;
	info.nReplaced = 0;
	info.iFindLen = strlen(pszFind);
	info.pWorld = m_pWorld;

	if (bEverything)
	{
		m_pWorld->EnumChildren(ENUMMAPCHILDRENPROC(ReplaceTexFunc), DWORD(&info), MAPCLASS_TYPE(CMapSolid));
	}
	else
	{
		// Selection only
		int nSelCount = Selection_GetCount();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pobj = Selection_GetObject(i);

			//
			// Call the texture replacement callback for this object (if it is a solid) and
			// all of its children.
			//
			if (pobj->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
			{
				ReplaceTexFunc((CMapSolid *)pobj, &info);
			}
			pobj->EnumChildren(ENUMMAPCHILDRENPROC(ReplaceTexFunc), DWORD(&info), MAPCLASS_TYPE(CMapSolid));
		}
	}

	CString str;
	if (!info.bMarkOnly)
	{
		str.Format("%d textures replaced.", info.nReplaced);
		if (info.nReplaced > 0)
		{
			SetModifiedFlag();
		}
	}
	else
	{
		str.Format("%d %s marked.", info.nReplaced, ( g_pToolManager->GetToolID() == TOOL_FACEEDIT_MATERIAL ) ? "faces" : "solids");
	}

	pSheet->EnableUpdate( true );
	SetCursor(hCursorOld);

	AfxMessageBox(str);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			pInfo - Pointer to the structure with info about how to do the find/replace.
// Output : 
//-----------------------------------------------------------------------------
static BOOL BatchReplaceTextureCallback( CMapClass *pObject, BatchReplaceTextures_t *pInfo )
{ 
	CMapSolid *solid;
	int numFaces, i;
	CMapFace *face;
	char szCurrentTexture[MAX_PATH];

	solid = ( CMapSolid * )pObject;
	numFaces = solid->GetFaceCount();
	for( i = 0; i < numFaces; i++ )
	{
		face = solid->GetFace( i );
		face->GetTextureName( szCurrentTexture );
		if( stricmp( szCurrentTexture, pInfo->szFindTexName ) == 0 )
		{
			face->SetTexture( pInfo->szReplaceTexName );
		}
	}
	return TRUE; // return TRUE to continue enumerating, FALSE to stop. 
}

  
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//-----------------------------------------------------------------------------
void CMapDoc::BatchReplaceTextures( FILE *fp )
{
	char *scan, *keyStart, *valStart;
	char buf[MAX_REPLACE_LINE_LENGTH];
	BatchReplaceTextures_t Info;

	while( fgets( buf, sizeof( buf ), fp ) )
	{
		scan = buf;

		// skip whitespace.
		while( *scan == ' ' || *scan == '\t' )
		{
			scan++;
		}

		// get the key.
		keyStart = scan;
		while( *scan != ' ' && *scan != '\t' )
		{
			if( *scan == '\0' || *scan == '\n' )
			{
				goto next_line;
			}
			scan++;
		}
		memcpy( Info.szFindTexName, keyStart, scan - keyStart );
		Info.szFindTexName[scan - keyStart] = '\0';

		// skip whitespace.
		while( *scan == ' ' || *scan == '\t' )
		{
			scan++;
		}

		// get the value
		valStart = scan;
		while( *scan != ' ' && *scan != '\t' && *scan != '\0' && *scan != '\n' )
		{
			scan++;
		}
		memcpy( Info.szReplaceTexName, valStart, scan - valStart );
		Info.szReplaceTexName[scan - valStart] = '\0';

		// Get rid of the file extension in val if there is one.
		char *period;
		period = Info.szReplaceTexName + strlen( Info.szReplaceTexName ) - 4;
		if( period > Info.szReplaceTexName && *period == '.' )
		{
			*period = '\0';
		}

		// Get of backslashes in both key and val.
		for( scan = Info.szFindTexName; *scan; scan++ )
		{
			if( *scan == '\\' )
			{
				*scan = '/';
			}
		}
		for( scan = Info.szReplaceTexName; *scan; scan++ )
		{
			if( *scan == '\\' )
			{
				*scan = '/';
			}
		}

		// Search and replace all key textures with val.
		m_pWorld->EnumChildren( ( ENUMMAPCHILDRENPROC )BatchReplaceTextureCallback, ( DWORD )&Info, MAPCLASS_TYPE( CMapSolid ) ); 
next_line:;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Invokes the replace textures dialog.
//-----------------------------------------------------------------------------
void CMapDoc::OnEditReplacetex(void)
{
	CReplaceTexDlg dlg(Selection_GetCount());

	dlg.m_strFind = GetDefaultTextureName();

	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Replace Textures");

	if (dlg.m_bMarkOnly)
	{
		SelectObject(NULL, scClear);	// clear selection first
	}

	ReplaceTextures(dlg.m_strFind, dlg.m_strReplace, dlg.m_iSearchAll, dlg.m_iAction | (dlg.m_bMarkOnly ? 0x100 : 0), dlg.m_bHidden);

	if (dlg.m_bMarkOnly)
	{
		// update 2d views!
		SelectObject(NULL, scUpdateDisplay);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Dnap the selected objects to the grid.
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsSnapselectedtogrid(void)
{
	if (!Selection_GetCount())
	{
		return;
	}

	// just snap the bmins of the bounding box.
	BoundBox NewObjectBox;
	m_pToolSelection->GetBounds(NewObjectBox.bmins, NewObjectBox.bmaxs);
	NewObjectBox.SnapToGrid(m_nGridSpacing);

	// calculate amount to move ..
	Vector ptMove = -(m_pToolSelection->bmins - NewObjectBox.bmins);

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Snap Objects");
	GetHistory()->Keep(Selection_GetList());

	// do move
	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);
		pObject->TransMove(ptMove);
	}

	ToolUpdateViews(CMapView2D::updEraseTool);	// remove sel rect
	UpdateBox ub;
	m_pToolSelection->GetBounds(ub.Box.bmins, ub.Box.bmaxs);
	ub.Box.UpdateBounds(&NewObjectBox);
	ub.Objects = Selection_GetList();

	// make sure selection rect bounds moved objects
	m_pToolSelection->UpdateBounds();
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : axes - 
//			nChar - 
//			bSnap - 
// FIXME: don't pass around Axes2 for nudge direction!
//-----------------------------------------------------------------------------
void CMapDoc::NudgeObjects(Axes2& axes, UINT nChar, BOOL bSnap)
{
	Vector Delta;

	if (!m_bSnapToGrid)
	{
		bSnap = FALSE;
	}

	ZeroVector(Delta);
	float fUnit = !bSnap ? 1 : m_nGridSpacing;
	if (nChar == VK_RIGHT)
	{
		Delta[axes.axHorz] = axes.bInvertHorz ? -fUnit : fUnit;
	}
	else if (nChar == VK_LEFT)
	{
		Delta[axes.axHorz] = axes.bInvertHorz ? fUnit : -fUnit;
	}
	else if (nChar == VK_DOWN)
	{
		Delta[axes.axVert] = axes.bInvertVert ? -fUnit : fUnit;
	}
	else if (nChar == VK_UP)
	{
		Delta[axes.axVert] = axes.bInvertVert ? fUnit : -fUnit;
	}

	if (g_pToolManager->GetToolID() == TOOL_POINTER)
	{
		UpdateBox ub;
		ub.Box.UpdateBounds(m_pToolSelection);

		GetHistory()->MarkUndoPosition(Selection_GetList(), "Nudge objects");
		GetHistory()->Keep(Selection_GetList());

		int nSelCount = Selection_GetCount();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = Selection_GetObject(i);
			pObject->TransMove(Delta);
		}

		m_pToolSelection->UpdateBounds();
		ub.Box.UpdateBounds(m_pToolSelection);
		ub.Objects = Selection_GetList();
		UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
	}
	else if(g_pToolManager->GetToolID() == TOOL_MORPH)
	{
		if (m_pToolMorph->GetSelectedHandleCount() == 1 && m_pToolMorph->GetSelectedType() == shtVertex)
		{
			// we have one vertex selected, so make sure
			// it's going to snap to grid.
			Vector Pos;
			m_pToolMorph->GetSelectedCenter(Pos);

			Vector NewPos = Pos + Delta;
			Snap(NewPos);	// snap it to grid

			// calculate new delta
			Delta = NewPos - Pos;
		}

		m_pToolMorph->MoveSelectedHandles(Delta);
		m_pToolMorph->FinishTranslation(TRUE);	// force checking for merges
	}

	//
	// The transformation may have changed some entity properties (such as the "origin" key),
	// so we must refresh the Object Properties dialog.
	//
	GetMainWnd()->pObjectProperties->RefreshData();

	Update3DViews();
	ToolUpdateViews(CMapView2D::updTool);
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToolsSplitface(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((g_pToolManager->GetToolID() == TOOL_MORPH) && m_pToolMorph->CanSplitFace());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsSplitface(void)
{
	if(!m_pToolMorph->SplitFace())
		return;

	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_COLOR_ONLY);
	ToolUpdateViews(CMapView2D::updTool);
}

	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
// Output : int
//-----------------------------------------------------------------------------
int CMapDoc::Snap(float x)
{
	if (!m_bSnapToGrid)
	{
		return (int)x;
	}

	return rint(x / m_nGridSpacing) * m_nGridSpacing;
}


//-----------------------------------------------------------------------------
// Purpose: Gets the default camera info for this document.
//-----------------------------------------------------------------------------
void CMapDoc::Camera_Get(Vector &vecPos, Vector &vecLookAt)
{
	if (m_pToolCamera->GetActiveCamera() == -1)
	{
		vecPos = vec3_origin;
		vecLookAt = Vector(0, 1, 0);
		return;
	}

	CAMSTRUCT camView;
	m_pToolCamera->GetCameraPos(&camView);
	vecPos = camView.position;
	vecLookAt = camView.look;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::Camera_Update(const Vector &vecPos, const Vector &vecLookAt)
{
	m_pToolCamera->Update(vecLookAt, vecPos);
}


//-----------------------------------------------------------------------------
// Purpose: Snaps a point to the grid, or to integer values if snap is disabled.
// Input  : pt - Point in world coordinates to snap.
//-----------------------------------------------------------------------------
void CMapDoc::Snap(Vector &pt, int nFlags)
{
	float flGridSpacing = m_nGridSpacing;
	
	//
	// Always start with at least a 1 unit grid.
	//
	if (!m_bSnapToGrid)
	{
		flGridSpacing = 1.0f;
	}

	if (nFlags & SNAP_HALF_GRID)
	{
		flGridSpacing *= 0.5f;
	}

	for (int i = 0; i < 3; i++)
	{
		pt[i] = rint(pt[i] / flGridSpacing) * flGridSpacing;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Snaps a point to the grid, or to integer values if snap is disabled.
// Input  : pt - Point in world coordinates to snap.
// FIXME: This can't snap to half units. Remove?
//-----------------------------------------------------------------------------
void CMapDoc::Snap(CPoint &pt)
{
	int nGridSpacing = m_nGridSpacing;

	//
	// Default to a 1 unit grid if snap is disabled.
	//
	if (!m_bSnapToGrid)
	{
		nGridSpacing = 1;
	}
	
	pt.x = (int)(rint((float)pt.x / nGridSpacing) * nGridSpacing);
	pt.y = (int)(rint((float)pt.y / nGridSpacing) * nGridSpacing);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsTransform(void)
{
	if(m_pToolSelection->IsEmpty())
	{
		AfxMessageBox("You must select some objects before you can\n"
			"transform them.");
		return;
	}

	CTransformDlg dlg;
	dlg.m_iMode = 0;

	if(dlg.DoModal() != IDOK)
		return;

	UpdateBox ub;
	ub.Box.UpdateBounds(m_pToolSelection);

	Vector Delta;
	Delta[0] = dlg.m_X;
	Delta[1] = dlg.m_Y;
	Delta[2] = dlg.m_Z;

	if (dlg.m_iMode == 1)
	{
		// make sure no 0.0 values
		for (int i = 0; i < 3; i++)
		{
			if (Delta[i] == 0.0f)
			{
				Delta[i] = 1.0f;
			}
		}
	}

	// find origin
	Vector Origin;
	ub.Box.GetBoundsCenter(Origin);

	GetHistory()->MarkUndoPosition(Selection_GetList(), "Transformation");
	GetHistory()->Keep(Selection_GetList());

	//
	// Save any properties that may have been changed in the entity properties dialog.
	// This prevents the LoadData below from losing any changes that were made in the
	// object properties dialog.
	//
	GetMainWnd()->pObjectProperties->SaveData();

	int nSelCount = Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = Selection_GetObject(i);
		switch (dlg.m_iMode)
		{
			case 0:	// rotate
			{
				pObject->TransRotate(&Origin, QAngle( Delta.x, Delta.y, Delta.z ));
				break;
			}

			case 1:	// scale
			{
				pObject->TransScale(Origin, Delta);
				break;
			}

			case 2: // move
			{
				pObject->TransMove(Delta);
				break;
			}
		}
	}

	//
	// The transformation may have changed some entity properties (such as the "angles" key),
	// so we must refresh the Object Properties dialog.
	//
	GetMainWnd()->pObjectProperties->LoadData(Selection_GetList(), false);

	m_pToolSelection->UpdateBounds();
	ub.Box.UpdateBounds(m_pToolSelection);
	ub.Objects = Selection_GetList();
	UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
	SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleDispSolidMask( void )
{
	m_bDispSolidDrawMask = !m_bDispSolidDrawMask;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleSolidMask(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck( m_bDispSolidDrawMask );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleDispDrawWalkable( void )
{
	m_bDispDrawWalkable = !m_bDispDrawWalkable;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleDispDrawWalkable( CCmdUI *pCmdUI )
{
	pCmdUI->SetCheck( m_bDispDrawWalkable );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleDispDrawBuildable( void )
{
	m_bDispDrawBuildable = !m_bDispDrawBuildable;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleDispDrawBuildable( CCmdUI *pCmdUI )
{
	pCmdUI->SetCheck( m_bDispDrawBuildable );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsToggletexlock(void)
{
	Options.SetLockingTextures(!Options.IsLockingTextures());
	SetStatusText(SBI_PROMPT, Options.IsLockingTextures() ? "Texture locking on" : "Texture locking off");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToolsToggletexlock(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(Options.IsLockingTextures());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToolsTextureAlignment(void)
{
	TextureAlignment_t eTextureAlignment;

	eTextureAlignment = Options.GetTextureAlignment();

	if (eTextureAlignment == TEXTURE_ALIGN_WORLD)
	{
		Options.SetTextureAlignment(TEXTURE_ALIGN_FACE);
		SetStatusText(SBI_PROMPT, "Face aligned textures");
	}
	else
	{
		Options.SetTextureAlignment(TEXTURE_ALIGN_WORLD);
		SetStatusText(SBI_PROMPT, "World aligned textures");
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToolsTextureAlignment(CCmdUI *pCmdUI) 
{
	pCmdUI->SetCheck(Options.GetTextureAlignment() == TEXTURE_ALIGN_FACE);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether a remote shell editing session has been initiated
//			through a "session_begin" shell command.
//-----------------------------------------------------------------------------
bool CMapDoc::IsShellSessionActive(void)
{
	return(GetMainWnd()->IsShellSessionActive());
}


//-----------------------------------------------------------------------------
// Purpose: Selects the object for morphing.
//-----------------------------------------------------------------------------
void CMapDoc::Morph_SelectObject(CMapSolid *pSolid, unsigned int uCmd)
{
	m_pToolMorph->SelectObject(pSolid, uCmd);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current bounds of the objects selected for morphing.
// Input  : mins - 
//			maxs - 
//			bReset - True to reset the bounds afterward.
//-----------------------------------------------------------------------------
void CMapDoc::Morph_GetBounds(Vector &mins, Vector &maxs, bool bReset)
{
	m_pToolMorph->GetMorphBounds(mins, maxs, bReset);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMapDoc::Morph_GetObjectCount(void)
{
	return(m_pToolMorph->GetObjectCount());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
// Output : CSSolid
//-----------------------------------------------------------------------------
CSSolid *CMapDoc::Morph_GetFirstObject(POSITION &pos)
{
	return(m_pToolMorph->GetFirstObject(pos));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSSolid *CMapDoc::Morph_GetNextObject(POSITION &pos)
{
	return(m_pToolMorph->GetNextObject(pos));
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current active state of the cordon tool.
//-----------------------------------------------------------------------------
bool CMapDoc::Cordon_IsCordoning(void)
{
	return(m_pToolCordon->IsCordonActive());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleCordon(void)
{
	m_pToolCordon->SetCordonActive(!m_pToolCordon->IsCordonActive());
	UpdateVisibilityAll();
	ToolUpdateViews(CMapView2D::updRenderAll);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleCordon(CCmdUI* pCmdUI) 
{
	if (m_pToolCordon->IsEmpty())
	{
		pCmdUI->Enable(false);
	}

	pCmdUI->SetCheck(m_pToolCordon->IsCordonActive());
}


//-----------------------------------------------------------------------------
// Purpose: Toggles between Groups selection mode and Solids selection mode.
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleGroupignore(void)
{
	SelectMode_t eSelectMode = Selection_GetMode();
	if (eSelectMode == selectSolids)
	{
		eSelectMode = selectGroups;
	}
	else
	{
		eSelectMode = selectSolids;
	}

	Selection_SetMode(eSelectMode);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleGroupignore(CCmdUI *pCmdUI) 
{
	pCmdUI->SetCheck(Selection_GetMode() == selectSolids);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnChangeVertexscale(void)
{
	m_pToolMorph->UpdateScale();
	ToolUpdateViews(CMapView2D::updTool);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnVscaleToggle(void)
{
	if (g_pToolManager->GetToolID() != TOOL_MORPH)
	{
		return;
	}

	m_pToolMorph->OnScaleCmd();
	ToolUpdateViews(CMapView2D::updTool);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapEntityreport(void) 
{
	CEntityReportDlg dlg(this);
	dlg.DoModal();
	dlg.SaveToIni();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleSelectbyhandle(void)
{
	Options.view2d.bSelectbyhandles = !Options.view2d.bSelectbyhandles;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleSelectbyhandle(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(Options.view2d.bSelectbyhandles);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleInfiniteselect() 
{
	Options.view2d.bAutoSelect = !Options.view2d.bAutoSelect;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggleInfiniteselect(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(Options.view2d.bAutoSelect);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSolid - 
//			*pInfo - 
// Output : static BOOL
//-----------------------------------------------------------------------------
static BOOL SaveDXFDisp(CMapDisp *pDisp, ExportDXFInfo_t *pInfo)
{
	char szName[128];
	sprintf(szName, "OBJECT%03d", pInfo->nObject);

	// count number of triangulated faces
	int nVertCount = pDisp->GetSize();
	int nTriFaces = pDisp->TriangleCount();

	fprintf(pInfo->fp,"0\nPOLYLINE\n8\n%s\n66\n1\n70\n64\n71\n%u\n72\n%u\n", 
		szName, nVertCount, nTriFaces);
	fprintf(pInfo->fp,"62\n50\n");

	// Write out vertices...
	int i;
	for (i = 0; i < nVertCount; i++)
	{
		Vector pos;
		pDisp->GetVert( i, pos );
		fprintf(pInfo->fp,	"0\nVERTEX\n8\n%s\n10\n%.6f\n20\n%.6f\n30\n%.6f\n70\n192\n", szName, pos[0], pos[1], pos[2]);
	}

	// triangulate each face and write
	int nWidth = pDisp->GetWidth();
	int nHeight = pDisp->GetHeight();
	for (i = 0; i < nHeight - 1; ++i)
	{
		for (int j = 0; j < nWidth - 1; ++j)
		{
			// DXF files are 1 based, not 0 based. That's what the extra 1 is for
			int idx = i * nHeight + j + 1;

			fprintf(pInfo->fp, "0\nVERTEX\n8\n%s\n10\n0\n20\n0\n30\n"
				"0\n70\n128\n71\n%d\n72\n%d\n73\n%d\n", szName,
				idx, idx + nHeight, idx + nHeight + 1 );

			fprintf(pInfo->fp, "0\nVERTEX\n8\n%s\n10\n0\n20\n0\n30\n"
				"0\n70\n128\n71\n%d\n72\n%d\n73\n%d\n", szName,
				idx, idx + nHeight + 1, idx + 1 );
		}
	}

	fprintf(pInfo->fp, "0\nSEQEND\n8\n%s\n", szName);

	return TRUE;
}

static BOOL SaveDXF(CMapSolid *pSolid, ExportDXFInfo_t *pInfo)
{
	if (pInfo->bVisOnly)
	{
		if (!pSolid->IsVisible())
		{
			return TRUE;
		}
	}

	CSSolid *pStrucSolid = new CSSolid;
	pStrucSolid->Attach(pSolid);
	pStrucSolid->Convert( true, true );
	pStrucSolid->SerializeDXF(pInfo->fp, pInfo->nObject++);
	delete pStrucSolid;

	// Serialize displacements
	for (int i = 0; i < pSolid->GetFaceCount(); ++i)
	{
		CMapFace *pMapFace = pSolid->GetFace( i );
		if (pMapFace->HasDisp())
		{
			EditDispHandle_t hDisp = pMapFace->GetDisp();
			CMapDisp *pDisp = EditDispMgr()->GetDisp( hDisp );
			if (!SaveDXFDisp( pDisp, pInfo ))
				return FALSE;
		}
	}

	return TRUE;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnFileExporttodxf(void)
{
	static CString str;

	if (str.IsEmpty())
	{
		int nDot;

		// Replace the extension with DXF.
		str = GetPathName();
		if ((nDot = str.ReverseFind('.')) != -1)
		{
			str = str.Left(nDot);
		}
		str += ".dxf";
	}

	CExportDlg dlg(str, "dxf", "DXF files (*.dxf)|*.dxf||");
	if(dlg.DoModal() == IDCANCEL)
		return;

	str = dlg.GetPathName();
	if(str.ReverseFind('.') == -1)
		str += ".dxf";
	
	FILE *fp = fopen(str, "wb");

	m_pWorld->CalcBounds(TRUE);

	BoundBox box;
	m_pWorld->GetRender2DBox(box.bmins, box.bmaxs);

	fprintf(fp,"0\nSECTION\n2\nHEADER\n");
	fprintf(fp,"9\n$ACADVER\n1\nAC1008\n");
	fprintf(fp,"9\n$UCSORG\n10\n0.0\n20\n0.0\n30\n0.0\n");
	fprintf(fp,"9\n$UCSXDIR\n10\n1.0\n20\n0.0\n30\n0.0\n");
	fprintf(fp,"9\n$TILEMODE\n70\n1\n");
	fprintf(fp,"9\n$UCSYDIR\n10\n0.0\n20\n1.0\n30\n0.0\n");
	fprintf(fp,"9\n$EXTMIN\n10\n%f\n20\n%f\n30\n%f\n",
		box.bmins[0], box.bmins[1], box.bmins[2]);
	fprintf(fp,"9\n$EXTMAX\n10\n%f\n20\n%f\n30\n%f\n",
		box.bmaxs[0], box.bmaxs[1], box.bmaxs[2]);
	fprintf(fp,"0\nENDSEC\n");
	
	/* Tables section */
	fprintf(fp,"0\nSECTION\n2\nTABLES\n");
	/* Continuous line type */
	fprintf(fp,"0\nTABLE\n2\nLTYPE\n70\n1\n0\nLTYPE\n2\nCONTINUOUS"
		"\n70\n64\n3\nSolid line\n72\n65\n73\n0\n40\n0.0\n");
	fprintf(fp,"0\nENDTAB\n");
	
	/* Object names for layers */
	fprintf(fp,"0\nTABLE\n2\nLAYER\n70\n%d\n",1);
	fprintf(fp,"0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n");
	fprintf(fp,"0\nENDTAB\n");
	fprintf(fp,"0\nTABLE\n2\nSTYLE\n70\n1\n0\nSTYLE\n2\nSTANDARD\n70\n0\n"
		"40\n0.0\n41\n1.0\n50\n0.0\n71\n0\n42\n0.2\n3\ntxt\n4\n\n0\nENDTAB\n");
	
	/* Default View? */
	
	/* UCS */ 
	fprintf(fp,"0\nTABLE\n2\nUCS\n70\n0\n0\nENDTAB\n");
	fprintf(fp,"0\nENDSEC\n");
	
	/* Entities section */
	fprintf(fp,"0\nSECTION\n2\nENTITIES\n");

	// export solids
	BeginWaitCursor();

	ExportDXFInfo_t info;
	info.bVisOnly = dlg.bVisibles;
	info.nObject = 0;
	info.pWorld = m_pWorld;
	info.fp = fp;

	m_pWorld->EnumChildren(ENUMMAPCHILDRENPROC(SaveDXF), DWORD(&info), MAPCLASS_TYPE(CMapSolid));

	EndWaitCursor();

	fprintf(fp,"0\nENDSEC\n0\nEOF\n");
	fclose(fp);
}


//-----------------------------------------------------------------------------
// Purpose: Toggles the 3D grid.
//-----------------------------------------------------------------------------
void CMapDoc::OnToggle3DGrid(void)
{
	m_bShow3DGrid = !m_bShow3DGrid;
	UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_UPDATE_3D);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the check state of the 3D grid toggle button.
// Input  : *pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateToggle3DGrid(CCmdUI *pCmdUI) 
{
	pCmdUI->SetCheck(m_bShow3DGrid);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnViewHidepaths(void)
{
	m_bHidePaths = !m_bHidePaths;
	if(g_pToolManager->GetToolID() != TOOL_PATH)
		UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapDoc::OnUpdateViewHidepaths(CCmdUI *pCmdUI) 
{
	pCmdUI->SetCheck(m_bHidePaths);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapLoadpointfile(void)
{
	if(m_strLastPointFile.IsEmpty())
	{
		m_strLastPointFile = GetPathName();
		char *p = m_strLastPointFile.GetBuffer(MAX_PATH);
		p = strrchr(p, '.');
		if (p)
		{
			if ((m_pGame->mapformat == mfHalfLife2) || (m_pGame->mapformat == mfTeamFortress2))
			{
				strcpy(p, ".lin");
			}
			else
			{
				strcpy(p, ".pts");
			}
		}
		m_strLastPointFile.ReleaseBuffer();
	}

	CString str;
	str.Format("Load default pointfile?\n(%s)", m_strLastPointFile);
	if(GetFileAttributes(m_strLastPointFile) == 0xFFFFFFFF ||
		AfxMessageBox(str, MB_ICONQUESTION | MB_YESNO) == IDNO)
	{
		CFileDialog dlg(TRUE, ".pts", m_strLastPointFile, OFN_HIDEREADONLY | 
			OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR, 
			"Pointfiles (*.pts;*.lin)|*.pts; *.lin||");
		
		if(dlg.DoModal() != IDOK)
			return;

		m_strLastPointFile = dlg.GetPathName();	
	}

	// load the file
	if(GetFileAttributes(m_strLastPointFile) == 0xFFFFFFFF)
	{
		AfxMessageBox("Couldn't load pointfile.");
		return;
	}
	
	ifstream file(m_strLastPointFile);
	char szLine[256];
	int nLines = 0;

	while(!file.eof())
	{
		file.getline(szLine, 256);
		++nLines;
	}

	if(m_nPFPoints)
	{
		delete[] m_pPFPoints;
		m_nPFPoints = 0;
		m_pPFPoints = NULL;
	}

	// load 'em up
	file.close();
	file.open(m_strLastPointFile);

	m_nPFPoints = nLines;
	m_pPFPoints = new Vector[m_nPFPoints];

	if(!m_pPFPoints)
		return;

	m_iCurPFPoint = -1;

	for(int i = 0; i < m_nPFPoints; i++)
	{
		file.getline(szLine, 256);
		if(sscanf(szLine, "%f %f %f", &m_pPFPoints[i][0], &m_pPFPoints[i][1],
			&m_pPFPoints[i][2]) != 3)
		{
			m_nPFPoints = i;
			break;
		}
	}

	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnMapUnloadpointfile(void)
{
	if(!m_nPFPoints)
		return;

	delete[] m_pPFPoints;
	m_nPFPoints = 0;
	m_pPFPoints = NULL;
	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDirection - 
//-----------------------------------------------------------------------------
void CMapDoc::GotoPFPoint(int iDirection)
{
	if(m_nPFPoints == 0)
		return;

	m_iCurPFPoint = m_iCurPFPoint + iDirection;

	if(m_iCurPFPoint == m_nPFPoints)
		m_iCurPFPoint = 0;
	else if(m_iCurPFPoint < 0)
		m_iCurPFPoint = m_nPFPoints-1;

	VIEW2DINFO vi;
	vi.wFlags = VI_CENTER;
	vi.ptCenter = m_pPFPoints[m_iCurPFPoint];

	SetView2dInfo(vi);
}


//-----------------------------------------------------------------------------
// Purpose: Adds an object to the world. This is the ONLY correct way to add an
//			object to the world. Calling directly through AddChild skips a bunch
//			of necessary bookkeeping.
// Input  : pObject - object being added to the world.
//-----------------------------------------------------------------------------
void CMapDoc::AddObjectToWorld(CMapClass *pObject, CMapClass *pParent)
{
	ASSERT(pObject != NULL);

	if (pObject != NULL)
	{
		m_pWorld->AddObjectToWorld(pObject, pParent);

		//
		// Give the renderer a chance to precache data. 
		//
		RenderPreloadObject(pObject);

		//
		// Update the object's visibility. This will recurse into solid children of entities as well.
		//
		UpdateVisibility(pObject);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes an object from the world object tree.
// Input  : pObject - object being removed from the world.
//-----------------------------------------------------------------------------
void CMapDoc::RemoveObjectFromWorld(CMapClass *pObject, bool bRemoveChildren)
{
	ASSERT(pObject != NULL);

	if (pObject != NULL)
	{
		m_pWorld->RemoveObjectFromWorld(pObject, bRemoveChildren);

		//
		// Clean up any visgroups with no members.
		//
		VisGroups_PurgeGroups();

		//
		// Make sure the object is not currently selected for editing.
		//
		GetMainWnd()->pObjectProperties->DontEdit(pObject);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//-----------------------------------------------------------------------------
void CMapDoc::RenderPreloadObject(CMapClass *pObject)
{
	POSITION p = GetFirstViewPosition();

	while (p)
	{
		CView *pView = GetNextView(p);
		if (pView->IsKindOf(RUNTIME_CLASS(CMapView3D)))
		{
			CMapView3D *pView3D = (CMapView3D *)pView;
			pView3D->RenderPreloadObject(pObject);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszFileName - 
//-----------------------------------------------------------------------------
bool CMapDoc::SaveVMF(const char *pszFileName, int saveFlags)
{
	CChunkFile File;

	ChunkFileResult_t eResult = File.Open(pszFileName, ChunkFile_Write);

	if (eResult == ChunkFile_Ok)
	{
		CSaveInfo SaveInfo;
		CMapObjectList CordonList;
		CMapWorld *pCordonWorld = NULL;

		if (!m_bPrefab && !(saveFlags & SAVEFLAGS_LIGHTSONLY))
		{
			SaveInfo.SetVisiblesOnly(bSaveVisiblesOnly == TRUE);

			//
			// Add cordon objects.
			//
			if (Cordon_IsCordoning())
			{
				//
				// Create "cordon world", add its objects to our real world, create a list in
				// CordonList so we can remove them again.
				//
				pCordonWorld = m_pToolCordon->CreateTempWorld();
				POSITION pos;
				CMapClass *pobj = pCordonWorld->GetFirstChild(pos);
				while (pobj != NULL)
				{
					pobj->SetTemporary(TRUE);
					m_pWorld->AddObjectToWorld(pobj);
					CordonList.AddTail(pobj);

					pobj = pCordonWorld->GetNextChild(pos);
				}

				//
				// dvs: HACK (not mine) - make the cordon bounds bigger so that the cordon brushes
				//		overlap the cordon bounds during serialization.
				//
				BoundBox CordonBox;
				m_pToolCordon->GetBounds(CordonBox.bmins, CordonBox.bmaxs);
 				for (int i = 0; i < 3; i++)
				{
					CordonBox.bmins[i] -= 1.f;
					CordonBox.bmaxs[i] += 1.f;
				}
			}
		}

		//
		// Write the map file version.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = SaveVersionInfoVMF(&File);
		}

		//
		// Save VisGroups information. Save this first so that we can assign visgroups while loading objects.
		//
		if (!m_bPrefab && !(saveFlags & SAVEFLAGS_LIGHTSONLY))
		{
			ChunkFileResult_t eResult = VisGroups_SaveVMF(&File, &SaveInfo);
		}

		//
		// Save view related settings (grid setting, splitter proportions, etc)
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = SaveViewSettingsVMF(&File, &SaveInfo);
		}


		//
		// Save the world.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = m_pWorld->SaveVMF(&File, &SaveInfo, saveFlags & SAVEFLAGS_LIGHTSONLY);
		}

		if (!m_bPrefab && !(saveFlags & SAVEFLAGS_LIGHTSONLY))
		{
			//
			// Remove cordon objects from the real world.
			//
			if (Cordon_IsCordoning())
			{
				POSITION pos = CordonList.GetHeadPosition();
				while (pos != NULL)
				{
					CMapClass *pobj = CordonList.GetNext(pos);
					m_pWorld->RemoveObjectFromWorld(pobj, true);
				}

				//
				// The cordon objects will be deleted in the cordon world's destructor.
				//
				delete pCordonWorld;
			}

			//
			// Save camera information.
			//
			if (eResult == ChunkFile_Ok)
			{
				eResult = m_pToolCamera->SaveVMF(&File, &SaveInfo);
			}

			//
			// Save cordon information.
			//
			if (eResult == ChunkFile_Ok)
			{
				eResult = m_pToolCordon->SaveVMF(&File, &SaveInfo);
			}
		}

		File.Close();
	}

	if (eResult != ChunkFile_Ok)
	{
		GetMainWnd()->MessageBox(File.GetErrorText(eResult), "Error Saving File", MB_OK);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Saves the version information chunk.
// Input  : *pFile - 
// Output : Returns ChunkFile_Ok on success, an error code on failure.
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::SaveVersionInfoVMF(CChunkFile *pFile)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("versioninfo");

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("editorversion", 400);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("editorbuild", build_number());
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("mapversion", GetDocVersion());
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueBool("prefab", m_bPrefab);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Call this function after you have made any modifications to the document.
//			By calling this function consistently, you ensure that the framework 
//			prompts the user to save changes before closing a document.
// Input  : bModified - TRUE to mark the doc as modified, FALSE to mark it as clean.
//-----------------------------------------------------------------------------
void CMapDoc::SetModifiedFlag(BOOL bModified)
{
	//
	// Increment internal version number when the doc changes from clean to dirty.
	// CShell::BeginSession checks this version number, enabling us to detect
	// out-of-sync problems when editing the map in the engine.
	//
	if ((bModified) && !IsModified())
	{
		m_nDocVersion++;
	}

	CDocument::SetModifiedFlag(bModified);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CMapDoc::SetUndoActive(bool bActive)
{
	m_pUndo->SetActive(bActive);
	m_pRedo->SetActive(bActive);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the hidden/shown state of the given object based on the current
//			settings including selection mode, visgroups, and cordon bounds.
// Input  : pObject - 
//-----------------------------------------------------------------------------
bool CMapDoc::ShouldObjectBeVisible(CMapClass *pObject)
{
	//
	// If hide entities is enabled and the object is an entity, hide the object.
	//
	if (m_bHideItems)
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
		if ((pEntity != NULL) && (pEntity->IsPlaceholder()))
		{
			return(false);
		}
	}

	//
	// If the object hides as clutter and clutter is currently hidden, hide it.
	//
	if (pObject->IsClutter() && (m_pToolSelection->GetHandleMode() == HandleMode_SelectionOnly))
	{
		return false;
	}

	//
	// If the object is a member of a visgroup and the visgroup is hidden, hide
	// the object.
	//
	CVisGroup *pGroup = pObject->GetVisGroup();
	if (pGroup)
	{
		if (!pGroup->IsVisible())
		{
			return(false);
		}
	}

	//
	// If the cordon tool is active and the object is not within the cordon bounds,
	// hide the object. The exception to this is some helpers, which only hide if their
	// parent entity is culled by the cordon.
	//
	if (Cordon_IsCordoning() && pObject->IsCulledByCordon())
	{
		Vector Mins;
		Vector Maxs;
		m_pToolCordon->GetBounds(Mins, Maxs);

		if (!pObject->IsIntersectingBox(Mins, Maxs))
		{
			return(false);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Notifies the document when an object has changed. The object is
//			added to a list which will be processed before the next view is rendered.
// Input  : pObject - Object that has changed.
//-----------------------------------------------------------------------------
void CMapDoc::UpdateObject(CMapClass *pObject)
{
	ASSERT(!pObject->IsTemporary());

	if (!m_UpdateList.Find(pObject))
	{
		m_UpdateList.AddTail(pObject);
	}

	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_OBJECTS);
}


//-----------------------------------------------------------------------------
// Purpose: Processes any objects that have changed since the last call.
//			Updates each object's visiblity and makes sure that there are no
//			invisible objects in the selection set.
//
//			This must be called from the outer loop, as selection.RemoveInvisibles
//			may change the contents of the selection. Therefore, this should never
//			be called from low-level code that might be inside an iteration of the
//			selection.
//-----------------------------------------------------------------------------
void CMapDoc::Update(void)
{
	if (!m_UpdateList.IsEmpty())
	{
		//
		// Process every object in the update list.
		//
		POSITION pos = m_UpdateList.GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = m_UpdateList.GetNext(pos);
			UpdateVisibility(pObject);
		}

		//
		// Make sure there aren't any invisible objects in the selection.
		//
		m_pToolSelection->RemoveInvisibles();

		//
		// Empty the update list now that it has been processed.
		//
		m_UpdateList.RemoveAll();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Callback for updating the visibility status of a single object and
//			its children.
// Input  : pObject - 
//			pDoc - 
//-----------------------------------------------------------------------------
BOOL CMapDoc::UpdateVisibilityCallback(CMapClass *pObject, CMapDoc *pDoc)
{
	ASSERT(!pObject->IsTemporary());

	bool bVisible = pDoc->ShouldObjectBeVisible(pObject);
	pObject->SetVisible(bVisible);
	if (bVisible)
	{
		//
		// If this is an entity and it is visible, recurse into any children.
		//
		if ((dynamic_cast<CMapEntity *>(pObject)) != NULL)
		{
			pObject->EnumChildren((ENUMMAPCHILDRENPROC)UpdateVisibilityCallback, (DWORD)pDoc);
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Updates the visibility of the given object and its children.
// Input  : pObject - 
//-----------------------------------------------------------------------------
void CMapDoc::UpdateVisibility(CMapClass *pObject)
{
	UpdateVisibilityCallback(pObject, this);
	if (pObject->IsGroup())
	{
		pObject->EnumChildrenRecurseGroupsOnly((ENUMMAPCHILDRENPROC)UpdateVisibilityCallback, (DWORD)this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates the visibility of all objects in the world.
//-----------------------------------------------------------------------------
void CMapDoc::UpdateVisibilityAll(void)
{
	//
	// Two stage recursion: first we recurse groups only, then from the callback we recurse
	// solid children of entities.
	//
	m_pWorld->EnumChildrenRecurseGroupsOnly((ENUMMAPCHILDRENPROC)UpdateVisibilityCallback, (DWORD)this);
	m_pToolSelection->RemoveInvisibles();
	UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_OBJECTS);
}


//-----------------------------------------------------------------------------
// Purpose: Adds a visgroup to this document.
// Input  : pszName - The name to assign the visgroup.
// Output : Returns a pointer to the newly created visgroup.
//-----------------------------------------------------------------------------
CVisGroup *CMapDoc::VisGroups_AddGroup(LPCTSTR pszName)
{
	CVisGroup *pGroup = new CVisGroup;
	pGroup->SetName(pszName);

	//
	// Generate a random color for the group.
	//
	pGroup->SetColor(80 + (random() % 176), 80 + (random() % 176), 80 + (random() % 176));
	
	//
	// Generate a unique id for this visgroup.
	//
	int id = 0;
	while (id++ < 2000)
	{
		CVisGroup *ptmp;
		if (!m_VisGroups.Lookup(id, ptmp))
		{
			break;
		}
	}

	pGroup->SetID(id);

	m_VisGroups.SetAt(id, pGroup);

	return pGroup;
}


//-----------------------------------------------------------------------------
// Purpose: Adds a visgroup to this document.
// Input  : pGroup - Visgroup to add.
// Output : Returns a pointer to the given visgroup.
//-----------------------------------------------------------------------------
CVisGroup *CMapDoc::VisGroups_AddGroup(CVisGroup *pGroup)
{
	m_VisGroups.SetAt(pGroup->GetID(), pGroup);
	return(pGroup);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			pGroup - 
// Output : Returns FALSE to stop enumerating if the object belonged to the given
//			visgroup. Otherwise, returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
BOOL CMapDoc::VisGroups_CheckForGroupCallback(CMapClass *pObject, CVisGroup *pGroup)
{
	if (pObject->GetVisGroup() == pGroup)
	{
		return(FALSE);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the number of visgroups in this document.
//-----------------------------------------------------------------------------
int CMapDoc::VisGroups_GetCount(void)
{
	return(m_VisGroups.GetCount());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the iterator for the VisGroups list.
//-----------------------------------------------------------------------------
POSITION CMapDoc::VisGroups_GetHeadPosition(void)
{
	return(m_VisGroups.GetStartPosition());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the iterator for the VisGroups list.
//-----------------------------------------------------------------------------
CVisGroup *CMapDoc::VisGroups_GetNext(POSITION &pos)
{
	CVisGroup *pGroup;
	unsigned short uID;
	m_VisGroups.GetNextAssoc(pos, uID, pGroup);
	return(pGroup);
}


//-----------------------------------------------------------------------------
// Purpose: Deletes any visgroups that no longer have any members.
//-----------------------------------------------------------------------------
void CMapDoc::VisGroups_PurgeGroups(void)
{
	bool bUpdate = false;

	POSITION pos = m_VisGroups.GetStartPosition();
	while (pos != NULL)
	{
		unsigned short id;
		CVisGroup *pGroup;

		m_VisGroups.GetNextAssoc(pos, id, pGroup);

		bool bKill = true;
		EnumChildrenPos_t pos2;
		CMapClass *pObject = m_pWorld->GetFirstDescendent(pos2);
		while (pObject != NULL)
		{
			if (pObject->GetVisGroup() == pGroup)
			{
				bKill = false;
				break;
			}

			pObject = m_pWorld->GetNextDescendent(pos2);
		}
		
		if (bKill)
		{
			bUpdate = true;
			m_VisGroups.RemoveKey(id);
		}
	}

	if (bUpdate)
	{
		GetMainWnd()->m_FilterControl.UpdateGroupList();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//-----------------------------------------------------------------------------
void CMapDoc::VisGroups_RemoveID(DWORD id)
{
	m_VisGroups.RemoveKey(id);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::VisGroups_SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	if (VisGroups_GetCount() == 0)
	{
		return(ChunkFile_Ok);
	}

	ChunkFileResult_t eResult = pFile->BeginChunk("visgroups");
	
	if (eResult == ChunkFile_Ok)
	{
		POSITION pos = VisGroups_GetHeadPosition();
		while (pos != NULL)
		{
			CVisGroup *pVisGroup = VisGroups_GetNext(pos);
			if (pVisGroup != NULL)
			{
				eResult = pVisGroup->SaveVMF(pFile, pSaveInfo);

				if (eResult != ChunkFile_Ok)
				{
					break;
				}
			}
		}
	}
			
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDoc::SaveViewSettingsVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("viewsettings");

	eResult = pFile->WriteKeyValueBool("bSnapToGrid", m_bSnapToGrid);
	if (eResult != ChunkFile_Ok)
		return eResult;

	eResult = pFile->WriteKeyValueBool("bShowGrid", m_bShowGrid);
	if (eResult != ChunkFile_Ok)
		return eResult;

	eResult = pFile->WriteKeyValueInt("nGridSpacing", m_nGridSpacing);
	if (eResult != ChunkFile_Ok)
		return eResult;

	eResult = pFile->WriteKeyValueBool("bShow3DGrid", m_bShow3DGrid);
	if (eResult != ChunkFile_Ok)
		return eResult;

	return(pFile->EndChunk());
}


//-----------------------------------------------------------------------------
// Purpose: Turns on lighting preview mode by loading the BSP file with the
//			same named as the VMF being edited.
//-----------------------------------------------------------------------------
void CMapDoc::InternalEnableLightPreview( bool bCustomFilename )
{
	OnDisableLightPreview();


	m_pBSPLighting = CreateBSPLighting();

	
	// Either use the VMF filename or the last-exported VMF name.
	CString strFile;
	if( m_strLastExportFileName.GetLength() == 0 )
		strFile = GetPathName();
	else
		strFile = m_strLastExportFileName;

	// Convert the extension to .bsp
	char *p = strFile.GetBuffer(MAX_PATH);
	char *ext = strrchr(p, '.');
	if( ext )
	{
		strcpy( ext, ".bsp" );
	}


	// Strip out the directory.
	char *cur = p;
	while( (cur = strstr(cur, "/")) )
		*cur = '\\';

	char fileName[MAX_PATH];

	char *pLastSlash = p;
	char *pTest;
	while( (pTest = strstr(pLastSlash, "\\")) )
		pLastSlash = pTest + 1;
	
	if( pLastSlash )
		strcpy( fileName, pLastSlash );
	else
		strcpy( fileName, p );

	strFile.ReleaseBuffer();


	// Use <mod directory> + "/maps/" + <filename>
	char fullPath[MAX_PATH*2];
	sprintf( fullPath, "%s\\maps\\%s", g_pGameConfig->m_szModDir, fileName );

	
	// Only do the dialog if they said to or if the default BSP file doesn't exist.
	if( !bCustomFilename )
	{
		FILE *fp = fopen( fullPath, "rb" );
		if( fp )
			fclose( fp );
		else
			bCustomFilename = true;
	}


	CString finalPath;
	if( bCustomFilename )
	{
		CFileDialog dlg(
			TRUE,		// bOpenFile
			"bsp",		// default extension
			fullPath,	// default filename,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,	// flags
			"BSP Files (*.bsp)|*.bsp|All Files (*.*)|*.*||",
			NULL 		// filter
			);

		if( dlg.DoModal() != IDOK )
			return;
	
		finalPath = dlg.GetPathName();
	}
	else
	{
		finalPath = fullPath;
	}

	if( !m_pBSPLighting->Load( finalPath ) )
	{
		char str[256];
		_snprintf( str, sizeof(str), "Can't load lighting from '%s'.", finalPath );
		AfxMessageBox( str );
	}


	// Switch the first mapview we find into 3D lighting preview.
	POSITION viewPos = GetFirstViewPosition();
	while( viewPos )
	{
		CView *pView = GetNextView( viewPos );
		if (pView->IsKindOf(RUNTIME_CLASS(CMapView3D)))
		{
			CMapView3D *pView3D = (CMapView3D *)pView;
			pView3D->SetDrawType( VIEW3D_LIGHTING_PREVIEW );
			break;
		}
	}


	APP()->SetForceRenderNextFrame();
}


void CMapDoc::OnEnableLightPreview()
{
	InternalEnableLightPreview( false );
}


void CMapDoc::OnEnableLightPreviewCustomFilename()
{
	InternalEnableLightPreview( true );
}


//-----------------------------------------------------------------------------
// Purpose: Turns off lighting preview mode.
//-----------------------------------------------------------------------------
void CMapDoc::OnDisableLightPreview()
{
	// Change any light preview views back to regular 3D.
	POSITION p = GetFirstViewPosition();
	while (p)
	{
		CView *pView = GetNextView(p);
		if (pView->IsKindOf(RUNTIME_CLASS(CMapView3D)))
		{
			CMapView3D *pView3D = (CMapView3D *)pView;

			if( pView3D->GetDrawType() == VIEW3D_LIGHTING_PREVIEW )
				pView3D->SetDrawType( VIEW3D_TEXTURED );
		}
	}

	if( m_pBSPLighting )
	{
		m_pBSPLighting->Release();
		m_pBSPLighting = 0;
	}
}


void CMapDoc::OnUpdateLightPreview()
{
	if( !m_pBSPLighting )
		return;

	// Save out a file with just the ents.
	char szFile[MAX_PATH];
	strcpy(szFile, GetPathName());
	szFile[strlen(szFile) - 1] = 'e';
	
	if( !SaveVMF( szFile, SAVEFLAGS_LIGHTSONLY ) )
	{
		CString str;
		str.FormatMessage( IDS_CANT_SAVE_ENTS_FILE, szFile );
		return;
	}

	// Get it in memory.
	CUtlVector<char> fileData;
	FILE *fp = fopen( szFile, "rb" );
	if( !fp )
	{
		CString str;
		str.FormatMessage( IDS_CANT_OPEN_ENTS_FILE, szFile );
		AfxMessageBox( str, MB_OK );
		return;
	}

	fseek( fp, 0, SEEK_END );
	fileData.SetSize( ftell( fp ) + 1 );
	fseek( fp, 0, SEEK_SET );
	fread( fileData.Base(), 1, fileData.Count(), fp );
	fclose( fp );

	// Null-terminate it.
	fileData[ fileData.Count() - 1 ] = 0;

	// Tell the incremental lighting manager to relight.
	m_pBSPLighting->StartLighting( fileData.Base() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::OnToggleLightPreview()
{
	if( m_pBSPLighting )
	{
		POSITION p = GetFirstViewPosition();
		while (p)
		{
			CView *pView = GetNextView(p);
			if (pView->IsKindOf(RUNTIME_CLASS(CMapView3D)))
			{
				CMapView3D *pView3D = (CMapView3D *)pView;

				if( pView3D->GetDrawType() == VIEW3D_LIGHTING_PREVIEW )
					pView3D->SetDrawType( VIEW3D_TEXTURED );
				else if( pView3D->GetDrawType() == VIEW3D_TEXTURED )
					pView3D->SetDrawType( VIEW3D_LIGHTING_PREVIEW );
			}
		}
	}
	else
	{
		// If no lighting is loaded, then load it.
		OnEnableLightPreview();
	}
}


void CMapDoc::OnAbortLightCalculation()
{
	if( !m_pBSPLighting )
		return;

	m_pBSPLighting->Interrupt();
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given object is in the selection list, false otherwise.
//-----------------------------------------------------------------------------
bool CMapDoc::Selection_IsSelected( CMapClass *pObj )
{
	return (bool)m_pToolSelection->IsSelected(pObj);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::Selection_GetBoundsCenter(Vector &vecCenter)
{
	m_pToolSelection->GetBoundsCenter(vecCenter);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::Selection_RemoveAll(void)
{
	m_pToolSelection->RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::Selection_UpdateBounds(void) // FIXME: Remove if possible
{
	m_pToolSelection->UpdateBounds();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current selection mode, which determines which objects
//			are selected when the user clicks on things.
//-----------------------------------------------------------------------------
SelectMode_t CMapDoc::Selection_GetMode(void)
{
	return m_pToolSelection->GetSelectMode();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the current selection mode, which determines which objects
//			are selected when the user clicks on things.
//-----------------------------------------------------------------------------
void CMapDoc::Selection_SetMode(SelectMode_t eNewSelectMode)
{
	SelectMode_t eOldSelectMode = m_pToolSelection->GetSelectMode();
	m_pToolSelection->SetSelectMode(eNewSelectMode);

	// dvsFIXME: move all this code into the selection tool
	if ((eOldSelectMode == selectSolids) ||
		((eOldSelectMode == selectObjects) && (eNewSelectMode == selectGroups)))
	{
		//
		// If we are going from a more specific selection mode to a less specific one,
		// clear the selection. This avoids unexpectedly selecting new things.
		//
		SelectObject(NULL, scClear | scUpdateDisplay);
	}
	else
	{
		//
		// Put all the children of the selected objects in a list, along with their children.
		//
		CMapObjectList NewList;
		int nSelCount = Selection_GetCount();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = Selection_GetObject(i);
			AddLeavesToListCallback(pObject, &NewList);
			pObject->EnumChildren((ENUMMAPCHILDRENPROC)AddLeavesToListCallback, (DWORD)&NewList);
		}
		SelectObject(NULL, scClear);

		//
		// Add child objects to selection.
		//
		POSITION p = NewList.GetHeadPosition();
		while (p)
		{
			CMapClass *pObject = NewList.GetNext(p);
			CMapClass *pSelObject = pObject->PrepareSelection(eNewSelectMode);
			if (pSelObject)
			{
				SelectObject(pSelObject, scSelect);
			}
		}

		//
		// Update the display.
		//
		SelectObject(NULL, scUpdateDisplay);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CMapDoc::Selection_Remove(CMapClass *pObject)
{
	m_pToolSelection->Remove(pObject);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the number of objects currently selected.
//-----------------------------------------------------------------------------
int CMapDoc::Selection_GetCount(void)
{
	return m_pToolSelection->GetCount();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
CMapClass *CMapDoc::Selection_GetObject(int nIndex)
{
	return m_pToolSelection->GetObject(nIndex);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapObjectList *CMapDoc::Selection_GetList(void)
{
	return m_pToolSelection->GetList();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::Selection_GetBounds(Vector &vecMins, Vector &vecMaxs)
{
	m_pToolSelection->GetBounds(vecMins, vecMaxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapDoc::Selection_GetLastValidBounds(Vector &vecMins, Vector &vecMaxs)
{
	m_pToolSelection->GetLastValidBounds(vecMins, vecMaxs);
}
