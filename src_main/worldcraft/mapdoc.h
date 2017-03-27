//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef MAPDOC_H
#define MAPDOC_H
#ifdef _WIN32
#pragma once
#endif

#include "MapClass.h"
#include "MapEntity.h"
#include "GameData.h"


class Axes2;
class Selection3D;
class CToolBlock;
class Marker3D;
class Morph3D;
class Path3D;
class Clipper3D;
class CToolOverlay;
class CToolMagnify;
class CToolMaterial;
class Cordon3D;
class Camera3D;
class CMapDoc;
class CGameConfig;
class CHistory;
class CMapGroup;
class CMapView;
class CMapView3D;
class CMapView2D;
class IBSPLighting;


struct FindEntity_t;
struct FindGroup_t;
struct FindObjectNumber_t;
struct AddNonSelectedInfo_t;


enum MAPFORMAT;
enum ToolID_t;


enum
{
	MAPVIEW_UPDATE_DISPLAY		= 0x01,	
	MAPVIEW_UPDATE_OBJECTS		= 0x02,
	MAPVIEW_COLOR_ONLY			= 0x04,		// only update if view uses solid colors
	MAPVIEW_UPDATE_SELECTION	= 0x08,
	MAPVIEW_UPDATE_2D			= 0x10,
	MAPVIEW_UPDATE_3D			= 0x20,
	MAPVIEW_OPTIONS_CHANGED		= 0x40
};


//
// Flags for snap.
//
enum
{
	SNAP_HALF_GRID = 0x01,		// Snap on the half grid instead of on the grid.
};


// Flags for view2dinfo.wflags
enum
{
	VI_ZOOM = 0x01,
	VI_CENTER = 0x02
};


typedef struct
{
	WORD wFlags;
	float fZoom;
	Vector ptCenter;
} VIEW2DINFO;


//
// To pass as hint to UpdateAllViews.
//
class UpdateBox : public CObject
{
	public:

		UpdateBox(void) { Objects = NULL; }

		CMapObjectList *Objects;
		BoundBox Box;
};


class CMapDoc : public CDocument
{
	protected:

		CMapDoc(void);
		virtual ~CMapDoc();

		DECLARE_DYNCREATE(CMapDoc)

	public:

		// attribs:
		bool m_bHidePaths;
		bool m_bSnapToGrid;
		bool m_bShowGrid;

		// pointfile stuff:
		enum
		{
			PFPNext = 1,
			PFPPrev = -1
		};

		int m_nPFPoints;
		int m_iCurPFPoint;
		Vector *m_pPFPoints;
		CString m_strLastPointFile;

		static inline CMapDoc *GetActiveMapDoc(void);
		static void SetActiveMapDoc(CMapDoc *pDoc);

	private:

		static CMapDoc *m_pMapDoc;

	public:

		IBSPLighting *GetBSPLighting()		{ return m_pBSPLighting; }

		void BeginShellSession(void);
		void EndShellSession(void);
		bool IsShellSessionActive(void);

		inline int GetDocVersion(void);
		inline void DecrementDocVersion(void);

		//
		// Interface to the map document. Functions for object creation, deletion, etc.
		//
		CMapEntity *CreateEntity(const char *pszClassName, float x, float y, float z);
		bool DeleteEntity(const char *pszClassName, float x, float y, float z);
		CMapEntity *FindEntity(const char *pszClassName, float x, float y, float z);
		bool FindEntitiesByKeyValue(CMapEntityList *pFound, const char *szKey, const char *szValue);
		bool FindEntitiesByClassName(CMapEntityList *pFound, const char *szClassName);

		void Update(void);
		void SetModifiedFlag(BOOL bModified = TRUE);

		void SetAnimationTime( float time );
		float GetAnimationTime( void ) { return m_flAnimationTime; }
		bool IsAnimating( void ) { return m_bIsAnimating; }

		// other stuff
		float m_flCurrentTime;
		void UpdateCurrentTime( void );
		float GetTime( void ) { return m_flCurrentTime; }
		
		void GotoPFPoint(int iDirection);

		//
		// Camera tool.
		//
		void Camera_Get(Vector &vecPos, Vector &vecLookAt);
		void Camera_Update(const Vector &vecPos, const Vector &vecLookAt);

