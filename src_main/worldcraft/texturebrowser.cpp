//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "stdafx.h"
#include <oleauto.h>
#include <oaidl.h>
#include <afxpriv.h>
#include "CustomMessages.h"
#include "GlobalFunctions.h"
#include "History.h"
#include "IEditorTexture.h"
#include "MapDoc.h"
#include "MapWorld.h"
#include "ReplaceTexDlg.h"
#include "TextureBrowser.h"
#include "TextureSystem.h"
#include "Worldcraft.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static LPCTSTR pszIniSection = "Texture Browser";


CStringArray CTextureBrowser::m_FilterHistory;
int CTextureBrowser::m_nFilterHistory;
char CTextureBrowser::m_szLastKeywords[MAX_PATH];


BEGIN_MESSAGE_MAP(CTextureBrowser, CDialog)
	//{{AFX_MSG_MAP(CTextureBrowser)
	ON_WM_SIZE()
	ON_CBN_SELENDOK(IDC_TEXTURESIZE, OnSelendokTexturesize)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_CBN_EDITCHANGE(IDC_FILTER, OnChangeFilterOrKeywords)
	ON_CBN_SELENDOK(IDC_FILTER, OnUpdateFiltersNOW)
	ON_CBN_EDITCHANGE(IDC_KEYWORDS, OnChangeFilterOrKeywords)
	ON_CBN_SELENDOK(IDC_KEYWORDS, OnUpdateKeywordsNOW)
	ON_BN_CLICKED(IDC_FILTER_OPAQUE, OnFilterOpaque)
	ON_BN_CLICKED(IDC_FILTER_TRANSLUCENT, OnFilterTranslucent)
	ON_BN_CLICKED(IDC_FILTER_SELFILLUM, OnFilterSelfIllum)
	ON_BN_CLICKED(IDC_FILTER_ENVMASK, OnFilterEnvmask)
	ON_BN_CLICKED(IDC_SHOW_ERROR, OnShowErrors)
	ON_BN_CLICKED(IDC_USED, OnUsed)
	ON_BN_CLICKED(IDC_MARK, OnMark)
	ON_BN_CLICKED(IDC_REPLACE, OnReplace)
	ON_MESSAGE(TWN_SELCHANGED, OnTexturewindowSelchange)
	ON_MESSAGE(TWN_LBUTTONDBLCLK, OnTextureWindowDblClk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pParent - 
//-----------------------------------------------------------------------------
CTextureBrowser::CTextureBrowser(CWnd* pParent)
	: CDialog(IDD, pParent)
{
	m_szNameFilter[0] = '\0';
	szInitialTexture[0] = '\0';
	m_bFilterChanged = FALSE;
	m_uLastFilterChange = 0xffffffff;
	m_bUsed = FALSE;
	m_szLastKeywords[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: Handles resize messages. Moves the child windows to the proper positions.
// Input  : nType - 
//			cx - 
//			cy - 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnSize(UINT nType, int cx, int cy) 
{
	if (nType == SIZE_MINIMIZED || !IsWindow(m_cTextureWindow.m_hWnd))
	{
		CDialog::OnSize(nType, cx, cy);
		return;
	}
	
	// reposition controls
	CRect clientrect;
	GetClientRect(&clientrect);

	CRect CtrlRect;
	GetDlgItem(IDC_CONTROLHEIGHT)->GetWindowRect(&CtrlRect);

	int iControlHeight = (CtrlRect.bottom - CtrlRect.top);
	int iThisCtrlHeight;

	//
	// Resize the texture window.
	//
	CtrlRect = clientrect;
	CtrlRect.bottom -= iControlHeight;
	m_cTextureWindow.MoveWindow(CtrlRect);

	clientrect.top = (clientrect.bottom - iControlHeight) + 4;

	//
	// Move the top row of controls to the correct vertical position,
	// leaving their horizontal position as it was set up in the dialog.
	//
	int iIDList[] = 
	{
		IDC_TEXTURESIZE,
		IDC_SIZEPROMPT,
		IDC_FILTERPROMPT,
		IDC_FILTER,
		IDC_CURNAME,
		IDC_FILTER_OPAQUE,
		IDC_FILTER_SELFILLUM,
		IDC_SHOW_ERROR,
		-1
	};

	for (int i = 0; iIDList[i] != -1; i++)
	{
		CWnd *pWnd = GetDlgItem(iIDList[i]);
		ASSERT(pWnd != NULL);

		if (pWnd != NULL)
		{
			pWnd->GetWindowRect(&CtrlRect);
			ScreenToClient(CtrlRect);
			iThisCtrlHeight = CtrlRect.bottom - CtrlRect.top;
			CtrlRect.top = clientrect.top;
			CtrlRect.bottom = CtrlRect.top + iThisCtrlHeight;
			pWnd->MoveWindow(CtrlRect);
		}
	}

	//
	// Move the middle row of controls to the correct vertical position,
	// leaving their horizontal position as it was set up in the dialog.
	//
	int iIDList2[] = 
	{ 
		IDC_KEYWORDS_TEXT,
		IDC_KEYWORDS,
		IDC_USED,
		IDC_MARK,
		IDC_REPLACE,
		IDC_CURDESCRIPTION,
		IDC_FILTER_TRANSLUCENT,
		IDC_FILTER_ENVMASK,
		-1
	};

	for (i = 0; iIDList2[i] != -1; i++)
	{
		CWnd *pWnd = GetDlgItem(iIDList2[i]);
		ASSERT(pWnd != NULL);

		if (pWnd != NULL)
		{
			pWnd->GetWindowRect(&CtrlRect);
			ScreenToClient(CtrlRect);
			iThisCtrlHeight = CtrlRect.bottom - CtrlRect.top;
			CtrlRect.top = clientrect.top + iControlHeight / 2 + 2;
			CtrlRect.bottom = CtrlRect.top + iThisCtrlHeight;
			pWnd->MoveWindow(CtrlRect);
		}
	}

	CDialog::OnSize(nType, cx, cy);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bUsed - 
//-----------------------------------------------------------------------------
void CTextureBrowser::SetUsed(BOOL bUsed)
{
	m_bUsed = bUsed;

	if(m_bUsed)
	{
		GetActiveWorld()->GetUsedTextures(&m_UsedGraphicsList);
		m_cTextureWindow.SetSpecificList(&m_UsedGraphicsList);
	}
	else
	{
		m_cTextureWindow.SetSpecificList(NULL);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnClose(void)
{
	WriteSettings();
	SaveAndExit();
	CDialog::OnCancel();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnCancel()
{
	WriteSettings();
	SaveAndExit();
	CDialog::OnCancel();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnUsed()
{
	if(!GetActiveWorld())
		return;

	SetUsed(m_cUsed.GetCheck());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszTexture - 
//-----------------------------------------------------------------------------
void CTextureBrowser::SetInitialTexture(LPCTSTR pszTexture)
{
	strcpy(szInitialTexture, pszTexture);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnSelendokTexturesize() 
{
	// change size of textures the texutre window displays
	int iCurSel = m_cSizeList.GetCurSel();

	switch(iCurSel)
	{
	case 0:
		m_cTextureWindow.SetDisplaySize(128);
		break;
	case 1:
		m_cTextureWindow.SetDisplaySize(256);
		break;
	case 2:
		m_cTextureWindow.SetDisplaySize(512);
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BOOL CTextureBrowser::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_cSizeList.SubclassDlgItem(IDC_TEXTURESIZE, this);
	m_cFilter.SubclassDlgItem(IDC_FILTER, this);
	m_cKeywords.SubclassDlgItem(IDC_KEYWORDS, this);
	m_cCurName.SubclassDlgItem(IDC_CURNAME, this);
	m_cCurDescription.SubclassDlgItem(IDC_CURDESCRIPTION, this);
	m_cUsed.SubclassDlgItem(IDC_USED, this);
   
	m_FilterOpaque.SubclassDlgItem(IDC_FILTER_OPAQUE, this);
	m_FilterTranslucent.SubclassDlgItem(IDC_FILTER_TRANSLUCENT, this);
	m_FilterSelfIllum.SubclassDlgItem(IDC_FILTER_SELFILLUM, this);
	m_FilterEnvMask.SubclassDlgItem(IDC_FILTER_ENVMASK, this);
	m_ShowErrors.SubclassDlgItem(IDC_SHOW_ERROR, this);

	m_FilterOpaque.SetCheck( true );
	m_FilterTranslucent.SetCheck( true );
	m_FilterSelfIllum.SetCheck( true );
	m_FilterEnvMask.SetCheck( true );
	m_ShowErrors.SetCheck( true );

	//
	// Create CTextureWindow that takes up area of dummy control.
	//
	RECT r;
	GetDlgItem(IDC_BROWSERDUMMY)->GetClientRect(&r);
	m_cTextureWindow.Create(this, r);

	// Show everything initially
	m_cTextureWindow.SetTypeFilter( ~0, true );

	//
	// Add latest history to the filter combo.
	//
	for (int i = 0; i < m_nFilterHistory; i++)
	{
		m_cFilter.AddString(m_FilterHistory[i]);
	}

	//
	// Set the name filter unless one was explicitly specified.
	//
	if (m_szNameFilter[0] == '\0')
	{
		//
		// No name filter specified. Use whatever is on top of the history.
		//
		if (m_cFilter.GetCount() > 0)
		{
			m_cFilter.GetLBText(0, m_szNameFilter);
			m_cFilter.SetCurSel(0);
		}
	}

	m_cFilter.SetWindowText(m_szNameFilter);
	m_cTextureWindow.SetNameFilter(m_szNameFilter);

	//
	// Done with the name filter; clear it for next time.
	//
	m_szNameFilter[0] = '\0';

	//
	// Add the global list of keywords to the keywords combo.
	//
	POSITION pos = g_Textures.KeywordsGetHeadPosition();
	while (pos != NULL)
	{
		char *pszKeyword = g_Textures.KeywordsGetNext(pos);
		m_cKeywords.AddString(pszKeyword);
	}

	//
	// Set the keyword filter.
	//
	m_cKeywords.SetWindowText(m_szLastKeywords);
	m_cTextureWindow.SetKeywords(m_szLastKeywords);

	m_cUsed.SetCheck(m_bUsed);

	CWinApp *pApp = AfxGetApp();
	CString str = pApp->GetProfileString(pszIniSection, "Position");
	if (!str.IsEmpty())
	{
		CRect r;
		sscanf(str, "%d %d %d %d", &r.left, &r.top, &r.right, &r.bottom);

		if (r.left < 0)
		{
			ShowWindow(SW_SHOWMAXIMIZED);
		}
		else
		{
			MoveWindow(r.left, r.top, r.right-r.left, r.bottom-r.top, FALSE);
		}
	}

	int iSize = pApp->GetProfileInt(pszIniSection, "ShowSize", 0);
	m_cSizeList.SetCurSel(iSize);
	OnSelendokTexturesize();

	if (szInitialTexture[0])
	{
		m_cTextureWindow.SelectTexture(szInitialTexture);
	}

	m_cTextureWindow.ShowWindow(SW_SHOW);

	SetTimer(1, 500, NULL);

	m_cFilter.SetFocus();

	return(FALSE);
}


//-----------------------------------------------------------------------------
// Purpose: Called when either the filter combo or the keywords combo text changes.
//-----------------------------------------------------------------------------
void CTextureBrowser::OnChangeFilterOrKeywords() 
{
	//
	// Start a timer to repaint the texture window using the new filters.
	//
	m_uLastFilterChange = time(NULL);
	m_bFilterChanged = TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnUpdateFiltersNOW() 
{
	m_uLastFilterChange = time(NULL);
	m_bFilterChanged = FALSE;

	CString str;
	int iSel = m_cFilter.GetCurSel();
	m_cFilter.GetLBText(iSel, str);
	m_cTextureWindow.SetNameFilter(str);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnUpdateKeywordsNOW() 
{
	m_uLastFilterChange = time(NULL);
	m_bFilterChanged = FALSE;

	CString str;
	int iSel = m_cKeywords.GetCurSel();
	m_cKeywords.GetLBText(iSel, str);
	m_cTextureWindow.SetKeywords(str);
}


//-----------------------------------------------------------------------------
// Purpose: Timer used to control updates when the filter terms change.
// Input  : nIDEvent - 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnTimer(UINT nIDEvent) 
{
	if (!m_bFilterChanged)
	{
		return;
	}

	if ((time(NULL) - m_uLastFilterChange) > 0)
	{
		KillTimer(nIDEvent);
		m_bFilterChanged = FALSE;

		m_cTextureWindow.EnableUpdate(false);

		CString str;
		m_cFilter.GetWindowText(str);
		m_cTextureWindow.SetNameFilter(str);

		m_cTextureWindow.EnableUpdate(true);

		m_cKeywords.GetWindowText(str);
		m_cTextureWindow.SetKeywords(str);

		SetTimer(nIDEvent, 500, NULL);
	}

	CDialog::OnTimer(nIDEvent);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wParam - 
//			lParam - 
// Output : LRESULT
//-----------------------------------------------------------------------------
LRESULT CTextureBrowser::OnTextureWindowDblClk(WPARAM wParam, LPARAM lParam)
{
	WriteSettings();
	SaveAndExit();
	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wParam - 
//			lParam - 
// Output : LRESULT
//-----------------------------------------------------------------------------
LRESULT CTextureBrowser::OnTexturewindowSelchange(WPARAM wParam, LPARAM lParam)
{
	IEditorTexture *pTex = g_Textures.FindActiveTexture(m_cTextureWindow.szCurTexture);
	CString str;
	char szName[MAX_PATH];

	if (pTex != NULL)
	{
		// create description of texture
		str.Format("%dx%d", pTex->GetWidth(), pTex->GetHeight());
		pTex->GetShortName(szName);
	}
	else
	{
		szName[0] = '\0';
	}

	m_cCurName.SetWindowText(szName);
	m_cCurDescription.SetWindowText(str);

	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::WriteSettings()
{
	// write position information
	CWinApp *pApp = AfxGetApp();
	CString str;
	CRect r;
	GetWindowRect(r);
	str.Format("%d %d %d %d", r.left, r.top, r.right, r.bottom);
	pApp->WriteProfileString(pszIniSection, "Position", str);
	pApp->WriteProfileInt(pszIniSection, "ShowSize", m_cSizeList.GetCurSel());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::SaveAndExit()
{
	// save current filter string
	CString str;
	m_cFilter.GetWindowText(str);

	for(int i = 0; i < m_nFilterHistory; i++)
	{
		if(!m_FilterHistory[i].CompareNoCase(str))
			break;
	}

	if(i != m_nFilterHistory)	// delete first
	{
		m_FilterHistory.RemoveAt(i);
		--m_nFilterHistory;
	}
	
	m_FilterHistory.InsertAt(0, str);
	++m_nFilterHistory;

	m_cKeywords.GetWindowText(m_szLastKeywords, sizeof(m_szLastKeywords));

	EndDialog(IDOK);
}


//-----------------------------------------------------------------------------
// Purpose: Sets a name filter that will override whatever is in the history
//			for this browser session.
//-----------------------------------------------------------------------------
void CTextureBrowser::SetFilter(const char *pszFilter)
{
	if (pszFilter)
	{
		strcpy(m_szNameFilter, pszFilter);
	}
	else
	{
		m_szNameFilter[0] = '\0';
	}
}


//-----------------------------------------------------------------------------
// Filter buttons
//-----------------------------------------------------------------------------

void CTextureBrowser::OnFilterOpaque(void)
{
	bool checked = m_FilterOpaque.GetCheck( ) != 0;
	m_cTextureWindow.SetTypeFilter( CTextureWindow::TYPEFILTER_OPAQUE, checked ); 
}

void CTextureBrowser::OnFilterTranslucent(void)
{
	bool checked = m_FilterTranslucent.GetCheck( ) != 0;
	m_cTextureWindow.SetTypeFilter( CTextureWindow::TYPEFILTER_TRANSLUCENT, checked ); 
}

void CTextureBrowser::OnFilterSelfIllum(void)
{
	bool checked = m_FilterSelfIllum.GetCheck( ) != 0;
	m_cTextureWindow.SetTypeFilter( CTextureWindow::TYPEFILTER_SELFILLUM, checked ); 
}

void CTextureBrowser::OnFilterEnvmask(void)
{
	bool checked = m_FilterEnvMask.GetCheck( ) != 0;
	m_cTextureWindow.SetTypeFilter( CTextureWindow::TYPEFILTER_ENVMASK, checked ); 
}

void CTextureBrowser::OnShowErrors(void)
{
	bool checked = m_ShowErrors.GetCheck( ) != 0;
	m_cTextureWindow.ShowErrors( checked ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextureBrowser::OnMark(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		pDoc->ReplaceTextures(m_cTextureWindow.szCurTexture, "", TRUE, 0x100, FALSE);
		EndDialog(IDOK);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Invokes the replace texture dialog.
//-----------------------------------------------------------------------------
void CTextureBrowser::OnReplace(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if(!pDoc)
		return;

	CReplaceTexDlg dlg(pDoc->Selection_GetCount());

	dlg.m_strFind = m_cTextureWindow.szCurTexture;

	if(dlg.DoModal() != IDOK)
		return;
	
	// mark undo position
	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Replace Textures");

	if(dlg.m_bMarkOnly)
	{
		pDoc->SelectObject(NULL, CMapDoc::scClear);	// clear selection first
	}

	pDoc->ReplaceTextures(dlg.m_strFind, dlg.m_strReplace,
		dlg.m_iSearchAll, dlg.m_iAction | (dlg.m_bMarkOnly ? 0x100 : 0),
		dlg.m_bHidden);

	//EndDialog(IDOK);

	if (m_bUsed)
	{
		SetUsed(TRUE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the texture format for browsing. Only textures of the given
//			format will be visible in the browse window. By default, this
//			format will be the same as the current active texture format.
// Input  : eTextureFormat - Texture format to use for browsing.
//-----------------------------------------------------------------------------
void CTextureBrowser::SetTextureFormat(TEXTUREFORMAT eTextureFormat)
{
	m_cTextureWindow.SetTextureFormat(eTextureFormat);
}


