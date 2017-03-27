#pragma once

//-----------------------------------------------------------------------------
// Purpose: encapsulates the data string in the map file 
//			that is used to initialise entities.  The data
//			string contains a set of key/value pairs.
//-----------------------------------------------------------------------------
class CEntityMapData
{
private:
	char	*m_pEntData;
	char	*m_pCurrentKey;

public:
	explicit CEntityMapData( char *entBlock ) : m_pEntData(entBlock), m_pCurrentKey(entBlock) {}

	// find the keyName in the entdata and puts it's value into Value.  returns false if key is not found
	bool ExtractValue( const char *keyName, char *Value );

	// find the keyName in the endata and change its value to specified one
	bool SetValue( const char *keyName, char *NewValue );
	
	bool GetFirstKey( char *keyName, char *Value );
	bool GetNextKey( char *keyName, char *Value );

	const char *CurrentBufferPosition( void );
};

#define MAPKEY_MAXLENGTH	256
