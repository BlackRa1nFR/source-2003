#ifndef SYS_H
#define SYS_H
#pragma once

#ifndef SYSEXTERNAL_H
#include "sysexternal.h"
#endif

// sys.h -- non-portable functions

//
// system IO
//
void Sys_mkdir (const char *path);
int Sys_CompareFileTime(long ft1, long ft2);
char const* Sys_FindFirst (const char *path, char *basename);
char const* Sys_FindNext (char *basename);
void Sys_FindClose (void);



void Sys_ShutdownMemory( void );
void Sys_InitMemory( void );

void Sys_Sleep ( int msec );
void Sys_GetRegKeyValue( char *pszSubKey, char *pszElement, char *pszReturnString, int nReturnLength, char *pszDefaultValue);

extern "C" void Sys_SetFPCW (void);
extern "C" void Sys_TruncateFPU( void );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct FileAssociationInfo
{
	char const  *extension;
	char const  *command_to_issue;
};

void Sys_CreateFileAssociations( int count, FileAssociationInfo *list );

#endif			// SYS_H
