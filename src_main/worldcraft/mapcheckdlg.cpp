//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "GameData.h"
#include "GlobalFunctions.h"
#include "History.h"
#include "MainFrm.h"
#include "MapCheckDlg.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapSolid.h"
#include "MapView2D.h"
#include "MapWorld.h"
#include "Options.h"
#include "ToolManager.h"
#include "Worldcraft.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMapCheckDlg::CMapCheckDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapCheckDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapCheckDlg)
	//}}AFX_DATA_INIT

	bChecked = FALSE;
}


void CMapCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapCheckDlg)
	DDX_Control(pDX, IDC_FIXALL, m_cFixAll);
	DDX_Control(pDX, IDC_FIX, m_Fix);
	DDX_Control(pDX, IDC_GO, m_Go);
	DDX_Control(pDX, IDC_DESCRIPTION, m_Description);
	DDX_Control(pDX, IDC_ERRORS, m_Errors);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapCheckDlg, CDialog)
	//{{AFX_MSG_MAP(CMapCheckDlg)
	ON_BN_CLICKED(IDC_GO, OnGo)
	ON_LBN_SELCHANGE(IDC_ERRORS, OnSelchangeErrors)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_FIX, OnFix)
	ON_BN_CLICKED(IDC_FIXALL, OnFixall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapCheckDlg message handlers

#define ID_STRTABLE_START 40000
#define ID_DESCRIPTIONS_START 40500


typedef enum
{
	ErrorNoPlayerStart,
	ErrorMixedFace,
	ErrorDuplicatePlanes,
	ErrorUnmatchedTarget,
	ErrorInvalidTexture,
	ErrorSolidStructure,
	ErrorUnusedKeyvalues,
	ErrorEmptyEntity,
	ErrorDuplicateKeys,
	ErrorSolidContents,
	ErrorInvalidTextureAxes,
	ErrorInvalidLightmapSizeOnDisplacement,
	ErrorDuplicateFaceIDs,
	ErrorDuplicateNodeIDs,
} MapErrorType;


typedef enum
{
	CantFix,
	NeedsFix,
	Fixed,
} FIXCODE;


typedef struct
{
	CMapClass *pObjects[3];
	MapErrorType Type;
	DWORD dwExtra;
	FIXCODE Fix;
} MapError;


//
// Fix functions.
//
static void FixDuplicatePlanes(MapError *pError);
static void FixSolidStructure(MapError *pError);
static void FixInvalidTexture(MapError *pError);
static void FixInvalidTextureAxes(MapError *pError);
static void FixUnusedKeyvalues(MapError *pError);
static void FixEmptyEntity(MapError *pError);
static void FixDuplicateKeys(MapError *pError);
static void FixInvalidContents(MapError *pError);
static void FixInvalidLightmapSizeOnDisplacement( MapError *pError );
static void FixDuplicateFaceIDs(MapError *pError);
static void FixDuplicateNodeIDs(MapError *pError);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapCheckDlg::OnGo() 
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();

	// change description to match error
	int iSel = m_Errors.GetCurSel();

	if(iSel == LB_ERR)
		return;	// no sel

	MapError *pError;
	pError = (MapError*) m_Errors.GetItemDataPtr(iSel);

	VIEW2DINFO vi;
	vi.wFlags = VI_CENTER;

	// get centerpoint
	pError->pObjects[0]->GetBoundsCenter(vi.ptCenter);
	g_pToolManager->SetTool(TOOL_POINTER);
	pDoc->SelectObject(pError->pObjects[0], CMapDoc::scClear | 
		CMapDoc::scSelect | CMapDoc::scUpdateDisplay);
	pDoc->SetView2dInfo(vi);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapCheckDlg::OnFix() 
{
	// fix error.. or try to
	int iSel = m_Errors.GetCurSel();

	if(iSel == LB_ERR)
		return;

	UpdateBox ub;
	CMapObjectList Objects;
	ub.Objects = &Objects;
	Fix(iSel, ub);

	OnSelchangeErrors();
	CMapDoc::GetActiveMapDoc()->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_DISPLAY, &ub);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSel - 
//			&ub - 
//-----------------------------------------------------------------------------
void CMapCheckDlg::Fix(int iSel, UpdateBox &ub)
{
	CString str;
	MapError *pError;
	pError = (MapError*) m_Errors.GetItemDataPtr(iSel);

	CMapDoc::GetActiveMapDoc()->SetModifiedFlag();

	if(pError->Fix != NeedsFix)
	{
		// should never get here because this button is supposed 
		//  to be disabled if the error cannot be fixed
		return;
	}

	for(int i = 0; i < 2; i++)
	{
		if(!pError->pObjects[i])
			continue;

		ub.Objects->AddTail(pError->pObjects[i]);

		Vector mins;
		Vector maxs;
		pError->pObjects[i]->GetRender2DBox(mins, maxs);
		ub.Box.UpdateBounds(mins, maxs);
	}

	switch(pError->Type)
	{
		case ErrorDuplicatePlanes:
		{
			FixDuplicatePlanes(pError);
			break;
		}
		case ErrorDuplicateFaceIDs:
		{
			FixDuplicatePlanes(pError);
			break;
		}
		case ErrorDuplicateNodeIDs:
		{
			FixDuplicateNodeIDs(pError);
			break;
		}
		case ErrorSolidStructure:
		{
			FixSolidStructure(pError);
			break;
		}
		case ErrorSolidContents:
		{
			FixInvalidContents(pError);
			break;
		}
		case ErrorInvalidTexture:
		{
			FixInvalidTexture(pError);
			break;
		}
		case ErrorInvalidTextureAxes:
		{
			FixInvalidTextureAxes(pError);
			break;
		}
		case ErrorUnusedKeyvalues:
		{
			FixUnusedKeyvalues(pError);
			break;
		}
		case ErrorEmptyEntity:
		{
			FixEmptyEntity(pError);
			break;
		}
		case ErrorDuplicateKeys:
		{
			FixDuplicateKeys(pError);
			break;
		}
		case ErrorInvalidLightmapSizeOnDisplacement:
		{
			FixInvalidLightmapSizeOnDisplacement( pError );
			break;
		}
	}

	pError->Fix = Fixed;

	for(i = 0; i < 2; i++)
	{
		if(!pError->pObjects[i])
			continue;

		Vector mins;
		Vector maxs;
		pError->pObjects[i]->GetRender2DBox(mins, maxs);
		ub.Box.UpdateBounds(mins, maxs);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapCheckDlg::OnFixall() 
{
	int iSel = m_Errors.GetCurSel();

	if(iSel == LB_ERR)
		return;

	MapError * pError = (MapError*) m_Errors.GetItemDataPtr(iSel);

	if(pError->Fix == CantFix)
	{
		// should never get here because this button is supposed 
		//  to be disabled if the error cannot be fixed
		return;
	}

	UpdateBox ub;
	CMapObjectList Objects;
	ub.Objects = &Objects;

	int nErrors = m_Errors.GetCount();
	for(int i = 0; i < nErrors; i++)
	{
		MapError * pError2 = (MapError*) m_Errors.GetItemDataPtr(i);
		if(pError2->Type != pError->Type)
			continue;

		Fix(i, ub);
	}

	OnSelchangeErrors();
	CMapDoc::GetActiveMapDoc()->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_DISPLAY, &ub);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapCheckDlg::OnSelchangeErrors() 
{
	// change description to match error
	int iSel = m_Errors.GetCurSel();

	if(iSel == LB_ERR)
	{
		m_Fix.EnableWindow(FALSE);
		m_cFixAll.EnableWindow(FALSE);
		m_Go.EnableWindow(FALSE);
	}

	CString str;
	MapError *pError;
	pError = (MapError*) m_Errors.GetItemDataPtr(iSel);

	str.LoadString(ID_DESCRIPTIONS_START + pError->Type);
	m_Description.SetSel(0, -1);
	m_Description.ReplaceSel(str);

	m_Go.EnableWindow(pError->pObjects[0] != NULL);

	// set state of fix button
	m_Fix.EnableWindow(pError->Fix == NeedsFix);
	m_cFixAll.EnableWindow(pError->Fix != CantFix);

	// set text of fix button
	switch(pError->Fix)
	{
	case NeedsFix:
		m_Fix.SetWindowText("&Fix");
		break;
	case CantFix:
		m_Fix.SetWindowText("Can't fix");
		break;
	case Fixed:
		m_Fix.SetWindowText("(fixed)");
		break;
	}

	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();

	pDoc->Selection_SetMode(selectObjects);
	
	if (pError->pObjects[0])
	{
		pDoc->SelectObject(pError->pObjects[0], CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay);
	}
	else
	{
		pDoc->SelectObject(NULL, CMapDoc::scClear | CMapDoc::scUpdateDisplay);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapCheckDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BOOL CMapCheckDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	DoCheck();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapCheckDlg::KillErrorList()
{
	// delete items in list.. their data ptrs are allocated objects
	int iSize = m_Errors.GetCount();
	for(int i = 0; i < iSize; i++)
	{
		MapError *pError = (MapError*) m_Errors.GetItemDataPtr(i);
		delete pError;
	}

	m_Errors.ResetContent();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			Type - 
//			dwExtra - 
//			... - 
//-----------------------------------------------------------------------------
static void AddError(CListBox *pList, MapErrorType Type, DWORD dwExtra, ...)
{
	CString str;

	str.LoadString(Type + ID_STRTABLE_START);

	if(str.Find('%') != -1)
	{
		CString str2 = str;
		str.Format(str2, dwExtra);
	}

	int iIndex = pList->AddString(str);

	MapError *pError = new MapError;
	memset(pError, 0, sizeof(MapError));

	pError->Type = Type;
	pError->dwExtra = dwExtra;
	pError->Fix = CantFix;

	va_list vl;
	va_start(vl, dwExtra);

	//
	// Get the object pointer from the variable argument list.
	//
	switch(Type)
	{
		case ErrorNoPlayerStart:
		{
			// no objects.
			break;
		}

		case ErrorMixedFace:
		case ErrorUnmatchedTarget:
		case ErrorDuplicatePlanes:
		case ErrorDuplicateFaceIDs:
		case ErrorDuplicateNodeIDs:
		case ErrorSolidStructure:
		case ErrorSolidContents:
		case ErrorInvalidTexture:
		case ErrorUnusedKeyvalues:
		case ErrorEmptyEntity:
		case ErrorDuplicateKeys:
		case ErrorInvalidTextureAxes:
		case ErrorInvalidLightmapSizeOnDisplacement:
		{
			pError->pObjects[0] = va_arg(vl, CMapClass *);
			break;
		}
	}

	//
	// Set the can fix flag.
	//
	switch(Type)
	{
		case ErrorSolidContents:
		case ErrorDuplicatePlanes:
		case ErrorDuplicateFaceIDs:
		case ErrorDuplicateNodeIDs:
		case ErrorSolidStructure:
		case ErrorInvalidTexture:
		case ErrorUnusedKeyvalues:
		case ErrorEmptyEntity:
		case ErrorDuplicateKeys:
		case ErrorInvalidTextureAxes:
		case ErrorInvalidLightmapSizeOnDisplacement:
		{
			pError->Fix = NeedsFix;
			break;
		}
	}

	va_end(vl);

	pList->SetItemDataPtr(iIndex, PVOID(pError));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			DWORD - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL FindPlayer(CMapEntity *pObject, DWORD)
{
	if (pObject->IsPlaceholder() && pObject->IsClass("info_player_start"))
	{
		return(FALSE);
	}
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckRequirements(CListBox *pList, CMapWorld *pWorld)
{
	// ensure there's a player start .. 
	if(pWorld->EnumChildren(ENUMMAPCHILDRENPROC(FindPlayer), 0, 
		MAPCLASS_TYPE(CMapEntity)))
	{
		// if rvl is !0, it was not stopped prematurely.. which means there is 
		// NO player start.
		AddError(pList, ErrorNoPlayerStart, 0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSolid - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL FindMixedSolids(CMapSolid *pSolid, CListBox *pList)
{
	// run thru faces..
	int iFaces = pSolid->GetFaceCount();
	int iSolid = 2;	// start off ambivalent
	for(int i = 0; i < iFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);

		char ch = pFace->texture.texture[0];
		if((ch == '*' && iSolid == 1) || (ch != '*' && iSolid == 0))
		{
			break;
		}
		else iSolid = (ch == '*') ? 0 : 1;
	}

	if(i == iFaces)	// all ok
		return TRUE;

	// NOT ok
	AddError(pList, ErrorMixedFace, 0, pSolid);

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckMixedFaces(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(FindMixedSolids), DWORD(pList),
		MAPCLASS_TYPE(CMapSolid));
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if there is another node entity in the world with the
//			same node ID as the given entity.
// Input  : pNode - 
//			pWorld - 
//-----------------------------------------------------------------------------
bool FindDuplicateNodeID(CMapEntity *pNode, CMapWorld *pWorld)
{
	EnumChildrenPos_t pos;
	CMapClass *pChild = pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if (pEntity && (pEntity != pNode) && pEntity->IsNodeClass())
		{
			int nNodeID1 = pNode->GetNodeID();
			int nNodeID2 = pEntity->GetNodeID();
			if ((nNodeID1 != 0) && (nNodeID2 != 0) && (nNodeID1 == nNodeID2))
			{
				return true;
			}
		}
		
		pChild = pWorld->GetNextDescendent(pos);
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Checks for node entities with the same node ID.
//-----------------------------------------------------------------------------
static void CheckDuplicateNodeIDs(CListBox *pList, CMapWorld *pWorld)
{
	EnumChildrenPos_t pos;
	CMapClass *pChild = pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if (pEntity && pEntity->IsNodeClass())
		{
			if (FindDuplicateNodeID(pEntity, pWorld))
			{
				AddError(pList, ErrorDuplicateNodeIDs, (DWORD)pWorld, pEntity);
			}
		}
		
		pChild = pWorld->GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Checks for faces with identical face normals in this solid object.
// Input  : pSolid - Solid to check for duplicate faces.
// Output : Returns TRUE if the face contains at least one duplicate face,
//			FALSE if the solid contains no duplicate faces.
//-----------------------------------------------------------------------------
BOOL DoesContainDuplicates(CMapSolid *pSolid)
{
	int iFaces = pSolid->GetFaceCount();
	for (int i = 0; i < iFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		Vector& pts1 = pFace->plane.normal;

		for (int j = 0; j < iFaces; j++)
		{
			// Don't check self.
			if (j == i)
			{
				continue;
			}

			CMapFace *pFace2 = pSolid->GetFace(j);
			Vector& pts2 = pFace2->plane.normal;

			if (pts1 == pts2)
			{
				return(TRUE);
			}
		}
	}
	
	return(FALSE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSolid - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL FindDuplicatePlanes(CMapSolid *pSolid, CListBox *pList)
{
	if (DoesContainDuplicates(pSolid))
	{
		AddError(pList, ErrorDuplicatePlanes, 0, pSolid);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckDuplicatePlanes(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(FindDuplicatePlanes), 
		DWORD(pList), MAPCLASS_TYPE(CMapSolid));
}


struct FindDuplicateFaceIDs_t
{
	CMapFaceList All;					// Collects all the face IDs in this map.
	CMapFaceList Duplicates;			// Collects the duplicate face IDs in this map.
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSolid - 
//			pData - 
// Output : Returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
static BOOL FindDuplicateFaceIDs(CMapSolid *pSolid, FindDuplicateFaceIDs_t *pData)
{
	int nFaceCount = pSolid->GetFaceCount();
	for (int i = 0; i < nFaceCount; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		if (pData->All.FindFaceID(pFace->GetFaceID()) != -1)
		{
			if (pData->Duplicates.FindFaceID(pFace->GetFaceID()) != -1)
			{
				pData->Duplicates.AddToTail(pFace);
			}
		}
		else
		{
			pData->All.AddToTail(pFace);
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Reports errors for all faces with duplicate face IDs.
// Input  : pList - 
//			pWorld -  
//-----------------------------------------------------------------------------
static void CheckDuplicateFaceIDs(CListBox *pList, CMapWorld *pWorld)
{
	FindDuplicateFaceIDs_t Lists;
	Lists.All.SetGrowSize(128);
	Lists.Duplicates.SetGrowSize(128);

	pWorld->EnumChildren((ENUMMAPCHILDRENPROC)FindDuplicateFaceIDs, (DWORD)&Lists, MAPCLASS_TYPE(CMapSolid));

	for (int i = 0; i < Lists.Duplicates.Count(); i++)
	{
		CMapFace *pFace = Lists.Duplicates.Element(i);
		AddError(pList, ErrorDuplicateFaceIDs, (DWORD)pFace, (CMapSolid *)pFace->GetParent());
	}
}


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
	return FALSE;	// this is the one
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL CheckTarget(CMapEntity *pEntity, CListBox *pList)
{
	LPCTSTR pszValue = pEntity->GetKeyValue("target");
	if (pszValue == NULL)	// no target
	{
		return(TRUE);
	}

	if (pEntity->IsClass("trigger_changelevel") || pEntity->IsClass("info_landmark"))
	{
		return(TRUE);
	}

	MDkeyvalue kv("targetname", pszValue);

	CMapWorld *pWorld = (CMapWorld*) pEntity->GetWorldObject(pEntity);

	if (pWorld->EnumChildren(ENUMMAPCHILDRENPROC(FindKeyValue), DWORD(&kv), MAPCLASS_TYPE(CMapEntity)))
	{
		// also check paths in world
		POSITION p = pWorld->m_Paths.GetHeadPosition();
		while(p)
		{
			CMapPath *pPath = pWorld->m_Paths.GetNext(p);
			if(!strcmpi(pPath->GetName(), pszValue))
				return TRUE;	// found it
		}

		// returned true - was not aborted prematurely - therefore there is
		// no matching targetname.
		AddError(pList, ErrorUnmatchedTarget, (DWORD)pszValue, pEntity);
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckTargets(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(CheckTarget), DWORD(pList),
		MAPCLASS_TYPE(CMapEntity));
}


//-----------------------------------------------------------------------------
// Purpose: Determines whether a solid is good or bad.
// Input  : pSolid - Solid to check.
//			pList - List box into which to place errors.
// Output : Always returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
static BOOL _CheckSolidIntegrity(CMapSolid *pSolid, CListBox *pList)
{
	CCheckFaceInfo cfi;
	int nFaces = pSolid->GetFaceCount();
	for (int i = 0; i < nFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);

		//
		// Reset the iPoint member so results from previous faces don't carry over.
		//
		cfi.iPoint = -1;

		//
		// Check the face.
		//
		if (!pFace->CheckFace(&cfi))
		{
			AddError(pList, ErrorSolidStructure, 0, pSolid);
			break;
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pList - 
//			*pWorld - 
//-----------------------------------------------------------------------------
static void CheckSolidIntegrity(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(_CheckSolidIntegrity),
		DWORD(pList), MAPCLASS_TYPE(CMapSolid));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSolid - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL _CheckSolidContents(CMapSolid *pSolid, CListBox *pList)
{
	CCheckFaceInfo cfi;
	int nFaces = pSolid->GetFaceCount();
	CMapFace *pFace = pSolid->GetFace(0);
	DWORD dwContents = pFace->texture.q2contents;

	for (int i = 1; i < nFaces; i++)
	{
		pFace = pSolid->GetFace(i);
		if (pFace->texture.q2contents == dwContents)
		{
			continue;
		}
		AddError(pList, ErrorSolidContents, 0, pSolid);
		break;
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckSolidContents(CListBox *pList, CMapWorld *pWorld)
{
	if (CMapDoc::GetActiveMapDoc() && CMapDoc::GetActiveMapDoc()->GetGame() && CMapDoc::GetActiveMapDoc()->GetGame()->mapformat == mfQuake2)
	{
		pWorld->EnumChildren(ENUMMAPCHILDRENPROC(_CheckSolidContents), DWORD(pList), MAPCLASS_TYPE(CMapSolid));
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSolid - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL _CheckLightmapSizeOnDisplacement( CMapSolid *pSolid, CListBox *pList )
{
	//
	// check all faces with the displacement parameter for proper 
	// lightmap size
	//
	int faceCount = pSolid->GetFaceCount();
	for( int i = 0; i < faceCount; i++ )
	{
		//
		// check for faces with a displacement map
		//
		CMapFace *pFace = pSolid->GetFace( i );
		if( !pFace->HasDisp() )
			continue;

		//
		// check the lightmap extents
		//
		if( !pFace->ValidLightmapSize() )
		{
			AddError( pList, ErrorInvalidLightmapSizeOnDisplacement, i, pSolid );
			break;
		}		
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckLightmapSizeOnDisplacement( CListBox *pList, CMapWorld *pWorld )
{
	pWorld->EnumChildren( ENUMMAPCHILDRENPROC( _CheckLightmapSizeOnDisplacement ), 
		                  DWORD( pList ),
						  MAPCLASS_TYPE( CMapSolid ) );
}


//-----------------------------------------------------------------------------
// Purpose: Determines if there are any invalid textures or texture axes on any
//			face of this solid. Adds an error message to the list box for each
//			error found.
// Input  : pSolid - Solid to check.
//			pList - Pointer to the error list box.
// Output : Returns TRUE.
//-----------------------------------------------------------------------------
static BOOL _CheckInvalidTextures(CMapSolid *pSolid, CListBox *pList)
{
	int nFaces = pSolid->GetFaceCount();
	for(int i = 0; i < nFaces; i++)
	{
		const CMapFace *pFace = pSolid->GetFace(i);

		IEditorTexture *pTex = pFace->GetTexture();
		if (pTex->IsDummy())
		{
			AddError(pList, ErrorInvalidTexture, (DWORD)pFace->texture.texture, pSolid);
			return TRUE;
		}

		if (!pFace->IsTextureAxisValid())
		{
			AddError(pList, ErrorInvalidTextureAxes, i, pSolid);
			return(TRUE);
		}
	}
	
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckInvalidTextures(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(_CheckInvalidTextures), DWORD(pList), MAPCLASS_TYPE(CMapSolid));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL _CheckUnusedKeyvalues(CMapEntity *pEntity, CListBox *pList)
{
	if (!pEntity->IsClass() || pEntity->IsClass("multi_manager"))
	{
		return(TRUE);	// can't check if no class associated
	}

	int nKeyValues = pEntity->GetKeyValueCount();
	GDclass *pClass = pEntity->GetClass();

	for (int i = 0; i < nKeyValues; i++)
	{
		if (pClass->VarForName(pEntity->GetKey(i)) == NULL)
		{
			AddError(pList, ErrorUnusedKeyvalues, (DWORD)pEntity->GetClassName(), pEntity);
			return(TRUE);
		}
	}
	
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pList - 
//			pWorld - 
//-----------------------------------------------------------------------------
static void CheckUnusedKeyvalues(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(_CheckUnusedKeyvalues),
		DWORD(pList), MAPCLASS_TYPE(CMapEntity));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			pList - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL _CheckEmptyEntities(CMapEntity *pEntity, CListBox *pList)
{
	if(!pEntity->IsPlaceholder() && !pEntity->GetChildCount())
	{
		AddError(pList, ErrorEmptyEntity, (DWORD)pEntity->GetClassName(), pEntity);
	}
	
	return(TRUE);
}


static void CheckEmptyEntities(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(_CheckEmptyEntities), DWORD(pList), MAPCLASS_TYPE(CMapEntity));
}


static BOOL _CheckDuplicateKeys(CMapEntity *pEntity, CListBox *pList)
{
	int nKeyValues = pEntity->GetKeyValueCount();

	for (int i = 0; i < nKeyValues; i++)
	{
		for (int j = 0; j < nKeyValues; j++)
		{
			if (i == j)
			{
				continue;
			}

			if (!strcmpi(pEntity->GetKey(i), pEntity->GetKey(j)))
			{
				AddError(pList, ErrorDuplicateKeys, (DWORD)pEntity->GetClassName(), pEntity);
				return TRUE;
			}
		}
	}
	
	return TRUE;
}


static void CheckDuplicateKeys(CListBox *pList, CMapWorld *pWorld)
{
	pWorld->EnumChildren(ENUMMAPCHILDRENPROC(_CheckDuplicateKeys),
		DWORD(pList), MAPCLASS_TYPE(CMapEntity));
}

//
// ** FIX FUNCTIONS
//

static void FixDuplicatePlanes(MapError *pError)
{
	// duplicate planes in pObjects[0]
	// run thru faces..

	CMapSolid *pSolid = (CMapSolid*) pError->pObjects[0];

ReStart:
	int iFaces = pSolid->GetFaceCount();
	for(int i = 0; i < iFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		Vector& pts1 = pFace->plane.normal;
		for (int j = 0; j < iFaces; j++)
		{
			// Don't check self
			if (j == i)
			{
				continue;
			}

			CMapFace *pFace2 = pSolid->GetFace(j);
			Vector& pts2 = pFace2->plane.normal;
			if (pts1 == pts2)
			{
				pSolid->DeleteFace(j);
				goto ReStart;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Repairs an invalid solid.
// Input  : pError - Contains information about the error.
//-----------------------------------------------------------------------------
static void FixSolidStructure(MapError *pError)
{
	CMapSolid *pSolid = (CMapSolid *)pError->pObjects[0];

	//
	// First make sure all the faces are good.
	//
	int nFaces = pSolid->GetFaceCount();
	for (int i = nFaces - 1; i >= 0; i--)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		if (!pFace->CheckFace(NULL))
		{
			pFace->Fix();
		}
		//
		// If the face has no points, just remove it from the solid.
		//
		if (pFace->GetPointCount() == 0)
		{
			pSolid->DeleteFace(i);
		}
	}

	//
	// Rebuild the solid from the planes.
	//
	pSolid->CreateFromPlanes();
	pSolid->PostUpdate(Notify_Changed);
}


LPCTSTR GetDefaultTextureName(); // dvs: BAD!


//-----------------------------------------------------------------------------
// Purpose: Replaces any missing textures with the default texture.
// Input  : pError - 
//-----------------------------------------------------------------------------
static void FixInvalidTexture(MapError *pError)
{
	CMapSolid *pSolid = (CMapSolid *)pError->pObjects[0];

	int nFaces = pSolid->GetFaceCount();
	for (int i = 0; i < nFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		if (pFace != NULL)
		{
			IEditorTexture *pTex = pFace->GetTexture();
			if (pTex != NULL)
			{
				if (pTex->IsDummy())
				{
					pFace->SetTexture(GetDefaultTextureName());
				}
			}
		}
	}
}


static void FixInvalidTextureAxes(MapError *pError)
{
	CMapSolid *pSolid = (CMapSolid *)pError->pObjects[0];

	int nFaces = pSolid->GetFaceCount();
	for (int i = 0; i < nFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		if (!pFace->IsTextureAxisValid())
		{
			pFace->InitializeTextureAxes(Options.GetTextureAlignment(), INIT_TEXTURE_FORCE | INIT_TEXTURE_AXES);
		}
	}
}


static void FixInvalidContents(MapError *pError)
{
	CMapSolid *pSolid = (CMapSolid *)pError->pObjects[0];

	CMapFace *pFace = pSolid->GetFace(0);
	DWORD dwContents = pFace->texture.q2contents;

	int nFaces = pSolid->GetFaceCount();
	for (int i = 1; i < nFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		pFace->texture.q2contents = dwContents;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fixes duplicate face IDs by assigning the face a unique ID within
//			the world.
// Input  : pError - Holds the world and the face that is in error.
//-----------------------------------------------------------------------------
static void FixDuplicateFaceIDs(MapError *pError)
{
	CMapWorld *pWorld = (CMapWorld *)pError->pObjects[0];
	CMapFace *pFace = (CMapFace *)pError->dwExtra;

	pFace->SetFaceID(pWorld->FaceID_GetNext());
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void FixInvalidLightmapSizeOnDisplacement( MapError *pError )
{
	CMapSolid *pSolid = ( CMapSolid* )pError->pObjects[0];

	//
	// check and fix all displacement faces of the solid with improper lightmap
	// extents
	//
	int faceCount = pSolid->GetFaceCount();
	for( int i = 0; i < faceCount; i++ )
	{
		CMapFace *pFace = pSolid->GetFace( i );
		if( !pFace->HasDisp() )
			continue;

		// find the bad surfaces
		if( pFace->ValidLightmapSize() )
			continue;

		// adjust the lightmap scale
		pFace->AdjustLightmapScale();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pError - 
//-----------------------------------------------------------------------------
static void FixUnusedKeyvalues(MapError *pError)
{
	CMapEntity *pEntity = (CMapEntity*) pError->pObjects[0];

	GDclass *pClass = pEntity->GetClass();
	if (!pClass)
	{
		return;
	}

	int nKeyValues = pEntity->GetKeyValueCount();
	for (int i = nKeyValues - 1; i >= 0; i--)
	{
		if (pClass->VarForName(pEntity->GetKey(i)) == NULL)
		{
			pEntity->DeleteKeyValue(pEntity->GetKey(i));
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pError - 
//-----------------------------------------------------------------------------
static void FixEmptyEntity(MapError *pError)
{
	CMapClass *pKillMe = pError->pObjects[0];

	if (pKillMe->GetParent() != NULL)
	{
		GetHistory()->KeepForDestruction(pKillMe);
		pKillMe->GetParent()->RemoveChild(pKillMe);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pError - 
//-----------------------------------------------------------------------------
static void FixDuplicateKeys(MapError *pError)
{
	// find duplicate keys in keyvalues and kill them
	CMapEntity *pEntity = (CMapEntity *)pError->pObjects[0];
	int nKeyValues = pEntity->GetKeyValueCount();

DoAgain:

	for (int i = 0; i < nKeyValues; i++)
	{
		for (int j = 0; j < nKeyValues; j++)
		{
			if (i == j)
			{
				continue;
			}

			if (!strcmpi(pEntity->GetKey(i), pEntity->GetKey(j)))
			{
				pEntity->RemoveKey(i);
				//--kv.nkv;
				goto DoAgain;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fixes duplicate node IDs by assigning the entity a unique node ID.
// Input  : pError - Holds the world and the entity that is in error.
//-----------------------------------------------------------------------------
static void FixDuplicateNodeIDs(MapError *pError)
{
	CMapEntity *pEntity = (CMapEntity *)pError->pObjects[0];
	CMapWorld *pWorld = (CMapWorld *)pError->dwExtra;
	pEntity->AssignNodeID();
}


//-----------------------------------------------------------------------------
// Purpose: Checks the map for problems.
//-----------------------------------------------------------------------------
void CMapCheckDlg::DoCheck(void)
{
	CMapWorld *pWorld = GetActiveWorld();

	// clear error list
	KillErrorList();

	// perform checks
	CheckRequirements(&m_Errors, pWorld);
	CheckMixedFaces(&m_Errors, pWorld);
	CheckTargets(&m_Errors, pWorld);
//	CheckDuplicatePlanes(&m_Errors, pWorld);
	CheckDuplicateFaceIDs(&m_Errors, pWorld);
	CheckDuplicateNodeIDs(&m_Errors, pWorld);
	CheckSolidIntegrity(&m_Errors, pWorld);
	CheckSolidContents(&m_Errors, pWorld);
	CheckInvalidTextures(&m_Errors, pWorld);
	CheckUnusedKeyvalues(&m_Errors, pWorld);
	CheckEmptyEntities(&m_Errors, pWorld);
	CheckDuplicateKeys(&m_Errors, pWorld);
	CheckLightmapSizeOnDisplacement( &m_Errors, pWorld );

	if(!m_Errors.GetCount())
	{
		AfxMessageBox("No errors were found.");
		EndDialog(IDOK);
	}
}

