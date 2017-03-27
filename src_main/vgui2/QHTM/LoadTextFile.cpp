/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	LoadTextFile.cpp
Owner:	russf@gipsysoft.com
Purpose:	Load a text file.
----------------------------------------------------------------------*/
#include "stdafx.h"

extern bool LoadTextFile( LPCTSTR pcszFilename, LPTSTR &pszBuffer );

bool LoadTextFile( LPCTSTR pcszFilename, LPTSTR &pszBuffer, UINT &uLength )
{
	bool bRetVal = false;
	HANDLE hFile = ::CreateFile( pcszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		uLength = ::GetFileSize( hFile, NULL );
		if( uLength )
		{
			LPTSTR pszFile = new char [ uLength + 1 ];
			DWORD dwRead;
			if( ::ReadFile( hFile, pszFile, uLength, &dwRead, NULL ) )
			{
				pszFile[ uLength ] = _T('\000');

#ifdef _UNICODE
				LPTSTR pszFile2 = new char [ uLength * sizeof( TCHAR ) + 1 ];
				MultiByteToWideChar(CP_ACP, 0, pszBuffer, uLength, pszFile2, uLength * sizeof( TCHAR ) );
				pszBuffer = pszFile2;
				delete pszFile;
#else	//	UNICODE
				pszBuffer = pszFile;
#endif	//	_UNICODE
				bRetVal = true;
			}
		}

		::CloseHandle( hFile );
	}
	return bRetVal;
}


