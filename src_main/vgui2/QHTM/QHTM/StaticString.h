/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	StaticString.h
Owner:	russf@gipsysoft.com
Purpose:	String class tied to some static text, so it can be used when
					passed around as a TextABC.
----------------------------------------------------------------------*/
#ifndef STATICSTRING_H
#define STATICSTRING_H

#ifndef TEXTABC_H
#include <TextABC.h>
#endif	//	TEXTABC_H

class CStaticString: public CTextABC
{
public:
	CStaticString()
		: m_pcszText( NULL )
		, m_uLength( 0 )
	{
	}
	CStaticString( LPCTSTR pcszText )
		: m_pcszText( pcszText )
		, m_uLength( _tcslen( pcszText ) )
	{
	}
	CStaticString( LPCTSTR pcszText, UINT uLength )
		: m_pcszText( pcszText )
		, m_uLength( uLength )
	{
	}
	CStaticString( const CStaticString &str )
		: m_pcszText( str.m_pcszText )
		, m_uLength( str.m_uLength )
	{
	}

	void Set( LPCTSTR pcszText, UINT uLength )
	{
		m_pcszText = pcszText;
		m_uLength = uLength;
	}

	LPCTSTR Find( TCHAR ch ) const
	{
		LPCTSTR pcszEnd = GetEndPointer();
		LPCTSTR p = GetData();
		while( p < pcszEnd )
		{
			if( *p == ch )
				return p;
			p++;
		}
		return NULL;
	}


	UINT GetLength() const
	{
		return m_uLength;
	}

	LPCTSTR GetData() const
	{
		return m_pcszText;
	}

	LPCTSTR GetEndPointer() const
	{
		return m_pcszText + m_uLength;
	}

private:
	UINT m_uLength;
	LPCTSTR m_pcszText;
};


namespace Container
{
	inline UINT HashIt( const CStaticString& s)
	{
		UINT uHash = 0;
		LPCTSTR pcszText = s.GetData();
		UINT uLength = s.GetLength();
		if (!uLength)
			return 0;
		while( uLength-- )
		{
			uHash = uHash << 1 ^ toupper( *pcszText++ );
		}
		return uHash;
	}

	inline bool ElementsTheSame( const CStaticString& lhs, const CStaticString& rhs )
	{
		return rhs.GetLength() == lhs.GetLength() || !_tcsnicmp( lhs.GetData(), rhs.GetData(), lhs.GetLength() );
	}

}


#endif //STATICSTRING_H