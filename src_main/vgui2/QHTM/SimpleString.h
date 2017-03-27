/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	SimpleString.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef SIMPLESTRING_H
#define SIMPLESTRING_H

#ifndef TEXTABC_H
	#include "TextABC.h"
#endif	//	TEXTABC_H

#ifndef QHTM_TYPES_H
	#include "QHTM_Types.h"
#endif	//	QHTM_TYPES_H

class CSimpleString : public CTextABC
{
public:
	CSimpleString();
	CSimpleString( LPCTSTR pcszText );
	CSimpleString( LPCTSTR pcszText, UINT uLength );
	CSimpleString( const CSimpleString &rhs );
	CSimpleString &operator = ( const CSimpleString &rhs );

	void Add( LPCTSTR pcszText, UINT uLength );
	void Set( LPCTSTR pcszText, UINT uLength );
	CSimpleString &operator += ( const CSimpleString &rhs );
	CSimpleString &operator += ( LPCTSTR pcszText );
	operator LPCTSTR () const { return GetData(); }

	virtual ~CSimpleString();

	void Delete( UINT uIndex, UINT uCount );

	void Empty();

	int Compare( const CTextABC & ) const;

	virtual UINT GetLength() const;
	virtual LPCTSTR GetData() const;

private:
	ArrayOfChar m_arrText;

private:
};

#endif //SIMPLESTRING_H