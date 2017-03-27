/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  parsemsg.cpp
//
#include "cbase.h"
#include "bitbuf.h"

static bf_read gBuf;

void BEGIN_READ( void *buf, int size )
{
	gBuf.StartReading( buf, size );
}


int READ_CHAR( void )
{
	return gBuf.ReadChar();
}

int READ_BYTE( void )
{
	return gBuf.ReadByte();
}

int READ_SHORT( void )
{
	return gBuf.ReadShort();
}

int READ_WORD( void )
{
	return gBuf.ReadWord();
}


int READ_LONG( void )
{
	return gBuf.ReadLong();
}

float READ_FLOAT( void )
{
	return gBuf.ReadFloat();
}

char* READ_STRING( void )
{
	static char     string[2048];
	gBuf.ReadString( string, 2048);
	return string;
}

float READ_COORD( void )
{
	return gBuf.ReadBitCoord();
}

float READ_ANGLE( void )
{
	return (float)(READ_CHAR() * (360.0/256));
}

float READ_HIRESANGLE( void )
{
	return (float)(READ_SHORT() * (360.0/65536));
}

void READ_VEC3COORD( Vector& vec )
{
	gBuf.ReadBitVec3Coord( vec );
}

void READ_VEC3NORMAL( Vector& vec )
{
	gBuf.ReadBitVec3Normal( vec );
}

void READ_ANGLES( QAngle &angles )
{
	gBuf.ReadBitAngles( angles );	
}

// bitwise
bool READ_BOOL( void )
{
	return gBuf.ReadOneBit() ? true : false;
}

unsigned int READ_UBITLONG( int numbits )
{
	return gBuf.ReadUBitLong( numbits );
}

int READ_SBITLONG( int numbits )
{
	return gBuf.ReadBitLong( numbits, true );
}

void READ_BITS( void *buffer, int nbits )
{
	gBuf.ReadBits( buffer, nbits );
}

