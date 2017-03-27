/*----------------------------------------------------------------------
Copyright (c) 1998 Russell Freeman. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
Email: russf@gipsysoft.com
Web site: http://www.gipsysoft.com

This notice must remain intact.
This file belongs wholly to Russell Freeman, 
You may use this file compiled in your applications.
You may not sell this file in source form.
This source code may not be distributed as part of a library,

This file is provided 'as is' with no expressed or implied warranty.
The author accepts no liability if it causes any damage to your
computer.

Please use and enjoy. Please let me know of any bugs/mods/improvements 
that you have found/implemented and I will fix/incorporate them into this
file.

File:	DebugHlp.cpp
Owner:	russf@gipsysoft.com
Purpose:	The main entry point
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "DebugHlp.h"

HINSTANCE g_hInst = 0;

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = hModule;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