		//
		// Selection tool.
		//
		// TODO: clean up doc's interface to selection
		void ClearHitList();
		void AddHit(CMapClass *pObject);
		POSITION GetFirstHitPosition();
		CMapClass *GetNextHit(POSITION& p);
		void SetCurHit(int iIndex, BOOL bRedrawViews = TRUE);
		SelectMode_t Selection_GetMode(void);
		void Selection_SetMode(SelectMode_t eSelectMode);
		void Selection_Remove(CMapClass *pObject);
		void Selection_RemoveAll(void);
		int Selection_GetCount(void);
		CMapClass *Selection_GetObject(int nIndex);
		void Selection_GetBounds(Vector &vecMins, Vector &vecMaxs);
		void Selection_GetBoundsCenter(Vector &vecCenter);
		void Selection_UpdateBounds(void);
		bool Selection_IsSelected(CMapClass *pObj);
		CMapObjectList *Selection_GetList(void); // FIXME: HACK for interim support of old code, remove
		void Selection_GetLastValidBounds(Vector &vecMins, Vector &vecMaxs);

		enum
		{
			hitNext = -1,
			hitPrev = 2
		};

		// SelectObject/SelectFace parameters:
		typedef enum
		{
			scToggle = 0x01,
			scSelect = 0x02,
			scUnselect = 0x04,
			scUpdateDisplay = 0x08,
			scClear = 0x10,				// Clear current before selecting
			scNoLift = 0x20,			// Don't lift face attributes into Face Properties dialog   dvs: lame!
			scNoApply = 0x40			// Don't apply face attributes from Face Properties dialog to selected face   dvs: lame!
		};

		//
		// Called by the tool manager when the active tool changes.
		//
		void ActivateTool(ToolID_t eNewTool);

		//
		// Cordon tool.
		//
		bool Cordon_IsCordoning(void);

		int Snap(float x);
		void Snap(CPoint &pt);
		void Snap(Vector &pt, int nFlags = 0);
		inline bool IsSnapEnabled(void);

		//
		// Face selection for face editing.
		//
		void SelectFace(CMapSolid *pSolid, int iFace, int cmd);
		void UpdateForApplicator(BOOL bApplicator);

		// cmd is from CMapView2d::updXXX
		void ToolUpdateViews(int cmd);

		void UpdateAnimation( void );

		void DeleteObject(CMapClass *pObject);
		void DeleteObjectList(CMapObjectList &List);

		void InternalEnableLightPreview( bool bCustomFilename );

		//
		// Object selection:
		//
		void SelectObject(CMapClass *pobj, int cmd = scSelect);
		void SelectObjectList(CMapObjectList *pList);
		void SelectRegion(BoundBox *pBox, BOOL bInsideOnly);

		void CenterSelection(void);
		Vector GetSelectedCenter();

		void GetBestVisiblePoint(Vector &ptOrg);
		void Paste(CMapObjectList &Objects, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, Vector vecOffset = vec3_origin, QAngle vecRotate = vec3_angle, CMapClass *pParent = NULL);
		void GetBestPastePoint(Vector &vecPasteOrigin);
		void NudgeObjects(Axes2& axes, UINT nChar, BOOL bSnap);
		void UpdateStatusbar();
		void SetView2dInfo(VIEW2DINFO& vi);
		void Center3DViewsOn( const Vector &vPos );
		void SetActiveView(CMapView *pViewActivate);
		void SetUndoActive(bool bActive);
		void UpdateTitle(CView*);

		void CountSolids();
		void CountSolids2();
		void ReplaceTextures(LPCTSTR pszFind, LPCTSTR pszReplace, BOOL bEverything, int iAction, BOOL bHidden);
        void BatchReplaceTextures( FILE *fp );

		//
		// Functions for finding brushes and entities by ordinal. This allows us to go directly to
		// brushes that are reported by the compile tools.
		//
		CMapSolid *FindBrushNumber(CMapClass *pEntity, int nBrushNumber, BoundBox *pBoundBox, BOOL bVisiblesOnly);
		CMapClass *FindEntityNumber(int nEntityNumber, BoundBox *pBoundBox, BOOL bVisiblesOnly);

		bool Is3DGridEnabled(void) { return(m_bShow3DGrid); }

		void ReleaseVideoMemory( );

		inline MAPFORMAT GetMapFormat(void);
		inline CMapWorld *GetMapWorld(void);
		inline CGameConfig *GetGame(void);
		inline int GetGridSpacing(void) { return(max(m_nGridSpacing, 1)); }

