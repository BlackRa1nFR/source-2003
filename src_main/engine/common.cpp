//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// common.c -- misc functions used in client and server
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "winquake.h"
#include "glquake.h"
#include <ctype.h>
#include "draw.h"
#include "zone.h"
#include "sys.h"
#include "edict.h"
#include "coordsize.h"
#include "characterset.h"
#include "gl_texture.h"
#include "bitbuf.h"
#include "common.h"
#include <malloc.h>
#include "traceinit.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "convar.h"
//#include "VGui_int.h"
#include "gl_matsysiface.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "vstdlib/ICommandLine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Things used by C files.
float V_CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);
int COM_CompareFileTime(char *filename1, char *filename2, int *iCompare);


// Things in other C files.
#define NUM_SAFE_ARGVS  7

#define MAX_LOG_DIRECTORIES	10000

char *gpszAppName = "Half-Life 2";

char gpszVersionString[32] = "1.0"; 

qboolean com_ignorecolons = false;  // YWB:  Ignore colons as token separators in COM_Parse

// wordbreak parsing set
static characterset_t	g_BreakSet, g_BreakSetIncludingColons;

#define COM_TOKEN_MAX_LENGTH 1024
char	com_token[COM_TOKEN_MAX_LENGTH];

char	g_TempStringBuf[2048];

/*
All of Quake's data access is through a hierarchical file system, but the contents of 
the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in engineparms->basedir.
This can be overridden with the "-basedir" command line parm to allow code
debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory
that all generated files (savegames, screenshots, demos, config files) will
be saved to.  This can be overridden with the "-game" command line parameter.
The game directory can never be changed while quake is executing.
This is a precacution against having a malicious server instruct clients
to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth,
especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.

FIXME:
The file "parms.txt" will be read out of the game directory and appended to the
current command line arguments to allow different games to initialize startup
parms differently.  This could be used to add a "-sspeed 22050" for the high
quality sound edition.  Because they are added at the end, they will not override
an explicit setting on the original command line.
*/


/*
==============================
Q_FileNameCmp

this is a specific string compare to use for filenames.  
 it treats all slashes as the same character. so you can compare
paths.  (Quake uses / internally, while NT uses \ for it's path-seperator thing)
NOTE: this function uses the same return value conventions as strcmp.
so 0 signals a match.
==============================
*/
int Q_FileNameCmp(const char* file1,const char* file2)
{
	do
	{
		if (*file1 == '/' && *file2=='\\')
			continue;
		if (*file2 == '/' && *file1=='\\')
			continue;
		if (tolower(*file1) != tolower(*file2))
			return -1;			
		if (!*file1)
			return 0;
	} while (*file1++ && *file2++);
	return 0; // fixed bogus warning
}



/*
==================
COM_Nibble

Returns the 4 bit nibble for a hex character
==================
*/
unsigned char COM_Nibble( char c )
{
	if ( ( c >= '0' ) &&
		 ( c <= '9' ) )
	{
		 return (unsigned char)(c - '0');
	}

	if ( ( c >= 'A' ) &&
		 ( c <= 'F' ) )
	{
		 return (unsigned char)(c - 'A' + 0x0a);
	}

	if ( ( c >= 'a' ) &&
		 ( c <= 'f' ) )
	{
		 return (unsigned char)(c - 'a' + 0x0a);
	}

	return '0';
}

/*
==================
COM_HexConvert

Converts pszInput Hex string to nInputLength/2 binary
==================
*/
void COM_HexConvert( const char *pszInput, int nInputLength, unsigned char *pOutput )
{
	unsigned char *p;
	int i;
	const char *pIn;

	p = pOutput;
	for (i = 0; i < nInputLength; i+=2 )
	{
		pIn = &pszInput[i];

		*p = COM_Nibble( pIn[0] ) << 4 | COM_Nibble( pIn[1] );		

		p++;
	}
}

/*
==============================
COM_BinPrintf

==============================
*/
char *COM_BinPrintf( unsigned char *buf, int nLen )
{
	static char szReturn[4096];
	unsigned char c;
	char szChunk[10];
	int i;

	Q_memset(szReturn, 0, sizeof( szReturn ) );

	for (i = 0; i < nLen; i++)
	{
		c = (unsigned char)buf[i];
		Q_snprintf(szChunk, sizeof( szChunk ), "%02x", c);
		Q_strcat(szReturn, szChunk);
	}

	return szReturn;
}

