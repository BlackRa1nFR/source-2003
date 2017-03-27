/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	APIExample.cpp
Owner:	russf@gipsysoft.com
Purpose:	Demonstrate QHTM usage in an API programme.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <COMMCTRL.H>
#include <stdlib.h>
#include <shellapi.h>
#include <Commdlg.h>
#include <QHTM/QHTM.h>

#define QHTM_FULL_VERSION

#include "Resource.h"

#include <debughlp.h>
#include <WinHelper.h>
static HINSTANCE g_hInstance = NULL;
static TCHAR g_szFilenameBuffer[ MAX_PATH + MAX_PATH ] = _T("");

extern void SaveAsBitmap( HWND hwndParent, LPCTSTR pcszHTMLFilename, LPCTSTR pcszBMPFilename );

////////////////////////////////////////

////////////////////////////////////////

extern void TestPrint( HWND hwndParent );

static void SetToolTipText( HWND hwndToolTip, HWND hwndControl, UINT uStringID )
//
//	Helper function that adds tools to a tooltip for us.
{
	TCHAR szTextBuffer[ 1024 ];		//	A resonably large buffer.
	LoadString( g_hInstance, uStringID, szTextBuffer, sizeof( szTextBuffer ) / sizeof( szTextBuffer[0] ) );

	TTTOOLINFO ti = {0};
	ti.cbSize = sizeof( ti );
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = hwndControl;
	ti.uId = (UINT) hwndControl;
	GetClientRect( hwndControl, &ti.rect );
	ti.lpszText = szTextBuffer;
	
	if( !SendMessage( hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti ) )
	{
		MessageBox( NULL, _T("Failed to create tool tips"), _T("API Exe"), MB_OK );
	}

}

static DWORD GetModulePath( HINSTANCE hInst, LPTSTR pszBuffer, DWORD dwSize )
//
//	Return the size of the path in bytes.
{
	DWORD dwLength = GetModuleFileName( hInst, pszBuffer, dwSize );
	if( dwLength )
	{
		while( dwLength && pszBuffer[ dwLength ] != _T('\\') )
		{
			dwLength--;
		}

		if( dwLength )
			pszBuffer[ dwLength + 1 ] = _T('\000');
	}
	return dwLength;
}




static void LoadSampleHTMLList( HWND hwndList )
//
//	Load our list box with the list of documents we will display
{
	struct SItem
	{
		LPCTSTR pcszDocumentName;
		LPCTSTR pcszFilenameName;
	};

	static const SItem item[] =
										{
											{ _T("Welcome"), _T("welcome.html") }
											, { _T("Fonts"), _T("fonts.html") }
											, { _T("Styles"), _T("styles.html") }
											, { _T("Headings"), _T("headings.html") }
											, { _T("Lists"), _T("lists.html") }
											, { _T("Tables"), _T("tables.html") }
											, { _T("Links"), _T("hyperlink.html") }
											, { _T("Images"), _T("images.html") }
											, { _T("About"), _T("about.html") }
											, { _T("Zoom+ Features"), _T("zoomplus/features.html") }
										};

	for( UINT u = 0; u < sizeof( item ) / sizeof( item[ 0 ] ); u++ )
	{
		const int nItemId = SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM)item[ u ].pcszDocumentName );
		if( nItemId >= LB_OKAY )
		{
			(void)SendMessage( hwndList, LB_SETITEMDATA, nItemId, (LPARAM)item[ u ].pcszFilenameName );
		}
	}
}


static void LoadHTML( HWND hwndList, HWND hwndHTML, UINT uIndex )
{
	LPCTSTR pcszFilename = reinterpret_cast<LPCTSTR>( SendMessage( hwndList, LB_GETITEMDATA, uIndex, 0 ) );
	if( pcszFilename )
	{
		//
		//	Because the "file open" dialog changes the current directory path we need to supply the
		//	full path to the QHTM_LoadFromFile function.
		GetModulePath( g_hInstance, g_szFilenameBuffer, sizeof( g_szFilenameBuffer ) / sizeof( g_szFilenameBuffer[0] ) );
		_tcscat( g_szFilenameBuffer, pcszFilename );
		//
		//	Note that we ignore the return value here, you should handle it and display appropriate warnings etc.
		QHTM_LoadFromFile( hwndHTML, g_szFilenameBuffer );
	}
}


