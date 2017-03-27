//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//			TODO: add an autoregistration system for tools a la LINK_ENTITY_TO_CLASS
//=============================================================================

#include "stdafx.h"
#include "MapDoc.h"
#include "MainFrm.h"
#include "MapView2D.h"			// FIXME: for MapView2D::updTool
#include "ToolAxisHandle.h"
#include "ToolDecal.h"
#include "ToolDisplace.h"
#include "ToolManager.h"
#include "ToolMagnify.h"
#include "ToolMaterial.h"
#include "ToolPickFace.h"
#include "ToolPickAngles.h"
#include "ToolPickEntity.h"
#include "ToolPointHandle.h"
#include "ToolSphere.h"


CToolManager *g_pToolManager = NULL;


//-----------------------------------------------------------------------------
// Purpose: Prepares the tool manager for use.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CToolManager::Init()
{
	g_pToolManager = new CToolManager;

	return ((g_pToolManager != NULL) &&
			(g_pToolManager->m_pToolDisplace != NULL) &&
			(g_pToolManager->m_pToolMagnify != NULL) &&
			(g_pToolManager->m_pToolDecal != NULL) &&
			(g_pToolManager->m_pToolMaterial != NULL) &&
			(g_pToolManager->m_pToolPickAngles != NULL) &&
			(g_pToolManager->m_pToolPickEntity != NULL) &&
			(g_pToolManager->m_pToolPickFace != NULL) &&
			(g_pToolManager->m_pToolPointHandle != NULL) &&
			(g_pToolManager->m_pToolSphere != NULL));
}


//-----------------------------------------------------------------------------
// Purpose: Shuts down the tool manager - called on app exit.
//-----------------------------------------------------------------------------
void CToolManager::Shutdown()
{
	delete g_pToolManager;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Allocates the tools.
//-----------------------------------------------------------------------------
CToolManager::CToolManager()
{
    m_pActiveTool = NULL;
	m_eToolID = TOOL_NONE;

	//
	// Create the tools that are held by the tool manager and add them
	// to the internal tools list.
	//
	m_pToolDisplace = new CToolDisplace;
	m_InternalTools.AddToTail(m_pToolDisplace);

	m_pToolMagnify = new CToolMagnify;
	m_InternalTools.AddToTail(m_pToolMagnify);

	m_pToolDecal = new CToolDecal;
	m_InternalTools.AddToTail(m_pToolDecal);

 	m_pToolMaterial = new CToolMaterial;
	m_InternalTools.AddToTail(m_pToolMaterial);

 	m_pToolAxisHandle = new CToolAxisHandle;
 	m_InternalTools.AddToTail(m_pToolAxisHandle);

 	m_pToolPointHandle = new CToolPointHandle;
 	m_InternalTools.AddToTail(m_pToolPointHandle);

 	m_pToolSphere = new CToolSphere;
 	m_InternalTools.AddToTail(m_pToolSphere);

 	m_pToolPickAngles = new CToolPickAngles;
 	m_InternalTools.AddToTail(m_pToolPickAngles);

 	m_pToolPickEntity = new CToolPickEntity;
 	m_InternalTools.AddToTail(m_pToolPickEntity);

 	m_pToolPickFace = new CToolPickFace;
 	m_InternalTools.AddToTail(m_pToolPickFace);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Deletes the tools.
//-----------------------------------------------------------------------------
CToolManager::~CToolManager()
{
	delete m_pToolDisplace;
	delete m_pToolMagnify;
	delete m_pToolDecal;
	delete m_pToolPickAngles;
 	delete m_pToolMaterial;
	delete m_pToolPickEntity;
	delete m_pToolPickFace;
 	delete m_pToolPointHandle;
 	delete m_pToolAxisHandle;
 	delete m_pToolSphere;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CToolManager::AddTool(CBaseTool *pTool)
{
	#ifdef _DEBUG
	int nToolCount = GetToolCount();
	for (int i = 0; i < nToolCount; i++)
	{
		CBaseTool *pToolCheck = GetTool(i);
		if (pToolCheck->GetToolID() == pTool->GetToolID())
		{
			ASSERT(false);
		}
	}
	#endif

	m_Tools.AddToTail(pTool);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseTool *CToolManager::GetTool()
{
	return m_pActiveTool;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a tool pointer for a given tool ID, NULL if there is no
//			corresponding tool.
//-----------------------------------------------------------------------------
CBaseTool *CToolManager::GetToolForID(ToolID_t eToolID)
{
	int nToolCount = GetToolCount();
	for (int i = 0; i < nToolCount; i++)
	{
		CBaseTool *pTool = GetTool(i);
		if (pTool->GetToolID() == eToolID)
		{
			return pTool;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the ID of the active tool.
//-----------------------------------------------------------------------------
ToolID_t CToolManager::GetToolID()
{
	return m_eToolID;
}


//-----------------------------------------------------------------------------
// Purpose: Pushes a new tool onto the tool stack and activates it. The active
//			tool will be deactivated and reactivated when PopTool is called.
//-----------------------------------------------------------------------------
void CToolManager::PushTool(ToolID_t eToolID)
{
	//
	// Add the new tool to the top of the tool stack.
	//
	if (eToolID != GetToolID())
	{
		m_ToolIDStack.AddToHead(GetToolID());
	}

	SetTool(eToolID);
}


//-----------------------------------------------------------------------------
// Purpose: Restores the active tool to what it was when PushTool was called.
//			If the stack is underflowed somehow, the pointer is restored.
//-----------------------------------------------------------------------------
void CToolManager::PopTool()
{
	int nCount = m_ToolIDStack.Count();
	if (nCount > 0)
	{
		ToolID_t eNewTool = m_ToolIDStack.Element(0);
		m_ToolIDStack.Remove(0);

		SetTool(eNewTool);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the current active tool by ID.
// Input  : iTool - ID of the tool to activate.
//-----------------------------------------------------------------------------
void CToolManager::SetTool(ToolID_t eNewTool)
{
	CBaseTool *pTool = GetToolForID(eNewTool);

    //
    // Deactivate the current tool unless it's the same as the one we're activating.
    //
	ToolID_t eOldTool = GetToolID();

	// Check to see that we can deactive the current tool
	if ( m_pActiveTool && !m_pActiveTool->CanDeactivate() )
		return;

	//
	// Deactivate the current tool unless we are just 'reactivating' it.
	//
	if ((m_pActiveTool) && (m_pActiveTool != pTool))
	{
		m_pActiveTool->Deactivate(eNewTool);
	}

	m_pActiveTool = pTool;

	//
	// Activate the new tool.
	//
	if (m_pActiveTool)
	{
		m_pActiveTool->Activate(eOldTool);
	}

	m_eToolID = eNewTool;

	//
	// Tell the active document to redraw the 2D views.
	//
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		pDoc->ToolUpdateViews(CMapView2D::updRenderAll);
	}

	// FIXME: When we start up, we end up here before the main window is created because
	//		  CFaceEditDispPage::OnSetActive() calls SetTool(TOOL_FACEEDIT_DISP). This
	//		  behavior is rather nonsensical during startup.
	CMainFrame *pwndMain = GetMainWnd();
	if (pwndMain != NULL)
	{
		pwndMain->m_ObjectBar.UpdateListForTool(eNewTool);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes all the document-created tools from the tools list.
//			TODO: consider removing all tools from the document
//-----------------------------------------------------------------------------
void CToolManager::RemoveAllTools()
{
	m_Tools.RemoveAll();
	m_Tools.AddVectorToTail(m_InternalTools);
}


