//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "GameData.h" // UNDONE: eliminate dependency
#include "GDClass.h"


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
GDclass::GDclass(void)
{
	m_nVariables = 0;
	m_bBase = false;
	m_bSolid = false;
	m_bBase = false;
	m_bSolid = false;
	m_bModel = false;
	m_bMove = false;
	m_bKeyFrame = false;
	m_bPoint = false;
	m_bNPC = false;
	m_bFilter = false;

	m_bHalfGridSnap = false;

	m_bGotSize = false;
	m_bGotColor = false;

	m_rgbColor.r = 220;
	m_rgbColor.g = 30;
	m_rgbColor.b = 220;
	m_rgbColor.a = 0;

	m_pszDescription = NULL;

	for (int i = 0; i < 3; i++)
	{
		m_bmins[i] = -8;
		m_bmaxs[i] = 8;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees variable and helper lists.
//-----------------------------------------------------------------------------
GDclass::~GDclass(void)
{
	//
	// Free variables.
	//
	POSITION pos = m_Variables.GetHeadPosition();
	while (pos != NULL)
	{
		GDinputvariable *pvi = m_Variables.GetNext(pos);
		delete pvi;
	}
	m_Variables.RemoveAll();

	//
	// Free helpers.
	//
	pos = m_Helpers.GetHeadPosition();
	while (pos != NULL)
	{
		CHelperInfo *pHelper = m_Helpers.GetNext(pos);
		delete pHelper;
	}
	m_Helpers.RemoveAll();

	//
	// Free inputs.
	//
	pos = m_Inputs.GetHeadPosition();
	while (pos != NULL)
	{
		CClassInput *pInput = m_Inputs.GetNext(pos);
		delete pInput;
	}
	m_Inputs.RemoveAll();

	//
	// Free outputs.
	//
	pos = m_Outputs.GetHeadPosition();
	while (pos != NULL)
	{
		CClassOutput *pOutput = m_Outputs.GetNext(pos);
		delete pOutput;
	}
	m_Outputs.RemoveAll();

	delete m_pszDescription;
}


//-----------------------------------------------------------------------------
// Purpose: Adds the base class's variables to our variable list. Acquires the
//			base class's bounding box and color, if any.
// Input  : pszBase - Name of base class to add.
//-----------------------------------------------------------------------------
void GDclass::AddBase(GDclass *pBase)
{
	int iBaseIndex;
	Parent->ClassForName(pBase->GetName(), &iBaseIndex);

	//
	// Add variables from base - update variable table
	//
	for (int i = 0; i < pBase->GetVariableCount(); i++)
	{
		GDinputvariable *pVar = pBase->GetVariableAt(i);
		AddVariable(pVar, pBase, iBaseIndex, i);
	}

	//
	// Add inputs from the base.
	// UNDONE: need to use references to inputs & outputs to conserve memory
	//
	POSITION pos;
	CClassInput *pInput = pBase->GetFirstInput(pos);
	while (pInput != NULL)
	{
		CClassInput *pNew = new CClassInput;
		*pNew = *pInput;
		AddInput(pNew);

		pInput = pBase->GetNextInput(pos);
	}

	//
	// Add outputs from the base.
	//
	CClassOutput *pOutput = pBase->GetFirstOutput(pos);
	while (pOutput != NULL)
	{
		CClassOutput *pNew = new CClassOutput;
		*pNew = *pOutput;
		AddOutput(pNew);

		pOutput = pBase->GetNextOutput(pos);
	}

	//
	// If we don't have a bounding box, try to get the base's box.
	//
	if (!m_bGotSize)
	{
		if (pBase->GetBoundBox(m_bmins, m_bmaxs))
		{
			m_bGotSize = true;
		}
	}

	//
	// If we don't have a color, use the base's color.
	//
	if (!m_bGotColor)
	{
		m_rgbColor = pBase->GetColor();
		m_bGotColor = true;
	}	
}


//-----------------------------------------------------------------------------
// Purpose: Adds the given GDInputVariable to this GDClass's list of variables.
// Input  : pVar - 
//			pBase - 
//			iBaseIndex - 
//			iVarIndex - 
// Output : Returns TRUE if the pVar pointer was copied directly into this GDClass,
//			FALSE if not. If this function returns TRUE, pVar should not be
//			deleted by the caller.
//-----------------------------------------------------------------------------
BOOL GDclass::AddVariable(GDinputvariable *pVar, GDclass *pBase, int iBaseIndex, int iVarIndex)
{
	int iThisIndex;
	GDinputvariable *pThisVar = VarForName(pVar->GetName(), &iThisIndex);
	
	//
	// Check to see if we are overriding an existing variable definition.
	//
	if (pThisVar != NULL)
	{
		//
		// Same name, different type. Flag this as an error.
		//
		if (pThisVar->GetType() != pVar->GetType())
		{
			return(false);
		}

		GDinputvariable *pAddVar;
		bool bReturn;

		//
		// Check to see if we need to combine a choices/flags array.
		//
		if (pVar->GetType() == ivFlags || pVar->GetType() == ivChoices)
		{
			//
			// Combine two variables' flags into a new variable. Add the new
			// variable to the local variable list and modify the old variable's
			// position in our variable map to reflect the new local variable.
			// This way, we can have multiple inheritance.
			//
			GDinputvariable *pNewVar = new GDinputvariable;

			*pNewVar = *pVar;
			pNewVar->Merge(*pThisVar);

			pAddVar = pNewVar;
			bReturn = false;
		}
		else
		{
			pAddVar = pVar;
			bReturn = true;
		}

		if (m_VariableMap[iThisIndex][0] == -1)
		{
			//
			// "pThisVar" is a leaf variable - we can remove since it is overridden.
			//
			POSITION p = m_Variables.Find(pThisVar);
			ASSERT(p);
			delete pThisVar;

			m_Variables.SetAt(p, pAddVar);

			//
			// No need to modify variable map - we just replaced
			// the pointer in the local variable list.
			//
		}
		else
		{
			//
			// "pThisVar" was declared in a base class - we can replace the reference in
			// our variable map with the new variable.
			//
			m_VariableMap[iThisIndex][0] = iBaseIndex;

			if (iBaseIndex == -1)
			{
				m_Variables.AddTail(pAddVar);
				m_VariableMap[iThisIndex][1] = m_Variables.GetCount() - 1;
			}
			else
			{
				m_VariableMap[iThisIndex][1] = iVarIndex;
			}
		}

		return(bReturn);
	}
	
	//
	// New variable.
	//
	if (iBaseIndex == -1)
	{
		//
		// Variable declared in the leaf class definition - add it to the list.
		//
		m_Variables.AddTail(pVar);
	}

	//
	// Too many variables already declared in this class definition - abort.
	//
	if (m_nVariables == GD_MAX_VARIABLES)
	{
		CString str;
		str.Format("Too many gamedata variables for class \"%s\"", m_szName);
		AfxMessageBox(str);
		return(false);
	}

	//
	// Add the variable to our list.
	//
	m_VariableMap[m_nVariables][0] = iBaseIndex;
	m_VariableMap[m_nVariables][1] = iVarIndex;
	++m_nVariables;
	
	//
	// We added the pointer to our list of items (see Variables.AddTail, above) so
	// we must return true here.
	//
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Finds an input with the given name.
// Input  : szName - Input name to search for.
// Output : Returns a pointer to the input with the given name, NULL if not found.
//-----------------------------------------------------------------------------
CClassInput *GDclass::FindInput(const char *szName)
{
	POSITION pos;

	CClassInput *pInput = GetFirstInput(pos);
	while (pInput != NULL)
	{
		if (!stricmp(pInput->GetName(), szName))
		{
			return(pInput);
		}
		pInput = GetNextInput(pos);
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Finds an output with the given name.
// Input  : szName - Output name to search for.
// Output : Returns a pointer to the output with the given name, NULL if not found.
//-----------------------------------------------------------------------------
CClassOutput *GDclass::FindOutput(const char *szName)
{
	POSITION pos;

	CClassOutput *pOutput = GetFirstOutput(pos);
	while (pOutput != NULL)
	{
		if (!stricmp(pOutput->GetName(), szName))
		{
			return(pOutput);
		}
		pOutput = GetNextOutput(pos);
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Gets the mins and maxs of the class's bounding box as read from the
//			FGD file. This controls the onscreen representation of any entities
//			derived from this class.
// Input  : pfMins - Receives minimum X, Y, and Z coordinates for the class.
//			pfMaxs - Receives maximum X, Y, and Z coordinates for the class.
// Output : Returns TRUE if this class has a specified bounding box, FALSE if not.
//-----------------------------------------------------------------------------
BOOL GDclass::GetBoundBox(Vector& pfMins, Vector& pfMaxs)
{
	if (m_bGotSize)
	{
		pfMins[0] = m_bmins[0];
		pfMins[1] = m_bmins[1];
		pfMins[2] = m_bmins[2];

		pfMaxs[0] = m_bmaxs[0];
		pfMaxs[1] = m_bmaxs[1];
		pfMaxs[2] = m_bmaxs[2];
	}

	return(m_bGotSize);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos - 
// Output : 
//-----------------------------------------------------------------------------
CHelperInfo *GDclass::GetFirstHelper(POSITION &pos)
{
	pos = m_Helpers.GetHeadPosition();
	if (pos != NULL)
	{
		return(m_Helpers.GetNext(pos));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos - 
// Output : CClassInput
//-----------------------------------------------------------------------------
CClassInput *GDclass::GetFirstInput(POSITION &pos)
{
	pos = m_Inputs.GetHeadPosition(); 
	if (pos != NULL)
	{
		return(m_Inputs.GetNext(pos));
	}
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos - 
// Output : CClassOutput
//-----------------------------------------------------------------------------
CClassOutput *GDclass::GetFirstOutput(POSITION &pos)
{
	pos = m_Outputs.GetHeadPosition(); 
	if (pos != NULL)
	{
		return(m_Outputs.GetNext(pos));
	}
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tr - 
//			pGD - 
// Output : Returns TRUE if worth continuing, FALSE otherwise.	
//-----------------------------------------------------------------------------
BOOL GDclass::InitFromTokens(TokenReader& tr, GameData *pGD)
{
	Parent = pGD;

	//
	// Initialize VariableMap
	//
	for (int i = 0; i < GD_MAX_VARIABLES; i++)
	{
		m_VariableMap[i][0] = -1;
		m_VariableMap[i][1] = -1;
	}

	//
	// Parse all specifiers (base, size, color, etc.)
	//
	if (!ParseSpecifiers(tr))
	{
		return(FALSE);
	}

	//
	// Specifiers should be followed by an "="
	//
	if (!GDSkipToken(tr, OPERATOR, "="))
	{
		return(FALSE);
	}

	//
	// Parse the class name.
	//
	if (!GDGetToken(tr, m_szName, sizeof(m_szName), IDENT))
	{
		return(FALSE);
	}

	//
	// Check next operator - if ":", we have a description - if "[",
	// we have no description.
	//
	char szToken[MAX_TOKEN];
	if ((tr.PeekTokenType(szToken) == OPERATOR) && IsToken(szToken, ":"))
	{
		// Skip ":"
		tr.NextToken(szToken, sizeof(szToken));

		//
		// Free any existing description and set the pointer to NULL so that GDGetToken
		// allocates memory for us.
		//
		delete m_pszDescription;
		m_pszDescription = NULL;

		// Load description
		if (!GDGetTokenDynamic(tr, &m_pszDescription, STRING))
		{
			return(FALSE);
		}
	}

	//
	// Opening square brace.
	//
	if (!GDSkipToken(tr, OPERATOR, "["))
	{
		return(FALSE);
	}

	//
	// Get class variables.
	//
	if (!ParseVariables(tr))
	{
		return(FALSE);
	}

	//
	// Closing square brace.
	//
	if (!GDSkipToken(tr, OPERATOR, "]"))
	{
		return(FALSE);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseBase(TokenReader &tr)
{
	char szToken[MAX_TOKEN];

	while (1)
	{
		if (!GDGetToken(tr, szToken, sizeof(szToken), IDENT))
		{
			return(false);
		}

		//
		// Find base class in list of classes.
		//
		GDclass *pBase = Parent->ClassForName(szToken);
		if (pBase == NULL)
		{
			GDError(tr, "undefined base class '%s", szToken);
			return(false);
		}

		AddBase(pBase);

		if (!GDGetToken(tr, szToken, sizeof(szToken), OPERATOR))
		{
			return(false);
		}

		if (IsToken(szToken, ")"))
		{
			break;
		}
		else if (!IsToken(szToken, ","))
		{
			GDError(tr, "expecting ',' or ')', but found %s", szToken);
			return(false);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseColor(TokenReader &tr)
{
	char szToken[MAX_TOKEN];

	//
	// Red.
	//
	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return(false);
	}
	BYTE r = atoi(szToken);

	//
	// Green.
	//
	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return(false);
	}
	BYTE g = atoi(szToken);

	//
	// Blue.
	//
	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return(false);
	}
	BYTE b = atoi(szToken);

	m_rgbColor.r = r;
	m_rgbColor.g = g;
	m_rgbColor.b = b;
	m_rgbColor.a = 0;

	m_bGotColor = true;

	if (!GDGetToken(tr, szToken, sizeof(szToken), OPERATOR, ")"))
	{
		return(false);
	}
	
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Parses a helper from the FGD file. Helpers are of the following format:
//
//			<helpername>(<parameter 0> <parameter 1> ... <parameter n>)
//
//			When this function is called, the helper name has already been parsed.
// Input  : tr - Tokenreader to use for parsing.
//			pszHelperName - Name of the helper being declared.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseHelper(TokenReader &tr, char *pszHelperName)
{
	char szToken[MAX_TOKEN];

	CHelperInfo *pHelper = new CHelperInfo;
	pHelper->SetName(pszHelperName);

	bool bCloseParen = false;
	while (!bCloseParen)
	{
		trtoken_t eType = tr.PeekTokenType(szToken);

		if (eType == OPERATOR)
		{
			if (!GDGetToken(tr, szToken, sizeof(szToken), OPERATOR))
			{
				delete pHelper;
				return(false);
			}

			if (IsToken(szToken, ")"))
			{
				bCloseParen = true;
			}
			else if (IsToken(szToken, "="))
			{
				delete pHelper;
				return(false);
			}
		}
		else
		{
			if (!GDGetToken(tr, szToken, sizeof(szToken), eType))
			{
				delete pHelper;
				return(false);
			}
			else
			{
				pHelper->AddParameter(szToken);
			}
		}
	}

	m_Helpers.AddTail(pHelper);

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseSize(TokenReader &tr)
{
	char szToken[MAX_TOKEN];

	//
	// Mins.
	//
	for (int i = 0; i < 3; i++)
	{
		if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
		{
			return(false);
		}

		m_bmins[i] = (float)atof(szToken);
	}

	if (tr.PeekTokenType(szToken) == OPERATOR && IsToken(szToken, ","))
	{
		//
		// Skip ","
		//
		tr.NextToken(szToken, sizeof(szToken));

		//
		// Get maxes.
		//
		for (int i = 0; i < 3; i++)
		{
			if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
			{
				return(false);
			}
			m_bmaxs[i] = (float)atof(szToken);
		}
	}
	else
	{
		//
		// Split mins across origin.
		//
		for (int i = 0; i < 3; i++)
		{
			float div2 = m_bmins[i] / 2;
			m_bmaxs[i] = div2;
			m_bmins[i] = -div2;
		}
	}

	m_bGotSize = true;

	if (!GDGetToken(tr, szToken, sizeof(szToken), OPERATOR, ")"))
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseSpecifiers(TokenReader &tr)
{
	char szToken[MAX_TOKEN];

	while (tr.PeekTokenType() == IDENT)
	{
		tr.NextToken(szToken, sizeof(szToken));

		//
		// Handle specifiers that don't have any parens after them.
		//
		if (IsToken(szToken, "halfgridsnap"))
		{
			m_bHalfGridSnap = true;
		}
		else
		{
			//
			// Handle specifiers require parens after them.
			//
			if (!GDSkipToken(tr, OPERATOR, "("))
			{
				return(false);
			}

			if (IsToken(szToken, "base"))
			{
				if (!ParseBase(tr))
				{
					return(false);
				}
			}
			else if (IsToken(szToken, "size"))
			{
				if (!ParseSize(tr))
				{
					return(false);
				}
			}
			else if (IsToken(szToken, "color"))
			{
				if (!ParseColor(tr))
				{
					return(false);
				}
			}
			else if (!ParseHelper(tr, szToken))
			{
				return(false);
			}
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Reads an input using a given token reader. If the input is
//			read successfully, the input is added to this class. If not, a
//			parsing failure is returned.
// Input  : tr - Token reader to use for parsing.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseInput(TokenReader &tr)
{
	char szToken[MAX_TOKEN];

	if (!GDGetToken(tr, szToken, sizeof(szToken), IDENT, "input"))
	{
		return(false);
	}

	CClassInput *pInput = new CClassInput;

	bool bReturn = ParseInputOutput(tr, pInput);
	if (bReturn)
	{
		AddInput(pInput);
	}
	else
	{
		delete pInput;
	}

	return(bReturn);
}


//-----------------------------------------------------------------------------
// Purpose: Reads an input or output using a given token reader.
// Input  : tr - Token reader to use for parsing.
//			pInputOutput - Input or output to fill out.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseInputOutput(TokenReader &tr, CClassInputOutputBase *pInputOutput)
{
	char szToken[MAX_TOKEN];

	//
	// Read the name.
	//
	if (!GDGetToken(tr, szToken, sizeof(szToken), IDENT))
	{
		return(false);
	}

	pInputOutput->SetName(szToken);

	//
	// Read the type.
	//
	if (!GDGetToken(tr, szToken, sizeof(szToken), OPERATOR, "("))
	{
		return(false);
	}

	if (!GDGetToken(tr, szToken, sizeof(szToken), IDENT))
	{
		return(false);
	}

	InputOutputType_t eType = pInputOutput->SetType(szToken);
	if (eType == iotInvalid)
	{
		GDError(tr, "bad input/output type '%s'", szToken);
		return(false);
	}

	if (!GDGetToken(tr, szToken, sizeof(szToken), OPERATOR, ")"))
	{
		return(false);
	}

	//
	// Check the next operator - if ':', we have a description.
	//
	if ((tr.PeekTokenType(szToken) == OPERATOR) && (IsToken(szToken, ":")))
	{
		//
		// Skip the ":".
		//
		tr.NextToken(szToken, sizeof(szToken));

		//
		// Read the description.
		//
		char *pszDescription;
		if (!GDGetTokenDynamic(tr, &pszDescription, STRING))
		{
			return(false);
		}

		pInputOutput->SetDescription(pszDescription);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Reads an output using a given token reader. If the output is
//			read successfully, the output is added to this class. If not, a
//			parsing failure is returned.
// Input  : tr - Token reader to use for parsing.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseOutput(TokenReader &tr)
{
	char szToken[MAX_TOKEN];

	if (!GDGetToken(tr, szToken, sizeof(szToken), IDENT, "output"))
	{
		return(false);
	}

	CClassOutput *pOutput = new CClassOutput;

	bool bReturn = ParseInputOutput(tr, pOutput);
	if (bReturn)
	{
		AddOutput(pOutput);
	}
	else
	{
		delete pOutput;
	}

	return(bReturn);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GDclass::ParseVariables(TokenReader &tr)
{
	while (1)
	{
		char szToken[MAX_TOKEN];

		if (tr.PeekTokenType(szToken) == OPERATOR)
		{
			break;
		}

		if (!stricmp(szToken, "input"))
		{
			if (!ParseInput(tr))
			{
				return(false);
			}

			continue;
		}

		if (!stricmp(szToken, "output"))
		{
			if (!ParseOutput(tr))
			{
				return(false);
			}

			continue;
		}

		if (!stricmp(szToken, "key"))
		{
			GDGetToken(tr, szToken, sizeof(szToken));
		}

		GDinputvariable * var = new GDinputvariable;
		if (!var->InitFromTokens(tr))
		{
			delete var;
			return(false);
		}

		int nDupIndex;
		GDinputvariable *pDupVar = VarForName(var->GetName(), &nDupIndex);
		
		// check for duplicate variable definitions
		if (pDupVar)
		{
			// Same name, different type.
			if (pDupVar->GetType() != var->GetType())
			{
				char szError[_MAX_PATH];

				sprintf(szError, "%s: Variable '%s' is multiply defined with different types.", GetName(), var->GetName());
				GDError(tr, szError);
			}
		}

		if (!AddVariable(var, this, -1, m_Variables.GetCount()))
		{
			delete var;
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iIndex - 
// Output : GDinputvariable *
//-----------------------------------------------------------------------------
GDinputvariable *GDclass::GetVariableAt(int iIndex)
{
	ASSERT(iIndex < m_nVariables);
	GDclass *pVarClass;

	if (m_VariableMap[iIndex][0] == -1)
	{
		// in this class
		POSITION p = m_Variables.FindIndex(m_VariableMap[iIndex][1]);
		ASSERT(p);
		return m_Variables.GetAt(p);
	}
	else
	{
		// find var's owner
		POSITION p = Parent->Classes.FindIndex(m_VariableMap[iIndex][0]);
		pVarClass = Parent->Classes.GetAt(p);
	}

	// find var in pVarClass
	return pVarClass->GetVariableAt(m_VariableMap[iIndex][1]);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			*piIndex - 
// Output : GDinputvariable *
//-----------------------------------------------------------------------------
GDinputvariable *GDclass::VarForName(LPCTSTR pszName, int *piIndex)
{
	for(int i = 0; i < GetVariableCount(); i++)
	{
		GDinputvariable *pVar = GetVariableAt(i);
		if(!strcmpi(pVar->GetName(), pszName))
		{
			if(piIndex)
				piIndex[0] = i;
			return pVar;
		}
	}

	return NULL;
}


