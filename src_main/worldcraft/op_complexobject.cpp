//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile: OP_ComplexObject.cpp $
// $Date: 8/03/99 6:57p $
//
//-----------------------------------------------------------------------------
// $Log: /Src/Worldcraft/OP_ComplexObject.cpp $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "OP_ComplexObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COP_ComplexObject property page

IMPLEMENT_DYNCREATE(COP_ComplexObject, CObjectPage)

COP_ComplexObject::COP_ComplexObject() : CObjectPage(COP_ComplexObject::IDD)
{
	//{{AFX_DATA_INIT(COP_ComplexObject)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

COP_ComplexObject::~COP_ComplexObject()
{
}

void COP_ComplexObject::DoDataExchange(CDataExchange* pDX)
{
	CObjectPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COP_ComplexObject)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COP_ComplexObject, CObjectPage)
	//{{AFX_MSG_MAP(COP_ComplexObject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COP_ComplexObject message handlers

BOOL COP_ComplexObject::OnInitDialog() 
{
//	CObjectPage::OnInitDialog();
	
	VERIFY(m_ParamList.SubclassDlgItem(IDC_PARAMETERS, this));
	
	static char szBuf[128];
	static int iInt;
	static int iYesNo;
	static int iOnOff;
	static int iChoice;

	static CStringArray ChoiceArray;
	
	static BOOL bInit = FALSE;

	if(!bInit)
	{
		ChoiceArray.Add("red");
		ChoiceArray.Add("blue");
		ChoiceArray.Add("green");
		ChoiceArray.Add("what a maroon!");
	}

	strcpy(szBuf, "I am krad");

	m_ParamList.AddItem("Choices", lbeChoices, &iChoice);
	m_ParamList.SetItemChoices(0, &ChoiceArray, 0);
	m_ParamList.AddItem("String", lbeString, szBuf);
	m_ParamList.AddItem("Integer", lbeInteger, &iInt);
	m_ParamList.AddItem("Yes/No", lbeYesNo, &iYesNo);
	m_ParamList.AddItem("On/Off", lbeOnOff, &iOnOff);

	bInit = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void COP_ComplexObject::UpdateData(int Mode, PVOID pData)
{
	return;

	if(!IsWindow(m_hWnd) || !pData)
	{
		return;
	}
}