static void OnInitiDialog( HWND hwnd )
{
	HWND hwndList = GetDlgItem( hwnd, IDC_DEMO_LIST );

	//
	//	Set toolips for the controls on this dialog
	HWND hwndToolTip = CreateWindow( TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, 0,0,0,0, hwnd, NULL, g_hInstance, 0 );
	HWND hwndZoom = GetDlgItem( hwnd, IDC_ZOOM );
	(void)::SendMessage( hwndToolTip, TTM_ACTIVATE, static_cast<WPARAM>(true), 0);
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDOK ), IDS_TIP_ID_OK );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_PRINT ), IDS_TIP_ID_PRINT );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_OPEN ), IDS_TIP_ID_OPEN );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_TRANSPARENT ), IDS_TIP_ID_TRANSPARENT );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_BORDER ), IDS_TIP_ID_BORDER );
	SetToolTipText( hwndToolTip, hwndList, IDS_TIP_ID_DEMO_LIST );
	SetToolTipText( hwndToolTip, hwndZoom, IDS_TIP_ID_ZOOM );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_REFRESH ), IDS_TIP_ID_REFRESH );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_VIEW_SOURCE ), IDS_TIP_ID_VIEW_SOURCE );
	SetToolTipText( hwndToolTip, GetDlgItem( hwnd, IDC_MESSAGEBOX ), IDS_TIP_ID_MESSAGEBOX );

	(void)CheckDlgButton( hwnd, IDC_BORDER, TRUE );
	
	//
	//	Load our sample list and also select the first item
	LoadSampleHTMLList( hwndList );
	HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
	(void)SendMessage( hwndList, LB_SETCURSEL, 0, 0 );
	LoadHTML( hwndList, hwndQHTM, 0 );

	//
	//	Set the QHTM zoom options and set the initial zoom value
	(void)SendMessage( hwndZoom, CB_ADDSTRING, 0, (LPARAM)_T("Smallest") );
	(void)SendMessage( hwndZoom, CB_ADDSTRING, 0, (LPARAM)_T("Smaller") );
	(void)SendMessage( hwndZoom, CB_ADDSTRING, 0, (LPARAM)_T("Medium") );
	(void)SendMessage( hwndZoom, CB_ADDSTRING, 0, (LPARAM)_T("Larger") );
	(void)SendMessage( hwndZoom, CB_ADDSTRING, 0, (LPARAM)_T("Largest") );
	(void)SendMessage( hwndZoom, CB_SETCURSEL, 2, 0 );
	QHTM_SetZoomLevel( hwndQHTM, QHTM_ZOOM_DEFAULT );

	//
	//	Enable this to have QHTM ask for the brush for the background.
	//	See WM_CTLCOLORSTATIC below.
	//QHTM_SetUseColorStatic( hwndQHTM, TRUE );
}


BOOL CALLBACK TestDialog( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			OnInitiDialog( hwnd );
		}
		break;

	case WM_CTLCOLORSTATIC:
		{
			if( GetWindowLong( (HWND)lParam, GWL_ID ) == IDC_HTML_DISPLAY )
			{
				HBRUSH hbr = CreateSolidBrush( RGB( 255, 0, 0 ) );
				return (BOOL)hbr;
			}
		}
		break;

	case WM_NOTIFY:
		switch( wParam )
		{
		case IDC_HTML_DISPLAY:
			{
				LPNMQHTM pnm = reinterpret_cast<LPNMQHTM>( lParam );
				if( pnm->pcszLinkText )
				{
					TCHAR szBigBuffer[ 1024 ];
					wsprintf( szBigBuffer, _T("Shall I let the QHTM control handle the link to <a href='%s'>%s</a>.<br>")
																	_T("Click Yes to let QHTM resolve the link or click no to do nothing.")
																	_T("<p>Yes, this is one of those pesky <code>QHTM_MessageBox</code> thingies.")
																	, pnm->pcszLinkText, pnm->pcszLinkText );
					if( ::QHTM_MessageBox( hwnd, szBigBuffer, _T("APIExample"), MB_YESNOCANCEL | MB_ICONQUESTION ) != IDYES )
					{
						pnm->resReturnValue = FALSE;
					}
				}
				return TRUE;
			}
			break;
		}
		break;


	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDOK:
		case IDCANCEL:
			(void)EndDialog( hwnd, LOWORD(wParam) );
			return TRUE;

		case IDC_SAVE_AS_BITMAP:
#ifdef QHTM_FULL_VERSION
			{
				TCHAR szBuffer[ sizeof( g_szFilenameBuffer ) / sizeof( g_szFilenameBuffer[0] ) ] = _T("");
				OPENFILENAME ofn = {0};
				ofn.lStructSize = sizeof( ofn );
				ofn.hwndOwner = hwnd;
				ofn.lpstrFilter = _T("All BMP files\0*.bmp;\0");
				ofn.lpstrFile = szBuffer;
				ofn.nMaxFile = sizeof( szBuffer ) / sizeof( szBuffer[0] );
				ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
				if( GetSaveFileName( &ofn ) )
				{
					SaveAsBitmap( hwnd, g_szFilenameBuffer, szBuffer );
				}
			}
#else	//	QHTM_FULL_VERSION
			QHTM_MessageBox( hwnd
						, _T("<h4>Sorry</h4>Rendering HTML onto any device is only available in the full version of <a href=\"http://www.gipsysoft.com/qhtm/\">QHTM</a>.<br>")
							_T("You can buy the full version by <a href=\"http://www.gipsysoft.com/qhtm/purchase.shtml\">visiting gipsysoft.com</a>")
						, _T("Sorry")
						, MB_ICONHAND | MB_OK );