/*
==============================
COM_ExplainDisconnection

==============================
*/
void COM_ExplainDisconnection( qboolean bPrint, char *fmt, ... )
{
	va_list		argptr;
	static char	string[1024];
	
	va_start (argptr, fmt);
	Q_vsnprintf(string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	Q_strncpy( gszDisconnectReason, string, 256 );
	gfExtendedError = true;

	if ( bPrint )
	{
		Con_Printf( "%s\n", gszDisconnectReason );
	}
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = (byte *)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = (byte *)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = (byte *)SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteWord (sizebuf_t *sb, int c)
{
	byte    *buf;
	
	buf = (byte *)SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte    *buf;
	
	buf = (byte *)SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float   f;
		int     l;
	} dat;
	
	
	dat.f = f;
	dat.l = LittleLong (dat.l);
	
	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, Q_strlen(s)+1);
}

void MSG_WriteBuf (sizebuf_t *sb, int iSize, void *buf)
{
	if (!buf)
		return;

	SZ_Write (sb, buf, iSize);
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, ((int)(f*256/360)) & 255);
}

void MSG_WriteHiresAngle (bf_write *sb, float f)
{
	sb->WriteUBitLong( ((int)(f*65536/360)) & 0xFFFF, 16 );
}

// Bit Field I/O routines
static bf_write		bfwrite( "global bfwrite", NULL, 0, -1 );
static bf_read		bfread( "global bfread", NULL, 0, -1 );

// ---------------------------------------------------------------------------------------- //
// bf_write wrappers.
// ---------------------------------------------------------------------------------------- //

void MSG_WriteOneBit( int nValue )
{
	bfwrite.WriteOneBit(nValue);
}

void MSG_StartBitWriting( sizebuf_t *buf )
{
	bfwrite.StartWriting(&buf->data, buf->maxsize, buf->cursize << 3);
}

void MSG_EndBitWriting ( sizebuf_t *buf )
{
	buf->cursize = bfwrite.GetNumBytesWritten();
}

// Append numbits least significant bits from data to the current bit stream
void MSG_WriteBitByte( unsigned char *data, int numbits )
{
	bfwrite.WriteUBitLong(*data, numbits);
}

void MSG_WriteBitShort( unsigned short *data, int numbits )
{
	bfwrite.WriteUBitLong(*data, numbits);
}

void MSG_WriteBitLong( unsigned int *data, int numbits )
{
	bfwrite.WriteUBitLong(*data, numbits);
}

// Sign bit comes first
void MSG_WriteSBitByte( char *data, int numbits )
{
	bfwrite.WriteSBitLong(*data, numbits);
}

void MSG_WriteSBitShort( short *data, int numbits )
{
	bfwrite.WriteSBitLong(*data, numbits);
}

void MSG_WriteSBitLong( int *data, int numbits )
{
	bfwrite.WriteSBitLong(*data, numbits);
}

void MSG_WriteBitAngle( float fAngle, int numbits )
{
	bfwrite.WriteBitAngle(fAngle, numbits);
}

void MSG_WriteBitCoord (const float f)
{
	bfwrite.WriteBitCoord(f);
}

void MSG_WriteBitVec3Coord( const Vector& fa )
{
	bfwrite.WriteBitVec3Coord(fa);
}

void MSG_WriteCoord (sizebuf_t *sb, const float f)
{
	bfwrite.WriteBitCoord(f);
}

void MSG_WriteVec3Coord (sizebuf_t *sb, const Vector& fa)
{
	bfwrite.WriteBitVec3Coord(fa);
}

void MSG_WriteVec3Normal (sizebuf_t *sb, const Vector& fa)
{
	bfwrite.WriteBitVec3Normal(fa);
}




// ---------------------------------------------------------------------------------------- //
// bf_read wrappers.
// ---------------------------------------------------------------------------------------- //

float MSG_ReadBitAngle( int numbits )
{
	return bfread.ReadBitAngle(numbits);
}

// The MSG_ functions just operate on these.
bf_read* MSG_GetReadBuf()
{
	return &bfread;
}

bf_write* MSG_GetWriteBuf()
{
	return &bfwrite;
}

void MSG_StartBitReading( sizebuf_t *buf )
{
	bfread.StartReading(&buf->data, buf->maxsize, buf->cursize << 3);
}

void MSG_EndBitReading ( sizebuf_t *buf )
{
}

int MSG_ReadOneBit( void )
{
	return bfread.ReadOneBit();
}

