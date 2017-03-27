/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	dllmain.cpp
Owner:	russf@gipsysoft.com
Purpose:	DLL main for the DLL version
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "QHTM_Includes.h"
#include "HTMLParseBase.h"
#include "QHTM.h"
#include "resource.h"

extern bool FASTCALL RegisterWindow( HINSTANCE hInst );

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CHTMLParseBase::Initialise();
		g_hQHTMInstance = (HINSTANCE)hModule;

		QHTM_SetResources( g_hQHTMInstance, IDC_QHTM_HAND, IDB_CANNOT_FIND_IMAGE );

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		extern void StopSublassing();
		StopSublassing();
		break;
  }
  return TRUE;
}