#endif	//	QHTM_FULL_VERSION
			break;

		case IDC_REFRESH:
			{
				HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
				QHTM_LoadFromFile( hwndQHTM, g_szFilenameBuffer );
			}
			break;


		case IDC_VIEW_SOURCE:
			{
				ShellExecute( hwnd, "open", "notepad.exe", g_szFilenameBuffer, NULL, SW_SHOW );
				HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
				QHTM_LoadFromFile( hwndQHTM, g_szFilenameBuffer );
			}
			break;

		case IDC_TRANSPARENT:
			if( IsDlgButtonChecked( hwnd, IDC_TRANSPARENT ) )
			{
				HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
				//
				//	Add transparent
				SetWindowLong( hwndQHTM, GWL_EXSTYLE, GetWindowLong( hwndQHTM, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
			{
				HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
				SetWindowLong( hwndQHTM, GWL_EXSTYLE, GetWindowLong( hwndQHTM, GWL_EXSTYLE ) & (~WS_EX_TRANSPARENT) );
			}
			break;


		case IDC_PRINT:
#ifdef QHTM_FULL_VERSION
			TestPrint( hwnd );
#else	//	QHTM_FULL_VERSION
			QHTM_MessageBox( hwnd
						, _T("<h4>Sorry</h4>Printing HTML is only available in the full version of <a href=\"http://www.gipsysoft.com/qhtm/\">QHTM</a>.<br>")
							_T("You can buy the full version by <a href=\"http://www.gipsysoft.com/qhtm/purchase.shtml\">visiting gipsysoft.com</a>")
						, _T("Sorry")
						, MB_ICONHAND | MB_OK );
#endif	//	QHTM_FULL_VERSION
			break;


		case IDC_MESSAGEBOX:
			{
				TCHAR szTextBuffer[ 4096 ];		//	A resonably large buffer.
				LoadString( g_hInstance, IDS_MESSAGEBOX_TEXT, szTextBuffer, sizeof( szTextBuffer ) / sizeof( szTextBuffer[0] ) );
				
				QHTM_MessageBox( hwnd, szTextBuffer, "My big MessageBox", MB_OK | MB_ICONWARNING );
			}
			break;

		case IDC_BORDER:
			if( IsDlgButtonChecked( hwnd, IDC_BORDER ) )
			{
				HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
				//
				//	Add borders
				SetWindowLong( hwndQHTM, GWL_EXSTYLE, GetWindowLong( hwndQHTM, GWL_EXSTYLE ) | WS_EX_CLIENTEDGE );
			}
			else
			{
				HWND hwndQHTM = GetDlgItem( hwnd, IDC_HTML_DISPLAY );
				SetWindowLong( hwndQHTM, GWL_EXSTYLE, GetWindowLong( hwndQHTM, GWL_EXSTYLE ) & (~WS_EX_CLIENTEDGE) );
			}
			break;

		case IDC_OPEN:
			{
				TCHAR szBuffer[ sizeof( g_szFilenameBuffer ) / sizeof( g_szFilenameBuffer[0] ) ] = _T("");
				OPENFILENAME ofn = {0};
				ofn.lStructSize = sizeof( ofn );
				ofn.hwndOwner = hwnd;
				ofn.lpstrFilter = _T("All HTML files\0*.html;*.shtml;*.htm\0");
				ofn.lpstrFile = szBuffer;
				ofn.nMaxFile = sizeof( szBuffer ) / sizeof( szBuffer[0] );
				ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
				if( GetOpenFileName( &ofn ) )
				{
					_tcscpy( g_szFilenameBuffer, szBuffer );
					QHTM_LoadFromFile( GetDlgItem( hwnd, IDC_HTML_DISPLAY ), ofn.lpstrFile );
				}
			}
			return TRUE;

		case IDC_ZOOM:
			switch( HIWORD( wParam ) )
			{
			case CBN_SELCHANGE:
				{
					HWND hwndCombo = reinterpret_cast<HWND> ( lParam );
					QHTM_SetZoomLevel( GetDlgItem( hwnd, IDC_HTML_DISPLAY ), SendMessage( hwndCombo, CB_GETCURSEL, 0, 0 ) );
				}
				break;
			}
			break;

		case IDC_DEMO_LIST:
			switch( HIWORD( wParam ) )
			{
			case LBN_SELCHANGE:
				{
					HWND hwndList = reinterpret_cast<HWND> ( lParam );
					LoadHTML( hwndList, GetDlgItem( hwnd, IDC_HTML_DISPLAY ), SendMessage( hwndList, LB_GETCURSEL, 0, 0 ) );
				}
				break;
			}
			break;
		}
		break;
	}
	return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE ,
                     LPSTR     ,
                     int       )
{
 	//	I ignore the return values to keep the code uncluttered.
	(void)QHTM_Initialize( hInstance );
	(void)QHTM_EnableCooltips();

	g_hInstance = hInstance;

	//
	//	This is the only *extra* function call needed when linking statically
	QHTM_SetResources( hInstance, IDC_QHTM_HAND, IDB_CANNOT_FIND_IMAGE );

	DialogBox( hInstance, MAKEINTRESOURCE( IDD_TEST ), NULL, TestDialog );
	return 0;
}
