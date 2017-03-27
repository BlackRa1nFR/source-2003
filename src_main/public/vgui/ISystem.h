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

#ifndef ISYSTEM_H
#define ISYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include <vgui/VGUI.h>
#include <vgui/keycode.h>

#ifdef PlaySound
#undef PlaySound
#endif

class KeyValues;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Wraps contextless windows system functions
//-----------------------------------------------------------------------------
class ISystem : public IBaseInterface
{
public:
	// called every frame
	virtual void RunFrame() = 0;

	virtual void	Shutdown() = 0;

	// use this with the "open" command to launch web browsers/explorer windows, eg. ShellExecute("open", "www.valvesoftware.com")
	virtual void ShellExecute(const char *command, const char *file) = 0;

	// returns the time at the start of the frame, in seconds
	virtual double GetFrameTime() = 0;

	// returns the current time, in seconds
	virtual double GetCurrentTime() = 0;

	// returns the current time, in milliseconds
	virtual long GetTimeMillis() = 0;

	// clipboard access
	virtual int GetClipboardTextCount() = 0;
	virtual void SetClipboardText(const char *text, int textLen) = 0;
	virtual void SetClipboardText(const wchar_t *text, int textLen) = 0;
	virtual int GetClipboardText(int offset, char *buf, int bufLen) = 0;
	virtual int GetClipboardText(int offset, wchar_t *buf, int bufLen) = 0;

	// windows registry
	virtual bool SetRegistryString(const char *key, const char *value) = 0;
	virtual bool GetRegistryString(const char *key, char *value, int valueLen) = 0;
	virtual bool SetRegistryInteger(const char *key, int value) = 0;
	virtual bool GetRegistryInteger(const char *key, int &value) = 0;

	// user config
	virtual KeyValues *GetUserConfigFileData(const char *dialogName, int dialogID) = 0;
	// sets the name of the config file to save/restore from.  Settings are loaded immediately.
	virtual void SetUserConfigFile(const char *fileName, const char *pathName) = 0;
	// saves all the current settings to the user config file
	virtual void SaveUserConfigFile() = 0;

	// sets the watch on global computer use
	// returns true if supported
	virtual bool SetWatchForComputerUse(bool state) = 0;
	// returns the time, in seconds, since the last computer use.
	virtual double GetTimeSinceLastUse() = 0;

	// Get a string containing the available drives
	// If the function succeeds, the return value is the length, in characters, 
	// of the strings copied to the buffer, 
	// not including the terminating null character.
	virtual int GetAvailableDrives(char *buf, int bufLen) = 0;

	// exe command line options accessors
	// returns whether or not the parameter was on the command line
	virtual bool CommandLineParamExists(const char *paramName) = 0;

	// returns the full command line, including the exe name
	virtual const char *GetFullCommandLine() = 0;

	// Convert a windows virtual key code to a VGUI key code.
	virtual KeyCode KeyCode_VirtualKeyToVGUI( int keyCode ) = 0;
};

}

#define VGUI_SYSTEM_INTERFACE_VERSION "VGUI_System009"


#endif // ISYSTEM_H
