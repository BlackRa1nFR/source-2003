//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef UTLSTRINGMAP_H
#define UTLSTRINGMAP_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlSymbol.h"

template <class T>
class CUtlStringMap
{
public:
	CUtlStringMap()
	{
	}

	T& operator[]( const char *pString )
	{
		CUtlSymbol symbol = m_SymbolTable.AddString( pString );
		int index = ( int )( UtlSymId_t )symbol;
		if( m_Vector.Count() <= index )
		{
			m_Vector.EnsureCount( index + 1 );
		}
		return m_Vector[index];
	}

	bool Defined( const char *pString )
	{
		return m_SymbolTable.Find( pString ) != UTL_INVAL_SYMBOL;
	}

private:
	CUtlVector<T> m_Vector;
	CUtlSymbolTable m_SymbolTable;
};

#endif // UTLSTRINGMAP_H
