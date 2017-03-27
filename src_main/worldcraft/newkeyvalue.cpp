// NewKeyValue.cpp : implementation file
//

#include "stdafx.h"
#include "Worldcraft.h"
#include "NewKeyValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewKeyValue dialog


CNewKeyValue::CNewKeyValue(CWnd* pParent /*=NULL*/)
	: CDialog(CNewKeyValue::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewKeyValue)
	m_Key = _T("");
	m_Value = _T("");
	//}}AFX_DATA_INIT
}


void CNewKeyValue::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewKeyValue)
	DDX_Text(pDX, IDC_KEY, m_Key);
	DDV_MaxChars(pDX, m_Key, 31);
	DDX_Text(pDX, IDC_VALUE, m_Value);
	DDV_MaxChars(pDX, m_Value, 80);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewKeyValue, CDialog)
	//{{AFX_MSG_MAP(CNewKeyValue)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewKeyValue message handlers
