//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "KeyValues.h"
#include "MapFace.h"


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
MDkeyvalue::~MDkeyvalue(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Assignment operator.
//-----------------------------------------------------------------------------
MDkeyvalue &MDkeyvalue::operator =(const MDkeyvalue &other)
{
	strcpy(szKey, other.szKey);
	strcpy(szValue, other.szValue);

	return(*this);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Sets the initial size of the keyvalue array.
//-----------------------------------------------------------------------------
KeyValues::KeyValues(void)
{
	m_KeyValues.SetSize(0, 4);
	m_nKeyValueCount = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Deletes the contents of this keyvalue array.
//-----------------------------------------------------------------------------
KeyValues::~KeyValues(void)
{
	//int i = 0;
	//while (i < m_KeyValues.GetSize())
	//{
	//	delete m_KeyValues.GetAt(i++);
	//}

	RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			*piIndex - 
// Output : LPCTSTR
//-----------------------------------------------------------------------------
LPCTSTR KeyValues::GetValue(LPCTSTR pszKey, int *piIndex) const
{
	for(int i = 0; i < m_nKeyValueCount; i++)
	{
		if(!strcmpi(pszKey, m_KeyValues.GetData()[i].szKey))
		{
			if(piIndex)
				piIndex[0] = i;
			return m_KeyValues.GetData()[i].szValue;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//-----------------------------------------------------------------------------
void KeyValues::RemoveKey(LPCTSTR pszKey)
{
	SetValue(pszKey, LPCTSTR(NULL));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			iValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetValue(const char *pszKey, int iValue)
{
	CString str;
	str.Format("%d", iValue);
	SetValue(pszKey, str);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			pszValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetValue(const char *pszKey, const char *pszValue)
{
	char szTmpKey[KEYVALUE_MAX_KEY_LENGTH];
	char szTmpValue[KEYVALUE_MAX_VALUE_LENGTH];

	strcpy(szTmpKey, pszKey);

	if (pszValue != NULL)
	{
		strcpy(szTmpValue, pszValue);
	}
	else
	{
		szTmpValue[0] = 0;
	}

	//
	// Strip trailing whitespace from key and value.
	//
	int iLen = strlen(szTmpKey) - 1;
	while (szTmpKey[iLen] == ' ')
	{
		szTmpKey[iLen--] = 0;
	}

	iLen = strlen(szTmpValue) - 1;
	while (szTmpValue[iLen] == ' ')
	{
		szTmpValue[iLen--] = 0;
	}

	//
	// Look for this key in our list of keyvalues.
	//
	for (int i = 0; i < m_nKeyValueCount; i++)
	{
		MDkeyvalue &kv = m_KeyValues.GetData()[i];

		//
		// If we find the key...
		//
		if (!strcmpi(kv.szKey, szTmpKey))
		{
			//
			// If we are setting to a non-NULL value, set the value.
			//
			if (pszValue != NULL)
			{
				strcpy(kv.szValue, szTmpValue);
			}
			//
			// If we are setting to a NULL value, delete the key.
			//
			else
			{
				m_KeyValues.RemoveAt(i);
				--m_nKeyValueCount;
			}
			return;
		}
	}

	if (!pszValue)
	{
		return;
	}

	//
	// Add the keyvalue to our list.
	//
	MDkeyvalue newkv;
	strcpy(newkv.szKey, szTmpKey);
	strcpy(newkv.szValue, szTmpValue);
	m_KeyValues.SetAtGrow(m_nKeyValueCount++, newkv);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void KeyValues::RemoveAll(void)
{
	m_nKeyValueCount = 0;
	m_KeyValues.SetSize(0, 4);
	m_KeyValues.RemoveAll();
}
