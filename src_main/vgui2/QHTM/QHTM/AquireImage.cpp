/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	AquireImage.cpp
Owner:	leea@gipsysoft.com
Purpose:	Function for handling default image acquisition
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <DebugHlp.h>
#include <ImgLib.h>
#include "AquireImage.h"
#include "QHTM_Types.h"
#include "stdlib.h"


CImage *AquireImage( HINSTANCE hInstance, LPCTSTR pcszFilePath, LPCTSTR pcszFilename )
{
	CImage *pImage = NULL;

	//
	//	Check for resource based images
	if( !_tcsnicmp( pcszFilename, _T("RES:"), 4 ) )
	{
		pcszFilename += 4;
		ASSERT( _tcslen( pcszFilename  ) );
		pImage = new CImage;
		if( !pImage->Load( hInstance, pcszFilename ) )
		{
			LPTSTR endptr;
			if( !pImage->Load( hInstance, MAKEINTRESOURCE( _tcstol( pcszFilename, &endptr, 10 ) ) ) )
			{
				delete pImage;
				pImage = NULL;
			}
		}
		return pImage;
	}

	//
	//	Check for file based images
	StringClass strImageName( pcszFilePath );
	strImageName += pcszFilename ;

	pImage = new CImage;
	if( !pImage->Load( strImageName ) )
	{
		delete pImage;
		pImage = NULL;
	}
	return pImage;
}