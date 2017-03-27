
// The ITweakPropDlg class is given a RecvTable, and sets up a UI for users
// to modify properties in the RecvTable.

#ifndef TWEAKPROPDLG_H
#define TWEAKPROPDLG_H


class RecvTable;


class ITweakPropDlg
{
public:
	virtual			~ITweakPropDlg() {}
	
	// Delete the object.
	virtual void	Release() = 0;

	// Change the object that the dialog is modifying.
	// Also set all the values in the object.
	virtual void	SetObj(void *pObj) = 0;
};

ITweakPropDlg* CreateTweakPropDlg(
	RecvTable *pTable, 
	void *pObj, 
	char *pTitle,
	void *hParentWindow,
	void *hInstance);


#endif


