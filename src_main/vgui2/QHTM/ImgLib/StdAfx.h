// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__9377D73B_2A34_11D3_9EC6_0020AF5C17CE__INCLUDED_)
#define AFX_STDAFX_H__9377D73B_2A34_11D3_9EC6_0020AF5C17CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <tchar.h>

#include <DebugHlp.h>
#include <Array.h>
#include <crtdbg.h>

#pragma warning( disable : 4710 )	//	function 'blah' not inlined

class CFrame;
typedef Container::CArray< CFrame*> CFrameArray;


#ifdef _DEBUG
  #define MYDEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   //allocations to be of _CLIENT_BLOCK type
#else
  #define MYDEBUG_NEW
#endif // _DEBUG

#ifdef _DEBUG
	#define new MYDEBUG_NEW
#endif


// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9377D73B_2A34_11D3_9EC6_0020AF5C17CE__INCLUDED_)
