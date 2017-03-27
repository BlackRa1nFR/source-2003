/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	SimpleString.cpp
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <DebugHlp.h>
#include "SimpleString.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CSimpleString & CSimpleString::operator = ( const CSimpleString &rhs )
{
	if( &rhs != this )
	{
		const UINT uLength = rhs.GetLength();
		if( uLength )
		{
			m_arrText.SetSize( uLength + 1 );
			_tcscpy( m_arrText.GetData(), rhs );
			m_arrText[ uLength ] = '\000';
		}
		else
		{
			Empty();
		}
		
	}
	return *this;
}

CSimpleString::CSimpleString( const CSimpleString &rhs )
{
	operator = ( rhs );
}


CSimpleString::CSimpleString( LPCTSTR pcszText, UINT uLength )
{
	m_arrText.SetSize( uLength + 1 );
	memcpy( m_arrText.GetData(), pcszText, uLength * sizeof( TCHAR ) );
	m_arrText[ uLength ] = '\000';
}


CSimpleString::CSimpleString( LPCTSTR pcszText )
{
	if( pcszText )
	{
		const UINT uLength = _tcslen( pcszText );
		m_arrText.SetSize( uLength + 1 );
		memcpy( m_arrText.GetData(), pcszText, uLength * sizeof( TCHAR ) );
	}
}

CSimpleString::CSimpleString()
{

}


CSimpleString::~CSimpleString()
{

}

UINT CSimpleString::GetLength() const
{
	if( m_arrText.GetSize() )
		return m_arrText.GetSize() - 1;
	return 0;
}


LPCTSTR CSimpleString::GetData() const
{
	return m_arrText.GetData();
}


void CSimpleString::Empty()
{
	m_arrText.SetSize( 0 );	
}


CSimpleString & CSimpleString::operator += ( const CSimpleString &rhs )
{
	const UINT u = GetLength();
	const UINT urhs = rhs.GetLength();
	m_arrText.SetSize( GetLength() + rhs.GetLength() + 1 );
	memcpy( m_arrText.GetData() + u, rhs.GetData(), urhs * sizeof( TCHAR ) );
	return *this;
}


CSimpleString & CSimpleString::operator += ( LPCTSTR pcszText )
{
	if( pcszText )
	{
		const UINT uLength = _tcslen( pcszText );
		m_arrText.SetSize( GetLength() + uLength + 1 );
		_tcscat( m_arrText.GetData(), pcszText );
	}
	return *this;
}


void CSimpleString::Add( LPCTSTR pcszText, UINT uLength )
{
	const UINT u = GetLength();
	m_arrText.SetSize( GetLength() + uLength + 1 );
	memcpy( m_arrText.GetData() + u, pcszText, uLength * sizeof( TCHAR ) );
}

void CSimpleString::Set( LPCTSTR pcszText, UINT uLength )
{
	m_arrText.SetSize( uLength + 1 );
	memcpy( m_arrText.GetData(), pcszText, uLength * sizeof( TCHAR ) );
	m_arrText[ uLength ] = '\000';
}



void CSimpleString::Delete( UINT uIndex, UINT uCount )
{
	m_arrText.RemoveAt( uIndex, uCount );
}

int CSimpleString::Compare( const CTextABC &txt ) const
{
	return _tcscmp( m_arrText.GetData(), txt );
}
