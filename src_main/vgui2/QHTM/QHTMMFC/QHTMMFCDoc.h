// QHTMMFCDoc.h : interface of the CQHTMMFCDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_QHTMMFCDOC_H__2398F33E_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_)
#define AFX_QHTMMFCDOC_H__2398F33E_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CQHTMMFCDoc : public CDocument
{
protected: // create from serialization only
	CQHTMMFCDoc();
	DECLARE_DYNCREATE(CQHTMMFCDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQHTMMFCDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR filename);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CQHTMMFCDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CQHTMMFCDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QHTMMFCDOC_H__2398F33E_DAB7_11D2_9E5A_0020AF5C17CE__INCLUDED_)
