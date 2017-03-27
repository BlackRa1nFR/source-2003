
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#include <winsock.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "tier0/vcrmode.h"

												
#define VCRFILE_VERSION		2
												
												
#define VCR_RuntimeAssert(x)	VCR_RuntimeAssertFn(x, #x)
												

bool		g_bExpectingWindowProcCalls = false;

IVCRHelpers	*g_pHelpers = 0;

FILE		*g_pVCRFile = NULL;
VCRMode		g_VCRMode = VCR_Disabled;
VCRMode		g_OldVCRMode = (VCRMode)-1;		// Stored temporarily between SetEnabled(0)/SetEnabled(1) blocks.
int			g_iCurEvent = 0;

int			g_CurFilePos = 0;				// So it knows when we're done playing back.
int			g_FileLen = 0;					

VCREvent	g_LastReadEvent = (VCREvent)-1;	// Last VCR_ReadEvent() call.

int			g_bVCREnabled = 0;


// ---------------------------------------------------------------------- //
// Internal functions.
// ---------------------------------------------------------------------- //

static void VCR_RuntimeAssertFn(int bAssert, char const *pStr)
{
	if(!bAssert)
	{
		char str[256];
		
		#ifdef _DEBUG
			__asm
			{
					int 3
			}
		#endif

		sprintf(str, "*** VCR ASSERT FAILED: %s ***\n", pStr);
		g_pHelpers->ErrorMessage( str );
		g_pVCR->End();
	}
}

static void VCR_Read(void *pDest, int size)
{
	if(!g_pVCRFile)
	{
		memset(pDest, 0, size);
		return;
	}

	fread(pDest, 1, size, g_pVCRFile);
	
	g_CurFilePos += size;
	
	VCR_RuntimeAssert(g_CurFilePos <= g_FileLen);
	
	if(g_CurFilePos >= g_FileLen)
	{
		g_pVCR->End();
	}
}

template<class T>
static void VCR_ReadVal(T &val)
{
	VCR_Read(&val, sizeof(val));
}

static void VCR_Write(void const *pSrc, int size)
{
	fwrite(pSrc, 1, size, g_pVCRFile);
	fflush(g_pVCRFile);
}

template<class T>
static void VCR_WriteVal(T &val)
{
	VCR_Write(&val, sizeof(val));
}


// Hook from ExtendedTrace.cpp
bool g_bTraceRead = false;
void OutputDebugStringFormat( const char *pMsg, ... )
{
	char msg[4096];
	va_list marker;
	va_start( marker, pMsg );
	_vsnprintf( msg, sizeof( msg ), pMsg, marker );
	va_end( marker );
	int len = strlen( msg );
	
	if ( g_bTraceRead )
	{
		char tempData[4096];
		int tempLen;
		VCR_ReadVal( tempLen );
		VCR_RuntimeAssert( tempLen <= sizeof( tempData ) );
		VCR_Read( tempData, tempLen );
		tempData[tempLen] = 0;
		OutputDebugString( "FILE: " );
		OutputDebugString( tempData );

		VCR_RuntimeAssert( memcmp( msg, tempData, len ) == 0 );
	}
	else
	{
		VCR_WriteVal( len );
		VCR_Write( msg, len );
	}
}


static VCREvent VCR_ReadEvent()
{
	g_bTraceRead = true;
	//STACKTRACE();

	char event;
	VCR_Read(&event, 1);
	g_LastReadEvent = (VCREvent)event;

	return (VCREvent)event;
}


static void VCR_WriteEvent(VCREvent event)
{
	g_bTraceRead = false;
	//STACKTRACE();

	// Write a stack trace.
	char cEvent = (char)event;
	VCR_Write(&cEvent, 1);
}

static void VCR_IncrementEvent()
{
	++g_iCurEvent;
}

static void VCR_Event(VCREvent type)
{
	if(g_VCRMode == VCR_Disabled)
		return;

	VCR_IncrementEvent();
	if(g_VCRMode == VCR_Record)
	{
		VCR_WriteEvent(type);
	}
	else
	{
		VCREvent currentEvent = VCR_ReadEvent();
		VCR_RuntimeAssert( currentEvent == type );
	}
}


