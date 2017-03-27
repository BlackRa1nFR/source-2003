//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef HELPERINFO_H
#define HELPERINFO_H
#pragma once

#include <afxtempl.h>
#include <afxcoll.h>


#define MAX_HELPER_NAME_LEN			256


typedef CTypedPtrList<CPtrList, char *> CParameterList;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHelperInfo
{
	public:

		inline CHelperInfo(void);
		inline ~CHelperInfo(void);

		inline const char *GetName(void) const { return(m_szName); }
		inline void SetName(const char *pszName);

		inline bool AddParameter(const char *pszParameter);

		inline int GetParameterCount(void) const { return(m_Parameters.GetCount()); }
		inline const char *GetFirstParameter(POSITION &pos) const;
		inline const char *GetNextParameter(POSITION &pos) const;

	protected:

		char m_szName[MAX_HELPER_NAME_LEN];
		CParameterList m_Parameters;
};



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CHelperInfo::CHelperInfo(void)
{
	m_szName[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CHelperInfo::~CHelperInfo(void)
{
	POSITION pos = m_Parameters.GetHeadPosition();
	while (pos != NULL)
	{
		char *pszParam = m_Parameters.GetNext(pos);
		ASSERT(pszParam != NULL);
		if (pszParam != NULL)
		{
			delete [] pszParam;
		}
	}

	m_Parameters.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *pszParameter - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CHelperInfo::AddParameter(const char *pszParameter)
{
	if ((pszParameter != NULL) && (pszParameter[0] != '\0'))
	{
		int nLen = strlen(pszParameter);
		
		if (nLen > 0)
		{
			char *pszNew = new char [nLen + 1];
			if (pszNew != NULL)
			{
				strcpy(pszNew, pszParameter);
				m_Parameters.AddTail(pszNew);
				return(true);
			}
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos - 
// Output : CClassInput
//-----------------------------------------------------------------------------
inline const char *CHelperInfo::GetFirstParameter(POSITION &pos) const
{
	pos = m_Parameters.GetHeadPosition(); 

	if (pos != NULL)
	{
		return(m_Parameters.GetNext(pos));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
// Output : const char
//-----------------------------------------------------------------------------
inline const char *CHelperInfo::GetNextParameter(POSITION &pos) const
{
	if (pos != NULL)
	{
		return(m_Parameters.GetNext(pos));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *pszName - 
//-----------------------------------------------------------------------------
inline void CHelperInfo::SetName(const char *pszName)
{
	if (pszName != NULL)
	{	
		strcpy(m_szName, pszName);
	}
}


typedef CTypedPtrList<CPtrList, CHelperInfo *> CHelperInfoList;


#endif // HELPERINFO_H
