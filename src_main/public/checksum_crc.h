//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CHECKSUM_CRC_H
#define CHECKSUM_CRC_H
#ifdef _WIN32
#pragma once
#endif

typedef unsigned long CRC32_t;

#define NUM_BYTES 256
extern const CRC32_t pulCRCTable[ NUM_BYTES ];

void CRC32_Init( CRC32_t *pulCRC );
void CRC32_ProcessBuffer( CRC32_t *pulCRC, void *p, int len );
void CRC32_Final( CRC32_t *pulCRC );

#endif // CHECKSUM_CRC_H
