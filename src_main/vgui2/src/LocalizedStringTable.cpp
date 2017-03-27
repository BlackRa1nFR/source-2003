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

#include <windows.h>
#include <wchar.h>

#include "vgui_internal.h"
#include "FileSystem.h"

#include <vgui/ILocalize.h>
#include <vgui/ISystem.h>

#include "UtlVector.h"
#include "UtlRBTree.h"
#include "UtlSymbol.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Maps token names to localized unicode strings
//-----------------------------------------------------------------------------
class CLocalizedStringTable : public vgui::ILocalize
{
public:
	CLocalizedStringTable();
	~CLocalizedStringTable();

	// adds the contents of a file to the localization table
	bool AddFile(IFileSystem *fileSystem, const char *fileName);

	// saves the entire contents of the token tree to the file
	bool SaveToFile(IFileSystem *fileSystem, const char *fileName);

	// adds a single name/unicode string pair to the table
	void AddString(const char *tokenName, wchar_t *unicodeString, const char *fileName);

	// Finds the localized text for pName
	wchar_t *Find(char const *pName);

	// finds the index of a token by token name
	StringIndex_t FindIndex(const char *pName);
	
	// Remove all strings in the table.
	void RemoveAll();

	// converts an english string to unicode
	// returns the number of wchar_t in resulting string, including null terminator
	int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSizeInBytes);

	// converts an unicode string to an english string
	// unrepresentable characters are converted to system default
	// returns the number of characters in resulting string, including null terminator
	int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize);

	void ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, wchar_t *formatString, int numFormatParameters, ...);

	// iteration functions
	StringIndex_t GetFirstStringIndex();

	// returns the next index, or INVALID_STRING_INDEX if no more strings available
	StringIndex_t GetNextStringIndex(StringIndex_t index);

	// gets the values from the index
	const char *GetNameByIndex(StringIndex_t index);
	wchar_t *GetValueByIndex(StringIndex_t index);
	const char *GetFileNameByIndex(StringIndex_t index);

	// sets the value in the index
	// has bad memory characteristics, should only be used in the editor
	void SetValueByIndex(StringIndex_t index, wchar_t *newValue);

	// iterates the filenames
	int GetLocalizationFileCount();
	const char *GetLocalizationFileName(int index);

private:
	char m_szLanguage[64];

	struct localizedstring_t
	{
		StringIndex_t nameIndex;
		StringIndex_t valueIndex;
		CUtlSymbol filename;
	};

	// Stores the symbol lookup
	CUtlRBTree<localizedstring_t, StringIndex_t> m_Lookup;
	
	// stores the string data
	CUtlVector<char> m_Names;
	CUtlVector<wchar_t> m_Values;
	CUtlSymbol m_CurrentFile;
	CUtlVector<CUtlSymbol> m_LocalizationFiles;
		
	// Less function, for sorting strings
	static bool SymLess( localizedstring_t const& i1, localizedstring_t const& i2 );
};

// global instance of table
CLocalizedStringTable g_StringTable;

