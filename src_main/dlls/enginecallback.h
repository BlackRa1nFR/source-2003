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
#ifndef ENGINECALLBACK_H
#define ENGINECALLBACK_H

#ifndef EIFACE_H
#include "eiface.h"
#endif

#include "gameinterface.h"

// Bytewise
#define WRITE_BYTE		(MessageWriteByte)
#define WRITE_CHAR		(MessageWriteChar)
#define WRITE_SHORT		(MessageWriteShort)
#define WRITE_WORD		(MessageWriteWord)
#define WRITE_LONG		(MessageWriteLong)
#define WRITE_FLOAT		(MessageWriteFloat)
#define WRITE_ANGLE		(MessageWriteAngle)
#define WRITE_COORD		(MessageWriteCoord)
#define WRITE_VEC3COORD	(MessageWriteVec3Coord)
#define WRITE_VEC3NORMAL (MessageWriteVec3Normal)
#define WRITE_ANGLES	(MessageWriteAngles)
#define WRITE_STRING	(MessageWriteString)
#define WRITE_ENTITY	(MessageWriteEntity)

// Bitwise
#define WRITE_BOOL		(MessageWriteBool)
#define WRITE_UBITLONG	(MessageWriteUBitLong)
#define WRITE_SBITLONG	(MessageWriteSBitLong)
#define WRITE_BITS		(MessageWriteBits)

#endif		//ENGINECALLBACK_H
