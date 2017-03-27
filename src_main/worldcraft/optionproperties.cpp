// OptionProperties.cpp : implementation file
//

#include "stdafx.h"
#include "Worldcraft.h"
#include "OptionProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionProperties

IMPLEMENT_DYNAMIC(COptionProperties, CPropertySheet)

COptionProperties::COptionProperties(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	DoStandardInit();
}

COptionProperties::COptionProperties(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	DoStandardInit();
}

COptionProperties::~COptionProperties()
{
}


BEGIN_MESSAGE_MAP(COptionProperties, CPropertySheet)
	//{{AFX_MSG_MAP(COptionProperties)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionProperties message handlers

void COptionProperties::DoStandardInit()
{
	AddPage(&Configs);
	AddPage(&General);
	AddPage(&View2D);
	AddPage(&View3D);
	AddPage(&Textures);
	AddPage(&Build);
}

void COptionProperties::OnClose() 
{
	CPropertySheet::OnClose();
}
