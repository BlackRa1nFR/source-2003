//
// vgui_int.h
//
// BILLY: 10-04-99
// Interface for engine interaction with vgui
//

#ifndef VGUI_INT_H
#define VGUI_INT_H

#ifdef _WIN32
#pragma once
#endif

void	VGui_Startup();
void	VGui_Shutdown();
void	VGui_Paint();
void	VGui_SetMouseCallbacks(void (* pfnGetMouse)(int &x, int &y), void (*pfnSetMouse)(int x, int y));
void	VGui_EngineTakingFocus( void );
void	VGui_LauncherTakingFocus( void );
void	VGui_EnableWindowsMessages( bool bEnable );

// Pass a windows message through to VGUI.
void	VGui_HandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam );


void	VGui_ClearConsole( void );
void	VGui_ShowConsole( void );
void	VGui_HideConsole( void );
int		VGui_IsConsoleVisible( void );
int		VGui_IsGameUIVisible( void );

int		VGui_IsDebugSystemVisible( void );
void	VGui_ShowDebugSystem( void );
void	VGui_HideDebugSystem( void );
void	VGui_DebugSystemKeyPressed( void );
bool	VGui_IsShiftKeyDown( void );

int		VGui_NeedSteamPassword(char *szAccountName, char *szUserName);
void	VGui_NotifyOfServerConnect(const char *game, int IP, int port);
void	VGui_NotifyOfServerDisconnect( void );
void	VGui_HideGameUI( void );
int		VGui_GameUIKeyPressed( void );
void	VGui_ConPrintf( const char *fmt, ...);
void	VGui_ConDPrintf( const char *fmt, ...);
void	VGui_ColorPrintf( Color& clr, const char *fmt, ...);


#endif









