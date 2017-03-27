//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: VCR mode records a client's game and allows you to 
//			play it back and reproduce it exactly. When playing it back, nothing
//			is simulated on the server, but all server packets are recorded.
//
//			Most of the VCR mode functionality is accomplished through hooks
//			called at various points in the engine.
//
// $NoKeywords: $
//=============================================================================

#ifndef VCRMODE_H
#define VCRMODE_H
#ifdef _WIN32
#pragma once
#endif


#include "tier0/platform.h"
#include "tier0/vcr_shared.h"

#ifdef _LINUX
void BuildCmdLine( int argc, char **argv );
static char *GetCommandLine();
#endif

// Enclose lines of code in this if you don't want anything in them written to or read from the VCR file.
#define NOVCR(x)	\
{\
	g_pVCR->SetEnabled(0);\
	x;\
	g_pVCR->SetEnabled(1);\
}


// ---------------------------------------------------------------------- //
// Definitions.
// ---------------------------------------------------------------------- //
typedef enum
{
	VCR_Disabled=0,
	VCR_Record,
	VCR_Playback
} VCRMode;



// ---------------------------------------------------------------------- //
// Functions.
// ---------------------------------------------------------------------- //

class IVCRHelpers
{
public:
	virtual void	ErrorMessage( const char *pMsg ) = 0;
	virtual void*	GetMainWindow() = 0;
};


// Used by the vcrtrace program.
class IVCRTrace
{
public:
	virtual VCREvent	ReadEvent() = 0;
	virtual void		Read( void *pDest, int size ) = 0;
};


typedef struct VCR_s
{
	// Start VCR record or play.
	int			(*Start)( char const *pFilename, bool bRecord, IVCRHelpers *pHelpers );
	void		(*End)();

	// Used by the VCR trace app.
	IVCRTrace*	(*GetVCRTraceInterface)();

	// Get the current mode the VCR is in.
	VCRMode		(*GetMode)();

	// This can be used to block out areas of code that are unpredictable (like things triggered by WM_TIMER messages).
	void		(*SetEnabled)(int bEnabled);

	// This can be called any time to put in a debug check to make sure things are synchronized.
	void		(*SyncToken)(char const *pToken);

	// Hook for Sys_FloatTime().
	double		(*Hook_Sys_FloatTime)(double time);

	// Note: this makes no guarantees about msg.hwnd being the same on playback. If it needs to be, then we need to add
	// an ID system for Windows and store the ID like in Goldsrc.
	int			(*Hook_PeekMessage)(
		struct tagMSG *msg, 
		void *hWnd, 
		unsigned int wMsgFilterMin, 
		unsigned int wMsgFilterMax, 
		unsigned int wRemoveMsg
		);

	// Call this to record game messages.
	void		(*Hook_RecordGameMsg)( unsigned int uMsg, unsigned int wParam, long lParam );
	void		(*Hook_RecordEndGameMsg)();
	
	// Call this to playback game messages until it returns false.
	bool		(*Hook_PlaybackGameMsg)( unsigned int &uMsg, unsigned int &wParam, long &lParam );

	// Hook for recvfrom() calls. This replaces the recvfrom() call.
	int			(*Hook_recvfrom)(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);

	void		(*Hook_GetCursorPos)(struct tagPOINT *pt);
	void		(*Hook_ScreenToClient)(void *hWnd, struct tagPOINT *pt);

	void		(*Hook_Cmd_Exec)(char **f);

	char*		(*Hook_GetCommandLine)();

	// Registry hooks.
	long		(*Hook_RegOpenKeyEx)( void *hKey, const char *lpSubKey, unsigned long ulOptions, unsigned long samDesired, void *pHKey );
	long		(*Hook_RegSetValueEx)(void *hKey, char const *lpValueName, unsigned long Reserved, unsigned long dwType, unsigned char const *lpData, unsigned long cbData);
	long		(*Hook_RegQueryValueEx)(void *hKey, char const *lpValueName, unsigned long *lpReserved, unsigned long *lpType, unsigned char *lpData, unsigned long *lpcbData);
	long		(*Hook_RegCreateKeyEx)(void *hKey, char const *lpSubKey, unsigned long Reserved, char *lpClass, unsigned long dwOptions, unsigned long samDesired, void *lpSecurityAttributes, void *phkResult, unsigned long *lpdwDisposition);
	void		(*Hook_RegCloseKey)(void *hKey);

	// hInput is a HANDLE.
	int			(*Hook_GetNumberOfConsoleInputEvents)( void *hInput, unsigned long *pNumEvents );

	// hInput is a HANDLE.
	// pRecs is an INPUT_RECORD pointer.
	int			(*Hook_ReadConsoleInput)( void *hInput, void *pRecs, int nMaxRecs, unsigned long *pNumRead );

	
	// This calls time() then gives you localtime()'s result.
	void		(*Hook_LocalTime)( struct tm *today );

	short		(*Hook_GetKeyState)( int nVirtKey );
} VCR_t;


// In the launcher, this is created by vcrmode.c. 
// In the engine, this is set when the launcher initializes its DLL.
PLATFORM_INTERFACE VCR_t *g_pVCR;


#endif // VCRMODE_H
