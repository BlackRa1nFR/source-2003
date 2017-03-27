//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Expose things from GameInterface.cpp. Mostly the engine interfaces.
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEINTERFACE_H
#define GAMEINTERFACE_H

#ifdef _WIN32
#pragma once
#endif

class IFileSystem;				// include FileSystem.h
class IUniformRandomStream;		// include vstdlib/random.h
class IEngineSound;				// include engine/IEngineSound.h
class IVEngineServer;			
class IVoiceServer;
class IStaticPropMgrServer;
class ISpatialPartition;
class IVModelInfo;
class IEngineTrace;
class IGameEventManager;

extern IVEngineServer			*engine;
extern IVoiceServer				*g_pVoiceServer;
extern IFileSystem				*filesystem;
extern IStaticPropMgrServer		*staticpropmgr;
extern ISpatialPartition		*partition;
extern IEngineSound				*enginesound;
extern IUniformRandomStream		*random;
extern IVModelInfo				*modelinfo;
extern IEngineTrace				*enginetrace;
extern IGameEventManager		*gameeventmanager;

// HACKHACK: Builds a global list of entities that were restored from all levels
void AddRestoredEntity( CBaseEntity *pEntity );

//-----------------------------------------------------------------------------
// Precaches a material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName );


class IRecipientFilter;
void MessageBegin( IRecipientFilter& filter, int msg_type = 0 );
void UserMessageBegin( IRecipientFilter& filter, const char *messagename );
void MessageEnd( void );

// bytewise
void MessageWriteByte( int iValue);
void MessageWriteChar( int iValue);
void MessageWriteShort( int iValue);
void MessageWriteWord( int iValue );
void MessageWriteLong( int iValue);
void MessageWriteFloat( float flValue);
void MessageWriteAngle( float flValue);
void MessageWriteCoord( float flValue);
void MessageWriteVec3Coord( const Vector& rgflValue);
void MessageWriteVec3Normal( const Vector& rgflValue);
void MessageWriteAngles( const QAngle& rgflValue);
void MessageWriteString( const char *sz );
void MessageWriteEntity( int iValue);

// bitwise
void MessageWriteBool( bool bValue );
void MessageWriteUBitLong( unsigned int data, int numbits );
void MessageWriteSBitLong( int data, int numbits );
void MessageWriteBits( const void *pIn, int nBits );

#endif // GAMEINTERFACE_H

