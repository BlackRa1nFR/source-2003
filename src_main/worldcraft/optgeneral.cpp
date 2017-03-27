//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
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
#include "Worldcraft.h"
#include "OPTGeneral.h"
#include "Options.h"


#pragma warning(disable:4244)


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(COPTGeneral, CPropertyPage)


BEGIN_MESSAGE_MAP(COPTGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(COPTGeneral)
	ON_BN_CLICKED(IDC_INDEPENDENTWINDOWS, OnIndependentwindows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COPTGeneral::COPTGeneral(void) : CPropertyPage(COPTGeneral::IDD)
{
	//{{AFX_DATA_INIT(COPTGeneral)
	m_iUndoLevels = 0;
	//}}AFX_DATA_INIT
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COPTGeneral::~COPTGeneral(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDX - 
//-----------------------------------------------------------------------------
void COPTGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COPTGeneral)
	DDX_Control(pDX, IDC_LOADWINPOSITIONS, m_cLoadWinPos);
	DDX_Control(pDX, IDC_INDEPENDENTWINDOWS, m_cIndependentWin);
	DDX_Control(pDX, IDC_UNDOSPIN, m_UndoSpin);
	DDX_Text(pDX, IDC_UNDO, m_iUndoLevels);
	DDX_Check(pDX, IDC_STRETCH_ARCH, Options.general.bStretchArches);
	DDX_Check(pDX, IDC_GROUPWHILEIGNOREGROUPS, Options.general.bGroupWhileIgnore);
	DDX_Check(pDX, IDC_INDEPENDENTWINDOWS, Options.general.bIndependentwin);
	DDX_Check(pDX, IDC_LOADWINPOSITIONS, Options.general.bLoadwinpos);
	//}}AFX_DATA_MAP
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL COPTGeneral::OnInitDialog(void)
{
	m_iUndoLevels = Options.general.iUndoLevels;

	CPropertyPage::OnInitDialog();

	// set undo range
	m_UndoSpin.SetRange(5, 999);

	OnIndependentwindows();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL COPTGeneral::OnApply(void)
{
	Options.general.iUndoLevels = m_iUndoLevels;
	Options.PerformChanges(COptions::secGeneral);	

	return(CPropertyPage::OnApply());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COPTGeneral::OnIndependentwindows(void)
{
	m_cLoadWinPos.EnableWindow(m_cIndependentWin.GetCheck());
}

