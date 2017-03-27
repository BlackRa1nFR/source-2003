//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "EntityHelpDlg.h"
#include "History.h"
#include "MainFrm.h"
#include "MapWorld.h"
#include "OP_Entity.h"
#include "CustomMessages.h"
#include "NewKeyValue.h"
#include "GlobalFunctions.h"
#include "GameData.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "ObjectProperties.h"
#include "TargetNameCombo.h"
#include "TextureBrowser.h"
#include "TextureSystem.h"
#include "ToolPickAngles.h"
#include "ToolPickEntity.h"
#include "ToolPickFace.h"
#include "ToolManager.h"


extern GameData *pGD;		// current game data


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define IDC_SMARTCONTROL 1


static KeyValues kvClipboard;
static BOOL bKvClipEmpty = TRUE;


class CMyEdit : public CEdit
{
	public:
		void SetParentPage(COP_Entity* pPage)
		{
			m_pParent = pPage;
		}

		afx_msg void OnChar(UINT, UINT, UINT);

		COP_Entity *m_pParent;
		DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CMyEdit, CEdit)
	ON_WM_CHAR()
END_MESSAGE_MAP()
	

class CMyComboBox : public CComboBox
{
	public:
		void SetParentPage(COP_Entity* pPage)
		{
			m_pParent = pPage;
		}

		afx_msg void OnChar(UINT, UINT, UINT);

		COP_Entity *m_pParent;
		DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CMyComboBox, CComboBox)
	ON_WM_CHAR()
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Called by the angles picker tool when a target point is picked. This
//			stuffs the angles into the smartedit control so that the entity
//			points at the target position.
//-----------------------------------------------------------------------------
void CPickAnglesTarget::OnNotifyPickAngles(const Vector &vecPos)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (!pDoc)
	{
		return;
	}

	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Point At");

	//
	// Update the edit control text with the entity name. This text will be
	// stuffed into the local keyvalue storage in OnChangeSmartControl.
	//
	POSITION pos = m_pDlg->m_pObjectList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = m_pDlg->m_pObjectList->GetNext(pos);
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
		ASSERT(pEntity != NULL);
		if (pEntity != NULL)
		{
			GetHistory()->Keep(pEntity);

			// Calculate the angles to point this entity at the chosen spot.
			Vector vecOrigin;
			pEntity->GetOrigin(vecOrigin);
			Vector vecForward = vecPos - vecOrigin;

			QAngle angFace;
			VectorAngles(vecForward, angFace);

			// HACK: lights negate pitch
			if (pEntity->GetClassName() && (!strnicmp(pEntity->GetClassName(), "light_", 6)))
			{
				angFace[PITCH] *= -1;
			}

			// Update the edit control with the calculated angles.
			char szAngles[80];	
			sprintf(szAngles, "%.0f %.0f %.0f", angFace[PITCH], angFace[YAW], angFace[ROLL]);
			pEntity->SetKeyValue("angles", szAngles);

			// HACK: lights have a separate "pitch" key
			if (pEntity->GetClassName() && (!strnicmp(pEntity->GetClassName(), "light_", 6)))
			{
				char szPitch[20];
				sprintf(szPitch, "%.0f", angFace[PITCH]);
				pEntity->SetKeyValue("pitch", szPitch);
			}

			// FIXME: this should be called automatically, but it isn't
			m_pDlg->OnChangeSmartcontrol();
		}
	}
	m_pDlg->StopPicking();

	GetMainWnd()->pObjectProperties->RefreshData();		
}


//-----------------------------------------------------------------------------
// Purpose: Called by the entity picker tool when an entity is picked. This
//			stuffs the entity name into the smartedit control.
//-----------------------------------------------------------------------------
void CPickEntityTarget::OnNotifyPickEntity(CToolPickEntity *pTool)
{
	//
	// Update the edit control text with the entity name. This text will be
	// stuffed into the local keyvalue storage in OnChangeSmartControl.
	//
	CMapEntityList Full;
	CMapEntityList Partial;
	pTool->GetSelectedEntities(Full, Partial);
	CMapEntity *pEntity = Full.Element(0);
	if (pEntity)
	{
		const char *pszName = pEntity->GetKeyValue("targetname");
		if (!pszName)
		{
			pszName = "";
		}

		// dvs: HACK: the smart control should be completely self-contained, the dialog
		//		should only set the type of the edited variable, then just set & get text
		CTargetNameComboBox *pCombo = dynamic_cast <CTargetNameComboBox *>(m_pDlg->m_pSmartControl);
		ASSERT(pCombo);
		if (pCombo)
		{
			pCombo->SetTargetComboText(pszName);
		}
		else
		{
			m_pDlg->m_pSmartControl->SetWindowText(pszName);
		}

		// FIXME: this should be called automatically, but it isn't
		m_pDlg->OnChangeSmartcontrol();
	}

	m_pDlg->StopPicking();
}


//-----------------------------------------------------------------------------
// Purpose: Called by the face picker tool when the face selection changes.
// Input  : pTool - The face picker tool that is notifying us.
//-----------------------------------------------------------------------------
void CPickFaceTarget::OnNotifyPickFace(CToolPickFace *pTool)
{
	m_pDlg->UpdatePickFaceText(pTool);
}