// Append numbits least significant bits from data to the current bit stream
unsigned char MSG_ReadBitByte( int numbits )
{
	return (unsigned char)bfread.ReadUBitLong(numbits);
}

unsigned char MSG_PeekBitByte( int numbits )
{
	return (unsigned char)bfread.PeekUBitLong(numbits);
}

unsigned short MSG_ReadBitShort( int numbits )
{
	return (unsigned short)bfread.ReadUBitLong(numbits);
}

unsigned int MSG_ReadBitLong( int numbits )
{
	return bfread.ReadUBitLong(numbits);
}

unsigned int MSG_PeekBitLong( int numbits )
{
	return bfread.PeekUBitLong(numbits);
}

// Append numbits least significant bits from data to the current bit stream
char MSG_ReadSBitByte( int numbits )
{
	return (char)bfread.ReadSBitLong(numbits);
}

short MSG_ReadSBitShort( int numbits )
{
	return (short)bfread.ReadSBitLong(numbits);
}

int MSG_ReadSBitLong( int numbits )
{
	return bfread.ReadSBitLong(numbits);
}

// Basic Coordinate Routines (these contain bit-field size AND fixed point scaling constants)

float MSG_ReadBitCoord (void)
{
	return bfread.ReadBitCoord();
}

void MSG_ReadBitVec3Coord( Vector& fa )
{
	bfread.ReadBitVec3Coord(fa);
}

void MSG_ReadBitVec3Normal( Vector& fa )
{
	bfread.ReadBitVec3Normal(fa);
}

void MSG_ReadBitAngles( QAngle& fa )
{
	bfread.ReadBitAngles(fa);
}



//
// reading functions
//
bool MSG_IsOverflowed()
{
	return bfread.IsOverflowed();
}

void MSG_BeginReading (void)
{
	bfread.StartReading(net_message.data, net_message.cursize);
}

int MSG_ReadChar (void)
{
	return bfread.ReadChar();
}

int MSG_ReadByte (void)
{
	return bfread.ReadByte();
}

int MSG_ReadShort (void)
{
	return bfread.ReadShort();
}

int MSG_ReadWord (void)
{
	return bfread.ReadWord();
}


int MSG_ReadLong (void)
{
	return bfread.ReadLong();
}

float MSG_ReadFloat (void)
{
	return bfread.ReadFloat();
}

int MSG_ReadBuf(int iSize, void *pbuf)
{
	return bfread.ReadBytes(pbuf, iSize) ? 1 : -1;
}

char *MSG_ReadString (void)
{
	bfread.ReadString(g_TempStringBuf, sizeof(g_TempStringBuf), false);
	return g_TempStringBuf;
}

char *MSG_ReadStringLine (void)
{
	bfread.ReadString(g_TempStringBuf, sizeof(g_TempStringBuf), true);
	return g_TempStringBuf;
}

float MSG_ReadAngle (void)
{
	return bfread.ReadBitAngle(8);
}

float MSG_ReadHiresAngle (void)
{
	return bfread.ReadBitAngle(16);
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void    *data;
	
	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
		{
			if (!buf->maxsize)
				Sys_Error ("SZ_GetSpace:  Tried to write to an uninitialized sizebuf_t");
			else
				Sys_Error ("SZ_GetSpace: overflow without allowoverflow set");
		}
		else
		{
			Con_Printf( "overflowing, ok\n" );
		}
		
		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);
			
		Con_Printf ("SZ_GetSpace: overflow\n");
		SZ_Clear (buf); 
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);         
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int             len;
	
	len = Q_strlen(data)+1;

// byte * cast to keep VC++ happy
	if (buf->data[buf->cursize-1])
		Q_memcpy ((byte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
	else
		Q_memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,data,len); // write over trailing 0
}


//============================================================================


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char    *last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, int outSize )
{
	// Find the last dot.
	const char *pLastExt = in;
	while( 1 )
	{
		const char *pTest = strchr( pLastExt, '.' );
		if ( !pTest )
			break;

		pLastExt = pTest + 1;
	}

	// Copy up to the last dot.
	if ( pLastExt > in )
	{
		int nChars = pLastExt - in - 1;
		nChars = min( nChars, outSize-1 );
		memcpy( out, in, nChars );
		out[nChars] = 0;
	}
	else
	{
		Q_strncpy( out, in, outSize );
	}
}


