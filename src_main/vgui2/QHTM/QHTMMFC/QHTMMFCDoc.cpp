// QHTMMFCDoc.cpp : implementation of the CQHTMMFCDoc class
//

#include "stdafx.h"
#include "QHTMMFC.h"

#include "QHTMMFCDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCDoc

IMPLEMENT_DYNCREATE(CQHTMMFCDoc, CDocument)

BEGIN_MESSAGE_MAP(CQHTMMFCDoc, CDocument)
	//{{AFX_MSG_MAP(CQHTMMFCDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCDoc construction/destruction

CQHTMMFCDoc::CQHTMMFCDoc()
{
	// TODO: add one-time construction code here

}

CQHTMMFCDoc::~CQHTMMFCDoc()
{
}

BOOL CQHTMMFCDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}


BOOL CQHTMMFCDoc::OnOpenDocument(LPCTSTR filename)
{
	// This is handled in the view. Return true.
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCDoc serialization

void CQHTMMFCDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCDoc diagnostics

#ifdef _DEBUG
void CQHTMMFCDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CQHTMMFCDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CQHTMMFCDoc commands
