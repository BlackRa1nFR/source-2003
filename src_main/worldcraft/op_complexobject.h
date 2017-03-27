// OP_ComplexObject.h : header file
//

#include "ListBoxEx.h"
#include "ObjectPage.h"

/////////////////////////////////////////////////////////////////////////////
// COP_ComplexObject dialog

class COP_ComplexObject : public CObjectPage
{
	DECLARE_DYNCREATE(COP_ComplexObject)

// Construction
public:
	COP_ComplexObject();
	~COP_ComplexObject();

	void UpdateData(int Mode, PVOID pData);

// Dialog Data
	//{{AFX_DATA(COP_ComplexObject)
	enum { IDD = IDD_OBJPAGE_COMPLEX };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COP_ComplexObject)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Attributes
protected:
	CListBoxEx m_ParamList;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COP_ComplexObject)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
