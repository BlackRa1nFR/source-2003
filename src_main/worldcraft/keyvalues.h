//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef KEYVALUES_H
#define KEYVALUES_H
#pragma once


#include <afxtempl.h>


class fstream;


#define KEYVALUE_MAX_KEY_LENGTH			80
#define KEYVALUE_MAX_VALUE_LENGTH		512


class MDkeyvalue 
{
	public:

		//
		// Constructors/Destructor.
		//
		inline MDkeyvalue(void);
		inline MDkeyvalue(const char *pszKey, const char *pszValue);
		~MDkeyvalue(void);

		MDkeyvalue &operator =(const MDkeyvalue &other);
		
		inline void Set(const char *pszKey, const char *pszValue);
		inline const char *Key(void) const;
		inline const char *Value(void) const;

		//
		// Serialization functions.
		//
		int SerializeRMF(fstream &f, BOOL bRMF);
		int SerializeMAP(fstream &f, BOOL bRMF);

		char szKey[KEYVALUE_MAX_KEY_LENGTH];			// The name of this key.
		char szValue[KEYVALUE_MAX_VALUE_LENGTH];		// The value of this key, stored as a string.
};


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
MDkeyvalue::MDkeyvalue(void)
{
	szKey[0] = '\0';
	szValue[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: Constructor with assignment.
//-----------------------------------------------------------------------------
MDkeyvalue::MDkeyvalue(const char *pszKey, const char *pszValue)
{
	szKey[0] = '\0';
	szValue[0] = '\0';

	Set(pszKey, pszValue);
}


//-----------------------------------------------------------------------------
// Purpose: Assigns a key and value.
//-----------------------------------------------------------------------------
void MDkeyvalue::Set(const char *pszKey, const char *pszValue)
{
	ASSERT(pszKey);
	ASSERT(pszValue);

	strcpy(szKey, pszKey);
	strcpy(szValue, pszValue);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the string keyname.
//-----------------------------------------------------------------------------
const char *MDkeyvalue::Key(void) const
{
	return szKey;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the string value of this keyvalue.
//-----------------------------------------------------------------------------
const char *MDkeyvalue::Value(void) const
{
	return szValue;
}


typedef CArray<MDkeyvalue, MDkeyvalue &> KeyValueArray;


class KeyValues
{
	public:

		KeyValues(void);
		~KeyValues(void);

		void RemoveAll(void);
		inline void RemoveKey(int nIndex);
		void RemoveKey(LPCTSTR pszKey);
	
		void SetValue(const char *pszKey, const char *pszValue);
		void SetValue(const char *pszKey, int iValue);
		inline void SetSize(int nSize);

		inline int GetCount(void) const;
		inline LPCTSTR GetKey(int nIndex) const;
		inline MDkeyvalue &GetKeyValue(int nIndex);
		inline MDkeyvalue GetKeyValue(int nIndex) const;
		inline LPCTSTR GetValue(int nIndex) const;
		LPCTSTR GetValue(LPCTSTR pszKey, int *piIndex = NULL) const;

	protected:

		KeyValueArray m_KeyValues;
		int m_nKeyValueCount;
};


//-----------------------------------------------------------------------------
// Purpose: Returns the number of keyvalues in the keyvalue list.
//-----------------------------------------------------------------------------
int KeyValues::GetCount(void) const
{
	return(m_nKeyValueCount);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
LPCTSTR KeyValues::GetKey(int nIndex) const
{
	ASSERT(nIndex < m_KeyValues.GetSize());
	ASSERT(nIndex < m_nKeyValueCount);

	return(m_KeyValues.GetData()[nIndex].szKey);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
// Output : MDKeyValue
//-----------------------------------------------------------------------------
MDkeyvalue &KeyValues::GetKeyValue(int nIndex)
{
	ASSERT(nIndex < m_KeyValues.GetSize());
	ASSERT(nIndex < m_nKeyValueCount);

	return(m_KeyValues.GetData()[nIndex]);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
// Output : MDkeyvalue
//-----------------------------------------------------------------------------
MDkeyvalue KeyValues::GetKeyValue(int nIndex) const
{
	ASSERT(nIndex < m_KeyValues.GetSize());
	ASSERT(nIndex < m_nKeyValueCount);

	return(m_KeyValues.GetData()[nIndex]);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
LPCTSTR KeyValues::GetValue(int nIndex) const
{
	ASSERT(nIndex < m_KeyValues.GetSize());
	ASSERT(nIndex < m_nKeyValueCount);

	return(m_KeyValues.GetData()[nIndex].szValue);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void KeyValues::RemoveKey(int nIndex)
{
	ASSERT(nIndex < m_KeyValues.GetSize());
	ASSERT(nIndex < m_nKeyValueCount);

	if ((nIndex >= 0) && (nIndex < m_nKeyValueCount))
	{
		m_KeyValues.RemoveAt(nIndex);
		m_nKeyValueCount--;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Preallocates elements of the array. This does not affect the count
//			of elements reserved by GetCount.
// Input  : nSize - Number of array elements to allocate.
//-----------------------------------------------------------------------------
void KeyValues::SetSize(int nSize)
{
	m_KeyValues.SetSize(nSize);
}

#endif // KEYVALUES_H