		inline CHistory *GetDocHistory(void);

		inline int GetNextMapObjectID(void);
		inline int GetNextNodeID(void);
		inline void SetNextNodeID(int nID);

		static POSITION GetFirstDocPosition();
		static CMapDoc *GetNextDoc(POSITION &p);

		void SetMRU(CMapView2D *pView);
		void RemoveMRU(CMapView2D *pView);
		CTypedPtrList<CPtrList, CMapView2D*> MRU2DViews;

		void Update3DViews(void);
		BOOL SelectDocType(void);
		BOOL SaveModified(void);

		// Set edit prefab mode.
		void EditPrefab3D(DWORD dwPrefabID);

		//
		// Call these when modifying the document contents.
		//
		void AddObjectToWorld(CMapClass *pObject, CMapClass *pParent = NULL);
		BOOL FindObject(CMapClass *pObject);
		void RemoveObjectFromWorld(CMapClass *pMapClass, bool bNotifyChildren);
		void RenderPreloadObject(CMapClass *pObject);
		void UpdateAllObjects(void);
		void UpdateObject(CMapClass *pMapClass);
		void UpdateVisibilityAll(void);
		void UpdateVisibility(CMapClass *pObject);

		//
		// Morph tool.
		//
		void Morph_SelectObject(CMapSolid *pSolid, unsigned int uCmd);
		void Morph_GetBounds(Vector &mins, Vector &maxs, bool bReset);
		int Morph_GetObjectCount(void);
		CSSolid *Morph_GetFirstObject(POSITION &pos);
		CSSolid *Morph_GetNextObject(POSITION &pos);

		//
		// Displacement Map Functinos
		//
		//void UpdateDispViews( void );
        void UpdateDispFaces( Selection3D *selection );

		inline bool IsDispSolidDrawMask( void ) { return m_bDispSolidDrawMask; }
		inline void SetDispDrawWalkable( bool bValue ) { m_bDispDrawWalkable = bValue; }
		inline bool IsDispDrawWalkable( void )  { return m_bDispDrawWalkable; }
		inline void SetDispDrawBuildable( bool bValue ) { m_bDispDrawBuildable = bValue; }
		inline bool IsDispDrawBuildable( void ) { return m_bDispDrawBuildable; }

