//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines an interface to the map editor for the execution of
//			editor shell commands from another application. Commands allow the
//			creation and deletion of entities, AI nodes, and AI node connections.
//
// $NoKeywords: $
//=============================================================================

#ifndef EDITOR_SENDCOMMAND_H
#define EDITOR_SENDCOMMAND_H
#pragma once


//
// Result codes from Worldcraft_SendCommand.
//
enum EditorSendResult_t
{
	Editor_OK = 0,			// Success.
	Editor_NotRunning,		// Unable to establish a communications channel with the editor.
	Editor_BadCommand,		// The editor did not accept the command.
};


//
// Wrappers around specific commands for convenience.
//
EditorSendResult_t Editor_BeginSession(const char *pszMapName, int nMapVersion, bool bShowUI = false);
EditorSendResult_t Editor_EndSession(bool bShowUI);
EditorSendResult_t Editor_CheckVersion(const char *pszMapName, int nMapVersion, bool bShowUI = false);

EditorSendResult_t Editor_CreateEntity(const char *pszEntity, float x, float y, float z, bool bShowUI = false);
EditorSendResult_t Editor_DeleteEntity(const char *pszEntity, float x, float y, float z, bool bShowUI = false);

EditorSendResult_t Editor_CreateNode(const char *pszNodeClass, int nID, float x, float y,  float z, bool bShowUI = false);
EditorSendResult_t Editor_DeleteNode(int nID, bool bShowUI = false);

EditorSendResult_t Editor_CreateNodeLink(int nStartID, int nEndID, bool bShowUI = false);
EditorSendResult_t Editor_DeleteNodeLink(int nStartID, int nEndID, bool bShowUI = false);


//
// Actually does the work. All the above commands route through this.
//
EditorSendResult_t Editor_SendCommand(const char *pszCommand, bool bShowUI);


#endif // EDITOR_SENDCOMMAND_H
