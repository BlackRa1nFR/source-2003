//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <windows.h>
#include <shellapi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PROTECTED_THINGS_DISABLE


#ifdef ShellExecute
#undef ShellExecute
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#ifdef GetTickCount
#undef GetTickCount
#endif

#include <vgui/VGUI.h>
#include <vgui/ISystem.h>
#include <KeyValues.h>
#include <vgui/IInputInternal.h>
#include <vgui/ISurface.h>
#include <tier0/dbg.h>
#include <tier0/vcrmode.h>

#include "vgui_internal.h"
#include "vgui_key_translation.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

SHORT System_GetKeyState( int virtualKeyCode )
{
//	return ::GetKeyState( virtualKeyCode ); 
	return g_pVCR->Hook_GetKeyState(virtualKeyCode);
}

class CSystem : public ISystem
{
public:
	CSystem();
	~CSystem();

	virtual void RunFrame();

	virtual void	Shutdown();

	virtual long GetTimeMillis();

	// returns the time at the start of the frame
	virtual double GetFrameTime();

	// returns the current time
	virtual double GetCurrentTime();

	virtual void ShellExecute(const char *command, const char *file);

	virtual int GetClipboardTextCount();
	virtual void SetClipboardText(const char *text, int textLen);
	virtual void SetClipboardText(const wchar_t *text, int textLen);
	virtual int GetClipboardText(int offset, char *buf, int bufLen);
	virtual int GetClipboardText(int offset, wchar_t *buf, int bufLen);

	virtual bool SetRegistryString(const char *key, const char *value);
	virtual bool GetRegistryString(const char *key, char *value, int valueLen);
	virtual bool SetRegistryInteger(const char *key, int value);
	virtual bool GetRegistryInteger(const char *key, int &value);

	virtual bool SetWatchForComputerUse(bool state);
	virtual double GetTimeSinceLastUse();
	virtual int GetAvailableDrives(char *buf, int bufLen);

	virtual KeyValues *GetUserConfigFileData(const char *dialogName, int dialogID);
	virtual void SetUserConfigFile(const char *fileName, const char *pathName);
	virtual void SaveUserConfigFile();

	virtual bool CommandLineParamExists(const char *commandName);
	virtual const char *GetFullCommandLine();

	virtual KeyCode KeyCode_VirtualKeyToVGUI( int keyCode );

private:
	// auto-away data
	bool m_bStaticWatchForComputerUse;
	HHOOK m_hStaticKeyboardHook;
	HHOOK m_hStaticMouseHook;
	double m_StaticLastComputerUseTime;
	int m_iStaticMouseOldX, m_iStaticMouseOldY;

	// timer data
	double m_flFrameTime;
	KeyValues *m_pUserConfigData;
	char m_szFileName[MAX_PATH];
	char m_szPathID[MAX_PATH];

};


CSystem g_System;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSystem, ISystem, VGUI_SYSTEM_INTERFACE_VERSION, g_System);

