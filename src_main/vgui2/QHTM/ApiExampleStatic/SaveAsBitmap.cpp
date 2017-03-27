/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	SaveAsBitmap.cpp
Owner:	russf@gipsysoft.com
Purpose:	Example to show saving a HTML file as a bitmap.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <QHTM/QHTM.h>

//
//	These two functions taken from Zoom+ source code
//		-->http://www.gipsysoft.com/zoomplus/
extern HANDLE DDBToDIB( HBITMAP hBitmap, DWORD dwCompression, HPALETTE hPal );
bool WriteDIB( LPCTSTR szFile, HANDLE hdib );

void SaveBitmapAsFile( HGDIOBJ hbmp, LPCTSTR pcszBMPFilename )
{
	HANDLE hDib = DDBToDIB( (HBITMAP)hbmp, BI_RGB, NULL );
	if( hDib )
	{
		(void)WriteDIB( pcszBMPFilename, hDib );
	}
}


void SaveAsBitmap( HWND hwndParent, LPCTSTR pcszHTMLFilename, LPCTSTR pcszBMPFilename )
{
	HCURSOR hOld = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

	//
	//	We set a maximum width just to be sure...
	const UINT uMaxWidth = 600;
	HDC hdcWindow = ::GetDC( hwndParent );
	HDC hdcDraw = ::CreateCompatibleDC( hdcWindow );

	UINT uHeight = 0;
	if( QHTM_GetHTMLHeight( hdcDraw, pcszHTMLFilename, NULL, QHTM_SOURCE_FILENAME, uMaxWidth, &uHeight ) )
	{
		//
		//	Now we have a width and a height...
		HGDIOBJ hbmp = ::CreateCompatibleBitmap( hdcWindow, uMaxWidth, uHeight );
		if( hbmp )
		{

			//
			//	Select the bitmap and render the HTML onto it.
			hbmp = ::SelectObject( hdcDraw, hbmp );
			if( QHTM_RenderHTML( hdcDraw, pcszHTMLFilename, NULL, QHTM_SOURCE_FILENAME, uMaxWidth ) )
			{
				//
				//	Deselect the bitmap
				hbmp = ::SelectObject( hdcDraw, hbmp );
				SaveBitmapAsFile( hbmp, pcszBMPFilename );
			}
		}
		(void)::DeleteObject( hbmp );
	}

	(void)::ReleaseDC( hwndParent, hdcWindow );

	(void)::DeleteDC( hdcDraw );
	(void)::SetCursor( hOld );
}