BEGIN_MESSAGE_MAP(COP_Entity, CObjectPage)
	//{{AFX_MSG_MAP(COP_Entity)
	ON_LBN_SELCHANGE(IDC_KEYVALUES, OnSelchangeKeyvalues)
	ON_BN_CLICKED(IDC_ADDKEYVALUE, OnAddkeyvalue)
	ON_BN_CLICKED(IDC_REMOVEKEYVALUE, OnRemovekeyvalue)
	ON_BN_CLICKED(IDC_SMARTEDIT, OnSmartedit)
	ON_CBN_SELCHANGE(IDC_CLASS, OnSelchangeClass)
	ON_EN_CHANGE(IDC_VALUE, OnChangeKeyorValue)
	ON_BN_CLICKED(IDC_COPY, OnCopy)
	ON_BN_CLICKED(IDC_PASTE, OnPaste)
	ON_BN_CLICKED(IDC_PICKCOLOR, OnPickColor)
	ON_EN_SETFOCUS(IDC_KEY, OnSetfocusKey)
	ON_EN_KILLFOCUS(IDC_KEY, OnKillfocusKey)
	ON_MESSAGE(ABN_CHANGED, OnChangeAngleBox)
	ON_CBN_SELCHANGE(IDC_SMARTCONTROL, OnChangeSmartcontrolSel)
	ON_CBN_EDITUPDATE(IDC_SMARTCONTROL, OnChangeSmartcontrol)
	ON_EN_CHANGE(IDC_SMARTCONTROL, OnChangeSmartcontrol)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_MARK, OnMark)
	ON_BN_CLICKED(IDC_PICK_FACES, OnPickFaces)
	ON_BN_CLICKED(IDC_ENTITY_HELP, OnEntityHelp)
	ON_BN_CLICKED(IDC_PICK_ANGLES, OnPickAngles)
	ON_BN_CLICKED(IDC_PICK_ENTITY, OnPickEntity)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE(COP_Entity, CObjectPage)


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COP_Entity::COP_Entity(void)
	: CObjectPage(COP_Entity::IDD)
{
	//{{AFX_DATA_INIT(COP_Entity)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bPicking = false;
	m_bChangingKeyName = false;

	m_bIgnoreKVChange = false;
	m_bSmartedit = true;
	m_pSmartControl = NULL;
	m_pObjClass = NULL;
	m_eEditType = ivString;

	m_nNewKeyCount = 0;

	m_bEnableControlUpdate = true;

	m_pEditObjectRuntimeClass = RUNTIME_CLASS(editCEditGameClass);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
COP_Entity::~COP_Entity(void)
{
	if (m_pSmartControl != NULL)
	{
		m_pSmartControl->DestroyWindow();
		delete m_pSmartControl;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDX - 
//-----------------------------------------------------------------------------
void COP_Entity::DoDataExchange(CDataExchange* pDX)
{
	CObjectPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COP_Entity)
	DDX_Control(pDX, IDC_VALUE, m_cValue);
	DDX_Control(pDX, IDC_KEYVALUES, m_VarList);
	DDX_Control(pDX, IDC_KEY, m_cKey);
	DDX_Control(pDX, IDC_ENTITY_COMMENTS, m_Comments);
	DDX_Control(pDX, IDC_KEYVALUE_HELP, m_KeyValueHelpText);
	//}}AFX_DATA_MAP
}


//-----------------------------------------------------------------------------
// Purpose: Adds an object's keys to our list of keys. If a given key is already
//			in the list, it is either ignored or set to a "different" if the value
//			is different from the value in our list.
// Input  : pEdit - Object whose keys are to be added to our list.
//-----------------------------------------------------------------------------
void COP_Entity::MergeObjectKeyValues(CEditGameClass *pEdit)
{
	int nKeyValues = pEdit->GetKeyValueCount();
	for (int i = 0; i < nKeyValues; i++)
	{
		LPCTSTR pszCurValue = m_kv.GetValue(pEdit->GetKey(i));
		if (pszCurValue == NULL)
		{
			//
			// Doesn't exist yet - set current value.
			//
			m_kv.SetValue(pEdit->GetKey(i), pEdit->GetKeyValue(i));
		}
		else if (strcmp(pszCurValue, pEdit->GetKeyValue(i)))
		{
			//
			// Already present - we need to merge the value with the existing data.
			//
			MergeKeyValue(pEdit->GetKey(i));
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates the dialog's keyvalue data with a given keyvalue. If the
//			data can be merged in with existing data in a meaningful manner,
//			that will be done. If not, VALUE_DIFFERENT_STRING will be set to
//			indicate that not all objects have the same value for the key.
// Input  : pszKey - 
//-----------------------------------------------------------------------------
void COP_Entity::MergeKeyValue(char const *pszKey)
{
	ASSERT(pszKey);
	if (!pszKey)
	{
		return;
	}

	bool bHandled = false;

	if (m_pObjClass != NULL)
	{
		GDinputvariable *pVar = m_pObjClass->VarForName(pszKey);
		if (pVar != NULL)
		{
			switch (pVar->GetType())
			{
				case ivSideList:
				{
					//
					// Merging sidelist keys is a little complicated. We build a string
					// representing the merged sidelists.
					//
					CMapFaceIDList FaceIDListFull;
					CMapFaceIDList FaceIDListPartial;

					GetFaceIDListsForKey(FaceIDListFull, FaceIDListPartial, pszKey);

					char szValue[KEYVALUE_MAX_VALUE_LENGTH];
					CMapWorld::FaceID_FaceIDListsToString(szValue, sizeof(szValue), &FaceIDListFull, &FaceIDListPartial);
					m_kv.SetValue(pszKey, szValue);
			
					bHandled = true;
					break;
				}

				case ivAngle:
				{
					//
					// We can't merge angles, so set the appropriate angles control to "different".
					// We'll catch the main angles control below, since it's supported even
					// for objects of an unknown class.
					//
					if (stricmp(pVar->GetName(), "angles"))
					{
						m_SmartAngle.SetDifferent(true);
					}
					break;
				}
			}
		}
	}

	if (!bHandled)
	{
		//
		// Can't merge with current value - show a "different" string.
		//
		m_kv.SetValue(pszKey, VALUE_DIFFERENT_STRING);

		if (!stricmp(pszKey, "angles"))
		{
			// We can't merge angles, so set the main angles control to "different".
			m_Angle.SetDifferent(true);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Mode - 
//			pData - 
//-----------------------------------------------------------------------------
void COP_Entity::UpdateData(int Mode, PVOID pData)
{
	if (!IsWindow(m_hWnd))
	{
		return;
	}

	if (GetFocus() == &m_cKey)
	{
		OnKillfocusKey();
	}

	if (Mode == LoadFinished)
	{
		m_kvAdded.RemoveAll();
		SetupForMode();
		return;
	}

	if (!pData)
	{
		return;
	}

	CEditGameClass *pEdit = (CEditGameClass *)pData;
	char szBuf[KEYVALUE_MAX_VALUE_LENGTH];

	if (Mode == LoadFirstData)
	{
		LoadClassList();

		m_nNewKeyCount = 1;

		QAngle vecAngles;
		pEdit->GetAngles(vecAngles);
		m_Angle.SetAngles(vecAngles, false);
		m_Angle.SetDifferent(false);

		#ifdef NAMES
		if(pEdit->pClass)
		{
			sprintf(szBuf, "%s (%s)", pEdit->pClass->GetDescription(), pEdit->pClass->GetName());
		}
		else
		#endif

		strcpy(szBuf, pEdit->GetClassName());
		m_cClasses.SetWindowText(szBuf);

		//
		// Can't change the class of the worldspawn entity.
		//
		if (pEdit->IsClass("worldspawn"))
		{
			m_cClasses.EnableWindow(FALSE);
		}
		else
		{
			m_cClasses.EnableWindow(TRUE);
		}

		//
		// Get the comments text from the entity.
		//
		m_Comments.SetWindowText(pEdit->GetComments());

		//
		// Add entity's keys to our local storage
		//
		m_kv.RemoveAll();
		int nKeyValues = pEdit->GetKeyValueCount();
		for (int i = 0; i < nKeyValues; i++)
		{
			const char *pszKey = pEdit->GetKey(i);
			const char *pszValue = pEdit->GetKeyValue(i);
			if ((pszKey != NULL) && (pszValue != NULL))
			{
				m_kv.SetValue(pszKey, pszValue);
			}
		}
		ASSERT(m_kv.GetCount() == nKeyValues);

		m_pObjClass = pGD->ClassForName(pEdit->GetClassName());
		UpdateClass(pEdit->GetClassName());

		SetCurKey(m_strLastKey);
	}
	else if (Mode == LoadData)
	{
		//
		// Not first data - merge with current stuff.
		//

		//
		// Deal with class name.
		//
		m_cClasses.GetWindowText(szBuf, 128);
		if (strcmpi(szBuf, pEdit->GetClassName()))
		{
			//
			// Not the same - set class to be blank and 
			// disable smartedit.
			//
			m_cClasses.SetWindowText("");
			m_pObjClass = NULL;
			UpdateClass("");
		}

		//
		// Mark the comments field as "(different)" if it isn't the same as this entity's
		// comments.
		//
		char szComments[1024];
		m_Comments.GetWindowText(szComments, sizeof(szComments));
		if (strcmp(szComments, pEdit->GetComments()) != 0)
		{
			m_Comments.SetWindowText(VALUE_DIFFERENT_STRING);
		}

		MergeObjectKeyValues(pEdit);
		SetCurKey(m_strLastKey);
	}
	else
	{
		ASSERT(FALSE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Stops entity, face, or angles picking.
//-----------------------------------------------------------------------------
void COP_Entity::StopPicking(void)
{
	if (m_bPicking)
	{
		m_bPicking = false;
		g_pToolManager->SetTool(m_ToolPrePick);

		//
		// Stop angles picking if we are doing so.
		//
		CButton *pButton = (CButton *)GetDlgItem(IDC_PICK_ANGLES);
		if (pButton)
		{
			pButton->SetCheck(0);
		}

		//
		// Stop entity picking if we are doing so.
		//
		pButton = (CButton *)GetDlgItem(IDC_PICK_ENTITY);
		if (pButton)
		{
			pButton->SetCheck(0);
		}

		//
		// Stop face picking if we are doing so.
		//
		pButton = (CButton *)GetDlgItem(IDC_PICK_FACES);
		if (pButton)
		{
			pButton->SetCheck(0);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called manually from CObjectProperties::OnApply because the Apply
//			button is implemented in a nonstandard way. I'm not sure why.
//-----------------------------------------------------------------------------
BOOL COP_Entity::OnApply(void)
{
	StopPicking();
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			pszKey - 
//			pszValue - 
//-----------------------------------------------------------------------------
void COP_Entity::ApplyKeyValueToObject(CEditGameClass *pObject, const char *pszKey, const char *pszValue)
{
	GDclass *pClass = pObject->GetClass();
	if (pClass != NULL)
	{
		GDinputvariable *pVar = pClass->VarForName(pszKey);
		if (pVar != NULL)
		{
			if ((pVar->GetType() == ivSideList) || (pVar->GetType() == ivSide))
			{
				CMapWorld *pWorld = GetActiveWorld();

				//
				// Get the face list currently set in this keyvalue.
				//
				CMapFaceIDList CurFaceList;
				const char *pszCurVal = pObject->GetKeyValue(pszKey);
				if (pszCurVal != NULL)
				{
					pWorld->FaceID_StringToFaceIDLists(&CurFaceList, NULL, pszCurVal);
				}

				//
				// Build the face list to apply. Only include the faces that are:
				//
				// 1. In the full selection list (outside the parentheses).
				// 2. In the partial selection set (inside the parentheses) AND are in the
				//    original face list.
				//
				// FACEID TODO: Optimize so that we only call StringToFaceList once per keyvalue
				//		 instead of once per keyvalue per entity being applied to.
				CMapFaceIDList FullFaceList;
				CMapFaceIDList PartialFaceList;
				pWorld->FaceID_StringToFaceIDLists(&FullFaceList, &PartialFaceList, pszValue);
				
				CMapFaceIDList KeepFaceList;
				for (int i = 0; i < PartialFaceList.Count(); i++)
				{
					int nFace = PartialFaceList.Element(i);

					if (CurFaceList.Find(nFace) != -1)
					{	
						KeepFaceList.AddToTail(nFace);
					}
				}

				FullFaceList.AddVectorToTail(KeepFaceList);

				char szSetValue[KEYVALUE_MAX_VALUE_LENGTH];
				CMapWorld::FaceID_FaceIDListsToString(szSetValue, sizeof(szSetValue), &FullFaceList, NULL);

				pObject->SetKeyValue(pszKey, szSetValue);
				return;
			}
		}
	}

	pObject->SetKeyValue(pszKey, pszValue);
}


//-----------------------------------------------------------------------------
// Purpose: Called by the sheet to let the page remember its state before a
//			refresh of the data.
//-----------------------------------------------------------------------------
void COP_Entity::RememberState(void)
{
	GetCurKey(m_strLastKey);
}


//-----------------------------------------------------------------------------
// Purpose: Saves the dialog data into the objects being edited.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool COP_Entity::SaveData(void)
{
	RememberState();

	//
	// Apply the dialog data to all the objects being edited.
	//
	POSITION pos = m_pObjectList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = m_pObjectList->GetNext(pos);
		CEditGameClass *pEdit = dynamic_cast <CEditGameClass *>(pObject);
		ASSERT(pEdit != NULL);

		if (pEdit != NULL)
		{
			char szBuf[KEYVALUE_MAX_VALUE_LENGTH];

			//
			// Handle the angle control.
			//
			/* dvs: shouldn't need this anymore -- handled by OnChangeAngleBox now
			QAngle vecAngles;
			if (m_Angle.GetAngles(vecAngles))
			{
				QAngle vecEditAngles;
				pEdit->GetAngles(vecEditAngles);

				//
				// Set the angles if the angle control has changed.
				//
				if (vecEditAngles != vecAngles)
				{
					pEdit->SetAngles(vecAngles);

					//
					// Set the angles key in our local storage so that it doesn't
					// stomp the edit object's angles key (set by above SetAngles call).
					//
					const char *pszAngles = pEdit->GetKeyValue("angles");
					m_kv.SetValue("angles", pszAngles);
				}
			}*/

			RemoveBlankKeys();

			//
			// Give keys back to object. For every key in our local storage that is
			// also found in the list of added keys, set the key value in the edit
			// object(s).
			//
			for (int i = 0; i < m_kv.GetCount(); i++)
			{
				MDkeyvalue &kvCur = m_kv.GetKeyValue(i);
				const char *pszAddedKeyValue = m_kvAdded.GetValue(kvCur.szKey);
				if (pszAddedKeyValue != NULL)
				{
					//
					// Don't store keys with multiple/undefined values.
					//
					if (strcmp(kvCur.szValue, VALUE_DIFFERENT_STRING))
					{
						ApplyKeyValueToObject(pEdit, kvCur.szKey, kvCur.szValue);
					}
				}
			}
			
			//
			// All keys in the object should also be found in local storage,
			// unless the user deleted them. If there are any such extraneous
			// keys, get rid of them. Go from the top down because otherwise
			// deleting messes up our iteration.
			//
			int nKeyValues = pEdit->GetKeyValueCount();
			for (i = nKeyValues - 1; i >= 0; i--)
			{
				//
				// If this key is in not in our local storage, delete it from the object.
				//
				if (!m_kv.GetValue(pEdit->GetKey(i)))
				{
					pEdit->DeleteKeyValue(pEdit->GetKey(i));
				}
			}

			//
			// Store class.
			//
			m_cClasses.GetWindowText(szBuf, 128);
			if (szBuf[0])
			{
				pEdit->SetClass(szBuf);
			}
			
			//
			// Store the entity comments.
			//
			char szComments[1024];
			szComments[0] = '\0';
			m_Comments.GetWindowText(szComments, sizeof(szComments));
			if (strcmp(szComments, VALUE_DIFFERENT_STRING) != 0)
			{
				pEdit->SetComments(szComments);
			}
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Sets up the current mode (smartedit/non) after loading an object
//			or toggling the SmartEdit button.
//-----------------------------------------------------------------------------
void COP_Entity::SetupForMode(void)
{
	m_VarList.ResetContent();

	char szBuf[KEYVALUE_MAX_KEY_LENGTH + KEYVALUE_MAX_VALUE_LENGTH + 1];

	if (m_pSmartControl)
	{
		m_pSmartControl->DestroyWindow();
		delete m_pSmartControl;
		m_pSmartControl = NULL;
	}

	if (!m_bSmartedit)
	{
		RemoveBlankKeys();

		int nKeyValues = m_kv.GetCount();
		for (int i = 0; i < nKeyValues; i++)
		{
			MDkeyvalue &KeyValue = m_kv.GetKeyValue(i);
			sprintf(szBuf, "%s\t%s", KeyValue.szKey, KeyValue.szValue);
			m_VarList.AddString(szBuf);
		}
	
		m_Angle.Enable(true);
	}
	else
	{
		// Too many entity variables! Increase GD_MAX_VARIABLES if you get this assertion.
		ASSERT(m_pObjClass->GetVariableCount() <= GD_MAX_VARIABLES);

		//
		// Add all the keys from the entity's class to the listbox. If the key is not already
		// in the entity, add it to the m_kvAdded list as well.
		//
		for (int i = 0, j = 0; i < m_pObjClass->GetVariableCount(); i++)
		{
			GDinputvariable *pVar = m_pObjClass->GetVariableAt(i);

			//
			// Spawnflags are handled separately - don't add that key.
			//
			if (strcmpi(pVar->GetName(), "spawnflags") != 0)
			{
				m_VarMap[j++] = i;

				m_VarList.AddString(pVar->GetLongName());

				int iIndex;
				LPCTSTR p = m_kv.GetValue(pVar->GetName(), &iIndex);
				if (!p)
				{
					MDkeyvalue newkv;
					pVar->ResetDefaults();
					pVar->ToKeyValue(&newkv);
					m_kv.SetValue(newkv.szKey, newkv.szValue);
					m_kvAdded.SetValue(newkv.szKey, "1");
				}
			}
		}

		//
		// If this class defines angles, enable the angles control.
		//
		if (m_pObjClass->VarForName("angles") != NULL)
		{
			m_Angle.Enable(true);
		}
		else
		{
			m_Angle.Enable(false);
		}
	}

	SetCurKey(m_strLastKey);
}


//-----------------------------------------------------------------------------
// Purpose: Removes any keys with blank values from our local keyvalue storage.
//-----------------------------------------------------------------------------
void COP_Entity::RemoveBlankKeys(void)
{
	int nKeyValues = m_kv.GetCount();
	for (int i = nKeyValues - 1; i >= 0; i--)
	{
		MDkeyvalue &KeyValue = m_kv.GetKeyValue(i);
		if (KeyValue.szValue[0] == '\0')
		{
			m_kv.RemoveKey(i);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::LoadClassList(void)
{
	CEditGameClass *pEdit = (CEditGameClass*) GetEditObject();
	int iSolidClasses = -1;

	if (pEdit->IsClass())
	{
		if (pEdit->IsSolidClass())
		{
			iSolidClasses = TRUE;
		}
		else
		{
			iSolidClasses = FALSE;
		}
	}

	// setup class list
	m_cClasses.SetRedraw(FALSE);
	m_cClasses.ResetContent();
	POSITION p = pGD->Classes.GetHeadPosition();
	CString str;
	while(p)
	{
		GDclass *pc = pGD->Classes.GetNext(p);
		if (!pc->IsBaseClass())
		{
			if (iSolidClasses == -1 || (iSolidClasses == (int)pc->IsSolidClass()))
			{
				#ifdef NAMES
				str.Format("%s (%s)", pc->GetDescription(), pc->GetName());
				#else
				str = pc->GetName();
				#endif

				if (!pc->IsClass("worldspawn"))
				{
					m_cClasses.AddString(str);
				}
			}
		}
	}
	m_cClasses.SetRedraw(TRUE);
	m_cClasses.Invalidate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BOOL COP_Entity::OnInitDialog(void)
{
	CObjectPage::OnInitDialog();

	// Hook up the main angles controls.
	m_Angle.SubclassDlgItem(IDC_ANGLEBOX, this);
	m_AngleEdit.SubclassDlgItem(IDC_ANGLEEDIT, this);
	m_Angle.SetEditControl(&m_AngleEdit);
	m_AngleEdit.SetAngleBox(&m_Angle);

	// Hook up the smart angles controls.
	m_SmartAngle.SubclassDlgItem(IDC_SMART_ANGLEBOX, this);
	m_SmartAngleEdit.SubclassDlgItem(IDC_SMART_ANGLEEDIT, this);
	m_SmartAngle.SetEditControl(&m_SmartAngleEdit);
	m_SmartAngleEdit.SetAngleBox(&m_SmartAngle);

	// Hook up the classes autoselect combo.
	m_cClasses.SubclassDlgItem(IDC_CLASS, this);

	// Hook up the pick color button.
	m_cPickColor.SubclassDlgItem(IDC_PICKCOLOR, this);

	LoadClassList();
	m_VarList.SetTabStops(65);

	m_bWantSmartedit = true;
	SetSmartedit(false);

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : strKey - 
//-----------------------------------------------------------------------------
void COP_Entity::GetCurKey(CString &strKey)
{
	int iSel = m_VarList.GetCurSel();

	if (iSel == -1)
	{
		strKey = "";
		return;
	}

	// get the active key
	if (!m_bSmartedit)
	{
		CString str;
		m_VarList.GetText(iSel, str);
		strKey = str.Left(str.Find('\t'));
	}
	else
	{
		GDinputvariable * pVar = m_pObjClass->GetVariableAt(m_VarMap[iSel]);
		strKey = pVar->GetName();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Selects the given key in the list of keys. If the key is not in the
//			list, the first key is selected.
// Input  : pszKey - Name of key to select.
//-----------------------------------------------------------------------------
void COP_Entity::SetCurKey(LPCTSTR pszKey)
{
	int nSel = m_VarList.GetCount();
	int iCurSel = m_VarList.GetCurSel();
	CString str;

	if (!m_bSmartedit)
	{
		CString strKey;
		for (int i = 0; i < nSel; i++)
		{
			m_VarList.GetText(i, str);
			strKey = str.Left(str.Find('\t'));
			if (!strcmpi(strKey, pszKey))
			{
				// found it here - 
				m_VarList.SetCurSel(i);
				if (iCurSel != i)
				{
					OnSelchangeKeyvalues();
				}
				return;
			}
		}
	}
	else
	{
		for (int i = 0; i < nSel; i++)
		{
			GDinputvariable * pVar = m_pObjClass->GetVariableAt(m_VarMap[i]);
			if (!strcmpi(pVar->GetName(), pszKey))
			{
				// found it here - 
				m_VarList.SetCurSel(i);
				
				// FIXME: Ideally we'd only call OnSelChangeKeyvalues if the selection index
				//        actually changed, but that makes the keyvalue text not refresh properly
				//		  when multiselecting entities with a sidelist key selected. 
				//if (iCurSel != i)
				{
					OnSelchangeKeyvalues();
				}
				return;
			}
		}
	}

	//
	// Not found, select the first key in the list.
	//
	m_VarList.SetCurSel(0);
	OnSelchangeKeyvalues();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::DestroySmartControls(void)
{
	//
	// If a smart control exists, destroy it.
	//
	if (m_pSmartControl != NULL)
	{
		m_pSmartControl->DestroyWindow();
		delete m_pSmartControl;
		m_pSmartControl = NULL;
	}

	//
	// If the Browse button exists, destroy it.
	//
	CWnd *pButton;
	if ((pButton = GetDlgItem(IDC_BROWSE)) != NULL)
	{
		pButton->DestroyWindow();
		delete pButton;
	}

	//
	// If the Mark button exists, destroy it.
	//
	if ((pButton = GetDlgItem(IDC_MARK)) != NULL)
	{
		pButton->DestroyWindow();
		delete pButton;
	}

	//
	// If the Pick Angles button exists, destroy it.
	//
	if ((pButton = GetDlgItem(IDC_PICK_ANGLES)) != NULL)
	{
		pButton->DestroyWindow();
		delete pButton;
	}

	//
	// If the Pick entity button exists, destroy it.
	//
	if ((pButton = GetDlgItem(IDC_PICK_ENTITY)) != NULL)
	{
		pButton->DestroyWindow();
		delete pButton;
	}

	//
	// If the Pick button exists, destroy it.
	//
	if ((pButton = GetDlgItem(IDC_PICK_FACES)) != NULL)
	{
		pButton->DestroyWindow();
		delete pButton;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates the smart controls based on the given type. Deletes any
//			smart controls that are not appropriate to the given type.
// Input  : eType - Type of keyvalue for which to create controls.
//-----------------------------------------------------------------------------
void COP_Entity::CreateSmartControls(GDinputvariable *pVar)
{
	// dvs: TODO: break this monster up into smaller functions

	if (pVar == NULL)
	{
		// UNDONE: delete smart controls?
		return;
	}

	//
	// Set this so that we don't process notifications when we set the edit text,
	// which makes us do things like assume the user changed a key when they didn't.
	//
	m_bIgnoreKVChange = true;

	DestroySmartControls();

	//
	// Build a rectangle in which to create a new control.
	//
	CRect ctrlrect;

	GetDlgItem(IDC_KEY)->GetWindowRect(ctrlrect);
	ScreenToClient(ctrlrect);
	int nHeight = ctrlrect.Height();
	int nRight = ctrlrect.right - 10;

	// Put it just to the right of the keyvalue list.
	m_VarList.GetWindowRect(ctrlrect);
	ScreenToClient(ctrlrect);

	ctrlrect.left = ctrlrect.right + 10;
	ctrlrect.right = nRight;
	ctrlrect.bottom = ctrlrect.top + nHeight;

	GDIV_TYPE eType = pVar->GetType();

	//
	// Hide or show color button.
	//
	m_cPickColor.ShowWindow((eType == ivColor255 || eType == ivColor1) ? SW_SHOW : SW_HIDE);

	HFONT hControlFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hControlFont == NULL)
	{
		hControlFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	}

	//
	// Hide or show the Smart angles controls.
	//
	bool bShowSmartAngles = false;
	if (eType == ivAngle)
	{
		if (stricmp(pVar->GetName(), "angles"))
		{
			bShowSmartAngles = true;

			CRect rectAngleBox;
			m_SmartAngle.GetWindowRect(&rectAngleBox);

			CRect rectAngleEdit;
			m_SmartAngleEdit.GetWindowRect(&rectAngleEdit);

			m_SmartAngle.SetWindowPos(NULL, ctrlrect.left + rectAngleEdit.Width() + 4, ctrlrect.bottom + 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			m_SmartAngleEdit.SetWindowPos(NULL, ctrlrect.left, (ctrlrect.bottom + rectAngleBox.Height() + 10) - rectAngleEdit.Height(), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

			// Update the smart control with the current value
			LPCTSTR pszValue = m_kv.GetValue(pVar->GetName());
			if (pszValue != NULL)
			{
				if (!stricmp(pszValue, VALUE_DIFFERENT_STRING))
				{
					m_SmartAngle.SetDifferent(true);
				}
				else
				{
					m_SmartAngle.SetDifferent(false);
					m_SmartAngle.SetAngles(pszValue);
				}
			}
		}

		//
		// Create an eyedropper button for picking target angles.
		//
		CRect ButtonRect;
		if (bShowSmartAngles)
		{
			CRect rectAngleBox;
			m_SmartAngle.GetWindowRect(&rectAngleBox);
			ScreenToClient(&rectAngleBox);

			CRect rectAngleEdit;
			m_SmartAngleEdit.GetWindowRect(&rectAngleEdit);
			ScreenToClient(&rectAngleEdit);
		
			ButtonRect.left = rectAngleBox.right + 8;
			ButtonRect.top = rectAngleEdit.top;
			ButtonRect.bottom = rectAngleEdit.bottom;
		}
		else
		{
			ButtonRect.left = ctrlrect.left;
			ButtonRect.top = ctrlrect.bottom + 4;
			ButtonRect.bottom = ButtonRect.top + ctrlrect.Height();
		}

		CButton *pButton = new CButton;
		pButton->CreateEx(0, "Button", "Point At...", WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
			ButtonRect.left, ButtonRect.top, 58, ButtonRect.Height() + 2, GetSafeHwnd(), (HMENU)IDC_PICK_ANGLES);
	  	pButton->SendMessage(WM_SETFONT, (WPARAM)hControlFont);

		CWinApp *pApp = AfxGetApp();
		HICON hIcon = pApp->LoadIcon(IDI_CROSSHAIR);
		pButton->SetIcon(hIcon);
	}

	m_SmartAngle.ShowWindow(bShowSmartAngles ? SW_SHOW : SW_HIDE);
	m_SmartAngleEdit.ShowWindow(bShowSmartAngles ? SW_SHOW : SW_HIDE);

	//
	// Choices, NPC classes and filter classes get a combo box.
	//
	if ((eType == ivChoices) || (eType == ivNPCClass) || (eType == ivFilterClass) )
	{
		CMyComboBox *pCombo = new CMyComboBox;
		pCombo->SetParentPage(this);
		ctrlrect.bottom += 150;
		pCombo->Create(CBS_DROPDOWN | CBS_HASSTRINGS | WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VSCROLL | ((eType != ivChoices) ? CBS_SORT : 0), ctrlrect, this, IDC_SMARTCONTROL);
	  	pCombo->SendMessage(WM_SETFONT, (WPARAM)hControlFont);
		pCombo->SetDroppedWidth(150);

		//
		// If we are editing multiple entities, start with the "(different)" string.
		//
		if (IsMultiEdit())
		{
			pCombo->AddString(VALUE_DIFFERENT_STRING);
		}

		//
		// If this is a choices field, give combo box text choices from GameData variable
		//
		if (eType == ivChoices)
		{
			for (int i = 0; i < pVar->m_nItems; i++)
			{
				pCombo->AddString(pVar->m_Items[i].szCaption);
			}
		}
		//
		// For filterclass display a list of all the names of filters that are in the map
		//
		else if (eType == ivFilterClass)
		{
			CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
			CMapWorld *pWorld = pDoc->GetMapWorld();
			CMapEntityList *pEntityList = pWorld->EntityList_GetList();
			POSITION pos = pEntityList->GetHeadPosition();
			while (pos != NULL)
			{
				CMapEntity *pEntity = pEntityList->GetNext(pos);
				GDclass *pClass = pEntity->GetClass();
				if (pClass && pClass->IsFilterClass())
				{
					const char *pString = pEntity->GetKeyValue("targetname");
					if (pString)
					{
						pCombo->AddString(pString);
					}
				}
			}
		}
		//
		// For npcclass fields, fill with all the NPC classes from the FGD.
		//
		else
		{
			if (pGD != NULL)
			{
				POSITION pos = pGD->Classes.GetHeadPosition();
				while (pos != NULL)
				{
					GDclass *pClass = pGD->Classes.GetNext(pos);
					if (pClass->IsNPCClass())
					{
						pCombo->AddString(pClass->GetName());
					}
				}
			}
		}

		//
		// If the current value is the "different" string, display that in the combo box.
		//
		LPCTSTR pszValue = m_kv.GetValue(pVar->GetName());
		if (pszValue != NULL)
		{
			if (strcmp(pszValue, VALUE_DIFFERENT_STRING) == 0)
			{
				pCombo->SelectString(-1, VALUE_DIFFERENT_STRING);
			}
			else
			{
				const char *p = NULL;
				
				//
				// If this is a choices field and the current value corresponds to one of our
				// choices, display the friendly name of the choice in the edit control.
				//
				if (eType == ivChoices)
				{
					p = pVar->ItemStringForValue(pszValue);
				}

				if (p != NULL)
				{
					pCombo->SelectString(-1, p);
				}
				//
				// Otherwise, just display the value as-is.
				//
				else
				{
					pCombo->SetWindowText(pszValue);
				}
			}
		}

		m_pSmartControl = pCombo;
	}
	else
	{
		if ((eType == ivTargetSrc) || (eType == ivTargetDest))
		{
			//
			// Create a combo box for picking target names.
			//
			CRect ComboRect = ctrlrect;
			CTargetNameComboBox *pCombo = new CTargetNameComboBox;
			ComboRect.bottom += 150;
			pCombo->Create(CBS_DROPDOWN | WS_TABSTOP | WS_CHILD | WS_BORDER | CBS_AUTOHSCROLL | WS_VSCROLL | CBS_SORT, ComboRect, this, IDC_SMARTCONTROL);
		  	pCombo->SendMessage(WM_SETFONT, (WPARAM)hControlFont);
			pCombo->SetDroppedWidth(150);

			//
			// Attach the world's entity list to the add dialog.
			//
			// HACK: This is ugly. It should be passed in from outside.
			//
			CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
			CMapWorld *pWorld = pDoc->GetMapWorld();
			pCombo->SetEntityList(pWorld->EntityList_GetList());
			pCombo->SetTargetComboText(m_kv.GetValue(pVar->GetName()));
			
			if (pVar->IsReadOnly())
			{
				pCombo->EnableWindow(FALSE);
			}

			m_pSmartControl = pCombo;
		}
		else
		{
			//
			// Create an edit control for inputting the keyvalue as text.
			//
			CMyEdit *pEdit = new CMyEdit;
			pEdit->SetParentPage(this);
			ctrlrect.bottom += 2;
			pEdit->CreateEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_TABSTOP | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 
				ctrlrect.left, ctrlrect.top, ctrlrect.Width(), ctrlrect.Height(), GetSafeHwnd(), HMENU(IDC_SMARTCONTROL));
		  	pEdit->SendMessage(WM_SETFONT, (WPARAM)hControlFont);

			pEdit->SetWindowText(m_kv.GetValue(pVar->GetName()));

			if (pVar->IsReadOnly())
			{
				pEdit->EnableWindow(FALSE);
			}

			m_pSmartControl = pEdit;
		}

		//
		// Create a "Browse..." button for browsing for files.
		//
		if ((eType == ivStudioModel) || (eType == ivSprite) || (eType == ivSound) || (eType == ivDecal) ||
			(eType == ivMaterial) || (eType == ivScene))
		{
			CRect ButtonRect = ctrlrect;

			ButtonRect.top = ctrlrect.bottom + 4;
			ButtonRect.bottom = ctrlrect.bottom + ctrlrect.Height() + 4;
			ButtonRect.right = ButtonRect.left + 54;

			CButton *pButton = new CButton;
			pButton->CreateEx(0, "Button", "Browse...", WS_TABSTOP | WS_CHILD | WS_VISIBLE, 
				ButtonRect.left, ButtonRect.top, ButtonRect.Width(), ButtonRect.Height(), 
				GetSafeHwnd(), (HMENU)IDC_BROWSE);
  			pButton->SendMessage(WM_SETFONT, (WPARAM)hControlFont);
		}
		else if ((eType == ivTargetDest) || (eType == ivTargetSrc))
		{
			//
			// Create a "Mark" button for finding target entities.
			//
			CRect ButtonRect = ctrlrect;

			ButtonRect.top = ctrlrect.bottom + 4;
			ButtonRect.bottom = ctrlrect.bottom + ctrlrect.Height() + 4;
			ButtonRect.right = ButtonRect.left + 48;

			CButton *pButton = new CButton;
			pButton->CreateEx(0, "Button", "Mark", WS_TABSTOP | WS_CHILD | WS_VISIBLE, 
				ButtonRect.left, ButtonRect.top, 48, ButtonRect.Height(),
				GetSafeHwnd(), (HMENU)IDC_MARK);
  			pButton->SendMessage(WM_SETFONT, (WPARAM)hControlFont);

			//
			// Create an eyedropper button for picking target entities.
			//
			pButton = new CButton;
			pButton->CreateEx(0, "Button", "", WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_ICON | BS_AUTOCHECKBOX | BS_PUSHLIKE,
				ButtonRect.left + ButtonRect.Width() + 4, ButtonRect.top, 32, ButtonRect.Height(),
				GetSafeHwnd(), (HMENU)IDC_PICK_ENTITY);

			CWinApp *pApp = AfxGetApp();
			HICON hIcon = pApp->LoadIcon(IDI_EYEDROPPER);
			pButton->SetIcon(hIcon);
		}
		else if ((eType == ivSide) || (eType == ivSideList))
		{
			//
			// Create a "Pick" button for clicking on brush sides.
			//
			CRect ButtonRect = ctrlrect;

			ButtonRect.top = ctrlrect.bottom + 4;
			ButtonRect.bottom = ctrlrect.bottom + ctrlrect.Height() + 4;
			ButtonRect.right = ButtonRect.left + 54;

			CButton *pButton = new CButton;
			pButton->CreateEx(0, "Button", "Pick...", WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
				ButtonRect.left, ButtonRect.top, ButtonRect.Width(), ButtonRect.Height(), 
				GetSafeHwnd(), (HMENU)IDC_PICK_FACES);
  			pButton->SendMessage(WM_SETFONT, (WPARAM)hControlFont);
		}
	}

	m_pSmartControl->ShowWindow(SW_SHOW);
	m_pSmartControl->SetWindowPos(&m_VarList, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);

	m_bIgnoreKVChange = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnSelchangeKeyvalues(void)
{
	//
	// Load new selection's key/values into the key/value 
	// edit controls.
	//
	if (m_VarList.m_hWnd == NULL)
	{
		return;
	}

	if (m_bSmartedit)
	{
		//
		// Stop face or entity picking if we are doing so.
		//
		StopPicking();
	}

	int iSel = m_VarList.GetCurSel();
	if (iSel == LB_ERR)
	{
		if (!m_bSmartedit)
		{
			// No selection; clear our the key and value controls.
			m_cKey.SetWindowText("");
			m_cValue.SetWindowText("");
		}

		return;
	}

	if (!m_bSmartedit)
	{
		CString str;
		m_VarList.GetText(iSel, str);

		//
		// Set the edit control text, but ignore the notifications caused by that.
		//
		m_bIgnoreKVChange = true;
		m_cKey.SetWindowText(str.Left(str.Find('\t')));
		m_cValue.SetWindowText(str.Right(str.GetLength() - str.ReverseFind('\t') - 1));
		m_bIgnoreKVChange = false;
	}
	else
	{
		GDinputvariable *pVar = m_pObjClass->GetVariableAt(m_VarMap[iSel]);

		//
		// Update the keyvalue help text control with this variable's help info.
		//
		m_KeyValueHelpText.SetWindowText(pVar->GetDescription());
		
		CreateSmartControls(pVar);
		m_eEditType = pVar->GetType();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used only in standard (non-SmartEdit) mode. Called when the contents
//			of the value edit control change. This is where the edit control
//			contents are converted to keyvalue data in standard mode.
//-----------------------------------------------------------------------------
void COP_Entity::OnChangeKeyorValue(void) 
{
	if (m_bIgnoreKVChange)
	{
		return;
	}

	int iSel = m_VarList.GetCurSel();
	if (iSel == LB_ERR)
	{
		return;
	}

	char szKey[KEYVALUE_MAX_KEY_LENGTH];
	char szValue[KEYVALUE_MAX_VALUE_LENGTH];

	m_cKey.GetWindowText(szKey, sizeof(szKey));
	m_cValue.GetWindowText(szValue, sizeof(szValue));

	UpdateKeyValue(szKey, szValue);

	//
	// Save it in our local kv storage.
	//
	m_kv.SetValue(szKey, szValue);

	if (m_bEnableControlUpdate)
	{
		// Update any controls that are displaying the same data as the edit control.

		// This code should only be hit as a result of user input in the edit control!
		// If they changed the "angles" key, update the main angles control.
		if (!stricmp(szKey, "angles"))
		{
			m_Angle.SetDifferent(false);
			m_Angle.SetAngles(szValue, true);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnAddkeyvalue(void)
{
	// create a new keyvalue at the end of the list
	CNewKeyValue newkv;
	newkv.m_Key.Format("newkey%d", m_nNewKeyCount++);
	newkv.m_Value = "value";
	if(newkv.DoModal() == IDCANCEL)
		return;

	// if the key we're adding is already in the list, do some
	//  stuff to make it unique
	if(m_kv.GetValue(newkv.m_Key))
	{
		CString strTemp;
		for(int i = 1; ; i++)
		{
			strTemp.Format("%s#%d", newkv.m_Key, i);
			if(!m_kv.GetValue(strTemp))
				break;
		}
		newkv.m_Key = strTemp;
	}
	
	CString strNew;
	strNew.Format("%s\t%s", newkv.m_Key, newkv.m_Value);
	m_VarList.AddString(strNew);
	m_VarList.SetCurSel(m_VarList.GetCount()-1);
	m_kvAdded.SetValue(newkv.m_Key, "1");
	m_kv.SetValue(newkv.m_Key, newkv.m_Value);
	OnSelchangeKeyvalues();
}


//-----------------------------------------------------------------------------
// Purpose: Deletes the selected keyvalue.
//-----------------------------------------------------------------------------
void COP_Entity::OnRemovekeyvalue(void) 
{
	int iSel = m_VarList.GetCurSel();
	if (iSel == LB_ERR)
	{
		return;
	}

	CString strBuf;
	m_VarList.GetText(iSel, strBuf);

	//
	// Remove the keyvalue from local storage.
	//
	m_kv.RemoveKey(strBuf.Left(strBuf.Find('\t')));

	m_VarList.DeleteString(iSel);
	if (iSel == m_VarList.GetCount())
	{
		m_VarList.SetCurSel(iSel - 1);
	}

	OnSelchangeKeyvalues();
}


//-----------------------------------------------------------------------------
// Purpose: Updates the dialog based on a change to the entity class name.
// Input  : pszClass - THe new class name.
//-----------------------------------------------------------------------------
void COP_Entity::UpdateClass(const char *pszClass)
{
	GDclass *pOldObjClass = m_pObjClass;

	m_pObjClass = pGD->ClassForName(pszClass);

	if (!m_pObjClass)
	{
		//
		// Object has no known class - get rid of smartedit.
		//
		if (m_bSmartedit)
		{
			SetSmartedit(false);
		}

		GetDlgItem(IDC_SMARTEDIT)->EnableWindow(FALSE);
	}    
	else
	{
		CEntityHelpDlg::SetEditGameClass(m_pObjClass);

		//
		// Known class - enable smartedit.
		//
		GetDlgItem(IDC_SMARTEDIT)->EnableWindow(TRUE);

		if (m_bWantSmartedit)
		{
			SetSmartedit(true);
		}
	}

	//
	// Remove unused keyvalues.
	//
	if (m_pObjClass != pOldObjClass && m_pObjClass && strcmpi(pszClass, "multi_manager"))
	{
		int nKeyValues = m_kv.GetCount();
		for (int i = nKeyValues - 1; i >= 0; i--)
		{
			MDkeyvalue &KeyValue = m_kv.GetKeyValue(i);
			if (m_pObjClass->VarForName(KeyValue.szKey) == NULL)
			{
				m_kv.RemoveKey(KeyValue.szKey);
			}
		}
	}

	SetupForMode();
}


//-----------------------------------------------------------------------------
// Purpose: Called when a keyvalue is modified by the user.
// Input  : szKey - The key name.
//			szValue - The new value.
//-----------------------------------------------------------------------------
void COP_Entity::UpdateKeyValue(const char *szKey, const char *szValue)
{
	m_kvAdded.SetValue(szKey, "1");
	m_kv.SetValue(szKey, szValue);

	if (!m_bSmartedit)
	{
		//
		// Update the list box contents for non-SmartEdit mode.
		//
		char szBuf[KEYVALUE_MAX_KEY_LENGTH + KEYVALUE_MAX_VALUE_LENGTH + 1];
		sprintf(szBuf, "%s\t%s", szKey, szValue);

		int nItem = m_VarList.FindString(-1, szKey);
		if (nItem != LB_ERR)
		{
			int nSel = m_VarList.GetCurSel();
			m_VarList.SetRedraw(FALSE);
			m_VarList.DeleteString(nItem);
			m_VarList.InsertString(nItem, szBuf);
			m_VarList.SetCurSel(nSel);
			m_VarList.SetRedraw(TRUE);
			m_VarList.Invalidate();
		}
		else
		{
			m_VarList.AddString(szBuf);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables SmartEdit mode, hiding showing the appropriate
//			dialog controls.
// Input  : b - TRUE to enable, FALSE to disable SmartEdit.
//-----------------------------------------------------------------------------
void COP_Entity::SetSmartedit(bool bSet)
{
	m_bSmartedit = bSet;

	//
	// If disabling smartedit, remove any smartedit-specific controls that may
	// or may not be currently visible.
	//
	if (!m_bSmartedit)
	{
		m_cPickColor.ShowWindow(SW_HIDE);
		m_SmartAngle.ShowWindow(SW_HIDE);
		m_SmartAngleEdit.ShowWindow(SW_HIDE);

		DestroySmartControls();
	}

	m_KeyValueHelpText.ShowWindow(m_bSmartedit ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_KEYVALUE_HELP_GROUP)->ShowWindow(m_bSmartedit ? SW_SHOW : SW_HIDE);

	//
	// Hide or show all controls after and including "delete kv" button.
	//
	CWnd *pWnd = GetDlgItem(IDC_REMOVEKEYVALUE);

	// dvs: HARDCODED CONSTANT!!
	for (int i = 0; i < 6; i++)
	{
		//pWnd->EnableWindow(!m_bSmartedit);
		pWnd->ShowWindow(m_bSmartedit ? SW_HIDE : SW_SHOW);
		pWnd = pWnd->GetNextWindow();
	}

	((CButton*)GetDlgItem(IDC_SMARTEDIT))->SetCheck(m_bSmartedit);

	SetupForMode();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnSmartedit(void) 
{
	SetSmartedit(!m_bSmartedit);
	m_bWantSmartedit = m_bSmartedit;
}


//-----------------------------------------------------------------------------
// Purpose: Updates the dialog contents when the class is changed in the combo.
//-----------------------------------------------------------------------------
void COP_Entity::OnSelchangeClass(void)
{
	char szClass[MAX_IO_NAME_LEN];
	szClass[0] = '\0';

	int nCurSel = m_cClasses.GetCurSel();
	if (nCurSel == CB_ERR)
	{
		if (m_cClasses.GetWindowText(szClass, sizeof(szClass)) > 0)
		{
			nCurSel = m_cClasses.FindStringExact(-1, szClass);
		}
	}

	if (nCurSel != CB_ERR)
	{
		m_cClasses.GetLBText(nCurSel, szClass);
	}

	UpdateClass(szClass);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COP_Entity::BrowseTextures( const char *szFilter )
{
	// browse for textures
	int iSel = m_VarList.GetCurSel();
	GDinputvariable * pVar = m_pObjClass->GetVariableAt(m_VarMap[iSel]);

	// get current texture
	char szInitialTexture[128];
	m_pSmartControl->GetWindowText(szInitialTexture, 128);
	
	// create a texture browser and set it to browse decals
	CTextureBrowser *pBrowser = new CTextureBrowser(GetMainWnd());

	// setup filter - if any
	if( szFilter[0] != '\0' )
	{
		pBrowser->SetFilter( szFilter );
	}

	pBrowser->SetInitialTexture(szInitialTexture);

	if (pBrowser->DoModal() == IDOK)
	{
		IEditorTexture *pTex = g_Textures.FindActiveTexture(pBrowser->m_cTextureWindow.szCurTexture);

		char szName[MAX_PATH];
		if (pTex)
		{
			pTex->GetShortName(szName);
		}
		else
		{
			szName[0] = '\0';
		}

		m_pSmartControl->SetWindowText(szName);

		// also set variable
		m_kv.SetValue(pVar->GetName(), szName);
		m_kvAdded.SetValue(pVar->GetName(), "1");
	}

	delete pBrowser;
}


//-----------------------------------------------------------------------------
// Purpose: Used only in SmartEdit mode. Called when the contents of the value
//			edit control change. This is where the edit control contents are
//			converted to keyvalue data in SmartEdit mode.
//-----------------------------------------------------------------------------
void COP_Entity::OnChangeSmartcontrol(void)
{
	//
	// We only respond to this message when it is due to user input.
	// Don't do anything if we're creating the smart control.
	//
	if (m_bIgnoreKVChange)
	{
		return;
	}

	int iSel = m_VarList.GetCurSel();
	GDinputvariable *pVar = m_pObjClass->GetVariableAt(m_VarMap[iSel]);

	char szKey[KEYVALUE_MAX_KEY_LENGTH];
	char szValue[KEYVALUE_MAX_VALUE_LENGTH];

	strcpy(szKey, pVar->GetName());
	m_pSmartControl->GetWindowText(szValue, sizeof(szValue));

	if (pVar->GetType() == ivChoices)
	{
		//
		// If a choicelist, change buffer to the string value of what we chose.
		//
		const char *pszValueString = pVar->ItemValueForString(szValue);
		if (pszValueString != NULL)
		{
			strcpy(szValue, pszValueString);
		}
	}

	UpdateKeyValue(szKey, szValue);

	if (m_bEnableControlUpdate)
	{
		// Update any controls that are displaying the same data as the edit control.
		// This code should only be hit as a result of user input in the edit control!
		if (pVar->GetType() == ivAngle)
		{
			// If they changed the "angles" key, update the main angles control.
			if (!stricmp(pVar->GetName(), "angles"))
			{
				m_Angle.SetDifferent(false);
				m_Angle.SetAngles(szValue, true);
			}
			// Otherwise update the Smart angles control.
			else
			{
				m_SmartAngle.SetDifferent(false);
				m_SmartAngle.SetAngles(szValue, true);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnChangeSmartcontrolSel(void)
{
	// update current value with this
	int iSel = m_VarList.GetCurSel();
	GDinputvariable * pVar = m_pObjClass->GetVariableAt(m_VarMap[iSel]);
	if ((pVar->GetType() != ivTargetSrc) &&
		(pVar->GetType() != ivTargetDest) &&
		(pVar->GetType() != ivChoices) &&
		(pVar->GetType() != ivNPCClass) &&
		(pVar->GetType() != ivFilterClass))
	{
		return;
	}

	CComboBox *pCombo = (CComboBox *)m_pSmartControl;

	char szBuf[128];

	// get current selection
	int iSmartsel = pCombo->GetCurSel();
	if (iSmartsel != LB_ERR)
	{
		// found a selection - now get the text
		pCombo->GetLBText(iSmartsel, szBuf);
	}
	else
	{
		// just get the text from the combo box (no selection)
		pCombo->GetWindowText(szBuf, 128);
	}

	if (pVar->GetType() == ivChoices)
	{
		const char *pszValue = pVar->ItemValueForString(szBuf);
		if (pszValue != NULL)
		{
			strcpy(szBuf, pszValue);
		}
	}

	m_kvAdded.SetValue(pVar->GetName(), "1");
	m_kv.SetValue(pVar->GetName(), szBuf);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//-----------------------------------------------------------------------------
void COP_Entity::SetNextVar(int cmd)
{
	int iSel = m_VarList.GetCurSel();
	int nCount = m_VarList.GetCount();
	if(iSel == LB_ERR)
		return;	// no!
	iSel += cmd;
	if(iSel == nCount)
		--iSel;
	if(iSel == -1)
		++iSel;
	m_VarList.SetCurSel(iSel);
	OnSelchangeKeyvalues();
}


//-----------------------------------------------------------------------------
// Purpose: Brings up the file browser in the appropriate default directory
//			based on the type of file being browsed for.
//-----------------------------------------------------------------------------
void COP_Entity::OnBrowse(void)
{
	// handle browsing for .fgd "material" type
	if( m_eEditType == ivMaterial )
	{
		BrowseTextures( "\0" );
		return;
	}

	// handle browsing for .fgd "decal" type
	if(m_eEditType == ivDecal)
	{
		if (g_pGameConfig->GetTextureFormat() == tfVMT)
		{
			BrowseTextures("decals/");
		}
		else
		{
			BrowseTextures("{");
		}		
		return;		
	}

	char *pszInitialDir= NULL;
	char *pszExtension = NULL;
	char *pszFilter = NULL;

	//
	// Based on the type of file that we are picking, set up the default extension,
	// default directory, and filters. Each type of file remembers its last directory.
	//
	switch (m_eEditType)
	{
		POSITION pos;

		case ivStudioModel:
		{
			static char szInitialDir[MAX_PATH] = "";

			pszExtension = "jpg";
			pszFilter = "Model thumbnail files (*.jpg)|*.jpg|Model files (*.mdl)|*.mdl||";
			if (szInitialDir[0] == '\0')
			{
				APP()->GetFirstSearchDir(SEARCHDIR_MODELS, szInitialDir, pos);
			}
			pszInitialDir = szInitialDir;
			break;
		}

		case ivSprite:
		{
			static char szInitialDir[MAX_PATH] = "";

			pszExtension = "spr";
			pszFilter = "Material files (*.vmt)|*.vmt||";
			if (szInitialDir[0] == '\0')
			{
				APP()->GetFirstSearchDir(SEARCHDIR_SPRITES, szInitialDir, pos);
			}
			pszInitialDir = szInitialDir;
			break;
		}

		case ivSound:
		{
			static char szInitialDir[MAX_PATH] = "";

			pszExtension = "wav";
			pszFilter = "Sound files (*.wav)|*.wav||";
			if (szInitialDir[0] == '\0')
			{
				APP()->GetFirstSearchDir(SEARCHDIR_SOUNDS, szInitialDir, pos);
			}
			pszInitialDir = szInitialDir;
			break;
		}

		case ivScene:
		{
			static char szInitialDir[MAX_PATH] = "";

			pszExtension = "vcd";
			pszFilter = "Scene files (*.vcd)|*.vcd||";
			if (szInitialDir[0] == '\0')
			{
				APP()->GetFirstSearchDir(SEARCHDIR_SCENES, szInitialDir, pos);
			}
			pszInitialDir = szInitialDir;
			break;
		}

		default:
		{
			static char szInitialDir[MAX_PATH] = "";

			pszExtension = "";
			pszFilter = "All files (*.*)|*.*||";

			if (szInitialDir[0] == '\0')
			{
				APP()->GetDirectory(DIR_PROGRAM, szInitialDir);
			}
			pszInitialDir = szInitialDir;
			break;
		}
	}

	//
	// Open the file browse dialog.
	//
	CFileDialog dlg(TRUE, pszExtension, NULL, OFN_FILEMUSTEXIST | OFN_ENABLESIZING, pszFilter);
	dlg.m_ofn.lpstrInitialDir = pszInitialDir;

	//
	// If they picked a file and hit OK, put everything after the last backslash
	// into the SmartEdit control. If there is no backslash, put the whole filename.
	//
	if (dlg.DoModal() == IDOK)
	{
		//
		// Save the default folder for next time.
		//
		strcpy(pszInitialDir, dlg.m_ofn.lpstrFile);
		char *pchSlash = strrchr(pszInitialDir, '\\');
		if (pchSlash != NULL)
		{
			*pchSlash = '\0';
		}

		if (m_pSmartControl != NULL)
		{
			//
			// Sounds are relative to the sound directory, everything else is relative to
			// the game directory.
			//
			SearchDir_t eSearchDir;

			if (m_eEditType == ivSound)
			{
				eSearchDir = SEARCHDIR_SOUNDS;
			}
			else
			{
				eSearchDir = SEARCHDIR_DEFAULT;
			}

			//
			// If they chose a file under the initial directory, only keep the portion
			// of the path relative to the initial directory. Otherwise, keep the whole path.
			//
			POSITION pos;
			char szStripDir[MAX_PATH];
			char *pszCopy = NULL;

			APP()->GetFirstSearchDir(eSearchDir, szStripDir, pos);
			while ((pos != NULL) && (pszCopy == NULL))
			{
				if (!strnicmp(dlg.m_ofn.lpstrFile, szStripDir, strlen(szStripDir)))
				{
					pszCopy = &dlg.m_ofn.lpstrFile[strlen(szStripDir)];
				}
				else
				{
					APP()->GetNextSearchDir(eSearchDir, szStripDir, pos);
				}
			}

			//
			// If the path wasn't relative to any of the search directories, store
			// the entire path.
			//
			if (pszCopy == NULL)
			{
				pszCopy = dlg.m_ofn.lpstrFile;
			}

			//
			// Reverse the slashes, because the engine expects them that way.
			//
			char szTemp[MAX_PATH];
			strcpy(szTemp, pszCopy);
			for (unsigned int i = 0; i < strlen(szTemp); i++)
			{
				if (szTemp[i] == '\\')
				{
					szTemp[i] = '/';
				}
			}

			//
			// Replace the .jpg extension with .mdl if browsing for models.
			//
			if (m_eEditType == ivStudioModel)
			{
				char *pchDot = strrchr(szTemp, '.');
				if (pchDot != NULL)
				{
					strcpy(pchDot, ".mdl");
				}
			}

			m_pSmartControl->SetWindowText(szTemp);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnCopy(void)
{
	// copy entity keyvalues
	kvClipboard.RemoveAll();
	bKvClipEmpty = FALSE;
	for (int i = 0; i < m_kv.GetCount(); i++)
	{
		if (stricmp(m_kv.GetKey(i), "origin"))
		{
			kvClipboard.SetValue(m_kv.GetKey(i), m_kv.GetValue(i));
		}
	}

	char szClass[128];
	m_cClasses.GetWindowText(szClass, 128);

	kvClipboard.SetValue("xxxClassxxx", szClass);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnEntityHelp(void)
{
	CEntityHelpDlg::ShowEntityHelpDialog();
	CEntityHelpDlg::SetEditGameClass(m_pObjClass);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnKillfocusKey(void)
{
	if (!m_bChangingKeyName)
		return;

	m_bChangingKeyName = false;
	CString strOutput;

	m_cKey.GetWindowText(strOutput);

	if (strOutput.IsEmpty())
	{
		AfxMessageBox("Use the delete button to remove key/value pairs.");
		return;
	}

	strOutput.MakeLower();

	// No change
	if (strOutput == m_szOldKeyName)
		return;

	char szSaveValue[KEYVALUE_MAX_VALUE_LENGTH];
	memset(szSaveValue, 0, sizeof(szSaveValue));
	strncpy(szSaveValue, m_kv.GetValue(m_szOldKeyName, NULL), sizeof(szSaveValue) - 1);

	int iSel = m_VarList.GetCurSel();
	if (iSel == LB_ERR)
		return;
	
	CString strBuf;
	m_VarList.GetText(iSel, strBuf);
	m_kv.RemoveKey(strBuf.Left(strBuf.Find('\t')));	// remove from local kv storage
	m_VarList.DeleteString(iSel);
	if(iSel == m_VarList.GetCount())
		m_VarList.SetCurSel(iSel-1);
	OnSelchangeKeyvalues();

	CNewKeyValue newkv;
	newkv.m_Key   = strOutput;
	newkv.m_Value = szSaveValue;

	// if the key we're adding is already in the list, do some
	//  stuff to make it unique
	CString strNew;
	strNew.Format("%s\t%s", newkv.m_Key, newkv.m_Value);
	m_VarList.AddString(strNew);
	m_VarList.SetCurSel(m_VarList.GetCount()-1);
	m_kvAdded.SetValue(newkv.m_Key, "1");
	m_kv.SetValue(newkv.m_Key, newkv.m_Value);
	OnSelchangeKeyvalues();
	
}


//-----------------------------------------------------------------------------
// Purpose: Marks all entities whose targetnames match the currently selected
//			key value.
//-----------------------------------------------------------------------------
void COP_Entity::OnMark(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();

	if (pDoc != NULL)
	{
		char szTargetName[MAX_IO_NAME_LEN];
		m_pSmartControl->GetWindowText(szTargetName, sizeof(szTargetName));
		if (szTargetName[0] != '\0')
		{
			CMapEntityList Found;
			pDoc->FindEntitiesByKeyValue(&Found, "targetname", szTargetName);

			if (Found.GetCount() != 0)
			{
				CMapObjectList Select;
				POSITION pos = Found.GetHeadPosition();
				while (pos != NULL)
				{
					CMapEntity *pEntity = Found.GetNext(pos);
					Select.AddTail(pEntity);
				}

				pDoc->SelectObjectList(&Select);
				pDoc->CenterSelection();
			}
			else
			{
				MessageBox("No entities were found with that targetname.", "No entities found", MB_ICONINFORMATION | MB_OK);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnPaste(void)
{
	if(bKvClipEmpty)
		return;

	CString str;
	GetCurKey(str);

	// copy entity keyvalues
	int nKeyValues = kvClipboard.GetCount();
	for (int i = 0; i < nKeyValues; i++)
	{
		if (!strcmp(kvClipboard.GetKey(i), "xxxClassxxx"))
		{
			int iIndex = m_cClasses.FindString(-1, kvClipboard.GetValue(i));
			if (iIndex >= 0)
			{
				m_cClasses.SetCurSel(iIndex);
			}

			UpdateClass(kvClipboard.GetValue(i));
			continue;
		}

		m_kv.SetValue(kvClipboard.GetKey(i), kvClipboard.GetValue(i));
		m_kvAdded.SetValue(kvClipboard.GetKey(i), "1");
	}

	SetupForMode();
	SetCurKey(str);
}


//-----------------------------------------------------------------------------
// Purpose: For the given key name, builds a list of faces that are common
//			to all entitie being edited and a list of faces that are found in at
//			least one entity being edited.
// Input  : FullFaces - 
//			PartialFaces - 
//			pszKey - the name of the key.
//-----------------------------------------------------------------------------
void COP_Entity::GetFaceIDListsForKey(CMapFaceIDList &FullFaces, CMapFaceIDList &PartialFaces, const char *pszKey)
{
	CMapWorld *pWorld = GetActiveWorld();

	if ((m_pObjectList != NULL) && (pWorld != NULL))
	{
		bool bFirst = true;
		POSITION pos = m_pObjectList->GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = m_pObjectList->GetNext(pos);
			CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
			if (pEntity != NULL)
			{
				const char *pszValue = pEntity->GetKeyValue(pszKey);

				if (bFirst)
				{
					pWorld->FaceID_StringToFaceIDLists(&FullFaces, NULL, pszValue);
					bFirst = false;
				}
				else
				{
					CMapFaceIDList TempFaces;
					pWorld->FaceID_StringToFaceIDLists(&TempFaces, NULL, pszValue);

					CMapFaceIDList TempFullFaces = FullFaces;
					FullFaces.RemoveAll();

					TempFaces.Intersect(TempFullFaces, FullFaces, PartialFaces);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: For the given key name, builds a list of faces that are common
//			to all entitie being edited and a list of faces that are found in at
//			least one entity being edited.
// Input  : FullFaces - 
//			PartialFaces - 
//			pszKey - the name of the key.
//-----------------------------------------------------------------------------
// dvs: FIXME: try to eliminate this function
void COP_Entity::GetFaceListsForKey(CMapFaceList &FullFaces, CMapFaceList &PartialFaces, const char *pszKey)
{
	CMapWorld *pWorld = GetActiveWorld();

	if ((m_pObjectList != NULL) && (pWorld != NULL))
	{
		bool bFirst = true;
		POSITION pos = m_pObjectList->GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = m_pObjectList->GetNext(pos);
			CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
			if (pEntity != NULL)
			{
				const char *pszValue = pEntity->GetKeyValue(pszKey);

				if (bFirst)
				{
					pWorld->FaceID_StringToFaceLists(&FullFaces, NULL, pszValue);
					bFirst = false;
				}
				else
				{
					CMapFaceList TempFaces;
					pWorld->FaceID_StringToFaceLists(&TempFaces, NULL, pszValue);

					CMapFaceList TempFullFaces = FullFaces;
					FullFaces.RemoveAll();

					TempFaces.Intersect(TempFullFaces, FullFaces, PartialFaces);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnPickFaces(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	ASSERT(pDoc != NULL);

	if (pDoc == NULL)
	{
		return;
	}

	CButton *pButton = (CButton *)GetDlgItem(IDC_PICK_FACES);
	ASSERT(pButton != NULL);

	if (pButton != NULL)
	{
		if (pButton->GetCheck())
		{
			int nSel = m_VarList.GetCurSel();
			ASSERT(nSel != LB_ERR);

			if (nSel != LB_ERR)
			{
				GDinputvariable *pVar = m_pObjClass->GetVariableAt(m_VarMap[nSel]);
				ASSERT((pVar->GetType() == ivSideList) || (pVar->GetType() == ivSide));

				// FACEID TODO: make the faces highlight even when the tool is not active

				//
				// Build the list of faces that are in all selected entities, and a list
				// of faces that are in at least one selected entity, so that we can do
				// multiselect properly.
				//
				CMapFaceList FullFaces;
				CMapFaceList PartialFaces;
				GetFaceListsForKey(FullFaces, PartialFaces, pVar->GetName());

				// Save the old tool so we can reset to the correct tool when we stop picking.
				m_ToolPrePick = g_pToolManager->GetToolID();
				m_bPicking = true;

				//
				// Activate the face picker tool. It will handle the picking of faces.
				//
				CToolPickFace *pTool = (CToolPickFace *)g_pToolManager->GetToolForID(TOOL_PICK_FACE);
				pTool->SetSelectedFaces(FullFaces, PartialFaces);
				m_PickFaceTarget.AttachEntityDlg(this);
				pTool->Attach(&m_PickFaceTarget);
				pTool->AllowMultiSelect(pVar->GetType() == ivSideList);

				g_pToolManager->SetTool(TOOL_PICK_FACE);
			}
		}
		else
		{
			//
			// Get the face IDs from the face picker tool.
			//
			m_bPicking = false;
			CToolPickFace *pTool = (CToolPickFace *)g_pToolManager->GetToolForID(TOOL_PICK_FACE);
			UpdatePickFaceText(pTool);
			g_pToolManager->SetTool(TOOL_POINTER);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnPickAngles(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL)
	{
		return;
	}

	CButton *pButton = (CButton *)GetDlgItem(IDC_PICK_ANGLES);
	ASSERT(pButton != NULL);

	if (pButton != NULL)
	{
		if (pButton->GetCheck())
		{
			int nSel = m_VarList.GetCurSel();
			ASSERT(nSel != LB_ERR);

			if (nSel != LB_ERR)
			{
				GDinputvariable *pVar = m_pObjClass->GetVariableAt(m_VarMap[nSel]);
				ASSERT(pVar->GetType() == ivAngle);

				// Save the old tool so we can reset to the correct tool when we stop picking.
				m_ToolPrePick = g_pToolManager->GetToolID();
				m_bPicking = true;

				//
				// Activate the angles picker tool.
				//
				CToolPickAngles *pTool = (CToolPickAngles *)g_pToolManager->GetToolForID(TOOL_PICK_ANGLES);
				m_PickAnglesTarget.AttachEntityDlg(this);
				pTool->Attach(&m_PickAnglesTarget);

				g_pToolManager->SetTool(TOOL_PICK_ANGLES);
			}
		}
		else
		{
			StopPicking();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnPickEntity(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL)
	{
		return;
	}

	CButton *pButton = (CButton *)GetDlgItem(IDC_PICK_ENTITY);
	ASSERT(pButton != NULL);

	if (pButton != NULL)
	{
		if (pButton->GetCheck())
		{
			int nSel = m_VarList.GetCurSel();
			ASSERT(nSel != LB_ERR);

			if (nSel != LB_ERR)
			{
				GDinputvariable *pVar = m_pObjClass->GetVariableAt(m_VarMap[nSel]);
				ASSERT((pVar->GetType() == ivTargetDest) || (pVar->GetType() == ivTargetSrc));

				// Save the old tool so we can reset to the correct tool when we stop picking.
				m_ToolPrePick = g_pToolManager->GetToolID();
				m_bPicking = true;

				//
				// Activate the entity picker tool.
				//
				CToolPickEntity *pTool = (CToolPickEntity *)g_pToolManager->GetToolForID(TOOL_PICK_ENTITY);
				m_PickEntityTarget.AttachEntityDlg(this);
				pTool->Attach(&m_PickEntityTarget);

				g_pToolManager->SetTool(TOOL_PICK_ENTITY);
			}
		}
		else
		{
			StopPicking();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnPickColor(void)
{
	static COLORREF CustomColors[16];
	int iSel = m_VarList.GetCurSel();
	GDinputvariable * pVar = m_pObjClass->GetVariableAt(m_VarMap[iSel]);

	// find current color
	COLORREF clr;
	BYTE r, g, b;
	DWORD brightness = 0xffffffff;

	char szTmp[128], *pTmp;
	m_pSmartControl->GetWindowText(szTmp, sizeof szTmp);

	pTmp = strtok(szTmp, " ");
	int iCurToken = 0;
	while(pTmp)
	{
		if(pTmp[0])
		{
			if(iCurToken == 3)
			{
				brightness = atol(pTmp);
			}
			else if(pVar->GetType() == ivColor255)
			{
				if(iCurToken == 0)
					r = BYTE(atol(pTmp));
				if(iCurToken == 1)
					g = BYTE(atol(pTmp));
				if(iCurToken == 2)
					b = BYTE(atol(pTmp));
			}
			else
			{
				if(iCurToken == 0)
					r = BYTE(atof(pTmp) * 255.f);
				if(iCurToken == 1)
					g = BYTE(atof(pTmp) * 255.f);
				if(iCurToken == 2)
					b = BYTE(atof(pTmp) * 255.f);
			}
			++iCurToken;
		}
		pTmp = strtok(NULL, " ");
	}

	clr = RGB(r, g, b);

	CColorDialog dlg(clr, CC_FULLOPEN);
	dlg.m_cc.lpCustColors = CustomColors;
	if(dlg.DoModal() != IDOK)
		return;

	r = GetRValue(dlg.m_cc.rgbResult);
	g = GetGValue(dlg.m_cc.rgbResult);
	b = GetBValue(dlg.m_cc.rgbResult);

	// set back in field
	if(pVar->GetType() == ivColor255)
	{
		sprintf(szTmp, "%d %d %d", r, g, b);
	}
	else
	{
		sprintf(szTmp, "%.3f %.3f %.3f", float(r) / 255.f,
			float(g) / 255.f, float(b) / 255.f);
	}

	if(brightness != 0xffffffff)
		sprintf(szTmp + strlen(szTmp), " %d", brightness);

	m_pSmartControl->SetWindowText(szTmp);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Entity::OnSetfocusKey(void)
{
	m_cKey.GetWindowText(m_szOldKeyName);

	if (m_szOldKeyName.IsEmpty())
		return;

	m_szOldKeyName.MakeLower();

	m_bChangingKeyName = true;
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever we are hidden or shown.
// Input  : bShow - TRUE if we are being shown, FALSE if we are being hidden.
//			nStatus - 
//-----------------------------------------------------------------------------
void COP_Entity::OnShowPropertySheet(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		//
		// Being shown. Make sure the data in the smartedit control is correct.
		//
		OnSelchangeKeyvalues();
	}
	else
	{
		//
		// Being hidden. Abort face picking if we are doing so.
		//
		StopPicking();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nChar - 
//			nRepCnt - 
//			nFlags - 
//-----------------------------------------------------------------------------
void CMyEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CEdit::OnChar(nChar, nRepCnt, nFlags);
	return;

	if(nChar == 1)	// ctrl+a
	{
		m_pParent->SetNextVar(1);
	}
	else if(nChar == 11)	// ctrl+q
	{
		m_pParent->SetNextVar(-1);
	}
	else
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nChar - 
//			nRepCnt - 
//			nFlags - 
//-----------------------------------------------------------------------------
void CMyComboBox::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CComboBox::OnChar(nChar, nRepCnt, nFlags);
	return;

	if(nChar == 1)	// ctrl+a
	{
		m_pParent->SetNextVar(1);
	}
	else if(nChar == 11)	// ctrl+q
	{
		m_pParent->SetNextVar(-1);
	}
	else
	{
		CComboBox::OnChar(nChar, nRepCnt, nFlags);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets the new face ID list from the pick face tool and updates the
//			contents of the edit control with space-delimited face IDs.
//-----------------------------------------------------------------------------
void COP_Entity::UpdatePickFaceText(CToolPickFace *pTool)
{
	char szList[KEYVALUE_MAX_VALUE_LENGTH];
	szList[0] = '\0';

	CMapFaceList FaceListFull;
	CMapFaceList FaceListPartial;

	pTool->GetSelectedFaces(FaceListFull, FaceListPartial);
	if (!CMapWorld::FaceID_FaceListsToString(szList, sizeof(szList), &FaceListFull, &FaceListPartial))
	{
		MessageBox("Too many faces selected for this keyvalue to hold. Deselect some faces.", "Error", MB_OK);
	}

	//
	// Update the edit control text with the new face IDs. This text will be
	// stuffed into the local keyvalue storage in OnChangeSmartControl.
	//
	m_pSmartControl->SetWindowText(szList);
}


//-----------------------------------------------------------------------------
// Purpose: Handles the message sent by the angles custom control when the user
//			changes the angle via the angle box or edit combo.
// Input  : WPARAM - The ID of the control that sent the message.
//			LPARAM - Unused.
//-----------------------------------------------------------------------------
LRESULT COP_Entity::OnChangeAngleBox(WPARAM nID, LPARAM)
{
	CString strKey;
	GetCurKey(strKey);

	char szValue[KEYVALUE_MAX_VALUE_LENGTH];
	bool bUpdateControl = false;
	if ((nID == IDC_ANGLEBOX) || (nID == IDC_ANGLEEDIT))
	{
		// From the main "angles" box.
		m_Angle.GetAngles(szValue);

		// Only update the edit control text if the "angles" key is selected.
		if (!strKey.CompareNoCase("angles"))
		{
			bUpdateControl = true;
		}

		// Slam "angles" into the key name since that's the key we're modifying.
		strKey = "angles";
	}
	else
	{
		// From the secondary angles box that edits the selected keyvalue.
		m_SmartAngle.GetAngles(szValue);
		bUpdateControl = true;
	}

	// Commit the change to our local storage.
	UpdateKeyValue(strKey, szValue);

	if (bUpdateControl)
	{
		if (m_bSmartedit)
		{
			// Reflect the change in the SmartEdit control.
			ASSERT(m_pSmartControl);
			if (m_pSmartControl)
			{
				m_bEnableControlUpdate = false;
				m_pSmartControl->SetWindowText(szValue);
				m_bEnableControlUpdate = true;
			}
		}
		else
		{
			// Reflect the change in the keyvalue control.
			m_bEnableControlUpdate = false;
			m_cValue.SetWindowText(szValue);
			m_bEnableControlUpdate = true;
		}
	}

	return 0;
}


