//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "ObjectProperties.h"
#include "ObjectPage.h"
#include "OP_Flags.h"
#include "OP_Groups.h"
#include "OP_Entity.h"
#include "OP_Output.h"
#include "OP_Model.h"
#include "OP_Input.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapGroup.h"
#include "MapSolid.h"
#include "MapStudioModel.h"
#include "MapWorld.h"
#include "History.h"
#include "GlobalFunctions.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Layout types for remembering the last layout of the dialog. We could
// also remember this as an array of booleans for which pages were visible.
//
enum LayoutType_t
{
	ltZero,
	ltNone,
	ltSolid,		// Enable groups only
	ltEntity,		// Enable entity, flags, groups
	ltWorld,		// Enable entity, flags, groups
	ltModelEntity,	// Enable entity, flags, groups, model
	ltMulti			// Enable groups only
};


//
// Used to indicate multiselect of entities with different keyvalues.
//
char *CObjectPage::VALUE_DIFFERENT_STRING = "(different)";


//
// Set while we are changing the page layout.
//
static BOOL bRESTRUCTURING = FALSE;


IMPLEMENT_DYNAMIC(CObjectProperties, CPropertySheet)


BEGIN_MESSAGE_MAP(CObjectProperties, CPropertySheet)
	//{{AFX_MSG_MAP(CObjectProperties)
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	ON_WM_CREATE()
	ON_COMMAND(IDC_APPLY, OnApply)
	ON_COMMAND(IDCANCEL, OnCancel)
	ON_COMMAND(IDI_INPUT, OnInputs)
	ON_COMMAND(IDI_OUTPUT, OnOutputs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


IMPLEMENT_DYNAMIC(editCMapClass, CObject);
IMPLEMENT_DYNAMIC(editCEditGameClass, CObject);


static editCMapClass e_CMapClass;
static editCEditGameClass e_CEditGameClass;


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CObjectProperties::CObjectProperties(void) :
	CPropertySheet()
{
	m_bDummy = false;
	m_pDummy = NULL;
	m_pApplyButton = NULL;
	m_pCancelButton = NULL;
	m_pInputButton = NULL;
	m_pOutputButton = NULL;

	CreatePages();
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
// Input  : nIDCaption - 
//			pParentWnd - 
//			iSelectPage - 
//-----------------------------------------------------------------------------
CObjectProperties::CObjectProperties(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_bDummy = false;
	m_pDummy = NULL;
	m_pApplyButton = NULL;
	m_pCancelButton = NULL;
	m_pInputButton = NULL;
	m_pOutputButton = NULL;

	CreatePages();
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
// Input  : pszCaption - 
//			pParentWnd - 
//			iSelectPage - 
//-----------------------------------------------------------------------------
CObjectProperties::CObjectProperties(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_bDummy = false;
	m_pDummy = NULL;
	m_pApplyButton = NULL;
	m_pCancelButton = NULL;
	m_pInputButton = NULL;
	m_pOutputButton = NULL;

	CreatePages();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CObjectProperties::~CObjectProperties()
{
	delete m_pDummy;

	delete m_pEntity;
	delete m_pFlags;
	delete m_pGroups;
	delete m_pOutput;
	delete m_pInput;
	delete m_pModel;

	delete m_pApplyButton;
	delete m_pCancelButton;
	delete m_pInputButton;
	delete m_pOutputButton;

	delete[] m_ppPages;
}


//-----------------------------------------------------------------------------
// Purpose: Creates all possible pages and attaches our object list to them.
//			Not all will be used depending on the types of objects being edited.
//-----------------------------------------------------------------------------
void CObjectProperties::CreatePages(void)
{
	m_pEntity = new COP_Entity;
	m_pEntity->SetObjectList(&m_DstObjects);

	m_pFlags = new COP_Flags;
	m_pFlags->SetObjectList(&m_DstObjects);

	m_pGroups = new COP_Groups;
	m_pGroups->SetObjectList(&m_DstObjects);

	m_pOutput = new COP_Output;
	m_pOutput->SetObjectList(&m_DstObjects);

	m_pInput = new COP_Input;
	m_pInput->SetObjectList(&m_DstObjects);

	m_pModel = new COP_Model;
	m_pModel->SetObjectList(&m_DstObjects);

	m_pDummy = new CPropertyPage(IDD_OBJPAGE_DUMMY);

	m_ppPages = NULL;
	m_nPages = 0;

	m_pLastActivePage = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pType - 
//-----------------------------------------------------------------------------
PVOID CObjectProperties::GetEditObject(CRuntimeClass *pType)
{
	if (pType == RUNTIME_CLASS(editCMapClass))
	{
		return PVOID((CMapClass*)&e_CMapClass);
	}
	else if (pType == RUNTIME_CLASS(editCEditGameClass))
	{
		return PVOID((CEditGameClass*)&e_CEditGameClass);
	}

	ASSERT(0);
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pobj - 
//			pType - 
//-----------------------------------------------------------------------------
PVOID CObjectProperties::GetEditObjectFromMapObject(CMapClass *pobj, CRuntimeClass *pType)
{
	if (pType == RUNTIME_CLASS(editCMapClass))
	{
		return PVOID(pobj);
	}
	else if (pType == RUNTIME_CLASS(editCEditGameClass))
	{
		if (pobj->IsMapClass(MAPCLASS_TYPE(CMapEntity)))
		{
			return PVOID((CEditGameClass*)((CMapEntity*)pobj));
		}

		if (pobj->IsMapClass(MAPCLASS_TYPE(CMapWorld)))
		{
			return PVOID((CEditGameClass*)((CMapWorld*)pobj));
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pobj - 
//-----------------------------------------------------------------------------
void CObjectProperties::CopyDataToEditObjects(CMapClass *pobj)
{
	//
	// All copies here are done without updating object dependencies, because
	// we're copying to a place that is outside of the world.
	//
	e_CMapClass.CopyFrom(pobj, false);

	if (pobj->IsMapClass(MAPCLASS_TYPE(CMapEntity)))
	{
		e_CEditGameClass.CopyFrom((CEditGameClass *)((CMapEntity *)pobj));
	}
	else if (pobj->IsMapClass(MAPCLASS_TYPE(CMapWorld)))
	{
		e_CEditGameClass.CopyFrom((CEditGameClass *)((CMapWorld *)pobj));
	}
}

//------------------------------------------------------------------------------
// Purpose:
// Input  : nState - 
//------------------------------------------------------------------------------
void CObjectProperties::SetOutputButtonState(int nState)
{
	if (nState == CONNECTION_GOOD)
	{
		m_pOutputButton->SetIcon(m_hIconOutputGood);
		m_pOutputButton->ShowWindow(SW_SHOW);
		m_pOutputButton->Invalidate();
		m_pOutputButton->UpdateWindow();
	}
	else if (nState == CONNECTION_BAD)
	{
		m_pOutputButton->SetIcon(m_hIconOutputBad);
		m_pOutputButton->ShowWindow(SW_SHOW);
		m_pOutputButton->Invalidate();
		m_pOutputButton->UpdateWindow();
	}
	else
	{
		m_pOutputButton->ShowWindow(SW_HIDE);
		m_pOutputButton->Invalidate();
		m_pOutputButton->UpdateWindow();
	}
}


//------------------------------------------------------------------------------
// Purpose:
// Input  : nState - 
//------------------------------------------------------------------------------
void CObjectProperties::SetInputButtonState(int nState)
{
	if (nState == CONNECTION_GOOD)
	{
		m_pInputButton->SetIcon(m_hIconInputGood);
		m_pInputButton->ShowWindow(SW_SHOW);
		m_pInputButton->Invalidate();
		m_pInputButton->UpdateWindow();
	}
	else if (nState == CONNECTION_BAD)
	{
		m_pInputButton->SetIcon(m_hIconInputBad);
		m_pInputButton->ShowWindow(SW_SHOW);
		m_pInputButton->Invalidate();
		m_pInputButton->UpdateWindow();
	}
	else
	{
		m_pInputButton->ShowWindow(SW_HIDE);
		m_pInputButton->Invalidate();
		m_pInputButton->UpdateWindow();
	}
}


//------------------------------------------------------------------------------
// Purpose: Set icon being displayed on output button.
//------------------------------------------------------------------------------
void CObjectProperties::UpdateOutputButton(void)
{
	if (!m_pOutputButton)
	{
		return;
	}

	bool bHaveConnection = false;

	POSITION pos = m_DstObjects.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = m_DstObjects.GetNext(pos);

		if ((pObject != NULL) && (pObject->IsMapClass(MAPCLASS_TYPE(CMapEntity))))
		{
			CMapEntity *pEntity = (CMapEntity *)pObject;
			int nStatus = CEntityConnection::ValidateOutputConnections(pEntity);
			if (nStatus == CONNECTION_BAD)
			{
				SetOutputButtonState(CONNECTION_BAD);
				return;
			}
			else if (nStatus == CONNECTION_GOOD)
			{
				bHaveConnection = true;
			}
		}
	}
	if (bHaveConnection)
	{
		SetOutputButtonState(CONNECTION_GOOD);
	}
	else
	{
		SetOutputButtonState(CONNECTION_NONE);
	}
}


//------------------------------------------------------------------------------
// Purpose: Set icon being displayed on input button.
//------------------------------------------------------------------------------
void CObjectProperties::UpdateInputButton()
{
	if (!m_pInputButton)
	{
		return;
	}

	bool bHaveConnection = false;

	POSITION pos = m_DstObjects.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = m_DstObjects.GetNext(pos);

		if ((pObject != NULL) && (pObject->IsMapClass(MAPCLASS_TYPE(CMapEntity))))
		{
			CMapEntity *pEntity = (CMapEntity *)pObject;
			int nStatus = CEntityConnection::ValidateInputConnections(pEntity);
			if (nStatus == CONNECTION_BAD)
			{
				SetInputButtonState(CONNECTION_BAD);
				return;
			}
			else if (nStatus == CONNECTION_GOOD)
			{
				bHaveConnection = true;
			}
		}
	}
	if (bHaveConnection)
	{
		SetInputButtonState(CONNECTION_GOOD);
	}
	else
	{
		SetInputButtonState(CONNECTION_NONE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates the Apply and Cancel buttons.
//-----------------------------------------------------------------------------
void CObjectProperties::CreateButtons(void)
{
	CRect rect;
	GetWindowRect(&rect);
	rect.InflateRect(0, 0, 0, 32);
	MoveWindow(&rect, FALSE);

	GetClientRect(&rect);

	//
	// Add an apply button.
	//
	m_pApplyButton = new CButton;
	m_pApplyButton->Create("Apply", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(rect.right - 80, rect.bottom - 28, rect.right - 6, rect.bottom - 6), this, IDC_APPLY);

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
	{
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	}
  	m_pApplyButton->SendMessage(WM_SETFONT, (WPARAM)hFont);

	//
	// Add a cancel button.
	//
	m_pCancelButton = new CButton;
	m_pCancelButton->Create("Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(rect.right - 160, rect.bottom - 28, rect.right - 86, rect.bottom - 6), this, IDCANCEL);
  	m_pCancelButton->SendMessage(WM_SETFONT, (WPARAM)hFont);

	//
	// Load Icons
	//
	CWinApp *pApp = AfxGetApp();
	m_hIconOutputGood = pApp->LoadIcon(IDI_OUTPUT);
	m_hIconOutputBad  = pApp->LoadIcon(IDI_OUTPUTBAD);
	m_hIconInputGood  = pApp->LoadIcon(IDI_INPUT);
	m_hIconInputBad   = pApp->LoadIcon(IDI_INPUTBAD);

	// Create buttons to display connection status icons
	m_pInputButton = new CButton;
	m_pInputButton->Create(_T("My button"), WS_CHILD|WS_VISIBLE|BS_ICON|BS_FLAT, CRect(6,rect.bottom - 34,38,rect.bottom - 2), this, IDI_INPUT);

	m_pOutputButton = new CButton;
	m_pOutputButton->Create(_T("My button"), WS_CHILD|WS_VISIBLE|BS_ICON|BS_FLAT, CRect(40,rect.bottom - 34,72,rect.bottom - 2), this, IDI_OUTPUT);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the appropriate page layout for the current object list.
//-----------------------------------------------------------------------------
LayoutType_t CObjectProperties::GetLayout(bool &bEntity, bool &bGroups, bool &bFlags, bool &bModel)
{
	bEntity = bGroups = bFlags = bModel = false;

	LayoutType_t eLayoutType;

	if ((m_DstObjects.GetCount() == 0) || (CMapDoc::GetActiveMapDoc() == NULL))
	{
		eLayoutType = ltNone;
	}
	else
	{
		POSITION pos = m_DstObjects.GetHeadPosition();
		bool bFirst = true;
		while (pos != NULL)
		{
			CMapClass *pObject = m_DstObjects.GetNext(pos);
			MAPCLASSTYPE ThisType = pObject->GetType();
			MAPCLASSTYPE PrevType;
			
			if (bFirst)
			{
				bFirst = false;

				if (ThisType == MAPCLASS_TYPE(CMapEntity))
				{
					CMapEntity *pEntity = (CMapEntity *)pObject;
					bGroups = true;
					bFlags = true;
					bEntity = true;
					
					//
					// Only show the model tab when we have a single entity selected that
					// has a model helper.
					//
					if (m_DstObjects.GetCount() == 1)
					{
						if (pEntity->GetChildOfType((CMapStudioModel *)NULL))
						{
							bModel = true;
						}
					}

					eLayoutType = ltEntity;
				}
				else if ((ThisType == MAPCLASS_TYPE(CMapSolid)) ||
						(ThisType == MAPCLASS_TYPE(CMapGroup)))
				{
					bGroups = true;
					eLayoutType = ltSolid;
				}
				else if (ThisType == MAPCLASS_TYPE(CMapWorld))
				{
					bEntity = true;
					eLayoutType = ltWorld;
				}
			}
			else if (ThisType != PrevType)
			{
				bEntity = false;
				bFlags = false;
				bGroups = true;
				eLayoutType = ltMulti;
			}

			PrevType = ThisType;
		}
	}

	return eLayoutType;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectProperties::RestoreActivePage(void)
{
	//
	// Try to restore the previously active page. If it is not in the page list
	// just activate page zero.
	//
	bool bPageSet = false;
	for (int i = 0; i < m_nPages; i++)
	{
		if (m_ppPages[i] == m_pLastActivePage)
		{
			SetActivePage(m_pLastActivePage);
			bPageSet = true;
			break;
		}
	}

	if (!bPageSet)
	{
		SetActivePage(0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectProperties::SaveActivePage(void)
{
	CObjectPage *pPage = (CObjectPage *)GetActivePage();
	if (pPage != NULL)
	{
		m_pLastActivePage = pPage;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets up pages to display based on "m_DstObjects".
// Output : Returns TRUE if the page structure changed, FALSE if not.
//-----------------------------------------------------------------------------
BOOL CObjectProperties::SetupPages(void)
{
	static bool bFirstTime = true;
	static LayoutType_t eLastLayoutType = ltZero;
	static LayoutType_t eLastValidLayoutType = ltZero;

	//
	// Save the current active page.
	//
	if ((eLastLayoutType != ltZero) && (eLastLayoutType != ltNone))
	{	
		SaveActivePage();
	}

	//
	// Determine the appropriate layout for the current object list.
	//	
	bool bEntity;
	bool bGroups;
	bool bFlags;
	bool bModel;

	LayoutType_t eLayoutType = GetLayout(bEntity, bGroups, bFlags, bModel);

	//
	// If the layout has not changed, we're done. All the pages are already set up.
	//
	if (eLayoutType == eLastLayoutType)
	{
		//
		// Try to restore the previously active page. If it has been deleted just
		// activate page zero.
		//
		RestoreActivePage();
		return(FALSE);
	}
	
	if ((eLayoutType != ltNone) && (eLayoutType != eLastValidLayoutType))
	{
		//
		// Forget last active page when the layout changes from one
		// valid layout to another (such as from entity to solid).
		//
		m_pLastActivePage = NULL;
		eLastValidLayoutType = eLayoutType;
	}

	eLastLayoutType = eLayoutType;

	bRESTRUCTURING = TRUE;

	UINT nAddPages = bEntity + bGroups + bFlags + bModel;

	// don't want to change focus .. just pages!
	CWnd *pActiveWnd = GetActiveWindow();

	bool bDisabledraw = false;
	if (::IsWindow(m_hWnd) && IsWindowVisible())
	{
		SetRedraw(FALSE);
		bDisabledraw = true;
	}

	if (!m_bDummy && (nAddPages == 0))
	{
		AddPage(m_pDummy);
		m_bDummy = true;
	}
	else if (m_bDummy && (nAddPages > 0))
	{
		RemovePage(m_pDummy);
		m_bDummy = false;
	}

	//
	// Remove all the pages.
	//
	if (GetPageIndex(m_pEntity) != -1)
	{
		RemovePage(m_pEntity);
	}

	if (GetPageIndex(m_pGroups) != -1)
	{
		RemovePage(m_pGroups);
	}

	if (GetPageIndex(m_pFlags) != -1)
	{
		RemovePage(m_pFlags);
	}

	if (GetPageIndex(m_pOutput) != -1)
	{
		RemovePage(m_pOutput);
	}

	if (GetPageIndex(m_pInput) != -1)
	{
		RemovePage(m_pInput);
	}

	if (GetPageIndex(m_pModel) != -1)
	{
		RemovePage(m_pModel);
	}

	//
	// Add the appropriate pages.
	//
	//   
	if (bEntity)
	{
		AddPage(m_pEntity);
		AddPage(m_pOutput);
		AddPage(m_pInput);
	}

	if (bModel)
	{
		AddPage(m_pModel);
	}

	if (bFlags)
	{
		AddPage(m_pFlags);
	}

	if (bGroups)
	{
		AddPage(m_pGroups);
	}

	//
	// Store active pages in our array.
	//
	if (!m_bDummy)
	{
		delete[] m_ppPages;
		m_nPages = GetPageCount();
		m_ppPages = new CObjectPage*[m_nPages];

		for (int i = 0; i < m_nPages; i++)
		{
			m_ppPages[i] = (CObjectPage *)GetPage(i);
			m_ppPages[i]->m_bFirstTimeActive = true;
		}
	}

	bRESTRUCTURING = FALSE;

	// FIXME: John's machine crashes here on a Tie To Entity!!! AfxMessageBox("Restore Active Page");
	RestoreActivePage();

	//
	// Add an apply and cancel button once only.
	//
	static bool bCreatedButtons = false;
	if (IsWindow(m_hWnd) && !bCreatedButtons)
	{
		CreateButtons();
		bCreatedButtons = true;
	}

	//
	// Enable redraws if they were disabled above.
	//
	if (bDisabledraw)
	{
		SetRedraw(TRUE);
		Invalidate(FALSE);
	}

	// Set button status
	UpdateOutputButton();
	UpdateInputButton();

	if (pActiveWnd != NULL)
	{
		pActiveWnd->SetActiveWindow();
	}

	bFirstTime = false;	

	return TRUE;	// pages changed - return true
}


//------------------------------------------------------------------------------
// Purpose: Set object properties dialogue to the Output tab and highlight
//			the given item
// Input  : pConnection - 
//------------------------------------------------------------------------------
void CObjectProperties::SetPageToOutput(CEntityConnection *pConnection)
{
	SetActivePage(m_pOutput);
	m_pOutput->SetSelectedConnection(pConnection);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectProperties::SaveData(void)
{
	//
	// Make sure window is visible - don't want to save otherwise.
	//
	if (!IsWindowVisible())
	{
		return;
	}

	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (!pDoc || !m_DstObjects.GetCount() || m_bDummy)
	{
		return;
	}

	//
	// Transfer all page data to the objects being edited.
	//
	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Change Properties");
	GetHistory()->Keep(&m_DstObjects);

	for (int i = 0; i < m_nPages; i++)
	{
		//
		// Pages that have never been shown have no hwnd.
		//
		if (IsWindow(m_ppPages[i]->m_hWnd))
		{
			m_ppPages[i]->SaveData();
		}
	}

	//
	// Objects may have changed. Update the views.
	//
	CMapDoc::GetActiveMapDoc()->Selection_UpdateBounds();
	CMapDoc::GetActiveMapDoc()->UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_OBJECTS);
	pDoc->SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Submits the objects to be edited to the property pages so they can
//			update their controls.
// Input  : iPage - Page index or -1 to update all pages.
//-----------------------------------------------------------------------------
void CObjectProperties::LoadDataForPages(int iPage)
{
	if (m_bDummy)
	{
		return;
	}

	//
	// Determine whether we are editing multiple objects or not.
	//
	bool bMultiEdit = (m_DstObjects.GetCount() > 1);

	//
	// Submit the edit objects to each page one at a time.
	//
	int nMode = CObjectPage::LoadFirstData;
	POSITION p = m_DstObjects.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pobj = m_DstObjects.GetNext(p);

		if (iPage != -1)
		{
			//
			// Specific page.
			//
			m_ppPages[iPage]->SetMultiEdit(bMultiEdit);

			void *pObject = GetEditObjectFromMapObject(pobj, m_ppPages[iPage]->GetEditObjectRuntimeClass());
			if (pObject != NULL)
			{
				m_ppPages[iPage]->UpdateData(nMode, pObject);
			}
		}
		else for (int i = 0; i < m_nPages; i++)
		{
			//
			// All pages.
			//
			m_ppPages[i]->SetMultiEdit(bMultiEdit);

			if (m_ppPages[i]->m_bFirstTimeActive)
			{
				continue;
			}

			void *pObject = GetEditObjectFromMapObject(pobj, m_ppPages[i]->GetEditObjectRuntimeClass());
			if (pObject != NULL)
			{
				m_ppPages[i]->UpdateData(nMode, pObject);
			}
		}

		nMode = CObjectPage::LoadData;
	}

	//
	// Tell the pages that we are done submitting data.
	//
	if (iPage != -1)
	{
		//
		// Specific page.
		//
		m_ppPages[iPage]->UpdateData(CObjectPage::LoadFinished, NULL);
	}
	else for (int i = 0; i < m_nPages; i++)
	{
		//
		// All pages.
		//
		m_ppPages[i]->UpdateData(CObjectPage::LoadFinished, NULL);
	}

	//
	// Update the input/output icons based on the new data.
	//
	UpdateOutputButton();
	UpdateInputButton();
}



//-----------------------------------------------------------------------------
// Purpose: Updates the property page data when the selection contents change.
// Input  : pObjects - List of currently selected objects.
//-----------------------------------------------------------------------------
void CObjectProperties::LoadData(CMapObjectList *pObjects, bool bSave)
{
	//
	// Disable window so it does not gain focus during this operation.
	//
	EnableWindow(FALSE);

	if (bSave)
	{
		SaveData();
	}

	m_DstObjects.RemoveAll();
	m_DstObjects.AddTail(pObjects);

	//
	// If there is only one object selected, copy its data to our temporary
	// edit objects.
	//
	if (m_DstObjects.GetCount() == 1)
	{
		//
		// Copy the single destination object's data to our temporary
		// edit objects.
		//
		POSITION p = m_DstObjects.GetHeadPosition();
		CMapClass *pobj = m_DstObjects.GetAt(p);
		CopyDataToEditObjects(pobj);

		//
		// Set the window title to include the object's description.
		//
		char szTitle[MAX_PATH];
		sprintf(szTitle, "Object Properties: %s", pobj->GetDescription());
		SetWindowText(szTitle);
	}
	else if (m_DstObjects.GetCount() > 1)
	{
		SetWindowText("Object Properties: multiple objects");
	}
	else
	{
		SetWindowText("Object Properties");
	}

	//
	// Only call LoadDataForPages() if page structure hasn't changed 
	// (SetupPages() returns 0) - otherwise, might be trying to update
	// pages with no hWnd since RemovePage() destroys a page's hWnd.
	//
	if (!SetupPages())
	{
		LoadDataForPages();
	}

	EnableWindow(TRUE);
}


BOOL CObjectProperties::OnInitDialog() 
{
	BOOL b = CPropertySheet::OnInitDialog();
	SetWindowText("Object Properties");
	return b;
}


//-----------------------------------------------------------------------------
// Purpose: Closes the object properties dialog, saving changes.
//-----------------------------------------------------------------------------
void CObjectProperties::OnClose(void)
{
	SaveData();
	ShowWindow(SW_HIDE);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectProperties::AbortData()
{
	m_DstObjects.RemoveAll();
	SetupPages();
}


void CObjectProperties::DontEdit(CMapClass *pObject)
{
	POSITION p = m_DstObjects.Find(pObject);
	if(!p)
		return;
	m_DstObjects.RemoveAt(p);
	SetupPages();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bShow - 
//			nStatus - 
//-----------------------------------------------------------------------------
void CObjectProperties::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	// Forget the last active page when the window is hidden or shown.
	// FIXME: SetupPages calls SaveActivePage, so we must switch to page 0 here
	SetActivePage(0);
	m_pLastActivePage = NULL;

	CPropertySheet::OnShowWindow(bShow, nStatus);

	for (int i = 0; i < m_nPages; i++)
	{
		m_ppPages[i]->OnShowPropertySheet(bShow, nStatus);
	}
}


IMPLEMENT_DYNCREATE(CObjectPage, CPropertyPage)


//-----------------------------------------------------------------------------
// Purpose: Called when we become the active page.
//-----------------------------------------------------------------------------
BOOL CObjectPage::OnSetActive(void)
{
	if (bRESTRUCTURING || !GetActiveWorld())
	{
		return CPropertyPage::OnSetActive();
	}

	CObjectProperties *pParent = (CObjectProperties *)GetParent();

	if (m_bFirstTimeActive)
	{
		m_bFirstTimeActive = false;
		pParent->LoadDataForPages(pParent->GetPageIndex(this));
	}

	return CPropertyPage::OnSetActive();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PVOID CObjectPage::GetEditObject()
{ 
	return ((CObjectProperties*) GetParent())->GetEditObject(GetEditObjectRuntimeClass());
}


//-----------------------------------------------------------------------------
// Purpose: Handles the Apply button.
//-----------------------------------------------------------------------------
void CObjectProperties::OnApply(void)
{
	for (int i = 0; i < m_nPages; i++)
	{
		if (!m_ppPages[i]->OnApply())
		{
			return;
		}
	}

	//
	// Save and reload the data so the GUI updates.
	//
	SaveData();

	CMapObjectList TempList;
	TempList.AddTail(&m_DstObjects);
	LoadData(&TempList, false);
}


//-----------------------------------------------------------------------------
// Purpose: Handles the Apply button.
//-----------------------------------------------------------------------------
void CObjectProperties::OnCancel(void)
{
	ShowWindow(SW_HIDE);

	//
	// Deselect the current selection, then reselect it to refresh the dialog contents.
	//
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	CMapObjectList OldSelection;
	OldSelection.AddTail(pDoc->Selection_GetList());
	pDoc->SelectObjectList(&OldSelection);
}


//-----------------------------------------------------------------------------
// Purpose: Handles the input icon button.
//-----------------------------------------------------------------------------
void CObjectProperties::OnInputs(void)
{
	SetActivePage(m_pInput);
}


//-----------------------------------------------------------------------------
// Purpose: Handles the output icon button.
//-----------------------------------------------------------------------------
void CObjectProperties::OnOutputs(void)
{
	SetActivePage(m_pOutput);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lpCreateStruct - 
// Output : int
//-----------------------------------------------------------------------------
int CObjectProperties::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	lpCreateStruct->dwExStyle |= WS_EX_TOOLWINDOW;

	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectProperties::RefreshData(void)
{
	for (int i = 0; i < m_nPages; i++)
	{
		if (m_ppPages[i]->m_hWnd)
		{
			m_ppPages[i]->RememberState();
		}
	}

	CMapObjectList TempList;
	TempList.AddTail(&m_DstObjects);
	LoadData(&TempList, false);
}

