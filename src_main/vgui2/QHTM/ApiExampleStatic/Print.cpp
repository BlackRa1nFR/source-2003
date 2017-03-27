/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	print.cpp
Owner:	russf@gipsysoft.com
Purpose:	Test/demo printing from the QHTM control.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <commdlg.h>
#include "Resource.h"
#include <QHTM/QHTM.h>


//
//	The title of teh document
static LPCTSTR g_pszTitle = _T("QHTM test print");

//
//	The file to print.
static LPCTSTR g_pcszFileToPrint = _T("zoomplus//features.html");

//
//	The stock zoom level, you can change this to see how the zoo mlevel affects printing
static const int g_nZoomLevel = QHTM_ZOOM_DEFAULT;


static LPCTSTR GetHeaderHTML()
//
//	Supply some header HTML
{
	return _T("Header. Maybe a place for the document title, the number of pages perhaps and my birthday (just so no-one forgets ;-)<hr>");
}


static LPCTSTR GetFooterHTML()
//
//	Supply some footer HTML
{
	return _T("<hr>Footer - You could put almost anything here");
}


static bool PrintHTML( LPCTSTR pcszHTML, HDC hdc, RECT &rc )
//
//	Helper function, simply prints the passed HTML to the DC provided.
{
	bool bRetVal = false;
	QHTMCONTEXT qhtmCtx = QHTM_PrintCreateContext( g_nZoomLevel );
	if( QHTM_PrintSetText( qhtmCtx, pcszHTML ) )
	{
		int nNumberOfPages;
		if( QHTM_PrintLayout( qhtmCtx, hdc, &rc, &nNumberOfPages ) )
		{

			//
			//	You aint given enough space for the html
			//ASSERT( nNumberOfPages == 1 );

			if( QHTM_PrintPage( qhtmCtx, hdc, 0, &rc ) )
				bRetVal = true;
		}
	}
	QHTM_PrintDestroyContext( qhtmCtx );

	return true;
}



void TestPrint( HWND hwndParent )
//
//	Note that you do not need to have a QHTM window in order to print!
{

	//
	//	Get a printer DC
	PRINTDLG  pd = { 0 };
	pd.lStructSize = sizeof(pd);

	pd.Flags = PD_NOSELECTION | PD_NOPAGENUMS | PD_USEDEVMODECOPIES | PD_RETURNDC;
	pd.hwndOwner = hwndParent;

	if( !PrintDlg( &pd ) )
		return;

	DOCINFO di = { 0 };
	di.cbSize = sizeof(di);
	di.lpszDocName = g_pszTitle;

	QHTMCONTEXT qhtmCtx = QHTM_PrintCreateContext( g_nZoomLevel );
	if( StartDoc( pd.hDC, &di ) > 0)
	{
		const RECT rcPage = { 0, 0, GetDeviceCaps( pd.hDC, HORZRES ), GetDeviceCaps( pd.hDC, VERTRES ) };
		const int nWidth = rcPage.right - rcPage.left;
		const int nHeaderHeight = QHTM_PrintGetHTMLHeight( pd.hDC, GetHeaderHTML(), nWidth, g_nZoomLevel );
		const int nFooterHeight = QHTM_PrintGetHTMLHeight( pd.hDC, GetFooterHTML(), nWidth, g_nZoomLevel );

		if( QHTM_PrintSetTextFile( qhtmCtx, g_pcszFileToPrint ) )
		{

			//
			//	Adjust the page rectangle for the header and footer heights
			RECT rcBodyPageRect = rcPage;
			rcBodyPageRect.top += nHeaderHeight;
			rcBodyPageRect.bottom -= nFooterHeight;

			//
			//	Layout our HTML and determine how many pages we need.
			int nNumberOfPages;

			//	We should check this return value but for brevity...
			(void)QHTM_PrintLayout( qhtmCtx, pd.hDC, &rcBodyPageRect, &nNumberOfPages );


			//
			//	For each page...
			for( int iPage = 0; iPage < nNumberOfPages; iPage++ )
			{
				if( StartPage( pd.hDC ) <= 0 )
					break;

				//
				//	Print the header
				RECT rcHeader = rcPage;
				rcHeader.bottom = nHeaderHeight;
				if( nHeaderHeight > 0 && !PrintHTML( GetHeaderHTML(), pd.hDC,  rcHeader ) )
					break;

				//
				//	Print our actual page
				if( !QHTM_PrintPage( qhtmCtx, pd.hDC, iPage, &rcBodyPageRect ) )
					break;


				//
				//	Print the footer.
				RECT rcFooter = rcPage;
				rcFooter.top = rcPage.bottom - nFooterHeight;
				if( nFooterHeight > 0 && !PrintHTML( GetFooterHTML(), pd.hDC, rcFooter ) )
					break;


				if( EndPage( pd.hDC ) <= 0 )
					break;
			}
			if( iPage < nNumberOfPages || EndDoc( pd.hDC ) <= 0 )
			{
				//
				//	Error handling //to do
			}
		}
	}

	//
	//	Some cleanup.
	DeleteDC( pd.hDC );
	pd.hDC = NULL;
	(void)QHTM_PrintDestroyContext( qhtmCtx );
}