		//
		// List of VisGroups.
		//
		CVisGroup *VisGroups_AddGroup(CVisGroup *pGroup);
		CVisGroup *VisGroups_AddGroup(LPCTSTR pszName);
		static BOOL VisGroups_CheckForGroupCallback(CMapClass *pObject, CVisGroup *pGroup);
		int VisGroups_GetCount(void);
		POSITION VisGroups_GetHeadPosition(void);
		CVisGroup *VisGroups_GetNext(POSITION &pos);
		CVisGroup *VisGroups_GroupForID(DWORD id);
		void VisGroups_PurgeGroups(void);
		void VisGroups_RemoveID(DWORD id);

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapDoc)
		public:
		virtual BOOL OnNewDocument();
		virtual void DeleteContents();
		virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
		virtual void OnCloseDocument(void);
		virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
		//}}AFX_VIRTUAL

		BOOL Serialize(fstream &file, BOOL fIsStoring, BOOL bRMF);
		
		// Save a VMF file. saveFlags is a combination of SAVEFLAGS_ defines.
		bool SaveVMF(const char *pszFileName, int saveFlags);
		
		bool LoadVMF(const char *pszFileName);
		void Postload(void);
		inline bool IsLoading(void);

		void ExpandObjectKeywords(CMapClass *pObject, CMapWorld *pWorld);

		#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
		#endif

		void UpdateAllCameras(Camera3D &camera);

		void SetSmoothingGroupVisual( int iGroup )		{ m_SmoothingGroupVisual = iGroup; }
		int GetSmoothingGroupVisual( void )				{ return m_SmoothingGroupVisual; }

	protected:

		void Initialize();

		//
		// Tools:
		//
  		Selection3D *m_pToolSelection;
  		CToolBlock *m_pToolBlock;
  		Marker3D *m_pToolMarker;
  		Morph3D *m_pToolMorph;
  		Path3D *m_pToolPath;
  		Clipper3D *m_pToolClipper;
  		CToolOverlay *m_pToolOverlay;
  		Cordon3D *m_pToolCordon;
  		Camera3D *m_pToolCamera;

		int m_nGridSpacing;

		bool m_bDispSolidDrawMask;
		bool m_bDispDrawWalkable;
		bool m_bDispDrawBuildable;

		bool m_bLoading; // Set to true while we are being loaded from VMF.

		//
		// Serialization.
		//
		ChunkFileResult_t SaveVersionInfoVMF(CChunkFile *pFile);
		ChunkFileResult_t VisGroups_SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);
		ChunkFileResult_t SaveViewSettingsVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);

		static bool HandleLoadError(CChunkFile *pFile, const char *szChunkName, ChunkFileResult_t eError, CMapDoc *pDoc);
		static ChunkFileResult_t LoadCamerasCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadCordonCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadEntityCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadHiddenCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadGroupKeyCallback(const char *szKey, const char *szValue, CMapGroup *pGroup);
		static ChunkFileResult_t LoadVersionInfoCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadVersionInfoKeyCallback(const char *szKey, const char *szValue, CMapDoc *pDoc);
		static ChunkFileResult_t LoadVisGroupCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadVisGroupKeyCallback(CChunkFile *pFile, CVisGroup *pGroup);
		static ChunkFileResult_t LoadVisGroupsCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadWorldCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadViewSettingsCallback(CChunkFile *pFile, CMapDoc *pDoc);
		static ChunkFileResult_t LoadViewSettingsKeyCallback(const char *szKey, const char *szValue, CMapDoc *pDoc);

		//
		// Search functions.
		//
		static BOOL FindEntityCallback(CMapClass *pObject, FindEntity_t *pFindInfo);
		static BOOL FindGroupCallback(CMapGroup *pGroup, FindGroup_t *pFindInfo);
		static BOOL FindObjectNumberCallback(CMapClass *pObject, FindObjectNumber_t *pFindInfo);

		static BOOL AddLeavesToListCallback(CMapClass *pObject, CMapObjectList *pList);
		static BOOL AddUnselectedToGroupCallback(CMapClass *pObject, AddNonSelectedInfo_t *pInfo);

		void AssignToGroups(void);
		void CountGUIDs(void);

		bool ShouldObjectBeVisible(CMapClass *pObject);
		static BOOL UpdateVisibilityCallback(CMapClass *pObject, CMapDoc *pDoc);
		static BOOL ForceVisibilityCallback(CMapClass *pObject, bool bVisibility);

		CMapWorld *m_pWorld;				// The world that this document represents.
		CMapObjectList m_UpdateList;		// List of objects that have changed since the last call to Update.
		CString m_strLastExportFileName;	// The full path that we last exported this document to. 
		int m_nDocVersion;					// A number that increments every time the doc is modified after being saved.

		// Undo/Redo system.
		CHistory *m_pUndo;
		CHistory *m_pRedo;

		// Hit selection.
		CMapObjectList HitSel;
		int nHitSel;
		int iCurHit;

		int m_nNextMapObjectID;			// The ID that will be assigned to the next CMapClass object in this document.
		int m_nNextNodeID;				// The ID that will be assigned to the next "info_node_xxx" object created in this document.

		// Editing prefabs data.
		DWORD m_dwPrefabID;
		DWORD m_dwPrefabLibraryID;
		BOOL m_bEditingPrefab;
		bool m_bPrefab;					// true if this document IS a prefab, false if not.

		// Game configuration.
		CGameConfig *m_pGame;

		bool m_bShow3DGrid;				// Whether to render a grid in the 3D views.
		bool m_bHideItems;				// Whether to render point entities in all views.

		//
		// Animation.
		//
		float m_flAnimationTime;		// Current time in the animation
		bool m_bIsAnimating;

		IBSPLighting		*m_pBSPLighting;

		//
		// Visgroups.
		//
		CTypedPtrMap<CMapWordToPtr, WORD, CVisGroup *> m_VisGroups;

		int				m_SmoothingGroupVisual;

		//
		// Expands %i keyword in prefab targetnames to generate unique targetnames for this map.
		//
		bool ExpandTargetNameKeywords(char *szNewTargetName, const char *szOldTargetName, CMapWorld *pWorld);
		bool DoExpandKeywords(CMapClass *pObject, CMapWorld *pWorld, char *szOldKeyword, char *szNewKeyword);

		//{{AFX_MSG(CMapDoc)
		afx_msg void OnEditDelete();
		afx_msg void OnMapSnaptogrid();
		afx_msg void OnMapEntityGallery();
		afx_msg void OnUpdateMapSnaptogrid(CCmdUI* pCmdUI);
		afx_msg void OnEditApplytexture();
		afx_msg void OnToolsSubtractselection();
		afx_msg void OnEnableLightPreview();
		afx_msg void OnEnableLightPreviewCustomFilename();
		afx_msg void OnDisableLightPreview();
		afx_msg void OnToggleLightPreview();
		afx_msg void OnUpdateLightPreview();
		afx_msg void OnAbortLightCalculation();
		afx_msg void OnEditCopy();
		afx_msg void OnEditPaste();
		afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
		afx_msg void OnEditCut();
		afx_msg void OnEditReplace();
		afx_msg void OnToolsGroup();
		afx_msg void OnToolsUngroup();
		afx_msg void OnViewGrid();
		afx_msg void OnUpdateViewGrid(CCmdUI* pCmdUI);
		afx_msg void OnEditSelectall();
		afx_msg void OnFileSaveAs();
		afx_msg void OnFileSave();
		afx_msg void OnMapGridlower();
		afx_msg void OnMapGridhigher();
		afx_msg void OnEditToworld();
		afx_msg void OnEditToWorld();
		afx_msg void OnFileExport();
		afx_msg void OnFileExportAgain();
		afx_msg void OnEditMapproperties();
		afx_msg void OnFileConvertWAD();
		afx_msg void OnUpdateFileConvertWAD(CCmdUI* pCmdUI);
		afx_msg void OnFileRunmap();
		afx_msg void OnToolsHideitems();
		afx_msg void OnUpdateToolsSubtractselection(CCmdUI* pCmdUI);
		afx_msg void OnUpdateToolsHideitems(CCmdUI* pCmdUI);
		afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
		afx_msg void OnUpdateEditFunction(CCmdUI* pCmdUI);
		afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
		afx_msg void OnUpdateEditMapproperties(CCmdUI* pCmdUI);
		afx_msg void OnMapInformation();
		afx_msg void OnViewCenteronselection();
		afx_msg void Center3DViewsOnSelection();
		afx_msg void OnToolsHollow();
		afx_msg void OnEditPastespecial();
		afx_msg void OnUpdateEditPastespecial(CCmdUI* pCmdUI);
		afx_msg void OnEditSelnext();
		afx_msg void OnEditSelprev();
		afx_msg void OnViewHideselectedobjects();
		afx_msg void OnViewShowhiddenobjects();
		afx_msg void OnMapCheck();
		afx_msg void OnViewShowconnections();
		afx_msg void OnUpdateViewShowconnections(CCmdUI* pCmdUI);
		afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
		afx_msg void OnToolsCreateprefab();
		afx_msg void OnInsertprefabOriginal();
		afx_msg void OnEditReplacetex();
		afx_msg void OnToolsSnapselectedtogrid();
		afx_msg void OnUpdateToolsSplitface(CCmdUI* pCmdUI);
		afx_msg void OnToolsSplitface();
		afx_msg void OnToolsTransform();
		afx_msg void OnToolsToggletexlock();
		afx_msg void OnUpdateToolsToggletexlock(CCmdUI* pCmdUI);
		afx_msg void OnToolsTextureAlignment(void);
		afx_msg void OnUpdateToolsTextureAlignment(CCmdUI *pCmdUI);
		afx_msg void OnToggleCordon();
		afx_msg void OnUpdateToggleCordon(CCmdUI* pCmdUI);
		afx_msg void OnEditCordon();
		afx_msg void OnUpdateEditCordon(CCmdUI* pCmdUI);
		afx_msg void OnViewHidenonselectedobjects();
		afx_msg void OnUpdateViewHidenonselectedobjects(CCmdUI* pCmdUI);
		afx_msg void OnToggleGroupignore();
		afx_msg void OnUpdateToggleGroupignore(CCmdUI* pCmdUI);
		afx_msg void OnVscaleToggle();
		afx_msg void OnMapEntityreport();
		afx_msg void OnToggleSelectbyhandle();
		afx_msg void OnUpdateToggleSelectbyhandle(CCmdUI* pCmdUI);
		afx_msg void OnToggleInfiniteselect();
		afx_msg void OnUpdateToggleInfiniteselect(CCmdUI* pCmdUI);
		afx_msg void OnFileExporttodxf();
		afx_msg void OnUpdateEditApplytexture(CCmdUI* pCmdUI);
		afx_msg void OnViewHidepaths();
		afx_msg void OnUpdateViewHidepaths(CCmdUI* pCmdUI);
		afx_msg void OnMapLoadpointfile();
		afx_msg void OnMapUnloadpointfile();
		afx_msg void OnUpdate3dViewUI(CCmdUI* pCmdUI);
		afx_msg BOOL OnChange3dViewType(UINT nID);
		afx_msg void OnEditToEntity();
		afx_msg BOOL OnUndoRedo(UINT nID);
		afx_msg void OnUpdateUndoRedo(CCmdUI* pCmdUI);
		afx_msg void OnChangeVertexscale();
		afx_msg void OnUpdateGroupEditFunction(CCmdUI* pCmdUI);
		afx_msg void OnUpdateFileExport(CCmdUI *pCmdUI);
		afx_msg void OnEditClearselection();
		afx_msg void OnUpdateToggle3DGrid(CCmdUI* pCmdUI);
		afx_msg void OnMapGotoBrush(void);
		afx_msg void OnEditFindEntities(void);
		afx_msg void OnToolsHideEntityNames(void);
		afx_msg void OnUpdateToolsHideEntityNames(CCmdUI *pCmdUI);
		afx_msg void OnToggleDispSolidMask( void );
		afx_msg void OnUpdateToggleSolidMask(CCmdUI* pCmdUI);
		afx_msg void OnToggleDispDrawWalkable( void );
		afx_msg void OnUpdateToggleDispDrawWalkable(CCmdUI* pCmdUI);
		afx_msg void OnToggleDispDrawBuildable( void );
		afx_msg void OnUpdateToggleDispDrawBuildable(CCmdUI* pCmdUI);
		afx_msg void OnUpdateViewShowHelpers(CCmdUI *pCmdUI);
		afx_msg void OnViewShowHelpers();
	public:
		afx_msg void OnToggle3DGrid();
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
};


