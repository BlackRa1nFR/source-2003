//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implements a dialog that allows the user to type in a brush number
//			and entity number for selection by ordinal.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "worldcraft.h"
#include "GotoBrushDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CGotoBrushDlg, CDialog)
	//{{AFX_MSG_MAP(CGotoBrushDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pParent /*=NULL*/ - 
// Output : 
//-----------------------------------------------------------------------------
CGotoBrushDlg::CGotoBrushDlg(CWnd *pParent)
	: CDialog(CGotoBrushDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGotoBrushDlg)
	m_nEntityNumber = 0;
	m_nBrushNumber = 0;
	m_bVisiblesOnly = FALSE;
	//}}AFX_DATA_INIT
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDX - 
//-----------------------------------------------------------------------------
void CGotoBrushDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGotoBrushDlg)
	DDX_Text(pDX, IDC_ENTITY_NUMBER, m_nEntityNumber);
	DDX_Text(pDX, IDC_BRUSH_NUMBER, m_nBrushNumber);
	DDX_Check(pDX, IDC_SEARCH_VISIBLE_BRUSHES_ONLY, m_bVisiblesOnly);
	//}}AFX_DATA_MAP
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CGotoBrushDlg::OnOK() 
{
	CDialog::OnOK();
}

