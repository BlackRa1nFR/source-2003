//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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

#ifndef VOX_H
#define VOX_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct sfxcache_t;
struct channel_t;

extern void				VOX_Init( void );
extern void 			VOX_Shutdown( void );
extern void				VOX_ReadSentenceFile( const char *psentenceFileName );
extern int				VOX_SentenceCount( void );
extern void				VOX_LoadSound( channel_t *pchan, const char *psz );
// UNDONE: Improve the interface of this call, it returns sentence data AND the sentence index
extern char				*VOX_LookupString( const char *pSentenceName, int *psentencenum );
extern const char		*VOX_SentenceNameFromIndex( int sentencenum );
extern float			VOX_SentenceLength( int sentence_num );
extern const char		*VOX_GroupNameFromIndex( int groupIndex );
extern int				VOX_GroupIndexFromName( const char *pGroupName );
extern int				VOX_GroupPick( int isentenceg, char *szfound, int strLen );
extern int				VOX_GroupPickSequential( int isentenceg, char *szfound, int szfoundLen, int ipick, int freset );

#ifdef __cplusplus
}
#endif

#endif // VOX_H
