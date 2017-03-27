//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
// common.h  -- general definitions -- shared by client and server portions of host
//=============================================================================

#ifndef COMMON_H
#define COMMON_H
#pragma once

#ifndef WORLDSIZE_H
#include "worldsize.h"
#endif

#include "basetypes.h"
#include "filesystem.h"

#include "vector.h" // @Note (toml 05-01-02): solely for definition of QAngle

#include "qlimits.h"

#ifndef _INC_STDIO
#include <stdio.h>
#endif

#ifndef HLDEMO_BUILD   // Remember to bump these version on rebuilding the engine.
#include "../common/version.h"
#else
#define VERSION_STRING "2.0.0.1"
#endif

extern char *gpszAppName;
extern char gpszVersionString[32];

class Vector;
struct cache_user_t;

//============================================================================
// ZOID: portability crap
#ifndef _WIN32

typedef unsigned char BYTE;
typedef signed long LONG;
typedef unsigned long ULONG;

typedef char * LPSTR;

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;

/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

typedef struct _GUID
{
	unsigned long Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
} GUID;

typedef GUID UUID;

#endif

//============================================================================
//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);
void SZ_Print (sizebuf_t *buf, const char *data);	// strcats onto the sizebuf

//============================================================================

#define COM_COPY_CHUNK_SIZE 1024   // For copying operations

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================
class bf_write;

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteWord (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteBuf (sizebuf_t *sb, int iSize, void *buf);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteHiresAngle (bf_write *sb, float f);

class bf_write;
bf_write *WriteDest_Parm (int dest);

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

bool MSG_IsOverflowed(); // Used to be msg_badread.
void MSG_BeginReading (void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadWord (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
char *MSG_ReadString (void);
char *MSG_ReadStringLine (void);
int MSG_ReadBuf(int iSize, void *pbuf);

float MSG_ReadAngle (void);
float MSG_ReadHiresAngle (void);

// Streaming bitwise IO
// The MSG_ bit functions just use these.
class bf_read;
class bf_write;
bf_read*  MSG_GetReadBuf();
bf_write* MSG_GetWriteBuf();

void MSG_StartBitWriting( sizebuf_t *buf );
void MSG_EndBitWriting ( sizebuf_t *buf );
void MSG_StartBitReading( sizebuf_t *buf );
void MSG_EndBitReading ( sizebuf_t *buf );
int MSG_ReadOneBit( void );

// Output
void MSG_WriteBitByte( unsigned char *data, int numbits );
void MSG_WriteBitShort( unsigned short *data, int numbits );
void MSG_WriteBitLong( unsigned int *data, int numbits );
void MSG_WriteSBitByte( char *data, int numbits );
void MSG_WriteSBitShort( short *data, int numbits );
void MSG_WriteSBitLong( int *data, int numbits );
void MSG_WriteBitAngle( float fAngle, int numbits );

// Basic Coordinate Routines (these contain bit-field size AND fixed point scaling constants)
float MSG_ReadBitCoord (void);
void MSG_ReadBitVec3Coord (Vector& fa);
void MSG_ReadBitVec3Normal( Vector& fa );
void MSG_ReadBitAngles( QAngle& fa );
void MSG_WriteBitCoord (const float f);
void MSG_WriteBitVec3Coord (const Vector& fa);
void MSG_WriteBitVec3Normal( Vector& fa );
void MSG_WriteBitAngles( QAngle& fa );

// NOTE: the sizebuf parameter is only for b/w compatibility. It used to switch in and out
// of bit mode if you weren't already in it, but now you're always in bit mode so it 
// ignores the parameter.
void MSG_WriteCoord (sizebuf_t *sb, const float f);
void MSG_WriteVec3Coord (sizebuf_t *sb, const Vector& fa);

// Input
unsigned char MSG_PeekBitByte( int numbits );
unsigned char MSG_ReadBitByte( int numbits );
unsigned short MSG_ReadBitShort( int numbits );
unsigned int MSG_PeekBitLong( int numbits );
unsigned int MSG_ReadBitLong( int numbits );
char MSG_ReadSBitByte( int numbits );
short MSG_ReadSBitShort( int numbits );
int MSG_ReadSBitLong( int numbits );
float MSG_ReadBitAngle( int numbits );

#include "vstdlib/strtools.h"


//============================================================================

unsigned char COM_Nibble( char c );
void COM_HexConvert( const char *pszInput, int nInputLength, unsigned char *pOutput );
char *COM_BinPrintf( unsigned char *buf, int nLen );

//============================================================================

extern	char		com_token[1024];
extern	qboolean	com_eof;

extern void COM_FileBase (const char *in, char *out);
extern void COM_WriteFile (char *filename, void *data, int len);
extern int COM_OpenFile( const char *filename, FileHandle_t* file );
extern void COM_CloseFile( FileHandle_t hFile );
extern void COM_CreatePath (char *path);
extern int COM_FileSize (const char *filename);
extern int COM_ExpandFilename (char *filename);
extern byte *COM_LoadFile (const char *path, int usehunk, int *pLength);

extern char *va(char *format, ...);

extern qboolean NET_StringToSockaddr(const char *s, struct sockaddr *sadr);

typedef struct	qpic_s qpic_t;

char *COM_Parse (char *data);
int COM_TokenWaiting( char *buffer );

extern qboolean com_ignorecolons;

void COM_Init (void);
void COM_Shutdown( void );
void COM_ParseDirectoryFromCmd( const char *pCmdName, char *pDirName, const char *pDefault );
void COM_StripTrailingSlash( char *ppath );

char *COM_SkipPath (char *pathname);
void COM_StripExtension ( const char *in, char *out, int outLen );
void COM_DefaultExtension (char *path, const char *extension, int pathStringLength);

// does a varargs printf into a temp buffer
char	*va(char *format, ...);
// prints a vector into a temp buffer.
char    *vstr(Vector& v);


//============================================================================

extern int com_filesize;

extern	char	com_gamedir[MAX_OSPATH];
extern	char	com_defaultgamedir[MAX_OSPATH];


byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize);
byte *COM_LoadTempFile (const char *path, int *pLength);
byte *COM_LoadHunkFile (const char *path);
void COM_LoadCacheFile (const char *path, cache_user_t *cu);
byte *COM_LoadFileForMe(const char *filename, int *pLength);
void COM_FreeFile(byte *buffer);
byte* COM_LoadFile(const char *path, int usehunk, int *pLength);

// Returns true if the file exists.
bool COM_DoesFileExist(const char *path);

void COM_CopyFileChunk( FileHandle_t dst, FileHandle_t src, int nSize );
void COM_FixSlashes( char *pname );

void COM_ChangeGameDir( const char *pszGameDir );
void COM_SetupLogDir( const char *mapname );
void COM_GetGameDir(char *szGameDir);
byte *COM_LoadFileForMe(const char *filename, int *pLength);
void COM_FreeFile (byte *buffer);
int COM_CompareFileTime(char *filename1, char *filename2, int *iCompare);
int COM_GetFileTime( const char *pFileName );
char *COM_ParseFile(char *data, char *token);

extern char gszDisconnectReason[256];
extern qboolean gfExtendedError;
void COM_ExplainDisconnection( qboolean bPrint, char *fmt, ... );

extern qboolean gfUseLANAuthentication;
void COM_CheckAuthenticationType( void );

extern	struct cvar_s	registered;

extern qboolean		standard_quake, rogue, hipnotic;

// Additional shared functions
int COM_EntsForPlayerSlots(int nPlayers);
char *COM_GetShortName(char *name);

void COM_Log( char *pszFile, char *fmt, ...); // Log a debug message to specified file ( if pszFile == NULL uses c:\\hllog.txt )

// Calls OutputDebugString but it's good to use this so you don't have to include
// the windows header.
// DebugOutStraight doesn't do printf formatting and DebugOut does.
extern void DebugOutStraight(const char *pStr);
extern void DebugOut(const char *pStr, ...);

// For obfuscation purposes only...sigh
void COM_Munge( unsigned char *data, int len, int seq );
void COM_UnMunge( unsigned char *data, int len, int seq );


#endif // COMMON_H
