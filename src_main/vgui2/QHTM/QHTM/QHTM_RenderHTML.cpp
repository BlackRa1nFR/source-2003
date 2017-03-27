/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	QHTM_RenderHTML.cpp
Owner:	russf@gipsysoft.com
Purpose:	Give some HTML and a restriction of width render the HTML to the device.
----------------------------------------------------------------------*/
#include "stdafx.h"

#include "qhtm.h"
#include "QHTM_Includes.h"
#include "Defaults.h"
#include "HtmlSection.h"


extern "C" BOOL WINAPI QHTM_RenderHTML( HDC hdc, LPCTSTR pcsz, HINSTANCE hInst, UINT uSource, UINT uWidth )
{
#ifdef QHTM_ALLOW_RENDER
	if( pcsz && hdc )
	{
		CHTMLSection sectHTML( NULL, &g_defaults );

		switch( uSource )
		{
		case QHTM_SOURCE_TEXT:
			sectHTML.SetHTML( pcsz, _tcslen( pcsz ), NULL );
			break;

		case QHTM_SOURCE_RESOURCE:
			if( !sectHTML.SetHTML( hInst, pcsz ) )
			{
				return FALSE;
			}
			break;

		case QHTM_SOURCE_FILENAME:
			if( !sectHTML.SetHTMLFile( pcsz ) )
			{
				return FALSE;
			}
			break;
		}

		CRect rcDraw( 0, 0, uWidth, 5 );

		//
		//	Needs to be scoped because firstly we are just measuring and this doesn't require a clip rect
		{
			CDrawContext dc( NULL, hdc, false );
			sectHTML.OnLayout( rcDraw, dc );
		}
		const CSize size( sectHTML.GetSize() );
		rcDraw.bottom = size.cy;

		//
		//	Now we know the size, we can pass this into the clip rect.
		{
			CDrawContext dc( &rcDraw, hdc, false );
			sectHTML.OnLayout( rcDraw, dc );
			SelectPalette( hdc, GetCurrentWindowsPalette(), TRUE );
			RealizePalette( hdc );
			sectHTML.OnDraw( dc );
		}
		return TRUE;
	}
#else	//	QHTM_ALLOW_RENDER
	UNREFERENCED_PARAMETER( hdc );
	UNREFERENCED_PARAMETER( pcsz );
	UNREFERENCED_PARAMETER( hInst );
	UNREFERENCED_PARAMETER( uSource );
	UNREFERENCED_PARAMETER( uWidth );
#endif	//	QHTM_ALLOW_RENDER
	return FALSE;
}

