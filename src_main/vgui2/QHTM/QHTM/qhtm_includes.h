/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	QHTM_Inlcudes.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef QHTM_INLCUDES_H
#define QHTM_INLCUDES_H

#ifndef WINHELPER_H
	#include <WinHelper.h>
#endif	//	WINHELPER_H

using namespace WinHelper;

#ifndef PALETTE_H
	#include "Palette.h"
#endif	//	PALETTE_H

extern HWND g_hwnd;
extern HINSTANCE g_hQHTMInstance;

extern UINT g_uHandCursorID;
extern UINT g_uNoImageBitmapID;
extern HINSTANCE g_hResourceInstance;

#define QHTM_COOLTIPS

#ifdef QHTM_BUILD_LIGHT

#else	//	QHTM_BUILD_LIGHT

	//
	//	Enable some features
	#define QHTM_ALLOW_PRINT
	#define QHTM_ALLOW_RENDER
	#define QHTM_ALLOW_HTML_BUTTON

#endif	//	QHTM_BUILD_LIGHT

#ifndef QHTM_TRACE_H
	#include "QHTM_Trace.h"
#endif	//	QHTM_TRACE_H

#endif //QHTM_INLCUDES_H