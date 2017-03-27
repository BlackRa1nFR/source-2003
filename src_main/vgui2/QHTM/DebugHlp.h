/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	debughlp.h
Owner:	russf@gipsysoft.com
Purpose:	Debugging helpers
----------------------------------------------------------------------*/
#ifndef DEBUGHLP_H
#define DEBUGHLP_H

#ifndef _WINDOWS_
	#include <Windows.h>
#endif	//	_WINDOWS_

#pragma warning( disable : 4127 )	//	: conditional expression is constant

#ifndef DEBUGHLP_EXPORTS
	//#pragma comment(lib, "DebugHlp.lib")
#endif	//	DEBUGHLP_EXPORTS

#ifdef __AFX_H__
	#undef ASSERT
	#undef VERIFY
	#undef TRACE
#endif	//	__AFX_H__

#ifndef LPCSTR
	typedef const char * LPCSTR;
#endif	//	LPCSTR

#ifndef LPCWSTR
	/*lint -e18 */
	typedef const unsigned short * LPCWSTR;
	/*lint +e18 */
#endif	//	LPCSTR
#ifndef BOOL
	typedef int BOOL;
#endif	//	BOOL

	void _cdecl DebugTraceA( LPCSTR pcszFormat, ... );
	void _cdecl DebugTraceW( LPCWSTR pcszFormat, ... );

	void _cdecl DebugTraceLineAndFileA( LPCSTR pcszFilename, int nLine );
	void _cdecl DebugTraceLineAndFileW( LPCWSTR pcszFilename, int nLine );

	//	Return TRUE if the program should retry, use STOPHRE to do the retry.
	BOOL _cdecl AssertFailed( LPCSTR pcszFilename, int nLine, LPCSTR pcszExpression );
	BOOL _cdecl ApiFailure( LPCSTR pcszFilename, int nLine, LPCSTR pcszExpression );

	#ifndef DEBUGHLP_BARE_TRACE
		#define TRACEA DebugTraceLineAndFileA( __FILE__, __LINE__ ), ::DebugTraceA
		#define TRACEW DebugTraceLineAndFileW( _T(__FILE__), __LINE__ ), ::DebugTraceW
		#define TRACENLA ::DebugTraceA
		#define TRACENLW ::DebugTraceW

	#else	//	DEBUGHLP_BARE_TRACE
		#define TRACEA ::DebugTraceA
		#define TRACEW ::DebugTraceW
	#endif	//	DEBUGHLP_BARE_TRACE

#ifdef _DEBUG
	#define STOPHERE()	_asm{ int 3 }
	#define VERIFY( exp )	ASSERT( exp )
	#define ASSERT( exp ) \
		/*lint -e717 -e506 -e774*/ \
		do \
		{ \
			if( !( exp ) )\
			{\
				TRACEA( "Assert Failed with expression %s\n", #exp);\
				if( AssertFailed( __FILE__, __LINE__, #exp ) ) \
				{\
					STOPHERE();	\
				}\
			}\
		} while (0) \
		/*lint +e717 +e506 +e774*/ \

	#define VAPI(exp) \
		/*lint -e717 -e506 -e774*/ \
		do \
		{ \
			if( !(exp) )\
			{\
				TRACEA( "VAPI Failed with expression %s\n", #exp);\
				if( ApiFailure( __FILE__, __LINE__, #exp ) ) \
				{\
					STOPHERE();	\
				}\
			}\
		} while (0) \
		/*lint +e717 +e506 +e774*/ \

	#define ASSERT_VALID_HWND( hwnd )						ASSERT( ::IsWindow( hwnd ) )
	#define ASSERT_VALID_STR_LEN( str, n )			ASSERT( !::IsBadStringPtr( str, n ) )
	#define ASSERT_VALID_STR( str )							ASSERT_VALID_STR_LEN( str, 0xfffff )
	#define ASSERT_VALID_WRITEPTR( obj, n )			ASSERT( !::IsBadWritePtr( obj, n ) )
	#define ASSERT_VALID_READPTR( obj, n )			ASSERT( !::IsBadReadPtr( obj, n ) )
	#define ASSERT_VALID_WRITEOBJPTR( obj )			ASSERT_VALID_WRITEPTR( obj, sizeof(*obj ) )
	#define ASSERT_VALID_READOBJPTR( obj )			ASSERT_VALID_READPTR( obj, sizeof(*obj ) )

	#ifdef _UNICODE
		#define TRACE TRACEW
		#define TRACENL	TRACENLW
	#else	//	_UNICODE
		#define TRACE TRACEA
		#define TRACENL	TRACENLA
	#endif	//	_UNICODE

#else	//	_DEBUG
	#define ASSERT_VALID_HWND( hwnd )						((void)0)
	#define ASSERT_VALID_STR_LEN( str, n )			((void)0)
	#define ASSERT_VALID_STR( str )							((void)0)
	#define ASSERT_VALID_WRITEPTR( obj, n )			((void)0)
	#define ASSERT_VALID_READPTR( obj, n )			((void)0)
	#define ASSERT_VALID_WRITEOBJPTR( obj )			((void)0)
	#define ASSERT_VALID_READOBJPTR( obj )			((void)0)

	#ifdef _UNICODE
		#define TRACE							1 ? (void)0 : ::DebugTraceW
		#define TRACENL						1 ? (void)0 : ::DebugTraceW
	#else	//	_UNICODE
		#define TRACE							1 ? (void)0 : ::DebugTraceA
		#define TRACENL						1 ? (void)0 : ::DebugTraceA
	#endif	//	_UNICODE

	#define ASSERT(exp)					((void)0)
	#define STOPHERE					((void)0)
	#define VERIFY(exp)					((void)(exp))
	#define VAPI(exp)					((void)(exp))
#endif	//	_DEBUG

#endif //DEBUGHLP_H
