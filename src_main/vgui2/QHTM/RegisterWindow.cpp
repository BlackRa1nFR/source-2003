/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	RegisterWindow.cpp
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "QHTM.h"
#include "QHTMControlSection.h"

extern bool FASTCALL RegisterWindow( HINSTANCE hInst );

bool FASTCALL RegisterWindow( HINSTANCE hInst )
{
	//	Needs an instance handle!
	ASSERT( hInst );

	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof( WNDCLASSEX );

	wcex.style			= CS_BYTEALIGNCLIENT;
	wcex.lpfnWndProc	= (WNDPROC)CQHTMControlSection::WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof( CWindowSection * );;
	wcex.hInstance		= hInst;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= QHTM_CLASSNAME;

	return RegisterClassEx( &wcex ) != 0;
}