//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline void
//-----------------------------------------------------------------------------
void CMapDoc::DecrementDocVersion(void)
{
	if (m_nDocVersion > 0)
	{
		m_nDocVersion--;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if we are loading, false if not.
//-----------------------------------------------------------------------------
bool CMapDoc::IsLoading(void)
{
	return(m_bLoading);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the document that is currently active.
//-----------------------------------------------------------------------------
CMapDoc *CMapDoc::GetActiveMapDoc(void)
{
	return(m_pMapDoc);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current version of the document that is being worked on.
//-----------------------------------------------------------------------------
int CMapDoc::GetDocVersion(void)
{
	return(m_nDocVersion);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the game configuration for this document.
//-----------------------------------------------------------------------------
CGameConfig *CMapDoc::GetGame(void)
{
	return(m_pGame);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the Undo system for this document.
//-----------------------------------------------------------------------------
CHistory *CMapDoc::GetDocHistory(void)
{
	return(m_pUndo);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the map format of the game configuration for this document.
//-----------------------------------------------------------------------------
MAPFORMAT CMapDoc::GetMapFormat(void)
{
	if (m_pGame != NULL)
	{
		return(m_pGame->mapformat);
	}

	return(mfHalfLife2);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the world that this document edits.
//-----------------------------------------------------------------------------
CMapWorld *CMapDoc::GetMapWorld(void)
{
	return(m_pWorld);
}


//-----------------------------------------------------------------------------
// Purpose: All map objects in a given document are assigned a unique ID.
//-----------------------------------------------------------------------------
int CMapDoc::GetNextMapObjectID(void)
{
	return(m_nNextMapObjectID++);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next unique ID for an AI node. Called when an AI node
//			is created so that each one can have a unique ID.
//
//			We can't use the unique object ID (above) for this because of
//			problems with in-engine editing of nodes and node connections.
//-----------------------------------------------------------------------------
int CMapDoc::GetNextNodeID(void)
{
	return(m_nNextNodeID++);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the next unique ID for a AI node creation. Called when an AI node
//			is created from the shell so that node IDs continue from there.
//-----------------------------------------------------------------------------
void CMapDoc::SetNextNodeID(int nID)
{
	m_nNextNodeID = nID;
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not grid snap is enabled. Called by the tools and
//			views to determine snap behavior.
//-----------------------------------------------------------------------------
bool CMapDoc::IsSnapEnabled(void)
{
	return m_bSnapToGrid;
}


#endif // MAPDOC_H
