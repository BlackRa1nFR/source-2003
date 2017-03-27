/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	GetFontSize.cpp
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <stdlib.h>
#include "staticstring.h"

int GetFontSize( const CStaticString &strParam, int nDefault )
{
  int nSize = 0;
	if( strParam.GetLength() )
	{
		LPCTSTR p = strParam.GetData();
		LPCTSTR pEnd = strParam.GetEndPointer();
		while( p < pEnd && isspace( *p ))
		{
			p++;
		}
  
		while( p < pEnd && isdigit( *p) )
		{
			nSize = 10 * nSize + (*p - _T('0') );     /* accumulate digit */
			p++;
		}

		if( p + 1 < pEnd && *p == _T('p') && *(p+1) == _T('t') )
		{
			nSize = -nSize;
		}
	}
	else
	{
		nSize = nDefault;
	}
	
	return nSize;
}