namespace vgui
{

vgui::ISystem *system()
{
	return &g_System;
}

}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSystem::CSystem()
{
	m_bStaticWatchForComputerUse = false;
	m_hStaticKeyboardHook = NULL;
	m_hStaticMouseHook = NULL;
	m_StaticLastComputerUseTime = 0.0;
	m_iStaticMouseOldX = m_iStaticMouseOldY = -1;
	m_flFrameTime = 0.0;
	m_pUserConfigData = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSystem::~CSystem()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSystem::Shutdown()
{
	if ( m_pUserConfigData )
	{
		m_pUserConfigData->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles all the per frame actions
//-----------------------------------------------------------------------------
void CSystem::RunFrame()
{
	// record the current frame time
	m_flFrameTime = GetCurrentTime();

	if (m_bStaticWatchForComputerUse)
	{
		// check for mouse movement
		int x, y;
		input()->GetCursorPos(x, y);
		// allow a little slack for jittery mice, don't reset until it's moved more than fifty pixels
		if (abs((x + y) - (m_iStaticMouseOldX + m_iStaticMouseOldY)) > 50)
		{
			m_StaticLastComputerUseTime = GetTimeMillis() / 1000.0;
			m_iStaticMouseOldX = x;
			m_iStaticMouseOldY = y;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the time at the start of the frame
//-----------------------------------------------------------------------------
double CSystem::GetFrameTime()
{
	return m_flFrameTime;
}

//-----------------------------------------------------------------------------
// Purpose: returns the current time
//-----------------------------------------------------------------------------
double CSystem::GetCurrentTime()
{
	return Plat_FloatTime();
}


//-----------------------------------------------------------------------------
// Purpose: returns the current time in milliseconds
//-----------------------------------------------------------------------------
long CSystem::GetTimeMillis()
{
	return (long)(GetCurrentTime() * 1000);
}

void CSystem::ShellExecute(const char *command, const char *file)
{
	::ShellExecuteA(NULL, command, file, NULL, NULL, SW_SHOWNORMAL);
}

void CSystem::SetClipboardText(const char *text, int textLen)
{
	if (!text)
		return;

	if (textLen <= 0)
		return;

	if (!OpenClipboard(NULL))
		return;

	EmptyClipboard();

	HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, textLen + 1);
	if (hmem)
	{
		void *ptr = GlobalLock(hmem);
		if (ptr != null)
		{
			memset(ptr, 0, textLen + 1);
			memcpy(ptr, text, textLen);
			GlobalUnlock(hmem);

			SetClipboardData(CF_TEXT, hmem);
		}
	}
	
	CloseClipboard();
}

//-----------------------------------------------------------------------------
// Purpose: Puts unicode text into the clipboard
//-----------------------------------------------------------------------------
void CSystem::SetClipboardText(const wchar_t *text, int textLen)
{
	if (!text)
		return;

	if (textLen <= 0)
		return;

	if (!OpenClipboard(NULL))
		return;

	EmptyClipboard();

	HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (textLen + 1) * sizeof(wchar_t));
	if (hmem)
	{
		void *ptr = GlobalLock(hmem);
		if (ptr != null)
		{
			memset(ptr, 0, (textLen + 1) * sizeof(wchar_t));
			memcpy(ptr, text, textLen * sizeof(wchar_t));
			GlobalUnlock(hmem);

			SetClipboardData(CF_UNICODETEXT, hmem);
		}
	}
	
	CloseClipboard();
}

int CSystem::GetClipboardTextCount()
{
	int count = 0;
	if (!OpenClipboard(NULL))
		return 0;
	
	HANDLE hmem = GetClipboardData(CF_TEXT);
	if (hmem)
	{
		count = GlobalSize(hmem);
	}

	CloseClipboard();
	return count;
}

int CSystem::GetClipboardText(int offset, char *buf, int bufLen)
{
	if (!buf)
		return 0;

	if (bufLen <= 0)
		return 0;

	int count = 0;
	if (!OpenClipboard(NULL))
		return 0;
	
	HANDLE hmem = GetClipboardData(CF_UNICODETEXT);
	if (hmem)
	{
		int len = GlobalSize(hmem);
		count = len - offset;
		if (count <= 0)
		{
			count = 0;
		}
		else
		{
			if (bufLen < count)
			{
				count = bufLen;
			}
			void *ptr = GlobalLock(hmem);
			if (ptr)
			{
				memcpy(buf, ((char *)ptr) + offset, count);
				GlobalUnlock(hmem);
			}
		}
	}

	CloseClipboard();
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves unicode text from the clipboard
//-----------------------------------------------------------------------------
int CSystem::GetClipboardText(int offset, wchar_t *buf, int bufLen)
{
	if (!buf)
		return 0;

	if (bufLen <= 0)
		return 0;

	int count = 0;
	if (!OpenClipboard(NULL))
		return 0;
	
	HANDLE hmem = GetClipboardData(CF_UNICODETEXT);
	if (hmem)
	{
		int len = GlobalSize(hmem);
		count = len - offset;
		if (count <= 0)
		{
			count = 0;
		}
		else
		{
			if (bufLen < count)
			{
				count = bufLen;
			}
			void *ptr = GlobalLock(hmem);
			if (ptr)
			{
				memcpy(buf, ((wchar_t *)ptr) + offset, count);
				GlobalUnlock(hmem);
			}
		}
	}

	CloseClipboard();
	return count / sizeof(wchar_t);
}

static bool staticSplitRegistryKey(const char *key, char *key0, int key0Len, char *key1, int key1Len)
{
	if(key==null)
	{
		return false;
	}
	
	int len=strlen(key);
	if(len<=0)
	{
		return false;
	}

	int state=0;
	int Start=-1;
	for(int i=len-1;i>=0;i--)
	{
		if(key[i]=='\\')
		{
			break;
		}
		else
		{
			Start=i;
		}
	}

	if(Start==-1)
	{
		return false;
	}
	
	vgui_strcpy(key0,Start+1,key);
	vgui_strcpy(key1,(len-Start)+1,key+Start);

	return true;
}

bool CSystem::SetRegistryString(const char *key, const char *value)
{
	HKEY hKey;

	HKEY hSlot = HKEY_CURRENT_USER;
	if (!strncmp(key, "HKEY_LOCAL_MACHINE", 18))
	{
		hSlot = HKEY_LOCAL_MACHINE;
		key += 19;
	}
	else if (!strncmp(key, "HKEY_CURRENT_USER", 17))
	{
		hSlot = HKEY_CURRENT_USER;
		key += 18;
	}

	char key0[256],key1[256];
	if(!staticSplitRegistryKey(key,key0,256,key1,256))
	{
		return false;
	}

	if(g_pVCR->Hook_RegCreateKeyEx(hSlot,key0,null,null,REG_OPTION_NON_VOLATILE,KEY_WRITE,null,&hKey,null)!=ERROR_SUCCESS)
	{
		return false;
	}
		
	if(g_pVCR->Hook_RegSetValueEx(hKey,key1,null,REG_SZ,(uchar*)value,strlen(value)+1)==ERROR_SUCCESS)
	{
		g_pVCR->Hook_RegCloseKey(hKey);
		return true;
	}

	g_pVCR->Hook_RegCloseKey(hKey);

	return false;
}

bool CSystem::GetRegistryString(const char *key, char *value, int valueLen)
{
	HKEY hKey;

	HKEY hSlot = HKEY_CURRENT_USER;
	if (!strncmp(key, "HKEY_LOCAL_MACHINE", 18))
	{
		hSlot = HKEY_LOCAL_MACHINE;
		key += 19;
	}
	else if (!strncmp(key, "HKEY_CURRENT_USER", 17))
	{
		hSlot = HKEY_CURRENT_USER;
		key += 18;
	}

	char key0[256],key1[256];
	if(!staticSplitRegistryKey(key,key0,256,key1,256))
	{
		return false;
	}

	if(g_pVCR->Hook_RegOpenKeyEx(hSlot,key0,null,KEY_READ,&hKey)!=ERROR_SUCCESS)
	{
		return false;
	}

	ulong len=valueLen;
	if(g_pVCR->Hook_RegQueryValueEx(hKey,key1,null,null,(uchar*)value,&len)==ERROR_SUCCESS)
	{		
		g_pVCR->Hook_RegCloseKey(hKey);
		return true;
	}

	g_pVCR->Hook_RegCloseKey(hKey);
	return false;
}

bool CSystem::SetRegistryInteger(const char *key, int value)
{
	HKEY hKey;
	HKEY hSlot = HKEY_CURRENT_USER;
	if (!strncmp(key, "HKEY_LOCAL_MACHINE", 18))
	{
		hSlot = HKEY_LOCAL_MACHINE;
		key += 19;
	}
	else if (!strncmp(key, "HKEY_CURRENT_USER", 17))
	{
		hSlot = HKEY_CURRENT_USER;
		key += 18;
	}

	char key0[256],key1[256];
	if(!staticSplitRegistryKey(key,key0,256,key1,256))
	{
		return false;
	}

	if(g_pVCR->Hook_RegCreateKeyEx(hSlot,key0,null,null,REG_OPTION_NON_VOLATILE,KEY_WRITE,null,&hKey,null)!=ERROR_SUCCESS)
	{
		return false;
	}
		
	if(g_pVCR->Hook_RegSetValueEx(hKey,key1,null,REG_DWORD,(uchar*)&value,4)==ERROR_SUCCESS)
	{
		g_pVCR->Hook_RegCloseKey(hKey);
		return true;
	}

	g_pVCR->Hook_RegCloseKey(hKey);
	return false;
}

bool CSystem::GetRegistryInteger(const char *key, int &value)
{
	HKEY hKey;
	HKEY hSlot = HKEY_CURRENT_USER;
	if (!strncmp(key, "HKEY_LOCAL_MACHINE", 18))
	{
		hSlot = HKEY_LOCAL_MACHINE;
		key += 19;
	}
	else if (!strncmp(key, "HKEY_CURRENT_USER", 17))
	{
		hSlot = HKEY_CURRENT_USER;
		key += 18;
	}

	char key0[256],key1[256];
	if(!staticSplitRegistryKey(key,key0,256,key1,256))
	{
		return false;
	}

	if(g_pVCR->Hook_RegOpenKeyEx(hSlot,key0,null,KEY_READ,&hKey)!=ERROR_SUCCESS)
	{
		return false;
	}

	ulong len=4;
	if(g_pVCR->Hook_RegQueryValueEx(hKey,key1,null,null,(uchar*)&value,&len)==ERROR_SUCCESS)
	{		
		g_pVCR->Hook_RegCloseKey(hKey);
		return true;
	}

	g_pVCR->Hook_RegCloseKey(hKey);
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: sets whether or not the app watches for global computer use
// Input  : state - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSystem::SetWatchForComputerUse(bool state)
{
	if (state == m_bStaticWatchForComputerUse)
		return true;

	m_bStaticWatchForComputerUse = state;

	if (m_bStaticWatchForComputerUse)
	{
		// enable watching
	}
	else
	{
		// disable watching
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns the time, in seconds, since the last computer use.
// Output : double
//-----------------------------------------------------------------------------
double CSystem::GetTimeSinceLastUse()
{
	if (m_bStaticWatchForComputerUse)
	{
		return (GetTimeMillis() / 1000.0) - m_StaticLastComputerUseTime;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Get the drives a user has available on thier system
//-----------------------------------------------------------------------------
int CSystem::GetAvailableDrives(char *buf, int bufLen)
{
	return GetLogicalDriveStrings(bufLen, buf);
}

//-----------------------------------------------------------------------------
// Purpose: user config
//-----------------------------------------------------------------------------
KeyValues *CSystem::GetUserConfigFileData(const char *dialogName, int dialogID)
{
	if (!m_pUserConfigData)
		return NULL;

	Assert(dialogName && *dialogName);

	if (dialogID)
	{
		char buf[256];
		Q_snprintf(buf, sizeof(buf) - 1, "%s_%d", dialogName, dialogID);
		dialogName = buf;
	}

	return m_pUserConfigData->FindKey(dialogName, true);
}

//-----------------------------------------------------------------------------
// Purpose: sets the name of the config file to save/restore from.  Settings are loaded immediately.
//-----------------------------------------------------------------------------
void CSystem::SetUserConfigFile(const char *fileName, const char *pathName)
{
	if (!m_pUserConfigData)
	{
		m_pUserConfigData = new KeyValues("UserConfigData");
	}

	strncpy(m_szFileName, fileName, sizeof(m_szFileName) - 1);
	strncpy(m_szPathID, pathName, sizeof(m_szPathID) - 1);
	
	

	// open
	m_pUserConfigData->UsesEscapeSequences( true ); // VGUI may use this
	m_pUserConfigData->LoadFromFile((IBaseFileSystem*)filesystem(), m_szFileName, m_szPathID);
}

//-----------------------------------------------------------------------------
// Purpose: saves all the current settings to the user config file
//-----------------------------------------------------------------------------
void CSystem::SaveUserConfigFile()
{
	if (m_pUserConfigData)
	{
		m_pUserConfigData->SaveToFile((IBaseFileSystem*)filesystem(), m_szFileName, m_szPathID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns whether or not the parameter was on the command line
//-----------------------------------------------------------------------------
bool CSystem::CommandLineParamExists(const char *paramName)
{
	const char *cmdLine = ::GetCommandLine();
	const char *loc = strstr(cmdLine, paramName);
	return (loc != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: returns the name of the currently running exe
//-----------------------------------------------------------------------------
const char *CSystem::GetFullCommandLine()
{
	return ::GetCommandLine();
}


KeyCode CSystem::KeyCode_VirtualKeyToVGUI( int keyCode )
{
	return ::KeyCode_VirtualKeyToVGUI( keyCode );
}
