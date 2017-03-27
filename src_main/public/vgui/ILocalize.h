//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ILOCALIZE_H
#define ILOCALIZE_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

// unicode character type
// for more unicode manipulation functions #include <wchar.h>
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

class IFileSystem;

namespace vgui
{

// direct references to localized strings
typedef unsigned short StringIndex_t;
const unsigned short INVALID_STRING_INDEX = (unsigned short) -1;

//-----------------------------------------------------------------------------
// Purpose: Handles localization of text
//			looks up string names and returns the localized unicode text
//-----------------------------------------------------------------------------
class ILocalize : public IBaseInterface
{
public:
	// adds the contents of a file to the localization table
	virtual bool AddFile(IFileSystem *fileSystem, const char *fileName) = 0;

	// Remove all strings from the table
	virtual void RemoveAll() = 0;

	// Finds the localized text for tokenName
	virtual wchar_t *Find(char const *tokenName) = 0;

	// converts an english string to unicode
	// returns the number of wchar_t in resulting string, including null terminator
	virtual int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSizeInBytes) = 0;

	// converts an unicode string to an english string
	// unrepresentable characters are converted to system default
	// returns the number of characters in resulting string, including null terminator
	virtual int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize) = 0;

	// finds the index of a token by token name, INVALID_STRING_INDEX if not found
	virtual StringIndex_t FindIndex(const char *tokenName) = 0;

	// builds a localized formatted string
	// uses the format strings first: %s1, %s2, ...  unicode strings (wchar_t *)
	virtual void ConstructString(wchar_t *unicodeOuput, int unicodeBufferSizeInBytes, wchar_t *formatString, int numFormatParameters, ...) = 0;
	
	// gets the values by the string index
	virtual const char *GetNameByIndex(StringIndex_t index) = 0;
	virtual wchar_t *GetValueByIndex(StringIndex_t index) = 0;

	///////////////////////////////////////////////////////////////////
	// the following functions should only be used by localization editors

	// iteration functions
	virtual StringIndex_t GetFirstStringIndex() = 0;
	// returns the next index, or INVALID_STRING_INDEX if no more strings available
	virtual StringIndex_t GetNextStringIndex(StringIndex_t index) = 0;

	// adds a single name/unicode string pair to the table
	virtual void AddString(const char *tokenName, wchar_t *unicodeString, const char *fileName) = 0;

	// changes the value of a string
	virtual void SetValueByIndex(StringIndex_t index, wchar_t *newValue) = 0;

	// saves the entire contents of the token tree to the file
	virtual bool SaveToFile(IFileSystem *fileSystem, const char *fileName) = 0;

	// iterates the filenames
	virtual int GetLocalizationFileCount() = 0;
	virtual const char *GetLocalizationFileName(int index) = 0;

	// returns the name of the file the specified localized string is stored in
	virtual const char *GetFileNameByIndex(StringIndex_t index) = 0;
};

#define VGUI_LOCALIZE_INTERFACE_VERSION "VGUI_Localize002"

}; // namespace vgui

#endif // ILOCALIZE_H
