//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( HOST_SAVERESTORE_H )
#define HOST_SAVERESTORE_H
#ifdef _WIN32
#pragma once
#endif

class CSaveRestoreData;

class ISaveRestore
{
public:
	virtual void					Init( void ) = 0;
	virtual void					Shutdown( void ) = 0;
	virtual bool					LoadGame( const char *pName ) = 0;
	virtual bool					LoadGameNoScreen(const char *pName) = 0;
	virtual int						SaveGame( const char *pszSlot, const char *pszComment ) = 0;
	virtual char					*GetSaveDir(void) = 0;
	virtual void					ClearSaveDir( void ) = 0;
	virtual void					RequestClearSaveDir( void ) = 0;
	virtual int						LoadGameState( char const *level, bool createPlayers ) = 0;
	virtual void					LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName ) = 0;
	virtual const char				*FindRecentSave( char *pNameBuf, int nameBufLen ) = 0;
	virtual int						SaveGameSlot( const char *pSaveName, const char *pSaveComment ) = 0;
	virtual CSaveRestoreData			*SaveGameState( void ) = 0;
	virtual int						IsValidSave( void ) = 0;
	virtual void					Finish( CSaveRestoreData *save ) = 0;

	virtual void					RestoreClientState( char const *fileName, bool adjacent ) = 0;
	virtual void					RestoreAdjacenClientState( char const *map ) = 0;

	virtual void					OnFinishedClientRestore() = 0;
};

// Wrapped, so we can pass to launcher interface
bool SaveRestore_LoadGame( const char *pName );
int	SaveRestore_SaveGame( const char *pszSlot, const char *pszComment );

void *SaveAllocMemory( size_t num, size_t size );
void SaveFreeMemory( void *pSaveMem );

extern ISaveRestore *saverestore;

#endif // HOST_SAVERESTORE_H