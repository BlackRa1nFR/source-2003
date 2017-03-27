//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "KeyValues.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *SetName )
{
	Init( SetName );
}

KeyValues::KeyValues( const char *SetName, const char *firstKey, const char *firstValue )
{
	Init( SetName );
	SetString( firstKey, firstValue );
}

KeyValues::KeyValues( const char *SetName, const char *firstKey, int firstValue )
{
	Init( SetName );
	SetInt( firstKey, firstValue );
}

KeyValues::KeyValues( const char *SetName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue )
{
	Init( SetName );
	SetString( firstKey, firstValue );
	SetString( secondKey, secondValue );
}

KeyValues::KeyValues( const char *SetName, const char *firstKey, int firstValue, const char *secondKey, int secondValue )
{
	Init( SetName );
	SetInt( firstKey, firstValue );
	SetInt( secondKey, secondValue );
}

void KeyValues::Init( const char *SetName )
{
	strncpy( m_sKeyName, SetName, KEYNAME_LENGTH-1 );
	m_sKeyName[KEYNAME_LENGTH-1] = 0;
	m_iDataType = TYPE_NONE;
	m_pSub = NULL;
	m_pPeer = NULL;
	m_sValue = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
KeyValues::~KeyValues()
{
	KeyValues *datNext = NULL;
	for ( KeyValues *dat = m_pSub; dat != NULL; dat = datNext )
	{
		datNext = dat->m_pPeer;
		delete dat;
	}

	::free(m_sValue);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the name of the current key section
// Output : const char
//-----------------------------------------------------------------------------
const char *KeyValues::GetName( void )
{
	return m_sKeyName;
}

//-----------------------------------------------------------------------------
// Purpose: Reads a single token from the file
// Input  : *f - 
// Output : static char *
//-----------------------------------------------------------------------------
static char *ReadToken( FILE *f )
{
	static char buf[256];
	int bufC = 0;
	int c;
	bool skipWhitespace = true;

	while (skipWhitespace)
	{
		skipWhitespace = false;

		// skip any whitespace
		do 
		{
			c = fgetc( f );
		}
		while ( isspace(c) );

		// skip // comments
		if ( c == '/' )
		{
			c = fgetc(f);

			if ( c != '/' )
			{
				// push the char back on the stream
				buf[bufC++] = '/';
			}
			else
			{
				while (c > 0 && c != '\n')
				{
					c = fgetc(f);
				}
				skipWhitespace = true;
			}
		}
	}

	// read quoted strings specially
	if ( c == '\"' )
	{
		// read the token till we hit the endquote
		while ( 1 )
		{
			c = fgetc( f );

			if ( c < 0 )
				break;

			if ( c == '\"' )
				break;

			// check for special chars
			if ( c == '\\' )
			{
				// get the next char
				c = fgetc( f );

				switch ( c )
				{
				case 'n':  c = '\n'; break;
				case '\\': c = '\\'; break;
				case 't':  c = '\t'; break;
				case '\"': c = '\"'; break;
				default:   c = c;    break;
				}
			}

			buf[bufC++] = c;
		}
	}
	else
	{
		// read in the token until we hit a whitespace
		while ( 1 )
		{
			if ( c < 0 )
				break;

			if ( isspace(c) )
				break;

			buf[bufC++] = c;

			c = fgetc( f );
		}
	}

	// check for EOF
	if ( c < 0 && bufC == 0 )
		return NULL;

	// null terminate
	buf[bufC] = 0;
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *resourceName - 
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromFile( const char *resourceName )
{
	FILE *f = fopen( resourceName, "rt" );

	if ( !f )
		return false;

	// the first thing must be a key
	char *s = ReadToken( f );
	if ( s )
	{
		int len = strlen( s );
		strncpy( m_sKeyName, s, KEYNAME_LENGTH-1 );
		m_sKeyName[KEYNAME_LENGTH-1] = 0;
	}

	// get the '{'
	s = ReadToken( f );

	if ( m_sKeyName && s && *s == '{' )
	{
		// header is valid so load the file
		RecursiveLoadFromFile( f );
	}

	fclose( f );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *f - 
//-----------------------------------------------------------------------------
void KeyValues::RecursiveLoadFromFile( FILE *f )
{
	while ( 1 )
	{
		// get the key name
		char *s = ReadToken( f );
		if ( !s )
			break;
		if ( !strcmp(s,"}") )
			break;

		KeyValues *dat = FindKey(s, true);

		// get the value
		s = ReadToken( f );
		
		if ( !strcmp(s,"{") )
		{
			// sub value list
			dat->RecursiveLoadFromFile( f );									
		}
		else 
		{
			int len = strlen( s );
			dat->m_sValue = (char *)::malloc(len + 1);
			memcpy( dat->m_sValue, s, len+1 );
			dat->m_iDataType = TYPE_STRING;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Save the keyvalues to disk
//			Creates the path to the file if it doesn't exist 
// Input  : char *resourceName - 
//-----------------------------------------------------------------------------
bool KeyValues::SaveToFile( const char *resourceName )
{
	// create a write file
	FILE *f = ::fopen(resourceName, "wt");

	// if the file won't create, make sure the paths are all created
	if (!f)
	{
		const char *currentFileName = resourceName;
		char szBuf[1024] = "";

		// Creates any necessary paths to create the file
		while (1)
		{
			const char *fileSlash = strchr(currentFileName, '\\');
			if (!fileSlash)
				break;

			// copy out the first string
			int pathSize = fileSlash - currentFileName;
			
			if (szBuf[0] != 0)
			{
				strcat(szBuf, "\\");
			}

			strncat(szBuf, currentFileName, pathSize);

			// make sure the first path is created
			::_mkdir(szBuf);

			// increment the parse point to create the next directory (if any)
			currentFileName += (pathSize + 1);
		}

		// try opening the file again
		f = ::fopen(resourceName, "wt");
	}

	if (!f)
		return false;

	RecursiveSaveToFile(f, 0);
	::fclose(f);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: writes out a set of indenting
// Input  : indentLevel - 
//-----------------------------------------------------------------------------
static void WriteIndents( FILE *f, int indentLevel )
{
	for ( int i = 0; i < indentLevel; i++ )
	{
		fprintf( f, "\t" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *f - 
//-----------------------------------------------------------------------------
void KeyValues::RecursiveSaveToFile( FILE *f, int indentLevel )
{
	// write header
	WriteIndents( f, indentLevel );
	fprintf( f, "\"%s\"\n", m_sKeyName );
	WriteIndents( f, indentLevel );
	fprintf( f, "{\n" );

	// loop through all our keys writing them to disk
	for ( KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer )
	{
		if ( dat->m_pSub )
		{
			dat->RecursiveSaveToFile( f, indentLevel + 1 );
		}
		else
		{
			// only write non-empty keys

			switch (dat->m_iDataType)
			{
			case TYPE_STRING:
				{
					if (dat->m_sValue && *(dat->m_sValue))
					{
						WriteIndents(f, indentLevel + 1);
						fprintf(f, "\"%s\"\t\t\"%s\"\n", dat->m_sKeyName, dat->m_sValue);
					}
					break;
				}

			case TYPE_INT:
				{
					WriteIndents(f, indentLevel + 1);
					fprintf(f, "\"%s\"\t\t\"%d\"\n", dat->m_sKeyName, dat->m_iValue);
					break;
				}

			case TYPE_FLOAT:
				{
					WriteIndents(f, indentLevel + 1);
					fprintf(f, "\"%s\"\t\t\"%f\"\n", dat->m_sKeyName, dat->m_flValue);
					break;
				}

			default:
				break;
			}
		}
	}

	// write tail
	WriteIndents( f, indentLevel );
	fprintf( f, "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
// Output : KeyValues
//-----------------------------------------------------------------------------
KeyValues *KeyValues::FindKey(const char *keyName, bool bCreate)
{
	// return the current key if a NULL subkey is asked for
	if (!keyName || !keyName[0])
		return this;

	// look for '/' characters deliminating sub fields
	char szBuf[256];
	char *subStr = strchr(keyName, '/');
	const char *searchStr = keyName;

	// pull out the substring if it exists
	if (subStr)
	{
		int size = subStr - keyName;
		memcpy( szBuf, keyName, size );
		szBuf[size] = 0;
		searchStr = szBuf;
	}

	KeyValues *lastItem = NULL;
	// find the searchStr in the current peer list
	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		lastItem = dat;	// record the last item looked at (for if we need to append to the end of the list)

		// case-insensitive string compare
		if ( !stricmp(dat->m_sKeyName, searchStr) )
		{
			break;
		}
	}

	// make sure a key was found
	if (!dat)
	{
		if (bCreate)
		{
			// we need to create a new key
			dat = new KeyValues( searchStr );

			// insert new key at end of list
			if (lastItem)
			{
				lastItem->m_pPeer = dat;
			}
			else
			{
				m_pSub = dat;
			}
			dat->m_pPeer = NULL;

			// a key graduates to be a submsg as soon as it's m_pSub is set
			// this should be the only place m_pSub is set
			m_iDataType = TYPE_NONE;
		}
		else
		{
			return NULL;
		}
	}
	
	// if we've still got a subStr we need to keep looking deeper in the tree
	if ( subStr )
	{
		// recursively chain down through the paths in the string
		return dat->FindKey(subStr + 1, bCreate);
	}

	return dat;
}

//-----------------------------------------------------------------------------
// Purpose: creates a new key, with an autogenerated name.  name is guaranteed to be an integer, of value 1 higher than the highest other integer key name
// Input  : 
// Output : KeyValues
//-----------------------------------------------------------------------------
KeyValues *KeyValues::CreateNewKey()
{
	int newID = 1;

	// search for any key with higher values
	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		// case-insensitive string compare
		int val = atoi(dat->m_sKeyName);
		if (newID <= val)
		{
			newID = val + 1;
		}
	}

	char buf[12];
	itoa(newID, buf, 10);

	return FindKey(buf, true);
}

//-----------------------------------------------------------------------------
// Purpose: removes a subkey from the list
// Input  : *subKey - 
// Output : KeyValues
//-----------------------------------------------------------------------------
void KeyValues::RemoveSubKey(KeyValues *subKey)
{
	if (!subKey)
		return;

	// check the list pointer
	if (m_pSub == subKey)
	{
		m_pSub = subKey->m_pPeer;
	}
	else
	{
		// look through the list
		KeyValues *kv = m_pSub;
		while (kv->m_pPeer)
		{
			if (kv->m_pPeer == subKey)
			{
				kv->m_pPeer = subKey->m_pPeer;
				break;
			}
			
			kv = kv->m_pPeer;
		}
	}

	subKey->m_pPeer = NULL;
}



//-----------------------------------------------------------------------------
// Purpose: returns the first subkey in the list
// Output : KeyValues *
//-----------------------------------------------------------------------------
KeyValues *KeyValues::GetFirstSubKey()
{
	return m_pSub;
}

//-----------------------------------------------------------------------------
// Purpose: returns the next subkey
// Output : KeyValues
//-----------------------------------------------------------------------------
KeyValues *KeyValues::GetNextKey()
{
	return m_pPeer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
// Output : int
//-----------------------------------------------------------------------------
int KeyValues::GetInt( const char *keyName, int defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_STRING:
			return atoi(dat->m_sValue);
		case TYPE_FLOAT:
			return (int)dat->m_flValue;
		case TYPE_INT:
		case TYPE_PTR:
		default:
			return dat->m_iValue;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
// Output : void *
//-----------------------------------------------------------------------------
void *KeyValues::GetPtr( const char *keyName, void *defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_PTR:
			return dat->m_pValue;

		case TYPE_STRING:
		case TYPE_FLOAT:
		case TYPE_INT:
		default:
			return NULL;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyname - 
// Output : float
//-----------------------------------------------------------------------------
float KeyValues::GetFloat( const char *keyName, float defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_STRING:
			return (float)atof(dat->m_sValue);
		case TYPE_FLOAT:
			return dat->m_flValue;
		case TYPE_INT:
			return (float)dat->m_iValue;
		case TYPE_PTR:
		default:
			return 0.0f;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
// Output : const char
//-----------------------------------------------------------------------------
const char *KeyValues::GetString( const char *keyName, const char *defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		// convert the data to string form then return it
		char buf[64];
		switch ( dat->m_iDataType )
		{
		case TYPE_FLOAT:
			sprintf( buf, "%f", dat->m_flValue );
			SetString( keyName, buf );
			break;
		case TYPE_INT:
		case TYPE_PTR:
			sprintf( buf, "%d", dat->m_iValue );
			SetString( keyName, buf );
			break;
		case TYPE_STRING:
			break;
		default:
			return defaultValue;
		};
		
		return dat->m_sValue;
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
//			char *value - 
//-----------------------------------------------------------------------------
void KeyValues::SetString( const char *keyName, const char *value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		// delete the old value
		::free(dat->m_sValue);

		if (!value)
		{
			// ensure a valid value
			value = "";
		}

		// allocate memory for the new value and copy it in
		int len = strlen( value );
		dat->m_sValue = (char *)::malloc(len + 1);
		memcpy( dat->m_sValue, value, len+1 );

		dat->m_iDataType = TYPE_STRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
//			value - 
//-----------------------------------------------------------------------------
void KeyValues::SetInt( const char *keyName, int value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_iValue = value;
		dat->m_iDataType = TYPE_INT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
//			value - 
//-----------------------------------------------------------------------------
void KeyValues::SetFloat( const char *keyName, float value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_flValue = value;
		dat->m_iDataType = TYPE_FLOAT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *keyName - 
//			value - 
//-----------------------------------------------------------------------------
void KeyValues::SetPtr( const char *keyName, void *value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_pValue = value;
		dat->m_iDataType = TYPE_PTR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes a copy of the whole key-value pair set
// Output : KeyValues
//-----------------------------------------------------------------------------
KeyValues *KeyValues::MakeCopy( void )
{
	KeyValues *newKeyValue = new KeyValues(m_sKeyName);

	// copy data
	newKeyValue->m_iDataType = m_iDataType;
	switch ( m_iDataType )
	{
	case TYPE_STRING:
		{
			if ( m_sValue )
			{
				int len = strlen( m_sValue );
				newKeyValue->m_sValue = (char *)::malloc(len + 1);
				memcpy( newKeyValue->m_sValue, m_sValue, len+1 );
			}
		}
		break;

	case TYPE_INT:
		newKeyValue->m_iValue = m_iValue;
		break;

	case TYPE_FLOAT:
		newKeyValue->m_flValue = m_flValue;
		break;

	case TYPE_PTR:
		newKeyValue->m_pValue = m_pValue;
		break;
	};

	// recursively copy subkeys
	for ( KeyValues *sub = m_pSub; sub != NULL; sub = sub->m_pPeer )
	{
		// take a copy of the subkey
		KeyValues *dat = sub->MakeCopy();

		// add into subkey list
		dat->m_pPeer = newKeyValue->m_pSub;
		newKeyValue->m_pSub = dat;
	}

	return newKeyValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *keyName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool KeyValues::IsEmpty(const char *keyName)
{
	KeyValues *dat = FindKey(keyName, false);
	if (!dat)
		return true;

	if (dat->m_iDataType == TYPE_NONE && dat->m_pSub == NULL)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: clears out all subkeys, and the current value
//-----------------------------------------------------------------------------
void KeyValues::Clear( void )
{
	delete m_pSub;
	m_pSub = NULL;
	m_iDataType = TYPE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *keyName - 
// Output : KeyValues::types_t
//-----------------------------------------------------------------------------
KeyValues::types_t KeyValues::GetDataType(const char *keyName)
{
	KeyValues *dat = FindKey(keyName, false);
	if (dat)
		return dat->m_iDataType;

	return TYPE_NONE;
}

#include "..\common\MemPool.h"

static CMemoryPool s_MemPool(sizeof(KeyValues), 4096);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : int iAllocSize - 
// Output : void KeyValues::operator
//-----------------------------------------------------------------------------
void *KeyValues::operator new( unsigned int iAllocSize )
{
	return s_MemPool.Alloc( iAllocSize );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMem - 
// Output : void KeyValues::operator
//-----------------------------------------------------------------------------
void KeyValues::operator delete( void *pMem )
{
	s_MemPool.Free( pMem );
}


