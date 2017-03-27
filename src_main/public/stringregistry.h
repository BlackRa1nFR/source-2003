//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		A registry of strings and associated ints
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef STRINGREGISTRY_H
#define STRINGREGISTRY_H
#pragma once

struct StringTable_t;

//-----------------------------------------------------------------------------
// Purpose: Just a convenience/legacy wrapper for CUtlDict<> .
//-----------------------------------------------------------------------------
class CStringRegistry
{
private:
	StringTable_t	*m_pStringList;

public:
	// returns a key for a given string
	unsigned short AddString(const char *stringText, int stringID);

	// This is optimized.  It will do 2 O(logN) searches
	// Only one of the searches needs to compare strings, the other compares symbols (ints)
	// returns -1 if the string is not present in the registry.
	int		GetStringID(const char *stringText);

	// This is unoptimized.  It will linearly search (but only compares ints, not strings)
	const char	*GetStringText(int stringID);

	// This is O(1).  It will not search.  key MUST be a value that was returned by AddString
	const char *GetStringForKey(unsigned short key);
	// This is O(1).  It will not search.  key MUST be a value that was returned by AddString
	int		GetIDForKey(unsigned short key);

	void	ClearStrings(void);

	~CStringRegistry(void);			// Need to free allocated memory
	CStringRegistry(void);
};

#endif // STRINGREGISTRY_H