/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (char *in)
{
	static char exten[8];
	int             i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i=0 ; i<7 && *in ; i++,in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase (const char *in, char *out)
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( start < 0 || ( in[start] != '/' && in[start] != '\\' ) )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	Q_strncpy( out, &in[start], len+1 );
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, const char *extension, int pathStringLength )
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	Q_strncat( path, extension, pathStringLength );
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	characterset_t	*breaks;
	
	breaks = &g_BreakSetIncludingColons;
	if ( com_ignorecolons )
		breaks = &g_BreakSet;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if ( IN_CHARACTERSET( *breaks, c ) )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if ( IN_CHARACTERSET( *breaks, c ) )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_Parse_Line

Parse a line out of a string
==============
*/
char *COM_ParseLine (char *data)
{
	int c;
	int len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;

	c = *data;

// parse a line out of the data
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while ( c>=' ' && ( len < COM_TOKEN_MAX_LENGTH - 1 ) );
	
	com_token[len] = 0;

	if (c==0) // end of file
		return NULL;

// eat whitespace (LF,CR,etc.) at the end of this line
	while ( (c = *data) < ' ' )
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}

	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char    *va(char *format, ...)
{
	va_list         argptr;
	static char             string[4][1024];
	static int curstring = 0;
	
	curstring = ( curstring + 1 ) % 4;

	va_start (argptr, format);
#ifdef _WIN32
	Q_vsnprintf( string[curstring], sizeof( string[curstring] ), format, argptr );
#else
	vsprintf (string[curstring], format,argptr);
#endif
	va_end (argptr);

	return string[curstring];  
}

/*
============
vstr

prints a vector into a temporary string
bufffer.
============
*/
char    *vstr(Vector& v)
{
	static int idx = 0;
	static char string[16][1024];

	idx++;
	idx &= 15;

	Q_snprintf(string[idx], sizeof( string[idx] ), "%.2f %.2f %.2f", v[0], v[1], v[2]);
	return string[idx];  
}

int     com_filesize;
char    com_gamedir[MAX_OSPATH];
char	com_defaultgamedir[MAX_OSPATH];

