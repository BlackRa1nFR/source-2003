//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_MIX_BUF_H
#define SND_MIX_BUF_H

#if defined( _WIN32 )
#pragma once
#endif

#define	PAINTBUFFER_SIZE		1024	// 44k: was 512
#define PAINTBUFFER				(g_curpaintbuffer)
#define REARPAINTBUFFER			(g_currearpaintbuffer)
#define CPAINTBUFFERS			5

// !!! if this is changed, it much be changed in asm_i386.h too !!!
struct portable_samplepair_t
{
	int left;
	int right;
};

// sound mixing buffer

#define CPAINTFILTERMEM			3
#define CPAINTFILTERS			4			// maximum number of consecutive upsample passes per paintbuffer

struct paintbuffer_t
{
	bool factive;							// if true, mix to this paintbuffer using flags
	bool fsurround;							// if true, mix to front and rear paintbuffers using flags
	int flags;								// SOUND_BUSS_ROOM, SOUND_BUSS_FACING, SOUND_BUSS_FACINGAWAY
	
	portable_samplepair_t *pbuf;			// front stereo mix buffer, for 2 or 4 channel mixing
	portable_samplepair_t *pbufrear;		// rear mix buffer, for 4 channel mixing

	int ifilter;							// current filter memory buffer to use for upsampling pass

	portable_samplepair_t fltmem[CPAINTFILTERS][CPAINTFILTERMEM];		// filter memory, for upsampling with linear or cubic interpolation
	portable_samplepair_t fltmemrear[CPAINTFILTERS][CPAINTFILTERMEM];	// filter memory, for upsampling with linear or cubic interpolation

};

extern "C"
{

extern portable_samplepair_t paintbuffer[];
extern portable_samplepair_t roombuffer[];
extern portable_samplepair_t facingbuffer[];
extern portable_samplepair_t facingawaybuffer[];
extern portable_samplepair_t drybuffer[];

extern portable_samplepair_t rearpaintbuffer[];
extern portable_samplepair_t rearroombuffer[];
extern portable_samplepair_t rearfacingbuffer[];
extern portable_samplepair_t rearfacingaway[];
extern portable_samplepair_t reardrybuffer[];

// temp paintbuffer - not included in main list of paintbuffers

extern portable_samplepair_t temppaintbuffer[];
	
extern paintbuffer_t paintbuffers[];

extern inline void MIX_SetCurrentPaintbuffer( int ipaintbuffer );
extern inline int MIX_GetCurrentPaintbufferIndex( void );
extern int MIX_GetCurrentPaintbufferIndex( void );
extern paintbuffer_t *MIX_GetCurrentPaintbufferPtr( void );
extern void MIX_ClearAllPaintBuffers( int SampleCount, bool clearFilters );
extern bool MIX_InitAllPaintbuffers(void);
extern void MIX_FreeAllPaintbuffers(void);
	
extern portable_samplepair_t *g_curpaintbuffer;
extern portable_samplepair_t *g_currearpaintbuffer;


};


#define CLIP(x) ((x) > 32767 ? 32767 : ((x) < -32767 ? -32767 : (x)))

#endif // SND_MIX_BUF_H
