//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "History.h"
#include "GlobalFunctions.h"
#include "MapDoc.h"
#include "MapWorld.h"
#include "SearchReplaceDlg.h"
#include "Worldcraft.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Context data for a FindFirstObject/FindNextObject session.
//
struct FindObject_t
{
	//
	// Where to look: in the world or in the selection set.
	//
	FindReplaceIn_t eFindIn;

	CMapWorld *pWorld;
	EnumChildrenPos_t WorldPos;					// A position in the world tree for world searches.

	CUtlVector<CMapClass *> SelectionList;		// A copy of the selection list for selection only searches.
	int nSelectionIndex;						// The index into the selection list for iterating the selection list.

	//
	// What to look for.
	//	
	CString strFindText;
	bool bVisiblesOnly;
	bool bCaseSensitive;
	bool bWholeWord;
};


CMapClass *FindNextObject(FindObject_t &FindObject);
bool FindCheck(CMapClass *pObject, FindObject_t &FindObject);


//-----------------------------------------------------------------------------
// Purpose: Returns true if the string matches the search criteria, false if not.
// Input  : pszString - String to check.
//			FindObject - Search criteria, including string to search for.
//-----------------------------------------------------------------------------
bool MatchString(const char *pszString, FindObject_t &FindObject)
{
	if (FindObject.bWholeWord)
	{
		if (FindObject.bCaseSensitive)
		{
			return (!strcmp(pszString, FindObject.strFindText));
		}

		return (!stricmp(pszString, FindObject.strFindText));
	}

	if (FindObject.bCaseSensitive)
	{
		return (strstr(pszString, FindObject.strFindText) != NULL);
	}

	return (stristr(pszString, FindObject.strFindText) != NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the string matches the search criteria, false if not.
// Input  : pszIn - 
//			pszOut - String to check.
//			FindObject - Search criteria, including string to search for.
//-----------------------------------------------------------------------------
bool ReplaceString(char *pszOut, const char *pszIn, FindObject_t &FindObject, const char *pszReplace)
{
	//
	// Whole matches are simple, just strcpy the replacement string into the out buffer.
	//
	if (FindObject.bWholeWord)
	{
		if (FindObject.bCaseSensitive && (!strcmp(pszIn, FindObject.strFindText)))
		{
			strcpy(pszOut, pszReplace);
			return true;
		}

		if (!stricmp(pszIn, FindObject.strFindText))
		{
			strcpy(pszOut, pszReplace);
			return true;
		}
	}

	//
	// Partial matches are a little tougher.
	//
	const char *pszStart = NULL;
	if (FindObject.bCaseSensitive)
	{
		pszStart = strstr(pszIn, FindObject.strFindText);
	}
	else
	{
		pszStart = stristr(pszIn, FindObject.strFindText);
	}

	if (pszStart != NULL)
	{
		int nOffset = pszStart - pszIn;

		strncpy(pszOut, pszIn, nOffset);
		pszOut += nOffset;
		pszIn += nOffset + strlen(FindObject.strFindText);

		strcpy(pszOut, pszReplace);
		pszOut += strlen(pszReplace);

		strcpy(pszOut, pszIn);

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Begins a Find or Find/Replace operation.
//-----------------------------------------------------------------------------
CMapClass *FindFirstObject(FindObject_t &FindObject)
{
	CMapClass *pObject = NULL;

	if (FindObject.eFindIn == FindInWorld)
	{
		// Search the entire world.
		pObject = FindObject.pWorld->GetFirstDescendent(FindObject.WorldPos);
	}
	else
	{
		// Search the selection only.
		if (FindObject.SelectionList.Count())
		{
			pObject = FindObject.SelectionList.Element(0);
			FindObject.nSelectionIndex = 1;
		}
	}

	if (!pObject)
	{
		return NULL;
	}

	if (FindCheck(pObject, FindObject))
	{
		return pObject;
	}

	return FindNextObject(FindObject);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//-----------------------------------------------------------------------------
CMapClass *FindNextObject(FindObject_t &FindObject)
{
	while (true)
	{
		CMapClass *pObject = NULL;
		if (FindObject.eFindIn == FindInWorld)
		{
			// Search the entire world.
			pObject = FindObject.pWorld->GetNextDescendent(FindObject.WorldPos);
		}
		else
		{
			// Search the selection only.
			if (FindObject.nSelectionIndex < FindObject.SelectionList.Count())
			{
				pObject = FindObject.SelectionList.Element(FindObject.nSelectionIndex);
				FindObject.nSelectionIndex++;
			}
		}

		if ((!pObject) || FindCheck(pObject, FindObject))
		{
			return pObject;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			FindObject - 
// Output : Returns true if the object matches the search criteria, false if not.
//-----------------------------------------------------------------------------
bool FindCheck(CMapClass *pObject, FindObject_t &FindObject)
{
	CMapEntity *pEntity = dynamic_cast <CMapEntity *>(pObject);
	if (!pEntity)
	{
		return false;
	}

	if (FindObject.bVisiblesOnly && !pObject->IsVisible())
	{
		return false;
	}

	//
	// Search keyvalues.
	//
	int nKeyCount = pEntity->GetKeyValueCount();
	for (int i = 0; i < nKeyCount; i++)
	{
		const char *pszValue = pEntity->GetKeyValue(i);
		if (pszValue && MatchString(pszValue, FindObject))
		{
			return true;
		}
	}

	//
	// Search connections.
	//
	POSITION pos = pEntity->Connections_GetHeadPosition();
	while (pos)
	{
		CEntityConnection *pConn = pEntity->Connections_GetNext(pos);
		if (pConn)
		{
			if (MatchString(pConn->GetTargetName(), FindObject) ||
				MatchString(pConn->GetParam(), FindObject))
			{
				return true;
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pLastFound - 
//			FindObject - 
//			pszReplaceText - 
// Output : Returns the number of occurrences of the find text that were replaced.
//-----------------------------------------------------------------------------
int FindReplace(CMapEntity *pEntity, FindObject_t &FindObject, const char *pszReplace)
{
	int nReplacedCount = 0;
	
	//
	// Replace keyvalues.
	//
	int nKeyCount = pEntity->GetKeyValueCount();
	for (int i = 0; i < nKeyCount; i++)
	{
		const char *pszValue = pEntity->GetKeyValue(i);
		char szNewValue[MAX_PATH];
		if (pszValue && ReplaceString(szNewValue, pszValue, FindObject, pszReplace))
		{
			const char *pszKey = pEntity->GetKey(i);
			if (pszKey)
			{
				pEntity->SetKeyValue(pszKey, szNewValue);
				nReplacedCount++;
			}
		}
	}

	//
	// Replace connections.
	//
	POSITION pos = pEntity->Connections_GetHeadPosition();
	while (pos)
	{
		CEntityConnection *pConn = pEntity->Connections_GetNext(pos);
		if (pConn)
		{
			char szNewValue[MAX_PATH];

			if (ReplaceString(szNewValue, pConn->GetTargetName(), FindObject, pszReplace))
			{
				pConn->SetTargetName(szNewValue);
				nReplacedCount++;
			}
			
			if (ReplaceString(szNewValue, pConn->GetParam(), FindObject, pszReplace))
			{
				pConn->SetParam(szNewValue);
				nReplacedCount++;
			}
		}
	}

	return nReplacedCount;
}


BEGIN_MESSAGE_MAP(CSearchReplaceDlg, CDialog)
	//{{AFX_MSG_MAP(CSearchReplaceDlg)
	ON_WM_SHOWWINDOW()
	ON_COMMAND_EX(IDC_FIND_NEXT, OnFindReplace)
	ON_COMMAND_EX(IDC_REPLACE, OnFindReplace)
	ON_COMMAND_EX(IDC_REPLACE_ALL, OnFindReplace)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pParent - 
//-----------------------------------------------------------------------------
CSearchReplaceDlg::CSearchReplaceDlg(CWnd *pParent)
	: CDialog(CSearchReplaceDlg::IDD, pParent)
{
	m_bNewSearch = true;

	//{{AFX_DATA_INIT(CSearchReplaceDlg)
	m_bVisiblesOnly = FALSE;
	m_nFindIn = FindInWorld;
	m_bWholeWord = FALSE;
	m_bCaseSensitive = FALSE;
	//}}AFX_DATA_INIT
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CSearchReplaceDlg::Create(CWnd *pwndParent)
{
	return CDialog::Create(CSearchReplaceDlg::IDD, pwndParent);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDX - 
//-----------------------------------------------------------------------------
void CSearchReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CSearchReplaceDlg)
	DDX_Check(pDX, IDC_VISIBLES_ONLY, m_bVisiblesOnly);
	DDX_Check(pDX, IDC_WHOLE_WORD, m_bWholeWord);
	DDX_Check(pDX, IDC_CASE_SENSITIVE, m_bCaseSensitive);
	DDX_Text(pDX, IDC_FIND_TEXT, m_strFindText);
	DDX_Text(pDX, IDC_REPLACE_TEXT, m_strReplaceText);
	DDX_Radio(pDX, IDC_SELECTION, m_nFindIn);
	//}}AFX_DATA_MAP
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSearchReplaceDlg::OnCancel(void)
{
	ShowWindow(SW_HIDE);
}


//-----------------------------------------------------------------------------
// Purpose: Fill out the find criteria from the dialog controls.
// Input  : FindObject - 
//-----------------------------------------------------------------------------
void CSearchReplaceDlg::GetFindCriteria(FindObject_t &FindObject, CMapDoc *pDoc)
{
	FindObject.pWorld = pDoc->GetMapWorld();

	if (m_nFindIn == FindInSelection)
	{
		FindObject.eFindIn = FindInSelection;

		FindObject.SelectionList.RemoveAll();

		int nSelCount = pDoc->Selection_GetCount();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = pDoc->Selection_GetObject(i);
			if (pObject)
			{
				FindObject.SelectionList.AddToTail(pObject);
			}
		}
	}
	else
	{
		FindObject.eFindIn = FindInWorld;
	}

	FindObject.strFindText = m_strFindText;
	FindObject.bVisiblesOnly = (m_bVisiblesOnly == TRUE);
	FindObject.bWholeWord = (m_bWholeWord == TRUE);
	FindObject.bCaseSensitive = (m_bCaseSensitive == TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Called when they hit the Find, the Replace, or the Replace All button.
// Input  : uCmd - The ID of the button the user hit, IDC_FIND_NEXT or IDC_REPLACE.
// Output : Returns TRUE to indicate that the message was handled.
//-----------------------------------------------------------------------------
BOOL CSearchReplaceDlg::OnFindReplace(UINT uCmd)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (!pDoc)
	{
		return TRUE;
	}

	static FindObject_t FindObject;
	static CMapClass *pLastFound = NULL;
	static nReplaceCount = 0;

	bool bDone = false;

	do
	{
		CMapClass *pObject = NULL;

		if (m_bNewSearch)
		{
			//
			// New search. Fetch the data from the controls.
			//
			UpdateData();
			GetFindCriteria(FindObject, pDoc);

			//
			// We have to keep track of the last object in the iteration for replacement,
			// because replacement is done when me advance to the next object.
			//
			pLastFound = NULL;
			nReplaceCount = 0;

			pObject = FindFirstObject(FindObject);
		}
		else
		{
			pObject = FindNextObject(FindObject);
		}

		//
		// Replace All is undone as single operation. Mark the undo position the first time
		// we find a match during a Replace All.
		//
		if (m_bNewSearch && (uCmd == IDC_REPLACE_ALL) && pObject)
		{
			GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Replace Text");
		}

		//
		// If we have an object to do the replace on, do the replace.
		//
		if (pLastFound && ((uCmd == IDC_REPLACE) || (uCmd == IDC_REPLACE_ALL)))
		{
			if (uCmd == IDC_REPLACE)
			{
				// Allow for undo each time we do a Replace.
				GetHistory()->MarkUndoPosition(NULL, "Replace Text");
			}

			//
			// Do the replace on the last matching object we found. This lets the user see what
			// object will be modified before it is done.
			//
			GetHistory()->Keep(pLastFound);
			nReplaceCount += FindReplace((CMapEntity *)pLastFound, FindObject, m_strReplaceText);

			GetDlgItem(IDCANCEL)->SetWindowText("Close");
		}

		if (pObject)
		{
			//
			// We found an object that satisfies our search.
			//
			if ((uCmd == IDC_FIND_NEXT) || (uCmd == IDC_REPLACE))
			{
				//
				// Highlight the match.
				//
				pDoc->SelectObject(pObject, CMapDoc::scClear | CMapDoc::scSelect);
				pDoc->CenterSelection();
			}

			//
			// Stop after one match unless we are doing a Replace All.
			//
			if (uCmd != IDC_REPLACE_ALL)
			{
				bDone = true;
			}

			m_bNewSearch = false;
			pLastFound = pObject;
		}
		else
		{
			//
			// No more objects in the search set match our criteria.
			//
			if ((m_bNewSearch) || (uCmd != IDC_REPLACE_ALL))
			{
				CString str;
				str.Format("Finished searching for '%s'.", m_strFindText);
				MessageBox(str, "Find/Replace Text", MB_OK);

				// TODO: put the old selection back
			}
			else if (uCmd == IDC_REPLACE_ALL)
			{
				CString str;
				str.Format("Replaced %d occurrences of the string '%s' with '%s'.", nReplaceCount, m_strFindText, m_strReplaceText);
				MessageBox(str, "Find/Replace Text", MB_OK);
			}

			m_bNewSearch = true;
			bDone = true;
		}

	} while (!bDone);

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSearchReplaceDlg::OnOK() 
{
}


//-----------------------------------------------------------------------------
// Purpose: Called any time we are hidden or shown.
// Input  : bShow - 
//			nStatus - 
//-----------------------------------------------------------------------------
void CSearchReplaceDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		m_bNewSearch = true;
		GetDlgItem(IDCANCEL)->SetWindowText("Cancel");

		m_nFindIn = FindInWorld;
		CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
		if (pDoc)
		{
			if (pDoc->Selection_GetCount() > 0)
			{
				m_nFindIn = FindInSelection;
			}
		}

		// Populate the controls with the current data.
		UpdateData(FALSE);
	}

	CDialog::OnShowWindow(bShow, nStatus);
}