//-----------------------------------------------------------------------------
// Purpose: Finds the file in the search path.
//  Sets com_filesize and file handle
// Input  : *filename - 
//			*file - 
// Output : int
//-----------------------------------------------------------------------------
int COM_FindFile( const char *filename, FileHandle_t *file )
{
	assert( file );

	*file = g_pFileSystem->Open( filename, "rb" );
	if ( *file )
	{
		com_filesize = g_pFileSystem->Size( *file );
	}
	else
	{
		com_filesize = -1;
	}
	return com_filesize;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//			*file - 
// Output : int
//-----------------------------------------------------------------------------
int COM_OpenFile( const char *filename, FileHandle_t *file )
{
	return COM_FindFile( (char *)filename, file );
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile (char *filename, void *data, int len)
{
	FileHandle_t  handle;
	
	int nameLen = strlen( filename ) + 2;
	char *pName = ( char * )_alloca( nameLen );

	Q_snprintf( pName, nameLen, "%s", filename);

	COM_FixSlashes( pName );
	COM_CreatePath( pName );

	handle = g_pFileSystem->Open( pName, "wb" );
	if ( !handle )
	{
		Sys_Printf ("COM_WriteFile: failed on %s\n", pName);
		return;
	}
	
	g_pFileSystem->Write( data, len, handle );
	g_pFileSystem->Close( handle );
}

/*
============
COM_FixSlashes

Changes all '/' characters into '\' characters, in place.
============
*/
void COM_FixSlashes( char *pname )
{
#ifdef _WIN32
	while ( *pname ) {
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}
#else
	while ( *pname ) {
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
#endif
}

/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void    COM_CreatePath (char *path)
{
	char    *ofs;
	char old;
	
	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{       // create the directory
			old = *ofs;
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = old;
		}
	}
}


/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed.  This is for the convenience of developers using ISDN from home.
===========
*/
void COM_CopyFile (char *netpath, char *cachepath)
{
	int             remaining, count;
	char			buf[4096];
	FileHandle_t in, out;

	in = g_pFileSystem->Open( netpath, "rb" );
	assert( in );
	
	// create directories up to the cache file
	COM_CreatePath (cachepath);     

	out = g_pFileSystem->Open( cachepath, "wb" );
	assert( out );
	
	remaining = g_pFileSystem->Size( in );
	while ( remaining > 0 )
	{
		if (remaining < sizeof(buf))
		{
			count = remaining;
		}
		else
		{
			count = sizeof(buf);
		}
		g_pFileSystem->Read( buf, count, in );
		g_pFileSystem->Write( buf, count, out );
		remaining -= count;
	}

	g_pFileSystem->Close( in );
	g_pFileSystem->Close( out );    
}


/*
===========
COM_ExpandFilename

Finds the file in the search path, copies over the name with the full path name.
This doesn't search in the pak file.
===========
*/
int COM_ExpandFilename( char *filename )
{
	char expanded[MAX_OSPATH];

	if ( g_pFileSystem->GetLocalPath( filename, expanded ) != NULL )
	{
		strcpy( filename, expanded );
		return 1;
	}

	if ( filename && filename[0] != '*' )
	{
		Sys_Printf ("COM_ExpandFilename: can't find %s\n", filename);
	}
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: If the requested file is inside a packfile, a new FileHandle_t will be opened
//  into the file.
// Input  : *filename - 
//			*file - 
// Output : int 
//-----------------------------------------------------------------------------
int COM_FOpenFile (const char *filename, FileHandle_t *file)
{
	return COM_FindFile( (char *)filename, file );
}


/*
===========
COM_FileSize

Returns the size of the file only.
===========
*/
int COM_FileSize (const char *filename)
{
#if 1
	return g_pFileSystem->Size(filename);
#endif

#if 0
	FileHandle_t fp = NULL;
	int iSize;

	iSize = COM_FindFile ( filename, &fp );
	if (fp)
	{
		g_pFileSystem->Close( fp );
	}

	return iSize;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Close file handle
// Input  : hFile - 
//-----------------------------------------------------------------------------
void COM_CloseFile( FileHandle_t hFile )
{
	g_pFileSystem->Close( hFile );
}
 

/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/
cache_user_t *loadcache;
byte    *loadbuf;
int             loadsize;
byte *COM_LoadFile (const char *path, int usehunk, int *pLength)
{
	FileHandle_t	hFile;
	byte			*buf = NULL;
	char			base[128];
	int             len;

	if (pLength)
	{
		*pLength = 0;
	}

// look for it in the filesystem or pack files
	len = COM_OpenFile( path, &hFile );
	if ( !hFile )
	{
		return NULL;
	}

	// Extract the filename base name for hunk tag
	COM_FileBase( path, base );
	
	switch ( usehunk )
	{
	case 1:
		buf = (byte *)Hunk_AllocName (len+1, base);
		break;
	case 2:
		buf = (byte *)Hunk_TempAlloc (len+1);
		break;
	case 3:
		buf = (byte *)Cache_Alloc (loadcache, len+1, base);
		break;
	case 4:
		{
			if (len+1 > loadsize)
				buf = (byte *)Hunk_TempAlloc (len+1);
			else
				buf = loadbuf;
		}
		break;
	case 5:
		buf = (byte *)malloc(len + 1);  // YWB:  FIXME, this is evil.
		break;
	default:
		Sys_Error ("COM_LoadFile: bad usehunk");
	}

	if ( !buf ) 
	{
		Sys_Error ("COM_LoadFile: not enough space for %s", path);
		COM_CloseFile(hFile);	// exit here to prevent fault on oom (kdb)
		return NULL;			
	}
		
	g_pFileSystem->Read( buf, len, hFile );
	COM_CloseFile( hFile );

	((byte *)buf)[ len ] = 0;

	if ( pLength )
	{
		*pLength = len;
	}
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool COM_DoesFileExist(const char *path)
{
	FileHandle_t hFile;

	if ( COM_OpenFile( path, &hFile ) == -1 )
	{
		return false;
	}

	COM_CloseFile(hFile);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buffer - 
// Output : void COM_FreeFile
//-----------------------------------------------------------------------------
void COM_FreeFile (byte *buffer)
{
	free( buffer );
}

/*
===============
COM_CopyFileChunk

===============
*/
void COM_CopyFileChunk( FileHandle_t dst, FileHandle_t src, int nSize )
{
	int   copysize = nSize;
	char  copybuf[COM_COPY_CHUNK_SIZE];

	while (copysize > COM_COPY_CHUNK_SIZE)
	{
		g_pFileSystem->Read ( copybuf, COM_COPY_CHUNK_SIZE, src );
		g_pFileSystem->Write( copybuf, COM_COPY_CHUNK_SIZE, dst );
		copysize -= COM_COPY_CHUNK_SIZE;
	}

	g_pFileSystem->Read ( copybuf, copysize, src );
	g_pFileSystem->Write( copybuf, copysize, dst );

	g_pFileSystem->Flush ( src );
	g_pFileSystem->Flush ( dst );
}

byte *COM_LoadHunkFile (const char *path)
{
	return COM_LoadFile (path, 1, NULL);
}

byte *COM_LoadTempFile (const char *path, int *pLength)
{
	return COM_LoadFile (path, 2, pLength);
}

void COM_LoadCacheFile (const char *path, cache_user_t *cu )
{
	loadcache = cu;
	COM_LoadFile (path, 3, NULL);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize)
{
	byte    *buf;
	
	loadbuf = (byte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, 4, NULL);
	
	return buf;
}

void COM_ShutdownFileSystem( void )
{
}

/*
================
COM_Shutdown

Remove the searchpaths
================
*/
void COM_Shutdown( void )
{
}

/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ... 
================
*/
void COM_AddGameDirectory ( const char *pszBaseDir, const char *pszDir )
{
	char dir[ MAX_OSPATH ];
	char platdir [MAX_OSPATH];

	Q_snprintf( dir, sizeof( dir ), "%s/%s", pszBaseDir, pszDir );
	strcpy ( com_gamedir, dir );

	g_pFileSystem->AddSearchPath( dir, "GAME", PATH_ADD_TO_HEAD );

	// It's OK for this to be NULL on the dedicated server.
	if( g_pMaterialSystemHardwareConfig )
	{
		int dxLevel = g_pMaterialSystemHardwareConfig->GetDXSupportLevel();
		if( dxLevel >= 60 && dxLevel < 70 )
		{
			// DX6
			Q_snprintf( platdir, sizeof( platdir ), "%s/platform_dx7", dir ); 
			g_pFileSystem->AddSearchPath( platdir, "GAME", PATH_ADD_TO_HEAD );
			Q_snprintf( platdir, sizeof( platdir ), "%s/platform_dx6", dir ); 
			g_pFileSystem->AddSearchPath( platdir, "GAME", PATH_ADD_TO_HEAD );
		}
		else if( dxLevel >= 70 && dxLevel < 80 )
		{
			// DX7
			Q_snprintf( platdir, sizeof( platdir ), "%s/platform_dx7", dir ); 
			g_pFileSystem->AddSearchPath( platdir, "GAME", PATH_ADD_TO_HEAD );
		}
		else if( dxLevel >= 80 && dxLevel < 90 )
		{
			// DX8

		}
		// DX9 and higher is assumed to be DX9 compatable:
		else if( dxLevel >= 90 )
		{
			// DX9
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Actually re-inits the fs
// Input  : *pszGameDir - 
//-----------------------------------------------------------------------------
void COM_ChangeGameDir( const char *pszGameDir )
{
	g_pFileSystem->RemoveSearchPath( NULL, "GAME" );
	g_pFileSystem->RemoveSearchPath( NULL, "PLATFORM" );

	// start up with the default game directory
	COM_AddGameDirectory ( host_parms.basedir, com_defaultgamedir );

	// Adds basedir/gamedir as an override game, only if it's different from the default, of course
	if ( stricmp( pszGameDir, com_defaultgamedir ) )
	{
		COM_AddGameDirectory ( host_parms.basedir, pszGameDir );
	}

	g_pFileSystem->AddSearchPath( pszGameDir, "PLATFORM" );

	// Set LOGDIR to be something reasonable
	COM_SetupLogDir( NULL );
}

void COM_SetupLogDir( const char *mapname )
{
	char gameDir[MAX_OSPATH];
	COM_GetGameDir( gameDir );

	// Blat out the all directories in the LOGDIR path
	g_pFileSystem->RemoveSearchPath( NULL, "LOGDIR" );

	// set the log directory
	if ( mapname && CommandLine()->FindParm("-uselogdir") )
	{
		int i;
		char sRelativeLogDir[MAX_PATH];
		for ( i = 0; i < MAX_LOG_DIRECTORIES; i++ )
		{
			Q_snprintf( sRelativeLogDir, sizeof( sRelativeLogDir ), "logs/%s/%04i", mapname, i );
			if ( !g_pFileSystem->IsDirectory( sRelativeLogDir, "GAME" ) )
				break;
		}

		// Loop at max
		if ( i == MAX_LOG_DIRECTORIES )
		{
			i = 0;	
			Q_snprintf( sRelativeLogDir, sizeof( sRelativeLogDir ), "logs/%s/%04i", mapname, i );
		}

		// Make sure the directories we need exist.
		g_pFileSystem->CreateDirHierarchy( sRelativeLogDir, "GAME" );	

		{
			static bool pathsetup = false;

			if ( !pathsetup )
			{
				pathsetup = true;

				// Set the search path
				char sLogDir[MAX_PATH];
				Q_snprintf( sLogDir, sizeof( sLogDir ), "%s/%s", gameDir, sRelativeLogDir );
				g_pFileSystem->AddSearchPath( sLogDir, "LOGDIR" );
			}
		}
	}
	else
	{
		// Default to the base game directory for logs.
		g_pFileSystem->AddSearchPath( gameDir, "LOGDIR" );
	}
}


void COM_StripTrailingSlash( char *ppath )
{
	int len;

	len = strlen( ppath );

	if ( len > 0 )
	{
		if ( (ppath[len-1] == '\\') || (ppath[len-1] == '/'))
			ppath[len-1] = 0;
	}
}


void COM_ParseDirectoryFromCmd( const char *pCmdName, char *pDirName, const char *pDefault )
{
	const char *pParameter = CommandLine()->ParmValue( pCmdName );

	// Found a valid parameter on the cmd line?
	if ( pParameter )
	{
		// Grab it
		strcpy( pDirName, pParameter );
	}
	else if ( pDefault )
	{
		// Ok, then use the default
		strcpy (pDirName, pDefault);
	}
	else
	{
		// If no default either, then just terminate the string
		pDirName[0] = 0;
	}

	COM_StripTrailingSlash( pDirName );
}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem (void)
{
	// Get the default game dir
	COM_ParseDirectoryFromCmd( "-defaultgamedir", com_defaultgamedir, "hl2" );

	// start up with the default game directory
	COM_AddGameDirectory ( host_parms.basedir, com_defaultgamedir );
	
	// -game <gamedir>
	// Adds basedir/gamedir as an override game, only if it's different from the default, of course
	const char *pModDir;
	const char *pGameDir = CommandLine()->ParmValue("-game", com_defaultgamedir );
	if ( stricmp( pGameDir, com_defaultgamedir ) )
	{
		COM_AddGameDirectory ( host_parms.basedir, pGameDir );
		pModDir = pGameDir;
	}
	else
	{
		pModDir = com_defaultgamedir;
	}

	// Some things (like searching for cfg/config.cfg) only look in the mod directory.
	char dir[ MAX_OSPATH ];
	Q_snprintf( dir, sizeof( dir ), "%s/%s", host_parms.basedir, pModDir );
	g_pFileSystem->AddSearchPath( dir, "MOD", PATH_ADD_TO_TAIL );

	// Set LOGDIR to be something reasonable
	COM_SetupLogDir( NULL );
}

void COM_Log( char *pszFile, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	FileHandle_t fp;
	char *pfilename;
	
	if ( !pszFile )
	{
		pfilename = "hllog.txt";
	}
	else
	{
		pfilename = pszFile;
	}
	va_start (argptr,fmt);
	Q_vsnprintf(string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	fp = g_pFileSystem->Open( pfilename, "a+t");
	if (fp)
	{
		g_pFileSystem->FPrintf(fp, "%s", string);
		g_pFileSystem->Close(fp);
	}
}

byte *COM_LoadFileForMe(const char *filename, int *pLength)
{
	return COM_LoadFile(filename, 5, pLength);
}

int COM_CompareFileTime(char *filename1, char *filename2, int *iCompare)
{
	int bRet = 0;
	*iCompare = 0;

	if (filename1 && filename2)
	{
		long ft1 = g_pFileSystem->GetFileTime( filename1 );
		long ft2 = g_pFileSystem->GetFileTime( filename2 );

		*iCompare = Sys_CompareFileTime( ft1,  ft2 );
		bRet = 1;
	}

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szGameDir - 
//-----------------------------------------------------------------------------
void COM_GetGameDir(char *szGameDir)
{
	if (!szGameDir) return;
	strcpy(szGameDir, com_gamedir);
}

//-----------------------------------------------------------------------------
// Purpose: Parse a token from a file stream
// Input  : *data - 
//			*token - 
// Output : char
//-----------------------------------------------------------------------------
char *COM_ParseFile(char *data, char *token)
{
	char *return_data = COM_Parse(data);
	strcpy(token, com_token);
	return return_data;	
}


///////////////////////////////////////////////////////
/*
=================
COM_CheckAuthenticationType()

  Determine whether this server is a private/LAN only server, or whether it is a valid
  Internet server.
=================
*/
void COM_CheckAuthenticationType()
{
	gfUseLANAuthentication = true;
}

/*
================
COM_EntsForPlayerSlots

Returns the appropriate size of the edicts array to allocate
based on the stated # of max players
================
*/
int COM_EntsForPlayerSlots(int nPlayers)
{
	// FIXME: due to all the weapons, we may run out...
	return MAX_EDICTS;
}

/*
===============
COM_GetShortName

Returns the player's nickname, in a directory safe 8 character max string.
===============
*/
char *COM_GetShortName(char *name)
{
	static char szReturnName[9];
	char *pDst, *pSrc;
	int   i;

	memset(szReturnName, 0, 9);

	if (strlen(name) < 1)
		return "Noname";
	
	pDst = szReturnName;
	pSrc = name;

	i = 0;  // Up to 8 characters.
	while ( *pSrc &&
		   ( i < 8 ) )
	{
		if (isalpha(*pSrc))
		{
			*pDst++ = *pSrc;
			i++;
		}

		pSrc++;
	}
	*pDst = 0;

	if (strlen(szReturnName) < 1)
		return "Noname";

	return szReturnName;

}

#define BLOCK_SIZE 1024   // For copying operations
/*
===============
CL_CopyChunk
===============
*/
void CL_CopyChunk( FileHandle_t dst, FileHandle_t src, int nSize )
{
	int   copysize = nSize;
	char  copybuf[BLOCK_SIZE];
	while (copysize > BLOCK_SIZE)
	{
		g_pFileSystem->Read ( copybuf, BLOCK_SIZE, src );
		g_pFileSystem->Write( copybuf, BLOCK_SIZE, dst );
		copysize -= BLOCK_SIZE;
	}
	g_pFileSystem->Read ( copybuf, copysize, src );
	g_pFileSystem->Write( copybuf, copysize, dst );
	g_pFileSystem->Flush ( src );
	g_pFileSystem->Flush ( dst );
}


void DebugOutStraight(const char *pStr)
{
#ifdef _WIN32
	OutputDebugString(pStr);
#elif _LINUX
	fprintf(stderr, "%s\n", pStr);
#endif
}


void DebugOut(const char *pStr, ...)
{
	char str[4096];
	va_list marker;

	va_start(marker, pStr);
	Q_vsnprintf(str, sizeof( str ), pStr, marker);
	va_end(marker);

#ifdef _WIN32
	OutputDebugString(str);
#elif _LINUX
	fprintf(stderr, "%s\n", pStr);
#endif
}

static unsigned char mungify_table[ 16 ] = 
{
	122, 100, 5, 241,
	27, 155, 160, 181,
	202, 237, 97, 13,
	74, 223, 142, 199,
};

/*
================
COM_Munge

Anti-proxy/aimbot obfuscation code

  COM_UnMunge should reversably fixup the data
================
*/
void COM_Munge( unsigned char *data, int len, int seq )
{
	int i;
	int mungelen;

	int c;
	int *pc;
	unsigned char *p;

	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for ( i = 0; i < mungelen; i++ )
	{
		pc = (int *)&data[ i * 4 ];
		c = *pc;

		c ^= ~seq;

		c = DWordSwap( c );
		
		p = ( unsigned char *)&c;
		for ( j = 0 ; j < 4; j++ )
		{
			*p++ ^= ( 0xa5 | ( j << j) | j | mungify_table[ ( i + j ) & 0x0f ] );
		}

		c ^= seq;
		
		*pc = c;
	}
}

/*
================
COM_UnMunge

Anti-proxy/aimbot obfuscation code
================
*/
void COM_UnMunge( unsigned char *data, int len, int seq )
{
	int i;
	int mungelen;

	int c;
	int *pc;
	unsigned char *p;

	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for ( i = 0; i < mungelen; i++ )
	{
		pc = (int *)&data[ i * 4 ];
		c = *pc;

		c ^= seq;

		p = ( unsigned char *)&c;
		for ( j = 0 ; j < 4; j++ )
		{
			*p++ ^= ( 0xa5 | ( j << j) | j | mungify_table[ ( i + j ) & 0x0f ] );
		}

		c = DWordSwap( c );

		c ^= ~seq;
		
		*pc = c;
	}
}

/*
================
COM_Init
================
*/

void COM_Init ( void )
{
	CharacterSetBuild( &g_BreakSet, "{}()'" );
	CharacterSetBuild( &g_BreakSetIncludingColons, "{}()':" );
}