// expose the interface
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CLocalizedStringTable, ILocalize, VGUI_LOCALIZE_INTERFACE_VERSION, g_StringTable);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLocalizedStringTable::CLocalizedStringTable() : 
	m_Lookup( 0, 0, SymLess ), m_Names( 1024 ), m_Values( 2048 )
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLocalizedStringTable::~CLocalizedStringTable()
{
	m_Names.Purge();
	m_Values.Purge();
	m_LocalizationFiles.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Advances until non-whitespace hit
//-----------------------------------------------------------------------------
wchar_t *AdvanceOverWhitespace(wchar_t *Start)
{
	while (*Start != 0 && iswspace(*Start))
	{
		Start++;
	}

	return Start;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
wchar_t *ReadUnicodeToken(wchar_t *start, wchar_t *token, int tokenBufferSize)
{
	// skip over any whitespace
	start = AdvanceOverWhitespace(start);

	if (!*start)
	{
		*token = 0;
		return start;
	}

	// check to see if it's a quoted string
	if (*start == '\"')
	{
		// copy out the string until we hit an end quote
		start++;
		int count = 0;
		while (*start && *start != '\"' && count < tokenBufferSize)
		{
			// check for special characters
			if (*start == '\\' && *(start+1) == 'n')
			{
				start++;
				*token = '\n';
			}
			else
			{
				*token = *start;
			}

			start++;
			token++;
			count++;
		}

		if (*start == '\"')
		{
			start++;
		}
	}
	else
	{
		// copy out the string until we hit a whitespace
		int count = 0;
		while (*start && !iswspace(*start) && count < tokenBufferSize)
		{
			// no checking for special characters if it's not a quoted string
			*token = *start;

			start++;
			token++;
			count++;
		}
	}

	*token = 0;
	return start;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first character after the next EOL characters
//-----------------------------------------------------------------------------
wchar_t *ReadToEndOfLine(wchar_t *start)
{
	if (!*start)
		return start;

	while (*start)
	{
		if (*start == 0x0D || *start== 0x0A)
			break;
		start++;
	}

	while (*start == 0x0D || *start== 0x0A)
		start++;

	return start;
}

//-----------------------------------------------------------------------------
// Purpose: Adds the contents of a file 
//-----------------------------------------------------------------------------
bool CLocalizedStringTable::AddFile(IFileSystem *fileSystem, const char *szFileName)
{
	// use the correct file based on the chosen language
	static char const *const LANGUAGE_STRING = "%language%";
	static char const *const ENGLISH_STRING = "english";
	static const int MAX_LANGUAGE_NAME_LENGTH = 64;
	char language[MAX_LANGUAGE_NAME_LENGTH];
	char fileName[MAX_PATH];
	int offs = 0;
	bool bTriedEnglish = false;

	strcpy(fileName, szFileName);
	char *langptr = strstr(szFileName, LANGUAGE_STRING);
	if (langptr)
	{
		// copy out the initial part of the string
		offs = langptr - szFileName;
		strncpy(fileName, szFileName, offs);
		fileName[offs] = 0;

		// append the language
		if (!vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Steam\\Language", language, MAX_LANGUAGE_NAME_LENGTH) || ( strcmp(language, "") == 0 ) )
		{
			// no reg key set, use the default
			strcpy(language, ENGLISH_STRING);
			bTriedEnglish = true;
		}

		strcat(fileName, language);

		// append the end of the initial string
		offs += strlen(LANGUAGE_STRING);
		strcat(fileName, szFileName + offs);
	}

	// parse out the file
	FileHandle_t file = fileSystem->Open(fileName, "rb");
	if (!file)
	{
		// try again...fallback to English (if we haven't already tried it)
		if (langptr && !bTriedEnglish)
		{
			// copy out the initial part of the string
			offs = langptr - szFileName;
			strncpy(fileName, szFileName, offs);
			fileName[offs] = 0;

			// append "english" as our default language
			strcat(fileName, ENGLISH_STRING);

			// append the end of the initial string
			offs += strlen(LANGUAGE_STRING);
			strcat(fileName, szFileName + offs);

			file = fileSystem->Open(fileName, "rb");
		}

		if (!file)
		{
			// assert(!("CLocalizedStringTable::AddFile() failed to load file"));
			return false;
		}
	}

	// this is an optimization so that the filename string doesn't have to get converted to a symbol for each key/value
	m_CurrentFile = fileName;
	m_LocalizationFiles.AddToTail(m_CurrentFile);

	// read into a memory block
	int fileSize = fileSystem->Size(file) ;
	wchar_t *memBlock = (wchar_t *)malloc(fileSize + sizeof(wchar_t));
	wchar_t *data = memBlock;
	fileSystem->Read(memBlock, fileSize, file);

	// null-terminate the stream
	memBlock[fileSize / sizeof(wchar_t)] = 0x0000;

	// check the first character, make sure this a little-endian unicode file
	if (data[0] != 0xFEFF)
	{
		fileSystem->Close(file);
		free(memBlock);
		return false;
	}
	data++;

	// parse out a token at a time
	enum states_e
	{
		STATE_BASE,		// looking for base settings
		STATE_TOKENS,	// reading in unicode tokens
	};

	states_e state = STATE_BASE;
	while (1)
	{
		// read the key and the value
		wchar_t keytoken[128];
		data = ReadUnicodeToken(data, keytoken, 128);
		if (!keytoken[0])
			break;	// we've hit the null terminator

		// convert the token to a string
		char key[128];
		ConvertUnicodeToANSI(keytoken, key, sizeof(key));

		// if we have a C++ style comment, read to end of line and continue
		if (!strnicmp(key, "//", 2))
		{
			data = ReadToEndOfLine(data);
			continue;
		}

		wchar_t valuetoken[2048];
		data = ReadUnicodeToken(data, valuetoken, 2048);
		if (!valuetoken[0])
			break;	// we've hit the null terminator
		
		if (state == STATE_BASE)
		{
			if (!stricmp(key, "Language"))
			{
				// copy out our language setting
				char value[2048];
				ConvertUnicodeToANSI(valuetoken, value, sizeof(value));
				strncpy(m_szLanguage, value, sizeof(m_szLanguage) - 1);
			}
			else if (!stricmp(key, "Tokens"))
			{
				state = STATE_TOKENS;
			}
			else if (!stricmp(key, "}"))
			{
				// we've hit the end
				break;
			}
		}
		else if (state == STATE_TOKENS)
		{
			if (!stricmp(key, "}"))
			{
				// end of tokens
				state = STATE_BASE;
			}
			else
			{
				// add the string to the table
				AddString(key, valuetoken, NULL);
			}
		}
	}

	fileSystem->Close(file);
	free(memBlock);
	m_CurrentFile = UTL_INVAL_SYMBOL;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: saves the entire contents of the token tree to the file
//-----------------------------------------------------------------------------
bool CLocalizedStringTable::SaveToFile(IFileSystem *fileSystem, const char *szFileName)
{
	// parse out the file
	FileHandle_t file = fileSystem->Open(szFileName, "wb");
	if (!file)
		return false;

	// only save the symbols relevant to this file
	CUtlSymbol fileName = szFileName;

	// write unicode marker
	unsigned short marker = 0xFEFF;
	fileSystem->Write(&marker, 2, file);

	char const *startStr = "\"lang\" { \"Language\" \"English\" \"Tokens\" { ";
	char const *endStr = "} } ";

	// write out the first string
	static wchar_t unicodeString[1024];
	int strLength = ConvertANSIToUnicode(startStr, unicodeString, sizeof(unicodeString));
	if (!strLength)
		return false;

	fileSystem->Write(unicodeString, (strLength - 1) * sizeof(wchar_t), file);

	// convert our spacing characters to unicode
	wchar_t unicodeSpace, unicodeQuote, unicodeNewline;
	ConvertANSIToUnicode(" ", &unicodeSpace, sizeof(wchar_t));
	ConvertANSIToUnicode("\"", &unicodeQuote, sizeof(wchar_t));
	ConvertANSIToUnicode("\n", &unicodeNewline, sizeof(wchar_t));

	// write out all the key/value pairs
	for (StringIndex_t idx = GetFirstStringIndex(); idx != INVALID_STRING_INDEX; idx = GetNextStringIndex(idx))
	{
		// only write strings that belong in this file
		if (fileName != m_Lookup[idx].filename)
			continue;

		const char *name = GetNameByIndex(idx);
		wchar_t *value = GetValueByIndex(idx);

		// convert the name to a unicode string
		int nameLength = ConvertANSIToUnicode(name, unicodeString, sizeof(unicodeString));

		// write out
		fileSystem->Write(&unicodeQuote, sizeof(wchar_t), file);
		fileSystem->Write(unicodeString, (nameLength - 1) * sizeof(wchar_t), file);
		fileSystem->Write(&unicodeQuote, sizeof(wchar_t), file);

		fileSystem->Write(&unicodeSpace, sizeof(wchar_t), file);

		fileSystem->Write(&unicodeQuote, sizeof(wchar_t), file);
		fileSystem->Write(value, wcslen(value) * sizeof(wchar_t), file);
		fileSystem->Write(&unicodeQuote, sizeof(wchar_t), file);

		fileSystem->Write(&unicodeSpace, sizeof(wchar_t), file);
	}

	// write end string
	strLength = ConvertANSIToUnicode(endStr, unicodeString, sizeof(unicodeString));
	fileSystem->Write(unicodeString, strLength * sizeof(wchar_t), file);

	fileSystem->Close(file);
	return true;
}

// helpers in sorting strings
struct LessCtx_t
{
	char const *m_pUserString;
	CLocalizedStringTable *m_pTable;
	
	LessCtx_t( ) : m_pUserString(0), m_pTable(0) {}
};

static LessCtx_t g_LessCtx;

//-----------------------------------------------------------------------------
// Purpose: Used to sort strings
//-----------------------------------------------------------------------------
bool CLocalizedStringTable::SymLess(localizedstring_t const &i1, localizedstring_t const &i2)
{
	char const *str1 = (i1.nameIndex == INVALID_STRING_INDEX) ? g_LessCtx.m_pUserString :
											&g_LessCtx.m_pTable->m_Names[i1.nameIndex];
	char const *str2 = (i2.nameIndex == INVALID_STRING_INDEX) ? g_LessCtx.m_pUserString :
											&g_LessCtx.m_pTable->m_Names[i2.nameIndex];
	
	return stricmp(str1, str2) < 0;
}


//-----------------------------------------------------------------------------
// Purpose: Finds a string in the table
//-----------------------------------------------------------------------------
wchar_t *CLocalizedStringTable::Find(char const *pName)
{	
	unsigned short idx = FindIndex(pName);
	if (idx == INVALID_STRING_INDEX)
		return NULL;

	return &m_Values[m_Lookup[idx].valueIndex];
}

//-----------------------------------------------------------------------------
// Purpose: finds the index of a token by token name
//-----------------------------------------------------------------------------
StringIndex_t CLocalizedStringTable::FindIndex(const char *pName)
{
	if (!pName)
		return NULL;

	// strip the pound character (which is used elsewhere to indicate that it's a string that should be translated)
	if (pName[0] == '#')
	{
		pName++;
	}
	
	// Store a special context used to help with insertion
	g_LessCtx.m_pUserString = pName;
	g_LessCtx.m_pTable = this;
	
	// Passing this special invalid symbol makes the comparison function
	// use the string passed in the context
	localizedstring_t invalidItem;
	invalidItem.nameIndex = INVALID_STRING_INDEX;
	invalidItem.valueIndex = INVALID_STRING_INDEX;
	return m_Lookup.Find( invalidItem );
}

//-----------------------------------------------------------------------------
// Finds and/or creates a symbol based on the string
//-----------------------------------------------------------------------------
void CLocalizedStringTable::AddString(char const *pString, wchar_t *pValue, const char *fileName)
{
	if (!pString) 
		return;

	wchar_t *str = Find(pString);
	
	// it's already in the table
	if (str)
		return;

	// didn't find, insert the string into the vector.
	int len = strlen(pString) + 1;
	int stridx = m_Names.AddMultipleToTail( len );
	memcpy( &m_Names[stridx], pString, len * sizeof(char) );

	len = wcslen(pValue) + 1;
	int valueidx = m_Values.AddMultipleToTail( len );
	memcpy( &m_Values[valueidx], pValue, len * sizeof(wchar_t) );

	localizedstring_t stringMapItem;
	stringMapItem.nameIndex = stridx;
	stringMapItem.valueIndex = valueidx;
	if (fileName)
	{
		stringMapItem.filename = fileName;
	}
	else
	{
		stringMapItem.filename = m_CurrentFile;
	}
	m_Lookup.Insert( stringMapItem );
}

//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------
void CLocalizedStringTable::RemoveAll()
{
	m_Lookup.RemoveAll();
	m_Names.RemoveAll();
	m_Values.RemoveAll();
	m_LocalizationFiles.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: iteration functions
//-----------------------------------------------------------------------------
StringIndex_t CLocalizedStringTable::GetFirstStringIndex()
{
	return m_Lookup.FirstInorder();
}

//-----------------------------------------------------------------------------
// Purpose: returns the next index, or INVALID_STRING_INDEX if no more strings available
//-----------------------------------------------------------------------------
StringIndex_t CLocalizedStringTable::GetNextStringIndex(StringIndex_t index)
{
	StringIndex_t idx = m_Lookup.NextInorder(index);
	if (idx == m_Lookup.InvalidIndex())
		return INVALID_STRING_INDEX;
	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: gets the name of the localization string by index
//-----------------------------------------------------------------------------
const char *CLocalizedStringTable::GetNameByIndex(StringIndex_t index)
{
	localizedstring_t &lstr = m_Lookup[index];
	return &m_Names[lstr.nameIndex];
}

//-----------------------------------------------------------------------------
// Purpose: gets the localized string value by index
//-----------------------------------------------------------------------------
wchar_t *CLocalizedStringTable::GetValueByIndex(StringIndex_t index)
{
	localizedstring_t &lstr = m_Lookup[index];
	return &m_Values[lstr.valueIndex];
}

//-----------------------------------------------------------------------------
// Purpose: returns which file a string was loaded from
//-----------------------------------------------------------------------------
const char *CLocalizedStringTable::GetFileNameByIndex(StringIndex_t index)
{
	localizedstring_t &lstr = m_Lookup[index];
	return lstr.filename.String();
}

//-----------------------------------------------------------------------------
// Purpose: sets the value in the index
//-----------------------------------------------------------------------------
void CLocalizedStringTable::SetValueByIndex(StringIndex_t index, wchar_t *newValue)
{
	// get the existing string
	localizedstring_t &lstr = m_Lookup[index];
	wchar_t *wstr = &m_Values[lstr.valueIndex];

	// see if the new string will fit within the old memory
	int newLen = wcslen(newValue);
	int oldLen = wcslen(wstr);

	if (newLen > oldLen)
	{
		// it won't fit, so allocate new memory - this is wasteful, but only happens in edit mode
		lstr.valueIndex = m_Values.AddMultipleToTail(newLen + 1);
		memcpy(&m_Values[lstr.valueIndex], newValue, (newLen + 1) * sizeof(wchar_t));
	}
	else
	{
		// copy the string into the old position
		wcscpy(wstr, newValue);		
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns number of localization files currently loaded
//-----------------------------------------------------------------------------
int CLocalizedStringTable::GetLocalizationFileCount()
{
	return m_LocalizationFiles.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns localization filename by index
//-----------------------------------------------------------------------------
const char *CLocalizedStringTable::GetLocalizationFileName(int index)
{
	return m_LocalizationFiles[index].String();
}

//-----------------------------------------------------------------------------
// Purpose: converts an english string to unicode
//-----------------------------------------------------------------------------
int CLocalizedStringTable::ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSizeInBytes)
{
	int chars = ::MultiByteToWideChar(CP_UTF8, 0, ansi, -1, unicode, unicodeBufferSizeInBytes / sizeof(wchar_t));
	unicode[(unicodeBufferSizeInBytes / sizeof(wchar_t)) - 1] = 0;
	return chars;
}

//-----------------------------------------------------------------------------
// Purpose: converts an unicode string to an english string
//-----------------------------------------------------------------------------
int CLocalizedStringTable::ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize)
{
	int result = ::WideCharToMultiByte(CP_UTF8, 0, unicode, -1, ansi, ansiBufferSize, NULL, NULL);
	ansi[ansiBufferSize - 1] = 0;
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: builds a localized formatted string
//-----------------------------------------------------------------------------
void CLocalizedStringTable::ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, wchar_t *formatString, int numFormatParameters, ...)
{
	if (!formatString)
	{
		unicodeOutput[0] = 0;
		return;
	}

	int unicodeBufferSize = unicodeBufferSizeInBytes / sizeof(wchar_t);

	va_list argList;
	va_start(argList, numFormatParameters);
	int argNum = 0;
	int stringNum = 1;

	while (1)
	{
		// scan for the string
		char searchStr[4] = { '%', 's', (char)stringNum + '0', 0 };
		wchar_t unicodePS[4];
		ConvertANSIToUnicode(searchStr, unicodePS, sizeof(unicodePS));

		// search the unicode string
		wchar_t *searchPos = wcsstr(formatString, unicodePS);
		int precopyCount = 0;
		if (searchPos)
		{
			precopyCount = searchPos - formatString;
		}
		else
		{
			// copy out the remainder of the string (including null terminator) and exit
			precopyCount = wcslen(formatString) + 1;
		}

		// copy out the format string up to the search pos
		if (precopyCount > unicodeBufferSize)
		{
			precopyCount = unicodeBufferSize;
		}

		wcsncpy(unicodeOutput, formatString, precopyCount);
		unicodeOutput += precopyCount;
		formatString += precopyCount;
		unicodeBufferSize -= precopyCount;

		if (!searchPos)
			break;

		if (argNum < numFormatParameters)
		{
			// add in the parameter
			wchar_t *param = va_arg(argList, wchar_t *);
			if (!param)
				return;
			
			int paramSize = wcslen(param);
			if (paramSize > unicodeBufferSize)
			{
				paramSize = unicodeBufferSize;
			}
			wcsncpy(unicodeOutput, param, paramSize);
			unicodeOutput += paramSize;
			unicodeBufferSize -= paramSize;
			// skip the formatting characters
			formatString += 3;

		}

		// move to the next argument
		stringNum++;
		argNum++;
	}

	va_end(argList);

	// ensure null termination
	unicodeOutput[unicodeBufferSize - 1] = 0;
}


