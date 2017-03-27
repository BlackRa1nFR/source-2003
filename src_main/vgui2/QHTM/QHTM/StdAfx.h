/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	stdafx.h
Owner:	russf@gipsysoft.com
Purpose:	Precompiled header file.
----------------------------------------------------------------------*/
#ifndef STDAFX_H
#define STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning( disable : 4355 )	//	'this' : used in base member initializer list
#pragma warning( disable : 4710 )	//	'function' : function not inlined - specifically for the ArrayClass

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <TCHAR.h>
#include <stdlib.h>

//#define QHTM_TRACE_ENABLED	1

#endif //STDAFX_H