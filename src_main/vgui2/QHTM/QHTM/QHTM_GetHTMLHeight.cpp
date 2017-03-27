/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	QHTM_GetHTMLHeight.cpp
Owner:	russf@gipsysoft.com
Purpose:	Give some HTML and a restriction of width measure the height
					of the HTML.
----------------------------------------------------------------------*/
#include "stdafx.h"

#include "qhtm.h"
#include "QHTM_Includes.h"
#include "Defaults.h"
#include "HtmlSection.h"

extern "C" BOOL WINAPI QHTM_GetHTMLHeight( HDC hdc, LPCTSTR pcsz, HINSTANCE hInst, UINT uSource, UINT uWidth, UINT *lpuHeight )
{
#ifdef QHTM_ALLOW_RENDER

	if( lpuHeight && pcsz && hdc )
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
		CDrawContext dc( NULL, hdc, false );
		sectHTML.OnLayout( rcDraw, dc );
		const CSize size( sectHTML.GetSize() );
		*lpuHeight = size.cy;
		return TRUE;
	}
#else	//	QHTM_ALLOW_RENDER
	UNREFERENCED_PARAMETER( hdc );
	UNREFERENCED_PARAMETER( pcsz );
	UNREFERENCED_PARAMETER( hInst );
	UNREFERENCED_PARAMETER( uSource );
	UNREFERENCED_PARAMETER( uWidth );
	UNREFERENCED_PARAMETER( lpuHeight );
#endif	//	QHTM_ALLOW_RENDER
	return FALSE;
}

