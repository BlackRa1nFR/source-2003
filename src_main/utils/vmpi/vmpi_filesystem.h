//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VMPI_FILESYSTEM_H
#define VMPI_FILESYSTEM_H
#ifdef _WIN32
#pragma once
#endif



class IBaseFileSystem;
class MessageBuffer;


// cmdlib implements this to share some functions.
class IVMPIFileSystemHelpers
{
public:
	virtual const char*	GetBaseDir() = 0;
	virtual void		COM_FixSlashes( char *pname ) = 0;
	virtual bool		StripLastDir( char *dirName ) = 0;
	virtual void		MakePath( char *dest, int destLen, const char *dirName, const char *fileName ) = 0;
};


// When you hook the file system with VMPI and are a worker, it blocks on file reads
// and uses MPI to communicate with the master to transfer files it needs over.
// It stores the files in a cache for fast access later.
//
// pBaseDir is "basedir" from cmdlib.
//
// NOTE: the file system you pass in here must allow absolute pathnames. To set this up,
// pass "" in to AddSearchPath and SetWritePath.
IBaseFileSystem* VMPI_FileSystem_Init( IVMPIFileSystemHelpers *pHelpers );

// On the master machine, this really should be called before the app shuts down and 
// global destructors are called. If it isn't, it might lock up waiting for a thread to exit.
void VMPI_FileSystem_Term();

// Causes it to error out on any Open() calls.
void VMPI_FileSystem_DisableFileAccess();

// Returns a factory that will hand out BASEFILESYSTEM_INTERFACE_VERSION when asked for it.
CreateInterfaceFn VMPI_FileSystem_GetFactory();

// This function creates a virtual file that workers can then open and read out of.
void VMPI_FileSystem_CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength );


#endif // VMPI_FILESYSTEM_H