// ---------------------------------------------------------------------- //
// VCR trace interface.
// ---------------------------------------------------------------------- //

class CVCRTrace : public IVCRTrace
{
public:
	virtual VCREvent	ReadEvent()
	{
		return VCR_ReadEvent();
	}

	virtual void		Read( void *pDest, int size )
	{
		VCR_Read( pDest, size );
	}
};

static CVCRTrace g_VCRTrace;


// ---------------------------------------------------------------------- //
// VCR interface.
// ---------------------------------------------------------------------- //

static int VCR_Start( char const *pFilename, bool bRecord, IVCRHelpers *pHelpers )
{
	unsigned long version;

	g_pHelpers = pHelpers;
	
	g_pVCR->End();

	g_OldVCRMode = (VCRMode)-1;
	if(bRecord)
	{
		g_pVCRFile = fopen( pFilename, "wb" );
		if( g_pVCRFile )
		{
			// Write the version.
			version = VCRFILE_VERSION;
			VCR_Write(&version, sizeof(version));

			g_VCRMode = VCR_Record;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		g_pVCRFile = fopen( pFilename, "rb" );
		if( g_pVCRFile )
		{
			// Get the file length.
			fseek(g_pVCRFile, 0, SEEK_END);
			g_FileLen = ftell(g_pVCRFile);
			fseek(g_pVCRFile, 0, SEEK_SET);
			g_CurFilePos = 0;

			// Verify the file version.
			VCR_Read(&version, sizeof(version));
			if(version != VCRFILE_VERSION)
			{
				assert(!"VCR_Start: invalid file version");
				g_pVCR->End();
				return FALSE;
			}
			
			g_VCRMode = VCR_Playback;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}


static void VCR_End()
{
	if(g_pVCRFile)
	{
		fclose(g_pVCRFile);
		g_pVCRFile = NULL;
	}

	g_VCRMode = VCR_Disabled;
}


static IVCRTrace* VCR_GetVCRTraceInterface()
{
	return &g_VCRTrace;
}


static VCRMode VCR_GetMode()
{
	return g_VCRMode;
}


static void VCR_SetEnabled(int bEnabled)
{
	if(bEnabled)
	{
		VCR_RuntimeAssert(g_OldVCRMode != (VCRMode)-1);
		g_VCRMode = g_OldVCRMode;
		g_OldVCRMode = (VCRMode)-1;
	}
	else
	{
		VCR_RuntimeAssert(g_OldVCRMode == (VCRMode)-1);
		g_OldVCRMode = g_VCRMode;
		g_VCRMode = VCR_Disabled;
	}
}


static void VCR_SyncToken(char const *pToken)
{
	unsigned char len;

	VCR_Event(VCREvent_SyncToken);

	if(g_VCRMode == VCR_Record)
	{
		int intLen = strlen( pToken );
		assert( intLen <= 255 );

		len = (unsigned char)intLen;
		
		VCR_Write(&len, 1);
		VCR_Write(pToken, len);
	}
	else if(g_VCRMode == VCR_Playback)
	{
		char test[256];

		VCR_Read(&len, 1);
		VCR_Read(test, len);
		
		VCR_RuntimeAssert( len == (unsigned char)strlen(pToken) );
		VCR_RuntimeAssert( memcmp(pToken, test, len) == 0 );
	}
}


static double VCR_Hook_Sys_FloatTime(double time)
{
	VCR_Event(VCREvent_Sys_FloatTime);

	if(g_VCRMode == VCR_Record)
	{
		VCR_Write(&time, sizeof(time));
	}
	else if(g_VCRMode == VCR_Playback)
	{
		VCR_Read(&time, sizeof(time));
	}

	return time;
}



static int VCR_Hook_PeekMessage(
	struct tagMSG *msg, 
	void *hWnd, 
	unsigned int wMsgFilterMin, 
	unsigned int wMsgFilterMax, 
	unsigned int wRemoveMsg
	)
{
	if( g_VCRMode == VCR_Record )
	{
		// The trapped windowproc calls should be flushed by the time we get here.
		int ret;
		ret = PeekMessage( (MSG*)msg, (HWND)hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg );

		// NOTE: this must stay AFTER the trapped window proc calls or things get 
		// read back in the wrong order.
		VCR_Event( VCREvent_PeekMessage );

		VCR_WriteVal(ret);
		if(ret)
			VCR_Write(msg, sizeof(MSG));

		return ret;
	}
	else if( g_VCRMode == VCR_Playback )
	{
		// Playback any windows messages that got trapped.
		VCR_Event( VCREvent_PeekMessage );

		int ret;
		VCR_ReadVal(ret);
		if(ret)
			VCR_Read(msg, sizeof(MSG));

		return ret;
	}
	else
	{
		return PeekMessage((MSG*)msg, (HWND)hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	}
}


void VCR_Hook_RecordGameMsg( unsigned int uMsg, unsigned int wParam, long lParam )
{
	if ( g_VCRMode == VCR_Record )
	{
		VCR_Event( VCREvent_GameMsg );
		
		char val = 1;
		VCR_WriteVal( val );
		VCR_WriteVal( uMsg );
		VCR_WriteVal( wParam );
		VCR_WriteVal( lParam );
	}
}


void VCR_Hook_RecordEndGameMsg()
{
	if ( g_VCRMode == VCR_Record )
	{
		VCR_Event( VCREvent_GameMsg );
		char val = 0;
		VCR_WriteVal( val );	// record that there are no more messages.
	}
}


bool VCR_Hook_PlaybackGameMsg( unsigned int &uMsg, unsigned int &wParam, long &lParam )
{
	if ( g_VCRMode == VCR_Playback )
	{
		VCR_Event( VCREvent_GameMsg );
		
		char bMsg;
		VCR_ReadVal( bMsg );
		if ( bMsg )
		{
			VCR_ReadVal( uMsg );
			VCR_ReadVal( wParam );
			VCR_ReadVal( lParam );
			return true;
		}
	}
	
	return false;
}


static void VCR_Hook_GetCursorPos(struct tagPOINT *pt)
{
	VCR_Event(VCREvent_GetCursorPos);

	if(g_VCRMode == VCR_Playback)
	{
		VCR_ReadVal(*pt);
	}
	else
	{
		GetCursorPos(pt);

		if(g_VCRMode == VCR_Record)
		{
			VCR_WriteVal(*pt);
		}
	}
}


static void VCR_Hook_ScreenToClient(void *hWnd, struct tagPOINT *pt)
{
	VCR_Event(VCREvent_ScreenToClient);

	if(g_VCRMode == VCR_Playback)
	{
		VCR_ReadVal(*pt);
	}
	else
	{
		ScreenToClient((HWND)hWnd, pt);

		if(g_VCRMode == VCR_Record)
		{
			VCR_WriteVal(*pt);
		}
	}
}


static int VCR_Hook_recvfrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
	VCR_Event(VCREvent_recvfrom);

	int ret;
	if ( g_VCRMode == VCR_Playback )
	{
		// Get the result from our file.
		VCR_Read(&ret, sizeof(ret));
		if(ret == SOCKET_ERROR)
		{
			int err;
			VCR_ReadVal(err);
			WSASetLastError(err);
		}
		else
		{
			VCR_Read( buf, ret );

			char bFrom;
			VCR_ReadVal( bFrom );
			if ( bFrom )
			{
				VCR_Read( from, *fromlen );
			}
		}
	}
	else
	{
		ret = recvfrom((SOCKET)s, buf, len, flags, from, fromlen);

		if ( g_VCRMode == VCR_Record )
		{
			// Record the result.
			VCR_Write(&ret, sizeof(ret));
			if(ret == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				VCR_WriteVal(err);
			}
			else
			{
				VCR_Write( buf, ret );
				
				char bFrom = !!from;
				VCR_WriteVal( bFrom );
				if ( bFrom )
					VCR_Write( from, *fromlen );
			}
		}
	}

	return ret;
}


static void VCR_Hook_Cmd_Exec(char **f)
{
	VCR_Event(VCREvent_Cmd_Exec);

	if(g_VCRMode == VCR_Playback)
	{
		int len;

		VCR_Read(&len, sizeof(len));
		if(len == -1)
		{
			*f = NULL;
		}
		else
		{
			*f = (char*)malloc(len);
			VCR_Read(*f, len);
		}
	}
	else if(g_VCRMode == VCR_Record)
	{
		int len;
		char *str = *f;

		if(str)
		{
			len = strlen(str)+1;
			VCR_Write(&len, sizeof(len));
			VCR_Write(str, len);
		}
		else
		{
			len = -1;
			VCR_Write(&len, sizeof(len));
		}
	}
}


static char* VCR_Hook_GetCommandLine()
{
	VCR_Event(VCREvent_CmdLine);

	int len;
	char *ret;

	if(g_VCRMode == VCR_Playback)
	{
		VCR_Read(&len, sizeof(len));
		ret = new char[len];
		VCR_Read(ret, len);
	}
	else
	{
		ret = GetCommandLine();

		if(g_VCRMode == VCR_Record)
		{
			len = strlen(ret) + 1;
			VCR_WriteVal(len);
			VCR_Write(ret, len);
		}
	}
	
	return ret;
}


static long VCR_Hook_RegOpenKeyEx( void *hKey, const char *lpSubKey, unsigned long ulOptions, unsigned long samDesired, void *pHKey )
{
	VCR_Event(VCREvent_RegOpenKeyEx);

	long ret;
	if(g_VCRMode == VCR_Playback)
	{
		VCR_ReadVal(ret); // (don't actually write anything to the person's registry when playing back).
	}
	else
	{
		ret = RegOpenKeyEx( (HKEY)hKey, lpSubKey, ulOptions, samDesired, (PHKEY)pHKey );

		if(g_VCRMode == VCR_Record)
			VCR_WriteVal(ret);
	}

	return ret;
}


static long VCR_Hook_RegSetValueEx(void *hKey, char const *lpValueName, unsigned long Reserved, unsigned long dwType, unsigned char const *lpData, unsigned long cbData)
{
	VCR_Event(VCREvent_RegSetValueEx);

	long ret;
	if(g_VCRMode == VCR_Playback)
	{
		VCR_ReadVal(ret); // (don't actually write anything to the person's registry when playing back).
	}
	else
	{
		ret = RegSetValueEx((HKEY)hKey, lpValueName, Reserved, dwType, lpData, cbData);

		if(g_VCRMode == VCR_Record)
			VCR_WriteVal(ret);
	}

	return ret;
}


static long VCR_Hook_RegQueryValueEx(void *hKey, char const *lpValueName, unsigned long *lpReserved, unsigned long *lpType, unsigned char *lpData, unsigned long *lpcbData)
{
	VCR_Event(VCREvent_RegQueryValueEx);

	// Doesn't support this being null right now (although it would be trivial to add support).
	assert(lpData);
	
	long ret;
	unsigned long dummy = 0;
	if(g_VCRMode == VCR_Playback)
	{
		VCR_ReadVal(ret);
		VCR_ReadVal(lpType ? *lpType : dummy);
		VCR_ReadVal(*lpcbData);
		VCR_Read(lpData, *lpcbData);
	}
	else
	{
		ret = RegQueryValueEx((HKEY)hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

		if(g_VCRMode == VCR_Record)
		{
			VCR_WriteVal(ret);
			VCR_WriteVal(lpType ? *lpType : dummy);
			VCR_WriteVal(*lpcbData);
			VCR_Write(lpData, *lpcbData);
		}
	}

	return ret;
}


static long VCR_Hook_RegCreateKeyEx(void *hKey, char const *lpSubKey, unsigned long Reserved, char *lpClass, unsigned long dwOptions, 
	unsigned long samDesired, void *lpSecurityAttributes, void *phkResult, unsigned long *lpdwDisposition)
{
	VCR_Event(VCREvent_RegCreateKeyEx);

	long ret;
	if(g_VCRMode == VCR_Playback)
	{
		VCR_ReadVal(ret); // (don't actually write anything to the person's registry when playing back).
	}
	else
	{
		ret = RegCreateKeyEx((HKEY)hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, (LPSECURITY_ATTRIBUTES)lpSecurityAttributes, (HKEY*)phkResult, lpdwDisposition);

		if(g_VCRMode == VCR_Record)
			VCR_WriteVal(ret);
	}

	return ret;
}


static void VCR_Hook_RegCloseKey(void *hKey)
{
	VCR_Event(VCREvent_RegCloseKey);

	if(g_VCRMode == VCR_Playback)
	{
	}
	else
	{
		RegCloseKey((HKEY)hKey);
	}
}


int VCR_Hook_GetNumberOfConsoleInputEvents( void *hInput, unsigned long *pNumEvents )
{
	VCR_Event( VCREvent_GetNumberOfConsoleInputEvents );

	char ret;
	if ( g_VCRMode == VCR_Playback )
	{
		VCR_ReadVal( ret );
		VCR_ReadVal( *pNumEvents );
	}
	else
	{
		ret = (char)GetNumberOfConsoleInputEvents( (HANDLE)hInput, pNumEvents );

		if ( g_VCRMode == VCR_Record )
		{
			VCR_WriteVal( ret );
			VCR_WriteVal( *pNumEvents );
		}
	}

	return ret;
}


int	VCR_Hook_ReadConsoleInput( void *hInput, void *pRecs, int nMaxRecs, unsigned long *pNumRead )
{
	VCR_Event( VCREvent_ReadConsoleInput );

	char ret;
	if ( g_VCRMode == VCR_Playback )
	{
		VCR_ReadVal( ret );
		if ( ret )
		{
			VCR_ReadVal( *pNumRead );
			VCR_Read( pRecs, *pNumRead * sizeof( INPUT_RECORD ) );
		}
	}
	else
	{
		ret = (char)ReadConsoleInput( (HANDLE)hInput, (INPUT_RECORD*)pRecs, nMaxRecs, pNumRead );

		if ( g_VCRMode == VCR_Record )
		{
			VCR_WriteVal( ret );
			if ( ret )
			{
				VCR_WriteVal( *pNumRead );
				VCR_Write( pRecs, *pNumRead * sizeof( INPUT_RECORD ) );
			}
		}
	}

	return ret;
}


void VCR_Hook_LocalTime( struct tm *today )
{
	// We just provide a wrapper on this function so we can protect access to time() everywhere.
	time_t ltime;
	time( &ltime );
	tm *pTime = localtime( &ltime );
	memcpy( today, pTime, sizeof( *today ) );
}


short VCR_Hook_GetKeyState( int nVirtKey )
{
	VCR_Event( VCREvent_GetKeyState );

	short ret;
	if ( g_VCRMode == VCR_Playback )
	{
		VCR_ReadVal( ret );
	}
	else
	{
		ret = ::GetKeyState( nVirtKey );
		if ( g_VCRMode == VCR_Record )
			VCR_WriteVal( ret );
	}

	return ret;
}


// ---------------------------------------------------------------------- //
// The global VCR interface.
// ---------------------------------------------------------------------- //

VCR_t g_VCR =
{
	VCR_Start,
	VCR_End,
	VCR_GetVCRTraceInterface,
	VCR_GetMode,
	VCR_SetEnabled,
	VCR_SyncToken,
	VCR_Hook_Sys_FloatTime,
	VCR_Hook_PeekMessage,
	VCR_Hook_RecordGameMsg,
	VCR_Hook_RecordEndGameMsg,
	VCR_Hook_PlaybackGameMsg,
	VCR_Hook_recvfrom,
	VCR_Hook_GetCursorPos,
	VCR_Hook_ScreenToClient,
	VCR_Hook_Cmd_Exec,
	VCR_Hook_GetCommandLine,
	VCR_Hook_RegOpenKeyEx,
	VCR_Hook_RegSetValueEx,
	VCR_Hook_RegQueryValueEx,
	VCR_Hook_RegCreateKeyEx,
	VCR_Hook_RegCloseKey,
	VCR_Hook_GetNumberOfConsoleInputEvents,
	VCR_Hook_ReadConsoleInput,
	VCR_Hook_LocalTime,
	VCR_Hook_GetKeyState
};

VCR_t *g_pVCR = &g_VCR;


